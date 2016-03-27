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

#include <world.h>
#include "world_hash.h"

struct world_allocator;
struct world_hashtable_entry;

struct world_hashtable_log {
  struct world_hashtable_entry *head;
  _Atomic(struct world_hashtable_entry *)tail;
  struct world_hashtable_entry *sentinel;
};

void world_hashtable_log_init(struct world_hashtable_log *l, struct world_allocator *a);
void world_hashtable_log_destroy(struct world_hashtable_log *l, struct world_allocator *a);
void world_hashtable_log_push_back(struct world_hashtable_log *l, struct world_hashtable_entry *entry);
void world_hashtable_log_pop_front(struct world_hashtable_log *l);
world_sequence world_hashtable_log_least_sequence(struct world_hashtable_log *l);
world_sequence world_hashtable_log_greatest_sequence(struct world_hashtable_log *l);
struct world_hashtable_entry *world_hashtable_log_front(struct world_hashtable_log *l);
struct world_hashtable_entry *world_hashtable_log_back(struct world_hashtable_log *l);
