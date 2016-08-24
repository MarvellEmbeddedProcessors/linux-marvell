/*
 * PHY support for Xenon SDHC
 *
 * Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * Author:	Hu Ziji <huziji@marvell.com>
 * Date:		2016-7-30
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/of_address.h>

#include "../core/core.h"
#include "../core/sdio_ops.h"
#include "../core/mmc_ops.h"

#include "sdhci.h"
#include "sdhci-pltfm.h"
#include "sdhci-xenon.h"

static const char * const phy_types[] = {
	"sdh phy",
	"emmc 5.0 phy",
	"emmc 5.1 phy"
};

enum phy_type_enum {
	SDH_PHY,
	EMMC_5_0_PHY,
	EMMC_5_1_PHY,
	NR_PHY_TYPES
};

static int xenon_delay_adj_test(struct mmc_card *card);

/*
 * eMMC PHY configuration and operations
 */

struct emmc_phy_params {
	u8 znr;
	u8 zpr;
	bool no_dll_tuning;

	/* Set SOC PHY PAD ctrl to fixed 1.8V */
	bool fixed_1_8v_pad_ctrl;

	/* MMC PAD address */
	void __iomem *pad_ctrl_addr;
};

static void xenon_emmc_phy_strobe_delay_adj(struct sdhci_host *host,
					struct mmc_card *card);
static int xenon_emmc_phy_fix_sampl_delay_adj(struct sdhci_host *host,
					struct mmc_card *card);
static void xenon_emmc_phy_set(struct sdhci_host *host,
					unsigned char timing);
static void xenon_emmc_phy_config_tuning(struct sdhci_host *host);
static void xenon_emmc_soc_pad_ctrl(struct sdhci_host *host,
					unsigned char signal_voltage);

static const struct xenon_phy_ops emmc_phy_ops = {
	.strobe_delay_adj = xenon_emmc_phy_strobe_delay_adj,
	.fix_sampl_delay_adj = xenon_emmc_phy_fix_sampl_delay_adj,
	.phy_set = xenon_emmc_phy_set,
	.config_tuning = xenon_emmc_phy_config_tuning,
	.soc_pad_ctrl = xenon_emmc_soc_pad_ctrl,
};

