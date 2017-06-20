/*
 * Copyright (C) 2017 Marvell International Ltd.

 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mv_soc_info.h>
#include <linux/mvebu-sample-at-reset.h>

#define KHz 1000
#define MHz (1000 * 1000)
#define GHz (1000 * 1000 * 1000)

/* SAR AP806 registers */
#define SAR_CLOCK_FREQ_MODE_OFFSET	0
#define SAR_CLOCK_FREQ_MODE_MASK	(0x1f << SAR_CLOCK_FREQ_MODE_OFFSET)
#define SAR_TEST_MODE_ENABLE_OFFSET	5
#define SAR_TEST_MODE_ENABLE_MASK	(0x1 << SAR_TEST_MODE_ENABLE_OFFSET)
#define SAR_SKIP_LINK_I2C_INIT_OFFSET	6
#define SAR_SKIP_LINK_I2C_INIT_MASK	(0x1 << SAR_SKIP_LINK_I2C_INIT_OFFSET)
#define SAR_POR_BYPASS_OFFSET		7
#define SAR_POR_BYPASS_MASK		(0x1 << SAR_POR_BYPASS_OFFSET)
#define SAR_BOOT_SOURCE_OFFSET		8
#define SAR_BOOT_SOURCE_MASK		(0x7 << SAR_BOOT_SOURCE_OFFSET)
#define SAR_PIDI_C2C_IHB_SELECT_OFFSET	11
#define SAR_PIDI_C2C_IHB_SELECT_MASK	(0x1 << SAR_PIDI_C2C_IHB_SELECT_OFFSET)
#define SAR_I2C_INIT_ENABLE_OFFSET	12
#define SAR_I2C_INIT_ENABLE_MASK	(0x1 << SAR_I2C_INIT_ENABLE_OFFSET)
#define SAR_SSCG_DISABLE_OFFSET		13
#define SAR_SSCG_DISABLE_MASK		(0x1 << SAR_SSCG_DISABLE_OFFSET)
#define SAR_PIDI_HW_TRAINING_DIS_OFFSET	14
#define SAR_PIDI_HW_TRAINING_DIS_MASK	(0x1 << SAR_PIDI_HW_TRAINING_DIS_OFFSET)
#define SAR_CPU_FMAX_REFCLK_OFFSET	15
#define SAR_CPU_FMAX_REFCLK_MASK	(0x1 << SAR_CPU_FMAX_REFCLK_OFFSET)
#define SAR_IHB_DIFF_REFCLK_DIS_OFFSET	16
#define SAR_IHB_DIFF_REFCLK_DIS_MASK	(0x1 << SAR_IHB_DIFF_REFCLK_DIS_OFFSET)
#define SAR_REF_CLK_MSTR_OFFSET		17
#define SAR_REF_CLK_MSTR_MASK		(0x1 << SAR_REF_CLK_MSTR_OFFSET)
#define SAR_CPU_WAKE_UP_OFFSET		18
#define SAR_CPU_WAKE_UP_MASK		(0x1 << SAR_CPU_WAKE_UP_OFFSET)
#define SAR_XTAL_BYPASS_OFFSET		19
#define SAR_XTAL_BYPASS_MASK		(0x1 << SAR_XTAL_BYPASS_OFFSET)
#define SAR_PIDI_LOW_SPEED_OFFSET	20
#define SAR_PIDI_LOW_SPEED_MASK		(0x1 << SAR_PIDI_LOW_SPEED_OFFSET)

#define AP806_SAR_1_REG			4
#define SAR1_PLL2_OFFSET		(9)
#define SAR1_PLL2_MASK			(0x1f << SAR1_PLL2_OFFSET)
#define SAR1_PLL1_OFFSET		(14)
#define SAR1_PLL1_MASK			(0x1f << SAR1_PLL1_OFFSET)
#define SAR1_PLL0_OFFSET		(19)
#define SAR1_PLL0_MASK			(0x1f << SAR1_PLL0_OFFSET)
#define SAR1_PIDI_CONNECT_OFFSET	(24)
#define SAR1_PIDI_CONNECT_MASK		(1 << SAR1_PIDI_CONNECT_OFFSET)

