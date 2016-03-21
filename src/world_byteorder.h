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

#pragma once

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <world.h>

static inline uintmax_t world_encode_key_size(world_key_size key_size);
static inline uintmax_t world_decode_key_size(world_key_size key_size);
static inline uintmax_t world_encode_data_size(world_data_size data_size);
static inline uintmax_t world_decode_data_size(world_data_size data_size);

static inline uintmax_t world_encode_key_size(world_key_size key_size)
{
  _Static_assert(sizeof(world_key_size) == sizeof(uint16_t), "world_key_size is uint16_t");
  return htons(key_size);
}

static inline uintmax_t world_decode_key_size(world_key_size key_size)
{
  _Static_assert(sizeof(world_key_size) == sizeof(uint16_t), "world_key_size is uint16_t");
  return ntohs(key_size);
}

static inline uintmax_t world_encode_data_size(world_data_size data_size)
{
  _Static_assert(sizeof(world_data_size) == sizeof(uint16_t), "world_data_size is uint16_t");
  return htons(data_size);
}

static inline uintmax_t world_decode_data_size(world_data_size data_size)
{
  _Static_assert(sizeof(world_data_size) == sizeof(uint16_t), "world_data_size is uint16_t");
  return ntohs(data_size);
}
