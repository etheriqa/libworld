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

#include <stdbool.h>
#include <stddef.h>

struct world_allocator;

struct world_vector {
  struct world_allocator *allocator;
  void *base;
  size_t capacity;
  size_t size;
};

typedef bool world_vector_heap_property(const void *, const void *);

void world_vector_init(struct world_vector *v, struct world_allocator *a);
void world_vector_destroy(struct world_vector *v);
size_t world_vector_size(struct world_vector *v);
void world_vector_resize(struct world_vector *v, size_t capacity, size_t size);
void world_vector_reserve(struct world_vector *v, size_t capacity, size_t size);
void *world_vector_at(struct world_vector *v, size_t i, size_t size);
void *world_vector_front(struct world_vector *v);
void *world_vector_back(struct world_vector *v, size_t size);
void world_vector_clear(struct world_vector *v);
void world_vector_push_back(struct world_vector *restrict v, const void *restrict element, size_t size);
void world_vector_pop_back(struct world_vector *v);
void world_vector_push_heap(struct world_vector *restrict v, const void *restrict element, size_t size, world_vector_heap_property property);
void world_vector_pop_heap(struct world_vector *v, size_t size, world_vector_heap_property property);
