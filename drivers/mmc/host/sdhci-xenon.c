/*
 * Driver for Marvell SOCP Xenon SDHC as a platform device
 *
 * Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * Author:	Hu Ziji <huziji@marvell.com>
 * Date:	2016-7-30
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * Inspired by Jisheng Zhang <jszhang@marvell.com>
 * Special thanks to Video BG4 project team.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/module.h>
#include <linux/of.h>

#include "sdhci-pltfm.h"
#include "sdhci.h"
#include "sdhci-xenon.h"

/*
 * Xenon Specific Initialization Operations
 */
static inline void xenon_set_tuning_count(struct sdhci_host *host,
				unsigned int count)
{
	/* A valid count value */
	host->tuning_count = 1 << (count - 1);
}

/*
 * Current driver can only support Tuning Mode 1.
 * Tuning timer is only setup only tuning_mode == Tuning Mode 1.
 * Thus host->tuning_mode has to be forced as Tuning Mode 1.
 */
static inline void xenon_set_tuning_mode(struct sdhci_host *host)
{
	host->tuning_mode = SDHCI_TUNING_MODE_1;
}

/* Set SDCLK-off-while-idle */
static void xenon_set_sdclk_off_idle(struct sdhci_host *host,
			unsigned char slot_idx, bool enable)
{
	u32 reg;
	u32 mask;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	/* Get the bit shift basing on the slot index */
	mask = (0x1 << (SDCLK_IDLEOFF_ENABLE_SHIFT + slot_idx));
	if (enable)
		reg |= mask;
	else
		reg &= ~mask;

	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable/Disable the Auto Clock Gating function */
static void xenon_set_acg(struct sdhci_host *host, bool enable)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	if (enable)
		reg &= ~AUTO_CLKGATE_DISABLE_MASK;
	else
		reg |= AUTO_CLKGATE_DISABLE_MASK;
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable this slot */
static void xenon_enable_slot(struct sdhci_host *host,
			unsigned char slot_idx)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg |= ((0x1 << slot_idx) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);

	/*
	 * Manually set the flag which all the slots require,
	 * including SD, eMMC, SDIO
	 */
	host->mmc->caps |= MMC_CAP_WAIT_WHILE_BUSY;
}

/* Disable this slot */
static void xenon_disable_slot(struct sdhci_host *host,
			unsigned char slot_idx)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_OP_CTRL);
	reg &= ~((0x1 << slot_idx) << SLOT_ENABLE_SHIFT);
	sdhci_writel(host, reg, SDHC_SYS_OP_CTRL);
}

/* Enable Parallel Transfer Mode */
static void xenon_enable_slot_parallel_tran(struct sdhci_host *host,
			unsigned char slot_idx)
{
	u32 reg;

	reg = sdhci_readl(host, SDHC_SYS_EXT_OP_CTRL);
	reg |= (0x1 << slot_idx);
	sdhci_writel(host, reg, SDHC_SYS_EXT_OP_CTRL);
}

static void xenon_slot_tuning_setup(struct sdhci_host *host)
{
	u32 reg;

	/* Disable the Re-Tuning Request functionality */
	reg = sdhci_readl(host, SDHC_SLOT_RETUNING_REQ_CTRL);
	reg &= ~RETUNING_COMPATIBLE;
	sdhci_writel(host, reg, SDHC_SLOT_RETUNING_REQ_CTRL);

	/* Disbale the Re-tuning Event Signal Enable */
	reg = sdhci_readl(host, SDHCI_SIGNAL_ENABLE);
	reg &= ~SDHCI_INT_RETUNE;
	sdhci_writel(host, reg, SDHCI_SIGNAL_ENABLE);

	/* Disable Auto-retuning */
	reg = sdhci_readl(host, SDHC_SLOT_AUTO_RETUNING_CTRL);
	reg &= ~ENABLE_AUTO_RETUNING;
	sdhci_writel(host, reg, SDHC_SLOT_AUTO_RETUNING_CTRL);
}

