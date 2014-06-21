#ifndef _LINUX_2WAKE_H
#define _LINUX_2WAKE_H

extern bool is_screen_on;
#ifdef CONFIG_INPUT_CAPELLA_CM3628_POCKETMOD
int pocket_detection_check(void);
int get_pocket_mod_switch_val(void);

static int device_is_pocketed() {
	if (!(is_screen_on))
		if (get_pocket_mod_switch_val())
			return pocket_detection_check();

	printk(KERN_INFO "%s: screen is on", __func__);
	return 0;
}
#endif


#endif	/* _LINUX_2WAKE_H */
