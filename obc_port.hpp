#ifndef __OBC_PORT_H__
#define __OBC_PORT_H__

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>


#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <fcntl.h>



const size_t CHUNK_SIZE = 512;
const char* SERVER_IP = "192.168.68.2";
//const char* SERVER_IP = "10.241.90.10";
const int SERVER_PORT = 25565;
const size_t SHM_SIZE = 1024 * 1024;  // 1MB shared memory
                                      //const size_t CHUNK_SIZE = 512;


//TODO: remove the pragma or understand what it does
//TODO: explain the magic number
//TODO: to find if this is even nessicary
#pragma pack(push, 1)
struct Header {
    uint32_t magic = 0x12345678;
    uint32_t total_chunks;
    uint32_t mem_size;
};
#pragma pack(pop)


namespace OBCPort {

    //TODO: make other instances of this bool
    //bool camera_thread_started;
    //int started = 0;
    //TODO: change other instances of this to bool
    //bool quit_signal;

    void quit();

    //static int sockfd;

    //TODO: don't make this shit  when transfering over to
    // the cpp file
    // TODO: remove???
    extern struct sockaddr_in client_addr;
    //static socklen_t addrlen;

    bool send_image(void* img_addr, const size_t map_size);

    void start_camera_thread();

    void start_listener();

    void handle_command(char cmd);


};

#endif //__OBC_PORT_H__
