# pi Pin Controller

A small programm to connect the gio pins of the raspberry pi
to an mqtt broker.

## Build requirements
This programm need the following libraries:
  * pigpio
  * paho mqtt

You can install them on rasbian using
  sudo apt install pigpio FIXME

## Installation
call make in the main directory and start the program using



### Additional Ports

You can extend the output ports of the raspberry pi by using 
one or more 74HC595. For this the following pin layout is 
necessary:

The pin 5 has to exported as an output pin and connected to the STCP pin of the 74HC595.



## Report Bugs

Please open an issue in the Github project: https://github.com/gumulka/piPinControl/issues
