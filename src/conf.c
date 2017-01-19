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
#include "conf.h"

#include <stdlib.h>
#include <string.h>

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

inline conf_file_t* conf_file_next(const conf_file_t* conf_file) {
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

int conf_file_set_name(conf_file_t* conf_file, const char* name) {
    if (!conf_file) {
        return CONF_EINVAL;
    }
    if (!name) {
        return CONF_EINVAL;
    }

    if (conf_file->name) {
        free(conf_file->name);
    }
    if (!(conf_file->name = strdup(name))) {
        return CONF_ENOMEM;
    }

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

inline conf_interface_t* conf_interface_next(const conf_interface_t* conf_interface) {
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

int conf_interface_set_name(conf_interface_t* conf_interface, const char* name) {
    if (!conf_interface) {
        return CONF_EINVAL;
    }
    if (!name) {
        return CONF_EINVAL;
    }

    if (conf_interface->name) {
        free(conf_interface->name);
    }
    if (!(conf_interface->name = strdup(name))) {
        return CONF_ENOMEM;
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

                conf->read = conf_file_next(conf_file);
                conf_file_free(conf_file);
            }
            conf->have_read = 0;
        }
        if (conf->have_input) {
            while (conf->input) {
                conf_interface_t* conf_interface = conf->input;

                conf->input = conf_interface_next(conf_interface);
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

int conf_set_filter(conf_t* conf, const char* filter) {
    if (!conf) {
        return CONF_EINVAL;
    }
    if (!filter) {
        return CONF_EINVAL;
    }

    if (conf->filter) {
        free(conf->filter);
    }
    if (!(conf->filter = strdup(filter))) {
        return CONF_ENOMEM;
    }
    conf->have_filter = 1;

    return CONF_OK;
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

int conf_add_read(conf_t* conf, const char* file) {
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
        err = conf_file_set_name(conf_file, file);
    if (err == CONF_OK)
        err = conf_file_set_next(conf_file, conf->read);
    if (err == CONF_OK) {
        conf->read = conf_file;
        conf->have_read = 1;
    }
    else
        conf_file_free(conf_file);

    return err;
}

int conf_add_input(conf_t* conf, const char* interface) {
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
        err = conf_interface_set_name(conf_interface, interface);
    if (err == CONF_OK)
        err = conf_interface_set_next(conf_interface, conf->input);
    if (err == CONF_OK) {
        conf->input = conf_interface;
        conf->have_input = 1;
    }
    else
        conf_interface_free(conf_interface);

    return CONF_OK;
}

int conf_set_write(conf_t* conf, const char* file) {
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

    if ((ret = conf_file_set_name(&(conf->write), file)) == CONF_OK) {
        conf->have_write = 1;
    }
    return ret;
}

int conf_set_output(conf_t* conf, const char* interface) {
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

    if ((ret = conf_interface_set_name(&(conf->output), interface)) == CONF_OK) {
        conf->have_output = 1;
    }
    return ret;
}

inline log_t* conf_log(conf_t* conf) {
    drool_assert(conf);
    return &(conf->log);
}

/*
 * conf parsers
 */

static conf_syntax_t _syntax[] = {
    /* debug <facility> [level] ; */
    {
        "debug",
        0,
        { TOKEN_STRINGS, TOKEN_END }
    },
    /* nodebug <facility> [level] ; */
    {
        "nodebug",
        0,
        { TOKEN_STRINGS, TOKEN_END }
    },
    /* read " <file.pcap> " ; */
    {
        "read",
        0,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* input " <interface> " ; */
    {
        "input",
        0,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* filter " <filter> " ; */
    {
        "filter",
        0,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* write " <file.pcap> " ; */
    {
        "write",
        0,
        { TOKEN_QSTRING, TOKEN_END }
    },
    /* output " <interface> " ; */
    {
        "output",
        0,
        { TOKEN_QSTRING, TOKEN_END }
    },
    { 0, 0, { TOKEN_END } }
};

int conf_parse_file(conf_t* conf, const char* file) {
    if (!conf) {
        return CONF_EINVAL;
    }
    if (!file) {
        return CONF_EINVAL;
    }

    /* TODO */

    return CONF_OK;
}

int conf_parse_text(conf_t* conf, const char* text) {
    if (!conf) {
        return CONF_EINVAL;
    }
    if (!text) {
        return CONF_EINVAL;
    }

    /* TODO */

    return CONF_OK;
}
