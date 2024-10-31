import treading
import queue

response_queue = queue.Queue()

class ConnectionListener:

    def __init__(self):

        if not self.thread or not self.thread.is_alive():
            self.stop_event.clear9)
            self.thread = threading.Thread(target=self._receive_message)
            self.thread.daemon = True
            self.thread.start()
        
