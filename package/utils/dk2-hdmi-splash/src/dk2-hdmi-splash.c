// SPDX-License-Identifier: GPL-2.0-only
#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>

#include "dk2-touch.h"

#define PHOTO_WIDTH 480
#define PHOTO_HEIGHT 800
#define HDMI_CONNECTED 1
#define HDMI_DISCONNECTED 2
#define FONT_PITCH 32
#define LOG_BUFFER_SIZE 16384

static uint32_t photo[PHOTO_HEIGHT][PHOTO_WIDTH];

struct splash_display {
	struct drm_mode_create_dumb buffer;
	void *pixels;
	uint32_t connector_id;
	int portrait;
};

struct pixel_layout {
	uint32_t *origin;
	ptrdiff_t x_step, y_step;
	unsigned int width, height;
};

static struct pixel_layout display_layout(const struct splash_display *display)
{
	ptrdiff_t stride = display->buffer.pitch / sizeof(uint32_t);
	uint32_t *pixels = display->pixels;

	if (display->portrait) {
		/* The panel is physically turned clockwise. */
		return (struct pixel_layout) {
			.origin = pixels + (display->buffer.height - 1) * stride,
			.x_step = -stride, .y_step = 1,
			.width = display->buffer.height, .height = display->buffer.width,
		};
	}
	return (struct pixel_layout) {
		.origin = pixels, .x_step = 1, .y_step = stride,
		.width = display->buffer.width, .height = display->buffer.height,
	};
}

static void draw_photo(const struct splash_display *display)
{
	struct pixel_layout layout = display_layout(display);
	unsigned int width = layout.width;
	unsigned int height = width * PHOTO_HEIGHT / PHOTO_WIDTH;
	unsigned int left, top;

	if (height > layout.height) {
		height = layout.height;
		width = height * PHOTO_WIDTH / PHOTO_HEIGHT;
	}
	left = (layout.width - width) / 2;
	top = (layout.height - height) / 2;

	memset(display->pixels, 0, display->buffer.size);
	for (unsigned int y = 0; y < height; y++) {
		uint32_t *row = layout.origin + (ptrdiff_t)(top + y) * layout.y_step +
				(ptrdiff_t)left * layout.x_step;

		for (unsigned int x = 0; x < width; x++)
			row[(ptrdiff_t)x * layout.x_step] = photo[y * PHOTO_HEIGHT / height]
								 [x * PHOTO_WIDTH / width];
	}
}

static int show_mode(int fd, uint32_t crtc_id, uint32_t connector_id,
		     const struct drm_mode_modeinfo *mode,
		     struct splash_display *display)
{
	struct drm_mode_create_dumb buffer = {
		.width = mode->hdisplay, .height = mode->vdisplay, .bpp = 32,
	};
	struct drm_mode_map_dumb mapping = { 0 };
	struct drm_mode_fb_cmd fb = { 0 };
	struct drm_mode_crtc crtc = { 0 };
	struct drm_mode_destroy_dumb destroy = { 0 };
	void *pixels;
	int result = -1;

	if (!buffer.width || !buffer.height ||
	    ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &buffer) < 0)
		return -1;

	mapping.handle = buffer.handle;
	if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mapping) < 0)
		goto destroy_buffer;
	pixels = mmap(NULL, buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED,
		      fd, mapping.offset);
	if (pixels == MAP_FAILED)
		goto destroy_buffer;

	*display = (struct splash_display) {
		.buffer = buffer, .pixels = pixels, .connector_id = connector_id,
	};
	draw_photo(display);

	fb.width = buffer.width;
	fb.height = buffer.height;
	fb.pitch = buffer.pitch;
	fb.bpp = 32;
	fb.depth = 24;
	fb.handle = buffer.handle;
	if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fb) < 0)
		goto unmap_buffer;

	crtc.set_connectors_ptr = (uintptr_t)&connector_id;
	crtc.count_connectors = 1;
	crtc.crtc_id = crtc_id;
	crtc.fb_id = fb.fb_id;
	crtc.mode_valid = 1;
	crtc.mode = *mode;
	result = ioctl(fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
	if (result == 0)
		return 0;

	ioctl(fd, DRM_IOCTL_MODE_RMFB, &fb.fb_id);
unmap_buffer:
	munmap(pixels, buffer.size);
destroy_buffer:
	destroy.handle = buffer.handle;
	ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	return result;
}

