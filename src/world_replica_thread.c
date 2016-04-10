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
#include "world_replica.h"
#include "world_replica_thread.h"

static void *_replica_main(void *arg);

void world_replica_thread_init(struct world_replica_thread *rt, struct world_replica *replica)
{
  world_io_multiplexer_init(&rt->multiplexer, &replica->allocator);
  world_replica_handler_init(&rt->handler, replica);
  world_io_multiplexer_attach(&rt->multiplexer, &rt->handler.base);

  rt->replica = replica;

  int err = pthread_create(&rt->thread, NULL, _replica_main, rt);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }
}

void world_replica_thread_destroy(struct world_replica_thread *rt)
{
  world_replica_thread_stop(rt);
  int err = pthread_join(rt->thread, NULL);
  if (err) {
    fprintf(stderr, "pthread_join: %s\n", strerror(err));
  }

  world_io_multiplexer_destroy(&rt->multiplexer);
  world_replica_handler_destroy(&rt->handler);
}

void world_replica_thread_stop(struct world_replica_thread *rt)
{
  int err = pthread_cancel(rt->thread);
  if (err) {
    fprintf(stderr, "pthread_cancel: %s\n", strerror(err));
  }
}

static void *_replica_main(void *arg)
{
  struct world_replica_thread *rt = arg;
  for (;;) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    world_io_multiplexer_dispatch(&rt->multiplexer);
  }
  return NULL;
}
