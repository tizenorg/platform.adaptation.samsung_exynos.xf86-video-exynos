cmake_minimum_required(VERSION 2.8)
project(xserver-xorg-video-exynos C)
set(CMAKE_VERBOSE_MAKEFILE TRUE)
message(STATUS "Test output")
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules")

# Library check
find_package(PkgConfig)
if(NOT PKG_CONFIG_FOUND)
    message(SEND_ERROR "Can't find FindPkgConfig.cmake")
    return()
endif()

enable_testing()
add_subdirectory(src)

configure_file(${CMAKE_SOURCE_DIR}/etc/exynos_drv.conf
               ${CMAKE_BINARY_DIR}/10-exynos.conf COPYONLY)

install(FILES ${CMAKE_BINARY_DIR}/10-exynos.conf
       DESTINATION ${SYSCONF_INSTALL_DIR}/X11/xorg.conf.d/)
#add_subdirectory(tests)

