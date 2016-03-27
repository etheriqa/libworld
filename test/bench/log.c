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

#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <world.h>
#include "../../src/world_allocator.h"
#include "../../src/world_io.h"
#include "../../src/world_origin.h"
#include "../helper.h"

static struct world_origin *_open_origin(size_t n_io_threads)
{
  struct world_originconf oc;
  world_originconf_init(&oc);
  oc.n_io_threads = n_io_threads;
  struct world_origin *origin = NULL;
  ASSERT(world_origin_open(&origin, &oc) == world_error_ok);
  return origin;
}

struct _replica {
  struct world_io_handler base;
  _Atomic(size_t) n_read;
};

static void _replica_read(struct world_io_handler *h)
{
  struct _replica *replica = (struct _replica *)h;

  char buf[4096];
  ssize_t n_read = read(replica->base.fd, buf, sizeof(buf));
  if (n_read == -1) {
    perror("read");
    abort();
  }

  size_t n_read_total = atomic_load_explicit(&replica->n_read, memory_order_relaxed);
  atomic_store_explicit(&replica->n_read, n_read_total + n_read, memory_order_relaxed);
}

static void _replica_init(struct _replica *replica, int fd)
{
  replica->base.fd = fd;
  replica->base.reader = _replica_read;
  replica->base.writer = NULL;
  replica->base.error = NULL;
  atomic_store_explicit(&replica->n_read, 0, memory_order_relaxed);
}

static void *_multiplexer_main(void *arg)
{
  struct world_io_multiplexer *multiplexer = arg;

  for (;;) {
    world_io_multiplexer_dispatch(multiplexer);
  }

  return NULL;
}

struct _reporter {
  struct world_origin *origin;
  struct _replica *replicas;
  size_t n_replicas;
  size_t message_size;
};

static void *_reporter_main(void *arg)
{
  struct _reporter *r = arg;

  world_sequence seq_0 = 0;
  size_t n_read_0 = 0;
  struct timeval t_0;
  if (gettimeofday(&t_0, NULL) == -1) {
    perror("gettimeofday");
    abort();
  }

  for (;;) {
    sleep(10);
    world_sequence lseq = world_hashtable_log_least_sequence(&r->origin->hashtable.log);
    world_sequence gseq = world_hashtable_log_greatest_sequence(&r->origin->hashtable.log);
    world_sequence seq_1 = lseq;
    size_t n_read_1 = 0;
    for (size_t i = 0; i < r->n_replicas; i++) {
      n_read_1 += atomic_load_explicit(&r->replicas[i].n_read, memory_order_relaxed);
    }
    struct timeval t_1;
    if (gettimeofday(&t_1, NULL) == -1) {
      perror("gettimeofday");
      abort();
    }
    world_sequence delta_seq = seq_1 - seq_0;
    size_t delta_n_read = n_read_1 - n_read_0;
    double delta_t = (t_1.tv_sec - t_0.tv_sec) + (t_1.tv_usec - t_0.tv_usec) * 1e-6;
    printf("seq %"PRIu64"-%"PRIu64", %.1f MB/s, %.3f M messages/s, %.3f K op/s\n",
      lseq,
      gseq,
      delta_n_read / delta_t * 1e-6,
      delta_n_read / delta_t * 1e-6 / r->message_size,
      delta_seq / delta_t * 1e-3);

    seq_0 = seq_1;
    n_read_0 = n_read_1;
    t_0 = t_1;
  }

  return NULL;
}

static const void *_generate_key(size_t count, size_t size)
{
  void *buf = calloc(count, size);
  if (!buf) {
    perror("calloc");
    abort();
  }

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
    perror("open");
    abort();
  }

  ssize_t n_read = read(fd, buf, count * size);
  if ((size_t)n_read != count * size) {
    perror("read");
    abort();
  }

  close(fd);

  return buf;
}

