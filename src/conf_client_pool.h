/*
 * DNS Reply Tool (drool)
 *
 * Copyright (c) 2017-2018, OARC, Inc.
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

#ifndef __drool_conf_client_pool_h
#define __drool_conf_client_pool_h

#include "client_pool_sendas.h"

#include <stddef.h>

/* clang-format off */
#define CONF_CLIENT_POOL_T_INIT { \
    0, \
    0, 0, 0, 0, \
    0, 0, 0, 0, \
    CLIENT_POOL_SENDAS_ORIGINAL \
}
/* clang-format on */
typedef struct drool_conf_client_pool drool_conf_client_pool_t;
struct drool_conf_client_pool {
    drool_conf_client_pool_t* next;

    unsigned short have_target : 1;
    unsigned short have_max_clients : 1;
    unsigned short have_client_ttl : 1;
    unsigned short skip_reply : 1;
    unsigned short have_max_reuse_clients : 1;
    unsigned short have_sendas : 1;

    char*  target_host;
    char*  target_service;
    size_t max_clients;
    double client_ttl;
    size_t max_reuse_clients;

    drool_client_pool_sendas_t sendas;
};

drool_conf_client_pool_t* conf_client_pool_new(void);
void conf_client_pool_free(drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_target(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_max_clients(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_client_ttl(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_max_reuse_clients(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_sendas(const drool_conf_client_pool_t* conf_client_pool);
const drool_conf_client_pool_t* conf_client_pool_next(const drool_conf_client_pool_t* conf_client_pool);
const char* conf_client_pool_target_host(const drool_conf_client_pool_t* conf_client_pool);
const char* conf_client_pool_target_service(const drool_conf_client_pool_t* conf_client_pool);
size_t conf_client_pool_max_clients(const drool_conf_client_pool_t* conf_client_pool);
double conf_client_pool_client_ttl(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_skip_reply(const drool_conf_client_pool_t* conf_client_pool);
size_t conf_client_pool_max_reuse_clients(const drool_conf_client_pool_t* conf_client_pool);
drool_client_pool_sendas_t conf_client_pool_sendas(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_set_next(drool_conf_client_pool_t* conf_client_pool, drool_conf_client_pool_t* next);
int conf_client_pool_set_target(drool_conf_client_pool_t* conf_client_pool, const char* host, size_t host_length, const char* service, size_t service_length);
int conf_client_pool_set_max_clients(drool_conf_client_pool_t* conf_client_pool, size_t max_clients);
int conf_client_pool_set_client_ttl(drool_conf_client_pool_t* conf_client_pool, double client_ttl);
int conf_client_pool_set_skip_reply(drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_set_max_reuse_clients(drool_conf_client_pool_t* conf_client_pool, size_t max_reuse_clients);
int conf_client_pool_set_sendas(drool_conf_client_pool_t* conf_client_pool, drool_client_pool_sendas_t sendas);

#endif /* __drool_conf_client_pool_h */
