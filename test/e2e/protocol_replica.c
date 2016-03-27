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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <world.h>
#include "../../src/world_byteorder.h"
#include "../helper.h"

int main(void)
{
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    abort();
  }

  struct world_replicaconf rc;
  world_replicaconf_init(&rc);
  rc.fd = fds[0];

  struct world_replica *replica;
  ASSERT(world_replica_open(&replica, &rc) == world_error_ok);

  struct world_iovec key, data;
  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;

  uint8_t buf[4096];

  union {
    world_key_size raw;
    uint8_t buf[2];
  } key_size;
  key_size.raw = world_encode_key_size(key.size);
  memcpy(&buf[0], key_size.buf, sizeof(key_size));

  union {
    world_data_size raw;
    uint8_t buf[2];
  } data_size;
  data_size.raw = world_encode_data_size(data.size);
  memcpy(&buf[2], data_size.buf, sizeof(data_size));

  memcpy(&buf[4], key.base, key.size);
  memcpy(&buf[8], data.base, data.size);

  ssize_t n_written = write(fds[1], buf, 20);
  if (n_written == -1) {
    perror("write");
    abort();
  }

  world_test_sleep_msec(100);

  struct world_iovec found;
  ASSERT(world_replica_get(replica, key, &found) == world_error_ok);
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  return TEST_STATUS;
}
