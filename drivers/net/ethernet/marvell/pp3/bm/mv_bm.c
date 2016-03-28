/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

/* includes */
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "qm/mv_qm.h"
#include "mv_bm.h"
#include "mv_bm_mem.h"
#include "mv_bm_regs.h"

static int bm_base_offset;
static void __iomem *mv_hw_silicon;

static int bm_regs_debug_flags;


/*------------------------------------------------------------------------------*/
/*			BM internal functions					*/
/*------------------------------------------------------------------------------*/


int bm_gp_pid_validation(int pid)
{
	if ((pid < BM_GP_POOL_MIN) || (pid > BM_GP_POOL_MAX)) {
		pr_err("%s Error - Invalid pool id %d\n", __func__, pid);
		return -1;
	}
	return 0;
}

static int bm_qm_pid_validation(int pid)
{
	if ((pid < BM_QM_GPM_POOL_0) || (pid > BM_QM_DRAM_POOL_1)) {
		pr_err("%s Error - Invalid pool id %d\n", __func__, pid);
		return -1;
	}
	return 0;
}

static int bm_pid_validation(int pid)
{
	int ret_val = 0;

	if ((pid < BM_QM_GPM_POOL_0) || (pid > BM_GP_POOL_MAX))
		ret_val = -1;

	/* pools 4-7 not exist */
	if ((pid > BM_QM_DRAM_POOL_1) && (pid < BM_GP_POOL_MIN))
		ret_val = -1;

	if (ret_val)
		pr_err("%s Error - Invalid pool id %d\n", __func__, pid);

	return ret_val;
}

static int bm_pid_to_bid(int pid)
{
	/* return pid index in cache bank */

	if (pid <= BM_QM_DRAM_POOL_1)
		return 0;
	else
		return (pid % 4) + 1;
}

static int bm_pid_to_local(int pid)
{
	/* return pid index in cache bank */

	if (pid <= BM_QM_DRAM_POOL_1)
		return pid;
	else
		return (pid - 8) >> 2;
}

static int bm_local_to_pid(int local_pid, int bid)
{
	/* return pid index in cache bank */
	if (bid == 0)
		return local_pid;
	else
		return bid - 1 + 8 + (local_pid << 2);
}


static int bm_pid_to_global(int pid)
{
	/* return sequential pool id
	   skip pool id 4-7*/

	if (pid >= BM_GP_POOL_MIN)
		return pid - 4;
	else
		return pid;
}

/*------------------------------------------------------------------------------*/
/*		BM registers & memories access APIs				*/
/*------------------------------------------------------------------------------*/

void bm_dbg_flags(u32 flag, u32 en)
{
	u32 bit_flag;

	bit_flag = (fls(flag) - 1);

	if (en)
		bm_regs_debug_flags |= (1 << bit_flag);
	else
		bm_regs_debug_flags &= ~(1 << bit_flag);

	return;
}

void mv_pp3_bm_init(void __iomem *base)
{
	bm_base_offset = BM_UNIT_OFFS;
	mv_hw_silicon = base;
}

static void bm_gl_reg_write(unsigned int reg, unsigned int data)
{
	mv_pp3_hw_reg_write(reg + bm_base_offset + mv_hw_silicon, data);

	if (bm_regs_debug_flags & BM_F_DBG_WR)
		pr_info("BM WRITE: %p = 0x%08x\n", reg + bm_base_offset + mv_hw_silicon, data);
}

static unsigned int bm_gl_reg_read(unsigned int reg)
{
	unsigned int reg_data;

	mv_pp3_hw_read(reg + bm_base_offset + mv_hw_silicon, 1, &reg_data);

	if (bm_regs_debug_flags & BM_F_DBG_RD)
		pr_info("BM READ : %p = 0x%08x\n", reg + bm_base_offset + mv_hw_silicon, reg_data);

	return reg_data;
}
static void bm_gl_reg_print(char *reg_name, unsigned int reg)
{
	pr_info(" %-32s: %p = 0x%08x\n",
		reg_name, reg + bm_base_offset + mv_hw_silicon, bm_gl_reg_read(reg));

}

static void bm_gl_reg_none_zr_print(char *reg_name, unsigned int reg)
{
	pr_info(" %-32s: %p = 0x%08x\n",
		reg_name, reg + bm_base_offset + mv_hw_silicon, bm_gl_reg_read(reg));
}

static void bm_entry_read(int offs, int words,  unsigned int *entry)
{
	mv_pp3_hw_read(mv_hw_silicon + offs, words, entry);

	if (bm_regs_debug_flags & BM_F_DBG_RD) {
		int i;

		for (i = 0; i < words; i++)
			pr_info("BM READ : %p = 0x%08x\n", mv_hw_silicon + offs + (4 * i), entry[i]);
	}
}

static void bm_entry_write(int offs, int words,  unsigned int *entry)
{
	mv_pp3_hw_write(mv_hw_silicon + offs, words, entry);

	if (bm_regs_debug_flags & BM_F_DBG_WR) {
		int i;

		for (i = 0; i < words; i++)
			pr_info("BM WRITE: %p = 0x%08x\n", mv_hw_silicon + offs + (4 * i), entry[i]);
	}
}

static int bm_entry_print(char *name, int offs, int words)
{
	char reg_name[100];
	unsigned int *entry;
	int i;

	entry = kzalloc(words * sizeof(unsigned int), GFP_KERNEL);

	if (!entry) {
		pr_info("%s: Error - out of memory\n", __func__);
		return -ENOMEM;
	}

	bm_entry_read(offs, words, entry);

	for (i = 0; i < words; i++) {
		sprintf(reg_name, "%s[%d]", name, i);
		pr_info(" %-32s: %p = 0x%08x\n", reg_name,
				mv_hw_silicon + offs + (4*i), entry[i]);
	}

	kfree(entry);
	return 0;
}

/*------------------------------------------------------------------------------*/
/*		BM User Application Interface					*/
/*------------------------------------------------------------------------------*/


/* Set default BM attributes for read/write in DRAM */
void bm_attr_all_pools_def_set(void)
{
	unsigned int reg_val;

	reg_val =  (BM_ADOMAIN << BM_DRAM_DOMAIN_CFG_WR_B0_OFFS) |
			(BM_ADOMAIN << BM_DRAM_DOMAIN_CFG_WR_BGP_OFFS) |
			(BM_ADOMAIN << BM_DRAM_DOMAIN_CFG_RD_B0_OFFS) |
			(BM_ADOMAIN << BM_DRAM_DOMAIN_CFG_RD_BGP_OFFS);
	bm_gl_reg_write(BM_DRAM_DOMAIN_CFG_REG, reg_val);

	reg_val =  (BM_AWCACHE << BM_DRAM_CACHE_CFG_WR_B0_OFFS) |
			(BM_AWCACHE << BM_DRAM_CACHE_CFG_WR_BGP_OFFS) |
			(BM_ARCACHE << BM_DRAM_CACHE_CFG_RD_B0_OFFS) |
			(BM_ARCACHE << BM_DRAM_CACHE_CFG_RD_BGP_OFFS);
	bm_gl_reg_write(BM_DRAM_CACHE_CFG_REG, reg_val);

	reg_val =  (BM_AWQOS << BM_DRAM_QOS_CFG_WR_B0_OFFS) |
			(BM_AWQOS << BM_DRAM_QOS_CFG_WR_BGP_OFFS) |
			(BM_ARQOS << BM_DRAM_QOS_CFG_RD_B0_OFFS) |
			(BM_ARQOS << BM_DRAM_QOS_CFG_RD_BGP_OFFS);
	bm_gl_reg_write(BM_DRAM_QOS_CFG_REG, reg_val);
}

