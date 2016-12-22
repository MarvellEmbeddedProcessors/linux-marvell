/*
 * PHY support for Xenon SDHC
 *
 * Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * Author:	Hu Ziji <huziji@marvell.com>
 * Date:	2016-8-24
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/of_address.h>

#include "sdhci-pltfm.h"
#include "sdhci-xenon.h"

/* Register base for eMMC PHY 5.0 Version */
#define SDHCI_EMMC_5_0_PHY_REG_BASE		0x0160
/* Register base for eMMC PHY 5.1 Version */
#define SDHCI_EMMC_PHY_REG_BASE			0x0170

#define SDHCI_EMMC_PHY_TIMING_ADJUST		SDHCI_EMMC_PHY_REG_BASE
#define SDHCI_EMMC_5_0_PHY_TIMING_ADJUST	SDHCI_EMMC_5_0_PHY_REG_BASE
#define SDHCI_TIMING_ADJUST_SLOW_MODE		BIT(29)
#define SDHCI_TIMING_ADJUST_SDIO_MODE		BIT(28)
#define SDHCI_OUTPUT_QSN_PHASE_SELECT		BIT(17)
#define SDHCI_SAMPL_INV_QSP_PHASE_SELECT	BIT(18)
#define SDHCI_SAMPL_INV_QSP_PHASE_SELECT_SHIFT	18
#define SDHCI_PHY_INITIALIZAION			BIT(31)
#define SDHCI_WAIT_CYCLE_BEFORE_USING_MASK	0xF
#define SDHCI_WAIT_CYCLE_BEFORE_USING_SHIFT	12
#define SDHCI_FC_SYNC_EN_DURATION_MASK		0xF
#define SDHCI_FC_SYNC_EN_DURATION_SHIFT		8
#define SDHCI_FC_SYNC_RST_EN_DURATION_MASK	0xF
#define SDHCI_FC_SYNC_RST_EN_DURATION_SHIFT	4
#define SDHCI_FC_SYNC_RST_DURATION_MASK		0xF
#define SDHCI_FC_SYNC_RST_DURATION_SHIFT	0

#define SDHCI_EMMC_PHY_FUNC_CONTROL		(SDHCI_EMMC_PHY_REG_BASE + 0x4)
#define SDHCI_EMMC_5_0_PHY_FUNC_CONTROL		\
	(SDHCI_EMMC_5_0_PHY_REG_BASE + 0x4)
#define SDHCI_ASYNC_DDRMODE_MASK		BIT(23)
#define SDHCI_ASYNC_DDRMODE_SHIFT		23
#define SDHCI_CMD_DDR_MODE			BIT(16)
#define SDHCI_DQ_DDR_MODE_SHIFT			8
#define SDHCI_DQ_DDR_MODE_MASK			0xFF
#define SDHCI_DQ_ASYNC_MODE			BIT(4)

#define SDHCI_EMMC_PHY_PAD_CONTROL		(SDHCI_EMMC_PHY_REG_BASE + 0x8)
#define SDHCI_EMMC_5_0_PHY_PAD_CONTROL		\
	(SDHCI_EMMC_5_0_PHY_REG_BASE + 0x8)
#define SDHCI_REC_EN_SHIFT			24
#define SDHCI_REC_EN_MASK			0xF
#define SDHCI_FC_DQ_RECEN			BIT(24)
#define SDHCI_FC_CMD_RECEN			BIT(25)
#define SDHCI_FC_QSP_RECEN			BIT(26)
#define SDHCI_FC_QSN_RECEN			BIT(27)
#define SDHCI_OEN_QSN				BIT(28)
#define SDHCI_AUTO_RECEN_CTRL			BIT(30)
#define SDHCI_FC_ALL_CMOS_RECEIVER		0xF000

#define SDHCI_EMMC5_FC_QSP_PD			BIT(18)
#define SDHCI_EMMC5_FC_QSP_PU			BIT(22)
#define SDHCI_EMMC5_FC_CMD_PD			BIT(17)
#define SDHCI_EMMC5_FC_CMD_PU			BIT(21)
#define SDHCI_EMMC5_FC_DQ_PD			BIT(16)
#define SDHCI_EMMC5_FC_DQ_PU			BIT(20)

#define SDHCI_EMMC_PHY_PAD_CONTROL1		(SDHCI_EMMC_PHY_REG_BASE + 0xC)
#define SDHCI_EMMC5_1_FC_QSP_PD			BIT(9)
#define SDHCI_EMMC5_1_FC_QSP_PU			BIT(25)
#define SDHCI_EMMC5_1_FC_CMD_PD			BIT(8)
#define SDHCI_EMMC5_1_FC_CMD_PU			BIT(24)
#define SDHCI_EMMC5_1_FC_DQ_PD			0xFF
#define SDHCI_EMMC5_1_FC_DQ_PU			(0xFF << 16)

