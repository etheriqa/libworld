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

#include "../../src/world_allocator.h"
#include "../../src/world_vector.h"
#include "../helper.h"

static void test_vector_manipulation(void)
{
  struct world_allocator allocator;
  world_allocator_init(&allocator);

  struct world_vector v;
  world_vector_init(&v, &allocator);

  EXPECT(world_vector_size(&v) == 0);

  int a = 2;
  world_vector_push_back(&v, &a, sizeof(int));
  EXPECT(world_vector_size(&v) == 1);
  EXPECT(*(int *)world_vector_at(&v, 0, sizeof(int)) == a);

  int b = 3;
  world_vector_push_back(&v, &b, sizeof(int));
  EXPECT(world_vector_size(&v) == 2);
  EXPECT(*(int *)world_vector_at(&v, 0, sizeof(int)) == a);
  EXPECT(*(int *)world_vector_at(&v, 1, sizeof(int)) == b);

  int c = 5;
  world_vector_push_back(&v, &c, sizeof(int));
  EXPECT(world_vector_size(&v) == 3);
  EXPECT(*(int *)world_vector_at(&v, 0, sizeof(int)) == a);
  EXPECT(*(int *)world_vector_at(&v, 1, sizeof(int)) == b);
  EXPECT(*(int *)world_vector_at(&v, 2, sizeof(int)) == c);

  world_vector_pop_back(&v);
  EXPECT(world_vector_size(&v) == 2);
  EXPECT(*(int *)world_vector_at(&v, 0, sizeof(int)) == a);
  EXPECT(*(int *)world_vector_at(&v, 1, sizeof(int)) == b);

  int d = 7;
  world_vector_push_back(&v, &d, sizeof(int));
  EXPECT(world_vector_size(&v) == 3);
  EXPECT(*(int *)world_vector_at(&v, 0, sizeof(int)) == a);
  EXPECT(*(int *)world_vector_at(&v, 1, sizeof(int)) == b);
  EXPECT(*(int *)world_vector_at(&v, 2, sizeof(int)) == d);

  for (size_t i = 0; i < 1000000; i++) {
    int value = 0;
    world_vector_push_back(&v, &value, sizeof(int));
  }
  EXPECT(world_vector_size(&v) == 1000003);

  for (size_t i = 0; i < 1000000; i++) {
    world_vector_pop_back(&v);
  }
  EXPECT(world_vector_size(&v) == 3);

  int product = 1;
  for (size_t i = 0; i < world_vector_size(&v); i++) {
    product *= *(int *)world_vector_at(&v, i, sizeof(int));
  }
  EXPECT(product == 42);

  world_vector_clear(&v);
  EXPECT(world_vector_size(&v) == 0);

  world_vector_destroy(&v);
}

static bool maxheap_property(const void *x, const void *y)
{
  return (*(const int *)x) >= (*(const int *)y);
}

static void test_heap_manipulation(void)
{
  struct world_allocator allocator;
  world_allocator_init(&allocator);

  struct world_vector v;
  world_vector_init(&v, &allocator);

  int b = 3;
  world_vector_push_heap(&v, &b, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == b);

  int c = 5;
  world_vector_push_heap(&v, &c, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == c);

  int a = 2;
  world_vector_push_heap(&v, &a, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == c);

  world_vector_pop_heap(&v, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == b);

  int d = 7;
  world_vector_push_heap(&v, &d, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == d);

  world_vector_pop_heap(&v, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == b);

  world_vector_pop_heap(&v, sizeof(int), maxheap_property);
  EXPECT(*(int *)world_vector_front(&v) == a);

  world_vector_pop_heap(&v, sizeof(int), maxheap_property);
  EXPECT(world_vector_front(&v) == NULL);

  world_vector_destroy(&v);
}

int main(void)
{
  test_vector_manipulation();
  test_heap_manipulation();
  return TEST_STATUS;
}
