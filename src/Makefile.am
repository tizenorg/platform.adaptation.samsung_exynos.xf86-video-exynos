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
	display/exynos_plane.c
#	display/exynos_clone.c
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
	xv/exynos_video_image.c
AM_CFLAGS += -I@top_srcdir@/src/xv

# util
exynos_drv_la_SOURCES += \
	util/exynos_util.c \
	util/exynos_mem_pool.c
AM_CFLAGS += -I@top_srcdir@/src/util

# g2d


# engine
exynos_drv_la_SOURCES += \
        engine/exynos_engine.c \
        engine/exynos_engine_pixman.c
AM_CFLAGS += -I@top_srcdir@/src/engine

AM_CFLAGS += -Werror-implicit-function-declaration
