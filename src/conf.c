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
#include "parseconf/parseconf.h"
#include "assert.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

drool_conf_t* conf_new(void) {
    drool_conf_t* conf = calloc(1, sizeof(drool_conf_t));
    return conf;
}

void conf_free(drool_conf_t* conf) {
    if (conf) {
        conf_release(conf);
        free(conf);
    }
}

void conf_release(drool_conf_t* conf) {
    if (conf) {
        if (conf->have_filter) {
            free(conf->filter);
            conf->filter = 0;
        }
        if (conf->have_read) {
            while (conf->read) {
                drool_conf_file_t* conf_file = conf->read;

                conf->read = conf_file->next;
                conf_file_free(conf_file);
            }
            conf->have_read = 0;
        }
        if (conf->have_input) {
            while (conf->input) {
                drool_conf_interface_t* conf_interface = conf->input;

                conf->input = conf_interface->next;
                conf_interface_free(conf_interface);
            }
            conf->have_input = 0;
        }
        /*
        if (conf->have_write) {
            conf_file_release(&(conf->write));
            conf->have_write = 0;
        }
        if (conf->have_output) {
            conf_interface_release(&(conf->output));
            conf->have_output = 0;
        }
        */
    }
}

inline int conf_have_filter(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_filter;
}

inline int conf_have_read(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_read;
}

inline int conf_have_input(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_input;
}

inline int conf_have_read_mode(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_read_mode;
}

/*
inline int conf_have_write(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_write;
}

inline int conf_have_output(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->have_output;
}
*/

inline const char* conf_filter(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->filter;
}

int conf_set_filter(drool_conf_t* conf, const char* filter, size_t length) {
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

inline const size_t conf_filter_length(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->filter_length;
}

inline const drool_conf_file_t* conf_read(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->read;
}

inline const drool_conf_interface_t* conf_input(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->input;
}

inline drool_conf_read_mode_t conf_read_mode(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->read_mode;
}

inline size_t conf_read_iter(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->read_iter;
}

/*
inline const drool_conf_file_t* conf_write(const drool_conf_t* conf) {
    drool_assert(conf);
    return &(conf->write);
}

inline const drool_conf_interface_t* conf_output(const drool_conf_t* conf) {
    drool_assert(conf);
    return &(conf->output);
}
*/

inline drool_timing_mode_t conf_timing_mode(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->timing_mode;
}

inline unsigned long int conf_timing_increase(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->timing_increase;
}

inline unsigned long int conf_timing_reduce(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->timing_reduce;
}

inline long double conf_timing_multiply(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->timing_multiply;
}

inline size_t conf_context_client_pools(const drool_conf_t* conf) {
    drool_assert(conf);
    return conf->context_client_pools;
}

int conf_add_read(drool_conf_t* conf, const char* file, size_t length) {
    drool_conf_file_t* conf_file;
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

int conf_add_input(drool_conf_t* conf, const char* interface, size_t length) {
    drool_conf_interface_t* conf_interface;
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

int conf_set_read_mode(drool_conf_t* conf, drool_conf_read_mode_t read_mode) {
    if (!conf) {
        return CONF_EINVAL;
    }

    conf->read_mode = read_mode;
    if (read_mode == CONF_READ_MODE_NONE)
        conf->have_read_mode = 0;
    else
        conf->have_read_mode = 1;

    return CONF_OK;
}

int conf_set_read_iter(drool_conf_t* conf, size_t read_iter) {
    if (!conf) {
        return CONF_EINVAL;
    }

    conf->read_iter = read_iter;

    return CONF_OK;
}

/*
int conf_set_write(drool_conf_t* conf, const char* file, size_t length) {
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

int conf_set_output(drool_conf_t* conf, const char* interface, size_t length) {
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
*/

int conf_set_context_client_pools(drool_conf_t* conf, size_t context_client_pools) {
    if (!conf) {
        return CONF_EINVAL;
    }
    if (!context_client_pools) {
        return CONF_EINVAL;
    }

    conf->context_client_pools = context_client_pools;

    return CONF_OK;
}

inline const drool_log_t* conf_log(const drool_conf_t* conf) {
    drool_assert(conf);
    return &(conf->log);
}

inline drool_log_t* conf_log_rw(drool_conf_t* conf) {
    drool_assert(conf);
    return &(conf->log);
}

inline const drool_conf_client_pool_t* conf_client_pool(const drool_conf_t* conf) {
    drool_assert(conf);
    return &(conf->client_pool);
}

/*
 * timing conf parsers
 */

static parseconf_token_type_t timing_ignore_tokens[] = {
    PARSECONF_TOKEN_END
};

static int parse_timing_ignore(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    conf->timing_mode = TIMING_MODE_IGNORE;

    return 0;
}

static parseconf_token_type_t timing_keep_tokens[] = {
    PARSECONF_TOKEN_END
};

static int parse_timing_keep(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    conf->timing_mode = TIMING_MODE_KEEP;

    return 0;
}

static parseconf_token_type_t timing_increase_tokens[] = {
    PARSECONF_TOKEN_NUMBER, PARSECONF_TOKEN_END
};

static int parse_timing_increase(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    unsigned long int nanoseconds = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_ulongint(&tokens[2], &nanoseconds, errstr)) {
        return 1;
    }

    conf->timing_mode = TIMING_MODE_INCREASE;
    conf->timing_increase = nanoseconds;

    return 0;
}

static parseconf_token_type_t timing_reduce_tokens[] = {
    PARSECONF_TOKEN_NUMBER, PARSECONF_TOKEN_END
};

static int parse_timing_reduce(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    unsigned long int nanoseconds = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_ulongint(&tokens[2], &nanoseconds, errstr)) {
        return 1;
    }

    conf->timing_mode = TIMING_MODE_REDUCE;
    conf->timing_reduce = nanoseconds;

    return 0;
}

static parseconf_token_type_t timing_multiply_tokens[] = {
    PARSECONF_TOKEN_FLOAT, PARSECONF_TOKEN_END
};

static int parse_timing_multiply(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    long double multiply = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_longdouble(&tokens[2], &multiply, errstr)) {
        return 1;
    }

    conf->timing_mode = TIMING_MODE_MULTIPLY;
    conf->timing_multiply = multiply;

    return 0;
}

