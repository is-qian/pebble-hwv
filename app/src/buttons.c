#include <errno.h>

#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/shell/shell.h>

static const struct device *const buttons = DEVICE_DT_GET(DT_ALIAS(buttons));
K_MSGQ_DEFINE(input_q, sizeof(struct input_event), 10, 1);
static bool initialized;

static void buttons_input_cb(struct input_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&input_q, evt, K_NO_WAIT);
}

INPUT_CALLBACK_DEFINE(buttons, buttons_input_cb, NULL);

static int cmd_buttons_check(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!initialized) {
		shell_error(sh, "Buttons module not initialized");
		return -EPERM;
	}

	ret = pm_device_runtime_get(buttons);
	if (ret < 0) {
		shell_error(sh, "Failed to get device (%d)", ret);
		return 0;
	}

	k_msgq_purge(&input_q);

	while (1) {
		int ret;
		struct input_event evt;

		ret = k_msgq_get(&input_q, &evt, K_SECONDS(5));
		if (ret == -EAGAIN) {
			shell_error(sh, "No input received");
			return 0;
		}

		switch (evt.code) {
		case INPUT_KEY_BACK:
			if (evt.value == 1) {
				shell_print(sh, "Back button pressed");
			} else {
				shell_print(sh, "Back button released");
			}
			break;
		case INPUT_KEY_UP:
			if (evt.value == 1) {
				shell_print(sh, "Up button pressed");
			} else {
				shell_print(sh, "Up button released");
			}
			break;
		case INPUT_KEY_ENTER:
			if (evt.value == 1) {
				shell_print(sh, "Enter button pressed");
			} else {
				shell_print(sh, "Enter button released");
			}
			break;
		case INPUT_KEY_DOWN:
			if (evt.value == 1) {
				shell_print(sh, "Down button pressed");
			} else {
				shell_print(sh, "Down button released");
			}
			break;
		}
	}

	ret = pm_device_runtime_put(buttons);
	if (ret < 0) {
		shell_error(sh, "Failed to put device (%d)", ret);
		return 0;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_buttons_cmds,
			       SHELL_CMD(check, NULL, "Check buttons", cmd_buttons_check),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), buttons, &sub_buttons_cmds, "Buttons", NULL, 0, 0);

int buttons_init(void)
{
	if (!device_is_ready(buttons)) {
		return -ENODEV;
	}

	initialized = true;

	return 0;
}
