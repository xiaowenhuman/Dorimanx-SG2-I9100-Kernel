/* linux/arch/arm/mach-exynos/mach-smdk4212.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/pwm_backlight.h>
#include <linux/input.h>
#include <linux/mmc/host.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/max8649.h>
#include <linux/regulator/fixed.h>
#include <linux/mfd/wm8994/pdata.h>
#ifdef CONFIG_LEDS_AAT1290A
#include <linux/leds-aat1290a.h>
#endif
#ifdef CONFIG_MFD_MAX77693
#include <linux/mfd/max77693.h>
#include <linux/mfd/max77693-private.h>
#include <linux/leds-max77693.h>
#endif
#ifdef CONFIG_FB_S5P_LD9040
#include <linux/lcd.h>
#include <linux/ld9040.h>
#endif
#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
#include <linux/battery/max17047_fuelgauge.h>
#endif
#if defined(CONFIG_BATTERY_SAMSUNG) || defined(CONFIG_BATTERY_SAMSUNG_S2PLUS)
#include <linux/power_supply.h>
#include <linux/battery/samsung_battery.h>
#endif
#if defined(CONFIG_CHARGER_MAX8922_U1) || defined(CONFIG_CHARGER_MAX8922_S2PLUS)
#include <linux/power/max8922_charger_u1.h>
#endif
#ifdef CONFIG_STMPE811_ADC
#include <linux/stmpe811-adc.h>
#endif
#include <linux/v4l2-mediabus.h>
#include <linux/memblock.h>
#include <linux/delay.h>
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/bootmem.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/keypad.h>
#include <plat/devs.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#include <plat/s3c64xx-spi.h>
#include <plat/tvout.h>
#include <plat/csis.h>
#include <plat/media.h>
#include <plat/adc.h>
#include <media/exynos_fimc_is.h>
#include <mach/exynos-ion.h>

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
#include <mach/tdmb_pdata.h>
#endif

#include <mach/map.h>
#include <mach/spi-clocks.h>

#include <mach/dev.h>
#include <mach/ppmu.h>

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
#include <plat/s5p-tmu.h>
#include <mach/regs-tmu.h>
#endif

#ifdef CONFIG_EXYNOS_C2C
#include <mach/c2c.h>
#endif
#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif

#include <linux/delay.h>
#ifdef CONFIG_FB_S5P_MIPI_DSIM
#include <mach/mipi_ddi.h>
#include <mach/dsim.h>
#include <plat/fb-s5p.h>
#endif
#include <../../../drivers/video/samsung/s3cfb.h>
#ifdef CONFIG_FB_S5P_EXTDSP
struct s3cfb_extdsp_lcd {
	int	width;
	int	height;
	int	bpp;
};
#endif
#include <mach/dev-sysmmu.h>

#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#include <plat/fimg2d.h>
#include <plat/sysmmu.h>

#include <mach/sec_debug.h>

#ifdef CONFIG_SND_SOC_WM8994
#include <linux/mfd/wm8994/pdata.h>
#include <linux/mfd/wm8994/gpio.h>
#endif

#include <mach/gpio-midas.h>
#include <mach/midas-power.h>
#ifdef CONFIG_SEC_THERMISTOR
#include <mach/sec_thermistor.h>
#endif
#include <mach/midas-thermistor.h>
#include <mach/midas-tsp.h>
#include <mach/regs-clock.h>

#ifdef CONFIG_FB_S5P_MDNIE
#include <linux/mdnie.h>
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif

#if defined(CONFIG_PHONE_IPC_SPI)
#include <linux/phone_svn/ipc_spi.h>
#include <linux/irq.h>
#endif

#include "board-mobile.h"

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4212_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDK4212_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDK4212_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg smdk4212_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
		.set_runstate	= set_gps_uart_op,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDK4212_UCON_DEFAULT,
		.ulcon		= SMDK4212_ULCON_DEFAULT,
		.ufcon		= SMDK4212_UFCON_DEFAULT,
	},
};

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI)

static struct s3c64xx_spi_csinfo spi1_csi[] = {
	[0] = {
		.line = EXYNOS4_GPB(5),
		.set_level = gpio_set_value,
		.fb_delay = 0x2,
	},
};

static struct spi_board_info spi1_board_info[] __initdata = {
	{
		.modalias = "s5c73m3_spi",
		.platform_data = NULL,
		.max_speed_hz = 50000000,
		.bus_num = 1,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi1_csi[0],
	}
};
#endif

#if defined(CONFIG_PHONE_IPC_SPI) \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
static struct s3c64xx_spi_csinfo spi2_csi[] = {
	[0] = {
		.line = EXYNOS4_GPC1(2),
		.set_level = gpio_set_value,
	},
};

static struct spi_board_info spi2_board_info[] __initdata = {
#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	{
		.modalias = "tdmbspi",
		.platform_data = NULL,
		.max_speed_hz = 5000000,
		.bus_num = 2,
		.chip_select = 0,
		.mode = SPI_MODE_0,
		.controller_data = &spi2_csi[0],
	},
#else
	{
		.modalias = "ipc_spi",
		.platform_data = NULL,
		.bus_num = 2,
		.chip_select = 0,
		.max_speed_hz = 12*1000*1000,
		.mode = SPI_MODE_1,
		.controller_data = &spi2_csi[0],
	}
#endif
};
#endif
#endif


#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
static void tdmb_set_config_poweron(void)
{
	s3c_gpio_cfgpin(GPIO_TDMB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_TDMB_INT, S3C_GPIO_SFN(GPIO_TDMB_INT_AF));
	s3c_gpio_setpull(GPIO_TDMB_INT, S3C_GPIO_PULL_NONE);
}
static void tdmb_set_config_poweroff(void)
{
	s3c_gpio_cfgpin(GPIO_TDMB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_TDMB_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_TDMB_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_TDMB_INT, GPIO_LEVEL_LOW);
}

static void tdmb_gpio_on(void)
{
	printk(KERN_DEBUG "tdmb_gpio_on\n");

	tdmb_set_config_poweron();

	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);
	usleep_range(1000, 1000);
	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_HIGH);
}

static void tdmb_gpio_off(void)
{
	printk(KERN_DEBUG "tdmb_gpio_off\n");

	tdmb_set_config_poweroff();

	gpio_set_value(GPIO_TDMB_EN, GPIO_LEVEL_LOW);
}

static struct tdmb_platform_data tdmb_pdata = {
	.gpio_on = tdmb_gpio_on,
	.gpio_off = tdmb_gpio_off,
};

static struct platform_device tdmb_device = {
	.name			= "tdmb",
	.id				= -1,
	.dev			= {
		.platform_data = &tdmb_pdata,
	},
};

static int __init tdmb_dev_init(void)
{
	tdmb_set_config_poweroff();
	s5p_register_gpio_interrupt(GPIO_TDMB_INT);
	tdmb_pdata.irq = GPIO_TDMB_IRQ;
	platform_device_register(&tdmb_device);

	return 0;
}
#endif

#if defined(CONFIG_PHONE_IPC_SPI)
static void ipc_spi_cfg_gpio(void);

static struct ipc_spi_platform_data ipc_spi_data = {
	.gpio_ipc_mrdy = GPIO_IPC_MRDY,
	.gpio_ipc_srdy = GPIO_IPC_SRDY,
	.gpio_ipc_sub_mrdy = GPIO_IPC_SUB_MRDY,
	.gpio_ipc_sub_srdy = GPIO_IPC_SUB_SRDY,

	.cfg_gpio = ipc_spi_cfg_gpio,
};

static struct resource ipc_spi_res[] = {
	[0] = {
		.start = IRQ_IPC_SRDY,
		.end = IRQ_IPC_SRDY,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device ipc_spi_device = {
	.name = "onedram",
	.id = -1,
	.num_resources = ARRAY_SIZE(ipc_spi_res),
	.resource = ipc_spi_res,
	.dev = {
		.platform_data = &ipc_spi_data,
	},
};

static void ipc_spi_cfg_gpio(void)
{
	int err = 0;

	unsigned gpio_ipc_mrdy = ipc_spi_data.gpio_ipc_mrdy;
	unsigned gpio_ipc_srdy = ipc_spi_data.gpio_ipc_srdy;
	unsigned gpio_ipc_sub_mrdy = ipc_spi_data.gpio_ipc_sub_mrdy;
	unsigned gpio_ipc_sub_srdy = ipc_spi_data.gpio_ipc_sub_srdy;

	err = gpio_request(gpio_ipc_mrdy, "IPC_MRDY");
	if (err) {
		printk(KERN_ERR "ipc_spi_cfg_gpio - fail to request gpio %s : %d\n",
			"IPC_MRDY", err);
	} else {
		gpio_direction_output(gpio_ipc_mrdy, 0);
		s3c_gpio_setpull(gpio_ipc_mrdy, S3C_GPIO_PULL_DOWN);
	}

	err = gpio_request(gpio_ipc_srdy, "IPC_SRDY");
	if (err) {
		printk(KERN_ERR "ipc_spi_cfg_gpio - fail to request gpio %s : %d\n",
			"IPC_SRDY", err);
	} else {
		gpio_direction_input(gpio_ipc_srdy);
		s3c_gpio_cfgpin(gpio_ipc_srdy, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(gpio_ipc_srdy, S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(gpio_ipc_sub_mrdy, "IPC_SUB_MRDY");
	if (err) {
		printk(KERN_ERR "ipc_spi_cfg_gpio - fail to request gpio %s : %d\n",
			"IPC_SUB_MRDY", err);
	} else {
		gpio_direction_output(gpio_ipc_sub_mrdy, 0);
		s3c_gpio_setpull(gpio_ipc_sub_mrdy, S3C_GPIO_PULL_DOWN);
	}

	err = gpio_request(gpio_ipc_sub_srdy, "IPC_SUB_SRDY");
	if (err) {
		printk(KERN_ERR "ipc_spi_cfg_gpio - fail to request gpio %s : %d\n",
			"IPC_SUB_SRDY", err);
	} else {
		gpio_direction_input(gpio_ipc_sub_srdy);
		s3c_gpio_cfgpin(gpio_ipc_sub_srdy, S3C_GPIO_SFN(0xF));
		s3c_gpio_setpull(gpio_ipc_sub_srdy, S3C_GPIO_PULL_NONE);
	}

	irq_set_irq_type(gpio_to_irq(GPIO_IPC_SRDY), IRQ_TYPE_EDGE_RISING);
	irq_set_irq_type(gpio_to_irq(GPIO_IPC_SUB_SRDY), IRQ_TYPE_EDGE_RISING);
}
#endif

#ifdef CONFIG_LEDS_AAT1290A
static int aat1290a_initGpio(void)
{
	int err;

	err = gpio_request(GPIO_CAM_SW_EN, "CAM_SW_EN");
	if (err) {
		printk(KERN_ERR "failed to request CAM_SW_EN\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_CAM_SW_EN, 1);
	gpio_set_value(GPIO_CAM_SW_EN, 1);

	return 0;
}

static void aat1290a_switch(int enable)
{
	gpio_set_value(GPIO_CAM_SW_EN, enable);
}

static int aat1290a_setGpio(void)
{
	int err;

	err = gpio_request(GPIO_TORCH_EN, "TORCH_EN");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_EN\n");
		return -EPERM;
	}
	gpio_direction_output(GPIO_TORCH_EN, 1);
	err = gpio_request(GPIO_TORCH_SET, "TORCH_SET");
	if (err) {
		printk(KERN_ERR "failed to request TORCH_SET\n");
		gpio_free(GPIO_TORCH_EN);
		return -EPERM;
	}
	gpio_direction_output(GPIO_TORCH_SET, 1);
	gpio_set_value(GPIO_TORCH_EN, 0);
	gpio_set_value(GPIO_TORCH_SET, 0);

	return 0;
}

static int aat1290a_freeGpio(void)
{
	gpio_free(GPIO_TORCH_EN);
	gpio_free(GPIO_TORCH_SET);

	return 0;
}

static void aat1290a_torch_en(int onoff)
{
	gpio_set_value(GPIO_TORCH_EN, onoff);
}

static void aat1290a_torch_set(int onoff)
{
	gpio_set_value(GPIO_TORCH_SET, onoff);
}

static struct aat1290a_led_platform_data aat1290a_led_data = {
	.brightness = TORCH_BRIGHTNESS_50,
	.status	= STATUS_UNAVAILABLE,
	.switch_sel = aat1290a_switch,
	.initGpio = aat1290a_initGpio,
	.setGpio = aat1290a_setGpio,
	.freeGpio = aat1290a_freeGpio,
	.torch_en = aat1290a_torch_en,
	.torch_set = aat1290a_torch_set,
};

static struct platform_device s3c_device_aat1290a_led = {
	.name	= "aat1290a-led",
	.id	= -1,
	.dev	= {
		.platform_data	= &aat1290a_led_data,
	},
};
#endif

unsigned int lcdtype;
static int __init lcdtype_setup(char *str)
{
	get_option(&str, &lcdtype);
	return 1;
}
__setup("lcdtype=", lcdtype_setup);

#ifdef CONFIG_FB_S5P
#ifdef CONFIG_FB_S5P_LD9040
static int lcd_cfg_gpio(void)
{
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data line (32bit) */
		s3c_gpio_cfgpin(EXYNOS4_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF0(i), S3C_GPIO_PULL_NONE);

	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* Drive strength to max */
	writel(0xffffffff, S5P_VA_GPIO + 0x18c);
	writel(0xffffffff, S5P_VA_GPIO + 0x1ac);
	writel(0xffffffff, S5P_VA_GPIO + 0x1cc);
	writel(readl(S5P_VA_GPIO + 0x1ec) | 0xffffff, S5P_VA_GPIO + 0x1ec);

	/* MLCD_RST */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(3), S3C_GPIO_PULL_NONE);

	/* LCD_SCLK */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(1), S3C_GPIO_PULL_NONE);

	/* LCD_SDI */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static int lcd_power_on(struct lcd_device *ld, int enable)
{
	struct regulator *regulator;

	if (ld == NULL) {
		printk(KERN_ERR "lcd device object is NULL.\n");
		return 0;
	}

	if (enable) {
		regulator = regulator_get(NULL, "vlcd_3.0v");
		if (IS_ERR(regulator))
			return 0;

		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vlcd_3.0v");

		if (IS_ERR(regulator))
			return 0;

		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);

		regulator_put(regulator);
	}

	return 1;
}

