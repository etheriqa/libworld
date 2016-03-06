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
#include <cstdint>
#include <random>
#include <unordered_map>
#include "world/hashmap.h"
#include "world/test.h"

using namespace world;

void TestSimpleManipulation()
{
  HashMap hashmap;

  const void* found;
  size_t found_size;

  const char key[] = "foo";
  const char data0[] = "Lorem ipsum";

  EXPECT_FALSE(hashmap.Get(key, sizeof(key), found, found_size));
  EXPECT_FALSE(hashmap.Replace(key, sizeof(key), data0, sizeof(data0)));
  EXPECT_FALSE(hashmap.Delete(key, sizeof(key)));

  // Add data0.
  {
    EXPECT(hashmap.SequenceNumber() == 0);
    EXPECT_TRUE(hashmap.Add(key, sizeof(key), data0, sizeof(data0)));
    EXPECT(hashmap.SequenceNumber() == 1);

    EXPECT_TRUE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT(found_size == sizeof(data0));
    EXPECT(memcmp(found, data0, found_size) == 0);

    EXPECT_FALSE(hashmap.CollectGarbage());
    hashmap.Checkpoint(hashmap.SequenceNumber());
    EXPECT_FALSE(hashmap.CollectGarbage());
  }

  // Replace with data0.
  // NOTE: It does not check equivalence of data.
  {
    EXPECT(hashmap.SequenceNumber() == 1);
    EXPECT_TRUE(hashmap.Replace(key, sizeof(key), data0, sizeof(data0)));
    EXPECT(hashmap.SequenceNumber() == 2);

    EXPECT_TRUE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT(found_size == sizeof(data0));
    EXPECT(memcmp(found, data0, found_size) == 0);

    EXPECT_TRUE(hashmap.CollectGarbage()); // seq = 1
    EXPECT_FALSE(hashmap.CollectGarbage());
    hashmap.Checkpoint(hashmap.SequenceNumber());
    EXPECT_FALSE(hashmap.CollectGarbage());

    // Checkpoint and garbage collection causes no side effect externally.
    EXPECT_TRUE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT(found_size == sizeof(data0));
    EXPECT(memcmp(found, data0, found_size) == 0);
  }

  const char data1[] = "dolor sit amet";

  EXPECT_FALSE(hashmap.Add(key, sizeof(key), data1, sizeof(data1)));

  // Replace with data1.
  {
    EXPECT(hashmap.SequenceNumber() == 2);
    EXPECT_TRUE(hashmap.Replace(key, sizeof(key), data1, sizeof(data1)));
    EXPECT(hashmap.SequenceNumber() == 3);

    EXPECT_TRUE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT(found_size == sizeof(data1));
    EXPECT(memcmp(found, data1, found_size) == 0);

    EXPECT_TRUE(hashmap.CollectGarbage()); // seq = 2
    EXPECT_FALSE(hashmap.CollectGarbage());
    hashmap.Checkpoint(hashmap.SequenceNumber());
    EXPECT_FALSE(hashmap.CollectGarbage());

    EXPECT_TRUE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT(found_size == sizeof(data1));
    EXPECT(memcmp(found, data1, found_size) == 0);
  }

  // Delete.
  {
    EXPECT(hashmap.SequenceNumber() == 3);
    EXPECT_TRUE(hashmap.Delete(key, sizeof(key)));
    EXPECT(hashmap.SequenceNumber() == 4);

    EXPECT_FALSE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT_FALSE(hashmap.Replace(key, sizeof(key), data1, sizeof(data1)));
    EXPECT_FALSE(hashmap.Delete(key, sizeof(key)));

    EXPECT_TRUE(hashmap.CollectGarbage()); // seq = 3
    EXPECT_FALSE(hashmap.CollectGarbage());
    hashmap.Checkpoint(hashmap.SequenceNumber());
    EXPECT_TRUE(hashmap.CollectGarbage()); // seq = 4
    EXPECT_FALSE(hashmap.CollectGarbage());

    EXPECT_FALSE(hashmap.Get(key, sizeof(key), found, found_size));
    EXPECT_FALSE(hashmap.Replace(key, sizeof(key), data1, sizeof(data1)));
    EXPECT_FALSE(hashmap.Delete(key, sizeof(key)));
  }
}

void TestRandomManipulation()
{
  using key_t = uint16_t;
  using data_t = uint64_t;
  std::unordered_map<key_t, data_t> reference;
  HashMap hashmap;

  std::mt19937_64 prng((std::random_device()()));
  for (size_t i = 0; i < 1000000; i++) {
    const auto key = std::uniform_int_distribution<key_t>()(prng);
    const auto data = std::uniform_int_distribution<data_t>()(prng);
    switch (std::uniform_int_distribution<uint8_t>(0, 9)(prng)) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      // Set
      reference[key] = data;
      hashmap.Set(&key, sizeof(key_t), &data, sizeof(data_t));
      break;
    case 7:
      // Add
      reference.emplace(key, data);
      hashmap.Add(&key, sizeof(key_t), &data, sizeof(data_t));
      break;
    case 8:
      // Replace
      if (reference.count(key)) {
        reference[key] = data;
      }
      hashmap.Replace(&key, sizeof(key_t), &data, sizeof(data_t));
      break;
    case 9:
      // Delete
      reference.erase(key);
      hashmap.Delete(&key, sizeof(key_t));
      break;
    }
    hashmap.Checkpoint(hashmap.SequenceNumber());
    while (hashmap.CollectGarbage()) {}
  }

  {
    for (const auto& kv : reference) {
      const void* found;
      size_t found_size;
      ASSERT_TRUE(hashmap.Get(&kv.first, sizeof(key_t), found, found_size));
      ASSERT(found_size == sizeof(data_t));
      ASSERT(*static_cast<const data_t*>(found) == kv.second);
    }
  }

  {
    size_t counter = 0;
    for (const auto& entry : hashmap.TakeSnapshot(hashmap.SequenceNumber())) {
      counter++;
      ASSERT(entry.key_size == sizeof(key_t));
      ASSERT(entry.data_size == sizeof(data_t));
      const auto key = *static_cast<const key_t*>(entry.key);
      const auto data = *static_cast<const data_t*>(entry.data);
      ASSERT(reference.count(key) == 1);
      ASSERT(reference[key] == data);
    }
    ASSERT(counter == reference.size());
  }
}

int main()
{
  TestSimpleManipulation();
  TestRandomManipulation();
  return STATUS;
}
