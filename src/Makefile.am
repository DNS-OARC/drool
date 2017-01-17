MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
CLEANFILES = drool.1

SUBDIRS = test

AM_CFLAGS = -I$(srcdir) \
    -I$(top_srcdir)
#    $(PTHREAD_CFLAGS)

EXTRA_DIST = drool.1.in \
    drool.conf.5.in \
    drool.conf.example

bin_PROGRAMS = drool

drool_SOURCES = conf.c drool.c log.c
dist_drool_SOURCES = assert.h conf.h drool.h log.h
# drool_LDADD = $(PTHREAD_LIBS)

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
