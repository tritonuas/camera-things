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


    struct attitude_message {
        uint32_t time_boot_ms;

        float roll;
        float pitch;
        float yaw;
        float rollspeed;
        float pitchspeed;
        float yawspeed;
    };

    struct gps_message {
        uint32_t time_boot_ms;

        int32_t lat;
        int32_t lon;
        int32_t alt;
        int32_t relative_alt;
        int16_t vx;
        int16_t vy;
        int16_t vz;
        int16_t hdg;
    };



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



    void mavlink(Port *port_);

    //~Mavlink();
    //
    //

    void handle_heartbeat(); 

    void handle_gps();

    void handle_attitude();

    void read_message();

    int write_message(mavlink_message_t message);

    int send_gps_message();

    int send_attitude_message();

    void start();

    void stop();

    void read_thread();

    void start_read_thread();


}


#endif
