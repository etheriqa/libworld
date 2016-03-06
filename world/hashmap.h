// Copyright (c) 2016 TAKAMORI Kaede <etheriqa@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cmath>
#include <deque>
#include <shared_mutex>
#include <vector>
#include <world/snapshot.h>
#include "world/hash.h"

namespace world {

class HashMap {
public:
  class Snapshot;
  class SnapshotIterator;
  struct SnapshotEntry;

  HashMap();
  HashMap(const HashMap&) = delete;
  HashMap(HashMap&&) = default;

  HashMap& operator=(const HashMap&) = delete;
  HashMap& operator=(HashMap&&) = default;

  sequence_t SequenceNumber() const;
  sequence_t CheckpointSequenceNumber() const;

  Snapshot TakeSnapshot(sequence_t sequence) const;
  SnapshotEntry Log(sequence_t sequence) const;

  bool Get(const void* key, size_t key_size,
           const void*& data, size_t& data_size) const;
  bool Get(const void* key, size_t key_size,
           const void*& data, size_t& data_size,
           sequence_t sequence) const;

  bool Set(const void* key, size_t key_size,
           const void* data, size_t data_size);
  bool Add(const void* key, size_t key_size,
           const void* data, size_t data_size);
  bool Replace(const void* key, size_t key_size,
               const void* data, size_t data_size);
  bool Delete(const void* key, size_t key_size);

  void Checkpoint(sequence_t sequence);
  bool CollectGarbage();

private:
  static constexpr std::float_t LoadFactorThreshold = 0.9;

  mutable std::shared_timed_mutex mtx_;

  struct Node;
  std::deque<Node*> log_list_;
  sequence_t checkpoint_sequence_;

  hash_t bucket_mask_;
  std::vector<Node*> bucket_list_;
  sequence_t n_entries_;

  struct NodeMinHeapComparator;
  std::vector<Node*> garbage_heap_;

  std::unique_lock<std::shared_timed_mutex> LockUnique();
  std::shared_lock<std::shared_timed_mutex> LockShared() const;

  Node* MakeBucket(hash_t index);
  Node* MakeVoid(hash_t hash, const void* key, size_t key_size);
  Node* MakeEntry(hash_t hash, const void* key, size_t key_size,
                  const void* data, size_t data_size);

  size_t EntrySize() const;
  size_t BucketSize() const;
  std::float_t LoadFactor() const;

  size_t BucketIndex(hash_t hash) const noexcept;
  bool Find(hash_t hash, const void* key, size_t key_size,
            Node*& cursor) noexcept;

  void AddBucket();
  void MarkGarbage(Node* garbage);
}; // class HashMap

class HashMap::Snapshot {
public:
  Snapshot(const HashMap& hashmap, sequence_t sequence) noexcept;

  sequence_t SequenceNumber() const noexcept;

  bool Get(const void* key, size_t key_size,
           const void*& data, size_t& data_size) const;

  SnapshotIterator begin() const;
  SnapshotIterator end() const;

private:
  const HashMap* hashmap_;
  sequence_t sequence_;
}; // class HashMap::Snapshot

class HashMap::SnapshotIterator {
public:
  SnapshotIterator(const HashMap& hashmap,
                   sequence_t sequence,
                   const Node* cursor) noexcept;

  SnapshotIterator& operator++() noexcept;
  SnapshotIterator operator++(int) noexcept;
  SnapshotEntry operator*() const noexcept;
  operator bool() const noexcept;
  bool operator==(const SnapshotIterator& rhs) const noexcept;
  bool operator!=(const SnapshotIterator& rhs) const noexcept;

private:
  const HashMap* hashmap_;
  sequence_t sequence_;
  const Node* cursor_;

  void AdjustCursor() noexcept;
}; // class HashMap::SnapshotIterator

struct HashMap::SnapshotEntry {
  size_t key_size;
  size_t data_size;
  const void* key;
  const void* data;

  SnapshotEntry() noexcept;
  SnapshotEntry(const void* key, size_t key_size,
                const void* data, size_t data_size) noexcept;

  operator bool() const noexcept;
}; // struct HashMap::SnapshotEntry

} // namespace world