static parseconf_syntax_t timing_syntax[] = {
    /* timing ignore; */
    { "ignore", parse_timing_ignore, timing_ignore_tokens, 0 },
    /* timing keep; */
    { "keep", parse_timing_keep, timing_keep_tokens, 0 },
    /* timing increase <nanoseconds>; */
    { "increase", parse_timing_increase, timing_increase_tokens, 0 },
    /* timing reduce <nanoseconds>; */
    { "reduce", parse_timing_reduce, timing_reduce_tokens, 0 },
    /* timing multiply <multiplication>; */
    { "multiply", parse_timing_multiply, timing_multiply_tokens, 0 },
    PARSECONF_SYNTAX_END
};

/*
 * client_pool conf parsers
 */

static parseconf_token_type_t client_pool_target_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_client_pool_target(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    return conf_client_pool_set_target(&(conf->client_pool), tokens[2].token, tokens[2].length, tokens[3].token, tokens[3].length);
}

static parseconf_token_type_t client_pool_max_clients_tokens[] = {
    PARSECONF_TOKEN_NUMBER, PARSECONF_TOKEN_END
};

static int parse_client_pool_max_clients(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    size_t max_clients = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_ulongint(&tokens[2], &max_clients, errstr)) {
        return 1;
    }

    return conf_client_pool_set_max_clients(&(conf->client_pool), max_clients);
}

static parseconf_token_type_t client_pool_client_ttl_tokens[] = {
    PARSECONF_TOKEN_FLOAT, PARSECONF_TOKEN_END
};

static int parse_client_pool_client_ttl(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    double client_ttl = 0.;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_double(&tokens[2], &client_ttl, errstr)) {
        return 1;
    }

    return conf_client_pool_set_client_ttl(&(conf->client_pool), client_ttl);
}

static parseconf_token_type_t client_pool_skip_reply_tokens[] = {
    PARSECONF_TOKEN_FLOAT, PARSECONF_TOKEN_END
};

static int parse_client_pool_skip_reply(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;

    if (!conf) {
        return 1;
    }

    return conf_client_pool_set_skip_reply(&(conf->client_pool));
}

static parseconf_token_type_t client_pool_max_reuse_clients_tokens[] = {
    PARSECONF_TOKEN_NUMBER, PARSECONF_TOKEN_END
};

static int parse_client_pool_max_reuse_clients(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    size_t max_reuse_clients = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_ulongint(&tokens[2], &max_reuse_clients, errstr)) {
        return 1;
    }

    return conf_client_pool_set_max_reuse_clients(&(conf->client_pool), max_reuse_clients);
}

