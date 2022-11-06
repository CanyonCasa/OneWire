# All things OneWire. 
Code and documentation for Maxim (Dallas Semiconductor) 1-wire interface.

## CircuitPython
Library of OneWire operations for Adafruit CircuitPython, derivative of MicroPython

Code presently worked to proof of concept for a broker application in development. Device drivers in limbo.

## Raspberry Pi
Project to write a simple bitbang Linux kernal driver. Goal is to have a data agnostic driver, similar to other serial devices such as serial ports, I2C, and SPI buses. Driver provides a bus reset function, (low level) bit read/write, byte read/write, and bytearray read/write operations. Writing a Linux kernel driver has so far proven a bit outside of my skillset as I haven't written much C code.

## OneWire Brief PowerPoint
My documentation and notes on OneWire
