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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char* argv[]) {
    int opt, err;
    size_t verbose = 0;
    conf_t conf = CONF_T_INIT;

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
                    err = conf_parse_text(&conf, optarg + 5);
                }
                else {
                    err = conf_parse_file(&conf, optarg);
                }
                if (err) {
                    fprintf(stderr, "Unable to read conf file\n");
                    exit(3);
                }
                break;

            case 'l':
            case 'L':
                {
                    char* level_str = strchr(optarg, ':');
                    log_facility_t facility = LOG_FACILITY_NONE;
                    log_level_t level = LOG_LEVEL_ALL;
                    size_t len = 0;

                    if (level_str) {
                        level_str++;
                        len = level_str - optarg;

                        if (strcmp(level_str, "debug")) {
                            level = LOG_LEVEL_DEBUG;
                        }
                        else if (strcmp(level_str, "info")) {
                            level = LOG_LEVEL_INFO;
                        }
                        else if (strcmp(level_str, "notice")) {
                            level = LOG_LEVEL_NOTICE;
                        }
                        else if (strcmp(level_str, "warning")) {
                            level = LOG_LEVEL_WARNING;
                        }
                        else if (strcmp(level_str, "error")) {
                            level = LOG_LEVEL_ERROR;;
                        }
                        else if (strcmp(level_str, "critical")) {
                            level = LOG_LEVEL_CRITICAL;
                        }
                        else {
                            fprintf(stderr, "Invalid log level %s\n", level_str);
                            exit(2);
                        }
                    }

                    if ((len && len == 4 && strncmp(optarg, "core", 4))
                        || (!len && strcmp(optarg, "core")))
                    {
                        facility = LOG_FACILITY_CORE;
                    }
                    else if ((len && len == 7 && strncmp(optarg, "network", 7))
                        || (!len && strcmp(optarg, "network")))
                    {
                        facility = LOG_FACILITY_NETWORK;
                    }
                    else {
                        if (len)
                            fprintf(stderr, "Invalid log facility %*s\n", (int)len, optarg);
                        else
                            fprintf(stderr, "Invalid log facility %s\n", optarg);
                        exit(2);
                    }

                    if ((opt == 'l' && log_enable(conf_log(&conf), facility, level))
                        || (opt == 'L' && log_disable(conf_log(&conf), facility, level)))
                    {
                        fprintf(stderr, "Unable to %s log facility[:level] %s\n", opt == 'l' ? "enable" : "disable", optarg);
                        exit(2);
                    }
                }
                break;

            case 'f':
                if (conf_set_filter(&conf, optarg) != CONF_OK) {
                    fprintf(stderr, "Unable to set filter to %s\n", optarg);
                    exit(2);
                }
                break;

            case 'i':
                if (conf_add_input(&conf, optarg) != CONF_OK) {
                    fprintf(stderr, "Unable to add interface %s as input\n", optarg);
                    exit(2);
                }
                break;

            case 'r':
                if (conf_add_read(&conf, optarg) != CONF_OK) {
                    fprintf(stderr, "Unable to add file %s as input\n", optarg);
                    exit(2);
                }
                break;

            case 'o':
                if ((err = conf_set_output(&conf, optarg)) != CONF_OK) {
                    fprintf(stderr, "Unable to set interface %s as output%s\n", optarg, err == CONF_EEXIST ? ": Another output already set" : "");
                    exit(2);
                }
                break;

            case 'w':
                if ((err = conf_set_write(&conf, optarg)) != CONF_OK) {
                    fprintf(stderr, "Unable to set file %s as output%s\n", optarg, err == CONF_EEXIST ? ": Another output already set" : "");
                    exit(2);
                }
                break;

            case 'v':
                verbose++;
                break;

            case 'h':
                usage();
                exit(0);

            case 'V':
                version();
                exit(0);

            default:
                usage();
                exit(2);
        }
    }

    if (verbose--) {
        log_enable(conf_log(&conf), LOG_FACILITY_CORE, LOG_LEVEL_NOTICE);
        log_enable(conf_log(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_NOTICE);
        if (verbose--) {
            log_enable(conf_log(&conf), LOG_FACILITY_CORE, LOG_LEVEL_INFO);
            log_enable(conf_log(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_INFO);
            if (verbose--) {
                log_enable(conf_log(&conf), LOG_FACILITY_CORE, LOG_LEVEL_DEBUG);
                log_enable(conf_log(&conf), LOG_FACILITY_NETWORK, LOG_LEVEL_DEBUG);
            }
        }
    }

    log_print(conf_log(&conf), LCORE, LDEBUG, "Start");

    return 0;
}
