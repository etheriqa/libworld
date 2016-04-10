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

#if defined(__cplusplus)
extern "C" {
#endif

#include <world.h>

/**
 * @brief An opaque structure represents a server.
 *
 * Unlike world_origin, the library handles preparing a TCP socket, accepting
 * arrival connections, and the like.
 *
 * @see worldaux_server_open(), worldaux_server_close(),
 * worldaux_server_get_origin()
 */
struct worldaux_server
#if defined(DOXYGEN)
{}
#endif
;

/**
 * @brief Returns a world_origin handle.
 *
 * Note that calling world_origin_close() with the origin that this function
 * returns may incur undefined behavior. Always use worldaux_server_close() to
 * close the server.
 *
 * @param server A worldaux_server handle.
 * @return A world_origin handle.
 */
struct world_origin *
worldaux_server_get_origin(struct worldaux_server *server);

/**
 * @brief Opens a worldaux_server.
 *
 * `host` and `port` must be specified.
 *
 * If `conf` is non-NULL, the values of the specified configuration except for
 * `fd` are used. Otherwise, the default values are used.
 *
 * @param server A pointer to a worldaux_server handle.
 * @param host Either a host name or a host address in terms of IPv4 or IPv6.
 * @param port A decimal port number.
 * @param conf A world_originconf object.
 * @see worldaux_server_close()
 */
enum world_error
worldaux_server_open(struct worldaux_server **server,
                     const char *host, const char *port,
                     const struct world_originconf *conf);

/**
 * @brief Closes a worldaux_server.
 *
 * @param server A worldaux_server handle.
 * @see worldaux_server_open()
 */
enum world_error
worldaux_server_close(struct worldaux_server *server);

/**
 * @brief An opaque structure represents a client.
 *
 * Unlike world_replica, the library handles preparing a TCP socket, connecting
 * to an origin, and the like.
 *
 * @see worldaux_client_open(), worldaux_client_close(),
 * worldaux_client_get_replica()
 */
struct worldaux_client
#if defined(DOXYGEN)
{}
#endif
;

/**
 * @brief Returns a world_replica handle.
 *
 * Note that calling world_replica_close() with the replica that this function
 * returns may incur undefined behavior. Always use worldaux_client_close() to
 * close the client.
 *
 * @param client A worldaux_client handle.
 * @return A world_replica handle.
 */
struct world_replica *
worldaux_client_get_replica(struct worldaux_client *client);

/**
 * @brief Opens a worldaux_client.
 *
 * `host` and `port` must be specified.
 *
 * If `conf` is non-NULL, the values of the specified configuration are used.
 * Otherwise, the default values are used.
 *
 * @param client A pointer to a worldaux_client handle.
 * @param host Either a host name or a host address in terms of IPv4 or IPv6.
 * @param port A decimal port number.
 * @param conf A world_replicaconf object.
 * @see worldaux_client_close()
 */
enum world_error
worldaux_client_open(struct worldaux_client **client,
                     const char *host, const char *port,
                     const struct world_replicaconf *conf);

/**
 * @brief Closes a worldaux_client.
 *
 * @param client A worldaux_client handle.
 * @see worldaux_client_open()
 */
enum world_error
worldaux_client_close(struct worldaux_client *client);

#if defined(__cplusplus)
}
#endif
