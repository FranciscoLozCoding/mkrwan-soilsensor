# LoraWan Soil Sensor, a MKR WAN 1310 CSU Soil Sensor

This repo shows how to connect a CSU soil sensor to a MKR WAN 1310 to give the sensor the ability to communicate via LoraWAN.

- This project was build off of work done by [amalaquias](https://github.com/amalaquias).
    - https://github.com/waggle-sensor/summer2022/tree/main/aldo

---

## Table of Contents
1. [Features](#features)
1. [Hardware Needed](#hardware-needed)
1. [Installing Software](#installing-software)
1. [Connecting MKR WAN 1310 to Sensor](#connecting-mkr-wan-1310-to-sensor)
1. [Connect MKR WAN 1310 to Computer](#connect-mkr-wan-1310-to-computer)
1. [Testing Soil Sensor Using Chirpstack](#testing-soil-sensor-using-chirpstack)
1. [LoRaWAN Downlink Commands](#lorawan-downlink-commands)

## Features

- Measures soil temperature and soil moisture in volts
- Communicates via LoRaWAN
- Receives LoRaWAN Downlink Commands

## Hardware Needed

- Micro USB Wire (other end must be able to connect to your computer)
- [Arduino MKR WAN 1310](https://store-usa.arduino.cc/products/arduino-mkr-wan-1310?selectedStore=us)
- Modified Colorado State University soil sensor with four jumper wires with female input
- LoRaWAN Gateway
    - we used [Rak Discover Kit 2](https://store.rakwireless.com/products/rak-discover-kit-2?variant=39942870302918)

## Installing Software

1. To connect to the MKR WAN 1310 board, you will need to install the [Arduino IDE](https://support.arduino.cc/hc/en-us/articles/360019833020-Download-and-install-Arduino-IDE)
1. Once you installed the IDE, you need to further install the board's software support by following the [SAMD21 core for MKR boards Documentation](https://docs.arduino.cc/software/ide-v1/tutorials/getting-started/cores/arduino-samd)

   <img src='./images/software_download.jpeg' alt='software download' height='200'>
1. You will also need the library for mkrwan. Under Library Manager, look up "mkrwan" and install "MKRWAN by Arduino".

   <img src='./images/library_download.jpeg' alt='library download' height='400'>
    
    >NOTE: At the time of configuring the board MKRWAN_v2 was not used because of bug issues related to the library.

## Connecting MKR WAN 1310 to Sensor

1. Grab the soil sensor

    <img src='./images/sensor.jpeg' alt='Sensor' height='400'>

2. Connect the wires from the sensor to the MKR WAN 1310 as shown:

    <img src='./images/sensor_connected.jpeg' alt='Connected Board' height='500'>

    > Note: red wire is for power, black is for ground, white for soil moisture, and blue for soil temperature

## Connect MKR WAN 1310 to Computer

1. Connect the board to your computer with the Micro USB wire
   1. You should see a green light glow on the board
1. Go to Tools in Arduino IDE and select `FILL OUT` for Board and select the correct serial port for the arduino as shown:

    `IMAGES GO HERE`

## Testing Soil Sensor Using Chirpstack

[Chirpstack](https://www.chirpstack.io/) was used to setup our LoRaWAN network and a RAK Discover Kit 2 was used as our Gateway.

>NOTE: We are using `US915` as our LoraWAN region. This must be changed in `main.ino` for different countries.

1. Retrieve your MKR WAN 1310's DevEUI by using `main.ino` in your `Arduino IDE`. The serial monitor will display your DevEUI.

    <img src='./images/DevEUI.jpeg' alt='DevEUI' height='50'>

    >NOTE: The program will fail because the device hasn't been given an App key

1. Using Chirpstack's UI, add your device using `OTAA` following [Chirpstack's Documentation](https://www.chirpstack.io/docs/guides/connect-device.html)

1. Once you've added your device, generate an application key or create one via the 'OTAA Keys' tab.

    <img src='./images/app_key.png' alt='App key' height='400'>

1. Provide the application key to `arduino_secrets.h`

    >NOTE: Chripstack does not use `APP EUI` when connecting devices via `OTAA` so this can be left as is.

1. Run `main.ino` in your `Arduino IDE`, if the device connects successfully the serial monitor will display the values the device is sending and chirpstack will receive a `join request` then the device's values.

    <img src='./images/values_sent.jpeg' alt='values sent' height='100'>

    <img src='./images/uplink_packet.png' alt='Uplink Packet' height='500'>

    >NOTE: The device might take a couple of minutes to join the LoRaWAN network.

1. Finally for chirpstack to decript the uplink packets, provide the `codec.js` via the Device Profile's Codec tab.

    <img src='./images/codec_tab.png' alt='Codec Tab' height='500'>

Viewing the uplink packets by clicking `up` in the device's events tab will now display the measurements and its values.

<img src='./images/decoded_packets.png' alt='Decoded Packets' height='700'>

## LoRaWAN Downlink Commands

Using the Network Server’s portal or API to send a downlink command, the device will respond to the ack. The downlink command takes effect and responds the next time the device uploads data.

### Changing the Data Uplink Interval

1. Send the downlink command (HEX) via Fport=1.

    Command List:

    | Description | Command |
    | ------------- |:-------:|
    | Set Uplink interval = 1 min (default) | 00 |
    | Set Uplink interval = 5 min | 01 |
    | Set Uplink interval = 15 min | 02 |
    | Set Uplink interval = 30 min | 03 |
    | Set Uplink interval = 1 hr | 04 |

2. Example: Set the device's uplink interval to 5 minutes using Chirpstack.

    <img src='./images/downlink_command.png' alt='Downlink Command' height='600'>

    Pressing `Enqueue` will send the downlink command next time the device uploads data. Essentially placing the command in a *queue*.
