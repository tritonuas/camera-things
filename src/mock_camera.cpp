#include "mock_camera.hpp"
#include "simple-cam.hpp"
#include "obc_port.hpp"
#include <loguru/loguru.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb_image.h"

namespace MockCamera {

    void convert_rgb_to_yuv420(const unsigned char* rgb_data, int width, int height,
                               std::vector<uint8_t>& y_plane,
                               std::vector<uint8_t>& u_plane,
                               std::vector<uint8_t>& v_plane) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int r = rgb_data[(y * width + x) * 3 + 0];
                int g = rgb_data[(y * width + x) * 3 + 1];
                int b = rgb_data[(y * width + x) * 3 + 2];

                int y_val =  (0.299 * r) + (0.587 * g) + (0.114 * b);
                int u_val = -(0.147 * r) - (0.289 * g) + (0.436 * b) + 128;
                int v_val =  (0.615 * r) - (0.515 * g) - (0.100 * b) + 128;

                // Clamp values
                y_plane[y * width + x] = std::clamp(y_val, 0, 255);
                
                // Downsample UV (just taking top-left pixel of 2x2 block)
                if (y % 2 == 0 && x % 2 == 0) {
                    u_plane[(y / 2) * (width / 2) + (x / 2)] = std::clamp(u_val, 0, 255);
                    v_plane[(y / 2) * (width / 2) + (x / 2)] = std::clamp(v_val, 0, 255);
                }
            }
        }
    }

    void mock_thread_loop() {
        LOG_F(INFO, "Starting mock camera thread...");
        
        std::string mock_dir = RPICam::config.mock_image_dir;
        std::vector<std::string> mock_images;
        
        try {
            for (const auto & entry : std::filesystem::directory_iterator(mock_dir)) {
                if (entry.path().extension() == ".jpg" || entry.path().extension() == ".png") {
                    mock_images.push_back(entry.path().string());
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            LOG_F(ERROR, "Filesystem error while reading mock directory: %s", e.what());
        }

        if (mock_images.empty()) {
            LOG_F(ERROR, "No mock images found in %s! Exiting mock thread.", mock_dir.c_str());
            return;
        }

        std::sort(mock_images.begin(), mock_images.end());
        LOG_F(INFO, "Found %zu mock images. Ready to stream.", mock_images.size());

        size_t current_image_idx = 0;
        int width = RPICam::config.width;
        int height = RPICam::config.height;

        while (true) {
            // Check if there are requests
            RPICam::send_count_mutex.lock();
            if (RPICam::send_count <= 0) {
                RPICam::send_count_mutex.unlock();
                // Sleep a bit to avoid maxing out CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            RPICam::send_count--;
            RPICam::send_count_mutex.unlock();

            std::string image_path = mock_images[current_image_idx];
            
            int img_w, img_h, channels;
            unsigned char *img_data = stbi_load(image_path.c_str(), &img_w, &img_h, &channels, 3); // Force 3 channels (RGB)
            
            if (!img_data) {
                LOG_F(ERROR, "Failed to load mock image: %s", image_path.c_str());
                current_image_idx = (current_image_idx + 1) % mock_images.size();
                continue;
            }

            if (img_w != width || img_h != height) {
                LOG_F(WARNING, "Image dimensions %dx%d do not match config %dx%d. Processing might be corrupted.", img_w, img_h, width, height);
            }

            // YUV420 Conversion
            size_t y_size = width * height;
            size_t uv_size = (width / 2) * (height / 2);
            
            std::vector<uint8_t> y_plane(y_size);
            std::vector<uint8_t> u_plane(uv_size);
            std::vector<uint8_t> v_plane(uv_size);

            convert_rgb_to_yuv420(img_data, width, height, y_plane, u_plane, v_plane);

            stbi_image_free(img_data);

            // Send 3 planes sequentially as required by the protocol
            OBCPort::send_image(y_plane.data(), y_plane.size());
            OBCPort::send_image(u_plane.data(), u_plane.size());
            OBCPort::send_image(v_plane.data(), v_plane.size());

            LOG_F(INFO, "Sent mock image: %s", image_path.c_str());

            current_image_idx = (current_image_idx + 1) % mock_images.size();
        }
    }

    void start_mock_thread() {
        std::thread mock_thread(mock_thread_loop);
        mock_thread.detach();
    }
}
