from pymavlink import mavutil
import queue
import threading


# Start a connection listening on a UDP port

# Wait for the first heartbeat
#   This sets the system and component ID of remote system for the link
connection.wait_heartbeat()
print("Heartbeat from system (system %u component %u)" % (connection.target_system, connection.target_component))


# Define command_long_encode message to send MAV_CMD_SET_MESSAGE_INTERVAL command
# param1: MAVLINK_MSG_ID_BATTERY_STATUS (message to stream)
# param2: 1000000 (Stream interval in microseconds)
message = connection.mav.command_long_encode(
        connection.target_system,  # Target system ID
        connection.target_component,  # Target component ID
        mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE,  # ID of command to send
        0,  # Confirmation
        mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  # param1: Message ID to be streamed
        0, # param2: Interval in microseconds
        0,       # param3 (unused)
        0,       # param4 (unused)
        0,       # param5 (unused)
        0,       # param5 (unused)
        0        # param6 (unused)
        )


print(message)
# Send the COMMAND_LONG
connection.mav.send(message)

# Wait for a response (blocking) to the MAV_CMD_SET_MESSAGE_INTERVAL command and print result
while True:
    response = connection.recv_match()
    if not response:
        continue
    if response.get_type() == "ATTITUDE":
        print(response)
    print(response.get_type())


class MavLink:
    
    request_attitude_message = connection.mav.command_long_encode(
            connection.target_system,
            connection.target_component,
            mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
            0,  
            mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
            0, # param2: Interval in microseconds
            0, 0, 0, 0, 0
            )
    
    request_position_message = connection.mav.command_long_encode(
            connection.target_system,
            connection.target_component,
            mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
            0,  
            mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
            0, # param2: Interval in microseconds
            0, 0, 0, 0, 0
            )

    response_queue = queue.Queue()
    stop_listener = threading.Event()

    def __init__(self):
        self.connection = mavutil.mavlink_connection('/dev/serial0',
                                                     baud=57600)

    def __start_listener(self):
        while not self.stop_event.is_set(): 
            self.response = self.connection.recv_match()
            if not self.response:
                continue
            if (self.response.get_type() == "ATTITUDE" or 
                self.reponse.get_type() == GLOBAL_POSITION_INT):
                self.response_queue.put(self.response)

    def __stop_listener(self):
        if self.thread and self.thread.is_alive():
            self.stop_event.set()
            self.thread.join()




    def get_attitude_and_position():

        connection.mav.send(request_attitude_message)
        connection.mav



