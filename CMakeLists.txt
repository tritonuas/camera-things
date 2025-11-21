cmake_minimum_required(VERSION 3.6)

project(RaspberryPi
    DESCRIPTION "A modular camera application for TritonUAS"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Winvalid-pch -Wnon-virtual-dtor -Wextra -Wno-unused-parameter")

# --- FIND SYSTEM DEPENDENCIES ---
find_package(PkgConfig)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
pkg_check_modules(LIBEVENT REQUIRED IMPORTED_TARGET libevent_pthreads)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# --- SETUP INCLUDE DIRECTORIES ---
include_directories(
    ${CMAKE_SOURCE_DIR}/include         # Your headers (simple-cam.hpp, etc.)
    ${CMAKE_SOURCE_DIR}/libs            # Library roots (for nlohmann)
    ${CMAKE_SOURCE_DIR}/libs/mavlink    # MAVLink root headers (mavlink_types.h)
    ${LIBCAMERA_INCLUDE_DIRS} 
    ${LIBEVENT_INCLUDE_DIRS} 
    ${LIBUSB_INCLUDE_DIRS}
)

# --- DEFINE THE EXECUTABLE ---
# We list the source files from the 'src' folder
add_executable(raspy 
    src/raspy.cpp 
    src/simple-cam.cpp 
    src/event_loop.cpp 
    src/mavlink.cpp
    src/obc_port.cpp
    src/helpers.cpp
    # src/udp_client.cpp   <-- Uncomment if you use this
)

# --- LINK LIBRARIES ---
target_link_libraries(raspy 
    PkgConfig::LIBEVENT
    PkgConfig::LIBCAMERA
    ${LIBUSB_LINK_LIBRARIES}
    pthread
)
