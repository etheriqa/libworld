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
#include <sys/uio.h>
#include "world/hashmap.h"

namespace world {

struct OriginOption;

class OriginWriter {
public:
  OriginWriter(std::shared_ptr<const OriginOption> option,
               std::shared_ptr<const HashMap> hashmap,
               int fd);

  ~OriginWriter() noexcept;

  int FD() const noexcept;
  sequence_t SequenceNumber() const noexcept;
  bool UpToDate() const noexcept;

  void Write();

private:
  static constexpr size_t IOVecSize = 4;

  std::shared_ptr<const OriginOption> option_;
  std::shared_ptr<const HashMap> hashmap_;

  int fd_;

  sequence_t sequence_;
  HashMap::SnapshotIterator iterator_;

  struct iovec iov_list_[IOVecSize];
  size_t iov_size_;

  void FillIOVec(const HashMap::SnapshotEntry& entry) noexcept;
  void DrainIOVec(size_t n_written) noexcept;
}; // class OriginWriter

} // namespace world