/* SAR CP110 registers */
#define SAR_RST_PCIE0_CLOCK_CONFIG_CP1_OFFSET	(0)
#define SAR_RST_PCIE0_CLOCK_CONFIG_CP1_MASK	(0x1 << SAR_RST_PCIE0_CLOCK_CONFIG_CP1_OFFSET)
#define SAR_RST_PCIE1_CLOCK_CONFIG_CP1_OFFSET	(1)
#define SAR_RST_PCIE1_CLOCK_CONFIG_CP1_MASK	(0x1 << SAR_RST_PCIE1_CLOCK_CONFIG_CP1_OFFSET)
#define SAR_RST_PCIE0_CLOCK_CONFIG_CP0_OFFSET	(2)
#define SAR_RST_PCIE0_CLOCK_CONFIG_CP0_MASK	(0x1 << SAR_RST_PCIE0_CLOCK_CONFIG_CP0_OFFSET)
#define SAR_RST_PCIE1_CLOCK_CONFIG_CP0_OFFSET	(3)
#define SAR_RST_PCIE1_CLOCK_CONFIG_CP0_MASK	(0x1 << SAR_RST_PCIE1_CLOCK_CONFIG_CP0_OFFSET)
#define SAR_RST_BOOT_MODE_AP_CP0_OFFSET		(4)
#define SAR_RST_BOOT_MODE_AP_CP0_MASK		(0x3F << SAR_RST_BOOT_MODE_AP_CP0_OFFSET)
#define SAR_RST_CLOCK_FREQ_MODE_AP_OFFSET	(10)
#define SAR_RST_CLOCK_FREQ_MODE_AP_MASK		(0x7 << SAR_RST_CLOCK_FREQ_MODE_AP_OFFSET)
#define SAR_RST_BOOT_STANDALONE_CP1_OFFSET	(13)
#define SAR_RST_BOOT_STANDALONE_CP1_MASK	(0x1 << SAR_RST_BOOT_STANDALONE_CP1_OFFSET)
#define SAR_RST_REF_CLK_SELECT_CPX_OFFSET	(14)
#define SAR_RST_REF_CLK_SELECT_CPX_MASK		(0x1 << SAR_RST_REF_CLK_SELECT_CPX_OFFSET)
#define SAR_RST_I2C_INIT_ENABLE_CP0_OFFSET	(15)
#define SAR_RST_I2C_INIT_ENABLE_CP0_MASK	(0x1 << SAR_RST_I2C_INIT_ENABLE_CP0_OFFSET)
#define SAR_RST_I2C_INIT_ENABLE_AP_OFFSET	(16)
#define SAR_RST_I2C_INIT_ENABLE_AP_MASK		(0x1 << SAR_RST_I2C_INIT_ENABLE_AP_OFFSET)
#define SAR_RST_TEST_MODE_ENABLE_CP0_OFFSET	(17)
#define SAR_RST_TEST_MODE_ENABLE_CP0_MASK	(0x1 << SAR_RST_TEST_MODE_ENABLE_CP0_OFFSET)
#define SAR_RST_TEST_MODE_ENABLE_CP1_OFFSET	(18)
#define SAR_RST_TEST_MODE_ENABLE_CP1_MASK	(0x1 << SAR_RST_TEST_MODE_ENABLE_CP1_OFFSET)
#define SAR_RST_TEST_BOOT_STANDALONE_CP0_OFFSET	(19)
#define SAR_RST_TEST_BOOT_STANDALONE_CP0_MASK	(0x1 << SAR_RST_TEST_BOOT_STANDALONE_CP0_OFFSET)
#define SAR_RST_PIDI_C2C_IHB_SEL_AP_CP0_OFFSET	(20)
#define SAR_RST_PIDI_C2C_IHB_SEL_AP_CP0_MASK	(0x1 << SAR_RST_PIDI_C2C_IHB_SEL_AP_CP0_OFFSET)
#define SAR_RST_HW_TRAINING_DIS_AP_CP0_OFFSET	(21)
#define SAR_RST_HW_TRAINING_DIS_AP_CP0_MASK	(0x1 << SAR_RST_HW_TRAINING_DIS_AP_CP0_OFFSET)
#define SAR_RST_XTAL_BYPASS_CP0_OFFSET		(22)
#define SAR_RST_XTAL_BYPASS_CP0_MASK		(0x1 << SAR_RST_XTAL_BYPASS_CP0_OFFSET)
#define SAR_RST_POR_BYPASS_CP0_OFFSET		(23)
#define SAR_RST_POR_BYPASS_CP0_MASK		(0x1 << SAR_RST_POR_BYPASS_CP0_OFFSET)