#define SDHCI_EMMC_PHY_PAD_CONTROL2		(SDHCI_EMMC_PHY_REG_BASE + 0x10)
#define SDHCI_EMMC_5_0_PHY_PAD_CONTROL2		\
	(SDHCI_EMMC_5_0_PHY_REG_BASE + 0xC)
#define SDHCI_ZNR_MASK				0x1F
#define SDHCI_ZNR_SHIFT				8
#define SDHCI_ZPR_MASK				0x1F
/* Perferred ZNR and ZPR value vary between different boards.
 * The specific ZNR and ZPR value should be defined here
 * according to board actual timing.
 */
#define SDHCI_ZNR_DEF_VALUE			0xF
#define SDHCI_ZPR_DEF_VALUE			0xF

#define SDHCI_EMMC_PHY_DLL_CONTROL		(SDHCI_EMMC_PHY_REG_BASE + 0x14)
#define SDHCI_EMMC_5_0_PHY_DLL_CONTROL		\
	(SDHCI_EMMC_5_0_PHY_REG_BASE + 0x10)
#define SDHCI_DLL_ENABLE			BIT(31)
#define SDHCI_DLL_UPDATE_STROBE_5_0		BIT(30)
#define SDHCI_DLL_REFCLK_SEL			BIT(30)
#define SDHCI_DLL_UPDATE			BIT(23)
#define SDHCI_DLL_PHSEL1_SHIFT			24
#define SDHCI_DLL_PHSEL0_SHIFT			16
#define SDHCI_DLL_PHASE_MASK			0x3F
#define SDHCI_DLL_PHASE_90_DEGREE		0x1F
#define SDHCI_DLL_FAST_LOCK			BIT(5)
#define SDHCI_DLL_GAIN2X			BIT(3)
#define SDHCI_DLL_BYPASS_EN			BIT(0)

#define SDHCI_EMMC_5_0_PHY_LOGIC_TIMING_ADJUST	\
	(SDHCI_EMMC_5_0_PHY_REG_BASE + 0x14)
#define SDHCI_EMMC_PHY_LOGIC_TIMING_ADJUST	(SDHCI_EMMC_PHY_REG_BASE + 0x18)
#define SDHCI_LOGIC_TIMING_VALUE		0x00AA8977

enum soc_pad_ctrl_type {
	SOC_PAD_SD,
	SOC_PAD_FIXED_1_8V,
};

/*
 * List offset of PHY registers and some special register values
 * in eMMC PHY 5.0 or eMMC PHY 5.1
 */
struct xenon_emmc_phy_regs {
	/* Offset of Timing Adjust register */
	u16 timing_adj;
	/* Offset of Func Control register */
	u16 func_ctrl;
	/* Offset of Pad Control register */
	u16 pad_ctrl;
	/* Offset of Pad Control register 2 */
	u16 pad_ctrl2;
	/* Offset of DLL Control register */
	u16 dll_ctrl;
	/* Offset of Logic Timing Adjust register */
	u16 logic_timing_adj;
	/* DLL Update Enable bit */
	u32 dll_update;
};

static const char * const phy_types[] = {
	"emmc 5.0 phy",
	"emmc 5.1 phy"
};

enum phy_type_enum {
	EMMC_5_0_PHY,
	EMMC_5_1_PHY,
	NR_PHY_TYPES
};

struct soc_pad_ctrl_table {
	const char *soc;
	void (*set_soc_pad)(struct sdhci_host *host,
			    unsigned char signal_voltage);
};

struct soc_pad_ctrl {
	/* Register address of SOC PHY PAD ctrl */
	void __iomem	*reg;
	/* SOC PHY PAD ctrl type */
	enum soc_pad_ctrl_type pad_type;
	/* SOC specific operation to set SOC PHY PAD */
	void (*set_soc_pad)(struct sdhci_host *host,
			    unsigned char signal_voltage);
};

static struct xenon_emmc_phy_regs xenon_emmc_5_0_phy_regs = {
	.timing_adj	= SDHCI_EMMC_5_0_PHY_TIMING_ADJUST,
	.func_ctrl	= SDHCI_EMMC_5_0_PHY_FUNC_CONTROL,
	.pad_ctrl	= SDHCI_EMMC_5_0_PHY_PAD_CONTROL,
	.pad_ctrl2	= SDHCI_EMMC_5_0_PHY_PAD_CONTROL2,
	.dll_ctrl	= SDHCI_EMMC_5_0_PHY_DLL_CONTROL,
	.logic_timing_adj = SDHCI_EMMC_5_0_PHY_LOGIC_TIMING_ADJUST,
	.dll_update	= SDHCI_DLL_UPDATE_STROBE_5_0,
};

