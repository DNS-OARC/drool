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

#include "callback.h"
#include "drool.h"
#include "log.h"
#include "query.h"
#include "omg-dns/omg_dns.h"
#include "client_pool.h"
#include "assert.h"

#define N1e9 1000000000

static void queue_dns(drool_t* context, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    omg_dns_t                   dns = OMG_DNS_T_INIT;
    int                         ret;
    drool_query_t*              query;
    const pcap_thread_packet_t* walkpkt;

    if ((ret = omg_dns_parse_header(&dns, payload, length))) {
        log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "omg-dns parse error %d", ret);
        context->packets_dropped++;
        return;
    }

    if (!omg_dns_have_qr(&dns) || omg_dns_qr(&dns)) {
        context->packets_ignored++;
        return;
    }

    /* TODO */
    query = query_new();
    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_udphdr) {
            query_set_udp(query);
            break;
        }
        if (walkpkt->have_tcphdr) {
            query_set_tcp(query);
            break;
        }
    }
    if (!walkpkt) {
        log_print(conf_log(context->conf), LNETWORK, LDEBUG, "packet is dns query but unknown protocol, ignoring");
        query_free(query);
        context->packets_dropped++;
        return;
    }

    if (query_set_raw(query, payload, length)) {
        log_print(conf_log(context->conf), LNETWORK, LDEBUG, "packet is dns query but unable to query_set_raw()");
        query_free(query);
        context->packets_dropped++;
        return;
    }
    if (client_pool_query(context->client_pool, query)) {
        log_print(conf_log(context->conf), LNETWORK, LERROR, "packet is dns query but unable to queue");
        query_free(query);
        context->packets_dropped++;
        return;
    }
    context->client_pool = context->client_pool->next ? context->client_pool->next : context->client_pools;

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "packet is dns query, queued %p", query);
    context->packets_sent++;
    context->packets_size += length;
}

void timing_init(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t*                    context = (drool_t*)user;
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (!walkpkt) {
        return;
    }

#if HAVE_CLOCK_NANOSLEEP
    if (clock_gettime(CLOCK_MONOTONIC, &context->last_ts)) {
        log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        return;
    }
    context->diff = context->last_ts;
    context->diff.tv_sec -= walkpkt->pkthdr.ts.tv_sec;
    context->diff.tv_nsec -= walkpkt->pkthdr.ts.tv_usec * 1000;
    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() with clock_nanosleep() now is %ld.%ld, diff of first pkt %ld.%ld",
        context->last_ts.tv_sec, context->last_ts.tv_nsec,
        context->diff.tv_sec, context->diff.tv_nsec);
#elif HAVE_NANOSLEEP
    log_print(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() with nanosleep()");
#else
#error "No clock_nanosleep() or nanosleep(), can not continue"
#endif

    context->last_pkthdr_ts = walkpkt->pkthdr.ts;

    switch (conf_timing_mode(context->conf)) {
    case TIMING_MODE_KEEP:
    case TIMING_MODE_BEST_EFFORT:
        log_print(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() mode keep");
        context->timing_callback = timing_keep;
        break;
    case TIMING_MODE_INCREASE:
        context->timing_callback = timing_increase;
        context->mod_ts.tv_sec   = conf_timing_increase(context->conf) / N1e9;
        context->mod_ts.tv_nsec  = conf_timing_increase(context->conf) % N1e9;
        log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() mode increase by %ld.%ld", context->mod_ts.tv_sec, context->mod_ts.tv_nsec);
        break;
    case TIMING_MODE_REDUCE:
        context->timing_callback = timing_reduce;
        context->mod_ts.tv_sec   = conf_timing_reduce(context->conf) / N1e9;
        context->mod_ts.tv_nsec  = conf_timing_reduce(context->conf) % N1e9;
        log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() mode reduce by %ld.%ld", context->mod_ts.tv_sec, context->mod_ts.tv_nsec);
        break;
    case TIMING_MODE_MULTIPLY:
        context->timing_callback = timing_multiply;
        log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_init() mode multiply by %f", conf_timing_multiply(context->conf));
        break;
    default:
        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "invalid timing mode %d", conf_timing_mode(context->conf));
        return;
    }

    queue_dns(context, packet, payload, length);
}

void timing_keep(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t*                    context = (drool_t*)user;
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (!walkpkt) {
        return;
    }

#if HAVE_CLOCK_NANOSLEEP
    {
        struct timespec to = {
            context->diff.tv_sec + walkpkt->pkthdr.ts.tv_sec,
            context->diff.tv_nsec + (walkpkt->pkthdr.ts.tv_usec * 1000)
        };
        int ret = EINTR;

        if (to.tv_nsec >= N1e9) {
            to.tv_sec += 1;
            to.tv_nsec -= N1e9;
        } else if (to.tv_nsec < 0) {
            to.tv_sec -= 1;
            to.tv_nsec += N1e9;
        }

        while (ret) {
            log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_keep() sleep to %ld.%ld", to.tv_sec, to.tv_nsec);
            ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &to, 0);
            if (ret && ret != EINTR) {
                log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "clock_nanosleep(%ld.%ld) %d", to.tv_sec, to.tv_nsec, ret);
                return;
            }
        }
    }
