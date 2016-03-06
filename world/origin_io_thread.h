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

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <world/snapshot.h>
#include "world/event.h"

namespace world {

struct OriginOption;
class HashMap;
class OriginWriter;

class OriginIOThread {
public:
  OriginIOThread(std::shared_ptr<const OriginOption> option,
                 std::shared_ptr<const HashMap> hashmap);

  ~OriginIOThread() noexcept;

  void Register(int fd);
  void Unregister(int fd);

  void Notify();

  sequence_t LeastSequenceNumber() const;

private:
  std::shared_ptr<const OriginOption> option_;
  std::shared_ptr<const HashMap> hashmap_;

  mutable std::mutex mtx_;
  std::unordered_map<int, std::unique_ptr<OriginWriter>> writer_table_;
  std::vector<int> disabled_fd_list_;
  EventDispatcher dispatcher_;

  std::atomic<bool> loop_condition_;
  std::thread thread_;

  std::unique_lock<std::mutex> Lock() const;

  void EventLoop();
  void HandleEvent(OriginWriter& writer);

  void EnableAll();
  void Disable(int fd);
}; // class OriginIOThread

} // namespace world
