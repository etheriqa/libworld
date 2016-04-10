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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <world.h>
#include <worldaux.h>

int main(int argc, char **argv)
{
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    abort();
  }

  struct world_originconf conf;
  world_originconf_init(&conf);
  conf.set_tcp_nodelay = false;

  struct worldaux_server *server;
  if (worldaux_server_open(&server, "127.0.0.1", "25200", &conf) != world_error_ok) {
    abort();
  }

  struct world_origin *origin = worldaux_server_get_origin(server);

  for (;;) {
    sleep(1);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct world_buffer key, data;
    key.base = "timeval";
    key.size = strlen(key.base) + 1;
    data.base = &tv;
    data.size = sizeof(tv);
    world_origin_set(origin, key, data);
  }

  return 0;
}
