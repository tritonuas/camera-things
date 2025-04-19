#include "obc_port.hpp"


namespace OBCPort {


    int client_sock;
    void quit() {
        quit_signal = 1;
    }

    bool send_image(void* data_ptr, const size_t map_size) {
        std::cout << "要发照片\n";
        //std::cout << "trying to send\n";
        char* ptr = static_cast<char*>(data_ptr);
        size_t total_sent = 0;
        //size_t remaining;
        int count = 0;
        while (total_sent < map_size && !quit_signal) {
            ssize_t sent = send(client_sock, ptr + total_sent, 
                    map_size - total_sent, 0);
            if (sent < 0) {
                std::cout << "wtf\n";
                perror("send failed");
                break;
            }
            total_sent += sent;
            std::cout << "啊啊啊";
        }

        std::cout<< "Total sent: " << total_sent << "\n";
        std::cout << "Count: " << count << "\n";

        return true;
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

        //Make socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket failed");
            return;
        }

        // set options

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int snd_buf = 1048576 * 5; // 1MB
        setsockopt(server_fd, SOL_SOCKET, SO_SNDBUF, &snd_buf, sizeof(snd_buf));


        std::cout << "Server ready listening to requets" << std::endl;



        // 4. Bind and listen
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(SERVER_PORT);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            close(server_fd);
            return;
        }

        listen(server_fd, 1);

        std::cout << "现在听：" << SERVER_IP << ":" << SERVER_PORT << std::endl;

        // Wait for request

        quit_signal = 0;
        while (!quit_signal) {


            std::cout << "Waiting for connection..." << std::endl;
            addrlen = sizeof(client_addr);
            client_sock = accept(server_fd, (sockaddr*)&client_addr, &addrlen);

            if (client_sock < 0) {
                perror("accept failed");
                continue;
            }

            // Set client socket options
            setsockopt(client_sock, SOL_SOCKET, SO_SNDBUF, &snd_buf, sizeof(snd_buf));
            std::cout << "Client connected. Waiting for requests..." << std::endl;


            while (!quit_signal) {

                std::cout << "在等" << std::endl;

                char request[4];
                ssize_t bytes = recv(client_sock, request, sizeof(request), 0);

                if (bytes <= 0) {
                    perror("Connection closed or error");
                    std::cout << "connection closed\n";
                    break;
                }


                std::cout << "受到了!" << std::endl;

                //(s)tart
                if (request[0] == 's') {
                    std::cout << "开始！";
                    start_camera_thread();
                }

                //(p)icture
                //TODO: make it actually sort
                if (request[0] == 'I') {
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
            close(client_sock);
        }
    }

};

