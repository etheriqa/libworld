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
#include <thread>
#include <fcntl.h>
#include <sys/socket.h>
#include <world/origin.h>
#include <world/replica.h>
#include "world/test.h"

using namespace world;

int main()
{
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "socketpair");
  }
  if (fcntl(fds[1], F_SETFL, O_NONBLOCK) == -1) {
    throw std::system_error(
      std::make_error_code(static_cast<std::errc>(errno)), "fcntl");
  }

  Origin* origin;
  {
    OriginOption option;
    option.close_on_destruct = true;
    origin = Origin::Open(option);
  }

  {
    const char key[] = "foo";
    const char data[] = "Lorem ipsum";
    ASSERT_TRUE(origin->Set(key, sizeof(key), data, sizeof(data)));
  }

  Replica* replica;
  {
    ReplicaOption option;
    option.fd = fds[0];
    replica = Replica::Open(option);
  }

  origin->AddReplica(fds[1]);
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  {
    const char key[] = "foo";
    const char data[] = "Lorem ipsum";
    const void* buf;
    size_t buf_size;
    ASSERT_TRUE(replica->Get(key, sizeof(key), buf, buf_size));
    ASSERT(buf_size == sizeof(data));
    ASSERT(memcmp(buf, data, buf_size) == 0);
  }

  {
    const char key[] = "bar";
    const char data[] = "dolor sit amet";
    ASSERT_TRUE(origin->Set(key, sizeof(key), data, sizeof(data)));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const void* buf;
    size_t buf_size;
    ASSERT_TRUE(replica->Get(key, sizeof(key), buf, buf_size));
    ASSERT(buf_size == sizeof(data));
    ASSERT(memcmp(buf, data, buf_size) == 0);
  }

  delete origin;
  delete replica;

  return STATUS;
}
