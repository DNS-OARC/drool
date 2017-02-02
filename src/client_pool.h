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
//#define DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT 1

#include "sllq/sllq.h"
#include "query.h"
#include "conf.h"
#include "client.h"
#ifdef DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
#include "socket_pool.h"
#endif

#ifndef __drool_client_pool_h
#define __drool_client_pool_h

#include <pthread.h>
#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef enum drool_client_pool_state drool_client_pool_state_t;
enum drool_client_pool_state {
    CLIENT_POOL_INACTIVE = 0,
    CLIENT_POOL_RUNNING,
    CLIENT_POOL_STOPPED,
    CLIENT_POOL_ERROR
};

typedef struct drool_client_pool drool_client_pool_t;
struct drool_client_pool {
    unsigned short  have_queued_queries : 1;
    unsigned short  is_stopping : 1;

    const drool_conf_t*         conf;
    drool_client_pool_state_t   state;
    pthread_t                   thread_id;

    struct ev_loop*             ev_loop;
    sllq_t                      queries;
    ev_async                    notify_query;
    ev_async                    notify_stop;
    ev_timer                    timeout;

    drool_client_t*             client_list;
    size_t                      clients;
    size_t                      max_clients;
    ev_tstamp                   client_ttl;

    struct addrinfo*            addrinfo;

#ifdef DROOL_CLIENT_POOL_ENABLE_REUSE_CLIENT
    drool_client_t*             reuse_client_list;
    size_t                      reuse_clients;
    size_t                      max_reuse_clients;
#endif
#ifdef DROOL_CLIENT_POOL_ENABLE_SOCKET_POOL
    drool_socket_pool_t*        sockpool_udpv4;
#endif
};

drool_client_pool_t* client_pool_new(const drool_conf_t* conf);
void client_pool_free(drool_client_pool_t* client_pool);
int client_pool_start(drool_client_pool_t* client_pool);
int client_pool_stop(drool_client_pool_t* client_pool);
int client_pool_query(drool_client_pool_t* client_pool, drool_query_t* query);

#endif /* __drool_client_pool_h */