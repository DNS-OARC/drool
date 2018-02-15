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

static void do_timing(drool_t* context, const pcap_thread_packet_t* packet, const u_char* payload, size_t length)
{
    const pcap_thread_packet_t* walkpkt;

    for (walkpkt = packet; walkpkt; walkpkt = walkpkt->prevpkt) {
        if (walkpkt->have_pkthdr) {
            break;
        }
    }
    if (walkpkt) {
	struct timespec mono_now;
	if (clock_gettime(CLOCK_MONOTONIC, &mono_now)) {
		drool_assert("unable to sync clocks" == NULL);
	}

	/* init */
	if (!context->mono_diff.tv_sec && !context->mono_diff.tv_nsec) {
		context->mono_diff.tv_sec = mono_now.tv_sec - walkpkt->pkthdr.ts.tv_sec;
		context->mono_diff.tv_nsec = mono_now.tv_nsec - walkpkt->pkthdr.ts.tv_usec * 1000;
	}

	struct timespec sleep_to;
	sleep_to.tv_sec = context->mono_diff.tv_sec + walkpkt->pkthdr.ts.tv_sec;
	sleep_to.tv_nsec = context->mono_diff.tv_nsec + walkpkt->pkthdr.ts.tv_usec * 1000;
	if (sleep_to.tv_nsec > 1e9) {
		sleep_to.tv_sec += 1;
		sleep_to.tv_nsec -= 1e9;
	} else if (sleep_to.tv_nsec < 0) {
		sleep_to.tv_sec -= 1;
		sleep_to.tv_nsec += 1e9;
	}
	drool_assert(sleep_to.tv_sec > 0);
	drool_assert(sleep_to.tv_nsec > 0);
	drool_assert(sleep_to.tv_nsec < 1e9);
	if (sleep_to.tv_sec - mono_now.tv_sec < 0 || ((sleep_to.tv_sec == mono_now.tv_sec) && sleep_to.tv_nsec - mono_now.tv_nsec < -1e6)) {
		printf("VELKY SPATNY, lag %ld sec %ld nsec\n", sleep_to.tv_sec - mono_now.tv_sec, sleep_to.tv_nsec - mono_now.tv_nsec);
	} else {
		int ret;
		while ((ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleep_to, 0)) == EINTR);
	}

        // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "pkthdr.ts %lu.%06lu", walkpkt->pkthdr.ts.tv_sec, walkpkt->pkthdr.ts.tv_usec);
        // log_printf(conf_log(context->conf), LNETWORK, LDEBUG, "last_packet %lu.%06lu", context->last_packet.tv_sec, context->last_packet.tv_usec);


    }

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
    do_timing(context, packet, payload, length);
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
