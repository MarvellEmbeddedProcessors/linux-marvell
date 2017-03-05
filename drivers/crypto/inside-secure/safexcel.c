/*
 * Copyright (C) 2016 Marvell
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/workqueue.h>

#include "safexcel.h"

static void eip197_prng_init(struct safexcel_crypto_priv *priv)
{
	/* disable PRNG and set to manual mode */
	writel(0, EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_CTRL);

	/* Write seed data */
	writel(EIP197_PE_EIP96_PRNG_SEED_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_SEED_L);
	writel(EIP197_PE_EIP96_PRNG_SEED_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_SEED_H);

	/* Write key data */
	writel(EIP197_PE_EIP96_PRNG_KEY_0_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_0_L);
	writel(EIP197_PE_EIP96_PRNG_KEY_0_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_0_H);
	writel(EIP197_PE_EIP96_PRNG_KEY_1_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_1_L);
	writel(EIP197_PE_EIP96_PRNG_KEY_1_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_1_H);

	/* Write LFSR data */
	writel(EIP197_PE_EIP96_PRNG_LFSR_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_LFSR_L);
	writel(EIP197_PE_EIP96_PRNG_LFSR_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_LFSR_H);

	/* enable PRNG and set to auto mode */
	writel(EIP197_PE_EIP96_PRNG_EN | EIP197_PE_EIP96_PRNG_AUTO,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_CTRL);
}

static int eip197_trc_cache_init(struct device *dev,
				 struct safexcel_crypto_priv *priv)
{
	u32 i, reg, reg_addr,
		rc_rec_wc,
		rc_rec1_wc,
		rc_rec2_wc,
		rc_record_cnt,
		rc_ht_wc,
		rc_ht_byte_offset,
		rc_ht_sz,
		rc_ht_factor,
		rc_ht_entry_cnt,
		rc_admn_ram_wc,
		rc_admn_ram_entry_cnt;

	rc_rec1_wc = EIP197_CS_TRC_REC_WC;
	rc_rec2_wc = EIP197_CS_TRC_LG_REC_WC;

	/* Determine the RC record size to use */
	if (rc_rec2_wc > rc_rec1_wc)
		rc_rec_wc = rc_rec2_wc;
	else
		rc_rec_wc = rc_rec1_wc;

	/* Calculate the maximum possible record count that
	 * the Record Cache Data RAM can contain
	 */
	rc_record_cnt = EIP197_TRC_RAM_WC / rc_rec_wc;

	/* rc_record_cnt is calculated using the configured RC Data RAM size. */

	/* rc_admn_ram_entry_cnt is calculated using
	 * the configured RC Admin RAM size.
	 */

	/* set the configured RC Admin RAM size */
	rc_admn_ram_wc = EIP197_TRC_ADMIN_RAM_WC;

	/* Calculate the maximum possible record count that
	 * the RC Hash Table (in Record Cache Administration RAM) can contain
	 */
	rc_admn_ram_entry_cnt = EIP197_RC_ADMN_MEMWORD_ENTRY_CNT * rc_admn_ram_wc /
				(EIP197_RC_ADMN_MEMWORD_WC +
				EIP197_RC_ADMN_MEMWORD_ENTRY_CNT *
				EIP197_RC_HEADER_WC);

	/* Try to extend the Hash Table in the RC Admin RAM */
	if (rc_record_cnt < rc_admn_ram_entry_cnt) {
		unsigned int ht_space_wc;

		/* Calculate the size of space available for the Hash Table */
		ht_space_wc = rc_admn_ram_wc -
			rc_record_cnt * EIP197_RC_HEADER_WC;

		/* Calculate maximum possible Hash Table entry count */
		rc_ht_entry_cnt = (ht_space_wc / EIP197_RC_ADMN_MEMWORD_WC) *
			EIP197_RC_ADMN_MEMWORD_ENTRY_CNT;
	} else {
		/* Extension impossible */
		rc_ht_entry_cnt = rc_admn_ram_entry_cnt;
	}

	/* Check minimum number of entries in the record cache */
	rc_ht_entry_cnt = max_t(u32, EIP197_RC_MIN_ENTRY_CNT,
				rc_ht_entry_cnt);

