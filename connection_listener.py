import asyncio
import queue

response_queue = queue.Queue()

class ConnectionListener:

    def __init__(self, connection):
        self.connection = connection
        _determine_time_offset()
        

    async def start_listener(self, queue_bool, unix_time_bool):
    
        self.responses = []
        self.loop = True
        while (self.loop)
            self.response = self.connection.recv_match()
            if not self.response:
                continue
            if (self.response.get_type() == "ATTITUDE" or 
                self.response.get_type() == "GLOBAL_POSITION_INT"):
                if (unix_time_bool):
                    self.response["time_unix_msec"] = (
                            self.response["time_boot_ms"] +
                            self.time_offset)

                if (queue_bool):
                    response_queue.put(self.response)

                else:
                    self.responses.append(response)

                    


    def stop_listener(self):
        self.loop = False
        pass


    async def request_time(self):
        self.connection.mav.send(self.REQUEST_POSITION_MESSAGE)
        while True:
            self.response = self.connection.recv_match()
            if (self.response.get_type() == "SYSTEM_TIME"):
                return response
    async def _determine_time_offset(self):
        self.request_time_response = request_time()
        self.time_offset = (self.request_time_response["time_unix_usec"] * 
                            MICRO_TO_MILLI -
                            self.request_time_response["time_boot_ms"])
