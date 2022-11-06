from onewire import OneWireBus, Device
from micropython import const


class DS2423(Device):
    """Device support for DS2423 OneWire 4kb RAM and counter."""

    # device specific constants...
    FAMILY = 0x1D
    DESC = 'DS2423 (0x1D) OneWire 4kb RAM and counter'
    SCRATCHPAD_WR = 0x0F
    SCRATCHPAD_RD = 0xAA
    SCRATCHPADD_COPY = 0x5A
    MEMORY_READ = 0xF0
    MEMORY_READ_AND_COUNTER = 0xA5

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address)

Device.register(DS2423.FAMILY,DS2423)


class DS2438(Device):
    """Device support for DS2438 OneWire Battery Gauge"""

    FAMILY = 0x26
    DESC = 'DS2438 (0x26) Battery Gauge'

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address)

Device.register(DS2438.FAMILY,DS2438)
