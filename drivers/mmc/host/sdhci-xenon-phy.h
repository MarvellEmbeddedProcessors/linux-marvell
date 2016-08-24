/* linux/drivers/mmc/host/sdhci-xenon-emmc-phy.h
 *
 * Author: Hu Ziji <huziji@marvell.com>
 * Date:		2016-6-31
 *
 *  Copyright (C) 2016 Marvell, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
#ifndef SDHCI_XENON_PHY_H_
#define SDHCI_XENON_PHY_H_

#include <linux/types.h>
#include "sdhci.h"

/* Register base for eMMC PHY 5.0 Version */
#define EMMC_5_0_PHY_REG_BASE			0x160
/* Register base for eMMC PHY 5.1 Version */
#define EMMC_PHY_REG_BASE			0x170

#define EMMC_PHY_TIMING_ADJUST			EMMC_PHY_REG_BASE
#define EMMC_5_0_PHY_TIMING_ADJUST		EMMC_5_0_PHY_REG_BASE
#define TIMING_ADJUST_SLOW_MODE			(1 << 29)
#define TIMING_ADJUST_SDIO_MODE			(1 << 28)
#define OUTPUT_QSN_PHASE_SELECT			(1 << 17)
#define SAMPL_INV_QSP_PHASE_SELECT		(1 << 18)
#define SAMPL_INV_QSP_PHASE_SELECT_SHIFT	18
#define PHY_INITIALIZAION			(1 << 31)
#define WAIT_CYCLE_BEFORE_USING_MASK		0xf
#define WAIT_CYCLE_BEFORE_USING_SHIFT		12
#define FC_SYNC_EN_DURATION_MASK		0xf
#define FC_SYNC_EN_DURATION_SHIFT		8
#define FC_SYNC_RST_EN_DURATION_MASK		0xf
#define FC_SYNC_RST_EN_DURATION_SHIFT		4
#define FC_SYNC_RST_DURATION_MASK		0xf
#define FC_SYNC_RST_DURATION_SHIFT		0

#define EMMC_PHY_FUNC_CONTROL			(EMMC_PHY_REG_BASE + 0x4)
#define EMMC_5_0_PHY_FUNC_CONTRL		(EMMC_5_0_PHY_REG_BASE + 0x4)
#define ASYNC_DDRMODE_MASK			(1 << 23)
#define ASYNC_DDRMODE_SHIFT			23
#define CMD_DDR_MODE				(1 << 16)
#define DQ_DDR_MODE_SHIFT			8
#define DQ_DDR_MODE_MASK			0xff
#define DQ_ASYNC_MODE				(1 << 4)

#define EMMC_PHY_PAD_CONTROL			(EMMC_PHY_REG_BASE + 0x8)
#define EMMC_5_0_PHY_PAD_CONTROL		(EMMC_5_0_PHY_REG_BASE + 0x8)
#define REC_EN_SHIFT				24
#define REC_EN_MASK				0xf
#define FC_DQ_RECEN				(1 << 24)
#define FC_CMD_RECEN				(1 << 25)
#define FC_QSP_RECEN				(1 << 26)
#define FC_QSN_RECEN				(1 << 27)
#define OEN_QSN					(1 << 28)
#define AUTO_RECEN_CTRL				(1 << 30)
#define FC_ALL_CMOS_RECEIVER			0xf000

#define EMMC5_FC_QSP_PD				(1 << 18)
#define EMMC5_FC_QSP_PU				(1 << 22)
#define EMMC5_FC_CMD_PD				(1 << 17)
#define EMMC5_FC_CMD_PU				(1 << 21)
#define EMMC5_FC_DQ_PD				(1 << 16)
#define EMMC5_FC_DQ_PU				(1 << 20)

#define EMMC_PHY_PAD_CONTROL1			(EMMC_PHY_REG_BASE + 0xc)
#define EMMC5_1_FC_QSP_PD			(1 << 9)
#define EMMC5_1_FC_QSP_PU			(1 << 25)
#define EMMC5_1_FC_CMD_PD			(1 << 8)
#define EMMC5_1_FC_CMD_PU			(1 << 24)
#define EMMC5_1_FC_DQ_PD			0xff
#define EMMC5_1_FC_DQ_PU			(0xff << 16)

#define EMMC_PHY_PAD_CONTROL2			(EMMC_PHY_REG_BASE + 0x10)
#define EMMC_5_0_PHY_PAD_CONTROL2		(EMMC_5_0_PHY_REG_BASE + 0xc)
#define ZNR_MASK				0x1f
#define ZNR_SHIFT				8
#define ZPR_MASK				0x1f
/* Perferred ZNR and ZPR value vary between different boards.
 * The specific ZNR and ZPR value should be defined here
 * according to board actual timing.
 */
#define ZNR_DEF_VALUE				0xf
#define ZPR_DEF_VALUE				0xf

#define EMMC_PHY_DLL_CONTROL			(EMMC_PHY_REG_BASE + 0x14)
#define EMMC_5_0_PHY_DLL_CONTROL		(EMMC_5_0_PHY_REG_BASE + 0x10)
#define DLL_ENABLE				(1 << 31)
#define DLL_UPDATE_STROBE_5_0			(1 << 30)
#define DLL_REFCLK_SEL				(1 << 30)
#define DLL_UPDATE				(1 << 23)
#define DLL_PHSEL1_SHIFT			24
#define DLL_PHSEL0_SHIFT			16
#define DLL_PHASE_MASK				0x3f
#define DLL_PHASE_90_DEGREE			0x1f
#define DLL_FAST_LOCK				(1 << 5)
#define DLL_GAIN2X				(1 << 3)
#define DLL_BYPASS_EN				(1 << 0)

#define EMMC_5_0_PHY_LOGIC_TIMING_ADJUST	(EMMC_5_0_PHY_REG_BASE + 0x14)
#define EMMC_PHY_LOGIC_TIMING_ADJUST		(EMMC_PHY_REG_BASE + 0x18)

#define SOC_PAD_1_8V				0x1
#define SOC_PAD_3_3V				0x0

enum sampl_fix_delay_phase {
	PHASE_0_DEGREE = 0x0,
	PHASE_90_DEGREE = 0x1,
	PHASE_180_DEGREE = 0x2,
	PHASE_270_DEGREE = 0x3,
};

#define SDH_PHY_SLOT_DLL_CTRL			(0x138)
#define SDH_PHY_ENABLE_DLL			(1 << 1)
#define SDH_PHY_FAST_LOCK_EN			(1 << 5)

#define SDH_PHY_SLOT_DLL_PHASE_SEL		(0x13C)
#define SDH_PHY_DLL_UPDATE_TUNING		(1 << 15)

struct xenon_phy_ops {
	void (*strobe_delay_adj)(struct sdhci_host *host,
				struct mmc_card *card);
	int (*fix_sampl_delay_adj)(struct sdhci_host *host,
				struct mmc_card *card);
	void (*phy_set)(struct sdhci_host *host, unsigned char timing);
	void (*config_tuning)(struct sdhci_host *host);
	void (*soc_pad_ctrl)(struct sdhci_host *host, unsigned char signal_voltage);
};
#endif
