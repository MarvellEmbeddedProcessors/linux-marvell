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
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

#include <crypto/internal/hash.h>
#include <crypto/internal/skcipher.h>

#include "safexcel.h"

/* eip_in_use holds the active engine id */
static int eip_in_use = -1;

/* Module param to save the assigned rings to the Kernel */
static uint rings[MAX_EIP_DEVICE] = {RINGS_UNINITIALIZED, RINGS_UNINITIALIZED};

/* Initialize pseudo random generator */
static void eip197_prng_init(struct safexcel_crypto_priv *priv, int pe)
{
	/* disable PRNG and set to manual mode */
	writel(0, EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_CTRL(pe));

	/* Write seed data */
	writel(EIP197_PE_EIP96_PRNG_SEED_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_SEED_L(pe));
	writel(EIP197_PE_EIP96_PRNG_SEED_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_SEED_H(pe));

	/* Write key data */
	writel(EIP197_PE_EIP96_PRNG_KEY_0_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_0_L(pe));
	writel(EIP197_PE_EIP96_PRNG_KEY_0_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_0_H(pe));
	writel(EIP197_PE_EIP96_PRNG_KEY_1_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_1_L(pe));
	writel(EIP197_PE_EIP96_PRNG_KEY_1_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_KEY_1_H(pe));

	/* Write LFSR data */
	writel(EIP197_PE_EIP96_PRNG_LFSR_L_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_LFSR_L(pe));
	writel(EIP197_PE_EIP96_PRNG_LFSR_H_VAL,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_LFSR_H(pe));

	/* enable PRNG and set to auto mode */
	writel(EIP197_PE_EIP96_PRNG_EN | EIP197_PE_EIP96_PRNG_AUTO,
	       EIP197_PE(priv) + EIP197_PE_EIP96_PRNG_CTRL(pe));
}

