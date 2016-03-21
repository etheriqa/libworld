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

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include "world_hash.h"
#include "world_hashtable_entry.h"
#include "world_origin.h"
#include "world_origin_iothread.h"

static struct world_origin_iothread *_iothread(struct world_origin *origin, int fd);
static world_sequence _least_sequence(struct world_origin *origin);
static void _checkpoint(struct world_origin *origin);

struct world_origin *world_origin_open(const struct world_originconf *conf)
{
  // TODO validate conf here

  struct world_origin *origin = malloc(sizeof(*origin));
  if (origin == NULL) {
    perror("malloc");
    abort();
  }

  memcpy((void *)&origin->conf, conf, sizeof(origin->conf));
  world_hashtable_init(&origin->hashtable, world_generate_seed());

  origin->threads = calloc(origin->conf.n_io_threads, sizeof(*origin->threads));
  if (origin->threads == NULL) {
    perror("calloc");
    abort();
  }
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_iothread_init(&origin->threads[i], origin);
  }

  return origin;
}

void world_origin_close(struct world_origin *origin)
{
  for (size_t i = 0; i < origin->conf.n_io_threads; i++) {
    world_origin_iothread_destroy(&origin->threads[i]);
  }

  world_hashtable_destroy(&origin->hashtable);

  free(origin->threads);
  free(origin);
}

bool world_origin_attach(struct world_origin *origin, int fd)
{
  // TODO check whether the value of fd fits into [0, RLIMIT_NOFILE) by calling getrlimit(2)
  // TODO return value

  if (origin->conf.set_nonblocking) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
      perror("fcntl");
      abort();
    }
  }

  if (origin->conf.set_tcp_nodelay) {
    int option = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)) == -1) {
      if (errno == EOPNOTSUPP) {
        // do nothing
      } else {
        perror("setsockopt");
        abort();
      }
    }
  }

  world_origin_iothread_attach(_iothread(origin, fd), fd);
  return true; // TODO
}

bool world_origin_detach(struct world_origin *origin, int fd)
{
  // TODO check whether the value of fd fits into [0, RLIMIT_NOFILE) by calling getrlimit(2)
  world_origin_iothread_detach(_iothread(origin, fd), fd);
  return true; // TODO
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
