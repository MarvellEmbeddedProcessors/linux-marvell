/*
 * SD and eMMC host controller driver for Marvell Xenon SDHC
 *
 * Copyright (C) 2016 Victor Gu
 *
 * Author: Victor Gu
 * Email : xigu@marvell.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <asm/setup.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "sdhci.h"
#include "sdhci-pltfm.h"
/* Below header files are included since need to call APIs inside of them
 * for delay adjust testing in eMMC/SD/SDIO modes
 */
#include "../core/mmc_ops.h"
#include "../core/sdio_ops.h"

/* Re-tuning event interrupt signal */
#define  SDHCI_RETUNE_EVT_INTSIG		0x00001000

/*
 * Xenon SD Host Controller self-defined registers
 */
#define SDHC_SYS_CFG_INFO			0x0104
#define  SLOT_TYPE_SDIO_SHIFT			24
#define  SLOT_TYPE_EMMC_MASK			0xff
#define  SLOT_TYPE_EMMC_SHIFT			16
#define  SLOT_TYPE_SD_SDIO_MMC_MASK		0xff
#define  SLOT_TYPE_SD_SDIO_MMC_SHIFT		8

#define SDHC_SYS_OP_CTRL			0x0108
#define  AUTO_CLKGATE_DISABLE_MASK		(0x1 << 20)
#define  SDCLK_IDLEOFF_ENABLE_SHIFT		8
#define  SLOT_ENABLE_SHIFT			0

#define SDHC_SYS_EXT_OP_CTRL			0x010c

#define SDHC_SLOT_OP_STATUS_CTRL		0x0128
#define	 DELAY_90_DEGREE_MASK_EMMC5		(1 << 7)
#define  DELAY_90_DEGREE_SHIFT_EMMC5		7
#define  TUNING_PROG_FIXED_DELAY_MASK		0xff
#define  FORCE_SEL_INVERSE_CLK_SHIFT		11

#define SDHC_SLOT_eMMC_CTRL			0x0130
#define  ENABLE_DATA_STROBE			(1 << 24)
#define  SET_EMMC_RSTN				(1 << 16)
#define  DISABLE_RD_DATA_CRC			(1 << 14)
#define  DISABLE_CRC_STAT_TOKEN			(1 << 13)
#define  eMMC_VCCQ_MASK				0x3
#define   eMMC_VCCQ_1_8V			0x1
#define   eMMC_VCCQ_1_2V			0x2
#define	  eMMC_VCCQ_3_3V			0x3

#define SDHC_SLOT_RETUNING_REQ_CTRL		0x0144
#define  RETUNING_COMPATIBLE			0x1

#define SDHC_SLOT_AUTO_RETUNING_CTRL		0x0148
#define  ENABLE_AUTO_RETUNING			0x1

#define SDHC_SLOT_DLL_CUR_DLY_VAL		0x0150

#define EMMC_PHY_REG_BASE			0x170

#define EMMC_PHY_TIMING_ADJUST			EMMC_PHY_REG_BASE
#define  TIMING_ADJUST_SLOW_MODE		(1 << 29)
#define  TIMING_ADJUST_SDIO_MODE		(1 << 28)
#define  OUTPUT_QSN_PHASE_SELECT		(1 << 17)
#define  SAMPL_INV_QSP_PHASE_SELECT		(1 << 18)
#define  SAMPL_INV_QSP_PHASE_SELECT_SHIFT	18
#define  PHY_INITIALIZATION			(1 << 31)
#define  WAIT_CYCLE_BEFORE_USING_MASK		0xf
#define  WAIT_CYCLE_BEFORE_USING_SHIFT		12
#define  FC_SYNC_EN_DURATION_MASK		0xf
#define  FC_SYNC_EN_DURATION_SHIFT		8
#define	 FC_SYNC_RST_EN_DURATION_MASK		0xf
#define  FC_SYNC_RST_EN_DURATION_SHIFT		4
#define  FC_SYNC_RST_DURATION_MASK		0xf
#define  FC_SYNC_RST_DURATION_SHIFT		0

#define EMMC_PHY_FUNC_CONTROL			(EMMC_PHY_REG_BASE + 0x4)
#define  ASYNC_DDRMODE_MASK			(1 << 23)
#define  ASYNC_DDRMODE_SHIFT			23
#define  CMD_DDR_MODE				(1 << 16)
#define  DQ_DDR_MODE_SHIFT			8
#define  DQ_DDR_MODE_MASK			0xff
#define  DQ_ASYNC_MODE				(1 << 4)

#define EMMC_PHY_PAD_CONTROL			(EMMC_PHY_REG_BASE + 0x8)
#define  REC_EN_SHIFT				24
#define  REC_EN_MASK				0xf
#define  FC_DQ_RECEN				(1 << 24)
#define  FC_CMD_RECEN				(1 << 25)
#define  FC_QSP_RECEN				(1 << 26)
#define  FC_QSN_RECEN				(1 << 27)
#define  OEN_QSN				(1 << 28)
#define  AUTO_RECEN_CTRL			(1 << 30)
#define  FC_ALL_CMOS_RECEIVER			0xf000

#define  EMMC5_FC_QSP_PD			(1 << 18)
#define  EMMC5_FC_QSP_PU			(1 << 22)
#define  EMMC5_FC_CMD_PD			(1 << 17)
#define  EMMC5_FC_CMD_PU			(1 << 21)
#define  EMMC5_FC_DQ_PD				(1 << 16)
#define  EMMC5_FC_DQ_PU				(1 << 20)

