#include "simple-cam.cpp"
#include "mavlink.cpp"
#include "mavlink.hpp"
#include "obc_port.cpp"
#include "function_queue.hpp"

#define UART_NAME "/dev/serial0"
#define BAUDRATE 57600
int main() {


    OBCPort::start_camera_thread();

    Port *port = new Port(UART_NAME, BAUDRATE); 
    port->start();

    //Mavlink::mavlink(port);
    //Mavlink::start();

    //Mavlink::send_attitude_message();
    //usleep(100000);

    functionQueue functionQ;

    functionQ.startSendingLoop();

    OBCPort::start_listener();
    
    return 0;
}

