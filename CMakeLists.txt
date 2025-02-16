cmake_minimum_required(VERSION 3.6)

project(RaspberryPi
	DESCRIPTION "A small and documented example application for libcamera"
	LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "-Wall -Winvalid-pch -Wnon-virtual-dtor -Wextra -Werror -Wno-unused-parameter")

find_package(PkgConfig)
# find_package(libusb-1.0 REQUIRED)

pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
message(STATUS "libcamera library found:")
message(STATUS "    version: ${LIBCAMERA_VERSION}")
message(STATUS "    libraries: ${LIBCAMERA_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBCAMERA_INCLUDE_DIRS}")

# libevent is used specifically by simple-cam as its event loop.
# Applications may use a different event handling implementation.
pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent_pthreads)
message(STATUS "libevent_pthreads library found:")
message(STATUS "    version: ${LIBEVENT_VERSION}")
message(STATUS "    libraries: ${LIBEVENT_LINK_LIBRARIES}")
message(STATUS "    include path: ${LIBEVENT_INCLUDE_DIRS}")

pkg_check_modules(LIBUSB REQUIRED libusb-1.0)


include_directories(${CMAKE_SOURCE_DIR} 
    ${LIBCAMERA_INCLUDE_DIRS} ${LIBEVENT_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS})

add_executable(raspy raspy.cpp simple-cam.hpp event_loop.cpp mavlink.hpp)

target_link_libraries(raspy PkgConfig::LIBEVENT)
target_link_libraries(raspy PkgConfig::LIBCAMERA)

target_link_libraries(raspy ${LIBUSB_LINK_LIBRARIES})
