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

#ifndef __drool_query_h
#define __drool_query_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct drool_query drool_query_t;
struct drool_query {
    unsigned short  is_udp : 1;
    unsigned short  is_tcp : 1;
    unsigned short  have_ipv4 : 1;
    unsigned short  have_ipv6 : 1;
    unsigned short  have_port : 1;
    unsigned short  have_raw : 1;

    union {
        struct in_addr  ip_dst;
        struct in6_addr ip6_dst;
    } addr;
    uint16_t    port;

    u_char      small[64];
    u_char*     raw;
    size_t      length;
};

drool_query_t* query_new(void);
void query_free(drool_query_t* query);

int query_is_udp(const drool_query_t* query);
int query_is_tcp(const drool_query_t* query);
int query_have_ipv4(const drool_query_t* query);
int query_have_ipv6(const drool_query_t* query);
int query_have_port(const drool_query_t* query);
int query_have_raw(const drool_query_t* query);

const struct in_addr* query_ip(const drool_query_t* query);
const struct in6_addr* query_ip6(const drool_query_t* query);
uint16_t query_port(const drool_query_t* query);
size_t query_length(const drool_query_t* query);
const u_char* query_raw(const drool_query_t* query);

int query_set_udp(drool_query_t* query);
int query_set_tcp(drool_query_t* query);
int query_set_ip(drool_query_t* query, const struct in_addr* addr);
int query_set_ip6(drool_query_t* query, const struct in6_addr* addr);
int query_set_port(drool_query_t* query, uint16_t port);
int query_set_raw(drool_query_t* query, const u_char* raw, size_t length);

#endif /* __drool_query_h */
