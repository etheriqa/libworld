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

#include <stdlib.h>
#include <string.h>
#include "world_assert.h"
#include "world_origin.h"
#include "world_origin_handler.h"
#include "world_origin_thread.h"

static void _stop(struct world_origin_thread *ot);
static void *_origin_main(void *arg);
static void _detach_closed_handlers(struct world_origin_thread *ot);
static void _enable_updated_handlers(struct world_origin_thread *ot);

static void _dispatcher_init(struct world_origin_thread_dispatcher *dp, struct world_allocator *a);
static void _dispatcher_destroy(struct world_origin_thread_dispatcher *dp);
static void _dispatcher_attach(struct world_origin_thread_dispatcher *dp, int fd, struct world_origin_thread *ot);
static void _dispatcher_detach(struct world_origin_thread_dispatcher *dp, int fd);
static void _dispatcher_interrupt(struct world_origin_thread_dispatcher *dp);
static struct world_origin_handler **_dispatcher_get_handler(struct world_origin_thread_dispatcher *dp, int fd);

void world_origin_thread_init(struct world_origin_thread *ot, struct world_origin *origin)
{
  _dispatcher_init(&ot->dispatcher, &origin->allocator);
  world_circular_init(&ot->closed, &origin->allocator);
  world_circular_init(&ot->updated, &origin->allocator);

  ot->origin = origin;

  int err = pthread_create(&ot->thread, NULL, _origin_main, ot);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }
}

void world_origin_thread_destroy(struct world_origin_thread *ot)
{
  _stop(ot);
  int err = pthread_join(ot->thread, NULL);
  if (err) {
    fprintf(stderr, "pthread_join: %s\n", strerror(err));
  }

  world_circular_destroy(&ot->updated);
  world_circular_destroy(&ot->closed);
  _dispatcher_destroy(&ot->dispatcher);
}

void world_origin_thread_attach(struct world_origin_thread *ot, int fd)
{
  world_mutex_lock(&ot->dispatcher.mtx);
  _dispatcher_attach(&ot->dispatcher, fd, ot);
  world_mutex_unlock(&ot->dispatcher.mtx);
}

void world_origin_thread_detach(struct world_origin_thread *ot, int fd)
{
  world_mutex_lock(&ot->dispatcher.mtx);
  _dispatcher_detach(&ot->dispatcher, fd);
  world_mutex_unlock(&ot->dispatcher.mtx);
}

void world_origin_thread_interrupt(struct world_origin_thread *ot)
{
  _dispatcher_interrupt(&ot->dispatcher);
}

void world_origin_thread_notify_closed(struct world_origin_thread *ot, int fd)
{
  world_circular_push_back(&ot->closed, &fd, sizeof(fd));
}

void world_origin_thread_notify_updated(struct world_origin_thread *ot, int fd)
{
  struct world_origin_handler **handler = _dispatcher_get_handler(&ot->dispatcher, fd);
  WORLD_ASSERT(handler && *handler);
  world_io_multiplexer_detach(&ot->dispatcher.multiplexer, &(*handler)->base);
  world_circular_push_back(&ot->updated, &fd, sizeof(fd));
}

world_sequence world_origin_thread_least_sequence(struct world_origin_thread *ot)
{
  world_sequence seq = world_hashtable_log(&ot->origin->hashtable)->base.seq;
  for (size_t i = 0; i < world_vector_size(&ot->dispatcher.handlers); i++) {
    struct world_origin_handler **handler = world_vector_at(&ot->dispatcher.handlers, i, sizeof(*handler));
    if (!*handler) {
      continue;
    }
    world_sequence seq_handler = world_origin_handler_sequence(*handler);
    if (seq > seq_handler) {
      seq = seq_handler;
    }
  }
  return seq;
}

static void _stop(struct world_origin_thread *ot)
{
  int err = pthread_cancel(ot->thread);
  if (err) {
    fprintf(stderr, "pthread_cancel: %s\n", strerror(err));
  }
}

