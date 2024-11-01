import asyncio
import queue

response_queue = queue.Queue()

class ConnectionListener:

    def __init__(self, connection):
        self.connection = connection

    async def start_listener(self):
        counter = 0
        while True:
            self.response = self.connection.recv_match()
            if not self.response:
                continue
            if (self.response.get_type() == "ATTITUDE" or 
                self.response.get_type() == "GLOBAL_POSITION_INT"):
                response_queue.put(self.response)
                print(self.response.get_type())
            counter += 1
            if (counter > 100):
                break

    def stop_listener(self):
            pass