#elif HAVE_NANOSLEEP
    {
        struct timespec diff = {
            walkpkt->pkthdr.ts.tv_sec - context->last_pkthdr_ts.tv_sec,
            (walkpkt->pkthdr.ts.tv_usec - context->last_pkthdr_ts.tv_usec) * 1000
        };
        int ret = EINTR;

        if (diff.tv_nsec >= N1e9) {
            diff.tv_sec += 1;
            diff.tv_nsec -= N1e9;
        } else if (diff.tv_nsec < 0) {
            diff.tv_sec -= 1;
            diff.tv_nsec += N1e9;
        }

        if (diff.tv_sec > -1 && diff.tv_nsec > -1) {
            while (ret) {
                log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_keep() sleep for %ld.%ld", diff.tv_sec, diff.tv_nsec);
                if ((ret = nanosleep(&diff, &diff))) {
                    ret = errno;
                    if (ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "nanosleep(%ld.%ld) %d", diff.tv_sec, diff.tv_nsec, ret);
                        return;
                    }
                }
            }
        }

        context->last_pkthdr_ts = walkpkt->pkthdr.ts;
    }
#endif

    queue_dns(context, packet, payload, length);
}

void timing_increase(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t*                    context = (drool_t*)user;
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (!walkpkt) {
        return;
    }

    {
        struct timespec diff = {
            walkpkt->pkthdr.ts.tv_sec - context->last_pkthdr_ts.tv_sec,
            (walkpkt->pkthdr.ts.tv_usec - context->last_pkthdr_ts.tv_usec) * 1000
        };
        int ret = EINTR;

        if (diff.tv_nsec >= N1e9) {
            diff.tv_sec += 1;
            diff.tv_nsec -= N1e9;
        } else if (diff.tv_nsec < 0) {
            diff.tv_sec -= 1;
            diff.tv_nsec += N1e9;
        }

        diff.tv_sec += context->mod_ts.tv_sec;
        diff.tv_nsec += context->mod_ts.tv_nsec;
        if (diff.tv_nsec >= N1e9) {
            diff.tv_sec += 1;
            diff.tv_nsec -= N1e9;
        }

        if (diff.tv_sec > -1 && diff.tv_nsec > -1) {
#if HAVE_CLOCK_NANOSLEEP
            {
                struct timespec to = {
                    context->last_ts.tv_sec + diff.tv_sec,
                    context->last_ts.tv_nsec + diff.tv_nsec
                };

                if (to.tv_nsec >= N1e9) {
                    to.tv_sec += 1;
                    to.tv_nsec -= N1e9;
                } else if (to.tv_nsec < 0) {
                    to.tv_sec -= 1;
                    to.tv_nsec += N1e9;
                }

                while (ret) {
                    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_increase() sleep to %ld.%ld", to.tv_sec, to.tv_nsec);
                    ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &to, 0);
                    if (ret && ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "clock_nanosleep(%ld.%ld) %d", to.tv_sec, to.tv_nsec, ret);
                        return;
                    }
                }
            }
#elif HAVE_NANOSLEEP
            while (ret) {
                log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_increase() sleep for %ld.%ld", diff.tv_sec, diff.tv_nsec);
                if ((ret = nanosleep(&diff, &diff))) {
                    ret = errno;
                    if (ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "nanosleep(%ld.%ld) %d", diff.tv_sec, diff.tv_nsec, ret);
                        return;
                    }
                }
            }
#endif
        }

        context->last_pkthdr_ts = walkpkt->pkthdr.ts;
    }

#if HAVE_CLOCK_NANOSLEEP
    if (clock_gettime(CLOCK_MONOTONIC, &context->last_ts)) {
        log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        return;
    }
#endif

    queue_dns(context, packet, payload, length);
}

void timing_reduce(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t*                    context = (drool_t*)user;
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (!walkpkt) {
        return;
    }

    {
        struct timespec diff = {
            walkpkt->pkthdr.ts.tv_sec - context->last_pkthdr_ts.tv_sec,
            (walkpkt->pkthdr.ts.tv_usec - context->last_pkthdr_ts.tv_usec) * 1000
        };
        int ret = EINTR;

        if (diff.tv_nsec >= N1e9) {
            diff.tv_sec += 1;
            diff.tv_nsec -= N1e9;
        } else if (diff.tv_nsec < 0) {
            diff.tv_sec -= 1;
            diff.tv_nsec += N1e9;
        }

        diff.tv_sec -= context->mod_ts.tv_sec;
        diff.tv_nsec -= context->mod_ts.tv_nsec;
        if (diff.tv_nsec < 0) {
            diff.tv_sec -= 1;
            diff.tv_nsec += N1e9;
        }

        if (diff.tv_sec > -1 && diff.tv_nsec > -1) {
#if HAVE_CLOCK_NANOSLEEP
            {
                struct timespec to = {
                    context->last_ts.tv_sec + diff.tv_sec,
                    context->last_ts.tv_nsec + diff.tv_nsec
                };

                if (to.tv_nsec >= N1e9) {
                    to.tv_sec += 1;
                    to.tv_nsec -= N1e9;
                } else if (to.tv_nsec < 0) {
                    to.tv_sec -= 1;
                    to.tv_nsec += N1e9;
                }

                while (ret) {
                    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_reduce() sleep to %ld.%ld", to.tv_sec, to.tv_nsec);
                    ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &to, 0);
                    if (ret && ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "clock_nanosleep(%ld.%ld) %d", to.tv_sec, to.tv_nsec, ret);
                        return;
                    }
                }
            }
#elif HAVE_NANOSLEEP
            while (ret) {
                log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_reduce() sleep for %ld.%ld", diff.tv_sec, diff.tv_nsec);
                if ((ret = nanosleep(&diff, &diff))) {
                    ret = errno;
                    if (ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "nanosleep(%ld.%ld) %d", diff.tv_sec, diff.tv_nsec, ret);
                        return;
                    }
                }
            }
