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
#include <algorithm>
#include <system_error>
#include <unistd.h>
#include <world/origin.h>
#include "world/origin_writer.h"

namespace world {

OriginWriter::OriginWriter(std::shared_ptr<const OriginOption> option,
                           std::shared_ptr<const HashMap> hashmap,
                           int fd)
: option_(option)
, hashmap_(hashmap)
, fd_(fd)
, sequence_(hashmap_->SequenceNumber())
, iterator_(hashmap_->TakeSnapshot(sequence_).begin())
, iov_list_{}
, iov_size_(0)
{}

OriginWriter::~OriginWriter() noexcept
{
  if (option_->close_on_destruct) {
    close(fd_);
  }
}

int OriginWriter::FD() const noexcept
{
  return fd_;
}

sequence_t OriginWriter::SequenceNumber() const noexcept
{
  return sequence_;
}

bool OriginWriter::UpToDate() const noexcept
{
  return !iterator_ && sequence_ == hashmap_->SequenceNumber();
}

void OriginWriter::Write()
{
  for (size_t i = 0; i < 10; i++) { // TODO define parameter
    if (iov_size_ == 0) {
      const auto entry = iterator_ ? *iterator_ : hashmap_->Log(sequence_ + 1);
      if (!entry) {
        return;
      }
      FillIOVec(entry);
    }
    const auto n_written = writev(fd_, iov_list_, IOVecSize);
    if (n_written < 0) {
      if (errno == EAGAIN) {
        return;
      }
      throw std::system_error(
        std::make_error_code(static_cast<std::errc>(errno)), "writev");
    }
    DrainIOVec(n_written);
    if (iov_size_ > 0) {
      return;
    }
    if (iterator_) {
      iterator_++;
    } else {
      sequence_++;
    }
  }
}

void OriginWriter::FillIOVec(
  const HashMap::SnapshotEntry& entry) noexcept
{
  // XXX endian
  iov_list_[0].iov_base = const_cast<size_t*>(&entry.key_size);
  iov_list_[1].iov_base = const_cast<size_t*>(&entry.data_size);
  iov_list_[2].iov_base = const_cast<void*>(entry.key);
  iov_list_[3].iov_base = const_cast<void*>(entry.data);
  assert(iov_size_ == 0);
  iov_size_ += iov_list_[0].iov_len = sizeof(entry.key_size);
  iov_size_ += iov_list_[1].iov_len = sizeof(entry.data_size);
  iov_size_ += iov_list_[2].iov_len = entry.key_size;
  iov_size_ += iov_list_[3].iov_len = entry.data_size;
}

void OriginWriter::DrainIOVec(size_t n_written) noexcept
{
  for (auto iov = iov_list_; iov < iov_list_ + IOVecSize; iov++) {
    if (n_written == 0) {
      break;
    }
    const auto size = std::min(n_written, iov->iov_len);
    iov->iov_base = reinterpret_cast<void*>(
      reinterpret_cast<uintptr_t>(iov->iov_base) + size);
    iov->iov_len -= size;
    iov_size_ -= size;
    n_written -= size;
  }
}

} // namespace world
