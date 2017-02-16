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

#include "client_pool.h"
#include "drool.h"
#include "log.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

/*
 * New/free
 */

static sllq_t client_pool_sllq_init = SLLQ_T_INIT;

drool_client_pool_t* client_pool_new(const drool_conf_t* conf) {
    drool_client_pool_t* client_pool;

    drool_assert(conf);
    if (!conf) {
        return 0;
    }

    if (!conf_client_pool_have_target(conf_client_pool(conf))) {
        return 0;
    }

    if ((client_pool = calloc(1, sizeof(drool_client_pool_t)))) {
        struct addrinfo hints;
        int err;

        client_pool->conf = conf;
        client_pool->max_clients = 100;
        client_pool->client_ttl = 0.05;
#ifdef DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT
        client_pool->max_reuse_clients = 0; /* TODO */
#endif

        if (conf_client_pool_have_max_clients(conf_client_pool(conf)))
            client_pool->max_clients = conf_client_pool_max_clients(conf_client_pool(conf));
        if (conf_client_pool_have_client_ttl(conf_client_pool(conf)))
            client_pool->max_clients = conf_client_pool_client_ttl(conf_client_pool(conf));

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        if ((err = getaddrinfo(conf_client_pool_target_host(conf_client_pool(conf)), conf_client_pool_target_service(conf_client_pool(conf)), &hints, &(client_pool->addrinfo)))) {
            log_printf(conf_log(conf), LNETWORK, LERROR, "getaddrinfo: %s", gai_strerror(err));
            free(client_pool);
            return 0;
        }
        if (!client_pool->addrinfo) {
            log_print(conf_log(conf), LNETWORK, LERROR, "getaddrinfo failed");
            free(client_pool);
            return 0;
        }

        memcpy(&(client_pool->queries), &client_pool_sllq_init, sizeof(sllq_t));
        sllq_set_size(&(client_pool->queries), 0x200);
        if (sllq_init(&(client_pool->queries)) != SLLQ_OK) {
            free(client_pool);
            return 0;
        }

        if (!(client_pool->ev_loop = ev_loop_new(EVFLAG_NOSIGMASK))) {
            sllq_destroy(&(client_pool->queries));
            free(client_pool);
            return 0;
        }

        ev_set_userdata(client_pool->ev_loop, (void*)client_pool);

#ifdef DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
        if ((client_pool->sockpool_udpv4 = socket_pool_new(conf, AF_INET, SOCK_DGRAM, 0, 1))) {
            socket_pool_start(client_pool->sockpool_udpv4);
        }
#endif
    }

    return client_pool;
}

static void client_pool_free_query(void* vp) {
    if (vp) {
        query_free((drool_query_t*)vp);
    }
}

void client_pool_free(drool_client_pool_t* client_pool) {
    if (client_pool) {
        sllq_flush(&(client_pool->queries), &client_pool_free_query);
        sllq_destroy(&(client_pool->queries));
        if (client_pool->ev_loop)
            ev_loop_destroy(client_pool->ev_loop);
        if (client_pool->addrinfo)
            freeaddrinfo(client_pool->addrinfo);
#ifdef DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT
        while (client_pool->reuse_client_list) {
            drool_client_t* client = client_pool->reuse_client_list;
            client_pool->reuse_client_list = client_next(client);
            client_close(client);
            client_free(client);
        }
#endif
#if DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
        if (client_pool->sockpool_udpv4) {
            socket_pool_stop(client_pool->sockpool_udpv4);
            socket_pool_free(client_pool->sockpool_udpv4);
        }
        free(client_pool);
#endif
    }
}

/*
 * Engine
 */

static void* client_pool_engine(void* vp) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)vp;

    drool_assert(client_pool);
    if (client_pool) {
        drool_assert(client_pool->ev_loop);
        log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool ev run");
        ev_run(client_pool->ev_loop, 0);
        log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool ev run exited");
    }

    return 0;
}

static void client_pool_client_callback(drool_client_t* client, struct ev_loop* loop) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)ev_userdata(loop);

    drool_assert(client);
    if (!client) {
        return;
    }
    drool_assert(client_pool);
    if (!client_pool) {
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    switch (client_state(client)) {
        case CLIENT_CONNECTED:
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client connected");
            if (client_send(client, loop)) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client failed to send");
                break;
            }
            return;

        case CLIENT_SUCCESS:
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client success");
            break;

        case CLIENT_FAILED:
            /* TODO */
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client failed");
            break;

        case CLIENT_ERRNO:
            errno = client_errno(client);
            log_errno(conf_log(client_pool->conf), LNETWORK, LERROR, "client state errno");
            break;

        default:
            log_printf(conf_log(client_pool->conf), LNETWORK, LERROR, "client state %d", client_state(client));
            break;
    }

    if (client == client_pool->client_list) {
        client_pool->client_list = client_next(client);
        client_set_prev(client_pool->client_list, 0);
    }
    else {
        if (client_next(client)) {
            client_set_prev(client_next(client), client_prev(client));
        }
        if (client_prev(client)) {
            client_set_next(client_prev(client), client_next(client));
        }
    }