/* Initialize transform record cache */
static int eip197_trc_cache_init(struct safexcel_crypto_priv *priv)
{
	u32 i, reg, reg_addr, rc_rec_wc, rc_rec1_wc, rc_rec2_wc,
	    rc_record_cnt, rc_ht_wc, rc_ht_byte_offset, rc_ht_sz,
	    rc_ht_factor, rc_ht_entry_cnt, rc_admn_ram_wc,
	    rc_admn_ram_entry_cnt;

	if (priv->eip197_hw_ver == EIP197B) {
		rc_rec1_wc = EIP197B_CS_TRC_REC_WC;
		rc_rec2_wc = EIP197B_CS_TRC_LG_REC_WC;
	} else {
		rc_rec1_wc = EIP197D_CS_TRC_REC_WC;
		rc_rec2_wc = EIP197D_CS_TRC_LG_REC_WC;
	}

	/* Determine the RC record size to use */
	if (rc_rec2_wc > rc_rec1_wc)
		rc_rec_wc = rc_rec2_wc;
	else
		rc_rec_wc = rc_rec1_wc;

	/* Calculate the maximum possible record count that
	 * the Record Cache Data RAM can contain
	 */
	if (priv->eip197_hw_ver == EIP197B)
		rc_record_cnt = EIP197B_TRC_RAM_WC / rc_rec_wc;
	else
		rc_record_cnt = EIP197D_TRC_RAM_WC / rc_rec_wc;

	/* rc_record_cnt is calculated using the configured RC Data RAM size. */

	/* rc_admn_ram_entry_cnt is calculated using
	 * the configured RC Admin RAM size.
	 */

	/* set the configured RC Admin RAM size */
	if (priv->eip197_hw_ver == EIP197B)
		rc_admn_ram_wc = EIP197B_TRC_ADMIN_RAM_WC;
	else
		rc_admn_ram_wc = EIP197D_TRC_ADMIN_RAM_WC;

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
		reg_addr = EIP197_CLASSIFICATION_RAMS + i * EIP197_RC_HEADER_WC * sizeof(u32);

		/* Write word 0 */
		writel((EIP197_RC_NULL << EIP197_RC_HASH_COLLISION_PREV) | /* Hash_Collision_Prev */
		       (EIP197_RC_NULL << EIP197_RC_HASH_COLLISION_NEXT),  /* Hash_Collision_Next */
			 priv->base + reg_addr);

		/* Write word 1 */
		reg_addr += sizeof(u32);

		if (i == rc_record_cnt - 1) {
			/* last record */
			writel(((i - 1) << EIP197_RC_FREE_LIST_PREV) |	/* Free_List_Prev */
				 EIP197_RC_NULL,			/* Free_List_Prev */
				 priv->base + reg_addr);
		} else if (!i) {
			/* first record */
			writel((EIP197_RC_NULL << EIP197_RC_FREE_LIST_PREV) |	/* Free_List_Prev */
			       (i + 1),						/* Free_List_Prev */
			       priv->base + reg_addr);
		} else {
			/* All other records */
			writel(((i - 1) << EIP197_RC_FREE_LIST_PREV) |	/* Free_List_Prev */
			       (i + 1),					/* Free_List_Prev */
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
	rc_ht_byte_offset = EIP197_CLASSIFICATION_RAMS +
			    rc_record_cnt * EIP197_RC_HEADER_WC * sizeof(u32);

	/* Clear all hash table words */
	for (i = 0; i < rc_ht_wc; i++)
		writel(EIP197_RC_HASH_TABLE_MASK,
		       priv->base + rc_ht_byte_offset + i * sizeof(u32));

	/* Disable Record Cache RAM access */
	writel(0, priv->base + EIP197_CS_RAM_CTRL);

	/* Write head and tail pointers to the RC Free Chain */
	writel(((rc_record_cnt - 1) & EIP197_TRC_TAIL_PTR_MASK) <<
	       EIP197_TRC_TAIL_PTR_OFFSET,
	       priv->base + EIP197_TRC_FREECHAIN);

	/* Set Hash Table start */
	reg = ((rc_rec1_wc << EIP197_TRC_RECORD_SIZE2_OFFSET)		|
		(EIP197_RC_DMA_WR_COMB_DLY << EIP197_TRC_DMA_WR_COMB_DLY_OFFSET)|
		(rc_record_cnt & EIP197_TRC_HASH_TABLE_START_MASK));
	writel(reg, priv->base + EIP197_TRC_PARAMS2);

	/* Select the highest clock count as specified by
	 * the Host and the Firmware for the FRC
	 */

	/* Take Record Cache out of reset */
	reg = ((rc_rec2_wc & EIP197_TRC_RECORD_SIZE_MASK) <<
	       EIP197_TRC_RECORD_SIZE_OFFSET)			| /* large record_size */
	       (1 << EIP197_TRC_BLCOK_TIMEBASE_OFFSET)		| /* block_timebase */
	       ((rc_ht_sz & EIP197_TRC_HASH_TABLE_SIZE_MASK) <<
	       EIP197_TRC_HASH_TABLE_SIZE_OFFSET);		  /* hash_table_size */
	writel(reg, priv->base + EIP197_TRC_PARAMS);

	return 0;
}

/* Load EIP197 firmare into the engine */
static int eip197_load_fw(struct safexcel_crypto_priv *priv)
{
	const struct firmware	*fw[FW_NB] = {0};
	const u32		*fw_data;
	int			i, j, ret = 0, pe;
	u32			fw_size, reg;
	const char		*fw_file_name[FW_NB] = {"ifpp.bin",
							    "ipue.bin"};
	char			fw_base[13] = {0};	/* "eip197/197X/\0" */
	char			fw_full_name[25] = {0};

	snprintf(fw_base, 13, "eip197/197%s/",
		 (priv->eip197_hw_ver == EIP197B) ? "b" : "d");

	for (i = 0; i < FW_NB; i++) {
		snprintf(fw_full_name, 21, "%s%s", fw_base, fw_file_name[i]);
		ret = request_firmware(&fw[i], fw_full_name, priv->dev);
		if (ret) {
			dev_err(priv->dev, "request_firmware failed (fw: %s)\n",
				fw_full_name);
			goto release_fw;
		}
	 }

	for (pe = 0; pe < priv->nr_pe; pe++) {
		/* Clear EIP-207c ICE Scratchpad RAM where the firmware */
		reg = (EIP197_PE_ICE_SCRATCH_CTRL_DFLT				|
		       EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_TIMER			|
		       EIP197_PE_ICE_SCRATCH_CTRL_TIMER_EN			|
		       ((0x1 << EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_OFFSET) &
			EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_MASK)		|
		       EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_ACCESS);
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_SCRATCH_CTRL_OFFSET(pe));

		/* Write the ICE Scratchpad RAM with 0 */
		for (i = 0; i < EIP197_NUM_OF_SCRATCH_BLOCKS; i++)
			writel(0, EIP197_PE(priv) + EIP197_PE_ICE_SCRATCH_RAM(i, pe));

		/* Reset the Input Flow Post-Processor micro-engine (IFPP) to make its
		 * program RAM accessible.
		 */
		reg = (EIP197_PE_ICE_FPP_CTRL_SW_RESET			|
		       EIP197_PE_ICE_FPP_CTRL_CLR_ECC_CORR		|
		       EIP197_PE_ICE_FPP_CTRL_CLR_ECC_NON_CORR);
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_FPP_CTRL(pe));

		/* Enable access to IFPP Program RAM */
		reg = (EIP197_PE_ICE_RAM_CTRL_DFLT |
		       EIP197_PE_ICE_RAM_CTRL_FPP_PROG_EN);
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL(pe));
	}

	/* Save pointer to the data and the size of the data */
	fw_data = (const u32 *)fw[FW_IFPP]->data;
	fw_size = fw[FW_IFPP]->size / sizeof(u32);

	/* Write the Input Flow post-Processor micro-Engine firmware */
	for (i = 0; i < fw_size; i++)
		writel(be32_to_cpu(*(fw_data + i)),
		       priv->base + EIP197_CLASSIFICATION_RAMS + (i * 4));

	for (pe = 0; pe < priv->nr_pe; pe++) {
		/* Disable access to IFPP Program RAM
		 * Enable access to IPUE Program RAM
		 */
		reg = (EIP197_PE_ICE_RAM_CTRL_DFLT |
		       EIP197_PE_ICE_RAM_CTRL_PUE_PROG_EN);
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL(pe));

		/* Reset the Input Pull-Up micro-Engine (IPUE) to make its
		 * program RAM accessible.
		 */
		reg = (EIP197_PE_ICE_PUE_CTRL_SW_RESET		|
		       EIP197_PE_ICE_PUE_CTRL_CLR_ECC_CORR	|
		       EIP197_PE_ICE_PUE_CTRL_CLR_ECC_NON_CORR);
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_PUE_CTRL(pe));
	}

	/* Save pointer to the data and the size of the data */
	fw_data = (const u32 *)fw[FW_IPUE]->data;
	fw_size = fw[FW_IPUE]->size / sizeof(u32);

	/* Write the Input Flow post-Processor micro-Engine firmware */
	for (i = 0; i < fw_size; i++)
		writel(be32_to_cpu(*(fw_data + i)),
		       priv->base + EIP197_CLASSIFICATION_RAMS + (i * sizeof(u32)));

	for (pe = 0; pe < priv->nr_pe; pe++) {
		/* Disable access to IPUE Program RAM */
		reg = EIP197_PE_ICE_RAM_CTRL_DFLT;
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_RAM_CTRL(pe));

		/* Release IFPP from reset */
		reg = readl(priv->base + EIP197_PE_ICE_PUE_CTRL(pe));
		reg &= ~EIP197_PE_ICE_FPP_CTRL_SW_RESET;
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_FPP_CTRL(pe));

		/* Release IPUE from reset */
		reg = readl(priv->base + EIP197_PE_ICE_PUE_CTRL(pe));
		reg &= ~EIP197_PE_ICE_PUE_CTRL_SW_RESET;
		writel(reg, EIP197_PE(priv) + EIP197_PE_ICE_PUE_CTRL(pe));
	}

release_fw:
	for (j = 0; j < i; j++)
		release_firmware(fw[j]);

	return ret;
}

/* Reset the command descriptor rings */
static void safexcel_hw_reset_cdesc_rings(struct safexcel_crypto_priv *priv)
{
	int i;

	for (i = 0; i < priv->config.rings; i++) {
		/* Reset ring base address */
		writel(0x0,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_LO);
		writel(0x0,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_HI);

		/* clear any pending interrupt */
		writel(EIP197_CDR_INTR_MASK,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_STAT);
	}
}