static int show_hdmi(int fd, struct splash_display *display)
{
	uint32_t crtc_id, connector_ids[2];
	struct drm_mode_card_res resources = {
		.crtc_id_ptr = (uintptr_t)&crtc_id, .count_crtcs = 1,
		.connector_id_ptr = (uintptr_t)connector_ids, .count_connectors = 2,
	};
	unsigned int i;

	/* The DK2 LTDC exposes one CRTC, one DSI connector and one HDMI connector. */
	if (ioctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &resources) < 0 ||
	    resources.count_crtcs != 1 || resources.count_connectors > 2)
		return -1;

	for (i = 0; i < resources.count_connectors; i++) {
		struct drm_mode_get_connector connector = { .connector_id = connector_ids[i] };
		struct drm_mode_modeinfo *modes;
		unsigned int count, j;
		int result = -1;

		if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) < 0 ||
		    connector.connector_type != DRM_MODE_CONNECTOR_HDMIA ||
		    connector.connection != HDMI_CONNECTED || !connector.count_modes)
			continue;

		count = connector.count_modes;
		modes = calloc(count, sizeof(*modes));
		if (!modes)
			return -1;
		/* Request modes only. The first call returned metadata counts,
		 * but we did not allocate encoder or property buffers.
		 */
		connector.count_encoders = 0;
		connector.count_props = 0;
		connector.modes_ptr = (uintptr_t)modes;
		if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) == 0 &&
		    connector.count_modes <= count) {
			/* The kernel puts preferred modes first. */
			for (j = 0; j < connector.count_modes && result != 0; j++)
				result = show_mode(fd, crtc_id, connector.connector_id,
						   &modes[j], display);
		}
		free(modes);
		return result;
	}
	return -1;
}

static int load_console_font(struct console_font_op *font)
{
	int fd = open("/dev/tty1", O_RDONLY | O_CLOEXEC);
	int result;

	if (fd < 0)
		return -1;
	result = ioctl(fd, KDFONTOP, font);
	close(fd);
	return result;
}

static void draw_text(const struct splash_display *display,
		      const struct console_font_op *font, const char *text)
{
	struct pixel_layout layout = display_layout(display);
	unsigned int glyph_pitch = (font->width + 7) / 8;
	unsigned int left = font->width;
	unsigned int right = layout.width;
	unsigned int bottom = layout.height;
	unsigned int x = left, y = font->height;

	memset(display->pixels, 0, display->buffer.size);
	if (right < left + font->width)
		return;
	for (; *text && y + font->height <= bottom; text++) {
		unsigned char c = *text;
		const unsigned char *glyph;

		if (c == '\n') {
			x = left;
			y += font->height;
			continue;
		}
		if (x + font->width > right) {
			x = left;
			y += font->height;
			if (y + font->height > bottom)
				break;
		}
		if (c < ' ')
			c = ' ';
		if (c > '~')
			c = '?';
		glyph = font->data + c * FONT_PITCH * glyph_pitch;
		for (unsigned int row = 0; row < font->height; row++) {
			uint32_t *pixels = layout.origin + (ptrdiff_t)(y + row) * layout.y_step +
					   (ptrdiff_t)x * layout.x_step;

			for (unsigned int col = 0; col < font->width; col++)
				if (glyph[row * glyph_pitch + col / 8] & (0x80U >> (col % 8)))
					pixels[(ptrdiff_t)col * layout.x_step] = 0x00ffffff;
		}
		x += font->width;
	}
}

static void refresh_logs(const struct splash_display *display,
			 const struct console_font_op *font, char *previous)
{
	char logs[LOG_BUFFER_SIZE];
	FILE *output = popen("printf '%s\\n' 'OpenWrt boot and kernel messages'; "
			     "dmesg | tail -n 8; "
			     "printf '\\n%s\\n' 'OpenWrt system log'; "
			     "logread | tail -n 8", "r");
	size_t size;

	if (!output)
		return;
	size = fread(logs, 1, sizeof(logs) - 1, output);
	pclose(output);
	logs[size] = '\0';
	if (strcmp(previous, logs) != 0) {
		draw_text(display, font, logs);
		memcpy(previous, logs, size + 1);
	}
}

static int update_view(struct splash_display *display, int *logs_visible,
		       enum dk2_gesture gesture)
{
	if (gesture == DK2_GESTURE_HOLD) {
		display->portrait = !display->portrait;
		return 1;
	}
	if ((!*logs_visible && gesture == DK2_GESTURE_TAP) ||
	    (*logs_visible && gesture == DK2_GESTURE_SWIPE)) {
		*logs_visible = !*logs_visible;
		return 1;
	}
	return 0;
}

