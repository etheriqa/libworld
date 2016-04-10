/*
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

#include "../../src/world_hash.h"
#include "../helper.h"

static void test_world_hash_reverse(void)
{
  EXPECT(world_hash_reverse(0x00000000) == 0x00000000);
  EXPECT(world_hash_reverse(0xFFFFFFFF) == 0xFFFFFFFF);

  EXPECT(world_hash_reverse(0x00000001) == 0x80000000);
  EXPECT(world_hash_reverse(0x00000002) == 0x40000000);
  EXPECT(world_hash_reverse(0x00000003) == 0xC0000000);
  EXPECT(world_hash_reverse(0x00000004) == 0x20000000);
  EXPECT(world_hash_reverse(0x00000005) == 0xA0000000);
  EXPECT(world_hash_reverse(0x00000006) == 0x60000000);
  EXPECT(world_hash_reverse(0x00000007) == 0xE0000000);
  EXPECT(world_hash_reverse(0x00000008) == 0x10000000);
  EXPECT(world_hash_reverse(0x00000009) == 0x90000000);
  EXPECT(world_hash_reverse(0x0000000A) == 0x50000000);
  EXPECT(world_hash_reverse(0x0000000B) == 0xD0000000);
  EXPECT(world_hash_reverse(0x0000000C) == 0x30000000);
  EXPECT(world_hash_reverse(0x0000000D) == 0xB0000000);
  EXPECT(world_hash_reverse(0x0000000E) == 0x70000000);
  EXPECT(world_hash_reverse(0x0000000F) == 0xF0000000);

  EXPECT(world_hash_reverse(0x00000001) == 0x80000000);
  EXPECT(world_hash_reverse(0x00000002) == 0x40000000);
  EXPECT(world_hash_reverse(0x00000004) == 0x20000000);
  EXPECT(world_hash_reverse(0x00000008) == 0x10000000);
  EXPECT(world_hash_reverse(0x00000010) == 0x08000000);
  EXPECT(world_hash_reverse(0x00000020) == 0x04000000);
  EXPECT(world_hash_reverse(0x00000040) == 0x02000000);
  EXPECT(world_hash_reverse(0x00000080) == 0x01000000);
  EXPECT(world_hash_reverse(0x00000100) == 0x00800000);
  EXPECT(world_hash_reverse(0x00000200) == 0x00400000);
  EXPECT(world_hash_reverse(0x00000400) == 0x00200000);
  EXPECT(world_hash_reverse(0x00000800) == 0x00100000);
  EXPECT(world_hash_reverse(0x00001000) == 0x00080000);
  EXPECT(world_hash_reverse(0x00002000) == 0x00040000);
  EXPECT(world_hash_reverse(0x00004000) == 0x00020000);
  EXPECT(world_hash_reverse(0x00008000) == 0x00010000);
  EXPECT(world_hash_reverse(0x00010000) == 0x00008000);
  EXPECT(world_hash_reverse(0x00020000) == 0x00004000);
  EXPECT(world_hash_reverse(0x00040000) == 0x00002000);
  EXPECT(world_hash_reverse(0x00080000) == 0x00001000);
  EXPECT(world_hash_reverse(0x00100000) == 0x00000800);
  EXPECT(world_hash_reverse(0x00200000) == 0x00000400);
  EXPECT(world_hash_reverse(0x00400000) == 0x00000200);
  EXPECT(world_hash_reverse(0x00800000) == 0x00000100);
  EXPECT(world_hash_reverse(0x01000000) == 0x00000080);
  EXPECT(world_hash_reverse(0x02000000) == 0x00000040);
  EXPECT(world_hash_reverse(0x04000000) == 0x00000020);
  EXPECT(world_hash_reverse(0x08000000) == 0x00000010);
  EXPECT(world_hash_reverse(0x10000000) == 0x00000008);
  EXPECT(world_hash_reverse(0x20000000) == 0x00000004);
  EXPECT(world_hash_reverse(0x40000000) == 0x00000002);
  EXPECT(world_hash_reverse(0x80000000) == 0x00000001);

  EXPECT(world_hash_reverse(0xDEADBEEF) == 0xF77DB57B);
}

int main(void)
{
  test_world_hash_reverse();
  return TEST_STATUS;
}
