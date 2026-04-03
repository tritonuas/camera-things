#ifndef __OBC_PORT_H__
#define __OBC_PORT_H__

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <deque>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <cstring>

// TODO: make these config variables from JSON
inline const size_t CHUNK_SIZE = 512;
inline const char* SERVER_IP = "192.168.77.2"; 
inline const int SERVER_PORT = 25565;
inline const size_t SHM_SIZE = 1024 * 1024; 

#pragma pack(push, 1)
struct Header {
    uint32_t magic = 0x12345678;
    uint32_t total_chunks;
    uint32_t mem_size;
};
#pragma pack(pop)

namespace OBCPort {
    void quit();
    extern struct sockaddr_in client_addr;
    bool send_image(void* img_addr, const size_t map_size);
    void start_camera_thread();
    void start_listener();
    void return_ping(char ping_id);
    void handle_command(char cmd);
};

#endif //__OBC_PORT_H__