static int alloc_emmc_phy(struct sdhci_xenon_priv *priv)
{
	struct emmc_phy_params *params;

	params = kzalloc(sizeof(struct emmc_phy_params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	priv->phy_params = params;
	priv->phy_ops = emmc_phy_ops;
	return 0;
}

static int emmc_phy_parse_param_dt(struct device_node *np,
						struct emmc_phy_params *params)
{
	u32 value;

	if (of_get_property(np, "xenon,phy-no-dll-tuning", NULL))
		params->no_dll_tuning = true;
	else
		params->no_dll_tuning = false;

	if (!of_property_read_u32(np, "xenon,phy-znr", &value))
		params->znr = value & ZNR_MASK;
	else
		params->znr = ZNR_DEF_VALUE;

	if (!of_property_read_u32(np, "xenon,phy-zpr", &value))
		params->zpr = value & ZPR_MASK;
	else
		params->zpr = ZPR_DEF_VALUE;

	if (of_property_read_bool(np, "xenon,fixed-1-8v-pad-ctrl"))
		params->fixed_1_8v_pad_ctrl = true;
	else
		params->fixed_1_8v_pad_ctrl = false;

	params->pad_ctrl_addr = of_iomap(np, 1);
	if (IS_ERR(params->pad_ctrl_addr))
		params->pad_ctrl_addr = 0;

	return 0;
}

static int xenon_emmc_phy_init(struct sdhci_host *host)
{
	u32 reg;
	u32 wait, clock;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int timing_adj_reg;

	if (priv->phy_type == EMMC_5_0_PHY)
		timing_adj_reg = EMMC_5_0_PHY_TIMING_ADJUST;
	else
		timing_adj_reg = EMMC_PHY_TIMING_ADJUST;

	reg = sdhci_readl(host, timing_adj_reg);
	reg |= PHY_INITIALIZAION;
	sdhci_writel(host, reg, timing_adj_reg);

	/* Add duration of FC_SYNC_RST */
	wait = ((reg >> FC_SYNC_RST_DURATION_SHIFT) &
			FC_SYNC_RST_DURATION_MASK);
	/* Add interval between FC_SYNC_EN and FC_SYNC_RST */
	wait += ((reg >> FC_SYNC_RST_EN_DURATION_SHIFT) &
			FC_SYNC_RST_EN_DURATION_MASK);
	/* Add duration of asserting FC_SYNC_EN */
	wait += ((reg >> FC_SYNC_EN_DURATION_SHIFT) &
			FC_SYNC_EN_DURATION_MASK);
	/* Add duration of waiting for PHY */
	wait += ((reg >> WAIT_CYCLE_BEFORE_USING_SHIFT) &
			WAIT_CYCLE_BEFORE_USING_MASK);
	/*
	 * According to Moyang, 4 addtional bus clock
	 * and 4 AXI bus clock are required
	 */
	wait += 8;
	/* left shift 20 bits */
	wait <<= 20;

	clock = host->clock;
	if (!clock)
		/* Use the possibly slowest bus frequency value */
		clock = 100000;
	/* get the wait time */
	wait /= clock;
	wait++;
	/* wait for host eMMC PHY init completes */
	udelay(wait);

	reg = sdhci_readl(host, timing_adj_reg);
	reg &= PHY_INITIALIZAION;
	if (reg) {
		pr_err("%s: eMMC PHY init cannot complete after %d us\n",
			mmc_hostname(host->mmc), wait);
		return -EIO;
	}

	return 0;
}

static inline void soc_pad_voltage_set(void __iomem *pad_ctrl,
					unsigned char signal_voltage)
{
	if (!pad_ctrl)
		return;

	if (signal_voltage == MMC_SIGNAL_VOLTAGE_180)
		writel(SOC_PAD_1_8V, pad_ctrl);
	else if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		writel(SOC_PAD_3_3V, pad_ctrl);
}

static void xenon_emmc_soc_pad_ctrl(struct sdhci_host *host,
					unsigned char signal_voltage)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;

	if (params->fixed_1_8v_pad_ctrl)
		soc_pad_voltage_set(params->pad_ctrl_addr,
					MMC_SIGNAL_VOLTAGE_180);
	else
		soc_pad_voltage_set(params->pad_ctrl_addr,
					signal_voltage);
}

static int xenon_emmc_phy_set_fix_sampl_delay(struct sdhci_host *host,
			unsigned int delay, bool invert, bool delay_90_degree)
{
	u32 reg;
	unsigned long flags;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int timing_adj_reg;
	u32 delay_mask;
	int ret = 0;

	spin_lock_irqsave(&host->lock, flags);

	if (priv->phy_type == EMMC_5_0_PHY) {
		timing_adj_reg = EMMC_5_0_PHY_TIMING_ADJUST;
		delay_mask = EMMC_5_0_PHY_FIXED_DELAY_MASK;
	} else {
		timing_adj_reg = EMMC_PHY_TIMING_ADJUST;
		delay_mask = EMMC_PHY_FIXED_DELAY_MASK;
	}

	/* Setup Sampling fix delay */
	reg = sdhci_readl(host, SDHC_SLOT_OP_STATUS_CTRL);
	reg &= ~delay_mask;
	reg |= delay & delay_mask;
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);

	if (priv->phy_type == EMMC_5_0_PHY) {
		/* set 90 degree phase if necessary */
		reg &= ~DELAY_90_DEGREE_MASK_EMMC5;
		reg |= (delay_90_degree << DELAY_90_DEGREE_SHIFT_EMMC5);
		sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);
	}

	/* Disable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~(SDHCI_CLOCK_CARD_EN | SDHCI_CLOCK_INT_EN);
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	if (priv->phy_type == EMMC_5_1_PHY) {
		/* set 90 degree phase if necessary */
		reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
		reg &= ~ASYNC_DDRMODE_MASK;
		reg |= (delay_90_degree << ASYNC_DDRMODE_SHIFT);
		sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
	}

	/* Setup Inversion of Sampling edge */
	reg = sdhci_readl(host, timing_adj_reg);
	reg &= ~SAMPL_INV_QSP_PHASE_SELECT;
	reg |= (invert << SAMPL_INV_QSP_PHASE_SELECT_SHIFT);
	sdhci_writel(host, reg, timing_adj_reg);

	/* Enable SD internal clock */
	ret = enable_xenon_internal_clk(host);
	if (ret)
		goto out;

	/* Enable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	/*
	 * Has to re-initialize eMMC PHY here to active PHY
	 * because later get status cmd will be issued.
	 */
	ret = xenon_emmc_phy_init(host);

