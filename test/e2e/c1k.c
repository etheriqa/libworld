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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <world.h>
#include "../helper.h"

#if defined(WORLD_USE_SELECT) || defined(WORLD_USE_POLL)
#define WORLD_TEST_E2E_N_REPLICAS 16
#else
#define WORLD_TEST_E2E_N_REPLICAS 1024
#endif

int main(void)
{
  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
    perror("getrlimit");
    abort();
  }
  rl.rlim_cur = 3 * WORLD_TEST_E2E_N_REPLICAS + 16; // XXX see world_generate_hash()
  if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
    perror("setrlimit");
    abort();
  }

  struct world_originconf oc;
  oc.n_io_threads = 4;
  world_originconf_init(&oc);
  struct world_origin *origin = NULL;
  ASSERT(world_origin_open(&origin, &oc) == world_error_ok);

  struct world_iovec key, data;
  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;

  ASSERT(world_origin_add(origin, key, data) == world_error_ok);

  struct world_replica *replicas[WORLD_TEST_E2E_N_REPLICAS];
  for (size_t i = 0; i < WORLD_TEST_E2E_N_REPLICAS; i++) {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
      perror("socketpair");
      abort();
    }

    struct world_replicaconf rc;
    world_replicaconf_init(&rc);
    rc.fd = fds[0];
    ASSERT(world_replica_open(&replicas[i], &rc) == world_error_ok);
    ASSERT(world_origin_attach(origin, fds[1]) == world_error_ok);
  }

  world_test_sleep_msec(100);

  for (size_t i = 0; i < WORLD_TEST_E2E_N_REPLICAS; i++) {
    struct world_iovec found;
    memset(&found, 0, sizeof(found));
    ASSERT(world_replica_get(replicas[i], key, &found) == world_error_ok);
    ASSERT(found.size == data.size);
    ASSERT(memcmp(found.base, data.base, data.size) == 0);
  }

  key.base = "bar";
  key.size = strlen(key.base) + 1;
  data.base = "dolor sit amet";
  data.size = strlen(data.base) + 1;

  ASSERT(world_origin_add(origin, key, data) == world_error_ok);

  world_test_sleep_msec(100);

  for (size_t i = 0; i < WORLD_TEST_E2E_N_REPLICAS; i++) {
    struct world_iovec found;
    memset(&found, 0, sizeof(found));
    ASSERT(world_replica_get(replicas[i], key, &found) == world_error_ok);
    ASSERT(found.size == data.size);
    ASSERT(memcmp(found.base, data.base, data.size) == 0);
  }

  return TEST_STATUS;
}
