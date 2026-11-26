// SPDX-License-Identifier: GPL-2.0-only
#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Coordinates are in panel pixels. */
#define SWIPE_DISTANCE 48

int main(int argc, char **argv)
{
	glob_t inputs = { 0 };
	struct input_event event;
	size_t i;
	int fd = -1;
	int require_swipe = argc == 2 && strcmp(argv[1], "--swipe") == 0;
	struct input_absinfo axis;
	int x, y, start_x = 0, start_y = 0;
	int touching = 0, tracking = 0, swiped = 0;

	if (argc != 1 && !require_swipe)
		return EXIT_FAILURE;

	if (glob("/dev/input/event*", 0, NULL, &inputs) == 0) {
		for (i = 0; i < inputs.gl_pathc; i++) {
			uint32_t properties = 0;

			fd = open(inputs.gl_pathv[i], O_RDONLY);
			if (fd < 0)
				continue;
			if (ioctl(fd, EVIOCGPROP(sizeof(properties)), &properties) >= 0 &&
			    (properties & (1U << INPUT_PROP_DIRECT)))
				break;
			close(fd);
			fd = -1;
		}
	}
	globfree(&inputs);
	if (fd < 0) {
		fputs("No direct touchscreen input device found\n", stderr);
		return EXIT_FAILURE;
	}

	if (ioctl(fd, EVIOCGABS(ABS_X), &axis) < 0)
		goto failed;
	x = axis.value;
	if (ioctl(fd, EVIOCGABS(ABS_Y), &axis) < 0)
		goto failed;
	y = axis.value;

	for (;;) {
		ssize_t size = read(fd, &event, sizeof(event));

		if (size < 0 && errno == EINTR)
			continue;
		if (size != sizeof(event))
			break;
		if (event.type == EV_KEY && event.code == BTN_TOUCH)
			touching = event.value != 0;
		if (event.type == EV_ABS && event.code == ABS_X)
			x = event.value;
		if (event.type == EV_ABS && event.code == ABS_Y)
			y = event.value;
		if (event.type != EV_SYN || event.code != SYN_REPORT)
			continue;

		if (touching && !tracking) {
			start_x = x;
			start_y = y;
			swiped = 0;
			tracking = 1;
		} else if (tracking) {
			if (abs(x - start_x) >= SWIPE_DISTANCE ||
			    abs(y - start_y) >= SWIPE_DISTANCE)
				swiped = 1;
			if (!touching) {
				tracking = 0;
				if (!require_swipe || swiped) {
					close(fd);
					return EXIT_SUCCESS;
				}
			}
		}
	}
failed:
	close(fd);
	return EXIT_FAILURE;
}
