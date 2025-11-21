#include "mavlink.hpp"
#include "obc_port.hpp"
#include <sys/mman.h>

/*
 * Mavlink code for talking to the pixhawk
 * TODO: maybe use the same code for talking to the gimbal board (if that is even nessicary)
 * 
 *
 */

namespace Mavlink {

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

    void handle_heartbeat(); 

    mavlink_global_position_int_t handle_gps(mavlink_message_t* msg) {
        mavlink_global_position_int_t gps;

        mavlink_msg_global_position_int_decode(msg, &gps);
        return gps;
    }

    mavlink_attitude_t handle_attitude_message(mavlink_message_t* msg) {
        mavlink_attitude_t attitude;

        mavlink_msg_attitude_decode(msg, &attitude);
        return attitude;
    }

    void send_mavlink_message(mavlink_message_t msg) {
        //uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

        void *buffer = mmap(NULL, MAVLINK_MAX_PACKET_LEN,
                PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        mavlink_msg_to_send_buffer(reinterpret_cast<uint8_t*>(buffer), &msg);

        funQ.push_front_function(OBCPort::send_image,
                buffer, MAVLINK_MAX_PACKET_LEN);
    }


    void print_gps(mavlink_global_position_int_t* msg) {
        std::cout << msg->time_boot_ms << "\n";
        std::cout << msg->lat << "\n";
        std::cout << msg->lon << "\n";
        std::cout << msg->alt << "\n";
        std::cout << msg->relative_alt << "\n";
        std::cout << msg->vx << "\n";
        std::cout << msg->vy << "\n";
        std::cout << msg->vz << "\n";
        std::cout << msg->hdg << "\n";
    }


    void print_attitude(mavlink_attitude_t *msg) {
        std::cout << msg->time_boot_ms << "\n";
        std::cout << msg->roll << "\n";
        std::cout << msg->pitch << "\n";
        std::cout << msg->yaw << "\n";
        std::cout << msg->rollspeed << "\n";
        std::cout << msg->pitchspeed << "\n";
        std::cout << msg->yawspeed << "\n";
    }

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
                            //printf("received heartbreak message\n");
                            break;

                        }
                    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
                        {
                            printf("received gps message\n");
                            send_mavlink_message(message);
                            break;
                        }
                    case MAVLINK_MSG_ID_ATTITUDE:
                        {
                            printf("received attitude message\n");
                            //mavlink_attitude_t temp_msg;
                            //temp_msg = handle_attitude_message(&message);
                            //print_attitude(&temp_msg);
                            send_mavlink_message(message);

                            break;
                        }
                    default:
                        {
                            //printf("message out of scope");
                            //std::cout << "Message id" << message.msgid << "\n";
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

    void send_both_messages() {
        send_attitude_message();
        send_gps_message();
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
            std::cout << "not found yet \n";
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
        std::cout << "Read thread started\n";
    }


}