out:
	spin_unlock_irqrestore(&host->lock, flags);
	return ret;
}

static int xenon_emmc_phy_do_fix_sampl_delay(struct sdhci_host *host,
			struct mmc_card *card, unsigned int delay,
			bool invert, bool quarter)
{
	int ret;

	xenon_emmc_phy_set_fix_sampl_delay(host, delay, invert, quarter);

	ret = xenon_delay_adj_test(card);
	if (ret) {
		pr_debug("Xenon fail when sampling fix delay = %d, phase = %d degree\n",
				delay, invert * 180 + quarter * 90);
		return -1;
	}
	return 0;
}

static int xenon_emmc_phy_fix_sampl_delay_adj(struct sdhci_host *host,
					struct mmc_card *card)
{
	enum sampl_fix_delay_phase phase;
	int idx, nr_pair;
	int ret;
	unsigned int delay;
	unsigned int min_delay, max_delay;
	bool invert, quarter;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u32 delay_mask, coarse_step, fine_step;
	/*
	 * Pairs to set the delay edge
	 * First column is the inversion sequence.
	 * Second column indicates delay 90 degree or not
	 */
	const enum sampl_fix_delay_phase delay_edge[] = {
		PHASE_0_DEGREE,
		PHASE_180_DEGREE,
		PHASE_90_DEGREE,
		PHASE_270_DEGREE
	};

	if (priv->phy_type == EMMC_5_0_PHY)
		delay_mask = EMMC_5_0_PHY_FIXED_DELAY_MASK;
	else
		delay_mask = EMMC_PHY_FIXED_DELAY_MASK;
	coarse_step = delay_mask >> 1;
	fine_step = coarse_step >> 2;

	nr_pair = ARRAY_SIZE(delay_edge);

	for (idx = 0; idx < nr_pair; idx++) {
		phase = delay_edge[idx];
		invert = (phase & 0x2) ? true : false;
		quarter = (phase & 0x1) ? true : false;

		/* increase dly value to get fix delay */
		for (min_delay = 0; min_delay <= delay_mask;
				min_delay += coarse_step) {
			ret = xenon_emmc_phy_do_fix_sampl_delay(host, card,
					min_delay, invert, quarter);
			if (!ret)
				break;
		}

		if (ret) {
			pr_debug("Fail to set Sampling Fixed Delay with phase = %d degree\n",
					phase * 90);
			continue;
		}

		for (max_delay = min_delay + fine_step;
			max_delay < delay_mask;
			max_delay += fine_step) {
			ret = xenon_emmc_phy_do_fix_sampl_delay(host, card,
					max_delay, invert, quarter);
			if (ret) {
				max_delay -= fine_step;
				break;
			}
		}

		if (!ret) {
			ret = xenon_emmc_phy_do_fix_sampl_delay(host, card,
					delay_mask, invert, quarter);
			if (!ret)
				max_delay = delay_mask;
		}

		/*
		 * Sampling Fixed Delay line window shoul be larger enough,
		 * thus the sampling point (the middle of the window)
		 * can work when environment varies.
		 * However, there is no clear conclusoin how large the window
		 * should be.
		 */
		if ((max_delay - min_delay) <=
				EMMC_PHY_FIXED_DELAY_WINDOW_MIN) {
			pr_info("The window size %d when phase = %d degree cannot meet timing requiremnt\n",
				max_delay - min_delay, phase * 90);
			continue;
		}

		delay = (min_delay + max_delay) / 2;
		xenon_emmc_phy_set_fix_sampl_delay(host, delay, invert,
					quarter);
		pr_debug("Xenon sampling fix delay = %d with phase = %d degree\n",
				delay, phase * 90);
		return 0;
	}

	return -EIO;
}

