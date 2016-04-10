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

#include <stdint.h>
#include <string.h>
#include "world_allocator.h"
#include "world_assert.h"
#include "world_vector.h"

static void _reserve_double(struct world_vector *v, size_t size);
static void *_at(struct world_vector *v, size_t i, size_t size);
static void _swap(struct world_vector *v, size_t i, size_t j, size_t size);

void world_vector_init(struct world_vector *v, struct world_allocator *a)
{
  v->allocator = a;
  v->base = NULL;
  v->capacity = 0;
  v->size = 0;
}

void world_vector_destroy(struct world_vector *v)
{
  world_allocator_free(v->allocator, v->base);
}

size_t world_vector_size(struct world_vector *v)
{
  return v->size;
}

void world_vector_resize(struct world_vector *v, size_t capacity, size_t size)
{
  // FIXME
  if (capacity > v->size) {
    world_vector_reserve(v, capacity, size);
    memset(_at(v, v->size, size), 0, (capacity - v->size) * size);
  }
  v->size = capacity;
}

void world_vector_reserve(struct world_vector *v, size_t capacity, size_t size)
{
  if (capacity <= v->capacity) {
    return;
  }
  v->base = world_allocator_realloc(v->allocator, v->base, capacity * size);
  v->capacity = capacity;
}

void *world_vector_at(struct world_vector *v, size_t i, size_t size)
{
  if (i >= v->size) {
    return NULL;
  }
  return _at(v, i, size);
}

void *world_vector_front(struct world_vector *v)
{
  if (v->size == 0) {
    return NULL;
  }
  return v->base;
}

void *world_vector_back(struct world_vector *v, size_t size)
{
  if (v->size == 0) {
    return NULL;
  }
  return _at(v, v->size - 1, size);
}

void world_vector_clear(struct world_vector *v)
{
  v->size = 0;
}

void world_vector_push_back(struct world_vector *restrict v, const void *restrict element, size_t size)
{
  if (v->capacity == v->size) {
    _reserve_double(v, size);
  }
  memcpy(_at(v, v->size, size), element, size);
  v->size++;
}

void world_vector_pop_back(struct world_vector *v)
{
  WORLD_ASSERT(v->size > 0);
  v->size--;
}

void world_vector_push_heap(struct world_vector *restrict v, const void *restrict element, size_t size, world_vector_heap_property *property)
{
  world_vector_push_back(v, element, size);
  size_t child = v->size - 1;
  while (child > 0) {
    size_t parent = (child - 1) / 2;
    if (!property(_at(v, parent, size), _at(v, child, size))) {
      _swap(v, parent, child, size);
    }
    child = parent;
  }
}

void world_vector_pop_heap(struct world_vector *v, size_t size, world_vector_heap_property property)
{
  _swap(v, 0, v->size - 1, size);
  world_vector_pop_back(v);
  size_t parent = 0;
  for (;;) {
    size_t left = 2 * parent + 1;
    size_t right = 2 * parent + 2;
    bool left_violated =
      left < v->size && !property(_at(v, parent, size), _at(v, left, size));
    bool right_violated =
      right < v->size && !property(_at(v, parent, size), _at(v, right, size));
    if (left_violated && right_violated) {
      if (property(_at(v, left, size), _at(v, right, size))) {
        _swap(v, parent, left, size);
        parent = left;
      } else {
        _swap(v, parent, right, size);
        parent = right;
      }
    } else if (left_violated) {
      _swap(v, parent, left, size);
      parent = left;
    } else if (right_violated) {
      _swap(v, parent, right, size);
      parent = right;
    } else {
      return;
    }
  }
}

static void _reserve_double(struct world_vector *v, size_t size)
{
  size_t capacity = v->capacity == 0 ? 1 : v->capacity * 2;
  world_vector_reserve(v, capacity, size);
}

static void *_at(struct world_vector *v, size_t i, size_t size)
{
  return (void *)((uintptr_t)v->base + i * size);
}

static void _swap(struct world_vector *v, size_t i, size_t j, size_t size)
{
  if (i == j) {
    return;
  }
  if (v->size == v->capacity) {
    _reserve_double(v, size);
  }
  void *tmp = _at(v, v->capacity - 1, size);
  void *x = _at(v, i, size);
  void *y = _at(v, j, size);
  memcpy(tmp, x, size);
  memcpy(x, y, size);
  memcpy(y, tmp, size);
}
