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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include "world_assert.h"
#include "world_byteorder.h"
#include "world_hashtable.h"
#include "world_hashtable_entry.h"
#include "world_replica.h"
#include "world_replica_handler.h"

static void _replica_io_reader(struct world_io_handler *h);
static void _replica_io_error(struct world_io_handler *h);
static void _reserve_body_buffer(struct world_replica_handler *rh);
static size_t _key_size(struct world_replica_handler *rh);
static size_t _data_size(struct world_replica_handler *rh);
static size_t _body_size(struct world_replica_handler *rh);
static bool _key_size_is_filled(struct world_replica_handler *rh);
static bool _data_size_is_filled(struct world_replica_handler *rh);
static bool _header_is_filled(struct world_replica_handler *rh);
static bool _body_is_filled(struct world_replica_handler *rh);
static void _fill_iovec(struct world_replica_handler *rh, struct iovec iovecs[3]);
static void _drain_iovec(struct world_replica_handler *rh, struct iovec iovecs[3], size_t n_read);
static void _apply_log(struct world_replica_handler *rh);

void world_replica_handler_init(struct world_replica_handler *rh, struct world_replica *replica)
{
  rh->base.fd = replica->conf.fd;
  rh->base.reader = _replica_io_reader;
  rh->base.writer = NULL;
  rh->base.error = _replica_io_error;
  rh->key_size.buffer = 0;
  rh->key_size.offset = 0;
  rh->data_size.buffer = 0;
  rh->data_size.offset = 0;
  rh->body.buffer = NULL;
  rh->body.capacity = 0;
  rh->body.offset = 0;
  rh->replica = replica;
}

void world_replica_handler_destroy(struct world_replica_handler *rh)
{
  world_allocator_free(&rh->replica->allocator, rh->body.buffer);
}

static void _replica_io_reader(struct world_io_handler *h)
{
  struct world_replica_handler *rh = (struct world_replica_handler *)h;

  _reserve_body_buffer(rh);

  struct iovec iovecs[3];
  memset(iovecs, 0, sizeof(iovecs));
  _fill_iovec(rh, iovecs);

  ssize_t n_read = readv(rh->replica->conf.fd, iovecs, 3);
  if (n_read == 0) {
    _replica_io_error(h);
    return;
  } else if (n_read == -1) {
    if (errno == EINTR || errno == EAGAIN) {
      return;
    }
    perror("readv");
    _replica_io_error(h);
    return;
  }

  _drain_iovec(rh, iovecs, n_read);
}

static void _replica_io_error(struct world_io_handler *h)
{
  struct world_replica_handler *rh = (struct world_replica_handler *)h;

  world_replica_thread_stop(&rh->replica->thread);
}

static void _reserve_body_buffer(struct world_replica_handler *rh)
{
  if (!_header_is_filled(rh)) {
    return;
  }
  size_t body_size = _body_size(rh);
  if (rh->body.capacity >= body_size) {
    return;
  }
  rh->body.buffer = world_allocator_realloc(&rh->replica->allocator, rh->body.buffer, body_size);
}

static size_t _key_size(struct world_replica_handler *rh)
{
  WORLD_ASSERT(_key_size_is_filled(rh));
  return world_decode_key_size(rh->key_size.buffer);
}

static size_t _data_size(struct world_replica_handler *rh)
{
  WORLD_ASSERT(_data_size_is_filled(rh));
  return world_decode_data_size(rh->data_size.buffer);
}

static size_t _body_size(struct world_replica_handler *rh)
{
  return _key_size(rh) + _data_size(rh);
}

static bool _key_size_is_filled(struct world_replica_handler *rh)
{
  return rh->key_size.offset == sizeof(rh->key_size.buffer);
}

static bool _data_size_is_filled(struct world_replica_handler *rh)
{
  return rh->data_size.offset == sizeof(rh->data_size.buffer);
}

static bool _header_is_filled(struct world_replica_handler *rh)
{
  return _key_size_is_filled(rh) &&
         _data_size_is_filled(rh);
}

static bool _body_is_filled(struct world_replica_handler *rh)
{
  return rh->body.offset && rh->body.offset == _body_size(rh);
}

static void _fill_iovec(struct world_replica_handler *rh, struct iovec iovecs[3])
{
  if (_header_is_filled(rh)) {
    iovecs[2].iov_base = (void *)((uintptr_t)rh->body.buffer + rh->body.offset);
    iovecs[2].iov_len = _body_size(rh) - rh->body.offset;
  } else {
    iovecs[0].iov_base = (void *)((uintptr_t)&rh->key_size.buffer + rh->key_size.offset);
    iovecs[0].iov_len = sizeof(rh->key_size.buffer) - rh->key_size.offset;
    iovecs[1].iov_base = (void *)((uintptr_t)&rh->data_size.buffer + rh->data_size.offset);
    iovecs[1].iov_len = sizeof(rh->data_size.buffer) - rh->data_size.offset;
  }
}

static void _drain_iovec(struct world_replica_handler *rh, struct iovec iovecs[3], size_t n_read)
{
  size_t n_read_key_size = iovecs[0].iov_len < n_read ? iovecs[0].iov_len : n_read;
  rh->key_size.offset += n_read_key_size;
  n_read -= n_read_key_size;

  size_t n_read_data_size = iovecs[1].iov_len < n_read ? iovecs[1].iov_len : n_read;
  rh->data_size.offset += n_read_data_size;
  n_read -= n_read_data_size;

  size_t n_read_body = iovecs[2].iov_len < n_read ? iovecs[2].iov_len : n_read;
  rh->body.offset += n_read_body;
  n_read -= n_read_body;

  if (_body_is_filled(rh)) {
    _apply_log(rh);
    rh->key_size.offset = 0;
    rh->data_size.offset = 0;
    rh->body.offset = 0;
  }

  WORLD_ASSERT(n_read == 0);
}

static void _apply_log(struct world_replica_handler *rh)
{
  WORLD_ASSERT(_body_is_filled(rh));

  size_t key_size = _key_size(rh);
  size_t data_size = _data_size(rh);

  struct world_buffer key, data;
  key.base = rh->body.buffer;
  key.size = key_size;
  if (data_size) {
    data.base = (void *)((uintptr_t)rh->body.buffer + key_size);
    data.size = data_size;
    world_hashtable_set(&rh->replica->hashtable, key, data);
  } else {
    data.base = NULL;
    data.size = 0;
    world_hashtable_delete(&rh->replica->hashtable, key);
  }

  world_sequence seq = world_hashtable_log(&rh->replica->hashtable)->base.seq;
  world_hashtable_checkpoint(&rh->replica->hashtable, seq, NULL);

  if (rh->replica->conf.callback) {
    rh->replica->conf.callback(key, data);
  }
}