static struct xenon_emmc_phy_regs xenon_emmc_5_1_phy_regs = {
	.timing_adj	= SDHCI_EMMC_PHY_TIMING_ADJUST,
	.func_ctrl	= SDHCI_EMMC_PHY_FUNC_CONTROL,
	.pad_ctrl	= SDHCI_EMMC_PHY_PAD_CONTROL,
	.pad_ctrl2	= SDHCI_EMMC_PHY_PAD_CONTROL2,
	.dll_ctrl	= SDHCI_EMMC_PHY_DLL_CONTROL,
	.logic_timing_adj = SDHCI_EMMC_PHY_LOGIC_TIMING_ADJUST,
	.dll_update	= SDHCI_DLL_UPDATE,
};

/*
 * eMMC PHY configuration and operations
 */
struct emmc_phy_params {
	bool	slow_mode;

	u8	znr;
	u8	zpr;

	/* Nr of consecutive Sampling Points of a Valid Sampling Window */
	u8	nr_tun_times;
	/* Divider for calculating Tuning Step */
	u8	tun_step_divider;

	struct soc_pad_ctrl pad_ctrl;
};

static int alloc_emmc_phy(struct sdhci_xenon_priv *priv)
{
	struct emmc_phy_params *params;

	params = kzalloc(sizeof(*params), GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	priv->phy_params = params;
	if (priv->phy_type == EMMC_5_0_PHY)
		priv->emmc_phy_regs = &xenon_emmc_5_0_phy_regs;
	else
		priv->emmc_phy_regs = &xenon_emmc_5_1_phy_regs;

	return 0;
}

/*
 * eMMC 5.0/5.1 PHY init/re-init.
 * eMMC PHY init should be executed after:
 * 1. SDCLK frequecny changes.
 * 2. SDCLK is stopped and re-enabled.
 * 3. config in emmc_phy_regs->timing_adj and emmc_phy_regs->func_ctrl
 * are changed
 */
static int emmc_phy_init(struct sdhci_host *host)
{
	u32 reg;
	u32 wait, clock;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct xenon_emmc_phy_regs *phy_regs = priv->emmc_phy_regs;

	reg = sdhci_readl(host, phy_regs->timing_adj);
	reg |= SDHCI_PHY_INITIALIZAION;
	sdhci_writel(host, reg, phy_regs->timing_adj);

	/* Add duration of FC_SYNC_RST */
	wait = ((reg >> SDHCI_FC_SYNC_RST_DURATION_SHIFT) &
			SDHCI_FC_SYNC_RST_DURATION_MASK);
	/* Add interval between FC_SYNC_EN and FC_SYNC_RST */
	wait += ((reg >> SDHCI_FC_SYNC_RST_EN_DURATION_SHIFT) &
			SDHCI_FC_SYNC_RST_EN_DURATION_MASK);
	/* Add duration of asserting FC_SYNC_EN */
	wait += ((reg >> SDHCI_FC_SYNC_EN_DURATION_SHIFT) &
			SDHCI_FC_SYNC_EN_DURATION_MASK);
	/* Add duration of waiting for PHY */
	wait += ((reg >> SDHCI_WAIT_CYCLE_BEFORE_USING_SHIFT) &
			SDHCI_WAIT_CYCLE_BEFORE_USING_MASK);
	/* 4 addtional bus clock and 4 AXI bus clock are required */
	wait += 8;
	wait <<= 20;

	clock = host->clock;
	if (!clock)
		/* Use the possibly slowest bus frequency value */
		clock = SDHCI_LOWEST_SDCLK_FREQ;
	/* get the wait time */
	wait /= clock;
	wait++;
	/* wait for host eMMC PHY init completes */
	udelay(wait);

	reg = sdhci_readl(host, phy_regs->timing_adj);
	reg &= SDHCI_PHY_INITIALIZAION;
	if (reg) {
		dev_err(mmc_dev(host->mmc), "eMMC PHY init cannot complete after %d us\n",
			wait);
		return -ETIMEDOUT;
	}

	return 0;
}

#define ARMADA_3700_SOC_PAD_1_8V	0x1
#define ARMADA_3700_SOC_PAD_3_3V	0x0

static void armada_3700_soc_pad_voltage_set(struct sdhci_host *host,
					    unsigned char signal_voltage)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;

	if (params->pad_ctrl.pad_type == SOC_PAD_FIXED_1_8V) {
		writel(ARMADA_3700_SOC_PAD_1_8V, params->pad_ctrl.reg);
	} else if (params->pad_ctrl.pad_type == SOC_PAD_SD) {
		if (signal_voltage == MMC_SIGNAL_VOLTAGE_180)
			writel(ARMADA_3700_SOC_PAD_1_8V, params->pad_ctrl.reg);
		else if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
			writel(ARMADA_3700_SOC_PAD_3_3V, params->pad_ctrl.reg);
	}
}

