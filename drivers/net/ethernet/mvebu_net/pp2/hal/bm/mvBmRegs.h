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


#ifndef __mvBmRegs_h__
#define __mvBmRegs_h__

#include "pp2/gbe/mvPp2GbeRegs.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MV_BM_POOLS                 8
#define MV_BM_POOL_CAP_MAX          (16*1024 - MV_BM_POOL_PTR_ALIGN/4)
#define MV_BM_POOL_CAP_MIN          128
#define MV_BM_POOL_PTR_ALIGN        128

/* Address of External Buffer Pointers Pool Register */
#define MV_BM_POOL_BASE_REG(pool)       (MV_PP2_REG_BASE + 0x6000 + ((pool) * 4))

#define MV_BM_POOL_BASE_ADDR_OFFS       7
#define MV_BM_POOL_BASE_ADDR_MASK       (0x1FFFFF << MV_BM_POOL_BASE_ADDR_OFFS)
/*-------------------------------------------------------------------------------*/

/* External Buffer Pointers Pool Size Register */
#define MV_BM_POOL_SIZE_REG(pool)       (MV_PP2_REG_BASE + 0x6040 + ((pool) * 4))

#define MV_BM_POOL_SIZE_OFFS            4
#define MV_BM_POOL_SIZE_MASK            (0xFFF << MV_BM_POOL_SIZE_OFFS)
/*-------------------------------------------------------------------------------*/

/* External Buffer Pointers Pool Read pointer Register */
#define MV_BM_POOL_READ_PTR_REG(pool)   (MV_PP2_REG_BASE + 0x6080 + ((pool) * 4))

#define MV_BM_POOL_GET_READ_PTR_OFFS    4
#define MV_BM_POOL_GET_READ_PTR_MASK    (0xFFF << MV_BM_POOL_GET_READ_PTR_OFFS)
/*-------------------------------------------------------------------------------*/

/* External Buffer Pointers Pool Number of Pointers Register */
#define MV_BM_POOL_PTRS_NUM_REG(pool)	(MV_PP2_REG_BASE + 0x60c0 + ((pool) * 4))

#define MV_BM_POOL_PTRS_NUM_OFFS		4
#define MV_BM_POOL_PTRS_NUM_MASK		(0xFFF << MV_BM_POOL_PTRS_NUM_OFFS)
/*-------------------------------------------------------------------------------*/

/* Internal Buffer Pointers Pool RD pointer Register */
#define MV_BM_BPPI_READ_PTR_REG(pool)   (MV_PP2_REG_BASE + 0x6100 + ((pool) * 4))
/*-------------------------------------------------------------------------------*/

/* Internal Buffer Pointers Pool Num of pointers Register */
#define MV_BM_BPPI_PTRS_NUM_REG(pool)   (MV_PP2_REG_BASE + 0x6140 + ((pool) * 4))

#define MV_BM_BPPI_PTR_NUM_OFFS    	0
#define MV_BM_BPPI_PTR_NUM_MASK	    	(0x7FF << MV_BM_BPPI_PTR_NUM_OFFS)

#define MV_BM_BPPI_PREFETCH_FULL_BIT    16
#define MV_BM_BPPI_PREFETCH_FULL_MASK	(0x1 << MV_BM_BPPI_PREFETCH_FULL_BIT)
/*-------------------------------------------------------------------------------*/

/* BM Activation Register */
#define MV_BM_POOL_CTRL_REG(pool)       (MV_PP2_REG_BASE + 0x6200 + ((pool) * 4))

#define MV_BM_START_BIT                 0
#define MV_BM_START_MASK                (1 << MV_BM_START_BIT)

#define MV_BM_STOP_BIT                  1
#define MV_BM_STOP_MASK                 (1 << MV_BM_STOP_BIT)

#define MV_BM_STATE_BIT                 4
#define MV_BM_STATE_MASK                (1 << MV_BM_STATE_BIT)