#ifdef DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT
    if (client_state(client) == CLIENT_SUCCESS && client_pool->reuse_clients < client_pool->max_reuse_clients) {
        client_set_next(client, client_pool->reuse_client_list);
        client_set_prev(client, 0);
        client_pool->reuse_client_list = client;
        client_pool->reuse_clients++;
    }
    else
#endif
    {
        client_close(client);
        client_free(client);
    }
    if (client_pool->clients) /* TODO: Error? */
        client_pool->clients--;
    if (!client_pool->client_list && ev_is_active(&(client_pool->timeout))) {
        ev_timer_stop(loop, &(client_pool->timeout));
    }

    ev_async_send(loop, &(client_pool->notify_query));
}

static void client_pool_engine_timeout(struct ev_loop* loop, ev_async* w, int revents) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)ev_userdata(loop);
    ev_tstamp timeout;
    drool_client_t* client;

    /* TODO: Check revents for EV_ERROR */

    drool_assert(client_pool);
    if (!client_pool) {
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    timeout = ev_now(loop) - client_pool->client_ttl;

    while (client_pool->client_list && client_start(client_pool->client_list) <= timeout) {
        client = client_pool->client_list;

        log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client timeout");

        client_abort(client, loop);
        client_close(client);
        client_pool->client_list = client_next(client);
        client_set_prev(client_pool->client_list, 0);
        client_free(client);
        if (client_pool->clients)
            client_pool->clients--;
    }

    ev_async_send(loop, &(client_pool->notify_query));

    if (client_pool->client_list) {
        ev_timer_set(&(client_pool->timeout), client_start(client_pool->client_list) - timeout, 0.);
        ev_timer_start(loop, &(client_pool->timeout));
    }
}

static void client_pool_engine_query(struct ev_loop* loop, ev_async* w, int revents) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)ev_userdata(loop);
    drool_query_t* query;
    int err;

    /* TODO: Check revents for EV_ERROR */

    drool_assert(client_pool);
    if (!client_pool) {
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    if (client_pool->clients >= client_pool->max_clients) {
        return;
    }

    err = sllq_shift(&(client_pool->queries), (void**)&query, 0);

    if (err == SLLQ_EMPTY) {
        if (client_pool->is_stopping && !client_pool->clients) {
            ev_async_stop(loop, &(client_pool->notify_query));
        }
        return;
    }
    else if (err == SLLQ_EAGAIN) {
        ev_async_send(loop, &(client_pool->notify_query));
        return;
    }
    else if (err) {
        if (err == SLLQ_ERRNO) {
            log_errnof(conf_log(client_pool->conf), LNETWORK, LERROR, "shift queue error %d: ", err);
        }
        else {
            log_printf(conf_log(client_pool->conf), LNETWORK, LERROR, "shift queue error %d", err);
        }
        return;
    }

    if (!query) {
        log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "shift queue null?");
        ev_async_send(loop, &(client_pool->notify_query));
        return;
    }

    log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "shift queue, query %p", query);

    {
        drool_client_t* client;
        int proto = -1;

#ifdef DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT
        if (client_pool->reuse_client_list) {
            /* TODO: udp or tcp? */
            client = client_pool->reuse_client_list;
            client_pool->reuse_client_list = client_next(client);
            if (client_pool->reuse_clients)
                client_pool->reuse_clients--;
            client_set_next(client, 0);
            client_set_prev(client, 0);

            if (client_reuse(client, query)) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "reuse client failed");
                client_close(client);
                client_free(client);
            }
            else {
                if (client_send(client, loop)) {
                    log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client send failed");
                    client_close(client);
                    client_free(client);
                }
                else {
                    client_pool->clients++;

                    if (client_pool->client_list)
                        client_set_prev(client_pool->client_list, client);
                    client_set_next(client, client_pool->client_list);
                    client_pool->client_list = client;

                    if (!ev_is_active(&(client_pool->timeout))) {
                        ev_timer_set(&(client_pool->timeout), client_pool->client_ttl, 0.);
                        ev_timer_start(loop, &(client_pool->timeout));
                    }

                    log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "reuse client (%lu/%lu)", client_pool->clients, client_pool->max_clients);
                }
            }
        }
        else
