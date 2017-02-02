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

#include "sllq/sllq.h"
#include "conf.h"

#ifndef __drool_socket_pool_h
#define __drool_socket_pool_h

#include <pthread.h>

typedef enum drool_socket_pool_state drool_socket_pool_state_t;
enum drool_socket_pool_state {
    SOCKET_POOL_INACTIVE = 0,
    SOCKET_POOL_RUNNING,
    SOCKET_POOL_STOPPED,
    SOCKET_POOL_ERROR
};

typedef struct drool_socket_pool drool_socket_pool_t;
struct drool_socket_pool {
    const drool_conf_t*         conf;
    drool_socket_pool_state_t   state;
    pthread_t                   thread_id;

    sllq_t                      queue;

    int                         family;
    int                         type;
    int                         proto;
    int                         nonblocking;
};

drool_socket_pool_t* socket_pool_new(const drool_conf_t* conf, int family, int type, int proto, int nonblocking);
void socket_pool_free(drool_socket_pool_t* socket_pool);

int socket_pool_family(const drool_socket_pool_t* socket_pool);
int socket_pool_type(const drool_socket_pool_t* socket_pool);
int socket_pool_proto(const drool_socket_pool_t* socket_pool);
int socket_pool_nonblocking(const drool_socket_pool_t* socket_pool);

int socket_pool_start(drool_socket_pool_t* socket_pool);
int socket_pool_stop(drool_socket_pool_t* socket_pool);
int socket_pool_getsock(drool_socket_pool_t* socket_pool, int* fd);

#endif /* __drool_socket_pool_h */