static int reset_lcd(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = EXYNOS4_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
		       "lcd reset control\n");
		return err;
	}

	gpio_request(reset_gpio, "MLCD_RST");

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);
	mdelay(10);
	gpio_direction_output(reset_gpio, 1);

	gpio_free(reset_gpio);

	return 1;
}

static int lcd_gpio_cfg_earlysuspend(struct lcd_device *ld)
{
	int reset_gpio = -1;
	int err;

	reset_gpio = EXYNOS4_GPY4(5);

	err = gpio_request(reset_gpio, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MLCD_RST for "
		       "lcd reset control\n");
		return err;
	}

	mdelay(10);
	gpio_direction_output(reset_gpio, 0);

	gpio_free(reset_gpio);

	return 0;
}

static int lcd_gpio_cfg_lateresume(struct lcd_device *ld)
{
	/* MLCD_RST */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(5), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(5), S3C_GPIO_PULL_NONE);

	/* LCD_nCS */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(3), S3C_GPIO_PULL_NONE);

	/* LCD_SCLK */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(1), S3C_GPIO_PULL_NONE);

	/* LCD_SDI */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(3), S3C_GPIO_PULL_NONE);

	return 0;
}

static struct s3cfb_lcd ld9040_info = {
	.width = 480,
	.height = 800,
	.p_width = 56,
	.p_height = 93,
	.bpp = 24,

	.freq = 60,
	.timing = {
		.h_fp = 16,
		.h_bp = 14,
		.h_sw = 2,
		.v_fp = 10,
		.v_fpe = 1,
		.v_bp = 4,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
};

extern struct ld9040_panel_data s2plus_panel_data;
static struct lcd_platform_data ld9040_platform_data = {
	.reset = reset_lcd,
	.power_on = lcd_power_on,
	.gpio_cfg_earlysuspend = lcd_gpio_cfg_earlysuspend,
	.gpio_cfg_lateresume = lcd_gpio_cfg_lateresume,
	/* it indicates whether lcd panel is enabled from u-boot. */
	.lcd_enabled = 0,
	.reset_delay = 20,	/* 20ms */
	.power_on_delay = 20,	/* 20ms */
	.power_off_delay = 200,	/* 200ms */
	.sleep_in_delay = 160,
	.pdata = &s2plus_panel_data,
};

#define LCD_BUS_NUM	3
#define DISPLAY_CS	EXYNOS4_GPY4(3)
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.max_speed_hz = 1200000,
		.bus_num = LCD_BUS_NUM,
		.chip_select = 0,
		.mode = SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

#define DISPLAY_CLK	EXYNOS4_GPY3(1)
#define DISPLAY_SI	EXYNOS4_GPY3(3)
static struct spi_gpio_platform_data lcd_spi_gpio_data = {
	.sck = DISPLAY_CLK,
	.mosi = DISPLAY_SI,
	.miso = SPI_GPIO_NO_MISO,
	.num_chipselect = 1,
};

static struct platform_device ld9040_spi_gpio = {
	.name = "spi_gpio",
	.id = LCD_BUS_NUM,
	.dev = {
		.parent = &s3c_device_fb.dev,
		.platform_data = &lcd_spi_gpio_data,
	},
};

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver = 0x70,
	.clk_name = "fimd",
	.nr_wins = 5,
#ifdef CONFIG_FB_S5P_DEFAULT_WINDOW
	.default_win = CONFIG_FB_S5P_DEFAULT_WINDOW,
#else
	.default_win = 0,
#endif
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,
	.lcd = &ld9040_info,
};

/* reading with 3-WIRE SPI with GPIO */
static inline void setcs(u8 is_on)
{
	gpio_set_value(DISPLAY_CS, is_on);
}

static inline void setsck(u8 is_on)
{
	gpio_set_value(DISPLAY_CLK, is_on);
}

static inline void setmosi(u8 is_on)
{
	gpio_set_value(DISPLAY_SI, is_on);
}

static inline unsigned int getmiso(void)
{
	return !!gpio_get_value(DISPLAY_SI);
}

static inline void setmosi2miso(u8 is_on)
{
	if (is_on)
		s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_INPUT);
	else
		s3c_gpio_cfgpin(DISPLAY_SI, S3C_GPIO_OUTPUT);
}

struct spi_ops ops = {
	.setcs = setcs,
	.setsck = setsck,
	.setmosi = setmosi,
	.setmosi2miso = setmosi2miso,
	.getmiso = getmiso,
};

static void __init ld9040_fb_init(void)
{
	strcpy(spi_board_info[0].modalias, "ld9040");
	spi_board_info[0].platform_data =
	(void *)&ld9040_platform_data;

	pdata = ld9040_platform_data.pdata;
	pdata->ops = &ops;

	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));

	lcd_cfg_gpio();
	s3cfb_set_platdata(&fb_platform_data);
}
#endif

#ifdef CONFIG_FB_S5P_LMS501KF03
static struct s3c_platform_fb lms501kf03_data __initdata = {
	.hw_ver = 0x70,
	.clk_name = "sclk_lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S5P_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,
};

#define		LCD_BUS_NUM	3
#define		DISPLAY_CS	EXYNOS4_GPB(5)
#define		DISPLAY_CLK	EXYNOS4_GPB(4)
#define		DISPLAY_SI	EXYNOS4_GPB(7)

static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "lms501kf03",
		.platform_data	= NULL,
		.max_speed_hz	= 1200000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	}
};

static struct spi_gpio_platform_data lms501kf03_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect = 1,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &lms501kf03_spi_gpio_data,
	},
};
#endif
#ifdef CONFIG_FB_S5P_MIPI_DSIM
#ifdef CONFIG_FB_S5P_S6E8AA0
/* for Geminus based on MIPI-DSI interface */
static struct s3cfb_lcd s6e8aa0 = {
	.name = "s6e8aa0",
	.width = 720,
	.height = 1280,
	.p_width = 60,		/* 59.76 mm */
	.p_height = 106,	/* 106.24 mm */
	.bpp = 24,

	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 10,
		.h_bp = 10,
		.h_sw = 5,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 11,	/*v_fp=stable_vfp + cmd_allow_len */
		.stable_vfp = 2,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

#ifdef CONFIG_FB_S5P_S6E39A0
static struct s3cfb_lcd s6e39a0 = {
	.name = "s6e8aa0",
	.width = 540,
	.height = 960,
	.p_width = 58,
	.p_height = 103,
	.bpp = 24,

	.freq = 60,

	/* minumun value is 0 except for wr_act time. */
	.cpu_timing = {
		.cs_setup = 0,
		.wr_setup = 0,
		.wr_act = 1,
		.wr_hold = 0,
	},

	.timing = {
		.h_fp = 0x48,
		.h_bp = 12,
		.h_sw = 4,
		.v_fp = 13,
		.v_fpe = 1,
		.v_bp = 1,
		.v_bpe = 1,
		.v_sw = 2,
		.cmd_allow_len = 0x4,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 0,
	},
};
#endif

static struct s3c_platform_fb fb_platform_data __initdata = {
	.hw_ver		= 0x70,
	.clk_name	= "fimd",
	.nr_wins	= 5,
#ifdef CONFIG_FB_S5P_DEFAULT_WINDOW
	.default_win	= CONFIG_FB_S5P_DEFAULT_WINDOW,
#else
	.default_win	= 0,
#endif
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,
#ifdef CONFIG_FB_S5P_S6E8AA0
	.lcd		= &s6e8aa0
#endif
#ifdef CONFIG_FB_S5P_S6E39A0
	.lcd		= &s6e39a0
#endif
};

#ifdef CONFIG_FB_S5P_S6E8AA0
static int reset_lcd(void)
{
	int err;

	/* Set GPY4[5] OUTPUT HIGH */
	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request GPY4(5) for "
		       "lcd reset control\n");
		return -EINVAL;
	}

	gpio_direction_output(GPIO_MLCD_RST, 1);
	usleep_range(5000, 5000);
	gpio_set_value(GPIO_MLCD_RST, 0);
	usleep_range(5000, 5000);
	gpio_set_value(GPIO_MLCD_RST, 1);
	usleep_range(5000, 5000);
	gpio_free(GPIO_MLCD_RST);
	return 0;
}
#endif