static int xenon_emmc_phy_enable_dll(struct sdhci_host *host)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int dll_ctrl;
	u32 dll_update;

	WARN_ON(host->clock <= MMC_HIGH_52_MAX_DTR);
	if (host->clock <= MMC_HIGH_52_MAX_DTR)
		return -EINVAL;

	if (priv->phy_type == EMMC_5_0_PHY) {
		dll_ctrl = EMMC_5_0_PHY_DLL_CONTROL;
		dll_update = DLL_UPDATE_STROBE_5_0;
	} else {
		dll_ctrl = EMMC_PHY_DLL_CONTROL;
		dll_update = DLL_UPDATE;
	}

	reg = sdhci_readl(host, dll_ctrl);
	if (reg & DLL_ENABLE)
		return 0;

	/* Enable DLL */
	reg = sdhci_readl(host, dll_ctrl);
	reg |= (DLL_ENABLE | DLL_FAST_LOCK);

	/*
	 * Set Phase as 90 degree, which is most common value.
	 * Might set another value if necessary.
	 * The granularity is 1 degree.
	 */
	reg &= ~((DLL_PHASE_MASK << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_MASK << DLL_PHSEL1_SHIFT));
	reg |= ((DLL_PHASE_90_DEGREE << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_90_DEGREE << DLL_PHSEL1_SHIFT));

	reg &= ~DLL_BYPASS_EN;
	reg |= dll_update;
	if (priv->phy_type == EMMC_5_1_PHY)
		reg &= ~DLL_REFCLK_SEL;
	sdhci_writel(host, reg, dll_ctrl);

	/* Wait max 5 ms */
	mdelay(5);
	return 0;
}

static void xenon_emmc_phy_config_tuning(struct sdhci_host *host)
{
	u32 reg, tuning_step;
	int ret;
	unsigned long flags;

	WARN_ON(host->clock <= MMC_HIGH_52_MAX_DTR);
	if (host->clock <= MMC_HIGH_52_MAX_DTR)
		return;

	spin_lock_irqsave(&host->lock, flags);

	ret = xenon_emmc_phy_enable_dll(host);
	if (ret) {
		spin_unlock_irqrestore(&host->lock, flags);
		return;
	}

	reg = sdhci_readl(host, SDHC_SLOT_DLL_CUR_DLY_VAL);
	tuning_step = reg / 40;
	if (unlikely(tuning_step > TUNING_STEP_MASK)) {
		WARN("%s: HS200 TUNING_STEP %d is larger than MAX value\n",
					mmc_hostname(host->mmc), tuning_step);
		tuning_step = TUNING_STEP_MASK;
	}

	reg = sdhci_readl(host, SDHC_SLOT_OP_STATUS_CTRL);
	reg &= ~(TUN_CONSECUTIVE_TIMES_MASK << TUN_CONSECUTIVE_TIMES_SHIFT);
	reg |= (TUN_CONSECUTIVE_TIMES << TUN_CONSECUTIVE_TIMES_SHIFT);
	reg &= ~(TUNING_STEP_MASK << TUNING_STEP_SHIFT);
	reg |= (tuning_step << TUNING_STEP_SHIFT);
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);

	spin_unlock_irqrestore(&host->lock, flags);
}

static void xenon_emmc_phy_strobe_delay_adj(struct sdhci_host *host,
					struct mmc_card *card)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	unsigned long flags;

	if (host->clock <= MMC_HIGH_52_MAX_DTR)
		return;

	pr_debug("%s: starts HS400 strobe delay adjustment\n",
				mmc_hostname(host->mmc));

	spin_lock_irqsave(&host->lock, flags);

	xenon_emmc_phy_enable_dll(host);

	/* Enable SDHC Data Strobe */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg |= ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);

	/* Set Data Strobe Pull down */
	if (priv->phy_type == EMMC_5_0_PHY) {
		reg = sdhci_readl(host, EMMC_5_0_PHY_PAD_CONTROL);
		reg |= EMMC5_FC_QSP_PD;
		reg &= ~EMMC5_FC_QSP_PU;
		sdhci_writel(host, reg, EMMC_5_0_PHY_PAD_CONTROL);
	} else {
		reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
		reg |= EMMC5_1_FC_QSP_PD;
		reg &= ~EMMC5_1_FC_QSP_PU;
		sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
	}
	spin_unlock_irqrestore(&host->lock, flags);
}

#define LOGIC_TIMING_VALUE	0x00aa8977

