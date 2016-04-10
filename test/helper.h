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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int TEST_STATUS = 0;

#define EXPECT(expression)                                                     \
  do {                                                                         \
    if (!(expression)) {                                                       \
      fprintf(stderr, "%s:%d:\n\t%s\n", __FILE__, __LINE__, #expression);      \
      TEST_STATUS = 1;                                                         \
    }                                                                          \
  } while (0)                                                                  \

#define EXPECT_TRUE(expression) EXPECT(expression == true)
#define EXPECT_FALSE(expression) EXPECT(expression == false)

#define ASSERT(expression)                                                     \
  do {                                                                         \
    if (!(expression)) {                                                       \
      fprintf(stderr, "%s:%d:\n\t%s\n", __FILE__, __LINE__, #expression);      \
      abort();                                                                 \
    }                                                                          \
  } while (0)                                                                  \

#define ASSERT_TRUE(expression) ASSERT(expression == true)
#define ASSERT_FALSE(expression) ASSERT(expression == false)

static inline void world_test_sleep_msec(size_t msec)
{
  struct timespec t;
  t.tv_sec = 0;
  t.tv_nsec = msec * 1000000;
  nanosleep(&t, NULL);
}
