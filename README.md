# VrGlassViaRpi
## Purpose of the system
To create 3D glasses that can display 3D content with a 3D effect. 

## How
Using Gear VR Optic System from Samsung, two 1.3 inch LCD Displays and two RPI Zero W to realize a VR system.
### Architecture
![Alt-Text](/doc/pic/vrGlassViaRpiArchitecture.png "VrGlassViaRpi - Architecture")

## Motivation 
My motivation was to learn how to build such a system and to reuse parts or the whole project for following projects.

## Install Instructions
### Precondition
32 GB SDcard (e.g.: SanDisk Extreme) with Raspberry Pi OS Lite (bullseye)
OS image donwload below: https://www.raspberrypi.com/software/operating-systems/

    sudo raspi-config
    enable interface SPI for the LCD display (and additional I2C on the Rpi for the right Eye LCD).

### Rpi Zero W

    sudo apt update
    sudo apt upgrade
    sudo apt install -y cmake git
    sudo apt install -y libjpeg9-dev
    sudo apt install -y ffmpeg
    
#### Install framebuffer LCD display driver

    cd ~
    git clone https://github.com/juj/fbcp-ili9341.git
    cd fbcp-ili9341
    mkdir build
    cd build
    cmake -DST7789VW=ON -DGPIO_TFT_DATA_CONTROL=25 -DGPIO_TFT_RESET_PIN=27 -DGPIO_TFT_BACKLIGHT=24 -DSINGLE_CORE_BOARD=ON -DBACKLIGHT_CONTROL=ON -DDISPLAY_CROPPED_INSTEAD_OF_SCALING=ON -DSTATISTICS=0 -DSPI_BUS_CLOCK_DIVISOR=8 -DDISPLAY_ROTATE_180_DEGREES=OFF ..

    make
    
    The result is the file: fbcp-ili9341.
    If you modify your config.txt(see description below), then can this programm switch the display output from HDMI output to the LCD display.
    After a reboot is the HDMI still the normal output. Below you will found a description to add this inside autostart(LCD display active).
    
#### Install ffmpeg streamer

    git clone https://github.com/jacksonliam/mjpg-streamer.git
    cd mjpg-streamer/mjpg-streamer-experimental
    make

    sudo cp mjpg_streamer /usr/local/bin
    sudo cp output_http.so input_file.so input_uvc.so /usr/local/lib/
    sudo cp -R www /usr/local/www

    sudo nano ~/.bashrc
    add to the last line
    export LD_LIBRARY_PATH=/usr/local/lib
    and enter ans save.

#### modify the config.txt options
>sudo nano /boot/config.txt

    hdmi_force_hotplug=1
    hdmi_cvt=240 240 60 1 0 0 0
    hdmi_group=2
    hdmi_mode=87
    display_rotate=3

    Info: clockwise
    #display_rotate:
    #0 =   0°
    #1 =  90°
    #2 = 180°
    #3 = 270°
    ....

    ....
    [pi4]
    # Enable DRM VC4 V3D driver on top of the dispmanx display stack
    #dtoverlay=vc4-fkms-v3d
    #max_framebuffers=2
    
    [all]
    #dtoverlay=vc4-fkms-v3d
    gpu_mem=128
    dtoverlay=w1-gpio

#### optional - Launching the display driver at startup with systemd

    cd /home/pi/fbcp-ili9341/build/
    sudo  mv fbcp-ili9341 fbcp
    sudo cp fbcp /usr/local/sbin
    cd ..
    cp fbcp-ili9341.conf fbcp.conf
    cp fbcp-ili9341.service fbcp.service
    modify fbcp.service and change all names from fbcp-ili9341 into fbcp
    sudo install -m 0644 -t /etc fbcp.conf 
    sudo install -m 0755 -t /etc/systemd/system fbcp.service 

    sudo systemctl daemon-reload
    sudo systemctl enable fbcp && sudo systemctl start fbcp
    
    ------------------------------------------------------------
    
    sudo reboot

### Config the Network
    All system must be in the same network.

    sudo nano /etc/wpa_supplicant/wpa_supplicant.conf
    
    modify it and use your own settings:
    
        ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
        
        update_config=1
        country=DE

        network={
                ssid="MyHotspot"
                psk="MyPWDfromHotspot"
                key_mgmt=WPA-PSK
        }


