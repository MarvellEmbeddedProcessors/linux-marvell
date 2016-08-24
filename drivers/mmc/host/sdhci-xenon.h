/*
 * Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * Author: Hu Ziji <huziji@marvell.com>
 * Date:   2016-7-30
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 */
#ifndef SDHCI_XENON_H_
#define SDHCI_XENON_H_

#include <linux/clk.h>
#include <linux/mmc/card.h>
#include <linux/of.h>
#include "sdhci.h"
#include "sdhci-xenon-phy.h"

/* Register Offset of SD Host Controller SOCP self-defined register */
#define SDHC_SYS_CFG_INFO			0x0104
#define SLOT_TYPE_SDIO_SHIFT			24
#define SLOT_TYPE_EMMC_MASK			0xff
#define SLOT_TYPE_EMMC_SHIFT			16
#define SLOT_TYPE_SD_SDIO_MMC_MASK		0xff
#define SLOT_TYPE_SD_SDIO_MMC_SHIFT		8

#define SDHC_SYS_OP_CTRL			0x0108
#define AUTO_CLKGATE_DISABLE_MASK		(0x1<<20)
#define SDCLK_IDLEOFF_ENABLE_SHIFT		8
#define SLOT_ENABLE_SHIFT			0

#define SDHC_SYS_EXT_OP_CTRL			0x010c

#define SDHC_SLOT_OP_STATUS_CTRL		0x0128
#define DELAY_90_DEGREE_MASK_EMMC5		(1 << 7)
#define DELAY_90_DEGREE_SHIFT_EMMC5		7
#define EMMC_5_0_PHY_FIXED_DELAY_MASK		0x7f
#define EMMC_PHY_FIXED_DELAY_MASK		0xff
#define EMMC_PHY_FIXED_DELAY_WINDOW_MIN		(EMMC_PHY_FIXED_DELAY_MASK >> 3)
#define SDH_PHY_FIXED_DELAY_MASK		0x1ff
#define SDH_PHY_FIXED_DELAY_WINDOW_MIN		(SDH_PHY_FIXED_DELAY_MASK >> 4)

#define TUN_CONSECUTIVE_TIMES_SHIFT		16
#define TUN_CONSECUTIVE_TIMES_MASK		0x7
#define TUN_CONSECUTIVE_TIMES			0x4
#define TUNING_STEP_SHIFT			12
#define TUNING_STEP_MASK			0xf

#define FORCE_SEL_INVERSE_CLK_SHIFT		11

#define SDHC_SLOT_eMMC_CTRL			0x0130
#define ENABLE_DATA_STROBE			(1 << 24)
#define SET_EMMC_RSTN				(1 << 16)
#define DISABLE_RD_DATA_CRC			(1 << 14)
#define DISABLE_CRC_STAT_TOKEN			(1 << 13)
#define eMMC_VCCQ_MASK				0x3
#define eMMC_VCCQ_1_8V				0x1
#define eMMC_VCCQ_3_3V				0x3

#define SDHC_SLOT_RETUNING_REQ_CTRL		0x0144
/* retuning compatible */
#define RETUNING_COMPATIBLE			0x1

#define SDHC_SLOT_AUTO_RETUNING_CTRL		0x0148
#define ENABLE_AUTO_RETUNING			0x1

#define SDHC_SLOT_DLL_CUR_DLY_VAL		0x150

/* Tuning Parameter */
#define TMR_RETUN_NO_PRESENT			0xf
#define XENON_MAX_TUN_COUNT			0xe

#define MMC_TIMING_FAKE				0xff

#define DEF_TUNING_COUNT			0x9

#define DEFAULT_SDCLK_FREQ			(400000)

/* Xenon specific Mode Select value */
#define XENON_SDHCI_CTRL_HS200			0x5
#define	XENON_SDHCI_CTRL_HS400			0x6

struct sdhci_xenon_priv {
	/*
	 * The bus_width, timing, and clock fields in below
	 * record the current setting of Xenon SDHC.
	 * Driver will call a Sampling Fixed Delay Adjustment
	 * if any setting is changed.
	 */
	unsigned char	bus_width;
	unsigned char	timing;
	unsigned char	tuning_count;
	unsigned int	clock;
	struct clk      *axi_clk;

	/* Slot idx */
	u8		slot_idx;

	int		phy_type;
	/*
	 * Contains board-specific PHY parameters
	 * passed from device tree.
	 */
	void		*phy_params;
	struct xenon_phy_ops phy_ops;

	/*
	 * When initializing card, Xenon has to determine card type and
	 * adjust Sampling Fixed delay.
	 * However, at that time, card structure is not linked to mmc_host.
	 * Thus a card pointer is added here to provide
	 * the delay adjustment function with the card structure
	 * of the card during initialization
	 */
	struct mmc_card *card_candidate;
};

static inline int enable_xenon_internal_clk(struct sdhci_host *host)
{
	u32 reg;
	u8 timeout;

	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_INT_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);
	/* Wait max 20 ms */
	timeout = 20;
	while (!((reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
			& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			pr_err("%s: Internal clock never stabilised.\n",
					mmc_hostname(host->mmc));
			return -EIO;
		}
		timeout--;
		mdelay(1);
	}

	return 0;
}

int xenon_phy_adj(struct sdhci_host *host, struct mmc_ios *ios);
int xenon_phy_parse_dt(struct device_node *np,
			struct sdhci_xenon_priv *priv);
void xenon_soc_pad_ctrl(struct sdhci_host *host,
			unsigned char signal_voltage);
#endif
