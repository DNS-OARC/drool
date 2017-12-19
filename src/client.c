/*
 * DNS Reply Tool (drool)
 *
 * Copyright (c) 2017, OARC, Inc.
 * Copyright (c) 2017, Comcast Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "client.h"
#include "log.h"
#include "assert.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/*
 * EV callbacks
 */

static void client_shutdown(struct ev_loop* loop, ev_io* w, int revents)
{
    drool_client_t* client;
    char            buf[512];

    /* TODO: Check revents for EV_ERROR */

    drool_assert(loop);
    drool_assert(w);
    client = (drool_client_t*)(w->data);
    drool_assert(client);

    if (recv(client->fd, buf, sizeof(buf), 0) > 0)
        return;

    ev_io_stop(loop, w);
    client->state = CLIENT_CLOSED;
    client->callback(client, loop);
}

static void client_read(struct ev_loop* loop, ev_io* w, int revents)
{
    drool_client_t* client;
    ssize_t         nrecv;
    char            buf[64 * 1024];

    /* TODO: Check revents for EV_ERROR */

    drool_assert(loop);
    drool_assert(w);
    client = (drool_client_t*)(w->data);
    drool_assert(client);

    /* TODO: How much should we read? */

    /* TODO:
    if (client->have_from_addr)
        memset(&(client->from_addr), 0, sizeof(struct sockaddr_storage));
    client->from_addrlen = sizeof(struct sockaddr_storage);
    nrecv = recvfrom(client->fd, buf, sizeof(buf), 0, &(client->from_addr), &(client->from_addrlen));
    */
    nrecv = recvfrom(client->fd, buf, sizeof(buf), 0, 0, 0);
    if (nrecv < 0) {
        switch (errno) {
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            return;

        case ECONNREFUSED:
        case ENETUNREACH:
            client->state = CLIENT_FAILED;
            break;

        default:
            client->errnum = errno;
            client->state  = CLIENT_ERRNO;
            break;
        }
        ev_io_stop(loop, w);
        client->callback(client, loop);
        return;
    }
    /* TODO:
    else if (nrecv > 0) {
    }
*/

    ev_io_stop(loop, w);
    client->state = CLIENT_SUCCESS;
    client->callback(client, loop);
}

static void client_write(struct ev_loop* loop, ev_io* w, int revents)
{
    drool_client_t* client;
    ssize_t         nsent;

    /* TODO: Check revents for EV_ERROR */

    drool_assert(loop);
    drool_assert(w);
    client = (drool_client_t*)(w->data);
    drool_assert(client);

    if (client->state == CLIENT_CONNECTING) {
        int       err = 0;
        socklen_t len = sizeof(err);

        ev_io_stop(loop, w);

        if (getsockopt(client->fd, SOL_SOCKET, SO_ERROR, (void*)&err, &len) < 0) {
            client->errnum = errno;
            client->state  = CLIENT_ERRNO;
        } else if (err) {
            switch (err) {
            case ECONNREFUSED:
            case ENETUNREACH:
                client->state = CLIENT_FAILED;
                break;

            default:
                client->errnum = err;
                client->state  = CLIENT_ERRNO;
                break;
            }
        } else {
            client->state        = CLIENT_CONNECTED;
            client->is_connected = 1;
        }

        client->callback(client, loop);
        return;
    }

    if (client->is_stream && !client->sent_length) {
        uint16_t length = htons(query_length(client->query));

        if (client->have_to_addr)
            nsent = sendto(client->fd, &length, 2, 0, (struct sockaddr*)&(client->to_addr), client->to_addrlen);
        else
            nsent = sendto(client->fd, &length, 2, 0, 0, 0);
        if (nsent < 0) {
            switch (errno) {
            case EAGAIN:
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK:
#endif
                return;

            default:
                break;
            }

            ev_io_stop(loop, w);
            client->errnum = errno;
            client->state  = errno == ECONNRESET ? CLIENT_FAILED : CLIENT_ERRNO;
            client->callback(client, loop);
            return;
        } else if (nsent != 2) {
            ev_io_stop(loop, w);
            client->errnum = ENOBUFS;
            client->state  = CLIENT_FAILED;
            client->callback(client, loop);
            return;
        }

        client->sent_length = 1;
    }

    if (client->have_to_addr)
        nsent = sendto(client->fd, query_raw(client->query) + client->sent, query_length(client->query) - client->sent, 0, (struct sockaddr*)&(client->to_addr), client->to_addrlen);
    else
        nsent = sendto(client->fd, query_raw(client->query) + client->sent, query_length(client->query) - client->sent, 0, 0, 0);
    if (nsent < 0) {
        switch (errno) {
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            return;

        default:
            break;
        }

        ev_io_stop(loop, w);
        client->errnum = errno;
        client->state  = errno == ECONNRESET ? CLIENT_FAILED : CLIENT_ERRNO;
        client->callback(client, loop);
        return;
    }

    client->sent += nsent;
    if (client->sent < query_length(client->query))
        return;

    ev_io_stop(loop, w);
    if (client->skip_reply) {
        client->state = CLIENT_SUCCESS;
        client->callback(client, loop);
        return;
    }
    ev_io_start(loop, &(client->read_watcher));
    client->state = CLIENT_RECIVING;
}