/* Reset the result descriptor rings */
static void safexcel_hw_reset_rdesc_rings(struct safexcel_crypto_priv *priv)
{
	int i;

	for (i = 0; i < priv->config.rings; i++) {
		/* Reset ring base address */
		writel(0x0,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_LO);
		writel(0x0,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_HI);

		/* clear any pending interrupt */
		writel(EIP197_RDR_INTR_MASK,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_STAT);
	}
}

/* Configure the command descriptor ring manager */
static int safexcel_hw_setup_cdesc_rings(struct safexcel_crypto_priv *priv)
{
	u32 hdw, cd_size_rnd, val;
	int i;

	hdw = readl(EIP197_HIA_AIC_G(priv) + EIP197_HIA_OPTIONS);
	hdw = (hdw & EIP197_xDR_HDW_MASK) >> EIP197_xDR_HDW_OFFSET;

	cd_size_rnd = (priv->config.cd_size + (BIT(hdw) - 1)) >> hdw;

	for (i = 0; i < priv->config.rings; i++) {
		/* ring base address */
		writel(lower_32_bits(priv->ring[i].cdr.base_dma),
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_LO);
		writel(upper_32_bits(priv->ring[i].cdr.base_dma),
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_HI);

		writel(EIP197_xDR_DESC_MODE_64BIT |
		       (priv->config.cd_offset << EIP197_xDR_DESC_CD_OFFSET) |
		       priv->config.cd_size,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_DESC_SIZE);
		writel(((EIP197_FETCH_COUNT * (cd_size_rnd << hdw)) << EIP197_XDR_CD_FETCH_THRESH) |
		       (EIP197_FETCH_COUNT * priv->config.cd_offset),
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_CFG);

		/* Configure DMA tx control */
		val = EIP197_HIA_xDR_CFG_WR_CACHE(WR_CACHE_3BITS);
		val |= EIP197_HIA_xDR_CFG_RD_CACHE(RD_CACHE_3BITS);

		if (priv->eip_type == EIP197 &&
		    priv->eip197_hw_ver == EIP197D) {
			val |= EIP197_HIA_xDR_CFG_xD_PROT(AXI_NONE_SECURE_ACCESS);
			val |= EIP197_HIA_xDR_CFG_DATA_PROT(AXI_NONE_SECURE_ACCESS);
			val |= EIP197_HIA_xDR_CFG_ACD_PROT(AXI_NONE_SECURE_ACCESS);
		}
		writel(val, EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_DMA_CFG);

		/* clear any pending interrupt */
		writel(EIP197_CDR_INTR_MASK,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_STAT);
	}

	return 0;
}

/* Configure the result descriptor ring manager */
static int safexcel_hw_setup_rdesc_rings(struct safexcel_crypto_priv *priv)
{
	u32 hdw, rd_size_rnd, val;
	int i;

	hdw = readl(EIP197_HIA_AIC_G(priv) + EIP197_HIA_OPTIONS);
	hdw = (hdw & EIP197_xDR_HDW_MASK) >> EIP197_xDR_HDW_OFFSET;

	rd_size_rnd = (priv->config.rd_size + (BIT(hdw) - 1)) >> hdw;

	for (i = 0; i < priv->config.rings; i++) {
		/* ring base address */
		writel(lower_32_bits(priv->ring[i].rdr.base_dma),
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_LO);
		writel(upper_32_bits(priv->ring[i].rdr.base_dma),
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_RING_BASE_ADDR_HI);

		writel(EIP197_xDR_DESC_MODE_64BIT |
		       priv->config.rd_offset << EIP197_xDR_DESC_CD_OFFSET |
		       priv->config.rd_size,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_DESC_SIZE);

		writel((EIP197_FETCH_COUNT * (rd_size_rnd << hdw)) << EIP197_XDR_CD_FETCH_THRESH |
		       (EIP197_FETCH_COUNT * priv->config.rd_offset),
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_CFG);

		/* Configure DMA tx control */
		val = EIP197_HIA_xDR_CFG_WR_CACHE(WR_CACHE_3BITS);
		val |= EIP197_HIA_xDR_CFG_RD_CACHE(RD_CACHE_3BITS);
		val |= EIP197_HIA_xDR_WR_RES_BUF | EIP197_HIA_xDR_WR_CTRL_BUF;

		if (priv->eip_type == EIP197 &&
		    priv->eip197_hw_ver == EIP197D) {
			val |= EIP197_HIA_xDR_CFG_xD_PROT(AXI_NONE_SECURE_ACCESS);
			val |= EIP197_HIA_xDR_CFG_DATA_PROT(AXI_NONE_SECURE_ACCESS);
		}
		writel(val, EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_DMA_CFG);

		/* clear any pending interrupt */
		writel(EIP197_RDR_INTR_MASK,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_STAT);

		/* enable ring interrupt */
		val = readl(EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ENABLE_CTRL(i));
		val |= EIP197_RDR_IRQ(i);
		writel(val, EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ENABLE_CTRL(i));
	}

	return 0;
}

