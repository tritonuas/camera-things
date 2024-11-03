import csv
import asyncio
import queue
from pymavlink import mavutil

response_queue = queue.Queue()

MICRO_TO_MILLI = 1000

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

    def __init__(self, connection):



        self.connection = connection

        self.REQUEST_TIME_MESSAGE = self.connection.mav.command_long_encode(
                self.connection.target_system,
                self.connection.target_component,
                mavutil.mavlink.MAV_CMD_REQUEST_MESSAGE, 
                0,  
                mavutil.mavlink.MAVLINK_MSG_ID_SYSTEM_TIME,
                0, # param2: Interval in microseconds
                0, 0, 0, 0, 0
                )
        print("testkajdlkasjdsld")
        self._determine_time_offset()

        

    async def start_listener(self,
                             queue_bool,
                             unix_time_bool,
                             file_frequency,
                             location):
    
        self.attitude_responses = []
        self.gps_responses = []
        self.loop = True
        self.number_of_gps_files = 0
        self.number_of_attitude_files = 0
        while (self.loop):
            self.response = self.connection.recv_match()
            if not self.response:
                continue
            if (self.response.get_type() == "ATTITUDE" or 
                self.response.get_type() == "GLOBAL_POSITION_INT"):
                #print(self.response)
                if (unix_time_bool):
                    self.temp_response = []
                    self.temp_response.append(
                            self.response.time_boot_ms +
                            self.time_offset)

                if (self.response.get_type() == "ATTITUDE"):
                    
                    self.temp_response.append(self.response.time_boot_ms)
                    self.temp_response.append(self.response.roll)
                    self.temp_response.append(self.response.pitch)
                    self.temp_response.append(self.response.yaw)
                    self.temp_response.append(self.response.rollspeed)
                    self.temp_response.append(self.response.pitchspeed)
                    self.temp_response.append(self.response.yawspeed)
                    if (not queue_bool):
                        self.attitude_responses.append(self.temp_response)

                if (self.response.get_type() == "GLOBAL_POSITION_INT"):
                    self.temp_response.append(self.response.time_boot_ms)
                    self.temp_response.append(self.response.lat)
                    self.temp_response.append(self.response.lon)
                    self.temp_response.append(self.response.alt)
                    self.temp_response.append(self.response.relative_alt)
                    self.temp_response.append(self.response.vx)
                    self.temp_response.append(self.response.vy)
                    self.temp_response.append(self.response.hdg)
                    if (not queue_bool):
                        self.gps_responses.append(self.temp_response)

                if (queue_bool):
                    response_queue.put(self.temp_response)

                else:

                    if (len(self.gps_responses) % file_frequency == 0 and not len(self.gps_responses) == 0):
                        self._write_to_csv(self.gps_responses, location + str(self.number_of_gps_files) + "gps.csv")
                        self.number_of_gps_files += 1
                        self.gps_responses = []
                        print("wrote gps")

                    if (len(self.attitude_responses) % file_frequency == 0 and not len(self.attitude_responses) == 0):
                        self._write_to_csv(self.attitude_responses, location + str(self.number_of_attitude_files) + "attitude.csv")
                        self.number_of_attitude_files += 1
                        self.attitude_responses = []
                        print("wrote att")

    def stop_listener(self):
        self.loop = False
        pass


    def request_time(self):
        self.connection.mav.send(self.REQUEST_TIME_MESSAGE)
        while True:
            self.response = self.connection.recv_match()
            if (not self.response == None):
                if (self.response.get_type() == "SYSTEM_TIME"):
                    print(self.response)
                    return self.response

                
    def _determine_time_offset(self):
        self.request_time_response = self.request_time()
        print("test")
        self.time_offset = (self.request_time_response.time_unix_usec * 
                            MICRO_TO_MILLI -
                            self.request_time_response.time_boot_ms)

    def _write_to_csv(self, l, filename):
        with open(filename, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerows(l)