#define CP110_SAR_1_REG				4
#define SAR1_RST_POR_BYPASS_CP1_OFFSET		(0)
#define SAR1_RST_POR_BYPASS_CP1_MASK		(0x1 << SAR1_RST_POR_BYPASS_CP1_OFFSET)
#define SAR1_RST_SSCG_DISABLE_AP_OFFSET		(1)
#define SAR1_RST_SSCG_DISABLE_AP_MASK		(0x1 << SAR1_RST_SSCG_DISABLE_AP_OFFSET)
#define SAR1_RST_RESERVED_AP_OFFSET		(2)
#define SAR1_RST_RESERVED_AP_MASK		(0x1 << SAR1_RST_RESERVED_AP_OFFSET)
#define SAR1_RST_SKIP_IHB_INIT_CP0_OFFSET	(3)
#define SAR1_RST_SKIP_IHB_INIT_CP0_MASK		(0x1 << SAR1_RST_SKIP_IHB_INIT_CP0_OFFSET)
#define SAR1_RST_AVS_SLAVE_CPX_OFFSET		(4)
#define SAR1_RST_AVS_SLAVE_CPX_MASK		(0x1 << SAR1_RST_AVS_SLAVE_CPX_OFFSET)
#define SAR1_RST_RESERVED_CP0_OFFSET		(5)
#define SAR1_RST_RESERVED_CP0_MASK		(0x1 << SAR1_RST_RESERVED_CP0_OFFSET)
#define SAR1_RST_RESERVED_CPX_OFFSET		(6)
#define SAR1_RST_RESERVED_CPX_MASK		(0x1 << SAR1_RST_RESERVED_CPX_OFFSET)
#define SAR1_RST_XTAL_BYPASS_CP1_OFFSET		(7)
#define SAR1_RST_XTAL_BYPASS_CP1_MASK		(0x1 << SAR1_RST_XTAL_BYPASS_CP1_OFFSET)

#define MV_SAR_DRIVER_NAME "mv_sample_at_reset_info"

/*
 * List of SoC supported in SAR driver.
 *  - SAR_AP806: AP806 Sample at Reset
 *  - SAR_CP110: CP110 Sample at Reset
 */
enum mvebu_sar_soc_opts {
	SAR_SOC_AP806,
	SAR_SOC_CP110,
	SAR_SOC_MAX
};

enum ap806_clocking_options {
	CPU_2000_DDR_1200_RCLK_1200 = 0x0,
	CPU_2000_DDR_1050_RCLK_1050 = 0x1,
	CPU_1600_DDR_800_RCLK_800 = 0x4,
	CPU_1800_DDR_1200_RCLK_1200 = 0x6,
	CPU_1800_DDR_1050_RCLK_1050 = 0x7,
	CPU_1600_DDR_900_RCLK_900 = 0x0b,
	CPU_1600_DDR_1050_RCLK_1050 = 0x0d,
	CPU_1600_DDR_900_RCLK_900_2 = 0x0e,
	CPU_1000_DDR_650_RCLK_650 = 0x13,
	CPU_1300_DDR_800_RCLK_800 = 0x14,
	CPU_1300_DDR_650_RCLK_650 = 0x17,
	CPU_1200_DDR_800_RCLK_800 = 0x19,
	CPU_1400_DDR_800_RCLK_800 = 0x1a,
	CPU_600_DDR_800_RCLK_800 = 0x1b,
	CPU_800_DDR_800_RCLK_800 = 0x1c,
	CPU_1000_DDR_800_RCLK_800 = 0x1d,
	CPU_FREQ_CLK_OPT_MAX = 16,
};

enum ap806_clocking_type {
	AP806_CPU_CLOCK_ID = 0,
	AP806_DDR_CLOCK_ID,
	AP806_RING_CLOCK_ID,
};

