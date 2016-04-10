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

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include "world_hashtable_entry.h"
#include "world_io.h"

struct world_origin;
struct world_origin_thread;

struct world_origin_handler {
  struct world_io_handler base;

  struct world_hashtable_entry *snapshot_cursor;
  _Atomic(struct world_hashtable_entry *)log_cursor;

  size_t offset;

  struct world_origin *origin;
  struct world_origin_thread *thread;
};

struct world_origin_handler *world_origin_handler_new(struct world_origin *origin, struct world_origin_thread *thread, int fd);
void world_origin_handler_delete(struct world_origin_handler *oh);
world_sequence world_origin_handler_sequence(struct world_origin_handler *oh);
