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

#define VENDOR_ID 0x1234       // Replace with your device's VID
#define PRODUCT_ID 0x5678      // Replace with your device's PID
#define TIMEOUT_MS 5000
#define IMAGE_PATH "image.jpg"

struct EndpointInfo {
    uint8_t address;
    uint16_t max_packet_size;
};



class OBCPort {

    public: 

        int camera_thread_started;

        bool find_endpoints(libusb_device_handle* handle, EndpointInfo& interrupt_in, EndpointInfo& bulk_out) {
            libusb_device* device = libusb_get_device(handle);
            libusb_config_descriptor* config = nullptr;

            if (libusb_get_active_config_descriptor(device, &config) != LIBUSB_SUCCESS) {
                return false;
            }

            for (int i = 0; i < config->bNumInterfaces; i++) {
                const libusb_interface& interface = config->interface[i];
                for (int j = 0; j < interface.num_altsetting; j++) {
                    const libusb_interface_descriptor& alt = interface.altsetting[j];
                    for (int k = 0; k < alt.bNumEndpoints; k++) {
                        const libusb_endpoint_descriptor& ep = alt.endpoint[k];

                        if ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_INTERRUPT &&
                                (ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
                            interrupt_in.address = ep.bEndpointAddress;
                            interrupt_in.max_packet_size = ep.wMaxPacketSize;
                        }
                        else if ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK &&
                                (ep.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
                            bulk_out.address = ep.bEndpointAddress;
                            bulk_out.max_packet_size = ep.wMaxPacketSize;
                        }
                    }
                }
            }

            libusb_free_config_descriptor(config);
            return (interrupt_in.address != 0 && bulk_out.address != 0);
        }


        /**
          bool send_mapped_memory(libusb_device_handle* handle, 
          const EndpointInfo& bulk_out,
          const unsigned char* mapped_data,
          size_t data_size) {
          size_t total_sent = 0;

          while (total_sent < data_size) {
          int chunk_size = std::min(static_cast<size_t>(bulk_out.max_packet_size),
          data_size - total_sent);
          int actual_sent;

          int result = libusb_bulk_transfer(handle,
          bulk_out.address,
          const_cast<unsigned char*>(&mapped_data[total_sent]),
          chunk_size,
          &actual_sent,
          TIMEOUT_MS);

          if (result != LIBUSB_SUCCESS) {
          std::cerr << "Bulk transfer error: " << libusb_error_name(result) << std::endl;
          return false;
          }
          total_sent += actual_sent;
          }

          std::cout << "Memory sent successfully (" << total_sent << " bytes)" << std::endl;
          return true;
          }
          */


        bool send_image(libusb_device_handle* handle, const EndpointInfo& bulk_out) {
            std::ifstream file(IMAGE_PATH, std::ios::binary | std::ios::ate);
            if (!file) {
                std::cerr << "Failed to open image file" << std::endl;
                return false;
            }

            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<unsigned char> buffer(size);
            if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                std::cerr << "Failed to read image file" << std::endl;
                return false;
            }

            size_t total_sent = 0;
            while (total_sent < buffer.size()) {
                int chunk_size = std::min(static_cast<size_t>(bulk_out.max_packet_size), 
                        buffer.size() - total_sent);
                int actual_sent;

                int result = libusb_bulk_transfer(handle,
                        bulk_out.address,
                        &buffer[total_sent],
                        chunk_size,
                        &actual_sent,
                        TIMEOUT_MS);

                if (result != LIBUSB_SUCCESS) {
                    std::cerr << "Bulk transfer error: " << libusb_error_name(result) << std::endl;
                    return false;
                }
                total_sent += actual_sent;
            }

            std::cout << "Image sent successfully (" << total_sent << " bytes)" << std::endl;
            return true;
        }


        void start_camera_thread() {
            std::thread cameraThread(RPICam::start);
            camera_thread_started = 1;
            cameraThread.detach();
        }
            

        void start_listener() {

            if (!camera_thread_started) {
                std::cout << "Camera thread needs to be started";
                return;
            }

            libusb_device_handle* handle;
            EndpointInfo interrupt_in;
            EndpointInfo bulk_out;
            int started = 0;
            if (started) {
                return;
            }


            started = 1;
            libusb_init(nullptr);
            libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

            handle = libusb_open_device_with_vid_pid(nullptr, VENDOR_ID, PRODUCT_ID);
            if (!handle) {
                std::cerr << "Device not found" << std::endl;
                return;
            }

            if (libusb_kernel_driver_active(handle, 0) == 1) {
                libusb_detach_kernel_driver(handle, 0);
            }

            if (libusb_claim_interface(handle, 0) != LIBUSB_SUCCESS) {
                std::cerr << "Could not claim interface" << std::endl;
                libusb_close(handle);
                return;
            }

            interrupt_in.address = 0;
            interrupt_in.max_packet_size = 0;
            bulk_out.address = 0;
            bulk_out.max_packet_size = 0;
            if (!find_endpoints(handle, interrupt_in, bulk_out)) {
                std::cerr << "Could not find required endpoints" << std::endl;
                libusb_release_interface(handle, 0);
                libusb_close(handle);
                return;
            }

            std::cout << "Server started. Waiting for requests..." << std::endl;

            std::vector<unsigned char> buffer(interrupt_in.max_packet_size);
            while (true) {
                int actual_length;
                int result = libusb_interrupt_transfer(handle,
                        interrupt_in.address,
                        buffer.data(),
                        buffer.size(),
                        &actual_length,
                        TIMEOUT_MS);

                if (result == LIBUSB_SUCCESS && actual_length > 0) {
                    std::cout << "Request received. Sending image..." << std::endl;
                    if (send_image(handle, bulk_out)) {
                        std::cout << "Ready for next request" << std::endl;
                    }
                }
                else if (result != LIBUSB_ERROR_TIMEOUT) {
                    std::cerr << "Interrupt transfer error: " << libusb_error_name(result) << std::endl;
                    break;
                }
            }

            started = 0;

            libusb_release_interface(handle, 0);
            libusb_close(handle);
            libusb_exit(nullptr);
            return;
        }
};

#endif