	/* Check maximum number of entries in the record cache */
	rc_ht_entry_cnt = min_t(u32, EIP197_RC_MAX_ENTRY_CNT,
				rc_ht_entry_cnt);

	/* calc power factor */
	rc_ht_factor = fls(rc_ht_entry_cnt) - 1;

	/* Round down to power of two */
	rc_ht_entry_cnt = 1 << rc_ht_factor;

	/* Hash Table Mask that determines the hash table size */
	if (rc_ht_factor >= EIP197_RC_HASH_TABLE_SIZE_POWER_FACTOR) {
		rc_ht_sz = rc_ht_factor -
			EIP197_RC_HASH_TABLE_SIZE_POWER_FACTOR;
	} else {
		/* Insufficient memory for Hash Table in the RC Admin RAM */
		dev_dbg(priv->dev, "Insufficient memory for HT\n");
		return -EINVAL;
	}

	/* Calculate the Hash Table size in 32-bit words */
	rc_ht_wc = rc_ht_entry_cnt / EIP197_RC_ADMN_MEMWORD_ENTRY_CNT *
		   EIP197_RC_ADMN_MEMWORD_WC;

	/* Recalculate the record count that fits the RC Admin RAM space
	 * without the Hash Table, restricting for the maximum records which
	 * fit the RC Data RAM Adjusted record count which fits the RC Admin
	 * RAM
	 */

	/* Maximum record count which fits the RC Data RAM - rc_record_cnt
	 * use the minimum of the two
	 */
	rc_record_cnt = min(rc_record_cnt, (rc_admn_ram_wc - rc_ht_wc) /
			    EIP197_RC_HEADER_WC);

	/* enable record cache RAM access */
	reg = ~EIP197_TRC_ENABLE_MASK & readl(priv->base + EIP197_CS_RAM_CTRL);
	reg |= EIP197_TRC_ENABLE(0);
	writel(reg, priv->base + EIP197_CS_RAM_CTRL);

	/* Clear all ECC errors */
	writel(0, priv->base + EIP197_TRC_ECCCTRL);

	/* Take Record Cache into reset
	 * Make cache administration RAM accessible
	 */
	writel(EIP197_TRC_SW_RESET, priv->base + EIP197_TRC_PARAMS);

	/* Clear all record administration words in Record Cache
	 * administration RAM
	 */
	for (i = 0; i < rc_record_cnt; i++) {
		/* Calculate byte offset for the current record */
		reg_addr = EIP197_CLASSIF_RAM_ACCESS_SPACE + i * EIP197_RC_HEADER_WC * sizeof(u32);

		/* Write word 0 */
		writel((EIP197_RC_NULL << 20) |		/* Hash_Collision_Prev */
			 (EIP197_RC_NULL << 10),	/* Hash_Collision_Next */
			 priv->base + reg_addr);

		/* Write word 1 */
		reg_addr += sizeof(u32);

		if (i == rc_record_cnt - 1) {
			/* last record */
			writel(((i - 1) << 10) |	/* Free_List_Prev */
				 EIP197_RC_NULL,	/* Free_List_Prev */
				 priv->base + reg_addr);
		} else if (!i) {
			/* first record */
			writel((EIP197_RC_NULL << 10) |	/* Free_List_Prev */
			       (i + 1),			/* Free_List_Prev */
			       priv->base + reg_addr);
		} else {
			/* All other records */
			writel(((i - 1) << 10) |	/* Free_List_Prev */
			       (i + 1),			/* Free_List_Prev */
			       priv->base + reg_addr);
		}

		/* Write word 2 */
		reg_addr += sizeof(u32);
		writel(0, priv->base + reg_addr);	/* Address_Key, low bits */

		/* Write word 3 */
		reg_addr += sizeof(u32);
		writel(0, priv->base + reg_addr);	/* Address_Key, high bits */
	}

	/* Calculate byte offset for hash table */
	rc_ht_byte_offset = EIP197_CLASSIF_RAM_ACCESS_SPACE +
			    rc_record_cnt * EIP197_RC_HEADER_WC * sizeof(u32);

	/* Clear all hash table words */
	for (i = 0; i < rc_ht_wc; i++)
		writel(GENMASK(29, 0),
		       priv->base + rc_ht_byte_offset + i * sizeof(u32));