/*
 * New/free functions
 */

drool_client_t* client_new(drool_query_t* query, drool_client_callback_t callback)
{
    drool_client_t* client;

    drool_assert(query);
    if (!query) {
        return 0;
    }
    drool_assert(callback);
    if (!callback) {
        return 0;
    }

    if (!query_have_raw(query)) {
        return 0;
    }

    if ((client = calloc(1, sizeof(drool_client_t)))) {
        client->query              = query;
        client->callback           = callback;
        client->write_watcher.data = (void*)client;
        ev_init(&(client->write_watcher), &client_write);
        client->read_watcher.data = (void*)client;
        ev_init(&(client->read_watcher), &client_read);
        client->shutdown_watcher.data = (void*)client;
        ev_init(&(client->shutdown_watcher), &client_shutdown);
    }

    return client;
}

void client_free(drool_client_t* client)
{
    if (client) {
        if (client->have_fd) {
            if (client->is_connected) {
                shutdown(client->fd, SHUT_RDWR);
            }
            close(client->fd);
        }
        if (client->query) {
            query_free(client->query);
        }
        free(client);
    }
}

/*
 * Get/set functions
 */

inline drool_client_t* client_next(drool_client_t* client)
{
    drool_assert(client);
    return client->next;
}

inline drool_client_t* client_prev(drool_client_t* client)
{
    drool_assert(client);
    return client->prev;
}

inline int client_fd(const drool_client_t* client)
{
    drool_assert(client);
    return client->fd;
}

inline const drool_query_t* client_query(const drool_client_t* client)
{
    drool_assert(client);
    return client->query;
}

inline drool_client_state_t client_state(const drool_client_t* client)
{
    drool_assert(client);
    return client->state;
}

inline int client_is_connected(const drool_client_t* client)
{
    drool_assert(client);
    return client->is_connected;
}

inline int client_errno(const drool_client_t* client)
{
    drool_assert(client);
    return client->errnum;
}

inline ev_tstamp client_start(const drool_client_t* client)
{
    drool_assert(client);
    return client->start;
}

inline int client_is_dgram(const drool_client_t* client)
{
    drool_assert(client);
    return client->is_dgram;
}

inline int client_is_stream(const drool_client_t* client)
{
    drool_assert(client);
    return client->is_stream;
}

int client_set_next(drool_client_t* client, drool_client_t* next)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }

    client->next = next;

    return 0;
}

int client_set_prev(drool_client_t* client, drool_client_t* prev)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }

    client->prev = prev;

    return 0;
}

int client_set_start(drool_client_t* client, ev_tstamp start)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }

    client->start = start;

    return 0;
}

int client_set_skip_reply(drool_client_t* client)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }

    client->skip_reply = 1;

    return 0;
}

drool_query_t* client_release_query(drool_client_t* client)
{
    drool_query_t* query;

    drool_assert(client);
    if (!client) {
        return 0;
    }

    query         = client->query;
    client->query = 0;

    return query;
}

/*
 * Control functions
 */

int client_connect(drool_client_t* client, int ipproto, const struct sockaddr* addr, socklen_t addrlen, struct ev_loop* loop)
{
    int socket_type, flags;

    drool_assert(client);
    if (!client) {
        return 1;
    }
    drool_assert(addr);
    if (!addr) {
        return 1;
    }
    drool_assert(addrlen);
    if (!addrlen) {
        return 1;
    }
    if (addrlen > sizeof(struct sockaddr_storage)) {
        return 1;
    }
    drool_assert(loop);
    if (!loop) {
        return 1;
    }
    if (client->state != CLIENT_NEW) {
        return 1;
    }

    switch (ipproto) {
    case IPPROTO_UDP:
        socket_type = SOCK_DGRAM;
        memcpy(&(client->to_addr), addr, addrlen);
        client->to_addrlen   = addrlen;
        client->have_to_addr = 1;
        client->is_dgram     = 1;
        break;

    case IPPROTO_TCP:
        socket_type       = SOCK_STREAM;
        client->is_stream = 1;
        break;

    default:
        return 1;
    }

    if ((client->fd = socket(addr->sa_family, socket_type, ipproto)) < 0) {
        client->errnum = errno;
        client->state  = CLIENT_ERRNO;
        return 1;
    }
    client->have_fd = 1;

    if ((flags = fcntl(client->fd, F_GETFL)) == -1
        || fcntl(client->fd, F_SETFL, flags | O_NONBLOCK)) {
        client->errnum = errno;
        client->state  = CLIENT_ERRNO;
        return 1;
    }

    ev_io_set(&(client->write_watcher), client->fd, EV_WRITE);
    ev_io_set(&(client->read_watcher), client->fd, EV_READ);
    ev_io_set(&(client->shutdown_watcher), client->fd, EV_READ);

    if (socket_type == SOCK_STREAM && connect(client->fd, addr, addrlen) < 0) {
        switch (errno) {
        case EINPROGRESS:
            ev_io_start(loop, &(client->write_watcher));
            client->state = CLIENT_CONNECTING;
            return 0;

        case ECONNREFUSED:
        case ENETUNREACH:
            client->state = CLIENT_FAILED;
            break;

        default:
            client->errnum = errno;
            client->state  = CLIENT_ERRNO;
            break;
        }
        return 1;
    }

    client->state        = CLIENT_CONNECTED;
    client->is_connected = 1;
    return 0;
}

