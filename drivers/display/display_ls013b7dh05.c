#define DT_DRV_COMPAT sharp_ls013b7dh05

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ls013b7dh05, CONFIG_DISPLAY_LOG_LEVEL);

#define LS013B7DH05_WRITE BIT(0)
#define LS013B7DH05_CLEAR BIT(2)

struct ls013b7dh05_config {
	struct spi_dt_spec spi;
	struct pwm_dt_spec extcomin;
	struct gpio_dt_spec disp;
	const struct device *backlight;
	uint8_t width;
	uint8_t height;
	uint8_t line_width;
	uint8_t *tx_buf;
};

static int ls013b7dh05_clear(const struct device *dev)
{
	const struct ls013b7dh05_config *config = dev->config;
	struct spi_buf sbuf;
	struct spi_buf_set sbufs = {.buffers = &sbuf, .count = 1U};
	uint8_t buf[2];

	buf[0] = LS013B7DH05_CLEAR;
	buf[1] = 0U;
	sbuf.buf = buf;
	sbuf.len = 1U;

	return spi_write_dt(&config->spi, &sbufs);
}

static int ls013b7dh05_blanking_on(const struct device *dev)
{
	const struct ls013b7dh05_config *config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->disp, 0);
	if (ret < 0) {
		return ret;
	}

	ret = pwm_set_pulse_dt(&config->extcomin, 0U);
	if (ret < 0) {
		return ret;
	}

	ret = led_off(config->backlight, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ls013b7dh05_blanking_off(const struct device *dev)
{
	const struct ls013b7dh05_config *config = dev->config;
	int ret;

	ret = gpio_pin_set_dt(&config->disp, 1);
	if (ret < 0) {
		return ret;
	}

	ret = pwm_set_pulse_dt(&config->extcomin, PWM_USEC(100));
	if (ret < 0) {
		return ret;
	}

	ret = led_on(config->backlight, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int ls013b7dh05_write(const struct device *dev, uint16_t x, uint16_t y,
			     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct ls013b7dh05_config *config = dev->config;
	struct spi_buf sbuf = {.buf = config->tx_buf};
	struct spi_buf_set sbufs = {.buffers = &sbuf, .count = 1U};
	uint8_t *tx_buf = config->tx_buf;
	uint8_t *d_buf = (uint8_t *)buf;
	uint8_t current_y = y + 1;
	uint8_t lines_pending = desc->height;
	int ret;

	if (desc->pitch != config->width || desc->height > config->height || desc->height == 0U) {
		LOG_ERR("Unsupported buffer descriptor");
		return -EINVAL;
	}

	if (x != 0U || y >= config->height) {
		LOG_ERR("Unsupported position");
		return -EINVAL;
	}

	while (lines_pending > 0) {
		uint16_t cnt = 0;
		uint8_t n_lines = MIN(CONFIG_DISPLAY_LS013B7DH05_TXBUF_LINES, lines_pending);

		/* WRITE */
		tx_buf[cnt++] = LS013B7DH05_WRITE;

		while (n_lines-- > 0) {
			/* ADDR */
			tx_buf[cnt++] = current_y++;
			/* DATA */
			memcpy(&tx_buf[cnt], d_buf, config->line_width);
			cnt += config->line_width;
			d_buf += config->line_width;
			/* DUMMY */
			tx_buf[cnt++] = 0U;

			lines_pending--;
		}

		/* LAST DUMMY */
		tx_buf[cnt++] = 0U;

		sbuf.len = cnt;
		ret = spi_write_dt(&config->spi, &sbufs);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int ls013b7dh05_set_brightness(const struct device *dev, uint8_t brightness)
{
	const struct ls013b7dh05_config *config = dev->config;

	if (brightness == 0U) {
		return led_off(config->backlight, 0);
	} else {
		int ret;

		ret = led_set_brightness(config->backlight, 0, brightness);
		if (ret < 0) {
			return ret;
		}

		return led_on(config->backlight, 0);
	}
}

static void ls013b7dh05_get_capabilities(const struct device *dev,
					 struct display_capabilities *capabilities)
{
	const struct ls013b7dh05_config *config = dev->config;

	memset(capabilities, 0, sizeof(*capabilities));

	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	capabilities->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	capabilities->current_pixel_format = PIXEL_FORMAT_MONO01;
	capabilities->current_orientation = 0;
}

static int ls013b7dh05_init(const struct device *dev)
{
	const struct ls013b7dh05_config *config = dev->config;
	int ret;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI not ready");
		return -ENODEV;
	}

	if (!pwm_is_ready_dt(&config->extcomin)) {
		LOG_ERR("EXTCOMIN PWM not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->disp)) {
		LOG_ERR("DISP GPIO not ready");
		return -ENODEV;
	}

	if (!device_is_ready(config->backlight)) {
		LOG_ERR("Backlight device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->disp, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure DISP GPIO");
		return ret;
	}

	ret = ls013b7dh05_clear(dev);
	if (ret < 0) {
		LOG_ERR("Failed to clear display");
		return ret;
	}

	return 0;
}

static const struct display_driver_api ls013b7dh05_api = {
	.blanking_on = ls013b7dh05_blanking_on,
	.blanking_off = ls013b7dh05_blanking_off,
	.write = ls013b7dh05_write,
	.set_brightness = ls013b7dh05_set_brightness,
	.get_capabilities = ls013b7dh05_get_capabilities,
};

#define LS013B7DH05_DEFINE(n)                                                                      \
	static uint8_t tx_buf##n[CONFIG_DISPLAY_LS013B7DH05_TXBUF_LINES *                          \
					 (DIV_ROUND_UP(DT_INST_PROP(n, width), 8U) + 2U) +         \
				 2U];                                                              \
                                                                                                   \
	static const struct ls013b7dh05_config ls013b7dh05_config_##n = {                          \
		.spi = SPI_DT_SPEC_INST_GET(n,                                                     \
					    SPI_OP_MODE_MASTER | SPI_WORD_SET(8U) |                \
						    SPI_TRANSFER_LSB | SPI_CS_ACTIVE_HIGH,         \
					    0U),                                                   \
		.disp = GPIO_DT_SPEC_INST_GET(n, disp_gpios),                                      \
		.extcomin = PWM_DT_SPEC_INST_GET(n),                                               \
		.backlight = DEVICE_DT_GET(DT_INST_PHANDLE(n, backlight)),                         \
		.width = DT_INST_PROP(n, width),                                                   \
		.height = DT_INST_PROP(n, height),                                                 \
		.line_width = DIV_ROUND_UP(DT_INST_PROP(n, width), 8U),                            \
		.tx_buf = tx_buf##n,                                                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ls013b7dh05_init, NULL, NULL, &ls013b7dh05_config_##n,           \
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &ls013b7dh05_api);

DT_INST_FOREACH_STATUS_OKAY(LS013B7DH05_DEFINE)
