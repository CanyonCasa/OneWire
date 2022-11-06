/*
 *	w1iolib.h
 *
 * Copyright (c) 2020 Enchanted Engineering
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __W1IOLIB_H
#define __W1IOLIB_H

// #include <linux/completion.h>
// #include <linux/device.h>
// #include <linux/mutex.h>

// #include "w1_family.h"

// #define W1_MAXNAMELEN		32

// OneWire bus commands...
#define W1_SEARCH		0xF0
#define W1_ALARM_SEARCH		0xEC
// #define W1_CONVERT_TEMP		0x44
#define W1_SKIP_ROM		0xCC
//#define W1_READ_SCRATCHPAD	0xBE
#define W1_READ_ROM		0x33
#define W1_READ_PSUPPLY		0xB4
#define W1_MATCH_ROM		0x55

//#define W1_SLAVE_ACTIVE		0

typedef struct {
    u8      b[8];
    u8      x;
} rom, seed;

typedef  struc {
    u8      busy;       // flag to indicate bus waiting on a parasitic device
    pin     gpio;       // GPIO line of the bus
} w1bus;


	/** the first parameter in all the functions below */
	void    *data;

    // primitive bus reset and presence detect operation
	u8		(*reset_bus)(void *);

	// primitive-level function to read a single bit from the bus
	u8		(*read_bit)(void *);

	// low-level fuction to read a single byte of data from the bus
    u8		(*read_byte)(void *);

    // higher-level function to read an array of bytes to the bus
	u8      (*read_bytes)(void *, u8 *, int);

	// low-level function to write a single bit to the bus 
	void    (*write_bit)(void *, u8, u8);

    // low-level function to write a single byte to the bus
	void    (*write_byte)(void *, u8, u8);

    // higher-level function to write an array of bytes to the bus
	void    (*write_bytes)(void *, const u8 *, int, u8);

    // single address bit operation for search acceleration mode; 2 reads & a write 
	u8      (*triplet)(void *, u8);

    // low-level bus b-tree search routine
	rom     (*search)(void *, seed *, rom *, u8);

} w1bus;



int w1_create_master_attributes(struct w1_master *);
void w1_destroy_master_attributes(struct w1_master *master);
void w1_search(struct w1_master *dev, u8 search_type, w1_slave_found_callback cb);
void w1_search_devices(struct w1_master *dev, u8 search_type, w1_slave_found_callback cb);
struct w1_slave *w1_search_slave(struct w1_reg_num *id);
void w1_search_process(struct w1_master *dev, u8 search_type);
struct w1_master *w1_search_master_id(u32 id);

/* Disconnect and reconnect devices in the given family.  Used for finding
 * unclaimed devices after a family has been registered or releasing devices
 * after a family has been unregistered.  Set attach to 1 when a new family
 * has just been registered, to 0 when it has been unregistered.
 */
void w1_reconnect_slaves(struct w1_family *f, int attach);
void w1_slave_detach(struct w1_slave *sl);

u8 w1_triplet(struct w1_master *dev, int bdir);
void w1_write_8(struct w1_master *, u8);
u8 w1_read_8(struct w1_master *);
int w1_reset_bus(struct w1_master *);
u8 w1_calc_crc8(u8 *, int);
void w1_write_block(struct w1_master *, const u8 *, int);
void w1_touch_block(struct w1_master *, u8 *, int);
u8 w1_read_block(struct w1_master *, u8 *, int);
int w1_reset_select_slave(struct w1_slave *sl);
void w1_next_pullup(struct w1_master *, int);

static inline struct w1_slave* dev_to_w1_slave(struct device *dev)
{
	return container_of(dev, struct w1_slave, dev);
}

static inline struct w1_slave* kobj_to_w1_slave(struct kobject *kobj)
{
	return dev_to_w1_slave(container_of(kobj, struct device, kobj));
}

static inline struct w1_master* dev_to_w1_master(struct device *dev)
{
	return container_of(dev, struct w1_master, dev);
}



#endif /* __W1_H */
© 2022 GitHub, Inc.
Terms
Privacy
Security
Status
Docs
Contact GitHub
Pricing
API
Training
Blog
About