/*
 * Set SOC PHY voltage PAD control register,
 * according to the operation voltage on PAD.
 * The detailed operation depends on SOC implementaion.
 */
static void emmc_phy_set_soc_pad(struct sdhci_host *host,
				 unsigned char signal_voltage)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;

	if (!params->pad_ctrl.reg)
		return;

	if (params->pad_ctrl.set_soc_pad)
		params->pad_ctrl.set_soc_pad(host, signal_voltage);
}

/*
 * Enable eMMC PHY HW DLL
 * DLL should be enabled and stable before HS200/SDR104 tuning,
 * and before HS400 data strobe setting.
 */
static int emmc_phy_enable_dll(struct sdhci_host *host)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct xenon_emmc_phy_regs *phy_regs = priv->emmc_phy_regs;
	u8 timeout;

	if (WARN_ON(host->clock <= MMC_HIGH_52_MAX_DTR))
		return -EINVAL;

	reg = sdhci_readl(host, phy_regs->dll_ctrl);
	if (reg & SDHCI_DLL_ENABLE)
		return 0;

	/* Enable DLL */
	reg = sdhci_readl(host, phy_regs->dll_ctrl);
	reg |= (SDHCI_DLL_ENABLE | SDHCI_DLL_FAST_LOCK);

	/*
	 * Set Phase as 90 degree, which is most common value.
	 * Might set another value if necessary.
	 * The granularity is 1 degree.
	 */
	reg &= ~((SDHCI_DLL_PHASE_MASK << SDHCI_DLL_PHSEL0_SHIFT) |
		 (SDHCI_DLL_PHASE_MASK << SDHCI_DLL_PHSEL1_SHIFT));
	reg |= ((SDHCI_DLL_PHASE_90_DEGREE << SDHCI_DLL_PHSEL0_SHIFT) |
		(SDHCI_DLL_PHASE_90_DEGREE << SDHCI_DLL_PHSEL1_SHIFT));

	reg &= ~SDHCI_DLL_BYPASS_EN;
	reg |= phy_regs->dll_update;
	if (priv->phy_type == EMMC_5_1_PHY)
		reg &= ~SDHCI_DLL_REFCLK_SEL;
	sdhci_writel(host, reg, phy_regs->dll_ctrl);

	/* Wait max 32 ms */
	timeout = 32;
	while (!(sdhci_readw(host, SDHCI_SLOT_EXT_PRESENT_STATE) &
		SDHCI_DLL_LOCK_STATE)) {
		if (!timeout) {
			dev_err(mmc_dev(host->mmc), "Wait for DLL Lock time-out\n");
			return -ETIMEDOUT;
		}
		timeout--;
		mdelay(1);
	}
	return 0;
}

/*
 * Config to eMMC PHY to prepare for tuning.
 * Enable HW DLL and set the TUNING_STEP
 */
static int emmc_phy_config_tuning(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;
	u32 reg, tuning_step;
	int ret;
	unsigned long flags;

	if (WARN_ON(host->clock <= MMC_HIGH_52_MAX_DTR))
		return -EINVAL;

	spin_lock_irqsave(&host->lock, flags);

	ret = emmc_phy_enable_dll(host);
	if (ret) {
		spin_unlock_irqrestore(&host->lock, flags);
		return ret;
	}

	/* Achieve TUNGING_STEP with HW DLL help */
	reg = sdhci_readl(host, SDHCI_SLOT_DLL_CUR_DLY_VAL);
	tuning_step = reg / params->tun_step_divider;
	if (unlikely(tuning_step > SDHCI_TUNING_STEP_MASK)) {
		dev_warn(mmc_dev(host->mmc),
			 "HS200 TUNING_STEP %d is larger than MAX value\n",
			 tuning_step);
		tuning_step = SDHCI_TUNING_STEP_MASK;
	}

	/* Set TUNING_STEP for later tuning */
	reg = sdhci_readl(host, SDHCI_SLOT_OP_STATUS_CTRL);
	reg &= ~(SDHCI_TUN_CONSECUTIVE_TIMES_MASK <<
		 SDHCI_TUN_CONSECUTIVE_TIMES_SHIFT);
	reg |= (params->nr_tun_times << SDHCI_TUN_CONSECUTIVE_TIMES_SHIFT);
	reg &= ~(SDHCI_TUNING_STEP_MASK << SDHCI_TUNING_STEP_SHIFT);
	reg |= (tuning_step << SDHCI_TUNING_STEP_SHIFT);
	sdhci_writel(host, reg, SDHCI_SLOT_OP_STATUS_CTRL);

	spin_unlock_irqrestore(&host->lock, flags);
	return 0;
}

