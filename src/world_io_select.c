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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include "world_io.h"
#include "world_io_select.h"

void world_io_multiplexer_init(struct world_io_multiplexer *m)
{
  assert(m);

  vector_init(&m->handlers);
  FD_ZERO(&m->read_fds);
  FD_ZERO(&m->write_fds);
  FD_ZERO(&m->error_fds);
}

void world_io_multiplexer_destroy(struct world_io_multiplexer *m)
{
  assert(m);

  vector_destroy(&m->handlers);
}

void world_io_multiplexer_attach(struct world_io_multiplexer *m, struct world_io_handler *h)
{
  assert(m);
  assert(h);

  if (h->fd >= vector_size(&m->handlers)) {
    vector_resize(&m->handlers, h->fd + 1, sizeof(struct world_io_handler *));
  }
  *(struct world_io_handler **)vector_at(&m->handlers, h->fd, sizeof(h)) = h;
}

void world_io_multiplexer_detach(struct world_io_multiplexer *m, struct world_io_handler *h)
{
  assert(m);
  assert(h);

  if (h->fd < vector_size(&m->handlers)) {
    *(struct world_io_handler **)vector_at(&m->handlers, h->fd, sizeof(h)) = NULL;
  }
}

void world_io_multiplexer_dispatch(struct world_io_multiplexer *m)
{
  FD_ZERO(&m->read_fds);
  FD_ZERO(&m->write_fds);
  FD_ZERO(&m->error_fds);
  for (size_t i = 0; i < vector_size(&m->handlers); i++) {
    struct world_io_handler **handler = vector_at(&m->handlers, i, sizeof(*handler));
    if (!handler || !*handler) {
      continue;
    }
    if ((*handler)->reader) {
      FD_SET((*handler)->fd, &m->read_fds);
    }
    if ((*handler)->writer) {
      FD_SET((*handler)->fd, &m->write_fds);
    }
    FD_SET((*handler)->fd, &m->error_fds);
  }

  int n_events;
  for (;;) {
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = WORLD_IO_MULTIPLEX_TIMEOUT_IN_MILLISECONDS * 1000;
    n_events = select(FD_SETSIZE, &m->read_fds, &m->write_fds, &m->error_fds, &timeout);
    if (n_events != -1) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    perror("world_io_multiplexer: select");
    abort();
  }

  for (size_t i = 0; i < FD_SETSIZE; i++) {
    // TODO handle errors
    struct world_io_handler **handler = vector_at(&m->handlers, i, sizeof(*handler));
    if (!handler || !*handler) {
      continue;
    }
    if (n_events != -1 && FD_ISSET(i, &m->read_fds) && (*handler)->reader) {
      (*handler)->reader(*handler);
    }
    if (n_events != -1 && FD_ISSET(i, &m->write_fds) && (*handler)->writer) {
      (*handler)->writer(*handler);
    }
  }
}
