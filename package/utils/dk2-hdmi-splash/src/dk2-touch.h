// SPDX-License-Identifier: GPL-2.0-only
#ifndef DK2_TOUCH_H
#define DK2_TOUCH_H

#include <linux/input.h>
#include <stdint.h>

enum dk2_gesture {
	DK2_GESTURE_NONE,
	DK2_GESTURE_TAP,
	DK2_GESTURE_SWIPE,
	DK2_GESTURE_HOLD,
};

struct dk2_touch {
	int x, y;
	int start_x, start_y;
	int swipe_x, swipe_y;
	int touching, tracking, swiped, dropped;
	int64_t press_time_us;
};

/* A zero bus matches any direct touchscreen. */
int dk2_touch_open(struct dk2_touch *touch, unsigned int bus);
enum dk2_gesture dk2_touch_event(struct dk2_touch *touch, const struct input_event *event);

#endif