static void xenon_emmc_phy_set(struct sdhci_host *host,
					unsigned char timing)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;
	struct mmc_card *card = priv->card_candidate;
	int pad_ctrl, timing_adj, pad_ctrl2, func_ctrl, logic_timing_adj;
	unsigned long flags;

	pr_debug("%s: eMMC PHY setting starts\n", mmc_hostname(host->mmc));

	if (priv->phy_type == EMMC_5_0_PHY) {
		pad_ctrl = EMMC_5_0_PHY_PAD_CONTROL;
		timing_adj = EMMC_5_0_PHY_TIMING_ADJUST;
		pad_ctrl2 = EMMC_5_0_PHY_PAD_CONTROL2;
		func_ctrl = EMMC_5_0_PHY_FUNC_CONTRL;
		logic_timing_adj = EMMC_5_0_PHY_LOGIC_TIMING_ADJUST;
	} else {
		pad_ctrl = EMMC_PHY_PAD_CONTROL;
		timing_adj = EMMC_PHY_TIMING_ADJUST;
		pad_ctrl2 = EMMC_PHY_PAD_CONTROL2;
		func_ctrl = EMMC_PHY_FUNC_CONTROL;
		logic_timing_adj = EMMC_PHY_LOGIC_TIMING_ADJUST;
	}

	spin_lock_irqsave(&host->lock, flags);

	/* Setup pad, set bit[28] and bits[26:24] */
	reg = sdhci_readl(host, pad_ctrl);
	reg |= (FC_DQ_RECEN | FC_CMD_RECEN | FC_QSP_RECEN | OEN_QSN);
	/*
	 * All FC_XX_RECEIVCE should be set as CMOS Type
	 */
	reg |= FC_ALL_CMOS_RECEIVER;
	sdhci_writel(host, reg, pad_ctrl);

	/* Set CMD and DQ Pull Up */
	if (priv->phy_type == EMMC_5_0_PHY) {
		reg = sdhci_readl(host, EMMC_5_0_PHY_PAD_CONTROL);
		reg |= (EMMC5_FC_CMD_PU | EMMC5_FC_DQ_PU);
		reg &= ~(EMMC5_FC_CMD_PD | EMMC5_FC_DQ_PD);
		sdhci_writel(host, reg, EMMC_5_0_PHY_PAD_CONTROL);
	} else {
		reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
		reg |= (EMMC5_1_FC_CMD_PU | EMMC5_1_FC_DQ_PU);
		reg &= ~(EMMC5_1_FC_CMD_PD | EMMC5_1_FC_DQ_PD);
		sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
	}

	if ((timing == MMC_TIMING_LEGACY) || !card)
		goto phy_init;

	/*
	 * FIXME: should depends on the specific board timing.
	 */
	if ((timing == MMC_TIMING_MMC_HS400) ||
		(timing == MMC_TIMING_MMC_HS200) ||
		(timing == MMC_TIMING_UHS_SDR50) ||
		(timing == MMC_TIMING_UHS_SDR104) ||
		(timing == MMC_TIMING_UHS_DDR50) ||
		(timing == MMC_TIMING_UHS_SDR25) ||
		(timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, timing_adj);
		reg &= ~OUTPUT_QSN_PHASE_SELECT;
		sdhci_writel(host, reg, timing_adj);
	}

	/*
	 * If SDIO card, set SDIO Mode
	 * Otherwise, clear SDIO Mode and Slow Mode
	 */
	if (mmc_card_sdio(card)) {
		reg = sdhci_readl(host, timing_adj);
		reg |= TIMING_ADJUST_SDIO_MODE;

		if ((timing == MMC_TIMING_UHS_SDR25) ||
			(timing == MMC_TIMING_UHS_SDR12) ||
			(timing == MMC_TIMING_SD_HS) ||
			(timing == MMC_TIMING_LEGACY))
			reg |= TIMING_ADJUST_SLOW_MODE;

		sdhci_writel(host, reg, timing_adj);
	} else {
		reg = sdhci_readl(host, timing_adj);
		reg &= ~(TIMING_ADJUST_SDIO_MODE | TIMING_ADJUST_SLOW_MODE);
		sdhci_writel(host, reg, timing_adj);
	}

	/*
	 * Set preferred ZNR and ZPR value
	 * The ZNR and ZPR value vary between different boards.
	 * Define them both in sdhci-xenon-emmc-phy.h.
	 */
	reg = sdhci_readl(host, pad_ctrl2);
	reg &= ~((ZNR_MASK << ZNR_SHIFT) | ZPR_MASK);
	reg |= ((params->znr << ZNR_SHIFT) | params->zpr);
	sdhci_writel(host, reg, pad_ctrl2);

	/*
	 * When setting EMMC_PHY_FUNC_CONTROL register,
	 * SD clock should be disabled
	 */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if ((timing == MMC_TIMING_UHS_DDR50) ||
		(timing == MMC_TIMING_MMC_HS400) ||
		(timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, func_ctrl);
		reg |= (DQ_DDR_MODE_MASK << DQ_DDR_MODE_SHIFT) | CMD_DDR_MODE;
		sdhci_writel(host, reg, func_ctrl);
	}

	if (timing == MMC_TIMING_MMC_HS400) {
		reg = sdhci_readl(host, func_ctrl);
		reg &= ~DQ_ASYNC_MODE;
		sdhci_writel(host, reg, func_ctrl);
	}

	/* Enable bus clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if (timing == MMC_TIMING_MMC_HS400)
		/* Hardware team recommend a value for HS400 */
		sdhci_writel(host, LOGIC_TIMING_VALUE, logic_timing_adj);

