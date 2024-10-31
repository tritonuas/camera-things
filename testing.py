import threading
import queue
import time

# Queue to store responses
response_queue = queue.Queue()

class ConnectionListener:
    """
    Manages a thread that listens for incoming messages on a connection.
    Allows starting, stopping, and restarting the listener.
    """
    def __init__(self, connection):
        self.connection = connection
        self.stop_event = threading.Event()
        self.thread = None

    def start_listener(self):
        """
        Starts the listener thread if it's not already running.
        """
        if not self.thread or not self.thread.is_alive():
            self.stop_event.clear()  # Reset stop event for fresh start
            self.thread = threading.Thread(target=self._receive_message)
            self.thread.daemon = True
            self.thread.start()
            print("Listener started.")
        else:
            print("Listener is already running.")

    def _receive_message(self):
        """
        Continuously waits for messages and places them in the response queue.
        Stops if the stop_event is set.
        """
        while not self.stop_event.is_set():
            response = self.connection.receive()  # Simulate receiving a message
            print("Response received:", response)
            response_queue.put(response)  # Put the response in the queue

    def stop_listener(self):
        """
        Stops the listener thread gracefully.
        """
        if self.thread and self.thread.is_alive():
            self.stop_event.set()
            self.thread.join()
            print("Listener stopped.")
        else:
            print("Listener is not running.")

def send_message(connection, message, response_count=2):
    """
    Sends a message through the connection and waits for a specified number of responses.
    """
    print("Sending message:", message)
    connection.send(message)  # Send the message

    # Collect the expected number of responses
    responses = []
    for _ in range(response_count):
        response = response_queue.get()  # Blocking until each response is available
        print("Received response:", response)
        responses.append(response)

    return responses

class MockConnection:
    """
    Mock connection object for demonstration purposes.
    """
    def send(self, message):
        print(f"Mock send: {message}")

    def receive(self):
        # Simulate waiting and receiving a response
        time.sleep(1)
        return "Mock response"

# Usage example
connection = MockConnection()
listener = ConnectionListener(connection)

# Start the listener
listener.start_listener()

# Send a message and wait for two responses
responses = send_message(connection, "Hello, Server!", response_count=2)
print("All responses received:", responses)

# Stop the listener
listener.stop_listener()

# Restart the listener
listener.start_listener()

# Send another message and wait for two responses
responses = send_message(connection, "Hello again, Server!", response_count=2)
print("All responses received:", responses)

# Stop the listener again
listener.stop_listener()

