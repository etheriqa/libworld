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
#include "world_allocator.h"
#include "world_assert.h"

void world_allocator_init(struct world_allocator *a)
{
  WORLD_ASSERT(a);
  // do nothing for now
}

void world_allocator_destroy(struct world_allocator *a)
{
  WORLD_ASSERT(a);
  // do nothing for now
}

void *world_allocator_malloc(struct world_allocator *a, size_t size)
{
  WORLD_ASSERT(a);
  void *p = malloc(size);
  if (p == NULL) {
    perror("malloc");
    abort();
  }
  return p;
}

void *world_allocator_calloc(struct world_allocator *a, size_t count, size_t size)
{
  WORLD_ASSERT(a);
  void *p = calloc(count, size);
  if (p == NULL) {
    perror("calloc");
    abort();
  }
  return p;
}

void *world_allocator_realloc(struct world_allocator *restrict a, void *restrict ptr, size_t size)
{
  WORLD_ASSERT(a);
  void *p = realloc(ptr, size);
  if (p == NULL) {
    perror("realloc");
    abort();
  }
  return p;
}

void world_allocator_free(struct world_allocator *restrict a, void *restrict ptr)
{
  WORLD_ASSERT(a);
  free(ptr);
}