static void lcd_cfg_gpio(void)
{
#if !defined(CONFIG_FB_S5P_MIPI_DSIM)
	int i, f3_end = 4;

	for (i = 0; i < 8; i++) {
		/* set GPF0,1,2[0:7] for RGB Interface and Data lines (32bit) */
		s3c_gpio_cfgpin(EXYNOS4_GPF0(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF0(i), S3C_GPIO_PULL_NONE);

	}
	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF1(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF1(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < 8; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF2(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF2(i), S3C_GPIO_PULL_NONE);
	}

	for (i = 0; i < f3_end; i++) {
		s3c_gpio_cfgpin(EXYNOS4_GPF3(i), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(EXYNOS4_GPF3(i), S3C_GPIO_PULL_NONE);
	}

	/* drive strength to max */
	writel(0xffffffff, S5P_VA_GPIO + 0x18c);
	writel(0xffffffff, S5P_VA_GPIO + 0x1ac);
	writel(0xffffffff, S5P_VA_GPIO + 0x1cc);
	writel(readl(S5P_VA_GPIO + 0x1ec) | 0xffffff, S5P_VA_GPIO + 0x1ec);
#endif
	/* MLCD_RST */
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);

#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	/* LCD_EN */
	s3c_gpio_cfgpin(GPIO_LCD_22V_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_22V_EN, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_LCD_22V_EN_00, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_LCD_22V_EN_00, S3C_GPIO_PULL_NONE);
#endif

#if 0
	/* LCD_nCS */
	s3c_gpio_cfgpin(EXYNOS4_GPY4(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY4(3), S3C_GPIO_PULL_NONE);
	/* LCD_SCLK */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(1), S3C_GPIO_PULL_NONE);
	/* LCD_SDI */
	s3c_gpio_cfgpin(EXYNOS4_GPY3(3), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(EXYNOS4_GPY3(3), S3C_GPIO_PULL_NONE);
#endif

	return;
}

static int lcd_power_on(void *ld, int enable)
{
	struct regulator *regulator;
	int err;

	printk(KERN_INFO "%s : enable=%d\n", __func__, enable);

	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request GPY4[5] for "
		       "MLCD_RST control\n");
		return -EPERM;
	}
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	err = gpio_request(GPIO_LCD_22V_EN, "LCD_EN");
	if (err) {
		printk(KERN_ERR "failed to request GPM4[4] for "
				"LCD_2.2V_EN control\n");
		return -EPERM;
	}
	err = gpio_request(GPIO_LCD_22V_EN_00, "LCD_EN");
	if (err) {
		printk(KERN_ERR "failed to request GPM4[4] for "
				"LCD_2.2V_EN control\n");
		return -EPERM;
	}
#endif

	if (enable) {
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
		gpio_set_value(GPIO_LCD_22V_EN, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_LCD_22V_EN_00, GPIO_LEVEL_HIGH);

#else
		regulator = regulator_get(NULL, "vlcd_2.2v");
		if (IS_ERR(regulator))
			goto out;
		regulator_enable(regulator);
		regulator_put(regulator);
#endif
		regulator = regulator_get(NULL, "vlcd_3.3v");
		if (IS_ERR(regulator))
			goto out;
		regulator_enable(regulator);
		regulator_put(regulator);
	} else {
		regulator = regulator_get(NULL, "vlcd_3.3v");
		if (IS_ERR(regulator))
			goto out;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
		gpio_set_value(GPIO_LCD_22V_EN, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_LCD_22V_EN_00, GPIO_LEVEL_LOW);
#else
		regulator = regulator_get(NULL, "vlcd_2.2v");
		if (IS_ERR(regulator))
			goto out;
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
		regulator_put(regulator);
#endif
		gpio_set_value(GPIO_MLCD_RST, 0);
	}

out:
	/* Release GPIO */
	gpio_free(GPIO_MLCD_RST);
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	gpio_free(GPIO_LCD_22V_EN);
	gpio_free(GPIO_LCD_22V_EN_00);
#endif
	return 0;
}

static void s5p_dsim_mipi_power_control(int enable)
{
	struct regulator *regulator;
	int power_en = 0;
#if defined(CONFIG_MACH_M0)
	if (system_rev != 0 && system_rev != 3)
		power_en = 1;
#elif defined(CONFIG_MACH_C1)
	if (system_rev == 2 || system_rev >= 4)
		power_en = 1;
#elif defined(CONFIG_MACH_C1VZW)
	if (system_rev != 0)
		power_en = 1;
#else
		power_en = 0;
#endif

	if (power_en == 1) {
		printk(KERN_INFO "%s : enable=%d\n", __func__, enable);

		if (enable) {
			regulator = regulator_get(NULL, "vmipi_1.0v");
			if (IS_ERR(regulator))
				goto out;
			regulator_enable(regulator);
			regulator_put(regulator);

			regulator = regulator_get(NULL, "vmipi_1.8v");
			if (IS_ERR(regulator))
				goto out;
			regulator_enable(regulator);
			regulator_put(regulator);
		} else {
			regulator = regulator_get(NULL, "vmipi_1.8v");
			if (IS_ERR(regulator))
				goto out;
			if (regulator_is_enabled(regulator))
				regulator_force_disable(regulator);
			regulator_put(regulator);

			regulator = regulator_get(NULL, "vmipi_1.0v");
			if (IS_ERR(regulator))
				goto out;
			if (regulator_is_enabled(regulator))
				regulator_force_disable(regulator);
			regulator_put(regulator);
		}
out:
	return ;
	} else {
		return ;
	}
}

static void __init mipi_fb_init(void)
{
	struct s5p_platform_dsim *dsim_pd = NULL;
	struct mipi_ddi_platform_data *mipi_ddi_pd = NULL;
	struct dsim_lcd_config *dsim_lcd_info = NULL;

	/* set platform data */

	/* gpio pad configuration for rgb and spi interface. */
	lcd_cfg_gpio();

	/*
	 * register lcd panel data.
	 */
	printk(KERN_INFO "%s :: fb_platform_data.hw_ver = 0x%x\n",
	       __func__, fb_platform_data.hw_ver);

	fb_platform_data.mipi_is_enabled = 1;
	fb_platform_data.interface_mode = FIMD_CPU_INTERFACE;

	dsim_pd = (struct s5p_platform_dsim *)
		s5p_device_dsim.dev.platform_data;

	dsim_pd->platform_rev = 1;
	dsim_pd->mipi_power = s5p_dsim_mipi_power_control;

	dsim_lcd_info = dsim_pd->dsim_lcd_info;

#ifdef CONFIG_FB_S5P_S6E8AA0
	dsim_lcd_info->lcd_panel_info = (void *)&s6e8aa0;

	/* 500Mbps */
	dsim_pd->dsim_info->p = 3;
	dsim_pd->dsim_info->m = 125;
	dsim_pd->dsim_info->s = 1;
#endif

	mipi_ddi_pd = (struct mipi_ddi_platform_data *)
		dsim_lcd_info->mipi_ddi_pd;
	mipi_ddi_pd->lcd_reset = reset_lcd;
	mipi_ddi_pd->lcd_power_on = lcd_power_on;

	platform_device_register(&s5p_device_dsim);

	s3cfb_set_platdata(&fb_platform_data);
}
#endif
#ifdef CONFIG_FB_S5P_MDNIE
static struct platform_device mdnie_device = {
	.name		= "mdnie",
	.id		= -1,
	.dev	    = {
		.parent = &exynos4_device_pd[PD_LCD0].dev,
	},
};
#endif
#endif

static DEFINE_MUTEX(notify_lock);

#define DEFINE_MMC_CARD_NOTIFIER(num) \
static void (*hsmmc##num##_notify_func)(struct platform_device *, int state); \
static int ext_cd_init_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func); \
	hsmmc##num##_notify_func = notify_func; \
	mutex_unlock(&notify_lock); \
	return 0; \
} \
static int ext_cd_cleanup_hsmmc##num(void (*notify_func)( \
			struct platform_device *, int state)) \
{ \
	mutex_lock(&notify_lock); \
	WARN_ON(hsmmc##num##_notify_func != notify_func); \
	hsmmc##num##_notify_func = NULL; \
	mutex_unlock(&notify_lock); \
	return 0; \
}

#ifdef CONFIG_S3C_DEV_HSMMC3
	DEFINE_MMC_CARD_NOTIFIER(3)
#endif

/*
 * call this when you need sd stack to recognize insertion or removal of card
 * that can't be told by SDHCI regs
 */
void sdhci_s3c_force_presence_change(struct platform_device *pdev)
{
	void (*notify_func)(struct platform_device *, int state) = NULL;
	mutex_lock(&notify_lock);
#ifdef CONFIG_S3C_DEV_HSMMC3
	if (pdev == &s3c_device_hsmmc3)
		notify_func = hsmmc3_notify_func;
#endif

	if (notify_func)
		notify_func(pdev, 1);
	else
		pr_warn("%s: called for device with no notifier\n", __func__);
	mutex_unlock(&notify_lock);
}
EXPORT_SYMBOL_GPL(sdhci_s3c_force_presence_change);

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata smdk4212_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata smdk4212_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata smdk4212_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_GPIO,
	.ext_cd_gpio		= EXYNOS4_GPX3(4),
	.ext_cd_gpio_invert	= true,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.vmmc_name		= "vtf_2.8v"
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata smdk4212_hsmmc3_pdata __initdata = {
/* new code for brm4334 */
	.cd_type		= S3C_SDHCI_CD_EXTERNAL,

	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
	.pm_flags = S3C_SDHCI_PM_IGNORE_SUSPEND_RESUME,
	.ext_cd_init = ext_cd_init_hsmmc3,
	.ext_cd_cleanup = ext_cd_cleanup_hsmmc3,
};
#endif

#ifdef CONFIG_EXYNOS4_DEV_MSHC
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.fifo_depth		= 0x80,
#if defined(CONFIG_EXYNOS4_MSHC_8BIT) && \
	defined(CONFIG_EXYNOS4_MSHC_DDR)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
				  MMC_CAP_UHS_DDR50 | MMC_CAP_CMD23,
	.host_caps2		= MMC_CAP2_PACKED_CMD | MMC_CAP2_CACHE_CTRL,
#elif defined(CONFIG_EXYNOS4_MSHC_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_CMD23,
#elif defined(CONFIG_EXYNOS4_MSHC_DDR)
	.host_caps		= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50 |
				  MMC_CAP_CMD23,
#endif
	.int_power_gpio		= GPIO_eMMC_EN,
};
#endif

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata smdk4212_ehci_pdata;

