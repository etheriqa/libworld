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

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include "world_hashtable_entry.h"
#include "world_io.h"

struct world_origin;
struct world_origin_iothread;

struct world_origin_iohandler {
  struct world_io_handler base;

  struct world_hashtable_entry *snapshot_cursor;
  _Atomic(struct world_hashtable_entry *)log_cursor;

  size_t offset;

  struct world_origin *origin;
  struct world_origin_iothread *thread;
};

struct world_origin_iohandler *world_origin_iohandler_new(struct world_origin *origin, struct world_origin_iothread *thread, int fd);
void world_origin_iohandler_delete(struct world_origin_iohandler *oh);

static inline world_sequence world_origin_iohandler_sequence(struct world_origin_iohandler *oh)
{
  return atomic_load_explicit(&oh->log_cursor, memory_order_relaxed)->base.seq;
}

static inline bool world_origin_iohandler_is_updated(struct world_origin_iohandler *oh)
{
  struct world_hashtable_entry *lcursor = atomic_load_explicit(&oh->log_cursor, memory_order_relaxed);

  return oh->offset == 0 &&
         oh->snapshot_cursor == NULL &&
         atomic_load_explicit(&lcursor->log, memory_order_relaxed) == NULL;
}
