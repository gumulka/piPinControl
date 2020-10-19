# pi Pin Controller

A small programm to connect the gio pins of the raspberry pi
to an mqtt broker.

## Build requirements
This programm need the following libraries:
  * pigpio
  * paho mqtt

You can install pigpio on rasbian using
  sudo apt install pigpio

For paho mqtt you have to download from
https://github.com/eclipse/paho.mqtt.c/releases
and follow the instructions provided.
It has been testet with Version 1.2.0 (Paho 1.3)


## Installation
call make in the main directory and start the program using

./piPinConnector

## Service
It can also be installed as a service. Just create a
file called piControl.service inside /etc/systemd/service/
directory and past in the following content:

```
[Unit]
Description=GPIO to MQTT translator

[Service]
Type=simple
ExecStart=/path/to/this/repository/piPinControl/piPinConnector
Restart=always
RestartSec=3


[Install]
WantedBy=multi-user.target
```

### Additional Ports

You can extend the output ports of the raspberry pi by using
one or more 74HC595. For this the following pin layout is
necessary:

PI | 74HC595
---|--------
5 | STCP
MOSI | DS
SLK | SHCP
GND | MR
GND | OE

MR and OE can also be connected to IO pins to be more flexible.

## Report Bugs

Please open an issue in the Github project: https://github.com/gumulka/piPinControl/issues