static int hdmi_disconnected(int fd, uint32_t connector_id, time_t *last_check)
{
	struct drm_mode_modeinfo mode;
	struct drm_mode_get_connector connector = {
		.connector_id = connector_id,
		/* A nonzero mode count reads cached hotplug status without probing EDID. */
		.count_modes = 1, .modes_ptr = (uintptr_t)&mode,
	};
	struct timespec now;

	clock_gettime(CLOCK_MONOTONIC, &now);
	if (now.tv_sec == *last_check)
		return 0;
	*last_check = now.tv_sec;
	return ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) == 0 &&
	       connector.connection == HDMI_DISCONNECTED;
}

static void watch_touch(struct splash_display *display, int drm_fd)
{
	static unsigned char glyphs[512 * FONT_PITCH * 4];
	struct console_font_op font = {
		.op = KD_FONT_OP_GET, .width = 32, .height = FONT_PITCH,
		.charcount = 512, .data = glyphs,
	};
	char previous[LOG_BUFFER_SIZE] = "";
	struct dk2_touch touch;
	struct timespec last_refresh = { 0 };
	time_t last_check = -1;
	int logs_visible = 0;
	int have_font;

	have_font = load_console_font(&font) == 0 && font.width && font.height &&
		    font.charcount >= 128;
	if (!have_font)
		syslog(LOG_WARNING, "Could not load the console font for HDMI boot logs");

	while (!hdmi_disconnected(drm_fd, display->connector_id, &last_check)) {
		struct pollfd input = {
			.fd = have_font ? dk2_touch_open(&touch, BUS_USB) : -1,
			.events = POLLIN,
		};

		if (input.fd < 0) {
			sleep(1);
			continue;
		}

		for (;;) {
			struct input_event events[32];
			int ready = poll(&input, 1, logs_visible ? 250 : 1000);

			if (ready < 0 && errno == EINTR)
				continue;
			if (ready < 0 || (input.revents & (POLLERR | POLLHUP | POLLNVAL)))
				break;
			if (hdmi_disconnected(drm_fd, display->connector_id, &last_check)) {
				close(input.fd);
				return;
			}
			if (input.revents & POLLIN) {
				ssize_t size = read(input.fd, events, sizeof(events));

				if (size < 0 && errno == EINTR)
					continue;
				if (size <= 0 || size % sizeof(events[0]) != 0)
					break;
				for (size_t i = 0; i < (size_t)size / sizeof(events[0]); i++) {
					enum dk2_gesture gesture = dk2_touch_event(&touch, &events[i]);

					if (!update_view(display, &logs_visible, gesture))
						continue;
					if (logs_visible) {
						if (gesture == DK2_GESTURE_HOLD) {
							draw_text(display, &font, previous);
						} else {
							refresh_logs(display, &font, previous);
							clock_gettime(CLOCK_MONOTONIC, &last_refresh);
						}
					} else {
						draw_photo(display);
						previous[0] = '\0';
					}
				}
			}
			if (logs_visible) {
				struct timespec now;

				clock_gettime(CLOCK_MONOTONIC, &now);
				if (now.tv_sec > last_refresh.tv_sec) {
					refresh_logs(display, &font, previous);
					last_refresh = now;
				}
			}
		}
		close(input.fd);
		if (logs_visible) {
			logs_visible = 0;
			previous[0] = '\0';
			draw_photo(display);
		}
	}
}

int main(int argc, char **argv)
{
	struct splash_display display;
	int fd;

	if (argc != 2 || fread(photo, sizeof(photo), 1, stdin) != 1)
		return EXIT_FAILURE;
	fd = open(argv[1], O_RDWR | O_CLOEXEC);
	if (fd < 0 || show_hdmi(fd, &display) < 0)
		return EXIT_FAILURE;

	/* Keep the DRM file open so the startup framebuffer stays on screen. */
	if (daemon(0, 0) < 0)
		return EXIT_FAILURE;
	watch_touch(&display, fd);

	/* Release both references so the DRM fbdev client can restore the LCD. */
	munmap(display.pixels, display.buffer.size);
	close(fd);
	execl("/etc/init.d/lcd-startup", "lcd-startup", "start", (char *)NULL);
	syslog(LOG_ERR, "Could not start the LCD splash after HDMI disconnect: %m");
	return EXIT_FAILURE;
}