phy_init:
	xenon_emmc_phy_init(host);

	spin_unlock_irqrestore(&host->lock, flags);

	pr_debug("%s: eMMC PHY setting completes\n", mmc_hostname(host->mmc));
}

/*
 * SDH PHY configuration and operations
 */
static int xenon_sdh_phy_set_fix_sampl_delay(struct sdhci_host *host,
					unsigned int delay, bool invert)
{
	u32 reg;
	unsigned long flags;
	int ret;

	if (invert)
		invert = 0x1;
	else
		invert = 0x0;

	spin_lock_irqsave(&host->lock, flags);

	/* Disable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~(SDHCI_CLOCK_CARD_EN | SDHCI_CLOCK_INT_EN);
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	/* Setup Sampling fix delay */
	reg = sdhci_readl(host, SDHC_SLOT_OP_STATUS_CTRL);
	reg &= ~(SDH_PHY_FIXED_DELAY_MASK |
			(0x1 << FORCE_SEL_INVERSE_CLK_SHIFT));
	reg |= ((delay & SDH_PHY_FIXED_DELAY_MASK) |
			(invert << FORCE_SEL_INVERSE_CLK_SHIFT));
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);

	/* Enable SD internal clock */
	ret = enable_xenon_internal_clk(host);

	/* Enable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	spin_unlock_irqrestore(&host->lock, flags);
	return ret;
}

static int xenon_sdh_phy_do_fix_sampl_delay(struct sdhci_host *host,
		struct mmc_card *card, unsigned int delay, bool invert)
{
	int ret;

	xenon_sdh_phy_set_fix_sampl_delay(host, delay, invert);

	ret = xenon_delay_adj_test(card);
	if (ret) {
		pr_debug("Xenon fail when sampling fix delay = %d, phase = %d degree\n",
				delay, invert * 180);
		return -1;
	}
	return 0;
}

#define SDH_PHY_COARSE_FIX_DELAY		(SDH_PHY_FIXED_DELAY_MASK / 2)
#define SDH_PHY_FINE_FIX_DELAY			(SDH_PHY_COARSE_FIX_DELAY / 4)

static int xenon_sdh_phy_fix_sampl_delay_adj(struct sdhci_host *host,
					struct mmc_card *card)
{
	u32 reg;
	bool dll_enable = false;
	unsigned int min_delay, max_delay, delay;
	const bool sampl_edge[] = {
		false,
		true,
	};
	int i, nr;
	int ret;

	if (host->clock > HIGH_SPEED_MAX_DTR) {
		/* Enable DLL when SDCLK is higher than 50MHz */
		reg = sdhci_readl(host, SDH_PHY_SLOT_DLL_CTRL);
		if (!(reg & SDH_PHY_ENABLE_DLL)) {
			reg |= (SDH_PHY_ENABLE_DLL | SDH_PHY_FAST_LOCK_EN);
			sdhci_writel(host, reg, SDH_PHY_SLOT_DLL_CTRL);
			mdelay(1);

			reg = sdhci_readl(host, SDH_PHY_SLOT_DLL_PHASE_SEL);
			reg |= SDH_PHY_DLL_UPDATE_TUNING;
			sdhci_writel(host, reg, SDH_PHY_SLOT_DLL_PHASE_SEL);
		}
		dll_enable = true;
	}

	nr = dll_enable ? ARRAY_SIZE(sampl_edge) : 1;
	for (i = 0; i < nr; i++) {
		for (min_delay = 0; min_delay <= SDH_PHY_FIXED_DELAY_MASK;
				min_delay += SDH_PHY_COARSE_FIX_DELAY) {
			ret = xenon_sdh_phy_do_fix_sampl_delay(host, card,
						min_delay, sampl_edge[i]);
			if (!ret)
				break;
		}

		if (ret) {
			pr_debug("Fail to set Fixed Sampling Delay with %s edge\n",
				sampl_edge[i] ? "negative" : "positive");
			continue;
		}

		for (max_delay = min_delay + SDH_PHY_FINE_FIX_DELAY;
				max_delay < SDH_PHY_FIXED_DELAY_MASK;
				max_delay += SDH_PHY_FINE_FIX_DELAY) {
			ret = xenon_sdh_phy_do_fix_sampl_delay(host, card,
						max_delay, sampl_edge[i]);
			if (ret) {
				max_delay -= SDH_PHY_FINE_FIX_DELAY;
				break;
			}
		}

		if (!ret) {
			ret = xenon_sdh_phy_do_fix_sampl_delay(host, card,
				SDH_PHY_FIXED_DELAY_MASK, sampl_edge[i]);
			if (!ret)
				max_delay = SDH_PHY_FIXED_DELAY_MASK;
		}

		if ((max_delay - min_delay) <= SDH_PHY_FIXED_DELAY_WINDOW_MIN) {
			pr_info("The window size %d when %s edge cannot meet timing requiremnt\n",
				max_delay - min_delay,
				sampl_edge[i] ? "negative" : "positive");
			continue;
		}

		delay = (min_delay + max_delay) / 2;
		xenon_sdh_phy_set_fix_sampl_delay(host, delay, sampl_edge[i]);
		pr_debug("Xenon sampling fix delay = %d with %s edge\n",
			delay, sampl_edge[i] ? "negative" : "positive");
		return 0;
	}
	return -EIO;
}