#define EMMC_PHY_PAD_CONTROL1			(EMMC_PHY_REG_BASE + 0xc)
#define  EMMC5_1_FC_QSP_PD			(1 << 9)
#define  EMMC5_1_FC_QSP_PU			(1 << 25)
#define  EMMC5_1_FC_CMD_PD			(1 << 8)
#define  EMMC5_1_FC_CMD_PU			(1 << 24)
#define  EMMC5_1_FC_DQ_PD			0xff
#define  EMMC5_1_FC_DQ_PU			(0xff << 16)

#define EMMC_PHY_PAD_CONTROL2			(EMMC_PHY_REG_BASE + 0x10)
#define  ZNR_MASK				(0x1f << 8)
#define  ZNR_SHIFT				8
#define  ZPR_MASK				0x1f
/* Perferred ZNR and ZPR values vary between different boards.
 * The specific ZNR and ZPR values should be defined here
 * according to board actual timing.
 */
#define  ZNR_PREF_VALUE				0xf
#define  ZPR_PREF_VALUE				0xf

#define EMMC_PHY_DLL_CONTROL			(EMMC_PHY_REG_BASE + 0x14)
#define  DLL_ENABLE				(1 << 31)
#define  DLL_REFCLK_SEL				(1 << 30)
#define  DLL_UPDATE				(1 << 23)
#define  DLL_PHSEL1_SHIFT			24
#define  DLL_PHSEL0_SHIFT			16
#define  DLL_PHASE_MASK				0x3f
#define  DLL_PHASE_90_DEGREE			0x1f
#define  DLL_DELAY_TEST_LOWER_SHIFT		8
#define  DLL_DELAY_TEST_LOWER_MASK		0xff
#define  DLL_FAST_LOCK				(1 << 5)
#define  DLL_BYPASS_EN				(1 << 0)

#define EMMC_LOGIC_TIMING_ADJUST		(EMMC_PHY_REG_BASE + 0x18)
#define EMMC_LOGIC_TIMING_ADJUST_LOW		(EMMC_PHY_REG_BASE + 0x1c)

/* Xenon quirks */
#define SDHCI_QUIRK_XENON_EMMC_SLOT		(1 << 0) /* only support eMMC */

/* Hardware team recommends below value for HS400
 * eMMC logic timing adjust register(0x188), refer to FS to details
 * bit[3:0] PHY response delay parameter
 * bit[7:4] PHY write delay parameter
 * bit[11:8] PHY stop CLK parameter
 * bit[15:12] PHY interrupt off delay
 * bit[19:16] PHY init det delay
 * bit[23:20] PHY read wait delay
 * bit[31:24] Reserved
 */
#define LOGIC_TIMING_VALUE				0x00aa8977

/* Max input clock, 400MHZ, it is also used as Max output clock
 */
#define XENON_SDHC_MAX_CLOCK			400000000

/* Re-tuning parameters
 */
/* Max re-tuning counter */
#define XENON_MAX_TUNING_COUNT			0xe
/* Default re-tuning counter */
#define XENON_DEF_TUNING_COUNT			0x9
/* No support for this tuning value */
#define XENON_TMR_RETUN_NO_PRESENT		0xf

/* Invalid XENON MMC timing value, which is used as default value */
#define XENON_MMC_TIMING_INVALID			0xff

/* Delay step */
#define COARSE_SAMPL_FIX_DELAY_STEP			100
#define FINE_SAMPL_FIX_DELAY_STEP			50

/* SDHC private parameters */
struct sdhci_xenon_priv {
	/* The three fields in below records the current
	 * setting of Xenon SDHC.
	 * Driver will call a Sampling Fixed Delay Adjustment
	 * if any setting is changed.
	 */
	unsigned char	bus_width;
	unsigned char	timing;
	unsigned char	tuning_count;
	unsigned int	clock;
	unsigned int	quirks; /* Xenon private quirks */
};

struct card_cntx {
	/* When initializing card, Xenon has to adjust
	 * sampling fixed delay.
	 * However, at that time, card structure is not
	 * linked to mmc_host.
	 * Thus a card pointer is added here providing
	 * the delay adjustment function with the card structure
	 * of the card during initialization
	 */
	struct mmc_card *delay_adjust_card;
};

/*
 * Xenon Specific Initialization Operations
 */
/*
 * Internal eMMC PHY routines
 */
static int sdhci_xenon_phy_init(struct sdhci_host *host)
{
	u32 reg;
	u32 wait;
	u32 clock;

	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg |= PHY_INITIALIZATION;
	sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);

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
	/* 4 additional bus clock and 4 AXI bus clock are required */
	wait += 8;
	/* left shift 20 bits */
	wait <<= 20;

	clock = host->mmc->actual_clock;
	if (clock == 0)
		/* Use the possibly slowest bus frequency value */
		clock = 100000;

	/* get the wait time */
	wait /= clock;
	wait++;

	/* wait for host eMMC PHY init to complete */
	udelay(wait);

	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg &= PHY_INITIALIZATION;
	if (reg) {
		dev_err(mmc_dev(host->mmc), "%s: eMMC PHY init cannot complete after %d us\n",
			mmc_hostname(host->mmc), wait);
		return -EIO;
	}

	dev_dbg(mmc_dev(host->mmc), "%s: eMMC PHY init complete\n",
		mmc_hostname(host->mmc));

	return 0;
}