/*
 * Operations inside struct sdhci_ops
 */
/* Recover the Register Setting cleared during SOFTWARE_RESET_ALL */
static void sdhci_xenon_reset_exit(struct sdhci_host *host,
					unsigned char slot_idx, u8 mask)
{
	/* Only SOFTWARE RESET ALL will clear the register setting */
	if (!(mask & SDHCI_RESET_ALL))
		return;

	/* Disable tuning request and auto-retuing again */
	xenon_slot_tuning_setup(host);

	xenon_set_acg(host, true);

	xenon_set_sdclk_off_idle(host, slot_idx, false);
}

static void sdhci_xenon_reset(struct sdhci_host *host, u8 mask)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	sdhci_reset(host, mask);
	sdhci_xenon_reset_exit(host, priv->slot_idx, mask);
}

static void xenon_platform_init(struct sdhci_host *host)
{
	xenon_set_acg(host, false);
}

/*
 * Xenon defines different values for HS200 and SDR104
 * in Host_Control_2
 */
static void xenon_set_uhs_signaling(struct sdhci_host *host,
				unsigned int timing)
{
	u16 ctrl_2;

	ctrl_2 = sdhci_readw(host, SDHCI_HOST_CONTROL2);
	/* Select Bus Speed Mode for host */
	ctrl_2 &= ~SDHCI_CTRL_UHS_MASK;
	if (timing == MMC_TIMING_MMC_HS200)
		ctrl_2 |= XENON_SDHCI_CTRL_HS200;
	else if (timing == MMC_TIMING_UHS_SDR104)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR104;
	else if (timing == MMC_TIMING_UHS_SDR12)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR12;
	else if (timing == MMC_TIMING_UHS_SDR25)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR25;
	else if (timing == MMC_TIMING_UHS_SDR50)
		ctrl_2 |= SDHCI_CTRL_UHS_SDR50;
	else if ((timing == MMC_TIMING_UHS_DDR50) ||
		 (timing == MMC_TIMING_MMC_DDR52))
		ctrl_2 |= SDHCI_CTRL_UHS_DDR50;
	else if (timing == MMC_TIMING_MMC_HS400)
		ctrl_2 |= XENON_SDHCI_CTRL_HS400;
	sdhci_writew(host, ctrl_2, SDHCI_HOST_CONTROL2);
}

static const struct sdhci_ops sdhci_xenon_ops = {
	.set_clock		= sdhci_set_clock,
	.set_bus_width		= sdhci_set_bus_width,
	.reset			= sdhci_xenon_reset,
	.set_uhs_signaling	= xenon_set_uhs_signaling,
	.platform_init		= xenon_platform_init,
	.get_max_clock		= sdhci_pltfm_clk_get_max_clock,
};

static const struct sdhci_pltfm_data sdhci_xenon_pdata = {
	.ops = &sdhci_xenon_ops,
	.quirks = SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
			SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12 |
			SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER |
			SDHCI_QUIRK_CAP_CLOCK_BASE_BROKEN,
	/*
	 * Add SOC specific quirks in the above .quirks, .quirks2
	 * fields.
	 */
};

/*
 * Xenon Specific Operations in mmc_host_ops
 */
static void xenon_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	unsigned long flags;

	/*
	 * HS400/HS200/eMMC HS doesn't have Preset Value register.
	 * However, sdhci_set_ios will read HS400/HS200 Preset register.
	 * Disable Preset Value register for HS400/HS200.
	 * eMMC HS with preset_enabled set will trigger a bug in
	 * get_preset_value().
	 */
	spin_lock_irqsave(&host->lock, flags);
	if ((ios->timing == MMC_TIMING_MMC_HS400) ||
		(ios->timing == MMC_TIMING_MMC_HS200) ||
		(ios->timing == MMC_TIMING_MMC_HS)) {
		host->preset_enabled = false;
		host->quirks2 |= SDHCI_QUIRK2_PRESET_VALUE_BROKEN;
	} else
		host->quirks2 &= ~SDHCI_QUIRK2_PRESET_VALUE_BROKEN;
	spin_unlock_irqrestore(&host->lock, flags);

	sdhci_set_ios(mmc, ios);
	xenon_phy_adj(host, ios);

	if (host->clock > DEFAULT_SDCLK_FREQ)
		xenon_set_sdclk_off_idle(host, priv->slot_idx, true);
}

