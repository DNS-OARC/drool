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

#include "conf.h"
#include "drool.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * conf file
 */

conf_file_t* conf_file_new(void) {
    conf_file_t* conf_file = calloc(1, sizeof(conf_file_t));
    return conf_file;
}

void conf_file_free(conf_file_t* conf_file) {
    if (conf_file) {
        conf_file_release(conf_file);
        free(conf_file);
    }
}

void conf_file_release(conf_file_t* conf_file) {
    if (conf_file) {
        if (conf_file->name) {
            free(conf_file->name);
            conf_file->name = 0;
        }
    }
}

inline const conf_file_t* conf_file_next(const conf_file_t* conf_file) {
    drool_assert(conf_file);
    return conf_file->next;
}

int conf_file_set_next(conf_file_t* conf_file, conf_file_t* next) {
    if (!conf_file) {
        return CONF_EINVAL;
    }
    if (!next) {
        return CONF_EINVAL;
    }

    conf_file->next = next;

    return CONF_OK;
}

inline const char* conf_file_name(const conf_file_t* conf_file) {
    drool_assert(conf_file);
    return conf_file->name;
}

int conf_file_set_name(conf_file_t* conf_file, const char* name, size_t length) {
    if (!conf_file) {
        return CONF_EINVAL;
    }
    if (!name) {
        return CONF_EINVAL;
    }

    if (conf_file->name) {
        free(conf_file->name);
    }
    if (length) {
        if (!(conf_file->name = strndup(name, length))) {
            return CONF_ENOMEM;
        }
    }
    else {
        if (!(conf_file->name = strdup(name))) {
            return CONF_ENOMEM;
        }
    }
    printf("name: %s\n", conf_file->name);

    return CONF_OK;
}

/*
 * conf interface
 */

conf_interface_t* conf_interface_new(void) {
    conf_interface_t* conf_interface = calloc(1, sizeof(conf_interface_t));
    return conf_interface;
}

void conf_interface_free(conf_interface_t* conf_interface) {
    if (conf_interface) {
        conf_interface_release(conf_interface);
        free(conf_interface);
    }
}

void conf_interface_release(conf_interface_t* conf_interface) {
    if (conf_interface) {
        if (conf_interface->name) {
            free(conf_interface->name);
            conf_interface->name = 0;
        }
    }
}

inline const conf_interface_t* conf_interface_next(const conf_interface_t* conf_interface) {
    drool_assert(conf_interface);
    return conf_interface->next;
}

int conf_interface_set_next(conf_interface_t* conf_interface, conf_interface_t* next) {
    if (!conf_interface) {
        return CONF_EINVAL;
    }
    if (!next) {
        return CONF_EINVAL;
    }

    conf_interface->next = next;

    return CONF_OK;
}

inline const char* conf_interface_name(const conf_interface_t* conf_interface) {
    drool_assert(conf_interface);
    return conf_interface->name;
}

int conf_interface_set_name(conf_interface_t* conf_interface, const char* name, size_t length) {
    if (!conf_interface) {
        return CONF_EINVAL;
    }
    if (!name) {
        return CONF_EINVAL;
    }

    if (conf_interface->name) {
        free(conf_interface->name);
    }
    if (length) {
        if (!(conf_interface->name = strndup(name, length))) {
            return CONF_ENOMEM;
        }
    }
    else {
        if (!(conf_interface->name = strdup(name))) {
            return CONF_ENOMEM;
        }
    }

    return CONF_OK;
}

/*
 * conf
 */

conf_t* conf_new(void) {
    conf_t* conf = calloc(1, sizeof(conf_t));
    return conf;
}

void conf_free(conf_t* conf) {
    if (conf) {
        conf_release(conf);
        free(conf);
    }
}

