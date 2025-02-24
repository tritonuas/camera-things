#include "mavlink.cpp"
#include "simple-cam.hpp"
#include "mavlink.hpp"
#include "obc_port.hpp"
#include "function_queue.hpp"

#define UART_NAME "/dev/serial0"
#define BAUDRATE 57600
int main() {

    OBCPort obcPort; 

    obcPort.start_camera_thread();

    /**
    Port *port = new Port(UART_NAME, BAUDRATE); 
    Mavlink::mavlink(port);
    Mavlink::start();
    **/

    functionQueue functionQ;

    functionQ.startSendingLoop();

    obcPort.start_listener();
}

