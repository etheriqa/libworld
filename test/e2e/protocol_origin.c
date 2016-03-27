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

  struct world_originconf oc;
  world_originconf_init(&oc);

  struct world_origin *origin;
  ASSERT(world_origin_open(&origin, &oc) == world_error_ok);
  ASSERT(world_origin_attach(origin, fds[1]) == world_error_ok);

  struct world_iovec key, data;
  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;
  ASSERT(world_origin_set(origin, key, data) == world_error_ok);

  world_test_sleep_msec(100);

  uint8_t buf[4096];
  ssize_t n_read = read(fds[0], buf, sizeof(buf));
  if (n_read == -1) {
    perror("read");
    abort();
  }


  ASSERT(n_read == 20);

  union {
    world_key_size raw;
    uint8_t buf[2];
  } key_size;
  memcpy(key_size.buf, buf, sizeof(key_size));
  EXPECT(world_decode_key_size(key_size.raw) == key.size);

  union {
    world_data_size raw;
    uint8_t buf[2];
  } data_size;
  memcpy(data_size.buf, &buf[2], sizeof(data_size));
  EXPECT(world_decode_data_size(data_size.raw) == data.size);

  EXPECT(memcmp(&buf[4], key.base, key.size) == 0);
  EXPECT(memcmp(&buf[8], data.base, data.size) == 0);

  return TEST_STATUS;
}