static void __emmc_phy_disable_data_strobe(struct sdhci_host *host)
{
	u32 reg;

	/* Disable SDHC Data Strobe */
	reg = sdhci_readl(host, SDHCI_SLOT_EMMC_CTRL);
	reg &= ~SDHCI_ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHCI_SLOT_EMMC_CTRL);
}

/* Set HS400 Data Strobe */
static void emmc_phy_strobe_delay_adj(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	unsigned long flags;
	u32 reg;

	if (WARN_ON(host->timing != MMC_TIMING_MMC_HS400))
		return;

	if (host->clock <= MMC_HIGH_52_MAX_DTR)
		return;

	dev_dbg(mmc_dev(host->mmc), "starts HS400 strobe delay adjustment\n");

	spin_lock_irqsave(&host->lock, flags);

	emmc_phy_enable_dll(host);

	/* Enable SDHC Data Strobe */
	reg = sdhci_readl(host, SDHCI_SLOT_EMMC_CTRL);
	reg |= SDHCI_ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHCI_SLOT_EMMC_CTRL);

	/* Set Data Strobe Pull down */
	if (priv->phy_type == EMMC_5_0_PHY) {
		reg = sdhci_readl(host, SDHCI_EMMC_5_0_PHY_PAD_CONTROL);
		reg |= SDHCI_EMMC5_FC_QSP_PD;
		reg &= ~SDHCI_EMMC5_FC_QSP_PU;
		sdhci_writel(host, reg, SDHCI_EMMC_5_0_PHY_PAD_CONTROL);
	} else {
		reg = sdhci_readl(host, SDHCI_EMMC_PHY_PAD_CONTROL1);
		reg |= SDHCI_EMMC5_1_FC_QSP_PD;
		reg &= ~SDHCI_EMMC5_1_FC_QSP_PU;
		sdhci_writel(host, reg, SDHCI_EMMC_PHY_PAD_CONTROL1);
	}
	spin_unlock_irqrestore(&host->lock, flags);
}

static inline bool temp_stage_hs200_to_hs400(struct sdhci_host *host,
					     struct sdhci_xenon_priv *priv)
{
	/*
	 * Tmep stages from HS200 to HS400
	 * from HS200 to HS in 200MHz
	 * from 200MHz to 52MHz
	 */
	if (((priv->timing == MMC_TIMING_MMC_HS200) &&
	     (host->timing == MMC_TIMING_MMC_HS)) ||
	    ((host->timing == MMC_TIMING_MMC_HS) &&
	     (priv->clock > host->clock)))
		return true;

	return false;
}

static inline bool temp_stage_hs400_to_h200(struct sdhci_host *host,
					    struct sdhci_xenon_priv *priv)
{
	/*
	 * Temp stages from HS400 t0 HS200:
	 * from 200MHz to 52MHz in HS400
	 * from HS400 to HS DDR in 52MHz
	 * from HS DDR to HS in 52MHz
	 * from HS to HS200 in 52MHz
	 */
	if (((priv->timing == MMC_TIMING_MMC_HS400) &&
	     ((host->clock == MMC_HIGH_52_MAX_DTR) ||
	      (host->timing == MMC_TIMING_MMC_DDR52))) ||
	    ((priv->timing == MMC_TIMING_MMC_DDR52) &&
	     (host->timing == MMC_TIMING_MMC_HS)) ||
	    ((host->timing == MMC_TIMING_MMC_HS200) &&
	     (host->clock == MMC_HIGH_52_MAX_DTR)))
		return true;

	return false;
}

/*
 * If eMMC PHY Slow Mode is required in lower speed mode in SDR mode
 * (SDLCK < 55MHz), enable Slow Mode to bypass eMMC PHY.
 * SDIO slower SDR mode also requires Slow Mode.
 *
 * If Slow Mode is enabled, return true.
 * Otherwise, return false.
 */
