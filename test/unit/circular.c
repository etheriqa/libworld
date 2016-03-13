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

#include "../../src/circular.h"
#include "../helper.h"

static void test_circular_manipulation(void)
{
  struct circular c;
  circular_init(&c);

  EXPECT(circular_size(&c) == 0);

  int a = 2;
  circular_push_back(&c, &a, sizeof(int));
  EXPECT(circular_size(&c) == 1);
  EXPECT(*(int *)circular_at(&c, 0, sizeof(int)) == a);

  int b = 3;
  circular_push_front(&c, &b, sizeof(int));
  EXPECT(circular_size(&c) == 2);
  EXPECT(*(int *)circular_at(&c, 0, sizeof(int)) == b);
  EXPECT(*(int *)circular_at(&c, 1, sizeof(int)) == a);

  int ccccc = 5;
  circular_push_back(&c, &ccccc, sizeof(int));
  EXPECT(circular_size(&c) == 3);
  EXPECT(*(int *)circular_at(&c, 0, sizeof(int)) == b);
  EXPECT(*(int *)circular_at(&c, 1, sizeof(int)) == a);
  EXPECT(*(int *)circular_at(&c, 2, sizeof(int)) == ccccc);

  int d = 7;
  circular_push_front(&c, &d, sizeof(int));
  EXPECT(circular_size(&c) == 4);
  EXPECT(*(int *)circular_at(&c, 0, sizeof(int)) == d);
  EXPECT(*(int *)circular_at(&c, 1, sizeof(int)) == b);
  EXPECT(*(int *)circular_at(&c, 2, sizeof(int)) == a);
  EXPECT(*(int *)circular_at(&c, 3, sizeof(int)) == ccccc);

  circular_pop_back(&c);
  EXPECT(circular_size(&c) == 3);
  EXPECT(*(int *)circular_at(&c, 0, sizeof(int)) == d);
  EXPECT(*(int *)circular_at(&c, 1, sizeof(int)) == b);
  EXPECT(*(int *)circular_at(&c, 2, sizeof(int)) == a);

  for (size_t i = 0; i < 500000; i++) {
    int value = 0;
    circular_push_back(&c, &value, sizeof(int));
  }
  for (size_t i = 0; i < 500000; i++) {
    int value = 0;
    circular_push_front(&c, &value, sizeof(int));
  }
  EXPECT(circular_size(&c) == 1000003);

  for (size_t i = 0; i < 500000; i++) {
    circular_pop_back(&c);
  }
  for (size_t i = 0; i < 500000; i++) {
    circular_pop_front(&c);
  }
  EXPECT(circular_size(&c) == 3);

  int product = 1;
  for (size_t i = 0; i < circular_size(&c); i++) {
    product *= *(int *)circular_at(&c, i, sizeof(int));
  }
  EXPECT(product == 42);

  circular_clear(&c);
  EXPECT(circular_size(&c) == 0);

  circular_destroy(&c);
}

int main(void)
{
  test_circular_manipulation();
  return TEST_STATUS;
}
