/*
 * Copyright (C) 2016 Marvell
 *
 * Antoine Tenart <antoine.tenart@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __SAFEXCEL_H__
#define __SAFEXCEL_H__

#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/aes.h>
#include <crypto/internal/hash.h>
#include <crypto/sha.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#define EIP197_HIA_VERSION_LE				0xca35
#define EIP197_HIA_VERSION_BE				0x35ca

/* Static configuration */
#define EIP197_DEFAULT_RING_SIZE			512

/* READ and WRITE cache control */
#define RD_CACHE_3BITS				0x5
#define WR_CACHE_3BITS				0x3
#define RD_CACHE_4BITS				(RD_CACHE_3BITS << 1 | 0x1)
#define WR_CACHE_4BITS				(WR_CACHE_3BITS << 1 | 0x1)

/* EIP-197 Classification Engine */
/* configuration parameters */
/* classification */
#define EIP197_CS_TRC_REC_WC				59
#define EIP197_CS_TRC_LG_REC_WC				73
/* record cache */
#define EIP197_RC_MAX_ENTRY_CNT				4096
#define EIP197_RC_MIN_ENTRY_CNT				32
#define EIP197_RC_ADMN_MEMWORD_ENTRY_CNT		8
#define EIP197_RC_ADMN_MEMWORD_WC			4
#define EIP197_RC_HEADER_WC				4
#define EIP197_RC_HASH_TABLE_SIZE_POWER_FACTOR		5
#define EIP197_RC_DMA_WR_COMB_DLY			0x07
#define EIP197_RC_NULL					0x3FF
/* transformation */
#define EIP197_TRC_ADMIN_RAM_WC				320
#define EIP197_TRC_RAM_WC				3840

/* Transformation Record Cache address */
 #define EIP197_TRC_PARAMS				0xf0820
#define EIP197_TRC_FREECHAIN				0xf0824
#define EIP197_TRC_PARAMS2				0xf0828
#define EIP197_TRC_ECCCTRL				0xf0830
#define EIP197_TRC_ECCSTAT				0xf0834

/* Classification regs */
#define EIP197_CS_RAM_CTRL				0xf7ff0
#define EIP197_CLASSIF_RAM_ACCESS_SPACE			0xe0000

#define EIP197_TRC_SW_RESET				(BIT(0))
#define EIP197_TRC_ENABLE(c)				(BIT(4) << c)
#define EIP197_TRC_ENABLE_MASK				(GENMASK(10, 0))

/* EIP-96 PRNG */
/* Registers   */
#define EIP197_PE_EIP96_PRNG_CTRL			0x01044
#define EIP197_PE_EIP96_PRNG_SEED_L			0x01048
#define EIP197_PE_EIP96_PRNG_SEED_H			0x0104c
#define EIP197_PE_EIP96_PRNG_KEY_0_L			0x01050
#define EIP197_PE_EIP96_PRNG_KEY_0_H			0x01054
#define EIP197_PE_EIP96_PRNG_KEY_1_L			0x01058
#define EIP197_PE_EIP96_PRNG_KEY_1_H			0x0105c
#define EIP197_PE_EIP96_PRNG_LFSR_L			0x01070
#define EIP197_PE_EIP96_PRNG_LFSR_H			0x01074
/* Register bits */
#define EIP197_PE_EIP96_PRNG_EN				BIT(0)
#define EIP197_PE_EIP96_PRNG_AUTO			BIT(1)
/* Default values */
#define EIP197_PE_EIP96_PRNG_SEED_L_VAL			0x48c24cfd
#define EIP197_PE_EIP96_PRNG_SEED_H_VAL			0x6c07f742
#define EIP197_PE_EIP96_PRNG_KEY_0_L_VAL		0xaee75681
#define EIP197_PE_EIP96_PRNG_KEY_0_H_VAL		0x0f27c239
#define EIP197_PE_EIP96_PRNG_KEY_1_L_VAL		0x79947198
#define EIP197_PE_EIP96_PRNG_KEY_1_H_VAL		0xe2991275
#define EIP197_PE_EIP96_PRNG_LFSR_L_VAL			0x21ac3c7c
#define EIP197_PE_EIP96_PRNG_LFSR_H_VAL			0xd008c4b4

/* Firmware */
#define EIP197_PE_ICE_SCRATCH_RAM(x)			(0x800 + (x * 4))
#define EIP197_NUM_OF_SCRATCH_BLOCKS			32

