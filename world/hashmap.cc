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

#include <cassert>
#include <cstring>
#include <algorithm>
#include <memory>
#include "world/bitwise.h"
#include "world/hash.h"
#include "world/hashmap.h"

namespace {

enum struct NodeType {
  Bucket,
  Void,
  Entry,
}; // enum struct NodeType

} // anonymous namespace

namespace world {

struct HashMap::Node {
  const sequence_t sequence;

  const hash_t hash;

  Node* next;
  Node* stale;

  const size_t key_size;
  const size_t data_size;

  explicit Node(hash_t hash) noexcept;
  Node(sequence_t sequence, hash_t hash,
       const void* key, size_t key_size) noexcept;
  Node(sequence_t sequence, hash_t hash,
       const void* key, size_t key_size,
       const void* data, size_t data_size) noexcept;

  ptrdiff_t KeyOffset() const noexcept;
  ptrdiff_t DataOffset() const noexcept;
  const void* Key() const noexcept;
  const void* Data() const noexcept;

  NodeType Type() const noexcept;
}; // struct HashMap::Node

struct HashMap::NodeMinHeapComparator {
  bool operator()(const Node* x, const Node* y) const noexcept;
}; // struct HashMap::NodeMinHeapComparator

HashMap::HashMap()
: mtx_()
, log_list_{}
, checkpoint_sequence_(0)
, bucket_mask_(0)
, bucket_list_{MakeBucket(0)}
, n_entries_(0)
, garbage_heap_()
{}

sequence_t HashMap::SequenceNumber() const
{
  auto lock = LockShared();
  return checkpoint_sequence_ + log_list_.size();
}

sequence_t HashMap::CheckpointSequenceNumber() const
{
  auto lock = LockShared();
  return checkpoint_sequence_;
}

auto HashMap::TakeSnapshot(sequence_t sequence) const -> Snapshot
{
  return Snapshot(*this, sequence);
}

auto HashMap::Log(sequence_t sequence) const -> SnapshotEntry
{
  auto lock = LockShared();
  if (sequence <= checkpoint_sequence_) {
    return SnapshotEntry();
  }
  const auto index = sequence - checkpoint_sequence_ - 1;
  if (index >= log_list_.size()) {
    return SnapshotEntry();
  }
  const auto log = log_list_[index];
  return SnapshotEntry(log->Key(), log->key_size, log->Data(), log->data_size);
}

bool HashMap::Get(const void* key, size_t key_size,
                  const void*& data, size_t& data_size) const
{
  const auto hash = Hash(key, key_size);
  auto lock = LockShared();
  const auto index = BucketIndex(hash);
  auto cursor = bucket_list_[index];
  while (true) {
    if (!cursor->next) {
      return false;
    }
    cursor = cursor->next;
    if (cursor->hash > hash) {
      return false;
    }
    if (cursor->Type() == NodeType::Void) {
      continue;
    }
    if (cursor->hash == hash &&
        cursor->key_size == key_size &&
        memcmp(cursor->Key(), key, key_size) == 0) {
      break;
    }
  }
  data = cursor->Data();
  data_size = cursor->data_size;
  return true;
}

bool HashMap::Get(const void* key, size_t key_size,
                  const void*& data, size_t& data_size,
                  sequence_t sequence) const
{
  const auto hash = Hash(key, key_size);
  auto lock = LockShared();
  const auto index = BucketIndex(hash);
  auto cursor = bucket_list_[index];
  while (true) {
    if (!cursor->next) {
      return false;
    }
    cursor = cursor->next;
    if (cursor->hash > hash) {
      return false;
    }
    if (cursor->hash == hash &&
        cursor->key_size == key_size &&
        memcmp(cursor->Key(), key, key_size) == 0) {
      break; // found
    }
  }
  while (true) {
    if (cursor->sequence <= sequence) {
      break; // found
    }
    if (!cursor->stale) {
      return false;
    }
    cursor = cursor->stale;
  }
  if (cursor->Type() == NodeType::Void) {
    return false;
  }
  data = cursor->Data();
  data_size = cursor->data_size;
  return true;
}

bool HashMap::Set(const void* key, size_t key_size,
                  const void* data, size_t data_size)
{
  const auto hash = Hash(key, key_size);
  auto lock = LockUnique();
  auto entry = MakeEntry(hash, key, key_size, data, data_size);
  Node* cursor;
  if (Find(hash, key, key_size, cursor)) {
    const auto stale = cursor->next;
    MarkGarbage(stale);
    entry->next = stale->next;
    entry->stale = stale;
    cursor->next = entry;
  } else {
    entry->next = cursor->next;
    cursor->next = entry;
  }
  assert(cursor->hash <= cursor->next->hash);
  assert(!entry->next || entry->hash <= entry->next->hash);
  AddBucket();
  return true;
}

bool HashMap::Add(const void* key, size_t key_size,
                  const void* data, size_t data_size)
{
  const auto hash = Hash(key, key_size);
  auto lock = LockUnique();
  bool found;
  Node* cursor;
  if ((found = Find(hash, key, key_size, cursor)) &&
      cursor->next->Type() != NodeType::Void) {
    return false;
  }
  auto entry = MakeEntry(hash, key, key_size, data, data_size);
  if (found) {
    const auto stale = cursor->next;
    MarkGarbage(stale);
    entry->next = stale->next;
    entry->stale = stale;
    cursor->next = entry;
  } else {
    entry->next = cursor->next;
    cursor->next = entry;
  }
  assert(cursor->hash <= cursor->next->hash);
  assert(!entry->next || entry->hash <= entry->next->hash);
  AddBucket();
  return true;
}

bool HashMap::Replace(const void* key, size_t key_size,
                      const void* data, size_t data_size)
{
  const auto hash = Hash(key, key_size);
  auto lock = LockUnique();
  Node* cursor;
  if (!Find(hash, key, key_size, cursor) ||
      cursor->next->Type() == NodeType::Void) {
    return false;
  }
  const auto stale = cursor->next;
  MarkGarbage(stale);
  auto entry = MakeEntry(hash, key, key_size, data, data_size);
  entry->next = stale->next;
  entry->stale = stale;
  cursor->next = entry;
  assert(cursor->hash <= cursor->next->hash);
  assert(!entry->next || entry->hash <= entry->next->hash);
  AddBucket();
  return true;
}

bool HashMap::Delete(const void* key, size_t key_size)
{
  const auto hash = Hash(key, key_size);
  auto lock = LockUnique();
  Node* cursor;
  if (!Find(hash, key, key_size, cursor) ||
      cursor->next->Type() == NodeType::Void) {
    return false;
  }
  const auto stale = cursor->next;
  MarkGarbage(stale);
  auto entry = MakeVoid(hash, key, key_size);
  MarkGarbage(entry);
  entry->next = cursor->next->next;
  entry->stale = cursor->next;
  cursor->next = entry;
  assert(cursor->hash <= cursor->next->hash);
  assert(!entry->next || entry->hash <= entry->next->hash);
  AddBucket();
  return true;
}

void HashMap::Checkpoint(sequence_t sequence)
{
  auto lock = LockUnique();
  while (checkpoint_sequence_ < sequence) {
    checkpoint_sequence_++;
    log_list_.pop_front();
  }
}

bool HashMap::CollectGarbage()
{
  auto lock = LockUnique();
  if (garbage_heap_.empty()) {
    return false;
  }
  const auto garbage = garbage_heap_.front();
  assert(!garbage->stale);
  if (garbage->sequence > checkpoint_sequence_) {
    return false;
  }
  std::pop_heap(garbage_heap_.begin(), garbage_heap_.end(),
                NodeMinHeapComparator());
  garbage_heap_.pop_back();
  const auto index = BucketIndex(garbage->hash);
  auto cursor = bucket_list_[index];
  while (true) {
    if (cursor->next == garbage) {
      cursor->next = cursor->next->next;
      break;
    }
    cursor = cursor->next;
    assert(cursor);
    if (cursor->stale == garbage) {
      cursor->stale = nullptr;
      break;
    }
    if (cursor->hash == garbage->hash &&
        cursor->key_size == garbage->key_size &&
        memcmp(cursor->Key(), garbage->Key(), garbage->key_size) == 0) {
      while (true) {
        cursor = cursor->stale;
        assert(cursor);
        if (cursor->stale == garbage) {
          cursor->stale = nullptr;
          break;
        }
      }
      break;
    }
  }
  delete garbage;
  n_entries_--;
  return true;
}

std::unique_lock<std::shared_timed_mutex> HashMap::LockUnique()
{
  return std::unique_lock<std::shared_timed_mutex>(mtx_);
}

std::shared_lock<std::shared_timed_mutex> HashMap::LockShared() const
{
  return std::shared_lock<std::shared_timed_mutex>(mtx_);
}

auto HashMap::MakeBucket(hash_t index) -> Node*
{
  return new Node(ReverseBitwise(index));
}

auto HashMap::MakeVoid(hash_t hash, const void* key, size_t key_size) -> Node*
{
  auto node = new(::operator new(sizeof(Node) + key_size))
    Node(checkpoint_sequence_ + log_list_.size() + 1,
         hash, key, key_size);
  log_list_.push_back(node); // XXX It may throw an exception
  n_entries_++;
  return node;
}

auto HashMap::MakeEntry(hash_t hash, const void* key, size_t key_size,
                        const void* data, size_t data_size) -> Node*
{
  auto node = new(::operator new(sizeof(Node) + key_size + data_size))
    Node(checkpoint_sequence_ + log_list_.size() + 1,
         hash, key, key_size, data, data_size);
  log_list_.push_back(node); // XXX It may throw an exception
  n_entries_++;
  return node;
}

size_t HashMap::EntrySize() const
{
  return n_entries_;
}

size_t HashMap::BucketSize() const
{
  return bucket_list_.size();
}

std::float_t HashMap::LoadFactor() const
{
  return static_cast<std::float_t>(EntrySize()) /
    static_cast<std::float_t>(BucketSize());
}

size_t HashMap::BucketIndex(hash_t hash) const noexcept
{
  const auto reverse_hash = ReverseBitwise(hash);
  auto index = reverse_hash & bucket_mask_;
  if (index >= bucket_list_.size()) {
    index = reverse_hash & (bucket_mask_ >> 1);
  }
  assert(index < bucket_list_.size());
  return index;
}

bool HashMap::Find(hash_t hash, const void* key, size_t key_size,
                   Node*& cursor) noexcept
{
  const auto index = BucketIndex(hash);
  cursor = bucket_list_[index];
  while (cursor->next && cursor->next->hash <= hash) {
    if (cursor->next->hash == hash &&
        cursor->next->key_size == key_size &&
        memcmp(cursor->next->Key(), key, key_size) == 0) {
      return true;
    }
    cursor = cursor->next;
  }
  return false;
}

void HashMap::AddBucket()
{
  if (LoadFactor() < LoadFactorThreshold) {
    return;
  }
  const auto index = bucket_list_.size();
  const auto bucket = MakeBucket(index);
  auto cursor = bucket_list_[BucketIndex(bucket->hash)];
  while (cursor->next && cursor->next->hash < bucket->hash) {
    cursor = cursor->next;
  }
  bucket->next = cursor->next;
  cursor->next = bucket;
  bucket_list_.push_back(bucket);
  if ((index & (index - 1)) == 0) {
    // index is a power of 2
    bucket_mask_ = (bucket_mask_ << 1) + 1;
  }
  assert(cursor->hash <= cursor->next->hash);
  assert(!bucket->next || bucket->hash <= bucket->next->hash);
}

void HashMap::MarkGarbage(Node* garbage)
{
  garbage_heap_.push_back(garbage);
  std::push_heap(garbage_heap_.begin(), garbage_heap_.end(),
                 NodeMinHeapComparator());
}

HashMap::Node::Node(hash_t hash) noexcept
: sequence(0)
, hash(hash)
, next(nullptr)
, stale(nullptr)
, key_size(0)
, data_size(0)
{}

HashMap::Node::Node(sequence_t sequence, hash_t hash,
                    const void* key, size_t key_size) noexcept
: sequence(sequence)
, hash(hash)
, next(nullptr)
, stale(nullptr)
, key_size(key_size)
, data_size(0)
{
  memcpy(const_cast<void*>(Key()), key, key_size);
}

HashMap::Node::Node(sequence_t sequence, hash_t hash,
                    const void* key, size_t key_size,
                    const void* data, size_t data_size) noexcept
: sequence(sequence)
, hash(hash)
, next(nullptr)
, stale(nullptr)
, key_size(key_size)
, data_size(data_size)
{
  memcpy(const_cast<void*>(Key()), key, key_size);
  memcpy(const_cast<void*>(Data()), data, data_size);
}

ptrdiff_t HashMap::Node::KeyOffset() const noexcept
{
  return sizeof(Node);
}

ptrdiff_t HashMap::Node::DataOffset() const noexcept
{
  return sizeof(Node) + key_size;
}

const void* HashMap::Node::Key() const noexcept
{
  return reinterpret_cast<const void*>(
    reinterpret_cast<uintptr_t>(this) + KeyOffset());
}

const void* HashMap::Node::Data() const noexcept
{
  return reinterpret_cast<const void*>(
    reinterpret_cast<uintptr_t>(this) + DataOffset());
}

NodeType HashMap::Node::Type() const noexcept
{
  if (key_size == 0) {
    return NodeType::Bucket;
  } else if (data_size == 0) {
    return NodeType::Void;
  } else {
    return NodeType::Entry;
  }
}

bool HashMap::NodeMinHeapComparator::operator()(const Node* x,
                                                const Node* y) const noexcept
{
  assert(x != nullptr);
  assert(y != nullptr);
  return x->sequence > y->sequence;
}

HashMap::Snapshot::Snapshot(const HashMap& hashmap,
                            sequence_t sequence) noexcept
: hashmap_(&hashmap)
, sequence_(sequence)
{}

sequence_t HashMap::Snapshot::SequenceNumber() const noexcept
{
  return sequence_;
}

bool HashMap::Snapshot::Get(const void* key, size_t key_size,
                            const void*& data, size_t& data_size) const
{
  return hashmap_->Get(key, key_size, data, data_size, sequence_);
}

auto HashMap::Snapshot::begin() const -> SnapshotIterator
{
  auto lock = hashmap_->LockShared();
  auto cursor = hashmap_->bucket_list_.front();
  return SnapshotIterator(*hashmap_, sequence_, cursor);
}

auto HashMap::Snapshot::end() const -> SnapshotIterator
{
  return SnapshotIterator(*hashmap_, sequence_, nullptr);
}

HashMap::SnapshotIterator::SnapshotIterator(const HashMap& hashmap,
                                            sequence_t sequence,
                                            const Node* cursor) noexcept
: hashmap_(&hashmap)
, sequence_(sequence)
, cursor_(cursor)
{
  AdjustCursor();
}

auto HashMap::SnapshotIterator::operator++() noexcept -> SnapshotIterator&
{
  auto lock = hashmap_->LockShared();
  if (cursor_) {
    cursor_ = cursor_->next;
    AdjustCursor();
  }
  return *this;
}

auto HashMap::SnapshotIterator::operator++(int) noexcept -> SnapshotIterator
{
  auto it = *this;
  ++*this;
  return it;
}

auto HashMap::SnapshotIterator::operator*() const noexcept -> SnapshotEntry
{
  if (cursor_) {
    return SnapshotEntry(cursor_->Key(), cursor_->key_size,
                         cursor_->Data(), cursor_->data_size);
  } else {
    return SnapshotEntry();
  }
}

HashMap::SnapshotIterator::operator bool() const noexcept
{
  return cursor_;
}

bool HashMap::SnapshotIterator::operator==(
  const SnapshotIterator& rhs) const noexcept
{
  return cursor_ == rhs.cursor_;
}

bool HashMap::SnapshotIterator::operator!=(
  const SnapshotIterator& rhs) const noexcept
{
  return cursor_ != rhs.cursor_;
}

void HashMap::SnapshotIterator::AdjustCursor() noexcept
{
  while (cursor_) {
    if (cursor_->Type() == NodeType::Bucket) {
      cursor_ = cursor_->next;
      continue;
    }
    while (cursor_->stale && cursor_->sequence > sequence_) {
      cursor_ = cursor_->stale;
    }
    if (cursor_->Type() == NodeType::Void || cursor_->sequence > sequence_) {
      cursor_ = cursor_->next;
      continue;
    }
    return;
  }
}

HashMap::SnapshotEntry::SnapshotEntry() noexcept
: key_size(0)
, data_size(0)
, key(nullptr)
, data(nullptr)
{}

HashMap::SnapshotEntry::SnapshotEntry(
  const void* key, size_t key_size,
  const void* data, size_t data_size) noexcept
: key_size(key_size)
, data_size(data_size)
, key(key)
, data(data)
{}

HashMap::SnapshotEntry::operator bool() const noexcept
{
  return key_size > 0;
}

} // namespace world