static parseconf_token_type_t client_pool_sendas_tokens[] = {
    PARSECONF_TOKEN_STRING, PARSECONF_TOKEN_END
};

static int parse_client_pool_sendas(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    drool_client_pool_sendas_t sendas;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (!strncmp(tokens[2].token, "original", tokens[2].length)) {
        sendas = CLIENT_POOL_SENDAS_ORIGINAL;
    }
    else if (!strncmp(tokens[2].token, "udp", tokens[2].length)) {
        sendas = CLIENT_POOL_SENDAS_UDP;
    }
    else if (!strncmp(tokens[2].token, "tcp", tokens[2].length)) {
        sendas = CLIENT_POOL_SENDAS_TCP;
    }
    else {
        *errstr = "Invalid send as";
        return 1;
    }

    return conf_client_pool_set_sendas(&(conf->client_pool), sendas);
}

static parseconf_syntax_t client_pool_syntax[] = {
    /* client_pool target <host> <port>; */
    { "target", parse_client_pool_target, client_pool_target_tokens, 0 },
    /* client_pool max_clients <num>; */
    { "max_clients", parse_client_pool_max_clients, client_pool_max_clients_tokens, 0 },
    /* client_pool client_ttl <float>; */
    { "client_ttl", parse_client_pool_client_ttl, client_pool_client_ttl_tokens, 0 },
    /* client_pool skip_reply; */
    { "skip_reply", parse_client_pool_skip_reply, client_pool_skip_reply_tokens, 0 },
    /* client_pool max_reuse_clients <num>; */
    { "max_reuse_clients", parse_client_pool_max_reuse_clients, client_pool_max_reuse_clients_tokens, 0 },
    /* client_pool sendas <what>; */
    { "sendas", parse_client_pool_sendas, client_pool_sendas_tokens, 0 },
    PARSECONF_SYNTAX_END
};

/*
 * context parsers
 */

static parseconf_token_type_t context_client_pools_tokens[] = {
    PARSECONF_TOKEN_NUMBER, PARSECONF_TOKEN_END
};

static int parse_context_client_pools(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    size_t client_pools = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if (parseconf_ulongint(&tokens[2], &client_pools, errstr)) {
        return 1;
    }

    return conf_set_context_client_pools(conf, client_pools);
}

static parseconf_syntax_t context_syntax[] = {
    /* context client_pools <num>; */
    { "client_pools", parse_context_client_pools, context_client_pools_tokens, 0 },
    PARSECONF_SYNTAX_END
};

/*
 * conf parsers
 */

static parseconf_token_type_t log_tokens[] = {
    PARSECONF_TOKEN_STRINGS, PARSECONF_TOKEN_END
};

