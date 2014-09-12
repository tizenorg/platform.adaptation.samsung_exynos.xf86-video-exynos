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

libxdbg_la_LTLIBRARIES = libxdbg.la
libxdbg_ladir = @moduledir@
libxdbg_la_LIBADD = @XDBG_LIBS@ ../dbus/libdbus.la ../lib/libxdbg-log.la
libxdbg_la_LDFLAGS = -module -avoid-version ${LDFLAGS}
libxdbg_la_CFLAGS = \
	${CFLAGS} \
	@XDBG_CFLAGS@ \
	-I$(top_srcdir)/dbus \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/module \
	-I$(top_srcdir)/module/ds

libxdbg_la_SOURCES = \
	xdbg_module.c \
	xdbg_module_main.c \
	xdbg_module_options.c \
	xdbg_module_command.c \
	xdbg_module_clist.c \
	xdbg_module_plist.c \
	xdbg_module_fpsdebug.c \
	xdbg_module_evlog.c \
	xdbg_module_evlog_request.c \
	xdbg_module_evlog_event.c \
	xdbg_module_drmevent.c \
	ds/bintree.c \
	ds/bool_exp_parser.c \
	ds/bool_exp_rule_checker.c \
	ds/bool_exp_tokenizer.c
