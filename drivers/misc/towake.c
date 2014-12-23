/*
 * drivers/misc/towake.c
 *
 * Copyright (C) 2014, Vineeth Raj <contact.twn@opmbx.org>
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
 *
 * Credits:
 * * TeamHackLG:
 *     for initial Sweep2Wake on Himax 8526A
 * * TeamCody
 *     for initial DoubleTap2Wake on Himax 8526A
 */

#include <linux/towake.h>

#ifdef CONFIG_HIMAX_WAKE_MOD_SWEEP2WAKE
unsigned sweep2wake_switch = 1;
#else
unsigned sweep2wake_switch = 0;
#endif //HIMAX_WAKE_MOD_SWEEP2WAKE

#define PICO_ABS_X_MAX 1024

static unsigned sweep2sleep_only_switch = 0;
static unsigned sweep2wake_xres_min_width = 650;
static unsigned sweep2wake_report = 1;
static unsigned sweep2wake_touched = 0;
static unsigned is_sweep2wake_touched;
static unsigned sweep2wake_init_x;
static unsigned sweep2wake_init_y;

#ifdef CONFIG_HIMAX_WAKE_MOD_DOUBLETAP2WAKE
unsigned doubletap2wake_switch = 1;
#else
unsigned doubletap2wake_switch = 0;
#endif //HIMAX_WAKE_MOD_DOUBLETAP2WAKE
#define DT2W_TIMEOUT_MAX 1000 // 1 second

static unsigned doubletap2wake_max_timeout = 400;
static unsigned doubletap2wake_delta = 50;
static s64 doubletap2wake_time[2] = {0, 0};
static unsigned int doubletap2wake_x = 0;
static unsigned int doubletap2wake_y = 0;

unsigned knock_code_switch = 1;
static unsigned knock_code_max_timeout = 400;
static unsigned knock_code_delta = 50;
static s64 knock_code_time[2] = {0, 0};
static unsigned int knock_code_x = 0;
static unsigned int knock_code_y = 0;
int knock_code_pattern[4] = {1,2,3,4};
int knock_code_input[4]   = {0,0,0,0};
int knock_code_x_arr[4]   = {0,0,0,0};
int knock_code_y_arr[4]   = {0,0,0,0};
int knock_code_touch_count = 0;
int knock_code_mid_x = 0;
int knock_code_mid_y = 0;
int knock_code_buffer_x = 0;
int knock_code_buffer_y = 0;
int knock_code_tmp_var = 0;

unsigned is_screen_on;

static struct input_dev * towake_pwrdev;

#ifdef CONFIG_HIMAX_WAKE_MOD_POCKETMOD
unsigned pocket_mod_switch = 1;
#else
unsigned pocket_mod_switch = 0;
#endif

int device_is_pocketed() {

	if (!(pocket_mod_switch))
		return 0;

	if (!(is_screen_on)) {
		if (pocket_mod_switch)
			return pocket_detection_check();
	}

	printk(KERN_INFO "%s: screen is on\n", __func__);
	return 0;
}


static DEFINE_MUTEX(keypress_work);

static void presspwr() {

	if (device_is_pocketed())
		return;

	if (!mutex_trylock(&keypress_work))
		return;
	input_event(towake_pwrdev, EV_KEY, KEY_POWER, 1);
	input_event(towake_pwrdev, EV_SYN, 0, 0);
	msleep(20);
	input_event(towake_pwrdev, EV_KEY, KEY_POWER, 0);
	input_event(towake_pwrdev, EV_SYN, 0, 0);
	msleep(20);
	mutex_unlock(&keypress_work);
	sweep2wake_set_touch(0);
	return;
}

unsigned get_keep_awake(void) {

	if (((sweep2wake_switch) && (!(sweep2sleep_only_switch))) || (doubletap2wake_switch))
		return 1;
	return 0;

}

// Sweep2Wake SysFS (start)
/* ------------------------------ */
static ssize_t sweep2wake_switch_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", sweep2wake_switch);
}

static ssize_t sweep2wake_switch_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ( ( val == 0 ) || ( val == 1 ) )
		sweep2wake_switch = val;

	return size;
}

static DEVICE_ATTR(enable,  0777, sweep2wake_switch_get, sweep2wake_switch_set);
/* ------------------------------ */
static ssize_t s2s_only_switch_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", sweep2sleep_only_switch);
}

