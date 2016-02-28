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

#include "world/bitwise.h"

namespace world {

uint32_t ReverseBitwise(uint32_t data) noexcept
{
  data = ((data & 0x55555555) << 1) | ((data & 0xAAAAAAAA) >> 1);
  data = ((data & 0x33333333) << 2) | ((data & 0xCCCCCCCC) >> 2);
  data = ((data & 0x0F0F0F0F) << 4) | ((data & 0xF0F0F0F0) >> 4);
  data = ((data & 0x00FF00FF) << 8) | ((data & 0xFF00FF00) >> 8);
  data = ((data & 0x0000FFFF) << 16) | ((data & 0xFFFF0000) >> 16);
  return data;
}

} // namespace world
