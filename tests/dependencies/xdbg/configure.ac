# Copyright 2013 Samsung Electronics co., Ltd. All Rights Reserved.
#
# Contact: Boram Park <boram1288.park@samsung.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

AC_PREREQ(2.57)
AC_INIT([xdbg], [0.1.0], [https://])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE()

AM_MAINTAINER_MODE

# Require xorg-macros: XORG_DEFAULT_OPTIONS
m4_ifndef([XORG_MACROS_VERSION], [AC_FATAL([must install xorg-macros 1.3 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.3)
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_LIBTOOL
AC_PROG_CC
XORG_DEFAULT_OPTIONS

AH_TOP([#include "xorg-server.h"])

AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
                           [moduledir="$withval"],
                           [moduledir="$libdir/xorg/modules"])

# Checks for pkg-config packages
PKG_CHECK_MODULES(XDBG, xorg-server x11 dbus-1 bigreqsproto compositeproto damageproto dmxproto dri2proto fixesproto fontsproto gestureproto inputproto kbproto randrproto recordproto renderproto resourceproto scrnsaverproto videoproto xcmiscproto xextproto xf86bigfontproto xf86dgaproto xf86driproto xf86vidmodeproto xineramaproto xproto libdrm)
AC_SUBST([XDBG_CFLAGS])
AC_SUBST([XDBG_LIBS])

# Checks for header files.
AC_HEADER_STDC

MODULE_NAME=xdbg
AC_SUBST([MODULE_NAME])
AC_SUBST([moduledir])

AC_DEFINE_UNQUOTED(MODULE_NAME, "$MODULE_NAME", [module name])

AC_OUTPUT([Makefile
           dbus/Makefile
           bin/Makefile
           bin/xdbg/Makefile
           lib/Makefile
           lib/xdbg.pc
           module/Makefile])

echo "CFLAGS     : $CFLAGS"
echo "LDFLAGS    : $LDFLAGS"
echo "XDBG_CFLAGS: $XDBG_CFLAGS"
echo "XDBG_LIBS  : $XDBG_LIBS"
