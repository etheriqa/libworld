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

#include "world/hashmap.h"
#include "world/origin_internal.h"
#include "world/origin_io_thread.h"

namespace world {

Origin* Origin::Open(const OriginOption& option)
{
  return new OriginInternal(option);
}

OriginInternal::OriginInternal(const OriginOption& option)
: option_(std::make_shared<OriginOption>(option))
, hashmap_(std::make_shared<HashMap>())
, io_thread_list_{}
, snapshot_set_(hashmap_)
{
  for (size_t i = 0; i < option_->n_io_threads; i++) {
    io_thread_list_.emplace_back(
      std::make_unique<OriginIOThread>(option_, hashmap_));
  }
}

bool OriginInternal::AddReplica(int fd)
{
  io_thread_list_[fd % io_thread_list_.size()]->Register(fd);
  return true;
}

bool OriginInternal::RemoveReplica(int fd)
{
  io_thread_list_[fd % io_thread_list_.size()]->Unregister(fd);
  return true;
}

const Snapshot* OriginInternal::TakeSnapshot()
{
  return snapshot_set_.TakeSnapshot();
}

bool OriginInternal::Get(const void* key, size_t key_size,
                         const void*& data, size_t& data_size) const
{
  return hashmap_->Get(key, key_size, data, data_size);
}

bool OriginInternal::Set(const void* key, size_t key_size,
                         const void* data, size_t data_size)
{
  if (!hashmap_->Set(key, key_size, data, data_size)) {
    return false;
  }
  NotifyIOThreads();
  Checkpoint();
  return true;
}

bool OriginInternal::Add(const void* key, size_t key_size,
                         const void* data, size_t data_size)
{
  if (!hashmap_->Add(key, key_size, data, data_size)) {
    return false;
  }
  NotifyIOThreads();
  Checkpoint();
  return true;
}

bool OriginInternal::Replace(const void* key, size_t key_size,
                             const void* data, size_t data_size)
{
  if (!hashmap_->Replace(key, key_size, data, data_size)) {
    return false;
  }
  NotifyIOThreads();
  Checkpoint();
  return true;
}

bool OriginInternal::Delete(const void* key, size_t key_size)
{
  if (!hashmap_->Delete(key, key_size)) {
    return false;
  }
  NotifyIOThreads();
  Checkpoint();
  return true;
}

void OriginInternal::NotifyIOThreads()
{
  for (const auto& io_thread : io_thread_list_) {
    io_thread->Notify();
  }
}

void OriginInternal::Checkpoint()
{
  sequence_t sequence = hashmap_->SequenceNumber();
  if (hashmap_->SequenceNumber() % option_->checkpoint_interval > 0) {
    return;
  }
  for (const auto& io_thread : io_thread_list_) {
    sequence = std::min(sequence, io_thread->LeastSequenceNumber());
  }
  sequence = std::min(sequence, snapshot_set_.LeastSequenceNumber());
  hashmap_->Checkpoint(sequence);
}

} // namespace world
