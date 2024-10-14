#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
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

static const struct device *const led = DEVICE_DT_GET(DT_ALIAS(led0));

static bool button_state;

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

static void input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	button_state = (bool)evt->value;

	(void)bt_lbs_send_button_state(button_state);
}

INPUT_CALLBACK_DEFINE(NULL, input_cb, NULL);

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

int main(void)
{
	int err;

	LOG_INF("Pebble Application %s", APP_VERSION_STRING);

	if (!device_is_ready(led)) {
		LOG_ERR("LED device not ready");
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
