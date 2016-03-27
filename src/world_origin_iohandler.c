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
#include "world_origin.h"
#include "world_origin_iohandler.h"
#include "world_origin_iothread.h"

static void _write(struct world_io_handler *h);
static void _fill_iovec(struct world_origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs);
static void _drain_iovec(struct world_origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs, size_t n_written);

struct world_origin_iohandler *world_origin_iohandler_new(struct world_origin *origin, struct world_origin_iothread *thread, int fd)
{
  struct world_origin_iohandler *oh = world_allocator_malloc(&origin->allocator, sizeof(*oh));

  oh->base.fd = fd;
  oh->base.reader = NULL;
  oh->base.writer = _write;
  oh->base.error = NULL; // TODO
  oh->snapshot_cursor = world_hashtable_front(&origin->hashtable);
  oh->log_cursor = world_hashtable_log(&origin->hashtable);
  oh->offset = 0;
  oh->origin = origin;
  oh->thread = thread;

  return oh;
}

void world_origin_iohandler_delete(struct world_origin_iohandler *oh)
{
  world_allocator_free(&oh->origin->allocator, oh);
}

static void _write(struct world_io_handler *h)
{
  struct world_origin_iohandler *oh = (struct world_origin_iohandler *)h;

  if (world_origin_iohandler_is_updated(oh)) {
    world_origin_iothread_mark_updated(oh->thread, oh->base.fd);
    return;
  }

  const size_t n_iovecs = 64;

  struct iovec iovecs[n_iovecs];
  _fill_iovec(oh, iovecs, n_iovecs);
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

  _drain_iovec(oh, iovecs, n_iovecs, n_written);
}

static void _fill_iovec(struct world_origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs)
{
  memset(iovecs, 0, sizeof(struct iovec) * n_iovecs);

  struct world_hashtable_entry *scursor = oh->snapshot_cursor;
  struct world_hashtable_entry *lcursor = oh->log_cursor;
  for (size_t i = 0; i < n_iovecs; i++) {
    struct world_iovec iovec;
    if (scursor) {
      struct world_hashtable_entry *entry = world_hashtable_entry_advance(&scursor, lcursor->base.seq);
      if (entry) {
        iovec = world_hashtable_entry_raw(entry);
      }
    }
    if (!scursor) {
      struct world_hashtable_entry *entry = atomic_load_explicit(&lcursor->log, memory_order_relaxed);
      if (!entry) {
        break;
      }
      iovec = world_hashtable_entry_raw(entry);
      lcursor = entry;
    }
    iovecs[i].iov_base = (char *)iovec.base;
    iovecs[i].iov_len = iovec.size;
  }

  assert(iovecs[0].iov_len >= oh->offset);
  iovecs[0].iov_base = (char *)((uintptr_t)iovecs[0].iov_base + oh->offset);
  iovecs[0].iov_len -= oh->offset;
}

static void _drain_iovec(struct world_origin_iohandler *oh, struct iovec *iovecs, size_t n_iovecs, size_t n_written)
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

    struct world_hashtable_entry *lcursor = atomic_load_explicit(&oh->log_cursor, memory_order_relaxed);
    if (oh->snapshot_cursor &&
        world_hashtable_entry_advance(&oh->snapshot_cursor, lcursor->base.seq)) {
      continue;
    }
    atomic_store_explicit(&oh->log_cursor, atomic_load_explicit(&lcursor->log, memory_order_relaxed), memory_order_relaxed);
    assert(oh->log_cursor);
  }
}
