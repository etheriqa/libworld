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
#include <sys/errno.h>
#include <sys/uio.h>
#include "origin.h"
#include "origin_iohandler.h"
#include "origin_iothread.h"

static bool origin_iohandler__is_updated(struct origin_iohandler *oh);
static void origin_iohandler__writer_proxy(struct io_handler *h);
static void origin_iohandler__write(struct origin_iohandler *oh);
static void origin_iohandler__fill_iovec(struct origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs);
static void origin_iohandler__drain_iovec(struct origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs, size_t n_written);

struct origin_iohandler *origin_iohandler_new(struct world_origin *origin, struct origin_iothread *thread, int fd)
{
  struct origin_iohandler *oh = malloc(sizeof(*oh));
  if (oh == NULL) {
    perror("malloc");
    abort();
  }

  oh->base.fd = fd;
  oh->base.reader = NULL;
  oh->base.writer = origin_iohandler__writer_proxy;
  oh->seq = hashtable_get_sequence(&origin->hashtable);
  oh->snapshot_iterator = hashtable_begin(&origin->hashtable, oh->seq);
  oh->offset = 0;
  oh->origin = origin;
  oh->thread = thread;

  return oh;
}

void origin_iohandler_delete(struct origin_iohandler *oh)
{
  free(oh);
}

static bool origin_iohandler__is_updated(struct origin_iohandler *oh)
{
  return oh->offset == 0 &&
         !hashtable_iterator_valid(oh->snapshot_iterator) &&
         oh->seq == hashtable_get_sequence(&oh->origin->hashtable);
}

static void origin_iohandler__writer_proxy(struct io_handler *h)
{
  origin_iohandler__write((struct origin_iohandler *)h);
}

static void origin_iohandler__write(struct origin_iohandler *oh)
{
  const size_t n_iovecs = 16;

  struct iovec iovecs[n_iovecs];
  origin_iohandler__fill_iovec(oh, iovecs, n_iovecs);
  if (iovecs[0].iov_len == 0) {
    return;
  }

  ssize_t n_written = writev(oh->base.fd, iovecs, n_iovecs);
  if (n_written == -1) {
    if (errno == EAGAIN) {
      return;
    }
    // TODO handle errors
    perror("writev");
    abort();
  }

  origin_iohandler__drain_iovec(oh, iovecs, n_iovecs, n_written);

  if (origin_iohandler__is_updated(oh)) {
    origin_iothread_disable(oh->thread, oh->base.fd);
  }
}

static void origin_iohandler__fill_iovec(struct origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs)
{
  memset(iovecs, 0, sizeof(struct iovec) * n_iovecs);

  world_sequence seq = oh->seq;
  struct hashtable_iterator cursor = oh->snapshot_iterator;
  for (size_t i = 0; i < n_iovecs; i++) {
    struct world_iovec iovec;
    if (hashtable_iterator_valid(cursor)) {
      iovec = hashtable_iterator_get_iovec(cursor, oh->seq);
      cursor = hashtable_iterator_next(cursor, oh->seq);
    } else if (seq < hashtable_get_sequence(&oh->origin->hashtable)) {
      seq++;
      iovec = hashtable_get_iovec(&oh->origin->hashtable, seq);
    } else if (i == 0) {
      return;
    } else {
      break;
    }
    iovecs[i].iov_base = (char *)iovec.base;
    iovecs[i].iov_len = iovec.size;
  }

  assert(iovecs[0].iov_len > oh->offset);
  iovecs[0].iov_base = (char *)((uintptr_t)iovecs[0].iov_base + oh->offset);
  iovecs[0].iov_len -= oh->offset;
}

static void origin_iohandler__drain_iovec(struct origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs, size_t n_written)
{
  for (size_t i = 0; i < n_iovecs; i++) {
    if (iovecs[i].iov_len == 0) {
      break;
    }

    if (n_written < iovecs[i].iov_len) {
      oh->offset += n_written;
      break;
    }

    oh->offset = 0;
    n_written -= iovecs[i].iov_len;
    if (hashtable_iterator_valid(oh->snapshot_iterator)) {
      oh->snapshot_iterator = hashtable_iterator_next(oh->snapshot_iterator, oh->seq);
    } else {
      oh->seq++;
    }
  }
}