static ssize_t s2s_only_switch_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	if (sweep2wake_switch == 0) {
		sweep2sleep_only_switch = 0;
		return size;
	}

	sscanf(buf, "%u\n", &val);

	if ( ( val == 0 ) || ( val == 1 ) )
		sweep2sleep_only_switch = val;

	return size;
}

static DEVICE_ATTR(s2s_only,  0777, s2s_only_switch_get, s2s_only_switch_set);

/* ------------------------------ */
static ssize_t sweep2wake_xres_min_width_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", sweep2wake_xres_min_width);
}

static ssize_t sweep2wake_xres_min_width_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ((val > 0) && ((PICO_ABS_X_MAX - val) > 100))
		sweep2wake_xres_min_width = val;

	return size;
}

static DEVICE_ATTR(xres_min_width, 0777, sweep2wake_xres_min_width_get, sweep2wake_xres_min_width_set);
/* ------------------------------ */
static ssize_t sweep2wake_report_events_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", sweep2wake_report);
}

static ssize_t sweep2wake_report_events_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ((val == 0) || (val == 1))
		sweep2wake_report = val;

	return size;
}

static DEVICE_ATTR(report_events, 0777, sweep2wake_report_events_get, sweep2wake_report_events_set);
/* ------------------------------ */
static struct attribute *sweep2wake_attributes[] =
{
	&dev_attr_enable.attr,
	&dev_attr_s2s_only.attr,
	&dev_attr_xres_min_width.attr,
	&dev_attr_report_events.attr,
	NULL
};

static struct attribute_group sweep2wake_group =
{
	.attrs  = sweep2wake_attributes,
};
/* ------------------------------ */
static int sweep2wake_init_sysfs(void) {

	int rc = 0;
	struct kobject *sweep2wake_kobj;
	sweep2wake_kobj = kobject_create_and_add("sweep2wake", android_touch_kobj);

	rc = sysfs_create_group(sweep2wake_kobj,
			&sweep2wake_group);

	if (unlikely(rc < 0))
		pr_err("sweep2wake: sysfs_create_group failed: %d\n", rc);

	return rc;
}
// Sweep2Wake SysFS (end)

// DoubleTap2Wake SysFS
static ssize_t doubletap2wake_switch_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", doubletap2wake_switch);
}

static ssize_t doubletap2wake_switch_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ( ( val == 0 ) || ( val == 1 ) )
		doubletap2wake_switch = val;

	return size;
}

static DEVICE_ATTR(enable_dt2w,  0777, doubletap2wake_switch_get, doubletap2wake_switch_set);
/* ------------------------------ */

static ssize_t doubletap2wake_maximum_timeout_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%u\n", doubletap2wake_max_timeout);
}

static ssize_t doubletap2wake_maximum_timeout_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if (!(val > DT2W_TIMEOUT_MAX))
		doubletap2wake_max_timeout = val;

	return size;
}

static DEVICE_ATTR(timeout_max_dt2w, 0777, doubletap2wake_maximum_timeout_get, doubletap2wake_maximum_timeout_set);
/* ------------------------------ */

static struct attribute *doubletap2wake_attributes[] =
{
	&dev_attr_enable_dt2w.attr,
	&dev_attr_timeout_max_dt2w.attr,
	NULL
};

static struct attribute_group doubletap2wake_group =
{
	.attrs  = doubletap2wake_attributes,
};

static int doubletap2wake_init_sysfs(void) {

	int rc = 0;

	struct kobject *doubletap2wake_kobj;
	doubletap2wake_kobj = kobject_create_and_add("doubletap2wake", android_touch_kobj);


	dev_attr_enable_dt2w.attr.name = "enable";
	dev_attr_timeout_max_dt2w.attr.name = "timeout_max";

	rc = sysfs_create_group(doubletap2wake_kobj,
			&doubletap2wake_group);

	if (unlikely(rc < 0))
		pr_err("doubletap2wake: sysfs_create_group failed: %d\n", rc);

	return rc;
}
// DoubleTap2Wake SysFS (end)

// Knock Code SysFS
static ssize_t knock_code_switch_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", knock_code_switch);
}

static ssize_t knock_code_switch_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	unsigned int val = 0;

	sscanf(buf, "%u\n", &val);

	if ( ( val == 0 ) || ( val == 1 ) )
		knock_code_switch = val;

	return size;
}

