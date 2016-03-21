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
#include "world_origin.h"
#include "world_origin_iohandler.h"
#include "world_origin_iothread.h"

static void *_main(void *arg);
static void _sleep_updated_handlers(struct world_origin_iothread *ot);
static void _wake_asleep_handlers(struct world_origin_iothread *ot);

static void _dispatcher_init(struct world_origin_iothread_dispatcher *dp);
static void _dispatcher_destroy(struct world_origin_iothread_dispatcher *dp);
static void _dispatcher_attach(struct world_origin_iothread_dispatcher *dp, int fd, struct world_origin_iothread *ot);
static void _dispatcher_detach(struct world_origin_iothread_dispatcher *dp, int fd);
static struct world_origin_iohandler **_dispatcher_get_iohandler(struct world_origin_iothread_dispatcher *dp, int fd);

static void _updated_init(struct world_origin_iothread_updated *up);
static void _updated_destroy(struct world_origin_iothread_updated *up);
static int *_updated_top(struct world_origin_iothread_updated *up);
static void _updated_enqueue(struct world_origin_iothread_updated *up, int fd);
static void _updated_dequeue(struct world_origin_iothread_updated *up);

static void _asleep_init(struct world_origin_iothread_asleep *as);
static void _asleep_destroy(struct world_origin_iothread_asleep *as);
static int *_asleep_top(struct world_origin_iothread_asleep *as);
static void _asleep_enqueue(struct world_origin_iothread_asleep *as, int fd);
static void _asleep_dequeue(struct world_origin_iothread_asleep *as);

void world_origin_iothread_init(struct world_origin_iothread *ot, struct world_origin *origin)
{
  _dispatcher_init(&ot->dispatcher);
  _updated_init(&ot->updated);
  _asleep_init(&ot->asleep);

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
  _updated_destroy(&ot->updated);
  _asleep_destroy(&ot->asleep);
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
  world_mutex_lock(&ot->updated.mtx);
  _updated_enqueue(&ot->updated, fd);
  world_mutex_unlock(&ot->updated.mtx);
}

world_sequence world_origin_iothread_least_sequence(struct world_origin_iothread *ot)
{
  world_sequence seq = world_hashtable_log(&ot->origin->hashtable)->base.seq;
  for (size_t i = 0; i < vector_size(&ot->dispatcher.handlers); i++) {
    struct world_origin_iohandler **handler = vector_at(&ot->dispatcher.handlers, i, sizeof(*handler));
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

    world_mutex_lock(&ot->updated.mtx);
    _sleep_updated_handlers(ot);
    world_mutex_unlock(&ot->updated.mtx);

    _wake_asleep_handlers(ot);
    world_io_multiplexer_dispatch(&ot->dispatcher.multiplexer);

    world_mutex_unlock(&ot->dispatcher.mtx);
  }
  return NULL;
}

static void _sleep_updated_handlers(struct world_origin_iothread *ot)
{
  int *fd = NULL;
  while ((fd = _updated_top(&ot->updated))) {
    struct world_origin_iohandler **handler = _dispatcher_get_iohandler(&ot->dispatcher, *fd);
    if (handler && *handler && world_origin_iohandler_is_updated(*handler)) {
      world_io_multiplexer_detach(&ot->dispatcher.multiplexer, &(*handler)->base);
      _asleep_enqueue(&ot->asleep, *fd);
    }
    _updated_dequeue(&ot->updated);
  }
}

static void _wake_asleep_handlers(struct world_origin_iothread *ot)
{
  int *fd = NULL;
  while ((fd = _asleep_top(&ot->asleep))) {
    struct world_origin_iohandler **handler = _dispatcher_get_iohandler(&ot->dispatcher, *fd);
    if (handler && *handler) {
      if (world_origin_iohandler_is_updated(*handler)) {
        return;
      }
      world_io_multiplexer_attach(&ot->dispatcher.multiplexer, &(*handler)->base);
    }
    _asleep_dequeue(&ot->asleep);
  }
}

static void _dispatcher_init(struct world_origin_iothread_dispatcher *dp)
{
  world_mutex_init(&dp->mtx);
  vector_init(&dp->handlers);
  world_io_multiplexer_init(&dp->multiplexer);
}

static void _dispatcher_destroy(struct world_origin_iothread_dispatcher *dp)
{
  struct world_origin_iohandler **handler;
  while ((handler = vector_back(&dp->handlers, sizeof(*handler)))) {
    if (*handler) {
      world_origin_iohandler_delete(*handler);
    }
    vector_pop_back(&dp->handlers);
  }

  world_mutex_destroy(&dp->mtx);
  vector_destroy(&dp->handlers);
  world_io_multiplexer_destroy(&dp->multiplexer);
}

static void _dispatcher_attach(struct world_origin_iothread_dispatcher *dp, int fd, struct world_origin_iothread *ot)
{
  vector_resize(&dp->handlers, fd + 1, sizeof(struct world_origin_iohandler *));

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
  if (vector_size(&dp->handlers) < (size_t)fd + 1) {
    return NULL;
  }
  return vector_at(&dp->handlers, fd, sizeof(struct world_origin_iohandler *));
}

static void _updated_init(struct world_origin_iothread_updated *up)
{
  world_mutex_init(&up->mtx);
  circular_init(&up->fds);
}

static void _updated_destroy(struct world_origin_iothread_updated *up)
{
  world_mutex_destroy(&up->mtx);
  circular_destroy(&up->fds);
}

static int *_updated_top(struct world_origin_iothread_updated *up)
{
  return circular_front(&up->fds, sizeof(int));
}

static void _updated_enqueue(struct world_origin_iothread_updated *up, int fd)
{
  circular_push_back(&up->fds, &fd, sizeof(fd));
}

static void _updated_dequeue(struct world_origin_iothread_updated *up)
{
  circular_pop_front(&up->fds);
}

static void _asleep_init(struct world_origin_iothread_asleep *as)
{
  circular_init(&as->fds);
}

static void _asleep_destroy(struct world_origin_iothread_asleep *as)
{
  circular_destroy(&as->fds);
}

static int *_asleep_top(struct world_origin_iothread_asleep *as)
{
  return circular_front(&as->fds, sizeof(int));
}

static void _asleep_enqueue(struct world_origin_iothread_asleep *as, int fd)
{
  circular_push_back(&as->fds, &fd, sizeof(fd));
}

static void _asleep_dequeue(struct world_origin_iothread_asleep *as)
{
  circular_pop_front(&as->fds);
}