/* BM module status */
static bool bm_enable_status(void)
{
	unsigned int reg_val;
	bool enable;

	reg_val = bm_gl_reg_read(BM_COMMON_GENERAL_CFG_REG);
	enable = reg_val & BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_MASK ? true : false;
	return enable;
}

static void bm_pool_enabled_get(int pool, bool *enabled, bool *quick_init)
{
	unsigned int reg_val, enabled_flag, quick_flag;
	int bid, pid_local;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);

	if  (bid == 0) { /* QM pools */
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		enabled_flag = reg_val & BM_B0_POOL_CFG_ENABLE_MASK;
		quick_flag = reg_val & BM_B0_POOL_CFG_QUICK_INIT_MASK;
	} else {
		/* GP pools */
		reg_val = bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		enabled_flag = reg_val & reg_val & BM_BGP_POOL_CFG_ENABLE_MASK;
		quick_flag = reg_val & BM_BGP_POOL_CFG_QUICK_INIT_MASK;
	}

	if (enabled)
		*enabled = enabled_flag ? true : false;
	if (quick_init)
		*quick_init = quick_flag ? true : false;
}
/*
 * enable BM module
 */
void bm_enable(void)
{
	unsigned int reg_val;

	reg_val = bm_gl_reg_read(BM_COMMON_GENERAL_CFG_REG);

	/* set swap in to 0 */
	reg_val &= ~BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_MASK;
	/* set request rcv to 1 */
	reg_val |= BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_MASK;

	bm_gl_reg_write(BM_COMMON_GENERAL_CFG_REG, reg_val);
}

void bm_disable(void)
{
	unsigned int reg_val;

	reg_val = bm_gl_reg_read(BM_COMMON_GENERAL_CFG_REG);

	/* set swap in to 0 */
	reg_val &= ~BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_MASK;
	/* set request rcv to 0 */
	reg_val &= ~BM_COMMON_GENERAL_CFG_BM_REQ_RCV_EN_MASK;

	bm_gl_reg_write(BM_COMMON_GENERAL_CFG_REG, reg_val);
}


void bm_vmid_set(unsigned int bm_vmid)
{
	unsigned int reg_val;

	reg_val = bm_gl_reg_read(BM_COMMON_GENERAL_CFG_REG);

	/* set swap in to 0 */
	reg_val &= ~BM_COMMON_GENERAL_CFG_DRM_SI_DECIDE_EXTRA_FILL_MASK;

	/* set vmid */
	reg_val &= ~BM_COMMON_GENERAL_CFG_DM_VMID_MASK;
	reg_val |= bm_vmid << BM_COMMON_GENERAL_CFG_DM_VMID_OFFS;

	bm_gl_reg_write(BM_COMMON_GENERAL_CFG_REG, reg_val);
}

/**
 *  Fill memory of pool with PE index
 *	PE index is incrementing value of 1 to num_of_buffers
 *  Mark PE with its location (GPM or DRAM)
 *  Return values:
 *		0 - success
 */
/*BM Internal functions*/
static int bm_memory_fill(int buf_num, struct mv_a40 *base_address)
{
	unsigned int i, *base;

	/*
	 * fill all PE's with incrementing value (starting with 1)
	 * Write in Dram in BM pool section an incrementing index
	 */

	base = (unsigned int *)(base_address->virt_lsb);

	for (i = 1; i <= buf_num; i++) {
		*base = i;
		base++;
	}

	return 0;
}

/**
 *  Configure BM with Fill level of pool in DRAM
 *  Note: Must be called before pool is enabled
 *  Return values:
 *		0 - success
 */
static int bm_pool_dram_set(int pool, int buf_num, int pe_bits, struct mv_a40 *base_address,
						unsigned int ae_thr, unsigned int af_thr)
{
	unsigned int entry_ptr[BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS];
	unsigned int entry_offset;
	int global_pid, pe_bytes;

	if (bm_pid_validation(pool))
		return -1;

	global_pid = bm_pid_to_global(pool);
	entry_offset = BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY(global_pid);

	bm_entry_read(entry_offset, BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS, entry_ptr);

	mv_field_set(BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_OFFS,
			BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_BITS, entry_ptr,  base_address->dma_msb);
	mv_field_set(BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_OFFS,
			BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_BITS, entry_ptr,  base_address->dma_lsb);

	/* for pools 1-3 pe_bits must be 32 bits */
	/* pe size in dram align to 4 bytes */
	pe_bytes = MV_ALIGN_UP(pe_bits, MV_WORD_BITS) / MV_BYTE_BITS;

	/* fields are in unit of 64 bytes */
	mv_field_set(BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_OFFS,
			BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_BITS, entry_ptr, (ae_thr*pe_bytes) / BM_DRAM_LINE_BYTES);
	mv_field_set(BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_OFFS,
			BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_BITS, entry_ptr, (af_thr*pe_bytes) / BM_DRAM_LINE_BYTES);
	mv_field_set(BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_OFFS,
			BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_BITS, entry_ptr, (buf_num*pe_bytes) / BM_DRAM_LINE_BYTES);

	bm_entry_write(entry_offset, BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS, entry_ptr);
	return 0;
}
/*
 *  Configure BM with Fill level of pool in DRAM
 *	Supports all pools (QM and general purpose)
 *  Note: Must be called before pool is enabled
 *	  for pools in quick init mode, buff_num sould set to 0
 */
static void bm_pool_fill_level_set(int pool, int buf_num, int pe_bits)
{
	unsigned int entry_ptr[BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS];
	unsigned int entry_offset, dram_fill;
	int global_pid, pe_bytes;

	global_pid = bm_pid_to_global(pool);
	entry_offset = BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY(global_pid);
	bm_entry_read(entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS, entry_ptr);

	/* pe size in dram align to 4 bytes */
	pe_bytes = MV_ALIGN_UP(pe_bits, MV_WORD_BITS) / MV_BYTE_BITS;
	dram_fill = pe_bytes * buf_num;

	/* dram fill level in unit of 8 bytes */
	mv_field_set(BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_OFFS,
		BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_BITS, entry_ptr, dram_fill / 8);

	bm_entry_write(entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS, entry_ptr);
}

static int bm_pool_quick_disable(int pool)
{
	unsigned int reg_val;
	int bid, pid_local;

	if (bm_pid_validation(pool))
		return -1;

	/* set dram fill level to 0 */
	bm_pool_fill_level_set(pool, 0, MV_32_BITS);


	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);

	if  (bid == 0) {
		/* QM pools */
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		reg_val &= ~BM_B0_POOL_CFG_QUICK_INIT_MASK;
		bm_gl_reg_write(BM_B0_POOL_CFG_REG(pid_local), reg_val);
	} else {
		/* QP pools */
		reg_val =  bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		reg_val &= ~BM_BGP_POOL_CFG_QUICK_INIT_MASK;
		bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);
	}
	return 0;
}