static bool emmc_phy_slow_mode(struct sdhci_host *host,
			       unsigned char timing)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;
	struct xenon_emmc_phy_regs *phy_regs = priv->emmc_phy_regs;
	u32 reg;

	/* Skip temp stages from HS200 to HS400 */
	if (temp_stage_hs200_to_hs400(host, priv))
		return false;

	/* Skip temp stages from HS400 t0 HS200 */
	if (temp_stage_hs400_to_h200(host, priv))
		return false;

	reg = sdhci_readl(host, phy_regs->timing_adj);
	/* Enable Slow Mode for SDIO in slower SDR mode */
	if ((priv->init_card_type == MMC_TYPE_SDIO) &&
	    ((timing == MMC_TIMING_UHS_SDR25) ||
	     (timing == MMC_TIMING_UHS_SDR12) ||
	     (timing == MMC_TIMING_SD_HS) ||
	     (timing == MMC_TIMING_LEGACY))) {
		reg |= SDHCI_TIMING_ADJUST_SLOW_MODE;
		sdhci_writel(host, reg, phy_regs->timing_adj);
		return true;
	}

	/* Check if Slow Mode is required in lower speed mode in SDR mode */
	if (((timing == MMC_TIMING_UHS_SDR50) ||
	     (timing == MMC_TIMING_UHS_SDR25) ||
	     (timing == MMC_TIMING_UHS_SDR12) ||
	     (timing == MMC_TIMING_SD_HS) ||
	     (timing == MMC_TIMING_MMC_HS) ||
	     (timing == MMC_TIMING_LEGACY)) && params->slow_mode) {
		reg |= SDHCI_TIMING_ADJUST_SLOW_MODE;
		sdhci_writel(host, reg, phy_regs->timing_adj);
		return true;
	}

	reg &= ~SDHCI_TIMING_ADJUST_SLOW_MODE;
	sdhci_writel(host, reg, phy_regs->timing_adj);
	return false;
}

/*
 * Set-up eMMC 5.0/5.1 PHY.
 * Specific onfiguration depends on the current speed mode in use.
 */
static void emmc_phy_set(struct sdhci_host *host,
			 unsigned char timing)
{
	u32 reg;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	struct emmc_phy_params *params = priv->phy_params;
	struct xenon_emmc_phy_regs *phy_regs = priv->emmc_phy_regs;
	unsigned long flags;

	dev_dbg(mmc_dev(host->mmc), "eMMC PHY setting starts\n");

	spin_lock_irqsave(&host->lock, flags);

	reg = sdhci_readl(host, SDHCI_SYS_EXT_OP_CTRL);
	reg |= SDHCI_MASK_CMD_CONFLICT_ERROR;
	sdhci_writel(host, reg, SDHCI_SYS_EXT_OP_CTRL);

	/* Setup pad, set bit[28] and bits[26:24] */
	reg = sdhci_readl(host, phy_regs->pad_ctrl);
	reg |= (SDHCI_FC_DQ_RECEN | SDHCI_FC_CMD_RECEN |
		SDHCI_FC_QSP_RECEN | SDHCI_OEN_QSN);
	/* All FC_XX_RECEIVCE should be set as CMOS Type */
	reg |= SDHCI_FC_ALL_CMOS_RECEIVER;
	sdhci_writel(host, reg, phy_regs->pad_ctrl);

	/* Set CMD and DQ Pull Up */
	if (priv->phy_type == EMMC_5_0_PHY) {
		reg = sdhci_readl(host, SDHCI_EMMC_5_0_PHY_PAD_CONTROL);
		reg |= (SDHCI_EMMC5_FC_CMD_PU | SDHCI_EMMC5_FC_DQ_PU);
		reg &= ~(SDHCI_EMMC5_FC_CMD_PD | SDHCI_EMMC5_FC_DQ_PD);
		sdhci_writel(host, reg, SDHCI_EMMC_5_0_PHY_PAD_CONTROL);
	} else {
		reg = sdhci_readl(host, SDHCI_EMMC_PHY_PAD_CONTROL1);
		reg |= (SDHCI_EMMC5_1_FC_CMD_PU | SDHCI_EMMC5_1_FC_DQ_PU);
		reg &= ~(SDHCI_EMMC5_1_FC_CMD_PD | SDHCI_EMMC5_1_FC_DQ_PD);
		sdhci_writel(host, reg, SDHCI_EMMC_PHY_PAD_CONTROL1);
	}

	if (timing == MMC_TIMING_LEGACY)
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
		reg = sdhci_readl(host, phy_regs->timing_adj);
		reg &= ~SDHCI_OUTPUT_QSN_PHASE_SELECT;
		sdhci_writel(host, reg, phy_regs->timing_adj);
	}

	/*
	 * If SDIO card, set SDIO Mode
	 * Otherwise, clear SDIO Mode
	 */
	reg = sdhci_readl(host, phy_regs->timing_adj);
	if (priv->init_card_type == MMC_TYPE_SDIO)
		reg |= SDHCI_TIMING_ADJUST_SDIO_MODE;
	else
		reg &= ~SDHCI_TIMING_ADJUST_SDIO_MODE;
	sdhci_writel(host, reg, phy_regs->timing_adj);

	if (emmc_phy_slow_mode(host, timing))
		goto phy_init;

	/*
	 * Set preferred ZNR and ZPR value
	 * The ZNR and ZPR value vary between different boards.
	 * Define them both in sdhci-xenon-emmc-phy.h.
	 */
	reg = sdhci_readl(host, phy_regs->pad_ctrl2);
	reg &= ~((SDHCI_ZNR_MASK << SDHCI_ZNR_SHIFT) | SDHCI_ZPR_MASK);
	reg |= ((params->znr << SDHCI_ZNR_SHIFT) | params->zpr);
	sdhci_writel(host, reg, phy_regs->pad_ctrl2);

	/*
	 * When setting EMMC_PHY_FUNC_CONTROL register,
	 * SD clock should be disabled
	 */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	reg = sdhci_readl(host, phy_regs->func_ctrl);
	if ((timing == MMC_TIMING_UHS_DDR50) ||
	    (timing == MMC_TIMING_MMC_HS400) ||
	    (timing == MMC_TIMING_MMC_DDR52))
		reg |= (SDHCI_DQ_DDR_MODE_MASK << SDHCI_DQ_DDR_MODE_SHIFT) |
		       SDHCI_CMD_DDR_MODE;
	else
		reg &= ~((SDHCI_DQ_DDR_MODE_MASK << SDHCI_DQ_DDR_MODE_SHIFT) |
			 SDHCI_CMD_DDR_MODE);

	if (timing == MMC_TIMING_MMC_HS400)
		reg &= ~SDHCI_DQ_ASYNC_MODE;
	else
		reg |= SDHCI_DQ_ASYNC_MODE;
	sdhci_writel(host, reg, phy_regs->func_ctrl);

	/* Enable bus clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if (timing == MMC_TIMING_MMC_HS400)
		/* Hardware team recommend a value for HS400 */
		sdhci_writel(host, SDHCI_LOGIC_TIMING_VALUE,
			     phy_regs->logic_timing_adj);
	else
		__emmc_phy_disable_data_strobe(host);

