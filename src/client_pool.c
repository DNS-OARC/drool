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
#include "log.h"
#include "assert.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

/*
 * List helpers
 */
static inline void client_list_add(drool_client_pool_t* client_pool, drool_client_t* client, struct ev_loop* loop) {
    if (client_pool->client_list_last) {
        client_set_next(client_pool->client_list_last, client);
        client_set_prev(client, client_pool->client_list_last);
    }
    client_pool->client_list_last = client;
    if (!client_pool->client_list_first)
        client_pool->client_list_first = client;
    client_pool->clients++;

    if (!ev_is_active(&(client_pool->timeout))) {
        ev_tstamp timeout = ev_now(loop) - client_start(client_pool->client_list_first);

        if (timeout < 0.)
            timeout = 0.;
        else if (timeout > client_pool->client_ttl)
            timeout = client_pool->client_ttl;

        ev_timer_set(&(client_pool->timeout), timeout, 0.);
        ev_timer_start(loop, &(client_pool->timeout));
    }
}

static inline void client_list_remove(drool_client_pool_t* client_pool, drool_client_t* client, struct ev_loop* loop) {
    if (client == client_pool->client_list_first) {
        client_pool->client_list_first = client_next(client);
    }
    if (client == client_pool->client_list_last) {
        client_pool->client_list_last = client_prev(client);
    }
    if (client_next(client)) {
        client_set_prev(client_next(client), client_prev(client));
    }
    if (client_prev(client)) {
        client_set_next(client_prev(client), client_next(client));
    }
    client_set_next(client, 0);
    client_set_prev(client, 0);

    if (client_pool->clients)
        client_pool->clients--;
    else
        log_print(conf_log(client_pool->conf), LNETWORK, LCRITICAL, "removed client but clients already zero");

    if (client_pool->clients && ev_is_active(&(client_pool->timeout))) {
        ev_timer_stop(loop, &(client_pool->timeout));
    }
}

static inline void client_close_free(drool_client_pool_t* client_pool, drool_client_t* client, struct ev_loop* loop) {
    if (client_state(client) == CLIENT_CLOSED) {
        client_free(client);
        return;
    }

    if (client_close(client, loop)) {
        log_print(conf_log(client_pool->conf), LNETWORK, LCRITICAL, "client close failed");
        client_free(client);
        return;
    }

    if (client_state(client) == CLIENT_CLOSING) {
        client_list_add(client_pool, client, loop);
    }
    else {
        client_free(client);
    }
}

static inline void client_reuse_add(drool_client_pool_t* client_pool, drool_client_t* client) {
    client_set_next(client, client_pool->reuse_client_list);
    client_set_prev(client, 0);
    client_pool->reuse_client_list = client;
    client_pool->reuse_clients++;
}

static inline drool_client_t* client_reuse_get(drool_client_pool_t* client_pool) {
    drool_client_t* client = client_pool->reuse_client_list;

    if (client) {
        client_pool->reuse_client_list = client_next(client);
        if (client_pool->reuse_clients)
            client_pool->reuse_clients--;
        else
            log_print(conf_log(client_pool->conf), LNETWORK, LCRITICAL, "remove reuse client but reuse_clients already zero");
        client_set_next(client, 0);
        client_set_prev(client, 0);
    }

    return client;
}

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
        client_pool->max_reuse_clients = 20;
        client_pool->sendas = CLIENT_POOL_SENDAS_ORIGINAL;

        if (conf_client_pool_have_max_clients(conf_client_pool(conf)))
            client_pool->max_clients = conf_client_pool_max_clients(conf_client_pool(conf));
        if (conf_client_pool_have_client_ttl(conf_client_pool(conf)))
            client_pool->client_ttl = conf_client_pool_client_ttl(conf_client_pool(conf));
        if (conf_client_pool_have_max_reuse_clients(conf_client_pool(conf)))
            client_pool->max_reuse_clients = conf_client_pool_max_reuse_clients(conf_client_pool(conf));
        if (conf_client_pool_have_sendas(conf_client_pool(conf)))
            client_pool->sendas = conf_client_pool_sendas(conf_client_pool(conf));

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
        sllq_set_size(&(client_pool->queries), 0x200); /* TODO: conf */
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
        while (client_pool->reuse_client_list) {
            drool_client_t* client = client_pool->reuse_client_list;
            client_pool->reuse_client_list = client_next(client);
            client_free(client);
        }
        free(client_pool);
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

    if (client_state(client) == CLIENT_CONNECTED) {
        log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client connected");
        if (client_send(client, loop)) {
            log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client failed to send");
        }
        else if (client_state(client) == CLIENT_RECIVING) {
            return;
        }
    }

    client_list_remove(client_pool, client, loop);

    switch (client_state(client)) {
        case CLIENT_SUCCESS:
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client success");

            if (client_is_dgram(client)
                && client_pool->reuse_clients < client_pool->max_reuse_clients)
            {
                client_reuse_add(client_pool, client);
                log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client added to reuse (%lu/%lu)", client_pool->reuse_clients, client_pool->max_reuse_clients);
            }
            else {
                client_close_free(client_pool, client, loop);
            }
            break;

        case CLIENT_FAILED:
            /* TODO */
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client failed");
            client_close_free(client_pool, client, loop);
            break;

        case CLIENT_ERRNO:
            log_errnum(conf_log(client_pool->conf), LNETWORK, LERROR, client_errno(client), "client errno");
            client_close_free(client_pool, client, loop);
            break;

        case CLIENT_CLOSED:
            log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client closed");
            client_free(client);
            break;

        default:
            log_printf(conf_log(client_pool->conf), LNETWORK, LERROR, "client state %d", client_state(client));
            client_close_free(client_pool, client, loop);
            break;
    }

    ev_async_send(loop, &(client_pool->notify_query));
}

