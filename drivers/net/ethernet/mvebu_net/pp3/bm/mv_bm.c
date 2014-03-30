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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

/* includes */
#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
/*
*/

#include "bm/mv_bm.h"
#include "bm/mv_qm.h"
#include "bm/mv_bm_regs.h"

int bm_open(void)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;

	rc = bm_reg_address_alias_init();
	if (rc != OK)
		return rc;
	rc = bm_reg_size_alias_init();
	if (rc != OK)
		return rc;
	rc = bm_reg_offset_alias_init();
	if (rc != OK)
		return rc;
	rc = bm_pid_bid_init();
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

/*BM User Application Interface*/
int bm_attr_all_pools_def_set(void)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 arDomain, awDomain, arCache, awCache, arQOS, awQOS;

	arDomain = 0;
	awDomain = 0;
	arCache  = 3;
	awCache  = 3;
	arQOS    = 1;
	awQOS    = 0;

	rc = bm_attr_qm_pool_set(arDomain, awDomain, arCache, awCache, arQOS, awQOS);
	if (rc != OK)
		return rc;
	rc = bm_attr_gp_pool_set(arDomain, awDomain, arCache, awCache, arQOS, awQOS);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_attr_qm_pool_set(u32 arDomain, u32 awDomain, u32 arCache, u32 awCache, u32 arQOS, u32 awQOS)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bm_req_rcv_en;
	struct bm_dram_domain_conf reg_dram_domain_conf;
	struct bm_dram_cache_conf  reg_dram_cache_conf;
	struct bm_dram_qos_conf    reg_dram_qos_conf;

	if ((arDomain < BM_ADOMAIN_MIN) || (arDomain > BM_ADOMAIN_MAX))
		return rc;
	if ((awDomain < BM_ADOMAIN_MIN) || (awDomain > BM_ADOMAIN_MAX))
		return rc;
	if ((arCache  <  BM_ACACHE_MIN) || (arCache  >  BM_ACACHE_MAX))
		return rc;
	if ((awCache  <  BM_ACACHE_MIN) || (awCache  >  BM_ACACHE_MAX))
		return rc;
	if ((arQOS    <    BM_AQOS_MIN) || (arQOS    >    BM_AQOS_MAX))
		return rc;
	if ((awQOS    <    BM_AQOS_MIN) || (awQOS    >    BM_AQOS_MAX))
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == 1) {
		rc = -BM_ATTR_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	reg_base_address =      bm.dram_domain_conf;
	reg_size   =   bm_reg_size.dram_domain_conf;
	reg_offset = bm_reg_offset.dram_domain_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_domain_conf);
	if (rc != OK)
		return rc;

	reg_dram_domain_conf.dwm_awdomain_b0 = awDomain;
	reg_dram_domain_conf.drm_ardomain_b0 = arDomain;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_domain_conf);
	if (rc != OK)
		return rc;

	reg_base_address =      bm.dram_cache_conf;
	reg_size   =   bm_reg_size.dram_cache_conf;
	reg_offset = bm_reg_offset.dram_cache_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_cache_conf);
	if (rc != OK)
		return rc;

	reg_dram_cache_conf.dwm_awcache_b0   = awCache;
	reg_dram_cache_conf.drm_arcache_b0   = arCache;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_cache_conf);
	if (rc != OK)
		return rc;

	reg_base_address =      bm.dram_qos_conf;
	reg_size   =   bm_reg_size.dram_qos_conf;
	reg_offset = bm_reg_offset.dram_qos_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_qos_conf);
	if (rc != OK)
		return rc;

	reg_dram_qos_conf.dwm_awqos_b0       = awQOS;
	reg_dram_qos_conf.drm_arqos_b0       = arQOS;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_qos_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_attr_gp_pool_set(u32 arDomain, u32 awDomain, u32 arCache, u32 awCache, u32 arQOS, u32 awQOS)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bm_req_rcv_en;
	struct bm_dram_domain_conf reg_dram_domain_conf;
	struct bm_dram_cache_conf  reg_dram_cache_conf;
	struct bm_dram_qos_conf    reg_dram_qos_conf;

	if ((arDomain < BM_ADOMAIN_MIN) || (arDomain > BM_ADOMAIN_MAX))
		return rc;
	if ((awDomain < BM_ADOMAIN_MIN) || (awDomain > BM_ADOMAIN_MAX))
		return rc;
	if ((arCache  <  BM_ACACHE_MIN) || (arCache  >  BM_ACACHE_MAX))
		return rc;
	if ((awCache  <  BM_ACACHE_MIN) || (awCache  >  BM_ACACHE_MAX))
		return rc;
	if ((arQOS    <    BM_AQOS_MIN) || (arQOS    >    BM_AQOS_MAX))
		return rc;
	if ((awQOS    <    BM_AQOS_MIN) || (awQOS    >    BM_AQOS_MAX))
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == 1) {
		rc = -BM_ATTR_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	reg_base_address =      bm.dram_domain_conf;
	reg_size   =   bm_reg_size.dram_domain_conf;
	reg_offset = bm_reg_offset.dram_domain_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_domain_conf);
	if (rc != OK)
		return rc;

	reg_dram_domain_conf.dwm_awdomain_bgp = awDomain;
	reg_dram_domain_conf.drm_ardomain_bgp = arDomain;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_domain_conf);
	if (rc != OK)
		return rc;

	reg_base_address =      bm.dram_cache_conf;
	reg_size   =   bm_reg_size.dram_cache_conf;
	reg_offset = bm_reg_offset.dram_cache_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_cache_conf);
	if (rc != OK)
		return rc;

	reg_dram_cache_conf.dwm_awcache_bgp   = awCache;
	reg_dram_cache_conf.drm_arcache_bgp   = arCache;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_cache_conf);
	if (rc != OK)
		return rc;

	reg_base_address =      bm.dram_qos_conf;
	reg_size   =   bm_reg_size.dram_qos_conf;
	reg_offset = bm_reg_offset.dram_qos_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_qos_conf);
	if (rc != OK)
		return rc;

	reg_dram_qos_conf.dwm_awqos_bgp       = awQOS;
	reg_dram_qos_conf.drm_arqos_bgp       = arQOS;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_qos_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_enable_status_get(u32 *bm_req_rcv_en)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	struct bm_common_general_conf          reg_common_general_conf;

	reg_base_address =      bm.common_general_conf;
	reg_size   =   bm_reg_size.common_general_conf;
	reg_offset = bm_reg_offset.common_general_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;

	*bm_req_rcv_en = reg_common_general_conf.bm_req_rcv_en;

	rc = OK;
	return rc;
}

int bm_qm_gpm_pools_def_quick_init(u32 num_of_buffers, u32 *qece_base_address, u32 *pl_base_address)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;

	ae_thr               = BM_AE_THR_DEF;
	af_thr               = BM_AF_THR_DEF;
	cache_vmid           = BM_CACHE_VMID_DEF;
	cache_attr           = BM_CACHE_ATTR_DEF;
	cache_si_thr         = BM_CACHE_SI_THR_QM_DEF;
	cache_so_thr         = BM_CACHE_SO_THR_QM_DEF;
	cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_QM_DEF;

	rc = bm_qm_gpm_pools_quick_init(num_of_buffers, qece_base_address, pl_base_address,
		ae_thr, af_thr,	cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_qm_dram_pools_def_quick_init(u32 num_of_buffers, u32 *qece_base_address,	u32 *pl_base_address)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;

	ae_thr               = BM_AE_THR_DEF;
	af_thr               = BM_AF_THR_DEF;
	cache_vmid           = BM_CACHE_VMID_DEF;
	cache_attr           = BM_CACHE_ATTR_DEF;
	cache_si_thr         = BM_CACHE_SI_THR_QM_DEF;
	cache_so_thr         = BM_CACHE_SO_THR_QM_DEF;
	cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_QM_DEF;

	rc = bm_qm_dram_pools_quick_init(num_of_buffers, qece_base_address,
				pl_base_address, ae_thr, af_thr,
				cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_qm_gpm_pools_quick_init(u32 num_of_buffers, u32 *qece_base_address,
						u32 *pl_base_address, u32 ae_thr, u32 af_thr,
						u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
						u32 cache_num_of_buffers)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pool, quick_init, pe_size, bm_req_rcv_en;
	struct mv_word40 base_address;
	u32 granularity_of_pe_in_dram, granularity_of_pe_in_cache;

	granularity_of_pe_in_dram  = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_DRAM;		/* 64/4 */
	granularity_of_pe_in_cache = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_CACHE;	/* 64/8 */

	if       ((num_of_buffers % granularity_of_pe_in_dram)  != 0)
		return rc;  /*qm PE are always 22bits which is 4Bytes */
	if               ((ae_thr % granularity_of_pe_in_dram)  != 0)
		return rc;
	if               ((af_thr % granularity_of_pe_in_dram)  != 0)
		return rc;
	if ((cache_num_of_buffers % granularity_of_pe_in_cache) != 0)
		return rc;
	if (ae_thr       >= af_thr)
		return rc;
	if (cache_so_thr >= cache_si_thr + 16)
		return rc;
	if ((num_of_buffers       < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers > BM_NUM_OF_BUFFERS_QM_GPM_MAX))
		return rc;
/*	if ((qece_base_address_hi <  BM_DRAM_ADDRESS_HI_MIN) || (qece_base_address_hi >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)qece_base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)qece_base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((qece_base_address_lo <  BM_DRAM_ADDRESS_LO_MIN) || (qece_base_address_lo >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)qece_base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)qece_base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
/*	if ((pl_base_address_hi   <  BM_DRAM_ADDRESS_HI_MIN) || (pl_base_address_hi   >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)pl_base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)pl_base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((pl_base_address_lo   <  BM_DRAM_ADDRESS_LO_MIN) || (pl_base_address_lo   >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)pl_base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)pl_base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
	if ((ae_thr               <           BM_AE_THR_MIN) || (ae_thr               >           BM_AE_THR_MAX))
		return rc;
	if ((af_thr               <           BM_AF_THR_MIN) || (af_thr               >           BM_AF_THR_MAX))
		return rc;
	if ((cache_vmid           <             BM_VMID_MIN) || (cache_vmid           >             BM_VMID_MAX))
		return rc;
	if ((cache_attr           <       BM_CACHE_ATTR_MIN) || (cache_attr           >       BM_CACHE_ATTR_MAX))
		return rc;
	if ((cache_so_thr         <     BM_CACHE_SO_THR_MIN) || (cache_so_thr         >     BM_CACHE_SO_THR_MAX))
		return rc;
	if ((cache_si_thr         <     BM_CACHE_SI_THR_MIN) || (cache_si_thr         >     BM_CACHE_SI_THR_MAX))
		return rc;
	if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_QM_MIN)	||
		(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_QM_MAX))
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;

	if (bm_req_rcv_en == 1) {
		rc = -BM_ATTR_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	pool = 0;
	base_address.hi = ((struct mv_word40 *)pl_base_address)->hi;
	base_address.lo = ((struct mv_word40 *)pl_base_address)->lo;
	quick_init = 1;
	pe_size = 1;
	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, (u32 *)&base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	rc = bm_pool_memory_fill(pool, num_of_buffers, (u32 *)&base_address);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;

	pool = 1;
	base_address.hi = ((struct mv_word40 *)qece_base_address)->hi;
	base_address.lo = ((struct mv_word40 *)qece_base_address)->lo;
	quick_init = 1;
	pe_size = 1;
	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, (u32 *)&base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	rc = bm_pool_memory_fill(pool, num_of_buffers, (u32 *)&base_address);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_qm_dram_pools_quick_init(u32 num_of_buffers, u32 *qece_base_address,
			u32 *pl_base_address, u32 ae_thr, u32 af_thr,
			u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
			u32 cache_num_of_buffers)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pool, base_address_allocate, quick_init, pe_size, bm_req_rcv_en, buffer_size;
	struct mv_word40 base_address;
	u32 granularity_of_pe_in_dram, granularity_of_pe_in_cache;

	granularity_of_pe_in_dram  = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_DRAM;		/* 64/4 */
	granularity_of_pe_in_cache = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_CACHE;	/* 64/4 */

	if       ((num_of_buffers % granularity_of_pe_in_dram)  != 0)
		return rc;  /*qm PE are always 22bits which is 4Bytes */
	if               ((ae_thr % granularity_of_pe_in_dram)  != 0)
		return rc;
	if               ((af_thr % granularity_of_pe_in_dram)  != 0)
		return rc;
	if ((cache_num_of_buffers % granularity_of_pe_in_cache) != 0)
		return rc;
	if (ae_thr       >= af_thr)
		return rc;
	if (cache_so_thr >= cache_si_thr + 16)
		return rc;

	if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers > BM_NUM_OF_BUFFERS_QM_DRAM_MAX))
		return rc;