static int bm_qm_pool_quick_enable(int pool, int buf_num, struct mv_a40 *base_address)
{
	int pid_local;
	unsigned int reg_val;

	if (bm_qm_pid_validation(pool))
		return -1;

	if (!base_address)
		return -1;

	pid_local = bm_pid_to_local(pool);

	/* set dram fill level */
	bm_pool_fill_level_set(pool, buf_num, MV_32_BITS);

	bm_memory_fill(buf_num, base_address);

	/* set quick int bit */
	reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
	reg_val |= BM_B0_POOL_CFG_QUICK_INIT_MASK;
	bm_gl_reg_write(BM_B0_POOL_CFG_REG(pid_local), reg_val);

	return 0;
}


int bm_qm_pool_total_thresholds(int pool, unsigned int a_empty, unsigned int aa_empty)
{
	int pid_local;
	unsigned int reg_val;

	if (bm_qm_pid_validation(pool))
		return -1;

	pid_local = bm_pid_to_local(pool);

	/* QM pools */
	reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
	reg_val &= ~BM_B0_POOL_CFG_AE_THR_MASK;
	reg_val &= ~BM_B0_POOL_CFG_AAE_THR_MASK;
	reg_val |= (a_empty << BM_B0_POOL_CFG_AE_THR_OFFS);
	reg_val |= (aa_empty << BM_B0_POOL_CFG_AAE_THR_OFFS);
	bm_gl_reg_write(BM_B0_POOL_CFG_REG(pid_local), reg_val);

	return 0;
}

int bm_gp_pool_total_threshold(int pool, unsigned int a_empty)
{
	int pid_local, bid;
	unsigned int reg_val;

	if (bm_gp_pid_validation(pool))
		return -1;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);

	/* QM pools */
	reg_val = bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
	reg_val &= ~BM_BGP_POOL_CFG_AE_THR_MASK;
	reg_val |= (a_empty << BM_BGP_POOL_CFG_AE_THR_OFFS);
	bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);

	return 0;
}

int bm_pool_enable(int pool)
{
	unsigned int reg_val;
	int bid, pid_local;

	if (bm_pid_validation(pool))
		return -1;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);

	if  (bid == 0) {
		/* QM pools */
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		reg_val |= BM_B0_POOL_CFG_ENABLE_MASK;
		bm_gl_reg_write(BM_B0_POOL_CFG_REG(pid_local), reg_val);
	} else {
		/* QP pools */
		reg_val =  bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		reg_val |= BM_BGP_POOL_CFG_ENABLE_MASK;
		bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);
	}
	return 0;
}

int bm_pool_disable(int pool)
{
	unsigned int reg_val;
	int bid, pid_local;

	if (bm_pid_validation(pool))
		return -1;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);

	if  (bid == 0) {
		/* QM pools */
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		reg_val &= ~BM_B0_POOL_CFG_ENABLE_MASK;
		bm_gl_reg_write(BM_B0_POOL_CFG_REG(pid_local), reg_val);
	} else {
		/* QP pools */
		reg_val =  bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		reg_val &= ~BM_BGP_POOL_CFG_ENABLE_MASK;
		bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);
	}

	return 0;
}
/**
 *  Set PE pointer size in general purpose pool
 *
 *  Return values:
 *		0 - success
 */
static int bm_gp_pool_pe_size_set(int pool, int pe_bits)
{
	unsigned int reg_val, bid, pid_local;

	if (bm_gp_pid_validation(pool))
		return -1;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);
	reg_val =  bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));

	if (pe_bits == MV_32_BITS)
		reg_val |= BM_BGP_POOL_CFG_PE_SIZE_MASK;
	else if (pe_bits == MV_40_BITS)
		reg_val &= ~BM_BGP_POOL_CFG_PE_SIZE_MASK;
	else
		return -1;

	bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);

	return 0;
}

/**
 *  Configure if global pool (8-35) is defined to work in pairs
 *
 *  Return values:
 *		0 - success
 */
static int bm_gp_pool_pair_set(int pool, bool pool_pair)
{
	unsigned int reg_val;
	int bid, pid_local;

	if (bm_gp_pid_validation(pool))
		return -1;

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);
	reg_val =  bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));

	if (pool_pair)
		reg_val |= BM_BGP_POOL_CFG_IN_PAIRS_MASK;
	else
		reg_val &= ~BM_BGP_POOL_CFG_IN_PAIRS_MASK;

	bm_gl_reg_write(BM_BGP_POOL_CFG_REG(bid, pid_local), reg_val);

	return 0;
}
/**
 *  Configure pool cache parameters
 *
 *  Note: Must be called before pool is enabled
 *  Return values:
 *		0 - success
 */
static int bm_pool_cache_set(int pool, unsigned int vmid, unsigned int attr,
				unsigned int so_thr, unsigned int si_thr, unsigned int cache_buf_num)
{

	unsigned int entry_ptr[BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS];
	unsigned int entry_offset;
	int pid, bid, curr_pid_local, pid_local, bank_pools_num;
	int pe_cache_units, end_max, cache_end, cache_start;
	bool pool_enable;

	/* fields calculated in cache lines with granularity of 64B
		for B0 it is 16 PE's
		for B1..B4 it is 8 PE's */

	pe_cache_units = 64 / BM_CACHE_PE_BYTES(pool);

	if (cache_buf_num % pe_cache_units)
		return -1;

	if (so_thr < si_thr + 16) {
		pr_err("cache swap out threshold should be larger than cache swap in plus 16\n");
		return -1;
	}

	bid = bm_pid_to_bid(pool);
	/* B0 - 4 pools shares same cache which is 512 lines, each with 4 PE's = 2048 PE's
	   B1..B4 each bank has 7 pools that shares 1024 lines, each with 1 PE = 1024 PE's */

	bank_pools_num = BM_BANK_POOLS_NUM(bid);

	end_max = -1;
	/* run on all pools in bank and check what is the end of the occupied part */
	for (curr_pid_local = 0; curr_pid_local < bank_pools_num; curr_pid_local++) {
		pid = bm_local_to_pid(curr_pid_local, bid);
		bm_pool_enabled_get(pid, &pool_enable, NULL);

		/* if pool is not enabled that we don't take its cache configuration into account */
		if (pool_enable == false)
			continue;

		entry_offset = BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, curr_pid_local);
		bm_entry_read(entry_offset, BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS, entry_ptr);
		cache_end = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_END_OFFS,
						BM_DPR_C_MNG_BANK_STAT_CACHE_END_BITS, entry_ptr);
		end_max = MV_MAX(end_max, cache_end);
	}

	cache_start = end_max + 1;

	cache_end = cache_start + (cache_buf_num / pe_cache_units) - 1;

	/* Swap-out threshold Unit is 1 PE.
	   Swap-in threshold Unit is 1 PE.
	   start and end are in line units (GP - 1PE, QM - 4 PE's)
	   and in 64B granularity */

	/* set all fields in entry */
	pid_local = bm_pid_to_local(pool);
	entry_offset = BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, pid_local);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_START_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_START_BITS, entry_ptr, cache_start);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_END_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_END_BITS, entry_ptr, cache_end);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_BITS, entry_ptr, si_thr);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_BITS, entry_ptr, so_thr);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_BITS, entry_ptr, attr);
	mv_field_set(BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_BITS, entry_ptr, vmid);
	bm_entry_write(entry_offset, BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS, entry_ptr);
	return 0;
}

