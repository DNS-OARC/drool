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

#include "drool.h"
#include "conf.h"
#include "callback.h"
#include "dropback.h"
#include "stats_callback.h"
#include "pcap-thread/pcap_thread.h"
#include "client_pool.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

char* program_name = 0;

static void usage(void) {
    printf(
        "usage: %s [options]\n"
        /* -o            description                                                 .*/
        "  -c [type:]config\n"
        "                Specify the configuration to use, if no type is given then\n"
        "                config expects to be a file. Valid types are file and text.\n"
        "                Can be given multiple times and will be processed in the\n"
        "                given order. See drool.conf(5) for configuration syntax.\n"
        "  -l facility[:level]\n"
        "                Enable logging for facility, optional log level can be given\n"
        "                to enable just that. Can be given multiple times and will be\n"
        "                processed in the given order.\n"
        "  -L facility[:level]\n"
        "                Disable logging for facility, optional log level can be\n"
        "                given to disable just that. Can be given multiple times and\n"
        "                will be processed in the given order.\n"
        "  -f filter     Set the Berkeley Packet Filter to use.\n"
        "  -i interface  Capture packets from interface, can be given multiple times.\n"
        "  -r file.pcap  Read packets from PCAP file, can be given multiple times.\n"
        "  -o interface  Send packets to interface, may not be given with -w.\n"
        "  -w file.pcap  Write packets to PCAP file, may not be given with -o.\n"
        "  -v            Enable verbose, a simple way to enable logging. Can be\n"
        "                given multiple times to increase verbosity level.\n"
        "  -h            Print this help and exit\n"
        "  -V            Print version and exit\n",
        program_name
    );
}

static void version(void) {
    printf("%s version " PACKAGE_VERSION "\n", program_name);
}

struct signal_context {
    sigset_t            set;
    const drool_conf_t* conf;
    pcap_thread_t*      pcap_thread;
};
static void* signal_handler_thread(void* arg) {
    struct signal_context* context = (struct signal_context*)arg;
    int sig, err;

    while (1) {
        sig = 0;
        err = sigwait(&(context->set), &sig);

        log_printf(conf_log(context->conf), LCORE, LDEBUG, "signal %d received", sig);

        if (sig == SIGINT) {
            pcap_thread_stop(context->pcap_thread);
            continue;
        }
        break;
    }

    exit(DROOL_ESIGRCV);

    return 0;
}

