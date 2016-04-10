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

#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include <world.h>
#include "../../src/world_origin.h"

struct world_originconf conf;
char *unix = "world.sock";
char *host = "localhost";
char *port = "25200";
int n_io_threads = 1;
int cardinality = 4096;
int key_size = 16;
int data_size = 144;
int batch_size = 50;
int frequency = 40;
int auto_transmission = 0;

static int open_unix_socket(void);
static int open_tcp_socket(void);
static void unlink_unix_socket(void);
static void *generate_key_set(void);
static void attach_all(int fd, struct world_origin* origin);

int main(int argc, char **argv)
{
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    abort();
  }

  int c;
  while ((c = getopt(argc, argv, "abcdfhkptu")) != -1) {
    switch (c) {
    case 'a':
      auto_transmission = 1;
      break;
    case 'b':
      batch_size = atoi(argv[optind]);
      if (batch_size < 0) {
        fprintf(stderr, "batch_size (-b) should be a non-negative integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'c':
      cardinality = atoi(argv[optind]);
      if (cardinality <= 0) {
        fprintf(stderr, "cardinality (-c) should be a positive integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'd':
      data_size = atoi(argv[optind]);
      if (data_size <= 0) {
        fprintf(stderr, "data_size (-d) should be a positive integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'f':
      frequency = atoi(argv[optind]);
      if (frequency <= 0) {
        fprintf(stderr, "frequency (-f) should be a positive integer\n");
        exit(EXIT_FAILURE);
      }
      optind++;
      break;
    case 'h':
      host = argv[optind];
      optind++;
      break;
    case 'k':
      key_size = atoi(argv[optind]);
      if (key_size <= 0) {
        fprintf(stderr, "key_size (-d) should be a positive integer\n");
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

  printf("unix (-u)              ... %s\n", unix);
  printf("host (-h)              ... %s\n", host);
  printf("port (-p)              ... %s\n", port);
  printf("n_io_threads (-t)      ... %d\n", n_io_threads);
  printf("cardinality (-c)       ... %d\n", cardinality);
  printf("key_size (-k)          ... %d\n", key_size);
  printf("data_size (-d)         ... %d\n", data_size);
  printf("batch_size (-b)        ... %d\n", batch_size);
  printf("frequency (-f)         ... %d\n", frequency);
  printf("auto_transmission (-a) ... %d\n", auto_transmission);

  unlink_unix_socket();

  const void *key_buf = generate_key_set();
  const void *data_buf = malloc(data_size);
  if (data_buf == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  world_originconf_init(&conf);
  conf.n_io_threads = n_io_threads;
  conf.set_nonblocking = true;
  conf.set_tcp_nodelay = true;
  conf.auto_transmission = auto_transmission;

  struct world_origin *origin = NULL;
  if (world_origin_open(&origin, &conf) != world_error_ok) {
    fprintf(stderr, "world_origin_open: an error has been occured\n");
    exit(EXIT_FAILURE);
  }

  int ufd = open_unix_socket();
  int tfd = open_tcp_socket();

  world_sequence gseq0 = 0;
  struct timeval tv0;
  if (gettimeofday(&tv0, NULL) == -1) {
    perror("gettimeofday");
    exit(EXIT_FAILURE);
  }

  struct timespec ts;
  ts.tv_sec = tv0.tv_sec;
  ts.tv_nsec = 0;

  size_t counter = 0;
  for (;;) {
    ts.tv_sec += 1;
    ts.tv_nsec = 0;
    for (size_t i = 0; i < (size_t)frequency; i++) {
      ts.tv_nsec += 1000000000 / frequency;

      for (size_t j = 0; j < (size_t)batch_size; j++) {
        struct world_buffer key, data;
        key.base = (void *)((uintptr_t)key_buf + (counter % cardinality) * key_size);
        key.size = key_size;
        data.base = data_buf;
        data.size = data_size;
        if (world_origin_set(origin, key, data) != world_error_ok) {
          fprintf(stderr, "world_origin_set: an error has been occured\n");
          exit(EXIT_FAILURE);
        }
        counter++;
      }

      if (world_origin_transmit(origin) != world_error_ok) {
        fprintf(stderr, "world_origin_transmit: an error has been occured\n");
        exit(EXIT_FAILURE);
      }

      struct timeval tv;
      if (gettimeofday(&tv, NULL) == -1) {
        perror("gettimeofday");
        exit(EXIT_FAILURE);
      }

      struct timespec rqt, rmt;
      rqt.tv_sec = 0;
      rqt.tv_nsec = ts.tv_nsec - tv.tv_usec * 1000;
      nanosleep(&rqt, &rmt);
    }

    world_sequence lseq1 = world_hashtable_log_least_sequence(&origin->hashtable.log);
    world_sequence gseq1 = world_hashtable_log_greatest_sequence(&origin->hashtable.log);

    struct timeval tv1;
    if (gettimeofday(&tv1, NULL) == -1) {
      perror("gettimeofday");
      exit(EXIT_FAILURE);
    }

    struct world_buffer key, data;
    key.base = "timeval";
    key.size = strlen(key.base) + 1;
    data.base = &tv1;
    data.size = sizeof(tv1);
    if (world_origin_set(origin, key, data) != world_error_ok) {
      fprintf(stderr, "world_origin_set: an error has been occured\n");
      exit(EXIT_FAILURE);
    }

    float elapsed = (tv1.tv_sec - tv0.tv_sec) + (tv1.tv_usec - tv0.tv_usec) * 1e-6f;
    world_sequence opps = (gseq1 - gseq0) / elapsed;

    printf("lseq = %"PRIu64"\tgseq = %"PRIu64"\tdelay = %"PRIu64"\t%"PRIu64" operations/s\n",
      lseq1,
      gseq1,
      gseq1 - lseq1,
      opps);

    attach_all(ufd, origin);
    attach_all(tfd, origin);

    gseq0 = gseq1;
    tv0 = tv1;
  }

  return EXIT_SUCCESS;
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

  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }

  if (bind(fd, (struct sockaddr *)&sun, sizeof(sun)) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(fd, 1024) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return fd;
}

static int open_tcp_socket(void)
{
  struct addrinfo hint, *ai;
  memset(&hint, 0, sizeof(hint));
  hint.ai_flags = AI_PASSIVE;
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

  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  if (bind(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  freeaddrinfo(ai);

  if (listen(fd, 1024) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  return fd;
}

static void unlink_unix_socket(void)
{
  if (unlink(unix) == -1) {
    perror("unlink");
  }
}

static void *generate_key_set(void)
{
  void *buf = calloc(cardinality, key_size);
  if (buf == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  ssize_t n_read = read(fd, buf, cardinality * key_size);
  if (n_read != cardinality * key_size) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  close(fd);

  return buf;
}

static void attach_all(int fd, struct world_origin* origin)
{
  for (;;) {
    int afd = accept(fd, NULL, NULL);
    if (afd == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      perror("accept");
      exit(EXIT_FAILURE);
    } else {
      world_origin_attach(origin, afd);
    }
  }
}
