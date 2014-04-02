if HAVE_CHECK
TESTS                       = exynos_test
noinst_PROGRAMS             = $(TESTS)
exynos_test_CFLAGS 		= @CHECK_CFLAGS@ \
-DTEST_COMPILE \
-I/usr/include/libdrm \
-I/usr/include/dlog \
-I/usr/include/xorg \
-I/usr/include/pixman-1 \
-I/usr/include/xdbg \
-I/usr/include/exynos \
-I@top_srcdir@/src \
-I@top_srcdir@/src/accel \
-I@top_srcdir@/src/display \
-I@top_srcdir@/src/util \
-I@top_srcdir@/src/xv \
-I@top_srcdir@/src/g2d \
-I@top_srcdir@/tests \
-I@top_srcdir@/tests/fakes
exynos_test_LDADD        = @CHECK_LIBS@ @EXYNOS_LIBS@
exynos_test_LDFLAGS = -rpath `cd $(top_builddir)/src/.libs/; pwd`
exynos_test_SOURCES = \
main_test.c \
@top_srcdir@/src/accel/exynos_dri2.c \
@top_srcdir@/src/xv/exynos_video_overlay.c \
fakes/drm.c \
fakes/tbm.c \
fakes/other.c \
fakes/mem.c \
fakes/x_and_os.c \
test_display.c \
test_clone.c \
test_video_capture.c \
test_video_buffer.c \
test_video_converter.c \
test_video_image.c \
test_video_ipp.c \
test_video.c \
test_mem_pool.c \
test_util.c \
test_fimg2d.c \
test_util_g2d.c \
test_crtc.c \
test_output.c \
test_exa_sw.c \
test_accel.c \
test_dri2.c \
test_exa.c \
prepares.c \
test_plane.c
endif

