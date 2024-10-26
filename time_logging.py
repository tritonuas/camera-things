import time
import csv


class timeLogging:

    def __init__():
        """Inits the class, and sets the nessicary vars
        """
        intergral = 0
        start_timer = time.time()
        log_list = []
    
    def start_timer():
        """Starts the timer
        Can also be run again to reset and start the timer again
        """
        intergral = 0
        start_timer = time.time()
        previous_log_time = start_timer

    def log():
        """Takes the time data from the time that the function is run and logs
        the data, including the current time, the time since it was run and
        the time since the last iteration
        """
        current_time = time.time()
        time_since_last_log = previous_log_time - current_time
        time_since_start = current_time - start_timer
        log_list.append([current_time, time_since_last_log, time_since_start])

    def log_and_print():
        """Takes the time data from the time that the function is run and logs
        the data, including the current time, the time since it was run and
        the time since the last iteration, and also prints the time since
        the timer was started and the time since the last iteration
        """
        current_time = time.time()
        time_since_last_log = previous_log_time - current_time
        time_since_start = current_time - start_timer
        print(str(time_since_last_log) + ', ' + str(time_since_start))
        log_list.append([current_time, time_since_last_log, time_since_start])
    
    def save_to_file():
        """Saves the logged data to a file named times.csv
        """
        try:
            with open('times.csv', mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerows(log_list)
        except Exception as e:
            print(f"Faid to write to file: {e}")
       