static int safexcel_hw_init(struct safexcel_crypto_priv *priv)
{
	u32 version, val;
	int i, ret, pe;

	/* Determine endianness and configure byte swap */
	version = readl(EIP197_HIA_AIC(priv) + EIP197_HIA_VERSION);
	val = readl(EIP197_HIA_AIC(priv) + EIP197_HIA_MST_CTRL);

	if ((version & 0xffff) == EIP197_HIA_VERSION_BE)
		val |= EIP197_MST_CTRL_BYTE_SWAP;
	else if (((version >> 16) & 0xffff) == EIP197_HIA_VERSION_LE)
		val |= (EIP197_MST_CTRL_NO_BYTE_SWAP >> 24);

	writel(val, EIP197_HIA_AIC(priv) + EIP197_HIA_MST_CTRL);

	/* configure wr/rd cache values */
	val = EIP197_MST_CTRL_RD_CACHE(RD_CACHE_4BITS) |
		EIP197_MST_CTRL_WD_CACHE(WR_CACHE_4BITS);

	if (priv->eip_type == EIP197 &&
	    priv->eip197_hw_ver == EIP197D)
		val |= MST_CTRL_SUPPORT_PROT(AXI_NONE_SECURE_ACCESS);
	writel(val, EIP197_HIA_GEN_CFG(priv) + EIP197_MST_CTRL);

	/* Interrupts reset */

	/* Disable all global interrupts */
	writel(0, EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ENABLE_CTRL);

	/* Clear any pending interrupt */
	writel(EIP197_AIC_G_ACK_ALL_MASK, EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ACK);

	/* Processing Engine configuration */
	for (pe = 0; pe < priv->nr_pe; pe++) {
		/* Data Fetch Engine configuration */

		/* Reset all DFE threads */
		writel(EIP197_DxE_THR_CTRL_RESET_PE,
		       EIP197_HIA_DFE_THR(priv) + EIP197_HIA_DFE_THR_CTRL(pe));

		/* Configure ring arbiter, available only for EIP197 */
		if (priv->eip_type == EIP197) {
			/* Reset HIA input interface arbiter */
			writel(EIP197_HIA_RA_PE_CTRL_RESET,
			       EIP197_HIA_AIC(priv) + EIP197_HIA_RA_PE_CTRL(pe));
		}

		/* DMA transfer size to use */
		val = EIP197_HIA_DFE_CFG_DIS_DEBUG;
		val |= EIP197_HIA_DxE_CFG_MIN_DATA_SIZE(5) | EIP197_HIA_DxE_CFG_MAX_DATA_SIZE(9);
		val |= EIP197_HIA_DxE_CFG_MIN_CTRL_SIZE(5) | EIP197_HIA_DxE_CFG_MAX_CTRL_SIZE(7);
		val |= EIP197_HIA_DxE_CFG_DATA_CACHE_CTRL(RD_CACHE_3BITS);
		val |= EIP197_HIA_DxE_CFG_CTRL_CACHE_CTRL(RD_CACHE_3BITS);
		writel(val, EIP197_HIA_DFE(priv) + EIP197_HIA_DFE_CFG(pe));

		/* Leave the DFE threads reset state */
		writel(0, EIP197_HIA_DFE_THR(priv) + EIP197_HIA_DFE_THR_CTRL(pe));

		/* Configure the procesing engine thresholds */
		writel(EIP197_PE_IN_xBUF_THRES_MIN(5) | EIP197_PE_IN_xBUF_THRES_MAX(9),
		      EIP197_PE(priv) + EIP197_PE_IN_DBUF_THRES(pe));
		writel(EIP197_PE_IN_xBUF_THRES_MIN(5) | EIP197_PE_IN_xBUF_THRES_MAX(7),
		      EIP197_PE(priv) + EIP197_PE_IN_TBUF_THRES(pe));

		/* Configure ring arbiter, available only for EIP197 */
		if (priv->eip_type == EIP197) {
			/* enable HIA input interface arbiter and rings */
			writel(EIP197_HIA_RA_PE_CTRL_EN | GENMASK(priv->config.hw_rings - 1, 0),
			       EIP197_HIA_AIC(priv) + EIP197_HIA_RA_PE_CTRL(pe));
		}

		/* Data Store Engine configuration */

		/* Reset all DSE threads */
		writel(EIP197_DxE_THR_CTRL_RESET_PE,
		       EIP197_HIA_DSE_THR(priv) + EIP197_HIA_DSE_THR_CTRL(pe));

		/* Wait for all DSE threads to complete */
		while ((readl(EIP197_HIA_DSE_THR(priv) + EIP197_HIA_DSE_THR_STAT(pe)) &
		       EIP197_DSE_THR_RDR_ID_MASK) != EIP197_DSE_THR_RDR_ID_MASK)
			;

		/* DMA transfer size to use */
		val = EIP197_HIA_DSE_CFG_DIS_DEBUG;
		val |= EIP197_HIA_DxE_CFG_MIN_DATA_SIZE(7) | EIP197_HIA_DxE_CFG_MAX_DATA_SIZE(8);
		val |= EIP197_HIA_DxE_CFG_DATA_CACHE_CTRL(WR_CACHE_3BITS);
		val |= EIP197_HIA_DSE_CFG_ALLWAYS_BUFFERABLE;
		/*
		 * TODO: Generally, EN_SINGLE_WR should be enabled.
		 * Some instabilities with this option enabled might occur,
		 * so further investigation is required before enabling it.
		 *
		 * Using EIP97 engine without SINGLE_WR impacts the performance.
		 */
		if (priv->eip_type == EIP197)
			val |= EIP197_HIA_DSE_CFG_EN_SINGLE_WR;
		writel(val, EIP197_HIA_DSE(priv) + EIP197_HIA_DSE_CFG(pe));

		/* Leave the DSE threads reset state */
		writel(0, EIP197_HIA_DSE_THR(priv) + EIP197_HIA_DSE_THR_CTRL(pe));

		/* Configure the procesing engine thresholds */
		writel(EIP197_PE_OUT_DBUF_THRES_MIN(7) | EIP197_PE_OUT_DBUF_THRES_MAX(8),
		       EIP197_PE(priv) + EIP197_PE_OUT_DBUF_THRES(pe));
	}

	/* Command Descriptor Rings prepare */
	for (i = 0; i < priv->config.hw_rings; i++) {
		/* Clear interrupts for this ring */
		writel(EIP197_HIA_AIC_R_ENABLE_CLR_ALL_MASK,
		       EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ENABLE_CLR(i));

		/* Disable external triggering */
		writel(0, EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_CFG);

		/* Clear the pending prepared counter */
		writel(EIP197_xDR_PREP_CLR_COUNT,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_PREP_COUNT);

		/* Clear the pending processed counter */
		writel(EIP197_xDR_PROC_CLR_COUNT,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_PROC_COUNT);

		writel(0,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_PREP_PNTR);
		writel(0,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_PROC_PNTR);

		writel((EIP197_DEFAULT_RING_SIZE * priv->config.cd_offset) << 2,
		       EIP197_HIA_CDR(priv, i) + EIP197_HIA_xDR_RING_SIZE);
	}

	/* Result Descriptor Ring prepare */
	for (i = 0; i < priv->config.hw_rings; i++) {
		/* Disable external triggering*/
		writel(0, EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_CFG);

		/* Clear the pending prepared counter */
		writel(EIP197_xDR_PREP_CLR_COUNT,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_PREP_COUNT);

		/* Clear the pending processed counter */
		writel(EIP197_xDR_PROC_CLR_COUNT,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_PROC_COUNT);

		writel(0,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_PREP_PNTR);
		writel(0,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_PROC_PNTR);

		/* Ring size */
		writel((EIP197_DEFAULT_RING_SIZE * priv->config.rd_offset) << 2,
		       EIP197_HIA_RDR(priv, i) + EIP197_HIA_xDR_RING_SIZE);
	}

	for (pe = 0; pe < priv->nr_pe; pe++) {
		/* Enable command descriptor rings */
		writel(EIP197_DxE_THR_CTRL_EN | GENMASK(priv->config.hw_rings - 1, 0),
		       EIP197_HIA_DFE_THR(priv) + EIP197_HIA_DFE_THR_CTRL(pe));

		/* Enable result descriptor rings */
		writel(EIP197_DxE_THR_CTRL_EN | GENMASK(priv->config.hw_rings - 1, 0),
		       EIP197_HIA_DSE_THR(priv) + EIP197_HIA_DSE_THR_CTRL(pe));
	}

	/* Clear any HIA interrupt */
	writel(EIP197_AIC_G_ACK_HIA_MASK, EIP197_HIA_AIC_G(priv) + EIP197_HIA_AIC_G_ACK);

	/*
	 * Initialize EIP197 specifics:
	 *	- PRNG
	 *	- Cache
	 *	- Firmware
	 */
	if (priv->eip_type == EIP197) {
		/* init PRNG */
		for (pe = 0; pe < priv->nr_pe; pe++)
			eip197_prng_init(priv, pe);

		/* init transform record cache */
		ret = eip197_trc_cache_init(priv);
		if (ret) {
			dev_err(priv->dev, "eip197_trc_cache_init failed\n");
			return ret;
		}

		/* Firmware load */
		ret = eip197_load_fw(priv);
		if (ret) {
			dev_err(priv->dev, "eip197_load_fw failed\n");
			return ret;
		}
	}

	safexcel_hw_setup_cdesc_rings(priv);
	safexcel_hw_setup_rdesc_rings(priv);

	return 0;
}

