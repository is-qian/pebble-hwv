#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>

#define SAMPLE_RATE_HZ 16000
#define SAMPLE_BITS    16
#define TIMEOUT_MS     1000
#define CAPTURE_MS     200
#define BLOCK_SIZE     ((SAMPLE_BITS / BITS_PER_BYTE) * (SAMPLE_RATE_HZ * CAPTURE_MS) / 1000)
#define BLOCK_COUNT    4

static const struct device *const dmic = DEVICE_DT_GET(DT_ALIAS(dmic0));

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static struct pcm_stream_cfg stream = {
	.pcm_rate = SAMPLE_RATE_HZ,
	.pcm_width = SAMPLE_BITS,
	.block_size = BLOCK_SIZE,
	.mem_slab = &mem_slab,
};

static struct dmic_cfg cfg = {
	.io =
		{
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc = 40,
			.max_pdm_clk_dc = 60,
		},
	.streams = &stream,
	.channel =
		{
			.req_num_streams = 1,
			.req_num_chan = 1,
		},
};
#include <zephyr/drivers/flash.h>
#include <zephyr/pm/device.h>
static const struct device *const flash = DEVICE_DT_GET(DT_ALIAS(flash0));
static bool initialized;

static int cmd_mic_capture(const struct shell *sh, size_t argc, char **argv)
{
	int ret,time = 1;
	void *buffer;
	uint32_t size;
	struct flash_pages_info info;
	uint16_t addr=0;
	int16_t buf;
	
	if (argc > 1) {
		time = atoi(argv[1]);
	}
	if (!initialized) {
		shell_error(sh, "Microphone module not initialized");
		return -EPERM;
	}
	ret = pm_device_action_run(flash, PM_DEVICE_ACTION_RESUME);
	ret = flash_get_page_info_by_offs(flash, addr, &info);
	ret = flash_erase(flash, addr, info.size);
	ret = dmic_configure(dmic, &cfg);
	if (ret < 0) {
		shell_error(sh, "Failed to configure DMIC(%d)", ret);
		return ret;
	}

	ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
	if (ret < 0) {
		shell_error(sh, "START trigger failed (%d)", ret);
		return ret;
	}
	for (int i = 0; i < time; i++) {
		ret = dmic_read(dmic, 0, &buffer, &size, TIMEOUT_MS);
		if (ret < 0) {
			shell_error(sh, "DMIC read failed (%d)", ret);
			return ret;
		}
		ret = flash_write(flash, addr, buffer, size);
		addr += size;
		k_mem_slab_free(&mem_slab, buffer);
	}
	ret = dmic_trigger(dmic, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		shell_error(sh, "STOP trigger failed (%d)", ret);
		return ret;
	}
	shell_print(sh, "S");
	for (int i = 0; i < addr / 2; i++) {
		ret = flash_read(flash, i * 2 , &buf, 2);
		shell_print(sh, "%d", buf);
	}
	shell_print(sh, "E");
	(void)pm_device_action_run(flash, PM_DEVICE_ACTION_SUSPEND);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mic_cmds,
			       SHELL_CMD(capture, NULL, "Capture microphone data", cmd_mic_capture),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), mic, &sub_mic_cmds, "Microphone", NULL, 0, 0);

int mic_init(void)
{
	if (!device_is_ready(dmic)) {
		return -ENODEV;
	}

	cfg.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);

	initialized = true;

	return 0;
}
