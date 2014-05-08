AC_PREREQ(2.60)
AC_INIT([xf86-video-exynos],
        0.0.1,
        [https://],
        xf86-video-exynos)

AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(.)

# Initialize Automake
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

# Require X.Org macros 1.8 or later
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.8)
XORG_DEFAULT_OPTIONS
XORG_MANPAGE_SECTIONS


# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AM_PROG_AS
AM_PROG_CC_C_O

AH_TOP([#include "xorg-server.h"])
# Define a configure option for an alternate module directory
AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
						   [moduledir="$withval"],
						   [moduledir="$libdir/xorg/modules"])

AC_ARG_ENABLE(pciaccess,
	      AS_HELP_STRING([--enable-pciaccess],
                             [Enable use of libpciaccess (default: disabled)]),
			     [PCIACCESS=$enableval], [PCIACCESS=no])

# Checks for extensions
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)

# Check check =) modules for unit tests
PKG_CHECK_MODULES([CHECK],[check >= 0.9.4],[HAVE_CHECK=yes],[HAVE_CHECK=no])
AM_CONDITIONAL(HAVE_CHECK, test x$HAVE_CHECK = xyes)
if test "x$HAVE_CHECK" = xyes ; then
    CHECK_LIBS="-lcheck -lpthread -lxdbg-log"
    CHECK_CFLAGS="-Wl,-V"
    AC_SUBST(CHECK_CFLAGS)
    AC_SUBST(CHECK_LIBS)
fi

# Checks for pkg-config packages
PKG_CHECK_MODULES(XORG, [xorg-server >= 1.0.99.901 xproto])
sdkdir=$(pkg-config --variable=sdkdir xorg-server)

# check the conditions
EXYNOS_CFALGS=""
EXYNOS_LIBS=""

PKG_CHECK_MODULES(XDBG, [xdbg], [xdbg=yes], [xdbg=no])
if test x"$xdbg" = xyes ; then
    AC_DEFINE(USE_XDBG_EXTERNAL,1,[Enable external log module])
fi
PKG_CHECK_MODULES(DRM, libdrm)
PKG_CHECK_MODULES(LIBTBM, libtbm)
PKG_CHECK_MODULES(UDEV, [libudev], [udev=yes], [udev=no])
if test x"$udev" = xyes ; then
    AC_DEFINE(HAVE_UDEV,1,[Enable udev-based monitor hotplug detection])
fi

PKG_CHECK_MODULES(PIXMAN, [pixman], [pixman=yes], [pixman=no])
if test x"$pixman" = xno ; then
    PKG_CHECK_MODULES(PIXMAN1, pixman-1)
fi

EXYNOS_CFLAGS="$EXYNOS_CFLAGS $DLOG_CFLAGS $XDBG_CFLAGS"
EXYNOS_LIBS="$EXYNOS_LIBS $DLOG_LIBS $XDBG_LIBS"

EXYNOS_CFLAGS="$EXYNOS_CFLAGS $DRM_CFLAGS $LIBTBM_CFLAGS "
EXYNOS_LIBS="$EXYNOS_LIBS $DRM_LIBS $LIBTBM_LIBS "

EXYNOS_CFLAGS="$EXYNOS_CFLAGS $UDEV_CFALGS "
EXYNOS_LIBS="$EXYNOS_LIBS $UDEV_LIBS "

AM_CONDITIONAL(PCIACCESS, [test "x$PCIACCESS" = xyes])
if test "x$PCIACCESS" = xyes; then
    AC_DEFINE(PCIACCESS, 1, [Use libpciaccess])
    PKG_CHECK_MODULES([PCIACCESS], [pciaccess >= 0.8.0])
    XORG_CFLAGS="$XORG_CFLAGS $PCIACCESS_CFLAGS "
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

AC_SUBST([EXYNOS_CFLAGS])
AC_SUBST([EXYNOS_LIBS])

AC_SUBST([XORG_CFLAGS])
AC_SUBST([moduledir])

DRIVER_NAME=exynos
AC_SUBST([DRIVER_NAME])

AC_OUTPUT([
    Makefile
	src/Makefile
	man/Makefile
    tests/Makefile
])
