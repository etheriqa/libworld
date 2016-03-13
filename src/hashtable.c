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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "byteorder.h"
#include "hashtable.h"

static float hashtable__get_load_factor(struct hashtable *ht);
static size_t hashtable__get_bucket_index(struct hashtable *ht, hash_type hash);
static struct node *hashtable__get_bucket(struct hashtable *ht, hash_type hash);
static bool hashtable__find(struct hashtable *ht, hash_type hash, struct world_iovec key, struct node **cursor);
static void hashtable__add_bucket(struct hashtable *ht);
static void hashtable__mark_garbage(struct hashtable *ht, struct node *node);
static void hashtable__collect_garbage(struct hashtable *ht, struct node *node);

struct node {
  hash_type hash;
  world_sequence seq;
  struct node *next;
};

static struct node *node_new(hash_type hash);
static bool node_is_bucket(struct node *node);
static bool node_is_entry(struct node *node);
static struct entry *node_get_entry(struct node *node);
static struct record *node_get_record(struct node *node);
static struct node *node_next_entry(struct node *node);

static bool node_garbage_minheap_property(const void *x, const void *y);

struct record {
  world_key_size key_size;
  world_data_size data_size;
};

static bool record_is_empty(struct record *record);
static world_key_size record_get_key_size(struct record *record);
static world_data_size record_get_data_size(struct record *record);
static const void *record_get_key_base(struct record *record);
static const void *record_get_data_base(struct record *record);
static struct world_iovec record_get_key(struct record *record);
static struct world_iovec record_get_data(struct record *record);
static struct world_iovec record_get_iovec(struct record *record);
static bool record_key_equals(struct record *record, struct world_iovec key);

struct entry {
  struct node node;
  struct entry *stale;
  struct record record;
};

static struct entry *entry_new(hash_type hash, world_sequence seq, struct world_iovec key, struct world_iovec data);

void hashtable_init(struct hashtable *ht)
{
  ht->seq = 0;
  ht->seq_cp = 0;
  circular_init(&ht->logs);
  ht->bucket_mask = 0;
  vector_init(&ht->buckets);
  struct node *bucket = node_new(0);
  vector_push_back(&ht->buckets, &bucket, sizeof(bucket));
  vector_init(&ht->garbages);
  ht->n_fresh_entries = 0;
}

void hashtable_destroy(struct hashtable *ht)
{
  hashtable_checkpoint(ht, ht->seq);
  struct node *cursor = *(struct node **)vector_front(&ht->buckets);
  while (cursor->next) {
    struct node *next = cursor->next;
    assert(node_is_bucket(next) || node_get_entry(next)->stale == NULL);
    free(cursor);
    cursor = next;
  }
  free(cursor);
  vector_destroy(&ht->garbages);
  vector_destroy(&ht->buckets);
  circular_destroy(&ht->logs);
}

struct world_iovec hashtable_get_iovec(struct hashtable *ht, world_sequence seq)
{
  assert(ht->seq_cp < seq && seq <= ht->seq);

  struct node **log = circular_at(&ht->logs, seq - ht->seq_cp - 1, sizeof(*log));
  return record_get_iovec(node_get_record(*log));
}

struct hashtable_iterator hashtable_begin(struct hashtable *ht, world_sequence seq)
{
  struct hashtable_iterator it;
  it.cursor = *(struct node **)vector_front(&ht->buckets);
  return hashtable_iterator_next(it, seq);
}

struct hashtable_iterator hashtable_end(struct hashtable *ht, world_sequence seq)
{
  struct hashtable_iterator it;
  it.cursor = NULL;
  return it;
}

bool hashtable_get(struct hashtable *ht, struct world_iovec key, struct world_iovec *data, world_sequence seq)
{
  assert(key.base);
  assert(key.size);
  assert(seq <= ht->seq);

  hash_type hash = murmurhash(key.base, key.size);
  struct node *cursor = NULL;
  if (!hashtable__find(ht, hash, key, &cursor)) {
    return false;
  }
  cursor = cursor->next;
  for (;;) {
    if (cursor->seq <= seq) {
      break;
    }
    if (!node_get_entry(cursor)->stale) {
      return false;
    }
    cursor = &node_get_entry(cursor)->stale->node;
  }
  struct entry *found = node_get_entry(cursor);
  if (record_is_empty(&found->record)) {
    return false;
  }
  if (data) {
    *data = record_get_data(&found->record);
  }
  return true;
}

