/*
 * Copyright (c) 2016 TAKAMORI Kaede <etheriqa@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world_hash.h"
#include "world_hashtable_entry.h"
#include "world_origin.h"
#include "world_origin_thread.h"
#include "world_system.h"

static bool _validate_conf(const struct world_originconf *conf);
static struct world_origin_thread *_thread(struct world_origin *origin, int fd);
static world_sequence _least_sequence(struct world_origin *origin);
static void _notify(struct world_origin *origin);
static void _checkpoint(struct world_origin *origin);

enum world_error world_origin_open(struct world_origin **o, const struct world_originconf *conf)
{
  if (!_validate_conf(conf)) {
    return world_error_invalid_argument;
  }

  struct world_allocator allocator;
  world_allocator_init(&allocator);
  struct world_origin *origin = world_allocator_malloc(&allocator, sizeof(*origin));
  memcpy(&origin->allocator, &allocator, sizeof(allocator));

  memcpy((void *)&origin->conf, conf, sizeof(origin->conf));
  world_hashtable_init(&origin->hashtable, world_generate_seed(), &origin->allocator);
  world_circular_init(&origin->garbages, &origin->allocator);

  origin->threads = world_allocator_calloc(&origin->allocator, origin->conf.n_io_threads, sizeof(*origin->threads));
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_thread_init(&origin->threads[i], origin);
  }

  *o = origin;
  return world_error_ok;
}

enum world_error world_origin_close(struct world_origin *origin)
{
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_thread_destroy(&origin->threads[i]);
  }

  world_hashtable_destroy(&origin->hashtable);
  world_circular_destroy(&origin->garbages);
  world_allocator_free(&origin->allocator, origin->threads);

  struct world_allocator allocator;
  memcpy(&allocator, &origin->allocator, sizeof(allocator));
  world_allocator_free(&allocator, origin);
  world_allocator_destroy(&allocator);

  return world_error_ok;
}

enum world_error world_origin_attach(struct world_origin *origin, int fd)
{
  if (!world_check_fd(fd)) {
    return world_error_invalid_argument;
  }

  if (origin->conf.set_nonblocking && !world_set_nonblocking(fd)) {
    return world_error_system;
  }

  if (origin->conf.set_tcp_nodelay && !world_set_tcp_nodelay(fd)) {
    return world_error_system;
  }

  world_origin_thread_attach(_thread(origin, fd), fd);

  return world_error_ok;
}

enum world_error world_origin_detach(struct world_origin *origin, int fd)
{
  if (!world_check_fd(fd)) {
    return world_error_invalid_argument;
  }

  world_origin_thread_detach(_thread(origin, fd), fd);

  return world_error_ok;
}

enum world_error world_origin_transmit(struct world_origin *origin)
{
  if (!origin->conf.auto_transmission) {
    _notify(origin);
    _checkpoint(origin);
  }

  return world_error_ok;
}

enum world_error world_origin_get(const struct world_origin *origin, struct world_buffer key, struct world_buffer *data)
{
  return world_hashtable_get((struct world_hashtable *)&origin->hashtable, key, data);
}

enum world_error world_origin_set(struct world_origin *origin, struct world_buffer key, struct world_buffer data)
{
  enum world_error err = world_hashtable_set(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  if (origin->conf.auto_transmission) {
    _notify(origin);
    _checkpoint(origin);
  }

  return world_error_ok;
}

enum world_error world_origin_add(struct world_origin *origin, struct world_buffer key, struct world_buffer data)
{
  enum world_error err = world_hashtable_add(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  if (origin->conf.auto_transmission) {
    _notify(origin);
    _checkpoint(origin);
  }

  return world_error_ok;
}

enum world_error world_origin_replace(struct world_origin *origin, struct world_buffer key, struct world_buffer data)
{
  enum world_error err = world_hashtable_replace(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  if (origin->conf.auto_transmission) {
    _notify(origin);
    _checkpoint(origin);
  }

  return world_error_ok;
}

enum world_error world_origin_delete(struct world_origin *origin, struct world_buffer key)
{
  enum world_error err = world_hashtable_delete(&origin->hashtable, key);
  if (err) {
    return err;
  }

  if (origin->conf.auto_transmission) {
    _notify(origin);
    _checkpoint(origin);
  }

  return world_error_ok;
}

static bool _validate_conf(const struct world_originconf *conf)
{
  if (conf->n_io_threads == 0) {
    fprintf(stderr, "world_origin_open: n_io_threads should be positive integer");
    return false;
  }

  return true;
}

static struct world_origin_thread *_thread(struct world_origin *origin, int fd)
{
  return &origin->threads[fd % origin->conf.n_io_threads];
}

static world_sequence _least_sequence(struct world_origin *origin)
{
  world_sequence seq = world_hashtable_log(&origin->hashtable)->base.seq;
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_sequence seq_thread = world_origin_thread_least_sequence(origin->threads + i);
    if (seq > seq_thread) {
      seq = seq_thread;
    }
  }
  return seq;
}

static void _notify(struct world_origin *origin)
{
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_thread_interrupt(origin->threads + i);
  }
}

static void _checkpoint(struct world_origin *origin)
{
  // FIXME ad hoc implementation. there is a bit of a chance of race condition
  while (world_circular_size(&origin->garbages) > 10000) {
    world_hashtable_entry_delete(*(void **)world_circular_front(&origin->garbages, sizeof(void *)), &origin->allocator);
    world_circular_pop_front(&origin->garbages);
  }

  world_hashtable_checkpoint(&origin->hashtable, _least_sequence(origin), &origin->garbages);
}
