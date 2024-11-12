import time
import asyncio
from pymavlink import mavutil
import queue
from connection_listener import response_queue, ConnectionListener


"""Constants
"""
RESPONSE_COUNT = 2
MICRO_TO_MILLI = 1000



class Mavlink:
    

    def __init__(self) -> None:

        """Start a connection on the serial0 port, with baud 57600
        """
        self.connection = mavutil.mavlink_connection('/dev/serial0',
                                                     baud=57600)
        """The mavlink messages to request attitude and GPS and time
        """
        self.REQUEST_ATTITUDE_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,
                mavutil.mavlink.MAVLINK_MSG_ID_ATTITUDE,  
                0,
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

        self.REQUEST_TIME_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,  
                mavutil.mavlink.MAVLINK_MSG_ID_SYSTEM_TIME,
                0, # param2: Interval in microseconds
                0, 0, 0, 0, 0
                )

        """Wait for the first heartbeat
        This sets the system and component ID of remote system for the link
        """
        self.connection.wait_heartbeat()
        print("Heartbeat from system (system %u component %u)" % 
              (self.connection.target_system, self.connection.target_component))
        #self._determine_time_offset()


    async def get_attitude_and_position(self) -> None:
        """This function bad, don't use for now
        """
        self.listener = ConnectionListener(self.connection)
        self.listener_task = asyncio.create_task(self.listener.start_listener(queue_bool=False, unix_time_bool=True, file_frequency=10))
        self.connection.mav.send(self.REQUEST_ATTITUDE_MESSAGE)
        self.connection.mav.send(self.REQUEST_POSITION_MESSAGE)
        return

        """
        self.responses = []
        self.responses_count = 0
        while (self.responses_count < RESPONSE_COUNT):
            self.response = await asyncio.to_thread(response_queue.get)
            self.response["time_unix_msec"] = (self.response["time_boot_ms"] +
                                               self.time_offset)
            self.responses.append(self.response)
            self.responses_count += 1
        self.listener_task.cancel()
        try:
            await self.listener_task
        except asyncio.CancelledError:
            print("Listener stopped.")
        return self.responses
        """
            
    def start_getting_attitude_and_position(self, location: str) -> None:
        """Starts the connection_listener in the background
        TODO: Make it so then it will actually take in arguments
        """


        """Initalizes the listerner claass
        """
        self.listener = ConnectionListener(self.connection)
        """Starts the listener class
        """
        self.listener_task = asyncio.create_task(self.listener.start_listener(
            use_queue=False,
            use_unix_time=True,
            file_frequency=10,
            location=location))

    async def request_attitude_and_position(self) -> None:
        """Sends mavlink messages to request attitude and postion
        Use with start_getting_attitude_and_position to receive the responses
        """
        self.connection.mav.send(self.REQUEST_ATTITUDE_MESSAGE)
        self.connection.mav.send(self.REQUEST_POSITION_MESSAGE)



    def _determine_time_offset(self) -> None:
        """Determines the time offset between unix time and time to boot
        TODO: see if this is really needed
        """
        self.request_time_response = request_time()
        self.time_offset = (self.request_time_response["time_unix_usec"] * 
                            MICRO_TO_MILLI -
                            self.request_time_response["time_boot_ms"])