#endif
        if ((client = client_new(query, &client_pool_client_callback))
            && !client_set_start(client, ev_now(loop)))
        {
#ifdef DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
            int fd;
#endif

            if (query_is_udp(query)) {
                proto = IPPROTO_UDP;
            }
            else if (query_is_tcp(query)) {
                proto = IPPROTO_TCP;
            }

            /* TODO: Multiple addrinfo entries? */

#ifdef DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
            fd = -1;
            if (client_pool->addrinfo->ai_family == AF_INET && proto == IPPROTO_UDP && client_pool->sockpool_udpv4) {
                socket_pool_getsock(client_pool->sockpool_udpv4, &fd);
            }

            if (fd > -1 && client_connect_fd(client, fd, client_pool->addrinfo->ai_addr, client_pool->addrinfo->ai_addrlen, loop)) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "new client fd failed");
                client_free(client);
            }
            else if (fd < 0 &&
#else
            if (
#endif
                client_connect(client, proto, client_pool->addrinfo->ai_addr, client_pool->addrinfo->ai_addrlen, loop))
            {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "new client failed");
                client_free(client);
            }
            else {
                if (client_state(client) == CLIENT_CONNECTED && client_send(client, loop)) {
                    log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client send failed");
                    client_close(client);
                    client_free(client);
                }
                else {
                    client_pool->clients++;

                    if (client_pool->client_list)
                        client_set_prev(client_pool->client_list, client);
                    client_set_next(client, client_pool->client_list);
                    client_pool->client_list = client;

                    if (!ev_is_active(&(client_pool->timeout))) {
                        ev_timer_set(&(client_pool->timeout), client_pool->client_ttl, 0.);
                        ev_timer_start(loop, &(client_pool->timeout));
                    }

                    log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "new client (%lu/%lu)", client_pool->clients, client_pool->max_clients);
                }
            }
        }
        else {
            log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "unable to create client, query lost");
            if (client)
                client_free(client);
        }
    }

    ev_async_send(loop, &(client_pool->notify_query));
}

static void client_pool_engine_stop(struct ev_loop* loop, ev_async* w, int revents) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)ev_userdata(loop);

    /* TODO: Check revents for EV_ERROR */

    drool_assert(client_pool);
    if (!client_pool) {
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    client_pool->is_stopping = 1;
    ev_async_stop(loop, &(client_pool->notify_stop));
    if (!client_pool->client_list && ev_is_active(&(client_pool->timeout))) {
        ev_timer_stop(loop, &(client_pool->timeout));
    }
    ev_async_send(loop, &(client_pool->notify_query));
}

/*
 * Start/stop
 */

int client_pool_start(drool_client_pool_t* client_pool) {
    int err;

    drool_assert(client_pool);
    if (!client_pool) {
        return 1;
    }
    if (client_pool->state != CLIENT_POOL_INACTIVE) {
        return 1;
    }
    drool_assert(client_pool->ev_loop);

    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool ev init");

    ev_async_init(&(client_pool->notify_query), &client_pool_engine_query);
    ev_async_init(&(client_pool->notify_stop), &client_pool_engine_stop);
    ev_timer_init(&(client_pool->timeout), &client_pool_engine_timeout, 0., 0.);

    ev_async_start(client_pool->ev_loop, &(client_pool->notify_query));
    ev_async_start(client_pool->ev_loop, &(client_pool->notify_stop));

    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool starting");

    if ((err = pthread_create(&(client_pool->thread_id), 0, &client_pool_engine, (void*)client_pool))) {
        client_pool->state = CLIENT_POOL_ERROR;
        errno = err;
        return 1;
    }
    client_pool->state = CLIENT_POOL_RUNNING;

    return 0;
}

int client_pool_stop(drool_client_pool_t* client_pool) {
    int err;

    drool_assert(client_pool);
    if (!client_pool) {
        return 1;
    }
    if (client_pool->state != CLIENT_POOL_RUNNING) {
        return 1;
    }
    drool_assert(client_pool->ev_loop);

    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool stopping");

    ev_async_send(client_pool->ev_loop, &(client_pool->notify_stop));

    if ((err = pthread_join(client_pool->thread_id, 0))) {
        client_pool->state = CLIENT_POOL_ERROR;
        errno = err;
        return 1;
    }

    client_pool->state = CLIENT_POOL_STOPPED;

    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool stopped");

    return 0;
}

/*
 * Query/process
 */

int client_pool_query(drool_client_pool_t* client_pool, drool_query_t* query) {
    int err;

    drool_assert(client_pool);
    if (!client_pool) {
        return 1;
    }
    if (client_pool->state != CLIENT_POOL_RUNNING) {
        return 1;
    }
    drool_assert(client_pool->ev_loop);

    err = SLLQ_EAGAIN;
    while (err == SLLQ_EAGAIN || err == SLLQ_ETIMEDOUT || err == SLLQ_FULL) {
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout)) {
            return 1;
        }
        timeout.tv_sec++;

        err = sllq_push(&(client_pool->queries), (void*)query, &timeout);
    }

    if (err) {
        if (err == SLLQ_ERRNO) {
            log_errnof(conf_log(client_pool->conf), LNETWORK, LERROR, "client pool query failed %d: ", err);
        }
        else {
            log_printf(conf_log(client_pool->conf), LNETWORK, LERROR, "client pool query failed %d", err);
        }
        return 1;
    }

    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client pool query ok, signaling");

    ev_async_send(client_pool->ev_loop, &(client_pool->notify_query));

    return 0;
}
