/* linux/arch/arm/mach-exynos/cpuidle-exynos4.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <asm/proc-fns.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

#include <mach/regs-clock.h>
#include <mach/regs-pmu.h>
#include <mach/pmu.h>
#include <mach/gpio.h>
#include <mach/smc.h>
#include <mach/clock-domain.h>
#include <mach/regs-audss.h>
#include <mach/asv.h>

#include <plat/regs-otg.h>
#include <plat/exynos4.h>
#include <plat/pm.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/usb-phy.h>
#include <mach/regs-usb-phy.h>

#ifdef CONFIG_ARM_TRUSTZONE
#define REG_DIRECTGO_ADDR	(S5P_VA_SYSRAM_NS + 0x24)
#define REG_DIRECTGO_FLAG	(S5P_VA_SYSRAM_NS + 0x20)
#else
#define REG_DIRECTGO_ADDR	(samsung_rev() < EXYNOS4210_REV_1_1 ?\
				(S5P_VA_SYSRAM + 0x24) : S5P_INFORM7)
#define REG_DIRECTGO_FLAG	(samsung_rev() < EXYNOS4210_REV_1_1 ?\
				(S5P_VA_SYSRAM + 0x20) : S5P_INFORM6)
#endif

extern unsigned long sys_pwr_conf_addr;
extern unsigned int l2x0_save[3];

enum hc_type {
	HC_SDHC,
	HC_MSHC,
};

enum idle_clock_down {
	HW_CLK_DWN,
	SW_CLK_DWN,
};

unsigned int use_clock_down;

struct check_device_op {
	void __iomem		*base;
	struct platform_device	*pdev;
	enum hc_type		type;
};

#if defined(CONFIG_MACH_MIDAS)
#define CPUDILE_ENABLE_MASK (ENABLE_LPA)
#else
#define CPUDILE_ENABLE_MASK (ENABLE_AFTR | ENABLE_LPA)
#endif

static enum {
	ENABLE_IDLE = 0x0,
	ENABLE_AFTR = 0x1,
	ENABLE_LPA  = 0x2
} enable_mask = CPUDILE_ENABLE_MASK;
module_param_named(enable_mask, enable_mask, uint, 0644);

#define ENABLE_LOWPWRMASK (ENABLE_AFTR | ENABLE_LPA)

static struct check_device_op chk_sdhc_op[] = {
#if defined(CONFIG_EXYNOS4_DEV_DWMCI)
	{.base = 0, .pdev = &exynos_device_dwmci, .type = HC_MSHC},
#endif
#if defined(CONFIG_EXYNOS4_DEV_MSHC)
	{.base = 0, .pdev = &s3c_device_mshci, .type = HC_MSHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC)
	{.base = 0, .pdev = &s3c_device_hsmmc0, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC1)
	{.base = 0, .pdev = &s3c_device_hsmmc1, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC2)
	{.base = 0, .pdev = &s3c_device_hsmmc2, .type = HC_SDHC},
#endif
#if defined(CONFIG_S3C_DEV_HSMMC3)
	{.base = 0, .pdev = &s3c_device_hsmmc3, .type = HC_SDHC},
#endif
};

static struct check_device_op chk_usbotg_op = {
	.base = 0, .pdev = &s3c_device_usbgadget, .type = 0
};

#define S3C_HSMMC_PRNSTS	(0x24)
#define S3C_HSMMC_CLKCON	(0x2c)
#define S3C_HSMMC_CMD_INHIBIT	0x00000001
#define S3C_HSMMC_DATA_INHIBIT	0x00000002
#define S3C_HSMMC_CLOCK_CARD_EN	0x0004

#define MSHCI_CLKENA	(0x10)  /* Clock enable */
#define MSHCI_STATUS	(0x48)  /* Status */
#define MSHCI_DATA_BUSY	(0x1<<9)
#define MSHCI_DATA_STAT_BUSY	(0x1<<10)
#define MSHCI_ENCLK	(0x1)

#define GPIO_OFFSET		0x20
#define GPIO_PUD_OFFSET		0x08
#define GPIO_CON_PDN_OFFSET	0x10
#define GPIO_PUD_PDN_OFFSET	0x14
#define GPIO_END_OFFSET		0x200

/* GPIO_END_OFFSET value of exynos4212 */
#define GPIO1_END_OFFSET	0x280
#define GPIO2_END_OFFSET	0x200
#define GPIO4_END_OFFSET	0xE0

