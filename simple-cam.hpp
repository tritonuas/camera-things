#ifndef INCLUDE_SIMPLE_CAM_
#define INCLUDE_SIMPLE_CAM_
#include <iomanip>
#include <iostream>
#include <memory>
#include <sys/mman.h>
#include <iostream>
#include <iomanip>
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

#define TIMEOUT_SEC 10000
#define BUFFER_COUNT 3

using namespace libcamera;



namespace RPICam {
    static EventLoop loop;
    static std::mutex mutex;
    static int send_count;
    static std::mutex send_count_mutex;
    static int send_current = 0;

    const static int save_to_file = 0;
    static int image_counter = 0;

    static int debug = 0;

    int settings_locked;
    int save_settings;

    //Config stuff
    int print_metadata = 0;
    int mavlink_enabled = 0;
    int send_to_obc = 0;
    


    //camera fucked idk
    const unsigned int invalid_controls[] = {0x18, 0x1a};


    functionQueue funQ;

    std::vector<std::unique_ptr<Request>> requests;
    static std::shared_ptr<Camera> camera;
    //static void requestComplete(Request *request);

    static void saveData(Request *request);

    static void processRequest(Request *request); 

    static void requestComplete(Request *request); 

    void lock_settings();

    void unlock_settings();


    std::string cameraName(Camera *camera);


    void send_next_image(); 

    void start();

    void stop_taking_pictures();
    
    void copy_controls(libcamera::ControlList *list_out,
            libcamera::ControlList *list_in, int all);

    ControlList controlList_copy;
    int image_count = 0;

};
#endif