	/* Disable Record Cache RAM access */
	writel(0, priv->base + EIP197_CS_RAM_CTRL);

	/* Write head and tail pointers to the RC Free Chain */
	writel(((rc_record_cnt - 1) & GENMASK(9, 0)) << 16,
		 priv->base + EIP197_TRC_FREECHAIN);

	/* Set Hash Table start */
	reg = ((EIP197_CS_TRC_REC_WC << 18) |
		(EIP197_RC_DMA_WR_COMB_DLY << 10) |
		(rc_record_cnt & GENMASK(9, 0)));
	writel(reg, priv->base + EIP197_TRC_PARAMS2);

	/* Select the highest clock count as specified by
	 * the Host and the Firmware for the FRC
	 */

	/* Take Record Cache out of reset */
	reg = ((rc_rec2_wc & GENMASK(8, 0)) << 18)	| /* large record_size */
	       (1 << 10)				| /* block_timebase */
	       ((rc_ht_sz & GENMASK(2, 0)) << 4);	  /* hash_table_size */
	writel(reg, priv->base + EIP197_TRC_PARAMS);

	return 0;
}

static int eip197_load_fw(struct device *dev, struct safexcel_crypto_priv *priv)
{
	const struct firmware	*fw[MAX_FW_NR] = {0};
	const u32		*fw_data;
	int			i, ret;
	u32			fw_size, reg;
	const char		*fw_name[MAX_FW_NR] = {"ifpp.bin", "ipue.bin"};

	for (i = 0; i < MAX_FW_NR; i++) {
		ret = request_firmware(&fw[i], fw_name[i], dev);
		if (ret) {
			dev_err(dev, "request_firmware failed (fw: %s)\n",
				fw_name[i]);
			goto release_fw;
		}
	 }

	/* Clear EIP-207c ICE Scratchpad RAM where the firmware */
	reg = (EIP197_PE_ICE_SCRATCH_CTRL_DFLT				|
	       EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_TIMER			|
	       EIP197_PE_ICE_SCRATCH_CTRL_TIMER_EN			|
	       ((0x1 << EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_OFFSET) &
		EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_MASK)		|
	       EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_ACCESS);
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_SCRATCH_CTRL_OFFSET);

	/* Write the ICE Scratchpad RAM with 0 */
	for (i = 0; i < EIP197_NUM_OF_SCRATCH_BLOCKS; i++)
		writel(0, EIP197_PE(priv) + EIP197_PE_ICE_SCRATCH_RAM(i));

	/* Reset the Input Flow Post-Processor micro-engine (IFPP) to make its
	 * program RAM accessible.
	 */
	reg = (EIP197_PE_ICE_FPP_CTRL_SW_RESET			|
	       EIP197_PE_ICE_FPP_CTRL_CLR_ECC_CORR		|
	       EIP197_PE_ICE_FPP_CTRL_CLR_ECC_NON_CORR);
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_FPP_CTRL);

	/* Enable access to IFPP Program RAM */
	reg = (EIP197_PE_ICE_RAM_CTRL_DFLT |
	       EIP197_PE_ICE_RAM_CTRL_FPP_PROG_EN);
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL);

	/* Save pointer to the data and the size of the data */
	fw_data = (const u32 *)fw[IFPP_FW]->data;
	fw_size = fw[IFPP_FW]->size / sizeof(u32);

	/* Write the Input Flow post-Processor micro-Engine firmware */
	for (i = 0; i < fw_size; i++)
		writel(be32_to_cpu(*(fw_data + i)),
		       priv->base + EIP197_CLASSIF_RAM_ACCESS_SPACE + (i * 4));

	/* Disable access to IFPP Program RAM
	 * Enable access to IPUE Program RAM
	 */
	reg = (EIP197_PE_ICE_RAM_CTRL_DFLT |
	       EIP197_PE_ICE_RAM_CTRL_PUE_PROG_EN);
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL);

	/* Reset the Input Pull-Up micro-Engine (IPUE) to make its
	 * program RAM accessible.
	 */
	reg = (EIP197_PE_ICE_PUE_CTRL_SW_RESET		|
	       EIP197_PE_ICE_PUE_CTRL_CLR_ECC_CORR	|
	       EIP197_PE_ICE_PUE_CTRL_CLR_ECC_NON_CORR);
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_PUE_CTRL);

	/* Save pointer to the data and the size of the data */
	fw_data = (const u32 *)fw[IPUE_FW]->data;
	fw_size = fw[IPUE_FW]->size / sizeof(u32);

	/* Write the Input Flow post-Processor micro-Engine firmware */
	for (i = 0; i < fw_size; i++)
		writel(be32_to_cpu(*(fw_data + i)), EIP197_RAM(priv) + (i * 4));

	/* Disable access to IPUE Program RAM */
	reg = EIP197_PE_ICE_RAM_CTRL_DFLT;
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL);

	/* Release IFPP from reset */
	reg = readl(priv->base + EIP197_PE_ICE_PUE_CTRL);
	reg &= ~EIP197_PE_ICE_FPP_CTRL_SW_RESET;
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_FPP_CTRL);

	/* Release IPUE from reset */
	reg = readl(priv->base + EIP197_PE_ICE_PUE_CTRL);
	reg &= ~EIP197_PE_ICE_PUE_CTRL_SW_RESET;
	writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_PUE_CTRL);

	for (i = 0; i < MAX_FW_NR; i++)
		release_firmware(fw[i]);

	return 0;

