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
#include <sys/event.h>
#include <unistd.h>
#include "world/event_kqueue.h"

namespace world {

EventDispatcherKQueue::EventDispatcherKQueue()
: kq_(kqueue())
{
  if (kq_ < 0) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "kqueue");
  }
}

EventDispatcherKQueue::~EventDispatcherKQueue() noexcept
{
  close(kq_);
}

void EventDispatcherKQueue::Register(int fd, void* ctx)
{
  struct kevent event;
  memset(&event, 0, sizeof(struct kevent));
  event.ident = fd;
  event.filter = EVFILT_WRITE;
  event.flags = EV_ADD;
  event.udata = ctx;
  if (kevent(kq_, &event, 1, nullptr, 0, nullptr) >= 0) {
    return;
  }
  throw std::system_error(
    std::make_error_code(static_cast<std::errc>(errno)), "kevent");
}

void EventDispatcherKQueue::Unregister(int fd)
{
  struct kevent event;
  memset(&event, 0, sizeof(struct kevent));
  event.ident = fd;
  event.filter = EVFILT_WRITE;
  event.flags = EV_DELETE;
  if (kevent(kq_, &event, 1, nullptr, 0, nullptr) >= 0) {
    return;
  }
  if (errno == ENOENT) {
    return;
  }
  throw std::system_error(
    std::make_error_code(static_cast<std::errc>(errno)), "kevent");
}

size_t EventDispatcherKQueue::Dispatch(std::function<void(void*)> callback,
                                       size_t timeout_ms)
{
  struct kevent events[EventBufferSize];
  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = timeout_ms * 1000000;
  const auto n_events =
    kevent(kq_, nullptr, 0, events, EventBufferSize, &timeout);
  if (n_events < 0) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "kevent");
  }
  for (auto event = events; event < events + n_events; event++) {
    // TODO error handling
    callback(event->udata);
  }
  return n_events;
}

} // namespace world