static void exynos4_gpio_conpdn_reg(void)
{
	void __iomem *gpio_base = S5P_VA_GPIO;
	unsigned int val;

	do {
		/* Keep the previous state in didle mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Pull up-down state in didle is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;

		if (gpio_base == S5P_VA_GPIO + GPIO_END_OFFSET)
			gpio_base = S5P_VA_GPIO2;

	} while (gpio_base <= S5P_VA_GPIO2 + GPIO_END_OFFSET);

	/* set the GPZ */
	gpio_base = S5P_VA_GPIO3;
	__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

	val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
	__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);
}

static void exynos4212_gpio_conpdn_reg(void)
{
	void __iomem *gpio_base = S5P_VA_GPIO;
	unsigned int val;

	do {
		/* Keep the previous state in didle mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Pull up-down state in didle is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;

		/* Skip gpio_base there aren't gpios in part1 & part4 of exynos4212 */
		if (gpio_base == (S5P_VA_GPIO + 0xE0))
			gpio_base = S5P_VA_GPIO + 0x180;
		else if (gpio_base == (S5P_VA_GPIO + 0x200))
			gpio_base = S5P_VA_GPIO + 0x240;
		else if (gpio_base == (S5P_VA_GPIO4 + 0x40))
			gpio_base = S5P_VA_GPIO4 + 0x60;
		else if (gpio_base == (S5P_VA_GPIO4 + 0xA0))
			gpio_base = S5P_VA_GPIO4 + 0xC0;

		if (gpio_base == S5P_VA_GPIO + GPIO1_END_OFFSET)
			gpio_base = S5P_VA_GPIO2 + 0x40; /* GPK0CON */

		if (gpio_base == S5P_VA_GPIO2 + GPIO2_END_OFFSET)
			gpio_base = S5P_VA_GPIO4;

	} while (gpio_base <= S5P_VA_GPIO4 + GPIO4_END_OFFSET);

	/* set the GPZ */
	gpio_base = S5P_VA_GPIO3;
	__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

	val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
	__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);
}

static int check_power_domain(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_PMU_LCD0_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_MFC_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_G3D_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_CAM_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_TV_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	tmp = __raw_readl(S5P_PMU_GPS_CONF);
	if ((tmp & S5P_INT_LOCAL_PWR_EN) == S5P_INT_LOCAL_PWR_EN)
		return 1;

	return 0;
}

static int __maybe_unused check_clock_gating(void)
{
	unsigned long tmp;

	tmp = __raw_readl(EXYNOS4_CLKGATE_IP_IMAGE);
	if (tmp & (EXYNOS4_CLKGATE_IP_IMAGE_MDMA | EXYNOS4_CLKGATE_IP_IMAGE_SMMUMDMA
		| EXYNOS4_CLKGATE_IP_IMAGE_QEMDMA))
		return 1;

	tmp = __raw_readl(EXYNOS4_CLKGATE_IP_FSYS);
	if (tmp & (EXYNOS4_CLKGATE_IP_FSYS_PDMA0 | EXYNOS4_CLKGATE_IP_FSYS_PDMA1))
		return 1;

	tmp = __raw_readl(EXYNOS4_CLKGATE_IP_PERIL);
	if (tmp & EXYNOS4_CLKGATE_IP_PERIL_I2C0_7)
		return 1;

	return 0;
}

static int sdmmc_dev_num;
/* If SD/MMC interface is working: return = 1 or not 0 */
static int check_sdmmc_op(unsigned int ch)
{
	unsigned int reg1, reg2;
	void __iomem *base_addr;

	if (unlikely(ch >= sdmmc_dev_num)) {
		printk(KERN_ERR "Invalid ch[%d] for SD/MMC\n", ch);
		return 0;
	}

	if (chk_sdhc_op[ch].type == HC_SDHC) {
		base_addr = chk_sdhc_op[ch].base;
		/* Check CLKCON [2]: ENSDCLK */
		reg2 = readl(base_addr + S3C_HSMMC_CLKCON);
		return !!(reg2 & (S3C_HSMMC_CLOCK_CARD_EN));
	} else if (chk_sdhc_op[ch].type == HC_MSHC) {
		base_addr = chk_sdhc_op[ch].base;
		/* Check STATUS [9] for data busy */
		reg1 = readl(base_addr + MSHCI_STATUS);
		return (reg1 & (MSHCI_DATA_BUSY)) ||
		       (reg1 & (MSHCI_DATA_STAT_BUSY));

	}
	/* should not be here */
	return 0;
}

