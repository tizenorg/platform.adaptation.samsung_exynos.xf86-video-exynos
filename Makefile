export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig
export LD_LIBRARY_PATH=/usr/lib


CCP = g++
CFLAGS = -g -O0 -Wall
LDFLAGS = -I/usr/include/libdrm \
-I/usr/include/dlog \
-I/usr/include/xorg \
-I/usr/include/pixman-1 \
-I/usr/include/xdbg \
-I/usr/include/exynos \
-I./src \
-I./src/accel \
-I./src/display \
-I./src/util \
-I./src/xv \
-I./src/g2d \
-I./src/tests \
-I./src/fakes

DFLAGS = -DPACKAGE_VERSION_MAJOR=1 -DPACKAGE_VERSION_MINOR=1 -DPACKAGE_VERSION_PATCHLEVEL=1 -DXORG_VERSION_CURRENT=123 -DTEST_COMPILE

SRCSC = ./src/main_test.c \
./src/xv/exynos_video_overlay.c \
./src/fakes/drm.c \
./src/fakes/tbm.c \
./src/fakes/other.c \
./src/fakes/mem.c \
./src/fakes/x_and_os.c \
./src/tests/test_display.c \
./src/tests/test_clone.c \
./src/tests/test_video_capture.c \
./src/tests/test_video_buffer.c \
./src/tests/test_video_converter.c \
./src/tests/test_video_image.c \
./src/tests/test_video_ipp.c \
./src/tests/test_video.c \
./src/tests/test_mem_pool.c \
./src/tests/test_util.c \
./src/tests/test_fimg2d.c \
./src/tests/test_util_g2d.c \
./src/tests/test_crtc.c \
./src/tests/test_output.c \
./src/tests/test_exa_sw.c \
./src/tests/test_accel.c \
./src/tests/test_dri2.c \
./src/tests/test_exa.c \
./src/tests/prepares.c \
./src/tests/test_plane.c

TARGET = test

all: $(SRCS)
	gcc -o ${TARGET} ${LDFLAGS} ${CFLAGS} ${SRCSC} ${DFLAGS} -lcheck -lpthread -lxdbg-log
#	gcc -o ${TARGET} ${LDFLAGS} ${CFLAGS} ${SRCSC} ${DFLAGS} -lcheck -lpthread -lxdbg-log -ltbm -ldrm
	
clean :
	rm -rf $(TARGET)
