
AC_PREREQ(2.61)
AC_INIT([xf86-video-exynos], 1.0.0,
        [https://bugs.tizen.org/jira],
        [xf86-video-exynos])

AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_AUX_DIR(.)

AM_INIT_AUTOMAKE([dist-bzip2])

AM_MAINTAINER_MODE

# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_CC_STDC
AM_PROG_AS
AM_PROG_CC_C_O

AH_TOP([#include "xorg-server.h"])

AC_ARG_WITH(xorg-module-dir,
            AC_HELP_STRING([--with-xorg-module-dir=DIR],
                           [Default xorg module directory [[default=$libdir/xorg/modules]]]),
						   [moduledir="$withval"],
						   [moduledir="$libdir/xorg/modules"])

AC_ARG_ENABLE(pciaccess,
	      AS_HELP_STRING([--enable-pciaccess],
                             [Enable use of libpciaccess (default: disabled)]),
			     [PCIACCESS=$enableval], [PCIACCESS=no])

AC_ARG_ENABLE(dri3,
	      AS_HELP_STRING([--enable-dri3],
                             [Enable DRI3 backend (default: disabled)]),
			     [DRI3=$enableval], [DRI3=no])

AC_ARG_ENABLE(ftests,
AS_HELP_STRING([--enable-ftests], [Enable functional tests (default: no)]),
                [FT="$enableval"], [FT="no"])
# Checks for extensions
PKG_CHECK_MODULES(XORG, xorg-server)
XORG_DRIVER_CHECK_EXT(RANDR, randrproto)
XORG_DRIVER_CHECK_EXT(RENDER, renderproto)
XORG_DRIVER_CHECK_EXT(XV, videoproto)

# Checks for pkg-config packages
PKG_CHECK_MODULES(PROTO, [xproto fontsproto])

sdkdir=$(pkg-config --variable=sdkdir xorg-server)

ftests=no
FT_CFLAGS=""
FT_LIBS=""
EXYNOS_CFLAGS=""
EXYNOS_LIBS=""
if test "x$FT" != "xno"; then
    PKG_CHECK_MODULES(XCB, [xcb xcb-atom xcb-shm xcb-xv xcb-util xcb-dri2], [ftests=yes],
    [ftests=no])
    if test "x$FT" = "xyes" -a "x$ftests" != "xyes"; then
        AC_MSG_ERROR([Not find xcb library for functional tests])
    fi
    FT_CFLAGS="$XCB_CFLAGS "
    FT_LIBS="$XCB_LIBS "
fi

AM_CONDITIONAL(HAVE_FT, test "x$ftests" = "xyes")

# check the conditions
PKG_CHECK_MODULES(XDBG, [xdbg], [xdbg=yes], [xdbg=no])
PKG_CHECK_MODULES(DRM, libdrm)
PKG_CHECK_MODULES(LIBTBM, libtbm)
PKG_CHECK_MODULES(UDEV, [libudev], [udev=yes], [udev=no])

if test x"$xdbg" = xno; then
    AC_MSG_ERROR([Not find xdbg lib])
fi

if test x"$udev" = xyes; then
	AC_DEFINE(HAVE_UDEV,1,[Enable udev-based monitor hotplug detection])
fi

EXYNOS_CFLAGS="$EXYNOS_CFLAGS $PROTO_CFLAGS $XDBG_CFLAGS $DRM_CFLAGS $LIBTBM_CFLAGS $UDEV_CFALGS"
EXYNOS_LIBS="$EXYNOS_LIBS $XDBG_LIBS $DRM_LIBS $LIBTBM_LIBS $UDEV_LIBS $XDBG_LIBS"
FT_CFLAGS="$FT_CFLAGS $EXYNOS_CFLAGS"
FT_LIBS="$FT_LIBS $EXYNOS_LIBS"

AM_CONDITIONAL(PCIACCESS, [test "x$PCIACCESS" = xyes])
if test "x$PCIACCESS" = xyes; then
    AC_DEFINE(PCIACCESS, 1, [Use libpciaccess])
    PKG_CHECK_MODULES([PCIACCESS], [pciaccess >= 0.8.0])
    XORG_CFLAGS="$XORG_CFLAGS $PCIACCESS_CFLAGS "
fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

if test "x$DRI3" = xyes; then
    AC_CHECK_FILES( /usr/include/xorg/dri3.h /usr/include/xorg/present.h, [dri3=yes], [dri3=no]) 
    AM_CONDITIONAL(ENABLE_DRI3_PRESENT, [test "x$dri3" = xyes])
    if test "x$dri3" = xyes; then
        AC_DEFINE(HAVE_DRI3_PRESENT_H, 1, [ DRI3 and Present is enabled ])
    fi
else
    AM_CONDITIONAL(ENABLE_DRI3_PRESENT, [test "x$dri3" = xno])
fi

AC_SUBST([EXYNOS_CFLAGS])
AC_SUBST([EXYNOS_LIBS])
AC_SUBST([FT_CFLAGS])
AC_SUBST([FT_LIBS])

AC_SUBST([XORG_CFLAGS])
AC_SUBST([moduledir])

DRIVER_NAME=exynos
AC_SUBST([DRIVER_NAME])

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	man/Makefile
    tests/functional/Makefile
    tests/functional/xv_test/Makefile
])
AC_OUTPUT
