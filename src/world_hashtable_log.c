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

#include <stdatomic.h>
#include <stdlib.h>
#include "world_assert.h"
#include "world_hashtable_entry.h"
#include "world_hashtable_log.h"

void world_hashtable_log_init(struct world_hashtable_log *l, struct world_allocator *a)
{
  struct world_buffer key;
  key.base = NULL;
  key.size = 0;
  struct world_hashtable_entry *sentinel = world_hashtable_entry_new_void(a, 0, key);
  l->head = sentinel;
  l->sentinel = sentinel;
  atomic_store_explicit(&l->tail, sentinel, memory_order_relaxed);
}

void world_hashtable_log_destroy(struct world_hashtable_log *l, struct world_allocator *a)
{
  world_hashtable_entry_delete(l->sentinel, a);
}

void world_hashtable_log_push_back(struct world_hashtable_log *l, struct world_hashtable_entry *entry)
{
  struct world_hashtable_entry *tail = atomic_load_explicit(&l->tail, memory_order_relaxed);
  WORLD_ASSERT(tail);
  entry->base.seq = tail->base.seq + 1;
  atomic_thread_fence(memory_order_release);
  atomic_store_explicit(&tail->log, entry, memory_order_relaxed);
  atomic_store_explicit(&l->tail, entry, memory_order_relaxed);
}

void world_hashtable_log_pop_front(struct world_hashtable_log *l)
{
  struct world_hashtable_entry *head = atomic_load_explicit(&l->head->log, memory_order_relaxed);
  WORLD_ASSERT(head);
  l->head = head;
}

world_sequence world_hashtable_log_least_sequence(struct world_hashtable_log *l)
{
  return world_hashtable_log_front(l)->base.seq;
}

world_sequence world_hashtable_log_greatest_sequence(struct world_hashtable_log *l)
{
  return world_hashtable_log_back(l)->base.seq;
}

struct world_hashtable_entry *world_hashtable_log_front(struct world_hashtable_log *l)
{
  return l->head;
}

struct world_hashtable_entry *world_hashtable_log_back(struct world_hashtable_log *l)
{
  return atomic_load_explicit(&l->tail, memory_order_relaxed);
}
