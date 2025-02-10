#include <stdlib.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>

static const struct device *const press = DEVICE_DT_GET(DT_ALIAS(press0));
static bool initialized;

static int cmd_press_get(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct sensor_value val;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!initialized) {
		shell_error(sh, "Pressure sensor module not initialized");
		return -EPERM;
	}

	err = pm_device_runtime_get(press);
	if (err < 0) {
		shell_error(sh, "Failed to get device (%d)", err);
		return 0;
	}

	err = sensor_sample_fetch(press);
	if (err < 0) {
		shell_error(sh, "Failed to fetch sensor data (%d)", err);
		return 0;
	}

	err = sensor_channel_get(press, SENSOR_CHAN_PRESS, &val);
	if (err < 0) {
		shell_error(sh, "Failed to get pressure data (%d)", err);
		return 0;
	}

	shell_print(sh, "Pressure: %.2f kPa", sensor_value_to_double(&val));

	err = sensor_channel_get(press, SENSOR_CHAN_AMBIENT_TEMP, &val);
	if (err < 0) {
		shell_error(sh, "Failed to get temperature data (%d)", err);
		return 0;
	}

	shell_print(sh, "Temperature: %.2f C", sensor_value_to_double(&val));

	(void)pm_device_runtime_put(press);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_press_cmds,
			       SHELL_CMD(get, NULL, "Get sensor data",
					     cmd_press_get),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), press, &sub_press_cmds, "Pressure sensor", NULL, 0, 0);

int press_init(void)
{
	int ret;

	if (!device_is_ready(press)) {
		return -ENODEV;
	}

	ret = pm_device_runtime_enable(press);
	if (ret < 0) {
		return ret;
	}

	initialized = true;

	return 0;
}