enum ap806_clocking_para_list {
	PARA_CPU_FREQ = 0,
	PARA_DDR_FREQ,
	PARA_RING_FREQ,
	PARA_CLOCK_OPTION,
	PARA_LIST_MAX,
};

struct sar_info {
	char *name;
	u32 offset;
	u32 mask;
};

struct bootsrc_idx_info {
	int start;
	int end;
	enum mvebu_bootsrc_type src;
	int index;
};

struct mv_sar_dev {
	void __iomem            *base;
	struct device           *dev;
	enum mvebu_sar_soc_opts sar_soc;
	struct list_head        list;
};

/* There is SAR on both AP806 and CP110.
 * A8K has 2 CPs and A7K has 1 CP, in order to support both SoC,
 * a SAR list is added, and put all SAR in the list after probe.
 */
static struct list_head sar_list = LIST_HEAD_INIT(sar_list);

/* ap806_sar_0: SAR info from AP806 SAR status0 register. */
static struct sar_info ap806_sar_0[] = {
	{"Clock Freq mode                 ", SAR_CLOCK_FREQ_MODE_OFFSET, SAR_CLOCK_FREQ_MODE_MASK },
	{"Test mode enable                ", SAR_TEST_MODE_ENABLE_OFFSET, SAR_TEST_MODE_ENABLE_MASK },
	{"Skip link i2c init              ", SAR_SKIP_LINK_I2C_INIT_OFFSET, SAR_SKIP_LINK_I2C_INIT_MASK },
	{"Por ByPass                      ", SAR_POR_BYPASS_OFFSET, SAR_POR_BYPASS_MASK },
	{"Boot Source                     ", SAR_BOOT_SOURCE_OFFSET, SAR_BOOT_SOURCE_MASK },
	{"PIDI C2C IHB select             ", SAR_PIDI_C2C_IHB_SELECT_OFFSET, SAR_PIDI_C2C_IHB_SELECT_MASK },
	{"I2C init enable                 ", SAR_I2C_INIT_ENABLE_OFFSET, SAR_I2C_INIT_ENABLE_MASK },
	{"SSCG disable                    ", SAR_SSCG_DISABLE_OFFSET, SAR_SSCG_DISABLE_MASK },
	{"PIDI hw training disable        ", SAR_PIDI_HW_TRAINING_DIS_OFFSET, SAR_PIDI_HW_TRAINING_DIS_MASK },
	{"CPU Fmax refclk select          ", SAR_CPU_FMAX_REFCLK_OFFSET, SAR_CPU_FMAX_REFCLK_MASK },
	{"IHB differential refclk disable ", SAR_IHB_DIFF_REFCLK_DIS_OFFSET, SAR_IHB_DIFF_REFCLK_DIS_MASK },
	{"Ref clk mstr                    ", SAR_REF_CLK_MSTR_OFFSET, SAR_REF_CLK_MSTR_MASK },
	{"CPU wake up                     ", SAR_CPU_WAKE_UP_OFFSET, SAR_CPU_WAKE_UP_MASK },
	{"Xtal ByPass                     ", SAR_XTAL_BYPASS_OFFSET, SAR_XTAL_BYPASS_MASK },
	{"PIDI low speed                  ", SAR_PIDI_LOW_SPEED_OFFSET, SAR_PIDI_LOW_SPEED_MASK },
	{"",			-1,			-1},
};

/* ap806_sar_1: SAR info from AP806 SAR status1 register. */
static struct sar_info ap806_sar_1[] = {
	{"PIDI connect       ", SAR1_PIDI_CONNECT_OFFSET, SAR1_PIDI_CONNECT_MASK },
	{"PLL0 Config        ", SAR1_PLL0_OFFSET, SAR1_PLL0_MASK },
	{"PLL1 Config        ", SAR1_PLL1_OFFSET, SAR1_PLL1_MASK },
	{"PLL2 Config        ", SAR1_PLL2_OFFSET, SAR1_PLL2_MASK },
	{"",			-1,			-1},
};

