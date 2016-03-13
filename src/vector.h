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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct vector {
  void *base;
  size_t capacity;
  size_t size;
};

typedef bool (*vector_heap_property)(const void *, const void *);

static inline void vector_init(struct vector *v);
static inline void vector_destroy(struct vector *v);
static inline size_t vector_size(struct vector *v);
static inline void vector_reserve(struct vector *v, size_t capacity, size_t size);
static inline void *vector_at(struct vector *v, size_t i, size_t size);
static inline void *vector_front(struct vector *v);
static inline void *vector_back(struct vector *v, size_t size);
static inline void vector_clear(struct vector *v);
static inline void vector_push_back(struct vector *v, const void *element, size_t size);
static inline void vector_pop_back(struct vector *v);
static inline void vector_push_heap(struct vector *v, const void *element, size_t size, vector_heap_property property);
static inline void vector_pop_heap(struct vector *v, size_t size, vector_heap_property property);

static inline void vector__reserve_double(struct vector *v, size_t size);
static inline void *vector__at(struct vector *v, size_t i, size_t size);
static inline void vector__swap(struct vector *v, size_t i, size_t j, size_t size);

static inline void vector_init(struct vector *v)
{
  assert(v);
  v->base = NULL;
  v->capacity = 0;
  v->size = 0;
}

static inline void vector_destroy(struct vector *v)
{
  assert(v);
  free(v->base);
}

static inline size_t vector_size(struct vector *v)
{
  assert(v);
  return v->size;
}

static inline void vector_reserve(struct vector *v, size_t capacity, size_t size)
{
  assert(v);
  if (capacity <= v->capacity) {
    return;
  }
  v->base = realloc(v->base, capacity * size);
  if (v->base == NULL) {
    perror("realloc");
    abort();
  }
  v->capacity = capacity;
}

static inline void *vector_at(struct vector *v, size_t i, size_t size)
{
  assert(v);
  if (i >= v->size) {
    return NULL;
  }
  return vector__at(v, i, size);
}

static inline void *vector_front(struct vector *v)
{
  assert(v);
  if (v->size == 0) {
    return NULL;
  }
  return v->base;
}

static inline void *vector_back(struct vector *v, size_t size)
{
  assert(v);
  if (v->size == 0) {
    return NULL;
  }
  return vector__at(v, v->size - 1, size);
}

static inline void vector_clear(struct vector *v)
{
  assert(v);
  v->size = 0;
}

static inline void vector_push_back(struct vector *v, const void *element, size_t size)
{
  assert(v);
  assert(element);
  if (v->capacity == v->size) {
    vector__reserve_double(v, size);
  }
  memcpy(vector__at(v, v->size, size), element, size);
  v->size++;
}

static inline void vector_pop_back(struct vector *v)
{
  assert(v);
  assert(v->size > 0);
  v->size--;
}

static inline void vector_push_heap(struct vector *v, const void *element, size_t size, vector_heap_property property)
{
  assert(v);
  assert(element);
  assert(property);
  vector_push_back(v, element, size);
  size_t child = v->size - 1;
  while (child > 0) {
    size_t parent = (child - 1) / 2;
    if (!property(vector__at(v, parent, size), vector__at(v, child, size))) {
      vector__swap(v, parent, child, size);
    }
    child = parent;
  }
}

static inline void vector_pop_heap(struct vector *v, size_t size, vector_heap_property property)
{
  assert(v);
  assert(property);
  vector__swap(v, 0, v->size - 1, size);
  vector_pop_back(v);
  size_t parent = 0;
  for (;;) {
    size_t left = 2 * parent + 1;
    size_t right = 2 * parent + 2;
    bool left_violated =
      left < v->size &&
      !property(vector__at(v, parent, size), vector__at(v, left, size));
    bool right_violated =
      right < v->size &&
      !property(vector__at(v, parent, size), vector__at(v, right, size));
    if (left_violated && right_violated) {
      if (property(vector__at(v, left, size), vector__at(v, right, size))) {
        vector__swap(v, parent, left, size);
        parent = left;
      } else {
        vector__swap(v, parent, right, size);
        parent = right;
      }
    } else if (left_violated) {
      vector__swap(v, parent, left, size);
      parent = left;
    } else if (right_violated) {
      vector__swap(v, parent, right, size);
      parent = right;
    } else {
      return;
    }
  }
}

static inline void vector__reserve_double(struct vector *v, size_t size)
{
  assert(v);
  size_t capacity = v->capacity == 0 ? 1 : v->capacity * 2;
  vector_reserve(v, capacity, size);
}

static inline void *vector__at(struct vector *v, size_t i, size_t size)
{
  assert(v);
  return (void *)((uintptr_t)v->base + i * size);
}

static inline void vector__swap(struct vector *v, size_t i, size_t j, size_t size)
{
  assert(v);
  if (i == j) {
    return;
  }
  if (v->size == v->capacity) {
    vector__reserve_double(v, size);
  }
  void *tmp = vector__at(v, v->capacity - 1, size);
  void *x = vector__at(v, i, size);
  void *y = vector__at(v, j, size);
  memcpy(tmp, x, size);
  memcpy(x, y, size);
  memcpy(y, tmp, size);
}