void conf_release(conf_t* conf) {
    if (conf) {
        if (conf->have_filter) {
            free(conf->filter);
            conf->filter = 0;
        }
        if (conf->have_read) {
            while (conf->read) {
                conf_file_t* conf_file = conf->read;

                conf->read = conf_file->next;
                conf_file_free(conf_file);
            }
            conf->have_read = 0;
        }
        if (conf->have_input) {
            while (conf->input) {
                conf_interface_t* conf_interface = conf->input;

                conf->input = conf_interface->next;
                conf_interface_free(conf_interface);
            }
            conf->have_input = 0;
        }
        if (conf->have_write) {
            conf_file_release(&(conf->write));
            conf->have_write = 0;
        }
        if (conf->have_output) {
            conf_interface_release(&(conf->output));
            conf->have_output = 0;
        }
    }
}

inline int conf_have_filter(const conf_t* conf) {
    drool_assert(conf);
    return conf->have_filter;
}

inline int conf_have_read(const conf_t* conf) {
    drool_assert(conf);
    return conf->have_read;
}

inline int conf_have_input(const conf_t* conf) {
    drool_assert(conf);
    return conf->have_input;
}

inline int conf_have_write(const conf_t* conf) {
    drool_assert(conf);
    return conf->have_write;
}

inline int conf_have_output(const conf_t* conf) {
    drool_assert(conf);
    return conf->have_output;
}

inline const char* conf_filter(const conf_t* conf) {
    drool_assert(conf);
    return conf->filter;
}

int conf_set_filter(conf_t* conf, const char* filter, size_t length) {
    if (!conf) {
        return CONF_EINVAL;
    }
    if (!filter) {
        return CONF_EINVAL;
    }

    if (conf->filter) {
        free(conf->filter);
    }
    if (length) {
        if (!(conf->filter = strndup(filter, length))) {
            return CONF_ENOMEM;
        }
        conf->filter_length = length;
    }
    else {
        if (!(conf->filter = strdup(filter))) {
            return CONF_ENOMEM;
        }
        conf->filter_length = strlen(conf->filter);
    }
    conf->have_filter = 1;

    return CONF_OK;
}

inline const size_t conf_filter_length(const conf_t* conf) {
    drool_assert(conf);
    return conf->filter_length;
}

inline const conf_file_t* conf_read(const conf_t* conf) {
    drool_assert(conf);
    return conf->read;
}

inline const conf_interface_t* conf_input(const conf_t* conf) {
    drool_assert(conf);
    return conf->input;
}

inline const conf_file_t* conf_write(const conf_t* conf) {
    drool_assert(conf);
    return &(conf->write);
}

inline const conf_interface_t* conf_output(const conf_t* conf) {
    drool_assert(conf);
    return &(conf->output);
}

int conf_add_read(conf_t* conf, const char* file, size_t length) {
    conf_file_t* conf_file;
    int err = CONF_OK;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!file) {
        return CONF_EINVAL;
    }

    if (!(conf_file = conf_file_new())) {
        return CONF_ENOMEM;
    }
    if (err == CONF_OK)
        err = conf_file_set_name(conf_file, file, length);
    if (err == CONF_OK && conf->read)
        err = conf_file_set_next(conf_file, conf->read);
    if (err == CONF_OK) {
        conf->read = conf_file;
        conf->have_read = 1;
    }
    else
        conf_file_free(conf_file);

    return err;
}

int conf_add_input(conf_t* conf, const char* interface, size_t length) {
    conf_interface_t* conf_interface;
    int err = CONF_OK;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!interface) {
        return CONF_EINVAL;
    }

    if (!(conf_interface = conf_interface_new())) {
        return CONF_ENOMEM;
    }
    if (err == CONF_OK)
        err = conf_interface_set_name(conf_interface, interface, length);
    if (err == CONF_OK && conf->input)
        err = conf_interface_set_next(conf_interface, conf->input);
    if (err == CONF_OK) {
        conf->input = conf_interface;
        conf->have_input = 1;
    }
    else
        conf_interface_free(conf_interface);

    return CONF_OK;
}

int conf_set_write(conf_t* conf, const char* file, size_t length) {
    int ret;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!file) {
        return CONF_EINVAL;
    }
    if (conf->have_write) {
        return CONF_EEXIST;
    }
    if (conf->have_output) {
        return CONF_EEXIST;
    }

    if ((ret = conf_file_set_name(&(conf->write), file, length)) == CONF_OK) {
        conf->have_write = 1;
    }
    return ret;
}