/*
 * Init of GP pools
 */
int bm_gp_pool_def_basic_init(int pool, int buf_num, struct mv_a40 *base_address)
{
	unsigned int ret_val = 0;

	if (bm_gp_pid_validation(pool))
		return -1;

	ret_val |= bm_pool_dram_set(pool, buf_num, MV_32_BITS, base_address,
					BM_DRAM_AE(buf_num), BM_DRAM_AF(buf_num));
	ret_val |= bm_pool_cache_set(pool, BM_VMID, BM_CACHE_ATTR,
					BM_GP_CACHE_SO, BM_GP_CACHE_SI, BM_GP_CACHE_BUF_NUM);
	ret_val |= bm_gp_pool_pe_size_set(pool, MV_32_BITS);
	ret_val |= bm_gp_pool_pair_set(pool, true);
	ret_val |= bm_pool_quick_disable(pool);
	ret_val |= bm_gp_pool_total_threshold(pool, BM_TOT_AE);
	ret_val |= bm_pool_enable(pool);
	return ret_val;
}

static int bm_pool_quick_init_complete(int pool)
{
	int bid, pid_local, count = 0;
	bool enabled, quick_init, complete = true;
	u32 reg_val, reg_address;

	bm_pool_enabled_get(pool, &enabled, &quick_init);

	if (!enabled) {
		pr_err("%s: Error - pool %d is disabled\n", __func__, pool);
		return -1;
	}
	if (!quick_init) {
		pr_err("%s: Error - pool %d not in quick init mode\n", __func__, pool);
		return -1;
	}

	pid_local = bm_pid_to_local(pool);

	bid = bm_pid_to_bid(pool);

	if (bid)

		reg_address = BM_BGP_POOL_STATUS_REG(bid, pid_local);
	else
		reg_address = BM_B0_POOL_STATUS_REG(pid_local);

	do {
		if (count++ >= 100) {
			pr_err("pool #%d quick initialization time out\n", pool);
			return -1;
		}

		mdelay(1);

		reg_val = bm_gl_reg_read(reg_address);

		complete = reg_val & BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_MASK ? true : false;

	} while (!complete);

	return 0;
}

/* Quick init of QM pools */
int bm_qm_gpm_pools_def_quick_init(int buf_num, struct mv_a40 *qece_base, struct mv_a40 *pl_base)
{
	struct mv_a40 *base_address;
	bool bm_enable;
	unsigned int pool, ret_val = 0;

	if (buf_num > BM_QM_GPM_POOL_CAPACITY) {
		pr_err("Invalid GPM pools num of buffers %d\n", buf_num);
		return -EINVAL;
	}

	/* HW limitation, pools 0-3 should hold at least 16 empty PEs */
	buf_num = buf_num - 16;

	bm_enable = bm_enable_status();

	if (bm_enable) {
		pr_err("%s: Error - pool 0 and 1 should be enabled before BM module is enabled\n", __func__);
		return -EINVAL;
	}

	for (pool = BM_QM_GPM_POOL_0; pool <= BM_QM_GPM_POOL_1; pool++) {

		base_address = (pool == BM_QM_GPM_POOL_0) ? pl_base : qece_base;

		ret_val |= bm_pool_dram_set(pool, buf_num, MV_32_BITS, base_address,
						BM_DRAM_AE(buf_num), BM_DRAM_AF(buf_num));
		ret_val |= bm_pool_cache_set(pool, BM_VMID, BM_CACHE_ATTR,
						BM_QM_CACHE_SO, BM_QM_CACHE_SI, BM_QM_CACHE_BUF_NUM);
		ret_val |= bm_qm_pool_total_thresholds(pool, BM_TOT_AE, BM_TOT_AAE);
		ret_val |= bm_qm_pool_quick_enable(pool, buf_num, base_address);
		ret_val |= bm_pool_enable(pool);

		if (ret_val) {
			pr_err("%s: Error- pool %d default quick initialization failed\n", __func__, pool);
			return ret_val;
		}

		if (bm_pool_quick_init_complete(pool) < 0) {
			pr_err("%s: Error - pool %d default quick initialization not complete\n", __func__, 1);
			return -1;
		}
	}

	return 0;
}