static void client_pool_engine_timeout(struct ev_loop* loop, ev_timer* w, int revents) {
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

    while ((client = client_pool->client_list_first) && client_start(client_pool->client_list_first) <= timeout) {
        client_list_remove(client_pool, client, loop);
        if (client_state(client) == CLIENT_CLOSING) {
            client_list_add(client_pool, client, loop);
            continue;
        }

        log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client timeout");
        client_close_free(client_pool, client, loop);
    }

    ev_async_send(loop, &(client_pool->notify_query));
}

static void client_pool_engine_retry(struct ev_loop* loop, ev_timer* w, int revents) {
    drool_client_pool_t* client_pool = (drool_client_pool_t*)ev_userdata(loop);

    /* TODO: Check revents for EV_ERROR */

    drool_assert(client_pool);
    if (!client_pool) {
        ev_break(loop, EVBREAK_ALL);
        return;
    }

    if (client_pool->query)
        ev_async_send(loop, &(client_pool->notify_query));
    else
        ev_timer_stop(loop, w);
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

    /* TODO:
     *   store one query within the pool, keep retrying to create a client
     *   if fail, start a timer for retry and retry on each query call but
     *   do not add async
     */
    if (client_pool->query) {
        query = client_pool->query;
        client_pool->query = 0;
    }
    else {
        err = sllq_shift(&(client_pool->queries), (void**)&query, 0);

        if (err == SLLQ_EMPTY) {
            if (client_pool->is_stopping && !client_pool->clients) {
                ev_async_stop(loop, &(client_pool->notify_query));
                ev_timer_stop(loop, &(client_pool->timeout));
                ev_timer_stop(loop, &(client_pool->retry));
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
    }

    if (!query) {
        log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "shift queue null?");
        ev_async_send(loop, &(client_pool->notify_query));
        return;
    }

    log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "shift queue, query %p", query);

    {
        drool_client_t* client = 0;
        int proto = -1;

        switch (client_pool->sendas) {
            case CLIENT_POOL_SENDAS_UDP:
                proto = IPPROTO_UDP;
                break;

            case CLIENT_POOL_SENDAS_TCP:
                proto = IPPROTO_TCP;
                break;

            default:
                if (query_is_udp(query)) {
                    proto = IPPROTO_UDP;
                }
                else if (query_is_tcp(query)) {
                    proto = IPPROTO_TCP;
                }
                break;
        }

        if (proto == IPPROTO_UDP
            && client_pool->reuse_client_list)
        {
            client = client_reuse_get(client_pool);
            if (client_reuse(client, query)) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "reuse client failed");
                client_close_free(client_pool, client, loop);
                client = 0;
            }
            else {
                /* client have taken ownership of query */
                query = 0;
            }

            if (client && client_set_start(client, ev_now(loop))) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "reuse client start failed");
                query = client_release_query(client);
                client_close_free(client_pool, client, loop);
                client = 0;
            }
        }

        if (!client && (client = client_new(query, &client_pool_client_callback))) {
            /* client have taken ownership of query */
            query = 0;

            /* TODO: Multiple addrinfo entries? */

            if (client_set_start(client, ev_now(loop))
                || (conf_client_pool_skip_reply(conf_client_pool(client_pool->conf)) && client_set_skip_reply(client))
                || client_connect(client, proto, client_pool->addrinfo->ai_addr, client_pool->addrinfo->ai_addrlen, loop))
            {
                if (client_state(client) == CLIENT_ERRNO)
                    log_errnum(conf_log(client_pool->conf), LNETWORK, LERROR, client_errno(client), "client start/connect failed");
                else
                    log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client start/connect failed");
                query = client_release_query(client);
                client_close_free(client_pool, client, loop);
                client = 0;
            }
        }

        if (client) {
            if (client_state(client) == CLIENT_CONNECTED && client_send(client, loop)) {
                log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "client send failed");
                client_close_free(client_pool, client, loop);
            }
            else {
                if (client_state(client) == CLIENT_SUCCESS) {
                    log_print(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client success");

                    if (client_is_dgram(client)
                        && client_pool->reuse_clients < client_pool->max_reuse_clients)
                    {
                        client_reuse_add(client_pool, client);
                        log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "client added to reuse (%lu/%lu)", client_pool->reuse_clients, client_pool->max_reuse_clients);
                    }
                    else {
                        client_close_free(client_pool, client, loop);
                    }
                }
                else {
                    client_list_add(client_pool, client, loop);
                    log_printf(conf_log(client_pool->conf), LNETWORK, LDEBUG, "new client (%lu/%lu)", client_pool->clients, client_pool->max_clients);
                }
            }
        }
        else if (query) {
            log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "unable to create client, query requeued");
            client_pool->query = query;
            if (!ev_is_active(&(client_pool->retry))) {
                ev_timer_start(loop, &(client_pool->retry));
            }
            return;
        }
        else {
            log_print(conf_log(client_pool->conf), LNETWORK, LERROR, "unable to create client, query lost");
        }
    }

    /* TODO: Can we optimize this? Not call it every time? */
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
    ev_timer_init(&(client_pool->retry), &client_pool_engine_retry, 1., 1.);

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
            log_errno(conf_log(client_pool->conf), LNETWORK, LERROR, "client pool query failed: clock_gettime()");
            return 1;
        }
        timeout.tv_nsec += 200000000;
        if (timeout.tv_nsec > 999999999) {
            timeout.tv_sec += timeout.tv_nsec / 1000000000;
            timeout.tv_nsec %= 1000000000;
        }

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
