#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>

#include <bluetooth/services/lbs.h>

#include <app_version.h>

LOG_MODULE_REGISTER(app, CONFIG_APP_LOG_LEVEL);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LBS_VAL),
};

static const struct device *const disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
static const struct device *const buttons = DEVICE_DT_GET(DT_NODELABEL(buttons));
static const struct device *const led = DEVICE_DT_GET(DT_ALIAS(led0));
static const struct device *const tap = DEVICE_DT_GET(DT_ALIAS(tap0));

static bool button_state;
static bool tap_led_state;

#define DISP_WIDTH  DT_PROP(DT_CHOSEN(zephyr_display), width)
#define DISP_HEIGHT DT_PROP(DT_CHOSEN(zephyr_display), height)
static uint8_t disp_buf[DISP_WIDTH * DISP_HEIGHT / 8];

static void draw_filled_rect(uint8_t *buf, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	for (size_t row = y0; row <= y1; row++) {
		for (size_t col = x0; col <= x1; col++) {
			buf[row * DISP_WIDTH / 8 + col / 8] |= 1 << (col % 8);
		}
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0U) {
		LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("Connected");

	(void)led_on(led, 0);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));

	(void)led_off(led, 0);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void buttons_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	button_state = (bool)evt->value;

	(void)bt_lbs_send_button_state(button_state);
}

INPUT_CALLBACK_DEFINE(buttons, buttons_input_cb, NULL);

static void led_cb(bool led_state)
{
	if (led_state) {
		(void)led_on(led, 1);
	} else {
		(void)led_off(led, 1);
	}
}

static bool button_cb(void)
{
	return button_state;
}

static struct bt_lbs_cb lbs_callbacks = {
	.led_cb = led_cb,
	.button_cb = button_cb,
};

static void tap_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	tap_led_state = !tap_led_state;

	if (tap_led_state) {
		(void)led_on(led, 2);
	} else {
		(void)led_off(led, 2);
	}
}

INPUT_CALLBACK_DEFINE(tap, tap_input_cb, NULL);

int main(void)
{
	int err;
	struct display_buffer_descriptor desc = {
		.buf_size = sizeof(disp_buf),
		.width = DISP_WIDTH,
		.height = DISP_HEIGHT,
		.pitch = DISP_WIDTH,
	};

	LOG_INF("Pebble Application %s", APP_VERSION_STRING);

	if (!device_is_ready(disp)) {
		LOG_ERR("Display device not ready");
		return 0;
	}

	err = display_blanking_off(disp);
	if (err < 0) {
		LOG_ERR("Failed to turn on display (%d)", err);
		return 0;
	}

	draw_filled_rect(disp_buf, 16, 16, DISP_WIDTH - 16, DISP_HEIGHT - 16);
	err = display_write(disp, 0, 0, &desc, disp_buf);
	if (err < 0) {
		LOG_ERR("Failed to write to display (%d)", err);
		return 0;
	}

	for (int8_t i = 100; i >= 0; i -= 10) {
		err = display_set_brightness(disp, (uint8_t)i);
		if (err < 0) {
			LOG_ERR("Failed to set brightness (%d)", err);
			return 0;
		}

		k_msleep(100);
	}

	if (!device_is_ready(led)) {
		LOG_ERR("LED device not ready");
		return 0;
	}

	if (!device_is_ready(tap)) {
		LOG_ERR("Tap device not ready");
		return 0;
	}

	err = bt_enable(NULL);
	if (err < 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}

	(void)bt_lbs_init(&lbs_callbacks);

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err < 0) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return 0;
	}

	return 0;
}
