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

#include "socket_pool.h"
#include "drool.h"
#include "log.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * New/free
 */

static sllq_t socket_pool_sllq_init = SLLQ_T_INIT;

drool_socket_pool_t* socket_pool_new(const drool_conf_t* conf, int family, int type, int proto, int nonblocking) {
    drool_socket_pool_t* socket_pool;

    drool_assert(conf);
    if (!conf) {
        return 0;
    }
    /* TODO: check family, type and proto */

    if ((socket_pool = calloc(1, sizeof(drool_socket_pool_t)))) {
        socket_pool->conf = conf;
        socket_pool->family = family;
        socket_pool->type = type;
        socket_pool->proto = proto;
        socket_pool->nonblocking = nonblocking;

        memcpy(&(socket_pool->queue), &socket_pool_sllq_init, sizeof(sllq_t));
        sllq_set_size(&(socket_pool->queue), 0x10);
        if (sllq_init(&(socket_pool->queue)) != SLLQ_OK) {
            free(socket_pool);
            return 0;
        }
    }

    return socket_pool;
}

void socket_pool_free(drool_socket_pool_t* socket_pool) {
    if (socket_pool) {
        sllq_destroy(&(socket_pool->queue));
        free(socket_pool);
    }
}

/*
 * Engine
 */

static void* socket_pool_engine(void* vp) {
    drool_socket_pool_t* socket_pool = (drool_socket_pool_t*)vp;

    drool_assert(socket_pool);
    if (socket_pool) {
        int fd, err;
#if SIZEOF_INT == SIZEOF_VOIDP
        int sock;
#elif SIZEOF_LONG == SIZEOF_VOIDP
        long sock;
#else
#error "fix"
#endif

        log_print(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool run");

        while (1) {
            pthread_testcancel();

            sock = -1;
            if ((fd = socket(socket_pool->family, socket_pool->type, socket_pool->proto)) < 0) {
                log_errno(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool run, socket()");
            }
            else if (socket_pool->nonblocking && fcntl(fd, F_SETFL, O_NONBLOCK)) {
                log_errno(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool run, fcntl()");
                close(fd);
            }
            else {
                sock = fd;
            }

            err = SLLQ_EAGAIN;
            while (err == SLLQ_EAGAIN || err == SLLQ_ETIMEDOUT || err == SLLQ_FULL) {
                struct timespec timeout;

                if (clock_gettime(CLOCK_REALTIME, &timeout)) {
                    log_errno(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool run, clock_gettime()");
                    break;
                }
                timeout.tv_sec++;

                err = sllq_push(&(socket_pool->queue), (void*)sock, &timeout);
            }

            if (err) {
                if (err == SLLQ_ERRNO) {
                    log_errnof(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool run failed %d: ", err);
                }
                else {
                    log_printf(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool run failed %d", err);
                }
                break;
            }

            if (sock < 0)
                break;

            log_printf(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool pushed %ld", (long)sock);
        }

        log_print(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool run exited");
    }

    return 0;
}

/*
 * Start/stop
 */

int socket_pool_start(drool_socket_pool_t* socket_pool) {
    int err;

    drool_assert(socket_pool);
    if (!socket_pool) {
        return 1;
    }
    if (socket_pool->state != SOCKET_POOL_INACTIVE) {
        return 1;
    }

    log_print(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool starting");

    if ((err = pthread_create(&(socket_pool->thread_id), 0, &socket_pool_engine, (void*)socket_pool))) {
        socket_pool->state = SOCKET_POOL_ERROR;
        errno = err;
        return 1;
    }
    socket_pool->state = SOCKET_POOL_RUNNING;

    return 0;
}

int socket_pool_stop(drool_socket_pool_t* socket_pool) {
    int err;

    drool_assert(socket_pool);
    if (!socket_pool) {
        return 1;
    }
    if (socket_pool->state != SOCKET_POOL_RUNNING) {
        return 1;
    }
    drool_assert(socket_pool->ev_loop);

    log_print(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool stopping");

    if ((err = pthread_cancel(socket_pool->thread_id))) {
        socket_pool->state = SOCKET_POOL_ERROR;
        errno = err;
        return 1;
    }

    if ((err = pthread_join(socket_pool->thread_id, 0))) {
        socket_pool->state = SOCKET_POOL_ERROR;
        errno = err;
        return 1;
    }

    socket_pool->state = SOCKET_POOL_STOPPED;

    log_print(conf_log(socket_pool->conf), LNETWORK, LDEBUG, "socket pool stopped");

    return 0;
}

/*
 * Get
 */

inline int socket_pool_family(const drool_socket_pool_t* socket_pool) {
    drool_assert(socket_pool);
    return socket_pool->family;
}

inline int socket_pool_type(const drool_socket_pool_t* socket_pool) {
    drool_assert(socket_pool);
    return socket_pool->type;
}

inline int socket_pool_proto(const drool_socket_pool_t* socket_pool) {
    drool_assert(socket_pool);
    return socket_pool->proto;
}

inline int socket_pool_nonblocking(const drool_socket_pool_t* socket_pool) {
    drool_assert(socket_pool);
    return socket_pool->nonblocking;
}

/*
 * Get socket
 */

int socket_pool_getsock(drool_socket_pool_t* socket_pool, int* fd) {
    int err;
#if SIZEOF_INT == SIZEOF_VOIDP
    int sock;
#elif SIZEOF_LONG == SIZEOF_VOIDP
    long sock;
#else
#error "fix"
#endif

    drool_assert(socket_pool);
    if (!socket_pool) {
        return 1;
    }
    drool_assert(fd);
    if (!fd) {
        return 1;
    }
    if (socket_pool->state != SOCKET_POOL_RUNNING) {
        return 1;
    }

    err = SLLQ_EAGAIN;
    while (err == SLLQ_EAGAIN || err == SLLQ_ETIMEDOUT || err == SLLQ_FULL) {
        struct timespec timeout;

        if (clock_gettime(CLOCK_REALTIME, &timeout)) {
            return 1;
        }
        timeout.tv_sec++;

        err = sllq_shift(&(socket_pool->queue), (void**)&sock, &timeout);
    }

    if (err) {
        if (err == SLLQ_ERRNO) {
            log_errnof(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool get failed %d: ", err);
        }
        else {
            log_printf(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool get failed %d", err);
        }
        return 1;
    }

    if (sock < 0) {
        log_printf(conf_log(socket_pool->conf), LNETWORK, LERROR, "socket pool got error %ld", sock);
        socket_pool->state = SOCKET_POOL_ERROR;
        return 1;
    }

    *fd = sock;

    return 0;
}
