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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "world_origin.h"
#include "world_origin_iohandler.h"
#include "world_origin_iothread.h"

static void *_main(void *arg);
static void _sleep_updated_handlers(struct world_origin_iothread *ot);
static void _wake_asleep_handlers(struct world_origin_iothread *ot);
static void _detach_disconnected_handlers(struct world_origin_iothread *ot);

static void _dispatcher_init(struct world_origin_iothread_dispatcher *dp, struct world_allocator *a);
static void _dispatcher_destroy(struct world_origin_iothread_dispatcher *dp);
static void _dispatcher_attach(struct world_origin_iothread_dispatcher *dp, int fd, struct world_origin_iothread *ot);
static void _dispatcher_detach(struct world_origin_iothread_dispatcher *dp, int fd);
static struct world_origin_iohandler **_dispatcher_get_iohandler(struct world_origin_iothread_dispatcher *dp, int fd);

void world_origin_iothread_init(struct world_origin_iothread *ot, struct world_origin *origin)
{
  _dispatcher_init(&ot->dispatcher, &origin->allocator);
  world_circular_init(&ot->updated, &origin->allocator);
  world_circular_init(&ot->disconnected, &origin->allocator);
  world_circular_init(&ot->asleep, &origin->allocator);

  ot->origin = origin;

  ot->thread_loop_condition = true;
  int err = pthread_create(&ot->thread, NULL, _main, ot);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }
}

void world_origin_iothread_destroy(struct world_origin_iothread *ot)
{
  ot->thread_loop_condition = false;
  int err = pthread_join(ot->thread, NULL);
  if (err) {
    fprintf(stderr, "pthread_join: %s\n", strerror(err));
  }

  _dispatcher_destroy(&ot->dispatcher);
  world_circular_destroy(&ot->updated);
  world_circular_destroy(&ot->disconnected);
  world_circular_destroy(&ot->asleep);
}

void world_origin_iothread_attach(struct world_origin_iothread *ot, int fd)
{
  world_mutex_lock(&ot->dispatcher.mtx);
  _dispatcher_attach(&ot->dispatcher, fd, ot);
  world_mutex_unlock(&ot->dispatcher.mtx);
}

void world_origin_iothread_detach(struct world_origin_iothread *ot, int fd)
{
  world_mutex_lock(&ot->dispatcher.mtx);
  _dispatcher_detach(&ot->dispatcher, fd);
  world_mutex_unlock(&ot->dispatcher.mtx);
}

void world_origin_iothread_mark_updated(struct world_origin_iothread *ot, int fd)
{
  world_circular_push_back(&ot->updated, &fd, sizeof(fd));
}

void world_origin_iothread_mark_disconnected(struct world_origin_iothread *ot, int fd)
{
  world_circular_push_back(&ot->disconnected, &fd, sizeof(fd));
}

world_sequence world_origin_iothread_least_sequence(struct world_origin_iothread *ot)
{
  world_sequence seq = world_hashtable_log(&ot->origin->hashtable)->base.seq;
  for (size_t i = 0; i < world_vector_size(&ot->dispatcher.handlers); i++) {
    struct world_origin_iohandler **handler = world_vector_at(&ot->dispatcher.handlers, i, sizeof(*handler));
    if (!*handler) {
      continue;
    }
    world_sequence seq_handler = world_origin_iohandler_sequence(*handler);
    if (seq > seq_handler) {
      seq = seq_handler;
    }
  }
  return seq;
}

static void *_main(void *arg)
{
  struct world_origin_iothread *ot = arg;
  while (ot->thread_loop_condition) {
    world_mutex_lock(&ot->dispatcher.mtx);

    _sleep_updated_handlers(ot);
    _wake_asleep_handlers(ot);
    world_io_multiplexer_dispatch(&ot->dispatcher.multiplexer);
    _detach_disconnected_handlers(ot);

    world_mutex_unlock(&ot->dispatcher.mtx);
  }
  return NULL;
}

