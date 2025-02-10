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

	ret = press_init();
	if (ret < 0) {
		printf("Failed to initialize pressure sensor module (%d)\n", ret);
		return 0;
	}

	return 0;
}