/* cp110_sar_0: SAR info from CP110 SAR status0 register. */
static struct sar_info cp110_sar_0[] = {
	{"CP1 PCIE0 clock config   ", SAR_RST_PCIE0_CLOCK_CONFIG_CP1_OFFSET, SAR_RST_PCIE0_CLOCK_CONFIG_CP1_MASK},
	{"CP1 PCIE1 clock config   ", SAR_RST_PCIE1_CLOCK_CONFIG_CP1_OFFSET, SAR_RST_PCIE1_CLOCK_CONFIG_CP1_MASK},
	{"CP0 PCIE0 clock config   ", SAR_RST_PCIE0_CLOCK_CONFIG_CP0_OFFSET, SAR_RST_PCIE0_CLOCK_CONFIG_CP0_MASK},
	{"CP0 PCIE1 clock config   ", SAR_RST_PCIE1_CLOCK_CONFIG_CP0_OFFSET, SAR_RST_PCIE1_CLOCK_CONFIG_CP0_MASK},
	{"CP0 Reset Boot Mode      ", SAR_RST_BOOT_MODE_AP_CP0_OFFSET, SAR_RST_BOOT_MODE_AP_CP0_MASK },
	{"AP Clock Freq Mode       ", SAR_RST_CLOCK_FREQ_MODE_AP_OFFSET, SAR_RST_CLOCK_FREQ_MODE_AP_MASK },
	{"CP1 Boot Standalone      ", SAR_RST_BOOT_STANDALONE_CP1_OFFSET, SAR_RST_BOOT_STANDALONE_CP1_MASK },
	{"CPx REF CLK Select       ", SAR_RST_REF_CLK_SELECT_CPX_OFFSET, SAR_RST_REF_CLK_SELECT_CPX_MASK },
	{"CP0 I2C INIT Enable      ", SAR_RST_I2C_INIT_ENABLE_CP0_OFFSET, SAR_RST_I2C_INIT_ENABLE_CP0_MASK },
	{"AP I2C INIT Enable       ", SAR_RST_I2C_INIT_ENABLE_AP_OFFSET, SAR_RST_I2C_INIT_ENABLE_AP_MASK },
	{"CP0 Test Mode Enable     ", SAR_RST_TEST_MODE_ENABLE_CP0_OFFSET, SAR_RST_TEST_MODE_ENABLE_CP0_MASK },
	{"CP1 Test Mode Enable     ", SAR_RST_TEST_MODE_ENABLE_CP1_OFFSET, SAR_RST_TEST_MODE_ENABLE_CP1_MASK },
	{"CP0 Tewst Boot Standalone", SAR_RST_TEST_BOOT_STANDALONE_CP0_OFFSET, SAR_RST_TEST_BOOT_STANDALONE_CP0_MASK },
	{"CP0 AP PIDI C2C IHB Sel  ", SAR_RST_PIDI_C2C_IHB_SEL_AP_CP0_OFFSET, SAR_RST_PIDI_C2C_IHB_SEL_AP_CP0_MASK },
	{"CP0 AP HW Training Dis   ", SAR_RST_HW_TRAINING_DIS_AP_CP0_OFFSET, SAR_RST_HW_TRAINING_DIS_AP_CP0_MASK },
	{"CP0 XTAL Bypass          ", SAR_RST_XTAL_BYPASS_CP0_OFFSET, SAR_RST_XTAL_BYPASS_CP0_MASK },
	{"CP0 POR Bypass           ", SAR_RST_POR_BYPASS_CP0_OFFSET, SAR_RST_POR_BYPASS_CP0_MASK },
	{"",			-1,			-1},
};