int bm_qm_dram_pools_def_quick_init(struct device *dev, int buf_num, struct mv_a40 *qece_base, struct mv_a40 *pl_base)
{

	struct mv_a40 *base_address;
	struct mv_a40 address_allocate[BM_QM_DRAM_POOLS_NUM];
	unsigned int pool, ret_val = 0;
	int index;


	memset(address_allocate, 0, BM_QM_DRAM_POOLS_NUM * sizeof(struct mv_a40));

	for (pool = BM_QM_DRAM_POOL_0; pool <= BM_QM_DRAM_POOL_1; pool++) {

		base_address = (pool == BM_QM_DRAM_POOL_0) ? pl_base : qece_base;
		index = pool - BM_QM_DRAM_POOL_0;

		ret_val |= bm_pool_dram_set(pool, buf_num, MV_32_BITS, base_address,
						BM_DRAM_AE(buf_num), BM_DRAM_AF(buf_num));
		ret_val |= bm_pool_cache_set(pool, BM_VMID, BM_CACHE_ATTR, BM_QM_CACHE_SO,
						BM_QM_CACHE_SI, BM_QM_CACHE_BUF_NUM);

		/* for pools 2&3 it allocates the buffer memory before filling the pool	*/

		address_allocate[index].virt_lsb =
				(unsigned int)dma_alloc_coherent(dev, (buf_num+1)*BM_QM_DRAM_POOL_BUF_SIZE(pool),
						&address_allocate[index].dma_lsb, GFP_KERNEL);

		if (address_allocate[index].virt_lsb == (unsigned int)NULL)
			goto oom;

		ret_val |= bm_qm_pool_quick_enable(pool, buf_num, base_address);
		ret_val |= bm_qm_pool_total_thresholds(pool, BM_TOT_AE, BM_TOT_AAE);
		ret_val |= bm_pool_enable(pool);

		if (ret_val) {
			pr_err("%s: Error- pool %d default quick initialization failed\n", __func__, pool);
			goto err;
		}

		if (bm_pool_quick_init_complete(pool) < 0) {
			pr_err("%s: Error - pool %d quick initialization not complete\n", __func__, pool);
			goto err;
		}
	}

	qm_pfe_base_address_pool_set(&address_allocate[1], &address_allocate[0]);

	return MV_OK;

oom:
	pr_err("%s: out of memory\n", __func__);
err:
	for (index = 0; index < BM_QM_DRAM_POOLS_NUM; index++)
		kfree((void *)address_allocate[index].virt_lsb);

	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*BM Debug functions*/
void bm_global_registers_dump(void)
{
	int i;
	char reg_name[50];

	pr_info("\n-------------- BM Global registers dump -----------");

	for (i = 0; i < 4; i++) {
		sprintf(reg_name, "SYS_NREC_COMMON_D%d_STATUS", i);
		bm_gl_reg_print(reg_name, BM_SYS_NREC_COMMON_DX_STATUS_REG(i));
	}

	bm_gl_reg_print("COMMON_GENERAL_CFG_REG" , BM_COMMON_GENERAL_CFG_REG);
	bm_gl_reg_print("DRAM_DOMAIN_CFG_REG" , BM_DRAM_DOMAIN_CFG_REG);
	bm_gl_reg_print("DRAM_CACHE_CFG_REG" , BM_DRAM_CACHE_CFG_REG);
	bm_gl_reg_print("DRAM_QOS_CFG_REG" , BM_DRAM_QOS_CFG_REG);
	bm_gl_reg_print("DM_AXI_FIFOS_STATUS_REG" , BM_DM_AXI_FIFOS_STATUS_REG);
	bm_gl_reg_print("DRM_PENDING_FIFO_STATUS_REG", BM_DRM_PENDING_FIFO_STATUS_REG);
	bm_gl_reg_print("DM_AXI_WRITE_PENDING_FIFO_STATUS_REG", BM_DM_AXI_WRITE_PENDING_FIFO_STATUS_REG);
	bm_gl_reg_print("IDLE_STATUS_REG", BM_IDLE_STATUS_REG);
}

void bm_pool_registers_dump(int pool)
{
	int bid, pid_local, pid_global;
	char reg_name[50];

	if (bm_pid_validation(pool)) {
		pr_err("Invalid pool id %d\n", pool);
		return;
	}
	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);
	pid_global = bm_pid_to_global(pool);

	pr_info("\n-------------- BM pool %d registers dump -----------\n", pool);

	if (bid == 0) {
		bm_gl_reg_print("B0_POOL_CFG_REG", BM_B0_POOL_CFG_REG(pid_local));
		bm_gl_reg_print("B0_POOL_STATUS_REG", BM_B0_POOL_STATUS_REG(pid_local));
	} else {
		sprintf(reg_name, "B%d_POOL_CFG_REG", bid);
		bm_gl_reg_print(reg_name, BM_BGP_POOL_CFG_REG(bid, pid_local));
		sprintf(reg_name, "B%d_POOL_STATUS_REG", bid);
		bm_gl_reg_print(reg_name, BM_BGP_POOL_STATUS_REG(bid, pid_local));
	}

	sprintf(reg_name, "DPR_C_MNG_B%d_STAT", bid);
	bm_entry_print(reg_name,
			BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, pid_local), BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS);

	sprintf(reg_name, "TPR_C_MNG_B%d_DYN", bid);
	bm_entry_print(reg_name,
			BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY(bid, pid_local), BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS);

	bm_entry_print("DPR_D_MNG_BALL_STAT",
			BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY(pid_global), BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS);

	bm_entry_print("TPR_DRO_MNG_BALL_DYN",
			BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY(pid_global), BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS);

	bm_entry_print("TPR_DRW_MNG_BALL_DYN",
			BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY(pid_global), BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS);

	sprintf(reg_name, "TPR_CTRS_B%d", bid);
	bm_entry_print(reg_name, BM_TPR_CTRS_BANK_TBL_ENTRY(bid, pid_local), BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS);
}

void bm_bank_registers_dump(int bank)
{
	char reg_name[50];

	if ((bank < BM_BANK_MIN) || (bank > BM_BANK_MAX)) {
		pr_err("Invalid bank id %d\n", bank);
		return;
	}

	pr_info("\n-------------- BM BANK %d registers dump -----------\n", bank);
	sprintf(reg_name, "MV_B%d_SYS_REC_D0_STATUS_REG", bank);
	bm_gl_reg_print(reg_name, BM_BANK_SYS_REC_D0_STATUS_REG(bank));
	sprintf(reg_name, "MV_B%d_SYS_REC_D1_STATUS_REG", bank);
	bm_gl_reg_print(reg_name, BM_BANK_SYS_REC_D1_STATUS_REG(bank));
	sprintf(reg_name, "MV_B%d_REQUEST_FIFOS_STATUS_REG", bank);
	bm_gl_reg_print(reg_name, BM_BANK_REQUEST_FIFOS_STATUS_REG(bank));

	if  (bank == 0) {
		/*QM pools */
		bm_gl_reg_print("BM_B0_RELEASE_WRAP_PPE_FIFOS_STATUS_REG",
					BM_B0_RELEASE_WRAP_PPE_FIFOS_STATUS_REG);
		bm_gl_reg_print("BM_B0_PAST_ALC_FIFOS_STATUS_REG",
					BM_B0_PAST_ALC_FIFOS_STATUS_REG);
	} else
		/* QP pools */
		bm_gl_reg_print("BM_BGP_PAST_ALC_FIFOS_FILL_STATUS_REG",
					BM_BGP_PAST_ALC_FIFOS_FILL_STATUS_REG);
}


