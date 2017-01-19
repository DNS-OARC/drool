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

#include "assert.h"
#include "log.h"

#include <stdarg.h>
#include <stdio.h>

inline int log_level_enable(log_settings_t* settings, const log_level_t level) {
    drool_assert(settings);

    switch (level) {
        case LOG_LEVEL_DEBUG:
            settings->debug = 1;
            return 0;
        case LOG_LEVEL_INFO:
            settings->info = 1;
            return 0;
        case LOG_LEVEL_NOTICE:
            settings->notice = 1;
            return 0;
        case LOG_LEVEL_WARNING:
            settings->warning = 1;
            return 0;
        case LOG_LEVEL_ERROR:
            settings->error = 1;
            return 0;
        case LOG_LEVEL_CRITICAL:
            settings->critical = 1;
            return 0;
        case LOG_LEVEL_ALL:
            settings->debug = 1;
            settings->info = 1;
            settings->notice = 1;
            settings->warning = 1;
            settings->error = 1;
            settings->critical = 1;
            return 0;
        default:
            break;
    }

    return -1;
}

inline int log_level_disable(log_settings_t* settings, const log_level_t level) {
    drool_assert(settings);

    switch (level) {
        case LOG_LEVEL_DEBUG:
            settings->debug = 0;
            return 0;
        case LOG_LEVEL_INFO:
            settings->info = 0;
            return 0;
        case LOG_LEVEL_NOTICE:
            settings->notice = 0;
            return 0;
        case LOG_LEVEL_WARNING:
            settings->warning = 0;
            return 0;
        case LOG_LEVEL_ERROR:
            settings->error = 0;
            return 0;
        case LOG_LEVEL_CRITICAL:
            settings->critical = 0;
            return 0;
        case LOG_LEVEL_ALL:
            settings->debug = 0;
            settings->info = 0;
            settings->notice = 0;
            settings->warning = 0;
            settings->error = 0;
            settings->critical = 0;
            return 0;
        default:
            break;
    }

    return -1;
}

inline int log_enable(log_t* log, const log_facility_t facility, const log_level_t level) {
    drool_assert(log);

    switch (facility) {
        case LOG_FACILITY_CORE:
            return log_level_enable(&(log->core), level);
        case LOG_FACILITY_NETWORK:
            return log_level_enable(&(log->network), level);
        default:
            break;
    }

    return -1;
}

inline int log_disable(log_t* log, const log_facility_t facility, const log_level_t level) {
    drool_assert(log);

    switch (facility) {
        case LOG_FACILITY_CORE:
            return log_level_disable(&(log->core), level);
        case LOG_FACILITY_NETWORK:
            return log_level_disable(&(log->network), level);
        default:
            break;
    }

    return -1;
}

void log_printf_fileline(const log_t* log, const log_facility_t facility, const log_level_t level, const char* file, size_t line, const char* format, ...) {
    va_list ap;
    const log_settings_t* settings = 0;
    const char* facility_name = LOG_FACILITY_UNKNOWN_STR;
    const char* level_name = LOG_LEVEL_UNKNOWN_STR;

    drool_assert(log);
    if (!log) {
        return;
    }
    drool_assert(format);
    if (!format) {
        return;
    }

    switch (facility) {
        case LOG_FACILITY_CORE:
            settings = &(log->core);
            facility_name = LOG_FACILITY_CORE_STR;
            break;

        case LOG_FACILITY_NETWORK:
            settings = &(log->network);
            facility_name = LOG_FACILITY_NETWORK_STR;
            break;

        case LOG_FACILITY_NONE:
            facility_name = LOG_FACILITY_NONE_STR;
        default:
            settings = &(log->none);
    }

    drool_assert(settings);
    if (!settings) {
        return;
    }

    switch (level) {
        case LOG_LEVEL_DEBUG:
            if (!settings->debug)
                return;
            level_name = LOG_LEVEL_DEBUG_STR;
            break;

        case LOG_LEVEL_INFO:
            if (!settings->info)
                return;
            level_name = LOG_LEVEL_INFO_STR;
            break;

        case LOG_LEVEL_NOTICE:
            if (!settings->notice)
                return;
            level_name = LOG_LEVEL_NOTICE_STR;
            break;

        case LOG_LEVEL_WARNING:
            if (!settings->warning)
                return;
            level_name = LOG_LEVEL_WARNING_STR;
            break;

        case LOG_LEVEL_ERROR:
            if (!settings->error)
                return;
            level_name = LOG_LEVEL_ERROR_STR;
            break;

        case LOG_LEVEL_CRITICAL:
            if (!settings->critical)
                return;
            level_name = LOG_LEVEL_CRITICAL_STR;
            break;
    }

    printf("%s:%06lu %s %s: ", file, line, facility_name, level_name);
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\n");
}