static void __init smdk4212_ehci_init(void)
{
	struct s5p_ehci_platdata *pdata = &smdk4212_ehci_pdata;

	s5p_ehci_set_platdata(pdata);
}
#endif

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata smdk4212_ohci_pdata;

static void __init smdk4212_ohci_init(void)
{
	struct s5p_ohci_platdata *pdata = &smdk4212_ohci_pdata;

	s5p_ohci_set_platdata(pdata);
}
#endif

/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata smdk4212_usbgadget_pdata;

#include <linux/usb/android_composite.h>
static void __init smdk4212_usbgadget_init(void)
{
	struct s5p_usbgadget_platdata *pdata = &smdk4212_usbgadget_pdata;
	struct android_usb_platform_data *android_pdata =
		s3c_device_android_usb.dev.platform_data;
	if (android_pdata) {
		unsigned int newluns = 2;
		printk(KERN_DEBUG "usb: %s: default luns=%d, new luns=%d\n",
				__func__, android_pdata->nluns, newluns);
		android_pdata->nluns = newluns;
	} else {
		printk(KERN_DEBUG "usb: %s android_pdata is not available\n",
				__func__);
	}

	s5p_usbgadget_set_platdata(pdata);
}
#endif

#ifdef CONFIG_MFD_MAX77693
#ifdef CONFIG_VIBETONZ
static struct max77693_haptic_platform_data max77693_haptic_pdata = {
	.max_timeout = 10000,
	.duty = 37641,
	.period = 38022,
	.reg2 = MOTOR_LRA | EXT_PWM | DIVIDER_128,
	.init_hw = NULL,
	.motor_en = NULL,
	.pwm_id = 0,
	.regulator_name = "vmotor",
};
#endif

#ifdef CONFIG_LEDS_MAX77693
static struct max77693_led_platform_data max77693_led_pdata = {
	.num_leds = 4,

	.leds[0].name = "leds-sec1",
	.leds[0].id = MAX77693_FLASH_LED_1,
	.leds[0].timer = MAX77693_FLASH_TIME_500MS,
	.leds[0].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[0].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[0].brightness = 0x1F,

	.leds[1].name = "leds-sec2",
	.leds[1].id = MAX77693_FLASH_LED_2,
	.leds[1].timer = MAX77693_FLASH_TIME_500MS,
	.leds[1].timer_mode = MAX77693_TIMER_MODE_MAX_TIMER,
	.leds[1].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[1].brightness = 0x1F,

	.leds[2].name = "torch-sec1",
	.leds[2].id = MAX77693_TORCH_LED_1,
	.leds[2].cntrl_mode = MAX77693_LED_CTRL_BY_FLASHSTB,
	.leds[2].brightness = 0x0F,

	.leds[3].name = "torch-sec2",
	.leds[3].id = MAX77693_TORCH_LED_2,
	.leds[3].cntrl_mode = MAX77693_LED_CTRL_BY_I2C,
	.leds[3].brightness = 0x0F,
};

#endif

#ifdef CONFIG_BATTERY_MAX77693_CHARGER
static struct max77693_charger_platform_data max77693_charger_pdata = {
#ifdef CONFIG_BATTERY_WPC_CHARGER
	.wpc_irq_gpio = GPIO_WPC_INT,
#endif
};
#endif

extern struct max77693_muic_data max77693_muic;
extern struct max77693_regulator_data max77693_regulators;

static bool is_muic_default_uart_path_cp(void)
{
#ifdef CONFIG_MACH_M0
	if (system_rev == 5)
		return true;
#endif
#ifdef CONFIG_MACH_C1
	if (system_rev == 4)
		return true;
#endif
#ifdef CONFIG_MACH_S2PLUS
	if (system_rev >= 2)
		return true;
#endif
	return false;
}

struct max77693_platform_data exynos4_max77693_info = {
	.irq_base	= IRQ_BOARD_IFIC_START,
	.irq_gpio	= GPIO_IF_PMIC_IRQ,
	.wakeup		= 1,
	.muic = &max77693_muic,
	.is_default_uart_path_cp =  is_muic_default_uart_path_cp,
	.regulators = &max77693_regulators,
	.num_regulators = MAX77693_REG_MAX,
#ifdef CONFIG_VIBETONZ
	.haptic_data = &max77693_haptic_pdata,
#endif
#ifdef CONFIG_LEDS_MAX77693
	.led_data = &max77693_led_pdata,
#endif
#ifdef CONFIG_BATTERY_MAX77693_CHARGER
	.charger_data = &max77693_charger_pdata,
#endif
};
#endif

#if defined(CONFIG_CHARGER_MAX8922_U1) || defined(CONFIG_CHARGER_MAX8922_S2PLUS)
static int max8922_cfg_gpio(void)
{
	printk(KERN_INFO "[Battery] %s called.\n", __func__);

	s3c_gpio_cfgpin(GPIO_CHG_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CHG_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CHG_EN, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_CHG_ING_N, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_CHG_ING_N, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_TA_nCONNECTED, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_TA_nCONNECTED, S3C_GPIO_PULL_NONE);

	return 0;
}

static struct max8922_platform_data max8922_pdata = {
	.cfg_gpio = max8922_cfg_gpio,
	.gpio_chg_en = GPIO_CHG_EN,
	.gpio_chg_ing = GPIO_CHG_ING_N,
	.gpio_ta_nconnected = GPIO_TA_nCONNECTED,
};

static struct platform_device max8922_device_charger = {
	.name = "max8922-charger",
	.id = -1,
	.dev.platform_data = &max8922_pdata,
};
#endif /* CONFIG_CHARGER_MAX8922_U1 || CONFIG_CHARGER_MAX8922_S2PLUS */

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
};


#ifdef CONFIG_SND_SOC_WM8994
/* vbatt_devices */
static struct regulator_consumer_supply vbatt_supplies[] = {
	REGULATOR_SUPPLY("LDO1VDD", NULL),
	REGULATOR_SUPPLY("SPKVDD1", NULL),
	REGULATOR_SUPPLY("SPKVDD2", NULL),
};

static struct regulator_init_data vbatt_initdata = {
	.constraints = {
		.always_on = 1,
	},
	.num_consumer_supplies = ARRAY_SIZE(vbatt_supplies),
	.consumer_supplies = vbatt_supplies,
};

static struct fixed_voltage_config vbatt_config = {
	.init_data = &vbatt_initdata,
	.microvolts = 5000000,
	.supply_name = "VBATT",
	.gpio = -EINVAL,
};

static struct platform_device vbatt_device = {
	.name = "reg-fixed-voltage",
	.id = -1,
	.dev = {
		.platform_data = &vbatt_config,
	},
};

/* wm1811 ldo1 */
static struct regulator_consumer_supply wm1811_ldo1_supplies[] = {
	REGULATOR_SUPPLY("AVDD1", NULL),
};

static struct regulator_init_data wm1811_ldo1_initdata = {
	.constraints = {
		.name = "WM1811 LDO1",
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo1_supplies),
	.consumer_supplies = wm1811_ldo1_supplies,
};

/* wm1811 ldo2 */
static struct regulator_consumer_supply wm1811_ldo2_supplies[] = {
	REGULATOR_SUPPLY("DCVDD", NULL),
};

static struct regulator_init_data wm1811_ldo2_initdata = {
	.constraints = {
		.name = "WM1811 LDO2",
		.always_on = true, /* Actually status changed by LDO1 */
	},
	.num_consumer_supplies = ARRAY_SIZE(wm1811_ldo2_supplies),
	.consumer_supplies = wm1811_ldo2_supplies,
};

static struct wm8994_pdata wm1811_pdata = {
	.gpio_defaults = {
		[0] = WM8994_GP_FN_IRQ,	  /* GPIO1 IRQ output, CMOS mode */
		[7] = WM8994_GPN_DIR | WM8994_GP_FN_PIN_SPECIFIC, /* DACDAT3 */
		[8] = WM8994_CONFIGURE_GPIO |
		      WM8994_GP_FN_PIN_SPECIFIC, /* ADCDAT3 */
		[9] = WM8994_CONFIGURE_GPIO |\
		      WM8994_GP_FN_PIN_SPECIFIC, /* LRCLK3 */
		[10] = WM8994_CONFIGURE_GPIO |\
		       WM8994_GP_FN_PIN_SPECIFIC, /* BCLK3 */
	},

	.irq_base = IRQ_BOARD_CODEC_START,

	/* The enable is shared but assign it to LDO1 for software */
	.ldo = {
		{
			.enable = GPIO_WM8994_LDO,
			.init_data = &wm1811_ldo1_initdata,
		},
		{
			.init_data = &wm1811_ldo2_initdata,
		},
	},

	/* Regulated mode at highest output voltage */
	.micbias = {0x2f, 0x2f},

	.micd_lvl_sel = 0xFF,

	.ldo_ena_always_driven = true,
	.ldo_ena_delay = 50000,

	.lineout1fb = 1,
	.lineout2fb = 1,
};
#endif

#ifdef CONFIG_S3C_DEV_I2C4
#ifdef CONFIG_MFD_MAX77693
static struct i2c_board_info i2c_devs4_max77693[] __initdata = {
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data	= &exynos4_max77693_info,
	}
};
#endif
#if defined(CONFIG_SND_SOC_WM8994)
static struct i2c_board_info i2c_devs4_wm1811[] __initdata = {
	{
		I2C_BOARD_INFO("wm1811", (0x34 >> 1)),	/* Audio CODEC */
		.platform_data = &wm1811_pdata,
		.irq = IRQ_EINT(30),
	},
};
#endif
#endif

#ifdef CONFIG_S3C_DEV_I2C5
static struct i2c_board_info i2c_devs5[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8997
	{
		I2C_BOARD_INFO("max8997", (0xcc >> 1)),
		.platform_data = &exynos4_max8997_info,
	},
#endif
#if defined(CONFIG_REGULATOR_MAX77686)
	/* max77686 on i2c5 other than M1 board */
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	},
#endif
};
#endif

static struct i2c_board_info i2c_devs7[] __initdata = {
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
#if defined(CONFIG_REGULATOR_MAX77686) /* max77686 on i2c7 with M1 board */
	{
		I2C_BOARD_INFO("max77686", (0x12 >> 1)),
		.platform_data	= &exynos4_max77686_info,
	},
#endif

#if defined(CONFIG_REGULATOR_S5M8767)
	{
		I2C_BOARD_INFO("s5m87xx", 0xCC >> 1),
		.platform_data = &exynos4_s5m8767_info,
		.irq	= IRQ_EINT(7),
	},
#endif
#endif
};

/* Bluetooth */
#ifdef CONFIG_BT_BCM4334
static struct platform_device bcm4334_bluetooth_device = {
	.name = "bcm4334_bluetooth",
	.id = -1,
};
#endif

static struct i2c_gpio_platform_data gpio_i2c_data8 = {
	.sda_pin = GPIO_3_TOUCH_SDA,
	.scl_pin = GPIO_3_TOUCH_SCL,
};

