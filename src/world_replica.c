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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world_hash.h"
#include "world_replica.h"
#include "world_system.h"

static bool _validate_conf(const struct world_replicaconf *conf);

enum world_error world_replica_open(struct world_replica **r, const struct world_replicaconf *conf)
{
  if (!_validate_conf(conf)) {
    return world_error_invalid_argument;
  }

  struct world_allocator allocator;
  world_allocator_init(&allocator);
  struct world_replica *replica = world_allocator_malloc(&allocator, sizeof(*replica));
  memcpy(&replica->allocator, &allocator, sizeof(allocator));

  memcpy((void *)&replica->conf, conf, sizeof(replica->conf));
  world_hashtable_init(&replica->hashtable, world_generate_seed(), &replica->allocator);
  world_replica_thread_init(&replica->thread, replica);

  *r = replica;
  return world_error_ok;
}

enum world_error world_replica_close(struct world_replica *replica)
{
  world_replica_thread_destroy(&replica->thread);
  world_hashtable_destroy(&replica->hashtable);

  struct world_allocator allocator;
  memcpy(&allocator, &replica->allocator, sizeof(allocator));
  world_allocator_free(&allocator, replica);
  world_allocator_destroy(&allocator);

  return world_error_ok;
}

enum world_error world_replica_get(const struct world_replica *replica, struct world_buffer key, struct world_buffer *data)
{
  return world_hashtable_get((struct world_hashtable *)&replica->hashtable, key, data);
}

static bool _validate_conf(const struct world_replicaconf *conf)
{
  if (!world_check_fd(conf->fd)) {
    fprintf(stderr, "world_replica_open: fd: invalid value");
    return false;
  }

  return true;
}
