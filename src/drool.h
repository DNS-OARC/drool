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

#include "conf.h"
#include "client_pool.h"

#ifndef __drool_drool_h
#define __drool_drool_h

#include <stdint.h>
#include <time.h>

#if DROOL_ENABLE_ASSERT
#undef NDEBUG
#include <assert.h>
#define drool_assert(x) assert(x)
#else
#define drool_assert(x)
#endif

#define DROOL_ERROR     1
#define DROOL_EOPT      2
#define DROOL_ECONF     3
#define DROOL_ESIGNAL   4
#define DROOL_ESIGRCV   5
#define DROOL_EPCAPT    6
#define DROOL_ENOMEM    7

#define DROOL_T_INIT { \
    0, \
    0, 0, 0, 0, 0, \
    { 0, 0 }, { 0, 0 }, { 0, 0 }, \
    0 \
}
typedef struct drool drool_t;
struct drool {
    drool_t*                next;

    const drool_conf_t*     conf;
    uint64_t                packets_seen;
    uint64_t                packets_sent;
    uint64_t                packets_dropped;
    uint64_t                packets_ignored;

    struct timeval          last_packet;
    struct timespec         last_time;
    struct timespec         last_realtime;
    struct timespec         last_time_queue;

    drool_client_pool_t*    client_pool;
};

#endif /* __drool_drool_h */
