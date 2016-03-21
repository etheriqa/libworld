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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world_assert.h"
#include "world_byteorder.h"
#include "world_hashtable_entry.h"

static struct world_hashtable_entry *_next_nonbucket(struct world_hashtable_entry *entry);
static world_key_size _key_size(struct world_hashtable_entry *entry);
static world_key_size _data_size(struct world_hashtable_entry *entry);
static void *_key_base(struct world_hashtable_entry *entry);
static void *_data_base(struct world_hashtable_entry *entry);

struct world_hashtable_entry *world_hashtable_entry_new(world_hash_type hash, struct world_iovec key, struct world_iovec data)
{
  struct world_hashtable_entry *entry = malloc(sizeof(*entry) + key.size + data.size);
  if (entry == NULL) {
    perror("malloc");
    abort();
  }

  entry->base.seq = 0;
  entry->base.hash = hash;
  atomic_store_explicit(&entry->base.next, NULL, memory_order_relaxed);
  atomic_store_explicit(&entry->log, NULL, memory_order_relaxed);
  atomic_store_explicit(&entry->stale, NULL, memory_order_relaxed);
  entry->header.key_size = world_encode_key_size(key.size);
  entry->header.data_size = world_encode_data_size(data.size);
  memcpy(_key_base(entry), key.base, key.size);
  memcpy(_data_base(entry), data.base, data.size);

  return entry;
}

struct world_hashtable_entry *world_hashtable_entry_new_void(world_hash_type hash, struct world_iovec key)
{
  struct world_hashtable_entry *entry = malloc(sizeof(*entry) + key.size);
  if (entry == NULL) {
    perror("malloc");
    abort();
  }

  entry->base.seq = 0;
  entry->base.hash = hash;
  atomic_store_explicit(&entry->base.next, NULL, memory_order_relaxed);
  atomic_store_explicit(&entry->log, NULL, memory_order_relaxed);
  atomic_store_explicit(&entry->stale, NULL, memory_order_relaxed);
  entry->header.key_size = world_encode_key_size(key.size);
  entry->header.data_size = world_encode_data_size(0);
  memcpy(_key_base(entry), key.base, key.size);

  return entry;
}

struct world_hashtable_entry *world_hashtable_entry_new_bucket(world_hash_type hash)
{
  struct world_hashtable_entry *entry = malloc(sizeof(entry->base));
  if (entry == NULL) {
    perror("malloc");
    abort();
  }

  entry->base.seq = 0;
  entry->base.hash = hash;
  atomic_store_explicit(&entry->base.next, NULL, memory_order_relaxed);

  return entry;
}

bool world_hashtable_entry_is_void(struct world_hashtable_entry *entry)
{
  return entry->base.seq > 0 && _data_size(entry) == 0;
}

bool world_hashtable_entry_is_bucket(struct world_hashtable_entry *entry)
{
  return entry->base.seq == 0;
}

struct world_hashtable_entry *world_hashtable_entry_advance(struct world_hashtable_entry **entry, world_sequence seq)
{
  for (;;) {
    *entry = _next_nonbucket(*entry);
    if (!*entry) {
      return NULL;
    }
    struct world_hashtable_entry *cursor = *entry;
    for (;;) {
      if (cursor->base.seq <= seq) {
        return cursor;
      }
      struct world_hashtable_entry *stale = atomic_load_explicit(&cursor->stale, memory_order_relaxed);
      if (!stale) {
        break;
      }
      cursor = stale;
    }
  }
}

struct world_iovec world_hashtable_entry_key(struct world_hashtable_entry *entry)
{
  WORLD_ASSERT(!world_hashtable_entry_is_bucket(entry));
  struct world_iovec iovec;
  iovec.base = _key_base(entry);
  iovec.size = _key_size(entry);
  return iovec;
}

struct world_iovec world_hashtable_entry_data(struct world_hashtable_entry *entry)
{
  WORLD_ASSERT(!world_hashtable_entry_is_bucket(entry));
  struct world_iovec iovec;
  iovec.base = _data_base(entry);
  iovec.size = _data_size(entry);
  return iovec;
}

struct world_iovec world_hashtable_entry_raw(struct world_hashtable_entry *entry)
{
  WORLD_ASSERT(!world_hashtable_entry_is_bucket(entry));
  struct world_iovec iovec;
  iovec.base = &entry->header;
  iovec.size = sizeof(entry->header) + _key_size(entry) + _data_size(entry);
  return iovec;
}

static struct world_hashtable_entry *_next_nonbucket(struct world_hashtable_entry *entry)
{
  do {
    entry = atomic_load_explicit(&entry->base.next, memory_order_relaxed);
  } while (entry && world_hashtable_entry_is_bucket(entry));
  return entry;
}

static world_key_size _key_size(struct world_hashtable_entry *entry)
{
  return world_decode_key_size(entry->header.key_size);
}

static world_key_size _data_size(struct world_hashtable_entry *entry)
{
  return world_decode_data_size(entry->header.data_size);
}

static void *_key_base(struct world_hashtable_entry *entry)
{
  return (void *)((uintptr_t)&entry->header + sizeof(entry->header));
}

static void *_data_base(struct world_hashtable_entry *entry)
{
  return (void *)((uintptr_t)&entry->header + sizeof(entry->header) + _key_size(entry));
}