int conf_set_output(conf_t* conf, const char* interface, size_t length) {
    int ret;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!interface) {
        return CONF_EINVAL;
    }
    if (conf->have_write) {
        return CONF_EEXIST;
    }
    if (conf->have_output) {
        return CONF_EEXIST;
    }

    if ((ret = conf_interface_set_name(&(conf->output), interface, length)) == CONF_OK) {
        conf->have_output = 1;
    }
    return ret;
}

inline const log_t* conf_log(const conf_t* conf) {
    drool_assert(conf);
    return &(conf->log);
}

inline log_t* conf_log_rw(conf_t* conf) {
    drool_assert(conf);
    return &(conf->log);
}

/*
 * conf parsers
 */

static int parse_log(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    log_facility_t facility = LOG_FACILITY_NONE;
    log_level_t level = LOG_LEVEL_ALL;
    int all = 0;

    if (!strncmp(tokens[1].token, "core", tokens[1].length)) {
        facility = LOG_FACILITY_CORE;
    }
    else if (!strncmp(tokens[1].token, "network", tokens[1].length)) {
        facility = LOG_FACILITY_NETWORK;
    }
    else if (!strncmp(tokens[1].token, "all", tokens[1].length)) {
        all = 1;
    }
    else {
        *errstr = "Invalid log facility";
        return 1;
    }

    if (tokens[2].type == TOKEN_STRING) {
        if (!strncmp(tokens[2].token, "debug", tokens[2].length)) {
            level = LOG_LEVEL_DEBUG;
        }
        else if (!strncmp(tokens[2].token, "info", tokens[2].length)) {
            level = LOG_LEVEL_INFO;
        }
        else if (!strncmp(tokens[2].token, "notice", tokens[2].length)) {
            level = LOG_LEVEL_NOTICE;
        }
        else if (!strncmp(tokens[2].token, "warning", tokens[2].length)) {
            level = LOG_LEVEL_WARNING;
        }
        else if (!strncmp(tokens[2].token, "error", tokens[2].length)) {
            level = LOG_LEVEL_ERROR;;
        }
        else if (!strncmp(tokens[2].token, "critical", tokens[2].length)) {
            level = LOG_LEVEL_CRITICAL;
        }
        else {
            *errstr = "Invalid log level";
            return 1;
        }
    }

    if (all) {
        if (log_enable(conf_log_rw(conf), LOG_FACILITY_CORE, level)
            || log_enable(conf_log_rw(conf), LOG_FACILITY_NETWORK, level))
        {
            *errstr = "Unable to enable log facility (and level)";
            return 1;
        }
    }
    else {
        if (log_enable(conf_log_rw(conf), facility, level)) {
            *errstr = "Unable to enable log facility (and level)";
            return 1;
        }
    }

    return 0;
}

static int parse_nolog(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    log_facility_t facility = LOG_FACILITY_NONE;
    log_level_t level = LOG_LEVEL_ALL;
    int all = 0;

    if (!strncmp(tokens[1].token, "core", tokens[1].length)) {
        facility = LOG_FACILITY_CORE;
    }
    else if (!strncmp(tokens[1].token, "network", tokens[1].length)) {
        facility = LOG_FACILITY_NETWORK;
    }
    else if (!strncmp(tokens[1].token, "all", tokens[1].length)) {
        all = 1;
    }
    else {
        *errstr = "Invalid log facility";
        return 1;
    }

    if (tokens[2].type == TOKEN_STRING) {
        if (!strncmp(tokens[2].token, "debug", tokens[2].length)) {
            level = LOG_LEVEL_DEBUG;
        }
        else if (!strncmp(tokens[2].token, "info", tokens[2].length)) {
            level = LOG_LEVEL_INFO;
        }
        else if (!strncmp(tokens[2].token, "notice", tokens[2].length)) {
            level = LOG_LEVEL_NOTICE;
        }
        else if (!strncmp(tokens[2].token, "warning", tokens[2].length)) {
            level = LOG_LEVEL_WARNING;
        }
        else if (!strncmp(tokens[2].token, "error", tokens[2].length)) {
            level = LOG_LEVEL_ERROR;;
        }
        else if (!strncmp(tokens[2].token, "critical", tokens[2].length)) {
            level = LOG_LEVEL_CRITICAL;
        }
        else {
            *errstr = "Invalid log level";
            return 1;
        }
    }

    if (all) {
        if (log_disable(conf_log_rw(conf), LOG_FACILITY_CORE, level)
            || log_disable(conf_log_rw(conf), LOG_FACILITY_NETWORK, level))
        {
            *errstr = "Unable to disable log facility (and level)";
            return 1;
        }
    }
    else {
        if (log_disable(conf_log_rw(conf), facility, level)) {
            *errstr = "Unable to disable log facility (and level)";
            return 1;
        }
    }

    return 0;
}

