from picamera2 import Picamera2, Preview
from time_logging import timeLogging
import time
import asyncio
from PIL import Image
import numpy as np
import argparse


VALID_FILE_TYPES=[
        'jpg',
        'png',
        'dng']


async def take_picture(cam):
    """Async take a picture and returns it as an array
    cam -- The picam2 object
    """
    return cam.capture_array("main")

async def save_to_file(image_list, filename):
    """Does nothing for now
    Later might be changed to send data over wifi or other interface
    """

    return

def initialize_camera():
    """Initializes the camera and returns a 
    picam2 object ready to be configured
    """
    picam2 = Picamera2()
    return picam2
    
def print_sensor_modes():
    """Prints the modes of the camera
    """
    modes = picam2.sensor_modes
    print(modes)


def select_2x2_binning(picam2):
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

def select_full_res(picam2):
    """Configures the camera to take full res photos
    returns the settings
    """

    camera_config = picam2.create_still_configuration(
            {"size": (4056, 3040)},
            buffer_count=1,
            queue=False,
            controls = {'NoiseReductionMode': 0})

    return camera_config

def capture_file(picam2, iteration, file_format, location):
    if (file_format == 'dng'):
        picam2.capture_file(f"{location}{iteration}.dng", 'raw')

    else:
        picam2.capture_file(f"{location}{iteration}.{file_format}")

async def main():
    """Main function
    kwargs -- any input parameters 

    binning -- if enabled, enable 2x2 binning
    save -- if enabled write to SD
    time -- if enabled, log the fps in a seperate csv
    location -- sets the location to save to file
    file-format -- sets the file format (jpg, png, dng)
    number -- sets the number of pictures to take
              1 default, 0 -> infinite
    """
    
    #Handle all the args stuff
    parser = argparse.ArgumentParser()
    parser.add_argument(
            '-b', "--binning", action='store_true')
    parser.add_argument(
            '-s', "--save", action='store_true')
    parser.add_argument(
            '-t', "--time", action='store_true')
    parser.add_argument(
            '-l', "--location")
    parser.add_argument(
            '-f', "--file-format")
    parser.add_argument(
            '-n', "--number")
    parser.add_argument(
            '-v', "--verbose", action='store_true')

    args = parser.parse_args()
    
    #Set the settings from the args correctly
    if (args.file_format not in VALID_FILE_TYPES and args.file_format != None):
        print("Invalid file format")
        return

    file_format = 'jpg'
    if (args.file_format != None):
        file_format = args.file_format


    
    #kinda pointless code so then these vars are saved as true booleans
    #rather than True or None
    if (args.binning):
        binning = True
    else:
        binning = False

    if (args.save):
        save = True
    else: 
        save = False

    if (args.time):
        time_logging = True
    else:
        time_logging = False

    if (args.verbose):
        verbose = True
    else:
        verbose = False


    #Handles the location settings
    location = ''
    if (args.location != None):
        save = True
        location = args.location
        if (location[-1] != '/'):
            location += '/'


    #handles the iteration stuff, default is 1
    iterations = 1
    if (args.number != None):
        iterations = int(args.number)

    #Initializes the camera
    picam2 = initialize_camera()


    #Set the appropirate camera config
    if (binning == True):
        camera_config = select_2x2_binning(picam2)
        picam2.configure(camera_config)
    else:
        camera_config = select_full_res(picam2)
        picam2.configure(camera_config)


    #Start the camera
    picam2.start()

    #Here incase photos written to array
    image_array=[[0,0,0]]

    #start the time logging stuff if enabled
    if (time_logging):
        timer = timeLogging()
        timer.start_timer()

    try:
        i = 0
        while True:
            #Take the photo and maybe save it
            if (save):
                capture_file(picam2, i, file_format, location) 

            else:
                image_array, _ = await asyncio.gather(
                        take_picture(picam2),
                        save_to_file(image_array, f'{i}.txt'))
            #If logging is enabled then log it, and if verbose, then print
            if (time_logging):
                if (verbose):
                    timer.log_and_print()
                else:
                    timer.log()

            #Count the iterations
            i += 1
            if (i == iterations):
                break
    #If KeyboardInterrupt to break the while True, time data is still saved
    except KeyboardInterrupt:
        if (time_logging):
            timer.save_to_file(location)
            print("saved to file")
        return
    if (time_logging):
        timer.save_to_file(location)
        print("saved to file")

            



# Run the main function
if __name__ == "__main__":
    asyncio.run(main())