static void sdhci_xenon_phy_reset(struct sdhci_host *host, unsigned int timing)
{
	u32 reg;
	struct mmc_card *card = host->mmc->card;

	dev_dbg(mmc_dev(host->mmc), "%s: eMMC PHY setting starts\n",
		mmc_hostname(host->mmc));

	/* Setup pad, set bit[28] and bits[26:24] */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL);
	reg |= (FC_DQ_RECEN | FC_CMD_RECEN | FC_QSP_RECEN | OEN_QSN);

	/* According to latest spec, all FC_XX_RECEIVCE should
	 * be set as CMOS Type
	 */
	reg |= FC_ALL_CMOS_RECEIVER;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL);

	/* Set CMD and DQ Pull Up */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
	reg |= (EMMC5_1_FC_CMD_PU | EMMC5_1_FC_DQ_PU);
	reg &= ~(EMMC5_1_FC_CMD_PD | EMMC5_1_FC_DQ_PD);
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);

	/* If timing belongs to high speed, set bit[17] of
	 * EMMC_PHY_TIMING_ADJUST register
	 */
	if ((timing == MMC_TIMING_MMC_HS400) ||
	    (timing == MMC_TIMING_MMC_HS200) ||
	    (timing == MMC_TIMING_UHS_SDR50) ||
	    (timing == MMC_TIMING_UHS_SDR104) ||
	    (timing == MMC_TIMING_UHS_DDR50) ||
	    (timing == MMC_TIMING_UHS_SDR25) ||
	    (timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg &= ~OUTPUT_QSN_PHASE_SELECT;
		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	}

	/* If SDIO card, set SDIO Mode
	 * Otherwise, clear SDIO Mode and Slow Mode
	 */
	if (card && (card->type & MMC_TYPE_SDIO)) {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg |= TIMING_ADJUST_SDIO_MODE;

		if ((timing == MMC_TIMING_UHS_SDR25) ||
		    (timing == MMC_TIMING_UHS_SDR12) ||
		    (timing == MMC_TIMING_SD_HS) ||
		    (timing == MMC_TIMING_LEGACY))
			reg |= TIMING_ADJUST_SLOW_MODE;

		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	} else {
		reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
		reg &= ~(TIMING_ADJUST_SDIO_MODE | TIMING_ADJUST_SLOW_MODE);
		sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);
	}

	/* Set preferred ZNR and ZPR value
	 * The ZNR and ZPR value vary between different boards.
	 */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL2);
	reg &= ~(ZNR_MASK | ZPR_MASK);
	reg |= ((ZNR_PREF_VALUE << ZNR_SHIFT) | ZPR_PREF_VALUE);
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL2);

	/* When setting EMMC_PHY_FUNC_CONTROL register,
	 * SD clock should be disabled
	 */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if ((timing == MMC_TIMING_UHS_DDR50) ||
	    (timing == MMC_TIMING_MMC_HS400) ||
	    (timing == MMC_TIMING_MMC_DDR52)) {
		reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
		reg |= (DQ_DDR_MODE_MASK << DQ_DDR_MODE_SHIFT) | CMD_DDR_MODE;
		sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
	}

	if (timing == MMC_TIMING_MMC_HS400) {
		reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
		reg &= ~DQ_ASYNC_MODE;
		sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);
	}

	/* Enable bus clock */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

	if (timing == MMC_TIMING_MMC_HS400)
		sdhci_writel(host, LOGIC_TIMING_VALUE,
			     EMMC_LOGIC_TIMING_ADJUST);

	sdhci_xenon_phy_init(host);

	dev_dbg(mmc_dev(host->mmc), "%s: eMMC PHY setting completed\n",
		mmc_hostname(host->mmc));
}

/* apply samping fix delay */
static int sdhci_xenon_set_fix_delay(struct sdhci_host *host,
				     unsigned int delay, bool invert,
				     bool phase)
{
	unsigned int reg;
	int ret = 0;
	u8 timeout;

	if (invert)
		invert = 0x1;
	else
		invert = 0x0;

	if (phase)
		phase = 0x1;
	else
		phase = 0x0;

	/* Setup sampling fix delay */
	reg = sdhci_readl(host, SDHC_SLOT_OP_STATUS_CTRL);
	reg &= ~TUNING_PROG_FIXED_DELAY_MASK;
	reg |= delay & TUNING_PROG_FIXED_DELAY_MASK;
	sdhci_writel(host, reg, SDHC_SLOT_OP_STATUS_CTRL);

	/* Disable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg &= ~(SDHCI_CLOCK_CARD_EN | SDHCI_CLOCK_INT_EN);
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	/* If phase = true, set 90 degree phase */
	reg = sdhci_readl(host, EMMC_PHY_FUNC_CONTROL);
	reg &= ~ASYNC_DDRMODE_MASK;
	reg |= (phase << ASYNC_DDRMODE_SHIFT);
	sdhci_writel(host, reg, EMMC_PHY_FUNC_CONTROL);

	/* Setup inversion of sampling edge */
	reg = sdhci_readl(host, EMMC_PHY_TIMING_ADJUST);
	reg &= ~SAMPL_INV_QSP_PHASE_SELECT;
	reg |= (invert << SAMPL_INV_QSP_PHASE_SELECT_SHIFT);
	sdhci_writel(host, reg, EMMC_PHY_TIMING_ADJUST);

	/* Enable SD internal clock */
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

	/* Enable SDCLK */
	reg = sdhci_readl(host, SDHCI_CLOCK_CONTROL);
	reg |= SDHCI_CLOCK_CARD_EN;
	sdhci_writel(host, reg, SDHCI_CLOCK_CONTROL);

	udelay(200);

	/* Has to re-initialize eMMC PHY here to active PHY
	 * because later get status cmd will be issued.
	 */
	ret = sdhci_xenon_phy_init(host);

	return ret;
}