/*	if ((qece_base_address_hi <  BM_DRAM_ADDRESS_HI_MIN) || (qece_base_address_hi >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)qece_base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)qece_base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((qece_base_address_lo <  BM_DRAM_ADDRESS_LO_MIN) || (qece_base_address_lo >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)qece_base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)qece_base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
/*	if ((pl_base_address_hi   <  BM_DRAM_ADDRESS_HI_MIN) || (pl_base_address_hi   >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)pl_base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)pl_base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((pl_base_address_lo   <  BM_DRAM_ADDRESS_LO_MIN) || (pl_base_address_lo   >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)pl_base_address)->hi < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)pl_base_address)->hi > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
	if ((ae_thr               <           BM_AE_THR_MIN) || (ae_thr               >           BM_AE_THR_MAX))
		return rc;
	if ((af_thr               <           BM_AF_THR_MIN) || (af_thr               >           BM_AF_THR_MAX))
		return rc;
	if ((cache_vmid           <             BM_VMID_MIN) || (cache_vmid           >             BM_VMID_MAX))
		return rc;
	if ((cache_attr           <       BM_CACHE_ATTR_MIN) || (cache_attr           >       BM_CACHE_ATTR_MAX))
		return rc;
	if ((cache_so_thr         <     BM_CACHE_SO_THR_MIN) || (cache_so_thr         >     BM_CACHE_SO_THR_MAX))
		return rc;
	if ((cache_si_thr         <     BM_CACHE_SI_THR_MIN) || (cache_si_thr         >     BM_CACHE_SI_THR_MAX))
		return rc;
	if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_QM_MIN)	||
		(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_QM_MAX))
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == 1) {
		rc = -BM_ATTR_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	pool = 2;
	base_address.hi = ((struct mv_word40 *)pl_base_address)->hi;
	base_address.lo = ((struct mv_word40 *)pl_base_address)->lo;
	quick_init = 1;
	pe_size = 1;
	buffer_size = BM_BUFFER_SIZE_P2;
	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, (u32 *)&base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	/*	for pools 2&3 it allocates the buffer memory before filling the pool	*/
	rc = ENOMEM;
	base_address_allocate = (u32)MV_MALLOC((num_of_buffers + 1) * buffer_size, GFP_KERNEL);
	if (base_address_allocate == (u32)NULL)
		return rc;
		/*base_address_hi need to be resolved - ???*/
	base_address.hi = 0;
	base_address.lo = base_address_allocate;
	rc = bm_pool_memory_fill(pool, num_of_buffers, (u32 *)&base_address);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;
	((struct mv_word40 *)pl_base_address)->hi = base_address.hi;
	((struct mv_word40 *)pl_base_address)->lo = base_address.lo;

	pool = 3;
	base_address.hi = ((struct mv_word40 *)qece_base_address)->hi;
	base_address.lo = ((struct mv_word40 *)qece_base_address)->lo;
	quick_init = 1;
	pe_size = 1;
	buffer_size = BM_BUFFER_SIZE_P3;
	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, (u32 *)&base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	/*	for pools 2&3 it allocates the buffer memory before filling the pool	*/
	rc = ENOMEM;
	base_address_allocate = (u32)MV_MALLOC((num_of_buffers + 1) * buffer_size, GFP_KERNEL);
	if (base_address_allocate == (u32)NULL)
		return rc;
		/*base_address_hi need to be resolved - ???*/
	base_address.hi = 0;
	base_address.lo = base_address_allocate;
	rc = bm_pool_memory_fill(pool, num_of_buffers, (u32 *)&base_address);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;
	((struct mv_word40 *)qece_base_address)->hi = base_address.hi;
	((struct mv_word40 *)qece_base_address)->lo = base_address.lo;

	rc = qm_pfe_base_address_pool_set(pl_base_address, qece_base_address);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_pool_quick_init_status_get(u32 pool, u32 *completed)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_st   reg_pool_st;

	if ((pool           <     BM_POOL_MIN) || (pool           >     BM_POOL_MAX))
		return rc;
	if ((pool           >  BM_POOL_QM_MAX) && (pool           <  BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if (((u32)completed < BM_DATA_PTR_MIN) || ((u32)completed > BM_DATA_PTR_MAX))
		return rc;

	pid       = (int)pool;
	bid       = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =            bm.b_pool_n_st[bid];
	reg_size         =   bm_reg_size.b_pool_n_st[bid];
	reg_offset       = bm_reg_offset.b_pool_n_st[bid] * pid_local;

	rc = bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pool_st);
	if (rc != OK)
		return rc;
	*completed = reg_pool_st.pool_fill_bgt_si_thr_st;

	rc = OK;
	return rc;
}

int bm_gp_pool_def_basic_init(u32 pool, u32 num_of_buffers, u32 *base_address, u32 partition_model)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr,
			cache_so_thr, cache_si_thr, cache_num_of_buffers;

	pe_size              = BM_PE_SIZE_DEF;
	pool_pair            = BM_POOL_PAIR_GP_DEF;
	ae_thr               = BM_AE_THR_DEF;
	af_thr               = BM_AF_THR_DEF;
	cache_vmid           = BM_CACHE_VMID_DEF;
	cache_attr           = BM_CACHE_ATTR_DEF;

	if (partition_model == 0) {			/* large partition in cache */
		cache_si_thr         = BM_CACHE_SI_THR_GP_BIG_DEF;
		cache_so_thr         = BM_CACHE_SO_THR_GP_BIG_DEF;
		cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_GP_BIG_DEF;
	} else if (partition_model == 1) {	/* small partition in cache */
		cache_si_thr         = BM_CACHE_SI_THR_GP_SMALL_DEF;
		cache_so_thr         = BM_CACHE_SO_THR_GP_SMALL_DEF;
		cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_GP_SMALL_DEF;
	} else
		return rc;

	rc = bm_gp_pool_basic_init(pool, num_of_buffers, base_address, pe_size, pool_pair,
					ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr,
					cache_num_of_buffers);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_gp_pool_basic_init(u32 pool, u32 num_of_buffers, u32 *base_address,
				u32 pe_size, u32 pool_pair, u32 ae_thr, u32 af_thr,
				u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
				u32 cache_num_of_buffers)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 quick_init = 0;	/* quick_init is FALSE */
	u32 granularity_of_pe_in_dram, granularity_of_pe_in_cache;

	if (pe_size == BM_PE_SIZE_IS_40_BITS) {
		granularity_of_pe_in_dram  =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_40_BITS_IN_BYTES_IN_DRAM;	/* 64/8 */
		granularity_of_pe_in_cache =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_IN_BYTES_IN_CACHE;			/* 64/8 */
	} else if (pe_size == BM_PE_SIZE_IS_32_BITS) {
		granularity_of_pe_in_dram  =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_32_BITS_IN_BYTES_IN_DRAM;	/* 64/4 */
		granularity_of_pe_in_cache =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_IN_BYTES_IN_CACHE;			/* 64/8 */
	} else
		return rc;

	if       ((num_of_buffers %  granularity_of_pe_in_dram) != 0)
		return rc;
	if               ((ae_thr %  granularity_of_pe_in_dram) != 0)
		return rc;
	if               ((af_thr %  granularity_of_pe_in_dram) != 0)
		return rc;
	if ((((struct mv_word40 *)base_address)->lo %  granularity_of_pe_in_dram) != 0)
		return rc;
	if ((cache_num_of_buffers % granularity_of_pe_in_cache) != 0)
		return rc;
	if (ae_thr       >= af_thr)
		return rc;
	if (cache_so_thr >= cache_si_thr + 16)
		return rc;




	if ((pool                 <          BM_POOL_GP_MIN) || (pool                 >          BM_POOL_GP_MAX))
		return rc;
	if ((num_of_buffers       < BM_NUM_OF_BUFFERS_GP_MIN) || (num_of_buffers       > BM_NUM_OF_BUFFERS_GP_MAX))
		return rc;
/*	if ((base_address_hi      <  BM_DRAM_ADDRESS_HI_MIN) || (base_address_hi      >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((base_address_lo      <  BM_DRAM_ADDRESS_LO_MIN) || (base_address_lo      >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
	if ((pe_size              <          BM_PE_SIZE_MIN) || (pe_size              >          BM_PE_SIZE_MAX))
		return rc;
	if ((pool_pair            <        BM_POOL_PAIR_MIN) || (pool_pair            >        BM_POOL_PAIR_MAX))
		return rc;
	if ((ae_thr               <           BM_AE_THR_MIN) || (ae_thr               >           BM_AE_THR_MAX))
		return rc;
	if ((af_thr               <           BM_AF_THR_MIN) || (af_thr               >           BM_AF_THR_MAX))
		return rc;
	if ((cache_vmid           <       BM_CACHE_VMID_MIN) || (cache_vmid           >       BM_CACHE_VMID_MAX))
		return rc;
	if ((cache_attr           <       BM_CACHE_ATTR_MIN) || (cache_attr           >       BM_CACHE_ATTR_MAX))
		return rc;
	if ((cache_so_thr         <     BM_CACHE_SO_THR_MIN) || (cache_so_thr         >     BM_CACHE_SO_THR_MAX))
		return rc;
	if ((cache_si_thr         <     BM_CACHE_SI_THR_MIN) || (cache_si_thr         >     BM_CACHE_SI_THR_MAX))
		return rc;
	if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_GP_MIN)	||
		(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_GP_MAX))
		return rc;

	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	rc = bm_gp_pool_pe_size_set(pool, pe_size);
	if (rc != OK)
		return rc;
	rc = bm_gp_pool_pair_set(pool, pool_pair);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_enable(void)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	struct bm_common_general_conf          reg_common_general_conf;
	u32 bm_req_rcv_en;

	reg_base_address =      bm.common_general_conf;
	reg_size   =   bm_reg_size.common_general_conf;
	reg_offset = bm_reg_offset.common_general_conf * 0;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == 1) {
		rc = -BM_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;
	reg_common_general_conf.drm_si_decide_extra_fill = 0;
	reg_common_general_conf.bm_req_rcv_en = 1;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_disable(void)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	struct bm_common_general_conf          reg_common_general_conf;
	u32 pool;

	for (pool = BM_POOL_GP_MIN; pool <= BM_POOL_GP_MAX; pool++) {
		rc = bm_pool_disable(pool);
		if (rc != OK)
			return rc;
	}

	for (pool = BM_POOL_QM_MIN; pool <= BM_POOL_QM_MAX; pool++) {
		rc = bm_pool_disable(pool);
		if (rc != OK)
			return rc;
	}

	reg_base_address =      bm.common_general_conf;
	reg_size   =   bm_reg_size.common_general_conf;
	reg_offset = bm_reg_offset.common_general_conf * 0;

	rc = bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;
	reg_common_general_conf.bm_req_rcv_en = 1;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;

/*	free(base_address_lo);	- ???*/

	rc = OK;
	return rc;
}

int bm_pool_fill_level_get(u32 pool, u32 *fill_level)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_tpr_drw_mng_ball_dyn_data         tab_tpr_drw_mng_ball_dyn;

	if ((pool            <     BM_POOL_MIN) || (pool            >     BM_POOL_MAX))
		return rc;
	if ((pool            >  BM_POOL_QM_MAX) && (pool            <  BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if (((u32)fill_level < BM_DATA_PTR_MIN) || ((u32)fill_level > BM_DATA_PTR_MAX))
		return rc;

	pid       = (int)pool;
	bid       = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.tpr_drw_mng_ball_dyn;
	reg_size   =   bm_reg_size.tpr_drw_mng_ball_dyn;
	reg_offset = bm_reg_offset.tpr_drw_mng_ball_dyn * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_drw_mng_ball_dyn);
	if (rc != OK)
		return rc;
	*fill_level = tab_tpr_drw_mng_ball_dyn.dram_fill * UNIT_OF__8_BYTES;

	rc = OK;
	return rc;
}