#define MV_BM_LOW_THRESH_OFFS           8
#define MV_BM_LOW_THRESH_MASK           (0x7F << MV_BM_LOW_THRESH_OFFS)
#define MV_BM_LOW_THRESH_VALUE(val)     ((val) << MV_BM_LOW_THRESH_OFFS)

#define MV_BM_HIGH_THRESH_OFFS          16
#define MV_BM_HIGH_THRESH_MASK          (0x7F << MV_BM_HIGH_THRESH_OFFS)
#define MV_BM_HIGH_THRESH_VALUE(val)    ((val) << MV_BM_HIGH_THRESH_OFFS)
/*-------------------------------------------------------------------------------*/

/* BM Interrupt Cause Register */
#define MV_BM_INTR_CAUSE_REG(pool)      (MV_PP2_REG_BASE + 0x6240 + ((pool) * 4))

#define MV_BM_RELEASED_DELAY_BIT        0
#define MV_BM_RELEASED_DELAY_MASK       (1 << MV_BM_RELEASED_DELAY_BIT)

#define MV_BM_ALLOC_FAILED_BIT          1
#define MV_BM_ALLOC_FAILED_MASK         (1 << MV_BM_ALLOC_FAILED_BIT)

#define MV_BM_BPPE_EMPTY_BIT            2
#define MV_BM_BPPE_EMPTY_MASK           (1 << MV_BM_BPPE_EMPTY_BIT)

#define MV_BM_BPPE_FULL_BIT             3
#define MV_BM_BPPE_FULL_MASK            (1 << MV_BM_BPPE_FULL_BIT)

#define MV_BM_AVAILABLE_BP_LOW_BIT      4
#define MV_BM_AVAILABLE_BP_LOW_MASK     (1 << MV_BM_AVAILABLE_BP_LOW_BIT)
/*-------------------------------------------------------------------------------*/

/* BM interrupt Mask Register */
#define MV_BM_INTR_MASK_REG(pool)       (MV_PP2_REG_BASE + 0x6280 + ((pool) * 4))
/*-------------------------------------------------------------------------------*/

/* BM physical address allocate */
#define MV_BM_PHY_ALLOC_REG(pool)	(MV_PP2_REG_BASE + 0x6400 + ((pool) * 4))

#define MV_BM_PHY_ALLOC_GRNTD_MASK	(0x1)

/* BM virtual address allocate */
#define MV_BM_VIRT_ALLOC_REG		(MV_PP2_REG_BASE + 0x6440)

/* BM physical address release */
#define MV_BM_PHY_RLS_REG(pool)		(MV_PP2_REG_BASE + 0x6480 + ((pool) * 4))

#define MV_BM_PHY_RLS_MC_BUFF_MASK	(0x1)
#define MV_BM_PHY_RLS_PRIO_EN_MASK	(0x2)
#define MV_BM_PHY_RLS_GRNTD_MASK	(0x4)

/* BM virtual address release */
#define MV_BM_VIRT_RLS_REG		(MV_PP2_REG_BASE + 0x64c0)

/*-------------------------------------------------------------------------------*/

/* BM MC release */
#define MV_BM_MC_RLS_REG		(MV_PP2_REG_BASE + 0x64c4)

#define MV_BM_MC_ID_OFFS		0
#define MV_BM_MC_ID_MASK		(0xfff << MV_BM_MC_ID_OFFS)

#define MV_BM_FORCE_RELEASE_OFFS	12
#define MV_BM_FORCE_RELEASE_MASK	(0x1 << MV_BM_FORCE_RELEASE_OFFS)

/*-------------------------------------------------------------------------------*/
/* BM prio alloc/release */
#define MV_BM_QSET_ALLOC_REG		(MV_PP2_REG_BASE + 0x63fc)

#define MV_BM_ALLOC_QSET_NUM_OFFS	0
#define MV_BM_ALLOC_QSET_NUM_MASK	(0x7f << MV_BM_ALLOC_QSET_NUM_OFFS)