struct platform_device s3c_device_i2c8 = {
	.name = "i2c-gpio",
	.id = 8,
	.dev.platform_data = &gpio_i2c_data8,
};

/* I2C8 */
static struct i2c_board_info i2c_devs8_emul[] __initdata = {
	{
#if defined(CONFIG_MACH_S2PLUS)
		I2C_BOARD_INFO("sec_touchkey", 0x20),
#else
		I2C_BOARD_INFO("melfas-touchkey", 0x20),
#endif
	},
};

/* I2C9 */
static struct i2c_board_info i2c_devs9_emul[] __initdata = {
};

/* I2C10 */
static struct i2c_board_info i2c_devs10_emul[] __initdata = {
};

/* I2C11 */
static struct i2c_board_info i2c_devs11_emul[] __initdata = {
};

/* I2C12 */
#if defined(CONFIG_PN65N_NFC) && !defined(CONFIG_MACH_C1) \
	&& !defined(CONFIG_MACH_C1VZW) && !defined(CONFIG_MACH_M3) \
	&& !defined(CONFIG_MACH_C1CTC)
static struct i2c_board_info i2c_devs12_emul[] __initdata = {
};
#endif

/* I2C13 */
static struct i2c_board_info i2c_devs13_emul[] __initdata = {
};

#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
static struct i2c_gpio_platform_data gpio_i2c_data14 = {
	.sda_pin = GPIO_FUEL_SDA,
	.scl_pin = GPIO_FUEL_SCL,
};

struct platform_device s3c_device_i2c14 = {
	.name = "i2c-gpio",
	.id = 14,
	.dev.platform_data = &gpio_i2c_data14,
};

static struct max17047_platform_data max17047_pdata = {
	.irq_gpio = GPIO_FUEL_ALERT,
};

/* I2C14 */
static struct i2c_board_info i2c_devs14_emul[] __initdata = {
	{
		I2C_BOARD_INFO("max17047-fuelgauge", 0x36),
		.platform_data = &max17047_pdata,
	},
};
#endif

/* I2C15 */
static struct i2c_gpio_platform_data gpio_i2c_data15 = {
	.sda_pin = GPIO_MHL_SDA_1_8V,
	.scl_pin = GPIO_MHL_SCL_1_8V,
	.udelay = 3,
	.timeout = 0,
};

struct platform_device s3c_device_i2c15 = {
	.name = "i2c-gpio",
	.id = 15,
	.dev = {
		.platform_data = &gpio_i2c_data15,
	}
};

static struct i2c_board_info i2c_devs15_emul[] __initdata = {
};

/* I2C16 */
static struct i2c_gpio_platform_data gpio_i2c_data16 = {
	.sda_pin = GPIO_MHL_DSDA_2_8V,
	.scl_pin = GPIO_MHL_DSCL_2_8V,
};

struct platform_device s3c_device_i2c16 = {
	.name = "i2c-gpio",
	.id = 16,
	.dev.platform_data = &gpio_i2c_data16,
};

static struct i2c_board_info i2c_devs16_emul[] __initdata = {
};

#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
static struct i2c_gpio_platform_data gpio_i2c_data17 = {
	.sda_pin = GPIO_IF_PMIC_SDA,
	.scl_pin = GPIO_IF_PMIC_SCL,
};

struct platform_device s3c_device_i2c17 = {
	.name = "i2c-gpio",
	.id = 17,
	.dev.platform_data = &gpio_i2c_data17,
};

/* I2C17 */
static struct i2c_board_info i2c_devs17_emul[] __initdata = {
#ifdef CONFIG_MFD_MAX77693
	{
		I2C_BOARD_INFO("max77693", (0xCC >> 1)),
		.platform_data	= &exynos4_max77693_info,
	}
#endif
};
#endif

#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
static struct i2c_gpio_platform_data i2c18_platdata = {
	.sda_pin		= GPIO_8M_CAM_SDA_18V,
	.scl_pin		= GPIO_8M_CAM_SCL_18V,
	.udelay			= 2, /* 250 kHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c18 = {
	.name	= "i2c-gpio",
	.id	= 18,
	.dev.platform_data	= &i2c18_platdata,
};

/* I2C18 */
/* No explicit i2c client array here. The channel number 18 is passed
   to camera driver from midas-camera.c instead. */
#endif

#if defined(CONFIG_STMPE811_ADC) || defined(CONFIG_FM_SI4709_MODULE) \
	|| defined(CONFIG_FM_SI4705_MODULE)
static struct i2c_gpio_platform_data gpio_i2c_data19 = {
	.sda_pin = GPIO_ADC_SDA,
	.scl_pin = GPIO_ADC_SCL,
};

struct platform_device s3c_device_i2c19 = {
	.name = "i2c-gpio",
	.id = 19,
	.dev.platform_data = &gpio_i2c_data19,
};


/* I2C19 */
static struct i2c_board_info i2c_devs19_emul[] __initdata = {
#if defined(CONFIG_STMPE811_ADC)
	{
		I2C_BOARD_INFO("stmpe811-adc", (0x82 >> 1)),
		.platform_data	= &stmpe811_pdata,
	},
#endif
#ifdef CONFIG_FM_SI4705_MODULE
	{
		I2C_BOARD_INFO("Si4709", (0x22 >> 1)),
	},
#endif
#ifdef CONFIG_FM_SI4709_MODULE
	{
		I2C_BOARD_INFO("Si4709", (0x20 >> 1)),
	},
#endif

};
#endif

/* I2C21 */
#ifdef CONFIG_LEDS_AN30259A
static struct i2c_gpio_platform_data gpio_i2c_data21 = {
	.scl_pin = GPIO_S_LED_I2C_SCL,
	.sda_pin = GPIO_S_LED_I2C_SDA,
};

struct platform_device s3c_device_i2c21 = {
	.name = "i2c-gpio",
	.id = 21,
	.dev.platform_data = &gpio_i2c_data21,
};
#endif

/* I2C21 */
static struct i2c_board_info i2c_devs21_emul[] __initdata = {
#ifdef CONFIG_LEDS_AN30259A
	{
		I2C_BOARD_INFO("an30259a", 0x30),
	},
#endif
};

#ifdef CONFIG_ANDROID_RAM_CONSOLE
static struct resource ram_console_resource[] = {
	{
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
};

static int __init setup_ram_console_mem(char *str)
{
	unsigned size = memparse(str, &str);

	if (size && (*str == '@')) {
		unsigned long long base = 0;

		base = simple_strtoul(++str, &str, 0);
		if (reserve_bootmem(base, size, BOOTMEM_EXCLUSIVE)) {
			pr_err("%s: failed reserving size %d "
			       "at base 0x%llx\n", __func__, size, base);
			return -1;
		}

		ram_console_resource[0].start = base;
		ram_console_resource[0].end = base + size - 1;
		pr_err("%s: %x at %llx\n", __func__, size, base);
	}
	return 0;
}

__setup("ram_console=", setup_ram_console_mem);
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.no_allocator	= 1,
	.cached		= 0,
	.start		= 0,
	.size		= 0
};

static struct android_pmem_platform_data pmem_cam_pdata = {
	.name		= "pmem_cam",
	.no_allocator	= 1,
	.cached		= 1,
	.start		= 0,
	.size		= 0,
};

static struct platform_device pmem_device = {
	.name	= "android_pmem",
	.id	= 0,
	.dev	= {
		.platform_data = &pmem_pdata
	},
};

static struct platform_device pmem_cam_device = {
	.name	= "android_pmem",
	.id	= 2,
	.dev	= {
		.platform_data = &pmem_cam_pdata
	},
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_CMA)
	pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
	pmem_cam_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_CAM * SZ_1K;
#endif
}
#endif

#if defined(CONFIG_BATTERY_SAMSUNG) || defined(CONFIG_BATTERY_SAMSUNG_S2PLUS)
static struct samsung_battery_platform_data samsung_battery_pdata = {
	.charger_name	= "max77693-charger",
	.fuelgauge_name	= "max17047-fuelgauge",
#if defined(CONFIG_CHARGER_MAX8922_U1) || defined(CONFIG_CHARGER_MAX8922_S2PLUS)
	.sub_charger_name   = "max8922-charger",
#if defined(CONFIG_CHARGER_MAX8922_S2PLUS)
	.use_sub_charger = true,
#endif
#endif
	.voltage_max = 4350,
	.voltage_min = 3400,

	.chng_interval = 30,
	.chng_susp_interval = 60,
	.norm_interval = 120,
	.norm_susp_interval = 600,
	.emer_interval = 10,

	.recharge_voltage = 4300,	/* it will be cacaluated in probe */
	.abstimer_charge_duration = 6 * 60 * 60,
	.abstimer_recharge_duration = 1.5 * 60 * 60,

	.overheat_stop_temp = 650,
	.overheat_recovery_temp = 430,
	.freeze_stop_temp = -50,
	.freeze_recovery_temp = 0,

	.temper_src = TEMPER_AP_ADC,
	.temper_ch = 2,
#ifdef CONFIG_S3C_ADC
	/* s3c adc driver does not convert raw adc data.
	 * so, register convert function.
	 */
	.covert_adc = convert_adc,
#endif

	.suspend_chging = true,

	.led_indicator = true,
};

static struct platform_device samsung_device_battery = {
	.name	= "samsung-battery",
	.id	= -1,
	.dev.platform_data = &samsung_battery_pdata,
};
#endif

#define GPIO_KEYS(_code, _gpio, _active_low, _iswake, _hook)	\
	{							\
		.code = _code,					\
		.gpio = _gpio,					\
		.active_low = _active_low,			\
		.type = EV_KEY,					\
		.wakeup = _iswake,				\
		.debounce_interval = 10,			\
		.isr_hook = _hook,				\
		.value = 1					\
	}

struct gpio_keys_button midas_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN,
		  1, 0, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
};

struct gpio_keys_platform_data midas_gpiokeys_platform_data = {
	midas_buttons,
	ARRAY_SIZE(midas_buttons),
};

static struct platform_device midas_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &midas_gpiokeys_platform_data,
	},
};

#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
struct gpio_keys_button m0_buttons[] = {
	GPIO_KEYS(KEY_VOLUMEUP, GPIO_VOL_UP_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_VOLUMEDOWN, GPIO_VOL_DOWN_00,
		  1, 1, sec_debug_check_crash_key),
	GPIO_KEYS(KEY_POWER, GPIO_nPOWER,
		  1, 1, sec_debug_check_crash_key),
#if defined(CONFIG_MACH_S2PLUS)
	GPIO_KEYS(KEY_HOME, GPIO_OK_KEY,
		  1, 1, sec_debug_check_crash_key),
#endif
};

struct gpio_keys_platform_data m0_gpiokeys_platform_data = {
	m0_buttons,
	ARRAY_SIZE(m0_buttons),
};

static struct platform_device m0_keypad = {
	.name	= "gpio-keys",
	.dev	= {
		.platform_data = &m0_gpiokeys_platform_data,
	},
};
#endif

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
	.hw_ver = 0x41,
	.parent_clkname = "mout_g2d0",
	.clkname = "sclk_fimg2d",
	.gate_clkname = "fimg2d",
	.clkrate = 199 * 1000000,	/* 160 Mhz */
};
#endif

