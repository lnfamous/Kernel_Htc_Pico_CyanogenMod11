/* drivers/misc/awesom3.c
 *
 * Code to dynamically resize the partition table for the device
 * HTC Explorer (codename 'pico')
 *
 * Copyright 2014 Vineeth Raj <contact.twn@openmailbox.org>
 *
 *         awesom3: hax the partition table!
 * Developers have long tried to sqeeze in ROMs into legacy
 * devices with low internal memory, and one of the ways to
 * do this was by using custom tailored mtd_parts, which
 * gave the kernel a partition table to use, with resized
 * partitions. This was widely used on many devices like
 * the HTC G1, MT3G, Hero, Evo4G, Desire, etc.
 *
 * While that was a viable option for the HTC Pico, a small
 * "problem" that popped up was that different phones have
 * different NAND Devices, with different partition tables
 * and pagesizes. Incase the modified custom mtd_parts even
 * *accidentally* touched the boot partition, or has been
 * offset'ed out of the available space, a partition would
 * be *lost*.
 *
 * As a solution, dynamically obtain a few amount of mB
 * from the userdata, cache and devlog partitions as
 * defined below, for the 'system' partition and
 * and hax the partition table after it's been
 * got from ATAG, and before it is used by other subsystems.
 * This should make this method work on *almost* all Pico's
 * out there, which have partition layouts as:
 *   misc:recovery:boot:system:cache:userdata:devlog
 * and whose partitions in the order:
 *   recovery:boot:system:cache:devlog:userdata:misc
 * which would be rearranged to:
 *   recovery:boot:misc:devlog:userdata:cache:system
 *
 * The sizes are not bounded by hardware. I'm not
 * responsible for the module failing due to
 * erroneus defines.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

// necessary NAND headers
#include <asm/mach/flash.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

// no. of mB's to be left in the 'cache' partition
#define CACHE_SIZE_LEAVE 8

// no. of mB's to be left in the 'userdata' partition
#define USERDATA_SIZE_LEAVE 4

// no. of mB's to be left in the 'devlog' partition
#define DEVLOG_SIZE_LEAVE 1

extern struct flash_platform_data msm_nand_data;

int configured = 0;

static int configure_awesome(void) {

	unsigned boot_part = 0;
	unsigned system_part = 0;
	unsigned cache_part = 0;
	unsigned userdata_part = 0;
	unsigned devlog_part = 0;
	unsigned misc_part = 0;

	int err = 0;

	printk(KERN_INFO "awesom3: going to try configure awesom3\n");

	unsigned count = msm_nand_data.nr_parts;
	int n = 0;

	for (n = 0; n < count; n++) {
		//let's make logs 'friendly' to read. hex is hard to 'read'.
		printk(KERN_INFO "Partition (for awesom3) %s "
				"-- Offset:%lli Size:%lli\n",
				msm_nand_data.parts[n].name, msm_nand_data.parts[n].offset, msm_nand_data.parts[n].size);

		if (!(strcmp(msm_nand_data.parts[n].name, "boot")))
			boot_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "system")))
			system_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "cache")))
			cache_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "userdata")))
			userdata_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "devlog")))
			devlog_part = n;
		if (!(strcmp(msm_nand_data.parts[n].name, "misc")))
			misc_part = n;
	}

	/* First check if we have all the partitions!
	 * Assuming it is a pico, it *should* have boot, system,
	 * cache, userdata, and devlog(?).
	 */
	if ( (!(boot_part)) || (!(system_part)) || (!(cache_part)) || (!(userdata_part)) || (!(devlog_part)) || (misc_part!=0) ) {
		printk(KERN_WARNING "awesom3: *NOT* modifying partition table!\n");
		printk(KERN_WARNING "awesom3: One or more partitions *NOT* found!\n");
		printk(KERN_INFO "awesom3: configuring awsome failed!\n");
		configured = -1;
		return 1;
	}

	/* Lets assume all Pico's have a standard 4 mB boot partition.
	 * This will help us determine how much 1 mB takes up.
	 */
	unsigned one_mb = 0;
	for (one_mb = 0, n = 0; n < msm_nand_data.parts[boot_part].size; one_mb++, n+=4) ;

	// Ctrl + x, Ctrl + v
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[devlog_part].size - (DEVLOG_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[devlog_part].size = DEVLOG_SIZE_LEAVE * one_mb;
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[cache_part].size- (CACHE_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[cache_part].size = CACHE_SIZE_LEAVE * one_mb;
	msm_nand_data.parts[system_part].size += msm_nand_data.parts[userdata_part].size- (USERDATA_SIZE_LEAVE * one_mb);
	msm_nand_data.parts[userdata_part].size = USERDATA_SIZE_LEAVE * one_mb;

	/* fix offsets.
	 * rewrite the table the way we want it to be.
	 * table: 0->misc::1->recovery::2->boot::3->system::4->cache::5->userdata::6->devlog
	 * actual layout: recovery::boot::system::cache::devlog::userdata::misc
	 * new layout   : recovery::boot::misc::devlog::userdata::cache::system
	 */
	msm_nand_data.parts[misc_part].offset = msm_nand_data.parts[boot_part].offset + msm_nand_data.parts[boot_part].size;
	msm_nand_data.parts[devlog_part].offset = msm_nand_data.parts[misc_part].offset + msm_nand_data.parts[misc_part].size;
	msm_nand_data.parts[userdata_part].offset = msm_nand_data.parts[devlog_part].offset + msm_nand_data.parts[devlog_part].size;
	msm_nand_data.parts[cache_part].offset = msm_nand_data.parts[userdata_part].offset + msm_nand_data.parts[userdata_part].size;
	msm_nand_data.parts[system_part].offset = msm_nand_data.parts[cache_part].offset + msm_nand_data.parts[cache_part].size;

	//let's make logs 'friendly' to read. hex is hard to 'read'.
	for (n = 0; n < count; n++) {
		printk(KERN_INFO "Partition (by  awesom3) %s "
				"-- Offset:%lli Size:%lli\n",
				msm_nand_data.parts[n].name, msm_nand_data.parts[n].offset, msm_nand_data.parts[n].size);
	}

	printk(KERN_INFO "awesom3: configuring awesom3 succeded!\n");
	configured = 1;
	return 0;
}

static int __init init_awesome(void)
{
	/* Already configured? */
	if (configured == 1)
		return 0;

	if (configured == -1)
		return 0;

	return configure_awesome();
}

arch_initcall(init_awesome);
