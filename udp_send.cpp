#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const char* DEST_IP = "192.168.68.2"; // Replace with Raspberry Pi's IP
const int DEST_PORT = 8888;          // Replace with your target port
const int BUFFER_SIZE = 1024;

int main() {
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // Configure destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    
    if (inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        close(sockfd);
        return 1;
    }

    // Create buffer with test data
    char buffer[BUFFER_SIZE] = "Hello from UDP!";
    size_t message_len = strlen(buffer);

    // Send the buffer
    ssize_t bytes_sent = sendto(sockfd, buffer, message_len, 0,
                               (const struct sockaddr*)&dest_addr, sizeof(dest_addr));
    
    if (bytes_sent < 0) {
        std::cerr << "Error sending data" << std::endl;
    } else {
        std::cout << "Sent " << bytes_sent << " bytes to " << DEST_IP 
                  << ":" << DEST_PORT << std::endl;
    }

    close(sockfd);
    return 0;
}
