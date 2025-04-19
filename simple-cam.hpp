#ifndef INCLUDE_SIMPLE_CAM_
#define INCLUDE_SIMPLE_CAM_
#include <iomanip>
#include <iostream>
#include <memory>
#include <sys/mman.h> // For mmap
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
    static int send_current;

    const static int save_to_file = 0;
    static int j;

    functionQueue funQ;

    std::vector<std::unique_ptr<Request>> requests;
    static std::shared_ptr<Camera> camera;
    //static void requestComplete(Request *request);

    /*
     * --------------------------------------------------------------------
     * Handles and processes the request
     * Heavy processing should not be done in this function and instead
     * done in a different thread
     */

    
    static void saveData(Request *request);

    static void processRequest(Request *request); 

    static void requestComplete(Request *request); 

    void lock_settings();

    void unlock_settings();


    std::string cameraName(Camera *camera);


    void send_next_image(); 

    void start();

    void stop_taking_pictures();

};
#endif