static DEVICE_ATTR(enable_knock_code,  0777, knock_code_switch_get, knock_code_switch_set);
/* ------------------------------ */
static ssize_t knock_code_pattern_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", knock_code_pattern[0],knock_code_pattern[1],knock_code_pattern[2],knock_code_pattern[3]);
}

static ssize_t knock_code_pattern_set(struct device * dev,
		struct device_attribute * attr, const char * buf, size_t size)
{
	if (!( (buf[0] - '0') == (buf[1] - '0') )) {
		knock_code_pattern[0] = buf[0] - '0';
		knock_code_pattern[1] = buf[1] - '0';
		knock_code_pattern[2] = buf[2] - '0';
		knock_code_pattern[3] = buf[3] - '0';
	}

	return size;
}

static DEVICE_ATTR(knock_code_pattern,  0777, knock_code_pattern_get, knock_code_pattern_set);

static struct attribute *knock_code_attributes[] =
{
	&dev_attr_enable_knock_code.attr,
	&dev_attr_knock_code_pattern.attr,
	NULL
};

static struct attribute_group knock_code_group =
{
	.attrs  = knock_code_attributes,
};

static int knock_code_init_sysfs(void) {
	int rc = 0;

	struct kobject *knock_code_kobj;
	knock_code_kobj = kobject_create_and_add("knock_code", android_touch_kobj);

	dev_attr_enable_knock_code.attr.name = "enable";
	dev_attr_knock_code_pattern.attr.name = "pattern";

	rc = sysfs_create_group(knock_code_kobj,
			&knock_code_group);

	if (unlikely(rc < 0))
		pr_err("knock_code: sysfs_create_group failed: %d\n", rc);

	return rc;
}
// Knock Code SysFS (end)

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


static int pwrdev_init(void) {
	int ret = 0;

	towake_pwrdev = input_allocate_device();
	if (!towake_pwrdev)
		pr_err("Can't allocate suspend autotest power button\n");

	input_set_capability(towake_pwrdev, EV_KEY, KEY_POWER);
	towake_pwrdev->name = "sweep2wake_pwrkey";
	towake_pwrdev->phys = "sweep2wake_pwrkey/input0";

	ret = input_register_device(towake_pwrdev);

	if (unlikely(ret < 0))
		pr_err("Can't register device %s\n", towake_pwrdev->name);

	return ret;
}

void sweep2wake_set_touch(unsigned i) {
	if (i)
		is_sweep2wake_touched = 1;
	else
		is_sweep2wake_touched = 0;
	return;
}

unsigned sweep2wake_get_touch_status() {
	return is_sweep2wake_touched;
}

unsigned sweep2sleep_parse() {
	if (is_screen_on)
		return 1;

	if (sweep2sleep_only_switch)
		return 0;

	return 1;
}

void sweep2wake_func(int *x/*, int *y*/) {
	if (!sweep2wake_get_touch_status()) {
		sweep2wake_set_touch(1);
		sweep2wake_init_x = *x;
		/*sweep2wake_init_y = *y;*/
		return;
	}

	if (*x < sweep2wake_init_x) {
		if ((sweep2wake_init_x - *x) > sweep2wake_xres_min_width) {
			if (sweep2sleep_parse())
				presspwr();
		}
	} else {
		if ((*x - sweep2wake_init_x) > sweep2wake_xres_min_width) {
			if (sweep2sleep_parse())
				presspwr();
		}
	}
}

int doubletap2wake_check_n_reset(void) {

	if (doubletap2wake_time[0] == 0) {
		doubletap2wake_time[0] = ktime_to_ms(ktime_get());
		return 0;
	}

	doubletap2wake_time[1] = ktime_to_ms(ktime_get());

	if ((doubletap2wake_time[1]-doubletap2wake_time[0])>doubletap2wake_max_timeout) {
		doubletap2wake_time[0] = ktime_to_ms(ktime_get());
		doubletap2wake_time[1] = 0;
	}

	return 0;
}

void doubletap2wake_func(int *x, int *y) {

	if ( (doubletap2wake_time[0]) && (!(doubletap2wake_time[1])) ) {
		doubletap2wake_x = *x;
		doubletap2wake_y = *y;
		return;
	}

	if ((abs((*x-doubletap2wake_x)) < doubletap2wake_delta)
		&& (abs((*y-doubletap2wake_y)) < doubletap2wake_delta)
		) {
			presspwr();
	}
	doubletap2wake_time[0] = 0;
	doubletap2wake_time[1] = 0;
	return;

}

