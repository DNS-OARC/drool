# DNS Reply Tool (drool)
#
# Copyright (c) 2017-2019, OARC, Inc.
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

AC_PREREQ(2.61)
AC_INIT([drool], [1.99.4], [admin@dns-oarc.net], [drool], [https://github.com/DNS-OARC/drool/issues])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/drool.in])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PATH_PROG([DNSJIT],[dnsjit])
AS_IF([test "x$ac_cv_path_DNSJIT" = "x"], [
  AC_MSG_ERROR([dnsjit was not found])
])
AC_MSG_CHECKING([for dnsjit >= 0.9.7])
AS_IF(["$DNSJIT" "$srcdir/dnsjit_version.lua" 0 9 7 2>/dev/null], [
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
  AC_MSG_ERROR([dnsjit version does not meet the requirements])
])

# Check --enable-warn-all, placeholer for OARC projects wide options
AC_ARG_ENABLE([warn-all])

# Check --with-extra-cflags, placeholer for OARC projects wide options
AC_ARG_WITH([extra-cflags])

# Check --with-extra-ldflags, placeholer for OARC projects wide options
AC_ARG_WITH([extra-ldflags])

# Check --with-shebang
AC_ARG_WITH([shebang], [AS_HELP_STRING([--with-shebang=shebang], [Set the shebang to use (without #!, default to /usr/bin/env dnsjit)])], [
  AC_MSG_NOTICE([using shebang... $withval])
  AC_SUBST([DROOL_SHEBANG], ["$withval"])
], [
  AC_MSG_NOTICE([using shebang... /usr/bin/env dnsjit])
  AC_SUBST([DROOL_SHEBANG], ["/usr/bin/env dnsjit"])
])

# Output Makefiles
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/test/Makefile
])
AC_OUTPUT
