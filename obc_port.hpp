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



const size_t CHUNK_SIZE = 1024;
const char* SERVER_IP = "192.168.68.2";
const int SERVER_PORT = 25565;
const size_t SHM_SIZE = 1024 * 1024;  // 1MB shared memory
//const size_t CHUNK_SIZE = 512;


#pragma pack(push, 1)
struct Header {
    uint32_t magic = 0x12345678;
    uint32_t total_chunks;
    uint32_t mem_size;
};
#pragma pack(pop)


class OBCPort {

    public: 

        int camera_thread_started;
        static inline int started = 0;
        int quit_signal;

        void quit() {
            quit_signal = 1;
        }

        static inline int sockfd;

        //TODO: don't make this shit inline when transfering over to
        // the cpp file
        static inline sockaddr_in client_addr{};
        static inline socklen_t client_len;


        
        bool send_image(char* ptr, const size_t map_size) {
            size_t total_sent = 0;
            size_t remaining;
            while (total_sent < map_size) {
                remaining = map_size  - total_sent;
                size_t send_size = std::min(remaining, CHUNK_SIZE);

                ssize_t sent = sendto(sockfd, ptr + total_sent, send_size, 0,
                        (const struct sockaddr*)&client_addr, 
                        client_len);

                if (sent < 0) {
                    return false;
                }

                total_sent += sent;
            }

            return false;
        }


        void start_camera_thread() {
            std::thread cameraThread(RPICam::start);
            camera_thread_started = 1;
            cameraThread.detach();
        }

        void start_listener() {

            /**
            if (!camera_thread_started) {
                std::cout << "Camera thread needs to be started";
                return;
            }
            **/

            client_len = sizeof(client_addr);
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                std::cerr << "Socket creation failed" << std::endl;
                shm_unlink("/demo_shm");
                return;
            }

            /**
            // Bind socket
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(SERVER_PORT);
            inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

            if (bind(sockfd, (const sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Bind failed" << std::endl;
            close(sockfd);
            return;
            }

            std::cout << "Server ready listening to requets" << std::endl;
             **/

            // Set SO_REUSEADDR to allow port reuse
            int reuse = 1;
            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
                std::cerr << "Setsockopt SO_REUSEADDR failed: " << strerror(errno) << std::endl;
                close(sockfd);
                return;
            }

            // Bind socket
            sockaddr_in server_addr{};
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(SERVER_PORT);

            // Try binding to all interfaces first for testing
            server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Use this instead of specific IP

            // Alternatively, use specific IP (check if it's correct)
            // if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
            //     std::cerr << "Invalid IP address: " << strerror(errno) << std::endl;
            //     close(sockfd);
            //     return;
            // }

            if (bind(sockfd, (const sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Bind failed: " << strerror(errno) << std::endl;
                close(sockfd);
                return;
            }

            std::cout << "现在听：" << SERVER_IP << ":" << SERVER_PORT << std::endl;

            // Wait for request
            char request[1];

            quit_signal = 0;
            while (!quit_signal) {

                std::cout << "在等" << std::endl;
                recvfrom(sockfd, request, sizeof(request), 0,
                        (sockaddr*)&client_addr, &client_len);
                std::cout << "受到了!" << std::endl;
                
                //(s)tart
                if (request[0] == 's') {
                    std::cout << "开始！";
                    start_camera_thread();
                }
                
                //(p)icture
                if (request[0] == 'p') {
                    RPICam::send_count_mutex.lock();
                    RPICam::send_count++;
                    RPICam::send_count_mutex.unlock();
                    std::cout << "照相！";
                }

                //(e)nd
                if (request[0] == 'e') {
                    std::cout << "结束";
                    quit();
                }

                //(l)ock controls
                if (request[0] == 'l') {
                    std::cout << "lock the controls";
                    //TODO: lock the controls
                }

            }

            std::cout << "server关了";
            close(sockfd);
        }
        /**
        // Prepare and send header
        Header header;
        header.total_chunks = (SHM_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;
        header.mem_size = SHM_SIZE;

        Header net_header = header;
        net_header.magic = htonl(net_header.magic);
        net_header.total_chunks = htonl(net_header.total_chunks);
        net_header.mem_size = htonl(net_header.mem_size);

        sendto(sockfd, &net_header, sizeof(net_header), 0,
        (const sockaddr*)&client_addr, client_len);

        // Send memory chunks
        for (uint32_t i = 0; i < header.total_chunks; ++i) {
        const size_t offset = i * CHUNK_SIZE;
        const size_t remaining = SHM_SIZE - offset;
        const size_t send_size = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;

        std::vector<char> chunk(sizeof(uint32_t) + send_size);
         *(uint32_t*)chunk.data() = htonl(i);
         memcpy(chunk.data() + sizeof(uint32_t), shm_ptr + offset, send_size);

         sendto(sockfd, chunk.data(), chunk.size(), 0,
         (const sockaddr*)&client_addr, client_len);
         }

         std::cout << "Memory region sent" << std::endl;

        // Cleanup
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink("/demo_shm");
        close(sockfd);
        return 0;
         **/
};

#endif
