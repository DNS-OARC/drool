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

#include "config.h"

#include "conf_client_pool.h"
#include "conf.h"
#include "assert.h"

#include <stdlib.h>
#include <string.h>

drool_conf_client_pool_t* conf_client_pool_new(void)
{
    drool_conf_client_pool_t* conf_client_pool = calloc(1, sizeof(drool_conf_client_pool_t));
    return conf_client_pool;
}

void conf_client_pool_free(drool_conf_client_pool_t* conf_client_pool)
{
    if (conf_client_pool) {
        if (conf_client_pool->target_host)
            free(conf_client_pool->target_host);
        if (conf_client_pool->target_service)
            free(conf_client_pool->target_service);
        free(conf_client_pool);
    }
}

inline int conf_client_pool_have_target(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->have_target;
}

inline int conf_client_pool_have_max_clients(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->have_max_clients;
}

inline int conf_client_pool_have_client_ttl(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->have_client_ttl;
}

inline int conf_client_pool_have_max_reuse_clients(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->have_max_reuse_clients;
}

inline int conf_client_pool_have_sendas(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->have_sendas;
}

inline const drool_conf_client_pool_t* conf_client_pool_next(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->next;
}

inline const char* conf_client_pool_target_host(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->target_host;
}

inline const char* conf_client_pool_target_service(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->target_service;
}

inline size_t conf_client_pool_max_clients(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->max_clients;
}

inline double conf_client_pool_client_ttl(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->client_ttl;
}

inline int conf_client_pool_skip_reply(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->skip_reply;
}

inline size_t conf_client_pool_max_reuse_clients(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->max_reuse_clients;
}

inline drool_client_pool_sendas_t conf_client_pool_sendas(const drool_conf_client_pool_t* conf_client_pool)
{
    drool_assert(conf_client_pool);
    return conf_client_pool->sendas;
}

int conf_client_pool_set_next(drool_conf_client_pool_t* conf_client_pool, drool_conf_client_pool_t* next)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }
    if (!next) {
        return CONF_EINVAL;
    }

    conf_client_pool->next = next;

    return CONF_OK;
}

int conf_client_pool_set_target(drool_conf_client_pool_t* conf_client_pool, const char* host, size_t host_length, const char* service, size_t service_length)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }
    if (!host) {
        return CONF_EINVAL;
    }
    if (!host_length) {
        return CONF_EINVAL;
    }
    if (!service) {
        return CONF_EINVAL;
    }
    if (!service_length) {
        return CONF_EINVAL;
    }

    if (conf_client_pool->target_host)
        free(conf_client_pool->target_host);
    if (conf_client_pool->target_service)
        free(conf_client_pool->target_service);
    conf_client_pool->have_target = 0;
    if (!(conf_client_pool->target_host = strndup(host, host_length))) {
        return CONF_ENOMEM;
    }
    if (!(conf_client_pool->target_service = strndup(service, service_length))) {
        return CONF_ENOMEM;
    }
    conf_client_pool->have_target = 1;

    return CONF_OK;
}

int conf_client_pool_set_max_clients(drool_conf_client_pool_t* conf_client_pool, size_t max_clients)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }

    conf_client_pool->max_clients      = max_clients;
    conf_client_pool->have_max_clients = 1;

    return CONF_OK;
}

int conf_client_pool_set_client_ttl(drool_conf_client_pool_t* conf_client_pool, double client_ttl)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }

    conf_client_pool->client_ttl      = client_ttl;
    conf_client_pool->have_client_ttl = 1;

    return CONF_OK;
}

int conf_client_pool_set_skip_reply(drool_conf_client_pool_t* conf_client_pool)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }

    conf_client_pool->skip_reply = 1;

    return CONF_OK;
}

int conf_client_pool_set_max_reuse_clients(drool_conf_client_pool_t* conf_client_pool, size_t max_reuse_clients)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }

    conf_client_pool->max_reuse_clients      = max_reuse_clients;
    conf_client_pool->have_max_reuse_clients = 1;

    return CONF_OK;
}

int conf_client_pool_set_sendas(drool_conf_client_pool_t* conf_client_pool, drool_client_pool_sendas_t sendas)
{
    if (!conf_client_pool) {
        return CONF_EINVAL;
    }

    conf_client_pool->sendas      = sendas;
    conf_client_pool->have_sendas = 1;

    return CONF_OK;
}
