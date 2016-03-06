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

#include <system_error>
#include <sys/uio.h>
#include <world/replica.h>
#include "world/hashmap.h"
#include "world/replica_io_thread.h"

namespace world {

static void FillHeader(struct iovec header[2],
                       size_t& key_size, size_t& data_size)
{
  header[0].iov_base = &key_size;
  header[1].iov_base = &data_size;
  header[0].iov_len = sizeof(size_t);
  header[1].iov_len = sizeof(size_t);
}

static void FillBody(struct iovec body[2],
                     void* key, size_t key_size, void* data, size_t data_size)
{
  body[0].iov_base = key;
  body[1].iov_base = data;
  body[0].iov_len = key_size;
  body[1].iov_len = data_size;
}

static size_t Drain(struct iovec& iov, size_t n_read)
{
  if (n_read == 0) {
    return 0;
  }
  const auto size = std::min(n_read, iov.iov_len);
  iov.iov_base = reinterpret_cast<void*>(
    reinterpret_cast<uintptr_t>(iov.iov_base) + size);
  iov.iov_len -= size;
  return n_read - size;
}

ReplicaIOThread::ReplicaIOThread(std::shared_ptr<const ReplicaOption> option,
                                 std::shared_ptr<HashMap> hashmap)
: option_(option)
, hashmap_(hashmap)
, loop_condition_(true)
, thread_([this]{ this->EventLoop(); })
{}

ReplicaIOThread::~ReplicaIOThread() noexcept
{
  loop_condition_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void ReplicaIOThread::EventLoop()
{
  // XXX endian
  // XXX size_t
  // FIXME this code seems to too ugly...
  enum struct State {
    ReadHeader,
    ReadBody,
  };
  auto state = State::ReadHeader;

  size_t key_size;
  size_t data_size;
  uint8_t key[4096];
  uint8_t data[4096];

  struct iovec header[2];
  struct iovec body[2];

  FillHeader(header, key_size, data_size);
  while (loop_condition_) {
    switch (state) {
    case State::ReadHeader:
      {
        auto n_read = readv(option_->fd, header, 2);
        if (n_read == 0) {
          break;
        }
        if (n_read < 0) {
          if (errno == EAGAIN) {
            continue;
          }
          throw std::system_error(
            std::make_error_code(static_cast<std::errc>(errno)), "readv");
        }
        n_read = Drain(header[0], n_read);
        n_read = Drain(header[1], n_read);
        if (header[0].iov_len + header[1].iov_len == 0) {
          FillBody(body, key, key_size, data, data_size);
          state = State::ReadBody;
        }
      }
      break;
    case State::ReadBody:
      {
        auto n_read = readv(option_->fd, body, 2);
        if (n_read == 0) {
          break;
        }
        if (n_read < 0) {
          if (errno == EAGAIN) {
            continue;
          }
          throw std::system_error(
            std::make_error_code(static_cast<std::errc>(errno)), "readv");
        }
        n_read = Drain(body[0], n_read);
        n_read = Drain(body[1], n_read);
        if (body[0].iov_len + body[1].iov_len == 0) {
          hashmap_->Set(key, key_size, data, data_size);
          FillHeader(header, key_size, data_size);
          state = State::ReadHeader;
        }
      }
      break;
    }
  }
}

} // namespace world