/* Dequeue crypto API requests and send to the engine */
void safexcel_dequeue(struct safexcel_crypto_priv *priv, int ring)
{
	struct crypto_async_request *req, *backlog;
	struct safexcel_context *ctx;
	struct safexcel_request *request;
	int ret, nreq = 0;
	int cdesc = 0, rdesc = 0;
	int commands, results;
	u32 val;

	req = priv->ring[ring].req;
	backlog = priv->ring[ring].backlog;

	if (req)
		goto handle_pending;

	do {
		/* get a new request if no ring saved req */
		spin_lock_bh(&priv->ring[ring].queue_lock);
		backlog = crypto_get_backlog(&priv->ring[ring].queue);
		req = crypto_dequeue_request(&priv->ring[ring].queue);
		spin_unlock_bh(&priv->ring[ring].queue_lock);

		/* no more requests, update ring saved req */
		if (!req) {
			/* no more requests, clear */
			priv->ring[ring].req = NULL;
			priv->ring[ring].backlog = NULL;
			goto finalize;
		}

handle_pending:
		request = kzalloc(sizeof(*request), EIP197_GFP_FLAGS(*req));
		if (!request)
			goto resource_fail;

		ctx = crypto_tfm_ctx(req->tfm);
		ret = ctx->send(req, ring, request, &commands, &results);
		if (ret) {
			kfree(request);
			goto resource_fail;
		}

		if (backlog)
			backlog->complete(backlog, -EINPROGRESS);

		backlog = NULL;
		req = NULL;

		cdesc += commands;
		rdesc += results;
		nreq++;
	} while (true);

resource_fail:
	/*
	 * resource alloc fail, bail out, save the request and backlog
	 * for later dequeue handling
	 */
	priv->ring[ring].req = req;
	priv->ring[ring].backlog = backlog;

finalize:
	spin_lock_bh(&priv->ring[ring].egress_lock);

	if (!priv->ring[ring].busy && priv->ring[ring].egress_cnt) {
		/* Configure when we want an interrupt */
		priv->ring[ring].busy = 1;

		val = EIP197_HIA_RDR_THRESH_PKT_MODE |
			EIP197_HIA_RDR_THRESH_PROC_PKT(min_t(int, priv->ring[ring].egress_cnt,
							   EIP197_MAX_BATCH_SZ));
		writel(val, EIP197_HIA_RDR(priv, ring) + EIP197_HIA_xDR_THRESH);
	}

	spin_unlock_bh(&priv->ring[ring].egress_lock);

	if (nreq) {
		/* let the RDR know we have pending descriptors */
		writel_relaxed((rdesc * priv->config.rd_offset) << EIP197_xDR_PREP_xD_COUNT_INCR_OFFSET,
			       EIP197_HIA_RDR(priv, ring) + EIP197_HIA_xDR_PREP_COUNT);

		/* let the CDR know we have pending descriptors */
		writel((cdesc * priv->config.cd_offset) << EIP197_xDR_PREP_xD_COUNT_INCR_OFFSET,
			EIP197_HIA_CDR(priv, ring) + EIP197_HIA_xDR_PREP_COUNT);
	}
}

/* Select the ring which will be used for the operation */
inline int safexcel_select_ring(struct safexcel_crypto_priv *priv)
{
	return (atomic_inc_return(&priv->ring_used) % priv->config.rings);
}

/* Acknoledge and release the used descriptors */
void safexcel_complete(struct safexcel_crypto_priv *priv, int ring)
{
	struct safexcel_command_desc *cdesc;

	/* Acknowledge the command descriptors */
	do {
		cdesc = safexcel_ring_next_rptr(priv, &priv->ring[ring].cdr);
		if (IS_ERR(cdesc)) {
			dev_err(priv->dev,
				"Could not retrieve the command descriptor\n");
			return;
		}
	} while (!cdesc->last_seg);
}

/* Context completion cache invalidation */
void safexcel_inv_complete(struct crypto_async_request *req, int error)
{
	struct safexcel_inv_result *result = req->data;

	if (error == -EINPROGRESS)
		return;

	result->error = error;
	complete(&result->completion);
}

/* Context cache invalidation */
int safexcel_invalidate_cache(struct crypto_async_request *async,
			      struct safexcel_crypto_priv *priv,
			      dma_addr_t ctxr_dma, int ring,
			      struct safexcel_request *request)
{
	struct safexcel_command_desc *cdesc;
	struct safexcel_result_desc *rdesc;
	int ret;

