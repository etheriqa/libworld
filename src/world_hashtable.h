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
#include <world.h>
#include "world_hash.h"
#include "world_hashtable_bucket.h"
#include "world_hashtable_log.h"
#include "world_mutex.h"
#include "world_vector.h"

struct world_allocator;
struct world_hashtable_entry;

struct world_hashtable {
  struct world_allocator *allocator;
  struct world_mutex mtx;
  struct world_hashtable_bucket bucket;
  struct world_hashtable_log log;
  struct world_vector garbages;
  world_hash_type seed;
  size_t n_fresh_entries;
};

void world_hashtable_init(struct world_hashtable *ht, world_hash_type seed, struct world_allocator *a);
void world_hashtable_destroy(struct world_hashtable *ht);
enum world_error world_hashtable_get(struct world_hashtable *ht, struct world_iovec key, struct world_iovec *found);
enum world_error world_hashtable_set(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data);
enum world_error world_hashtable_add(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data);
enum world_error world_hashtable_replace(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data);
enum world_error world_hashtable_delete(struct world_hashtable *ht, struct world_iovec key);
struct world_hashtable_entry *world_hashtable_front(struct world_hashtable *ht);
struct world_hashtable_entry *world_hashtable_log(struct world_hashtable *ht);
void world_hashtable_checkpoint(struct world_hashtable *ht, world_sequence seq);
