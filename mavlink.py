import asyncio
from pymavlink import mavutil
import queue
from connection_listener import response_queue, ConnectionListener


class MavLink:
    
    REQUEST_ATTITUDE_MESSAGE = connection.mav.command_long_encode(
            connection.target_system,
            connection.target_component,
            mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
            0,  
            mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
            0, # param2: Interval in microseconds
            0, 0, 0, 0, 0
            )
    
    REQUEST_POSITION_MESSAGE = connection.mav.command_long_encode(
            connection.target_system,
            connection.target_component,
            mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
            0,  
            mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
            0, # param2: Interval in microseconds
            0, 0, 0, 0, 0
            )

    def __init__(self):

        # Start a connection listening on a UDP port
        self.connection = mavutil.mavlink_connection('/dev/serial0',
                                                     baud=57600)

        # Wait for the first heartbeat
        # This sets the system and component ID of remote system for the link
        self.connection.wait_heartbeat()
        print("Heartbeat from system (system %u component %u)" % 
              (self.connection.target_system, self.connection.target_component))

    def get_attitude_and_position():

        self.listener = ConnectionListener(self.connection, connection)
        self.listener_task = asyncio.create_task(self.listener.start_listener)
        
        self.connection.mav.send(REQUEST_ATTITUDE_MESSAGE)
        self.connection.mav.send(REQUEST_POSITION_MESSAGE)

        self.responses = []
        self.responses_count = 0
        while (responses_count < 2):
            self.response = await asyncio.to_thread(response_queue.get)
            print("Response:" + self.response)
            self.responses.append(self.response)
        self.listener_task.cancel()
        try:
            await listener_task
        except asyncio.CancelledError:
            print("Listener stopped.")
        return responses
            






        
        
        



