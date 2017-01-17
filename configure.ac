AC_PREREQ(2.61)
AC_INIT([drool], [1.0.0-alpha.1], [admin@dns-oarc.net], [drool], [https://github.com/DNS-OARC/drool/issues])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/drool.c])
AC_CONFIG_HEADER([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_CANONICAL_HOST
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([disable-static])

# pcap_thread
# AX_PCAP_THREAD

# Checks for libraries.

# Check for OS specific libraries
# case "$host_os" in
# esac

# Checks for header files.

# Checks for library functions.

# Output Makefiles
AC_CONFIG_FILES([
    Makefile
    src/Makefile
	src/test/Makefile
])
AC_OUTPUT
