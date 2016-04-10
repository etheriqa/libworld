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
#include <sys/errno.h>
#include <unistd.h>
#include "world_io.h"

static void _interrupter_read(struct world_io_handler *h);

void world_io_interrupter_init(struct world_io_interrupter *i)
{
  if (pipe(i->fds) == -1) {
    perror("pipe");
    abort();
  }
  i->handler.fd = i->fds[0];
  i->handler.reader = _interrupter_read;
  i->handler.writer = NULL;
  i->handler.error = NULL;
}

void world_io_interrupter_destroy(struct world_io_interrupter *i)
{
  for (;;) {
    if (close(i->fds[0]) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("close");
      break;
    }
  }
  for (;;) {
    if (close(i->fds[1]) == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("close");
      break;
    }
  }
}

void world_io_interrupter_invoke(struct world_io_interrupter *i)
{
  char c = 0;
  ssize_t n_written = write(i->fds[1], &c, 1);
  if (n_written == -1) {
    perror("write");
    abort();
  }
}

static void _interrupter_read(struct world_io_handler *h)
{
  char buf[4096];
  ssize_t n_read = read(h->fd, buf, sizeof(buf));
  if (n_read == -1) {
    perror("read");
    abort();
  }
}
