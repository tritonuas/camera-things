#include <iostream>
#include <fstream>
#include <string>
#include <loguru/loguru.hpp>
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
        LOG_F(WARNING, "Could not open %s. Using default settings.", filename.c_str());
        return;
    }

    LOG_F(INFO, "Loading settings from %s...", filename.c_str());

    try {
        json data = json::parse(file);

        UART_NAME = data.value("UART_NAME", "dev/serial0");
        BAUDRATE = data.value("BAUDRATE", 57600);
        DEBUG_MODE = data.value("DEBUG_MODE", 1);
        SEND_TO_OBC = data.value("SEND_TO_OBC", 0);
        MAVLINK_ENABLED = data.value("MAVLINK_ENABLED", 0);

        // Print out configs for sanity check
        LOG_F(INFO, "  UART_NAME: %s", UART_NAME.c_str());
        LOG_F(INFO, "  BAUDRATE: %d", BAUDRATE);
        LOG_F(INFO, "  DEBUG_MODE: %d", DEBUG_MODE);
        LOG_F(INFO, "  SEND_TO_OBC: %d", SEND_TO_OBC);
        LOG_F(INFO, "  MAVLINK_ENABLED: %d", MAVLINK_ENABLED);

    } catch (json::parse_error& e) {
        LOG_F(ERROR, "JSON Parse Error: %s", e.what());
        LOG_F(ERROR, "Falling back to defaults.");
    }
}

//TODO: make a sanity check of sorts to make sure all the components work before fully starting

/*
 * Main executable for camera code
 */
int main(int argc, char* argv[]) {
    loguru::init(argc, argv);
    loguru::add_file("everything.log", loguru::Append, loguru::Verbosity_MAX);
    LOG_F(INFO, "Starting Camera Application");

    load_configuration("../config/picam.json");
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
    LOG_F(INFO, "Started listening to OBC port");

    //TODO: Port thing is a memory leak
    
    return 0;
}