void knock_code_reset_vars(void) {
	knock_code_touch_count = 0;
	knock_code_input[0] = 0;
	knock_code_input[1] = 0;
	knock_code_input[2] = 0;
	knock_code_input[3] = 0;

	knock_code_x_arr[0] = 0;
	knock_code_x_arr[1] = 0;
	knock_code_x_arr[2] = 0;
	knock_code_x_arr[3] = 0;

	knock_code_y_arr[0] = 0;
	knock_code_y_arr[1] = 0;
	knock_code_y_arr[2] = 0;
	knock_code_y_arr[3] = 0;

	knock_code_mid_x = knock_code_mid_y = 0;

	knock_code_buffer_x = knock_code_buffer_y = 0;

	knock_code_time[0] = knock_code_time[1] = 0;

	knock_code_tmp_var = 0;
}

int knock_code_check_n_reset(void) {

	printk(KERN_INFO "%s: knock_code_time[0] = %lld\n", __func__, knock_code_time[0]);
	printk(KERN_INFO "%s: knock_code_time[1] = %lld\n", __func__, knock_code_time[1]);

	if (knock_code_time[0] == 0) {

		knock_code_time[0] = ktime_to_ms(ktime_get());
		knock_code_time[1] = 0;
		printk(KERN_INFO "%s: knock_code begin\n", __func__);

		knock_code_touch_count = 0;

		knock_code_input[0] = 0;
		knock_code_input[1] = 0;
		knock_code_input[2] = 0;
		knock_code_input[3] = 0;

		knock_code_x_arr[0] = 0;
		knock_code_x_arr[1] = 0;
		knock_code_x_arr[2] = 0;
		knock_code_x_arr[3] = 0;

		knock_code_y_arr[0] = 0;
		knock_code_y_arr[1] = 0;
		knock_code_y_arr[2] = 0;
		knock_code_y_arr[3] = 0;

		knock_code_mid_x = knock_code_mid_y = 0;

		knock_code_buffer_x = knock_code_buffer_y = 0;

		knock_code_tmp_var = 0;

		return 0;

	} else if ((knock_code_time[0]) && (!knock_code_time[1])) {

		knock_code_time[1] = ktime_to_ms(ktime_get());

		printk(KERN_INFO "%s: knock_code_time[1] - knock_code_time[0] = %lld\n", __func__, (knock_code_time[1]-knock_code_time[0]));

		if ((knock_code_time[1]-knock_code_time[0])>knock_code_max_timeout) {

			printk(KERN_INFO "%s: knock_code reset\n", __func__);
			knock_code_time[0] = knock_code_time[1];
			knock_code_time[1] = 0;

			knock_code_touch_count = 0;
			knock_code_input[0] = 0;
			knock_code_input[1] = 0;
			knock_code_input[2] = 0;
			knock_code_input[3] = 0;

			knock_code_x_arr[0] = 0;
			knock_code_x_arr[1] = 0;
			knock_code_x_arr[2] = 0;
			knock_code_x_arr[3] = 0;

			knock_code_y_arr[0] = 0;
			knock_code_y_arr[1] = 0;
			knock_code_y_arr[2] = 0;
			knock_code_y_arr[3] = 0;

			knock_code_mid_x = knock_code_mid_y = 0;

			knock_code_buffer_x = knock_code_buffer_y = 0;

			knock_code_tmp_var = 0;

			return 0;

		}
	}

	return 0;

}