bool hashtable_set(struct hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  assert(key.base);
  assert(key.size);
  assert(data.base);
  assert(data.size);

  hash_type hash = murmurhash(key.base, key.size);
  ht->seq++;
  struct entry *entry = entry_new(hash, ht->seq, key, data);
  circular_push_back(&ht->logs, &entry, sizeof(entry));
  struct node *cursor = NULL;
  if (hashtable__find(ht, hash, key, &cursor)) {
    struct node *stale = cursor->next;
    hashtable__mark_garbage(ht, stale);
    entry->node.next = stale->next;
    entry->stale = node_get_entry(stale);
    cursor->next = &entry->node;
  } else {
    entry->node.next = cursor->next;
    cursor->next = &entry->node;
    ht->n_fresh_entries++;
  }

  assert(cursor->hash <= cursor->next->hash);
  assert(!cursor->next->next || cursor->next->hash <= cursor->next->next->hash);

  hashtable__add_bucket(ht);
  return true;
}

bool hashtable_add(struct hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  assert(key.base);
  assert(key.size);
  assert(data.base);
  assert(data.size);

  hash_type hash = murmurhash(key.base, key.size);
  bool found;
  struct node *cursor = NULL;
  if ((found = hashtable__find(ht, hash, key, &cursor)) &&
      !record_is_empty(node_get_record(cursor->next))) {
    return false;
  }
  ht->seq++;
  struct entry *entry = entry_new(hash, ht->seq, key, data);
  circular_push_back(&ht->logs, &entry, sizeof(entry));
  if (found) {
    struct node *stale = cursor->next;
    hashtable__mark_garbage(ht, stale);
    entry->node.next = stale->next;
    entry->stale = node_get_entry(stale);
    cursor->next = &entry->node;
  } else {
    entry->node.next = cursor->next;
    cursor->next = &entry->node;
    ht->n_fresh_entries++;
  }

  assert(cursor->hash <= cursor->next->hash);
  assert(!cursor->next->next || cursor->next->hash <= cursor->next->next->hash);

  hashtable__add_bucket(ht);
  return true;
}

bool hashtable_replace(struct hashtable *ht, struct world_iovec key, struct world_iovec data)
{
  // TODO Refactor. The following is almost the same as hashtable_delete().
  assert(key.base);
  assert(key.size);
  assert(data.base);
  assert(data.size);

  hash_type hash = murmurhash(key.base, key.size);
  struct node *cursor = NULL;
  if (!hashtable__find(ht, hash, key, &cursor) ||
      record_is_empty(node_get_record(cursor->next))) {
    return false;
  }
  ht->seq++;
  struct entry *entry = entry_new(hash, ht->seq, key, data);
  circular_push_back(&ht->logs, &entry, sizeof(entry));
  struct node *stale = cursor->next;
  hashtable__mark_garbage(ht, stale);
  entry->node.next = stale->next;
  entry->stale = node_get_entry(stale);
  cursor->next = &entry->node;

  assert(cursor->hash <= cursor->next->hash);
  assert(!cursor->next->next || cursor->next->hash <= cursor->next->next->hash);

  hashtable__add_bucket(ht);
  return true;
}

bool hashtable_delete(struct hashtable *ht, struct world_iovec key)
{
  // TODO Refactor. The following is almost the same as hashtable_replace().
  assert(key.base);
  assert(key.size);

  hash_type hash = murmurhash(key.base, key.size);
  struct node *cursor = NULL;
  if (!hashtable__find(ht, hash, key, &cursor) ||
      record_is_empty(node_get_record(cursor->next))) {
    return false;
  }
  ht->seq++;
  struct world_iovec data;
  data.base = NULL;
  data.size = 0;
  struct entry *entry = entry_new(hash, ht->seq, key, data);
  circular_push_back(&ht->logs, &entry, sizeof(entry));
  struct node *stale = cursor->next;
  hashtable__mark_garbage(ht, stale);
  entry->node.next = stale->next;
  entry->stale = node_get_entry(stale);
  cursor->next = &entry->node;

  assert(cursor->hash <= cursor->next->hash);
  assert(!cursor->next->next || cursor->next->hash <= cursor->next->next->hash);

  hashtable__add_bucket(ht);
  return true;
}

