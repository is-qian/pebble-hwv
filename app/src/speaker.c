#include "audio_data.h"

#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#define DA7212_CIF_CTRL              0x1D
#define DA7212_DIG_ROUTING_DAI       0x21
#define DA7212_SR                    0x22
#define DA7212_REFERENCES            0x23
#define DA7212_DAI_CLK_MODE          0x28
#define DA7212_DAI_CTRL              0x29
#define DA7212_DIG_ROUTING_DAC       0x2A
#define DA7212_DAC_R_GAIN            0x46
#define DA7212_LINE_GAIN             0x4A
#define DA7212_MIXOUT_R_SELECT       0x4C
#define DA7212_DAC_R_CTRL            0x6A
#define DA7212_LINE_CTRL             0x6D
#define DA7212_MIXOUT_R_CTRL         0x6F

#define DA7212_SYSTEM_ACTIVE         0xFD

#define TIMEOUT             1000
#define SAMPLE_BIT_WIDTH    16
#define INITIAL_BLOCKS      2
#define BLOCK_COUNT (INITIAL_BLOCKS + 2)

static const struct i2c_dt_spec codec = I2C_DT_SPEC_GET(DT_NODELABEL(da7212));
static const struct device *const i2s = DEVICE_DT_GET(DT_NODELABEL(i2s0));
K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);
static bool initialized;

static void codec_setup(void)
{
	// CIF_CTRL: soft reset
	i2c_reg_write_byte_dt(&codec, DA7212_CIF_CTRL, 0x80);

	k_msleep(10);

	// SYSTEM_ACTIVE: wake-up
	i2c_reg_write_byte_dt(&codec, DA7212_SYSTEM_ACTIVE, 0x01);

	// REFERENCES: enable master bias
	i2c_reg_write_byte_dt(&codec, DA7212_REFERENCES, 0x08);
	// SR: 44.1KHz
	i2c_reg_write_byte_dt(&codec, DA7212_SR, 0x0a);
	// DAI_CLK_MODE: slave, 32BCLK per WCLK
	i2c_reg_write_byte_dt(&codec, DA7212_DAI_CLK_MODE, 0x00);
	// DAI_CTRL: enable, 16-bit
	i2c_reg_write_byte_dt(&codec, DA7212_DAI_CTRL, 0x80);
	// DIG_ROUTING_DAI: DAI_R/L_SRC to DAI_R/L
	i2c_reg_write_byte_dt(&codec, DA7212_DIG_ROUTING_DAI, 0x32);
	// DIG_ROUTING_DAC: DAC_R/L mono mix of R/L
	i2c_reg_write_byte_dt(&codec, DA7212_DIG_ROUTING_DAC, 0xba);
	// DAC_R_GAIN: 0dB
	i2c_reg_write_byte_dt(&codec, DA7212_DAC_R_GAIN, 0x6f);
	// DAC_R_CTRL: enable
	i2c_reg_write_byte_dt(&codec, DA7212_DAC_R_CTRL, 0x80);
	// MIXOUT_R_SELECT: DAC_R
	i2c_reg_write_byte_dt(&codec, DA7212_MIXOUT_R_SELECT, 0x08);
	// MIXOUT_R_CTRL: enable, softmix enable, amp enable
	i2c_reg_write_byte_dt(&codec, DA7212_MIXOUT_R_CTRL, 0x98);
	// LINE_GAIN: 15dB
	i2c_reg_write_byte_dt(&codec, DA7212_LINE_GAIN, 0x3f);
	// LINE_CTRL: enable
	i2c_reg_write_byte_dt(&codec, DA7212_LINE_CTRL, 0x80);
}

static void codec_standby(void)
{
	i2c_reg_write_byte_dt(&codec, DA7212_SYSTEM_ACTIVE, 0x00);
}

static int i2s_setup(void)
{
	int ret;
	struct i2s_config config;

	config.word_size = SAMPLE_BIT_WIDTH;
	config.channels = NUMBER_OF_CHANNELS;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
	config.frame_clk_freq = SAMPLE_FREQUENCY;
	config.mem_slab = &mem_slab;
	config.block_size = BLOCK_SIZE;
	config.timeout = TIMEOUT;

	ret = i2s_configure(i2s, I2S_DIR_TX, &config);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int cmd_speaker_play(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!initialized) {
		shell_error(sh, "Speaker module not initialized");
		return -EPERM;
	}

	codec_setup();

	ret = i2s_setup();
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < 40; i++) {
		ret = i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_START);
		if (ret < 0) {
			return ret;
		}

		ret = i2s_write(i2s, audio_data, BLOCK_SIZE);
		if (ret < 0) {
			return ret;
		}

		k_msleep(100);

		ret = i2s_trigger(i2s, I2S_DIR_TX, I2S_TRIGGER_DROP);
		if (ret < 0) {
			return ret;
		}
	}

	codec_standby();

	shell_print(sh, "Speaker test done");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_speaker_cmds,
			       SHELL_CMD(play, NULL, "Play sound", cmd_speaker_play),
			       SHELL_SUBCMD_SET_END);

SHELL_SUBCMD_ADD((hwv), speaker, &sub_speaker_cmds, "Speaker", NULL, 0, 0);

int speaker_init(void)
{
	if (!i2c_is_ready_dt(&codec)) {
		return -ENODEV;
	}

	if (!device_is_ready(i2s)) {
		return -ENODEV;
	}

	initialized = true;

	return 0;
}
