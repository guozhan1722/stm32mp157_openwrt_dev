// SPDX-License-Identifier: GPL-2.0-only
#include <fcntl.h>
#include <glob.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "dk2-touch.h"

#define PANEL_SWIPE_DISTANCE 48
#define HOLD_TIME_US 1000000

static int usb_swipe_distance(const struct input_absinfo *axis)
{
	/* A swipe is at least 6% of either axis, regardless of HID units. */
	int distance = ((int64_t)axis->maximum - axis->minimum) * 6 / 100;

	return distance > 0 ? distance : 1;
}

int dk2_touch_open(struct dk2_touch *touch, unsigned int bus)
{
	glob_t inputs = { 0 };
	int fd = -1;

	if (glob("/dev/input/event*", 0, NULL, &inputs) == 0) {
		for (size_t i = 0; i < inputs.gl_pathc; i++) {
			struct input_absinfo x, y;
			struct input_id id;
			uint32_t properties = 0;

			fd = open(inputs.gl_pathv[i], O_RDONLY | O_CLOEXEC);
			if (fd < 0)
				continue;
			if (ioctl(fd, EVIOCGPROP(sizeof(properties)), &properties) >= 0 &&
			    (properties & (1U << INPUT_PROP_DIRECT)) &&
			    (!bus || (ioctl(fd, EVIOCGID, &id) >= 0 && id.bustype == bus)) &&
			    ioctl(fd, EVIOCGABS(ABS_X), &x) >= 0 &&
			    ioctl(fd, EVIOCGABS(ABS_Y), &y) >= 0) {
				int clock_id = CLOCK_MONOTONIC;

				/* Use monotonic timestamps when the driver supports them. */
				if (bus == BUS_USB)
					ioctl(fd, EVIOCSCLOCKID, &clock_id);
				*touch = (struct dk2_touch) {
					.x = x.value,
					.y = y.value,
					.swipe_x = bus == BUS_USB ? usb_swipe_distance(&x) :
								    PANEL_SWIPE_DISTANCE,
					.swipe_y = bus == BUS_USB ? usb_swipe_distance(&y) :
								    PANEL_SWIPE_DISTANCE,
				};
				break;
			}
			close(fd);
			fd = -1;
		}
	}
	globfree(&inputs);
	return fd;
}

enum dk2_gesture dk2_touch_event(struct dk2_touch *touch, const struct input_event *event)
{
	int64_t time_us;

	if (event->type == EV_SYN && event->code == SYN_DROPPED) {
		touch->touching = 0;
		touch->tracking = 0;
		touch->dropped = 1;
	}
	if (touch->dropped) {
		if (event->type == EV_SYN && event->code == SYN_REPORT)
			touch->dropped = 0;
		return DK2_GESTURE_NONE;
	}

	if (event->type == EV_KEY && event->code == BTN_TOUCH)
		touch->touching = event->value != 0;
	if (event->type == EV_ABS && event->code == ABS_X)
		touch->x = event->value;
	if (event->type == EV_ABS && event->code == ABS_Y)
		touch->y = event->value;
	if (event->type != EV_SYN || event->code != SYN_REPORT)
		return DK2_GESTURE_NONE;

	time_us = (int64_t)event->input_event_sec * 1000000 + event->input_event_usec;
	if (touch->touching && !touch->tracking) {
		touch->start_x = touch->x;
		touch->start_y = touch->y;
		touch->swiped = 0;
		touch->tracking = 1;
		touch->press_time_us = time_us;
	} else if (touch->tracking) {
		if (llabs((int64_t)touch->x - touch->start_x) >= touch->swipe_x ||
		    llabs((int64_t)touch->y - touch->start_y) >= touch->swipe_y)
			touch->swiped = 1;
		if (!touch->touching) {
			touch->tracking = 0;
			if (touch->swiped)
				return DK2_GESTURE_SWIPE;
			if (time_us - touch->press_time_us >= HOLD_TIME_US)
				return DK2_GESTURE_HOLD;
			return DK2_GESTURE_TAP;
		}
	}
	return DK2_GESTURE_NONE;
}
