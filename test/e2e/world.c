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

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <world.h>
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
  oc.set_tcp_nodelay = false; // TODO consider removing this

  struct world_replicaconf rc;
  world_replicaconf_init(&rc);
  rc.fd = fds[0];

  struct world_origin *origin = world_origin_open(&oc);
  struct world_replica *replica = world_replica_open(&rc);

  struct world_iovec key, data, found;
  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;

  ASSERT_TRUE(world_origin_set(origin, key, data));
  ASSERT_FALSE(world_replica_get(replica, key, NULL));

  ASSERT_TRUE(world_origin_attach(origin, fds[1]));

  sleep(1);

  ASSERT_TRUE(world_replica_get(replica, key, &found));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  key.base = "bar";
  key.size = strlen(key.base) + 1;
  data.base = "dolor sit amet";
  data.size = strlen(data.base) + 1;

  ASSERT_TRUE(world_origin_set(origin, key, data));;

  sleep(1);

  ASSERT_TRUE(world_replica_get(replica, key, &found));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  world_origin_close(origin);
  world_replica_close(replica);

  return TEST_STATUS;
}