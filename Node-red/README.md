# Replacement node-red-contrib-1wire node
Replacement for the **node-red-contrib-1wire** node that supports both port and temperature devices. To subflow wrappers are included add necessary properties for the particular/specific device type.

Note: only the 1wire.js file changed and can simply be copied into your install under the respective folder.

## Port Driver Permissions
**If you run Node-red without root permissions as I do then note the following.**

Use of the DS2408/DS2413 port devices requires Node-red to have write permission for the device output file, as in:

    -rw-rw-r-- root gpio ... output

Where the node-red user has gpio permissions. 
Since this output file exists in the _/sys_ namespace this must be reestablished after each boot (as it does not persist). I use a _systemd_ script to achieve this.

Create the file _/etc/systemd/system/w1_gpio_permissions.service_ that includes the following:

    [Unit]
    Description=Make OneWire GPIO writeable for non-root for use with port devices
    Before=nodered.service

    [Service]
    Type=oneshot
    User=root
    ExecStart=/bin/bash -c "/bin/chown root:gpio /sys/bus/w1/devices/*/output && /bin/chmod 664 /sys/bus/w1/devices/*/output"

    [Install]
    WantedBy=multi-user.target

Then enable the service to run:

    systemctl enable w1_gpio_permissions.service


## WARNINGS

### I/O Errors
Using node-red 2.x on RPi, I experienced many device I/O errors with the original driver, which I believe originate with the Linux kernel driver. This driver wraps each I/O operation in a loop that attemps the operation 3 times before failing, which seems to have eliminated the problem. (I have only ever seen a single failure, so the driver succedes on the second try but I added an additional try for robustness.)

## Temperature fix
Note the original driver does not handle Fahrenheit conversion correctly. A separate fix is included for simply that change.

