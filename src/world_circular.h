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

#pragma once

#include <stddef.h>

struct world_allocator;

struct world_circular {
  struct world_allocator *allocator;
  void *base;
  size_t capacity;
  size_t size;
  size_t offset;
};

void world_circular_init(struct world_circular *c, struct world_allocator *a);
void world_circular_destroy(struct world_circular *c);
size_t world_circular_size(struct world_circular *c);
void world_circular_reserve(struct world_circular *c, size_t capacity, size_t size);
void *world_circular_at(struct world_circular *c, size_t i, size_t size);
void *world_circular_front(struct world_circular *c, size_t size);
void *world_circular_back(struct world_circular *c, size_t size);
void world_circular_clear(struct world_circular *c);
void world_circular_push_back(struct world_circular *restrict c, const void *restrict element, size_t size);
void world_circular_pop_back(struct world_circular *c);
void world_circular_push_front(struct world_circular *restrict c, const void *restrict element, size_t size);
void world_circular_pop_front(struct world_circular *c);
