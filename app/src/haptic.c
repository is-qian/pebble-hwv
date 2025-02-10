#include <stdlib.h>

#include <zephyr/shell/shell.h>

#include <hwv/drivers/haptic.h>

static const struct device *const haptic = DEVICE_DT_GET(DT_ALIAS(haptic0));
static bool initialized;

static int cmd_haptic_configure(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	uint8_t ampl;

	if (!initialized) {
		shell_error(sh, "Haptic module not initialized");
		return -EPERM;
	}

	if (argc < 2) {
		shell_error(sh, "Missing amplitude");
		return -EINVAL;
	}

	ampl = strtoul(argv[1], NULL, 0);
	if (ampl > 100U) {
		shell_error(sh, "Amplitude must be less than or equal to 100");
		return -EINVAL;
	}

	err = haptic_configure(haptic, ampl);
	if (err < 0) {
		shell_error(sh, "Failed to configure haptic (%d)", err);
		return err;
	}

	shell_print(sh, "Haptic configured");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_haptic_cmds,
			       SHELL_CMD_ARG(configure, NULL, "Configure haptic",
					     cmd_haptic_configure, 2, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), haptic, &sub_haptic_cmds, "Haptic", NULL, 0, 0);

int haptic_init(void)
{
	if (!device_is_ready(haptic)) {
		return -ENODEV;
	}

	initialized = true;

	return 0;
}
