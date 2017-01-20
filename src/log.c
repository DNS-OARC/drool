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

#include "log.h"
#include "drool.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static const log_settings_t* get_facility(const log_t* log, const log_facility_t facility) {
    drool_assert(log);
    switch (facility) {
        case LOG_FACILITY_CORE:
            return &(log->core);
        case LOG_FACILITY_NETWORK:
            return &(log->network);
        default:
            break;
    }
    return &(log->none);
}

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

const char* log_level_name(const log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return LOG_LEVEL_DEBUG_STR;
        case LOG_LEVEL_INFO:
            return LOG_LEVEL_INFO_STR;
        case LOG_LEVEL_NOTICE:
            return LOG_LEVEL_NOTICE_STR;
        case LOG_LEVEL_WARNING:
            return LOG_LEVEL_WARNING_STR;
        case LOG_LEVEL_ERROR:
            return LOG_LEVEL_ERROR_STR;
        case LOG_LEVEL_CRITICAL:
            return LOG_LEVEL_CRITICAL_STR;
        case LOG_LEVEL_ALL:
            return LOG_LEVEL_ALL_STR;
    }
    return LOG_LEVEL_UNKNOWN_STR;
}

const char* log_facility_name(const log_facility_t facility) {
    switch (facility) {
        case LOG_FACILITY_CORE:
            return LOG_FACILITY_CORE_STR;
        case LOG_FACILITY_NETWORK:
            return LOG_FACILITY_NETWORK_STR;
        case LOG_FACILITY_NONE:
            return LOG_FACILITY_NONE_STR;
    }
    return LOG_FACILITY_UNKNOWN_STR;
}

int log_is_enabled(const log_t* log, const log_facility_t facility, const log_level_t level) {
    const log_settings_t* settings;

    drool_assert(log);
    if (!log) {
        return 0;
    }

    settings = get_facility(log, facility);
    drool_assert(settings);
    if (!settings) {
        return 0;
    }

    switch (level) {
        case LOG_LEVEL_DEBUG:
            if (settings->debug)
                return 1;
            break;

        case LOG_LEVEL_INFO:
            if (settings->info)
                return 1;
            break;

        case LOG_LEVEL_NOTICE:
            if (settings->notice)
                return 1;
            break;

        case LOG_LEVEL_WARNING:
            if (settings->warning)
                return 1;
            break;

        case LOG_LEVEL_ERROR:
            if (settings->error)
                return 1;
            break;

        case LOG_LEVEL_CRITICAL:
            if (settings->critical)
                return 1;
            break;
    }

    return 0;
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

    drool_assert(log);
    if (!log) {
        return;
    }
    drool_assert(format);
    if (!format) {
        return;
    }

    if (!log_is_enabled(log, facility, level)) {
        return;
    }

    printf("%s:%06lu %s %s: ", file, line, log_facility_name(facility), log_level_name(level));
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf("\n");
}

void log_errnof_fileline(const log_t* log, const log_facility_t facility, const log_level_t level, const char* file, size_t line, const char* format, ...) {
    va_list ap;
    char buf[512];
    int errnum = errno;

    drool_assert(log);
    if (!log) {
        return;
    }
    drool_assert(format);
    if (!format) {
        return;
    }

    if (!log_is_enabled(log, facility, level)) {
        return;
    }

    memset(buf, 0, sizeof(buf));

#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE) || defined(__FreeBSD__) || defined(__OpenBSD__)
    /* XSI-compliant version */
    {
        int ret = strerror_r(errnum, buf, sizeof(buf));
        if (ret > 0) {
            (void)strerror_r(ret, buf, sizeof(buf));
        }
        else {
            (void)strerror_r(errno, buf, sizeof(buf));
        }
    }
#else
    /* GNU-specific version */
    buf = strerror_r(errnum, buf, sizeof(buf));
#endif

    printf("%s:%06lu %s %s: ", file, line, log_facility_name(facility), log_level_name(level));
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    printf(": %s\n", buf);
};
