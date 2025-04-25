#include "speaker.h"
#include "buttons.h"
#include "charger.h"
#include "display.h"
#include "flash.h"
#include "haptic.h"
#include "light.h"
#include "imu.h"
#include "mag.h"
#include "mic.h"
#include "press.h"

#include <stdio.h>

#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

#include <app_version.h>
#include <zephyr/kernel.h>

#define STACK_SIZE 1024 
#define PRIORITY 7  

K_THREAD_STACK_DEFINE(test_stack_area, STACK_SIZE); 
struct k_thread test_thread_data;                 

SHELL_SUBCMD_SET_CREATE(hwv_cmds, (hwv));
SHELL_CMD_REGISTER(hwv, &hwv_cmds, "HWV commands", NULL);

void test_loop(void)
{
	const struct shell *shell = shell_backend_uart_get_ptr();
	bool last_buttons_flag = buttons_flag;
	while (1) {
		if(buttons_flag)
		{
			if(buttons_flag != last_buttons_flag)
			{
				shell_execute_cmd(NULL, "hwv display on");
				shell_execute_cmd(NULL, "hwv display vpattern");
				shell_execute_cmd(NULL, "hwv display brightness 100");
				shell_execute_cmd(NULL, "hwv haptic configure 50");
				shell_execute_cmd(NULL, "hwv ble on");
				printf("buttons_flag = %d\n", buttons_flag);
				last_buttons_flag = buttons_flag;
			}

			shell_execute_cmd(shell, "hwv speaker play");
			shell_execute_cmd(shell, "hwv imu get");
			shell_execute_cmd(shell, "hwv light get");
			shell_execute_cmd(shell, "hwv mag get");
			shell_execute_cmd(shell, "hwv press get");
		}
		else
		{
			if(buttons_flag != last_buttons_flag)
			{
				shell_execute_cmd(NULL, "hwv display off");
				shell_execute_cmd(NULL, "hwv display brightness 0");
				shell_execute_cmd(NULL, "hwv haptic configure 0");
				shell_execute_cmd(NULL, "hwv ble off");
				printf("buttons_flag = %d\n", buttons_flag);
				last_buttons_flag = buttons_flag;
			}
		}
		k_sleep(K_MSEC(1000));
	}
}

int main(void)
{
	int ret;

	printf("HWV v%s\n", APP_VERSION_STRING);

	ret = buttons_init();
	if (ret < 0) {
		printf("Failed to initialize buttons module (%d)\n", ret);
	}

	ret = charger_init();
	if (ret < 0) {
		printf("Failed to initialize charger module (%d)\n", ret);
	}

	ret = display_init();
	if (ret < 0) {
		printf("Failed to initialize display module (%d)\n", ret);
	}

	ret = flash_init();
	if (ret < 0) {
		printf("Failed to initialize flash module (%d)\n", ret);
	}

	ret = haptic_init();
	if (ret < 0) {
		printf("Failed to initialize haptic module (%d)\n", ret);
	}

	ret = light_init();
	if (ret < 0) {
		printf("Failed to initialize light sensor module (%d)\n", ret);
	}

	ret = imu_init();
	if (ret < 0) {
		printf("Failed to initialize IMU module (%d)\n", ret);
	}

	ret = mag_init();
	if (ret < 0) {
		printf("Failed to initialize magnetometer module (%d)\n", ret);
	}

	ret = mic_init();
	if (ret < 0) {
		printf("Failed to initialize microphone module (%d)\n", ret);
	}

	ret = press_init();
	if (ret < 0) {
		printf("Failed to initialize pressure sensor module (%d)\n", ret);
	}

	ret = speaker_init();
	if (ret < 0) {
		printf("Failed to initialize speaker module (%d)\n", ret);
	}

    k_thread_create(&test_thread_data, test_stack_area, STACK_SIZE,
		(k_thread_entry_t)test_loop, NULL, NULL, NULL,
		PRIORITY, 0, K_NO_WAIT);

    const struct shell *shell = shell_backend_uart_get_ptr();
	shell_execute_cmd(shell, "hwv buttons check");
	
	return 0;
}
