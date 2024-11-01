import asyncio
from pymavlink import mavutil
import queue
from connection_listener import response_queue, ConnectionListener


class Mavlink:
    

    def __init__(self):

        # Start a connection listening on a UDP port
        self.connection = mavutil.mavlink_connection('/dev/serial0',
                                                     baud=57600)
        self.REQUEST_ATTITUDE_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,  
                mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
                0, # param2: Interval in microseconds
                0, 0, 0, 0, 0
                )
        
        self.REQUEST_POSITION_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,  
                mavutil.mavlink.MAVLINK_MSG_ID_GLOBAL_POSITION_INT,
                0, # param2: Interval in microseconds
                0, 0, 0, 0, 0
                )

        # Wait for the first heartbeat
        # This sets the system and component ID of remote system for the link
        self.connection.wait_heartbeat()
        print("Heartbeat from system (system %u component %u)" % 
              (self.connection.target_system, self.connection.target_component))

    async def get_attitude_and_position(self):

        self.listener = ConnectionListener(self.connection)
        self.listener_task = asyncio.create_task(self.listener.start_listener())
        self.connection.mav.send(self.REQUEST_ATTITUDE_MESSAGE)
        self.connection.mav.send(self.REQUEST_POSITION_MESSAGE)

        self.responses = []
        self.responses_count = 0
        while (self.responses_count < 2):
            self.response = await asyncio.to_thread(response_queue.get)
            self.responses.append(self.response)
            self.responses_count += 1
        self.listener_task.cancel()
        try:
            await self.listener_task
        except asyncio.CancelledError:
            print("Listener stopped.")
        return self.responses
            






        
        
        



