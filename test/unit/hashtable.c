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

#include <string.h>
#include "../../src/hashtable.h"
#include "../helper.h"

static void test_hashtable_manipulation(void)
{
  struct hashtable ht;
  hashtable_init(&ht);

  struct world_iovec key, data, found;

  key.base = "foo";
  key.size = strlen(key.base) + 1;
  data.base = "Lorem ipsum";
  data.size = strlen(data.base) + 1;

  // ERROR: GET "foo"
  ASSERT_FALSE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));

  // ERROR: REPLACE "foo" "Lorem ipsum"
  ASSERT_FALSE(hashtable_replace(&ht, key, data));

  // ERROR: DELETE "foo"
  ASSERT_FALSE(hashtable_delete(&ht, key));

  // OK: ADD "foo" "Lorem ipsum"
  ASSERT_TRUE(hashtable_add(&ht, key, data));

  // OK: GET "foo"
  ASSERT_TRUE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  // ERROR: ADD "foo" "Lorem ipsum"
  ASSERT_FALSE(hashtable_add(&ht, key, data));

  // OK: REPLACE "foo" "Lorem ipsum"
  ASSERT_TRUE(hashtable_replace(&ht, key, data));

  // OK: GET "foo"
  ASSERT_TRUE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  data.base = "Lorem ipsum dolor sit amet";
  data.size = strlen(data.base) + 1;

  // ERROR: ADD "foo" "Lorem ipsum dolor sit amet"
  ASSERT_FALSE(hashtable_add(&ht, key, data));

  // OK: REPLACE "foo" "Lorem ipsum dolor sit amet"
  ASSERT_TRUE(hashtable_replace(&ht, key, data));

  // OK: GET "foo"
  ASSERT_TRUE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  key.base = "bar";
  key.size = strlen(key.base) + 1;
  data.base = "consectetur adipiscing el it";
  data.size = strlen(data.base) + 1;

  // OK: SET "bar" "consectetur adipiscing el it"
  ASSERT_TRUE(hashtable_set(&ht, key, data));

  // OK: GET "bar"
  ASSERT_TRUE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  key.base = "foo";
  key.size = strlen(key.base) + 1;

  // OK: DELETE "foo"
  ASSERT_TRUE(hashtable_delete(&ht, key));

  // ERROR: GET "foo"
  ASSERT_FALSE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));

  key.base = "bar";
  key.size = strlen(key.base) + 1;
  data.base = "consectetur adipiscing el it";
  data.size = strlen(data.base) + 1;

  // OK: GET "bar"
  ASSERT_TRUE(hashtable_get(&ht, key, &found, hashtable_get_sequence(&ht)));
  ASSERT(found.size == data.size);
  ASSERT(memcmp(found.base, data.base, data.size) == 0);

  hashtable_destroy(&ht);
}

int main(void)
{
  test_hashtable_manipulation();
  return TEST_STATUS;
}