int client_send(drool_client_t* client, struct ev_loop* loop)
{
    ssize_t nsent;

    drool_assert(client);
    if (!client) {
        return 1;
    }
    drool_assert(loop);
    if (!loop) {
        return 1;
    }
    if (client->state != CLIENT_CONNECTED) {
        return 1;
    }

    if (client->is_stream && !client->sent_length) {
        uint16_t length = htons(query_length(client->query));

        if (client->have_to_addr)
            nsent = sendto(client->fd, &length, 2, 0, (struct sockaddr*)&(client->to_addr), client->to_addrlen);
        else
            nsent = sendto(client->fd, &length, 2, 0, 0, 0);
        if (nsent < 0) {
            switch (errno) {
            case EAGAIN:
#if EAGAIN != EWOULDBLOCK
            case EWOULDBLOCK:
#endif
                ev_io_start(loop, &(client->write_watcher));
                client->state = CLIENT_SENDING;
                return 0;

            default:
                break;
            }

            client->errnum = errno;
            client->state  = errno == ECONNRESET ? CLIENT_FAILED : CLIENT_ERRNO;
            return 1;
        } else if (nsent != 2) {
            client->errnum = ENOBUFS;
            client->state  = CLIENT_FAILED;
            return 1;
        }

        client->sent_length = 1;
    }

    if (client->have_to_addr)
        nsent = sendto(client->fd, query_raw(client->query), query_length(client->query), 0, (struct sockaddr*)&(client->to_addr), client->to_addrlen);
    else
        nsent = sendto(client->fd, query_raw(client->query), query_length(client->query), 0, 0, 0);
    if (nsent < 0) {
        switch (errno) {
        case EAGAIN:
#if EAGAIN != EWOULDBLOCK
        case EWOULDBLOCK:
#endif
            ev_io_start(loop, &(client->write_watcher));
            client->state = CLIENT_SENDING;
            return 0;

        default:
            break;
        }

        client->errnum = errno;
        client->state  = errno == ECONNRESET ? CLIENT_FAILED : CLIENT_ERRNO;
        return 1;
    }

    if (nsent < query_length(client->query)) {
        client->sent = nsent;
        ev_io_start(loop, &(client->write_watcher));
        client->state = CLIENT_SENDING;
        return 0;
    }

    if (client->skip_reply) {
        client->state = CLIENT_SUCCESS;
        return 0;
    }

    ev_io_start(loop, &(client->read_watcher));
    client->state = CLIENT_RECIVING;
    return 0;
}

int client_reuse(drool_client_t* client, drool_query_t* query)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (client->state != CLIENT_SUCCESS) {
        return 1;
    }

    if (client->query)
        query_free(client->query);
    client->query       = query;
    client->sent        = 0;
    client->recv        = 0;
    client->state       = CLIENT_CONNECTED;
    client->sent_length = 0;

    return 0;
}

int client_close(drool_client_t* client, struct ev_loop* loop)
{
    drool_assert(client);
    if (!client) {
        return 1;
    }
    drool_assert(loop);
    if (!loop) {
        return 1;
    }

    switch (client->state) {
    case CLIENT_CONNECTING:
    case CLIENT_SENDING:
    case CLIENT_RECIVING:
        ev_io_stop(loop, &(client->write_watcher));
        ev_io_stop(loop, &(client->read_watcher));
        break;

    case CLIENT_CLOSING:
        return 0;

    default:
        break;
    }

    if (client->have_fd) {
        if (client->is_connected) {
            client->is_connected = 0;
            if (!shutdown(client->fd, SHUT_RDWR)) {
                ev_io_start(loop, &(client->shutdown_watcher));
                client->state = CLIENT_CLOSING;
                return 0;
            }
        }
        close(client->fd);
        client->have_fd = 0;
    }
    client->state = CLIENT_CLOSED;

    return 0;
}
