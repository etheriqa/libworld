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

#include <string.h>
#include <worldaux.h>
#include "../helper.h"

int main(void)
{
  struct worldaux_server *server;
  ASSERT(worldaux_server_open(&server, "127.0.0.1", "25200") == world_error_ok);

  struct worldaux_client *client;
  ASSERT(worldaux_client_open(&client, "127.0.0.1", "25200") == world_error_ok);

  struct world_origin *origin = worldaux_server_get_origin(server);
  struct world_iovec key, data;
  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;
  ASSERT(world_origin_set(origin, key, data) == world_error_ok);

  world_test_sleep_msec(100);

  struct world_replica *replica = worldaux_client_get_replica(client);
  struct world_iovec found;
  ASSERT(world_replica_get(replica, key, &found) == world_error_ok);
  ASSERT(data.size == found.size);
  ASSERT(memcmp(data.base, found.base, found.size) == 0);

  ASSERT(worldaux_server_close(server) == world_error_ok);
  ASSERT(worldaux_client_close(client) == world_error_ok);

  return TEST_STATUS;
}
