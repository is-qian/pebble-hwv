#include "buttons.h"
#include "display.h"
#include "flash.h"
#include "haptic.h"
#include "light.h"
#include "press.h"

#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <app_version.h>

SHELL_SUBCMD_SET_CREATE(hwv_cmds, (hwv));
SHELL_CMD_REGISTER(hwv, &hwv_cmds, "HWV commands", NULL);

int main(void)
{
	int ret;

	printf("HWV v%s\n", APP_VERSION_STRING);

	ret = buttons_init();
	if (ret < 0) {
		printf("Failed to initialize buttons module (%d)\n", ret);
		return 0;
	}

	ret = display_init();
	if (ret < 0) {
		printf("Failed to initialize display module (%d)\n", ret);
		return 0;
	}

	ret = flash_init();
	if (ret < 0) {
		printf("Failed to initialize flash module (%d)\n", ret);
		return 0;
	}

	ret = haptic_init();
	if (ret < 0) {
		printf("Failed to initialize haptic module (%d)\n", ret);
		return 0;
	}

	ret = light_init();
	if (ret < 0) {
		printf("Failed to initialize light sensor module (%d)\n", ret);
		return 0;
	}

	ret = press_init();
	if (ret < 0) {
		printf("Failed to initialize pressure sensor module (%d)\n", ret);
		return 0;
	}

	return 0;
}
