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
#include <string.h>
#include "world_assert.h"
#include "world_hashtable.h"
#include "world_hashtable_entry.h"

static float _load_factor(struct world_hashtable *ht);
static bool _find(struct world_hashtable *ht, world_hash_type hash, struct world_iovec key, struct world_hashtable_entry **cursor);
static void _append_bucket(struct world_hashtable *ht);
static void _mark_garbage(struct world_hashtable *ht, struct world_hashtable_entry *entry);
static void _dispose_garbage(struct world_hashtable *ht, struct world_hashtable_entry *entry);
static bool _garbage_heap_property(const void *x, const void *y);

void world_hashtable_init(struct world_hashtable *ht, world_hash_type seed)
{
  world_mutex_init(&ht->mtx);
  world_hashtable_bucket_init(&ht->bucket);
  world_hashtable_log_init(&ht->log);
  vector_init(&ht->garbages);
  ht->seed = seed;
  ht->n_fresh_entries = 0;
}

void world_hashtable_destroy(struct world_hashtable *ht)
{
  vector_destroy(&ht->garbages);
  world_hashtable_log_destroy(&ht->log);
  world_hashtable_bucket_destroy(&ht->bucket);
  world_mutex_destroy(&ht->mtx);
}

enum world_error world_hashtable_get(struct world_hashtable *ht, struct world_iovec key, struct world_iovec *found)
{
  if (!key.base || !key.size) {
    return world_error_invalid_argument;
  }

  world_hash_type hash = world_hash(key.base, key.size, ht->seed);
  struct world_hashtable_entry *cursor = NULL;
  if (!_find(ht, hash, key, &cursor)) {
    return world_error_no_such_key;
  }

  cursor = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  if (world_hashtable_entry_is_void(cursor)) {
    return world_error_no_such_key;
  }

  if (found) {
    struct world_iovec data = world_hashtable_entry_data(cursor);
    found->base = data.base;
    found->size = data.size;
  }

  return world_error_ok;
}

enum world_error world_hashtable_set(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  if (!key.base || !key.size) {
    return world_error_invalid_argument;
  }

  world_mutex_lock(&ht->mtx);

  world_hash_type hash = world_hash(key.base, key.size, ht->seed);
  struct world_hashtable_entry *cursor = NULL;
  bool found = _find(ht, hash, key, &cursor);

  struct world_hashtable_entry *entry = world_hashtable_entry_new(hash, key, data);
  world_hashtable_log_push_back(&ht->log, entry);

  struct world_hashtable_entry *next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  if (found) {
    struct world_hashtable_entry *nextnext = atomic_load_explicit(&next->base.next, memory_order_relaxed);
    _mark_garbage(ht, next);
    atomic_store_explicit(&entry->base.next, nextnext, memory_order_relaxed);
    atomic_store_explicit(&entry->stale, next, memory_order_relaxed);
    atomic_store_explicit(&cursor->base.next, entry, memory_order_release);
  } else {
    atomic_store_explicit(&entry->base.next, next, memory_order_relaxed);
    atomic_store_explicit(&cursor->base.next, entry, memory_order_release);
    ht->n_fresh_entries++;
  }

  _append_bucket(ht);

  world_mutex_unlock(&ht->mtx);

  return world_error_ok;
}

enum world_error world_hashtable_add(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  if (!key.base || !key.size) {
    return world_error_invalid_argument;
  }

  enum world_error err = world_error_ok;
  world_mutex_lock(&ht->mtx);

  world_hash_type hash = world_hash(key.base, key.size, ht->seed);
  struct world_hashtable_entry *cursor = NULL;
  bool found = _find(ht, hash, key, &cursor);

  struct world_hashtable_entry *next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  if (found && !world_hashtable_entry_is_void(next)) {
    err = world_error_key_exists;
    goto release;
  }

  struct world_hashtable_entry *entry = world_hashtable_entry_new(hash, key, data);
  world_hashtable_log_push_back(&ht->log, entry);

  if (found) {
    struct world_hashtable_entry *nextnext = atomic_load_explicit(&next->base.next, memory_order_relaxed);
    atomic_store_explicit(&entry->base.next, nextnext, memory_order_relaxed);
    atomic_store_explicit(&entry->stale, next, memory_order_relaxed);
    atomic_store_explicit(&cursor->base.next, entry, memory_order_release);
  } else {
    atomic_store_explicit(&entry->base.next, next, memory_order_relaxed);
    atomic_store_explicit(&cursor->base.next, entry, memory_order_release);
  }
  ht->n_fresh_entries++;

  _append_bucket(ht);

release:
  world_mutex_unlock(&ht->mtx);
  return err;
}

enum world_error world_hashtable_replace(struct world_hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  if (!key.base || !key.size) {
    return world_error_invalid_argument;
  }

  enum world_error err = world_error_ok;
  world_mutex_lock(&ht->mtx);

  world_hash_type hash = world_hash(key.base, key.size, ht->seed);
  struct world_hashtable_entry *cursor = NULL;
  bool found = _find(ht, hash, key, &cursor);

  struct world_hashtable_entry *next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  if (!found || world_hashtable_entry_is_void(next)) {
    err = world_error_no_such_key;
    goto release;
  }

  struct world_hashtable_entry *entry = world_hashtable_entry_new(hash, key, data);
  world_hashtable_log_push_back(&ht->log, entry);

  struct world_hashtable_entry *stale = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  _mark_garbage(ht, stale);
  atomic_store_explicit(&entry->base.next, next, memory_order_relaxed);
  atomic_store_explicit(&entry->stale, stale, memory_order_relaxed);
  atomic_store_explicit(&cursor->base.next, entry, memory_order_release);

release:
  world_mutex_unlock(&ht->mtx);
  return err;
}

