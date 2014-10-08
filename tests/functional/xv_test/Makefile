export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/share/pkgconfig
export LD_LIBRARY_PATH=/usr/lib

CC = gcc
CFLAGS = -g -O0 -Wall
LDFLAGS = -lpthread `pkg-config xcb xcb-xv xcb-shm xcb-util xcb-dri2 xcb-atom libtbm --cflags --libs`

SRCS = xv_test.c test_xv_resize.c data.c xcb_api.c

TARGET = test_xv

all: $(SRCS)
	$(CC) ${CFLAGS} -o $(TARGET) $(SRCS) $(LDFLAGS)

clean :
	rm -rf $(TARGET)
