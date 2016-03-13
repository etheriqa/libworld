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

#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct circular {
  void *base;
  size_t capacity;
  size_t size;
  size_t offset;
};

static inline void circular_init(struct circular *c);
static inline void circular_destroy(struct circular *c);
static inline size_t circular_size(struct circular *c);
static inline void circular_reserve(struct circular *c, size_t capacity, size_t size);
static inline void *circular_at(struct circular *c, size_t i, size_t size);
static inline void *circular_front(struct circular *c, size_t size);
static inline void *circular_back(struct circular *c, size_t size);
static inline void circular_clear(struct circular *c);
static inline void circular_push_back(struct circular *c, const void *element, size_t size);
static inline void circular_pop_back(struct circular *c);
static inline void circular_push_front(struct circular *c, const void *element, size_t size);
static inline void circular_pop_front(struct circular *c);

static inline void circular__reserve_double(struct circular *c, size_t size);
static inline void *circular__at(struct circular *c, size_t i, size_t size);

static inline void circular_init(struct circular *c)
{
  assert(c);
  c->base = NULL;
  c->capacity = 0;
  c->size = 0;
  c->offset = 0;
}

static inline void circular_destroy(struct circular *c)
{
  assert(c);
  free(c->base);
}

static inline size_t circular_size(struct circular *c)
{
  assert(c);
  return c->size;
}

static inline void circular_reserve(struct circular *c, size_t capacity, size_t size)
{
  assert(c);
  if (capacity <= c->capacity) {
    return;
  }
  void *base = calloc(capacity, size);
  if (base == NULL) {
    perror("calloc");
    abort();
  }
  size_t leading = c->offset * size;
  size_t trailing = (c->capacity - c->offset) * size;
  memcpy(base, (void *)((uintptr_t)c->base + leading), trailing);
  memcpy((void *)((uintptr_t)base + trailing), c->base, leading);
  free(c->base);
  c->base = base;
  c->capacity = capacity;
  c->offset = 0;
}

static inline void *circular_at(struct circular *c, size_t i, size_t size)
{
  assert(c);
  if (i >= c->size) {
    return NULL;
  }
  return circular__at(c, i, size);
}

static inline void *circular_front(struct circular *c, size_t size)
{
  assert(c);
  return circular_at(c, 0, size);
}

static inline void *circular_back(struct circular *c, size_t size)
{
  assert(c);
  return circular_at(c, c->size - 1, size);
}

static inline void circular_clear(struct circular *c)
{
  assert(c);
  c->size = 0;
  c->offset = 0;
}

static inline void circular_push_back(struct circular *c, const void *element, size_t size)
{
  assert(c);
  assert(element);
  if (c->capacity == c->size) {
    circular__reserve_double(c, size);
  }
  memcpy(circular__at(c, c->size, size), element, size);
  c->size++;
}

static inline void circular_pop_back(struct circular *c)
{
  assert(c);
  assert(c->size > 0);
  c->size--;
}

static inline void circular_push_front(struct circular *c, const void *element, size_t size)
{
  assert(c);
  assert(element);
  if (c->capacity == c->size) {
    circular__reserve_double(c, size);
  }
  memcpy(circular__at(c, c->capacity - 1, size), element, size);
  c->size++;
  c->offset = (c->offset - 1 + c->capacity) % c->capacity;
}

static inline void circular_pop_front(struct circular *c)
{
  assert(c);
  assert(c->size > 0);
  c->size--;
  c->offset = (c->offset + 1) % c->capacity;
}

static inline void circular__reserve_double(struct circular *c, size_t size)
{
  assert(c);
  size_t capacity = c->capacity == 0 ? 1 : c->capacity * 2;
  circular_reserve(c, capacity, size);
}

static inline void *circular__at(struct circular *c, size_t i, size_t size)
{
  assert(c);
  assert(i < c->capacity);
  return (void *)((uintptr_t)c->base + (c->offset + i) % c->capacity * size);
}
