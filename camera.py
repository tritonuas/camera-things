from picamera2 import Picamera2, Preview
from time_logging import timeLogging
from mavlink import Mavlink
import time
import asyncio
from PIL import Image
import numpy as np
import argparse


class Camera:
    def __init__(self, **kwargs):
        

        """Main function
        kwargs -- any input parameters 

        binning -- if enabled, enable 2x2 binning
        save -- if enabled write to SD
        time -- if enabled, log the fps in a seperate csv
        location -- sets the location to save to file
        file-format -- sets the file format (jpg, png, dng)
        mavlink -- enables mavlink logging of attitude and gps
        """

        VALID_FILE_TYPES=[
                'jpg',
                'png',
                'dng']

        #Set the settings from the args correcty
        self.file_format = kwargs.get('format', 'jpg')
        if (self.file_format not in VALID_FILE_TYPES): 
            print("Invalid file format")
            return

        self.binning = kwargs.get('binning', False)
        self.save = kwargs.get('save', False)
        self.time = kwargs.get('time', False)
        self.verbose = kwargs.get('verbose', False)
        self.mavlink_enabled = kwargs.get('mavlink', False)


        #Handles the location settings
        self.location = kwargs.get('location', '.')
        if (self.location ==''):
            self.location = '.'

        if (self.location[-1] != '/'):
            self.location += '/'

        if (self.mavlink_enabled):
            self.mavlink = Mavlink()
            self.mavlink.start_getting_attitude_and_position(self.location)



    async def take_picture(self, cam):
        """Async take a picture and returns it as an array
        cam -- The picam2 object
        """
        return cam.capture_array("main")

    async def save_to_file(self, image_list, filename):
        """Does nothing for now
        Later might be changed to send data over wifi or other interface
        """

        return

    def initialize_camera(self):
        """Initializes the camera and returns a 
        picam2 object ready to be configured
        """
        picam2 = Picamera2()
        return picam2
        
    def print_sensor_modes(self):
        """Prints the modes of the camera
        """
        modes = picam2.sensor_modes
        print(modes)


    def select_2x2_binning(self, picam2):
        """Configures the settigns to use 2x2 binning mode and returns the config
        """

        modes = picam2.sensor_modes
        mode = modes[2]
        camera_config = picam2.create_still_configuration(
                {"size": mode['size']}, 
                buffer_count=4,
                raw={'format': mode['unpacked']}, 
                sensor={'output_size': mode['size'],
                        'bit_depth': mode['bit_depth']})

        return camera_config

    def select_full_res(self, picam2):
        """Configures the camera to take full res photos
        returns the settings
        """

        camera_config = picam2.create_still_configuration(
                {"size": (4056, 3040)},
                buffer_count=1,
                queue=False,
                controls = {'NoiseReductionMode': 0})

        return camera_config

    def capture_file(self, picam2, iteration, file_format, location):
        if (file_format == 'dng'):
            picam2.capture_file(f"{location}{iteration}.dng", 'raw')

        else:
            picam2.capture_file(f"{location}{iteration}.{file_format}")

    def format_number(self, num, digits):
        return f"{num:0{digits}d}"





    def initialize_and_configure(self):

        #Initializes the camera
        self.picam2 = self.initialize_camera()
        #Set the appropirate camera config
        if (self.binning == True):
            self.picam2.configure(self.select_2x2_binning(self.picam2))
        else:
            self.picam2.configure(self.select_full_res(self.picam2))


    def start(self):
        #Start the camera
        self.picam2.start()



    def take_photo(self):
        self.take_photos(1)
    async def take_photos(self, iterations):

        #Here incase photos written to array
        self.image_array=[[0,0,0]]

        #start the time logging stuff if enabled
        if (self.time):
            self.timer = timeLogging()
            self.timer.start_timer()

        # Determine the number of leading zeros
        # depending on the number of iterations
        if (self.save):
            self.digits = len(str(iterations))
        if (iterations == 0):
            self.digits = 8

        print(iterations)
        try:
            i = 0
            while True:
                #Take the photo and maybe save it
                if (self.save):
                    if (self.mavlink_enabled):
                        await self.mavlink.request_attitude_and_position()
                    self.capture_file(self.picam2, 
                                 self.format_number(i, self.digits),
                                 self.file_format,
                                 self.location) 

                else:
                    """
                    self.image_array, _ = await asyncio.gather(
                            take_picture(self.picam2),
                            save_to_file(self.image_array, f'{format_number(i, digits)}.txt'))
                    """
                    # Remove later, right now, we only need to be saving
                    break

                #If logging is enabled then log it, and if verbose, then print
                if (self.time):
                    if (self.verbose):
                        self.timer.log_and_print()
                    else:
                        self.timer.log()

                #Count the iterations
                i += 1
                if (i == iterations):
                    break
        #TODO: Make it so then when the method is ran in main file, this still
        #      Gets the KeyboardInterrupt error
        #If KeyboardInterrupt to break the while True, time data is still saved
        except KeyboardInterrupt:
            if (self.time):
                self.timer.save_to_file(self.location)
                print("saved to file")
            return
        if (self.time):
            self.timer.save_to_file(self.location)
            print("saved to file")




                