static int __emmc_signal_voltage_switch(struct mmc_host *mmc,
				const unsigned char signal_voltage)
{
	u32 ctrl;
	unsigned char voltage_code;
	struct sdhci_host *host = mmc_priv(mmc);

	if (signal_voltage == MMC_SIGNAL_VOLTAGE_330)
		voltage_code = eMMC_VCCQ_3_3V;
	else if (signal_voltage == MMC_SIGNAL_VOLTAGE_180)
		voltage_code = eMMC_VCCQ_1_8V;
	else
		return -EINVAL;

	/*
	 * This host is for eMMC, XENON self-defined
	 * eMMC slot control register should be accessed
	 * instead of Host Control 2
	 */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= ~eMMC_VCCQ_MASK;
	ctrl |= voltage_code;
	sdhci_writel(host, ctrl, SDHC_SLOT_eMMC_CTRL);

	/* There is no standard to determine this waiting period */
	usleep_range(1000, 2000);

	/* Check whether io voltage switch is done */
	ctrl = sdhci_readl(host, SDHC_SLOT_eMMC_CTRL);
	ctrl &= eMMC_VCCQ_MASK;
	/*
	 * This bit is set only when regulator feedbacks the voltage switch
	 * results to Xenon SDHC.
	 * However, in actaul implementation, regulator might not provide
	 * this feedback.
	 * Thus we shall not rely on this bit to determine if switch failed.
	 * If the bit is not set, just throw a warning.
	 * Besides, error code should neither be returned.
	 */
	if (ctrl != voltage_code)
		pr_info("%s: Xenon fail to detect eMMC signal voltage stable\n",
					mmc_hostname(mmc));
	return 0;
}

static int xenon_emmc_signal_voltage_switch(struct mmc_host *mmc,
					struct mmc_ios *ios)
{
	unsigned char voltage = ios->signal_voltage;

	if ((voltage == MMC_SIGNAL_VOLTAGE_330) ||
		(voltage == MMC_SIGNAL_VOLTAGE_180))
		return __emmc_signal_voltage_switch(mmc, voltage);

	pr_err("%s: Xenon Unsupported signal voltage: %d\n",
				mmc_hostname(mmc), voltage);
	return -EINVAL;
}

static int xenon_start_signal_voltage_switch(struct mmc_host *mmc,
					     struct mmc_ios *ios)
{
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	/*
	 * Before SD/SDIO set signal voltage, SD bus clock should be
	 * disabled. However, sdhci_set_clock will also disable the Internal
	 * clock in mmc_set_signal_voltage().
	 * If Internal clock is disabled, the 3.3V/1.8V bit can not be updated.
	 * Thus here manually enable internal clock.
	 *
	 * After switch completes, it is unnessary to disable internal clock,
	 * since keeping internal clock active obeys SD spec.
	 */
	enable_xenon_internal_clk(host);

	if (priv->card_candidate) {
		if (mmc_card_mmc(priv->card_candidate))
			return xenon_emmc_signal_voltage_switch(mmc, ios);
	}

	return sdhci_start_signal_voltage_switch(mmc, ios);
}

/* After determining the slot is used for SDIO,
 * some addtional task is required.
 */
