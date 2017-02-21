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

#include "conf_file.h"
#include "conf.h"
#include "assert.h"

#include <stdlib.h>
#include <string.h>

drool_conf_file_t* conf_file_new(void) {
    drool_conf_file_t* conf_file = calloc(1, sizeof(drool_conf_file_t));
    return conf_file;
}

void conf_file_free(drool_conf_file_t* conf_file) {
    if (conf_file) {
        conf_file_release(conf_file);
        free(conf_file);
    }
}

void conf_file_release(drool_conf_file_t* conf_file) {
    if (conf_file) {
        if (conf_file->name) {
            free(conf_file->name);
            conf_file->name = 0;
        }
    }
}

inline const drool_conf_file_t* conf_file_next(const drool_conf_file_t* conf_file) {
    drool_assert(conf_file);
    return conf_file->next;
}

int conf_file_set_next(drool_conf_file_t* conf_file, drool_conf_file_t* next) {
    if (!conf_file) {
        return CONF_EINVAL;
    }
    if (!next) {
        return CONF_EINVAL;
    }

    conf_file->next = next;

    return CONF_OK;
}

inline const char* conf_file_name(const drool_conf_file_t* conf_file) {
    drool_assert(conf_file);
    return conf_file->name;
}

int conf_file_set_name(drool_conf_file_t* conf_file, const char* name, size_t length) {
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

    return CONF_OK;
}
