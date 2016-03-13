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

#include <stdio.h>
#include <stdlib.h>
#include "replica.h"

struct world_replica *world_replica_open(const struct world_replicaconf *conf)
{
  // TODO validate conf here
  struct world_replica *replica = malloc(sizeof(*replica));
  if (replica == NULL) {
    perror("malloc");
    abort();
  }

  memcpy((void *)&replica->conf, conf, sizeof(replica->conf));
  hashtable_init(&replica->hashtable);
  replica_iothread_init(&replica->thread, replica);

  return replica;
}

void world_replica_close(struct world_replica *replica)
{
  hashtable_destroy(&replica->hashtable);
  replica_iothread_destroy(&replica->thread);

  free(replica);
}

bool world_replica_get(const struct world_replica *replica, struct world_iovec key, struct world_iovec *data)
{
  return hashtable_get((struct hashtable *)&replica->hashtable, key, data, hashtable_get_sequence((struct hashtable *)&replica->hashtable));
}
