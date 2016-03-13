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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include "byteorder.h"
#include "hashtable.h"
#include "replica.h"
#include "replica_iohandler.h"

static void replica_iohandler__reader_proxy(struct io_handler *h);
static void replica_iohandler__read(struct replica_iohandler *rh);
static void replica_iohandler__reserve_body_buffer(struct replica_iohandler *rh);
static size_t replica_iohandler__get_key_size(struct replica_iohandler *rh);
static size_t replica_iohandler__get_data_size(struct replica_iohandler *rh);
static size_t replica_iohandler__get_body_size(struct replica_iohandler *rh);
static bool replica_iohandler__key_size_is_filled(struct replica_iohandler *rh);
static bool replica_iohandler__data_size_is_filled(struct replica_iohandler *rh);
static bool replica_iohandler__header_is_filled(struct replica_iohandler *rh);
static bool replica_iohandler__body_is_filled(struct replica_iohandler *rh);
static void replica_iohandler__fill_iovec(struct replica_iohandler *rh, struct iovec iovecs[3]);
static void replica_iohandler__drain_iovec(struct replica_iohandler *rh, struct iovec iovecs[3], size_t n_read);
static void replica_iohandler__apply_log(struct replica_iohandler *rh);

void replica_iohandler_init(struct replica_iohandler *rh, struct world_replica *replica)
{
  rh->base.fd = replica->conf.fd;
  rh->base.reader = replica_iohandler__reader_proxy;
  rh->base.writer = NULL;
  rh->key_size.buffer = 0;
  rh->key_size.offset = 0;
  rh->data_size.buffer = 0;
  rh->data_size.offset = 0;
  rh->body.buffer = NULL;
  rh->body.capacity = 0;
  rh->body.offset = 0;
  rh->replica = replica;
}

void replica_iohandler_destroy(struct replica_iohandler *rh)
{
  free(rh->body.buffer);
}

static void replica_iohandler__reader_proxy(struct io_handler *h)
{
  replica_iohandler__read((struct replica_iohandler *)h);
}

static void replica_iohandler__read(struct replica_iohandler *rh)
{
  replica_iohandler__reserve_body_buffer(rh);

  struct iovec iovecs[3];
  memset(iovecs, 0, sizeof(iovecs));
  replica_iohandler__fill_iovec(rh, iovecs);

  ssize_t n_read = readv(rh->replica->conf.fd, iovecs, 3);
  if (n_read == -1) {
    if (errno == EAGAIN) {
      return;
    }
    // TODO handle errors
    perror("readv");
    abort();
  }

  replica_iohandler__drain_iovec(rh, iovecs, n_read);
}

static void replica_iohandler__reserve_body_buffer(struct replica_iohandler *rh)
{
  if (!replica_iohandler__header_is_filled(rh)) {
    return;
  }
  size_t body_size = replica_iohandler__get_body_size(rh);
  if (rh->body.capacity >= body_size) {
    return;
  }
  rh->body.buffer = realloc(rh->body.buffer, body_size);
  if (rh->body.buffer == NULL) {
    perror("realloc");
    abort();
  }
}

static size_t replica_iohandler__get_key_size(struct replica_iohandler *rh)
{
  assert(replica_iohandler__key_size_is_filled(rh));
  return decode_key_size(rh->key_size.buffer);
}

static size_t replica_iohandler__get_data_size(struct replica_iohandler *rh)
{
  assert(replica_iohandler__data_size_is_filled(rh));
  return decode_key_size(rh->data_size.buffer);
}

static size_t replica_iohandler__get_body_size(struct replica_iohandler *rh)
{
  return replica_iohandler__get_key_size(rh) + replica_iohandler__get_data_size(rh);
}

static bool replica_iohandler__key_size_is_filled(struct replica_iohandler *rh)
{
  return rh->key_size.offset == sizeof(rh->key_size.buffer);
}

static bool replica_iohandler__data_size_is_filled(struct replica_iohandler *rh)
{
  return rh->data_size.offset == sizeof(rh->data_size.buffer);
}

static bool replica_iohandler__header_is_filled(struct replica_iohandler *rh)
{
  return replica_iohandler__key_size_is_filled(rh) &&
         replica_iohandler__data_size_is_filled(rh);
}

static bool replica_iohandler__body_is_filled(struct replica_iohandler *rh)
{
  return rh->body.offset && rh->body.offset == replica_iohandler__get_body_size(rh);
}

static void replica_iohandler__fill_iovec(struct replica_iohandler *rh, struct iovec iovecs[3])
{
  iovecs[1].iov_base = &rh->key_size.buffer;
  iovecs[1].iov_len = sizeof(rh->key_size.buffer);
  iovecs[2].iov_base = &rh->data_size.buffer;
  iovecs[2].iov_len = sizeof(rh->data_size.buffer);

  if (replica_iohandler__header_is_filled(rh)) {
    iovecs[0].iov_base = (void *)((uintptr_t)rh->body.buffer + rh->body.offset);
    iovecs[0].iov_len = replica_iohandler__get_body_size(rh) - rh->body.offset;
  } else {
    iovecs[1].iov_base = (void *)((uintptr_t)iovecs[1].iov_base + rh->key_size.offset);
    iovecs[1].iov_len -= rh->key_size.offset;
    iovecs[2].iov_base = (void *)((uintptr_t)iovecs[2].iov_base + rh->data_size.offset);
    iovecs[2].iov_len -= rh->data_size.offset;
  }
}

static void replica_iohandler__drain_iovec(struct replica_iohandler *rh, struct iovec iovecs[3], size_t n_read)
{
  size_t n_read_body = iovecs[0].iov_len < n_read ? iovecs[0].iov_len : n_read;
  rh->body.offset += n_read_body;
  n_read -= n_read_body;

  if (replica_iohandler__body_is_filled(rh)) {
    replica_iohandler__apply_log(rh);
    rh->key_size.offset = 0;
    rh->data_size.offset = 0;
    rh->body.offset = 0;
  }

  size_t n_read_key_size = iovecs[1].iov_len < n_read ? iovecs[1].iov_len : n_read;
  rh->key_size.offset += n_read_key_size;
  n_read -= n_read_key_size;

  size_t n_read_data_size = iovecs[2].iov_len < n_read ? iovecs[2].iov_len : n_read;
  rh->data_size.offset += n_read_data_size;
  n_read -= n_read_data_size;

  assert(!n_read);
}

static void replica_iohandler__apply_log(struct replica_iohandler *rh)
{
  assert(replica_iohandler__body_is_filled(rh));

  size_t key_size = replica_iohandler__get_key_size(rh);
  size_t data_size = replica_iohandler__get_data_size(rh);

  struct world_iovec key;
  key.base = rh->body.buffer;
  key.size = key_size;
  if (data_size) {
    struct world_iovec data;
    data.base = (void *)((uintptr_t)rh->body.buffer + key_size);
    data.size = data_size;
    hashtable_set(&rh->replica->hashtable, key, data);
  } else {
    hashtable_delete(&rh->replica->hashtable, key);
  }
}
