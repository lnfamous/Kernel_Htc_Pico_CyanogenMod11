#ifndef _LINUX_TOWAKE_H
#define _LINUX_TOWAKE_H

#include <linux/device.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#ifdef CONFIG_INPUT_CAPELLA_CM3628_POCKETMOD
#include <linux/pocket_mod.h>
#endif
#include <linux/workqueue.h>

extern unsigned is_screen_on;

extern struct kobject *android_touch_kobj;

unsigned get_keep_awake(void);

extern unsigned sweep2wake_switch;
void sweep2wake_func(int *x/*, int *y*/);
void sweep2wake_set_touch(unsigned i);

extern unsigned doubletap2wake_switch;
int doubletap2wake_check_n_reset(void);
void doubletap2wake_func(int *x, int *y);

extern unsigned pocket_mod_switch;

int device_is_pocketed();

#endif	/* _LINUX_TOWAKE_H */
