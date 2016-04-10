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
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <world.h>
#include "../../src/world_allocator.h"
#include "../../src/world_io.h"

struct world_replicaconf conf;
char *mode = "tcp";
char *unix = "world.sock";
char *host = "localhost";
char *port = "25200";
int n_io_threads = 1;
int n_connections = 16;

_Atomic(size_t) total_read = 0;
size_t total_read0 = 0;
size_t total_read1 = 0;
size_t n_messages = 0;
size_t n_messages0 = 0;
size_t n_messages1 = 0;

struct timeval tv0, tv1;

static int open_socket(void);
static int open_unix_socket(void);
static int open_tcp_socket(void);
static void replica_callback(struct world_buffer key, struct world_buffer data);
static void reader(struct world_io_handler *handler);
static void error(struct world_io_handler *handler);
static void *dispatcher(void *arg);

int main(int argc, char **argv)
{
  int c;
  while ((c = getopt(argc, argv, "chmptu")) != -1) {
    switch (c) {
    case 'c':
      n_connections = atoi(argv[optind]);
      if (n_connections <= 0) {
        fprintf(stderr, "n_connections (-c) should be positive integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'h':
      host = argv[optind];
      optind++;
      break;
    case 'm':
      mode = argv[optind];
      if (strcmp(mode, "unix") != 0 && strcmp(mode, "tcp") != 0) {
        fprintf(stderr, "mode (-m) should be either unix or tcp\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'p':
      port = argv[optind];
      optind++;
      break;
    case 't':
      n_io_threads = atoi(argv[optind]);
      if (n_io_threads <= 0) {
        fprintf(stderr, "n_io_threads (-t) should be a positive integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'u':
      unix = argv[optind];
      optind++;
      break;
    default:
      fprintf(stderr, "invalid argument\n");
      exit(EXIT_FAILURE);
      break;
    }
  }

  printf("mode (-m)          ... %s\n", mode);
  printf("unix (-u)          ... %s\n", unix);
  printf("host (-h)          ... %s\n", host);
  printf("port (-p)          ... %s\n", port);
  printf("n_io_threads (-t)  ... %d\n", n_io_threads);
  printf("n_connections (-c) ... %d\n", n_connections);

  if (gettimeofday(&tv0, NULL) == -1) {
    perror("gettimeofday");
    exit(EXIT_FAILURE);
  }

  world_replicaconf_init(&conf);
  conf.fd = open_socket();
  conf.callback = replica_callback;

  struct world_replica *replica;
  if (world_replica_open(&replica, &conf) != world_error_ok) {
    fprintf(stderr, "world_replica_open: an error has been occured\n");
    exit(EXIT_FAILURE);
  }

  struct world_allocator allocator;
  world_allocator_init(&allocator);
  struct world_io_multiplexer multiplexer;
  world_io_multiplexer_init(&multiplexer, &allocator);

  pthread_t thread;
  int err = pthread_create(&thread, NULL, dispatcher, &multiplexer);
  if (err) {
    fprintf(stderr, "pthread_create: %s\n", strerror(err));
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < n_connections; i++) {
    struct world_io_handler *handler = world_allocator_malloc(&allocator, sizeof(*handler));
    handler->fd = open_socket();
    handler->reader = reader;
    handler->writer = NULL;
    handler->error = error;
    world_io_multiplexer_attach(&multiplexer, handler);
  }

  pthread_join(thread, NULL);

  return EXIT_SUCCESS;
}

static int open_socket(void)
{
  if (strcmp(mode, "unix") == 0) {
    return open_unix_socket();
  }
  if (strcmp(mode, "tcp") == 0) {
    return open_tcp_socket();
  }
  return -1;
}

static int open_unix_socket(void)
{
  struct sockaddr_un sun;
  memset(&sun, 0, sizeof(sun));
  sun.sun_family = AF_UNIX;
  size_t path_size = strlen(unix) + 1;
  if (path_size > sizeof(sun.sun_path)) {
    fprintf(stderr, "unix path is too long\n");
    exit(EXIT_FAILURE);
  }
  memcpy(&sun.sun_path, unix, path_size);

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (connect(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  return fd;
}

static int open_tcp_socket(void)
{
  struct addrinfo hint, *ai;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  int err = getaddrinfo(host, port,  &hint, &ai);
  if (err) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
    exit(EXIT_FAILURE);
  }

  int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (fd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  if (connect(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(ai);

  return fd;
}

static void replica_callback(struct world_buffer key, struct world_buffer data)
{
  total_read += sizeof(world_key_size) + sizeof(world_data_size) + key.size + data.size;
  n_messages++;

  if (strlen("timeval") + 1 != key.size ||
      sizeof(struct timeval) != data.size ||
      memcmp("timeval", key.base, key.size) != 0) {
    return;
  }

  if (gettimeofday(&tv1, NULL) == -1) {
    perror("gettimeofday");
    exit(EXIT_FAILURE);
  }
  total_read1 = total_read;
  n_messages1 = n_messages;

  struct timeval tv_server;
  memcpy(&tv_server, data.base, data.size);
  float latency = (tv1.tv_sec - tv_server.tv_sec) + (tv1.tv_usec - tv_server.tv_usec) * 1e-6f;
  float elapsed = (tv1.tv_sec - tv0.tv_sec) + (tv1.tv_usec - tv0.tv_usec) * 1e-6f;

  printf("latency = %.1f ms\tthroughput = %.1f Mbps\t%.1f K messages/s\n",
    latency * 1e3f,
    8 * (total_read1 - total_read0) / elapsed * 1e-6f,
    (n_messages1 - n_messages0) / elapsed * n_connections * 1e-3f);

  total_read0 = total_read1;
  n_messages0 = n_messages1;
  tv0 = tv1;
}

static void reader(struct world_io_handler *handler)
{
  char buf[4096];
  ssize_t n_read = read(handler->fd, buf, sizeof(buf));
  if (n_read == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }
  total_read += n_read;
}

static void error(struct world_io_handler *handler)
{
  exit(EXIT_FAILURE);
}

static void *dispatcher(void *arg)
{
  struct world_io_multiplexer *multiplexer = arg;
  for (;;) {
    world_io_multiplexer_dispatch(multiplexer);
  }
  return NULL;
}