/* Check all sdmmc controller */
static int loop_sdmmc_check(void)
{
	unsigned int iter;

	for (iter = 0; iter < sdmmc_dev_num; iter++) {
		if (check_sdmmc_op(iter)) {
			printk(KERN_DEBUG "SDMMC [%d] working\n", iter);
			return 1;
		}
	}
	return 0;
}

#if 0 //Unused function
/*
 * Check USBOTG is working or not
 * GOTGCTL(0xEC000000)
 * BSesVld (Indicates the Device mode transceiver status)
 * BSesVld =	1b : B-session is valid
 *		0b : B-session is not valid
 */
static int check_usbotg_op(void)
{
	void __iomem *base_addr;
	unsigned int val;

	base_addr = chk_usbotg_op.base;
	val = __raw_readl(base_addr + S3C_UDC_OTG_GOTGCTL);

	return val & (A_SESSION_VALID | B_SESSION_VALID);
}
#endif

#ifdef CONFIG_SND_SAMSUNG_RP
extern int srp_get_op_level(void);	/* By srp driver */
#endif

#if defined(CONFIG_BT)
static inline int check_bt_op(void)
{
	extern int bt_is_running;

	return bt_is_running;
}
#endif

static int gps_is_running;

void set_gps_uart_op(int onoff)
{
	pr_info("%s: %s\n", __func__, onoff ? "on" : "off");
	gps_is_running = onoff;
}

static inline int check_gps_uart_op(void)
{
	return gps_is_running;
}

static int exynos4_check_operation(void)
{
	if (check_power_domain())
		return 1;

	if (clock_domain_enabled(LPA_DOMAIN))
		return 1;

	if (loop_sdmmc_check() || exynos4_check_usb_op())
		return 1;
#ifdef CONFIG_SND_SAMSUNG_RP
	if (srp_get_op_level())
		return 1;
#endif

#if defined(CONFIG_BT)
	if (check_bt_op())
		return 1;
#endif

	if (check_gps_uart_op())
		return 1;

	return 0;
}

static struct sleep_save exynos4_lpa_save[] = {
	/* CMU side */
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_TOP),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_CAM),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_TV),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_LCD0),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_LCD1),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_MAUDIO),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_FSYS),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_PERIL0),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_PERIL1),
	SAVE_ITEM(EXYNOS4_CLKSRC_MASK_DMC),
};

static struct sleep_save exynos4_aftr_save[] = {
	/* CMU side */
	SAVE_ITEM(S5P_CLKSRC_AUDSS),
	SAVE_ITEM(S5P_CLKDIV_AUDSS),
};

static struct sleep_save exynos4_set_clksrc[] = {
	{ .reg = EXYNOS4_CLKSRC_MASK_TOP			, .val = 0x00000001, },
	{ .reg = EXYNOS4_CLKSRC_MASK_CAM			, .val = 0x11111111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_TV				, .val = 0x00000111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_LCD0			, .val = 0x00001111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_MAUDIO			, .val = 0x00000001, },
	{ .reg = EXYNOS4_CLKSRC_MASK_FSYS			, .val = 0x01011111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_PERIL0			, .val = 0x01111111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_PERIL1			, .val = 0x01110111, },
	{ .reg = EXYNOS4_CLKSRC_MASK_DMC			, .val = 0x00010000, },
};

static struct sleep_save exynos4210_set_clksrc[] = {
	{ .reg = EXYNOS4_CLKSRC_MASK_LCD1			, .val = 0x00001111, },
};

static int exynos4_check_enter(void)
{
	unsigned int ret;
	unsigned int check_val;

	ret = 0;

	/* Check UART for console is empty */
	check_val = __raw_readl(S5P_VA_UART(CONFIG_S3C_LOWLEVEL_UART_PORT) +
				0x18);

	ret = ((check_val >> 16) & 0xff);

	return ret;
}

void exynos4_flush_cache(void *addr, phys_addr_t phy_ttb_base)
{
	outer_clean_range(virt_to_phys(addr - 0x40),
			  virt_to_phys(addr + 0x40));
	outer_clean_range(virt_to_phys(cpu_resume),
			  virt_to_phys(cpu_resume + 0x40));
	outer_clean_range(phy_ttb_base, phy_ttb_base + 0xffff);
	flush_cache_all();
}

