#include "simple-cam.cpp"
#include "mavlink.cpp"
#include "mavlink.hpp"
#include "obc_port.cpp"
#include "function_queue.hpp"

#define UART_NAME "/dev/serial0"
#define BAUDRATE 57600
#define DEBUG_MODE 0
#define SEND_TO_OBC 0
#define MAVLINK_ENABLED 0

//TODO: make a sanity check of sorts to make sure all the components work before fully starting

/*
 * Main executable for camera code
 */
int main() {

    
    /*
     * Sets the config stuff
     */
    RPICam::debug = DEBUG_MODE;
    RPICam::send_to_obc = SEND_TO_OBC;
    RPICam::mavlink_enabled = MAVLINK_ENABLED;


    OBCPort::start_camera_thread();


    if (RPICam::mavlink_enabled) {
        Port *port = new Port(UART_NAME, BAUDRATE); 
        port->start();

        Mavlink::mavlink(port);
        Mavlink::start();
    }


    functionQueue functionQ;


    functionQ.startSendingLoop();

    OBCPort::start_listener();
    std::cout << "started listening";

    //TODO: Port thing is a memory leak
    
    
    return 0;
}

