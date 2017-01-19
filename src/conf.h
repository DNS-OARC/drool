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

/*
 * conf file struct and functions
 */

#define CONF_FILE_T_INIT { 0, 0 }
typedef struct conf_file conf_file_t;
struct conf_file {
    conf_file_t*        next;
    char*               name;
};

conf_file_t* conf_file_new(void);
void conf_file_free(conf_file_t* conf_file);
void conf_file_release(conf_file_t* conf_file);
conf_file_t* conf_file_next(const conf_file_t* conf_file);
int conf_file_set_next(conf_file_t* conf_file, conf_file_t* next);
const char* conf_file_name(const conf_file_t* conf_file);
int conf_file_set_name(conf_file_t* conf_file, const char* name);

/*
 * conf interface struct and functions
 */

#define CONF_INTERFACE_T_INIT { 0, 0 }
typedef struct conf_interface conf_interface_t;
struct conf_interface {
    conf_interface_t*   next;
    char*               name;
};

conf_interface_t* conf_interface_new(void);
void conf_interface_free(conf_interface_t* conf_interface);
void conf_interface_release(conf_interface_t* conf_interface);
conf_interface_t* conf_interface_next(const conf_interface_t* conf_interface);
int conf_interface_set_next(conf_interface_t* conf_interface, conf_interface_t* next);
const char* conf_interface_name(const conf_interface_t* conf_interface);
int conf_interface_set_name(conf_interface_t* conf_interface, const char* name);

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
    TOKEN_ANY
};

typedef struct conf_token conf_token_t;
struct conf_token {
    conf_token_type_t           type;
    const char*                 token;
    size_t                      length;
};

typedef int (*conf_token_callback_t)(const conf_token_t* token);

typedef struct conf_syntax conf_syntax_t;
struct conf_syntax {
    const char*                 token;
    conf_token_callback_t       callback;
    const conf_token_type_t     syntax[8];
};

/*
 * conf struct and functions
 */

#define CONF_T_INIT { \
    0, 0, 0, 0, 0, \
    0, \
    0, 0, \
    CONF_FILE_T_INIT, CONF_INTERFACE_T_INIT, \
    LOG_T_INIT \
}
typedef struct conf conf_t;
struct conf {
    unsigned short      have_filter : 1;
    unsigned short      have_read : 1;
    unsigned short      have_input : 1;
    unsigned short      have_write : 1;
    unsigned short      have_output : 1;

    char*               filter;

    conf_file_t*        read;
    conf_interface_t*   input;

    conf_file_t         write;
    conf_interface_t    output;

    log_t               log;
};

conf_t* conf_new(void);
void conf_free(conf_t* conf);
void conf_release(conf_t* conf);
int conf_have_filter(const conf_t* conf);
int conf_have_read(const conf_t* conf);
int conf_have_input(const conf_t* conf);
int conf_have_write(const conf_t* conf);
int conf_have_output(const conf_t* conf);
const char* conf_filter(const conf_t* conf);
int conf_set_filter(conf_t* conf, const char* filter);
const conf_file_t* conf_read(const conf_t* conf);
const conf_interface_t* conf_input(const conf_t* conf);
const conf_file_t* conf_write(const conf_t* conf);
const conf_interface_t* conf_output(const conf_t* conf);
int conf_add_read(conf_t* conf, const char* file);
int conf_add_input(conf_t* conf, const char* interface);
int conf_set_write(conf_t* conf, const char* file);
int conf_set_output(conf_t* conf, const char* interface);
log_t* conf_log(conf_t* conf);

int conf_parse_file(conf_t* conf, const char* file);
int conf_parse_text(conf_t* conf, const char* text);

#endif /* __drool_conf_h */