	spin_lock_bh(&priv->ring[ring].egress_lock);

	/* Prepare command descriptor */
	cdesc = safexcel_add_cdesc(priv, ring, true, true, 0, 0, 0, ctxr_dma);
	if (IS_ERR(cdesc)) {
		ret = PTR_ERR(cdesc);
		goto unlock;
	}

	cdesc->control_data.type = EIP197_TYPE_EXTENDED;
	cdesc->control_data.options = 0;
	cdesc->control_data.refresh = 0;
	cdesc->control_data.control0 = CONTEXT_CONTROL_INV_TR;

	/* Prepare result descriptor */
	rdesc = safexcel_add_rdesc(priv, ring, true, true, 0, 0);

	if (IS_ERR(rdesc)) {
		ret = PTR_ERR(rdesc);
		goto cdesc_rollback;
	}

	request->req = async;
	list_add_tail(&request->list, &priv->ring[ring].list);

	/* update the ring request count */
	priv->ring[ring].egress_cnt++;

	spin_unlock_bh(&priv->ring[ring].egress_lock);

	return 0;

cdesc_rollback:
	safexcel_ring_rollback_wptr(priv, &priv->ring[ring].cdr);

unlock:
	spin_unlock_bh(&priv->ring[ring].egress_lock);
	return ret;
}

/* Generic result handler */
static void safexcel_handle_result_descriptor(struct safexcel_crypto_priv *priv,
					      int ring)
{
	struct safexcel_request *sreq;
	struct safexcel_context *ctx;
	int ret, i, ndesc, ndesc_tot, nreq_cnt;
	u32 val, results;
	bool should_complete;
	int egress_cnt;

more_results:
	results = readl(EIP197_HIA_RDR(priv, ring) + EIP197_HIA_xDR_PROC_COUNT);
	results = (results >> EIP197_xDR_PROC_xD_PKT_OFFSET) & EIP197_xDR_PROC_xD_PKT_MASK;

	nreq_cnt = 0;
	ndesc_tot = 0;

	for (i = 0; i < results; i++) {
		spin_lock_bh(&priv->ring[ring].egress_lock);
		sreq = list_first_entry(&priv->ring[ring].list, struct safexcel_request, list);
		list_del(&sreq->list);
		priv->ring[ring].egress_cnt--;
		spin_unlock_bh(&priv->ring[ring].egress_lock);

		ctx = crypto_tfm_ctx(sreq->req->tfm);
		ndesc = ctx->handle_result(priv, ring, sreq->req,
					   &should_complete, &ret);
		if (ndesc < 0) {
			kfree(sreq);
			dev_err(priv->dev, "failed to handle result (%d)", ndesc);
			return;
		}

		nreq_cnt++;
		ndesc_tot += ndesc;

		if (should_complete) {
			local_bh_disable();
			sreq->req->complete(sreq->req, ret);
			local_bh_enable();
		}

		kfree(sreq);
	}

	if (nreq_cnt) {
		/* decrement the handled results */
		val = EIP197_xDR_PROC_xD_PKT(nreq_cnt) |
			EIP197_xDR_PROC_xD_COUNT(ndesc_tot * priv->config.rd_offset);

		writel(val, EIP197_HIA_RDR(priv, ring) +
		       EIP197_HIA_xDR_PROC_COUNT);
	}

	/* more results ready in the ring? */
	if (results == EIP197_xDR_PROC_xD_PKT_MASK)
		goto more_results;

	spin_lock_bh(&priv->ring[ring].egress_lock);

	/* get the pending request count */
	egress_cnt = min_t(int, priv->ring[ring].egress_cnt, EIP197_MAX_BATCH_SZ);

	if (!egress_cnt) {
		/* no more request in ring */
		priv->ring[ring].busy = 0;
		spin_unlock_bh(&priv->ring[ring].egress_lock);

		return;
	}

	/* Configure when we want an interrupt */
	val = EIP197_HIA_RDR_THRESH_PKT_MODE |
		EIP197_HIA_RDR_THRESH_PROC_PKT(egress_cnt);

	writel(val, EIP197_HIA_RDR(priv, ring) +
	       EIP197_HIA_xDR_THRESH);

	spin_unlock_bh(&priv->ring[ring].egress_lock);
}

/* dequeue from Crypto API FIFO and insert requests into HW ring */
static void safexcel_dequeue_work(struct work_struct *work)
{
	struct safexcel_work_data *data =
			container_of(work, struct safexcel_work_data, work);

	safexcel_dequeue(data->priv, data->ring);
}

struct safexcel_ring_irq_data {
	struct safexcel_crypto_priv *priv;
	int ring;
};

static irqreturn_t safexcel_irq_ring(int irq, void *data)
{
	struct safexcel_ring_irq_data *irq_data = data;
	struct safexcel_crypto_priv *priv = irq_data->priv;
	int ring = irq_data->ring, rc = IRQ_NONE;
	u32 status, stat;

	status = readl(EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ENABLED_STAT(ring));
	if (!status)
		return rc;

	/* RDR interrupts */
	if (status & EIP197_RDR_IRQ(ring)) {
		stat = readl(EIP197_HIA_RDR(priv, ring) + EIP197_HIA_xDR_STAT);

		if (unlikely(stat & EIP197_xDR_ERR)) {
			/*
			 * Fatal error, the RDR is unusable and must be
			 * reinitialized. This should not happen under
			 * normal circumstances.
			 */
			dev_err(priv->dev, "RDR: fatal error.");
		} else if (likely(stat & EIP197_xDR_THRESH)) {
			rc = IRQ_WAKE_THREAD;
		}

		/* ACK the interrupts */
		writel(stat & 0xff,
		       EIP197_HIA_RDR(priv, ring) + EIP197_HIA_xDR_STAT);
	}

	/* ACK the interrupts */
	writel(status, EIP197_HIA_AIC_R(priv) + EIP197_HIA_AIC_R_ACK(ring));

	return rc;
}

