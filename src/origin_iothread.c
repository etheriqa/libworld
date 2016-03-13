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
#include "origin.h"
#include "origin_iohandler.h"
#include "origin_iothread.h"

static void origin_iothread__reserve_handlers(struct origin_iothread *ot, int fd);
static struct origin_iohandler **origin_iothread__get_iohandler(struct origin_iothread *ot, int fd);
static void *origin_iothread__main(void *arg);

void origin_iothread_init(struct origin_iothread *ot, struct world_origin *origin)
{
  io_multiplexer_init(&ot->multiplexer);
  vector_init(&ot->handlers);
  mutex_init(&ot->handlers_mtx);
  vector_init(&ot->disabled_fds);

  ot->origin = origin;

  ot->thread_loop_condition = true;
  int err = pthread_create(&ot->thread, NULL, origin_iothread__main, ot);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }
}

void origin_iothread_destroy(struct origin_iothread *ot)
{
  ot->thread_loop_condition = false;
  int err = pthread_join(ot->thread, NULL);
  if (err) {
    fprintf(stderr, "pthread_join: %s\n", strerror(err));
  }

  struct origin_iohandler **handler;
  while ((handler = vector_back(&ot->handlers, sizeof(*handler)))) {
    if (*handler) {
      origin_iohandler_delete(*handler);
    }
    vector_pop_back(&ot->handlers);
  }

  io_multiplexer_destroy(&ot->multiplexer);
  vector_destroy(&ot->handlers);
  mutex_destroy(&ot->handlers_mtx);
  vector_destroy(&ot->disabled_fds);
}

void origin_iothread_attach(struct origin_iothread *ot, int fd)
{
  mutex_lock(&ot->handlers_mtx);

  origin_iothread__reserve_handlers(ot, fd);

  struct origin_iohandler **handler = origin_iothread__get_iohandler(ot, fd);
  if (*handler) {
    goto cleanup;
  }
  *handler = origin_iohandler_new(ot->origin, ot, fd);
  io_multiplexer_attach(&ot->multiplexer, &(*handler)->base);

cleanup:
  mutex_unlock(&ot->handlers_mtx);
}

void origin_iothread_detach(struct origin_iothread *ot, int fd)
{
  mutex_lock(&ot->handlers_mtx);

  struct origin_iohandler **handler = origin_iothread__get_iohandler(ot, fd);
  if (!handler || !*handler) {
    goto cleanup;
  }
  io_multiplexer_detach(&ot->multiplexer, &(*handler)->base);
  origin_iohandler_delete(*handler);
  *handler = NULL;

cleanup:
  mutex_unlock(&ot->handlers_mtx);
}

void origin_iothread_disable(struct origin_iothread *ot, int fd)
{
  mutex_lock(&ot->handlers_mtx);

  struct origin_iohandler **handler = origin_iothread__get_iohandler(ot, fd);
  if (!handler || !*handler) {
    goto cleanup;
  }
  io_multiplexer_detach(&ot->multiplexer, &(*handler)->base);
  vector_push_back(&ot->disabled_fds, &fd, sizeof(fd));

cleanup:
  mutex_unlock(&ot->handlers_mtx);
}

void origin_iothread_enable_all(struct origin_iothread *ot)
{
  mutex_lock(&ot->handlers_mtx);

  int *fd = NULL;
  while ((fd = vector_back(&ot->disabled_fds, sizeof(*fd)))) {
    struct origin_iohandler **handler = origin_iothread__get_iohandler(ot, *fd);
    if (handler && *handler) {
      io_multiplexer_attach(&ot->multiplexer, &(*handler)->base);
    }
    vector_pop_back(&ot->disabled_fds);
  }

  mutex_unlock(&ot->handlers_mtx);
}

static void origin_iothread__reserve_handlers(struct origin_iothread *ot, int fd)
{
  vector_reserve(&ot->handlers, fd + 1, sizeof(struct origin_iohandler *));
  while (vector_size(&ot->handlers) < (size_t)fd + 1) {
    const void *element = NULL;
    vector_push_back(&ot->handlers, &element, sizeof(struct origin_iohandler *));
  }
}

static struct origin_iohandler **origin_iothread__get_iohandler(struct origin_iothread *ot, int fd)
{
  if (vector_size(&ot->handlers) < (size_t)fd + 1) {
    return NULL;
  }
  return vector_at(&ot->handlers, fd, sizeof(struct origin_iohandler *));
}

static void *origin_iothread__main(void *arg)
{
  struct origin_iothread *ot = arg;
  while (ot->thread_loop_condition) {
    io_multiplexer_dispatch(&ot->multiplexer);
  }
  return NULL;
}
