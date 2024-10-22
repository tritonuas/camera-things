from picamera2 import Picamera2, Preview
import time
import asyncio
from PIL import Image
import numpy as np



async def take_picture(cam):
    return cam.capture_array("main")

async def save_to_file(image_list, filename):
    # Convert the numpy array to a PIL Image object
    image_array = np.array(image_list, dtype=np.uint8)
    image = Image.fromarray(image_array)
    
    # Save the image as a JPG file
    image.save(f"{filename}.jpg", "JPEG")
    print(f"Image saved to {filename}.jpg")


async def main():

    picam2 = Picamera2()
    camera_config = picam2.create_still_configuration({"size": (4056, 3040)}, buffer_count=1, queue=True, controls = {'NoiseReductionMode': 0})
    picam2.configure(camera_config)
    check = picam2.camera_configuration()['raw']
    print(camera_config)
    print(picam2.camera_controls)


    picam2.start_preview(Preview.NULL)
    picam2.start()

    current_time = time.time()
    intergral = 0

    image_array=[[0,0,0]]
    for i in range(10):
        temp_current_time = time.time()
        print(temp_current_time - current_time)
        intergral += temp_current_time - current_time
        current_time = temp_current_time

        #image_array = await take_picture(picam2)
        #await save_to_file(image_array, f'captured_image_{i}.npy')
        iamge_array, _ = await asyncio.gather(take_picture(picam2), save_to_file(image_array, f'captured_image_{i}'))

# Run the main function
if __name__ == "__main__":
    asyncio.run(main())


