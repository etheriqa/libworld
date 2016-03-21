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
#include "world_replica_iothread.h"

static void *_main(void *arg);

void world_replica_iothread_init(struct world_replica_iothread *rt, struct world_replica *replica)
{
  world_io_multiplexer_init(&rt->multiplexer);
  world_replica_iohandler_init(&rt->handler, replica);
  world_io_multiplexer_attach(&rt->multiplexer, &rt->handler.base);

  rt->replica = replica;

  rt->thread_loop_condition = true;
  int err = pthread_create(&rt->thread, NULL, _main, rt);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    abort();
  }
}

void world_replica_iothread_destroy(struct world_replica_iothread *rt)
{
  rt->thread_loop_condition = false;
  int err = pthread_join(rt->thread, NULL);
  if (err) {
    fprintf(stderr, "pthread_join: %s\n", strerror(err));
  }

  world_io_multiplexer_destroy(&rt->multiplexer);
  world_replica_iohandler_destroy(&rt->handler);
}

static void *_main(void *arg)
{
  struct world_replica_iothread *rt = arg;
  while (rt->thread_loop_condition) {
    world_io_multiplexer_dispatch(&rt->multiplexer);
  }
  return NULL;
}
