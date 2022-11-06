from onewire import OneWireBus, Device


class OneWirePort(Device):

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address)
    
class DS2408(OneWirePort):
    """Device support for DS2408 8-bit I/O port."""

    # device specific constants...
    FAMILY = 0x29
    DESC = 'DS2408 (0x29) 8-bit I/O port'

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address, params)

Device.register(DS2408.FAMILY,DS2408)


class DS2413(OneWirePort):
    """Device support for DS2413 2-bit I/O port."""

    # device specific constants...
    FAMILY = 0x3A
    DESC = 'DS2413 (0x3A) 2-bit I/O port'

    def __init__(self, bus: OneWireBus, address: bytearray, params: dict):
        super().__init__(bus, address, params)

Device.register(DS2413.FAMILY,DS2413)
