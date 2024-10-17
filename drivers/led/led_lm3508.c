#define DT_DRV_COMPAT ti_lm3508

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(lm3508, CONFIG_LED_LOG_LEVEL);

struct lm3508_config {
	struct pwm_dt_spec pwm;
	struct gpio_dt_spec en;
};

static int lm3508_on(const struct device *dev, uint32_t led)
{
	const struct lm3508_config *config = dev->config;
	int ret;

	ARG_UNUSED(led);

	ret = gpio_pin_set_dt(&config->en, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int lm3508_off(const struct device *dev, uint32_t led)
{
	const struct lm3508_config *config = dev->config;
	int ret;

	ARG_UNUSED(led);

	ret = gpio_pin_set_dt(&config->en, 0);
	if (ret < 0) {
		return ret;
	}

	return pwm_set_pulse_dt(&config->pwm, 0U);
}

static int lm3508_set_brightness(const struct device *dev, uint32_t led, uint8_t brightness)
{
	const struct lm3508_config *config = dev->config;

	ARG_UNUSED(led);

	return pwm_set_pulse_dt(&config->pwm, config->pwm.period * brightness / 100U);
}

static const struct led_driver_api lm3508_api = {
	.on = lm3508_on,
	.off = lm3508_off,
	.set_brightness = lm3508_set_brightness,
};

static int lm3508_init(const struct device *dev)
{
	const struct lm3508_config *config = dev->config;
	int ret;

	if (!pwm_is_ready_dt(&config->pwm)) {
		LOG_ERR("PWM not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&config->en)) {
		LOG_ERR("EN GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->en, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure EN GPIO");
		return ret;
	}

	return 0;
}

#define LS013B7DH05_DEFINE(n)                                                                      \
	static const struct lm3508_config lm3508_config_##n = {                                    \
		.en = GPIO_DT_SPEC_INST_GET(n, en_gpios),                                          \
		.pwm = PWM_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &lm3508_init, NULL, NULL, &lm3508_config_##n, POST_KERNEL,        \
			      CONFIG_LED_LM3508_INIT_PRIORITY, &lm3508_api);

DT_INST_FOREACH_STATUS_OKAY(LS013B7DH05_DEFINE)