phy_init:
	emmc_phy_init(host);

	spin_unlock_irqrestore(&host->lock, flags);

	dev_dbg(mmc_dev(host->mmc), "eMMC PHY setting completes\n");
}

static int get_dt_pad_ctrl_data(struct sdhci_host *host,
				struct device_node *np,
				struct emmc_phy_params *params)
{
	int ret = 0;
	const char *name;
	struct resource iomem;

	if (of_device_is_compatible(np, "marvell,armada-3700-sdhci"))
		params->pad_ctrl.set_soc_pad = armada_3700_soc_pad_voltage_set;
	else
		return 0;

	if (of_address_to_resource(np, 1, &iomem)) {
		dev_err(mmc_dev(host->mmc), "Unable to find SOC PAD ctrl register address for %s\n",
			np->name);
		return -EINVAL;
	}

	params->pad_ctrl.reg = devm_ioremap_resource(mmc_dev(host->mmc),
						     &iomem);
	if (IS_ERR(params->pad_ctrl.reg)) {
		dev_err(mmc_dev(host->mmc), "Unable to get SOC PHY PAD ctrl regiser for %s\n",
			np->name);
		return PTR_ERR(params->pad_ctrl.reg);
	}

	ret = of_property_read_string(np, "marvell,pad-type", &name);
	if (ret) {
		dev_err(mmc_dev(host->mmc), "Unable to determine SOC PHY PAD ctrl type\n");
		return ret;
	}
	if (!strcmp(name, "sd")) {
		params->pad_ctrl.pad_type = SOC_PAD_SD;
	} else if (!strcmp(name, "fixed-1-8v")) {
		params->pad_ctrl.pad_type = SOC_PAD_FIXED_1_8V;
	} else {
		dev_err(mmc_dev(host->mmc), "Unsupported SOC PHY PAD ctrl type %s\n",
			name);
		return -EINVAL;
	}

	return ret;
}

static int emmc_phy_parse_param_dt(struct sdhci_host *host,
				   struct device_node *np,
				   struct emmc_phy_params *params)
{
	u32 value;

	if (of_property_read_bool(np, "marvell,xenon-phy-slow-mode"))
		params->slow_mode = true;
	else
		params->slow_mode = false;

	if (!of_property_read_u32(np, "marvell,xenon-phy-znr", &value))
		params->znr = value & SDHCI_ZNR_MASK;
	else
		params->znr = SDHCI_ZNR_DEF_VALUE;

	if (!of_property_read_u32(np, "marvell,xenon-phy-zpr", &value))
		params->zpr = value & SDHCI_ZPR_MASK;
	else
		params->zpr = SDHCI_ZPR_DEF_VALUE;

