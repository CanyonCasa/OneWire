# MIT License
"""
Device support for DS18X20 type temperature sensors

* Author(s): CanyonCasa
"""

#__version__ = "0.0.0-auto.0"
#__repo__ = "https://github.com/canyoncasa/OneWire.git"

from onewire import OneWireBus, Device
from time import monotonic as mark, sleep


class TemperatureSensor(Device):

    TEMP_CONVERT_WAIT = 0.8 # @ 12 bits
    CONVERT_T = 0x44
    RD_SCRATCH = 0xBE
    WR_SCRATCH = 0x4E
    COPY_SCRATCH = 0x48
    
    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address)

class DS18X20(TemperatureSensor):
    # Device specific definitions
    FAMILY = 0x28
    DESC = 'DS18x20 (0x28) Temperature Sensor'

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address, params)
        bits = params.get('resolution',0)   
        self._bits = bits if bits in [9,10,11,12] else 12   # default 12
        units = params.get('units','').upper()
        self._units = units if units in ['F','C','K','R','X'] else 'F'  # default F
        self.resolution(self._bits)
        self._wait = self.TEMP_CONVERT_WAIT * 2**(self._bits-12)
        self._timex = None

    def scratchpad_copy(self):
        self.select()
        self.io.write([self.COPY_SCRATCH])

    def scratchpad_read(self) -> bytearray:
        self.select()
        self.io.write([self.RD_SCRATCH])
        sp_and_crc = self.io.read(9)
        if self.io.crc8(sp_and_crc):
            return bytearray(8)
        return sp_and_crc[0:8]

    def scratchpad_write(self, buf: bytearray) -> None:
        self.select()
        self.io.write([self.WR_SCRATCH])
        self.io.write(buf)
    
    def temperature(self, wait=False) -> float:
        # converts raw temperature to specified format
        def temp_as(raw: int, units: str) -> float:
            if units == 'C':
                return raw / 16
            elif units == 'F':
                return (raw / 16) * 1.8 + 32
            elif units == 'K':
                return (raw / 16) + 273.15
            elif units == 'R':
                return (raw / 16) * 1.8 + 491.67
            else:
                return "0x{:04X}".format(raw)
        # load scratchpad and extract temperature
        def getT():
            self.select()
            buf = self.scratchpad_read()
            raw_temp = (buf[1]<<8) + buf[0]
            return temp_as(raw_temp,self._units)

        if not self.busy():
            # begin conversion
            self.select()
            self.io.write([self.CONVERT_T])
            if wait:
                sleep(self._wait)
                return getT()
            else:
                self._timex = mark() + self._wait
                self.busy(True)
        else:
            if mark() < self._timex:
                return None
            # conversion ready
            t = getT()
            self.busy(False)
            self._timex = None
            return t
        return None

    # sets a temperature resolution
    def resolution(self, bits: int) -> int:
        self.select()
        sp = self.scratchpad_read()
        cfg = (bits-9) << 5 | 0x1F
        buf = bytearray([sp[2], sp[3], cfg])
        self.scratchpad_write(buf)
        self.scratchpad_copy()
        return bits

Device.register(DS18X20.FAMILY, DS18X20)