int main(int argc, char **argv)
{
  char *socket_type = "unix";
  char *port = "25200";
  size_t n_io_threads = 1;
  size_t n_replicas = 16;
  size_t cardinality = 65536;
  size_t key_size = 16;
  size_t data_size = 64;
  size_t watermark = 1024;

  {
    char c;
    while ((c = getopt(argc, argv, "cdkprstw")) != -1) {
      switch (c) {
      case 'c':
        cardinality = atoi(argv[optind]);
        optind++;
        break;
      case 'd':
        data_size = atoi(argv[optind]);
        optind++;
        break;
      case 'k':
        key_size = atoi(argv[optind]);
        optind++;
        break;
      case 'p':
        port = argv[optind];
        optind++;
        break;
      case 'r':
        n_replicas = atoi(argv[optind]);
        optind++;
        break;
      case 't':
        n_io_threads = atoi(argv[optind]);
        optind++;
        break;
      case 's':
        socket_type = argv[optind];
        optind++;
        break;
      case 'w':
        watermark = atoi(argv[optind]);
        optind++;
        break;
      }
    }
  }

  if (strcmp(socket_type, "unix") != 0 && strcmp(socket_type, "tcp") != 0) {
    printf("Unknown socket type: %s\n", socket_type);
    exit(EXIT_FAILURE);
  }

  printf("socket type (unix|tcp) (-s) ... %s\n", socket_type);
  printf("port                   (-p) ... %s\n", port);
  printf("# of I/O threads       (-t) ... %zu\n", n_io_threads);
  printf("# of replicas          (-r) ... %zu\n", n_replicas);
  printf("cardinality            (-c) ... %zu\n", cardinality);
  printf("key size               (-k) ... %zu\n", key_size);
  printf("data size              (-d) ... %zu\n", data_size);
  printf("watermark              (-w) ... %zu\n", watermark);

  struct world_allocator allocator;
  world_allocator_init(&allocator);

  struct world_io_multiplexer *multiplexers = calloc(n_io_threads, sizeof(*multiplexers));
  if (multiplexers == NULL) {
    perror("calloc");
    abort();
  }
  for (size_t i = 0; i < n_io_threads; i++) {
    world_io_multiplexer_init(multiplexers + i, &allocator);
  }

  struct world_origin *origin = _open_origin(n_io_threads);

  struct _replica *replicas = calloc(n_replicas, sizeof(*replicas));
  if (replicas == NULL) {
    perror("calloc");
    abort();
  }

  if (strcmp(socket_type, "unix") == 0) {
    for (size_t i = 0; i < n_replicas; i++) {
      int fds[2];
      if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
        perror("socketpair");
        abort();
      }
      world_origin_attach(origin, fds[1]);
      _replica_init(&replicas[i], fds[0]);
      world_io_multiplexer_attach(&multiplexers[i % n_io_threads], &replicas[i].base);
    }
  }

  if (strcmp(socket_type, "tcp") == 0) {
    struct addrinfo hint, *ai;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo("127.0.0.1", port, &hint, &ai);
    if (err) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
      abort();
    }

    int listen_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (listen_fd == -1) {
      perror("socket");
      abort();
    }
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
      perror("setsockopt");
      abort();
    }
    if (bind(listen_fd, ai->ai_addr, ai->ai_addrlen) == -1) {
      perror("bind");
      abort();
    }
    if (listen(listen_fd, n_replicas) == -1) {
      perror("listen");
      abort();
    }

    for (size_t i = 0; i < n_replicas; i++) {
      int replica_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
      if (replica_fd == -1) {
        perror("socket");
        abort();
      }
      if (connect(replica_fd, ai->ai_addr, ai->ai_addrlen) == -1) {
        perror("connect");
        abort();
      }
      int origin_fd = accept(listen_fd, NULL, NULL);
      world_origin_attach(origin, origin_fd);
      _replica_init(&replicas[i], replica_fd);
      world_io_multiplexer_attach(&multiplexers[i % n_io_threads], &replicas[i].base);
    }
  }

  for (size_t i = 0; i < n_io_threads; i++) {
    int err;
    pthread_t t;
    if ((err = pthread_create(&t, NULL, _multiplexer_main, &multiplexers[i]))) {
      fprintf(stderr, "pthread_create: %s\n", strerror(err));
      abort();
    }
  }

  struct _reporter r;
  r.origin = origin;
  r.replicas = replicas;
  r.n_replicas = n_replicas;
  r.message_size = sizeof(world_key_size) + sizeof(world_data_size) + key_size + data_size;
  {
    int err;
    pthread_t t;
    if ((err = pthread_create(&t, NULL, _reporter_main, &r))) {
      fprintf(stderr, "pthread_create: %s\n", strerror(err));
      abort();
    }
  }

  const void *key_buf = _generate_key(cardinality, key_size);
  const void *data_buf = malloc(data_size);
  if (data_buf == NULL) {
    perror("malloc");
    abort();
  }

  for (size_t i = 0;; i++) {
    world_sequence lseq = world_hashtable_log_least_sequence(&origin->hashtable.log);
    world_sequence gseq = world_hashtable_log_greatest_sequence(&origin->hashtable.log);
    if (gseq - lseq >= watermark) {
      world_test_sleep_msec(1);
    }
    struct world_iovec key, data;
    key.base = (void *)((uintptr_t)key_buf + (i % cardinality) * key_size);
    key.size = key_size;
    data.base = data_buf;
    data.size = data_size;
    world_origin_set(origin, key, data);
  }

  return TEST_STATUS;
}
