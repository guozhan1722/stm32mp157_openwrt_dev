// SPDX-License-Identifier: GPL-2.0-only
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dk2-touch.h"

int main(int argc, char **argv)
{
	struct dk2_touch touch;
	struct input_event event;
	int require_swipe = argc == 2 && strcmp(argv[1], "--swipe") == 0;
	int fd;

	if (argc != 1 && !require_swipe)
		return EXIT_FAILURE;
	fd = dk2_touch_open(&touch, 0);
	if (fd < 0) {
		fputs("No direct touchscreen input device found\n", stderr);
		return EXIT_FAILURE;
	}

	for (;;) {
		ssize_t size = read(fd, &event, sizeof(event));
		enum dk2_gesture gesture;

		if (size < 0 && errno == EINTR)
			continue;
		if (size != sizeof(event))
			break;
		gesture = dk2_touch_event(&touch, &event);
		if (gesture == DK2_GESTURE_SWIPE ||
		    (!require_swipe && (gesture == DK2_GESTURE_TAP || gesture == DK2_GESTURE_HOLD))) {
			close(fd);
			return EXIT_SUCCESS;
		}
	}
	close(fd);
	return EXIT_FAILURE;
}