/* cp110_sar_1: SAR info from CP110 SAR status1 register. */
static struct sar_info cp110_sar_1[] = {
	{"CP1 POR Bypass           ", SAR1_RST_POR_BYPASS_CP1_OFFSET, SAR1_RST_POR_BYPASS_CP1_MASK},
	{"AP SSCG Disable          ", SAR1_RST_SSCG_DISABLE_AP_OFFSET, SAR1_RST_SSCG_DISABLE_AP_MASK},
	{"AP Reserved              ", SAR1_RST_RESERVED_AP_OFFSET, SAR1_RST_RESERVED_AP_MASK},
	{"CP0 Skip IHB INIT        ", SAR1_RST_SKIP_IHB_INIT_CP0_OFFSET, SAR1_RST_SKIP_IHB_INIT_CP0_MASK},
	{"CPX AVS Slave            ", SAR1_RST_AVS_SLAVE_CPX_OFFSET, SAR1_RST_AVS_SLAVE_CPX_MASK },
	{"CP0 Reserved             ", SAR1_RST_RESERVED_CP0_OFFSET, SAR1_RST_RESERVED_CP0_MASK },
	{"CPX Reserved             ", SAR1_RST_RESERVED_CPX_OFFSET, SAR1_RST_RESERVED_CPX_MASK },
	{"CP1 XTAL Bypass          ", SAR1_RST_XTAL_BYPASS_CP1_OFFSET, SAR1_RST_XTAL_BYPASS_CP1_MASK },
	{"",			-1,			-1},
};

static struct bootsrc_idx_info bootsrc_list[] = {
	{0x0,	0x5,	BOOTSRC_NOR,		0},
	{0xA,	0x25,	BOOTSRC_NAND,		0},
	{0x28,	0x28,	BOOTSRC_AP_SD_EMMC,	0},
	{0x29,	0x29,	BOOTSRC_SD_EMMC,	0},
	{0x2A,	0x2A,	BOOTSRC_AP_SD_EMMC,	0},
	{0x2B,	0x2B,	BOOTSRC_SD_EMMC,	0},
	{0x30,	0x30,	BOOTSRC_AP_SPI,		0},
	{0x31,	0x31,	BOOTSRC_AP_SPI,		0}, /* BootRom disabled */
	{0x32,	0x33,	BOOTSRC_SPI,		1},
	{0x34,	0x35,	BOOTSRC_SPI,		0},
	{0x36,	0x37,	BOOTSRC_SPI,		1}, /* BootRom disabled */
	{-1,	-1,	-1}
};

static const u32 ap806_pll_freq_tbl[CPU_FREQ_CLK_OPT_MAX][PARA_LIST_MAX] = {
	/* CPU */   /* DDR */   /* Ring */         /* Option */
	{2.0 * GHz, 1.2  * GHz, 1.2  * GHz, CPU_2000_DDR_1200_RCLK_1200},
	{2.0 * GHz, 1.05 * GHz, 1.05 * GHz, CPU_2000_DDR_1050_RCLK_1050},
	{1.8 * GHz, 1.2  * GHz, 1.2  * GHz, CPU_1800_DDR_1200_RCLK_1200},
	{1.8 * GHz, 1.05 * GHz, 1.05 * GHz, CPU_1800_DDR_1050_RCLK_1050},
	{1.6 * GHz, 1.05 * GHz, 1.05 * GHz, CPU_1600_DDR_1050_RCLK_1050},
	{1.6 * GHz, 900  * MHz, 900  * MHz, CPU_1600_DDR_900_RCLK_900_2},
	{1.3 * GHz, 800  * MHz, 800  * MHz, CPU_1300_DDR_800_RCLK_800},
	{1.3 * GHz, 650  * MHz, 650  * MHz, CPU_1300_DDR_650_RCLK_650},
	{1.6 * GHz, 800  * MHz, 800  * MHz, CPU_1600_DDR_800_RCLK_800},
	{1.6 * GHz, 900  * MHz, 900  * MHz, CPU_1600_DDR_900_RCLK_900},
	{1.0 * GHz, 650  * MHz, 650  * MHz, CPU_1000_DDR_650_RCLK_650},
	{1.2 * GHz, 800  * MHz, 800  * MHz, CPU_1200_DDR_800_RCLK_800},
	{1.4 * GHz, 800  * MHz, 800  * MHz, CPU_1400_DDR_800_RCLK_800},
	{600 * MHz, 800  * MHz, 800  * MHz, CPU_600_DDR_800_RCLK_800},
	{800 * MHz, 800  * MHz, 800  * MHz, CPU_800_DDR_800_RCLK_800},
	{1.0 * GHz, 800  * MHz, 800  * MHz, CPU_1000_DDR_800_RCLK_800}
};

