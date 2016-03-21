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

#include <stddef.h>
#include "vector.h"
#include "world_hash.h"

struct world_hashtable_entry;

struct world_hashtable_bucket {
  struct vector buckets;
  struct world_hashtable_entry *front;
  world_hash_type mask;
};

void world_hashtable_bucket_init(struct world_hashtable_bucket *b);
void world_hashtable_bucket_destroy(struct world_hashtable_bucket *b);
void world_hashtable_bucket_append(struct world_hashtable_bucket *b);
size_t world_hashtable_bucket_size(struct world_hashtable_bucket *b);
struct world_hashtable_entry *world_hashtable_bucket_front(struct world_hashtable_bucket *b);
struct world_hashtable_entry *world_hashtable_bucket_find(struct world_hashtable_bucket *b, world_hash_type hash);