release_fw:
	for (i = 0; i < MAX_FW_NR; i++)
		release_firmware(fw[i]);

	return ret;
}

static void eip_priv_unit_offset_init(struct safexcel_crypto_priv *priv)
{
	struct safexcel_unit_offset *unit_off = &priv->unit_off;

	unit_off->hia_aic = EIP197_HIA_AIC_ADDR;
	unit_off->hia_aic_g = EIP197_HIA_AIC_G_ADDR;
	unit_off->hia_aic_r = EIP197_HIA_AIC_R_ADDR;
	unit_off->hia_xdr = EIP197_HIA_AIC_xDR_ADDR;
	unit_off->hia_dfe = EIP197_HIA_AIC_DFE_ADDR;
	unit_off->hia_dfe_thrd = EIP197_HIA_AIC_DFE_THRD_ADDR;
	unit_off->hia_dse = EIP197_HIA_AIC_DSE_ADDR;
	unit_off->hia_dse_thrd = EIP197_HIA_AIC_DSE_THRD_ADDR;
	unit_off->hia_gen_cfg = EIP197_HIA_GC;
	unit_off->pe = EIP197_HIA_PE_ADDR;
}

static int eip_hw_init(struct device *dev, struct safexcel_crypto_priv *priv)
{
	u32 version, val;
	int i, ret;

	/* Determine endianness and configure byte swap */
	version = readl(EIP197_HIA_AIC(priv) + EIP197_HIA_VERSION);
	val = readl(EIP197_HIA_AIC(priv) + EIP197_HIA_MST_CTRL);

	if ((version & 0xffff) == EIP197_HIA_VERSION_BE)
		val |= EIP197_HIA_SLAVE_BYTE_SWAP;
	else if (((version >> 16) & 0xffff) == EIP197_HIA_VERSION_LE)
		val |= (EIP197_HIA_SLAVE_NO_BYTE_SWAP);

	writel(val, EIP197_HIA_AIC(priv) + EIP197_HIA_MST_CTRL);


	/* configure wr/rd cache values */
	val = MST_CTRL_RD_CACHE(RD_CACHE_4BITS) | MST_CTRL_WD_CACHE(WR_CACHE_4BITS);
	writel(val, EIP197_HIA_GEN_CFG(priv) + EIP197_MST_CTRL);

	/*
	 * Interrupts reset
	 */

	/* Disable all global interrupts */
	writel(0, EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ENABLE_CTRL);

	/* Clear any pending interrupt */
	writel(GENMASK(31, 0), EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ACK);

	/*
	 * Data Fetch Engine configuration
	 */

	/* Reset all DFE threads */
	writel(EIP197_DxE_THR_CTRL_RESET_PE,
	       EIP197_HIA_DFE_THRD(priv) + EIP197_HIA_DFE_THR_CTRL);

	/* Reset HIA input interface arbiter */
	writel(EIP197_HIA_RA_PE_CTRL_RESET,
	       EIP197_HIA_AIC(priv) + EIP197_HIA_RA_PE_CTRL);

	/* DMA transfer size to use */
	val = EIP197_HIA_DFE_CFG_DIS_DEBUG;
	val |= EIP197_HIA_DxE_CFG_MIN_DATA_SIZE(5) | EIP197_HIA_DxE_CFG_MAX_DATA_SIZE(9);
	val |= EIP197_HIA_DxE_CFG_MIN_CTRL_SIZE(5) | EIP197_HIA_DxE_CFG_MAX_CTRL_SIZE(7);
	val |= EIP197_HIA_DxE_CFG_DATA_CACHE_CTRL(RD_CACHE_3BITS);
	val |= EIP197_HIA_DxE_CFG_CTRL_CACHE_CTRL(RD_CACHE_3BITS);
	writel(val, EIP197_HIA_DFE(priv) + EIP197_HIA_DFE_CFG);

	/* Leave the DFE threads reset state */
	writel(0, EIP197_HIA_DFE_THRD(priv) + EIP197_HIA_DFE_THR_CTRL);

	/* Configure the procesing engine thresholds */
	writel(EIP197_PE_IN_xBUF_THRES_MIN(5) | EIP197_PE_IN_xBUF_THRES_MAX(9),
	      EIP197_PE(priv) + EIP197_PE_IN_DBUF_THRES);
	writel(EIP197_PE_IN_xBUF_THRES_MIN(5) | EIP197_PE_IN_xBUF_THRES_MAX(7),
	      EIP197_PE(priv) + EIP197_PE_IN_TBUF_THRES);

	/* enable HIA input interface arbiter and rings */
	writel(EIP197_HIA_RA_PE_CTRL_EN | GENMASK(priv->config.rings - 1, 0),
	       EIP197_HIA_AIC(priv) + EIP197_HIA_RA_PE_CTRL);

	/*
	 * Data Store Engine configuration
	 */

	/* Reset all DSE threads */
	writel(EIP197_DxE_THR_CTRL_RESET_PE,
	       EIP197_HIA_DSE_THRD(priv) + EIP197_HIA_DSE_THR_CTRL);

	/* Wait for all DSE threads to complete */
	while ((readl(EIP197_HIA_DSE_THRD(priv) + EIP197_HIA_DSE_THR_STAT) &
	       GENMASK(15, 12)) != GENMASK(15, 12))
		;

	/* DMA transfer size to use */
	val = EIP197_HIA_DSE_CFG_DIS_DEBUG;
	val |= EIP197_HIA_DxE_CFG_MIN_DATA_SIZE(7) | EIP197_HIA_DxE_CFG_MAX_DATA_SIZE(8);
	val |= EIP197_HIA_DxE_CFG_DATA_CACHE_CTRL(RD_CACHE_3BITS);
	writel(val, EIP197_HIA_DSE(priv) + EIP197_HIA_DSE_CFG);

	/* Leave the DSE threads reset state */
	writel(0, EIP197_HIA_DSE_THRD(priv) + EIP197_HIA_DSE_THR_CTRL);

	/* Configure the procesing engine thresholds */
	writel(EIP197_PE_OUT_DBUF_THRES_MIN(7) | EIP197_PE_OUT_DBUF_THRES_MAX(8),
	       EIP197_PE(priv) + EIP197_PE_OUT_DBUF_THRES);

	/*
	 * Processing Engine configuration
	 */

	/*
	 * Command Descriptor Rings prepare
	 */
	for (i = 0; i < priv->config.rings; i++) {
		/* Clear interrupts for this ring */
		writel(GENMASK(31, 0),
		       EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ENABLE_CLR(i));

		/* disable external triggering */
		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_CFG);

		/* Clear the pending prepared counter */
		writel(EIP197_xDR_PREP_CLR_COUNT,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_PREP_COUNT);

		/* Clear the pending processed counter */
		writel(EIP197_xDR_PROC_CLR_COUNT,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_PROC_COUNT);

		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_PREP_PNTR);
		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_PROC_PNTR);

		writel((EIP197_DEFAULT_RING_SIZE * priv->config.cd_offset) << 2,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_CDR(i) + EIP197_HIA_xDR_RING_SIZE);
	}

	/*
	 * Result Descriptor Ring prepare
	 */

	for (i = 0; i < priv->config.rings; i++) {
		/* disable external triggering*/
		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_CFG);

		/* Clear the pending prepared counter */
		writel(EIP197_xDR_PREP_CLR_COUNT,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_PREP_COUNT);

		/* Clear the pending processed counter */
		writel(EIP197_xDR_PROC_CLR_COUNT,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_PROC_COUNT);

		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_PREP_PNTR);
		writel(0, EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_PROC_PNTR);

		/* ring size */
		writel((EIP197_DEFAULT_RING_SIZE * priv->config.rd_offset) << 2,
		       EIP197_HIA_AIC_xDR(priv) + EIP197_HIA_RDR(i) + EIP197_HIA_xDR_RING_SIZE);
	}

	/* Enable command descriptor rings */
	writel(EIP197_DxE_THR_CTRL_EN | GENMASK(priv->config.rings - 1, 0),
	       EIP197_HIA_DFE_THRD(priv) + EIP197_HIA_DFE_THR_CTRL);

	/* Enable result descriptor rings */
	writel(EIP197_DxE_THR_CTRL_EN | GENMASK(priv->config.rings - 1, 0),
	       EIP197_HIA_DSE_THRD(priv) + EIP197_HIA_DSE_THR_CTRL);

	/* Clear any HIA interrupt */
	writel(GENMASK(30, 20), EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ACK);

	/* init PRNG */
	eip197_prng_init(priv);

	/* init transform record cache */
	ret = eip197_trc_cache_init(dev, priv);
	if (ret) {
		dev_err(dev, "eip197_trc_cache_init failed\n");
		return ret;
	}

	/* Firmware load */
	ret = eip197_load_fw(dev, priv);
	if (ret) {
		dev_err(dev, "eip197_load_fw failed\n");
		return ret;
	}

	return 0;
}

