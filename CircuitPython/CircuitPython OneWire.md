# Circuitpython OneWire.py

>_**Derived from the Adafruit OneWire CircuitPython library BUT behaves differently, noteably, it supports "parasitic" device operation (with a proper hardware interface, see information on OneWire Interfaces for more info), no *sleep* blocking (default), and integrated device management and extensibility.**_

**OneWireBus** class implements a full 1-Wire bus protocol and extendible set of supported devices. OneWireBus supports low level reset, bit read, bit write operations of the core onewireio module, as well as higher-level byte and block (i.e. bytearray) read/write operations, as well as device discovery (scan) and intialization (define_device).

### Author(s): CanyonCasa

Usage:
```python
import board
from onewire import OneWireBus

ow = OneWireBus(board.D2)
found = ow.scan()
```

## OneWireBus Class
### Low Level Functions
OneWireBus directly exposes the onewireio bus primitive and its functions for completeness though generally not accessed in normal operation:

* *_bus*: OneWire bus primitive
* *reset()*: Issue a OneWire bus reset
* *writebit(bit)*: Writes a single bit to the bus.
* *readbit()*: Reads a bit from the bus.
* *attribute _busy*: Bus busy flag.

### Bus Operations
The class extends the low level functionality with a number of bus-level operations, not generally used except in the definition of new device types:

* *readbyte()*: Reads a single byte from the bus.
* *writebyte(data: byte)*: Writes a single byte to the bus.
* *read(n:int)*: Reads *n* bytes from the bus. (Note: Does not pass a buffer, as does the Adafruit function)
* *write(buf:bytearray)*: Writes an array of bytes, buf, to the bus.
* *_search_rom()*: Internal function that makes a "bus acceleration mode" single pass of the device detection algorithm. 

### High Level Bus Operations
These functions wrap the lower level functions and bus operations to provide higher level behavior:

* *define_device(address, params:dict={}, callback=None)*: Creates a new bus specific device of the type corresponding the its family code or a base core Device if the module has no such defined type. The address parameter may be the tuple returned by the scan function or any of the first 3 formats of it. *Note, this function may be called directly with the callback set to the class of a custom device not defined in the library to automatically extend the bus functionallity with that device type. __See the custom device section for more details and example.__* 
* *scan()*: Loops the _select_rom function to find the addresses of all devices on the bus. **NOTE: This function does not behave the same as the equivalent Adafruit scan function. It returns a tuple of address formats for each device versus a OneWireDevice object.** Note also that the bus does not define a MAX_DEVICES as does the Adafruit library.

    Scan operation returns a list of device IDs, with each ID being a typle of multiple formats as defined here, referenced by tuple index as given below:

    ```
    1. bytearray(b'(\xfa\xb2\xff\x00\x00\x00\x84') # internal address
    2. '28 FA B2 FF 00 00 00 84'    # data sheet syntax
    3. '28FAB2FF00000084'           # index form
    4. '28-fab2ff000000'            # node-red SN No CRC
    5. '84 00 00 00 FF B2 FA 28'    # true bit order MSB-LSB

        where in this case 28 = family code (DS18B20), 84 = crc
    ``` 

### Utility Operations
The class includes some utility functions intended primarily for internal use but exposed for custom device development:
* *bytes2hex(address: bytearray)*: formats a bytearray into hex format matching the de facto address format of OneWire documentation. Not limited to ROM addresses.
* *crc8(data:bytearray)*: Computes the fundamental onewire bus CRC (as used for device addresses) from a data buffer. If the data includes the CRC, then function returns 0 for a valid CRC check.
* *crc16i(data,...)*: By default, computes an **inverted CRC16** and appends to the data with multiple options for seed (0), non-inverted form (True), and return value (asBytes, True). Many OneWire memories use this CRC for verification.
* *crc16check(data:bytearray)*: Performs a CRC validity check for a block of data that includes it's inverted CRC16 and returns 0 for a valid result.
* *busy*: Property (i.e. getter/setter) for bus *_busy* flag. Denotes bus is inactive and waiting for a parasitic measurement and MUST NOT execute read/write operations. User responsible for bus management at application level. In other words flag does not prevent bus activity when True.

    **Note: bytes2hex, crc8, crc16i, and crc16check all declared as static class methods.**


## OneWireDevice Class
Base (parent) class for defined devices that defines common functionally across all devices. The class is not normally used except in defining new modules for new device types.

