import asyncio
from queue import Queue

response_queue = queue.Queue()

class ConnectionListener:

    def __init__(self, connection):
        self.connection = connection

    async def start_listener(self):
        while True:
            self.response = self.connection.recv_match()
            if not self.response:
                continue
            if (self.response.get_type() == "ATTITUDE" or 
                self.reponse.get_type() == GLOBAL_POSITION_INT):
                self.response_queue.put(self.response)

    def stop_listener(self):
            pass



