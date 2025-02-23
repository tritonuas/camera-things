#ifndef INCLUDE_MAVLINK_HPP_
#define INCLUDE_MAVLINK_HPP_

#define RAW_ATTITUDE_SIZE 29
#define RAW_GPS_SIZE 37

#include <chrono>
#include <cstdint>
#include <iostream>
#include <future>
#include <memory>
#include <thread>


#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h> // This uses POSIX Threads
#include <unistd.h>  // UNIX standard function definitions
#include <mutex>

#include <common/mavlink.h>

#include "port.hpp"


namespace Mavlink {
    struct Mavlink_Messages {

        int sysid;
        int compid;

        // Heartbeat
        mavlink_heartbeat_t heartbeat;

        // System Status
        mavlink_sys_status_t sys_status;

        // Radio Status
        mavlink_radio_status_t radio_status;

        // Local Position
        mavlink_local_position_ned_t local_position_ned;

        // Global Position
        mavlink_global_position_int_t global_position_int;

        // Local Position Target
        mavlink_position_target_local_ned_t position_target_local_ned;

        // Global Position Target
        mavlink_position_target_global_int_t position_target_global_int;

        // HiRes IMU
        mavlink_highres_imu_t highres_imu;

        // Attitude
        mavlink_attitude_t attitude;

        // System Parameters?
    };


    void start_read_thread();



    Port *port;

    bool time_to_exit;

    int result;
    pthread_t read_tid;

    char reading_status;
    char writing_status;
    char control_status;

    int system_id;
    int autopilot_id;
    int companion_id;

    uint64_t write_count;

    Mavlink_Messages current_messages;



    void mavlink(Port *port_) {
        // initialize attributes
        write_count = 0;

        reading_status = 0;      // whether the read thread is running
        writing_status = 0;      // whether the write thread is running
        control_status = 0;      // whether the autopilot is in offboard control mode
        time_to_exit   = false;  // flag to signal thread exit


        system_id    = 0; // system id
        autopilot_id = 0; // autopilot component id
        companion_id = 0; // companion computer component id

        current_messages.sysid  = system_id;
        current_messages.compid = autopilot_id;

        port = port_; // port management object
    }

    //~Mavlink();
    //
    //

    /*
       void handle_heartbeat(); 
       void handle_gps();
       void handle_attitude(mavlink_message_t message) {
       mavlink_msg_attitude_decode(message, current_messages.attitude);
       }

*/

    void read_message() {

        bool success;
        bool received_all = false;


        while (!received_all && !time_to_exit) {
            //Read the message
            //
            mavlink_message_t message; 
            success = port->read_message(message);

            //handle message

            if (success) {
                current_messages.sysid = message.sysid;
                current_messages.compid = message.compid;

                switch (message.msgid) {

                    case MAVLINK_MSG_ID_HEARTBEAT:
                        {
                            printf("received heartbreak message");
                            break;

                        }
                    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                        {
                            printf("received gps message");
                            break;
                        }
                    case MAVLINK_MSG_ID_ATTITUDE:
                        {
                            printf("received attitude message");
                            break;
                        }
                    default:
                        {
                            printf("message out of scope");
                        }
                }
            }
        }
    }

    int write_message(mavlink_message_t message) {

        //write
        int len = port->write_message(message);

        write_count++;
        return len;
    }


    int send_gps_message() {
        mavlink_command_long_t com = {};
        com.target_system    = system_id;
        com.target_component = autopilot_id;
        com.command          = MAV_CMD_REQUEST_MESSAGE;
        com.confirmation     = true;
        com.param1           = MAVLINK_MSG_ID_GLOBAL_POSITION_INT;
        com.param7           = 1;

        // Encode
        mavlink_message_t message;
        mavlink_msg_command_long_encode(system_id, companion_id, &message, &com);

        // Send the message
        return write_message(message);
    }


    int send_attitude_message() {

        mavlink_command_long_t com = {};
        com.target_system    = system_id;
        com.target_component = autopilot_id;
        com.command          = MAV_CMD_REQUEST_MESSAGE;
        com.confirmation     = true;
        com.param1           = MAVLINK_MSG_ID_ATTITUDE;
        com.param7           = 1;

        // Encode
        mavlink_message_t message;
        mavlink_msg_command_long_encode(system_id, companion_id, &message, &com);

        // Send the message
        return write_message(message);
    }

    void start() {

        //check port

        if (!port->is_running()) {
            printf("Port not open\n");
            throw 1;
        }

        //read thread

        printf("Start read thread\n");
        start_read_thread();


        while (not current_messages.sysid) {
            if (time_to_exit) {
                return;
            }
            usleep(500000);
        }

        printf("Found\n");


        // TODO: the actual system will have multiple thing sysid,
        // make sure that it is consistant
        // System ID
        if ( not system_id )
        {
            system_id = current_messages.sysid;
            printf("GOT VEHICLE SYSTEM ID: %i\n", system_id );
        }

        // Component ID
        if ( not autopilot_id )
        {
            autopilot_id = current_messages.compid;
            printf("GOT AUTOPILOT COMPONENT ID: %i\n", autopilot_id);
            printf("\n");
        }
    }

    void stop() {
        time_to_exit = true;
    }

    void read_thread() {

        reading_status = true;

        while ( ! time_to_exit )
        {
            read_message();
            usleep(100000); // Read batches at 10Hz
        }

        reading_status = false;

        return;
    }

    void start_read_thread() {
        std::thread mavlinkReadThread(read_thread);
        mavlinkReadThread.detach();
        std::cout << "Read thread started";
    }


}


#endif
