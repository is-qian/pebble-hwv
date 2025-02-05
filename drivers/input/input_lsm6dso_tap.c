#define DT_DRV_COMPAT st_lsm6dso_tap

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <stmemsc.h>
#include <lsm6dso_reg.h>

LOG_MODULE_REGISTER(lsm6dso_tap, CONFIG_INPUT_LOG_LEVEL);

struct lsm6dso_tap_config {
	stmdev_ctx_t ctx;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int1;
};

struct lsm6dso_tap_data {
	const struct device *dev;
	struct gpio_callback int1_cb;
};

static void lsm6dso_int1_callback_handler(const struct device *port, struct gpio_callback *cb,
					  gpio_port_pins_t pins)
{
	struct lsm6dso_tap_data *data = CONTAINER_OF(cb, struct lsm6dso_tap_data, int1_cb);

	LOG_DBG("TAP detected");

	input_report_key(data->dev, INPUT_BTN_TOUCH, 0, true, K_NO_WAIT);
}

static int lsm6dso_tap_init(const struct device *dev)
{
	const struct lsm6dso_tap_config *config = dev->config;
	struct lsm6dso_tap_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&config->ctx;
	int ret;
	uint8_t chip_id;
	uint8_t rst = 0;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->int1)) {
		LOG_ERR("INT1 GPIO not ready");
		return -ENODEV;
	}

	ret = lsm6dso_device_id_get(ctx, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id");
		return ret;
	}

	if (chip_id != LSM6DSO_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -ENODEV;
	}

	/*
	 * initialize sensor for single tap detection
	 * ref. 5.5.4 Single-tap example, AN5192
	 */

	lsm6dso_reset_set(ctx, PROPERTY_ENABLE);
	do {
		lsm6dso_reset_get(ctx, &rst);
	} while (rst);

	/* full scale: +/- 2g */
	ret = lsm6dso_xl_full_scale_set(ctx, LSM6DSO_2g);
	if (ret < 0) {
		LOG_ERR("Failed to set accelerometer data rate");
		return ret;
	}

	/* data rate: 26 Hz */
	ret = lsm6dso_xl_data_rate_set(ctx, LSM6DSO_XL_ODR_26Hz);
	if (ret < 0) {
		LOG_ERR("Failed to set accelerometer data rate");
		return ret;
	}

	ret = lsm6dso_fsm_data_rate_set(ctx, LSM6DSO_ODR_FSM_26Hz);
	if (ret < 0) {
		LOG_ERR("Failed to set FSM data rate");
		return ret;
	}

	/* low power normal mode (no HP) */
	ret = lsm6dso_xl_power_mode_set(ctx, LSM6DSO_LOW_NORMAL_POWER_MD);
	if (ret < 0) {
		LOG_ERR("Failed to set power mode");
		return ret;
	}

	/* tap detection on all axis: X, Y, Z */
	ret = lsm6dso_tap_detection_on_x_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to enable tap detection on X axis");
		return ret;
	}

	ret = lsm6dso_tap_detection_on_y_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to enable tap detection on Y axis");
		return ret;
	}

	ret = lsm6dso_tap_detection_on_z_set(ctx, PROPERTY_ENABLE);
	if (ret < 0) {
		LOG_ERR("Failed to enable tap detection on Z axis");
		return ret;
	}

	/* X,Y,Z threshold: 1 * FS_XL / 2^5 = 1 * 2 / 32 = 62.5 mg */
	ret = lsm6dso_tap_threshold_x_set(ctx, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on X axis");
		return ret;
	}

	ret = lsm6dso_tap_threshold_y_set(ctx, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on Y axis");
		return ret;
	}

	ret = lsm6dso_tap_threshold_z_set(ctx, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on Z axis");
		return ret;
	}

	/* shock time: 2 / ODR_XL = 2 / 26 ~= 77 ms */
	ret = lsm6dso_tap_shock_set(ctx, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return ret;
	}

	/* quiet time: 2 / ODR_XL = 2 / 26 ~= 77 ms */
	ret = lsm6dso_tap_quiet_set(ctx, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set tap quiet duration");
		return ret;
	}

	/* enable single tap only */
	ret = lsm6dso_tap_mode_set(ctx, LSM6DSO_ONLY_SINGLE);
	if (ret < 0) {
		LOG_ERR("Failed to set tap mode");
		return ret;
	}

	/* route single rap to INT1 */
	lsm6dso_pin_int1_route_t route = {.single_tap = 1};
	ret = lsm6dso_pin_int1_route_set(ctx, route);
	if (ret < 0) {
		LOG_ERR("Failed to route interrupt");
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->int1, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure INT1 GPIO");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int1, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure INT1 GPIO interrupt");
		return ret;
	}

	gpio_init_callback(&data->int1_cb, lsm6dso_int1_callback_handler, BIT(config->int1.pin));

	ret = gpio_add_callback(config->int1.port, &data->int1_cb);
	if (ret < 0) {
		LOG_ERR("Could not add INT1 GPIO callback (%d)", ret);
		return ret;
	}

	return 0;
}

#define LSM6DSO_TAP_DEFINE(n)                                                                      \
	static struct lsm6dso_tap_data lsm6dso_tap_data_##n = {                                    \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
                                                                                                   \
	static const struct lsm6dso_tap_config lsm6dso_tap_config_##n = {                          \
		STMEMSC_CTX_I2C(&lsm6dso_tap_config_##n.i2c),                                      \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
		.int1 = GPIO_DT_SPEC_INST_GET(n, int1_gpios),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &lsm6dso_tap_init, NULL, &lsm6dso_tap_data_##n,                   \
			      &lsm6dso_tap_config_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(LSM6DSO_TAP_DEFINE)