static void exynos4_set_wakeupmask(void)
{
	__raw_writel(0x0000ff3e, S5P_WAKEUP_MASK);
}

static void vfp_enable(void *unused)
{
	u32 access = get_copro_access();

	/*
	 * Enable full access to VFP (cp10 and cp11)
	 */
	set_copro_access(access | CPACC_FULL(10) | CPACC_FULL(11));
}

static int exynos4_enter_core0_aftr(struct cpuidle_device *dev,
					struct cpuidle_driver *drv,
					int index)
{
	struct timeval before, after;
	int idle_time;
	unsigned long tmp;

	/*
	 * Defence code to avoid start up code latency after wakeup from aftr mode
	 */
	s3c_pm_do_save(exynos4_aftr_save, ARRAY_SIZE(exynos4_aftr_save));

	tmp = __raw_readl(S5P_CLKDIV_AUDSS);
	tmp &= ~S5P_AUDSS_CLKDIV_RP_MASK;
	__raw_writel(tmp, S5P_CLKDIV_AUDSS);

	local_irq_disable();
	do_gettimeofday(&before);

	exynos4_set_wakeupmask();

	__raw_writel(virt_to_phys(exynos4_idle_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	/* Set value of power down register for aftr mode */
	exynos4_sys_powerdown_conf(SYS_AFTR);

	if (!soc_is_exynos4210())
		exynos4_reset_assert_ctrl(0);

	if (!soc_is_exynos4210())
		exynos4x12_set_abb(ABB_MODE_100V);

	if (exynos4_enter_lp(0, PLAT_PHYS_OFFSET - PAGE_OFFSET) == 0) {

		/*
		 * Clear Central Sequence Register in exiting early wakeup
		 */
		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

		goto early_wakeup;
	}
	flush_tlb_all();

	cpu_init();

	vfp_enable(NULL);

	s3c_pm_do_restore_core(exynos4_aftr_save,
			       ARRAY_SIZE(exynos4_aftr_save));
early_wakeup:
	if ((exynos_result_of_asv > 3) && !soc_is_exynos4210())
		exynos4x12_set_abb(ABB_MODE_130V);

	if (!soc_is_exynos4210())
		exynos4_reset_assert_ctrl(1);

	/* Clear wakeup state register */
	__raw_writel(0x0, S5P_WAKEUP_STAT);

	do_gettimeofday(&after);

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

	dev->last_residency = idle_time;
	return index;
}

extern void bt_uart_rts_ctrl(int flag);

static int exynos4_enter_core0_lpa(struct cpuidle_device *dev,
					struct cpuidle_driver *drv,
					int index)
{
	struct timeval before, after;
	int idle_time;
	unsigned long tmp;

	s3c_pm_do_save(exynos4_lpa_save, ARRAY_SIZE(exynos4_lpa_save));

	/*
	 * Before enter central sequence mode, clock src register have to set
	 */
	s3c_pm_do_restore_core(exynos4_set_clksrc,
			       ARRAY_SIZE(exynos4_set_clksrc));

	if (soc_is_exynos4210())
		s3c_pm_do_restore_core(exynos4210_set_clksrc, ARRAY_SIZE(exynos4210_set_clksrc));

#if defined(CONFIG_BT)
	/* BT-UART RTS Control (RTS High) */
	bt_uart_rts_ctrl(1);
#endif
	local_irq_disable();

	do_gettimeofday(&before);

	/*
	 * Unmasking all wakeup source.
	 */
	__raw_writel(0x3ff0000, S5P_WAKEUP_MASK);

	/* Configure GPIO Power down control register */
	if (soc_is_exynos4210())
		exynos4_gpio_conpdn_reg();
	else
#ifdef CONFIG_MIDAS_COMMON
		if (exynos4_sleep_gpio_table_set)
			exynos4_sleep_gpio_table_set();
#else
		exynos4212_gpio_conpdn_reg();
#endif

	/* ensure at least INFORM0 has the resume address */
	__raw_writel(virt_to_phys(exynos4_idle_resume), S5P_INFORM0);

	__raw_writel(virt_to_phys(exynos4_idle_resume), REG_DIRECTGO_ADDR);
	__raw_writel(0xfcba0d10, REG_DIRECTGO_FLAG);

	__raw_writel(S5P_CHECK_LPA, S5P_INFORM1);
	exynos4_sys_powerdown_conf(SYS_LPA);

	/* Should be fixed on EVT1 */
	if (!soc_is_exynos4210())
		exynos4_reset_assert_ctrl(0);

	do {
		/* Waiting for flushing UART fifo */
	} while (exynos4_check_enter());

	if (!soc_is_exynos4210())
		exynos4x12_set_abb(ABB_MODE_100V);


	if (exynos4_enter_lp(0, PLAT_PHYS_OFFSET - PAGE_OFFSET) == 0) {

		/*
		 * Clear Central Sequence Register in exiting early wakeup
		 */
		tmp = __raw_readl(S5P_CENTRAL_SEQ_CONFIGURATION);
		tmp |= (S5P_CENTRAL_LOWPWR_CFG);
		__raw_writel(tmp, S5P_CENTRAL_SEQ_CONFIGURATION);

		goto early_wakeup;
	}
	flush_tlb_all();

	cpu_init();

	vfp_enable(NULL);

	s3c_pm_do_restore_core(exynos4_lpa_save,
			       ARRAY_SIZE(exynos4_lpa_save));

	/* For release retention */
	__raw_writel((1 << 28), S5P_PAD_RET_GPIO_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_UART_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_MMCA_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_MMCB_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_EBIA_OPTION);
	__raw_writel((1 << 28), S5P_PAD_RET_EBIB_OPTION);

early_wakeup:
	if ((exynos_result_of_asv > 3) && !soc_is_exynos4210())
		exynos4x12_set_abb(ABB_MODE_130V);

	if (!soc_is_exynos4210())
		exynos4_reset_assert_ctrl(1);

	/* Clear wakeup state register */
	__raw_writel(0x0, S5P_WAKEUP_STAT);

	__raw_writel(0x0, S5P_WAKEUP_MASK);

	do_gettimeofday(&after);

	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

#if defined(CONFIG_BT)
	/* BT-UART RTS Control (RTS Low) */
	bt_uart_rts_ctrl(0);
#endif

    dev->last_residency = idle_time;
    return index;
}

static int exynos4_enter_idle(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
			    int index);

static int exynos4_enter_lowpower(struct cpuidle_device *dev,
				struct cpuidle_driver *drv,
			    int index);

static struct cpuidle_state exynos4_cpuidle_set[] __initdata = {
	[0] = {
		.enter				= exynos4_enter_idle,
		.exit_latency		= 1,
		.target_residency	= 10000,
		.flags				= CPUIDLE_FLAG_TIME_VALID,
		.name				= "IDLE",
		.desc				= "ARM clock gating(WFI)",
	},
	[1] = {
		.enter				= exynos4_enter_lowpower,
		.exit_latency		= 300,
		.target_residency	= 10000,
		.flags				= CPUIDLE_FLAG_TIME_VALID,
		.name				= "LOW_POWER",
		.desc				= "ARM power down",
	},
};

static DEFINE_PER_CPU(struct cpuidle_device, exynos4_cpuidle_device);

static struct cpuidle_driver exynos4_idle_driver = {
	.name		= "exynos4_idle",
	.owner		= THIS_MODULE,
};

static unsigned int cpu_core;
static unsigned int old_div;
static DEFINE_SPINLOCK(idle_lock);

static int exynos4_enter_idle(struct cpuidle_device *dev,
                struct cpuidle_driver *drv,
                int index)
{
	struct timeval before, after;
	int idle_time;
	int cpu;
	unsigned int tmp;

	local_irq_disable();
	do_gettimeofday(&before);

	if (use_clock_down == SW_CLK_DWN) {
		/* USE SW Clock Down */
		cpu = get_cpu();

		spin_lock(&idle_lock);
		cpu_core |= (1 << cpu);

		if ((cpu_core == 0x3) || (cpu_online(1) == 0)) {
			old_div = __raw_readl(EXYNOS4_CLKDIV_CPU);
			tmp = old_div;
			tmp |= ((0x7 << 28) | (0x7 << 0));
			__raw_writel(tmp, EXYNOS4_CLKDIV_CPU);

			do {
				tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU);
			} while (tmp & 0x10000001);

		}

		spin_unlock(&idle_lock);

		cpu_do_idle();

		spin_lock(&idle_lock);

		if ((cpu_core == 0x3) || (cpu_online(1) == 0)) {
			__raw_writel(old_div, EXYNOS4_CLKDIV_CPU);

			do {
				tmp = __raw_readl(EXYNOS4_CLKDIV_STATCPU);
			} while (tmp & 0x10000001);

		}

		cpu_core &= ~(1 << cpu);
		spin_unlock(&idle_lock);

		put_cpu();
	} else
		cpu_do_idle();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
		    (after.tv_usec - before.tv_usec);

    dev->last_residency = idle_time;
    return index;
}

static int exynos4_check_entermode(void)
{
	unsigned int ret;
	unsigned int mask = (enable_mask & ENABLE_LOWPWRMASK);

	if (!mask)
		return 0;

	if ((mask & ENABLE_LPA) && !exynos4_check_operation())
		ret = S5P_CHECK_LPA;
	else if (mask & ENABLE_AFTR)
		ret = S5P_CHECK_DIDLE;
	else
		ret = 0;

	return ret;
}

static int exynos4_enter_lowpower(struct cpuidle_device *dev,
                struct cpuidle_driver *drv,
                int index)
{
	unsigned int enter_mode;
	unsigned int tmp;
	int new_index = index;

	/* This mode only can be entered when only Core0 is online */
	if (num_online_cpus() > 1)
		new_index = drv->safe_state_index;
	
	if (!soc_is_exynos4210()) {
		tmp = S5P_USE_STANDBY_WFI0 | S5P_USE_STANDBY_WFE0;
		__raw_writel(tmp, S5P_CENTRAL_SEQ_OPTION);
	}

    if (new_index == 0)
        return exynos4_enter_idle(dev, drv, new_index);

	enter_mode = exynos4_check_entermode();
	if (!enter_mode)
		return exynos4_enter_idle(dev, drv, new_index);
	else if (enter_mode == S5P_CHECK_DIDLE)
		return exynos4_enter_core0_aftr(dev, drv, new_index);
	else
		return exynos4_enter_core0_lpa(dev, drv, new_index);
}

static int exynos4_cpuidle_notifier_event(struct notifier_block *this,
					  unsigned long event,
					  void *ptr)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		disable_hlt();
		pr_debug("PM_SUSPEND_PREPARE for CPUIDLE\n");
		return NOTIFY_OK;
	case PM_POST_RESTORE:
	case PM_POST_SUSPEND:
		enable_hlt();
		pr_debug("PM_POST_SUSPEND for CPUIDLE\n");
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block exynos4_cpuidle_notifier = {
	.notifier_call = exynos4_cpuidle_notifier_event,
};

#ifdef CONFIG_EXYNOS4_ENABLE_CLOCK_DOWN
static void __init exynos4_core_down_clk(void)
{
	unsigned int tmp;

	tmp = __raw_readl(EXYNOS4_PWR_CTRL1);

	tmp &= ~(PWR_CTRL1_CORE2_DOWN_MASK | PWR_CTRL1_CORE1_DOWN_MASK);

	/* set arm clock divider value on idle state */
	tmp |= ((0x7 << PWR_CTRL1_CORE2_DOWN_RATIO) |
		(0x7 << PWR_CTRL1_CORE1_DOWN_RATIO));

	if (soc_is_exynos4212()) {
	/* Set PWR_CTRL1 register to use clock down feature */
		tmp |= (PWR_CTRL1_DIV2_DOWN_EN |
			PWR_CTRL1_DIV1_DOWN_EN |
			PWR_CTRL1_USE_CORE1_WFE |
			PWR_CTRL1_USE_CORE0_WFE |
			PWR_CTRL1_USE_CORE1_WFI |
			PWR_CTRL1_USE_CORE0_WFI);
	} else if (soc_is_exynos4412()) {
		tmp |= (PWR_CTRL1_DIV2_DOWN_EN |
			PWR_CTRL1_DIV1_DOWN_EN |
			PWR_CTRL1_USE_CORE3_WFE |
			PWR_CTRL1_USE_CORE2_WFE |
			PWR_CTRL1_USE_CORE1_WFE |
			PWR_CTRL1_USE_CORE0_WFE |
			PWR_CTRL1_USE_CORE3_WFI |
			PWR_CTRL1_USE_CORE2_WFI |
			PWR_CTRL1_USE_CORE1_WFI |
			PWR_CTRL1_USE_CORE0_WFI);
	}

	__raw_writel(tmp, EXYNOS4_PWR_CTRL1);

	tmp = __raw_readl(EXYNOS4_PWR_CTRL2);

	tmp &= ~(PWR_CTRL2_DUR_STANDBY2_MASK | PWR_CTRL2_DUR_STANDBY1_MASK |
		PWR_CTRL2_CORE2_UP_MASK | PWR_CTRL2_CORE1_UP_MASK);

	/* set duration value on middle wakeup step */
	tmp |=  ((0x1 << PWR_CTRL2_DUR_STANDBY2) |
		 (0x1 << PWR_CTRL2_DUR_STANDBY1));

	/* set arm clock divier value on middle wakeup step */
	tmp |= ((0x1 << PWR_CTRL2_CORE2_UP_RATIO) |
		(0x1 << PWR_CTRL2_CORE1_UP_RATIO));

	/* Set PWR_CTRL2 register to use step up for arm clock */
	tmp |= (PWR_CTRL2_DIV2_UP_EN | PWR_CTRL2_DIV1_UP_EN);

	__raw_writel(tmp, EXYNOS4_PWR_CTRL2);

	printk(KERN_INFO "Exynos4 : ARM Clock down on idle mode is enabled\n");
}
#else
#define exynos4_core_down_clk()	do { } while (0)
#endif

static int __init exynos4_init_cpuidle(void)
{
	int i, max_cpuidle_state, cpu_id;
	struct cpuidle_device *device;
	struct cpuidle_driver *drv = &exynos4_idle_driver;
	struct platform_device *pdev;
	struct resource *res;

	if (soc_is_exynos4210())
		use_clock_down = SW_CLK_DWN;
	else
		use_clock_down = HW_CLK_DWN;

	/* Clock down feature can use only EXYNOS4212 */
	if (use_clock_down == HW_CLK_DWN)
		exynos4_core_down_clk();

	/* Setup cpuidle driver */
	drv->state_count = (sizeof(exynos4_cpuidle_set) /
						sizeof(struct cpuidle_state));
	max_cpuidle_state = drv->state_count;
	for (i = 0; i < max_cpuidle_state; i++) {
		memcpy(&drv->states[i], &exynos4_cpuidle_set[i],
						sizeof(struct cpuidle_state));
	}

	cpuidle_register_driver(&exynos4_idle_driver);

	for_each_cpu(cpu_id, cpu_online_mask) {
		device = &per_cpu(exynos4_cpuidle_device, cpu_id);
		device->cpu = cpu_id;

		if (cpu_id == 0)
			device->state_count = ARRAY_SIZE(exynos4_cpuidle_set);
		else
			device->state_count = 1;	/* Support IDLE only */

		device->state_count = drv->state_count;

		if (cpuidle_register_device(device)) {
			printk(KERN_ERR "CPUidle register device failed\n,");
			return -EIO;
		}
	}

	sdmmc_dev_num = ARRAY_SIZE(chk_sdhc_op);

	for (i = 0; i < sdmmc_dev_num; i++) {

		pdev = chk_sdhc_op[i].pdev;

		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			printk(KERN_ERR "failed to get iomem region\n");
			return -EINVAL;
		}

		chk_sdhc_op[i].base = ioremap(res->start, resource_size(res));

		if (!chk_sdhc_op[i].base) {
			printk(KERN_ERR "failed to map io region\n");
			return -EINVAL;
		}
	}

	pdev = chk_usbotg_op.pdev;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		printk(KERN_ERR "failed to get iomem region\n");
		return -EINVAL;
	}

	chk_usbotg_op.base = ioremap(res->start, resource_size(res));

	if (!chk_usbotg_op.base) {
		printk(KERN_ERR "failed to map io region\n");
		return -EINVAL;
	}

	register_pm_notifier(&exynos4_cpuidle_notifier);
	sys_pwr_conf_addr = (unsigned long)S5P_CENTRAL_SEQ_CONFIGURATION;

	/* Save register value for L2X0 */
	l2x0_save[0] = __raw_readl(S5P_VA_L2CC + 0x108);
	l2x0_save[1] = __raw_readl(S5P_VA_L2CC + 0x10C);
	l2x0_save[2] = __raw_readl(S5P_VA_L2CC + 0xF60);

	flush_cache_all();
	outer_clean_range(virt_to_phys(l2x0_save), ARRAY_SIZE(l2x0_save));

	return 0;
}
device_initcall(exynos4_init_cpuidle);