#ifdef CONFIG_EXYNOS_C2C
struct exynos_c2c_platdata smdk4212_c2c_pdata = {
	.setup_gpio	= NULL,
	.shdmem_addr	= C2C_SHAREDMEM_BASE,
	.shdmem_size	= C2C_MEMSIZE_64,
	.ap_sscm_addr	= NULL,
	.cp_sscm_addr	= NULL,
	.rx_width	= C2C_BUSWIDTH_16,
	.tx_width	= C2C_BUSWIDTH_16,
	.clk_opp100	= 400,
	.clk_opp50	= 266,
	.clk_opp25	= 0,
	.default_opp_mode	= C2C_OPP50,
	.get_c2c_state	= NULL,
};
#endif
/* BUSFREQ to control memory/bus */
static struct device_domain busfreq;

static struct platform_device exynos4_busfreq = {
	.id = -1,
	.name = "exynos-busfreq",
};

#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651) || \
	defined(CONFIG_SENSORS_CM3663)
static struct i2c_gpio_platform_data i2c9_platdata = {
#if defined(CONFIG_MACH_S2PLUS)
	.sda_pin	= GPIO_PS_ALS_SDA_18V,
	.scl_pin	= GPIO_PS_ALS_SCL_18V,
#else
	.sda_pin	= GPIO_PS_ALS_SDA_28V,
	.scl_pin	= GPIO_PS_ALS_SCL_28V,
#endif
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c9 = {
	.name	= "i2c-gpio",
	.id	= 9,
	.dev.platform_data	= &i2c9_platdata,
};
#endif

#if (defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651))\
	&& (defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1) ||\
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_C1VZW)) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
static struct i2c_gpio_platform_data i2c9_platdata_rgb = {
	.sda_pin	= GPIO_RGB_SDA_1_8V,
	.scl_pin	= GPIO_RGB_SCL_1_8V,
	.udelay = 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c9_rgb = {
	.name	= "i2c-gpio",
	.id = 9,
	.dev.platform_data	= &i2c9_platdata_rgb,
};
#endif

#ifdef CONFIG_SENSORS_AK8975C
static struct i2c_gpio_platform_data i2c10_platdata = {
	.sda_pin	= GPIO_MSENSOR_SDA_18V,
	.scl_pin	= GPIO_MSENSOR_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c10 = {
	.name	= "i2c-gpio",
	.id	= 10,
	.dev.platform_data	= &i2c10_platdata,
};
#endif

#ifdef CONFIG_SENSORS_LPS331
static struct i2c_gpio_platform_data i2c11_platdata = {
	.sda_pin	= GPIO_BSENSE_SDA_18V,
	.scl_pin	= GPIO_BENSE_SCL_18V,
	.udelay	= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c11 = {
	.name			= "i2c-gpio",
	.id	= 11,
	.dev.platform_data	= &i2c11_platdata,
};
#endif

#if defined(CONFIG_PN65N_NFC) && !defined(CONFIG_MACH_C1) \
	&& !defined(CONFIG_MACH_C1VZW) && !defined(CONFIG_MACH_M3) \
	&& !defined(CONFIG_MACH_C1CTC)
static struct i2c_gpio_platform_data i2c12_platdata = {
	.sda_pin		= GPIO_NFC_SDA_18V,
	.scl_pin		= GPIO_NFC_SCL_18V,
	.udelay			= 2, /* 250 kHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only = 0,
};

static struct platform_device s3c_device_i2c12 = {
	.name	= "i2c-gpio",
	.id		= 12,
	.dev.platform_data	= &i2c12_platdata,
};
#endif

#ifdef CONFIG_USB_HOST_NOTIFY
static void otg_accessory_power(int enable)
{
	u8 on = (u8)!!enable;

	/* max77693 otg power control */
	otg_control(enable);

	gpio_request(GPIO_OTG_EN, "USB_OTG_EN");
	gpio_direction_output(GPIO_OTG_EN, on);
	gpio_free(GPIO_OTG_EN);
	pr_info("%s: otg accessory power = %d\n", __func__, on);
}

static struct host_notifier_platform_data host_notifier_pdata = {
	.ndev.name	= "usb_otg",
	.booster	= otg_accessory_power,
	.thread_enable	= 0,
};

struct platform_device host_notifier_device = {
	.name = "host_notifier",
	.dev.platform_data = &host_notifier_pdata,
};
#endif

static struct platform_device *midas_devices[] __initdata = {
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_cam_device,
#endif
	/* Samsung Power Domain */
	&exynos4_device_pd[PD_MFC],
	&exynos4_device_pd[PD_G3D],
	&exynos4_device_pd[PD_LCD0],
	&exynos4_device_pd[PD_CAM],
	&exynos4_device_pd[PD_TV],
	&exynos4_device_pd[PD_GPS],
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_pd[PD_ISP],
#endif
	&exynos4_device_pd[PD_GPS_ALIVE],
	/* legacy fimd */
#ifdef CONFIG_FB_S5P
	&s3c_device_fb,
#ifdef CONFIG_FB_S5P_LMS501KF03
	&s3c_device_spi_gpio,
#endif
#ifdef CONFIG_FB_S5P_LD9040
	&ld9040_spi_gpio,
#endif
#endif

#ifdef CONFIG_FB_S5P_MDNIE
	&mdnie_device,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif

#ifdef CONFIG_SND_SOC_WM8994
	&vbatt_device,
#endif