static void safexcel_configure(struct safexcel_crypto_priv *priv)
{
	u32 val;

	val = readl(EIP197_HIA_AIC_G(priv) + EIP197_HIA_OPTIONS);
	priv->config.rings = (val & GENMASK(3, 0));
}

static int safexcel_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct safexcel_crypto_priv *priv;
	int ret;

	priv = devm_kzalloc(dev, sizeof(struct safexcel_crypto_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->eip_type = (enum safexcel_eip_type)of_device_get_match_data(dev);

	eip_priv_unit_offset_init(priv);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base)) {
		dev_err(dev, "failed to get resource\n");
		return PTR_ERR(priv->base);
	}

	priv->clk = of_clk_get(dev->of_node, 0);
	if (!IS_ERR(priv->clk)) {
		ret = clk_prepare_enable(priv->clk);
		if (ret) {
			dev_err(dev, "unable to enable clk (%d)\n", ret);
			return ret;
		}
	} else {
		/* The clock isn't mandatory */
		if (PTR_ERR(priv->clk) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	safexcel_configure(priv);

	platform_set_drvdata(pdev, priv);

	spin_lock_init(&priv->lock);

	ret = eip_hw_init(dev, priv);
	if (ret) {
		dev_err(dev, "EIP h/w init failed (%d)\n", ret);
		goto err_clk;
	}

	return 0;

err_clk:
	clk_disable_unprepare(priv->clk);
	return ret;
}


static int safexcel_remove(struct platform_device *pdev)
{
	struct safexcel_crypto_priv *priv = platform_get_drvdata(pdev);

	clk_disable_unprepare(priv->clk);

	return 0;
}

static const struct of_device_id safexcel_of_match_table[] = {
	{
		.compatible = "inside-secure,safexcel-eip197",
		.data = (void *)EIP197,
	},
};


static struct platform_driver  crypto_safexcel = {
	.probe		= safexcel_probe,
	.remove		= safexcel_remove,
	.driver		= {
		.name	= "crypto-safexcel",
		.of_match_table = safexcel_of_match_table,
	},
};
module_platform_driver(crypto_safexcel);

MODULE_AUTHOR("Antoine Tenart <antoine.tenart@free-electrons.com>");
MODULE_DESCRIPTION("Support for SafeXcel Cryptographic Engines EIP197");
MODULE_LICENSE("GPL v2");
