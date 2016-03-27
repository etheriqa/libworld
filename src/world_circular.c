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

#include <stdint.h>
#include <string.h>
#include "world_allocator.h"
#include "world_assert.h"
#include "world_circular.h"

static void _reserve_double(struct world_circular *c, size_t size);
static void *_at(struct world_circular *c, size_t i, size_t size);

void world_circular_init(struct world_circular *c, struct world_allocator *a)
{
  c->allocator = a;
  c->base = NULL;
  c->capacity = 0;
  c->size = 0;
  c->offset = 0;
}

void world_circular_destroy(struct world_circular *c)
{
  world_allocator_free(c->allocator, c->base);
}

size_t world_circular_size(struct world_circular *c)
{
  return c->size;
}

void world_circular_reserve(struct world_circular *c, size_t capacity, size_t size)
{
  if (capacity <= c->capacity) {
    return;
  }
  void *base = world_allocator_calloc(c->allocator, capacity, size);
  size_t leading = c->offset * size;
  size_t trailing = (c->capacity - c->offset) * size;
  memcpy(base, (void *)((uintptr_t)c->base + leading), trailing);
  memcpy((void *)((uintptr_t)base + trailing), c->base, leading);
  world_allocator_free(c->allocator, c->base);
  c->base = base;
  c->capacity = capacity;
  c->offset = 0;
}

void *world_circular_at(struct world_circular *c, size_t i, size_t size)
{
  if (i >= c->size) {
    return NULL;
  }
  return _at(c, i, size);
}

void *world_circular_front(struct world_circular *c, size_t size)
{
  return world_circular_at(c, 0, size);
}

void *world_circular_back(struct world_circular *c, size_t size)
{
  return world_circular_at(c, c->size - 1, size);
}

void world_circular_clear(struct world_circular *c)
{
  c->size = 0;
  c->offset = 0;
}

void world_circular_push_back(struct world_circular *restrict c, const void *restrict element, size_t size)
{
  if (c->capacity == c->size) {
    _reserve_double(c, size);
  }
  memcpy(_at(c, c->size, size), element, size);
  c->size++;
}

void world_circular_pop_back(struct world_circular *c)
{
  WORLD_ASSERT(c->size > 0);
  c->size--;
}

void world_circular_push_front(struct world_circular *restrict c, const void *restrict element, size_t size)
{
  if (c->capacity == c->size) {
    _reserve_double(c, size);
  }
  memcpy(_at(c, c->capacity - 1, size), element, size);
  c->size++;
  c->offset = (c->offset - 1 + c->capacity) % c->capacity;
}

void world_circular_pop_front(struct world_circular *c)
{
  WORLD_ASSERT(c->size > 0);
  c->size--;
  c->offset = (c->offset + 1) % c->capacity;
}

static void _reserve_double(struct world_circular *c, size_t size)
{
  size_t capacity = c->capacity == 0 ? 1 : c->capacity * 2;
  world_circular_reserve(c, capacity, size);
}

static void *_at(struct world_circular *c, size_t i, size_t size)
{
  WORLD_ASSERT(i < c->capacity);
  return (void *)((uintptr_t)c->base + (c->offset + i) % c->capacity * size);
}
