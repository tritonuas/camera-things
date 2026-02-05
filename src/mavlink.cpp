#include "mavlink.hpp"
#include "obc_port.hpp"
#include "simple-cam.hpp"
#include <loguru/loguru.hpp>
#include <sys/mman.h>

/*
 * Mavlink code for talking to the pixhawk
 * TODO: maybe use the same code for talking to the gimbal board (if that is even nessicary)
 * 
 *
 */

namespace Mavlink {

    #ifndef MAV_COMP_ID_PI
    #define MAV_COMP_ID_PI 191
    #endif

    void mavlink(Port *port_) {
        // initialize attributes
        write_count = 0;

        reading_status = 0;      // whether the read thread is running
        writing_status = 0;      // whether the write thread is running
        control_status = 0;      // whether the autopilot is in offboard control mode
        time_to_exit   = false;  // flag to signal thread exit

        system_id    = 0; // system id
        autopilot_id = 0; // autopilot component id
        companion_id = MAV_COMP_ID_PI; // companion computer component id

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

    void process_mavlink_message(mavlink_message_t msg) {
        //uint8_t buffer[MAVLINK_MAX_PACKET_LEN];

	if (RPICam::send_to_obc) {
        void *buffer = mmap(NULL, MAVLINK_MAX_PACKET_LEN,
                PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        mavlink_msg_to_send_buffer(reinterpret_cast<uint8_t*>(buffer), &msg);

        funQ.push_front_function(OBCPort::send_image,
                buffer, MAVLINK_MAX_PACKET_LEN);
	}

	else{
		LOG_F(DEBUG, "Mavlink: Sending to OBC disabled (Local)");
	}
    }


    void print_gps(mavlink_global_position_int_t* msg) {
        LOG_F(INFO, "GPS: time_boot_ms=%u lat=%d lon=%d alt=%d relative_alt=%d vx=%d vy=%d vz=%d hdg=%u",
            msg->time_boot_ms, msg->lat, msg->lon, msg->alt, msg->relative_alt, 
            msg->vx, msg->vy, msg->vz, msg->hdg);
    }


    void print_attitude(mavlink_attitude_t *msg) {
        LOG_F(INFO, "Attitude: time_boot_ms=%u roll=%f pitch=%f yaw=%f rollspeed=%f pitchspeed=%f yawspeed=%f",
            msg->time_boot_ms, msg->roll, msg->pitch, msg->yaw, 
            msg->rollspeed, msg->pitchspeed, msg->yawspeed);
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
                            LOG_F(1, "Mavlink: Received GPS");
                            process_mavlink_message(message);
                            break;
                        }
                    case MAVLINK_MSG_ID_ATTITUDE:
                        {
                            LOG_F(1, "Mavlink: Received Attitude");
                            //mavlink_attitude_t temp_msg;
                            //temp_msg = handle_attitude_message(&message);
                            //print_attitude(&temp_msg);
                            process_mavlink_message(message);

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
            LOG_F(ERROR, "Mavlink: Port not open");
            throw 1;
        }

        //read thread

        LOG_F(INFO, "Mavlink: Read thread starting");
        start_read_thread();


        while (not current_messages.sysid) {
            if (time_to_exit) {
                return;
            }
            LOG_F(WARNING, "Mavlink: Waiting for connection...");
            usleep(500000);
        }

        LOG_F(INFO, "Mavlink: Connected");


        // TODO: the actual system will have multiple thing sysid,
        // make sure that it is consistant
        // System ID
        if ( not system_id )
        {
            system_id = current_messages.sysid;
            LOG_F(INFO, "Mavlink: System ID: %d", system_id);
        }

        // Component ID
        if ( not autopilot_id )
        {
            autopilot_id = current_messages.compid;
            LOG_F(INFO, "Mavlink: Autopilot ID: %d", autopilot_id);
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
            usleep(1000);
        }

        reading_status = false;

        return;
    }

    void start_read_thread() {
        std::thread mavlinkReadThread(read_thread);
        mavlinkReadThread.detach();
        LOG_F(INFO, "Mavlink: Read thread started");
    }

}