void hashtable_checkpoint(struct hashtable *ht, world_sequence seq)
{
  assert(seq >= ht->seq_cp);
  assert(seq <= ht->seq);

  while (seq > ht->seq_cp) {
    ht->seq_cp++;
    circular_pop_front(&ht->logs);
  }
  while (vector_size(&ht->garbages) > 0) {
    struct node *node = *(struct node **)vector_front(&ht->garbages);
    if (node->seq > ht->seq_cp) {
      break;
    }
    vector_pop_heap(&ht->garbages, sizeof(node), node_garbage_minheap_property);
    hashtable__collect_garbage(ht, node);
  }
}

static float hashtable__get_load_factor(struct hashtable *ht)
{
  return ht->n_fresh_entries / vector_size(&ht->buckets);
}

static size_t hashtable__get_bucket_index(struct hashtable *ht, hash_type hash)
{
  hash_type reversed = hash_reverse(hash);
  size_t index = reversed & ht->bucket_mask;
  if (index >= vector_size(&ht->buckets)) {
    index = reversed & (ht->bucket_mask >> 1);
  }
  assert(index < vector_size(&ht->buckets));
  return index;
}

static struct node *hashtable__get_bucket(struct hashtable *ht, hash_type hash)
{
  size_t index = hashtable__get_bucket_index(ht, hash);
  return *(struct node **)vector_at(&ht->buckets, index, sizeof(struct node *));
}

static bool hashtable__find(struct hashtable *ht, hash_type hash, struct world_iovec key, struct node **cursor)
{
  assert(key.base);
  assert(key.size);
  assert(cursor);

  *cursor = hashtable__get_bucket(ht, hash);
  for (;;) {
    struct node *next = (*cursor)->next;
    if (!next) {
      return false;
    }
    if (next->hash > hash) {
      return false;
    }
    if (node_is_entry(next) &&
        next->hash == hash &&
        record_key_equals(node_get_record(next), key)) {
      return true;
    }
    *cursor = next;
  }
}

static void hashtable__add_bucket(struct hashtable *ht)
{
  if (hashtable__get_load_factor(ht) < 0.9) {
    return;
  }
  size_t index = vector_size(&ht->buckets);
  struct node *bucket = node_new(hash_reverse(index));
  struct node *cursor = hashtable__get_bucket(ht, bucket->hash);
  while (cursor->next && cursor->next->hash < bucket->hash) {
    cursor = cursor->next;
  }
  bucket->next = cursor->next;
  cursor->next = bucket;
  vector_push_back(&ht->buckets, &bucket, sizeof(bucket));
  if ((index & (index - 1)) == 0) {
    // index is a power of 2
    ht->bucket_mask = (ht->bucket_mask << 1) + 1;
  }

  assert(cursor->hash <= cursor->next->hash);
  assert(!cursor->next->next || cursor->next->hash <= cursor->next->next->hash);
}

static void hashtable__mark_garbage(struct hashtable *ht, struct node *node)
{
  vector_push_heap(&ht->garbages, &node, sizeof(node), node_garbage_minheap_property);
}

static void hashtable__collect_garbage(struct hashtable *ht, struct node *node)
{
  assert(node->seq <= ht->seq_cp);
  assert(!node_get_entry(node)->stale);

  // We know what to be removed from the hashtable, so we use a special way to find a node instead of hashtable__find().
  struct node *cursor = hashtable__get_bucket(ht, node->hash);
  for (;;) {
    if (cursor->next == node) {
      cursor->next = node->next;
      ht->n_fresh_entries--;
      break;
    }
    cursor = cursor->next;
    assert(cursor);
    if (&node_get_entry(cursor)->stale->node == node) {
      node_get_entry(cursor)->stale = NULL;
      break;
    }
    if (node_is_entry(cursor) &&
        cursor->hash == node->hash &&
        record_key_equals(node_get_record(cursor), record_get_key(node_get_record(node)))) {
      for (;;) {
        cursor = &node_get_entry(cursor)->stale->node;
        if (&node_get_entry(cursor)->stale->node == node) {
          node_get_entry(cursor)->stale = NULL;
          break;
        }
      }
      break;
    }
  }
  free(node);
}