#define EIP197_PE_ICE_SCRATCH_CTRL_OFFSET		0xd04
#define EIP197_PE_ICE_SCRATCH_CTRL_DFLT			0x001f0200
#define EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_TIMER		BIT(2)
#define EIP197_PE_ICE_SCRATCH_CTRL_TIMER_EN		BIT(3)
#define EIP197_PE_ICE_SCRATCH_CTRL_CHANGE_ACCESS	BIT(24)
#define EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_OFFSET	25
#define EIP197_PE_ICE_SCRATCH_CTRL_SCRATCH_ACCESS_MASK	(GENMASK(28, 25))

#define EIP197_PE_ICE_PUE_CTRL				0xc80
#define EIP197_PE_ICE_PUE_CTRL_SW_RESET			BIT(0)
#define EIP197_PE_ICE_PUE_CTRL_CLR_ECC_CORR		BIT(14)
#define EIP197_PE_ICE_PUE_CTRL_CLR_ECC_NON_CORR		BIT(15)

#define EIP197_PE_ICE_FPP_CTRL				0xd80
#define EIP197_PE_ICE_FPP_CTRL_SW_RESET			BIT(0)
#define EIP197_PE_ICE_FPP_CTRL_CLR_ECC_NON_CORR		BIT(14)
#define EIP197_PE_ICE_FPP_CTRL_CLR_ECC_CORR		BIT(15)

#define EIP197_PE_ICE_RAM_CTRL				0xff0
#define EIP197_PE_ICE_RAM_CTRL_DFLT			0x00000000
#define EIP197_PE_ICE_RAM_CTRL_PUE_PROG_EN		BIT(0)
#define EIP197_PE_ICE_RAM_CTRL_FPP_PROG_EN		BIT(1)

/* EIP197_MST_CTRL values */
#define MST_CTRL_RD_CACHE(n)				(((n) & 0xf) << 0)
#define MST_CTRL_WD_CACHE(n)				(((n) & 0xf) << 4)

/* CDR/RDR register offsets */
#define EIP197_HIA_xDR_OFF(r)				((r) * 0x1000)
#define EIP197_HIA_CDR(r)				(EIP197_HIA_xDR_OFF(r))
#define EIP197_HIA_RDR(r)				(0x800 + EIP197_HIA_xDR_OFF(r))
#define EIP197_HIA_xDR_RING_SIZE			0x18
#define EIP197_HIA_xDR_CFG				0x20
#define EIP197_HIA_xDR_PREP_COUNT			0x2c
#define EIP197_HIA_xDR_PROC_COUNT			0x30
#define EIP197_HIA_xDR_PREP_PNTR			0x34
#define EIP197_HIA_xDR_PROC_PNTR			0x38

/* EIP197_HIA_xDR_PREP_COUNT */
#define EIP197_xDR_PREP_CLR_COUNT			BIT(31)

/* EIP197_HIA_xDR_PROC_COUNT */
#define EIP197_xDR_PROC_CLR_COUNT			BIT(31)

#define EIP197_HIA_RA_PE_CTRL_RESET			BIT(31)
#define EIP197_HIA_RA_PE_CTRL_EN			BIT(30)

/* Register offsets */
/* unit offsets */
#define EIP197_HIA_AIC_ADDR				0x90000
#define EIP197_HIA_AIC_G_ADDR				0x90000
#define EIP197_HIA_AIC_R_ADDR				0x90800
#define EIP197_HIA_AIC_xDR_ADDR				0x80000
#define EIP197_HIA_AIC_DFE_ADDR				0x8c000
#define EIP197_HIA_AIC_DFE_THRD_ADDR			0x8c040
#define EIP197_HIA_AIC_DSE_ADDR				0x8d000
#define EIP197_HIA_AIC_DSE_THRD_ADDR			0x8d040
#define EIP197_HIA_PE_ADDR				0xa0000
#define EIP197_CLASSIFICATION_RAMS			0xe0000
#define EIP197_HIA_GC					0xf0000

#define EIP197_HIA_AIC_R_OFF(r)			((r) * 0x1000)
#define EIP197_HIA_AIC_R_ENABLE_CLR(r)		(0xe014 - EIP197_HIA_AIC_R_OFF(r))

#define EIP197_HIA_RA_PE_CTRL			0x010

#define EIP197_HIA_DFE_CFG			0x000
#define EIP197_HIA_DFE_THR_CTRL			0x000

#define EIP197_HIA_DSE_CFG			0x000
#define EIP197_HIA_DSE_THR_CTRL			0x000
#define EIP197_HIA_DSE_THR_STAT			0x004

