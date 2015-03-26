/*
 * drivers/input/touchscreen/wake_helpers.c
 *
 *
 * Copyright (c) 2015, Vineeth Raj <contact.twn@openmailbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>

#include <linux/input/wake_helpers.h>
#include <linux/pocket_mod.h>

#define ANDROID_TOUCH_DECLARED

#ifdef ANDROID_TOUCH_DECLARED
extern struct kobject *android_touch_kobj;
#else
struct kobject *android_touch_kobj;
EXPORT_SYMBOL_GPL(android_touch_kobj);
#endif


int is_headset_in_use(void) {
	return (headset_plugged_in && is_dsp_event);
}

int is_earpiece_on(void) {
	return 0; // TODO: fixup on Pico later.
}

bool get_s2w_scr_suspended() {
    //s2w_scr_suspended == false
    //htc_on_charge == true
    //if screen is !suspended; and htc_on_charge is true; return screen_suspended.
    if (!s2w_scr_suspended) {
        if (htc_on_charge)
            return true;
    }
    return s2w_scr_suspended;
}

bool get_dt2w_scr_suspended() {
    if (!dt2w_scr_suspended) {
        if (htc_on_charge)
            return true;
    }
    return dt2w_scr_suspended;
}

//#ifdef CONFIG_HIMAX_WAKE_MOD_POCKETMOD
unsigned pocket_mod_switch = 1;
//#else
//unsigned pocket_mod_switch = 0;
//#endif

int device_is_pocketed() {

    if (!(pocket_mod_switch))
        return 0;

    if ((get_s2w_scr_suspended() || get_dt2w_scr_suspended())) {
        if (pocket_mod_switch)
            return pocket_detection_check();
    }

    return 0;

}

// PocketMod
static ssize_t himax_pocket_mod_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", pocket_mod_switch);
}

static ssize_t himax_pocket_mod_set(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
    unsigned int val = 0;

    sscanf(buf, "%u\n", &val);

    if ( ( val == 0 ) || ( val == 1 ) )
        pocket_mod_switch = val;

    return size;
}

static DEVICE_ATTR(pocket_mod_enable, 0777,
        himax_pocket_mod_show, himax_pocket_mod_set);

static struct attribute *pocket_mod_attributes[] =
{
    &dev_attr_pocket_mod_enable.attr,
    NULL
};

static struct attribute_group pocket_mod_group =
{
    .attrs  = pocket_mod_attributes,
};

static int pocket_mod_init_sysfs(void) {

    int rc = 0;

    struct kobject *pocket_mod_kobj;
    pocket_mod_kobj = kobject_create_and_add("pocket_mod", android_touch_kobj);

    dev_attr_pocket_mod_enable.attr.name = "enable";

    rc = sysfs_create_group(pocket_mod_kobj,
            &pocket_mod_group);

    if (unlikely(rc < 0))
        pr_err("pocket_mod: sysfs_create_group failed: %d\n", rc);

    return rc;

}
// PocketMod (end)

static int __init wake_helpers_init(void)
{
    int ret = 0;

    pocket_mod_init_sysfs();
    if (unlikely(ret < 0))
        pr_err("pocket_mod_init_sysfs failed!\n");

    return ret;
}

late_initcall(wake_helpers_init);
