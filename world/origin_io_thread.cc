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

#include <world/origin.h>
#include "world/origin_io_thread.h"
#include "world/origin_writer.h"
#include "world/hashmap.h"

namespace world {

OriginIOThread::OriginIOThread(std::shared_ptr<const OriginOption> option,
                               std::shared_ptr<const HashMap> hashmap)
: option_(std::move(option))
, hashmap_(std::move(hashmap))
, mtx_()
, writer_table_{}
, disabled_fd_list_{}
, dispatcher_()
, loop_condition_(true)
, thread_([this]{ this->EventLoop(); })
{}

OriginIOThread::~OriginIOThread() noexcept
{
  loop_condition_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void OriginIOThread::Register(int fd)
{
  auto lock = Lock();
  auto writer = std::make_unique<OriginWriter>(option_, hashmap_, fd);
  writer_table_.emplace(fd, std::move(writer));
  // XXX Register() can throw an exception; it makes an inconsistent state
  dispatcher_.Register(fd, writer_table_[fd].get());
}

void OriginIOThread::Unregister(int fd)
{
  auto lock = Lock();
  dispatcher_.Unregister(fd);
  writer_table_.erase(fd);
}

void OriginIOThread::Notify()
{
  auto lock = Lock();
  EnableAll();
}

sequence_t OriginIOThread::LeastSequenceNumber() const
{
  auto sequence = hashmap_->SequenceNumber();
  auto lock = Lock();
  for (const auto& kv : writer_table_) {
    sequence = std::min(sequence, kv.second->SequenceNumber());
  }
  return sequence;
}

std::unique_lock<std::mutex> OriginIOThread::Lock() const
{
  return std::unique_lock<std::mutex>(mtx_);
}

void OriginIOThread::EventLoop()
{
  while (loop_condition_) {
    dispatcher_.Dispatch(
      [this](auto ctx){ this->HandleEvent(*static_cast<OriginWriter*>(ctx)); },
      option_->event_timeout_ms);
  }
}

void OriginIOThread::HandleEvent(OriginWriter& writer)
{
  writer.Write();
  if (writer.UpToDate()) {
    Disable(writer.FD());
  }
}

void OriginIOThread::EnableAll()
{
  for (const auto fd : disabled_fd_list_) {
    dispatcher_.Register(fd, writer_table_[fd].get());
  }
  disabled_fd_list_.clear();
}

void OriginIOThread::Disable(int fd)
{
  dispatcher_.Unregister(fd);
  disabled_fd_list_.push_back(fd);
}

} // namespace world
