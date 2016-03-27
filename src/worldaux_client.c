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

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <worldaux.h>
#include "world_allocator.h"
#include "world_replica.h"
#include "worldaux_client.h"

struct world_replica *worldaux_client_get_replica(struct worldaux_client *client)
{
  return client->replica;
}

enum world_error worldaux_client_open(struct worldaux_client **c, const char *host, const char *port)
{
  struct addrinfo hint, *ai;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  {
    int err = getaddrinfo(host, port, &hint, &ai);
    if (err) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
      return world_error_system;
    }
  }

  int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (fd == -1) {
    perror("socket");
    return world_error_system;
  }

  if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    perror("connect");
    close(fd);
    return world_error_system;
  }

  struct world_replicaconf conf;
  world_replicaconf_init(&conf);
  conf.fd = fd;

  struct world_replica *replica;
  enum world_error err = world_replica_open(&replica, &conf);
  if (err) {
    close(fd);
    return err;
  }

  struct worldaux_client *client = world_allocator_malloc(&replica->allocator, sizeof(client));
  client->replica = replica;

  *c = client;
  return world_error_ok;
}

enum world_error worldaux_client_close(struct worldaux_client *c)
{
  // XXX check errors if needed
  struct world_replica *replica = c->replica;
  world_allocator_free(&replica->allocator, c);
  world_replica_close(replica);
  return world_error_ok;
}