int bm_vmid_set(u32 bm_vmid)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	struct bm_common_general_conf          reg_common_general_conf;
	u32 bm_req_rcv_en;

	if ((bm_vmid < BM_VMID_MIN) || (bm_vmid > BM_VMID_MAX))
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == 1) {
		rc = -BM_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	reg_base_address =      bm.common_general_conf;
	reg_size   =   bm_reg_size.common_general_conf;
	reg_offset = bm_reg_offset.common_general_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;
	reg_common_general_conf.dm_vmid = bm_vmid;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_gp_pool_def_quick_init(u32 pool, u32 num_of_buffers, u32 fill_level,
					u32 *base_address, u32 partition_model)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr,
					cache_so_thr, cache_si_thr, cache_num_of_buffers;
/*	u32 large_cache;*/

	pe_size              = BM_PE_SIZE_IS_32_BITS;
	pool_pair            = BM_POOL_PAIR_GP_DEF;
	ae_thr               = BM_AE_THR_DEF;
	af_thr               = BM_AF_THR_DEF;
	cache_vmid           = BM_CACHE_VMID_DEF;
	cache_attr           = BM_CACHE_ATTR_DEF;

/*	large_cache = !(partition_model);	???*/

	if (partition_model == 0) {			/* large partition in cache */
		cache_si_thr         = BM_CACHE_SI_THR_GP_BIG_DEF;
		cache_so_thr         = BM_CACHE_SO_THR_GP_BIG_DEF;
		cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_GP_BIG_DEF;
	} else if (partition_model == 1) {	/* small partition in cache */
		cache_si_thr         = BM_CACHE_SI_THR_GP_SMALL_DEF;
		cache_so_thr         = BM_CACHE_SO_THR_GP_SMALL_DEF;
		cache_num_of_buffers = BM_CACHE_NUM_OF_BUFFERS_GP_SMALL_DEF;
	} else
		return rc;

	rc = bm_gp_pool_quick_init(pool, num_of_buffers, fill_level, base_address,
					pe_size, pool_pair, ae_thr, af_thr,
					cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_gp_pool_quick_init(u32 pool, u32 num_of_buffers, u32 fill_level, u32 *base_address,
				u32 pe_size, u32 pool_pair, u32 ae_thr, u32 af_thr,
				u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
				u32 cache_num_of_buffers)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 quick_init, bm_req_rcv_en;
	u32 granularity_of_pe_in_dram, granularity_of_pe_in_cache;

	quick_init = 1;	/*quick_init is TRUE*/

	if (pe_size == BM_PE_SIZE_IS_40_BITS) {
		granularity_of_pe_in_dram  =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_40_BITS_IN_BYTES_IN_DRAM;	/* 64/8 */
		granularity_of_pe_in_cache =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_IN_BYTES_IN_CACHE;			/* 64/8 */
	} else if (pe_size == BM_PE_SIZE_IS_32_BITS) {
		granularity_of_pe_in_dram  =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_32_BITS_IN_BYTES_IN_DRAM;	/* 64/4 */
		granularity_of_pe_in_cache =
			GRANULARITY_OF_64_BYTES / GP_PE_SIZE_IN_BYTES_IN_CACHE;			/* 64/8 */
	} else
		return rc;

	if ((num_of_buffers % granularity_of_pe_in_dram) != 0)
		return rc;
	if         ((ae_thr % granularity_of_pe_in_dram) != 0)
		return rc;
	if         ((af_thr % granularity_of_pe_in_dram) != 0)
		return rc;
	if ((((struct mv_word40 *)base_address)->lo % granularity_of_pe_in_dram)  != 0)
		return rc;
	if ((cache_num_of_buffers % granularity_of_pe_in_cache) != 0)
		return rc;

	if (ae_thr       >= af_thr)
		return rc;
	if (cache_so_thr >= cache_si_thr + 16)
		return rc;

	if ((pool                 <          BM_POOL_GP_MIN) || (pool                 >          BM_POOL_GP_MAX))
		return rc;
	if ((num_of_buffers       < BM_NUM_OF_BUFFERS_GP_MIN) || (num_of_buffers      > BM_NUM_OF_BUFFERS_GP_MAX))
		return rc;
	if ((fill_level           <       BM_FILL_LEVEL_MIN) || (fill_level           >       BM_FILL_LEVEL_MAX))
		return rc;
/*	if ((base_address_hi      <  BM_DRAM_ADDRESS_HI_MIN) || (base_address_hi      >  BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((base_address_lo      <  BM_DRAM_ADDRESS_LO_MIN) || (base_address_lo      >  BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
	if ((pe_size              <          BM_PE_SIZE_MIN) || (pe_size              >          BM_PE_SIZE_MAX))
		return rc;
	if ((pool_pair            <        BM_POOL_PAIR_MIN) || (pool_pair            >        BM_POOL_PAIR_MAX))
		return rc;
	if ((ae_thr               <           BM_AE_THR_MIN) || (ae_thr               >           BM_AE_THR_MAX))
		return rc;
	if ((af_thr               <           BM_AF_THR_MIN) || (af_thr               >           BM_AF_THR_MAX))
		return rc;
	if ((cache_vmid           <             BM_VMID_MIN) || (cache_vmid           >             BM_VMID_MAX))
		return rc;
	if ((cache_attr           <       BM_CACHE_ATTR_MIN) || (cache_attr           >       BM_CACHE_ATTR_MAX))
		return rc;
	if ((cache_so_thr         <     BM_CACHE_SO_THR_MIN) || (cache_so_thr         >     BM_CACHE_SI_THR_MAX))
		return rc;
	if ((cache_si_thr         <     BM_CACHE_SI_THR_MIN) || (cache_si_thr         >     BM_CACHE_SI_THR_MAX))
		return rc;
	if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_GP_MIN)	||
		(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_GP_MAX))
		return rc;

	rc = bm_pool_dram_set(pool, num_of_buffers, pe_size, base_address, ae_thr, af_thr);
	if (rc != OK)
		return rc;
	rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
	if (rc != OK)
		return rc;
	rc = bm_gp_pool_pe_size_set(pool, pe_size);
	if (rc != OK)
		return rc;
	rc = bm_gp_pool_pair_set(pool, pool_pair);
	if (rc != OK)
		return rc;
	rc = bm_pool_enable(pool, quick_init);
	if (rc != OK)
		return rc;

	rc = bm_enable_status_get(&bm_req_rcv_en);
	if (rc != OK)
		return rc;
	if (bm_req_rcv_en == ON) {
		rc = -BM_ATTR_CHANGE_AFTER_BM_ENABLE;
		return rc;
	}

	rc = OK;
	return rc;
}

/*BM Debug functions*/
int bm_global_registers_dump(void)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	char reg_name[50];

	struct bm_sys_nrec_common_d0_st  reg_sys_nrec_common_d0_st;
	struct bm_sys_nrec_common_d1_st  reg_sys_nrec_common_d1_st;
	struct bm_sys_nrec_common_d2_st  reg_sys_nrec_common_d2_st;
	struct bm_sys_nrec_common_d3_st  reg_sys_nrec_common_d3_st;
	struct bm_common_general_conf    reg_common_general_conf;
	struct bm_dram_domain_conf       reg_dram_domain_conf;
	struct bm_dram_cache_conf        reg_dram_cache_conf;
	struct bm_dram_qos_conf          reg_dram_qos_conf;
	struct bm_dm_axi_fifos_st        reg_dm_axi_fifos_st;
	struct bm_drm_pend_fifo_st       reg_drm_pend_fifo_st;
	struct bm_dm_axi_wr_pend_fifo_st reg_dm_axi_wr_pend_fifo_st;
	struct bm_bm_idle_st             reg_bm_idle_st;

	reg_base_address =      bm.sys_nrec_common_d0_st;
	reg_size   =   bm_reg_size.sys_nrec_common_d0_st;
	reg_offset = bm_reg_offset.sys_nrec_common_d0_st * 0;

	pr_info("\n-------------- BM Global registers dump -----------");

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sys_nrec_common_d0_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_sys_nrec_common_d0_st));

	pr_info("\t sys_nrec_common_d0_st.qm_last_alc_viol_pid_st  = 0x%08X\n",
				reg_sys_nrec_common_d0_st.qm_last_alc_viol_pid_st);
	pr_info("\t sys_nrec_common_d0_st.qm_last_rls_viol_pid_st  = 0x%08X\n",
				reg_sys_nrec_common_d0_st.qm_last_rls_viol_pid_st);
	pr_info("\t sys_nrec_common_d0_st.ppe_last_alc_viol_pid_st = 0x%08X\n",
				reg_sys_nrec_common_d0_st.ppe_last_alc_viol_pid_st);
	pr_info("\t sys_nrec_common_d0_st.ppe_last_rls_viol_pid_st = 0x%08X\n",
				reg_sys_nrec_common_d0_st.ppe_last_rls_viol_pid_st);

	reg_base_address =      bm.sys_nrec_common_d1_st;
	reg_size   =   bm_reg_size.sys_nrec_common_d1_st;
	reg_offset = bm_reg_offset.sys_nrec_common_d1_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sys_nrec_common_d1_st);
	if (rc != OK)
		return rc;
	pr_info("\n");

	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_sys_nrec_common_d1_st));

	pr_info("\t sys_nrec_common_d1_st.mac_last_alc_viol_pid_st = 0x%08X\n",
				reg_sys_nrec_common_d1_st.mac_last_alc_viol_pid_st);
	pr_info("\t sys_nrec_common_d1_st.mac_last_rls_viol_pid_st = 0x%08X\n",
				reg_sys_nrec_common_d1_st.mac_last_rls_viol_pid_st);
	pr_info("\t sys_nrec_common_d1_st.drm_last_dram_err_pid_st = 0x%08X\n",
				reg_sys_nrec_common_d1_st.drm_last_dram_err_pid_st);
	pr_info("\t sys_nrec_common_d1_st.dwm_last_fail_so_pid_st  = 0x%08X\n",
				reg_sys_nrec_common_d1_st.dwm_last_fail_so_pid_st);

	reg_base_address =      bm.sys_nrec_common_d2_st;
	reg_size   =   bm_reg_size.sys_nrec_common_d2_st;
	reg_offset = bm_reg_offset.sys_nrec_common_d2_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sys_nrec_common_d2_st);
	if (rc != OK)
		return rc;
	pr_info("\n");

	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_sys_nrec_common_d2_st));

	pr_info("\t sys_nrec_common_d2_st.qm_last_alc_viol_blen_st  = 0x%08X\n",
				reg_sys_nrec_common_d2_st.qm_last_alc_viol_blen_st);
	pr_info("\t sys_nrec_common_d2_st.qm_last_rls_viol_blen_st  = 0x%08X\n",
				reg_sys_nrec_common_d2_st.qm_last_rls_viol_blen_st);
	pr_info("\t sys_nrec_common_d2_st.ppe_last_alc_viol_blen_st = 0x%08X\n",
				reg_sys_nrec_common_d2_st.ppe_last_alc_viol_blen_st);
	pr_info("\t sys_nrec_common_d2_st.ppe_last_rls_viol_blen_st = 0x%08X\n",
				reg_sys_nrec_common_d2_st.ppe_last_rls_viol_blen_st);

	reg_base_address =      bm.sys_nrec_common_d3_st;
	reg_size   =   bm_reg_size.sys_nrec_common_d3_st;
	reg_offset = bm_reg_offset.sys_nrec_common_d3_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sys_nrec_common_d3_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_sys_nrec_common_d3_st));

	pr_info("\t sys_nrec_common_d3_st.mac_last_alc_viol_blen_st = 0x%08X\n",
				reg_sys_nrec_common_d3_st.mac_last_alc_viol_blen_st);
	pr_info("\t sys_nrec_common_d3_st.mac_last_rls_viol_blen_st = 0x%08X\n",
				reg_sys_nrec_common_d3_st.mac_last_rls_viol_blen_st);

	reg_base_address =      bm.common_general_conf;
	reg_size   =   bm_reg_size.common_general_conf;
	reg_offset = bm_reg_offset.common_general_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_common_general_conf);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_common_general_conf));

	pr_info("\t common_general_conf.mac_last_alc_viol_pid_st = 0x%08X\n",
				reg_common_general_conf.drm_si_decide_extra_fill);
	pr_info("\t common_general_conf.mac_last_rls_viol_pid_st = 0x%08X\n",
				reg_common_general_conf.dm_vmid);
	pr_info("\t common_general_conf.drm_last_dram_err_pid_st = 0x%08X\n",
				reg_common_general_conf.bm_req_rcv_en);

	reg_base_address =      bm.dram_domain_conf;
	reg_size   =   bm_reg_size.dram_domain_conf;
	reg_offset = bm_reg_offset.dram_domain_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_domain_conf);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_dram_domain_conf));

	pr_info("\t dram_domain_conf.dwm_awdomain_b0  = 0x%08X\n", reg_dram_domain_conf.dwm_awdomain_b0);
	pr_info("\t dram_domain_conf.dwm_awdomain_bgp = 0x%08X\n", reg_dram_domain_conf.dwm_awdomain_bgp);
	pr_info("\t dram_domain_conf.drm_ardomain_b0  = 0x%08X\n", reg_dram_domain_conf.drm_ardomain_b0);
	pr_info("\t dram_domain_conf.drm_ardomain_bgp = 0x%08X\n", reg_dram_domain_conf.drm_ardomain_bgp);

	reg_base_address =      bm.dram_cache_conf;
	reg_size   =   bm_reg_size.dram_cache_conf;
	reg_offset = bm_reg_offset.dram_cache_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_cache_conf);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset,
		*((u32 *)&reg_dram_cache_conf));

	pr_info("\t dram_cache_conf.dwm_awcache_b0  = 0x%08X\n", reg_dram_cache_conf.dwm_awcache_b0);
	pr_info("\t dram_cache_conf.dwm_awcache_bgp = 0x%08X\n", reg_dram_cache_conf.dwm_awcache_bgp);
	pr_info("\t dram_cache_conf.drm_arcache_b0  = 0x%08X\n", reg_dram_cache_conf.drm_arcache_b0);
	pr_info("\t dram_cache_conf.drm_arcache_bgp = 0x%08X\n", reg_dram_cache_conf.drm_arcache_bgp);

	reg_base_address =      bm.dram_qos_conf;
	reg_size   =   bm_reg_size.dram_qos_conf;
	reg_offset = bm_reg_offset.dram_qos_conf * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dram_qos_conf);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_dram_qos_conf));

	pr_info("\t dram_qos_conf.dwm_awqos_b0  = 0x%08X\n", reg_dram_qos_conf.dwm_awqos_b0);
	pr_info("\t dram_qos_conf.dwm_awqos_bgp = 0x%08X\n", reg_dram_qos_conf.dwm_awqos_bgp);
	pr_info("\t dram_qos_conf.drm_arqos_b0  = 0x%08X\n", reg_dram_qos_conf.drm_arqos_b0);
	pr_info("\t dram_qos_conf.drm_arqos_bgp = 0x%08X\n", reg_dram_qos_conf.drm_arqos_bgp);

	reg_base_address =      bm.dm_axi_fifos_st;
	reg_size   =   bm_reg_size.dm_axi_fifos_st;
	reg_offset = bm_reg_offset.dm_axi_fifos_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dm_axi_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_dm_axi_fifos_st));

	pr_info("\t dm_axi_fifos_st.dwm_waddr_fifo_fill_st = 0x%08X\n",
				reg_dm_axi_fifos_st.dwm_waddr_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.dwm_wdata_fifo_fill_st = 0x%08X\n",
				reg_dm_axi_fifos_st.dwm_wdata_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.drm_raddr_fifo_fill_st = 0x%08X\n",
				reg_dm_axi_fifos_st.drm_raddr_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.drm_rdata_fifo_fill_st = 0x%08X\n",
				reg_dm_axi_fifos_st.drm_rdata_fifo_fill_st);

	reg_base_address =      bm.drm_pend_fifo_st;
	reg_size   =   bm_reg_size.drm_pend_fifo_st;
	reg_offset = bm_reg_offset.drm_pend_fifo_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_drm_pend_fifo_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_drm_pend_fifo_st));

	pr_info("\t drm_pend_fifo_st.drm_pend_fifo_fill_st = 0x%08X\n",
						reg_drm_pend_fifo_st.drm_pend_fifo_fill_st);

	reg_base_address =      bm.dm_axi_wr_pend_fifo_st;
	reg_size   =   bm_reg_size.dm_axi_wr_pend_fifo_st;
	reg_offset = bm_reg_offset.dm_axi_wr_pend_fifo_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dm_axi_wr_pend_fifo_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_dm_axi_wr_pend_fifo_st));

	pr_info("\t dm_axi_wr_pend_fifo_st.axi_wr_pend_fifo_fill_st = 0x%08X\n",
						reg_dm_axi_wr_pend_fifo_st.axi_wr_pend_fifo_fill_st);

	reg_base_address =      bm.bm_idle_st;
	reg_size   =   bm_reg_size.bm_idle_st;
	reg_offset = bm_reg_offset.bm_idle_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bm_idle_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_bm_idle_st));

	pr_info("\t bm_idle_st.bm_idle_st = 0x%08X\n", reg_bm_idle_st.bm_idle_st);

	rc = OK;
	return rc;
}

