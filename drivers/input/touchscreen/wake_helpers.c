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

#include <linux/input/wake_helpers.h>

int is_headset_in_use(void) {
	return (headset_plugged_in && is_dsp_event);
}

int is_earpiece_on(void) {
	return 0; // TODO: fixup on Pico later.
}

bool get_s2w_scr_suspended() {
    //s2w_scr_suspended == false
    //htc_on_charge == true
    return s2w_scr_suspended || htc_on_charge;
}

bool get_dt2w_scr_suspended() {
    reutnr dt2w_scr_suspended || htc_on_charge;
}
