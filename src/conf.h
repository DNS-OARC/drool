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

#ifndef __drool_conf_h
#define __drool_conf_h

#include "log.h"
#include "timing.h"
#include "conf_file.h"
#include "conf_interface.h"
#include "conf_client_pool.h"

#include <stddef.h>

#define CONF_EEXIST     -4
#define CONF_ENOMEM     -3
#define CONF_EINVAL     -2
#define CONF_ERROR      -1
#define CONF_OK         0
#define CONF_LAST       1
#define CONF_COMMENT    2
#define CONF_EMPTY      3

#define CONF_EEXIST_STR "Already exists"
#define CONF_ENOMEM_STR "Out of memory"
#define CONF_EINVAL_STR "Invalid arguments"
#define CONF_ERROR_STR  "Generic error"

typedef enum drool_conf_read_mode drool_conf_read_mode_t;
enum drool_conf_read_mode {
    CONF_READ_MODE_NONE = 0,
    CONF_READ_MODE_LOOP,
    CONF_READ_MODE_ITER
};

#define CONF_T_INIT { \
    0, 0, 0, 0, /*0, 0,*/ 0, \
    0, 0, \
    0, 0, \
    0, 0, \
    /* CONF_FILE_T_INIT, CONF_INTERFACE_T_INIT,*/ \
    LOG_T_INIT, \
    TIMING_MODE_KEEP, 0, 0, 0.0, \
    CONF_CLIENT_POOL_T_INIT, \
    1 \
}
typedef struct drool_conf drool_conf_t;
struct drool_conf {
    unsigned short  have_filter : 1;
    unsigned short  have_read : 1;
    unsigned short  have_input : 1;
    unsigned short  have_read_mode : 1;
    /*
    unsigned short  have_write : 1;
    unsigned short  have_output : 1;
    */
    unsigned short  is_dry_run : 1;

    char*                   filter;
    size_t                  filter_length;

    drool_conf_file_t*      read;
    drool_conf_interface_t* input;

    drool_conf_read_mode_t  read_mode;
    size_t                  read_iter;

    /*
    drool_conf_file_t       write;
    drool_conf_interface_t  output;
    */

    drool_log_t             log;

    drool_timing_mode_t     timing_mode;
    unsigned long int       timing_increase;
    unsigned long int       timing_reduce;
    long double             timing_multiply;

    drool_conf_client_pool_t    client_pool;

    size_t                  context_client_pools;
};

drool_conf_t* conf_new(void);
void conf_free(drool_conf_t* conf);
void conf_release(drool_conf_t* conf);
int conf_have_filter(const drool_conf_t* conf);
int conf_have_read(const drool_conf_t* conf);
int conf_have_input(const drool_conf_t* conf);
int conf_have_read_mode(const drool_conf_t* conf);
/*
int conf_have_write(const drool_conf_t* conf);
int conf_have_output(const drool_conf_t* conf);
*/
int conf_is_dry_run(const drool_conf_t* conf);
const char* conf_filter(const drool_conf_t* conf);
int conf_set_filter(drool_conf_t* conf, const char* filter, size_t length);
size_t conf_filter_length(const drool_conf_t* conf);
const drool_conf_file_t* conf_read(const drool_conf_t* conf);
const drool_conf_interface_t* conf_input(const drool_conf_t* conf);
drool_conf_read_mode_t conf_read_mode(const drool_conf_t* conf);
size_t conf_read_iter(const drool_conf_t* conf);
/*
const drool_conf_file_t* conf_write(const drool_conf_t* conf);
const drool_conf_interface_t* conf_output(const drool_conf_t* conf);
*/
drool_timing_mode_t conf_timing_mode(const drool_conf_t* conf);
unsigned long int conf_timing_increase(const drool_conf_t* conf);
unsigned long int conf_timing_reduce(const drool_conf_t* conf);
long double conf_timing_multiply(const drool_conf_t* conf);
size_t conf_context_client_pools(const drool_conf_t* conf);
int conf_add_read(drool_conf_t* conf, const char* file, size_t length);
int conf_add_input(drool_conf_t* conf, const char* interface, size_t length);
int conf_set_read_mode(drool_conf_t* conf, drool_conf_read_mode_t read_mode);
int conf_set_read_iter(drool_conf_t* conf, size_t read_iter);
/*
int conf_set_write(drool_conf_t* conf, const char* file, size_t length);
int conf_set_output(drool_conf_t* conf, const char* interface, size_t length);
*/
int conf_set_dry_run(drool_conf_t* conf, int dry_run);
int conf_set_context_client_pools(drool_conf_t* conf, size_t context_client_pools);
const drool_log_t* conf_log(const drool_conf_t* conf);
drool_log_t* conf_log_rw(drool_conf_t* conf);
const drool_conf_client_pool_t* conf_client_pool(const drool_conf_t* conf);

int conf_parse_file(drool_conf_t* conf, const char* file);
int conf_parse_text(drool_conf_t* conf, const char* text, const size_t length);

const char* conf_strerr(int errnum);

#endif /* __drool_conf_h */
