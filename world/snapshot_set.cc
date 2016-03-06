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
#include "world/hashmap.h"

namespace world {

class SnapshotSet::Snapshot : public world::Snapshot {
public:
  Snapshot(SnapshotSet& snapshot_set, const HashMap::Snapshot& snapshot);

  ~Snapshot() noexcept;

  bool Get(const void* key, size_t key_size,
           const void*& data, size_t& data_size) const;

private:
  SnapshotSet& snapshot_set_;
  HashMap::Snapshot snapshot_;
}; // class SnapshotSet::Sequence

SnapshotSet::SnapshotSet(std::shared_ptr<const HashMap> hashmap)
: hashmap_(std::move(hashmap))
, mtx_()
, sequence_set_{}
{}

sequence_t SnapshotSet::LeastSequenceNumber() const
{
  auto lock = Lock();
  if (sequence_set_.empty()) {
    return hashmap_->SequenceNumber();
  } else {
    return *sequence_set_.begin();
  }
}

world::Snapshot* SnapshotSet::TakeSnapshot()
{
  auto lock = Lock();
  const auto sequence = hashmap_->SequenceNumber();
  sequence_set_.emplace(sequence);
  return new SnapshotSet::Snapshot(*this, hashmap_->TakeSnapshot(sequence));
}

std::unique_lock<std::mutex> SnapshotSet::Lock() const
{
  return std::unique_lock<std::mutex>(mtx_);
}

SnapshotSet::Snapshot::Snapshot(SnapshotSet& snapshot_set,
                                const HashMap::Snapshot& snapshot)
: snapshot_set_(snapshot_set)
, snapshot_(snapshot)
{}

SnapshotSet::Snapshot::~Snapshot() noexcept
{
  auto lock = snapshot_set_.Lock();
  auto& sequence_set = snapshot_set_.sequence_set_;
  sequence_set.erase(sequence_set.find(snapshot_.SequenceNumber()));
}

bool SnapshotSet::Snapshot::Get(const void* key, size_t key_size,
                                const void*& data, size_t& data_size) const
{
  return snapshot_.Get(key, key_size, data, data_size);
}

} // namespace world