int bm_pool_registers_dump(u32 pool)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	char reg_name[50];

	struct bm_pool_conf_b0              reg_pool_conf_b0;
	struct bm_pool_conf_bgp             reg_pool_conf_bgp;
	struct bm_pool_st                   reg_pool_st;
	struct bm_c_mng_stat_data           tab_dpr_c_mng_stat;
	struct bm_c_mng_dyn_data            tab_tpr_c_mng_dyn;
	struct bm_d_mng_ball_stat_data      tab_dpr_d_mng_ball_stat_data;
	struct bm_tpr_dro_mng_ball_dyn_data tab_tpr_dro_mng_ball_dyn_data;
	struct bm_tpr_drw_mng_ball_dyn_data tab_tpr_drw_mng_ball_dyn_data;
	struct bm_tpr_ctrs_0_data           tab_tpr_ctrs_0_data;

	if ((pool            <     BM_POOL_MIN) || (pool            >    BM_POOL_MAX))
		return rc;
	if ((pool            >  BM_POOL_QM_MAX) && (pool            < BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */

	pid       = (int)pool;
	bid       = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	pr_info("\n bm_pool_registers_dump:");

	reg_base_address =      bm.b_pool_n_conf[bid];
	reg_size   =   bm_reg_size.b_pool_n_conf[bid];
	reg_offset = bm_reg_offset.b_pool_n_conf[bid] * pid_local;

	if  (bid == 0) {	/* QM pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pool_conf_b0);
		if (rc != OK)
			return rc;

		pr_info("\n");
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_pool_conf_b0));

		pr_info("\t b%d_pool_%d_conf.pool_enable     = 0x%08X\n",
					bid, pid_local, reg_pool_conf_b0.pool_enable);
		pr_info("\t b%d_pool_%d_conf.pool_quick_init = 0x%08X\n",
					bid, pid_local, reg_pool_conf_b0.pool_quick_init);
	} else if ((bid == 1) || (bid == 2) || (bid == 3) || (bid == 4)) {	/* QP pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pool_conf_bgp);
		if (rc != OK)
			return rc;

		pr_info("\n");
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_pool_conf_bgp));

		pr_info("\t b%d_pool_%d_conf.pool_enable     = 0x%08X\n",
					bid, pid_local, reg_pool_conf_bgp.pool_enable);
		pr_info("\t b%d_pool_%d_conf.pool_in_pairs   = 0x%08X\n",
					bid, pid_local, reg_pool_conf_bgp.pool_in_pairs);
		pr_info("\t b%d_pool_%d_conf.PE_size         = 0x%08X\n",
					bid, pid_local, reg_pool_conf_bgp.pe_size);
		pr_info("\t b%d_pool_%d_conf.pool_quick_init = 0x%08X\n",
					bid, pid_local, reg_pool_conf_bgp.pool_quick_init);
	} else {
		rc = -BM_INPUT_NOT_IN_RANGE;
		return rc;
	}

	reg_base_address =      bm.b_pool_n_st[bid];
	reg_size   =   bm_reg_size.b_pool_n_st[bid];
	reg_offset = bm_reg_offset.b_pool_n_st[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pool_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_pool_st));

	pr_info("\t b%d_pool_%d_st.pool_nempty_st          = 0x%08X\n",
			bid, pid_local, reg_pool_st.pool_nempty_st);
	pr_info("\t b%d_pool_%d_st.dpool_ae_st             = 0x%08X\n",
			bid, pid_local, reg_pool_st.dpool_ae_st);
	pr_info("\t b%d_pool_%d_st.dpool_af_st             = 0x%08X\n",
			bid, pid_local, reg_pool_st.dpool_af_st);
	pr_info("\t b%d_pool_%d_st.pool_fill_bgt_si_thr_st = 0x%08X\n",
			bid, pid_local, reg_pool_st.pool_fill_bgt_si_thr_st);

	reg_base_address =      bm.dpr_c_mng_stat[bid];
	reg_size   =   bm_reg_size.dpr_c_mng_stat[bid];
	reg_offset = bm_reg_offset.dpr_c_mng_stat[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_c_mng_stat);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_dpr_c_mng_stat));

	pr_info("\t dpr_c_mng_b%d_stat.cache_start  = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_start);
	pr_info("\t dpr_c_mng_b%d_stat.cache_end    = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_end);
	pr_info("\t dpr_c_mng_b%d_stat.cache_si_thr = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_si_thr);
	pr_info("\t dpr_c_mng_b%d_stat.cache_so_thr = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_so_thr);
	pr_info("\t dpr_c_mng_b%d_stat.cache_attr   = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_attr);
	pr_info("\t dpr_c_mng_b%d_stat.cache_vmid   = 0x%08X\n", bid, tab_dpr_c_mng_stat.cache_vmid);

	reg_base_address =      bm.tpr_c_mng_b_dyn[bid];
	reg_size   =   bm_reg_size.tpr_c_mng_b_dyn[bid];
	reg_offset = bm_reg_offset.tpr_c_mng_b_dyn[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_c_mng_dyn);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_tpr_c_mng_dyn));

	pr_info("\t tpr_c_mng_b%d_dyn.cache_fill_min = 0x%08X\n", bid, tab_tpr_c_mng_dyn.cache_fill_min);
	pr_info("\t tpr_c_mng_b%d_dyn.cache_fill_max = 0x%08X\n", bid, tab_tpr_c_mng_dyn.cache_fill_max);
	pr_info("\t tpr_c_mng_b%d_dyn.cache_rd_ptr   = 0x%08X\n", bid, tab_tpr_c_mng_dyn.cache_rd_ptr);
	pr_info("\t tpr_c_mng_b%d_dyn.cache_wr_ptr   = 0x%08X\n", bid, tab_tpr_c_mng_dyn.cache_wr_ptr);

	reg_base_address =      bm.dpr_d_mng_ball_stat;
	reg_size   =   bm_reg_size.dpr_d_mng_ball_stat;
	reg_offset = bm_reg_offset.dpr_d_mng_ball_stat * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_d_mng_ball_stat_data);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_dpr_d_mng_ball_stat_data));

	pr_info("\t dpr_d_mng_ball_stat.dram_ae_thr = 0x%08X\n",     tab_dpr_d_mng_ball_stat_data.dram_ae_thr);
	pr_info("\t dpr_d_mng_ball_stat.dram_af_thr = 0x%08X\n",     tab_dpr_d_mng_ball_stat_data.dram_af_thr);
	pr_info("\t dpr_d_mng_ball_stat.dram_start  = 0x%02X%08X\n",
		tab_dpr_d_mng_ball_stat_data.dram_start_hi, tab_dpr_d_mng_ball_stat_data.dram_start_lo);
	pr_info("\t dpr_d_mng_ball_stat.dram_size   = 0x%08X\n",     tab_dpr_d_mng_ball_stat_data.dram_size);

	reg_base_address =      bm.tpr_dro_mng_ball_dyn;
	reg_size   =   bm_reg_size.tpr_dro_mng_ball_dyn;
	reg_offset = bm_reg_offset.tpr_dro_mng_ball_dyn * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_dro_mng_ball_dyn_data);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_tpr_dro_mng_ball_dyn_data));

	pr_info("\t tpr_dro_mng_ball_dyn.dram_rd_ptr = 0x%08X\n", tab_tpr_dro_mng_ball_dyn_data.dram_rd_ptr);
	pr_info("\t tpr_dro_mng_ball_dyn.dram_wr_ptr = 0x%08X\n", tab_tpr_dro_mng_ball_dyn_data.dram_wr_ptr);

	reg_base_address =      bm.tpr_drw_mng_ball_dyn;
	reg_size   =   bm_reg_size.tpr_drw_mng_ball_dyn;
	reg_offset = bm_reg_offset.tpr_drw_mng_ball_dyn * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_drw_mng_ball_dyn_data);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_tpr_drw_mng_ball_dyn_data));

	pr_info("\t tpr_drw_mng_ball_dyn.dram_fill = 0x%08X\n", tab_tpr_drw_mng_ball_dyn_data.dram_fill);

	reg_base_address =      bm.tpr_ctrs_0_b[bid];
	reg_size   =   bm_reg_size.tpr_ctrs_0_b[bid];
	reg_offset = bm_reg_offset.tpr_ctrs_0_b[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_ctrs_0_data);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&tab_tpr_ctrs_0_data));

	pr_info("\t tpr_ctrs_0_b%d.delayed_releases_ctr = 0x%08X\n",
		bid, tab_tpr_ctrs_0_data.delayed_releases_ctr);
	pr_info("\t tpr_ctrs_0_b%d.failed_allocs_ctr    = 0x%08X\n",
		bid, tab_tpr_ctrs_0_data.failed_allocs_ctr);
	pr_info("\t tpr_ctrs_0_b%d.released_pes_ctr     = 0x%08X\n",
		bid, tab_tpr_ctrs_0_data.released_pes_ctr);
	pr_info("\t tpr_ctrs_0_b%d.allocated_pes_ctr    = 0x%08X\n",
		bid, tab_tpr_ctrs_0_data.allocated_pes_ctr);

	rc = OK;
	return rc;
}

int bm_bank_registers_dump(u32 bank)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bid;
	char reg_name[50];

	struct bm_b_sys_rec_bank_d0_st       reg_b_sys_rec_bank_d0_st;
	struct bm_b_sys_rec_bank_d1_st       reg_b_sys_rec_bank_d1_st;
	struct bm_b_bank_req_fifos_st        reg_b_bank_req_fifos_st;
	struct bm_b0_past_alc_fifos_st       reg_b0_past_alc_fifos_st;
	struct bm_bgp_past_alc_fifos_st      reg_bgp_past_alc_fifos_st;
	struct bm_b0_rls_wrp_ppe_fifos_st    reg_b0_rls_wrp_ppe_fifos_st;

	if ((bank            <     BM_BANK_MIN) || (bank            >     BM_BANK_MAX))
		return rc;

	bid       = bank;

	pr_info("\n bm_bank_registers_dump:");

	reg_base_address =      bm.b_sys_rec_bank_d0_st[bid];
	reg_size   =   bm_reg_size.b_sys_rec_bank_d0_st[bid];
	reg_offset = bm_reg_offset.b_sys_rec_bank_d0_st[bid] * bid;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_sys_rec_bank_d0_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b_sys_rec_bank_d0_st));

	pr_info("\t b%d_sys_rec_bank_d0_st.b_last_vmid_mis_alc_vmid_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d0_st.b_last_vmid_mis_alc_vmid_st);
	pr_info("\t b%d_sys_rec_bank_d0_st.b_last_vmid_mis_rls_vmid_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d0_st.b_last_vmid_mis_rls_vmid_st);
	pr_info("\t b%d_sys_rec_bank_d0_st.b_last_vmid_mis_alc_pid_st  = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d0_st.b_last_vmid_mis_alc_pid_st);
	pr_info("\t b%d_sys_rec_bank_d0_st.b_last_vmid_mis_rls_pid_st  = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d0_st.b_last_vmid_mis_rls_pid_st);

	reg_base_address =      bm.b_sys_rec_bank_d1_st[bid];
	reg_size   =   bm_reg_size.b_sys_rec_bank_d1_st[bid];
	reg_offset = bm_reg_offset.b_sys_rec_bank_d1_st[bid] * bid;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_sys_rec_bank_d1_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b_sys_rec_bank_d1_st));

	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_vmid_mis_alc_src_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_vmid_mis_alc_src_st);
	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_vmid_mis_rls_src_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_vmid_mis_rls_src_st);
	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_alc_dis_pool_src_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_alc_dis_pool_src_st);
	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_rls_dis_pool_src_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_rls_dis_pool_src_st);
	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_alc_dis_pool_pid_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_alc_dis_pool_pid_st);
	pr_info("\t b%d_sys_rec_bank_d1_st.b_last_rls_dis_pool_pid_st = 0x%08X\n",
				bid, reg_b_sys_rec_bank_d1_st.b_last_rls_dis_pool_pid_st);

	reg_base_address =      bm.b_bank_req_fifos_st;
	reg_size   =   bm_reg_size.b_bank_req_fifos_st;
	reg_offset = bm_reg_offset.b_bank_req_fifos_st * bid;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_bank_req_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b_bank_req_fifos_st));

	pr_info("\t b%d_bank_req_fifos_st.b_alc_fifo_fill_st = 0x%08X\n",
				bid, reg_b_bank_req_fifos_st.b_alc_fifo_fill_st);
	pr_info("\t b%d_bank_req_fifos_st.b_rls_fifo_fill_st = 0x%08X\n",
				bid, reg_b_bank_req_fifos_st.b_rls_fifo_fill_st);
	pr_info("\t b%d_bank_req_fifos_st.b_so_fifo_fill_st: = 0x%08X\n",
				bid, reg_b_bank_req_fifos_st.b_so_fifo_fill_st);
	pr_info("\t b%d_bank_req_fifos_st.b_si_fifo_fill_st: = 0x%08X\n",
				bid, reg_b_bank_req_fifos_st.b_si_fifo_fill_st);

	if  (bid == 0) {	/* QM pools */
		reg_base_address =      bm.b0_past_alc_fifos_st;
		reg_size   =   bm_reg_size.b0_past_alc_fifos_st;
		reg_offset = bm_reg_offset.b0_past_alc_fifos_st * bid;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_past_alc_fifos_st);
		if (rc != OK)
			return rc;

		pr_info("\n");
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_b0_past_alc_fifos_st));

		pr_info("\t b0_past_alc_fifos_st.b0_past_alc_fifo_fill_st     = 0x%08X\n",
				reg_b0_past_alc_fifos_st.b0_past_alc_fifo_fill_st);
		pr_info("\t b0_past_alc_fifos_st.b0_past_alc_ppe_fifo_fill_st = 0x%08X\n",
				reg_b0_past_alc_fifos_st.b0_past_alc_ppe_fifo_fill_st);
	} else if ((bid == 1) || (bid == 2) || (bid == 3) || (bid == 4)) {	/* QP pools */
		reg_base_address =      bm.bgp_past_alc_fifos_st;
		reg_size   =   bm_reg_size.bgp_past_alc_fifos_st;
		reg_offset = bm_reg_offset.bgp_past_alc_fifos_st * bid;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_past_alc_fifos_st);
		if (rc != OK)
			return rc;

		pr_info("\n");
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_bgp_past_alc_fifos_st));

		pr_info("\t bgp_past_alc_fifos_st.b1_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b1_past_alc_fifo_fill_st);
		pr_info("\t bgp_past_alc_fifos_st.b2_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b2_past_alc_fifo_fill_st);
		pr_info("\t bgp_past_alc_fifos_st.b3_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b3_past_alc_fifo_fill_st);
		pr_info("\t bgp_past_alc_fifos_st.b4_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b4_past_alc_fifo_fill_st);
	} else {
		rc = -BM_INPUT_NOT_IN_RANGE;
		return rc;
	}

	reg_base_address =      bm.b0_rls_wrp_ppe_fifos_st;
	reg_size   =   bm_reg_size.b0_rls_wrp_ppe_fifos_st;
	reg_offset = bm_reg_offset.b0_rls_wrp_ppe_fifos_st * bid;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_rls_wrp_ppe_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b0_rls_wrp_ppe_fifos_st));

	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_0_fill_st = 0x%08X\n",
			reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_0_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_1_fill_st = 0x%08X\n",
			reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_1_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_2_fill_st = 0x%08X\n",
			reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_2_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_3_fill_st = 0x%08X\n",
			reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_3_fill_st);

	rc = OK;
	return rc;
}

