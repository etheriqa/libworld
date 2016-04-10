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

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <worldaux.h>
#include "world_allocator.h"
#include "world_origin.h"
#include "worldaux_server.h"

static void *_accept_main(void *arg);

struct world_origin *worldaux_server_get_origin(struct worldaux_server *server)
{
  return server->origin;
}

enum world_error worldaux_server_open(struct worldaux_server **s, const char *host, const char *port, const struct world_originconf *conf)
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
    freeaddrinfo(ai);
    return world_error_system;
  }

  {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
      perror("setsockopt");
      // go through
    }
  }

  if (bind(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    perror("bind");
    close(fd);
    freeaddrinfo(ai);
    return world_error_system;
  }

  freeaddrinfo(ai);

  if (listen(fd, 16) == -1) {
    perror("listen");
    close(fd);
    return world_error_system;
  }

  struct world_originconf oc;
  if (conf) {
    memcpy(&oc, conf, sizeof(oc));
  } else {
    world_originconf_init(&oc);
  }

  struct world_origin *origin;
  enum world_error err = world_origin_open(&origin, &oc);
  if (err) {
    close(fd);
    return err;
  }

  struct worldaux_server *server = world_allocator_malloc(&origin->allocator, sizeof(server));
  server->origin = origin;
  server->fd = fd;

  {
    int err = pthread_create(&server->thread, NULL, _accept_main, server);
    if (err) {
      world_allocator_free(&origin->allocator, server);
      world_origin_close(origin);
      close(fd);
      return world_error_system;
    }
  }

  *s = server;
  return world_error_ok;
}

enum world_error worldaux_server_close(struct worldaux_server *s)
{
  // XXX check errors if needed
  struct world_origin *origin = s->origin;
  int fd = s->fd;
  pthread_cancel(s->thread);
  world_allocator_free(&origin->allocator, s);
  world_origin_close(origin);
  close(fd);
  return world_error_ok;
}

static void *_accept_main(void *arg)
{
  struct worldaux_server *server = arg;

  for (;;) {
    int fd = accept(server->fd, NULL, NULL);
    if (fd == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      continue;
    }
    world_origin_attach(server->origin, fd); // XXX handle an error
  }

  return NULL;
}
