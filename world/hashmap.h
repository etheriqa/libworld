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
#include <shared_mutex>
#include <vector>
#include "world/hash.h"
#include "world/snapshot_set.h"

namespace world {

class HashMap {
public:
  HashMap();
  HashMap(const HashMap&) = delete;
  HashMap(HashMap&&) = default;

  HashMap& operator=(const HashMap&) = delete;
  HashMap& operator=(HashMap&&) = default;

  sequence_t SequenceNumber() const;
  Snapshot* TakeSnapshot();

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

  size_t EntrySize() const;
  size_t BucketSize() const;
  std::float_t LoadFactor() const;

  void AddBucket();
  bool CollectGarbage();

private:
  mutable std::shared_timed_mutex mtx_;

  sequence_t sequence_;
  sequence_t n_entries_;

  SnapshotSet snapshot_set_;

  struct Node;
  hash_t bucket_mask_;
  std::vector<Node*> bucket_list_;

  struct NodeMinHeapComparator;
  std::vector<Node*> garbage_heap_;

  std::unique_lock<std::shared_timed_mutex> LockUnique();
  std::shared_lock<std::shared_timed_mutex> LockShared() const;

  size_t BucketIndex(hash_t hash) const noexcept;

  bool Find(hash_t hash, const void* key, size_t key_size,
            Node*& cursor) noexcept;

  void MarkGarbage(Node* garbage);
}; // class HashMap

} // namespace world