void bm_pool_status_dump(int pool)
{
	int bid, pid_local, pid_global, pes_in_cache_line, pe_bytes, cache_line_bytes;

	unsigned int reg_val, entry_offset, cache_start, cache_end, cache_si, cache_so;
	unsigned int cache_fill_min, cache_fill_max, cache_rd, cache_wr, cache_vmid, cache_size;
	unsigned int delay_release, failed_alloce, release, alloce;
	unsigned int dram_rd, dram_wr, dram_start, dram_size, dram_ae, dram_af, dram_fill;
	unsigned int dpr_c_mng_entry[BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS];
	unsigned int tpr_c_mng_entry[BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS];
	unsigned int tpr_dro_mng_entry[BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS];
	unsigned int dpr_d_mng_entry[BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS];
	unsigned int drw_mng_ball_entry[BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS];
	unsigned int tpr_ctrs_entry[BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS];

	if (bm_pid_validation(pool)) {
		pr_err("Invalid pool id %d\n", pool);
		return;
	}

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);
	pid_global = bm_pid_to_global(pool);

	pr_info("\n---------------- BM pool %d (bank %d) -------------------", pool, bid);

	if  (bid == 0) { /* QM pools */
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		pr_info("pool status         : %s\n", (reg_val & BM_B0_POOL_CFG_ENABLE_MASK) ? "enable" : "disable");
		pr_info("quick init          : %s\n", (reg_val & BM_B0_POOL_CFG_QUICK_INIT_MASK) ? "yes" : "no");
		pr_info("total ae       [PEs]: %d\n",
						(reg_val & BM_B0_POOL_CFG_AE_THR_MASK) >> BM_B0_POOL_CFG_AE_THR_OFFS);
		pr_info("total aae      [PEs]: %d\n",
						(reg_val & BM_B0_POOL_CFG_AAE_THR_MASK) >> BM_B0_POOL_CFG_AAE_THR_OFFS);

		reg_val = bm_gl_reg_read(BM_B0_POOL_STATUS_REG(pid_local));

	} else { /* GP pools */
		reg_val = bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		pr_info("pool status         : %s\n", (reg_val & BM_BGP_POOL_CFG_ENABLE_MASK) ? "enable" : "disable");
		pr_info("quick init          : %s\n", (reg_val & BM_BGP_POOL_CFG_QUICK_INIT_MASK) ? "yes" : "no");
		pr_info("pairs mode          : %s\n", (reg_val & BM_BGP_POOL_CFG_IN_PAIRS_MASK) ? "enable" : "disable");
		pr_info("element size  [bits]: %d bits\n",
					(reg_val & BM_BGP_POOL_CFG_PE_SIZE_MASK) ? MV_32_BITS : MV_40_BITS);
		pr_info("total ae       [PEs]: %d\n",
						(reg_val & BM_BGP_POOL_CFG_AE_THR_MASK) >> BM_BGP_POOL_CFG_AE_THR_OFFS);

		reg_val = bm_gl_reg_read(BM_BGP_POOL_STATUS_REG(bid, pid_local));
	}

	pes_in_cache_line = BM_PES_IN_CACHE_LINE(pool);
	pe_bytes = BM_CACHE_PE_BYTES(pool);
	cache_line_bytes = BM_CACHE_LINE_BYTES(pool);

	pr_info("dram almost empty   : %s\n", reg_val & BM_BGP_POOL_STATUS_AE_MASK ? "yes" : "no");
	pr_info("dram almost full    : %s\n", reg_val & BM_BGP_POOL_STATUS_AF_MASK ? "yes" : "no");

	/*---------------------------------------------------------------------*/

	entry_offset = BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS, dpr_c_mng_entry);


	cache_vmid = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_OFFS,
					BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_BITS, dpr_c_mng_entry);
	cache_start = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_START_OFFS,
					BM_DPR_C_MNG_BANK_STAT_CACHE_START_BITS, dpr_c_mng_entry);
	cache_end = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_END_OFFS,
					BM_DPR_C_MNG_BANK_STAT_CACHE_END_BITS, dpr_c_mng_entry);
	cache_si = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_OFFS,
					BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_BITS, dpr_c_mng_entry);
	cache_so = mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_OFFS,
					BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_BITS, dpr_c_mng_entry);

	cache_size = (cache_end - cache_start + 1) * 64 / BM_CACHE_PE_BYTES(pool);


	pr_info("cache state         : %s, vmid = 0x%x\n",
		reg_val & BM_BGP_POOL_STATUS_NEMPTY_MASK ? "not empty" : "empty", cache_vmid);

	pr_info("cache limits  [line]: start = %d, end = %d\n",
		cache_start * 64 / cache_line_bytes,
		cache_end * 64 / cache_line_bytes);

	pr_info("cache size     [PEs]: size = %d\n", cache_size);

	pr_info("cache thresh   [PEs]: si = %d, so = %d\n", cache_si, cache_so);

	/*---------------------------------------------------------------------*/
	entry_offset = BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS, tpr_c_mng_entry);

	cache_fill_min = mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_OFFS,
					BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_BITS, tpr_c_mng_entry);
	cache_fill_max = mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_OFFS,
					BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_BITS, tpr_c_mng_entry);
	cache_wr = mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_OFFS,
					BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_BITS, tpr_c_mng_entry);
	cache_rd = mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_OFFS,
					BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_BITS, tpr_c_mng_entry);

	pr_info("cache fill     [PEs]: [%d - %d]\n",
			cache_fill_min * pes_in_cache_line,
			cache_fill_max * pes_in_cache_line);

	pr_info("cache line    [line]: read = %d, write = %d\n", cache_rd, cache_wr);

	/*---------------------------------------------------------------------*/
	entry_offset = BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS, tpr_dro_mng_entry);

	dram_rd = mv_field_get(BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_OFFS,
					BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_BITS, tpr_dro_mng_entry);
	dram_wr = mv_field_get(BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_OFFS,
					BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_BITS, tpr_dro_mng_entry);

	pr_info("dram pointer        : read = 0x%x, write = 0x%x\n", dram_rd, dram_wr);

	/*---------------------------------------------------------------------*/
	entry_offset = BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS, dpr_d_mng_entry);
	dram_start = mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_OFFS,
					BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_BITS, dpr_d_mng_entry);
	dram_size = mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_OFFS,
					BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_BITS, dpr_d_mng_entry);
	dram_ae = mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_OFFS,
					BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_BITS, dpr_d_mng_entry);
	dram_af = mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_OFFS,
					BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_BITS, dpr_d_mng_entry);

	/* fields in unit of 64 bytes,  16 = 64 / PE size in dram */
	pr_info("dram start pointer  : 0x%x\n", dram_start);
	pr_info("dram size      [PEs]: %d\n", dram_size * 16);
	pr_info("dram thresh    [PEs]: ae = %d, af = %d\n", dram_ae * 16, dram_af * 16);

	/*---------------------------------------------------------------------*/
	entry_offset = BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS, drw_mng_ball_entry);

	dram_fill = mv_field_get(BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_OFFS,
		BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_BITS, drw_mng_ball_entry);

	pr_info("dram fill      [PEs]: %d\n", dram_fill * 8);
	/*---------------------------------------------------------------------*/
	entry_offset = BM_TPR_CTRS_BANK_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS, tpr_ctrs_entry);

	delay_release = mv_field_get(BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_OFFS,
					BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_BITS, tpr_ctrs_entry);
	release = mv_field_get(BM_TPR_CTRS_BANK_RELEASED_PES_CTR_OFFS,
					BM_TPR_CTRS_BANK_RELEASED_PES_CTR_BITS, tpr_ctrs_entry);
	failed_alloce = mv_field_get(BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_OFFS,
					BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_BITS, tpr_ctrs_entry);
	alloce = mv_field_get(BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_OFFS,
					BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_BITS, tpr_ctrs_entry);

	pr_info("delay releases [PEs]: 0x%08X\n", delay_release);
	pr_info("failed allocs  [PEs]: 0x%08X\n", failed_alloce);
	pr_info("released       [PEs]: 0x%08X\n", release);
	pr_info("allocated      [PEs]: 0x%08X\n", alloce);
	pr_info("\n");
}


