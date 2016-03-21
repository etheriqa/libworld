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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "world_hash.h"

world_hash_type world_generate_seed(void)
{
  // FIXME it may be platform dependent implementation

  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
    perror("open");
    abort();
  }

  world_hash_type seed = 0;
  if (read(fd, &seed, sizeof(seed)) == -1) {
    perror("read");
    abort();
  }

  if (close(fd) == -1) {
    perror("close");
    abort();
  }

  return seed;
}

world_hash_type world_hash(const void *data, size_t size, world_hash_type seed)
{
  // We employ MurmurHash1.
  // https://github.com/aappleby/smhasher/blob/master/src/MurmurHash1.cpp

  const world_hash_type m = 0xc6a4a793;

  world_hash_type h = seed ^ (size * m);

  while (size >= sizeof(world_hash_type)) {
    h += ((world_hash_type *)data)[0];
    h *= m;
    h ^= h >> 16;
    data = (world_hash_type *)data + 1;
    size -= sizeof(world_hash_type);
  }

  switch (size) {
  case 3:
    h += ((uint8_t *)data)[2] << 16;
  case 2:
    h += ((uint8_t *)data)[1] << 16;
  case 1:
    h += ((uint8_t *)data)[0] << 16;
    h *= m;
    h ^= h >> 16;
  }

  h *= m;
  h ^= h >> 10;
  h *= m;
  h ^= h >> 17;

  return h;
}

world_hash_type world_hash_reverse(world_hash_type hash)
{
  hash = ((hash & 0x55555555) << 1) | ((hash & 0xAAAAAAAA) >> 1);
  hash = ((hash & 0x33333333) << 2) | ((hash & 0xCCCCCCCC) >> 2);
  hash = ((hash & 0x0F0F0F0F) << 4) | ((hash & 0xF0F0F0F0) >> 4);
  hash = ((hash & 0x00FF00FF) << 8) | ((hash & 0xFF00FF00) >> 8);
  hash = ((hash & 0x0000FFFF) << 16) | ((hash & 0xFFFF0000) >> 16);
  return hash;
}
