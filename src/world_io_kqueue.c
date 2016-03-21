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
#include <sys/event.h>
#include <time.h>
#include <unistd.h>
#include "world_io.h"
#include "world_io_kqueue.h"

void world_io_multiplexer_init(struct world_io_multiplexer *m)
{
  assert(m);
  m->fd = kqueue();
  if (m->fd == -1) {
    perror("world_io_multiplexer: kqueue");
    abort();
  }
}

void world_io_multiplexer_destroy(struct world_io_multiplexer *m)
{
  assert(m);
  while (close(m->fd) == -1) {
    if (errno == EINTR) {
      // XXX is this ok?
      continue;
    }
    perror("world_io_multiplexer: close");
    abort();
  }
}

void world_io_multiplexer_attach(struct world_io_multiplexer *m, struct world_io_handler *h)
{
  assert(m);
  assert(h);

  struct kevent event;
  memset(&event, 0, sizeof(event));
  event.ident = h->fd;
  event.filter |= (h->reader ? EVFILT_READ : 0);
  event.filter |= (h->writer ? EVFILT_WRITE : 0);
  event.flags = EV_ADD;
  event.udata = h;

  if (kevent(m->fd, &event, 1, NULL, 0, NULL) != -1) {
    return;
  }

  perror("world_io_multiplexer: kevent");
  abort();
}

void world_io_multiplexer_detach(struct world_io_multiplexer *m, struct world_io_handler *h)
{
  assert(m);
  assert(h);

  struct kevent event;
  memset(&event, 0, sizeof(event));
  event.ident = h->fd;
  event.filter = (h->reader ? EVFILT_READ : 0) | (h->writer ? EVFILT_WRITE : 0);
  event.flags = EV_DELETE;
  event.udata = h;

  if (kevent(m->fd, &event, 1, NULL, 0, NULL) != -1 || errno == ENOENT) {
    return;
  }

  perror("world_io_multiplexer: kevent");
  abort();
}

void world_io_multiplexer_dispatch(struct world_io_multiplexer *m)
{
  struct kevent events[16];
  int n_events;
  for (;;) {
    struct timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = WORLD_IO_MULTIPLEX_TIMEOUT_IN_MILLISECONDS * 1000000;
    n_events = kevent(m->fd, NULL, 0, events, 16, &timeout);
    if (n_events != -1) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    perror("world_io_multiplexer: kevent");
    abort();
  }

  for (struct kevent *event = events; event < events + n_events; event++) {
    // TODO handle errors
    struct world_io_handler *handler = event->udata;
    if (event->filter == EVFILT_READ && handler->reader) {
      handler->reader(handler);
    }
    if (event->filter == EVFILT_WRITE && handler->writer) {
      handler->writer(handler);
    }
  }
}