struct hashtable_iterator hashtable_iterator_next(struct hashtable_iterator it, world_sequence seq)
{
  assert(hashtable_iterator_valid(it));

  for (;;) {
    it.cursor = node_next_entry(it.cursor);
    if (!hashtable_iterator_valid(it)) {
      return it;
    }
    struct entry *cursor = node_get_entry(it.cursor);
    while (cursor && cursor->node.seq > seq) {
      cursor = cursor->stale;
    }
    if (cursor) {
      return it;
    }
  }
}

struct world_iovec hashtable_iterator_get_iovec(struct hashtable_iterator it, world_sequence seq)
{
  assert(hashtable_iterator_valid(it));

  struct entry *cursor = node_get_entry(it.cursor);
  while (cursor->node.seq > seq) {
    cursor = cursor->stale;
    assert(cursor);
  }
  return record_get_iovec(&cursor->record);
}

static struct node *node_new(hash_type hash)
{
  struct node *node = malloc(sizeof(*node));
  if (node == NULL) {
    perror("malloc");
    abort();
  }
  node->hash = hash;
  node->seq = 0;
  node->next = NULL;
  return node;
}

static bool node_is_bucket(struct node *node)
{
  return node->seq == 0;
}

static bool node_is_entry(struct node *node)
{
  return node->seq > 0;
}

static struct entry *node_get_entry(struct node *node)
{
  assert(node_is_entry(node));

  return (struct entry *)node;
}

static struct record *node_get_record(struct node *node)
{
  assert(node_is_entry(node));

  return &node_get_entry(node)->record;
}

static struct node *node_next_entry(struct node *node)
{
  assert(node);
  do {
    node = node->next;
  } while (node && node_is_bucket(node));
  return node;
}

static bool node_garbage_minheap_property(const void *x, const void *y)
{
  return (*(struct node **)x)->seq <= (*(struct node **)y)->seq;
}

static bool record_is_empty(struct record *record)
{
  return record->data_size == 0;
}

static world_key_size record_get_key_size(struct record *record)
{
  return decode_key_size(record->key_size);
}

static world_data_size record_get_data_size(struct record *record)
{
  return decode_data_size(record->data_size);
}

static const void *record_get_key_base(struct record *record)
{
  return (const void *)((uintptr_t)record + sizeof(*record));
}

static const void *record_get_data_base(struct record *record)
{
  return (const void *)((uintptr_t)record + sizeof(*record) + record_get_key_size(record));
}

static struct world_iovec record_get_key(struct record *record)
{
  struct world_iovec key;
  key.base = record_get_key_base(record);
  key.size = record_get_key_size(record);
  return key;
}

static struct world_iovec record_get_data(struct record *record)
{
  struct world_iovec data;
  data.base = record_get_data_base(record);
  data.size = record_get_data_size(record);
  return data;
}

static struct world_iovec record_get_iovec(struct record *record)
{
  struct world_iovec iovec;
  iovec.base = record;
  iovec.size = sizeof(*record) + record_get_key_size(record) + record_get_data_size(record);
  return iovec;
}

static bool record_key_equals(struct record *record, struct world_iovec key)
{
  return record_get_key_size(record) == key.size &&
         memcmp(record_get_key_base(record), key.base, key.size) == 0;
}

static struct entry *entry_new(hash_type hash, world_sequence seq, struct world_iovec key, struct world_iovec data)
{
  assert(seq > 0);

  struct entry *entry = malloc(sizeof(*entry) + key.size + data.size);
  if (entry == NULL) {
    perror("malloc");
    abort();
  }
  entry->node.hash = hash;
  entry->node.seq = seq;
  entry->node.next = NULL;
  entry->stale = NULL;
  entry->record.key_size = encode_key_size(key.size);
  entry->record.data_size = encode_key_size(data.size);
  memcpy((void *)record_get_key_base(&entry->record), key.base, key.size);
  memcpy((void *)record_get_data_base(&entry->record), data.base, data.size);
  return entry;
}