static void _sleep_updated_handlers(struct world_origin_iothread *ot)
{
  int *fd = NULL;
  while ((fd = world_circular_front(&ot->updated, sizeof(*fd)))) {
    struct world_origin_iohandler **handler = _dispatcher_get_iohandler(&ot->dispatcher, *fd);
    if (handler && *handler && world_origin_iohandler_is_updated(*handler)) {
      world_io_multiplexer_detach(&ot->dispatcher.multiplexer, &(*handler)->base);
      world_circular_push_back(&ot->asleep, fd, sizeof(*fd));
    }
    world_circular_pop_front(&ot->updated);
  }
}

static void _wake_asleep_handlers(struct world_origin_iothread *ot)
{
  int *fd = NULL;
  while ((fd = world_circular_front(&ot->asleep, sizeof(*fd)))) {
    struct world_origin_iohandler **handler = _dispatcher_get_iohandler(&ot->dispatcher, *fd);
    if (handler && *handler) {
      if (world_origin_iohandler_is_updated(*handler)) {
        return;
      }
      world_io_multiplexer_attach(&ot->dispatcher.multiplexer, &(*handler)->base);
    }
    world_circular_pop_front(&ot->asleep);
  }
}

static void _detach_disconnected_handlers(struct world_origin_iothread *ot)
{
  int *fd = NULL;
  while ((fd = world_circular_front(&ot->disconnected, sizeof(*fd)))) {
    struct world_origin_iohandler **handler = _dispatcher_get_iohandler(&ot->dispatcher, *fd);
    if (handler && *handler) {
      _dispatcher_detach(&ot->dispatcher, *fd);
      if (close(*fd) == -1) {
        perror("close");
        // go through
      }
    }
    world_circular_pop_front(&ot->disconnected);
  }
}

static void _dispatcher_init(struct world_origin_iothread_dispatcher *dp, struct world_allocator *a)
{
  world_mutex_init(&dp->mtx);
  world_vector_init(&dp->handlers, a);
  world_io_multiplexer_init(&dp->multiplexer, a);
}

static void _dispatcher_destroy(struct world_origin_iothread_dispatcher *dp)
{
  struct world_origin_iohandler **handler;
  while ((handler = world_vector_back(&dp->handlers, sizeof(*handler)))) {
    if (*handler) {
      world_origin_iohandler_delete(*handler);
    }
    world_vector_pop_back(&dp->handlers);
  }

  world_mutex_destroy(&dp->mtx);
  world_vector_destroy(&dp->handlers);
  world_io_multiplexer_destroy(&dp->multiplexer);
}

static void _dispatcher_attach(struct world_origin_iothread_dispatcher *dp, int fd, struct world_origin_iothread *ot)
{
  world_vector_resize(&dp->handlers, fd + 1, sizeof(struct world_origin_iohandler *));

  struct world_origin_iohandler **handler = _dispatcher_get_iohandler(dp, fd);
  if (*handler) {
    return;
  }
  *handler = world_origin_iohandler_new(ot->origin, ot, fd);
  world_io_multiplexer_attach(&dp->multiplexer, &(*handler)->base);
}

static void _dispatcher_detach(struct world_origin_iothread_dispatcher *dp, int fd)
{
  struct world_origin_iohandler **handler = _dispatcher_get_iohandler(dp, fd);
  if (!handler || !*handler) {
    return;
  }
  world_io_multiplexer_detach(&dp->multiplexer, &(*handler)->base);
  world_origin_iohandler_delete(*handler);
  *handler = NULL;
}

static struct world_origin_iohandler **_dispatcher_get_iohandler(struct world_origin_iothread_dispatcher *dp, int fd)
{
  if (world_vector_size(&dp->handlers) < (size_t)fd + 1) {
    return NULL;
  }
  return world_vector_at(&dp->handlers, fd, sizeof(struct world_origin_iohandler *));
}
