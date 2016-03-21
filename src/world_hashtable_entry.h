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

struct world_hashtable_entry {
  struct {
    world_sequence seq;
    world_hash_type hash;
    _Atomic(struct world_hashtable_entry *)next;
  } base;
  _Atomic(struct world_hashtable_entry *)log;
  _Atomic(struct world_hashtable_entry *)stale;
  struct {
    world_key_size key_size;
    world_data_size data_size;
  } header;
};

struct world_hashtable_entry *world_hashtable_entry_new(world_hash_type hash, struct world_iovec key, struct world_iovec data);
struct world_hashtable_entry *world_hashtable_entry_new_void(world_hash_type hash, struct world_iovec key);
struct world_hashtable_entry *world_hashtable_entry_new_bucket(world_hash_type hash);
bool world_hashtable_entry_is_void(struct world_hashtable_entry *entry);
bool world_hashtable_entry_is_bucket(struct world_hashtable_entry *entry);
struct world_hashtable_entry *world_hashtable_entry_advance(struct world_hashtable_entry **entry, world_sequence seq);
struct world_iovec world_hashtable_entry_key(struct world_hashtable_entry *entry);
struct world_iovec world_hashtable_entry_data(struct world_hashtable_entry *entry);
struct world_iovec world_hashtable_entry_raw(struct world_hashtable_entry *entry);
