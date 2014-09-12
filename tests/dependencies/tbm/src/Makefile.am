SUBDIRS = .

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(PTHREADSTUBS_CFLAGS) \
	@LIBTBM_CFLAGS@

libtbm_la_LTLIBRARIES = libtbm.la
libtbm_ladir = $(libdir)
libtbm_la_LDFLAGS = -version-number 1:0:0 -no-undefined
libtbm_la_LIBADD = @LIBTBM_LIBS@ @PTHREADSTUBS_LIBS@ @CLOCK_LIB@ -ldl

libtbm_la_SOURCES = \
	tbm_bufmgr_backend.c \
	tbm_bufmgr.c

libtbmincludedir=$(includedir)
libtbminclude_HEADERS = tbm_bufmgr.h tbm_bufmgr_backend.h

