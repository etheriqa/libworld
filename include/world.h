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

/**
 * API Overview
 *
 * struct world_originconf;
 * struct world_originconf_init(struct world_originconf *);
 *
 * struct world_origin *world_origin_open(const struct world_originconf *);
 * void                 world_origin_close(struct world_origin *);
 * bool                 world_origin_attach(struct world_origin *, int);
 * bool                 world_origin_detach(struct world_origin *, int);
 * bool                 world_origin_get(const struct world_origin *,
 *                                       struct world_iovec,
 *                                       struct world_iovec *);
 * bool                 world_origin_set(struct world_origin *,
 *                                       struct world_iovec,
 *                                       struct world_iovec);
 * bool                 world_origin_add(struct world_origin *,
 *                                       struct world_iovec,
 *                                       struct world_iovec);
 * bool                 world_origin_replace(struct world_origin *,
 *                                           struct world_iovec,
 *                                           struct world_iovec);
 * bool                 world_origin_delete(struct world_origin *,
 *                                          struct world_iovec,
 *                                          struct world_iovec);
 *
 * struct world_replicaconf;
 * struct world_replicaconf_init(struct world_replicaconf *);
 *
 * struct world_replica *world_replica_open(const struct world_replicaconf *);
 * void                  world_replica_close(struct world_replica *);
 * bool                  world_replica_get(const struct world_replica *,
 *                                         struct world_iovec,
 *                                         struct world_iovec *);
 */

#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t world_sequence;
typedef uint16_t world_key_size;
typedef uint16_t world_data_size;

/**
 * A structure represents a configuration for world_origin_open().
 *
 * @see world_originconf_init(), world_origin_open()
 */
struct world_originconf {
  size_t n_io_threads;
  bool set_nonblocking;
  bool set_tcp_nodelay;
  bool close_on_destroy;
};

/**
 * Initializes a world_originconf with default values.
 *
 * @param conf A pointer to world_originconf.
 * @see world_origin_open()
 */
static inline void world_originconf_init(struct world_originconf *conf)
{
  conf->n_io_threads = 1;
  conf->set_nonblocking = true;
  conf->set_tcp_nodelay = true;
  conf->close_on_destroy = true; // TODO Not yet implemented.
}

/**
 * A structure represents a configuration for world_replica_open().
 *
 * @see world_replicaconf_init(), world_replica_open()
 */
struct world_replicaconf {
  int fd;
  bool close_on_destroy;
};

/**
 * Initializes a world_replicaconf with default values.
 *
 * @param conf A pointer to world_replicaconf.
 * @see world_replica_open()
 */
static inline void world_replicaconf_init(struct world_replicaconf *conf)
{
  conf->close_on_destroy = true; // TODO Not yet implemented.
}

struct world_iovec {
  const void *base;
  size_t size;
};

/**
 * A handle structure represents an origin ("master").
 */
struct world_origin;

/**
 * Opens an origin with a specified configuration.
 *
 * Modifying a world_originconf object passed to world_origin_open() after the
 * call returns does not affect a world_origin object.
 *
 * @param conf A pointer to a world_originconf object.
 * @return A pointer to a world_origin object.
 * @see world_originconf_init(), world_origin_close()
 */
struct world_origin *world_origin_open(const struct world_originconf *conf);

/**
 * Closes an origin.
 *
 * @see world_origin_open()
 */
void world_origin_close(struct world_origin *origin);

/**
 * Attaches a socket to an origin.
 *
 * @param origin A pointer to world_origin object.
 * @param fd A file descriptor connected to a replica.
 * @return TODO Not yet implemented.
 * @see world_origin_detach()
 */
bool world_origin_attach(struct world_origin *origin, int fd);

/**
 * Detaches a socket from an origin.
 *
 * @param origin A pointer to world_origin object.
 * @param fd A file descriptor connected to a replica.
 * @return TODO Not yet implemented.
 * @see world_origin_attach()
 */
bool world_origin_detach(struct world_origin *origin, int fd);

bool world_origin_get(const struct world_origin *origin, struct world_iovec key, struct world_iovec *found);
bool world_origin_set(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
bool world_origin_add(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
bool world_origin_replace(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
bool world_origin_delete(struct world_origin *origin, struct world_iovec key);

/**
 * A handle structure represents a replica ("slave").
 */
struct world_replica;

/**
 * Opens a replica with a specified configuration.
 *
 * Modifying a world_replicaconf object passed to world_replica_open() after the
 * call returns does not affect a world_replica object.
 *
 * @see world_replicaconf_init(), world_replica_close()
 */
struct world_replica *world_replica_open(const struct world_replicaconf *conf);

/**
 * Closes a replica.
 *
 * @see world_replica_open()
 */
void world_replica_close(struct world_replica *replica);

/**
 * @return TODO Not yet implemented.
 */
bool world_replica_get(const struct world_replica *replica, struct world_iovec key, struct world_iovec *found);

#if defined(__cplusplus)
}
#endif
