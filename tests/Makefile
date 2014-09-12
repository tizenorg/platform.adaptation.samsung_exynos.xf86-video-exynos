#export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig
#export LD_LIBRARY_PATH=/usr/lib

CCP=gcc
CFLAGS= -g -O0 -w
LDFLAGS= -I./dependencies/X/hw/xfree86/ddc \
-I./dependencies/X/mi \
-I./dependencies/X/hw/xfree86/common \
-I./dependencies/X/hw/xfree86/os-support \
-I./dependencies/X/fb \
-I./dependencies/X/hw/xfree86/ramdac \
-I./dependencies/X/include \
-I./dependencies/X/hw/xfree86 \
-I./dependencies/X/Xext \
-I./dependencies/X \
-I./dependencies/X/hw/xfree86/modes \
-I./dependencies/X/miext/damage \
-I./dependencies/X/render \
-I./dependencies/X/randr \
-I./dependencies/X/hw/xfree86/parser \
-I./dependencies/X/hw/xfree86/i2c \
-I./dependencies/X/include \
-I./dependencies/X/exa \
-I./dependencies/X/hw/xfree86/dri2 \
-I./dependencies/X/hw/xfree86/fbdevhw \
-I./dependencies/xdbg/lib \
-I./dependencies/udev/src \
-I./dependencies/tbm \
-I./dependencies/tbm/src \
-I./dependencies/drm/include/drm \
-I./dependencies/drm \
-I./dependencies/pixman/pixman \
-I./dependencies \
-I./usr/include \
-I./fakes \
-I../src \
-I../src/accel \
-I../src/crtcconfig \
-I../src/debug \
-I../src/g2d \
-I../src/ipp \
-I../src/memory \
-I../src/neon \
-I../src/util \
-I../src/xv 


DFLAGS = -DPACKAGE_VERSION_MAJOR=1 \
 -DPACKAGE_VERSION_MINOR=1 \
 -DPACKAGE_VERSION_PATCHLEVEL=1 \
 -DXORG_VERSION_CURRENT=123 \
 -DTEST_COMPILE \
 -DXACE \
 -DDRM_MODE_CONNECTOR_VIRTUAL=15 \
 -DCREATE_PIXMAP_USAGE_OVERLAY=5

SRCSC= ./main.c \
./fakes/fake_drm.c \
./fakes/fake_tbm.c \
./fakes/fake_other.c \
./fakes/fake_mem.c \
./fakes/fake_x_and_os.c \
./fakes/fake_udev.c \
./fakes/fake_pixman.c \
./fakes/fake_xdbg.c \
./src/test_sec.c \
./src/accel/test_sec_dri2.c \
./src/accel/test_sec_exa_g2d.c \
./src/accel/test_sec_exa_sw.c \
./src/accel/test_sec_exa.c \
./src/crtcconfig/test_sec_crtc.c \
./src/crtcconfig/test_sec_display.c \
./src/crtcconfig/test_sec_dummy.c \
./src/crtcconfig/test_sec_layer.c \
./src/crtcconfig/test_sec_output.c \
./src/crtcconfig/test_sec_plane.c \
./src/crtcconfig/test_sec_prop.c \
./src/crtcconfig/test_sec_xberc.c \
./src/debug/test_sec_drmmode_dump.c \
./src/g2d/test_fimg2d.c \
./src/g2d/test_util_g2d.c \
./src/ipp/test_sec_converter.c \
./src/ipp/test_sec_drm_ipp.c \
./src/ipp/test_sec_wb.c \
./src/neon/test_copy_area.c \
./src/util/test_sec_util.c \
./src/xv/test_sec_video.c \
./src/xv/test_sec_video_display.c \
./src/xv/test_sec_video_tvout.c  \
./src/xv/test_sec_video_virtual.c
 

TARGET=test

all : $(SRCS)
	$(CCP) -o $(TARGET) $(LDFLAGS) $(CFLAGS) ${SRCSC} $(DFLAGS) `pkg-config --libs --cflags check`
	
clean :
	rm -rf $(TARGET)
