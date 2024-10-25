from picamera2 import Picamera2, Preview
import time
import asyncio
from PIL import Image
import numpy as np
import argparse



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
    """Initializes the camera and returns a picam2 object ready to be configured
    """
    picam2 = Picamera2()
    return picam2
    
def print_sensor_modes():
    """Prints the modes of the camera
    """
    modes = picam2.sensor_modes
    print(modes)


def select_2x2_binning(picam2):
    """Configures the settigns to use 2x2 binning mode and returns it
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
async def main():
    """Main function
    kwargs -- any input parameters 

    binning -- boolean, if true, 2x2 binning, false, full re 
    write_to_sd -- boolean, if true: write to SD, if false: don't
    """

    '''
    args = parser.parge_args()
    binning = args.binning
    write_to_sd = args.write_to_sd


    '''
    picam2 = initialize_camera()
    binning = True
    if (binning == True):
        camera_config = select_2x2_binning(picam2)
        picam2.configure(camera_config)
    
    else:
        camera_config = select_full_res(picam2)

        picam2.configure(camera_config)


    


    picam2.start()

    current_time = time.time()
    intergral = 0

    image_array=[[0,0,0]]

    for i in range(20):
        temp_current_time = time.time()
        print(temp_current_time - current_time)
        intergral += temp_current_time - current_time
        current_time = temp_current_time
        image_array, _ = await asyncio.gather(take_picture(picam2), save_to_file(image_array, f'{i}.txt'))

        #picam2.capture_file(f"{i}.jpg")
    print(intergral) 


# Run the main function
if __name__ == "__main__":
    asyncio.run(main())


