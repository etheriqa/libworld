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

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint64_t world_sequence;
typedef uint16_t world_key_size;
typedef uint16_t world_data_size;

enum world_error {
  world_error_ok               = 0,
  world_error_invalid_argument = 1,
  world_error_no_such_key      = 2,
  world_error_key_exists       = 3,
  world_error_system           = 4,
  world_error_internal         = 5,
  world_error_fatal            = 6,
};

/**
 * A structure represents a configuration for world_origin_open().
 *
 * @see world_originconf_init(), world_origin_open()
 */
struct world_originconf {
  size_t n_io_threads;
  bool ignore_sigpipe;
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
  conf->ignore_sigpipe = true;
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
 * A handle structure represents an origin ("master" or "publisher").
 */
struct world_origin;

/**
 * Opens an origin with a specified configuration.
 *
 * Modifying a world_originconf object passed to world_origin_open() after the
 * call returns does not affect a world_origin object.
 *
 * @param origin A pointer to a world_origin handle.
 * @param conf A pointer to a world_originconf object.
 * @return TODO Not yet implemented.
 * @see world_originconf_init(), world_origin_close()
 */
enum world_error world_origin_open(struct world_origin **origin, const struct world_originconf *conf);

/**
 * Closes an origin.
 *
 * @param origin A world_origin handle.
 * @return TODO Not yet implemented.
 * @see world_origin_open()
 */
enum world_error world_origin_close(struct world_origin *origin);

/**
 * Attaches a socket to an origin.
 *
 * @param origin A world_origin handle.
 * @param fd A file descriptor connected to a replica.
 * @return TODO Not yet implemented.
 * @see world_origin_detach()
 */
enum world_error world_origin_attach(struct world_origin *origin, int fd);

/**
 * Detaches a socket from an origin.
 *
 * @param origin A world_origin handle.
 * @param fd A file descriptor connected to a replica.
 * @return TODO Not yet implemented.
 * @see world_origin_attach()
 */
enum world_error world_origin_detach(struct world_origin *origin, int fd);

enum world_error world_origin_get(const struct world_origin *origin, struct world_iovec key, struct world_iovec *found);
enum world_error world_origin_set(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
enum world_error world_origin_add(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
enum world_error world_origin_replace(struct world_origin *origin, struct world_iovec key, struct world_iovec data);
enum world_error world_origin_delete(struct world_origin *origin, struct world_iovec key);

/**
 * A handle structure represents a replica ("slave" or "subscriber").
 */
struct world_replica;

/**
 * Opens a replica with a specified configuration.
 *
 * Modifying a world_replicaconf object passed to world_replica_open() after the
 * call returns does not affect a world_replica object.
 *
 * @param replica A pointer to a world_replica handle.
 * @param conf A pointer to a world_replicaconf object.
 * @return TODO Not yet implemented.
 * @see world_replicaconf_init(), world_replica_close()
 */
enum world_error world_replica_open(struct world_replica **replica, const struct world_replicaconf *conf);

/**
 * Closes a replica.
 *
 * @param replica A world_replica handle.
 * @return TODO Not yet implemented.
 * @see world_replica_open()
 */
enum world_error world_replica_close(struct world_replica *replica);

enum world_error world_replica_get(const struct world_replica *replica, struct world_iovec key, struct world_iovec *found);

#if defined(__cplusplus)
}
#endif
