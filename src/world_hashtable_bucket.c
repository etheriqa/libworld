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

#include <stdatomic.h>
#include <stdlib.h>
#include "world_hashtable_bucket.h"
#include "world_hashtable_entry.h"

void world_hashtable_bucket_init(struct world_hashtable_bucket *b)
{
  struct world_hashtable_entry *front = world_hashtable_entry_new_bucket(0);
  vector_init(&b->buckets);
  vector_push_back(&b->buckets, &front, sizeof(front));
  b->front = front;
  b->mask = 0;
}

void world_hashtable_bucket_destroy(struct world_hashtable_bucket *b)
{
  // TODO free buckets and entries
  vector_destroy(&b->buckets);
}

void world_hashtable_bucket_append(struct world_hashtable_bucket *b)
{
  size_t index = world_hashtable_bucket_size(b);
  world_hash_type hash = world_hash_reverse(index);
  struct world_hashtable_entry *bucket = world_hashtable_entry_new_bucket(hash);

  struct world_hashtable_entry *cursor = world_hashtable_bucket_find(b, hash);
  struct world_hashtable_entry *next = NULL;
  for (;;) {
    next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
    if (next && next->base.hash < bucket->base.hash) {
      cursor = next;
      continue;
    } else {
      break;
    }
  }

  atomic_store_explicit(&bucket->base.next, next, memory_order_relaxed);
  atomic_store_explicit(&cursor->base.next, bucket, memory_order_relaxed);
  vector_push_back(&b->buckets, &bucket, sizeof(bucket));

  if ((index & (index - 1)) == 0) {
    b->mask = (b->mask << 1) | 0x1;
  }
}

size_t world_hashtable_bucket_size(struct world_hashtable_bucket *b)
{
  return vector_size(&b->buckets);
}

struct world_hashtable_entry *world_hashtable_bucket_front(struct world_hashtable_bucket *b)
{
  return b->front;
}

struct world_hashtable_entry *world_hashtable_bucket_find(struct world_hashtable_bucket *b, world_hash_type hash)
{
  world_hash_type r = world_hash_reverse(hash);
  size_t index = r & b->mask;
  if (index >= world_hashtable_bucket_size(b)) {
    index = r & (b->mask >> 1);
  }
  struct world_hashtable_entry **position = vector_at(&b->buckets, index, sizeof(*position));
  return *position;
}
