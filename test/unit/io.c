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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../../src/world_io.h"
#include "../helper.h"

struct handler {
  struct world_io_handler base;
  size_t n_called_reader;
  size_t n_called_writer;
};

static void reader(struct world_io_handler *h)
{
  struct handler *handler = (struct handler *)h;
  handler->n_called_reader++;
}

static void writer(struct world_io_handler *h)
{
  struct handler *handler = (struct handler *)h;
  handler->n_called_writer++;
}

static void test_world_io_multiplex(void)
{
  int fds[2];
  if (pipe(fds) == -1) {
    perror("pipe");
    abort();
  }

  struct handler read_handler;
  read_handler.base.fd = fds[0];
  read_handler.base.reader = reader;
  read_handler.base.writer = NULL;
  read_handler.base.error = NULL;
  read_handler.n_called_reader = 0;
  read_handler.n_called_writer = 0;

  struct handler write_handler;
  write_handler.base.fd = fds[1];
  write_handler.base.reader = NULL;
  write_handler.base.writer = writer;
  write_handler.base.error = NULL;
  write_handler.n_called_reader = 0;
  write_handler.n_called_writer = 0;

  struct world_io_multiplexer multiplexer;
  world_io_multiplexer_init(&multiplexer);
  world_io_multiplexer_attach(&multiplexer, &read_handler.base);
  world_io_multiplexer_attach(&multiplexer, &write_handler.base);

  world_io_multiplexer_dispatch(&multiplexer);
  EXPECT(read_handler.n_called_reader == 0);
  EXPECT(read_handler.n_called_writer == 0);
  EXPECT(write_handler.n_called_reader == 0);
  EXPECT(write_handler.n_called_writer == 1);

  world_io_multiplexer_dispatch(&multiplexer);
  EXPECT(read_handler.n_called_reader == 0);
  EXPECT(read_handler.n_called_writer == 0);
  EXPECT(write_handler.n_called_reader == 0);
  EXPECT(write_handler.n_called_writer == 2);

  const char data[] = "Lorem ipsum";
  if (write(fds[1], data, sizeof(data)) == -1) {
    perror("write");
    abort();
  }

  world_io_multiplexer_dispatch(&multiplexer);
  EXPECT(read_handler.n_called_reader == 1);
  EXPECT(read_handler.n_called_writer == 0);
  EXPECT(write_handler.n_called_reader == 0);
  EXPECT(write_handler.n_called_writer == 3);

  char c;
  if (read(fds[0], &c, sizeof(c)) == -1) {
    perror("read");
    abort();
  }

  world_io_multiplexer_dispatch(&multiplexer);
  EXPECT(read_handler.n_called_reader == 2);
  EXPECT(read_handler.n_called_writer == 0);
  EXPECT(write_handler.n_called_reader == 0);
  EXPECT(write_handler.n_called_writer == 4);

  world_io_multiplexer_detach(&multiplexer, &read_handler.base);

  world_io_multiplexer_dispatch(&multiplexer);
  EXPECT(read_handler.n_called_reader == 2);
  EXPECT(read_handler.n_called_writer == 0);
  EXPECT(write_handler.n_called_reader == 0);
  EXPECT(write_handler.n_called_writer == 5);

  world_io_multiplexer_destroy(&multiplexer);
}

int main(void)
{
  test_world_io_multiplex();
  return TEST_STATUS;
}
