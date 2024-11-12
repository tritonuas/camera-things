import time
import csv


class timeLogging:


    def __init__(self) -> None:
        """Inits the class, and sets the nessicary vars
        """
        self.intergral = 0
        self.start_time = time.time()
        self.log_list = []
        self.previous_log_time = self.start_time
    
    def start_timer(self) -> None:
        """Starts the timer
        Can also be run again to reset and start the timer again
        """
        self.intergral = 0
        self.start_time = time.time()
        self.previous_log_time = self.start_time

    def log(self):
        """Takes the time data from the time that the function is run and logs
        the data, including the current time, the time since it was run and
        the time since the last iteration
        """
        self.current_time = time.time()
        self.time_since_last_log = self.current_time - self.previous_log_time
        self.time_since_start = self.current_time - self.start_time
        self.previous_log_time = self.current_time
        self.log_list.append([
            self.current_time, 
            self.time_since_last_log, 
            self.time_since_start])

    def log_and_print(self) -> None:
        """Takes the time data from the time that the function is run and logs
        the data, including the current time, the time since it was run and
        the time since the last iteration, and also prints the time since
        the timer was started and the time since the last iteration
        """
        self.current_time = time.time()
        self.time_since_last_log = self.current_time - self.previous_log_time
        self.time_since_start = self.current_time - self.start_time
        self.previous_log_time = self.current_time
        print(str(self.time_since_last_log) + ', ' + str(self.time_since_start))
        self.log_list.append([
            self.current_time, 
            self.time_since_last_log, 
            self.time_since_start])
    
    def save_to_file(self, location: str) -> None:
        """Saves the logged data to a file named times.csv
        """
        try:
            with open(location + 'times.csv', mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerows(self.log_list)
        except Exception as e:
            print(f"Faid to write to file: {e}")
       




