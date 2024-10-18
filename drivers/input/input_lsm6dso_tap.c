#define DT_DRV_COMPAT st_lsm6dso_tap

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <stmemsc.h>
#include <lsm6dso_reg.h>

LOG_MODULE_REGISTER(lsm6dso_tap, CONFIG_INPUT_LOG_LEVEL);

struct lsm6dso_tap_config {
	stmdev_ctx_t ctx;
	struct spi_dt_spec spi;
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

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI not ready");
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

	/* full scale: +/- 2g */
	ret = lsm6dso_xl_full_scale_set(ctx, LSM6DSO_2g);
	if (ret < 0) {
		LOG_ERR("Failed to set accelerometer data rate");
		return ret;
	}

	/* data rate: 417 Hz */
	ret = lsm6dso_xl_data_rate_set(ctx, LSM6DSO_XL_ODR_417Hz);
	if (ret < 0) {
		LOG_ERR("Failed to set accelerometer data rate");
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

	/* X,Y,Z threshold: 9 * FS_XL / 2^5 = 9 * 2 / 32 = 562.5 mg */
	ret = lsm6dso_tap_threshold_x_set(ctx, 9);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on X axis");
		return ret;
	}

	ret = lsm6dso_tap_threshold_y_set(ctx, 9);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on Y axis");
		return ret;
	}

	ret = lsm6dso_tap_threshold_z_set(ctx, 9);
	if (ret < 0) {
		LOG_ERR("Failed to set tap threshold on Z axis");
		return ret;
	}

	/* shock time: 2 * 8 / ODR_XL = 2 * 8 / 417 ~= 38.3 ms */
	ret = lsm6dso_tap_shock_set(ctx, 2);
	if (ret < 0) {
		LOG_ERR("Failed to set tap shock duration");
		return ret;
	}

	/* quiet time: 1 * 4 / ODR_XL = 1 * 4 / 417 ~= 9.6 ms */
	ret = lsm6dso_tap_quiet_set(ctx, 1);
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
		STMEMSC_CTX_SPI(&lsm6dso_tap_config_##n.spi),                                      \
		.spi = SPI_DT_SPEC_INST_GET(                                                       \
			n, SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA,   \
			0U),                                                                       \
		.int1 = GPIO_DT_SPEC_INST_GET(n, int1_gpios),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &lsm6dso_tap_init, NULL, &lsm6dso_tap_data_##n,                   \
			      &lsm6dso_tap_config_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,    \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(LSM6DSO_TAP_DEFINE)