int bm_cache_memory_dump(u32 bank)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bid, line;
	char reg_name[50];

	struct bm_cache_b0_mem_data     reg_cache_b0_data;
	struct bm_cache_bgp_data        reg_cache_bgp_data;

	if ((bank            <     BM_BANK_MIN) || (bank            >     BM_BANK_MAX))
		return rc;

	bid       = bank;

	pr_info("\n bm_cache_memory_dump:");

	if  (bid == 0) {	/* QM pools */
		for (line = 0; line < BM_NUM_OF_LINE_QM; line++) {
			reg_base_address =      bm.sram_b_cache[bid];
			reg_size   =   bm_reg_size.sram_b_cache[bid];
			reg_offset = bm_reg_offset.sram_b_cache[bid] * line;

			rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_cache_b0_data);
			if (rc != OK)
				return rc;

			pr_info("\n");
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
				reg_base_address + reg_offset, *((u32 *)&reg_cache_b0_data));

			pr_info("\t line %04d: sram_b%d_cache.cache_b0_data_0 = 0x%08X\n",
						line, bid, reg_cache_b0_data.cache_b0_data_0);
			pr_info("\t line %04d: sram_b%d_cache.cache_b0_data_1 = 0x%08X\n",
						line, bid, reg_cache_b0_data.cache_b0_data_1);
			pr_info("\t line %04d: sram_b%d_cache.cache_b0_data_2 = 0x%08X\n",
						line, bid, reg_cache_b0_data.cache_b0_data_2);
			pr_info("\t line %04d: sram_b%d_cache.cache_b0_data_3 = 0x%08X\n",
						line, bid, reg_cache_b0_data.cache_b0_data_3);
		}
	} else if ((bid == 1) || (bid == 2) || (bid == 3) || (bid == 4)) {	/* QP pools */
		pr_info("\n");

		for (line = 0; line < BM_NUM_OF_LINE_GP; line++) {
			reg_base_address =      bm.sram_b_cache[bid];
			reg_size   =   bm_reg_size.sram_b_cache[bid];
			reg_offset = bm_reg_offset.sram_b_cache[bid] * line;

			rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_cache_bgp_data);
			if (rc != OK)
				return rc;

			pr_info("\n");
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_cache_bgp_data));

			pr_info("\t line %04d: sram_b%d_cache.cache_bgp_data = 0x%02X_%08X\n",
				line, bid, reg_cache_bgp_data.cache_bgp_data_hi,
							reg_cache_bgp_data.cache_bgp_data_lo);
		}
	} else {
		rc = -BM_INPUT_NOT_IN_RANGE;
		return rc;
	}

	rc = OK;
	return rc;
}

int bm_idle_status_get(u32 *status)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	struct bm_bm_idle_st reg_bm_idle_st;

	if (((u32)status < BM_DATA_PTR_MIN) || ((u32)status > BM_DATA_PTR_MAX))
		return rc;

	reg_base_address =      bm.bm_idle_st;
	reg_size   =   bm_reg_size.bm_idle_st;
	reg_offset = bm_reg_offset.bm_idle_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size,  (u32 *)&reg_bm_idle_st);
/*	rc =  bm_register_read(reg_base_address, reg_offset, reg_size,  &status);  if (rc != 0) return rc; */
	if (rc != OK)
		return rc;
	*status = reg_bm_idle_st.bm_idle_st;

	rc = OK;
	return rc;
}

int bm_pool_status_get(u32 pool, u32 *pool_nempty, u32 *dpool_ae, u32 *dpool_af)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_st   reg_pool_st;

	if ((pool             <     BM_POOL_MIN) || (pool             >     BM_POOL_MAX))
		return rc;
	if ((pool             >  BM_POOL_QM_MAX) && (pool             <  BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if (((u32)pool_nempty < BM_DATA_PTR_MIN) || ((u32)pool_nempty > BM_DATA_PTR_MAX))
		return rc;
	if (((u32)dpool_ae    < BM_DATA_PTR_MIN) || ((u32)dpool_ae    > BM_DATA_PTR_MAX))
		return rc;
	if (((u32)dpool_af    < BM_DATA_PTR_MIN) || ((u32)dpool_af    > BM_DATA_PTR_MAX))
		return rc;

	pid = (int)pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.b_pool_n_st[bid];
	reg_size   =   bm_reg_size.b_pool_n_st[bid];
	reg_offset = bm_reg_offset.b_pool_n_st[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_pool_st);
	if (rc != OK)
		return rc;

	*pool_nempty = reg_pool_st.pool_nempty_st;
	*dpool_ae    = reg_pool_st.dpool_ae_st;
	*dpool_af    = reg_pool_st.dpool_af_st;

	rc = OK;
	return rc;
}

int bm_idle_debug(void)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bid;
	struct bm_b_bank_req_fifos_st          reg_b_bank_req_fifos_st;
	struct bm_b0_past_alc_fifos_st         reg_b0_past_alc_fifos_st;
	struct bm_bgp_past_alc_fifos_st        reg_bgp_past_alc_fifos_st;
	struct bm_b0_rls_wrp_ppe_fifos_st      reg_b0_rls_wrp_ppe_fifos_st;
	struct bm_dm_axi_fifos_st              reg_dm_axi_fifos_st;
	struct bm_drm_pend_fifo_st             reg_drm_pend_fifo_st;
	struct bm_dm_axi_wr_pend_fifo_st       reg_dm_axi_wr_pend_fifo_st;
	char reg_name[50];

	for (bid = 0; bid < BM_NUMBER_OF_BANKS; bid++) {
		reg_base_address =      bm.b_bank_req_fifos_st;
		reg_size   =   bm_reg_size.b_bank_req_fifos_st;
		reg_offset = bm_reg_offset.b_bank_req_fifos_st * bid;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_bank_req_fifos_st);
		if (rc != OK)
			return rc;

		pr_info("\n for bank = %d:\n", bid);
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_b_bank_req_fifos_st));

		pr_info("\t b%d_bank_req_fifos_st.b_alc_fifo_fill_st = 0x%08X\n",
					bid, reg_b_bank_req_fifos_st.b_alc_fifo_fill_st);
		pr_info("\t b%d_bank_req_fifos_st.b_rls_fifo_fill_st = 0x%08X\n",
					bid, reg_b_bank_req_fifos_st.b_rls_fifo_fill_st);
		pr_info("\t b%d_bank_req_fifos_st.b_so_fifo_fill_st  = 0x%08X\n",
					bid, reg_b_bank_req_fifos_st.b_so_fifo_fill_st);
		pr_info("\t b%d_bank_req_fifos_st.b_si_fifo_fill_st  = 0x%08X\n",
					bid, reg_b_bank_req_fifos_st.b_si_fifo_fill_st);
	}

	reg_base_address =      bm.b0_past_alc_fifos_st;
	reg_size   =   bm_reg_size.b0_past_alc_fifos_st;
	reg_offset = bm_reg_offset.b0_past_alc_fifos_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_past_alc_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n for bank 0:\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b0_past_alc_fifos_st));

	pr_info("\t b0_past_alc_fifos_st.b0_past_alc_fifo_fill_st     = 0x%08X\n",
				reg_b0_past_alc_fifos_st.b0_past_alc_fifo_fill_st);
	pr_info("\t b0_past_alc_fifos_st.b0_past_alc_ppe_fifo_fill_st = 0x%08X\n",
				reg_b0_past_alc_fifos_st.b0_past_alc_ppe_fifo_fill_st);

	reg_base_address =      bm.bgp_past_alc_fifos_st;
	reg_size   =   bm_reg_size.bgp_past_alc_fifos_st;
	reg_offset = bm_reg_offset.bgp_past_alc_fifos_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_past_alc_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n for banks 1,2,3,4:\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_bgp_past_alc_fifos_st));

	pr_info("\t bgp_past_alc_fifos_st.b1_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b1_past_alc_fifo_fill_st);
	pr_info("\t bgp_past_alc_fifos_st.b2_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b2_past_alc_fifo_fill_st);
	pr_info("\t bgp_past_alc_fifos_st.b3_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b3_past_alc_fifo_fill_st);
	pr_info("\t bgp_past_alc_fifos_st.b4_past_alc_fifo_fill_st = 0x%08X\n",
				reg_bgp_past_alc_fifos_st.b4_past_alc_fifo_fill_st);

	reg_base_address =      bm.b0_rls_wrp_ppe_fifos_st;
	reg_size   =   bm_reg_size.b0_rls_wrp_ppe_fifos_st;
	reg_offset = bm_reg_offset.b0_rls_wrp_ppe_fifos_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_rls_wrp_ppe_fifos_st);
	if (rc != OK)
		return rc;

	pr_info("\n for bank 0:\n");
	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_b0_rls_wrp_ppe_fifos_st));

	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_0_fill_st = 0x%08X\n",
					reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_0_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_1_fill_st = 0x%08X\n",
					reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_1_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_2_fill_st = 0x%08X\n",
					reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_2_fill_st);
	pr_info("\t b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_3_fill_st = 0x%08X\n",
					reg_b0_rls_wrp_ppe_fifos_st.rls_wrp_ppe_fifo_3_fill_st);

	pr_info("\n for all banks:\n");
	reg_base_address =      bm.dm_axi_fifos_st;
	reg_size   =   bm_reg_size.dm_axi_fifos_st;
	reg_offset = bm_reg_offset.dm_axi_fifos_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dm_axi_fifos_st);
	if (rc != OK)
		return rc;

	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_dm_axi_fifos_st));

	pr_info("\t dm_axi_fifos_st.dwm_waddr_fifo_fill_st = 0x%08X\n",
						reg_dm_axi_fifos_st.dwm_waddr_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.dwm_wdata_fifo_fill_st = 0x%08X\n",
						reg_dm_axi_fifos_st.dwm_wdata_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.drm_raddr_fifo_fill_st = 0x%08X\n",
						reg_dm_axi_fifos_st.drm_raddr_fifo_fill_st);
	pr_info("\t dm_axi_fifos_st.drm_rdata_fifo_fill_st = 0x%08X\n",
						reg_dm_axi_fifos_st.drm_rdata_fifo_fill_st);

	reg_base_address =      bm.drm_pend_fifo_st;
	reg_size   =   bm_reg_size.drm_pend_fifo_st;
	reg_offset = bm_reg_offset.drm_pend_fifo_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_drm_pend_fifo_st);
	if (rc != OK)
		return rc;

	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_drm_pend_fifo_st));

	pr_info("\t drm_pend_fifo_st.drm_pend_fifo_fill_st = 0x%08X\n",
					reg_drm_pend_fifo_st.drm_pend_fifo_fill_st);

	reg_base_address =      bm.dm_axi_wr_pend_fifo_st;
	reg_size   =   bm_reg_size.dm_axi_wr_pend_fifo_st;
	reg_offset = bm_reg_offset.dm_axi_wr_pend_fifo_st * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_dm_axi_wr_pend_fifo_st);
	if (rc != OK)
		return rc;

	rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
	if (rc != OK)
		return rc;
	pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
		reg_base_address + reg_offset, *((u32 *)&reg_dm_axi_wr_pend_fifo_st));

	pr_info("\t dm_axi_wr_pend_fifo_st.axi_wr_pend_fifo_fill_st = 0x%08X\n",
			reg_dm_axi_wr_pend_fifo_st.axi_wr_pend_fifo_fill_st);

	rc = OK;
	return rc;
}

/*Errors and interrupts handling  TBD*/
/*
1.	bm_inter_read(u32 group, u32 *dataPtr)
2.	bm_inter_clean(u32 group)
3.	bm_inter_mask_set(u32 group, u32 mask)
Note: it is recommended that interrupts mask will be set after BM is enabled and that the interrupted will be
							unmasked with correlation to the specific configuration
4.	bm_error_read(u32 group, u32 *dataPtr)
5.	bm_error_clean(u32 group)
6.	bm_error_mask_set (u32 group, u32 mask)
7.	List of registers that are used for interrupt handling. API will be defined later with the interrupt
							handling definition
a.	bn_sys_rec_bank_d0_st: PID/VMID of last VMID-miss event for alloc/release:
							Postponed till we handle interupts
b.	bn_sys_rec_bank_d1_st: PID/source client of last release/alloc request to disabled pool:
							Postponed till we handle interupts
c.	sys_nrec_common_d0_st: PID of last alloc/release. Postponed till we handle interupts
d.	sys_nrec_common_d1_st: PID of last DRAM read. Postponed till we handle interupts
e.	common_general_conf: field drm_si_decide_extra_fill. According to Koby this is currently not in used
*/

