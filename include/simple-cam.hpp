#ifndef INCLUDE_SIMPLE_CAM_
#define INCLUDE_SIMPLE_CAM_

#include <iomanip>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <list>
#include <algorithm>
#include <mutex>
#include <chrono>
#include <ctime>
#include <libcamera/libcamera.h>

#include "event_loop.h"
#include "mavlink.hpp"
#include "function_queue.hpp"
#include "obc_port.hpp"


using namespace libcamera;

namespace RPICam {
    inline EventLoop loop;
    inline std::mutex mutex;
    inline int send_count;
    inline std::mutex send_count_mutex;
    inline int send_current = 0;

    inline const int save_to_file = 1;
    inline int image_counter = 0;

    inline int debug = 0;

    inline int settings_locked;
    inline int save_settings;

    inline int print_metadata = 0;
    inline int mavlink_enabled = 0;
    inline int send_to_obc = 0;
    
    inline const unsigned int invalid_controls[] = {0x18, 0x1a};

    struct CameraConfig {
        int width = 1456;
        int height = 1088;
        std::string pixel_format = "YUV420";
        int buffer_count = 3;
        int timeout = 10000;
        bool mock_mode = false;
        std::string mock_image_dir = "../images/mock";
    };
    inline CameraConfig config;

    inline functionQueue funQ;
    inline std::vector<std::unique_ptr<Request>> requests;
    inline std::shared_ptr<Camera> camera;

    void saveData(Request *request);
    void processRequest(Request *request); 
    void requestComplete(Request *request); 
    void lock_settings();
    void unlock_settings();
    std::string cameraName(Camera *camera);
    void send_next_image(); 
    void start();
    void stop_taking_pictures();
    void copy_controls(libcamera::ControlList *list_out, libcamera::ControlList *list_in, int all);

    inline ControlList controlList_copy;
    inline int image_count = 0;
};
#endif