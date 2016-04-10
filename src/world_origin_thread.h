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

#pragma once

#include <pthread.h>
#include "world_circular.h"
#include "world_io.h"
#include "world_mutex.h"
#include "world_vector.h"

struct world_origin;

struct world_origin_thread {
  struct world_origin_thread_dispatcher {
    struct world_mutex mtx;
    struct world_vector handlers;
    struct world_io_multiplexer multiplexer;
    struct world_io_interrupter interrupter;
  } dispatcher;

  struct world_circular closed;
  struct world_circular updated;

  pthread_t thread;

  struct world_origin *origin;
};

void world_origin_thread_init(struct world_origin_thread *ot, struct world_origin *origin);
void world_origin_thread_destroy(struct world_origin_thread *ot);
void world_origin_thread_attach(struct world_origin_thread *ot, int fd);
void world_origin_thread_detach(struct world_origin_thread *ot, int fd);
void world_origin_thread_interrupt(struct world_origin_thread *ot);
void world_origin_thread_notify_closed(struct world_origin_thread *ot, int fd);
void world_origin_thread_notify_updated(struct world_origin_thread *ot, int fd);
world_sequence world_origin_thread_least_sequence(struct world_origin_thread *ot);
