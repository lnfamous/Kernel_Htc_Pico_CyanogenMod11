/* arch/arm/mach-msm/nand_partitions.c
 *
 * Code to extract partition information from ATAG set up by the
 * bootloader.
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2014, Vineeth Raj <contact.twn@opmbx.org>.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

#include <asm/mach/flash.h>
#include <linux/io.h>

#include <asm/setup.h>

#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <mach/msm_iomap.h>

#include <mach/board.h>
#include "smd_private.h"

/* configuration tags specific to msm */

#define ATAG_MSM_PARTITION 0x4d534D70 /* MSMp */

struct msm_ptbl_entry {
	char name[16];
	__u32 offset;
	__u32 size;
	__u32 flags;
};

#define MSM_MAX_PARTITIONS 8

static struct mtd_partition msm_nand_partitions[MSM_MAX_PARTITIONS];
static char msm_nand_names[MSM_MAX_PARTITIONS * 16];

extern struct flash_platform_data msm_nand_data;

static int __init parse_tag_msm_partition(const struct tag *tag)
{
	struct mtd_partition *ptn = msm_nand_partitions;
	char *name = msm_nand_names;
	struct msm_ptbl_entry *entry = (void *) &tag->u;
	unsigned count, n;
#ifdef CONFIG_PICO_NAND_RESIZE_PART
	unsigned boot_part = 0;
	unsigned system_part = 0;
	unsigned cache_part = 0;
	unsigned userdata_part = 0;
	unsigned devlog_part = 0;
	unsigned misc_part = 0;
#endif

	count = (tag->hdr.size - 2) /
		(sizeof(struct msm_ptbl_entry) / sizeof(__u32));

	if (count > MSM_MAX_PARTITIONS)
		count = MSM_MAX_PARTITIONS;

	for (n = 0; n < count; n++) {
		memcpy(name, entry->name, 15);
		name[15] = 0;

		ptn->name = name;
		ptn->offset = entry->offset;
		ptn->size = entry->size;

#ifdef CONFIG_PICO_NAND_RESIZE_PART
		if (!(strcmp(ptn->name, "boot")))
			boot_part = n;
		if (!(strcmp(ptn->name, "system")))
			system_part = n;
		if (!(strcmp(ptn->name, "cache")))
			cache_part = n;
		if (!(strcmp(ptn->name, "userdata")))
			userdata_part = n;
		if (!(strcmp(ptn->name, "devlog")))
			devlog_part = n;
		if (!(strcmp(ptn->name, "misc")))
			misc_part = n;
#endif

		printk(KERN_INFO "Partition (from atag) %s "
				"-- Offset:%llx Size:%llx\n",
				ptn->name, ptn->offset, ptn->size);

		name += 16;
		entry++;
		ptn++;
	}

	msm_nand_data.nr_parts = count;

#ifdef CONFIG_PICO_NAND_RESIZE_PART

#define CACHE_SIZE_LEAVE 8
#define USERDATA_SIZE_LEAVE 4
#define DEVLOG_SIZE_LEAVE 1

	/* First check if we have all the partitions!
	 * Assuming it is a pico, it *should* have boot, system,
	 * cache, userdata, and devlog(?).
	 */
	if ( (!(boot_part)) || (!(system_part)) || (!(cache_part)) || (!(userdata_part)) || (!(devlog_part)) || (misc_part!=0) ) {
		printk(KERN_INFO "aw3som3: *NOT* modifying partition table!\n");
		printk(KERN_INFO "aw3som3: One or more partitions *NOT* found!\n");
		msm_nand_data.parts = msm_nand_partitions;
		return 0;
	}

	/* Lets assume all Pico's have a standard 4 mB boot partition.
	 * This will help us determine how much 1 mB takes up.
	 */
	unsigned one_mb = 0;
	for (one_mb = 0, n = 0; n < msm_nand_partitions[boot_part].size; one_mb++, n+=4) ;

	// Ctrl + x, Ctrl + v
	msm_nand_partitions[system_part].size += msm_nand_partitions[devlog_part].size - (DEVLOG_SIZE_LEAVE * one_mb);
	msm_nand_partitions[devlog_part].size = DEVLOG_SIZE_LEAVE * one_mb;
	msm_nand_partitions[system_part].size += msm_nand_partitions[cache_part].size- (CACHE_SIZE_LEAVE * one_mb);
	msm_nand_partitions[cache_part].size = CACHE_SIZE_LEAVE * one_mb;
	msm_nand_partitions[system_part].size += msm_nand_partitions[userdata_part].size- (USERDATA_SIZE_LEAVE * one_mb);
	msm_nand_partitions[userdata_part].size = USERDATA_SIZE_LEAVE * one_mb;

	/* fix offsets.
	 * rewrite the table the way we want it to be.
	 * table: 0->misc::1->recovery::2->boot::3->system::4->cache::5->userdata::6->devlog
	 * actual layout: recovery::boot::system::cache::devlog::userdata::misc
	 * new layout   : recovery::boot::misc::devlog::userdata::cache::system
	 */
	msm_nand_partitions[misc_part].offset = msm_nand_partitions[boot_part].offset + msm_nand_partitions[boot_part].size;
	msm_nand_partitions[devlog_part].offset = msm_nand_partitions[misc_part].offset + msm_nand_partitions[misc_part].size;
	msm_nand_partitions[userdata_part].offset = msm_nand_partitions[devlog_part].offset + msm_nand_partitions[devlog_part].size;
	msm_nand_partitions[cache_part].offset = msm_nand_partitions[userdata_part].offset + msm_nand_partitions[userdata_part].size;
	msm_nand_partitions[system_part].offset = msm_nand_partitions[cache_part].offset + msm_nand_partitions[cache_part].size;

#endif

	msm_nand_data.parts = msm_nand_partitions;

	return 0;
}