static irqreturn_t safexcel_irq_ring_thread(int irq, void *data)
{
	struct safexcel_ring_irq_data *irq_data = data;
	struct safexcel_crypto_priv *priv = irq_data->priv;
	int ring = irq_data->ring;

	safexcel_handle_result_descriptor(priv, ring);

	queue_work(priv->ring[ring].workqueue,
		   &priv->ring[ring].work_data.work);

	return IRQ_HANDLED;
}

/* Register ring interrupts */
static int safexcel_request_ring_irq(struct platform_device *pdev, const char *name,
				     irq_handler_t handler,
				     irq_handler_t threaded_handler,
				     struct safexcel_ring_irq_data *ring_irq_priv)
{
	int ret, irq = platform_get_irq_byname(pdev, name);

	if (irq < 0) {
		dev_err(&pdev->dev, "unable to get IRQ '%s'\n", name);
		return irq;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq, handler,
					threaded_handler, IRQF_ONESHOT,
					dev_name(&pdev->dev), ring_irq_priv);
	if (ret) {
		dev_err(&pdev->dev, "unable to request IRQ %d\n", irq);
		return ret;
	}

	return irq;
}

/* List of supported algorithms */
static struct safexcel_alg_template *safexcel_algs[] = {
	&safexcel_alg_ecb_aes,
	&safexcel_alg_cbc_aes,
	&safexcel_alg_cbc_des,
	&safexcel_alg_cbc_des3_ede,
	&safexcel_alg_ecb_des,
	&safexcel_alg_ecb_des3_ede,
	&safexcel_alg_sha1,
	&safexcel_alg_sha224,
	&safexcel_alg_sha256,
	&safexcel_alg_hmac_sha1,
	&safexcel_alg_hmac_sha256,
	&safexcel_alg_md5,
	&safexcel_alg_hmac_md5,
};

/* Register the supported hash and cipher algorithms */
static int safexcel_register_algorithms(struct safexcel_crypto_priv *priv)
{
	int i, j, ret = 0;

	for (i = 0; i < ARRAY_SIZE(safexcel_algs); i++) {
		safexcel_algs[i]->priv = priv;

		if (safexcel_algs[i]->type == SAFEXCEL_ALG_TYPE_CIPHER)
			ret = crypto_register_alg(&safexcel_algs[i]->alg.crypto);
		else
			ret = crypto_register_ahash(&safexcel_algs[i]->alg.ahash);

		if (ret)
			goto fail;
	}

	return 0;

fail:
	for (j = i; j < 0; j--) {
		if (safexcel_algs[j]->type == SAFEXCEL_ALG_TYPE_CIPHER)
			crypto_unregister_alg(&safexcel_algs[j]->alg.crypto);
		else
			crypto_unregister_ahash(&safexcel_algs[j]->alg.ahash);
	}

	return ret;
}

/* Unregister the hash and cipher algorithms */
static void safexcel_unregister_algorithms(struct safexcel_crypto_priv *priv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(safexcel_algs); i++) {
		if (safexcel_algs[i]->type == SAFEXCEL_ALG_TYPE_CIPHER)
			crypto_unregister_alg(&safexcel_algs[i]->alg.crypto);
		else
			crypto_unregister_ahash(&safexcel_algs[i]->alg.ahash);
	}
}

/* Configure rings basic parameters */
static void safexcel_configure(struct safexcel_crypto_priv *priv)
{
	u32 val, mask;

	val = readl(EIP197_HIA_AIC_G(priv) + EIP197_HIA_OPTIONS);

	/* Read number of PEs from the engine & set eip197 HW version */
	mask = (priv->eip_type == EIP97) ? EIP97_N_PES_MASK : EIP197_N_PES_MASK;
	priv->nr_pe = (val >> EIP197_N_PES_OFFSET) & mask;
	if (priv->eip_type == EIP197) {
		if (priv->nr_pe == 1)
			priv->eip197_hw_ver = EIP197B;
		else
			priv->eip197_hw_ver = EIP197D;
	}

	/* Read number of rings from the engine */
	priv->config.hw_rings = val & EIP197_N_RINGS_MASK;

	val = (val & EIP197_xDR_HDW_MASK) >> EIP197_xDR_HDW_OFFSET;
	mask = BIT(val) - 1;

	/* Check the requested number of rings given in the module param.
	 * If the module param is uninitialized, use all available rings
	 */
	if (rings[priv->id] == RINGS_UNINITIALIZED)
		rings[priv->id] = priv->config.hw_rings;

	/* Check if the number of requested rings in module param is valid */
	if (rings[priv->id] > priv->config.hw_rings) {
		/* Invalid, use all available rings */
		priv->config.rings = priv->config.hw_rings;
		dev_warn(priv->dev, "requested %d rings, given only %d rings\n",
			 rings[priv->id], priv->config.hw_rings);
	} else  {
		priv->config.rings = rings[priv->id];
	}

	priv->config.cd_size = (sizeof(struct safexcel_command_desc) / sizeof(u32));
	priv->config.cd_offset = (priv->config.cd_size + mask) & ~mask;

	priv->config.rd_size = (sizeof(struct safexcel_result_desc) / sizeof(u32));
	priv->config.rd_offset = (priv->config.rd_size + mask) & ~mask;
}

static void safexcel_init_register_offsets(struct safexcel_crypto_priv *priv)
{
	struct safexcel_register_offsets *offsets = &priv->offsets;

	if (priv->eip_type == EIP197) {
		offsets->hia_aic	= EIP197_HIA_AIC_BASE;
		offsets->hia_aic_g	= EIP197_HIA_AIC_G_BASE;
		offsets->hia_aic_r	= EIP197_HIA_AIC_R_BASE;
		offsets->hia_aic_xdr	= EIP197_HIA_AIC_xDR_BASE;
		offsets->hia_dfe	= EIP197_HIA_DFE_BASE;
		offsets->hia_dfe_thr	= EIP197_HIA_DFE_THR_BASE;
		offsets->hia_dse	= EIP197_HIA_DSE_BASE;
		offsets->hia_dse_thr	= EIP197_HIA_DSE_THR_BASE;
		offsets->hia_gen_cfg	= EIP197_HIA_GEN_CFG_BASE;
		offsets->pe		= EIP197_PE_BASE;
	} else {
		offsets->hia_aic	= EIP97_HIA_AIC_BASE;
		offsets->hia_aic_g	= EIP97_HIA_AIC_G_BASE;
		offsets->hia_aic_r	= EIP97_HIA_AIC_R_BASE;
		offsets->hia_aic_xdr	= EIP97_HIA_AIC_xDR_BASE;
		offsets->hia_dfe	= EIP97_HIA_DFE_BASE;
		offsets->hia_dfe_thr	= EIP97_HIA_DFE_THR_BASE;
		offsets->hia_dse	= EIP97_HIA_DSE_BASE;
		offsets->hia_dse_thr	= EIP97_HIA_DSE_THR_BASE;
		offsets->hia_gen_cfg	= EIP97_HIA_GEN_CFG_BASE;
		offsets->pe		= EIP97_PE_BASE;
	}
}

