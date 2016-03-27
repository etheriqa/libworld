/**
 * Copyright (c) 2016 TAKAMORI Kaede <etheriqa@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include "world_system.h"

bool world_check_fd(int fd)
{
  if (fd < 0) {
    return false;
  }

  struct rlimit rl;
  if (getrlimit(RLIMIT_NOFILE, &rl) == -1) {
    perror("getrlimit");
    return false;
  }

  return (rlim_t)fd < rl.rlim_cur;
}

bool world_set_nonblocking(int fd)
{
  if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl");
    return false;
  }

  return true;
}

bool world_set_tcp_nodelay(int fd)
{
  int option = 1;
  if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)) == -1) {
    if (errno == EOPNOTSUPP) {
      // do nothing
    } else {
      perror("setsockopt");
      return false;
    }
  }

  return true;
}

bool world_ignore_sigpipe(void)
{
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &act, NULL) == -1) {
    perror("sigaction");
    return false;
  }

  return true;
}
