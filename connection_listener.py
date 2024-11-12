import csv
import asyncio
import queue
from pymavlink import mavutil

"""Not used currently
Use if mavlink response needs to get sent back to mavlink class
"""
response_queue = queue.Queue()

"""Constants"""
MICRO_TO_MILLI = 1000

"""Not used currently
The dict elements of the attitude and gps
"""
DICT_ELEMENTS_ATTITUDE = [
        "time_boot_ms",
        "time_unix_ms",
        "roll",
        "pitch",
        "yaw",
        "rollspeed",
        "pitchspeed",
        "yawspeed"]

DICT_ELEMENTS_GPS = [
        "time_boot_ms",
        "time_unix_ms",
        "lat",
        "lon",
        "alt",
        "relative_alt",
        "vx",
        "vy",
        "vz",
        "hdg"]


class ConnectionListener:

    def __init__(self, connection) -> None:
        """Constructor for ConnectionListener class
        connection -- The mavlink connection
        """

        """Sets the connection to the input connection
        """
        self.connection = connection

        """The request time message to sync the clocks
        """
        self.REQUEST_TIME_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,  
                mavutil.mavlink.MAVLINK_MSG_ID_SYSTEM_TIME,
                0, # param2: Interval in microseconds
                0, 0, 0, 0, 0
                )
        """Determines the time offset between time_boot_ms and unix time
        """
        self._determine_time_offset()

        

    async def start_listener(self,
                             use_queue: bool,
                             use_unix_time: bool,
                             file_frequency: int,
                             location: str) -> None:

        """Starts the listener
        This receices mavlink responses and writes it to a file or puts it
        in the queue

        use_queue -- whether to put the response in the queue or save it to
                        a file
        use_unix_time -- Whether to also save the unix time along with the
                            boot time
        file_frequency -- How frequently to save the mavlink data to a file
        location -- The location to save the file
        """
    

        """List to store responses for the mavlink data
        """
        self.attitude_responses = []
        self.gps_responses = []
        """Used to break the loop via self.stop_listener
        """
        self.loop = True
        """Counter for the number of files in each list
        TODO: Find out if there is preformance difference between this
        """
        self.number_of_gps_files = 0
        self.number_of_attitude_files = 0
        while (self.loop):
            """Receive the response
            """

            self.response = self.connection.recv_match()
            """Check if the responses is real"""
            if not self.response:
                continue

            """Check if the responses is not an imposter
            """
            if (self.response.get_type() == "ATTITUDE" or 
                self.response.get_type() == "GLOBAL_POSITION_INT"):
                
                """Add the unix time if enabled
                """
                if (use_unix_time):
                    self.temp_response = []
                    self.temp_response.append(
                            self.response.time_boot_ms +
                            self.time_offset)

                """If the response is attitude, then save that data
                """
                if (self.response.get_type() == "ATTITUDE"):
                    
                    self.temp_response.append(self.response.time_boot_ms)
                    self.temp_response.append(self.response.roll)
                    self.temp_response.append(self.response.pitch)
                    self.temp_response.append(self.response.yaw)
                    self.temp_response.append(self.response.rollspeed)
                    self.temp_response.append(self.response.pitchspeed)
                    self.temp_response.append(self.response.yawspeed)
                    if (not use_queue):
                        self.attitude_responses.append(self.temp_response)

                """If the response is GPS, then save that data
                """
                if (self.response.get_type() == "GLOBAL_POSITION_INT"):
                    self.temp_response.append(self.response.time_boot_ms)
                    self.temp_response.append(self.response.lat)
                    self.temp_response.append(self.response.lon)
                    self.temp_response.append(self.response.alt)
                    self.temp_response.append(self.response.relative_alt)
                    self.temp_response.append(self.response.vx)
                    self.temp_response.append(self.response.vy)
                    self.temp_response.append(self.response.hdg)
                    if (not use_queue):
                        self.gps_responses.append(self.temp_response)

                if (use_queue):
                    """Puts the response in the queue if enabled
                    """
                    response_queue.put(self.temp_response)
                else:
                    """If the queue is not enabled, then save it to file
                    """
                    """Saves the file if there is the right number of responses
                    """
                    if (len(self.gps_responses) % file_frequency == 0 and 
                        not len(self.gps_responses) == 0):
                        self._write_to_csv(self.gps_responses,
                                           location + 
                                           str(self.number_of_gps_files) +
                                           "gps.csv")
                        """Resets the vars
                        """
                        self.number_of_gps_files += 1
                        self.gps_responses = []

                    if (len(self.attitude_responses) % file_frequency == 0 and
                        not len(self.attitude_responses) == 0):
                        self._write_to_csv(self.attitude_responses,
                                           location +
                                           str(self.number_of_attitude_files) +
                                           "attitude.csv")
                        """Resets the vars
                        """
                        self.number_of_attitude_files += 1
                        self.attitude_responses = []


    def stop_listener(self) -> None:
        """Stops the listener
        """
        self.loop = False
        pass


    def _request_time(self) -> None:
        """Requests the time of the flight computer
        """
        self.connection.mav.send(self.REQUEST_TIME_MESSAGE)
        while True:
            """Receives the reponse
            """
            self.response = self.connection.recv_match()
            """Check if the response exists
            """
            if (not self.response == None):
                """Make sure the response is an imposter
                """
                if (self.response.get_type() == "SYSTEM_TIME"):
                    print(type(self.response))
                    return self.response

                
    def _determine_time_offset(self) -> None:
        """Determines the time offset of the boot time and unix time
        """
        self.request_time_response = self._request_time()
        self.time_offset = (self.request_time_response.time_unix_usec * 
                            MICRO_TO_MILLI -
                            self.request_time_response.time_boot_ms)

    def _write_to_csv(self, l: list, filename: str) -> None:
        """Writes to csv
        l -- the data
        filename -- The filename
        """
        """TODO: format the filename good (with leading zeros)
        """
        with open(filename, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerows(l)


