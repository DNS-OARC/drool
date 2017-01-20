# DNS Reply Tool (drool)
#
# Copyright (c) 2017, OARC, Inc.
# Copyright (c) 2017, Comcast Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
CLEANFILES = drool.1

SUBDIRS = test

AM_CFLAGS = -I$(srcdir) \
    -I$(top_srcdir) \
    $(PTHREAD_CFLAGS)

EXTRA_DIST = drool.1.in \
    drool.conf.5.in \
    drool.conf.example

bin_PROGRAMS = drool

drool_SOURCES = conf.c drool.c log.c \
    omg-dns/omg_dns.c \
    pcap-thread/pcap_thread.c
dist_drool_SOURCES = conf.h drool.h log.h \
    omg-dns/omg_dns.h \
    pcap-thread/pcap_thread.h
drool_LDADD = $(PTHREAD_LIBS)

man1_MANS = drool.1 \
    drool.conf.5

drool.1: drool.1.in Makefile
	sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
        -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
        -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
        < $(srcdir)/drool.1.in > drool.1

drool.conf.5: drool.conf.5.in Makefile
	sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
        -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
        -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
        < $(srcdir)/drool.conf.5.in > drool.conf.5
