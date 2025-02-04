#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

static const struct device *const mag = DEVICE_DT_GET(DT_ALIAS(mag0));
static bool initialized;

static int cmd_mag_get(const struct shell *sh, size_t argc, char **argv)
{
	int err;
	struct sensor_value val_x, val_y, val_z;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!initialized) {
		shell_error(sh, "Magure sensor module not initialized");
		return -EPERM;
	}

	err = sensor_sample_fetch(mag);
	if (err < 0) {
		shell_error(sh, "Failed to fetch sensor data (%d)", err);
		return 0;
	}

	err = sensor_channel_get(mag, SENSOR_CHAN_MAGN_X, &val_x);
	if (err < 0) {
		shell_error(sh, "Failed to get X magnetic field data (%d)", err);
		return 0;
	}

	err = sensor_channel_get(mag, SENSOR_CHAN_MAGN_Y, &val_y);
	if (err < 0) {
		shell_error(sh, "Failed to get Y magnetic field data (%d)", err);
		return 0;
	}

	err = sensor_channel_get(mag, SENSOR_CHAN_MAGN_Z, &val_z);
	if (err < 0) {
		shell_error(sh, "Failed to get Z magnetic field data (%d)", err);
		return 0;
	}

	shell_print(sh, "Magnetic field (gauss): %.6f, %.6f, %.6f", val_x.val1 + val_x.val2 / 100.0,
		    val_y.val1 + val_y.val2 / 100.0, val_z.val1 + val_z.val2 / 100.0);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mag_cmds, SHELL_CMD(get, NULL, "Get sensor data", cmd_mag_get),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), mag, &sub_mag_cmds, "Magnetic sensor", NULL, 0, 0);

int mag_init(void)
{
	int ret;
	struct sensor_value val;

	if (!device_is_ready(mag)) {
		return -ENODEV;
	}

	val.val1 = 25;
	val.val2 = 0;
	ret = sensor_attr_set(mag, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &val);
	if (ret < 0) {
		return ret;
	}

	/* "Normal" mode */
	val.val1 = 1;
	val.val2 = 0;
	ret = sensor_attr_set(mag, SENSOR_CHAN_MAGN_XYZ, SENSOR_ATTR_OVERSAMPLING, &val);
	if (ret < 0) {
		return ret;
	}

	initialized = true;

	return 0;
}
