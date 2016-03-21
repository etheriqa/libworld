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
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include "world_io.h"
#include "world_io_poll.h"

void world_io_multiplexer_init(struct world_io_multiplexer *m)
{
  assert(m);

  vector_init(&m->handlers);
  vector_init(&m->poll_fds);
}

void world_io_multiplexer_destroy(struct world_io_multiplexer *m)
{
  assert(m);

  vector_destroy(&m->handlers);
  vector_destroy(&m->poll_fds);
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
  vector_clear(&m->poll_fds);
  for (size_t i = 0; i < vector_size(&m->handlers); i++) {
    struct world_io_handler **handler = vector_at(&m->handlers, i, sizeof(*handler));
    if (!handler || !*handler) {
      continue;
    }
    struct pollfd event;
    memset(&event, 0, sizeof(event));
    event.fd = (*handler)->fd;
    event.events = ((*handler)->reader ? POLLIN : 0) |
                   ((*handler)->writer ? POLLOUT : 0);
    vector_push_back(&m->poll_fds, &event, sizeof(event));
  }

  int n_events;
  for (;;) {
    n_events = poll(vector_front(&m->poll_fds), vector_size(&m->poll_fds), WORLD_IO_MULTIPLEX_TIMEOUT_IN_MILLISECONDS);
    if (n_events != -1) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    perror("world_io_multiplexer: poll");
    abort();
  }

  for (size_t i = 0; i < vector_size(&m->poll_fds); i++) {
    // TODO handle errors (POLLERR, POLLHUP, POLLNVAL)
    struct pollfd *event = vector_at(&m->poll_fds, i, sizeof(*event));
    struct world_io_handler **handler = vector_at(&m->handlers, event->fd, sizeof(*handler));
    if (!handler || !*handler) {
      continue;
    }
    if (event->revents & POLLIN && (*handler)->reader) {
      (*handler)->reader(*handler);
    }
    if (event->revents & POLLOUT && (*handler)->writer) {
      (*handler)->writer(*handler);
    }
  }
}