* *attribute _bus*: Owning OneWireBus object.
* *attribute address*: 8 byte array specifying the unique part address.
* *attribute family*: byte 0 of the address specifying the part family type.
* *attribute _sn*: hex string equivalent of address.
* *_select()*: Method to address a specific device on the bus using the MATCH_ROM command.

### Device Declaration
The base Device declaration for the class takes 2 parameters:

* *bus: OneWireBus*: The owning OneWireBus instance
* *address: bytearray*: The unique 64-bit device ID in bytearray format only.

Note: The OneWireBus.define_device wrapper method does not take the same arguments. It does not include the bus argument since it is "bus aware", it pre-parses different address formats, passes a device specific params argument, and takes an optional callback class for custom classes.

### Defining Custom Device Classes
Users can easily extend the OneWireBus device classes without modification of the library. New device types should be defined using the Device class as a parent class as follows:

``` python
from onewire import OneWireBus, Device

class MY_OW_SENSOR(Device):

    # device specific constants as needed to not clutter global space...
    DEV_MY_OW_SENSOR = const(0x00)
    SENSE = const(0x00)

    # required minimum __init__ method to inherit Device class...
    def __init__(self, bus: OneWireBus, address: bytearray, params: dict={}):
        super().__init__(bus, address)

    # device methods as needed, for example ...
    def info(self):
        return (self.address, self.sn, self.family, self.SENSE)
```
All device class types should use the  declaration signature given in the example above with the defined arguments:

* *bus: OneWireBus*: The owning OneWireBus instance passed to base Device class
* *address: bytearray*: The unique 64-bit device ID in bytearray format only passed to base class Device.
* *params: dict={}*: An optional list of parameters specific to the device type.

To define an instance of the device add it's class to the OneWireBus.define_device as a callback, as follows:

```python
import board
from onewire import OneWireBus
import MY_OW_SENSOR

ow = OneWireBus(board.D2)
found = ow.scan()
# assume my sensor is found[0]
mine = ow.define_device(found[0],{},MY_OW_SENSOR)
print('Mine initialized as:',mine)
print(mine.info)
```


This form can be used to easily create new device types OR override existing built in types for custom behavior.

## DEFINED DEVICE CLASSES...
## DS18X20 Class
A class defining the DS18B20 temperature sensor family.

Example Usage:
``` python
# Assume previous setup to define OneWire bus (ow) and discover device address (address)...

outside = ow.define_device(address,{resolution: 12, units:'C'})
temp = None
while temp == None:
    temp = outside.temparture()
    # do other code tasks, except onewire bus operations...
print("It's {}C outside!".format(temp))
```
### Methods

* *scratchpad_copy()*: Writes 3 bytes of scratch pad to non-volatile memory.
* *scratchpad_read()*: Reads 8 bytes of scratch pad and checks CRC.
* *scratchpad_write()*: Writes 3 bytes to scratch pad.
* *temperature(wait:bool = False)*: Converts and returns the current device temperature. See Temperature Reading section for details. Note: This method does not work the same as the Adafruit library.
* *_resolition*: Internal function to set the device operation during __init__.

### Temperature Reading
The temperature method both starts and completes a temperature measurement. By default (i.e. wait=False), the first time called, it begins a conversion and returns *None*. On successive calls the method continues to return *None* until the measurement is complete, which then returns the value in the speicifed format. Repeatedly checking only assesses the bus busy state and whether the sensor has finished conversion; it does not poll the sensor on the bus. This enables "parasitic" measurements and allows other non-onewire activites to continue without blocking; however, in this case the user MUST ensure no other bus operations occur until the conversion completes. 

Alternately, the device can be accessed synchronously, as the Adafruit method operates, by setting the wait flag *True*, as follows. Note, this operation waits for the temperature conversion to complete before returning by calling sleep, and thus blocks other code execution for up to 800ms.

``` python
# Assume previous OneWire bus setup to define ow and address...

from onewire import DS18X20

blower = DS18X20(ow,address,{resolution: 9, units:'K'})
temp = blower.temparture(True)
print("The blower output is {} Kelvin !".format(temp))
```

### Initialization Parameters
The last argument to the device provides a dictionary of parameters for configuring the device.
* *resolution*: number of bits of the conversion: 9, 10, 11, 12 with default 12.
* *units*: units of the output reading 'F', 'C', 'K', 'R', 'X' with default 'F', where 'X' is the raw reading integer data.

## To Do
Code presently worked to proof of concept for a broker application in development. Device drivers in limbo.
