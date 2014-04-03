enable_testing()
message(STATUS "Test output" ${CMAKE_MODULE_PATH})
find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
# Source

include_directories(. ./fakes/)

# Main

list (APPEND SRC ./main_test.c)
list (APPEND SRC ./prepares.c)

# Fakes functions

list (APPEND SRC ./fakes/drm.c)
list (APPEND SRC ./fakes/tbm.c)
list (APPEND SRC ./fakes/other.c)
list (APPEND SRC ./fakes/mem.c)
list (APPEND SRC ./fakes/x_and_os.c)
list (APPEND SRC ./fakes/gc.c)
# Tests
list (APPEND SRC ./test_display.c)
list (APPEND SRC ./test_clone.c)
list (APPEND SRC ./test_video_capture.c)
list (APPEND SRC ./test_video_buffer.c)
list (APPEND SRC ./test_video_converter.c)
list (APPEND SRC ./test_video_image.c)
list (APPEND SRC ./test_video_ipp.c)
list (APPEND SRC ./test_video.c)
list (APPEND SRC ./test_mem_pool.c)
list (APPEND SRC ./test_util.c)
list (APPEND SRC ./test_fimg2d.c)
list (APPEND SRC ./test_util_g2d.c)
list (APPEND SRC ./test_crtc.c)
list (APPEND SRC ./test_output.c)
list (APPEND SRC ./test_exa_sw.c)
list (APPEND SRC ./test_accel.c)
list (APPEND SRC ./test_dri2.c)
list (APPEND SRC ./test_exa.c)
list (APPEND SRC ./test_plane.c)

#Other (need fix)

list (APPEND SRC ../src/accel/exynos_dri2.c)
list (APPEND SRC ../src/xv/exynos_video_overlay.c)
get_directory_property(HEADERS_DRV DIRECTORY ../src DEFINITION HEADERS)
get_directory_property(LIBS_DRV DIRECTORY ../src DEFINITION LIBS)
include_directories(${HEADERS_DRV})
add_definitions(-DHAVE_CONFIG_H=1)
add_definitions(-DTEST_COMPILE)
add_definitions(-DPACKAGE_VERSION_MAJOR=1)
add_definitions(-DPACKAGE_VERSION_MINOR=1)
add_definitions(-DPACKAGE_VERSION_PATCHLEVEL=1)
add_definitions(-DXORG_VERSION_CURRENT=123)
add_executable(exynos_drv_test ${SRC})
target_link_libraries(exynos_drv_test ${LIBS_DRV} check)
add_test(exynos_drv_test ${CMAKE_CURRENT_BINARY_DIR}/exynos_drv_test)
