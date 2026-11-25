// SPDX-License-Identifier: GPL-2.0-only
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <drm/drm.h>
#include <drm/drm_mode.h>

#define PHOTO_WIDTH 480
#define PHOTO_HEIGHT 800
#define HDMI_CONNECTED 1

static uint32_t photo[PHOTO_HEIGHT][PHOTO_WIDTH];

static int show_mode(int fd, uint32_t crtc_id, uint32_t connector_id,
		     const struct drm_mode_modeinfo *mode)
{
	struct drm_mode_create_dumb buffer = {
		.width = mode->hdisplay, .height = mode->vdisplay, .bpp = 32,
	};
	struct drm_mode_map_dumb mapping = { 0 };
	struct drm_mode_fb_cmd fb = { 0 };
	struct drm_mode_crtc crtc = { 0 };
	struct drm_mode_destroy_dumb destroy = { 0 };
	unsigned int width, height, left, top;
	unsigned int x, y;
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

	width = buffer.width;
	height = width * PHOTO_HEIGHT / PHOTO_WIDTH;
	if (height > buffer.height) {
		height = buffer.height;
		width = height * PHOTO_WIDTH / PHOTO_HEIGHT;
	}
	left = (buffer.width - width) / 2;
	top = (buffer.height - height) / 2;
	for (y = 0; y < height; y++) {
		uint32_t *row = (uint32_t *)((char *)pixels + (top + y) * buffer.pitch);

		for (x = 0; x < width; x++)
			row[left + x] = photo[y * PHOTO_HEIGHT / height]
					     [x * PHOTO_WIDTH / width];
	}
	munmap(pixels, buffer.size);

	fb.width = buffer.width;
	fb.height = buffer.height;
	fb.pitch = buffer.pitch;
	fb.bpp = 32;
	fb.depth = 24;
	fb.handle = buffer.handle;
	if (ioctl(fd, DRM_IOCTL_MODE_ADDFB, &fb) < 0)
		goto destroy_buffer;

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
destroy_buffer:
	destroy.handle = buffer.handle;
	ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
	return result;
}

static int show_hdmi(int fd)
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
		connector.modes_ptr = (uintptr_t)modes;
		if (ioctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) == 0 &&
		    connector.count_modes <= count) {
			/* The kernel puts preferred modes first. */
			for (j = 0; j < connector.count_modes && result != 0; j++)
				result = show_mode(fd, crtc_id, connector.connector_id, &modes[j]);
		}
		free(modes);
		return result;
	}
	return -1;
}

int main(int argc, char **argv)
{
	int fd;

	if (argc != 2 || fread(photo, sizeof(photo), 1, stdin) != 1)
		return EXIT_FAILURE;
	fd = open(argv[1], O_RDWR);
	if (fd < 0 || show_hdmi(fd) < 0)
		return EXIT_FAILURE;

	/* Keep the DRM file open so the startup framebuffer stays on screen. */
	if (daemon(0, 0) < 0)
		return EXIT_FAILURE;
	for (;;)
		pause();
}
