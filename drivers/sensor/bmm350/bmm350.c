#define DT_DRV_COMPAT bosch_bmm350

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(bmm350, CONFIG_SENSOR_LOG_LEVEL);

#define INT24_MAX 0x7FFFFF

#define BMM350_SOFT_RESET_DELAY_US       24000U
#define BMM350_START_UP_TIME_FROM_POR_US 3000U

#define BMM350_CHIP_ID     0x00U
#define BMM350_PMU_CMD     0x06U
#define BMM350_MAG_X_XLSB  0x31U
#define BMM350_OPT_CMD_REG 0x50U
#define BMM350_CMD         0x7EU

#define BMM350_CHIP_ID_VAL 0x33U

#define BMM350_PMU_CMD_NM  0x01U

#define BMM350_OPT_CMD_REG_OPT_CMD             GENMASK(7, 5)
#define BMM350_OPT_CMD_REG_OPT_CMD_PWR_OFF_OTP 0x4U

#define BMM350_CMD_SOFTRESET 0xB6U

struct bmm350_config {
	struct i2c_dt_spec i2c;
};

struct bmm350_data {
	struct sensor_value x;
	struct sensor_value y;
	struct sensor_value z;
};

static int bmm350_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct bmm350_config *config = dev->config;
	struct bmm350_data *data = dev->data;
	uint8_t buf[9];
	uint8_t addr;
	int ret;

	addr = BMM350_MAG_X_XLSB;
	ret = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Could not read data (%d)", ret);
		return ret;
	}

	/* just obtain raw data, conversion to uT requires reading OTP! */
	data->x.val1 = (int32_t)sys_get_le24(&buf[0]);
	if (data->x.val1 > INT24_MAX) {
		data->x.val1 -= (INT24_MAX + 1) * 2;
	}

	data->y.val1 = (int32_t)sys_get_le24(&buf[3]);
	if (data->y.val1 > INT24_MAX) {
		data->y.val1 -= (INT24_MAX + 1) * 2;
	}

	data->z.val1 = (int32_t)sys_get_le24(&buf[6]);
	if (data->z.val1 > INT24_MAX) {
		data->z.val1 -= (INT24_MAX + 1) * 2;
	}

	return 0;
}

static int bmm350_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmm350_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		*val = data->x;
		break;
	case SENSOR_CHAN_MAGN_Y:
		*val = data->y;
		break;
	case SENSOR_CHAN_MAGN_Z:
		*val = data->z;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bmm350_api = {
	.sample_fetch = bmm350_sample_fetch,
	.channel_get = bmm350_channel_get,
};

static int bmm350_init(const struct device *dev)
{
	const struct bmm350_config *config = dev->config;
	uint8_t chip_id;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	k_usleep(BMM350_START_UP_TIME_FROM_POR_US);

	ret = i2c_reg_read_byte_dt(&config->i2c, BMM350_CHIP_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Could not read chip id (%d)", ret);
		return ret;
	}

	if (chip_id != BMM350_CHIP_ID) {
		LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, BMM350_CMD, BMM350_CMD_SOFTRESET);
	if (ret < 0) {
		LOG_ERR("Could not reset device (%d)", ret);
		return ret;
	}

	k_usleep(BMM350_SOFT_RESET_DELAY_US);

	/* turn off OTP (ignore OTP data for now) */
	ret = i2c_reg_write_byte_dt(
		&config->i2c, BMM350_OPT_CMD_REG,
		FIELD_PREP(BMM350_OPT_CMD_REG_OPT_CMD, BMM350_OPT_CMD_REG_OPT_CMD_PWR_OFF_OTP));
	if (ret < 0) {
		LOG_ERR("Could not power off OPT (%d)", ret);
		return ret;
	}

	/* turn ON, use AVG/ODR defaults after reset */
	ret = i2c_reg_write_byte_dt(&config->i2c, BMM350_PMU_CMD, BMM350_PMU_CMD_NM);
	if (ret < 0) {
		LOG_ERR("Could not power up device (%d)", ret);
		return ret;
	}

	return 0;
}

#define BMM350_DEFINE(i)                                                                           \
	static const struct bmm350_config bmm350_config_##i = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(i),                                                    \
	};                                                                                         \
                                                                                                   \
	static struct bmm350_data bmm350_data_##i;                                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(i, bmm350_init, NULL, &bmm350_data_##i, &bmm350_config_##i,          \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bmm350_api);

DT_INST_FOREACH_STATUS_OKAY(BMM350_DEFINE)
