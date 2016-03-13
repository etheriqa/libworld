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

struct io_handler;
struct io_multiplexer;

void io_multiplexer_init(struct io_multiplexer *m);
void io_multiplexer_destroy(struct io_multiplexer *m);
void io_multiplexer_attach(struct io_multiplexer *m, struct io_handler *h);
void io_multiplexer_detach(struct io_multiplexer *m, struct io_handler *h);
void io_multiplexer_dispatch(struct io_multiplexer *m);

struct io_handler {
  int fd;
  void (*reader)(struct io_handler *h);
  void (*writer)(struct io_handler *h);
};

#if defined(WORLD_USE_POLL)
#include "io_poll.h"
#elif defined(WORLD_USE_KQUEUE)
#include "io_kqueue.h"
#elif defined(WORLD_USE_EPOLL)
#include "io_epoll.h"
#else
#error "No I/O multiplexing method is found"
#endif