static void xenon_init_card(struct mmc_host *mmc, struct mmc_card *card)
{
	struct sdhci_host *host = mmc_priv(mmc);
	u32 reg;
	u8 slot_idx;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);

	/* Link the card for delay adjustment */
	priv->card_candidate = card;
	/* Set Xenon tuning */
	xenon_set_tuning_mode(host);
	xenon_set_tuning_count(host, priv->tuning_count);

	slot_idx = priv->slot_idx;
	if (!mmc_card_sdio(card)) {
		/* Re-enable the Auto-CMD12 cap flag. */
		host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags |= SDHCI_AUTO_CMD12;

		/* Clear SDIO Card Insterted indication */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg &= ~(1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);

		if (mmc_card_mmc(card)) {
			mmc->caps |= MMC_CAP_NONREMOVABLE | MMC_CAP_1_8V_DDR;
			/*
			 * Force to clear BUS_TEST to
			 * skip bus_test_pre and bus_test_post
			 */
			mmc->caps &= ~MMC_CAP_BUS_WIDTH_TEST;
			mmc->caps2 |= MMC_CAP2_HS400_1_8V |
				MMC_CAP2_HC_ERASE_SZ | MMC_CAP2_PACKED_CMD;
		}
		/* Xenon SD doesn't support DDR50 tuning.*/
		if (mmc_card_sd(card))
			mmc->caps2 |= MMC_CAP2_NO_DDR50_TUNING;
	} else {
		/*
		 * Delete the Auto-CMD12 cap flag.
		 * Otherwise, when sending multi-block CMD53,
		 * Driver will set Transfer Mode Register to enable Auto CMD12.
		 * However, SDIO device cannot recognize CMD12.
		 * Thus SDHC will time-out for waiting for CMD12 response.
		 */
		host->quirks &= ~SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;
		host->flags &= ~SDHCI_AUTO_CMD12;

		/*
		 * Set SDIO Card Insterted indication
		 * to inform that the current slot is for SDIO
		 */
		reg = sdhci_readl(host, SDHC_SYS_CFG_INFO);
		reg |= (1 << (slot_idx + SLOT_TYPE_SDIO_SHIFT));
		sdhci_writel(host, reg, SDHC_SYS_CFG_INFO);
	}
}

static void xenon_replace_mmc_host_ops(struct sdhci_host *host)
{
	host->mmc_host_ops.set_ios = xenon_set_ios;
	host->mmc_host_ops.start_signal_voltage_switch =
			xenon_start_signal_voltage_switch;
	host->mmc_host_ops.init_card = xenon_init_card;
}

static int xenon_probe_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct mmc_host *mmc = host->mmc;
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int err;
	u32 slot_idx;
	u32 tuning_count;

	/* Standard MMC property */
	err = mmc_of_parse(mmc);
	if (err)
		return err;

	/* Statndard SDHCI property */
	sdhci_get_of_property(pdev);

	/*
	 * Xenon Specific property:
	 * slotno: the index of slot. Refer to SDHC_SYS_CFG_INFO register
	 * tuning-count: the interval between re-tuning
	 * PHY type: "sdhc phy", "emmc phy 5.0" or "emmc phy 5.1"
	 */
	if (!of_property_read_u32(np, "xenon,slotno", &slot_idx))
		priv->slot_idx = slot_idx & 0xff;
	else
		priv->slot_idx = 0x0;

	if (!of_property_read_u32(np, "xenon,tuning-count", &tuning_count)) {
		if (unlikely(tuning_count >= TMR_RETUN_NO_PRESENT)) {
			pr_err("%s: Wrong Re-tuning Count. Set default value %d\n",
				mmc_hostname(mmc), DEF_TUNING_COUNT);
			tuning_count = DEF_TUNING_COUNT;
		}
		priv->tuning_count = tuning_count & 0xf;
	} else
		priv->tuning_count = DEF_TUNING_COUNT;

	err = xenon_phy_parse_dt(np, priv);
	return err;
}

