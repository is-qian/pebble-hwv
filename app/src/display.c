#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/shell/shell.h>

#define DISP_WIDTH  DT_PROP(DT_CHOSEN(zephyr_display), width)
#define DISP_HEIGHT DT_PROP(DT_CHOSEN(zephyr_display), height)

static const struct device *const disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct display_buffer_descriptor desc = {
	.width = DISP_WIDTH,
	.height = DISP_HEIGHT,
	.pitch = DISP_WIDTH + 2 * 8,
};

static int cmd_display_on(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = display_blanking_off(disp);
	if (err < 0) {
		shell_error(sh, "Failed to turn on display (%d)", err);
		return 0;
	}

	shell_print(sh, "Display ON");

	return 0;
}

static int cmd_display_off(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	err = display_blanking_on(disp);
	if (err < 0) {
		shell_error(sh, "Failed to turn off display (%d)", err);
		return 0;
	}

	shell_print(sh, "Display OFF");

	return 0;
}

static int cmd_display_vpattern(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t *buf;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	buf = display_get_framebuffer(disp);

	for (uint16_t i = 0; i < desc.height; i++) {
		memset(&buf[i * desc.pitch / 8], 0xff, desc.width / 8);
	}

	for (uint16_t y = 0U; y <= desc.height; y++) {
		for (uint16_t x = 0U; x <= desc.width; x++) {
			if (x % 4 != 0) {
				buf[y * desc.pitch / 8 + x / 8] &= ~(1 << (x % 8));
			}
		}
	}

	err = display_write(disp, 0, 0, &desc, buf);
	if (err < 0) {
		shell_error(sh, "Failed to write to display (%d)", err);
		return 0;
	}

	shell_print(sh, "Vertical pattern displayed");

	return 0;
}

static int cmd_display_hpattern(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t *buf;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	buf = display_get_framebuffer(disp);

	for (uint16_t i = 0; i < desc.height; i++) {
		memset(&buf[i * desc.pitch / 8], 0xff, desc.width / 8);
	}

	for (uint16_t y = 0U; y <= desc.height; y++) {
		for (uint16_t x = 0U; x <= desc.width; x++) {
			if (y % 4 != 0) {
				buf[y * desc.pitch / 8 + x / 8] &= ~(1 << (x % 8));
			}
		}
	}

	err = display_write(disp, 0, 0, &desc, buf);
	if (err < 0) {
		shell_error(sh, "Failed to write to display (%d)", err);
		return 0;
	}

	shell_print(sh, "Horizontal pattern displayed");

	return 0;
}

static int cmd_display_brightness(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t brightness;

	if (argc < 2) {
		shell_error(sh, "Missing brightness value");
		return -EINVAL;
	}

	brightness = strtoul(argv[1], NULL, 0);
	if (brightness > 100) {
		shell_error(sh, "Invalid brightness value");
		return -EINVAL;
	}

	err = display_set_brightness(disp, brightness);
	if (err < 0) {
		shell_error(sh, "Failed to set brightness (%d)", err);
		return 0;
	}

	shell_print(sh, "Brightness set to %d%%", brightness);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_display_cmds, SHELL_CMD(on, NULL, "Turn on display", cmd_display_on),
	SHELL_CMD(off, NULL, "Turn off display", cmd_display_off),
	SHELL_CMD(vpattern, NULL, "Display vertical pattern", cmd_display_vpattern),
	SHELL_CMD(hpattern, NULL, "Display horizontal pattern", cmd_display_hpattern),
	SHELL_CMD_ARG(brightness, NULL, "Set display brightness", cmd_display_brightness, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), display, &sub_display_cmds, "Display", NULL, 0, 0);
