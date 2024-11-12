# Camera Things
Code for the raspberry pi that takes pictures from a raspberrypi camera HQ, and tags images with metadata via mavlink

## Dependencies
pymavlink
picamera2
pyserial (NOT serial)

## Installation
First create a python virtual enviroment
```python -m venv .venv```

Then activate it
```source .venv/bin/activate```

Then install the needed packages

```pip install pymavlink pyserial picamera2```

## Configuration
### Raspberry Pi Configuration

```sudo raspi-config```

Select "Interfacing Options" -> "Serial"

Select **no** to "Would you like a login shell to be accessible over serial?".
Select **yes** to "Would you like the serial port hardware to be enabled?".

The Pi's serial port will be on ```/dev/serial0```

### Ardupilot configuration

TODO

## Running

Run main.py to start taking photos

#### Command line arguments

```
--binning Enable 2x2 binning
--save Save the photos
--time Enable time logging
--location Sets the file location
--file-format The file format to save the files (jpg, png, dng) Default: jpg
--number The number of photos to take. Default 1, 0 for infinite photos
--verbose If enabled, print the time data
--mavlink If enabled send mavlink messages to request attitude and GPS data
```