#endif
        }

        context->last_pkthdr_ts = walkpkt->pkthdr.ts;
    }

#if HAVE_CLOCK_NANOSLEEP
    if (clock_gettime(CLOCK_MONOTONIC, &context->last_ts)) {
        log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        return;
    }
#endif

    queue_dns(context, packet, payload, length);
}

void timing_multiply(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t*                    context = (drool_t*)user;
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (!walkpkt) {
        return;
    }

    {
        struct timespec diff = {
            walkpkt->pkthdr.ts.tv_sec - context->last_pkthdr_ts.tv_sec,
            (walkpkt->pkthdr.ts.tv_usec - context->last_pkthdr_ts.tv_usec) * 1000
        };
        int ret = EINTR;

        if (diff.tv_nsec >= N1e9) {
            diff.tv_sec += 1;
            diff.tv_nsec -= N1e9;
        } else if (diff.tv_nsec < 0) {
            diff.tv_sec -= 1;
            diff.tv_nsec += N1e9;
        }

        diff.tv_sec  = (time_t)((float)diff.tv_sec * conf_timing_multiply(context->conf));
        diff.tv_nsec = (long)((float)diff.tv_nsec * conf_timing_multiply(context->conf));

        if (diff.tv_sec > -1 && diff.tv_nsec > -1) {
            if (diff.tv_nsec >= N1e9) {
                diff.tv_sec += diff.tv_nsec / N1e9;
                diff.tv_nsec %= N1e9;
            }

#if HAVE_CLOCK_NANOSLEEP
            {
                struct timespec to = {
                    context->last_ts.tv_sec + diff.tv_sec,
                    context->last_ts.tv_nsec + diff.tv_nsec
                };

                if (to.tv_nsec >= N1e9) {
                    to.tv_sec += 1;
                    to.tv_nsec -= N1e9;
                } else if (to.tv_nsec < 0) {
                    to.tv_sec -= 1;
                    to.tv_nsec += N1e9;
                }

                while (ret) {
                    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_multiply() sleep to %ld.%ld", to.tv_sec, to.tv_nsec);
                    ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &to, 0);
                    if (ret && ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "clock_nanosleep(%ld.%ld) %d", to.tv_sec, to.tv_nsec, ret);
                        return;
                    }
                }
            }
#elif HAVE_NANOSLEEP
            while (ret) {
                log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "timing_multiply() sleep for %ld.%ld", diff.tv_sec, diff.tv_nsec);
                if ((ret = nanosleep(&diff, &diff))) {
                    ret = errno;
                    if (ret != EINTR) {
                        log_errnof(conf_log(context->conf), LNETWORK, LDEBUG, "nanosleep(%ld.%ld) %d", diff.tv_sec, diff.tv_nsec, ret);
                        return;
                    }
                }
            }
#endif
        }

        context->last_pkthdr_ts = walkpkt->pkthdr.ts;
    }

#if HAVE_CLOCK_NANOSLEEP
    if (clock_gettime(CLOCK_MONOTONIC, &context->last_ts)) {
        log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        return;
    }
#endif

    queue_dns(context, packet, payload, length);
}

void callback_udp(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t* context = (drool_t*)user;

    drool_assert(context);
    drool_assert(packet);
    drool_assert(payload);

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "udp packet received from %s", packet->name);

    context->packets_seen++;

    if (conf_timing_mode(context->conf) == TIMING_MODE_IGNORE) {
        queue_dns(context, packet, payload, length);
        return;
    }
    context->timing_callback(user, packet, payload, length);
}

void callback_tcp(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    drool_t* context = (drool_t*)user;

    drool_assert(context);
    drool_assert(packet);
    drool_assert(payload);

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "tcp packet received from %s", packet->name);

    context->packets_seen++;

    if (length < 3) {
        context->packets_dropped++;
        return;
    }
    payload += 2;
    length -= 2;

    if (conf_timing_mode(context->conf) == TIMING_MODE_IGNORE) {
        queue_dns(context, packet, payload, length);
        return;
    }
    context->timing_callback(user, packet, payload, length);
}
