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

hash_t Hash(const void* key, size_t size) noexcept
{
  // We employ MurmurHash1.
  // https://github.com/aappleby/smhasher/blob/master/src/MurmurHash1.cpp

  const hash_t m = 0xc6a4a793;

  hash_t h = size * m;

  while (size >= sizeof(hash_t)) {
    h += static_cast<const hash_t*>(key)[0];
    h *= m;
    h ^= h >> 16;
    key = static_cast<const void*>(static_cast<const hash_t*>(key) + 1);
    size -= sizeof(hash_t);
  }

  switch (size) {
  case 3:
    h += static_cast<const uint8_t*>(key)[2] << 16;
  case 2:
    h += static_cast<const uint8_t*>(key)[1] << 8;
  case 1:
    h += static_cast<const uint8_t*>(key)[0];
    h *= m;
    h ^= h >> 16;
  }

  h *= m;
  h ^= h >> 10;
  h *= m;
  h ^= h >> 17;

  return h;
}

} // namespace world