#define EIP197_HIA_AIC_G_ENABLE_CTRL		0xf808
#define EIP197_HIA_AIC_G_ACK			0xf810
#define EIP197_HIA_MST_CTRL			0xfff4
#define EIP197_HIA_OPTIONS			0xfff8
#define EIP197_HIA_VERSION			0xfffc
#define EIP197_PE_IN_DBUF_THRES			0x0000
#define EIP197_PE_IN_TBUF_THRES			0x0100
#define EIP197_PE_OUT_DBUF_THRES		0x1c00
#define EIP197_MST_CTRL				0xfff4

/* EIP197_HIA_DFE/DSE_CFG */
#define EIP197_HIA_DxE_CFG_MIN_DATA_SIZE(n)	((n) << 0)
#define EIP197_HIA_DxE_CFG_DATA_CACHE_CTRL(n)	(((n) & 0x7) << 4)
#define EIP197_HIA_DxE_CFG_MAX_DATA_SIZE(n)	((n) << 8)
#define EIP197_HIA_DxE_CFG_MIN_CTRL_SIZE(n)	((n) << 16)
#define EIP197_HIA_DxE_CFG_CTRL_CACHE_CTRL(n)	(((n) & 0x7) << 20)
#define EIP197_HIA_DxE_CFG_MAX_CTRL_SIZE(n)	((n) << 24)
#define EIP197_HIA_DFE_CFG_DIS_DEBUG		(BIT(31) | BIT(29))
#define EIP197_HIA_DSE_CFG_DIS_DEBUG		BIT(31)

/* EIP197_HIA_DFE/DSE_THR_CTRL */
#define EIP197_DxE_THR_CTRL_EN			BIT(30)
#define EIP197_DxE_THR_CTRL_RESET_PE		BIT(31)

/* EIP197_HIA_MST_CTRL */
#define EIP197_HIA_SLAVE_BYTE_SWAP			BIT(24)
#define EIP197_HIA_SLAVE_NO_BYTE_SWAP		BIT(25)

/* EIP197_PE_IN_DBUF/TBUF_THRES */
#define EIP197_PE_IN_xBUF_THRES_MIN(n)		((n) << 8)
#define EIP197_PE_IN_xBUF_THRES_MAX(n)		((n) << 12)

/* EIP197_PE_OUT_DBUF_THRES */
#define EIP197_PE_OUT_DBUF_THRES_MIN(n)		((n) << 0)
#define EIP197_PE_OUT_DBUF_THRES_MAX(n)		((n) << 4)

/*
 * Internal structures & functions
 */
enum eip197_fw {
	IFPP_FW = 0,
	IPUE_FW,
	MAX_FW_NR
};

enum safexcel_eip_type {
	EIP197,
};

/* internal unit register offset */
struct safexcel_unit_offset {
	u32 hia_aic;
	u32 hia_aic_g;
	u32 hia_aic_r;
	u32 hia_xdr;
	u32 hia_dfe;
	u32 hia_dfe_thrd;
	u32 hia_dse;
	u32 hia_dse_thrd;
	u32 hia_gen_cfg;
	u32 pe;
};

#define EIP197_HIA_AIC(priv)		((priv)->base + (priv)->unit_off.hia_aic)
#define EIP197_HIA_AIC_G(priv)		((priv)->base + (priv)->unit_off.hia_aic_g)
#define EIP197_HIA_AIC_R(priv)		((priv)->base + (priv)->unit_off.hia_aic_r)
#define EIP197_HIA_AIC_xDR(priv)	((priv)->base + (priv)->unit_off.hia_xdr)
#define EIP197_HIA_DFE(priv)		((priv)->base + (priv)->unit_off.hia_dfe)
#define EIP197_HIA_DFE_THRD(priv)	((priv)->base + (priv)->unit_off.hia_dfe_thrd)
#define EIP197_HIA_DSE(priv)		((priv)->base + (priv)->unit_off.hia_dse)
#define EIP197_HIA_DSE_THRD(priv)	((priv)->base + (priv)->unit_off.hia_dse_thrd)
#define EIP197_HIA_GEN_CFG(priv)	((priv)->base + (priv)->unit_off.hia_gen_cfg)
#define EIP197_PE(priv)			((priv)->base + (priv)->unit_off.pe)
#define EIP197_RAM(priv)		((priv)->base + EIP197_CLASSIFICATION_RAMS)

struct safexcel_config {
	u32 rings;

	u32 cd_size;
	u32 cd_offset;

	u32 rd_size;
	u32 rd_offset;
};

struct safexcel_crypto_priv {
	void __iomem *base;
	struct safexcel_unit_offset unit_off;
	struct device *dev;
	struct clk *clk;
	enum safexcel_eip_type eip_type;
	struct safexcel_config config;

	spinlock_t lock;
};

#endif
