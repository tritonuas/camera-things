#include <iostream>
#include <vector>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdint>
#include <string>
#include <chrono>
#include <cstring>

#pragma pack(push, 1)
struct Header {
    uint32_t magic;
    uint32_t total_chunks;
    uint32_t mem_size;
};
#pragma pack(pop

const uint32_t EXPECTED_MAGIC = 0x12345678;
const size_t CHUNK_SIZE = 1024;

class UDPClient {
    int sockfd;
    struct sockaddr_in servaddr;
    int request_counter = 0;

public:
    UDPClient(const char* ip, int port) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &servaddr.sin_addr);
    }

    void send_command(char cmd) {
        sendto(sockfd, &cmd, 1, 0, 
              (const struct sockaddr*)&servaddr, sizeof(servaddr));
    }

    void receive_and_save_buffer(int buffer_num) {
        Header header;
        socklen_t len = sizeof(servaddr);
        
        // Receive header
        ssize_t received = recvfrom(sockfd, &header, sizeof(header), 0,
                                  (struct sockaddr*)&servaddr, &len);
        std::cout << "Received header for buffer " << buffer_num 
                << ": " << received << " bytes\n";

        if (received != sizeof(header)) {
            std::cerr << "Header receive error" << std::endl;
            return;
        }

        // Convert network byte order
        header.magic = ntohl(header.magic);
        header.total_chunks = ntohl(header.total_chunks);
        header.mem_size = ntohl(header.mem_size);

        if (header.magic != EXPECTED_MAGIC) {
            std::cerr << "Invalid magic number" << std::endl;
            return;
        }

	std::cout << "The expected size of the thing is: " << header.mem_size << "\n";

        // Prepare buffer and receive chunks
        std::vector<char> buffer_data(header.mem_size);
        char chunk_buffer[CHUNK_SIZE + sizeof(uint32_t)];
        size_t total_received = 0;

        for (uint32_t i = 0; i < header.total_chunks; i++) {
            ssize_t chunk_size = recvfrom(sockfd, chunk_buffer, sizeof(chunk_buffer), 0,
                                        (struct sockaddr*)&servaddr, &len);
            
            total_received += chunk_size;

            if (chunk_size > 2000) {
                std::cerr << "Invalid chunk received: " << chunk_size << std::endl;
                return;
            }

	    if (chunk_size <= -1) {
		    std::cout << "Chunk size negative, wtf\n";
	    }

            uint32_t chunk_idx = ntohl(*reinterpret_cast<uint32_t*>(chunk_buffer));
            uint64_t data_size = chunk_size - sizeof(uint32_t);
            uint64_t offset = chunk_idx * CHUNK_SIZE;

	    //std::cout << total_received << "\n";

            if (offset + data_size > header.mem_size) {
		std::cout << "Size of the data: " << (offset) << "\n";
                std::cerr << "Chunk exceeds buffer bounds" << std::endl;
                return;
            }

            memcpy(buffer_data.data() + offset, 
                 chunk_buffer + sizeof(uint32_t), 
                 data_size);
        }

        // Save to file
        std::string filename = "out/" + std::to_string(request_counter) + "." + 
                             std::to_string(buffer_num) + ".dat";
        std::ofstream file(filename, std::ios::binary);
        file.write(buffer_data.data(), buffer_data.size());
        std::cout << "Saved " << buffer_data.size() 
                << " bytes to " << filename << "\n";
    }

    void request_image() {
        request_counter++;
        std::cout << "\n=== Starting request #" << request_counter << " ===\n";
        
        send_command('I');
        
        // Receive and save all three buffers
        for (int i = 1; i <= 3; i++) {
            receive_and_save_buffer(i);
        }
        
        std::cout << "=== Completed request #" << request_counter << " ===\n\n";
    }

    void continuous_capture(int interval_sec = 0) {
        while(true) {
            request_image();
            sleep(interval_sec);
        }
    }

    ~UDPClient() {
        close(sockfd);
    }
};

int main() {
    UDPClient client("192.168.68.1", 25565);
    client.continuous_capture();
    return 0;
}


