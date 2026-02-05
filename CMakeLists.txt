cmake_minimum_required(VERSION 3.6)

project(RaspberryPi
    DESCRIPTION "Camera-things"
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
    ${CMAKE_SOURCE_DIR}/include        
    ${CMAKE_SOURCE_DIR}/libs            
    ${CMAKE_SOURCE_DIR}/libs/mavlink    
    ${LIBCAMERA_INCLUDE_DIRS} 
    ${LIBEVENT_INCLUDE_DIRS} 
    ${LIBUSB_INCLUDE_DIRS}
)

# --- DEFINE THE EXECUTABLE ---
# aka source files from src/ we wanna use
add_executable(raspy 
    src/raspy.cpp 
    src/simple-cam.cpp 
    src/event_loop.cpp 
    src/mavlink.cpp
    src/obc_port.cpp
    libs/loguru/loguru.cpp
    # src/udp_client.cpp   # Not used rn
)

# --- LINK LIBRARIES ---
target_link_libraries(raspy 
    PkgConfig::LIBEVENT
    PkgConfig::LIBCAMERA
    ${LIBUSB_LINK_LIBRARIES}
    pthread
    dl
)
