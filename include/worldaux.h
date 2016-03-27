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

#include <world.h>

struct worldaux_server;

/**
 * @param server A worldaux_server handle.
 * @return A world_origin handle.
 */
struct world_origin *worldaux_server_get_origin(struct worldaux_server *server);

/**
 * @param server A pointer to a worldaux_server handle.
 * @param host Either a host name or a host address in terms of IPv4 or IPv6.
 * @parma port A decimal port number.
 * @return TODO Not yet implemented.
 * @see worldaux_server_close()
 */
enum world_error worldaux_server_open(struct worldaux_server **server, const char *host, const char *port);

/**
 * @param server A worldaux_server handle.
 * @return TODO Not yet implemented.
 * @see worldaux_server_open()
 */
enum world_error worldaux_server_close(struct worldaux_server *server);

struct worldaux_client;

/**
 * @param server A worldaux_client handle.
 * @return A world_replica handle.
 */
struct world_replica *worldaux_client_get_replica(struct worldaux_client *client);

/**
 * @param client A pointer to a worldaux_client handle.
 * @param host Either a host name or a host address in terms of IPv4 or IPv6.
 * @parma port A decimal port number.
 * @return TODO Not yet implemented.
 * @see worldaux_client_close()
 */
enum world_error worldaux_client_open(struct worldaux_client **client, const char *host, const char *port);

/**
 * @param client A worldaux_client handle.
 * @return TODO Not yet implemented.
 * @see worldaux_client_open()
 */
enum world_error worldaux_client_close(struct worldaux_client *client);

#if defined(__cplusplus)
}
#endif