int bm_error_dump(void)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 bid, error_sum_bit;
	struct bm_b_sys_rec_bank_intr_cause     reg_b_sys_rec_bank_intr_cause;
	struct bm_sw_debug_rec_intr_cause       reg_sw_debug_rec_intr_cause;
	struct bm_sys_nrec_common_intr_cause    reg_sys_nrec_common_intr_cause;
	struct bm_error_intr_cause              reg_error_intr_cause;
	struct bm_func_intr_cause               reg_func_intr_cause;
	struct bm_ecc_err_intr_cause            reg_ecc_err_intr_cause;
	struct bm_b_pool_nempty_intr_cause      reg_b_pool_nempty_intr_cause;
	struct bm_b_dpool_ae_intr_cause         reg_b_dpool_ae_intr_cause;
	struct bm_b_dpool_af_intr_cause         reg_b_dpool_af_intr_cause;
	char reg_name[50];

	for (bid = 0; bid < BM_NUMBER_OF_BANKS; bid++) {
		reg_base_address =      bm.b_sys_rec_bank_intr_cause[bid];
		reg_size   =   bm_reg_size.b_sys_rec_bank_intr_cause[bid];
		reg_offset = bm_reg_offset.b_sys_rec_bank_intr_cause[bid] * 0;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_sys_rec_bank_intr_cause);
		if (rc != OK)
			return rc;

		error_sum_bit = reg_b_sys_rec_bank_intr_cause.sys_rec_bank_intr_cause_sum;
		if (error_sum_bit != OK) {
			pr_info("\n for bank = %d:\n", bid);
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
				reg_base_address + reg_offset, *((u32 *)&reg_b_sys_rec_bank_intr_cause));

			pr_info("\t b%d_sys_rec_bank_intr_cause.sys_rec_bank_intr_cause_sum = %d\n",
						bid, reg_b_sys_rec_bank_intr_cause.sys_rec_bank_intr_cause_sum);
			pr_info("\t b%d_sys_rec_bank_intr_cause.alc_vmid_mis_s = 0x%08X\n",
						bid, reg_b_sys_rec_bank_intr_cause.alc_vmid_mis_s);
			pr_info("\t b%d_sys_rec_bank_intr_cause.rls_vmid_mis_s = 0x%08X\n",
						bid, reg_b_sys_rec_bank_intr_cause.rls_vmid_mis_s);
			pr_info("\t b%d_sys_rec_bank_intr_cause.alc_dis_pool_s = 0x%08X\n",
						bid, reg_b_sys_rec_bank_intr_cause.alc_dis_pool_s);
			pr_info("\t b%d_sys_rec_bank_intr_cause.rls_dis_pool_s = 0x%08X\n",
						bid, reg_b_sys_rec_bank_intr_cause.rls_dis_pool_s);
		}
	}

	reg_base_address =      bm.sw_debug_rec_intr_cause;
	reg_size   =   bm_reg_size.sw_debug_rec_intr_cause;
	reg_offset = bm_reg_offset.sw_debug_rec_intr_cause * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sw_debug_rec_intr_cause);
	if (rc != OK)
		return rc;
	error_sum_bit = reg_sw_debug_rec_intr_cause.sw_debug_rec_intr_cause_sum;
	if (error_sum_bit != OK) {
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_sw_debug_rec_intr_cause));

		pr_info("\t reg_sw_debug_rec_intr_cause.sw_debug_rec_intr_cause_sum = %d\n",
					reg_sw_debug_rec_intr_cause.sw_debug_rec_intr_cause_sum);
		pr_info("\t reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_c_s = %d\n",
					reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_c_s);
		pr_info("\t reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_c_dyn_s = %d\n",
					reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_c_dyn_s);
		pr_info("\t reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_d_dro_s = %d\n",
					reg_sw_debug_rec_intr_cause.rams_ctl_sw_wr_d_dro_s);
		pr_info("\t reg_sw_debug_rec_intr_cause.qm_bm_rf_err_s = %d\n",
					reg_sw_debug_rec_intr_cause.qm_bm_rf_err_s);
	}

	reg_base_address =      bm.sys_nrec_common_intr_cause;
	reg_size   =   bm_reg_size.sys_nrec_common_intr_cause;
	reg_offset = bm_reg_offset.sys_nrec_common_intr_cause * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_sys_nrec_common_intr_cause);
	if (rc != OK)
		return rc;
	error_sum_bit = reg_sys_nrec_common_intr_cause.sys_nrec_common_intr_cause_sum;
	if (error_sum_bit != OK) {
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_sys_nrec_common_intr_cause));

		pr_info("\t reg_sys_nrec_common_intr_cause.sys_nrec_common_intr_cause_sum = %d\n",
					reg_sys_nrec_common_intr_cause.sys_nrec_common_intr_cause_sum);
		pr_info("\t reg_sys_nrec_common_intr_cause.qm_alc_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.qm_alc_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.qm_rls_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.qm_rls_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.ppe_alc_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.ppe_alc_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.ppe_rls_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.ppe_rls_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.ppe_alc_blen_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.ppe_alc_blen_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.ppe_rls_blen_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.ppe_rls_blen_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.mac_alc_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.mac_alc_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.mac_rls_pairs_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.mac_rls_pairs_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.mac_alc_pid_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.mac_alc_pid_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.mac_rls_pid_viol_s = %d\n",
					reg_sys_nrec_common_intr_cause.mac_rls_pid_viol_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.drm_dram_err_s = %d\n",
					reg_sys_nrec_common_intr_cause.drm_dram_err_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.dwm_dram_err_s = %d\n",
					reg_sys_nrec_common_intr_cause.dwm_dram_err_s);
		pr_info("\t reg_sys_nrec_common_intr_cause.dwm_fail_so_dram_fill_s = %d\n",
					reg_sys_nrec_common_intr_cause.dwm_fail_so_dram_fill_s);
	}

	reg_base_address =      bm.error_intr_cause;
	reg_size   =   bm_reg_size.error_intr_cause;
	reg_offset = bm_reg_offset.error_intr_cause * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_error_intr_cause);
	if (rc != OK)
		return rc;
	error_sum_bit = reg_error_intr_cause.qm_bm_err_intr_sum;
	if (error_sum_bit != OK) {
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_error_intr_cause));

		pr_info("\t reg_error_intr_cause.qm_bm_err_intr_sum = %d\n",
					reg_error_intr_cause.qm_bm_err_intr_sum);
		pr_info("\t reg_error_intr_cause.ecc_err_intr_s = %d\n",
					reg_error_intr_cause.ecc_err_intr_s);
		pr_info("\t reg_error_intr_cause.b0_sys_events_rec_bank_s = %d\n",
					reg_error_intr_cause.b0_sys_events_rec_bank_s);
		pr_info("\t reg_error_intr_cause.b0_sys_events_rec_bank_s = %d\n",
					reg_error_intr_cause.b0_sys_events_rec_bank_s);
		pr_info("\t reg_error_intr_cause.b2_sys_events_rec_bank_s = %d\n",
					reg_error_intr_cause.b1_sys_events_rec_bank_s);
		pr_info("\t reg_error_intr_cause.b3_sys_events_rec_bank_s = %d\n",
					reg_error_intr_cause.b3_sys_events_rec_bank_s);
		pr_info("\t reg_error_intr_cause.b4_sys_events_rec_bank_s = %d\n",
					reg_error_intr_cause.b4_sys_events_rec_bank_s);
		pr_info("\t reg_error_intr_cause.sw_debug_events_rec_s = %d\n",
					reg_error_intr_cause.sw_debug_events_rec_s);
		pr_info("\t reg_error_intr_cause.sys_events_nrec_common_s = %d\n",
					reg_error_intr_cause.sys_events_nrec_common_s);
	}

	reg_base_address =      bm.func_intr_cause;
	reg_size   =   bm_reg_size.func_intr_cause;
	reg_offset = bm_reg_offset.func_intr_cause * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_func_intr_cause);
	if (rc != OK)
		return rc;
	error_sum_bit = reg_func_intr_cause.qm_bm_func_intr_sum;
	if (error_sum_bit != OK) {
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_func_intr_cause));

		pr_info("\t reg_func_intr_cause.qm_bm_func_intr_sum = %d\n",
					reg_func_intr_cause.qm_bm_func_intr_sum);
		pr_info("\t reg_func_intr_cause.b0_pool_nempty_intr_s = %d\n",
					reg_func_intr_cause.b0_pool_nempty_intr_s);
		pr_info("\t reg_func_intr_cause.b0_dpool_ae_intr_s = %d\n",
					reg_func_intr_cause.b0_dpool_ae_intr_s);
		pr_info("\t reg_func_intr_cause.b0_dpool_af_intr_s = %d\n",
					reg_func_intr_cause.b0_dpool_af_intr_s);
		pr_info("\t reg_func_intr_cause.b1_pool_nempty_intr_s = %d\n",
					reg_func_intr_cause.b1_pool_nempty_intr_s);
		pr_info("\t reg_func_intr_cause.b1_dpool_ae_intr_s = %d\n",
					reg_func_intr_cause.b1_dpool_ae_intr_s);
		pr_info("\t reg_func_intr_cause.b1_dpool_af_intr_s = %d\n",
					reg_func_intr_cause.b1_dpool_af_intr_s);
		pr_info("\t reg_func_intr_cause.b2_pool_nempty_intr_s = %d\n",
					reg_func_intr_cause.b2_pool_nempty_intr_s);
		pr_info("\t reg_func_intr_cause.b2_dpool_ae_intr_s = %d\n",
					reg_func_intr_cause.b2_dpool_ae_intr_s);
		pr_info("\t reg_func_intr_cause.b2_dpool_af_intr_s = %d\n",
					reg_func_intr_cause.b2_dpool_af_intr_s);
		pr_info("\t reg_func_intr_cause.b3_pool_nempty_intr_s = %d\n",
					reg_func_intr_cause.b3_pool_nempty_intr_s);
		pr_info("\t reg_func_intr_cause.b3_dpool_ae_intr_s = %d\n",
					reg_func_intr_cause.b3_dpool_ae_intr_s);
		pr_info("\t reg_func_intr_cause.b3_dpool_af_intr_s = %d\n",
					reg_func_intr_cause.b3_dpool_af_intr_s);
		pr_info("\t reg_func_intr_cause.b4_pool_nempty_intr_s = %d\n",
					reg_func_intr_cause.b4_pool_nempty_intr_s);
		pr_info("\t reg_func_intr_cause.b4_dpool_ae_intr_s = %d\n",
					reg_func_intr_cause.b4_dpool_ae_intr_s);
		pr_info("\t reg_func_intr_cause.b4_dpool_af_intr_s = %d\n",
					reg_func_intr_cause.b4_dpool_af_intr_s);
	}

	reg_base_address =      bm.ecc_err_intr_cause;
	reg_size   =   bm_reg_size.ecc_err_intr_cause;
	reg_offset = bm_reg_offset.ecc_err_intr_cause * 0;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_ecc_err_intr_cause);
	if (rc != OK)
		return rc;
	error_sum_bit = reg_ecc_err_intr_cause.ecc_err_intr_sum;
	if (error_sum_bit != OK) {
		rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
		if (rc != OK)
			return rc;
		pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
			reg_base_address + reg_offset, *((u32 *)&reg_ecc_err_intr_cause));

		pr_info("\t reg_ecc_err_intr_cause.ecc_err_intr_sum = %d\n",
					reg_ecc_err_intr_cause.ecc_err_intr_sum);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b0_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b0_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b0_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b0_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b1_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b1_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b1_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b1_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b2_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b2_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b2_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b2_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b3_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b3_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b3_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b3_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b4_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b4_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_c_mng_b4_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_c_mng_b4_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_d_mng_ball_stat_ser_err_1_s = %d\n",
					reg_ecc_err_intr_cause.dpr_d_mng_ball_stat_ser_err_1_s);
		pr_info("\t reg_ecc_err_intr_cause.dpr_d_mng_ball_stat_ser_err_2_s = %d\n",
					reg_ecc_err_intr_cause.dpr_d_mng_ball_stat_ser_err_2_s);
		pr_info("\t reg_ecc_err_intr_cause.sram_b0_cache_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.sram_b0_cache_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.sram_b1_cache_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.sram_b1_cache_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.sram_b2_cache_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.sram_b2_cache_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.sram_b3_cache_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.sram_b3_cache_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.sram_b4_cache_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.sram_b4_cache_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_c_mng_b0_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_c_mng_b0_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_c_mng_b1_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_c_mng_b1_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_c_mng_b2_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_c_mng_b2_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_c_mng_b3_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_c_mng_b3_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_c_mng_b4_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_c_mng_b4_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_dro_mng_ball_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_dro_mng_ball_dyn_ser_err_s);
		pr_info("\t reg_ecc_err_intr_cause.tpr_drw_mng_ball_dyn_ser_err_s = %d\n",
					reg_ecc_err_intr_cause.tpr_drw_mng_ball_dyn_ser_err_s);
	}

	for (bid = 0; bid < BM_NUMBER_OF_BANKS; bid++) {
		reg_base_address =      bm.pool_nempty_intr_cause[bid];
		reg_size   =   bm_reg_size.pool_nempty_intr_cause[bid];
		reg_offset = bm_reg_offset.pool_nempty_intr_cause[bid] * 0;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_pool_nempty_intr_cause);
		if (rc != OK)
			return rc;
		error_sum_bit = reg_b_pool_nempty_intr_cause.b_pool_nempty_intr_sum;
		if (error_sum_bit != OK) {
			pr_info("\n for bank = %d:\n", bid);
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
				reg_base_address + reg_offset, *((u32 *)&reg_b_pool_nempty_intr_cause));

			pr_info("\t b%d_pool_nempty_intr_cause.b_pool_nempty_intr_sum = %d\n",
						bid, reg_b_pool_nempty_intr_cause.b_pool_nempty_intr_sum);
			pr_info("\t b%d_pool_nempty_intr_cause.b_pool_0_nempty_s = 0x%08X\n",
						bid, reg_b_pool_nempty_intr_cause.b_pool_0_nempty_s);
			pr_info("\t b%d_pool_nempty_intr_cause.b_pool_1_nempty_s = 0x%08X\n",
						bid, reg_b_pool_nempty_intr_cause.b_pool_1_nempty_s);
			pr_info("\t b%d_pool_nempty_intr_cause.b_pool_2_nempty_s = 0x%08X\n",
						bid, reg_b_pool_nempty_intr_cause.b_pool_2_nempty_s);
			pr_info("\t b%d_pool_nempty_intr_cause.b_pool_3_nempty_s = 0x%08X\n",
						bid, reg_b_pool_nempty_intr_cause.b_pool_3_nempty_s);
			if (bid != 0) {
				pr_info("\t b%d_pool_nempty_intr_cause.b_pool_4_nempty_s = 0x%08X\n",
							bid, reg_b_pool_nempty_intr_cause.b_pool_4_nempty_s);
				pr_info("\t b%d_pool_nempty_intr_cause.b_pool_5_nempty_s = 0x%08X\n",
							bid, reg_b_pool_nempty_intr_cause.b_pool_5_nempty_s);
				pr_info("\t b%d_pool_nempty_intr_cause.b_pool_6_nempty_s = 0x%08X\n",
							bid, reg_b_pool_nempty_intr_cause.b_pool_6_nempty_s);
			}
		}

		reg_base_address =      bm.dpool_ae_intr_cause[bid];
		reg_size   =   bm_reg_size.dpool_ae_intr_cause[bid];
		reg_offset = bm_reg_offset.dpool_ae_intr_cause[bid] * 0;

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_dpool_ae_intr_cause);
		if (rc != OK)
			return rc;
		error_sum_bit = reg_b_dpool_ae_intr_cause.b_dpool_ae_intr_sum;
		if (error_sum_bit != OK) {
			pr_info("\n for bank = %d:\n", bid);
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
				reg_base_address + reg_offset, *((u32 *)&reg_b_dpool_ae_intr_cause));

			pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_ae_intr_sum = %d\n",
						bid, reg_b_dpool_ae_intr_cause.b_dpool_ae_intr_sum);
			pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_0_ae_s = 0x%08X\n",
						bid, reg_b_dpool_ae_intr_cause.b_dpool_0_ae_s);
			pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_1_ae_s = 0x%08X\n",
						bid, reg_b_dpool_ae_intr_cause.b_dpool_1_ae_s);
			pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_2_ae_s = 0x%08X\n",
						bid, reg_b_dpool_ae_intr_cause.b_dpool_2_ae_s);
			pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_3_ae_s = 0x%08X\n",
						bid, reg_b_dpool_ae_intr_cause.b_dpool_3_ae_s);
			if (bid != 0) {
				pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_4_ae_s = 0x%08X\n",
							bid, reg_b_dpool_ae_intr_cause.b_dpool_4_ae_s);
				pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_5_ae_s = 0x%08X\n",
							bid, reg_b_dpool_ae_intr_cause.b_dpool_5_ae_s);
				pr_info("\t b%d_dpool_ae_intr_cause.b_dpool_6_ae_s = 0x%08X\n",
							bid, reg_b_dpool_ae_intr_cause.b_dpool_6_ae_s);
			}
		}

		reg_base_address =      bm.dpool_af_intr_cause[bid];
		reg_size   =   bm_reg_size.dpool_af_intr_cause[bid];
		reg_offset = bm_reg_offset.dpool_af_intr_cause[bid];

		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b_dpool_af_intr_cause);
		if (rc != OK)
			return rc;
		error_sum_bit = reg_b_dpool_af_intr_cause.b_dpool_af_intr_sum;
		if (error_sum_bit != OK) {
			pr_info("\n for bank = %d:\n", bid);
			rc =  bm_register_name_get(reg_base_address, reg_offset, reg_name);
			if (rc != OK)
				return rc;
			pr_info("  %-32s: 0x%x = 0x%08x\n", reg_name,
				reg_base_address + reg_offset, *((u32 *)&reg_b_dpool_af_intr_cause));

			pr_info("\t b%d_dpool_af_intr_cause.b_dpool_af_intr_sum = %d\n",
						bid, reg_b_dpool_af_intr_cause.b_dpool_af_intr_sum);
			pr_info("\t b%d_dpool_af_intr_cause.b_dpool_0_af_s = 0x%08X\n",
						bid, reg_b_dpool_af_intr_cause.b_dpool_0_af_s);
			pr_info("\t b%d_dpool_af_intr_cause.b_dpool_1_af_s = 0x%08X\n",
						bid, reg_b_dpool_af_intr_cause.b_dpool_1_af_s);
			pr_info("\t b%d_dpool_af_intr_cause.b_dpool_2_af_s = 0x%08X\n",
						bid, reg_b_dpool_af_intr_cause.b_dpool_2_af_s);
			pr_info("\t b%d_dpool_af_intr_cause.b_dpool_3_af_s = 0x%08X\n",
						bid, reg_b_dpool_af_intr_cause.b_dpool_3_af_s);
			if (bid != 0) {
				pr_info("\t b%d_dpool_af_intr_cause.b_dpool_4_af_s = 0x%08X\n",
							bid, reg_b_dpool_af_intr_cause.b_dpool_4_af_s);
				pr_info("\t b%d_dpool_af_intr_cause.b_dpool_5_af_s = 0x%08X\n",
							bid, reg_b_dpool_af_intr_cause.b_dpool_5_af_s);
				pr_info("\t b%d_dpool_af_intr_cause.b_dpool_6_af_s = 0x%08X\n",
							bid, reg_b_dpool_af_intr_cause.b_dpool_6_af_s);
			}
		}
	}

	rc = OK;
	return rc;
}