static const struct xenon_phy_ops sdh_phy_ops = {
	.fix_sampl_delay_adj = xenon_sdh_phy_fix_sampl_delay_adj,
};

static int alloc_sdh_phy(struct sdhci_xenon_priv *priv)
{
	priv->phy_params = NULL;
	priv->phy_ops = sdh_phy_ops;
	return 0;
}

/*
 * Common functions for all PHYs
 */
void xenon_soc_pad_ctrl(struct sdhci_host *host,
			unsigned char signal_voltage)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	if (priv->phy_ops.soc_pad_ctrl)
		priv->phy_ops.soc_pad_ctrl(host, signal_voltage);
}

static int __xenon_emmc_delay_adj_test(struct mmc_card *card)
{
	int err;
	u8 *ext_csd = NULL;

	err = mmc_get_ext_csd(card, &ext_csd);
	kfree(ext_csd);

	return err;
}

static int __xenon_sdio_delay_adj_test(struct mmc_card *card)
{
	u8 reg;

	return mmc_io_rw_direct(card, 0, 0, 0, 0, &reg);
}

static int __xenon_sd_delay_adj_test(struct mmc_card *card)
{
	return mmc_send_status(card, NULL);
}

static int xenon_delay_adj_test(struct mmc_card *card)
{
	if (mmc_card_mmc(card))
		return __xenon_emmc_delay_adj_test(card);
	else if (mmc_card_sd(card))
		return __xenon_sd_delay_adj_test(card);
	else if (mmc_card_sdio(card))
		return __xenon_sdio_delay_adj_test(card);
	else
		return -EINVAL;
}

static void xenon_phy_set(struct sdhci_host *host,
		struct sdhci_xenon_priv *priv, unsigned char timing)
{
	if (priv->phy_ops.phy_set)
		priv->phy_ops.phy_set(host, timing);
}

static void xenon_hs400_strobe_delay_adj(struct sdhci_host *host,
					struct mmc_card *card,
					struct sdhci_xenon_priv *priv)
{
	WARN_ON(!mmc_card_hs400(card));
	if (!mmc_card_hs400(card))
		return;

	/* Enable the DLL to automatically adjust HS400 strobe delay.
	 */
	if (priv->phy_ops.strobe_delay_adj)
		priv->phy_ops.strobe_delay_adj(host, card);
}

static int xenon_fix_sampl_delay_adj(struct sdhci_host *host,
				struct mmc_card *card,
				struct sdhci_xenon_priv *priv)
{
	if (priv->phy_ops.fix_sampl_delay_adj)
		return priv->phy_ops.fix_sampl_delay_adj(host, card);

	return 0;
}

static void xenon_phy_config_tuning(struct sdhci_host *host,
				struct sdhci_xenon_priv *priv)
{
	if (priv->phy_ops.config_tuning)
		return priv->phy_ops.config_tuning(host);
}

