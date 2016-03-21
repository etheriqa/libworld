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

#pragma once

#include <pthread.h>
#include <stdatomic.h>
#include "circular.h"
#include "vector.h"
#include "world_io.h"
#include "world_mutex.h"

struct world_origin;

struct world_origin_iothread {
  struct world_origin_iothread_dispatcher {
    struct world_mutex mtx;
    struct vector handlers;
    struct world_io_multiplexer multiplexer;
  } dispatcher;

  struct world_origin_iothread_updated {
    struct world_mutex mtx;
    struct circular fds;
  } updated;

  struct world_origin_iothread_asleep {
    struct circular fds;
  } asleep;

  pthread_t thread;
  atomic_bool thread_loop_condition;

  struct world_origin *origin;
};

void world_origin_iothread_init(struct world_origin_iothread *ot, struct world_origin *origin);
void world_origin_iothread_destroy(struct world_origin_iothread *ot);
void world_origin_iothread_attach(struct world_origin_iothread *ot, int fd);
void world_origin_iothread_detach(struct world_origin_iothread *ot, int fd);
void world_origin_iothread_mark_updated(struct world_origin_iothread *ot, int fd);
world_sequence world_origin_iothread_least_sequence(struct world_origin_iothread *ot);
