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
#include "world/snapshot_set.h"

namespace world {

struct SnapshotSet::Node : public Snapshot {
  std::recursive_mutex& mtx;
  const sequence_t sequence;
  Node* prev;
  Node* next;

  Node(std::recursive_mutex& mtx) noexcept;
  Node(std::recursive_mutex& mtx, sequence_t sequence,
       Node* prev, Node* next) noexcept;
  Node(const Node&) = delete;
  Node(Node&&) = delete;

  ~Node() noexcept;

  Node& operator=(const Node&) = delete;
  Node& operator=(Node&&) = delete;

  sequence_t SequenceNumber() const noexcept;
}; // struct SnapshotSet::Node

SnapshotSet::SnapshotSet()
: mtx_(new std::recursive_mutex)
, sentinel_(new Node(*mtx_))
{}

SnapshotSet::~SnapshotSet() noexcept
{
  assert(sentinel_->prev == sentinel_);
  assert(sentinel_->next == sentinel_);
  delete sentinel_;
  delete mtx_;
}

bool SnapshotSet::LeastSequenceNumber(sequence_t& sequence) const
{
  auto lock = Lock();
  if (sentinel_->next == sentinel_) {
    return false;
  }
  sequence = sentinel_->next->sequence;
  return true;
}

bool SnapshotSet::GreatestSequenceNumber(sequence_t& sequence) const
{
  auto lock = Lock();
  if (sentinel_->prev == sentinel_) {
    return false;
  }
  sequence = sentinel_->prev->sequence;
  return true;
}

Snapshot* SnapshotSet::TakeSnapshot(sequence_t sequence)
{
  auto lock = Lock();
  auto node = new Node(*mtx_, sequence, sentinel_->prev, sentinel_);
  assert(node->prev->sequence <= sequence);
  node->prev->next = node;
  node->next->prev = node;
  return node;
}

std::unique_lock<std::recursive_mutex> SnapshotSet::Lock() const
{
  return std::unique_lock<std::recursive_mutex>(*mtx_);
}

SnapshotSet::Node::Node(std::recursive_mutex& mtx) noexcept
: mtx(mtx)
, sequence(0)
, prev(this)
, next(this)
{}

SnapshotSet::Node::Node(std::recursive_mutex& mtx, sequence_t sequence,
                        Node* prev, Node* next) noexcept
: mtx(mtx)
, sequence(sequence)
, prev(prev)
, next(next)
{}

SnapshotSet::Node::~Node() noexcept
{
  // XXX it may throw an exception
  std::unique_lock<std::recursive_mutex> lock(mtx);
  prev->next = next;
  next->prev = prev;
}

sequence_t SnapshotSet::Node::SequenceNumber() const noexcept
{
  return sequence;
}

} // namespace world