static int safexcel_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	struct safexcel_crypto_priv *priv;
	int i, ret;
	u32 dma_bus_width;

	priv = devm_kzalloc(dev, sizeof(struct safexcel_crypto_priv),
			    GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	priv->eip_type = (enum safexcel_eip_type)of_device_get_match_data(dev);

	safexcel_init_register_offsets(priv);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(priv->base)) {
		dev_err(dev, "failed to get resource\n");
		return PTR_ERR(priv->base);
	}

	ret = of_property_read_u32(dev->of_node, "cell-index", &priv->id);
	if (ret) {
		dev_err(dev, "failed to read cell-index property\n");
		return ret;
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

	ret = of_property_read_u32(dev->of_node, "dma-bus-width", &dma_bus_width);
	if (ret) {
		dev_err(dev, "Failed to read dma-bus-width property\n");
		goto err_clk;
	}

	ret = dma_set_mask_and_coherent(dev,
					DMA_BIT_MASK(dma_bus_width));
	if (ret)
		goto err_clk;

	priv->context_pool = dmam_pool_create("safexcel-context", dev,
					      sizeof(struct safexcel_context_record),
					      1, 0);
	if (!priv->context_pool) {
		ret = -ENOMEM;
		goto err_clk;
	}

	safexcel_configure(priv);

	priv->ring = devm_kzalloc(dev,
				  (priv->config.rings * sizeof(*priv->ring)),
				  GFP_KERNEL);
	if (!priv->ring)
		return -ENOMEM;

	for (i = 0; i < priv->config.rings; i++) {
		char irq_name[6] = {0}; /* "ringX\0" */
		char wq_name[9] = {0}; /* "wq_ringX\0" */
		int irq;
		struct safexcel_ring_irq_data *ring_irq;

		ret = safexcel_init_ring_descriptors(priv,
						     &priv->ring[i].cdr,
						     &priv->ring[i].rdr);
		if (ret)
			goto err_clk;

		ring_irq = devm_kzalloc(dev, sizeof(*ring_irq), GFP_KERNEL);
		if (!ring_irq) {
			ret = -ENOMEM;
			goto err_clk;
		}

		ring_irq->priv = priv;
		ring_irq->ring = i;

		snprintf(irq_name, 6, "ring%d", i);
		irq = safexcel_request_ring_irq(pdev, irq_name, safexcel_irq_ring,
						safexcel_irq_ring_thread,
						ring_irq);
		if (irq < 0) {
			ret = irq;
			goto err_clk;
		}

		priv->ring[i].work_data.priv = priv;
		priv->ring[i].work_data.ring = i;
		INIT_WORK(&priv->ring[i].work_data.work, safexcel_dequeue_work);

		snprintf(wq_name, 9, "wq_ring%d", i);
		priv->ring[i].workqueue = create_singlethread_workqueue(wq_name);
		if (!priv->ring[i].workqueue) {
			ret = -ENOMEM;
			goto err_clk;
		}

		priv->ring[i].egress_cnt = 0;
		priv->ring[i].busy = 0;

		crypto_init_queue(&priv->ring[i].queue,
				  EIP197_DEFAULT_RING_SIZE);

		INIT_LIST_HEAD(&priv->ring[i].list);
		spin_lock_init(&priv->ring[i].lock);
		spin_lock_init(&priv->ring[i].egress_lock);
		spin_lock_init(&priv->ring[i].queue_lock);
	}

	platform_set_drvdata(pdev, priv);
	atomic_set(&priv->ring_used, 0);

	ret = safexcel_hw_init(priv);
	if (ret) {
		dev_err(priv->dev, "EIP h/w init failed (%d)\n", ret);
		goto err_clk;
	}

	/*
	 * Kernel crypto API doesn't allow to register 2 engines.
	 * Allowing working with 2 engines requires additional modification
	 * which are planned as future work (Modify the Kernel crypto API or
	 * implement load balance in EIP driver to handle 2 engines).
	 *
	 * Currently we want to register the first probed engine.
	 */
	if (eip_in_use == -1 && priv->config.rings) {
		eip_in_use = priv->id;
		ret = safexcel_register_algorithms(priv);
		if (ret) {
			dev_err(dev, "Failed to register algorithms (%d)\n", ret);
			goto err_clk;
		}
	}

	return 0;

err_clk:
	clk_disable_unprepare(priv->clk);
	return ret;
}


static int safexcel_remove(struct platform_device *pdev)
{
	struct safexcel_crypto_priv *priv = platform_get_drvdata(pdev);
	int i;

	if (priv->id == eip_in_use)
		safexcel_unregister_algorithms(priv);

	safexcel_hw_reset_cdesc_rings(priv);
	safexcel_hw_reset_rdesc_rings(priv);

	clk_disable_unprepare(priv->clk);

	for (i = 0; i < priv->config.rings; i++)
		destroy_workqueue(priv->ring[i].workqueue);

	return 0;
}

static const struct of_device_id safexcel_of_match_table[] = {
	{
		.compatible = "inside-secure,safexcel-eip97",
		.data = (void *)EIP97,
	},
	{
		.compatible = "inside-secure,safexcel-eip197",
		.data = (void *)EIP197,
	},
	{},
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
module_param_array(rings, uint, NULL, 0);
MODULE_PARM_DESC(rings, "number of rings to be used by the driver");

MODULE_AUTHOR("Antoine Tenart <antoine.tenart@free-electrons.com>");
MODULE_AUTHOR("Ofer Heifetz <oferh@marvell.com>");
MODULE_AUTHOR("Igal Liberman <igall@marvell.com>");
MODULE_DESCRIPTION("Support for SafeXcel cryptographic engine EIP197");
MODULE_LICENSE("GPL v2");
