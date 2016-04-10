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

#pragma once

/**
 * @mainpage
 *
 * - world.h    Core API
 * - worldaux.h Auxiliary functions
 */

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
};

enum world_log_level {
  world_log_error = 0,
  world_log_info  = 1,
  world_log_debug = 2,
};

/**
 * @brief A structure represents information on a buffer.
 */
struct world_buffer {
  const void *base;
  size_t size;
};

/**
 * @brief A structure represents a configuration for world_origin_open().
 *
 * @see world_originconf_init()
 */
struct world_originconf {
  /**
   * @brief A number of I/O threads.
   *
   * The value should be positive integer.
   *
   * The default value is 1.
   */
  size_t n_io_threads;

  /**
   * @brief Whether an origin automatically sets an attached fd `O_NONBLOCK`.
   *
   * The default value is true.
  */
  bool set_nonblocking;

  /**
   * @brief Whether an origin automatically sets an attached fd `TCP_NODELAY`.
   *
   * The default value is true.
   */
  bool set_tcp_nodelay;

  /**
   * @brief Whether an origin implicitly transmits logs whenever a log is
   * generated.
   *
   * If the value is set to false, user should call world_origin_transmit()
   * after one or a batch of writing. This may improve performance under some
   * circumstances.
   *
   * The default value is true.
   *
   * @see world_origin_transmit()
   */
  bool auto_transmission;

  /**
   * @brief Reserved.
   */
  void (*logger)(enum world_log_level level, const char *message);
};

/**
 * @brief Initializes a world_originconf with default values.
 *
 * @param conf A pointer to world_originconf.
 */
static inline void
world_originconf_init(struct world_originconf *conf)
{
  conf->n_io_threads = 1;
  conf->set_nonblocking = true;
  conf->set_tcp_nodelay = true;
  conf->auto_transmission = true;
  conf->logger = NULL; // TODO not yet implemented
}

/**
 * @brief A structure represents a configuration for world_replica_open().
 *
 * @see world_replicaconf_init()
 */
struct world_replicaconf {
  /**
   * @brief A file descriptor already connected to an origin.
   *
   * The value should be specified by user.
   */
  int fd;

  /**
   * @brief A callback function invoked whenever a new log is arrived.
   */
  void (*callback)(struct world_buffer key, struct world_buffer data);

  /**
   * @brief Reserved.
   */
  void (*logger)(enum world_log_level level, const char *message);
};

/**
 * @brief Initializes a world_replicaconf with default values.
 *
 * @param conf A pointer to world_replicaconf.
 */
static inline void
world_replicaconf_init(struct world_replicaconf *conf)
{
  conf->fd = -1;
  conf->callback = NULL;
  conf->logger = NULL; // TODO not yet implemented
}

/**
 * @brief An opaque structure represents an origin (often referred as *master*
 * or *publisher*).
 *
 * An origin has a single dataset (associative array) that can be read and be
 * written by user.
 *
 * An origin is associated with zero or multiple replicas. Associations can be
 * manipulated by world_origin_attach() and world_origin_detach().
 *
 * Written data are asynchronously sent to associated replicas. This is done by
 * internal I/O threads.
 *
 * All the corresponding functions are thread-safe.
 *
 * @see world_originconf
 * @see world_origin_open(), world_origin_close()
 * @see world_origin_attach(), world_origin_detach()
 * @see world_origin_get()
 * @see world_origin_set(), world_origin_add(), world_origin_replace(),
 * world_origin_delete()
 */
struct world_origin
#if defined(DOXYGEN)
{}
#endif
;

/**
 * @brief Opens an origin with a specified configuration.
 *
 * Modifying a world_originconf object pointed by `conf` after the call returns
 * does not affect behavior of `origin`.
 *
 * @param origin A pointer to a world_origin handle.
 * @param conf A pointer to a world_originconf object.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @see world_origin_close()
 */
enum world_error
world_origin_open(struct world_origin **origin,
                  const struct world_originconf *conf);

/**
 * @brief Closes an origin.
 *
 * @param origin A world_origin handle.
 * @return world_error_ok
 * @see world_origin_open()
 */
enum world_error
world_origin_close(struct world_origin *origin);

/**
 * @brief Attaches a socket to an origin.
 *
 * @param origin A world_origin handle.
 * @param fd A file descriptor connected to a replica.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_system
 * @see world_origin_detach()
 */
enum world_error
world_origin_attach(struct world_origin *origin, int fd);