	&s3c_device_wdt,
	&s3c_device_rtc,

	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c3,
#ifdef CONFIG_S3C_DEV_I2C4
	&s3c_device_i2c4,
#endif
	/* &s3c_device_i2c5, */
	&s3c_device_i2c7,
	&s3c_device_i2c8,
	/* &s3c_device_i2c9, */
#ifdef CONFIG_SENSORS_AK8975C
	&s3c_device_i2c10,
#endif
#ifdef CONFIG_SENSORS_LPS331
	&s3c_device_i2c11,
#endif
	/* &s3c_device_i2c12, */
	/* &s3c_device_i2c13, */
#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
	&s3c_device_i2c14,	/* max17047-fuelgauge */
#endif

#ifdef CONFIG_SAMSUNG_MHL
	&s3c_device_i2c15,
	&s3c_device_i2c16,
#endif
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	&s3c_device_i2c17,
	&s3c_device_i2c18,
#endif
#ifdef CONFIG_LEDS_AN30259A
	&s3c_device_i2c21,
#endif
#if defined CONFIG_USB_EHCI_S5P && !defined CONFIG_LINK_DEVICE_HSIC
	&s5p_device_ehci,
#endif
#if defined CONFIG_USB_OHCI_S5P && !defined CONFIG_LINK_DEVICE_HSIC
	&s5p_device_ohci,
#endif
#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#if defined(CONFIG_USB_ANDROID) || defined(CONFIG_USB_G_ANDROID)
	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
#endif
#ifdef CONFIG_EXYNOS4_DEV_MSHC
	&s3c_device_mshci,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif

#ifdef CONFIG_SND_SAMSUNG_AC97
	&exynos_device_ac97,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
	&exynos_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
	&exynos_device_pcm0,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
	&exynos_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
	&exynos_device_srp,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&exynos4_device_fimc_is,
#endif
#ifdef CONFIG_FB_S5P_LD9040
	&ld9040_spi_gpio,
#endif
#ifdef CONFIG_VIDEO_TVOUT
	&s5p_device_tvout,
	&s5p_device_cec,
	&s5p_device_hpd,
#endif
#ifdef CONFIG_FB_S5P_EXTDSP
	&s3c_device_extdsp,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_fimc3,
/* CONFIG_VIDEO_SAMSUNG_S5P_FIMC is the feature for mainline */
#elif defined(CONFIG_VIDEO_SAMSUNG_S5P_FIMC)
	&s5p_device_fimc0,
	&s5p_device_fimc1,
	&s5p_device_fimc2,
	&s5p_device_fimc3,
#endif
#if defined(CONFIG_VIDEO_FIMC_MIPI)
	&s3c_device_csis0,
	&s3c_device_csis1,
#endif
#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	&s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
	&SYSMMU_PLATDEV(g2d_acp),
	&SYSMMU_PLATDEV(fimc0),
	&SYSMMU_PLATDEV(fimc1),
	&SYSMMU_PLATDEV(fimc2),
	&SYSMMU_PLATDEV(fimc3),
	&SYSMMU_PLATDEV(jpeg),
	&SYSMMU_PLATDEV(mfc_l),
	&SYSMMU_PLATDEV(mfc_r),
	&SYSMMU_PLATDEV(tv),
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	&SYSMMU_PLATDEV(is_isp),
	&SYSMMU_PLATDEV(is_drc),
	&SYSMMU_PLATDEV(is_fd),
	&SYSMMU_PLATDEV(is_cpu),
#endif
#endif
#ifdef CONFIG_ION_EXYNOS
	&exynos_device_ion,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_LITE
	&exynos_device_flite0,
	&exynos_device_flite1,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	&s5p_device_fimg2d,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
	&s5p_device_jpeg,
#endif
	&samsung_asoc_dma,
#ifndef CONFIG_SND_SOC_SAMSUNG_USE_DMA_WRAPPER
	&samsung_asoc_idma,
#endif
#if defined(CONFIG_CHARGER_MAX8922_U1) || defined(CONFIG_CHARGER_MAX8922_S2PLUS)
	&max8922_device_charger,
#endif
#if defined(CONFIG_BATTERY_SAMSUNG) || defined(CONFIG_BATTERY_SAMSUNG_S2PLUS)
	&samsung_device_battery,
#endif
#ifdef CONFIG_EXYNOS_C2C
	&exynos_device_c2c,
#endif
#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI)
	&exynos_device_spi1,
#endif
#if defined(CONFIG_PHONE_IPC_SPI)
	&exynos_device_spi2,
	&ipc_spi_device,
#elif defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	&exynos_device_spi2,
#endif
#endif

#ifdef CONFIG_BT_BCM4334
	&bcm4334_bluetooth_device,
#endif
#ifdef CONFIG_S5P_DEV_ACE
	&s5p_device_ace,
#endif
	&exynos4_busfreq,
#ifdef CONFIG_USB_HOST_NOTIFY
	&host_notifier_device,
#endif
#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	&s5p_device_tmu,
#endif
};

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
/* below temperature base on the celcius degree */
struct s5p_platform_tmu midas_tmu_data __initdata = {
	.ts = {
		.stop_1st_throttle  = 78,
		.start_1st_throttle = 80,
		.stop_2nd_throttle  = 87,
		.start_2nd_throttle = 103,
		.start_tripping	    = 110, /* temp to do tripping */
		.start_emergency    = 120, /* To protect chip,forcely kernel panic */
		.stop_mem_throttle  = 80,
		.start_mem_throttle = 85,
	},
	.cpufreq = {
		.limit_1st_throttle  = 800000, /* 800MHz in KHz order */
		.limit_2nd_throttle  = 200000, /* 200MHz in KHz order */
	},
};
#endif

#if defined CONFIG_USB_OHCI_S5P && defined CONFIG_LINK_DEVICE_HSIC
static int __init s5p_ohci_device_initcall(void)
{
	return platform_device_register(&s5p_device_ohci);
}
late_initcall(s5p_ohci_device_initcall);
#endif
#if defined CONFIG_USB_EHCI_S5P && defined CONFIG_LINK_DEVICE_HSIC
static int __init s5p_ehci_device_initcall(void)
{
	return platform_device_register(&s5p_device_ehci);
}
late_initcall(s5p_ehci_device_initcall);
#endif

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#if defined(CONFIG_CMA)
static void __init exynos4_reserve_mem(void)
{
	static struct cma_region regions[] = {
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		{
			.name = "fimc_is",
			.size = CONFIG_VIDEO_EXYNOS_MEMSIZE_FIMC_IS * SZ_1K,
			{
				.alignment = 1 << 26,
			},
			.start = 0
		},
#endif
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM
		{
			.name = "pmem",
			.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
		{
			.name = "fimd",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		{
			.name = "fimc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
		{
			.name = "fimc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0)
		{
			.name = "mfc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
		{
			.name = "mfc",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0
		},
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
	defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
		{
			.name		= "b2",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "b1",
			.size		= 32 << 20,
			{ .alignment	= 128 << 10 },
		},
		{
			.name		= "fw",
			.size		= 1 << 20,
			{ .alignment	= 128 << 10 },
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
		{
			.name = "jpeg",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
		{
			.name = "srp",
			.size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
		{
			.name = "fimg2d",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_CAM
		{
			.name = "pmem_cam",
			.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_CAM * SZ_1K,
			.start = 0,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
		{
			.name = "fimc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
			.start = 0
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
		{
			.name = "mfc1",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
			{
				.alignment = 1 << 17,
			},
			.start = 0,
		},
#endif
		{
			.size = 0
		},
	};
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
	static struct cma_region regions_secure[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
		{
			.name = "fimc2",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
		},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0
		{
			.name = "mfc0",
			.size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
		},
#endif
		{
			.name = "sectbl",
			.size = SZ_1M,
		},
		{
			.size = 0
		},
	};
#else /* !CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION */
	struct cma_region *regions_secure = NULL;
#endif

	static const char map[] __initconst =
#ifdef CONFIG_EXYNOS_C2C
		"samsung-c2c=c2c_shdmem;"
#endif
		"android_pmem.0=pmem;android_pmem.2=pmem_cam;"
		"s3cfb.0=fimd;exynos4-fb.0=fimd;"
		"s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
		"exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_ION_EXYNOS
		"ion-exynos=fimc2;"
#endif
#ifdef CONFIG_VIDEO_MFC5X
		"s3c-mfc=mfc,mfc0,mfc1;"
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_S5P_MFC
		"s5p-mfc/f=fw;"
		"s5p-mfc/a=b1;"
		"s5p-mfc/b=b2;"
#endif
		"samsung-rp=srp;"
		"s5p-jpeg=jpeg;"
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
		"exynos4-fimc-is=fimc_is;"
#endif
		"s5p-fimg2d=fimg2d;"
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
		"s5p-smem/video=video;"
		"s5p-smem/sectbl=sectbl;"
#endif
		"s5p-smem/mfc=mfc0;"
		"s5p-smem/fimc=fimc2;"
		"s5p-smem/mfc-shm=mfc1;";

		s5p_cma_region_reserve(regions, regions_secure, SZ_64M, map);
}
#else
static inline void exynos4_reserve_mem(void)
{
}
#endif

#ifdef CONFIG_BACKLIGHT_PWM
/* LCD Backlight data */
static struct samsung_bl_gpio_info smdk4212_bl_gpio_info = {
	.no = EXYNOS4_GPD0(1),
	.func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data smdk4212_bl_data = {
	.pwm_id = 1,
#ifdef CONFIG_FB_S5P_LMS501KF03
	.pwm_period_ns = 1000,
#endif
};
#endif

static void __init midas_map_io(void)
{
	clk_xusbxti.rate = 24000000;
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdk4212_uartcfgs, ARRAY_SIZE(smdk4212_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
	exynos4_reserve_mem();
#endif

	/* as soon as INFORM6 is visible, sec_debug is ready to run */
	sec_debug_init();
}

static void __init exynos_sysmmu_init(void)
{
	ASSIGN_SYSMMU_POWERDOMAIN(fimc0, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc1, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc2, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(fimc3, &exynos4_device_pd[PD_CAM].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(jpeg, &exynos4_device_pd[PD_CAM].dev);

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC) || defined(CONFIG_VIDEO_MFC5X)
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
#endif
	ASSIGN_SYSMMU_POWERDOMAIN(tv, &exynos4_device_pd[PD_TV].dev);
#ifdef CONFIG_VIDEO_FIMG2D
	sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#ifdef CONFIG_VIDEO_MFC5X
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
#ifdef CONFIG_VIDEO_FIMC
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s3c_device_fimc0.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s3c_device_fimc1.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s3c_device_fimc2.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s3c_device_fimc3.dev);
#endif
#ifdef CONFIG_VIDEO_TVOUT
	sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_tvout.dev);
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
	sysmmu_set_owner(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	ASSIGN_SYSMMU_POWERDOMAIN(is_isp, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_drc, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_fd, &exynos4_device_pd[PD_ISP].dev);
	ASSIGN_SYSMMU_POWERDOMAIN(is_cpu, &exynos4_device_pd[PD_ISP].dev);

	sysmmu_set_owner(&SYSMMU_PLATDEV(is_isp).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_drc).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_fd).dev,
		&exynos4_device_fimc_is.dev);
	sysmmu_set_owner(&SYSMMU_PLATDEV(is_cpu).dev,
		&exynos4_device_fimc_is.dev);
#endif
}

#ifdef CONFIG_FB_S5P_EXTDSP
struct platform_device s3c_device_extdsp = {
	.name		= "s3cfb_extdsp",
	.id		= 0,
};

static struct s3cfb_extdsp_lcd dummy_buffer = {
	.width = 1920,
	.height = 1080,
	.bpp = 16,
};

static struct s3c_platform_fb default_extdsp_data __initdata = {
	.hw_ver		= 0x70,
	.nr_wins	= 1,
	.default_win	= 0,
	.swap		= FB_SWAP_WORD | FB_SWAP_HWORD,
	.lcd		= &dummy_buffer
};

void __init s3cfb_extdsp_set_platdata(struct s3c_platform_fb *pd)
{
	struct s3c_platform_fb *npd;
	int i;

	if (!pd)
		pd = &default_extdsp_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fb), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		for (i = 0; i < npd->nr_wins; i++)
			npd->nr_buffers[i] = 1;
		s3c_device_extdsp.dev.platform_data = npd;
	}
}
#endif

static inline int need_i2c5(void)
{
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC) || defined(CONFIG_MACH_S2PLUS)
	return system_rev != 3;
#elif defined(CONFIG_MACH_JENGA)
	return 0;
#else
	return 1;
#endif
}

#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651) || \
	defined(CONFIG_SENSORS_CM3663)
static inline int need_rgb(void)
{
#if defined(CONFIG_MACH_C1) ||	\
	defined(CONFIG_MACH_JENGA) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	return (system_rev != 3 && system_rev != 0);
#elif defined(CONFIG_MACH_C1VZW)
	return 1;
#else
	return 0;
#endif
}
#endif

static void __init midas_machine_init(void)
{
	struct clk *ppmu_clk = NULL;

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI)
	unsigned int gpio;
	struct clk *sclk = NULL;
	struct clk *prnt = NULL;
	struct device *spi1_dev = &exynos_device_spi1.dev;
#endif
#if defined(CONFIG_PHONE_IPC_SPI) \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	struct device *spi2_dev = &exynos_device_spi2.dev;
#endif
#endif

	/*
	  * prevent 4x12 ISP power off problem
	  * ISP_SYS Register has to be 0 before ISP block power off.
	  */
	__raw_writel(0x0, S5P_CMU_RESET_ISP_SYS);

	/* initialise the gpios */
	midas_config_gpio_table();
	exynos4_sleep_gpio_table_set = midas_config_sleep_gpio_table;

	midas_power_init();

	s3c_i2c0_set_platdata(NULL);
	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

	s3c_i2c1_set_platdata(NULL);
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

	s3c_i2c3_set_platdata(NULL);
	midas_tsp_init();

#ifdef CONFIG_LEDS_AAT1290A
	platform_device_register(&s3c_device_aat1290a_led);
#endif

#ifdef CONFIG_S3C_DEV_I2C4
#ifdef CONFIG_MACH_MIDAS_02_BD
	s3c_i2c4_set_platdata(NULL);
	i2c_register_board_info(4, i2c_devs4_max77693,
		ARRAY_SIZE(i2c_devs4_max77693));
#else
	s3c_i2c4_set_platdata(NULL);
	if (system_rev != 3 && system_rev >= 0) {
		i2c_register_board_info(4, i2c_devs4_wm1811,
			ARRAY_SIZE(i2c_devs4_wm1811));
	} else {
		i2c_register_board_info(4, i2c_devs4_max77693,
			ARRAY_SIZE(i2c_devs4_max77693));
	}
#endif
#endif

#ifdef CONFIG_S3C_DEV_I2C5
	if (need_i2c5()) {
		s3c_i2c5_set_platdata(NULL);
		i2c_register_board_info(5, i2c_devs5,
			ARRAY_SIZE(i2c_devs5));
	}
#endif

	s3c_i2c7_set_platdata(NULL);
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

	i2c_register_board_info(8, i2c_devs8_emul, ARRAY_SIZE(i2c_devs8_emul));
#ifndef CONFIG_LEDS_AAT1290A
	gpio_request(GPIO_3_TOUCH_INT, "3_TOUCH_INT");
	s5p_register_gpio_interrupt(GPIO_3_TOUCH_INT);
#endif

	i2c_register_board_info(9, i2c_devs9_emul, ARRAY_SIZE(i2c_devs9_emul));

	i2c_register_board_info(10, i2c_devs10_emul,
				ARRAY_SIZE(i2c_devs10_emul));

	i2c_register_board_info(11, i2c_devs11_emul,
				ARRAY_SIZE(i2c_devs11_emul));

#if defined(CONFIG_PN65N_NFC) && !defined(CONFIG_MACH_C1) \
	&& !defined(CONFIG_MACH_C1VZW) && !defined(CONFIG_MACH_M3) \
	&& !defined(CONFIG_MACH_C1CTC)
	i2c_register_board_info(12, i2c_devs12_emul,
				ARRAY_SIZE(i2c_devs12_emul));
#endif

#ifdef CONFIG_BATTERY_MAX17047_FUELGAUGE
	/* max17047 fuel gauge */
	i2c_register_board_info(14, i2c_devs14_emul,
				ARRAY_SIZE(i2c_devs14_emul));
#endif
#ifdef CONFIG_SAMSUNG_MHL
	printk(KERN_INFO "%s() register sii9234 driver\n", __func__);

	i2c_register_board_info(15, i2c_devs15_emul,
				ARRAY_SIZE(i2c_devs15_emul));
	i2c_register_board_info(16, i2c_devs16_emul,
				ARRAY_SIZE(i2c_devs16_emul));
#endif
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	i2c_register_board_info(17, i2c_devs17_emul,
				ARRAY_SIZE(i2c_devs17_emul));
#endif
#if defined(CONFIG_STMPE811_ADC) || defined(CONFIG_FM_SI4709_MODULE) \
	|| defined(CONFIG_FM_SI4705_MODULE)
	i2c_register_board_info(19, i2c_devs19_emul,
				ARRAY_SIZE(i2c_devs19_emul));
#endif

#ifdef CONFIG_LEDS_AN30259A
	i2c_register_board_info(21, i2c_devs21_emul,
				ARRAY_SIZE(i2c_devs21_emul));
#endif

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif

#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	gpio_request(GPIO_OLED_DET, "OLED_DET");
	s5p_register_gpio_interrupt(GPIO_OLED_DET);
	gpio_free(GPIO_OLED_DET);
#endif
#ifdef CONFIG_FB_S5P
#ifdef CONFIG_FB_S5P_LMS501KF03
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&lms501kf03_data);
#else
	s3cfb_set_platdata(NULL);
#endif
#ifdef CONFIG_EXYNOS_DEV_PD
	s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_USB_EHCI_S5P
	smdk4212_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
	smdk4212_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
	smdk4212_usbgadget_init();
#endif

#if defined(CONFIG_FB_S5P_MIPI_DSIM)
	mipi_fb_init();
#elif defined(CONFIG_FB_S5P_LD9040)
	ld9040_fb_init();
#else
	samsung_bl_set(&smdk4212_bl_gpio_info, &smdk4212_bl_data);
#endif

#ifdef CONFIG_VIDEO_EXYNOS_FIMC_IS
	exynos4_fimc_is_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
	exynos4_device_fimc_is.dev.parent = &exynos4_device_pd[PD_ISP].dev;
#endif
#endif
#ifdef CONFIG_EXYNOS4_DEV_MSHC
	s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
	s3c_sdhci0_set_platdata(&smdk4212_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s3c_sdhci1_set_platdata(&smdk4212_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s3c_sdhci2_set_platdata(&smdk4212_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s3c_sdhci3_set_platdata(&smdk4212_hsmmc3_pdata);
#endif

	midas_camera_init();

#ifdef CONFIG_FB_S5P_EXTDSP
	s3cfb_extdsp_set_platdata(&default_extdsp_data);
#endif

#if defined(CONFIG_VIDEO_TVOUT)
	s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
	s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
	exynos4_device_pd[PD_TV].dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_ION_EXYNOS
	exynos_ion_set_platdata();
#endif

#if defined(CONFIG_VIDEO_MFC5X) || defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
#ifdef CONFIG_EXYNOS_DEV_PD
	s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
	exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 267 * MHZ);
#endif

#if defined(CONFIG_VIDEO_SAMSUNG_S5P_MFC)
	dev_set_name(&s5p_device_mfc.dev, "s3c-mfc");
	clk_add_alias("mfc", "s5p-mfc", "mfc", &s5p_device_mfc.dev);
	s5p_mfc_setname(&s5p_device_mfc, "s5p-mfc");
#endif
#ifdef CONFIG_VIDEO_FIMG2D
	s5p_fimg2d_set_platdata(&fimg2d_data);
#endif
#ifdef CONFIG_EXYNOS_C2C
	exynos_c2c_set_platdata(&smdk4212_c2c_pdata);
#endif

	brcm_wlan_init();

#ifdef CONFIG_EXYNOS4_SETUP_THERMAL
	s5p_tmu_set_platdata(&midas_tmu_data);
#endif

	exynos_sysmmu_init();

	platform_add_devices(midas_devices, ARRAY_SIZE(midas_devices));

#ifdef CONFIG_S3C_ADC
#if defined(CONFIG_MACH_S2PLUS)
	platform_device_register(&s3c_device_adc);
#else
	if (system_rev != 3)
		platform_device_register(&s3c_device_adc);
#endif
#endif
#if defined(CONFIG_STMPE811_ADC) || defined(CONFIG_FM_SI4709_MODULE) \
	|| defined(CONFIG_FM_SI4705_MODULE)
	platform_device_register(&s3c_device_i2c19);
#endif
#ifdef CONFIG_SEC_THERMISTOR
	platform_device_register(&sec_device_thermistor);
#endif
#if defined(CONFIG_MACH_C1) || defined(CONFIG_MACH_C1VZW) || \
	defined(CONFIG_MACH_JENGA) || defined(CONFIG_MACH_S2PLUS) || \
	defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_M3)
	if (system_rev != 3 && system_rev >= 1)
		platform_device_register(&m0_keypad);
	else
		platform_device_register(&midas_keypad);
#elif defined(CONFIG_MACH_C1CTC)
	platform_device_register(&m0_keypad);
#endif

#if defined(CONFIG_S3C_DEV_I2C5)
	if (need_i2c5())
		platform_device_register(&s3c_device_i2c5);
#endif

#if defined(CONFIG_SENSORS_GP2A) || defined(CONFIG_SENSORS_CM36651) || \
	defined(CONFIG_SENSORS_CM3663)
#if defined(CONFIG_MACH_M0) || defined(CONFIG_MACH_C1) ||\
	defined(CONFIG_MACH_JENGA) || \
	defined(CONFIG_MACH_C1VZW) || defined(CONFIG_MACH_M3) || \
	defined(CONFIG_MACH_C1CTC)
	if (need_rgb())
		platform_device_register(&s3c_device_i2c9_rgb);
	else
#endif
		platform_device_register(&s3c_device_i2c9);
#endif

#if defined(CONFIG_PN65N_NFC) && !defined(CONFIG_MACH_C1) \
	&& !defined(CONFIG_MACH_C1VZW) && !defined(CONFIG_MACH_M3) \
	&& !defined(CONFIG_MACH_C1CTC)
	platform_device_register(&s3c_device_i2c12);
#endif

#if defined(CONFIG_S3C64XX_DEV_SPI)
#if defined(CONFIG_VIDEO_S5C73M3_SPI)
	sclk = clk_get(spi1_dev, "dout_spi1");
	if (IS_ERR(sclk))
		dev_err(spi1_dev, "failed to get sclk for SPI-1\n");
	prnt = clk_get(spi1_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi1_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
		       prnt->name, sclk->name);

	clk_set_rate(sclk, 800 * 1000 * 1000);
	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPB(5), "SPI_CS1")) {
		gpio_direction_output(EXYNOS4_GPB(5), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPB(5), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPB(5), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(1, EXYNOS_SPI_SRCCLK_SCLK,
				    ARRAY_SIZE(spi1_csi));
	}

	for (gpio = EXYNOS4_GPB(4); gpio < EXYNOS4_GPB(8); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	spi_register_board_info(spi1_board_info, ARRAY_SIZE(spi1_board_info));
#endif

#if defined(CONFIG_PHONE_IPC_SPI) \
	|| defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	sclk = NULL;
	prnt = NULL;

	sclk = clk_get(spi2_dev, "dout_spi2");
	if (IS_ERR(sclk))
		dev_err(spi2_dev, "failed to get sclk for SPI-2\n");
	prnt = clk_get(spi2_dev, "mout_mpll_user");
	if (IS_ERR(prnt))
		dev_err(spi2_dev, "failed to get prnt\n");
	if (clk_set_parent(sclk, prnt))
		printk(KERN_ERR "Unable to set parent %s of clock %s.\n",
		       prnt->name, sclk->name);

	clk_put(sclk);
	clk_put(prnt);

	if (!gpio_request(EXYNOS4_GPC1(2), "SPI_CS2")) {
		gpio_direction_output(EXYNOS4_GPC1(2), 1);
		s3c_gpio_cfgpin(EXYNOS4_GPC1(2), S3C_GPIO_SFN(1));
		s3c_gpio_setpull(EXYNOS4_GPC1(2), S3C_GPIO_PULL_UP);
		exynos_spi_set_info(2, EXYNOS_SPI_SRCCLK_SCLK,
			ARRAY_SIZE(spi2_csi));
	}
	for (gpio = EXYNOS4_GPC1(1); gpio < EXYNOS4_GPC1(5); gpio++)
		s5p_gpio_set_drvstr(gpio, S5P_GPIO_DRVSTR_LV3);

	spi_register_board_info(spi2_board_info, ARRAY_SIZE(spi2_board_info));
#endif

#if defined(CONFIG_TDMB) || defined(CONFIG_TDMB_MODULE)
	tdmb_dev_init();
#endif
#endif

#ifdef CONFIG_BUSFREQ_OPP
	dev_add(&busfreq, &exynos4_busfreq.dev);

	/* PPMUs using for cpufreq get clk from clk_list */
	ppmu_clk = clk_get(NULL, "ppmudmc0");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc0\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmudmc1");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_dmc1\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_clk = clk_get(NULL, "ppmucpu");
	if (IS_ERR(ppmu_clk))
		printk(KERN_ERR "failed to get ppmu_cpu\n");
	clk_enable(ppmu_clk);
	clk_put(ppmu_clk);

	ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
	ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif


	/* 400 kHz for initialization of MMC Card  */
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS3) & 0xfffffff0)
		     | 0x9, EXYNOS4_CLKDIV_FSYS3);
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS2) & 0xfff0fff0)
		     | 0x80008, EXYNOS4_CLKDIV_FSYS2);
	__raw_writel((__raw_readl(EXYNOS4_CLKDIV_FSYS1) & 0xfff0fff0)
		     | 0x80008, EXYNOS4_CLKDIV_FSYS1);
}

#ifdef CONFIG_EXYNOS_C2C
static void __init exynos_c2c_reserve(void)
{
	static struct cma_region region = {
			.name = "c2c_shdmem",
			.size = 64 * SZ_1M,
			{ .alignment	= 64 * SZ_1M },
			.start = C2C_SHAREDMEM_BASE
	};

	BUG_ON(cma_early_region_register(&region));
	BUG_ON(cma_early_region_reserve(&region));

	pr_info("%s %10s %8x %8x\n", __func__,
		region.name, region.start, region.size);
}
#endif

static void __init exynos_init_reserve(void)
{
	sec_debug_magic_init();
}

MACHINE_START(SMDK4412, "SMDK4x12")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= midas_map_io,
	.init_machine	= midas_machine_init,
	.timer		= &exynos4_timer,
#ifdef CONFIG_EXYNOS_C2C
	.reserve	= &exynos_c2c_reserve,
#endif
	.init_early	= &exynos_init_reserve,
MACHINE_END

MACHINE_START(SMDK4212, "SMDK4x12")
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= exynos4_init_irq,
	.map_io		= midas_map_io,
	.init_machine	= midas_machine_init,
	.timer		= &exynos4_timer,
#ifdef CONFIG_EXYNOS_C2C
	.reserve	= &exynos_c2c_reserve,
#endif
	.init_early	= &exynos_init_reserve,
MACHINE_END