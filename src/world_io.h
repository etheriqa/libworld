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

#include <stdint.h>

#define WORLD_IO_MULTIPLEX_TIMEOUT_IN_MILLISECONDS 10

struct world_io_handler;
struct world_io_multiplexer;

void world_io_multiplexer_init(struct world_io_multiplexer *m);
void world_io_multiplexer_destroy(struct world_io_multiplexer *m);
void world_io_multiplexer_attach(struct world_io_multiplexer *m, struct world_io_handler *h);
void world_io_multiplexer_detach(struct world_io_multiplexer *m, struct world_io_handler *h);
void world_io_multiplexer_dispatch(struct world_io_multiplexer *m);

struct world_io_handler {
  uint32_t fd;
  void (*reader)(struct world_io_handler *h);
  void (*writer)(struct world_io_handler *h);
  void (*error)(struct world_io_handler *h); // TODO implement
};

#if defined(WORLD_USE_SELECT)
#include "world_io_select.h"
#elif defined(WORLD_USE_POLL)
#include "world_io_poll.h"
#elif defined(WORLD_USE_KQUEUE)
#include "world_io_kqueue.h"
#elif defined(WORLD_USE_EPOLL)
#include "world_io_epoll.h"
#else
#error "No I/O multiplex methods are found"
#endif
