MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
CLEANFILES = drool.1

SUBDIRS = test

AM_CFLAGS = -I$(srcdir) \
    -I$(top_srcdir)
#    $(PTHREAD_CFLAGS)

EXTRA_DIST = dnscap.1.in

bin_PROGRAMS = drool

drool_SOURCES = drool.c
dist_drool_SOURCES = drool.h
# drool_LDADD = $(PTHREAD_LIBS)

man1_MANS = drool.1

drool.1: drool.1.in Makefile
	sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
        -e 's,[@]PACKAGE_URL[@],$(PACKAGE_URL),g' \
        -e 's,[@]PACKAGE_BUGREPORT[@],$(PACKAGE_BUGREPORT),g' \
        < $(srcdir)/drool.1.in > drool.1
