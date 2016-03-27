/**
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
#include "world_origin_iothread.h"
#include "world_system.h"

static struct world_origin_iothread *_iothread(struct world_origin *origin, int fd);
static world_sequence _least_sequence(struct world_origin *origin);
static void _checkpoint(struct world_origin *origin);

enum world_error world_origin_open(struct world_origin **o, const struct world_originconf *conf)
{
  // TODO validate conf here

  if (conf->ignore_sigpipe && !world_ignore_sigpipe()) {
    return world_error_system;
  }

  struct world_allocator allocator;
  world_allocator_init(&allocator);
  struct world_origin *origin = world_allocator_malloc(&allocator, sizeof(*origin));
  memcpy(&origin->allocator, &allocator, sizeof(allocator));

  memcpy((void *)&origin->conf, conf, sizeof(origin->conf));
  world_hashtable_init(&origin->hashtable, world_generate_seed(), &origin->allocator);

  origin->threads = world_allocator_calloc(&origin->allocator, origin->conf.n_io_threads, sizeof(*origin->threads));
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_iothread_init(&origin->threads[i], origin);
  }

  *o = origin;
  return world_error_ok;
}

enum world_error world_origin_close(struct world_origin *origin)
{
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_iothread_destroy(&origin->threads[i]);
  }

  world_hashtable_destroy(&origin->hashtable);
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

  world_origin_iothread_attach(_iothread(origin, fd), fd);

  return world_error_ok;
}

enum world_error world_origin_detach(struct world_origin *origin, int fd)
{
  if (!world_check_fd(fd)) {
    return world_error_invalid_argument;
  }

  world_origin_iothread_detach(_iothread(origin, fd), fd);

  return world_error_ok;
}

enum world_error world_origin_get(const struct world_origin *origin, struct world_iovec key, struct world_iovec *data)
{
  return world_hashtable_get((struct world_hashtable *)&origin->hashtable, key, data);
}

enum world_error world_origin_set(struct world_origin *origin, struct world_iovec key, struct world_iovec data)
{
  enum world_error err = world_hashtable_set(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  _checkpoint(origin);

  return world_error_ok;
}

enum world_error world_origin_add(struct world_origin *origin, struct world_iovec key, struct world_iovec data)
{
  enum world_error err = world_hashtable_add(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  _checkpoint(origin);

  return world_error_ok;
}

enum world_error world_origin_replace(struct world_origin *origin, struct world_iovec key, struct world_iovec data)
{
  enum world_error err = world_hashtable_replace(&origin->hashtable, key, data);
  if (err) {
    return err;
  }

  _checkpoint(origin);

  return world_error_ok;
}

enum world_error world_origin_delete(struct world_origin *origin, struct world_iovec key)
{
  enum world_error err = world_hashtable_delete(&origin->hashtable, key);
  if (err) {
    return err;
  }

  _checkpoint(origin);

  return world_error_ok;
}

static struct world_origin_iothread *_iothread(struct world_origin *origin, int fd)
{
  return &origin->threads[fd % origin->conf.n_io_threads];
}

static world_sequence _least_sequence(struct world_origin *origin)
{
  world_sequence seq = world_hashtable_log(&origin->hashtable)->base.seq;
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_sequence seq_thread = world_origin_iothread_least_sequence(origin->threads + i);
    if (seq > seq_thread) {
      seq = seq_thread;
    }
  }
  return seq;
}

static void _checkpoint(struct world_origin *origin)
{
  world_hashtable_checkpoint(&origin->hashtable, _least_sequence(origin));
}