/* mv_ap806_sar_get_clk_freq_mode
 * The function read the AP806 SAR and check the value with supported clock
 * option in ap806_pll_freq_tbl.
 * Return: if there is match, return the entry index of ap806_pll_freq_tbl.
 *         if there is no match, return fail error number.
 */
static u32 mv_ap806_sar_get_clk_freq_mode(struct mv_sar_dev *sar)
{
	u32 i;
	u32 clock_freq = (readl(sar->base) & SAR_CLOCK_FREQ_MODE_MASK) >>
			 SAR_CLOCK_FREQ_MODE_OFFSET;

	for (i = 0; i < CPU_FREQ_CLK_OPT_MAX; i++) {
		if (ap806_pll_freq_tbl[i][PARA_CLOCK_OPTION] == clock_freq)
			return i;
	}

	dev_err(sar->dev, "Unsupported clock freq mode %d", clock_freq);
	return -EINVAL;
}

/* mv_cp110_sar_bootsrc_get
 * The function read the CP110 SAR and check the boot mode with supported boot
 * source entry in bootsrc_list.
 * Output: val - if there is a match, val contains valid boot type and index.
 * Return: if there is match, return 0.
 *         if there is no match, return fail error number.
 */
static int mv_cp110_sar_bootsrc_get(struct mv_sar_dev *sar,
						struct sar_val *val)
{
	u32 reg, mode;
	int i;

	reg = readl(sar->base);
	mode = (reg & SAR_RST_BOOT_MODE_AP_CP0_MASK) >>
	       SAR_RST_BOOT_MODE_AP_CP0_OFFSET;

	val->raw_sar_val = mode;

	i = 0;
	while (bootsrc_list[i].start != -1) {
		if ((mode >= bootsrc_list[i].start) &&
		    (mode <= bootsrc_list[i].end)) {
			val->bootsrc.type = bootsrc_list[i].src;
			val->bootsrc.index = bootsrc_list[i].index;
			break;
		}
		i++;
	}

	if (bootsrc_list[i].start == -1) {
		dev_err(sar->dev,
			"Invalid CP110 sample at reset boot mode (%d)\n", mode);
		return -EINVAL;
	}

	return 0;
}

/* mv_sar_dev_find
 * The function traverses the SAR list and find the SAR matches with input
 * parameter of dev.
 * Return: if there is match, return matched SAR device.
 *         if there is no match, return NULL.
 */
static struct mv_sar_dev *mv_sar_dev_find(struct device *dev)
{
	struct list_head *curr;
	struct mv_sar_dev *sar_node;

	if (!dev || list_empty(&sar_list))
		return NULL;

	list_for_each(curr, &sar_list) {
		sar_node = list_entry(curr, struct mv_sar_dev, list);
		if (dev == sar_node->dev)
			return sar_node;
	}

	return NULL;
}

/* mv_sar_value_get
 * The API is to read the SAR info from the device input.
 * - dev: the device attached to the sar
 * - sar_opt: the sar info option to get
 * - val: the pointer to store the SAR info
 * Return: 0: success; non-zero: failed
 */
int mv_sar_value_get(struct device *dev,
			     enum mvebu_sar_opts sar_opt,
			     struct sar_val *val)
{
	u32 reg, mode, clock_type, clock_freq_mode;
	struct mv_sar_dev *sar_dev;

	sar_dev = mv_sar_dev_find(dev);
	if (!sar_dev)
		return -EINVAL;

	reg = readl(sar_dev->base);
	val->raw_sar_val = reg;

