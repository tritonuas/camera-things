from camera import Camera
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
    parser.add_argument(
            '-m', "--mavlink", action='store_true')
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

    if (args.mavlink):
        mavlink_enabled = True
    else:
        mavlink_enabled = False




    #Handles the location settings
    location = ''
    if (args.location != None):
        location = args.location
        save = True


    #handles the iteration stuff, default is 1
    iterations = 1
    if (args.number != None):
        iterations = int(args.number)

    rpicam = Camera(
            binning=binning,
            save=save,
            time=time_logging,
            location=location,
            file_format=file_format,
            verbose=verbose,
            mavlink=mavlink_enabled)


    rpicam.initialize_and_configure()

    #Start the camera
    rpicam.start()

    print(iterations)
    await rpicam.take_photos(iterations)





            



# Run the main function
if __name__ == "__main__":
    asyncio.run(main())