void bm_pool_registers_parse(int pool)
{
	int bid, pid_local, pid_global;
	unsigned int reg_val, entry_offset;
	unsigned int dpr_c_mng_entry[BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS];
	unsigned int tpr_c_mng_entry[BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS];
	unsigned int tpr_dro_mng_entry[BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS];
	unsigned int dpr_d_mng_entry[BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS];
	unsigned int drw_mng_ball_entry[BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS];
	unsigned int tpr_ctrs_entry[BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS];

	if (bm_pid_validation(pool)) {
		pr_err("Invalid pool id %d\n", pool);
		return;
	}

	bid = bm_pid_to_bid(pool);
	pid_local = bm_pid_to_local(pool);
	pid_global = bm_pid_to_global(pool);

	pr_info("\nbm_pool_registers_dump\n\n");
	/*---------------------------------------------------------------------*/

	if  (bid == 0) {	/* QM pools */
		bm_gl_reg_print("BANK_POOL_CFG_REG", BM_B0_POOL_CFG_REG(pid_local));
		reg_val = bm_gl_reg_read(BM_B0_POOL_CFG_REG(pid_local));
		pr_info("  %-46s0x%08X\n", "enable",
				(reg_val & BM_B0_POOL_CFG_ENABLE_MASK) >> BM_B0_POOL_CFG_ENABLE_OFFS);
		pr_info("  %-46s0x%08X\n", "quick_init",
				(reg_val & BM_B0_POOL_CFG_QUICK_INIT_MASK) >> BM_B0_POOL_CFG_QUICK_INIT_OFFS);
		pr_info("  %-46s0x%08X\n", "ae_tot_thresh",
				(reg_val & BM_B0_POOL_CFG_AE_THR_MASK) >> BM_B0_POOL_CFG_AE_THR_OFFS);
		pr_info("  %-46s0x%08X\n", "aae_tot_thresh",
				(reg_val & BM_B0_POOL_CFG_AAE_THR_MASK) >> BM_B0_POOL_CFG_AAE_THR_OFFS);
		pr_info("\n");
		bm_gl_reg_print("BANK_POOL_STATUS_REG", BM_B0_POOL_CFG_REG(pid_local));
		reg_val = bm_gl_reg_read(BM_B0_POOL_STATUS_REG(pid_local));
	} else {
		bm_gl_reg_print("BANK_POOL_CFG_REG", BM_BGP_POOL_CFG_REG(bid, pid_local));
		reg_val = bm_gl_reg_read(BM_BGP_POOL_CFG_REG(bid, pid_local));
		pr_info("  %-46s0x%08X\n", "enable",
				(reg_val & BM_BGP_POOL_CFG_ENABLE_MASK) >> BM_BGP_POOL_CFG_ENABLE_OFFS);
		pr_info("  %-46s0x%08X\n", "quick_init",
				(reg_val & BM_BGP_POOL_CFG_QUICK_INIT_MASK) >> BM_BGP_POOL_CFG_QUICK_INIT_OFFS);
		pr_info("  %-46s0x%08X\n", "ae_tot_thresh",
				(reg_val & BM_BGP_POOL_CFG_AE_THR_MASK) >> BM_BGP_POOL_CFG_AE_THR_OFFS);
		pr_info("  %-46s0x%08X\n", "pairs",
				(reg_val & BM_BGP_POOL_CFG_IN_PAIRS_MASK) >> BM_BGP_POOL_CFG_IN_PAIRS_OFFS);
		pr_info("  %-46s0x%08X\n", "pe_size",
				(reg_val & BM_BGP_POOL_CFG_PE_SIZE_MASK) >> BM_BGP_POOL_CFG_PE_SIZE_OFFS);
		pr_info("\n");
		bm_gl_reg_print("BANK_POOL_STATUS_REG",  BM_BGP_POOL_STATUS_REG(bid, pid_local));
		reg_val = bm_gl_reg_read(BM_BGP_POOL_STATUS_REG(bid, pid_local));
	}

	/*---------------------------------------------------------------------*/
	pr_info("  %-46s0x%08X\n", "nempty",
			(reg_val & BM_BGP_POOL_STATUS_NEMPTY_MASK) >> BM_BGP_POOL_STATUS_NEMPTY_OFFS);
	pr_info("  %-46s0x%08X\n", "ae",
			(reg_val & BM_BGP_POOL_STATUS_AE_MASK) >> BM_BGP_POOL_STATUS_AE_OFFS);
	pr_info("  %-46s0x%08X\n", "af",
			(reg_val & BM_BGP_POOL_STATUS_AF_MASK) >> BM_BGP_POOL_STATUS_AF_OFFS);
	pr_info("  %-46s0x%08X\n", "fill_bgt",
			(reg_val & BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_MASK) >> BM_BGP_POOL_STATUS_FILL_BGT_SI_THR_OFFS);

	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS, dpr_c_mng_entry);
	bm_entry_print("DPR_C_MNG_BANK_STAT", entry_offset, BM_DPR_C_MNG_BANK_STAT_TBL_ENTRY_WORDS);
	pr_info("  %-46s0x%08X\n", "cache_start",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_START_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_START_BITS, dpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_end",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_END_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_END_BITS, dpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_si_thr",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_SI_THR_BITS, dpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_so_thr",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_SO_THR_BITS, dpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_vmid",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_VMID_BITS, dpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_attr",
			mv_field_get(BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_OFFS,
				BM_DPR_C_MNG_BANK_STAT_CACHE_ATTR_BITS, dpr_c_mng_entry));
	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS, tpr_c_mng_entry);
	bm_entry_print("TPR_C_MNG_BANK_DYN", entry_offset, BM_TPR_C_MNG_BANK_DYN_TBL_ENTRY_WORDS);

	pr_info("  %-46s0x%08X\n", "cache_fill_min",
			mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_OFFS,
				BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MIN_BITS, tpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_fill_max",
			mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_OFFS,
				BM_TPR_C_MNG_BANK_DYN_CACHE_FILL_MAX_BITS, tpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_wr_ptr",
			mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_OFFS,
				BM_TPR_C_MNG_BANK_DYN_CACHE_WR_PTR_BITS, tpr_c_mng_entry));
	pr_info("  %-46s0x%08X\n", "cache_rd_ptr",
			mv_field_get(BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_OFFS,
				BM_TPR_C_MNG_BANK_DYN_CACHE_RD_PTR_BITS, tpr_c_mng_entry));
	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS, dpr_d_mng_entry);
	bm_entry_print("DPR_D_MNG_BALL_STAT_TBL", entry_offset, BM_DPR_D_MNG_BALL_STAT_TBL_ENTRY_WORDS);

	pr_info("  %-46s0x%08X\n", "dram_ae_thr",
			mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_OFFS,
				BM_DPR_D_MNG_BALL_STAT_DRAM_AE_THR_BITS, dpr_d_mng_entry));
	pr_info("  %-46s0x%08X\n", "dram_af_thr",
			mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_OFFS,
				BM_DPR_D_MNG_BALL_STAT_DRAM_AF_THR_BITS, dpr_d_mng_entry));
	pr_info("  %-46s0x%08X\n", "dram_start_lsb",
			mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_OFFS,
				BM_DPR_D_MNG_BALL_STAT_DRAM_START_LSB_BITS, dpr_d_mng_entry));
	pr_info("  %-46s0x%08X\n", "dram_start_msb",
			mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_OFFS,
				BM_DPR_D_MNG_BALL_STAT_DRAM_START_MSB_BITS, dpr_d_mng_entry));
	pr_info("  %-46s0x%08X\n", "dram_size",
			mv_field_get(BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_OFFS,
				BM_DPR_D_MNG_BALL_STAT_DRAM_SIZE_BITS, dpr_d_mng_entry));
	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS, tpr_dro_mng_entry);
	bm_entry_print("TPR_DRO_MNG_BALL_DYN", entry_offset, BM_TPR_DRO_MNG_BALL_DYN_TBL_ENTRY_WORDS);
	pr_info("  %-46s0x%08X\n", "dram_rd_ptr",
			mv_field_get(BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_OFFS,
				BM_TPR_DRO_MNG_BALL_DYN_DRAM_RD_PTR_BITS, tpr_dro_mng_entry));
	pr_info("  %-46s0x%08X\n", "dram_wr_ptr",
			mv_field_get(BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_OFFS,
				BM_TPR_DRO_MNG_BALL_DYN_DRAM_WR_PTR_BITS, tpr_dro_mng_entry));
	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY(pid_global);
	bm_entry_read(entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS, drw_mng_ball_entry);
	bm_entry_print("TPR_DRW_MNG_BALL_DYN", entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS);

	pr_info("  %-46s0x%08X\n", "dram_fill",
			mv_field_get(BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_OFFS,
				BM_TPR_DRW_MNG_BALL_DYN_DRAM_FILL_BITS, drw_mng_ball_entry));
	/*---------------------------------------------------------------------*/
	pr_info("\n");
	entry_offset = BM_TPR_CTRS_BANK_TBL_ENTRY(bid, pid_local);
	bm_entry_read(entry_offset, BM_TPR_CTRS_BANK_TBL_ENTRY_WORDS, tpr_ctrs_entry);
	bm_entry_print("TPR_DRW_MNG_BALL_DYN", entry_offset, BM_TPR_DRW_MNG_BALL_DYN_TBL_ENTRY_WORDS);

	pr_info("  %-46s0x%08X\n", "delayed_releases_ctr",
			mv_field_get(BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_OFFS,
				BM_TPR_CTRS_BANK_DELAYED_RELEASES_CTR_BITS, tpr_ctrs_entry));
	pr_info("  %-46s0x%08X\n", "failed_allocs_ctr",
			mv_field_get(BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_OFFS,
				BM_TPR_CTRS_BANK_FAILED_ALLOCS_CTR_BITS, tpr_ctrs_entry));
	pr_info("  %-46s0x%08X\n", "released_pes_ctr",
			mv_field_get(BM_TPR_CTRS_BANK_RELEASED_PES_CTR_OFFS,
				BM_TPR_CTRS_BANK_RELEASED_PES_CTR_BITS, tpr_ctrs_entry));
	pr_info("  %-46s0x%08X\n", "allocated_pes_ctr",
			mv_field_get(BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_OFFS,
				BM_TPR_CTRS_BANK_ALLOCATED_PES_CTR_BITS, tpr_ctrs_entry));
}


