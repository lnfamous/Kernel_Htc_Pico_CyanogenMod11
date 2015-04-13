/* drivers/misc/glumberg.c
 *
 * Copyright 2014 Vineeth Raj <contact.twn@openmailbox.org>
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

extern struct flash_platform_data msm_nand_data;
static char *glumname = "glumboot";

int configured = 0;

static int configure_glumberg(void) {

	unsigned boot_part = 0;
	unsigned system_part = 0;
	unsigned cache_part = 0;
	unsigned userdata_part = 0;
	unsigned devlog_part = 0;
	unsigned misc_part = 0;
	unsigned glumboot_part = 0;

	int err = 0;

	printk(KERN_INFO "glumberg: going to try configure glumberg\n");

	unsigned count = msm_nand_data.nr_parts;
	glumboot_part = count;
	int n = 0;

	for (n = 0; n < count; n++) {
		//let's make logs 'friendly' to read. hex is hard to 'read'.
		printk(KERN_INFO "Partition (for glumberg) %s "
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
		if (!(strcmp(msm_nand_data.parts[n].name, "misc"))) {
			misc_part = n;
		}
	}

	/* First check if we have all the partitions!
	 * Assuming it is a pico, it *should* have boot, system,
	 * cache, userdata, and devlog(?).
	 */
	if ( (!(boot_part)) || (!(system_part)) || (!(cache_part)) || (!(userdata_part)) || (!(devlog_part)) || (misc_part!=0) ) {
		printk(KERN_WARNING "glumberg: *NOT* modifying partition table!\n");
		printk(KERN_WARNING "glumberg: One or more partitions *NOT* found!\n");
		printk(KERN_INFO "glumberg: configuring glumberg failed!\n");
		configured = -1;
		return 1;
	}

	/* Lets assume all Pico's have a standard 4 mB boot partition.
	 * This will help us determine how much 1 mB takes up.
	 */
	unsigned one_mb = 0;
	for (one_mb = 0, n = 0; n < msm_nand_data.parts[boot_part].size; one_mb++, n+=4) ;

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

	/*
	 * new layout   : recovery::boot::misc::devlog::userdata::cache::system
	 * glum layout  : recovery::boot::glumboot::misc::devlog::userdata::cache::system
	 *
	 */

	// increment msm_nand_data.nr_parts
	msm_nand_data.nr_parts += 1;
	count += 1;

	// set glumboot offset to misc
	msm_nand_data.parts[glumboot_part].offset = msm_nand_data.parts[misc_part].offset;
	msm_nand_data.parts[glumboot_part].size = one_mb;

	// set misc offset to glumboot offset + one_mb
	msm_nand_data.parts[misc_part].offset += one_mb;

	// set devlog offset to misc_part.offset + size
	msm_nand_data.parts[devlog_part].offset = msm_nand_data.parts[misc_part].offset + msm_nand_data.parts[misc_part].size;

	// get one_mb from devlog (for glumboot)
	msm_nand_data.parts[devlog_part].size -= one_mb;

	//strcpy()
	msm_nand_data.parts[glumboot_part].name = glumname;

	//todo: let's kick out devlog :)

	//let's make logs 'friendly' to read. hex is hard to 'read'.
	for (n = 0; n < count; n++) {
		printk(KERN_INFO "Partition (by glumberg) %s "
				"-- Offset:%lli Size:%lli\n",
				msm_nand_data.parts[n].name, msm_nand_data.parts[n].offset, msm_nand_data.parts[n].size);
	}

	printk(KERN_INFO "glumberg: configuring glumberg succeded!\n");
	configured = 1;
	return 0;
}

static int __init init_glumberg(void)
{
	/* Already configured? */
	if (configured == 1)
		return 0;

	if (configured == -1)
		return 0;

	return configure_glumberg();
}

arch_initcall(init_glumberg);
