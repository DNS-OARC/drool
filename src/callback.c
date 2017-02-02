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

#include "callback.h"
#include "drool.h"
#include "log.h"
#include "query.h"
#include "omg-dns/omg_dns.h"
#include "client_pool.h"

static void queue_dns(drool_t* context, const pcap_thread_packet_t* packet, const u_char* payload, size_t length) {
    omg_dns_t dns = OMG_DNS_T_INIT;
    int ret;
    drool_query_t* query;
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

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "packet is dns query, queued %p", query);
    context->packets_sent++;
}

static void do_timing(drool_t* context, const pcap_thread_packet_t* packet, const u_char* payload, size_t length) {
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (walkpkt) {
       struct timespec now = { 0, 0 };

        // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "pkthdr.ts %lu.%06lu", walkpkt->pkthdr.ts.tv_sec, walkpkt->pkthdr.ts.tv_usec);
        // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "last_packet %lu.%06lu", context->last_packet.tv_sec, context->last_packet.tv_usec);

        if (clock_gettime(CLOCK_MONOTONIC, &(now))) {
            log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        }

        if ((context->last_time_queue.tv_sec || context->last_time_queue.tv_nsec)
            && context->last_packet.tv_sec
            && (context->last_time.tv_sec || context->last_time.tv_nsec)
            && timercmp(&(walkpkt->pkthdr.ts), &(context->last_packet), >))
        {
            struct timespec pdiff = { 0, 0 };
            struct timeval diff;
            struct timespec sleep_to;

            if (now.tv_sec > context->last_time_queue.tv_sec)
                pdiff.tv_sec = now.tv_sec - context->last_time_queue.tv_sec;
            if (now.tv_nsec > context->last_time_queue.tv_nsec)
                pdiff.tv_nsec = now.tv_nsec - context->last_time_queue.tv_nsec;

            if (context->last_time_queue.tv_sec > context->last_time.tv_sec)
                pdiff.tv_sec += context->last_time_queue.tv_sec - context->last_time.tv_sec;
            if (context->last_time_queue.tv_nsec > context->last_time.tv_nsec)
                pdiff.tv_nsec += context->last_time_queue.tv_nsec - context->last_time.tv_nsec;

            if (pdiff.tv_nsec > 999999999) {
                pdiff.tv_sec += pdiff.tv_nsec / 1000000000;
                pdiff.tv_nsec %= 1000000000;
            }

            // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "process diff %lu.%09lu", pdiff.tv_sec, pdiff.tv_nsec);

            timersub(&(walkpkt->pkthdr.ts), &(context->last_packet), &diff);

            // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "diff %lu.%06lu", diff.tv_sec, diff.tv_usec);

            sleep_to = context->last_time;
            sleep_to.tv_nsec += diff.tv_usec * 1000;
            if (sleep_to.tv_nsec > 999999999) {
                sleep_to.tv_sec += sleep_to.tv_nsec / 1000000000;
                sleep_to.tv_nsec %= 1000000000;
            }
            sleep_to.tv_sec += diff.tv_sec;

            if (pdiff.tv_sec) {
                if (sleep_to.tv_sec > pdiff.tv_sec)
                    sleep_to.tv_sec -= pdiff.tv_sec;
                else
                    sleep_to.tv_sec = 0;
            }
            if (pdiff.tv_nsec) {
                if (sleep_to.tv_nsec >= pdiff.tv_nsec)
                    sleep_to.tv_nsec -= pdiff.tv_nsec;
                else if (sleep_to.tv_sec) {
                    sleep_to.tv_sec -= 1;
                    sleep_to.tv_nsec += 1000000000 - pdiff.tv_nsec;
                }
                else
                    sleep_to.tv_nsec = 0;
            }

            switch (conf_timing_mode(context->conf)) {
                case TIMING_MODE_INCREASE:
                    sleep_to.tv_nsec += conf_timing_increase(context->conf);
                    break;

                case TIMING_MODE_REDUCE:
                    {
                        unsigned long int nsec = conf_timing_reduce(context->conf);

                        if (nsec > 999999999) {
                            unsigned long int sec = nsec/1000000000;
                            if (sleep_to.tv_sec > sec)
                                sleep_to.tv_sec -= sec;
                            else
                                sleep_to.tv_sec = 0;
                            nsec %= 1000000000;
                        }
                        if (nsec) {
                            if (sleep_to.tv_nsec >= nsec)
                                sleep_to.tv_nsec -= nsec;
                            else if (sleep_to.tv_sec) {
                                sleep_to.tv_sec -= 1;
                                sleep_to.tv_nsec += 1000000000 - nsec;
                            }
                            else
                                sleep_to.tv_nsec = 0;
                        }
                    }
                    break;

                default:
                    break;
            }

            if (sleep_to.tv_nsec > 999999999) {
                sleep_to.tv_sec += sleep_to.tv_nsec / 1000000000;
                sleep_to.tv_nsec %= 1000000000;
            }

            log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "last %lu.%09lu", context->last_time.tv_sec, context->last_time.tv_nsec);
            log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "now %lu.%09lu", now.tv_sec, now.tv_nsec);
            log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "sleep_to %lu.%09lu", sleep_to.tv_sec, sleep_to.tv_nsec);

            if (sleep_to.tv_sec || sleep_to.tv_nsec)
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_to, 0);
        }

        if (clock_gettime(CLOCK_MONOTONIC, &(context->last_time))) {
            log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
            context->last_time.tv_sec = 0;
            context->last_time.tv_nsec = 0;
        }

        context->last_packet = walkpkt->pkthdr.ts;
    }

    queue_dns(context, packet, payload, length);

    if (clock_gettime(CLOCK_MONOTONIC, &(context->last_time_queue))) {
        log_errno(conf_log(context->conf), LNETWORK, LDEBUG, "clock_gettime()");
        context->last_time_queue.tv_sec = 0;
        context->last_time_queue.tv_nsec = 0;
    }
}

void callback_udp(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length) {
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
    do_timing(context, packet, payload, length);
}

void callback_tcp(u_char* user, const pcap_thread_packet_t* packet, const u_char* payload, size_t length) {
    drool_t* context = (drool_t*)user;

    drool_assert(context);
    drool_assert(packet);
    drool_assert(payload);

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "tcp packet received from %s", packet->name);

    context->packets_seen++;

    if (conf_timing_mode(context->conf) == TIMING_MODE_IGNORE) {
        queue_dns(context, packet, payload, length);
        return;
    }
    do_timing(context, packet, payload, length);
}

/*
void callback(u_char* user, const struct pcap_pkthdr* pkthdr, const u_char* pkt, const char* name, int dlt) {
    drool_t* context = (drool_t*)user;

    drool_assert(context);
    drool_assert(pkthdr);
    drool_assert(pkt);
    drool_assert(name);

    log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "packet received from %s", name);

    context->packets_seen++;
}
*/