/* apply strobe delay */
static void sdhci_xenon_set_strobe_delay(struct sdhci_host *host)
{
	u32 reg;

	/* Enable DLL */
	reg = sdhci_readl(host, EMMC_PHY_DLL_CONTROL);
	reg |= (DLL_ENABLE | DLL_FAST_LOCK);

	/* Set phase as 90 degree */
	reg &= ~((DLL_PHASE_MASK << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_MASK << DLL_PHSEL1_SHIFT));
	reg |= ((DLL_PHASE_90_DEGREE << DLL_PHSEL0_SHIFT) |
			(DLL_PHASE_90_DEGREE << DLL_PHSEL1_SHIFT));

	reg &= ~DLL_BYPASS_EN;
	reg |= DLL_UPDATE;
	if (host->clock > 500000000)
		reg &= ~DLL_REFCLK_SEL;
	sdhci_writel(host, reg, EMMC_PHY_DLL_CONTROL);

	/* Set data strobe pull down */
	reg = sdhci_readl(host, EMMC_PHY_PAD_CONTROL1);
	reg |= EMMC5_1_FC_QSP_PD;
	reg &= ~EMMC5_1_FC_QSP_PU;
	sdhci_writel(host, reg, EMMC_PHY_PAD_CONTROL1);
}

/* eMMC delay adjust test */
static int sdhci_xenon_emmc_delay_adj(struct mmc_card *card)
{
	int err;
	u8 *ext_csd = NULL;

	err = mmc_get_ext_csd(card, &ext_csd);
	kfree(ext_csd);

	return err;
}

/* SD delay adjust test */
static int sdhci_xenon_sd_delay_adj(struct mmc_card *card)
{
	return mmc_send_status(card, NULL);
}

/* SDIO delay adjust test */
static int sdhci_xenon_sdio_delay_adj(struct mmc_card *card)
{
	u8 reg;

	return mmc_io_rw_direct(card, 0, 0, 0, 0, &reg);
}

static int sdhci_xenon_delay_adj_test(struct sdhci_host *host,
		struct mmc_card *card, unsigned int delay,
		bool invert, bool phase)
{
	int ret;

	sdhci_xenon_set_fix_delay(host, delay, invert, phase);

	if (mmc_card_mmc(card))
		ret = sdhci_xenon_emmc_delay_adj(card);
	else if (mmc_card_sd(card))
		ret = sdhci_xenon_sd_delay_adj(card);
	else if (mmc_card_sdio(card))
		ret = sdhci_xenon_sdio_delay_adj(card);
	else
		return -EINVAL;
	if (ret) {
		pr_debug("Xenon failed when sampling fix delay %d, inverted %d, phase %d\n",
			 delay, invert, phase);
		return -1;
	}

	pr_debug("Xenon succeeded when sampling fixed delay %d, inverted %d, phase %d\n",
		 delay, invert, phase);
	return 0;
}

/* Adjust the fix delay
 * This routine tries to calculate a proper fix delay.It is because tuning
 * is only available in HS200 mode, need to adjust delay for other modes,
 * and even adjust the delay before tuning.
 */
static int sdhci_xenon_fix_delay_adj(struct sdhci_host *host,
				     struct mmc_card *card)
{
	bool invert, phase;
	int idx, nr_pair;
	int ret;
	unsigned int delay;
	unsigned int min_delay, max_delay;
	u32 reg;
	/* Pairs to set the delay edge.
	 * First column is the inversion sequence.
	 * Second column indicates delay 90 degree or not.
	 */
	bool delay_edge_pair[][2] = {
			{ true,		false},
			{ true,		true},
			{ false,	false},
			{ false,	true }
	};

	nr_pair = sizeof(delay_edge_pair) / ARRAY_SIZE(delay_edge_pair);

