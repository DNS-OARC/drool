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

#include "query.h"
#include "assert.h"

#include <string.h>
#include <stdlib.h>

/*
 * New/Free
 */

drool_query_t* query_new(void)
{
    return calloc(1, sizeof(drool_query_t));
}

void query_free(drool_query_t* query)
{
    drool_assert(query);
    if (query) {
        if (query->raw) {
            free(query->raw);
        }
        free(query);
    }
}

/*
 * Have functions
 */

inline int query_is_udp(const drool_query_t* query)
{
    drool_assert(query);
    return query->is_udp;
}

inline int query_is_tcp(const drool_query_t* query)
{
    drool_assert(query);
    return query->is_tcp;
}

inline int query_have_ipv4(const drool_query_t* query)
{
    drool_assert(query);
    return query->have_ipv4;
}

inline int query_have_ipv6(const drool_query_t* query)
{
    drool_assert(query);
    return query->have_ipv6;
}

inline int query_have_port(const drool_query_t* query)
{
    drool_assert(query);
    return query->have_port;
}

inline int query_have_raw(const drool_query_t* query)
{
    drool_assert(query);
    return query->have_raw;
}

/*
 * Get functions
 */

inline const struct in_addr* query_ip(const drool_query_t* query)
{
    drool_assert(query);

    if (!query->have_ipv4)
        return 0;

    return &(query->addr.ip_dst);
}

inline const struct in6_addr* query_ip6(const drool_query_t* query)
{
    drool_assert(query);

    if (!query->have_ipv6)
        return 0;

    return &(query->addr.ip6_dst);
}

inline uint16_t query_port(const drool_query_t* query)
{
    drool_assert(query);
    return query->port;
}

inline size_t query_length(const drool_query_t* query)
{
    drool_assert(query);
    return query->length;
}

inline const u_char* query_raw(const drool_query_t* query)
{
    drool_assert(query);
    return query->raw ? query->raw : query->small;
}

/*
 * Set functions
 */

int query_set_udp(drool_query_t* query)
{
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (query->is_tcp) {
        return 1;
    }

    query->is_udp = 1;

    return 0;
}

int query_set_tcp(drool_query_t* query)
{
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (query->is_udp) {
        return 1;
    }

    query->is_tcp = 1;

    return 0;
}

int query_set_ip(drool_query_t* query, const struct in_addr* addr)
{
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (!addr) {
        return 1;
    }
    if (query->have_ipv6) {
        return 1;
    }

    memcpy(&(query->addr.ip_dst), addr, sizeof(struct in_addr));
    query->have_ipv4 = 1;

    return 0;
}

int query_set_ip6(drool_query_t* query, const struct in6_addr* addr)
{
    drool_assert(query);
    if (!addr) {
        return 1;
    }
    if (query->have_ipv4) {
        return 1;
    }

    memcpy(&(query->addr.ip6_dst), addr, sizeof(struct in6_addr));
    query->have_ipv6 = 1;

    return 0;
}

int query_set_port(drool_query_t* query, uint16_t port)
{
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (!port) {
        return 1;
    }

    query->port = port;

    return 0;
}

int query_set_raw(drool_query_t* query, const u_char* raw, size_t length)
{
    drool_assert(query);
    if (!query) {
        return 1;
    }
    if (!raw) {
        return 1;
    }
    if (!length) {
        return 1;
    }

    if (query->raw) {
        free(query->raw);
        query->raw      = 0;
        query->length   = 0;
        query->have_raw = 0;
    }
    if (length > sizeof(query->small)) {
        if (!(query->raw = calloc(length, 1))) {
            return 1;
        }
        memcpy(query->raw, raw, length);
    } else {
        memcpy(query->small, raw, length);
    }
    query->length   = length;
    query->have_raw = 1;

    return 0;
}