/*BM Internal functions*/
int bm_pool_memory_fill(u32 pool, u32 num_of_buffers, u32 *base_address)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 granularity_of_pe_in_dram;
	u32 i, *p;
/*
	u64 p_long;
	uint64_t p_long;
	uintptr_t p_long;
*/
	granularity_of_pe_in_dram = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_DRAM;	/* 64/4 */

	if ((num_of_buffers % granularity_of_pe_in_dram) != 0) /* UNIT_OF__1_BYTES = 1 */
		return rc; /* PE_size is 22 bit */
	if ((pool            <         BM_POOL_QM_MIN) || (pool            >         BM_POOL_QM_MAX))
		return rc;
	if ((pool == 0) || (pool == 1)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers >  BM_NUM_OF_BUFFERS_QM_GPM_MAX))
			return rc;
	}
	if ((pool == 2) || (pool == 3)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers > BM_NUM_OF_BUFFERS_QM_DRAM_MAX))
			return rc;
	}
/*	if ((base_address_hi < BM_DRAM_ADDRESS_HI_MIN) || (base_address_hi > BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((base_address_lo < BM_DRAM_ADDRESS_LO_MIN) || (base_address_lo > BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;

/*
	action
	Takes pool_base_address and use it as a pointer to fill all PE's with incrementing value (starting with 1)
	Write in Dram in BM pool section an incrementing index */

	p = (u32 *)(((struct mv_word40 *)base_address)->lo);
/*
	base_address->hi - ???
	p = (u32 *)((base_address->hi << 32) + base_address->lo);
	p = (u32 *)temp;
	p++;*/
	for (i = 0; i < num_of_buffers; i++)
		*p++ = i + 1;

	rc = OK;
	return rc;
}

int bm_pool_dram_set(u32 pool, u32 num_of_buffers, u32 pe_size, u32 *base_address,
						u32 ae_thr, u32 af_thr)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local, dram_size, dram_ae_thr, dram_af_thr;
	struct bm_d_mng_ball_stat_data          tab_dpr_d_mng_ball_stat;
	u32 granularity_of_pe_in_dram;

	if ((pool >= BM_POOL_QM_MIN) && (pool <= BM_POOL_QM_MAX)) { /* QM pools */
		granularity_of_pe_in_dram = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_DRAM;	/* 64/4 */
	} else if ((pool >= BM_POOL_GP_MIN) && (pool <= BM_POOL_GP_MAX)) { /* GP pools */
		if (pe_size == BM_PE_SIZE_IS_40_BITS)
			granularity_of_pe_in_dram =
				GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_40_BITS_IN_BYTES_IN_DRAM;	/* 64/8 */
		else if (pe_size == BM_PE_SIZE_IS_32_BITS)
			granularity_of_pe_in_dram =
				GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_32_BITS_IN_BYTES_IN_DRAM;	/* 64/4 */
		else
			return rc;
	} else
		return rc;

	if ((num_of_buffers % granularity_of_pe_in_dram) != 0)
		return rc;
	if         ((ae_thr % granularity_of_pe_in_dram) != 0)
		return rc;
	if         ((af_thr % granularity_of_pe_in_dram) != 0)
		return rc;
	if ((((struct mv_word40 *)base_address)->lo % GRANULARITY_OF_64_BYTES) != 0)
		return rc;

	if (ae_thr       >= af_thr)
		return rc;

	if ((pool            <            BM_POOL_MIN) || (pool            >            BM_POOL_MAX))
		return rc;
	if ((pool            >         BM_POOL_QM_MAX) && (pool            <         BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if ((pool == 0) || (pool == 1)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers >  BM_NUM_OF_BUFFERS_QM_GPM_MAX))
			return rc;
	} else if ((pool == 2) || (pool == 3)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers > BM_NUM_OF_BUFFERS_QM_DRAM_MAX))
			return rc;
	} else if ((pool >= BM_POOL_GP_MIN) && (pool <= BM_POOL_GP_MAX)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_GP_MIN) || (num_of_buffers >      BM_NUM_OF_BUFFERS_GP_MAX))
			return rc; /* pools 4, 5, 6, 7 don't exist */
	} else
		return rc;

	if ((pe_size         <         BM_PE_SIZE_MIN) || (pe_size         >         BM_PE_SIZE_MAX))
		return rc;

/*	if ((base_address_hi < BM_DRAM_ADDRESS_HI_MIN) || (base_address_hi > BM_DRAM_ADDRESS_HI_MAX)) */
	if ((((struct mv_word40 *)base_address)->hi < BM_DRAM_ADDRESS_HI_MIN) ||
		(((struct mv_word40 *)base_address)->hi > BM_DRAM_ADDRESS_HI_MAX))
		return rc;