	for (idx = 0; idx < nr_pair; idx++) {
		invert = delay_edge_pair[idx][0];
		phase = delay_edge_pair[idx][1];

		/* Increase delay value to get min fix delay */
		for (min_delay = 0; min_delay <= TUNING_PROG_FIXED_DELAY_MASK;
				min_delay += COARSE_SAMPL_FIX_DELAY_STEP) {
			ret = sdhci_xenon_delay_adj_test(host, card, min_delay,
							 invert, phase);
			if (!ret)
				break;
		}

		if (ret) {
			pr_debug("Failed to set sampling fixed delay with inversion %d, phase %d\n",
				 invert, phase);
			continue;
		}

		/* Increase delay value to get max fix delay */
		for (max_delay = min_delay + 1;
			max_delay < TUNING_PROG_FIXED_DELAY_MASK;
			max_delay += FINE_SAMPL_FIX_DELAY_STEP) {
			ret = sdhci_xenon_delay_adj_test(host, card, max_delay,
							 invert, phase);
			if (ret) {
				max_delay -= FINE_SAMPL_FIX_DELAY_STEP;
				break;
			}
		}

		/* Handle the round case in case the max allowed edge passed
		 * delay adjust testing.
		 */
		if (!ret) {
			ret = sdhci_xenon_delay_adj_test(host, card,
						   TUNING_PROG_FIXED_DELAY_MASK,
						   invert, phase);
			if (!ret)
				max_delay = TUNING_PROG_FIXED_DELAY_MASK;
		}

		/* For eMMC PHY, sampling fixed delay line window should
		 * be large enough, thus the sampling point(the middle of
		 * the window) can work when environment varies.
		 * However, there is no clear conclusoin how large the window
		 * should be. We just make an assumption that
		 * the window should be larger 25% of a SDCLK cycle.
		 * Please note that this judgement is only based on experience.
		 *
		 * The field delay value of main delay line in
		 * register SDHC_SLOT_DLL_CUR represents a half of SDCLK
		 * cycle. Thus the window should be larger than 1/2 of
		 * the field delay value of main delay line value.
		 */
		reg = sdhci_readl(host, SDHC_SLOT_DLL_CUR_DLY_VAL);
		reg &= TUNING_PROG_FIXED_DELAY_MASK;
		/* get the 1/4 SDCLK cycle */
		reg >>= 1;

		if ((max_delay - min_delay) < reg) {
			pr_info("The window size %d when inversion = %d, phase = %d cannot meet timing requiremnt\n",
				max_delay - min_delay, invert, phase);
			continue;
		}

		delay = (min_delay + max_delay) / 2;
		sdhci_xenon_set_fix_delay(host, delay, invert, phase);
		pr_debug("Xenon sampling fix delay = %d with inversion = %d, phase = %d\n",
			 delay, invert, phase);
		return 0;
	}

	return -EIO;
}

/* Adjust the strobe delay for HS400 mode */
static int sdhci_xenon_strobe_delay_adj(struct sdhci_host *host,
					struct mmc_card *card)
{
	int ret = 0;
	u32 reg;

	/* Enable SDHC data strobe */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg |= ENABLE_DATA_STROBE;
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);

	/* Enable the DLL to automatically adjust HS400 strobe delay */
	sdhci_xenon_set_strobe_delay(host);
	return ret;
}

/*
 * sdhci_xenon_delay_adj should not be called inside of IRQ context,
 * either hardware IRQ or software IRQ.
 */
static int sdhci_xenon_delay_adj(struct sdhci_host *host, struct mmc_ios *ios)
{
	int ret;
	struct mmc_host *mmc = host->mmc;
	struct mmc_card *card = NULL;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv;
	struct card_cntx *cntx;

	if (!host->clock)
		return 0;

	priv = (struct sdhci_xenon_priv *)pltfm_host->private;
	if (ios->timing != priv->timing)
		sdhci_xenon_phy_reset(host, ios->timing);

	/* Legacy mode is a special case.
	 * In legacy Mode, usually it is not necessary to adjust
	 * sampling fixed delay, since the SDCLK frequency is quiet low.
	 */
	if (ios->timing == MMC_TIMING_LEGACY) {
		priv->timing = ios->timing;
		return 0;
	}

	/* If the timing, frequency or bus width are changed,
	 * better to set eMMC PHY based on current setting
	 * and adjust Xenon SDHC delay.
	 */
	if ((host->clock == priv->clock) &&
		(ios->bus_width == priv->bus_width) &&
		(ios->timing == priv->timing))
		return 0;

	/* Save the values */
	priv->clock = host->clock;
	priv->bus_width = ios->bus_width;
	priv->timing = ios->timing;

	cntx = (struct card_cntx *)mmc->slot.handler_priv;
	card = cntx->delay_adjust_card;
	if (unlikely(card == NULL))
		return -EIO;

	/* No need to set any delay for some cases in this stage
	 * since it will be reset as legacy mode soon.
	 * For example,
	 * during the hardware reset in high speed mode,
	 * and the SDCLK is no more than 400k (legacy mode).
	 */
	if (host->clock <= 4000000)
		return 0;

	if (mmc_card_hs400(card)) {
		pr_debug("%s: start HS400 strobe delay adjustment\n",
			 mmc_hostname(mmc));
		ret = sdhci_xenon_strobe_delay_adj(host, card);
		if (ret)
			pr_err("%s: strobe fixed delay adjustment failed\n",
			       mmc_hostname(mmc));

		return ret;
	}

	pr_debug("%s: start sampling fixed delay adjustment\n",
		 mmc_hostname(mmc));
	ret = sdhci_xenon_fix_delay_adj(host, card);
	if (ret)
		pr_err("%s: sampling fixed delay adjustment failed\n",
		       mmc_hostname(mmc));

	return ret;
}

/* After determining the slot is used for SDIO, some addtional
 * task is required. It will save the pointer to current card,
 * and do other utilities such as enable or disable auto cmd12
 * according to card type, and whether indicate current slot as SDIO slot.
 */
static void sdhci_xenon_init_card(struct sdhci_host *host,
				  struct mmc_card *card)
{
	u32 reg;
	u8 slot_idx;
	struct card_cntx *cntx;
	struct mmc_host *mmc = host->mmc;

