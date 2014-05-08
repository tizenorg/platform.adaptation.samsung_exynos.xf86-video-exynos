AM_CFLAGS = -I@top_srcdir@/src @XORG_CFLAGS@ @EXYNOS_CFLAGS@
exynos_drv_la_LTLIBRARIES = exynos_drv.la
exynos_drv_la_LDFLAGS = -module -avoid-version
exynos_drv_la_LIBADD = @EXYNOS_LIBS@
exynos_drv_ladir = @moduledir@/drivers
# driver
exynos_drv_la_SOURCES = \
	exynos_driver.c

# display
exynos_drv_la_SOURCES += \
	display/exynos_display.c \
	display/exynos_crtc.c \
	display/exynos_output.c \
	display/exynos_clone.c \
	display/exynos_plane.c
AM_CFLAGS += -I@top_srcdir@/src/display
# display/exynos_output_prop.c

# accel
exynos_drv_la_SOURCES += \
	accel/exynos_accel.c \
	accel/exynos_exa.c   \
	accel/exynos_exa_sw.c \
	accel/exynos_dri2.c
AM_CFLAGS += -I@top_srcdir@/src/accel

# xv
exynos_drv_la_SOURCES += \
	xv/exynos_video.c \
	xv/exynos_video_image.c \
	xv/exynos_video_capture.c \
	xv/exynos_video_buffer.c \
	xv/exynos_video_converter.c \
	xv/exynos_video_ipp.c \
	xv/exynos_video_overlay.c
AM_CFLAGS += -I@top_srcdir@/src/xv

# util
exynos_drv_la_SOURCES += \
	util/exynos_util.c \
	util/exynos_mem_pool.c
AM_CFLAGS += -I@top_srcdir@/src/util

# g2d
exynos_drv_la_SOURCES += \
	g2d/util_g2d.c \
        g2d/fimg2d.c        
AM_CFLAGS += -I@top_srcdir@/src/g2d -Werror-implicit-function-declaration
