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
#include <pthread.h>
#include <unistd.h>
#include <mutex>
#include <common/mavlink.h>

#include "port.hpp"
#include "function_queue.hpp"

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
        mavlink_heartbeat_t heartbeat;
        mavlink_sys_status_t sys_status;
        mavlink_radio_status_t radio_status;
        mavlink_local_position_ned_t local_position_ned;
        mavlink_global_position_int_t global_position_int;
        mavlink_position_target_local_ned_t position_target_local_ned;
        mavlink_position_target_global_int_t position_target_global_int;
        mavlink_highres_imu_t highres_imu;
        mavlink_attitude_t attitude;
    };

    void start_read_thread();

    inline Port *port;
    inline bool time_to_exit;
    inline int result;
    inline pthread_t read_tid;

    inline char reading_status;
    inline char writing_status;
    inline char control_status;

    inline int system_id;
    inline int autopilot_id;
    inline int companion_id;

    inline uint64_t write_count;

    inline Mavlink_Messages current_messages;

    inline functionQueue funQ;
    // ------------------------------------------------

    void mavlink(Port *port_);
    void handle_heartbeat(); 
    void handle_gps();
    void handle_attitude();
    void read_message();
    int write_message(mavlink_message_t message);
    int send_gps_message();
    int send_attitude_message();
    void send_both_messages();
    void start();
    void stop();
    void read_thread();
}

#endif