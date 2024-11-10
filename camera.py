from picamera2 import Picamera2, Preview
from time_logging import timeLogging
from mavlink import Mavlink
import time
import asyncio
from PIL import Image
import numpy as np
import argparse


VALID_FILE_TYPES=[
        'jpg',
        'png',
        'dng']

class Camera:
    def __init__(self, **kwargs):
        

        """Constructor for Camera class

        kwargs -- any input parameters :

        binning -- if enabled, enable 2x2 binning
        save -- if enabled write to SD
        time -- if enabled, log the fps in a seperate csv
        location -- sets the location to save to file
        file-format -- sets the file format (jpg, png, dng)
        mavlink -- enables mavlink logging of attitude and gps
        """

        """Set the settings from the args correcty
        Makes sure that the file type is valid"""
        self.file_format = kwargs.get('format', 'jpg')
        if (self.file_format not in VALID_FILE_TYPES): 
            print("Invalid file format")
            return

        """Gets all the args stuff
        """
        self.binning = kwargs.get('binning', False)
        self.save = kwargs.get('save', False)
        self.time = kwargs.get('time', False)
        self.verbose = kwargs.get('verbose', False)
        self.mavlink_enabled = kwargs.get('mavlink', False)


        """Handles the location settings
        Default is current directory, appends / at the end"""
        self.location = kwargs.get('location', '.')
        if (self.location ==''):
            self.location = '.'

        if (self.location[-1] != '/'):
            self.location += '/'

        """If mavlink is enabled, then create the object
        """
        if (self.mavlink_enabled):
            self.mavlink = Mavlink()
            self.mavlink.start_getting_attitude_and_position(self.location)



    async def _take_picture(self, cam):
        """Async take a picture and returns it as an array
        cam -- The picam2 object
        """
        return cam.capture_array("main")

    async def _save_to_file(self, image_list, filename):
        """Does nothing for now
        Later might be changed to send data over wifi or other interface
        and need it to be written to a var
        """

        return

    def _initialize_camera(self):
        """Initializes the camera and returns a 
        picam2 object ready to be configured
        """
        picam2 = Picamera2()
        return picam2
        
    def _print_sensor_modes(self):
        """Prints the modes of the camera
        """
        modes = picam2.sensor_modes
        print(modes)


    def _select_2x2_binning(self, picam2):
        """Returns the camera settings for 2x2 binning
        picam2 -- The picamera2 object
        """

        """Gets the 2x2 binning mode
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

    def _select_full_res(self, picam2):
        """Returns the camera settings for rull resolution photos
        picam2 -- The picamera2 object
        """

        camera_config = picam2.create_still_configuration(
                {"size": (4056, 3040)},
                buffer_count=1,
                queue=False,
                controls = {'NoiseReductionMode': 0})

        return camera_config

    def _capture_file(self, picam2, iteration, file_format, location):
        """Captures a file and saves it to a file

        picam2 -- The picamera2 object
        iteration -- The number of the file (for naming)
        file_format -- The format of the files
        location -- The location to save the files
        """

        """Takes the photo
        dng is raw, so the format is special
        """
        if (file_format == 'dng'):
            picam2.capture_file(f"{location}{iteration}.dng", 'raw')

        else:
            picam2.capture_file(f"{location}{iteration}.{file_format}")

    def _format_number(self, num, digits):
        """Formats the numbers correct
        Adds leadings zeros to the number to make stuff nicer

        num -- The original number
        digits -- The number of total digits in the number
        """
        return f"{num:0{digits}d}"

    def initialize_and_configure(self):
        """Initializes and configures the camera
        """

        self.picam2 = self._initialize_camera()

        """Set the appropirate camera config"""
        if (self.binning == True):
            self.picam2.configure(self._select_2x2_binning(self.picam2))
        else:
            self.picam2.configure(self._select_full_res(self.picam2))

    def lock_controls(self):
        """Disables automatic white balance and exposure settings
        """
        self._disable_auto_while_balance()
        self._disable_auto_exposure()

    def unlock_controls(self):
        self._enable_auto_while_balance()
        self._enable_auto_exposure()

    def _disable_auto_while_balance(self):
        self.picam2.set_controls({"AwbEnable": False})

    def _disable_auto_exposure(self):
        self.picam2.set_controls({"AeEnable": False})

    def _enable_auto_while_balance(self):
        self.picam2.set_controls({"AwbEnable": True})

    def _enable_auto_exposure(self):
        self.picam2.set_controls({"AeEnable": True})

    def start(self):
        """Start the camera
        """
        self.picam2.start()




    def take_photo(self):
        """Take a single photo
        """
        self.take_photos(1)


    async def take_photos(self, iterations):
        """Takes multiple photos
        iterations -- The number of photos to take
        """

        """Here incase photos written to array"""
        self.image_array=[[0,0,0]]

        """start the time logging stuff if enabled"""
        if (self.time):
            self.timer = timeLogging()
            self.timer.start_timer()

        """Determine the number of leading zeros
        depending on the number of iterations"""
        if (self.save):
            self.digits = len(str(iterations))
        """If infinte amount of iterations, then set to 8 digits
        """
        if (iterations == 0):
            self.digits = 8

        """Start taking photos
        In try loop so then the program can be KeyboardInterrupt'ed gracefully
        """
        try:
            """Iteration counter
            """
            i = 0
            while True:
                """Take the photo and maybe save it
                """
                if (self.save):
                    """Does mavlink stuff if enabled

                    TODO: For now, it takes the mavlink data before capturing
                    the file. Try adding mavlink logging after or even during
                    the image taking so then an algorithm can be run to
                    correct distortion
                    """
                    if (self.mavlink_enabled):
                        await self.mavlink.request_attitude_and_position()
                    self._capture_file(self.picam2, 
                                 self._format_number(i, self.digits),
                                 self.file_format,
                                 self.location) 

                else:
                    """TODO: add an option to send the image + metadata over
                    internet or other interface
                    """
                    self.image_array, _ = await asyncio.gather(
                            take_picture(self.picam2),
                            save_to_file(self.image_array,
                                         f'{format_number(i, digits)}.txt'))
                    # Remove later, right now, we only need to be saving
                    break

                """If logging is enabled then log it, and if verbose, then print
                """
                if (self.time):
                    if (self.verbose):
                        self.timer.log_and_print()
                    else:
                        self.timer.log()

                """Count the iterations and break if iteration count reached"""
                i += 1
                if (i == iterations):
                    break
        except KeyboardInterrupt:
            """If KeyboardInterrupt is used to break the while True loop,
            time data is still saved
            So then time data can still be saved when Ctrl-C
            """
            if (self.time):
                self.timer.save_to_file(self.location)
                print("saved to file")
            return

        """If the loop ends naturally, then log the time
        """
        if (self.time):
            self.timer.save_to_file(self.location)
            print("saved to file")
