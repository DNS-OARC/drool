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

#ifndef __drool_log_h
#define __drool_log_h

#include <stddef.h>
#include <errno.h>

#ifndef LOG_DATETIME
#define LOG_DATETIME 1
#endif
#ifndef LOG_FILENAME_LINE
#define LOG_FILENAME_LINE 1
#endif
#ifndef LOG_SHORT_FILENAME
#define LOG_SHORT_FILENAME 1
#endif
#ifndef LOG_THREAD_ID
#define LOG_THREAD_ID 0
#endif

#define LOG_SETTINGS_T_INIT \
    {                       \
        0, 0, 0, 1, 1, 1    \
    }
#define LOG_SETTINGS_T_INIT_NONE \
    {                            \
        0, 0, 0, 0, 0, 0         \
    }
#define LOG_SETTINGS_T_INIT_ALL \
    {                           \
        1, 1, 1, 1, 1, 1        \
    }
typedef struct drool_log_settings drool_log_settings_t;
struct drool_log_settings {
    unsigned short debug : 1;
    unsigned short info : 1;
    unsigned short notice : 1;
    unsigned short warning : 1;
    unsigned short error : 1;
    unsigned short critical : 1;
};

#define LOG_LEVEL_UNKNOWN_STR "unknown"
#define LOG_LEVEL_DEBUG_STR "debug"
#define LOG_LEVEL_INFO_STR "info"
#define LOG_LEVEL_NOTICE_STR "notice"
#define LOG_LEVEL_WARNING_STR "warning"
#define LOG_LEVEL_ERROR_STR "error"
#define LOG_LEVEL_CRITICAL_STR "critical"
#define LOG_LEVEL_ALL_STR "all"
typedef enum drool_log_level drool_log_level_t;
enum drool_log_level {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,

    LOG_LEVEL_ALL
};
#define LDEBUG LOG_LEVEL_DEBUG
#define LINFO LOG_LEVEL_INFO
#define LNOTICE LOG_LEVEL_NOTICE
#define LWARNING LOG_LEVEL_WARNING
#define LERROR LOG_LEVEL_ERROR
#define LCRITICAL LOG_LEVEL_CRITICAL

int log_level_enable(drool_log_settings_t* settings, const drool_log_level_t level);
int log_level_disable(drool_log_settings_t* settings, const drool_log_level_t level);
const char* log_level_name(const drool_log_level_t level);

#define LOG_FACILITY_UNKNOWN_STR "unknown"
#define LOG_FACILITY_NONE_STR "none"
#define LOG_FACILITY_CORE_STR "core"
#define LOG_FACILITY_NETWORK_STR "network"
typedef enum drool_log_facility drool_log_facility_t;
enum drool_log_facility {
    LOG_FACILITY_NONE = 0,
    LOG_FACILITY_CORE,
    LOG_FACILITY_NETWORK
};
#define LNONE LOG_FACILITY_NONE
#define LCORE LOG_FACILITY_CORE
#define LNETWORK LOG_FACILITY_NETWORK

const char* log_facility_name(const drool_log_facility_t facility);

#define LOG_T_INIT               \
    {                            \
        LOG_SETTINGS_T_INIT_ALL, \
            LOG_SETTINGS_T_INIT, \
            LOG_SETTINGS_T_INIT, \
    }
typedef struct drool_log drool_log_t;
struct drool_log {
    drool_log_settings_t none;
    drool_log_settings_t core;
    drool_log_settings_t network;
};

int log_is_enabled(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level);
int log_enable(drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level);
int log_disable(drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level);

void log_printf_fileline(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level, const char* file, size_t line, const char* format, ...);
#define log_print(log, facility, level, text) \
    log_printf_fileline(log, facility, level, __FILE__, __LINE__, text)
#define log_printf(log, facility, level, format, args...) \
    log_printf_fileline(log, facility, level, __FILE__, __LINE__, format, args)

void log_errnumf_fileline(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level, const char* file, size_t line, int errnum, const char* format, ...);
#define log_errnum(log, facility, level, errnum, text) \
    log_errnumf_fileline(log, facility, level, __FILE__, __LINE__, errnum, text)
#define log_errnumf(log, facility, level, errnum, format, args...) \
    log_errnumf_fileline(log, facility, level, __FILE__, __LINE__, errnum, format, args)

#define log_errno(log, facility, level, text) \
    log_errnumf_fileline(log, facility, level, __FILE__, __LINE__, errno, text)
#define log_errnof(log, facility, level, format, args...) \
    log_errnumf_fileline(log, facility, level, __FILE__, __LINE__, errno, format, args)

#endif /* __drool_log_h */