#define MV_BM_ALLOC_YELLOW_MASK		(0x1 << 8)

#define MV_BM_ALLOC_PRIO_EN_MASK	(0x1 << 12)


#define MV_BM_QSET_RLS_REG		(MV_PP2_REG_BASE + 0x64c8)

#define MV_BM_RLS_QSET_NUM_OFFS		0
#define MV_BM_RLS_QSET_NUM_MASK		(0x7f << MV_BM_RLS_QSET_NUM_OFFS)
/*-------------------------------------------------------------------------------*/
/* BM Priority Configuration Registers */

#define MV_BM_PRIO_CTRL_REG		(MV_PP2_REG_BASE + 0x6800)


#define MV_BM_PRIO_IDX_REG		(MV_PP2_REG_BASE + 0x6810)
#define MV_BM_PRIO_IDX_MASK		0xff


#define MV_BM_CPU_QSET_REG		(MV_PP2_REG_BASE + 0x6814)

#define MV_BM_CPU_SHORT_QSET_OFFS	0
#define MV_BM_CPU_SHORT_QSET_MASK	(0x7f << MV_BM_CPU_SHORT_QSET_OFFS)

#define MV_BM_CPU_LONG_QSET_OFFS	8
#define MV_BM_CPU_LONG_QSET_MASK	(0x7f << MV_BM_CPU_LONG_QSET_OFFS)


#define MV_BM_HWF_QSET_REG		(MV_PP2_REG_BASE + 0x6818)

#define MV_BM_HWF_SHORT_QSET_OFFS	0
#define MV_BM_HWF_SHORT_QSET_MASK	(0x7f << MV_BM_HWF_SHORT_QSET_OFFS)

#define MV_BM_HWF_LONG_QSET_OFFS	8
#define MV_BM_HWF_LONG_QSET_MASK	(0x7f << MV_BM_HWF_LONG_QSET_OFFS)


#define MV_BM_QSET_SET_MAX_REG		(MV_PP2_REG_BASE + 0x6820)

#define MV_BM_QSET_MAX_SHARED_OFFS	0
#define MV_BM_QSET_MAX_GRNTD_OFFS	16

#define MV_BM_QSET_MAX_SHARED_MASK	(0xffff << MV_BM_QSET_MAX_SHARED_OFFS)
#define MV_BM_QSET_MAX_GRNTD_MASK	(0xffff << MV_BM_QSET_MAX_GRNTD_OFFS)


#define MV_BM_QSET_SET_CNTRS_REG	(MV_PP2_REG_BASE + 0x6824)

#define MV_BM_QSET_CNTR_SHARED_OFFS	0
#define MV_BM_QSET_CNTR_GRNTD_OFFS	16


#define MV_BM_POOL_MAX_SHARED_REG(pool)	(MV_PP2_REG_BASE + 0x6840 + ((pool) * 4))
#define MV_BM_POOL_MAX_SHARED_OFFS	0
#define MV_BM_POOL_MAX_SHARED_MASK	(0xffff << MV_BM_POOL_MAX_SHARED_OFFS)

#define MV_BM_POOL_SET_CNTRS_REG(pool)	(MV_PP2_REG_BASE + 0x6880 + ((pool) * 4))

#define MV_BM_POOL_CNTR_SHARED_OFFS	0
#define MV_BM_POOL_CNTR_GRNTD_OFFS	16

#define MV_BM_V1_PKT_DROP_REG(pool)		(MV_PP2_REG_BASE + 0x7300 + 4 * (pool))
#define MV_BM_V1_PKT_MC_DROP_REG(pool)		(MV_PP2_REG_BASE + 0x7340 + 4 * (pool))


#define MV_BM_POOL_SHARED_STATUS(pool)		(MV_PP2_REG_BASE + 0x68c0 + ((pool) * 4))


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvBmRegs_h__ */

