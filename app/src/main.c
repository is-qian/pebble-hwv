#include <stdio.h>

#include <zephyr/shell/shell.h>

#include <app_version.h>

SHELL_SUBCMD_SET_CREATE(hwv_cmds, (hwv));
SHELL_CMD_REGISTER(hwv, &hwv_cmds, "HWV commands", NULL);

int main(void)
{
	printf("HWV v%s\n", APP_VERSION_STRING);

	return 0;
}
