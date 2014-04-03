set (VERSION_MAJOR 0)
set (VERSION_MINOR 0)
set (VERSION_PATCHLEVEL 1)
set (SRC_DIR ${CMAKE_SOURCE_DIR}/src)
# Source

list (APPEND HEADERS ${SRC_DIR}/)
list (APPEND SRC ${SRC_DIR}/exynos_driver.c)

# display
list (APPEND HEADERS ${SRC_DIR}/display/)
list (APPEND SRC ${SRC_DIR}/display/exynos_display.c)
list (APPEND SRC ${SRC_DIR}/display/exynos_crtc.c)
list (APPEND SRC ${SRC_DIR}/display/exynos_output.c)
list (APPEND SRC ${SRC_DIR}/display/exynos_clone.c)
list (APPEND SRC ${SRC_DIR}/display/exynos_plane.c)

# accel
list (APPEND HEADERS ${SRC_DIR}/accel/)
list (APPEND SRC ${SRC_DIR}/accel/exynos_accel.c)
list (APPEND SRC ${SRC_DIR}/accel/exynos_exa.c)
list (APPEND SRC ${SRC_DIR}/accel/exynos_exa_sw.c)
list (APPEND SRC ${SRC_DIR}/accel/exynos_dri2.c)

# xv
list (APPEND HEADERS ${SRC_DIR}/xv/)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_image.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_capture.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_buffer.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_converter.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_ipp.c)
list (APPEND SRC ${SRC_DIR}/xv/exynos_video_overlay.c)

# util

list (APPEND HEADERS ${SRC_DIR}/util)
list (APPEND SRC ${SRC_DIR}/util/exynos_util.c)
list (APPEND SRC ${SRC_DIR}/util/exynos_mem_pool.c)

# g2d
list (APPEND HEADERS ${SRC_DIR}/g2d)
list (APPEND SRC ${SRC_DIR}/g2d/util_g2d.c)
list (APPEND SRC ${SRC_DIR}/g2d/fimg2d.c)


# *************** Checking modules
PKG_CHECK_MODULES(PIXMAN pixman)
	if(NOT PIXMAN_FOUND)
  		PKG_CHECK_MODULES(PIXMAN REQUIRED pixman-1)
	endif(NOT PIXMAN_FOUND)
PKG_CHECK_MODULES(XDBG xdbg)
if(XDBG_FOUND)
    PKG_CHECK_MODULES(DLOG dlog)
endif(XDBG_FOUND)
PKG_CHECK_MODULES(LIBTBM REQUIRED libtbm)
PKG_CHECK_MODULES(XORG REQUIRED xorg-server)
PKG_CHECK_MODULES(LIBDRM REQUIRED libdrm)
PKG_CHECK_MODULES(DRM_EXYNOS REQUIRED libdrm_exynos)
PKG_CHECK_MODULES(UDEV libudev)
# @todo Rework pciaccess dependences
PKG_CHECK_MODULES(PCIACCESS pciaccess)

# ************** Adding external headers path
list (APPEND HEADERS ${PIXMAN_INCLUDE_DIRS})
list (APPEND HEADERS ${XDBG_INCLUDE_DIRS})
list (APPEND HEADERS ${LIBTBM_INCLUDE_DIRS})
list (APPEND HEADERS ${XORG_INCLUDE_DIRS})
list (APPEND HEADERS ${LIBDRM_INCLUDE_DIRS})
list (APPEND HEADERS ${DRM_EXYNOS_INCLUDE_DIRS})
list (APPEND HEADERS ${DLOG_INCLUDE_DIRS})
list (APPEND HEADERS ${UDEV_INCLUDE_DIRS})
list (APPEND HEADERS ${PCIACCESS_INCLUDE_DIRS})
include_directories(${HEADERS})

# ************** Adding external libs path
list (APPEND LIBS_PATH ${PIXMAN_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${XDBG_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${LIBTBM_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${XORG_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${DRM_EXYNOS_LIBDRM_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${DLOG_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${UDEV_LIBRARY_DIRS})
list (APPEND LIBS_PATH ${PCIACCESS_LIBRARY_DIRS})
link_directories(${LIBS_PATH})

# ************* Adding external libraries

list (APPEND LIBS ${PIXMAN_LIBRARIES})
if(XDBG_FOUND AND DLOG_FOUND)
    list (APPEND LIBS ${XDBG_LIBRARIES})
    list (APPEND LIBS ${DLOG_LIBRARIES})
endif(XDBG_FOUND AND DLOG_FOUND)
list (APPEND LIBS ${LIBTBM_LIBRARIES})
list (APPEND LIBS ${XORG_LIBRARIES})
list (APPEND LIBS ${DRM_EXYNOS_LIBRARIES})
list (APPEND LIBS ${UDEV_LIBRARIES})
list (APPEND LIBS ${PCIACCESS_LIBRARIES})
# @todo Check RT lib
list (APPEND LIBS rt)

# ************* Setup config.h

if(UDEV_FOUND)
	set(HAVE_UDEV 1)
endif(UDEV_FOUND)

if(XDBG_FOUND AND DLOG_FOUND)
    set(USE_XDBG_EXTERNAL 1)
    message(STATUS "Using external log module")
else(XDBG_FOUND AND DLOG_FOUND)
    message(STATUS "Using internal log module")
endif(XDBG_FOUND AND DLOG_FOUND)

if(PCIACCESS_FOUND)
	set(HAVE_PCIACCESS 1)
endif(PCIACCESS_FOUND)

find_path(INCLUDE_PATH NAMES dlfcn.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_DLFCN_H TRUE) 
endif()

find_path(INCLUDE_PATH NAMES inttypes.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_INTTYPES_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES memory.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_MEMORY_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES stdint.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_STDINT_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES stdlib.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_STDLIB_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES string.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_STRING_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES strings.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_STRINGS_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES stat.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_SYS_STAT_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES types.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_SYS_TYPES_H TRUE)
endif()

find_path(INCLUDE_PATH NAMES unistd.h)
if (NOT INCLUDE_PATH_NOTFOUND)
    set(HAVE_UNISTD_H TRUE)
endif()

#TODO add check STDC later
set (STDC_HEADERS 1)

set (PACKAGE "${PROJECT_NAME}")
set (PACKAGE_BUGREPORT "http://")
set (PACKAGE_NAME "${PROJECT_NAME}")
set (PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCHLEVEL}")
set (PACKAGE_STRING "${PACKAGE} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "${PACKAGE}")
set (PACKAGE_URL "http://")
set (PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set (PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set (PACKAGE_VERSION_PATCHLEVEL ${VERSION_PATCHLEVEL})
set (VERSION ${PACKAGE_VERSION})

configure_file(${CMAKE_SOURCE_DIR}/config.h.cmake ${SRC_DIR}/config.h)

add_definitions(-DHAVE_CONFIG_H=1)

add_library(exynos_drv SHARED ${SRC})
target_link_libraries(exynos_drv ${LIBS})
set_target_properties(exynos_drv PROPERTIES PREFIX "")
set(LIB_INSTALL_DIR, ${CMAKE_INSTALL_LIBDIR})
install(TARGETS exynos_drv DESTINATION ${LIB_INSTALL_DIR}/xorg/modules/drivers/)
