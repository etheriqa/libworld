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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <world.h>
#include <worldaux.h>

static void callback(struct world_buffer key, struct world_buffer data);

int main(int argc, char **argv)
{
  struct world_replicaconf conf;
  world_replicaconf_init(&conf);
  conf.callback = callback;

  struct worldaux_client *client;
  if (worldaux_client_open(&client, "127.0.0.1", "25200", &conf) != world_error_ok) {
    abort();
  }

  for (;;) {
    sleep(1);
  }

  return 0;
}

static void callback(struct world_buffer key, struct world_buffer data)
{
  if (strlen("timeval") + 1 != key.size ||
      sizeof(struct timeval) != data.size ||
      memcmp("timeval", key.base, key.size) != 0) {
    return;
  }

  struct timeval tv_server, tv_client, tv_diff;
  memcpy(&tv_server, data.base, data.size);
  gettimeofday(&tv_client, NULL);
  tv_diff.tv_sec = tv_client.tv_sec - tv_server.tv_sec;
  tv_diff.tv_usec = tv_client.tv_usec - tv_server.tv_usec + 1000000;
  if (tv_diff.tv_usec >= 1000000) {
    tv_diff.tv_usec -= 1000000;
  } else {
    tv_diff.tv_sec -= 1;
  }
  printf("server = %ld.%06ld client = %ld.%06ld diff = %ld.%06ld\n",
    tv_server.tv_sec, (long)tv_server.tv_usec,
    tv_client.tv_sec, (long)tv_client.tv_usec,
    tv_diff.tv_sec, (long)tv_diff.tv_usec);
}