	if (!of_property_read_u32(np, "marvell,xenon-phy-nr-success-tun",
				  &value))
		params->nr_tun_times = value & SDHCI_TUN_CONSECUTIVE_TIMES_MASK;
	else
		params->nr_tun_times = SDHCI_TUN_CONSECUTIVE_TIMES;

	if (!of_property_read_u32(np, "marvell,xenon-phy-tun-step-divider",
				  &value))
		params->tun_step_divider = value & 0xFF;
	else
		params->tun_step_divider = SDHCI_TUNING_STEP_DIVIDER;

	return get_dt_pad_ctrl_data(host, np, params);
}

/* Set SOC PHY Voltage PAD */
void xenon_soc_pad_ctrl(struct sdhci_host *host,
			unsigned char signal_voltage)
{
	emmc_phy_set_soc_pad(host, signal_voltage);
}

/*
 * Setting PHY when card is working in High Speed Mode.
 * HS400 set data strobe line.
 * HS200/SDR104 set tuning config to prepare for tuning.
 */
static int xenon_hs_delay_adj(struct sdhci_host *host)
{
	int ret = 0;

	if (WARN_ON(host->clock <= SDHCI_DEFAULT_SDCLK_FREQ))
		return -EINVAL;

	if (host->timing == MMC_TIMING_MMC_HS400) {
		emmc_phy_strobe_delay_adj(host);
		return 0;
	}

	if ((host->timing == MMC_TIMING_MMC_HS200) ||
	    (host->timing == MMC_TIMING_UHS_SDR104)) {
		ret = emmc_phy_config_tuning(host);
		if (!ret)
			return 0;
	}

	/*
	 * DDR Mode requires driver to scan Sampling Fixed Delay Line,
	 * to find out a perfect operation sampling point.
	 * It is hard to implement such a scan in host driver since initiating
	 * commands by host driver is not safe.
	 * Thus so far just keep PHY Sampling Fixed Delay in default value
	 * in DDR mode.
	 *
	 * If any timing issue occrus in DDR mode on Marvell products,
	 * please contact maintainer to ask for internal support in Marvell.
	 */
	if ((host->timing == MMC_TIMING_MMC_DDR52) ||
	    (host->timing == MMC_TIMING_UHS_DDR50))
		dev_warn(mmc_dev(host->mmc), "Timing issue might occur in DDR mode\n");
	return ret;
}

/*
 * Adjust PHY setting.
 * PHY setting should be adjusted when SDCLK frequency, Bus Width
 * or Speed Mode is changed.
 * Addtional config are required when card is working in High Speed mode,
 * after leaving Legacy Mode.
 */
int xenon_phy_adj(struct sdhci_host *host, struct mmc_ios *ios)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int ret = 0;

	if (!host->clock) {
		priv->clock = 0;
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

	emmc_phy_set(host, ios->timing);

	/* Update the record */
	priv->bus_width = ios->bus_width;

	/* Skip temp stages from HS200 to HS400 */
	if (temp_stage_hs200_to_hs400(host, priv))
		return 0;

	/* Skip temp stages from HS400 t0 HS200 */
	if (temp_stage_hs400_to_h200(host, priv))
		return 0;

	priv->timing = ios->timing;
	priv->clock = host->clock;

	/* Legacy mode is a special case */
	if (ios->timing == MMC_TIMING_LEGACY)
		return 0;

	if (host->clock > SDHCI_DEFAULT_SDCLK_FREQ)
		ret = xenon_hs_delay_adj(host);
	return ret;
}

static int add_xenon_phy(struct device_node *np, struct sdhci_host *host,
			 const char *phy_name)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int i, ret;

	for (i = 0; i < NR_PHY_TYPES; i++) {
		if (!strcmp(phy_name, phy_types[i])) {
			priv->phy_type = i;
			break;
		}
	}
	if (i == NR_PHY_TYPES) {
		dev_err(mmc_dev(host->mmc),
			"Unable to determine PHY name %s. Use default eMMC 5.1 PHY\n",
			phy_name);
		priv->phy_type = EMMC_5_1_PHY;
	}

	ret = alloc_emmc_phy(priv);
	if (ret)
		return ret;

	return emmc_phy_parse_param_dt(host, np, priv->phy_params);
}

int xenon_phy_parse_dt(struct device_node *np, struct sdhci_host *host)
{
	const char *phy_type = NULL;

	if (!of_property_read_string(np, "marvell,xenon-phy-type", &phy_type))
		return add_xenon_phy(np, host, phy_type);

	dev_info(mmc_dev(host->mmc), "Fail to get Xenon PHY type. Use default eMMC 5.1 PHY\n");
	return add_xenon_phy(np, host, "emmc 5.1 phy");
}
