# MIT License
# Derived from Adafruit CircuitPython OneWire library
"""
`onewire`
====================================================
Implements a full 1-Wire bus protocol and extendible set of supported devices
OneWireBus supports low level reset, bitread, bit write operations of core
onewireio module, as well as higher level-byte and block (i.e. bytearray) 
read/write operations and device discovery (scan).

See readme for details 

* Author(s): CanyonCasa
"""

#__version__ = "0.0.0-auto.0"
#__repo__ = "https://github.com/canyoncasa/OneWire.git"

from microcontroller import Pin
import onewireio
from time import monotonic as mark, sleep


class OneWireBus:
    """A class to represent a 1-Wire bus/pin."""

    # OneWire bus commands...
    SEARCH_ROM = 0xF0
    REGISTERED = {}     # Holds defined device types used to auto assign found devices

    def __init__(self, pin: Pin) -> None:
        self.io = onewireio.OneWire(pin)
        self.readbit = self.io.read_bit
        self.writebit = self.io.write_bit
        self.reset = self.io.reset
        self.busyflag = False

    @property
    def busy(self):
        return self.busyflag
    @busy.setter
    def busy(self, state):
        self.busyflag = state
        return self.busyflag

    @staticmethod
    def crc8(data: bytearray) -> int:
        """Perform the 1-Wire CRC check on the provided data."""
        crc = 0
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x01:
                    crc = (crc >> 1) ^ 0x8C
                else:
                    crc >>= 1
                crc &= 0xFF
        return crc

    @staticmethod
    def crc16i(data: bytearray, seed: int = 0, invert: bool=True, as_bytes: bool=True):
        """Generates CRC16, inverted, as bytearray, by default for appending to data block"""
        oddparity = [0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0]
        crc = seed
        for i in range(len(data)):
            cdata = data[i]
            cdata = (cdata ^ crc) & 0xFF
            crc = crc >> 8
            if oddparity[cdata & 0x0F] ^ oddparity[cdata >> 4]:
                crc = crc ^ 0xC001
            cdata = cdata << 6
            crc = crc ^ cdata
            cdata = cdata << 1
            crc = crc ^ cdata
        if invert:
            crc = crc ^ 0xFFFF
        return crc if not as_bytes else bytearray([crc&0xFF,(crc>>8)&0xFF])

    @staticmethod
    def crc16check(data: bytearray):
        """Checks a data block with an inverted crc16 and return zero for valid data"""
        crc = OneWireBus.crc16i(data, 0, False, False)
        return crc ^ 0xB001

    @staticmethod
    def bytes2hex(b: bytearray) -> str:
        """Converts a bytearray to hex string format"""

        return '' if not b else " ".join(list(map(lambda x: "{:02x}".format(x).upper(), b)))

    def define_device(self, address, params: dict, dev_class=None):
        if type(address)==tuple:    # direct from scan function
            return self.define_device(address[0],params, callback)
        elif type(address)==bytearray:
            pass
        elif type(address)==str:
            sn = address.replace(' ','').replace('-','')
            address = bytearray([int(sn[i:i+2],16) for i in range(0,len(sn),2)])
            if len(address)==7:
                address.append(self.crc8(address))
        else:
            return None
        if dev_class:
            return dev_class(self, address, params)
        return Device(self, address)

    def read(self, n: int) -> bytearray:
        """Reads n number of bytes from the bus and returns as a bytearray."""

        buf = bytearray(n) if type(n)==int else n   # accomodate a buffer or buffer size
        for i in range(n):
            buf[i] = self.readbyte()
        return buf

    def readbyte(self) -> int:
        val = 0
        for i in range(8):
            val |= self.io.read_bit() << i
        return val

    @staticmethod
    def register(family: int, device_class):
        OneWireBus.REGISTERED[family] = device_class

    def scan(self) -> list:
        """Scan bus for devices present and return a list of valid addresses."""

        addresses = []
        diff = 65
        rom = None
        while True:
            rom, diff = self._search_rom(rom, diff)
            if rom==None: return addresses
            if not self.crc8(rom):  # zero for a valid address
                # rom as bytearray
                family = rom[0]
                hx = self.bytes2hex(rom)    # defacto hex string
                hxx = hx.replace(' ','')    # hexstring, no spaces
                nr = (hxx[:2] + '-' + hxx[2:14]).lower()    # node-red format
                rev = ' '.join(hx.split(' ')[::-1]) # reverse order
                addresses.append((family,rom,hx,hxx,nr,rev))
            else:
                print(f'WARN: OneWire ROM[{rom}] failed CRC! Device ignored')
            if not diff:
                break
        return addresses

    def status(self,dump=False):
        r = self.reset()
        if dump:
            print(f'Reset: {('Bus OK', 'Bus fault')[r]}')
            print('Registered devices...')
            for f,d in OneWireBus.REGISTERED.items():
                print(f'  {f:02}:   {d.DESC}')
        return r
        
    def write(self, buf: bytearray) -> None:
        """Write the bytes from ``buf`` to the bus."""

        for i in range(len(buf)):
            self.writebyte(buf[i])

    def writebyte(self, value: int) -> None:
        for i in range(8):
            bit = (value >> i) & 0x1
            self.io.write_bit(bit)

    def _search_rom(self, l_rom: bytearray, diff: int) -> tuple: # (bytearray, int)
        if self.io.reset():    # False when any devices present
            return None, 0
        self.writebyte(OneWireBus.SEARCH_ROM)
        if not l_rom:
            l_rom = bytearray(8)
        rom = bytearray(8)
        next_diff = 0
        i = 64
        for byte in range(8):
            r_b = 0
            for bit in range(8):
                # accelerate mode read true bit, then false bit, then write desired
                b = self.readbit()  # 1st address bit read
                if self.readbit():  # 2nd address bit read
                    if b:  # 11: there are no devices or there is an error on the bus
                        return None, 0
                else:
                    if not b:  # 00: collision, two devices with different bit meaning
                        if diff > i or ((l_rom[byte] & (1 << bit)) and diff != i):
                            b = 1
                            next_diff = i
                self.writebit(b)
                r_b |= b << bit
                i -= 1
            rom[byte] = r_b
        return rom, next_diff


class Device:
    """A base class to represent any single device on the 1-Wire bus."""

    FAMILY = None
    DESC = 'Generic Device not defined'
    MATCH_ROM = 0x55
    SKIP_ROM = 0xCC

    def __init__(self, bus: OneWireBus, address: bytearray):
        self.bus = bus
        self.address = address
        self.family = self.address[0]
        self.sn = self.bus.bytes2hex(self.address) # de facto hex string format
        self.desc = 'tbd'

    # select a single device on the bus
    def select(self) -> None:
        self.bus.reset()
        self.bus.write([MATCH_ROM])
        self.bus.write(self.address)

    def info(self):
        return { address: self.address, family:self.family, sn: self.sn, desc: self.desc }

    @staticmethod
    def register(family: int, device_class):
        OneWireBus.register(family, device_class)
