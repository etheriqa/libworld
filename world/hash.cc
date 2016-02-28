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

#include "world/hash.h"

namespace world {

hash_t Hash(const void* data, size_t size) noexcept
{
  // TODO

  const hash_t P = 0xaaaaaa97;
  const hash_t A = 0x9e3779b9;

  hash_t hash = 0;
  while (size >= sizeof(hash_t)) {
    hash ^= *static_cast<const hash_t*>(data);
    hash *= P;
    data = static_cast<const void*>(static_cast<const hash_t*>(data) + 1);
    size -= sizeof(hash_t);
  }
  while (size > 0) {
    hash ^= *static_cast<const uint8_t*>(data);
    hash *= P;
    data = static_cast<const void*>(static_cast<const uint8_t*>(data) + 1);
    size -= sizeof(uint8_t);
  }

  return hash * A;
}

} // namespace world
