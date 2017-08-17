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

#ifndef __drool_client_h
#define __drool_client_h

#include "query.h"
#include "conf.h"

#include <ev.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef enum drool_client_state drool_client_state_t;
enum drool_client_state {
    CLIENT_NEW = 0,

    CLIENT_CONNECTING,
    CLIENT_CONNECTED,
    CLIENT_SENDING,
    CLIENT_RECIVING,
    CLIENT_CLOSING,

    CLIENT_SUCCESS,
    CLIENT_FAILED,
    CLIENT_ERROR,
    CLIENT_ERRNO,

    CLIENT_ABORTED,
    CLIENT_CLOSED
};

typedef struct drool_client drool_client_t;
typedef void (*drool_client_callback_t)(drool_client_t* client, struct ev_loop* loop);
struct drool_client {
    unsigned short have_to_addr : 1;
    unsigned short have_from_addr : 1;
    unsigned short have_fd : 1;
    unsigned short is_connected : 1;
    unsigned short skip_reply : 1;
    unsigned short is_dgram : 1;
    unsigned short is_stream : 1;

    ev_tstamp       start;
    drool_client_t* next;
    drool_client_t* prev;

    int                     fd;
    drool_query_t*          query;
    ev_io                   write_watcher;
    ev_io                   read_watcher;
    ev_io                   shutdown_watcher;
    drool_client_callback_t callback;
    drool_client_state_t    state;
    int                     errnum;
    size_t                  sent;
    size_t                  recv;

    struct sockaddr to_addr;
    socklen_t       to_addrlen;
    struct sockaddr from_addr;
    socklen_t       from_addrlen;
};

drool_client_t* client_new(drool_query_t* query, drool_client_callback_t callback);
void client_free(drool_client_t* client);

drool_client_t* client_next(drool_client_t* client);
drool_client_t* client_prev(drool_client_t* client);
int client_fd(const drool_client_t* client);
const drool_query_t* client_query(const drool_client_t* client);
drool_client_state_t client_state(const drool_client_t* client);
int client_is_connected(const drool_client_t* client);
int client_errno(const drool_client_t* client);
ev_tstamp client_start(const drool_client_t* client);
int client_is_dgram(const drool_client_t* client);
int client_is_stream(const drool_client_t* client);
int client_set_next(drool_client_t* client, drool_client_t* next);
int client_set_prev(drool_client_t* client, drool_client_t* prev);
int client_set_start(drool_client_t* client, ev_tstamp start);
int client_set_skip_reply(drool_client_t* client);
drool_query_t* client_release_query(drool_client_t* client);

int client_connect(drool_client_t* client, int ipproto, const struct sockaddr* addr, socklen_t addlen, struct ev_loop* loop);
int client_send(drool_client_t* client, struct ev_loop* loop);
int client_reuse(drool_client_t* client, drool_query_t* query);
int client_close(drool_client_t* client, struct ev_loop* loop);

#endif /* __drool_client_h */