int main(int argc, char* argv[]) {
    int opt, err;
    size_t verbose = 0;
    drool_conf_t conf = CONF_T_INIT;
    struct signal_context sigcontext;
    pcap_thread_t pcap_thread = PCAP_THREAD_T_INIT;
    struct timespec ts_start, ts_end, ts_diff;
    drool_t* contexts = 0;
    drool_t* context;

    if ((program_name = strrchr(argv[0], '/'))) {
        program_name++;
    }
    else {
        program_name = argv[0];
    }

    while ((opt = getopt(argc, argv, "c:l:L:f:i:r:o:w:vhV")) != -1) {
        switch (opt) {
            case 'c':
                if (!strncmp(optarg, "file:", 5)) {
                    err = conf_parse_file(&conf, optarg + 5);
                }
                else if (!strncmp(optarg, "text:", 5)) {
                    err = conf_parse_text(&conf, optarg + 5, strlen(optarg) - 5);
                }
                else {
                    err = conf_parse_file(&conf, optarg);
                }
                if (err) {
                    fprintf(stderr, "Unable to read conf file: %s\n", conf_strerr(err));
                    exit(DROOL_ECONF);
                }
                break;

            case 'l':
            case 'L':
                {
                    char* level_str = strchr(optarg, ':');
                    drool_log_facility_t facility = LOG_FACILITY_NONE;
                    drool_log_level_t level = LOG_LEVEL_ALL;
                    size_t len = 0;
                    int all = 0;

                    if (level_str) {
                        level_str++;
                        len = level_str - optarg;

                        if (!strcmp(level_str, "debug")) {
                            level = LOG_LEVEL_DEBUG;
                        }
                        else if (!strcmp(level_str, "info")) {
                            level = LOG_LEVEL_INFO;
                        }
                        else if (!strcmp(level_str, "notice")) {
                            level = LOG_LEVEL_NOTICE;
                        }
                        else if (!strcmp(level_str, "warning")) {
                            level = LOG_LEVEL_WARNING;
                        }
                        else if (!strcmp(level_str, "error")) {
                            level = LOG_LEVEL_ERROR;;
                        }
                        else if (!strcmp(level_str, "critical")) {
                            level = LOG_LEVEL_CRITICAL;
                        }
                        else {
                            fprintf(stderr, "Invalid log level %s\n", level_str);
                            exit(DROOL_EOPT);
                        }
                    }

                    if ((len && len == 4 && !strncmp(optarg, "core", 4))
                        || (!len && strcmp(optarg, "core")))
                    {
                        facility = LOG_FACILITY_CORE;
                    }
                    else if ((len && len == 7 && !strncmp(optarg, "network", 7))
                        || (!len && strcmp(optarg, "network")))
                    {
                        facility = LOG_FACILITY_NETWORK;
                    }
                    else if ((len && len == 3 && !strncmp(optarg, "all", 3))
                        || (!len && strcmp(optarg, "all")))
                    {
                        all = 1;
                    }
                    else {
                        if (len)
                            fprintf(stderr, "Invalid log facility %.*s\n", (int)len, optarg);
                        else
                            fprintf(stderr, "Invalid log facility %s\n", optarg);
                        exit(DROOL_EOPT);
                    }

                    if (all) {
                        if ((opt == 'l' && log_enable(conf_log_rw(&conf), LOG_FACILITY_CORE, level))
                            || (opt == 'L' && log_disable(conf_log_rw(&conf), LOG_FACILITY_CORE, level)))
                        {
                            fprintf(stderr, "Unable to %s log facility[:level] %s\n", opt == 'l' ? "enable" : "disable", optarg);
                            exit(DROOL_EOPT);
                        }
                        if ((opt == 'l' && log_enable(conf_log_rw(&conf), LOG_FACILITY_NETWORK, level))
                            || (opt == 'L' && log_disable(conf_log_rw(&conf), LOG_FACILITY_NETWORK, level)))
                        {
                            fprintf(stderr, "Unable to %s log facility[:level] %s\n", opt == 'l' ? "enable" : "disable", optarg);
                            exit(DROOL_EOPT);
                        }
                    }
                    else {
                        if ((opt == 'l' && log_enable(conf_log_rw(&conf), facility, level))
                            || (opt == 'L' && log_disable(conf_log_rw(&conf), facility, level)))
                        {
                            fprintf(stderr, "Unable to %s log facility[:level] %s\n", opt == 'l' ? "enable" : "disable", optarg);
                            exit(DROOL_EOPT);
                        }
                    }
                }
                break;

            case 'f':
                if ((err = conf_set_filter(&conf, optarg, 0)) != CONF_OK) {
                    fprintf(stderr, "Unable to set filter to %s: %s\n", optarg, conf_strerr(err));
                    exit(DROOL_EOPT);
                }
                break;

            case 'i':
                if ((err = conf_add_input(&conf, optarg, 0)) != CONF_OK) {
                    fprintf(stderr, "Unable to add interface %s as input: %s\n", optarg, conf_strerr(err));
                    exit(DROOL_EOPT);
                }
                break;

            case 'r':
                if ((err = conf_add_read(&conf, optarg, 0)) != CONF_OK) {
                    fprintf(stderr, "Unable to add file %s as input: %s\n", optarg, conf_strerr(err));
                    exit(DROOL_EOPT);
                }
                break;

            case 'o':
                if ((err = conf_set_output(&conf, optarg, 0)) != CONF_OK) {
                    fprintf(stderr, "Unable to set interface %s as output: %s\n", optarg, err == CONF_EEXIST ? "Another output already set" : conf_strerr(err));
                    exit(DROOL_EOPT);
                }
                break;

            case 'w':
                if ((err = conf_set_write(&conf, optarg, 0)) != CONF_OK) {
                    fprintf(stderr, "Unable to set file %s as output: %s\n", optarg, err == CONF_EEXIST ? "Another output already set" : conf_strerr(err));
                    exit(DROOL_EOPT);
                }
                break;

            case 'v':
                verbose++;

                if (verbose == 1) {
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_CORE, LOG_LEVEL_NOTICE);
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_NOTICE);
                }
                if (verbose == 2) {
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_CORE, LOG_LEVEL_INFO);
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_INFO);
                }
                if (verbose == 3) {
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_CORE, LOG_LEVEL_DEBUG);
                    log_enable(conf_log_rw(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_DEBUG);
                }

                break;

            case 'h':
                usage();
                exit(0);

            case 'V':
                version();
                exit(0);

            default:
                usage();
                exit(DROOL_EOPT);
        }
    }

    /*
     * Setup signal handling
     */

    log_print(conf_log(&conf), LCORE, LINFO, "setup signal handling");

    sigfillset(&(sigcontext.set));
    if ((err = pthread_sigmask(SIG_BLOCK, &(sigcontext.set), 0))) {
        log_errno(conf_log(&conf), LCORE, LCRITICAL, "Unable to set blocked signals with pthread_sigmask()");
        exit(DROOL_ESIGNAL);
    }

    sigemptyset(&(sigcontext.set));
    sigaddset(&(sigcontext.set), SIGTERM);
    sigaddset(&(sigcontext.set), SIGQUIT);
    sigaddset(&(sigcontext.set), SIGINT);

    sigcontext.conf = &conf;
    sigcontext.pcap_thread = &pcap_thread;

    {
        pthread_t sighthr;

        if ((err = pthread_create(&sighthr, 0, &signal_handler_thread, (void*)&sigcontext))) {
            log_errno(conf_log(&conf), LCORE, LCRITICAL, "Unable to start signal thread with pthread_create()");
            exit(DROOL_ESIGNAL);
        }
    }

    /*
     * Initialize PCAP thread
     */

    log_print(conf_log(&conf), LCORE, LINFO, "initialize pcap-thread");

    /* TODO: make most pcap-thread options configurable */
    {
        const char* errstr = "Unknown error";

        if ((err = pcap_thread_set_use_threads(&pcap_thread, 1)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread use threads";
        }
        else if ((err = pcap_thread_set_snaplen(&pcap_thread, 64*1024)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread snaplen";
        }
        else if ((err = pcap_thread_set_buffer_size(&pcap_thread, 4*1024*1024)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread buffer size";
        }
        else if (conf_have_filter(&conf) && (err = pcap_thread_set_filter(&pcap_thread, conf_filter(&conf), conf_filter_length(&conf))) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread filter";
        }
        else if ((err = pcap_thread_set_queue_mode(&pcap_thread, PCAP_THREAD_QUEUE_MODE_DIRECT)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread queue mode to direct";
        }
        else if ((err = pcap_thread_set_use_layers(&pcap_thread, 1)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread use layers";
        }
        else if ((err = pcap_thread_set_callback_udp(&pcap_thread, &callback_udp)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread udp callback";
        }
        else if ((err = pcap_thread_set_callback_tcp(&pcap_thread, &callback_tcp)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread tcp callback";
        }
        else if ((err = pcap_thread_set_dropback(&pcap_thread, &dropback)) != PCAP_THREAD_OK) {
            errstr = "Unable to set pcap-thread dropback";
        }

        if (err == PCAP_THREAD_ERRNO)
            log_errnof(conf_log(&conf), LCORE, LCRITICAL, "%s: %s", errstr, pcap_thread_errbuf(&pcap_thread));
        else if (err == PCAP_THREAD_EPCAP && pcap_thread_status(&pcap_thread))
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "%s: %s: %s: %s", errstr, pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread), pcap_statustostr(pcap_thread_status(&pcap_thread)));
        else if (err == PCAP_THREAD_EPCAP)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "%s: %s: %s", errstr, pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread));
        else if (err != PCAP_THREAD_OK)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "%s: %s", errstr, pcap_thread_strerr(err));
        if (err != PCAP_THREAD_OK)
            exit(DROOL_EPCAPT);

        if (conf_have_read(&conf)) {
            const drool_conf_file_t* conf_file = conf_read(&conf);

            while (conf_file) {
                if (!(context = calloc(1, sizeof(drool_t)))) {
                    log_errno(conf_log(&conf), LCORE, LCRITICAL, "Unable to allocate context with calloc()");
                    exit(DROOL_ENOMEM);
                }

                context->conf = &conf;
                context->client_pool = client_pool_new(&conf); /* TODO */
                context->next = contexts;
                contexts = context;

                if ((err = pcap_thread_open_offline(&pcap_thread, conf_file_name(conf_file), (void*)context))) {
                    if (err == PCAP_THREAD_ERRNO)
                        log_errnof(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread offline file %s: %s", conf_file_name(conf_file), pcap_thread_errbuf(&pcap_thread));
                    else if (err == PCAP_THREAD_EPCAP && pcap_thread_status(&pcap_thread))
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread offline file %s: %s: %s: %s", conf_file_name(conf_file), pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread), pcap_statustostr(pcap_thread_status(&pcap_thread)));
                    else if (err == PCAP_THREAD_EPCAP)
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread offline file %s: %s: %s", conf_file_name(conf_file), pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread));
                    else if (err != PCAP_THREAD_OK)
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread offline file %s: %s", conf_file_name(conf_file), pcap_thread_strerr(err));
                    exit(DROOL_EPCAPT);
                }
                conf_file = conf_file_next(conf_file);
            }
        }

        if (conf_have_input(&conf)) {
            const drool_conf_interface_t* conf_interface = conf_input(&conf);

            while (conf_interface) {
                if (!(context = calloc(1, sizeof(drool_t)))) {
                    log_errno(conf_log(&conf), LCORE, LCRITICAL, "Unable to allocate context with calloc()");
                    exit(DROOL_ENOMEM);
                }

                context->conf = &conf;
                context->client_pool = client_pool_new(&conf); /* TODO */
                context->next = contexts;
                contexts = context;

                if ((err = pcap_thread_open(&pcap_thread, conf_interface_name(conf_interface), (void*)context))) {
                    if (err == PCAP_THREAD_ERRNO)
                        log_errnof(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread interface %s: %s", conf_interface_name(conf_interface), pcap_thread_errbuf(&pcap_thread));
                    else if (err == PCAP_THREAD_EPCAP && pcap_thread_status(&pcap_thread))
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread interface %s: %s: %s: %s", conf_interface_name(conf_interface), pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread), pcap_statustostr(pcap_thread_status(&pcap_thread)));
                    else if (err == PCAP_THREAD_EPCAP)
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread interface %s: %s: %s", conf_interface_name(conf_interface), pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread));
                    else if (err != PCAP_THREAD_OK)
                        log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to open pcap-thread interface %s: %s", conf_interface_name(conf_interface), pcap_thread_strerr(err));
                    exit(DROOL_EPCAPT);
                }
                conf_interface = conf_interface_next(conf_interface);
            }
        }
    }

    /*
     * Start the engine
     */

    log_print(conf_log(&conf), LCORE, LINFO, "start");

    ts_start.tv_sec = 0;
    ts_start.tv_nsec = 0;
    ts_end = ts_diff = ts_start;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* TODO */
    for (context = contexts; context; context = context->next) {
        client_pool_start(context->client_pool);
    }

    if ((err = pcap_thread_run(&pcap_thread)) != PCAP_THREAD_OK) {
        if (err == PCAP_THREAD_ERRNO)
            log_errnof(conf_log(&conf), LCORE, LCRITICAL, "Unable to run pcap-thread: %s", pcap_thread_errbuf(&pcap_thread));
        else if (err == PCAP_THREAD_EPCAP && pcap_thread_status(&pcap_thread))
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to run pcap-thread: %s: %s: %s", pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread), pcap_statustostr(pcap_thread_status(&pcap_thread)));
        else if (err == PCAP_THREAD_EPCAP)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to run pcap-thread: %s: %s", pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread));
        else if (err != PCAP_THREAD_OK)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to run pcap-thread: %s", pcap_thread_strerr(err));
        exit(DROOL_EPCAPT);
    }

    /* TODO */
    for (context = contexts; context; context = context->next) {
        client_pool_stop(context->client_pool);
        client_pool_free(context->client_pool);
    }

    /*
     * Finish
     */

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    log_print(conf_log(&conf), LCORE, LINFO, "end");

    {
        float pkts_fraction = 0;
        uint64_t seen = 0, sent = 0, dropped = 0, ignored = 0, size = 0;

        for (context = contexts; context; context = context->next) {
            seen += context->packets_seen;
            sent += context->packets_sent;
            size += context->packets_size;
            dropped += context->packets_dropped;
            ignored += context->packets_ignored;
        }

        if (ts_end.tv_sec == ts_start.tv_sec && ts_end.tv_nsec >= ts_start.tv_nsec) {
            pkts_fraction = 1 / (((float)ts_end.tv_nsec - (float)ts_start.tv_nsec) / (float)1000000000);
            log_printf(conf_log(&conf), LCORE, LINFO, "runtime 0.%09ld seconds", ts_end.tv_nsec - ts_start.tv_nsec);
        }
        else if (ts_end.tv_sec > ts_start.tv_sec) {
            long nsec = 1000000000 - ts_start.tv_nsec + ts_end.tv_nsec;
            long sec = ts_end.tv_sec - ts_start.tv_sec - 1;

            pkts_fraction = 1 / (((float)ts_end.tv_sec - (float)ts_start.tv_sec - 1) + ((float)nsec/(float)1000000000));

            if (nsec > 1000000000) {
                sec += nsec / 1000000000;
                nsec %= 1000000000;
            }

            log_printf(conf_log(&conf), LCORE, LINFO, "runtime %ld.%09ld seconds", sec, nsec);
        }
        else {
            log_print(conf_log(&conf), LCORE, LINFO, "Unable to compute runtime, clock is behind starting point");
        }

        log_printf(conf_log(&conf), LCORE, LINFO, "saw %lu packets, %.0f/pps", seen, seen*pkts_fraction);
        log_printf(conf_log(&conf), LCORE, LINFO, "sent %lu packets, %.0f/pps %.0f/abpp", sent, sent*pkts_fraction, (float)size/(float)sent);
        log_printf(conf_log(&conf), LCORE, LINFO, "dropped %lu packets", dropped);
        log_printf(conf_log(&conf), LCORE, LINFO, "ignored %lu packets", ignored);
    }

    if ((err = pcap_thread_stats(&pcap_thread, &drool_stats_callback, (void*)&conf)) != PCAP_THREAD_OK) {
        if (err == PCAP_THREAD_ERRNO)
            log_errnof(conf_log(&conf), LCORE, LCRITICAL, "Unable to get pcap-thread stats: %s", pcap_thread_errbuf(&pcap_thread));
        else if (err == PCAP_THREAD_EPCAP && pcap_thread_status(&pcap_thread))
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to get pcap-thread stats: %s: %s: %s", pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread), pcap_statustostr(pcap_thread_status(&pcap_thread)));
        else if (err == PCAP_THREAD_EPCAP)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to get pcap-thread stats: %s: %s", pcap_thread_strerr(err), pcap_thread_errbuf(&pcap_thread));
        else if (err != PCAP_THREAD_OK)
            log_printf(conf_log(&conf), LCORE, LCRITICAL, "Unable to get pcap-thread stats: %s", pcap_thread_strerr(err));
        exit(DROOL_EPCAPT);
    }

    pcap_thread_close(&pcap_thread);

    return 0;
}
