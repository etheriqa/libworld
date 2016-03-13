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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct mutex {
  pthread_mutex_t handler;
};

static inline void mutex_init(struct mutex *mtx)
{
  int err = pthread_mutex_init(&mtx->handler, NULL);
  if (err) {
    fprintf(stderr, "pthread_mutex_init: %s\n", strerror(err));
    abort();
  }
}

static inline void mutex_destroy(struct mutex *mtx)
{
  int err = pthread_mutex_destroy(&mtx->handler);
  if (err) {
    fprintf(stderr, "pthread_mutex_destroy: %s\n", strerror(err));
  }
}

static inline void mutex_lock(struct mutex *mtx)
{
  int err = pthread_mutex_lock(&mtx->handler);
  if (err) {
    fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(err));
    abort();
  }
}

static inline void mutex_unlock(struct mutex *mtx)
{
  int err = pthread_mutex_unlock(&mtx->handler);
  if (err) {
    fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(err));
    abort();
  }
}