	if (sar_dev->sar_soc == SAR_SOC_AP806) {
		switch (sar_opt) {
		case(SAR_CPU_FREQ):
			clock_type = AP806_CPU_CLOCK_ID;
			break;
		case(SAR_DDR_FREQ):
			clock_type = AP806_DDR_CLOCK_ID;
			break;
		case(SAR_AP_FABRIC_FREQ):
			clock_type = AP806_RING_CLOCK_ID;
			break;
		default:
			dev_err(sar_dev->dev,
				"AP806: Unsupported Sample at reset field %d\n",
				sar_opt);
			return -EINVAL;
		}
		clock_freq_mode = mv_ap806_sar_get_clk_freq_mode(sar_dev);
		val->freq = ap806_pll_freq_tbl[clock_freq_mode][clock_type];
	} else {
		switch (sar_opt) {
		case SAR_BOOT_SRC:
			return mv_cp110_sar_bootsrc_get(sar_dev, val);
		case SAR_CP_PCIE0_CLK:
			mode = (reg & SAR_RST_PCIE0_CLOCK_CONFIG_CP0_MASK) >>
			       SAR_RST_PCIE0_CLOCK_CONFIG_CP0_OFFSET;
			break;
		case SAR_CP_PCIE1_CLK:
			mode = (reg & SAR_RST_PCIE1_CLOCK_CONFIG_CP0_MASK) >>
			       SAR_RST_PCIE1_CLOCK_CONFIG_CP0_OFFSET;
			break;
		default:
			dev_err(sar_dev->dev,
				"CP110: Unsupported Sample at reset field %d\n",
				sar_opt);
			return -EINVAL;
		}
		val->clk_direction = mode;
	}

	return 0;
}
EXPORT_SYMBOL(mv_sar_value_get);

/* mv_sar_dump
 * The API is to dump all SAR info from the device input.
 * - dev: the device attached to the sar
 * Return: 0: success; non-zero: failed.
 */
int mv_sar_dump(struct device *dev)
{
	u32 reg, val, sar1;
	struct sar_info *sar;
	struct mv_sar_dev *sar_dev;

	sar_dev = mv_sar_dev_find(dev);
	if (!sar_dev)
		return -EINVAL;

	reg = readl(sar_dev->base);
	dev_info(sar_dev->dev, "Sample at Reset register 0 [0x%08x]:\n", reg);
	dev_info(sar_dev->dev, "----------------------------------\n");
	sar = (sar_dev->sar_soc == SAR_SOC_AP806) ? ap806_sar_0 : cp110_sar_0;
	while (sar->offset != -1) {
		val = (reg & sar->mask) >> sar->offset;
		dev_info(sar_dev->dev, "%s  0x%x\n", sar->name, val);
		sar++;
	}

	sar1 = (sar_dev->sar_soc == SAR_SOC_AP806) ? AP806_SAR_1_REG :
						     CP110_SAR_1_REG;
	reg = readl(sar_dev->base + sar1);
	dev_info(sar_dev->dev, "Sample at Reset register 1 [0x%08x]:\n", reg);
	dev_info(sar_dev->dev, "----------------------------------\n");
	sar = (sar_dev->sar_soc == SAR_SOC_AP806) ? ap806_sar_1 : cp110_sar_1;
	while (sar->offset != -1) {
		val = (reg & sar->mask) >> sar->offset;
		dev_info(sar_dev->dev, "%s  0x%x\n", sar->name, val);
		sar++;
	}

	return 0;
}
EXPORT_SYMBOL(mv_sar_dump);

static int mv_sar_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct mv_sar_dev *sar;
	int err;
	struct device *dev = &pdev->dev;

	sar = devm_kcalloc(dev, 1, sizeof(struct mv_sar_dev), GFP_KERNEL);
	if (!sar)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res) {
		sar->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(sar->base)) {
			err = PTR_ERR(sar->base);
			return err;
		}
	}

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,sample-at-reset-cp110"))
		sar->sar_soc = SAR_SOC_CP110;
	else
		sar->sar_soc = SAR_SOC_AP806;
	sar->dev = &pdev->dev;

	/* Add to sar list */
	list_add(&sar->list, &sar_list);

	return 0;
}

static const struct of_device_id mv_sar_match[] = {
	{ .compatible = "marvell,sample-at-reset-ap806"},
	{ .compatible = "marvell,sample-at-reset-cp110"},
	{ }
};
MODULE_DEVICE_TABLE(of, mv_sar_match);

static struct platform_driver mv_sar_driver = {
	.probe = mv_sar_probe,
	.driver = {
		.name = MV_SAR_DRIVER_NAME,
		.of_match_table = mv_sar_match,
	},
};

module_platform_driver(mv_sar_driver);

MODULE_DESCRIPTION("Marvell Armada 80x0 Sample at Reset info driver");
MODULE_AUTHOR("Evan Wang(xswang@marvell.com)");
MODULE_LICENSE("GPL");

