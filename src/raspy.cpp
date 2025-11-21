#include <iostream>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "simple-cam.hpp" 
#include "mavlink.hpp"
#include "obc_port.hpp"
#include "function_queue.hpp"

// Config Defaults
std::string UART_NAME = "/dev/serial0";
int BAUDRATE = 57600;
int DEBUG_MODE = 1;
int SEND_TO_OBC = 0;
int MAVLINK_ENABLED = 0;

void load_configuration(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "[CONFIG] Warning: Could not open " << filename << ". Using default settings.\n";
        return;
    }

    std::cout << "[CONFIG] Loading settings from " << filename << "...\n";

    try {
        json data = json::parse(file);

        UART_NAME = data.value("UART_NAME", "dev/serial0");
        BAUDRATE = data.value("BAUDRATE", 57600);
        DEBUG_MODE = data.value("DEBUG_MODE", 1);
        SEND_TO_OBC = data.value("SEND_TO_OBC", 0);
        MAVLINK_ENABLED = data.value("MAVLINK_ENABLED", 0);

        // Print out configs for sanity check
        std::cout << "  UART_NAME: " << UART_NAME << "\n";
        std::cout << "  BAUDRATE: " << BAUDRATE << "\n";
        std::cout << "  DEBUG_MODE: " << DEBUG_MODE << "\n";
        std::cout << "  SEND_TO_OBC: " << SEND_TO_OBC << "\n";
        std::cout << "  MAVLINK_ENABLED: " << MAVLINK_ENABLED << "\n";

    } catch (json::parse_error& e) {
        std::cerr << "[CONFIG] JSON Parse Error: " << e.what() << "\n";
        std::cerr << "[CONFIG] Falling back to defaults.\n";
    }
}

//TODO: make a sanity check of sorts to make sure all the components work before fully starting

/*
 * Main executable for camera code
 */
int main() {

    load_configuration("/config/picam.json");
    /*
     * Set the config 
     */
    RPICam::debug = DEBUG_MODE;
    RPICam::send_to_obc = SEND_TO_OBC;
    RPICam::mavlink_enabled = MAVLINK_ENABLED;


    OBCPort::start_camera_thread();


    if (RPICam::mavlink_enabled) {
        Port *port = new Port(UART_NAME.c_str(), BAUDRATE); 
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