/*
 * xenon_delay_adj should not be called inside IRQ context,
 * either Hard IRQ or Softirq.
 */
static int xenon_hs_delay_adj(struct mmc_host *mmc, struct mmc_card *card,
			struct sdhci_xenon_priv *priv)
{
	int ret = 0;
	struct sdhci_host *host = mmc_priv(mmc);
	struct emmc_phy_params *params = priv->phy_params;

	WARN_ON(host->clock <= DEFAULT_SDCLK_FREQ);
	if (host->clock <= DEFAULT_SDCLK_FREQ)
		return -EINVAL;

	if (mmc_card_hs400(card)) {
		xenon_hs400_strobe_delay_adj(host, card, priv);
		return 0;
	}

	if (!params->no_dll_tuning && ((priv->phy_type == EMMC_5_1_PHY) ||
		(priv->phy_type == EMMC_5_0_PHY)) &&
		(mmc_card_hs200(card) ||
		(host->timing == MMC_TIMING_UHS_SDR104))) {
		xenon_phy_config_tuning(host, priv);
		return 0;
	}

	ret = xenon_fix_sampl_delay_adj(host, card, priv);
	if (ret)
		pr_err("%s: fails sampling fixed delay adjustment\n",
			mmc_hostname(mmc));
	return ret;
}

int xenon_phy_adj(struct sdhci_host *host, struct mmc_ios *ios)
{
	struct mmc_host *mmc = host->mmc;
	struct mmc_card *card;
	int ret = 0;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	if (!host->clock)
		return 0;

	if ((ios->timing != priv->timing) || (ios->clock != priv->clock))
		xenon_phy_set(host, priv, ios->timing);

	/* Legacy mode is a special case */
	if (ios->timing == MMC_TIMING_LEGACY) {
		priv->timing = ios->timing;
		return 0;
	}

	/*
	 * The timing, frequency or bus width is changed,
	 * better to set eMMC PHY based on current setting
	 * and adjust Xenon SDHC delay.
	 */
	if ((host->clock == priv->clock) &&
		(ios->bus_width == priv->bus_width) &&
		(ios->timing == priv->timing))
		return 0;

	/* Update the record */
	priv->bus_width = ios->bus_width;
	/* Temp stage from HS200 to HS400 */
	if (((priv->timing == MMC_TIMING_MMC_HS200) &&
		(ios->timing == MMC_TIMING_MMC_HS)) ||
		((priv->timing == MMC_TIMING_MMC_HS200) &&
		(priv->clock > host->clock))) {
		priv->timing = ios->timing;
		priv->clock = host->clock;
		return 0;
	}
	priv->timing = ios->timing;
	priv->clock = host->clock;

	card = priv->card_candidate;
	if (unlikely(card == NULL)) {
		WARN("%s: card is not present\n", mmc_hostname(mmc));
		return -EINVAL;
	}

	if (host->clock > DEFAULT_SDCLK_FREQ)
		ret = xenon_hs_delay_adj(mmc, card, priv);
	return ret;
}

static int add_xenon_phy(struct device_node *np, struct sdhci_xenon_priv *priv,
			const char *phy_name)
{
	int i, ret;

	for (i = 0; i < NR_PHY_TYPES; i++) {
		if (!strcmp(phy_name, phy_types[i])) {
			priv->phy_type = i;
			break;
		}
	}
	if (i == NR_PHY_TYPES) {
		pr_err("Unable to determine PHY name %s. Use default eMMC 5.1 PHY\n",
			phy_name);
		priv->phy_type = EMMC_5_1_PHY;
	}

	if (priv->phy_type == SDH_PHY)
		return alloc_sdh_phy(priv);
	else if ((priv->phy_type == EMMC_5_0_PHY) ||
			(priv->phy_type == EMMC_5_1_PHY)) {
		ret = alloc_emmc_phy(priv);
		if (ret)
			return ret;
		return emmc_phy_parse_param_dt(np, priv->phy_params);
	}

	return -EINVAL;
}

int xenon_phy_parse_dt(struct device_node *np, struct sdhci_xenon_priv *priv)
{
	const char *phy_type = NULL;

	if (!of_property_read_string(np, "xenon,phy-type", &phy_type))
		return add_xenon_phy(np, priv, phy_type);

	pr_err("Fail to get Xenon PHY type. Use default eMMC 5.1 PHY\n");
	return add_xenon_phy(np, priv, "emmc 5.1 phy");
}
