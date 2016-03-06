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

#include <cstring>
#include <system_error>
#include <poll.h>
#include "world/event_poll.h"

namespace world {

EventDispatcherPoll::EventDispatcherPoll()
: mtx_()
, fd_table_{}
{}

void EventDispatcherPoll::Register(int fd, void* ctx)
{
  fd_table_.emplace(fd, ctx);
}

void EventDispatcherPoll::Unregister(int fd)
{
  fd_table_.erase(fd);
}

size_t EventDispatcherPoll::Dispatch(std::function<void(void*)> callback,
                                     size_t timeout_ms)
{
  const auto event_size = fd_table_.size();
  const auto events = new pollfd[event_size];
  memset(events, 0, sizeof(pollfd) * event_size);
  {
    auto event = events;
    for (const auto& kv : fd_table_) {
      event->fd = kv.first;
      event->events = POLLOUT;
      event++;
    }
  }
  const auto n_events = poll(events, event_size, timeout_ms);
  if (n_events < 0) {
    delete[] events;
    if (errno == EAGAIN) {
      return 0;
    }
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "poll");
  }
  for (auto event = events; event < events + event_size; event++) {
    if (event->revents & POLLERR) {
      fd_table_.erase(event->fd);
    }
    if (event->revents & POLLOUT) {
      callback(fd_table_[event->fd]);
    }
  }
  delete[] events;
  return n_events;
}

} // namespace world