__tagtable(ATAG_MSM_PARTITION, parse_tag_msm_partition);

#define FLASH_PART_MAGIC1     0x55EE73AA
#define FLASH_PART_MAGIC2     0xE35EBDDB
#define FLASH_PARTITION_VERSION   0x3

#define LINUX_FS_PARTITION_NAME  "0:EFS2APPS"

struct flash_partition_entry {
	char name[16];
	u32 offset;	/* Offset in blocks from beginning of device */
	u32 length;	/* Length of the partition in blocks */
	u8 attrib1;
	u8 attrib2;
	u8 attrib3;
	u8 which_flash;	/* Numeric ID (first = 0, second = 1) */
};
struct flash_partition_table {
	u32 magic1;
	u32 magic2;
	u32 version;
	u32 numparts;
	struct flash_partition_entry part_entry[16];
};

static int get_nand_partitions(void)
{
	struct flash_partition_table *partition_table;
	struct flash_partition_entry *part_entry;
	struct mtd_partition *ptn = msm_nand_partitions;
	char *name = msm_nand_names;
	int part;

	if (msm_nand_data.nr_parts)
		return 0;

	partition_table = (struct flash_partition_table *)
	    smem_alloc(SMEM_AARM_PARTITION_TABLE,
		       sizeof(struct flash_partition_table));

	if (!partition_table) {
		printk(KERN_WARNING "%s: no flash partition table in shared "
		       "memory\n", __func__);
		return -ENOENT;
	}

	if ((partition_table->magic1 != (u32) FLASH_PART_MAGIC1) ||
	    (partition_table->magic2 != (u32) FLASH_PART_MAGIC2) ||
	    (partition_table->version != (u32) FLASH_PARTITION_VERSION)) {
		printk(KERN_WARNING "%s: version mismatch -- magic1=%#x, "
		       "magic2=%#x, version=%#x\n", __func__,
		       partition_table->magic1,
		       partition_table->magic2,
		       partition_table->version);
		return -EFAULT;
	}

	msm_nand_data.nr_parts = 0;

	/* Get the LINUX FS partition info */
	for (part = 0; part < partition_table->numparts; part++) {
		part_entry = &partition_table->part_entry[part];

		/* Find a match for the Linux file system partition */
		if (strcmp(part_entry->name, LINUX_FS_PARTITION_NAME) == 0) {
			strcpy(name, part_entry->name);
			ptn->name = name;

			/*TODO: Get block count and size info */
			ptn->offset = part_entry->offset;

			/* For SMEM, -1 indicates remaining space in flash,
			 * but for MTD it is 0
			 */
			if (part_entry->length == (u32)-1)
				ptn->size = 0;
			else
				ptn->size = part_entry->length;

			msm_nand_data.nr_parts = 1;
			msm_nand_data.parts = msm_nand_partitions;

			printk(KERN_INFO "Partition(from smem) %s "
					"-- Offset:%llx Size:%llx\n",
					ptn->name, ptn->offset, ptn->size);

			return 0;
		}
	}

	printk(KERN_WARNING "%s: no partition table found!", __func__);

	return -ENODEV;
}

device_initcall(get_nand_partitions);
