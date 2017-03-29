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
#include "assert.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

static const drool_log_settings_t* get_facility(const drool_log_t* log, const drool_log_facility_t facility) {
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

inline int log_level_enable(drool_log_settings_t* settings, const drool_log_level_t level) {
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

inline int log_level_disable(drool_log_settings_t* settings, const drool_log_level_t level) {
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

const char* log_level_name(const drool_log_level_t level) {
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

const char* log_facility_name(const drool_log_facility_t facility) {
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

int log_is_enabled(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level) {
    const drool_log_settings_t* settings;

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

        default:
            break;
    }

    return 0;
}

inline int log_enable(drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level) {
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

inline int log_disable(drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level) {
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

void log_printf_fileline(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level, const char* file, size_t line, const char* format, ...) {
    va_list ap;
    char buf[1024];
    size_t s;
    int n, n2;
#if LOG_FILENAME_LINE && LOG_SHORT_FILENAME
    char *filep;
#endif
#if LOG_DATETIME
    struct tm tm;
    time_t t;
#endif

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

#if LOG_DATETIME
    memset(&tm, 0, sizeof(tm));
    time(&t);
    localtime_r(&t, &tm);
#endif

    s = sizeof(buf) - 2;
    n = snprintf(buf, s,
#if LOG_DATETIME
        "%d-%02d-%02d %02d:%02d:%02d "
#endif
#if LOG_FILENAME_LINE
        "%s:%04lu "
#endif
#if LOG_THREAD_ID
        "t:%lu "
#endif
        "%s %s: ",
#if LOG_DATETIME
        1900+tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
#endif
#if LOG_FILENAME_LINE
#if LOG_SHORT_FILENAME
        (filep = strrchr(file, '/')) ? (filep+1) : file,
#else
        file,
#endif
        line,
#endif
#if LOG_THREAD_ID
        pthread_self(),
#endif
        log_facility_name(facility), log_level_name(level)
    );
    if (n < 1) {
        printf("log_printf_fileline(): snprintf() failed with %d\n", n);
        return;
    }
    if (n < s) {
        va_start(ap, format);
        n2 = vsnprintf(&buf[n], s - n, format, ap);
        va_end(ap);
        if (n2 < 1) {
            printf("log_printf_fileline(): vsnprintf() failed with %d\n", n2);
            return;
        }
    }
    fprintf(stdout, "%s\n", buf);
    fflush(stdout);
}

void log_errnumf_fileline(const drool_log_t* log, const drool_log_facility_t facility, const drool_log_level_t level, const char* file, size_t line, int errnum, const char* format, ...) {
    va_list ap;
    char errbuf[512];
    char buf[1024];
    size_t s;
    int n, n2;
#if LOG_FILENAME_LINE && LOG_SHORT_FILENAME
    char *filep;
#endif
#if LOG_DATETIME
    struct tm tm;
    time_t t;
#endif

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

    memset(errbuf, 0, sizeof(errbuf));

#if ((_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    /* XSI-compliant version */
    {
        int ret = strerror_r(errnum, errbuf, sizeof(errbuf));
        if (ret > 0) {
            (void)strerror_r(ret, errbuf, sizeof(errbuf));
        }
        else {
            (void)strerror_r(errno, errbuf, sizeof(errbuf));
        }
    }
#else
    /* GNU-specific version */
    errbuf = strerror_r(errnum, errbuf, sizeof(errbuf));
#endif

#if LOG_DATETIME
    memset(&tm, 0, sizeof(tm));
    time(&t);
    localtime_r(&t, &tm);
#endif

    s = sizeof(buf) - 2;
    n = snprintf(buf, s,
#if LOG_DATETIME
        "%d-%02d-%02d %02d:%02d:%02d "
#endif
#if LOG_FILENAME_LINE
        "%s:%04lu "
#endif
#if LOG_THREAD_ID
        "t:%lu "
#endif
        "%s %s: ",
#if LOG_DATETIME
        1900+tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
#endif
#if LOG_FILENAME_LINE
#if LOG_SHORT_FILENAME
        (filep = strrchr(file, '/')) ? (filep+1) : file,
#else
        file,
#endif
        line,
#endif
#if LOG_THREAD_ID
        pthread_self(),
#endif
        log_facility_name(facility), log_level_name(level)
    );
    if (n < 1) {
        printf("log_printf_fileline(): snprintf() failed with %d\n", n);
        return;
    }
    if (n < s) {
        va_start(ap, format);
        n2 = vsnprintf(&buf[n], s - n, format, ap);
        va_end(ap);
        if (n2 < 1) {
            printf("log_printf_fileline(): vsnprintf() failed with %d\n", n2);
            return;
        }
    }
    fprintf(stdout, "%s: %s\n", buf, errbuf);
    fflush(stdout);
};
