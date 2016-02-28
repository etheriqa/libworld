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

#include <cstddef>
#include <world/snapshot.h>

namespace world {

struct OriginOption {
  size_t n_io_threads;

  OriginOption() noexcept;
}; // struct OriginOption

class Origin {
public:
  static Origin* Open(const OriginOption& option);

  virtual ~Origin() noexcept {}

  virtual bool AddReplica(int fd) = 0;
  virtual bool RemoveReplica(int fd) = 0;

  virtual const Snapshot* TakeSnapshot() = 0;

  virtual bool Get(const void* key, size_t key_size,
                   const void*& data, size_t& data_size,
                   const Snapshot* snapshot = nullptr) const = 0;

  virtual bool Set(const void* key, size_t key_size,
                   const void* data, size_t data_size) = 0;
  virtual bool Add(const void* key, size_t key_size,
                   const void* data, size_t data_size) = 0;
  virtual bool Replace(const void* key, size_t key_size,
                       const void* data, size_t data_size) = 0;
  virtual bool Delete(const void* key, size_t key_size) = 0;
}; // class Origin

} // namespace world