/**
 * @brief Detaches a socket from an origin.
 *
 * @param origin A world_origin handle.
 * @param fd A file descriptor connected to a replica.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @see world_origin_attach()
 */
enum world_error
world_origin_detach(struct world_origin *origin, int fd);

/**
 * @brief Transmits logs explicitly.
 *
 * A call indicates to the internal I/O threads that the dataset may have been
 * written so that they notice written data and transmit them.
 *
 * If `auto_transmission` is set to true, a call performs no operation.
 *
 * @param origin A world_origin handle.
 * @return world_error_ok
 * @see world_originconf.auto_transmission
 */
enum world_error
world_origin_transmit(struct world_origin *origin);

/**
 * @brief Gets a data with a given key.
 *
 * The size of `key` should not be zero.
 * If `found` is non-NULL and if the data associated with `key` exists, it is
 * returned.
 *
 * @param origin A world_origin handle.
 * @param key A key.
 * @param found A data.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_no_such_key
 */
enum world_error
world_origin_get(const struct world_origin *origin,
                 struct world_buffer key, struct world_buffer *found);

/**
 * @brief Sets a given key to a given data.
 *
 * The size of both `key` and `data` should not be zero.
 *
 * @param origin A world_origin handle.
 * @param key A key.
 * @param data A data.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @see world_origin_add(), world_origin_replace()
 */
enum world_error
world_origin_set(struct world_origin *origin,
                 struct world_buffer key, struct world_buffer data);

/**
 * @brief Adds a given data.
 *
 * The size of both `key` and `data` should not be zero.
 * If `key` exists in the dataset, the call returns an error of
 * `world_error_key_exists`.
 *
 * @param origin A world_origin handle.
 * @param key A key.
 * @param data A data.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_key_exists
 * @see world_origin_set(), world_origin_replace()
 */
enum world_error
world_origin_add(struct world_origin *origin,
                 struct world_buffer key, struct world_buffer data);

/**
 * @brief Replaces an existing data with a given one.
 *
 * The size of both `key` and `data` should not be zero.
 * If `key` does not exist in the dataset, the call returns an error of
 * `world_error_no_such_key`.
 *
 * @param origin A world_origin handle.
 * @param key A key.
 * @param data A data.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_no_such_key
 * @see world_origin_set(), world_origin_add()
 */
enum world_error
world_origin_replace(struct world_origin *origin,
                     struct world_buffer key, struct world_buffer data);

/**
 * @brief Deletes a given key.
 *
 * The size of `key` should not be zero.
 * If `key` does not exist in the dataset, the call returns an error of
 * `world_error_no_such_key`.
 *
 * @param origin A world_origin handle.
 * @param key A key.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_no_such_key
 */
enum world_error
world_origin_delete(struct world_origin *origin,
                    struct world_buffer key);

/**
 * @brief An opaque structure represents a replica (often referred as *slave*
 * or *subscriber*).
 *
 * A replica has a single dataset (associative array) that can be read but
 * cannot be written by user.
 *
 * A replica is associated with one origin.
 *
 * The corresponding dataset is asynchronously updated in a consistent way. This
 * is done by an internal I/O thread.
 *
 * All the corresponding functions are thread-safe.
 *
 * @see world_replicaconf
 * @see world_replica_open(), world_replica_close()
 * @see world_replica_get()
 */
struct world_replica
#if defined(DOXYGEN)
{}
#endif
;

/**
 * @brief Opens a replica with a specified configuration.
 *
 * Modifying a world_replicaconf object pointed by `conf` after the call returns
 * does not affect behavior of `replica`.
 *
 * @param replica A pointer to a world_replica handle.
 * @param conf A pointer to a world_replicaconf object.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @see world_replica_close()
 */
enum world_error
world_replica_open(struct world_replica **replica,
                   const struct world_replicaconf *conf);

/**
 * @brief Closes a replica.
 *
 * @param replica A world_replica handle.
 * @return world_error_ok
 * @see world_replica_open()
 */
enum world_error
world_replica_close(struct world_replica *replica);

/**
 * @brief Gets a data with a given key.
 *
 * The size of `key` should not be zero.
 * If `found` is non-NULL and if the data associated with `key` exists, it is
 * returned.
 *
 * @param replica A world_replica handle.
 * @param key A key.
 * @param found A data.
 * @return world_error_ok
 * @return world_error_invalid_argument
 * @return world_error_no_such_key
 */
enum world_error
world_replica_get(const struct world_replica *replica,
                  struct world_buffer key, struct world_buffer *found);

#if defined(__cplusplus)
}
#endif