static int parse_read(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    int err;

    if ((err = conf_add_read(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static int parse_input(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    int err;

    if ((err = conf_add_input(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static int parse_filter(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    int err;

    if ((err = conf_set_filter(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }
    return 0;
}

static int parse_write(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    int err;

    if ((err = conf_set_write(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static int parse_output(conf_t* conf, const conf_token_t* tokens, const char** errstr) {
    int err;

    if ((err = conf_set_output(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static conf_syntax_t _syntax[] = {
    /* log <facility> [level] ; */
    {
        "log",
        parse_log,
        { TOKEN_STRINGS, TOKEN_END }
    },
    /* nolog <facility> [level] ; */
    {
        "nolog",
        parse_nolog,
        { TOKEN_STRINGS, TOKEN_END }
    },
    /* read " <file.pcap> " ; */
    {
        "read",
        parse_read,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* input " <interface> " ; */
    {
        "input",
        parse_input,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* filter " <filter> " ; */
    {
        "filter",
        parse_filter,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* write " <file.pcap> " ; */
    {
        "write",
        parse_write,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* output " <interface> " ; */
    {
        "output",
        parse_output,
        { TOKEN_QSTRING, TOKEN_END }
    },
    { 0, 0, { TOKEN_END } }
};

static int parse_token(const char** conf, size_t* length, conf_token_t* token) {
    int quoted = 0, end = 0;

    if (!conf || !*conf || !length || !token) {
        return CONF_EINVAL;
    }
    if (!*length) {
        return CONF_ERROR;
    }
    if (**conf == ' ' || **conf == '\t' || **conf == ';' || !**conf || **conf == '\n' || **conf == '\r') {
        return CONF_ERROR;
    }
    if (**conf == '#') {
        return CONF_COMMENT;
    }

    if (**conf == '"') {
        quoted = 1;
        (*conf)++;
        (*length)--;
        token->type = TOKEN_QSTRING;
    }
    else {
        token->type = TOKEN_NUMBER;
    }

    token->token = *conf;
    token->length = 0;

    for (; **conf && length; (*conf)++, (*length)--) {
        if (quoted && **conf == '"') {
            end = 1;
            continue;
        }
        else if ((!quoted || end) && (**conf == ' ' || **conf == '\t' || **conf == ';')) {
            if (**conf == ';') {
                (*conf)++;
                (*length)--;
                return CONF_LAST;
            }
            (*conf)++;
            (*length)--;
            return CONF_OK;
        }
        else if (end || **conf == '\n' || **conf == '\r' || !**conf) {
            return CONF_ERROR;
        }

        if ((**conf < '0' || **conf > '9') && token->type != TOKEN_QSTRING) {
            token->type = TOKEN_STRING;
        }

        token->length++;
    }

    return CONF_ERROR;
}

static int parse_tokens(conf_t* conf, const conf_token_t* tokens, size_t token_size, size_t line) {
    const conf_syntax_t*        syntax;
    const conf_token_type_t*    type;
    size_t i;

    if (!tokens || !token_size) {
        log_printf(conf_log(conf), LCORE, LERROR, "Internal error in config at line %lu", line);
        return CONF_ERROR;
    }

    if (tokens[0].type != TOKEN_STRING) {
        log_printf(conf_log(conf), LCORE, LERROR, "Wrong first config token at line %lu, expected a string", line);
        return CONF_ERROR;
    }

    for (syntax = _syntax; syntax->token; syntax++) {
        if (!strncmp(tokens[0].token, syntax->token, tokens[0].length)) {
            break;
        }
    }
    if (!syntax->token) {
        log_printf(conf_log(conf), LCORE, LERROR, "Unknown config option %.*s at line %lu", (int)tokens[0].length, tokens[0].token, line);
        return CONF_ERROR;
    }

    for (type = syntax->syntax, i = 1; *type != TOKEN_END && i < token_size; i++) {
        if (*type == TOKEN_STRINGS) {
            if (tokens[i].type != TOKEN_STRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Wrong config token for argument %lu at line %lu, expected a string", i, line);
                return CONF_ERROR;
            }
            continue;
        }
        if (*type == TOKEN_QSTRINGS) {
            if (tokens[i].type != TOKEN_QSTRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Wrong config token for argument %lu at line %lu, expected a quoted string", i, line);
                return CONF_ERROR;
            }
            continue;
        }
        if (*type == TOKEN_NUMBERS) {
            if (tokens[i].type != TOKEN_NUMBER) {
                log_printf(conf_log(conf), LCORE, LERROR, "Wrong config token for argument %lu at line %lu, expected a number", i, line);
                return CONF_ERROR;
            }
            continue;
        }
        if (*type == TOKEN_ANY) {
            if (tokens[i].type != TOKEN_STRING && tokens[i].type != TOKEN_NUMBER && tokens[i].type != TOKEN_QSTRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Wrong config token for argument %lu at line %lu, expected a string or number", i, line);
                return CONF_ERROR;
            }
            continue;
        }

        if (tokens[i].type != *type) {
            log_printf(conf_log(conf), LCORE, LERROR, "Wrong config token for argument %lu at line %lu%s", i, line,
                *type == TOKEN_STRING ? ", expected a string"
                    : *type == TOKEN_NUMBER ? ", expected a number"
                        : *type == TOKEN_QSTRING ? ", expected a quoted string"
                            : ""
            );
            return CONF_ERROR;
        }
        type++;
    }

    if (syntax->callback) {
        int ret;
        const char* errstr = "Syntax error or invalid arguments";

        log_printf(conf_log(conf), LCORE, LDEBUG, "Calling config callback for %.*s at line %lu", (int)tokens[0].length, tokens[0].token, line);
        ret = syntax->callback(conf, tokens, &errstr);

        if (ret < 0) {
            log_errnof(conf_log(conf), LCORE, LERROR, "Config error at line %lu: %s", line, errstr);
        }
        if (ret > 0) {
            log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu for %.*s: %s", line, (int)tokens[0].length, tokens[0].token, errstr);
        }
        return ret ? CONF_ERROR : CONF_OK;
    }

    return CONF_OK;
}

int conf_parse_file(conf_t* conf, const char* file) {
    FILE* fp;
    char* buffer = 0;
    size_t bufsize = 0;
    const char* buf;
    size_t s, i, line = 0;
    conf_token_t tokens[8];
    int ret, ret2;
    size_t loop = 10;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!file) {
        return CONF_EINVAL;
    }

    log_printf(conf_log(conf), LCORE, LDEBUG, "Opening config file %s", file);
    if (!(fp = fopen(file, "r"))) {
        return CONF_ERROR;
    }
    ret2 = getline(&buffer, &bufsize, fp);
    buf = buffer;
    s = bufsize;
    line++;
    while (ret2 > 0) {
        memset(tokens, 0, sizeof(tokens));
        /*
         * Go to the first non white-space character
         */
        for (ret = CONF_OK; *buf && s; buf++, s--) {
            if (*buf != ' ' && *buf != '\t') {
                if (*buf == '\n' || *buf == '\r') {
                    ret = CONF_EMPTY;
                }
                break;
            }
        }
        /*
         * Parse all the tokens
         */
        for (i = 0; i < 8 && ret == CONF_OK; i++) {
            ret = parse_token(&buf, &s, &tokens[i]);
        }

        if (ret == CONF_COMMENT) {
            /*
             * Line ended with comment, reduce the number of tokens
             */
            i--;
        }
        else if (ret == CONF_EMPTY) {
            i = 0;
        }
        else if (ret == CONF_OK) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu for %.*s, too many arguments", line, (int)tokens[0].length, tokens[0].token);
            }
            else {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu, too many arguments", line);
            }
            free(buffer);
            fclose(fp);
            return CONF_ERROR;
        }
        else if (ret != CONF_LAST) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu for %.*s, invalid syntax", line, (int)tokens[0].length, tokens[0].token);
            }
            else {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu, invalid syntax", line);
            }
            free(buffer);
            fclose(fp);
            return CONF_ERROR;
        }

        if (i) {
            /*
             * Config using the tokens
             */
            if (parse_tokens(conf, tokens, i, line) != CONF_OK) {
                free(buffer);
                fclose(fp);
                return CONF_ERROR;
            }
        }

        if (ret == CONF_COMMENT || !s || !*buf || *buf == '\n' || *buf == '\r') {
            ret2 = getline(&buffer, &bufsize, fp);
            buf = buffer;
            s = bufsize;
            line++;
        }
    }
    if (ret2 < 0) {
        long pos;
        char errbuf[512];

        pos = ftell(fp);
        if (fseek(fp, 0, SEEK_END)) {
            log_errnof(conf_log(conf), LCORE, LERROR, "Config error at line %lu, fseek()", line);
        }
        else if (ftell(fp) < pos) {
            log_errnof(conf_log(conf), LCORE, LERROR, "Config error at line %lu, ftell()", line);
        }
    }
    free(buffer);
    fclose(fp);

    return CONF_OK;
}

int conf_parse_text(conf_t* conf, const char* text, const size_t length) {
    const char* buf;
    size_t s, i, line = 0;
    conf_token_t tokens[8];
    int ret, ret2;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!text) {
        return CONF_EINVAL;
    }

    memset(tokens, 0, sizeof(tokens));
    buf = text;
    s = length;
    line++;

    while (1) {
        /*
         * Go to the first non white-space character
         */
        for (ret = CONF_OK; *buf && s; buf++, s--) {
            if (*buf != ' ' && *buf != '\t') {
                if (*buf == '\n' || *buf == '\t') {
                    ret = CONF_EMPTY;
                }
                break;
            }
        }
        /*
         * Parse all the tokens
         */
        for (i = 0; i < 8 && ret == CONF_OK; i++) {
            ret = parse_token(&buf, &s, &tokens[i]);
        }

        if (ret == CONF_COMMENT) {
            /*
             * Line ended with comment, reduce the number of tokens
             */
            i--;
            if (!i) {
                /*
                 * Comment was the only token so the line is empty
                 */
                return CONF_OK;
            }
        }
        else if (ret == CONF_EMPTY) {
            i = 0;
        }
        else if (ret == CONF_OK) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu for %.*s, too many arguments", line, (int)tokens[0].length, tokens[0].token);
            }
            else {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu, too many arguments", line);
            }
            return CONF_ERROR;
        }
        else if (ret != CONF_LAST) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu for %.*s, invalid syntax", line, (int)tokens[0].length, tokens[0].token);
            }
            else {
                log_printf(conf_log(conf), LCORE, LERROR, "Config error at line %lu, invalid syntax", line);
            }
            return CONF_ERROR;
        }

        /*
         * Configure using the tokens
         */
        if (i && parse_tokens(conf, tokens, i, line) != CONF_OK) {
            return CONF_ERROR;
        }

        if (ret == CONF_COMMENT || !s || !*buf || *buf == '\n' || *buf == '\r') {
            break;
        }
    }

    return CONF_OK;
}

/*
 * Error strings
 */

const char* conf_strerr(int errnum) {
    switch (errnum) {
        case CONF_ERROR:
            return CONF_ERROR_STR;
        case CONF_EINVAL:
            return CONF_EINVAL_STR;
        case CONF_ENOMEM:
            return CONF_ENOMEM_STR;
        case CONF_EEXIST:
            return CONF_EEXIST_STR;
        default:
            break;
    }
    return "Unknown error";
}
