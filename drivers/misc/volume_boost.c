/*
 * Copyright 2014 Vineeth Raj <contact.twn@opmbx.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/volume_boost.h>

static unsigned int boost_val = 0;

unsigned short get_vol_boost_val() {
	return (unsigned short)(boost_val * 1024);
}

static ssize_t volume_boost_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", boost_val);
}

static ssize_t volume_boost_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ( ( val < 0 ) || ( val > 8 ) )
		boost_val = 0;
	else
		boost_val = val;

	return size;
}

static DEVICE_ATTR(volume_boost,  S_IRUGO | S_IWUGO,
					volume_boost_get, volume_boost_set);

static int __init soundcontrol_init(void)
{
	int ret = 0;

	struct kobject *sound_control_kobj;
	sound_control_kobj = kobject_create_and_add("sound_control", NULL);

	if (unlikely(!sound_control_kobj))
		return -ENOMEM;

	ret = sysfs_create_file(sound_control_kobj,
			&dev_attr_volume_boost.attr);

	if (unlikely(ret < 0)) {
		pr_err("volume_boost: sysfs_create_file failed: %d\n", ret);
		return ret;
	}

	return 0;
}

late_initcall(soundcontrol_init);