	cntx = (struct card_cntx *)mmc->slot.handler_priv;
	cntx->delay_adjust_card = card;

	slot_idx = mmc->slotno;

	if (!mmc_card_sdio(card)) {
		/* Re-enable the Auto-CMD12 cap flag. */
		host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags |= SDHCI_AUTO_CMD12;

		/* Clear SDHC system config information register[31:24] */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg &= ~(1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
	} else {
		/* Delete the Auto-CMD12 cap flag.
		 * Otherwise, when sending multi-block CMD53,
		 * driver will set transfer mode register to enable Auto CMD12.
		 * However, SDIO device cannot recognize CMD12.
		 * Thus SDHC will be time out for waiting for CMD12 response.
		 */
		host->quirks &= ~SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags &= ~SDHCI_AUTO_CMD12;

		/* Set SDHC system configuration information register[31:24]
		 * to inform that the current slot is for SDIO.
		 */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg |= (1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
	}
}

/* Enable/Disable the Auto Clock Gating function */
static void sdhci_xenon_set_acg(struct sdhci_host *host, bool enable)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	if (enable)
		reg &= ~AUTO_CLKGATE_DISABLE_MASK;
	else
		reg |= AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable or disable this slot */
static void sdhci_xenon_set_slot(struct sdhci_host *host, unsigned char slot,
				 bool enable)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	if (enable)
		reg |= ((0x1 << slot) << SLOT_ENABLE_SHIFT);
	else
		reg &= ~((0x1 << slot) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);

	/* Manually set the flag which all the slots require,
	 * including SD, eMMC, SDIO
	 */
	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;
}

/* Set SDCLK-off-while-idle */
static void sdhci_xenon_set_sdclk_off_idle(struct sdhci_host *host,
					   unsigned char slotno, bool enable)
{
	u32 reg;
	u32 mask;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	/* Get the bit shift based on the slot index */
	mask = (0x1 << (SDCLK_IDLEOFF_ENABLE_SHIFT + slotno));
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable Parallel Transfer Mode */
static void sdhci_xenon_enable_parallel_tran(struct sdhci_host *host,
					     unsigned char slot)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_EXT_OP_CTRL);
	reg |= (0x1 << slot);
	sdhci_writel(host, reg, SDHC_SYS_EXT_OP_CTRL);
}

/* Disable re-tuning request, event and auto-retuning*/
static void sdhci_xenon_setup_tuning(struct sdhci_host *host)
{
	u8 reg;

	/* Disable the Re-Tuning Request functionality */
	reg = sdhci_readl(host, SDHC_SLOT_RETUNING_REQ_CTRL);
	reg &= ~RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, SDHC_SLOT_RETUNING_REQ_CTRL);

	/* Disbale the Re-tuning Event Signal Enable */
	reg = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	reg &= ~SDHCI_RETUNE_EVT_INTSIG;
	sdhci_writel(host, reg, SDHCI_SIGNAL_ENABLE);

	/* Disable Auto-retuning */
	reg = sdhci_readl(host, SDHC_SLOT_AUTO_RETUNING_CTRL);
	reg &= ~ENABLE_AUTO_RETUNING;
	sdhci_writel(host, reg, SDHC_SLOT_AUTO_RETUNING_CTRL);
}

/* Recover the register setting cleared during SOFTWARE_RESET_ALL */
static void sdhci_xenon_reset_exit(struct sdhci_host *host,
				   unsigned char slotno, u8 mask)
{
	/* Only SOFTWARE RESET ALL will clear the register setting */
	if (!(mask & SDHCI_RESET_ALL))
		return;

	/* Disable tuning request and auto-retuing again */
	sdhci_xenon_setup_tuning(host);

	sdhci_xenon_set_acg(host, false);

	sdhci_xenon_set_sdclk_off_idle(host, slotno, false);
}

static void sdhci_xenon_reset(struct sdhci_host *host, u8 mask)
{
	sdhci_reset(host, mask);

	sdhci_xenon_reset_exit(host, host->mmc->slotno, mask);
}

static void sdhci_xenon_voltage_switch(struct sdhci_host *host)
{
	u32 reg;
	unsigned char voltage;
	unsigned char voltage_code;

	voltage = host->mmc->ios.signal_voltage;

	if (voltage == MMC_SIGNAL_VOLTAGE_330) {
		voltage_code = eMMC_VCCQ_3_3V;
	} else if (voltage == MMC_SIGNAL_VOLTAGE_180) {
		voltage_code = eMMC_VCCQ_1_8V;
	} else {
		pr_err("%s: Xenon unsupported signal voltage\n",
		       mmc_hostname(host->mmc));
		return;
	}

	/* This host is for eMMC, XENON self-defined
	 * eMMC slot control register should be accessed
	 * instead of Host Control 2
	 */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg &= ~eMMC_VCCQ_MASK;
	reg |= voltage_code;
	sdhci_writel(host, reg, SDHC_SLOT_eMMC_CTRL);

	/* There is no standard to determine this waiting period */
	usleep_range(1000, 2000);

	/* Check whether io voltage switch is done */
	reg = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	reg &= eMMC_VCCQ_MASK;
	/*
	 * This bit is set only when regulator feedbacks the voltage switch
	 * result. However, in actaul implementation, regulator might not
	 * provide this feedback. Thus we shall not rely on this bit status
	 * to determine if switch is failed. If the bit is not set, just
	 * throws a warning, error level message is not need.
	 */
	if (reg != voltage_code)
		pr_warn("%s: Xenon failed to switch signal voltage\n",
			mmc_hostname(host->mmc));
}