static void *_origin_main(void *arg)
{
  struct world_origin_thread *ot = arg;
  for (;;) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    world_mutex_lock(&ot->dispatcher.mtx);

    world_io_multiplexer_dispatch(&ot->dispatcher.multiplexer);
    _enable_updated_handlers(ot);
    _detach_closed_handlers(ot);

    world_mutex_unlock(&ot->dispatcher.mtx);
  }
  return NULL;
}

static void _detach_closed_handlers(struct world_origin_thread *ot)
{
  int *fd = NULL;
  while ((fd = world_circular_front(&ot->closed, sizeof(*fd)))) {
    _dispatcher_detach(&ot->dispatcher, *fd);
    world_circular_pop_front(&ot->closed);
  }
}

static void _enable_updated_handlers(struct world_origin_thread *ot)
{
  int *fd = NULL;
  while ((fd = world_circular_front(&ot->updated, sizeof(*fd)))) {
    struct world_origin_handler **handler = _dispatcher_get_handler(&ot->dispatcher, *fd);
    WORLD_ASSERT(handler && *handler);

    (*handler)->base.writer(&(*handler)->base);
    if (world_circular_size(&ot->updated) > 1 &&
        *(int *)world_circular_back(&ot->updated, sizeof(*fd)) == *fd) {
      world_circular_pop_back(&ot->updated);
      return;
    }
    if (world_circular_size(&ot->closed) == 0 ||
        *(int *)world_circular_back(&ot->closed, sizeof(*fd)) != *fd) {
      world_io_multiplexer_attach(&ot->dispatcher.multiplexer, &(*handler)->base);
    }

    world_circular_pop_front(&ot->updated);
  }
}

static void _dispatcher_init(struct world_origin_thread_dispatcher *dp, struct world_allocator *a)
{
  world_mutex_init(&dp->mtx);
  world_vector_init(&dp->handlers, a);
  world_io_multiplexer_init(&dp->multiplexer, a);
  world_io_interrupter_init(&dp->interrupter);

  world_io_multiplexer_attach(&dp->multiplexer, &dp->interrupter.handler);
}

static void _dispatcher_destroy(struct world_origin_thread_dispatcher *dp)
{
  struct world_origin_handler **handler;
  while ((handler = world_vector_back(&dp->handlers, sizeof(*handler)))) {
    if (*handler) {
      world_origin_handler_delete(*handler);
    }
    world_vector_pop_back(&dp->handlers);
  }

  world_mutex_destroy(&dp->mtx);
  world_vector_destroy(&dp->handlers);
  world_io_multiplexer_destroy(&dp->multiplexer);
  world_io_interrupter_destroy(&dp->interrupter);
}

static void _dispatcher_attach(struct world_origin_thread_dispatcher *dp, int fd, struct world_origin_thread *ot)
{
  // XXX rewrite with world_vector_resize()
  while (world_vector_size(&dp->handlers) <= (size_t)fd) {
    void *value = NULL;
    world_vector_push_back(&dp->handlers, &value, sizeof(value));
  }

  struct world_origin_handler **handler = _dispatcher_get_handler(dp, fd);
  if (*handler) {
    return;
  }
  *handler = world_origin_handler_new(ot->origin, ot, fd);
  world_io_multiplexer_attach(&dp->multiplexer, &(*handler)->base);
  (*handler)->base.writer(&(*handler)->base);
}

static void _dispatcher_detach(struct world_origin_thread_dispatcher *dp, int fd)
{
  struct world_origin_handler **handler = _dispatcher_get_handler(dp, fd);
  if (!handler || !*handler) {
    return;
  }
  world_io_multiplexer_detach(&dp->multiplexer, &(*handler)->base);
  world_origin_handler_delete(*handler);
  *handler = NULL;
}

static void _dispatcher_interrupt(struct world_origin_thread_dispatcher *dp)
{
  world_io_interrupter_invoke(&dp->interrupter);
}

static struct world_origin_handler **_dispatcher_get_handler(struct world_origin_thread_dispatcher *dp, int fd)
{
  if (world_vector_size(&dp->handlers) <= (size_t)fd) {
    return NULL;
  }
  return world_vector_at(&dp->handlers, fd, sizeof(struct world_origin_handler *));
}