enum world_error world_hashtable_delete(struct world_hashtable *ht, struct world_iovec key)
{
  if (!key.base || !key.size) {
    return world_error_invalid_argument;
  }

  enum world_error err = world_error_ok;
  world_mutex_lock(&ht->mtx);

  world_hash_type hash = world_hash(key.base, key.size, ht->seed);
  struct world_hashtable_entry *cursor = NULL;
  bool found = _find(ht, hash, key, &cursor);

  struct world_hashtable_entry *next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  if (!found || world_hashtable_entry_is_void(next)) {
    err = world_error_no_such_key;
    goto release;
  }

  struct world_hashtable_entry *entry = world_hashtable_entry_new_void(hash, key);
  _mark_garbage(ht, entry);
  world_hashtable_log_push_back(&ht->log, entry);

  struct world_hashtable_entry *stale = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
  _mark_garbage(ht, stale);
  atomic_store_explicit(&entry->base.next, next, memory_order_relaxed);
  atomic_store_explicit(&entry->stale, stale, memory_order_relaxed);
  atomic_store_explicit(&cursor->base.next, entry, memory_order_release);
  ht->n_fresh_entries--;

release:
  world_mutex_unlock(&ht->mtx);
  return err;
}

struct world_hashtable_entry *world_hashtable_front(struct world_hashtable *ht)
{
  return world_hashtable_bucket_front(&ht->bucket);
}

struct world_hashtable_entry *world_hashtable_log(struct world_hashtable *ht)
{
  return world_hashtable_log_back(&ht->log);
}

void world_hashtable_checkpoint(struct world_hashtable *ht, world_sequence seq)
{
  world_mutex_lock(&ht->mtx);

  while (seq > world_hashtable_log_least_sequence(&ht->log)) {
    world_hashtable_log_pop_front(&ht->log);
  }

  while (vector_size(&ht->garbages) > 0) {
    struct world_hashtable_entry **position = vector_front(&ht->garbages);
    if ((*position)->base.seq >= world_hashtable_log_least_sequence(&ht->log)) {
      break;
    }
    _dispose_garbage(ht, *position);
    vector_pop_heap(&ht->garbages, sizeof(*position), _garbage_heap_property);
  }

  world_mutex_unlock(&ht->mtx);
}

static float _load_factor(struct world_hashtable *ht)
{
  return (float)ht->n_fresh_entries / world_hashtable_bucket_size(&ht->bucket);
}

static bool _find(struct world_hashtable *ht, world_hash_type hash, struct world_iovec key, struct world_hashtable_entry **cursor)
{
  *cursor = world_hashtable_bucket_find(&ht->bucket, hash);
  for (;;) {
    struct world_hashtable_entry *next = atomic_load_explicit(&(*cursor)->base.next, memory_order_relaxed);
    if (!next || next->base.hash > hash) {
      return false;
    }
    if (next->base.hash == hash) {
      struct world_iovec k = world_hashtable_entry_key(next);
      if (k.size == key.size && memcmp(key.base, k.base, k.size) == 0) {
        return true;
      }
    }
    *cursor = next;
  }
}

static void _append_bucket(struct world_hashtable *ht)
{
  if (_load_factor(ht) > 0.9f) {
    world_hashtable_bucket_append(&ht->bucket);
  }
}

static void _mark_garbage(struct world_hashtable *ht, struct world_hashtable_entry *entry)
{
  vector_push_heap(&ht->garbages, &entry, sizeof(entry), _garbage_heap_property);
}

static void _dispose_garbage(struct world_hashtable *ht, struct world_hashtable_entry *entry)
{
  // We know what to be removed from the hashtable, so we use an optimized way,
  // instead of _find(), to find a entry to be disposed.
  struct world_hashtable_entry *cursor = world_hashtable_bucket_find(&ht->bucket, entry->base.hash);
  for (;;) {
    // Check a next entry
    struct world_hashtable_entry *next = atomic_load_explicit(&cursor->base.next, memory_order_relaxed);
    if (next == entry) {
      WORLD_ASSERT(next->stale == NULL);
      struct world_hashtable_entry *nextnext = atomic_load_explicit(&next->base.next, memory_order_relaxed);
      atomic_store_explicit(&cursor->base.next, nextnext, memory_order_release);
      goto found;
    }
    cursor = next;

    if (world_hashtable_entry_is_bucket(cursor)) {
      continue;
    }

    // Check a stale entry
    struct world_hashtable_entry *stale = atomic_load_explicit(&cursor->stale, memory_order_relaxed);
    if (stale == entry) {
      WORLD_ASSERT(stale->stale == NULL);
      atomic_store_explicit(&cursor->stale, NULL, memory_order_release);
      goto found;
    }

    if (stale == NULL || cursor->base.hash != entry->base.hash) {
      continue;
    }

    // Check stale entries, recursively
    for (;;) {
      struct world_hashtable_entry *stalestale = atomic_load_explicit(&stale->stale, memory_order_relaxed);
      if (stalestale == NULL) {
        break;
      }
      if (stalestale == entry) {
        WORLD_ASSERT(stalestale->stale == NULL);
        atomic_store_explicit(&stale->stale, NULL, memory_order_relaxed);
        goto found;
      }
      stale = stalestale;
    }
  }

found:
  free(entry);
}

static bool _garbage_heap_property(const void *x, const void *y)
{
  struct world_hashtable_entry *const *xx = x;
  struct world_hashtable_entry *const *yy = y;
  return (*xx)->base.seq <= (*yy)->base.seq;
}
