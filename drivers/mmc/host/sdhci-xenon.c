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
#include <linux/of.h>
#include <linux/of_device.h>
#include "sdhci.h"
#include "sdhci-pltfm.h"

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

#define SDHC_SLOT_RETUNING_REQ_CTRL		0x0144
#define  RETUNING_COMPATIBLE			0x1

#define SDHC_SLOT_AUTO_RETUNING_CTRL		0x0148
#define  ENABLE_AUTO_RETUNING			0x1

/* Xenon quirks */
#define SDHCI_QUIRK_XENON_EMMC_SLOT		(1 << 0) /* only support eMMC */

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

/* SDHC private parameters */
struct sdhci_xenon_priv {
	/* The three fields in below records the current
	 * setting of Xenon SDHC.
	 * Driver will call a Sampling Fixed Delay Adjustment
	 * if any setting is changed.
	 */
	unsigned char	bus_width;
	unsigned char	tuning_count;
	unsigned int	clock;
	unsigned int	quirks; /* Xenon private quirks */
};

/*
 * Xenon Specific Initialization Operations
 */
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
};

static struct sdhci_pltfm_data sdhci_xenon_pdata = {
	.ops = &sdhci_xenon_ops,
	.quirks = SDHCI_QUIRK_NO_ENDATTR_IN_NOPDESC |
		  SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12 |
		  SDHCI_QUIRK_NO_SIMULT_VDD_AND_POWER,
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
	struct sdhci_xenon_priv *priv =
				 (struct sdhci_xenon_priv *)pltfm_host->private;

	sdhci_xenon_set_tuning_mode(host);

	sdhci_xenon_set_tuning_count(host, priv->tuning_count);
}

static void sdhci_xenon_slot_probe(struct sdhci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	u8 slotno = mmc->slotno;

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

	sdhci_xenon_slot_probe(host);

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
