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
#include <sys/epoll.h>
#include <unistd.h>
#include "world/event_epoll.h"

namespace world {

EventDispatcherEPoll::EventDispatcherEPoll()
: ep_(epoll_create(1))
{
  if (ep_ < 0) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "epoll_create");
  }
}

EventDispatcherEPoll::~EventDispatcherEPoll() noexcept
{
  close(ep_);
}

void EventDispatcherEPoll::Register(int fd, void* ctx)
{
  epoll_event event;
  memset(&event, 0, sizeof(struct event));
  event.events = EPOLLOUT;
  event.data.ptr = ctx;
  if (epoll_ctl(ep_, EPOLL_CTL_ADD, fd, &event) == 0) {
    return;
  }
  if (errno == EEXIST) {
    return;
  }
  throw std::system_error(
    std::make_error_code(static_cast<std::errc>(errno)), "epoll_ctl");
}

void EventDispatcherEPoll::Unregister(int fd)
{
  epoll_event event;
  memset(&event, 0, sizeof(struct event));
  event.events = EPOLLOUT;
  if (epoll_ctl(ep_, EPOLL_CTL_DEL, fd, &event) == 0) {
    return;
  }
  if (errno == ENOENT) {
    return;
  }
  throw std::system_error(
    std::make_error_code(static_cast<std::errc>(errno)), "epoll_ctl");
}

size_t EventDispatcherEPoll::Dispatch(std::function<void(void*)> callback,
                                      size_t timeout_ms)
{
  epoll_event events[EventBufferSize];
  const auto n_events = epoll_wait(ep_, events, EventBufferSize, timeout_ms);
  if (n_events < 0) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "epoll_wait");
  }
  for (auto event = events; event < events + n_events; event++) {
    // TODO error handling
    callback(event->data.ptr);
  }
}

}; // namespace world