void knock_code_func(int *x, int *y) {

	knock_code_x_arr[knock_code_touch_count] = *x;
	knock_code_y_arr[knock_code_touch_count] = *y;

	printk(KERN_INFO "%s: ###################################################\n", __func__);
	printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
	printk(KERN_INFO "%s: knock_code_touch_count = %d\n", __func__, knock_code_touch_count);
	printk(KERN_INFO "%s: knock_code_x_arr[0] = %d\n", __func__, knock_code_x_arr[0]);
	printk(KERN_INFO "%s: knock_code_x_arr[1] = %d\n", __func__, knock_code_x_arr[1]);
	printk(KERN_INFO "%s: knock_code_x_arr[2] = %d\n", __func__, knock_code_x_arr[2]);
	printk(KERN_INFO "%s: knock_code_x_arr[3] = %d\n", __func__, knock_code_x_arr[3]);

	printk(KERN_INFO "%s: knock_code_y_arr[0] = %d\n", __func__, knock_code_y_arr[0]);
	printk(KERN_INFO "%s: knock_code_y_arr[1] = %d\n", __func__, knock_code_y_arr[1]);
	printk(KERN_INFO "%s: knock_code_y_arr[2] = %d\n", __func__, knock_code_y_arr[2]);
	printk(KERN_INFO "%s: knock_code_y_arr[3] = %d\n", __func__, knock_code_y_arr[3]);
	printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);

	int a = 0;

	if (knock_code_touch_count == 0) {
		knock_code_time[1] = 0;
		knock_code_touch_count += 1;
		printk(KERN_INFO "%s: kctc = 0, x = %d, y = %d\n", __func__, *x, *y);
		printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		return;
	}

	if (knock_code_touch_count == 1) {

		if (!(
			((abs((knock_code_x_arr[0])-(knock_code_x_arr[1]))) > knock_code_delta) ||
			((abs((knock_code_y_arr[0])-(knock_code_y_arr[1]))) > knock_code_delta)
			)) {
			printk(KERN_INFO "%s: it is double tap, return\n", __func__);
			printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
			knock_code_reset_vars();
			return;
		}

		a = ((abs(knock_code_x_arr[0] - knock_code_x_arr[1])) / 2);
		printk(KERN_INFO "%s: delta x / 2 = %d\n", __func__, a);

		if (knock_code_x_arr[0] < knock_code_x_arr[1]) {
			knock_code_mid_x = knock_code_x_arr[0] + a;
			knock_code_buffer_x = knock_code_x_arr[0] - a;
		} else {
			knock_code_mid_x = knock_code_x_arr[0] - a;
			knock_code_buffer_x = knock_code_x_arr[0] + a;
		}

		a = ((abs(knock_code_y_arr[0] - knock_code_y_arr[1])) / 2);
		printk(KERN_INFO "%s: delta y / 2 = %d\n", __func__, a);

		if (knock_code_y_arr[0] < knock_code_y_arr[1]) {
			knock_code_mid_y = knock_code_y_arr[0] + a;
			knock_code_buffer_y = knock_code_y_arr[0] - a;
		} else {
			knock_code_mid_y = knock_code_y_arr[0] - a;
			knock_code_buffer_y = knock_code_y_arr[0] + a;
		}

		printk(KERN_INFO "%s: knock_code_mid_x = %d\n", __func__, knock_code_mid_x);
		printk(KERN_INFO "%s: knock_code_mid_y = %d\n", __func__, knock_code_mid_y);

		printk(KERN_INFO "%s: knock_code_buffer_x = %d\n", __func__, knock_code_buffer_x);
		printk(KERN_INFO "%s: knock_code_buffer_y = %d\n", __func__, knock_code_buffer_y);

		knock_code_x = *x;
		knock_code_y = *y;
		printk(KERN_INFO "%s: kctc = 1, x = %d, y = %d\n", __func__, *x, *y);
		printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		knock_code_time[0] = knock_code_time[1];
		knock_code_time[1] = 0;
		knock_code_touch_count += 1;
		return;
	}

	if (knock_code_touch_count == 2) { //third touch. lot to do here.

		if (knock_code_y_arr[2] < knock_code_buffer_y) { // third touch above, i.e. first two touches around 4, 3
			printk(KERN_INFO "%s: y < knock_code_buffer_y = %d < %d\n", __func__, *y, knock_code_buffer_y);
			knock_code_mid_y = knock_code_buffer_y;

			// i have no idea what this does now.
			//if ((abs(knock_code_y_arr[2] - knock_code_y_arr[1])) > (a)) {
			//	printk(KERN_INFO "%s: ky3 - ky2 > a + delta %d < %d\n", __func__, (abs(knock_code_y_arr[1] - knock_code_y_arr[2])), (a + knock_code_delta));
			//	knock_code_mid_y = ((knock_code_y_arr[1] + knock_code_y_arr[2]) / 2);
			//}

			knock_code_tmp_var = 1;
		} else { //kcy2 > buffer, i.e. below.
			int avg = 0;
			avg = ((abs(knock_code_y_arr[0] + knock_code_y_arr[1])) / 2); //idk why we are averaging, but fux it.

			if ((knock_code_y_arr[2] - avg) > knock_code_delta) {
				if (knock_code_y_arr[2] > (avg)) { // greater than delta
											// why exactly > delta? idk. feels like it.
					//midify these two
					knock_code_mid_y = (knock_code_y_arr[2] + avg) / 2;
					knock_code_tmp_var = 1;
				}
			}

		}

		if (knock_code_x_arr[2] < knock_code_buffer_x) { // third touch left, i.e. first two touches around 2, 3
			printk(KERN_INFO "%s: x < knock_code_buffer_x = %d < %d\n", __func__, *x, knock_code_buffer_x);

			if ((knock_code_x_arr[2] < knock_code_mid_x) &&
				(((((knock_code_x_arr[0] + knock_code_x_arr[1]) / 2) + knock_code_x_arr[2]) / 2) > knock_code_mid_x)
				) {
				knock_code_mid_x = knock_code_buffer_x;
			}

			// i have no idea what this is supposed to do.
			//if ((abs(knock_code_x_arr[2] - knock_code_x_arr[1])) > (a)) {
			//	printk(KERN_INFO "%s: kx3 - kx2 > a + delta %d < %d\n", __func__, (abs(knock_code_x_arr[1] - knock_code_x_arr[2])), (a + knock_code_delta));
			//	knock_code_mid_x = ((knock_code_x_arr[1] + knock_code_x_arr[2]) / 2);
			//}

			knock_code_tmp_var = 1;
		} else { //kcx2 > buffer, i.e. to right
			int avg = 0;
			avg = ((abs(knock_code_x_arr[0] + knock_code_x_arr[1])) / 2); //idk why we are averaging, but fux it.

			if ((knock_code_x_arr[2] - avg) > knock_code_delta) {
				if (knock_code_x_arr[2] > (avg)) { // greater than delta
											// why exactly > delta? idk. feels like it.
					//midify these two
					knock_code_mid_x = (knock_code_x_arr[2] + avg) / 2;
					knock_code_tmp_var = 1;
				}
			}
		}

		knock_code_x = *x;
		knock_code_y = *y;
		printk(KERN_INFO "%s: kctc = 2, x = %d, y = %d\n", __func__, *x, *y);
		printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		knock_code_time[0] = knock_code_time[1];
		knock_code_time[1] = 0;
		knock_code_touch_count += 1;
		return;

	}

	if (knock_code_touch_count == 3) {

		if (knock_code_y_arr[3] < knock_code_buffer_y) {
			if (knock_code_tmp_var == 0) {
				printk(KERN_INFO "%s: y < knock_code_buffer_y = %d < %d\n", __func__, *y, knock_code_buffer_y);
				knock_code_mid_y = knock_code_buffer_y;
			}

			if ((abs(knock_code_y_arr[3] - knock_code_y_arr[2])) > (a + knock_code_delta)) {
				printk(KERN_INFO "%s: ky4 - ky3 > a + delta %d < %d\n", __func__, (abs(knock_code_y_arr[1] - knock_code_y_arr[2])), (a + knock_code_delta));
				knock_code_mid_y = ((knock_code_y_arr[1] + knock_code_y_arr[2]) / 2);
			}


		}

		if (0) {
			if (knock_code_tmp_var == 0) {
				printk(KERN_INFO "%s: x < knock_code_buffer_x = %d < %d\n", __func__, *x, knock_code_buffer_x);
				knock_code_mid_x = knock_code_buffer_x;
			}

			if ((abs(knock_code_x_arr[3] - knock_code_x_arr[2])) > (a + knock_code_delta)) {
				printk(KERN_INFO "%s: kx4 - kx3 > a + delta %d < %d\n", __func__, (abs(knock_code_x_arr[1] - knock_code_x_arr[2])), (a + knock_code_delta));
				knock_code_mid_x = ((knock_code_x_arr[1] + knock_code_x_arr[2]) / 2);
			}

		}

		knock_code_x = *x;
		knock_code_y = *y;
		printk(KERN_INFO "%s: kctc = 3, x = %d, y = %d\n", __func__, *x, *y);
		printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		knock_code_touch_count += 1;

		knock_code_time[0] = knock_code_time[1] = 0;
		knock_code_touch_count = 0;

	}

	printk(KERN_INFO "%s: kcmidx = %d\n", __func__, knock_code_mid_x);
	printk(KERN_INFO "%s: kcmidy = %d\n", __func__, knock_code_mid_y);
	printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
	//printk(KERN_INFO "%s: kcin[0] = %d\n", __func__, knock_code_input[0]);
	//printk(KERN_INFO "%s: kcin[1] = %d\n", __func__, knock_code_input[1]);
	//printk(KERN_INFO "%s: kcin[2] = %d\n", __func__, knock_code_input[2]);
	//printk(KERN_INFO "%s: kcin[3] = %d\n", __func__, knock_code_input[3]);
	//printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);

	if (
	(knock_code_mid_x > knock_code_x_arr[0])
	&&
	(knock_code_mid_x > knock_code_x_arr[1])
	&&
	(knock_code_mid_x > knock_code_x_arr[2])
	&&
	(knock_code_mid_x > knock_code_x_arr[3])
	) {
		//smethg wrong here.
		/* let's try averaging everything! */
	}


	int i = 0;

	for (i = 0; i < 4; i++) {
		if ((knock_code_x_arr[i] < knock_code_mid_x) &&
			(knock_code_y_arr[i] < knock_code_mid_y)) {
			knock_code_input[i] = 1;
			printk(KERN_INFO "%s: fixing kcin[%d] = %d\n", __func__, i, knock_code_input[i]);
			printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		} else if ((knock_code_x_arr[i] > knock_code_mid_x) &&
			(knock_code_y_arr[i] < knock_code_mid_y)) {
			knock_code_input[i] = 2;
			printk(KERN_INFO "%s: fixing kcin[%d] = %d\n", __func__, i, knock_code_input[i]);
			printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		} else if ((knock_code_x_arr[i] > knock_code_mid_x) &&
			(knock_code_y_arr[i] > knock_code_mid_y)) {
			knock_code_input[i] = 3;
			printk(KERN_INFO "%s: fixing kcin[%d] = %d\n", __func__, i, knock_code_input[i]);
			printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		} else if ((knock_code_x_arr[i] < knock_code_mid_x) &&
			(knock_code_y_arr[i] > knock_code_mid_y)) {
			knock_code_input[i] = 4;
			printk(KERN_INFO "%s: fixing kcin[%d] = %d\n", __func__, i, knock_code_input[i]);
			printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
		}
	}

	printk(KERN_INFO "%s: kcin[0] = %d\n", __func__, knock_code_input[0]);
	printk(KERN_INFO "%s: kcin[1] = %d\n", __func__, knock_code_input[1]);
	printk(KERN_INFO "%s: kcin[2] = %d\n", __func__, knock_code_input[2]);
	printk(KERN_INFO "%s: kcin[3] = %d\n", __func__, knock_code_input[3]);
	printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);

	if (
		(knock_code_pattern[0] == knock_code_input[0]) &&
		(knock_code_pattern[1] == knock_code_input[1]) &&
		(knock_code_pattern[2] == knock_code_input[2]) &&
		(knock_code_pattern[3] == knock_code_input[3])
		) {
		printk(KERN_INFO "%s: pattern matches!\n", __func__);
		//presspwr();
		printk(KERN_INFO "%s: ---------------------------------------------------\n", __func__);
	}

	knock_code_reset_vars();
	printk(KERN_INFO "%s: ###################################################\n", __func__);
	return;

}

static int __init towake_init(void)
{
	int ret = 0;

	ret = sweep2wake_init_sysfs();
	if (unlikely(ret < 0))
		pr_err("sweep2wake_init_sysfs failed!\n");

	ret = doubletap2wake_init_sysfs();
	if (unlikely(ret < 0))
		pr_err("doubletap2wake_init_sysfs failed!\n");

	ret = knock_code_init_sysfs();
	if (unlikely(ret < 0))
		pr_err("knock_code_init_sysfs failed!\n");

	pocket_mod_init_sysfs();
	if (unlikely(ret < 0))
		pr_err("pocket_mod_init_sysfs failed!\n");

	pwrdev_init();
	if (unlikely(ret < 0))
		pr_err("pwrdev_init failed!\n");

	return ret;
}

late_initcall(towake_init);

