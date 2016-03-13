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
#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include "io.h"

void io_multiplexer_init(struct io_multiplexer *m)
{
  assert(m);
  m->fd = epoll_create(1);
  if (m->fd == -1) {
    perror("world: io: epoll_create");
    abort();
  }
}

void io_multiplexer_destroy(struct io_multiplexer *m)
{
  assert(m);
  while (close(m->fd) == -1) {
    if (errno == EINTR) {
      // XXX is this ok?
      continue;
    }
    perror("world: io: close");
    abort();
  }
}

void io_multiplexer_attach(struct io_multiplexer *m, struct io_handler *h)
{
  assert(m);
  assert(h);
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = (h->reader ? EPOLLIN : (enum EPOLL_EVENTS)0) |
                 (h->writer ? EPOLLOUT : (enum EPOLL_EVENTS)0);
  event.data.ptr = h;
  if (epoll_ctl(m->fd, EPOLL_CTL_ADD, h->fd, &event) == 0) {
    return;
  }
  if (errno == EEXIST && epoll_ctl(m->fd, EPOLL_CTL_MOD, h->fd, &event) == 0) {
    return;
  }
  perror("world: io: epoll_ctl");
  abort();
}

void io_multiplexer_detach(struct io_multiplexer *m, struct io_handler *h)
{
  assert(m);
  assert(h);
  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = (h->reader ? EPOLLIN : (enum EPOLL_EVENTS)0) |
                 (h->writer ? EPOLLOUT : (enum EPOLL_EVENTS)0);
  event.data.ptr = h;
  if (epoll_ctl(m->fd, EPOLL_CTL_DEL, h->fd, &event) == 0 || errno == ENOENT) {
    return;
  }
  perror("world: io: epoll_ctl");
  abort();
}

void io_multiplexer_dispatch(struct io_multiplexer *m)
{
  struct epoll_event events[16];
  int n_events;
  for (;;) {
    n_events = epoll_wait(m->fd, events, 16, 1);
    if (n_events != -1) {
      break;
    }
    if (errno == EINTR) {
      continue;
    }
    perror("world: io: epoll_wait");
    abort();
  }
  for (struct epoll_event *event = events; event < events + n_events; event++) {
    // FIXME handle errors
    if (event->events & EPOLLIN) {
      assert(((struct io_handler *)event->data.ptr)->reader);
      ((struct io_handler *)event->data.ptr)->reader(event->data.ptr);
    }
    if (event->events & EPOLLOUT) {
      assert(((struct io_handler *)event->data.ptr)->writer);
      ((struct io_handler *)event->data.ptr)->writer(event->data.ptr);
    }
  }
}
