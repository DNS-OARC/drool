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

#include "log.h"
#include "timing.h"

#ifndef __drool_conf_h
#define __drool_conf_h

#include <sys/types.h>

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

/*
 * conf file struct and functions
 */

#define CONF_FILE_T_INIT { 0, 0 }
typedef struct drool_conf_file drool_conf_file_t;
struct drool_conf_file {
    drool_conf_file_t*  next;
    char*               name;
};

drool_conf_file_t* conf_file_new(void);
void conf_file_free(drool_conf_file_t* conf_file);
void conf_file_release(drool_conf_file_t* conf_file);
const drool_conf_file_t* conf_file_next(const drool_conf_file_t* conf_file);
int conf_file_set_next(drool_conf_file_t* conf_file, drool_conf_file_t* next);
const char* conf_file_name(const drool_conf_file_t* conf_file);
int conf_file_set_name(drool_conf_file_t* conf_file, const char* name, size_t length);

/*
 * conf interface struct and functions
 */

#define CONF_INTERFACE_T_INIT { 0, 0 }
typedef struct drool_conf_interface drool_conf_interface_t;
struct drool_conf_interface {
    drool_conf_interface_t* next;
    char*                   name;
};

drool_conf_interface_t* conf_interface_new(void);
void conf_interface_free(drool_conf_interface_t* conf_interface);
void conf_interface_release(drool_conf_interface_t* conf_interface);
const drool_conf_interface_t* conf_interface_next(const drool_conf_interface_t* conf_interface);
int conf_interface_set_next(drool_conf_interface_t* conf_interface, drool_conf_interface_t* next);
const char* conf_interface_name(const drool_conf_interface_t* conf_interface);
int conf_interface_set_name(drool_conf_interface_t* conf_interface, const char* name, size_t length);

/*
 * conf client_pool struct and functions
 */

#define CONF_CLIENT_POOL_T_INIT { \
    0, \
    0, 0, 0, \
    0, 0, 0, 0 \
}
typedef struct drool_conf_client_pool drool_conf_client_pool_t;
struct drool_conf_client_pool {
    drool_conf_client_pool_t*   next;

    unsigned short  have_target : 1;
    unsigned short  have_max_clients : 1;
    unsigned short  have_client_ttl : 1;

    char*           target_host;
    char*           target_service;
    size_t          max_clients;
    double          client_ttl;
};

drool_conf_client_pool_t* conf_client_pool_new(void);
void conf_client_pool_free(drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_target(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_max_clients(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_have_client_ttl(const drool_conf_client_pool_t* conf_client_pool);
const drool_conf_client_pool_t* conf_client_pool_next(const drool_conf_client_pool_t* conf_client_pool);
const char* conf_client_pool_target_host(const drool_conf_client_pool_t* conf_client_pool);
const char* conf_client_pool_target_service(const drool_conf_client_pool_t* conf_client_pool);
size_t conf_client_pool_max_clients(const drool_conf_client_pool_t* conf_client_pool);
double conf_client_pool_client_ttl(const drool_conf_client_pool_t* conf_client_pool);
int conf_client_pool_set_next(drool_conf_client_pool_t* conf_client_pool, drool_conf_client_pool_t* next);
int conf_client_pool_set_target(drool_conf_client_pool_t* conf_client_pool, const char* host, size_t host_length, const char* service, size_t service_length);
int conf_client_pool_set_max_clients(drool_conf_client_pool_t* conf_client_pool, size_t max_clients);
int conf_client_pool_set_client_ttl(drool_conf_client_pool_t* conf_client_pool, double client_ttl);

/*
 * conf struct and functions
 */

#define CONF_T_INIT { \
    0, 0, 0, 0, 0, \
    0, 0, \
    0, 0, \
    CONF_FILE_T_INIT, CONF_INTERFACE_T_INIT, \
    LOG_T_INIT, \
    TIMING_MODE_KEEP, 0, 0, 0.0, \
    CONF_CLIENT_POOL_T_INIT \
}
typedef struct drool_conf drool_conf_t;
struct drool_conf {
    unsigned short  have_filter : 1;
    unsigned short  have_read : 1;
    unsigned short  have_input : 1;
    unsigned short  have_write : 1;
    unsigned short  have_output : 1;

    char*                   filter;
    size_t                  filter_length;

    drool_conf_file_t*      read;
    drool_conf_interface_t* input;

    drool_conf_file_t       write;
    drool_conf_interface_t  output;

    drool_log_t             log;

    drool_timing_mode_t     timing_mode;
    unsigned long int       timing_increase;
    unsigned long int       timing_reduce;
    long double             timing_multiply;

    drool_conf_client_pool_t    client_pool;
};

drool_conf_t* conf_new(void);
void conf_free(drool_conf_t* conf);
void conf_release(drool_conf_t* conf);
int conf_have_filter(const drool_conf_t* conf);
int conf_have_read(const drool_conf_t* conf);
int conf_have_input(const drool_conf_t* conf);
int conf_have_write(const drool_conf_t* conf);
int conf_have_output(const drool_conf_t* conf);
const char* conf_filter(const drool_conf_t* conf);
int conf_set_filter(drool_conf_t* conf, const char* filter, size_t length);
const size_t conf_filter_length(const drool_conf_t* conf);
const drool_conf_file_t* conf_read(const drool_conf_t* conf);
const drool_conf_interface_t* conf_input(const drool_conf_t* conf);
const drool_conf_file_t* conf_write(const drool_conf_t* conf);
const drool_conf_interface_t* conf_output(const drool_conf_t* conf);
drool_timing_mode_t conf_timing_mode(const drool_conf_t* conf);
unsigned long int conf_timing_increase(const drool_conf_t* conf);
unsigned long int conf_timing_reduce(const drool_conf_t* conf);
long double conf_timing_multiply(const drool_conf_t* conf);
int conf_add_read(drool_conf_t* conf, const char* file, size_t length);
int conf_add_input(drool_conf_t* conf, const char* interface, size_t length);
int conf_set_write(drool_conf_t* conf, const char* file, size_t length);
int conf_set_output(drool_conf_t* conf, const char* interface, size_t length);
const drool_log_t* conf_log(const drool_conf_t* conf);
drool_log_t* conf_log_rw(drool_conf_t* conf);
const drool_conf_client_pool_t* conf_client_pool(const drool_conf_t* conf);

int conf_parse_file(drool_conf_t* conf, const char* file);
int conf_parse_text(drool_conf_t* conf, const char* text, const size_t length);

const char* conf_strerr(int errnum);

/*
 * conf token structs
 */

typedef enum conf_token_type conf_token_type_t;
enum conf_token_type {
    TOKEN_END = 0,
    TOKEN_STRING,
    TOKEN_QSTRING,
    TOKEN_NUMBER,
    TOKEN_STRINGS,
    TOKEN_QSTRINGS,
    TOKEN_NUMBERS,
    TOKEN_ANY,
    TOKEN_FLOAT,
    TOKEN_FLOATS,
    TOKEN_NESTED
};

typedef struct conf_token conf_token_t;
struct conf_token {
    conf_token_type_t           type;
    const char*                 token;
    size_t                      length;
};

typedef int (*conf_token_callback_t)(drool_conf_t* conf, const conf_token_t* tokens, const char** errstr);

#define CONF_SYNTAX_T_TOKENS 8

typedef struct conf_syntax conf_syntax_t;
struct conf_syntax {
    const char*                 token;
    conf_token_callback_t       callback;
    const conf_token_type_t     syntax[CONF_SYNTAX_T_TOKENS];
    const conf_syntax_t*        nested;
};

#endif /* __drool_conf_h */