static void sdhci_xenon_voltage_switch_pre(struct sdhci_host *host)
{
	u32 reg;
	int timeout;

	/*
	 * Before SD/SDIO set signal voltage, SD bus clock should be disabled.
	 * However, sdhci_set_clock will also disable the internal clock.
	 * For some host SD controller, if internal clock is disabled,
	 * the 3.3V/1.8V bit can not be updated.
	 * Thus here manually enable internal clock.
	 * After switch completes, it is unnessary to disable internal clock,
	 * since keeping internal clock active follows SD spec.
	 */
	reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL);
	if (!(reg & SDHCI_CLOCK_INT_EN)) {
		reg |= SDHCI_CLOCK_INT_EN;
		sdhci_writew(host, reg, SDHCI_CLOCK_CONTROL);

		/* Wait max 20 ms */
		timeout = 20;
		while (!((reg = sdhci_readw(host, SDHCI_CLOCK_CONTROL))
				& SDHCI_CLOCK_INT_STABLE)) {
			if (timeout == 0) {
				pr_err("%s: Internal clock never stabilised.\n",
				       mmc_hostname(host->mmc));
				break;
			}
			timeout--;
			mdelay(1);
		}
	}
}

static const struct of_device_id sdhci_xenon_dt_ids[] = {
	{ .compatible = "marvell,xenon-sdhci",},
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_xenon_dt_ids);

static void sdhci_xenon_platform_init(struct sdhci_host *host)
{
	sdhci_xenon_set_acg(host, false);
}

unsigned int sdhci_xenon_get_max_clock(struct sdhci_host *host)
{
	return XENON_SDHC_MAX_CLOCK;
}

static struct sdhci_ops sdhci_xenon_ops = {
	.set_clock		= sdhci_set_clock,
	.set_bus_width		= sdhci_set_bus_width,
	.reset			= sdhci_xenon_reset,
	.set_uhs_signaling	= sdhci_set_uhs_signaling,
	.platform_init		= sdhci_xenon_platform_init,
	.get_max_clock		= sdhci_xenon_get_max_clock,
	.voltage_switch		= sdhci_xenon_voltage_switch,
	.voltage_switch_pre	= sdhci_xenon_voltage_switch_pre,
	.delay_adj		= sdhci_xenon_delay_adj,
	.init_card		= sdhci_xenon_init_card,
};

static struct sdhci_pltfm_data sdhci_xenon_pdata = {
	.ops = &sdhci_xenon_ops,
	.quirks = SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
		  SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12 |
		  SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER,
	.quirks2 = SDHCI_QUIRK2_TIMING_HS200_HS400,
	/* Add SOC specific quirks in the above .quirks, .quirks2
	 * fields.
	 */
};

static int sdhci_xenon_probe_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host =
				    (struct sdhci_pltfm_host *)sdhci_priv(host);
	struct sdhci_xenon_priv *priv =
				 (struct sdhci_xenon_priv *)pltfm_host->private;
	struct mmc_host *mmc = host->mmc;
	int err;
	u32 slotno;
	u32 tuning_count;

	/* Standard MMC property */
	err = mmc_of_parse(mmc);
	if (err) {
		pr_err("%s: Failed to call mmc_of_parse.\n", mmc_hostname(mmc));
		return err;
	}

	/* Statndard SDHCI property */
	sdhci_get_of_property(pdev);

	/* Xenon specific property:
	 * emmc: this is a emmc slot.
	 *   Actually, whether current slot is for emmc can be
	 *   extracted from SDHC_SYS_CFG_INFO register. However, some Xenon IP
	 *   versions might not implement the slot type information. In such a
	 *   case, it is necessary to explicitly indicate the emmc type.
	 * slotno: the index of slot. Refer to SDHC_SYS_CFG_INFO register
	 */
	if (of_get_property(np, "xenon,emmc", NULL))
		priv->quirks |= SDHCI_QUIRK_XENON_EMMC_SLOT;

	if (!of_property_read_u32(np, "xenon,slotno", &slotno))
		mmc->slotno = slotno & 0xff;

	if (!of_property_read_u32(np, "xenon,tuning-count", &tuning_count))
		priv->tuning_count = tuning_count & 0xf;
	else
		priv->tuning_count = XENON_DEF_TUNING_COUNT;

	return 0;
}

static bool sdhci_xenon_slot_type_emmc(struct sdhci_host *host,
				       unsigned char slotno)
{
	u32 reg;
	unsigned int emmc_slot;
	unsigned int sd_slot;
	struct sdhci_pltfm_host *pltfm_host =
				    (struct sdhci_pltfm_host *)sdhci_priv(host);
	struct sdhci_xenon_priv *priv =
				 (struct sdhci_xenon_priv *)pltfm_host->private;

	if (priv->quirks & SDHCI_QUIRK_XENON_EMMC_SLOT)
		return true;

	/* Read the eMMC slot type field from SYS_CFG_INFO register
	 * If a bit is set, this slot supports eMMC
	 */
	reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
	emmc_slot = reg >> SLOT_TYPE_EMMC_SHIFT;
	emmc_slot &= SLOT_TYPE_EMMC_MASK;
	emmc_slot &= (1 << slotno);

	/* This slot doesn't support eMMC */
	if (!emmc_slot)
		return false;

	/* Read the SD/SDIO/MMC slot type field from SYS_CFG_INFO register
	 * if that bit is NOT set, this slot can only support eMMC
	 */
	sd_slot = reg >> SLOT_TYPE_SD_SDIO_MMC_SHIFT;
	sd_slot &= SLOT_TYPE_SD_SDIO_MMC_MASK;
	emmc_slot &= sd_slot;

	/* ONLY support emmc */
	if (!emmc_slot)
		return true;

	return false;
}

