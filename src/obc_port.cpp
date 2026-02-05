// obc_port.cpp
#include "obc_port.hpp"
#include "simple-cam.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <iostream>
#include <thread>

namespace OBCPort {
    // Thread-safe globals
    std::atomic<int> server_sock(-1);
    std::atomic<bool> quit_signal(false);
    std::atomic<bool> camera_thread_started(false);

    // Protected client information
    std::mutex client_mutex;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(sockaddr_in);

    void quit() {
        quit_signal = true;
        if(int sock = server_sock.exchange(-1); sock != -1) {
            close(sock);
        }
    }

    // Define the header structure with packing to ensure correct byte alignment
#pragma pack(push, 1)
    struct Header {
        uint32_t magic = 0x12345678;
        uint32_t total_chunks;
        uint32_t mem_size;
    };
#pragma pack(pop)
    bool send_image(void* img_addr, const size_t map_size) {
        struct sockaddr_in current_client_addr;
        socklen_t current_addr_len;
        int current_sock;

        {
            std::lock_guard<std::mutex> lock(client_mutex);
            current_sock = server_sock.load();
            if (current_sock == -1) return false;
            current_client_addr = client_addr;
            current_addr_len = addr_len;
        }

        const size_t CHUNK_SIZE = 1024; // Match the first code's chunk size
        const size_t total_chunks = (map_size + CHUNK_SIZE - 1) / CHUNK_SIZE;


        // Prepare and send the header
        Header header;
        header.magic = 0x12345678;
        header.total_chunks = total_chunks;
        header.mem_size = map_size;

        // Convert header fields to network byte order
        Header net_header = header;
        net_header.magic = htonl(net_header.magic);
        net_header.total_chunks = htonl(net_header.total_chunks);
        net_header.mem_size = htonl(net_header.mem_size);

        // Send the header
        ssize_t sent_header = sendto(current_sock, &net_header, sizeof(net_header), 0,
                (struct sockaddr*)&current_client_addr, current_addr_len);
        std::cout << "sent the header";

        if (sent_header != sizeof(net_header)) {
            perror("Failed to send header");
            return false;
        }

        // Send each chunk with only the chunk index as prefix
        for (uint32_t chunk_idx = 0; chunk_idx < total_chunks; ++chunk_idx) {
            // Check if the socket has changed during transmission
            if (server_sock.load() != current_sock) return false;

            const size_t offset = chunk_idx * CHUNK_SIZE;
            const size_t remaining = map_size - offset;
            const size_t data_size = std::min(CHUNK_SIZE, remaining);

            // Prepare chunk: 4-byte index + data
            std::vector<char> packet(sizeof(uint32_t) + data_size);
            uint32_t* index_ptr = reinterpret_cast<uint32_t*>(packet.data());
            *index_ptr = htonl(chunk_idx);
            memcpy(packet.data() + sizeof(uint32_t), static_cast<char*>(img_addr) + offset, data_size);

            // Send the chunk
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            ssize_t sent = sendto(current_sock, packet.data(), packet.size(), 0,
                    (struct sockaddr*)&current_client_addr, current_addr_len);
            if (sent < 0 || static_cast<size_t>(sent) != packet.size()) {
                perror("UDP chunk send failed");
                return false;
            }
        }

        return true;
    }
    void start_camera_thread() {
        if(camera_thread_started.exchange(true)) return;

        std::thread([](){
                try {
                RPICam::start();
                } catch(const std::exception& e) {
                std::cerr << "Camera thread failed: " << e.what() << "\n";
                camera_thread_started = false;
                }
                }).detach();
    }

    void start_listener() {
        if(server_sock != -1) return;

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0) {
            perror("Socket creation failed");
            return;
        }

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(SERVER_PORT);

        if(bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Bind failed");
            close(sock);
            return;
        }

        server_sock = sock;
        std::cout << "UDP server ready on port " << SERVER_PORT << "\n";

        char buffer[1024];
        while(!quit_signal) {
            sockaddr_in tmp_addr;
            socklen_t tmp_len = sizeof(tmp_addr);

            ssize_t len = recvfrom(server_sock, buffer, sizeof(buffer), 0,
                    (struct sockaddr*)&tmp_addr, &tmp_len);

            if(len > 0) {
                {
                    std::lock_guard<std::mutex> lock(client_mutex);
                    client_addr = tmp_addr;
                    addr_len = tmp_len;
                }
                handle_command(buffer[0]);
            } else if(len < 0 && !quit_signal) {
                perror("recvfrom error");
            }
        }

        if(int sock = server_sock.exchange(-1); sock != -1) {
            close(sock);
        }
    }
    /**
     * @brief Processes received UDP commands
     * @param cmd The single-character command received
     * 
     * Handles command execution similar to TCP version but in connectionless context
     */
    void handle_command(char cmd) {
        switch(cmd) {
            case 's':  // Start camera
                std::cout << "Camera start command received\n";
                start_camera_thread();
                break;

            case 'I':  // Image request
                std::cout << "Image capture requested\n";
                RPICam::send_count_mutex.lock();
                RPICam::send_count++;
                RPICam::send_count_mutex.unlock();
                std::cout << "prkajsdkajskljasd\n";
                break;

            case 'e':  // Exit command
                std::cout << "Shutdown command received\n";
                quit();
                break;

            case 'l':  // Lock controls
                std::cout << "Control lock command (not implemented)\n";
                //TODO: Implement control locking
                break;
        }
    }
};
