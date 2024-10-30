# ProjectNotes

*Keywords:	Raspberry Pi, RPi, Node-red, 1-wire, OneWire.

## Node-red Applications

### OneWire (1-wire) Operation Within Node-red

**Limited Searches** - To limit continuous wire searches for a fixed application set the search count to a low number, such as 2. This will stop searching after 2 attempts. To do so, add the paramater wire.search_count=2 to the cmdline.txt file in /boot. Verify the limit by checking the file /sys/devices/w1_bus_master1/w1_master_attempts after boot. Note: the driver performs searches at the rate specified in /sys/devices/w1_bus_master1/w1_master_timeout, 10 seconds by default, so it takes some time after boot to complete.

**Write Permissions** - The kernal driver for the bus master (incorrectly) restricts write permissions to OneWire outputs to _root_ instaed of _gpio_ group AND since the devices exist in /sys space, the permissions get (re)set on each boot. To overcome this create a service to override permissions at boot. As root, defince /etc/systemd/system/w1_gpio_permissions.service with the contents:


 ```
[Unit]
Description=Make OneWire GPIO writeable for non-root for use with port devices
Before=nodered.service

[Service]
Type=oneshot
User=root
ExecStart=/bin/bash -c "/bin/chown root:gpio /sys/bus/w1/devices/*/output && /bin/chmod 664 /sys/bus/w1/devices/*/output"

[Install]
WantedBy=multi-user.target
```

**read/Write Errors** - Due to random and sporadic read/write errors (occuring ~1-10 times a day with a 1 minute operating loop that performs ~6 read and 6 write operations per cycle) by the kernal driver it's necessary to trap errors and retry. I/O has never logged any fails With 3 tries.