static void sdhci_xenon_set_tuning_count(struct sdhci_host *host,
					 unsigned int count)
{
	if (unlikely(count >= XENON_TMR_RETUN_NO_PRESENT)) {
		pr_err("%s: Wrong Re-tuning Count. Set default value %d\n",
			mmc_hostname(host->mmc), XENON_DEF_TUNING_COUNT);
		host->tuning_count = XENON_DEF_TUNING_COUNT;
		return;
	}

	/* A valid count value */
	host->tuning_count = count;
}

/* Current BSP can only support Tuning Mode 1.
 * Tuning timer can only be setup when in Tuning Mode 1.
 * Thus host->tuning_mode has to be forced to Tuning Mode 1.
 */
static void sdhci_xenon_set_tuning_mode(struct sdhci_host *host)
{
	host->tuning_mode = SDHCI_TUNING_MODE_1;
}

/* set additional fixup for Xenon */
static void sdhci_xenon_add_host_fixup(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv;

	sdhci_xenon_set_tuning_mode(host);

	priv = (struct sdhci_xenon_priv *)pltfm_host->private;
	sdhci_xenon_set_tuning_count(host, priv->tuning_count);
}

static int sdhci_xenon_slot_probe(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u8 slotno = mmc->slotno;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv =
				 (struct sdhci_xenon_priv *)pltfm_host->private;
	struct card_cntx *cntx;

	cntx = devm_kzalloc(mmc_dev(mmc), sizeof(*cntx), GFP_KERNEL);
	if (!cntx)
		return -ENOMEM;
	mmc->slot.handler_priv = cntx;

	/* Enable slot */
	sdhci_xenon_set_slot(host, slotno, true);

	/* Enable ACG */
	sdhci_xenon_set_acg(host, true);

	/* Enable Parallel Transfer Mode */
	sdhci_xenon_enable_parallel_tran(host, slotno);

	/* Do eMMC setup if it is an eMMC slot */
	if (sdhci_xenon_slot_type_emmc(host, slotno)) {
		/* Mark the flag which requires Xenon eMMC-specific
		 * operations, such as voltage switch
		 */
		mmc->caps |= MMC_CAP_BUS_WIDTH_TEST | MMC_CAP_NONREMOVABLE;
		mmc->caps2 |= MMC_CAP2_HC_ERASE_SZ | MMC_CAP2_PACKED_CMD;
	}

	/* Set tuning functionality of this slot */
	sdhci_xenon_setup_tuning(host);

	/* set default timing value */
	priv->timing = XENON_MMC_TIMING_INVALID;

	return 0;
}

static void sdhci_xenon_slot_remove(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u8 slotno = mmc->slotno;

	/* Disable slot */
	sdhci_xenon_set_slot(host, slotno, false);
}

static int sdhci_xenon_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host;
	const struct of_device_id *match;
	int err;

	match = of_match_device(of_match_ptr(sdhci_xenon_dt_ids), &pdev->dev);
	if (!match)
		return -EINVAL;

	host = sdhci_pltfm_init(pdev, &sdhci_xenon_pdata,
				sizeof(struct sdhci_xenon_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);

	err = sdhci_xenon_probe_dt(pdev);
	if (err) {
		pr_err("%s: Failed to probe dt.\n", mmc_hostname(host->mmc));
		goto free_pltfm;
	}

	err = sdhci_xenon_slot_probe(host);
	if (err) {
		pr_err("%s: Failed to probe slot.\n", mmc_hostname(host->mmc));
		goto free_pltfm;
	}

	err = sdhci_add_host(host);
	if (err) {
		pr_err("%s: Failed to call add sdhci host\n",
		       mmc_hostname(host->mmc));
		goto remove_slot;
	}

	sdhci_xenon_add_host_fixup(host);

	return 0;

remove_slot:
	sdhci_xenon_slot_remove(host);
free_pltfm:
	sdhci_pltfm_free(pdev);
	return err;
}

static int sdhci_xenon_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	int dead = (readl(host->ioaddr + SDHCI_INT_STATUS) == 0xffffffff);

	sdhci_xenon_slot_remove(host);

	sdhci_remove_host(host, dead);
	sdhci_pltfm_free(pdev);

	return 0;
}

static struct platform_driver sdhci_xenon_driver = {
	.driver		= {
		.name	= "mv-xenon-sdhci",
		.owner	= THIS_MODULE,
		.of_match_table = sdhci_xenon_dt_ids,
		.pm = SDHCI_PLTFM_PMOPS,
	},
	.probe		= sdhci_xenon_probe,
	.remove		= sdhci_xenon_remove,
};

module_platform_driver(sdhci_xenon_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Marvell Xenon SDHC");
MODULE_AUTHOR("Victor Gu <xigu@marvell.com>");
MODULE_LICENSE("GPL v2");