/*	if ((base_address_lo < BM_DRAM_ADDRESS_LO_MIN) || (base_address_lo > BM_DRAM_ADDRESS_LO_MAX)) */
	if ((((struct mv_word40 *)base_address)->lo < BM_DRAM_ADDRESS_LO_MIN) ||
		(((struct mv_word40 *)base_address)->lo > BM_DRAM_ADDRESS_LO_MAX))
		return rc;
	if ((ae_thr          <          BM_AE_THR_MIN) || (ae_thr          >          BM_AE_THR_MAX))
		return rc;
	if ((af_thr          <          BM_AF_THR_MIN) || (af_thr          >          BM_AF_THR_MAX))
		return rc;

	pid       = (int)pool;
	bid       = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.dpr_d_mng_ball_stat;
	reg_size   =   bm_reg_size.dpr_d_mng_ball_stat;
	reg_offset = bm_reg_offset.dpr_d_mng_ball_stat * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_d_mng_ball_stat);
	if (rc != OK)
		return rc;

	tab_dpr_d_mng_ball_stat.dram_start_hi = ((struct mv_word40 *)base_address)->hi;
	tab_dpr_d_mng_ball_stat.dram_start_lo = ((struct mv_word40 *)base_address)->lo;
	if (bid == 0) {
		dram_ae_thr = BM_QM_PE_UNITS_TO_BYTES(ae_thr);
		dram_af_thr = BM_QM_PE_UNITS_TO_BYTES(af_thr);
		dram_size   = BM_QM_PE_UNITS_TO_BYTES(num_of_buffers);
	} else if (bid != 0) {
		dram_ae_thr = BM_GP_PE_UNITS_TO_BYTES(ae_thr,         pe_size);
		dram_af_thr = BM_GP_PE_UNITS_TO_BYTES(af_thr,         pe_size);
		dram_size   = BM_GP_PE_UNITS_TO_BYTES(num_of_buffers, pe_size);
	} else
		return rc;

	tab_dpr_d_mng_ball_stat.dram_ae_thr	=    ae_thr / UNIT_OF_64_BYTES;
	tab_dpr_d_mng_ball_stat.dram_af_thr	=    af_thr / UNIT_OF_64_BYTES;
	tab_dpr_d_mng_ball_stat.dram_size   = dram_size / UNIT_OF_64_BYTES;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_d_mng_ball_stat);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_pool_fill_level_set(u32 pool, u32 num_of_buffers, u32 pe_size, u32 quick_init)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local, dram_fill;
	struct bm_tpr_drw_mng_ball_dyn_data         tab_tpr_drw_mng_ball_dyn;
	u32 granularity_of_pe_in_dram;

	if ((pool >= BM_POOL_QM_MIN) && (pool <= BM_POOL_QM_MAX)) { /* QM pools */
		granularity_of_pe_in_dram = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_DRAM;	/* 64/4 */
	} else if ((pool >= BM_POOL_GP_MIN) && (pool <= BM_POOL_GP_MAX)) { /* GP pools */
		if (pe_size == BM_PE_SIZE_IS_40_BITS)
			granularity_of_pe_in_dram =
				GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_40_BITS_IN_BYTES_IN_DRAM;	/* 64/8 */
		else if (pe_size == BM_PE_SIZE_IS_32_BITS)
			granularity_of_pe_in_dram =
				GRANULARITY_OF_64_BYTES / GP_PE_SIZE_OF_32_BITS_IN_BYTES_IN_DRAM;	/* 64/4 */
		else
			return rc;
	} else
		return rc;

	if ((num_of_buffers % granularity_of_pe_in_dram) != 0)
		return rc;

	if ((pool           <       BM_POOL_MIN) || (pool           >            BM_POOL_MAX))
		return rc;
	if ((pool           >    BM_POOL_QM_MAX) && (pool           <         BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if ((pool == 0) || (pool == 1)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers >  BM_NUM_OF_BUFFERS_QM_GPM_MAX))
			return rc;
	} else if ((pool == 2) || (pool == 3)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_QM_MIN) || (num_of_buffers > BM_NUM_OF_BUFFERS_QM_DRAM_MAX))
			return rc;
	} else if ((pool >= BM_POOL_GP_MIN) && (pool <= BM_POOL_GP_MAX)) {
		if ((num_of_buffers < BM_NUM_OF_BUFFERS_GP_MIN) || (num_of_buffers >      BM_NUM_OF_BUFFERS_GP_MAX))
			return rc; /* pools 4, 5, 6, 7 don't exist */
	} else
		return rc;

	if ((pe_size        <    BM_PE_SIZE_MIN) || (pe_size        >         BM_PE_SIZE_MAX))
		return rc;
	if ((quick_init     < BM_QUICK_INIT_MIN) || (quick_init     >      BM_QUICK_INIT_MAX))
		return rc;

	pid       = (int)pool;
	bid       = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.tpr_drw_mng_ball_dyn;
	reg_size   =   bm_reg_size.tpr_drw_mng_ball_dyn;
	reg_offset = bm_reg_offset.tpr_drw_mng_ball_dyn * BM_PID_TO_GLOBAL_POOL_IDX(pid);

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_drw_mng_ball_dyn);
	if (rc != OK)
		return rc;

	if (bid == 0)
		dram_fill	= BM_QM_PE_UNITS_TO_BYTES(num_of_buffers);
	if (bid != 0)
		dram_fill	= BM_GP_PE_UNITS_TO_BYTES(num_of_buffers, pe_size);
	tab_tpr_drw_mng_ball_dyn.dram_fill	= dram_fill/UNIT_OF__8_BYTES;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&tab_tpr_drw_mng_ball_dyn);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_pool_enable(u32 pool, u32 quick_init)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_conf_b0  reg_b0_pool_conf;
	struct bm_pool_conf_bgp reg_bgp_pool_conf;

	if ((pool       <       BM_POOL_MIN) || (pool       >       BM_POOL_MAX))
		return rc;
	if ((pool       >    BM_POOL_QM_MAX) && (pool       <    BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if ((quick_init < BM_QUICK_INIT_MIN) || (quick_init > BM_QUICK_INIT_MAX))
		return rc;

	pid = (int)pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.b_pool_n_conf[bid];
	reg_size   =   bm_reg_size.b_pool_n_conf[bid];
	reg_offset = bm_reg_offset.b_pool_n_conf[bid] * pid_local;

	if  (bid == 0) { /* QM pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_pool_conf);
		if (rc != OK)
			return rc;

		reg_b0_pool_conf.pool_enable		= ON;
		reg_b0_pool_conf.pool_quick_init	= quick_init;
		rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_pool_conf);
		if (rc != OK)
			return rc;
	} else if ((bid == 1) || (bid == 2) || (bid == 3) || (bid == 4)) { /* QP pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
		if (rc != OK)
			return rc;

		reg_bgp_pool_conf.pool_enable		= ON;
		reg_bgp_pool_conf.pool_quick_init	= quick_init;
		rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
		if (rc != OK)
			return rc;
	} else {
		rc = -BM_INPUT_NOT_IN_RANGE;
		return rc;
	}

	rc = OK;
	return rc;
}

int bm_gp_pool_pe_size_set(u32 pool, u32 pe_size)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_conf_bgp reg_bgp_pool_conf;

	if ((pool    < BM_POOL_GP_MIN) || (pool    > BM_POOL_GP_MAX))
		return rc;
	if ((pe_size < BM_PE_SIZE_MIN) || (pe_size > BM_PE_SIZE_MAX))
		return rc;

	pid = (int)pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.b_pool_n_conf[bid];
	reg_size   =   bm_reg_size.b_pool_n_conf[bid];
	reg_offset = bm_reg_offset.b_pool_n_conf[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
	if (rc != OK)
		return rc;

	reg_bgp_pool_conf.pe_size	= pe_size;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_gp_pool_pair_set(u32 pool, u32 pool_pair)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_conf_bgp reg_bgp_pool_n_conf;

	if ((pool      <   BM_POOL_GP_MIN) || (pool      >   BM_POOL_GP_MAX))
		return rc;
	if ((pool_pair < BM_POOL_PAIR_MIN) || (pool_pair > BM_POOL_PAIR_MAX))
		return rc;

	pid = (int)pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.b_pool_n_conf[bid];
	reg_size   =   bm_reg_size.b_pool_n_conf[bid];
	reg_offset = bm_reg_offset.b_pool_n_conf[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_n_conf);
	if (rc != OK)
		return rc;

	reg_bgp_pool_n_conf.pool_in_pairs	= pool_pair;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_n_conf);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

/*int bm_pool_cache_set(u32 pool, u32 vmid, u32 attr, u32 so_thr, u32 si_thr, u32 end, u32 start)*/
int bm_pool_cache_set(u32 pool, u32 cache_vmid, u32 cache_attr, u32 cache_so_thr, u32 cache_si_thr,
						u32 cache_num_of_buffers)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local, pid_temp, bid_temp, pid_local_temp;
	u32 granularity_of_pe_in_cache, cache_start, cache_end, cache_end_max[BM_BANK_MAX], pool_enable;
	struct bm_pool_conf_b0    reg_b0_pool_conf;
	struct bm_pool_conf_bgp   reg_bgp_pool_conf;
	struct bm_c_mng_stat_data tab_dpr_c_mng_stat;

	if ((pool >= BM_POOL_QM_MIN) || (pool <= BM_POOL_QM_MAX)) /* QM pools */
		granularity_of_pe_in_cache = GRANULARITY_OF_64_BYTES / QM_PE_SIZE_IN_BYTES_IN_CACHE;	/* 64/4 */
	else if ((pool >= BM_POOL_GP_MIN) || (pool <= BM_POOL_GP_MAX)) /* GM pools */
		granularity_of_pe_in_cache = GRANULARITY_OF_64_BYTES / GP_PE_SIZE_IN_BYTES_IN_CACHE;	/* 64/8 */
	else
		return rc;

	if ((cache_num_of_buffers % granularity_of_pe_in_cache) != 0)
		return rc;
	if (cache_so_thr >= cache_si_thr + 16)
		return rc;

	if ((pool                 <          BM_POOL_MIN) || (pool                 >          BM_POOL_MAX))
		return rc;
	if ((pool                 >       BM_POOL_QM_MAX) && (pool                 <       BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */
	if ((cache_vmid           <          BM_VMID_MIN) || (cache_vmid           >          BM_VMID_MAX))
		return rc;
	if ((cache_attr           <    BM_CACHE_ATTR_MIN) || (cache_attr           >    BM_CACHE_ATTR_MAX))
		return rc;
	if ((cache_so_thr         <  BM_CACHE_SO_THR_MIN) || (cache_so_thr         >  BM_CACHE_SO_THR_MAX))
		return rc;
	if ((cache_si_thr         <  BM_CACHE_SI_THR_MIN) || (cache_si_thr         >  BM_CACHE_SI_THR_MAX))
		return rc;
	if ((pool >= BM_POOL_QM_MIN) || (pool <= BM_POOL_QM_MAX)) { /* QM pools */
		if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_QM_MIN) ||
			(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_QM_MAX))
			return rc;
	} else if ((pool >= BM_POOL_GP_MIN) || (pool <= BM_POOL_GP_MAX)) { /* GM pools */
		if ((cache_num_of_buffers < BM_CACHE_NUM_OF_BUFFERS_GP_MIN) ||
			(cache_num_of_buffers > BM_CACHE_NUM_OF_BUFFERS_GP_MAX))
			return rc;
	} else
		return rc;

	for (bid_temp = BM_BANK_MIN; bid_temp < BM_BANK_MAX; bid_temp++)
		cache_end_max[bid_temp] = 0;

	for (pid_temp = BM_POOL_MIN; pid_temp < BM_POOL_MAX; pid_temp++) {
		bid_temp = BM_PID_TO_BANK(pid_temp);
		pid_local_temp = BM_PID_TO_PID_LOCAL(pid_temp);

		if ((pid_temp > BM_POOL_QM_MAX) && (pid_temp < BM_POOL_GP_MIN))
			continue; /* pools 4, 5, 6, 7 don't exist */

		reg_base_address =      bm.b_pool_n_conf[bid_temp];
		reg_size   =   bm_reg_size.b_pool_n_conf[bid_temp];
		reg_offset = bm_reg_offset.b_pool_n_conf[bid_temp] * pid_local_temp;

		if  (bid_temp == 0) { /* QM pools */
			rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_pool_conf);
			if (rc != OK)
				return rc;
			pool_enable = reg_b0_pool_conf.pool_enable;
		} else if ((bid_temp == 1) || (bid_temp == 2) || (bid_temp == 3) || (bid_temp == 4)) { /* QP pools */
			rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
			if (rc != OK)
				return rc;
			pool_enable = reg_bgp_pool_conf.pool_enable;
		} else {
			rc = -BM_INPUT_NOT_IN_RANGE;
			return rc;
		}

		if (pool_enable == OFF)
			continue;	/* pool pid_local_temp is not enabled */

		reg_base_address =      bm.dpr_c_mng_stat[bid_temp];
		reg_size   =   bm_reg_size.dpr_c_mng_stat[bid_temp];
		reg_offset = bm_reg_offset.dpr_c_mng_stat[bid_temp] * pid_local_temp;

		rc = bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_c_mng_stat);
		if (rc != OK)
			return rc;
		cache_end_max[bid_temp] = MV_MAX(cache_end_max[bid_temp],
						tab_dpr_c_mng_stat.cache_end * UNIT_OF_64_BYTES);
	}

	pid = pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	if ((cache_end_max[bid]   < BM_CACHE_END_MIN) || (cache_end_max[bid]    >      BM_CACHE_END_MAX))
		return rc;

	reg_base_address =      bm.dpr_c_mng_stat[bid];
	reg_size   =   bm_reg_size.dpr_c_mng_stat[bid];
	reg_offset = bm_reg_offset.dpr_c_mng_stat[bid] * pid_local;

	rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_c_mng_stat);
	if (rc != OK)
		return rc;
	cache_start  = cache_end_max[bid] + 1;
	cache_end    = cache_start + (cache_num_of_buffers / granularity_of_pe_in_cache - 1);
	cache_end    = cache_start + (cache_num_of_buffers / granularity_of_pe_in_cache - 1);

	if ((cache_start <     BM_START_MIN) || (cache_start  >    BM_START_MAX))
		return rc;
	if ((cache_end   < BM_CACHE_END_MIN) || (cache_end    >    BM_CACHE_END_MAX))
		return rc;

	tab_dpr_c_mng_stat.cache_start  = cache_start / UNIT_OF_64_BYTES;
	tab_dpr_c_mng_stat.cache_end    = cache_end   / UNIT_OF_64_BYTES;
	tab_dpr_c_mng_stat.cache_si_thr = cache_si_thr;
	tab_dpr_c_mng_stat.cache_so_thr = cache_so_thr;
	tab_dpr_c_mng_stat.cache_attr   = cache_attr;
	tab_dpr_c_mng_stat.cache_vmid   = cache_vmid;
	rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&tab_dpr_c_mng_stat);
	if (rc != OK)
		return rc;

	rc = OK;
	return rc;
}

int bm_pool_disable(u32 pool)
{
	u32 reg_base_address, reg_size, reg_offset;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 pid, bid, pid_local;
	struct bm_pool_conf_b0  reg_b0_pool_conf;
	struct bm_pool_conf_bgp reg_bgp_pool_conf;

	if ((pool  <    BM_POOL_MIN) || (pool  >     BM_POOL_MAX))
		return rc;
	if ((pool  > BM_POOL_QM_MAX) && (pool  < BM_POOL_GP_MIN))
		return rc; /* pools 4, 5, 6, 7 don't exist */

	pid = (int)pool;
	bid = BM_PID_TO_BANK(pid);
	pid_local = BM_PID_TO_PID_LOCAL(pid);

	reg_base_address =      bm.b_pool_n_conf[bid];
	reg_size   =   bm_reg_size.b_pool_n_conf[bid];
	reg_offset = bm_reg_offset.b_pool_n_conf[bid] * pid_local;

	if  (bid == 0) { /* QM pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_pool_conf);
		if (rc != OK)
			return rc;
		reg_b0_pool_conf.pool_enable = OFF;
		rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_b0_pool_conf);
		if (rc != OK)
			return rc;
	} else if ((bid == 1) || (bid == 2) || (bid == 3) || (bid == 4)) { /* QP pools */
		rc =  bm_register_read(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
		if (rc != OK)
			return rc;

		reg_bgp_pool_conf.pool_enable = OFF;
		rc = bm_register_write(reg_base_address, reg_offset, reg_size, (u32 *)&reg_bgp_pool_conf);
		if (rc != OK)
			return rc;
	} else {
		rc = -BM_INPUT_NOT_IN_RANGE;
		return rc;
	}

	rc = OK;
	return rc;
}

#define	COMPLETE_HW_WRITE

#define	my_RW_DEBUG_UNITEST	/* for unitest */
#ifdef my_RW_DEBUG_UNITEST
int bm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;
/*	char reg_name[50];
*/
	u32 *temp;
	u32 i;

	if ((base_address <   BM_ADDRESS_MIN) || (base_address >   BM_ADDRESS_MAX))
		return rc;
	if ((offset       <    BM_OFFSET_MIN) || (offset       >    BM_OFFSET_MAX))
		return rc;
	if ((wordsNumber  < BM_DATA_SIZE_MIN) || (wordsNumber  > BM_DATA_SIZE_MAX))
		return rc;
	if (((u32)dataPtr <  BM_DATA_PTR_MIN) || ((u32)dataPtr >  BM_DATA_PTR_MAX))
		return rc;

/*	In the future we can also add printing of the fields of the register */
/*	pr_info(" DUMMY_PRINT  read by function <%s>,  result = 0x%08X\n", __func__, *(u32 *)dataPtr);

	bm_register_name_get(base_address, offset, reg_name);
	pr_info("[QM-BM]  READ_REG add = 0x%08X : name = %s : value =", base_address, reg_name);
*/
	temp = dataPtr;
	for (i = 0; i < wordsNumber; i++) {
		/*
		pr_info(" 0x%08X", *(u32 *)temp);*/
		*(u32 *)temp = 0;
		temp++;
	}
	pr_info("\n");
/*	return OK;	 */
/*
	rc = mv_pp3_hw_read(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK) {
		pr_info(" Not Available\n");
		return rc;
	}
*/
/*	if (rc != OK)
		return rc;*/

	COMPLETE_HW_WRITE
	rc = OK;
	return rc;
}

int bm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	char reg_name[50];
	int rc = -BM_INPUT_NOT_IN_RANGE;
	u32 *temp;
	u32 i;

	if ((base_address <   BM_ADDRESS_MIN) || (base_address >   BM_ADDRESS_MAX))
		return rc;
	if ((offset       <    BM_OFFSET_MIN) || (offset       >    BM_OFFSET_MAX))
		return rc;
	if ((wordsNumber  < BM_DATA_SIZE_MIN) || (wordsNumber  > BM_DATA_SIZE_MAX))
		return rc;
	if (((u32)dataPtr <  BM_DATA_PTR_MIN) || ((u32)dataPtr >  BM_DATA_PTR_MAX))
		return rc;

	bm_register_name_get(base_address, offset, reg_name);
	pr_info("[QM-BM] WROTE_REG add = 0x%08X : name = %s : value =", base_address, reg_name);
	temp = dataPtr;
	for (i = 0; i < wordsNumber; i++) {
		pr_info(" 0x%08X", *(u32 *)temp);
		temp++;
	}
	pr_info("\n");

/*	pr_info(" DUMMY_PRINT, result=%d\n", *(u32 *)dataPtr);*/
/*	pr_info(" DUMMY_PRINT, result = 0x%08X\n", *(u32 *)dataPtr);*/
/*	pr_info(" DUMMY_PRINT write by function <%s>, result = 0x%08X\n", __func__, *(u32 *)dataPtr);*/

/*	return OK;	 */
/*
	rc = mv_pp3_hw_write(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;
*/

	COMPLETE_HW_WRITE
	rc = OK;
	return rc;
}
#else
int bm_register_read(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;

	if ((base_address <   BM_ADDRESS_MIN) || (base_address >   BM_ADDRESS_MAX))
		return rc;
	if ((offset       <    BM_OFFSET_MIN) || (offset       >    BM_OFFSET_MAX))
		return rc;
	if ((wordsNumber  < BM_DATA_SIZE_MIN) || (wordsNumber  > BM_DATA_SIZE_MAX))
		return rc;
	if (((u32)dataPtr <  BM_DATA_PTR_MIN) || ((u32)dataPtr >  BM_DATA_PTR_MAX))
		return rc;

	/*rc = */
	mv_pp3_hw_read(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;

	COMPLETE_HW_WRITE
	rc = OK;
	return rc;
}

int bm_register_write(u32 base_address, u32 offset, u32 wordsNumber, u32 *dataPtr)
{
	int rc = -BM_INPUT_NOT_IN_RANGE;

	if ((base_address <   BM_ADDRESS_MIN) || (base_address >   BM_ADDRESS_MAX))
		return rc;
	if ((offset       <    BM_OFFSET_MIN) || (offset       >    BM_OFFSET_MAX))
		return rc;
	if ((wordsNumber  < BM_DATA_SIZE_MIN) || (wordsNumber  > BM_DATA_SIZE_MAX))
		return rc;
	if (((u32)dataPtr <  BM_DATA_PTR_MIN) || ((u32)dataPtr >  BM_DATA_PTR_MAX))
		return rc;

	/*rc = */
	mv_pp3_hw_write(base_address+offset, wordsNumber, dataPtr);
	if (rc != OK)
		return rc;

	COMPLETE_HW_WRITE
	rc = OK;
	return rc;
}
#endif

void bm_register_register_fields_print(u32 base_address, u32 value)
{

}
#ifdef MY_HIDE_DEBUG
#endif /* MY_HIDE_DEBUG */
