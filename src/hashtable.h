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
#include "circular.h"
#include "hash.h"
#include "vector.h"

struct hashtable;
struct hashtable_iterator;

void hashtable_init(struct hashtable *ht);
void hashtable_destroy(struct hashtable *ht);
static inline world_sequence hashtable_get_sequence(struct hashtable *ht);
struct world_iovec hashtable_get_iovec(struct hashtable *ht, world_sequence seq);
struct hashtable_iterator hashtable_begin(struct hashtable *ht, world_sequence seq);
struct hashtable_iterator hashtable_end(struct hashtable *ht, world_sequence seq);
bool hashtable_get(struct hashtable *ht, struct world_iovec key, struct world_iovec *data, world_sequence seq);
bool hashtable_set(struct hashtable *ht, struct world_iovec key, struct world_iovec data);
bool hashtable_add(struct hashtable *ht, struct world_iovec key, struct world_iovec data);
bool hashtable_replace(struct hashtable *ht, struct world_iovec key, struct world_iovec data);
bool hashtable_delete(struct hashtable *ht, struct world_iovec key);
void hashtable_checkpoint(struct hashtable *ht, world_sequence seq);

static inline bool hashtable_iterator_valid(struct hashtable_iterator it);
struct hashtable_iterator hashtable_iterator_next(struct hashtable_iterator it, world_sequence seq);
struct world_iovec hashtable_iterator_get_iovec(struct hashtable_iterator it, world_sequence seq);

struct hashtable {
  world_sequence seq;
  world_sequence seq_cp;
  struct circular logs;

  hash_type bucket_mask;
  struct vector buckets;

  struct vector garbages;

  world_sequence n_fresh_entries;
};

struct hashtable_iterator {
  void *cursor;
};

static inline world_sequence hashtable_get_sequence(struct hashtable *ht)
{
  return ht->seq;
}

static inline bool hashtable_iterator_valid(struct hashtable_iterator it)
{
  return it.cursor;
}
