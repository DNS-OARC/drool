ACLOCAL_AMFLAGS = -I m4
# -I src/pcap-thread/m4

MAINTAINERCLEANFILES = $(srcdir)/Makefile.in \
    $(srcdir)/src/config.h.in~ \
    $(srcdir)/configure

SUBDIRS = src

dist_doc_DATA = README.md LICENSE

EXTRA_DIST = m4

test: check