static int xenon_slot_probe(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u8 slot_idx = priv->slot_idx;

	/* Enable slot */
	xenon_enable_slot(host, slot_idx);

	/* Enable ACG */
	xenon_set_acg(host, true);

	/* Enable Parallel Transfer Mode */
	xenon_enable_slot_parallel_tran(host, slot_idx);

	priv->timing = MMC_TIMING_FAKE;

	return 0;
}

static void xenon_slot_remove(struct sdhci_host *host)
{
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	u8 slot_idx = priv->slot_idx;

	/* disable slot */
	xenon_disable_slot(host, slot_idx);
}

static int sdhci_xenon_probe(struct platform_device *pdev)
{
	struct sdhci_pltfm_host *pltfm_host;
	struct sdhci_host *host;
	struct clk *clk, *axi_clk;
	struct sdhci_xenon_priv *priv;
	int err;

	host = sdhci_pltfm_init(pdev, &sdhci_xenon_pdata,
		sizeof(struct sdhci_xenon_priv));
	if (IS_ERR(host))
		return PTR_ERR(host);

	pltfm_host = sdhci_priv(host);
	priv = sdhci_pltfm_priv(pltfm_host);

	/*
	 * Link Xenon specific mmc_host_ops function,
	 * to replace standard ones in sdhci_ops.
	 */
	xenon_replace_mmc_host_ops(host);

	clk = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(clk)) {
		pr_err("%s: Failed to setup input clk.\n",
			mmc_hostname(host->mmc));
		err = PTR_ERR(clk);
		goto free_pltfm;
	}
	clk_prepare_enable(clk);
	pltfm_host->clk = clk;

	/*
	 * Some SOCs require additional clock to
	 * manage AXI bus clock.
	 * It is optional.
	 */
	axi_clk = devm_clk_get(&pdev->dev, "axi");
	if (!IS_ERR(axi_clk)) {
		clk_prepare_enable(axi_clk);
		priv->axi_clk = axi_clk;
	}

	err = xenon_probe_dt(pdev);
	if (err)
		goto err_clk;

	err = xenon_slot_probe(host);
	if (err)
		goto err_clk;

	err = sdhci_add_host(host);
	if (err)
		goto remove_slot;

	/* Set tuning functionality of this slot */
	xenon_slot_tuning_setup(host);

	return 0;

remove_slot:
	xenon_slot_remove(host);
err_clk:
	clk_disable_unprepare(pltfm_host->clk);
	if (!IS_ERR(axi_clk))
		clk_disable_unprepare(axi_clk);
free_pltfm:
	sdhci_pltfm_free(pdev);
	return err;
}

static int sdhci_xenon_remove(struct platform_device *pdev)
{
	struct sdhci_host *host = platform_get_drvdata(pdev);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_xenon_priv *priv = sdhci_pltfm_priv(pltfm_host);
	int dead = (readl(host->ioaddr + SDHCI_INT_STATUS) == 0xffffffff);

	xenon_slot_remove(host);

	sdhci_remove_host(host, dead);

	clk_disable_unprepare(pltfm_host->clk);
	clk_disable_unprepare(priv->axi_clk);

	sdhci_pltfm_free(pdev);

	return 0;
}

static const struct of_device_id sdhci_xenon_dt_ids[] = {
	{ .compatible = "marvell,xenon-sdhci",},
	{}
};
MODULE_DEVICE_TABLE(of, sdhci_xenon_dt_ids);

static struct platform_driver sdhci_xenon_driver = {
	.driver	= {
		.name	= "mv-xenon-sdhci",
		.of_match_table = sdhci_xenon_dt_ids,
		.pm = SDHCI_PLTFM_PMOPS,
	},
	.probe	= sdhci_xenon_probe,
	.remove	= sdhci_xenon_remove,
};

module_platform_driver(sdhci_xenon_driver);

MODULE_DESCRIPTION("SDHCI platform driver for Marvell Xenon SDHC");
MODULE_AUTHOR("Hu Ziji <huziji@marvell.com>");
MODULE_LICENSE("GPL v2");
