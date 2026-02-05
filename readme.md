# Pi Camera UDP Server | Camera-things

This is a repository for a camera server that runs on a Raspberry Pi Zero 2 W and sends images to a Jetson Orin Nano via UDP.

## Dependencies

- Raspberry Pi Zero 2 W
- Jetson Orin Nano
- Micro-USB to USB-A data cable

## Quick Setup

1. [Setup the Networking Connection](#network-setup)
2. Then setup the codebase below:

```bash
git clone git@github.com:tritonuas/camera-things.git
cd camera-things

# Install dependencies
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkg-config \
    libcamera-dev \
    libevent-dev \
    libusb-1.0-0-dev

# Build the project
mkdir build && cd build
cmake ..    # configures the CMake build system
make

# Run the server
./raspy
```

## Config Files

The server uses one config file `config/picam.json`. 

```json
{
    "UART_NAME": "/dev/serial0",    // UART port to use for MAVLink communication
    "BAUDRATE": 57600,              // Baudrate for MAVLink communication
    "DEBUG_MODE": 1,                // 1 for debug mode, 0 for normal mode
    "SEND_TO_OBC": 0,               // 1 to send images to OBC, 0 to send to Jetson
    "MAVLINK_ENABLED": 1            // 1 to enable MAVLink communication, 0 to disable
}
```

## Network Setup

### 1. Enable USB Gadget Mode & Set Static MAC
We need to load the necessary kernel modules and force a specific MAC address so the Jetson always recognizes the Pi as the same device.

1.  Open `config.txt`:
    ```bash
    sudo nano /boot/firmware/config.txt
    ```
2.  Add this line to the very bottom:
    ```text
    dtoverlay=dwc2
    ```
3.  Save and exit (`Ctrl+O`, `Enter`, `Ctrl+X`).

4.  Open `cmdline.txt`:
    ```bash
    sudo nano /boot/firmware/cmdline.txt
    ```
5.  Find the line of text. Do not create a new line. Add the following text strictly after `rootwait`:
    ```text
    modules-load=dwc2,g_ether g_ether.host_addr=00:dc:c8:f7:75:14 g_ether.dev_addr=00:dd:dc:eb:6d:da
    ```
    *Make sure there is a space before and after this addition.*

### 2. Configure Static IP (192.168.77.2)
We configure NetworkManager to assign a static IP to the `usb0` interface.

```bash
# Create the connection profile
sudo nmcli con add type ethernet con-name "usb-gadget" ifname usb0

# Set Static IP
sudo nmcli con modify "usb-gadget" ipv4.addresses 192.168.77.2/24
sudo nmcli con modify "usb-gadget" ipv4.method manual

# Disable IPv6 to reduce overhead (optional but recommended)
sudo nmcli con modify "usb-gadget" ipv6.method ignore
```

### 3. Add Crontab "Kickstarter"
To ensure the network comes up reliably on every boot, we add a cron job to force the connection.

1.  Open crontab:
    ```bash
    sudo crontab -e
    ```
2.  Add this line to the bottom:
    ```text
    @reboot sleep 30 && /usr/bin/nmcli con up "usb-gadget" > /var/log/gadget_connect.log 2>&1
    ```

3.  **Reboot the Pi:** `sudo reboot now`

---

### 4. Physical Connection & Testing

#### 1. Connect the Hardware
1.  Plug the **Micro-USB** end into the **Data Port** of the Pi Zero 2 W (the port labeled "USB", closest to the center).
2.  Plug the **USB-A** end into the Jetson Orin Nano.

#### 2. Verify Connectivity
Wait about 45-60 seconds after plugging in (for the Pi to boot and the Crontab job to run).

On the Jetson:
```bash
# 1. Check if the interface exists
ip link show
# Look for an interface with MAC 00:dc:c8:f7:75:14

# 2. Ping the Pi (we should see the packet returning)
ping 192.168.77.2
```