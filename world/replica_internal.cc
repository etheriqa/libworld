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

#include "world/replica_internal.h"

namespace world {

ReplicaOption::ReplicaOption(int fd) noexcept
: fd(fd)
{}

Replica* Replica::Open(const ReplicaOption& option)
{
  return new ReplicaInternal(option);
}

ReplicaInternal::ReplicaInternal(const ReplicaOption& option)
: option_(option)
, storage_()
{
  // TODO
}

const Snapshot* ReplicaInternal::TakeSnapshot()
{
  return storage_.TakeSnapshot();
}

bool ReplicaInternal::Get(const void* key, size_t key_size,
                          const void*& data, size_t& data_size,
                          const Snapshot* snapshot) const
{
  if (snapshot) {
    return storage_.Get(key, key_size, data, data_size,
                        snapshot->SequenceNumber());
  } else {
    return storage_.Get(key, key_size, data, data_size);
  }
}

} // namespace world