static int parse_log(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    drool_log_facility_t facility = LOG_FACILITY_NONE;
    drool_log_level_t level = LOG_LEVEL_ALL;
    int all = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

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

    if (tokens[2].type == PARSECONF_TOKEN_STRING) {
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

static parseconf_token_type_t nolog_tokens[] = {
    PARSECONF_TOKEN_STRINGS, PARSECONF_TOKEN_END
};

static int parse_nolog(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    drool_log_facility_t facility = LOG_FACILITY_NONE;
    drool_log_level_t level = LOG_LEVEL_ALL;
    int all = 0;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

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

    if (tokens[2].type == PARSECONF_TOKEN_STRING) {
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

/*
static parseconf_token_type_t read_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_read(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    int err;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if ((err = conf_add_read(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static parseconf_token_type_t input_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_input(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    int err;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if ((err = conf_add_input(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}
*/

static parseconf_token_type_t filter_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_filter(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    int err;

    if (!conf) {
        return 1;
    }
    if (!tokens) {
        return 1;
    }
    if (!errstr) {
        return 1;
    }

    if ((err = conf_set_filter(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }
    return 0;
}

/*
static parseconf_token_type_t write_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_write(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    int err;

    if ((err = conf_set_write(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}

static parseconf_token_type_t output_tokens[] = {
    PARSECONF_TOKEN_QSTRING, PARSECONF_TOKEN_END
};

static int parse_output(void* user, const parseconf_token_t* tokens, const char** errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;
    int err;

    if ((err = conf_set_output(conf, tokens[1].token, tokens[1].length)) != CONF_OK) {
        *errstr = conf_strerr(err);
        return 1;
    }

    return 0;
}
*/

static parseconf_token_type_t nested_tokens[] = {
    PARSECONF_TOKEN_NESTED
};

static parseconf_syntax_t _syntax2[] = {
    /* log <facility> [level] ; */
    { "log", parse_log, log_tokens, 0 },
    /* nolog <facility> [level] ; */
    { "nolog", parse_nolog, nolog_tokens, 0 },
    /* read " <file.pcap> " ; */
    /*{ "read", parse_read, read_tokens, 0 },*/
    /* input " <interface> " ; */
    /*{ "input", parse_input, input_tokens, 0 },*/
    /* filter " <filter> " ; */
    { "filter", parse_filter, filter_tokens, 0 },
    /* write " <file.pcap> " ; */
    /*{ "write", parse_write, write_tokens, 0 },*/
    /* output " <interface> " ; */
    /*{ "output", parse_output, output_tokens, 0 },*/
    /* timing ... ; */
    { "timing", 0, nested_tokens, timing_syntax },
    /* client_pool ... ; */
    { "client_pool", 0, nested_tokens, client_pool_syntax },
    /* context ... ; */
    { "context", 0, nested_tokens, context_syntax },
    PARSECONF_SYNTAX_END
};

static void parseconf_error(void* user, parseconf_error_t error, size_t line, size_t token, const parseconf_token_t* tokens, const char* errstr) {
    drool_conf_t* conf = (drool_conf_t*)user;

    if (!conf) {
        return;
    }

    switch (error) {
        case PARSECONF_ERROR_INTERNAL:
            log_printf(conf_log(conf), LCORE, LERROR, "Internal conf error at line %lu", line);
            break;

        case PARSECONF_ERROR_EXPECT_STRING:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, expected a string", line, token);
            break;

        case PARSECONF_ERROR_EXPECT_NUMBER:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, expected a number", line, token);
            break;

        case PARSECONF_ERROR_EXPECT_QSTRING:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, expected a quoted string", line, token);
            break;

        case PARSECONF_ERROR_EXPECT_FLOAT:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, expected a float", line, token);
            break;

        case PARSECONF_ERROR_EXPECT_ANY:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, expected any type", line, token);
            break;

        case PARSECONF_ERROR_UNKNOWN:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu for argument %lu, unknown configuration", line, token);
            break;

        case PARSECONF_ERROR_NO_NESTED:
            log_printf(conf_log(conf), LCORE, LERROR, "Internal conf error at line %lu for argument %lu, no nested conf", line, token);
            break;

        case PARSECONF_ERROR_NO_CALLBACK:
            log_printf(conf_log(conf), LCORE, LERROR, "Internal conf error at line %lu for argument %lu, no callback", line, token);
            break;

        case PARSECONF_ERROR_CALLBACK:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu, %s", line, errstr);
            break;

        case PARSECONF_ERROR_FILE_ERRNO:
            log_errnof(conf_log(conf), LCORE, LERROR, "Internal conf error at line %lu for argument %lu, errno", line, token);
            break;

        case PARSECONF_ERROR_TOO_MANY_ARGUMENTS:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu, too many arguments", line);
            break;

        case PARSECONF_ERROR_INVALID_SYNTAX:
            log_printf(conf_log(conf), LCORE, LERROR, "Conf error at line %lu, invalid syntax", line);
            break;

        default:
            log_printf(conf_log(conf), LCORE, LERROR, "Unknown conf error %d at %lu", error, line);
            break;
    }
}

int conf_parse_file(drool_conf_t* conf, const char* file) {
    int err;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!file) {
        return CONF_EINVAL;
    }

    log_printf(conf_log(conf), LCORE, LDEBUG, "Opening config file %s", file);
    if ((err = parseconf_file((void*)conf, file, _syntax2, parseconf_error)) != PARSECONF_OK) {
        log_printf(conf_log(conf), LCORE, LERROR, "Parsing file %s failed: %s", file, parseconf_strerr(err));
        return CONF_ERROR;
    }

    return CONF_OK;
}

int conf_parse_text(drool_conf_t* conf, const char* text, size_t length) {
    int err;

    if (!conf) {
        return CONF_EINVAL;
    }
    if (!text) {
        return CONF_EINVAL;
    }
    if (!length) {
        return CONF_EINVAL;
    }

    if ((err = parseconf_text((void*)conf, text, length, _syntax2, parseconf_error)) != PARSECONF_OK) {
        log_printf(conf_log(conf), LCORE, LERROR, "Parsing text failed: %s", parseconf_strerr(err));
        return CONF_ERROR;
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