static void bm_bank0_cache_memory_dump(void)
{
	unsigned int sram_cache_entry[BM_SRAM_B0_CACHE_TBL_ENTRY_WORDS];
	int line, entry_offset;

	pr_info("\n bank 0 cache memory dump:");

	for (line = 0; line < BM_QM_CACHE_LINES; line++) {
		entry_offset = BM_SRAM_B0_CACHE_TBL_ENTRY(line);
		bm_entry_read(entry_offset, BM_SRAM_B0_CACHE_TBL_ENTRY_WORDS, sram_cache_entry);
		pr_info("%4d: 0x%08x    0x%08x    0x%08x    0x%08x\n", line,
			sram_cache_entry[0], sram_cache_entry[1], sram_cache_entry[2], sram_cache_entry[3]);
	}
}

static void bm_bgp_cache_memory_dump(int bank)
{
	unsigned int sram_cache_entry[BM_SRAM_BGP_CACHE_TBL_ENTRY_WORDS];

	int line, entry_offset;

	pr_info("\n bank %d cache memory dump:", bank);

	for (line = 0; line < BM_GP_CACHE_LINES; line++) {

		entry_offset = BM_SRAM_BGP_CACHE_TBL_ENTRY(bank, line);
		bm_entry_read(entry_offset, BM_SRAM_BGP_CACHE_TBL_ENTRY_WORDS, sram_cache_entry);

		pr_info("%4d: 0x%08x    0x%08x\n", line, sram_cache_entry[0], sram_cache_entry[1]);
	}
}


void bm_bank_cache_dump(int bank)
{
	if ((bank < BM_BANK_MIN) || (bank > BM_BANK_MAX)) {
		pr_err("Invalid bank id %d\n", bank);
		return;
	}

	if (bank == 0)
		bm_bank0_cache_memory_dump();
	else
		bm_bgp_cache_memory_dump(bank);
}

void bm_idle_status_dump(void)
{
	char reg_name[50];
	int bid;

	bm_gl_reg_print("B0_PAST_ALC_FIFOS_STATUS", BM_B0_PAST_ALC_FIFOS_STATUS_REG);
	bm_gl_reg_print("B0_RELEASE_WRAP_PPE_FIFOS_STATUS", BM_B0_RELEASE_WRAP_PPE_FIFOS_STATUS_REG);

	for (bid = 0; bid < BM_BANK_NUM; bid++) {
		sprintf(reg_name, "B%d_REQUEST_FIFOS_STATUS", bid);
		bm_gl_reg_print(reg_name, BM_BANK_REQUEST_FIFOS_STATUS_REG(bid));
	}

	pr_info("\n for all banks:\n");
	bm_gl_reg_print("BGP_PAST_ALC_FIFOS_STATUS", BM_BGP_PAST_ALC_FIFOS_FILL_STATUS_REG);
	bm_gl_reg_print("BM_DM_AXI_FIFOS_STATUS", BM_DM_AXI_FIFOS_STATUS_REG);
	bm_gl_reg_print("BM_DRM_PENDING_FIFO_STATUS", BM_DRM_PENDING_FIFO_STATUS_REG);
	bm_gl_reg_print("BM_DM_AXI_WRITE_PENDING_FIFO_STATUS", BM_DM_AXI_WRITE_PENDING_FIFO_STATUS_REG);
}

void bm_error_dump(void)
{
	int bid;
	char reg_name[50];


	for (bid = 0; bid < BM_BANK_NUM; bid++) {
		pr_info("bank %d innterrupt regs:\n", bid);
		sprintf(reg_name, "B%d_SYS_REC_INTERRUPT_CAUSE", bid);
		bm_gl_reg_none_zr_print(reg_name, BM_BANK_SYS_REC_INTERRUPT_CAUSE_REG(bid));

		if (bid == 0) {
			bm_gl_reg_none_zr_print("B0_POOL_NEMPTY_INTERRUPT_CAUSE",
						BM_B0_POOL_NEMPTY_INTERRUPT_CAUSE_REG);
			bm_gl_reg_none_zr_print("B0_AE_INTERRUPT_CAUSE", BM_B0_AE_INTERRUPT_CAUSE_REG);
			bm_gl_reg_none_zr_print("B0_AF_INTERRUPT_CAUSE", BM_B0_AF_INTERRUPT_CAUSE_REG);

		} else {
			sprintf(reg_name, "B%d_POOL_NEMPTY_INTERRUPT_CAUSE", bid);
			bm_gl_reg_none_zr_print(reg_name, BM_BGP_POOL_NEMPTY_INTERRUPT_CAUSE_REG(bid));
			sprintf(reg_name, "B%d_AE_INTERRUPT_CAUSE", bid);
			bm_gl_reg_none_zr_print(reg_name, BM_BGP_AE_INTERRUPT_CAUSE_REG(bid));
			sprintf(reg_name, "B%d_AF_INTERRUPT_CAUSE", bid);
			bm_gl_reg_none_zr_print(reg_name, BM_BGP_AF_INTERRUPT_CAUSE_REG(bid));
		}
	}

	pr_info("global innterrupt regs:\n");

	bm_gl_reg_none_zr_print("SW_DBG_REC_INT_CAUSE", BM_SW_DBG_REC_INT_CAUSE_REG);
	bm_gl_reg_none_zr_print("ERR_INTERRUPT_CAUSE_REG", BM_ERR_INTERRUPT_CAUSE_REG);
	bm_gl_reg_none_zr_print("FUNC_INTERRUPT_CAUSE_REG", BM_FUNC_INTERRUPT_CAUSE_REG);
	bm_gl_reg_none_zr_print("ECC_ERR_INTERRUPT_CAUSE_REG", BM_ECC_ERR_INTERRUPT_CAUSE_REG);
}
