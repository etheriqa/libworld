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

#include <memory>
#include <vector>
#include <world/origin.h>
#include "world/snapshot_set.h"

namespace world {

class HashMap;
class OriginIOThread;

class OriginInternal : public Origin {
public:
  explicit OriginInternal(const OriginOption& option);

  bool AddReplica(int fd);
  bool RemoveReplica(int fd);

  const Snapshot* TakeSnapshot();

  bool Get(const void* key, size_t key_size,
           const void*& data, size_t& data_size) const;

  bool Set(const void* key, size_t key_size,
           const void* data, size_t data_size);
  bool Add(const void* key, size_t key_size,
           const void* data, size_t data_size);
  bool Replace(const void* key, size_t key_size,
               const void* data, size_t data_size);
  bool Delete(const void* key, size_t key_size);

private:
  std::shared_ptr<const OriginOption> option_;
  std::shared_ptr<HashMap> hashmap_;
  std::vector<std::unique_ptr<OriginIOThread>> io_thread_list_;
  SnapshotSet snapshot_set_;

  void NotifyIOThreads();
  void Checkpoint();
}; // class OriginInternal

} // namespace world
