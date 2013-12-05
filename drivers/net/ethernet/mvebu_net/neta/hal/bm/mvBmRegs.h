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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define MV_BM_POOLS                 4
#define MV_BM_POOL_CAP_MAX          (16*1024 - MV_BM_POOL_PTR_ALIGN/4)
#define MV_BM_POOL_CAP_MIN          128
#define MV_BM_POOL_PTR_ALIGN        128

/* BM Configuration Register */
#define MV_BM_CONFIG_REG                (MV_BM_REG_BASE + 0x0)

#define MV_BM_SRC_BURST_SIZE_OFFS       0
#define MV_BM_SRC_BURST_SIZE_MASK       (3 << MV_BM_SRC_BURST_SIZE_OFFS)
#define MV_BM_SRC_BURST_SIZE_128B       (0 << MV_BM_SRC_BURST_SIZE_OFFS)
#define MV_BM_SRC_BURST_SIZE_32B        (1 << MV_BM_SRC_BURST_SIZE_OFFS)

#define MV_BM_DST_BURST_SIZE_OFFS       2
#define MV_BM_DST_BURST_SIZE_MASK       (3 << MV_BM_DST_BURST_SIZE_OFFS)
#define MV_BM_DST_BURST_SIZE_128B       (0 << MV_BM_DST_BURST_SIZE_OFFS)
#define MV_BM_DST_BURST_SIZE_32B        (1 << MV_BM_DST_BURST_SIZE_OFFS)

#define MV_BM_DST_SWAP_BIT              4
#define MV_BM_DST_SWAP_MASK             (1 << MV_BM_DST_SWAP_BIT)

#define MV_BM_SRC_SWAP_BIT              5
#define MV_BM_SRC_SWAP_MASK             (1 << MV_BM_SRC_SWAP_BIT)

#define MV_BM_LOW_THRESH_OFFS           8
#define MV_BM_LOW_THRESH_MASK           (0xF << MV_BM_LOW_THRESH_OFFS)
#define MV_BM_LOW_THRESH_VALUE(val)     ((val) << MV_BM_LOW_THRESH_OFFS)

#define MV_BM_HIGH_THRESH_OFFS          12
#define MV_BM_HIGH_THRESH_MASK          (0xF << MV_BM_HIGH_THRESH_OFFS)
#define MV_BM_HIGH_THRESH_VALUE(val)    ((val) << MV_BM_HIGH_THRESH_OFFS)

#define MV_BM_MAX_IN_BURST_SIZE_OFFS    17
#define MV_BM_MAX_IN_BURST_SIZE_MASK    (3 << MV_BM_MAX_IN_BURST_SIZE_OFFS)
#define MV_BM_MAX_IN_BURST_SIZE_32BP    (0 << MV_BM_MAX_IN_BURST_SIZE_OFFS)
#define MV_BM_MAX_IN_BURST_SIZE_24BP    (1 << MV_BM_MAX_IN_BURST_SIZE_OFFS)
#define MV_BM_MAX_IN_BURST_SIZE_16BP    (2 << MV_BM_MAX_IN_BURST_SIZE_OFFS)
#define MV_BM_MAX_IN_BURST_SIZE_8BP     (3 << MV_BM_MAX_IN_BURST_SIZE_OFFS)

#define MV_BM_EMPTY_LIMIT_BIT			19
#define MV_BM_EMPTY_LIMIT_MASK			(1 << MV_BM_EMPTY_LIMIT_BIT)


/* BM Activation Register */
#define MV_BM_COMMAND_REG               (MV_BM_REG_BASE + 0x4)

#define MV_BM_START_BIT                 0
#define MV_BM_START_MASK                (1 << MV_BM_START_BIT)

#define MV_BM_STOP_BIT                  1
#define MV_BM_STOP_MASK                 (1 << MV_BM_STOP_BIT)

#define MV_BM_PAUSE_BIT                 2
#define MV_BM_PAUSE_MASK                (1 << MV_BM_PAUSE_BIT)

#define MV_BM_STATUS_OFFS               4
#define MV_BM_STATUS_ALL_MASK           (0x3)
#define MV_BM_STATUS_NOT_ACTIVE         (0x0)
#define MV_BM_STATUS_ACTIVE             (0x1)
#define MV_BM_STATUS_PAUSED             (0x2)
#define MV_BM_STATUS_MASK(status)       ((status) << MV_BM_STATUS_OFFS)

/* BM Xbar interface Register */
#define MV_BM_XBAR_01_REG               (MV_BM_REG_BASE + 0x8)
#define MV_BM_XBAR_23_REG               (MV_BM_REG_BASE + 0xC)

#define MV_BM_XBAR_POOL_REG(pool)       (((pool) < 2) ? MV_BM_XBAR_01_REG : MV_BM_XBAR_23_REG)

#define MV_BM_TARGET_ID_OFFS(pool)      (((pool) & 1) ? 16 : 0)
#define MV_BM_TARGET_ID_MASK(pool)      (0xF << MV_BM_TARGET_ID_OFFS(pool))
#define MV_BM_TARGET_ID_VAL(pool, id)   ((id) << MV_BM_TARGET_ID_OFFS(pool))

#define MV_BM_XBAR_ATTR_OFFS(pool)      (((pool) & 1) ? 20 : 4)
#define MV_BM_XBAR_ATTR_MASK(pool)      (0xFF << MV_BM_XBAR_ATTR_OFFS(pool))
#define MV_BM_XBAR_ATTR_VAL(pool, attr) ((attr) << MV_BM_XBAR_ATTR_OFFS(pool))

/* Address of External Buffer Pointers Pool Register */
#define MV_BM_POOL_BASE_REG(pool)       (MV_BM_REG_BASE + 0x10 + ((pool) << 4))

#define MV_BM_POOL_ENABLE_BIT           0
#define MV_BM_POOL_ENABLE_MASK          (1 << MV_BM_POOL_ENABLE_BIT)

#define MV_BM_POOL_BASE_ADDR_OFFS       2
#define MV_BM_POOL_BASE_ADDR_MASK       (0x3FFFFFFF << MV_BM_POOL_BASE_ADDR_OFFS)

/* External Buffer Pointers Pool RD pointer Register */
#define MV_BM_POOL_READ_PTR_REG(pool)   (MV_BM_REG_BASE + 0x14 + ((pool) << 4))

#define MV_BM_POOL_SET_READ_PTR_OFFS    0
#define MV_BM_POOL_SET_READ_PTR_MASK    (0xFFFC << MV_BM_POOL_SET_READ_PTR_OFFS)
#define MV_BM_POOL_SET_READ_PTR(val)    ((val) << MV_BM_POOL_SET_READ_PTR_OFFS)

#define MV_BM_POOL_GET_READ_PTR_OFFS    16
#define MV_BM_POOL_GET_READ_PTR_MASK    (0xFFFC << MV_BM_POOL_GET_READ_PTR_OFFS)


/* External Buffer Pointers Pool WR pointer */
#define MV_BM_POOL_WRITE_PTR_REG(pool)  (MV_BM_REG_BASE + 0x18 + ((pool) << 4))

#define MV_BM_POOL_SET_WRITE_PTR_OFFS   0
#define MV_BM_POOL_SET_WRITE_PTR_MASK   (0xFFFC << MV_BM_POOL_SET_WRITE_PTR_OFFS)
#define MV_BM_POOL_SET_WRITE_PTR(val)   ((val) << MV_BM_POOL_SET_WRITE_PTR_OFFS)

#define MV_BM_POOL_GET_WRITE_PTR_OFFS   16
#define MV_BM_POOL_GET_WRITE_PTR_MASK   (0xFFFC << MV_BM_POOL_GET_WRITE_PTR_OFFS)

/* External Buffer Pointers Pool Size Register */
#define MV_BM_POOL_SIZE_REG(pool)       (MV_BM_REG_BASE + 0x1C + ((pool) << 4))

#define MV_BM_POOL_SIZE_OFFS            0
#define MV_BM_POOL_SIZE_MASK            (0x3FFF << MV_BM_POOL_SIZE_OFFS)
#define MV_BM_POOL_SIZE_VAL(size)       ((size) << MV_BM_POOL_SIZE_OFFS)


/* BM Interrupt Cause Register */
#define MV_BM_INTR_CAUSE_REG            (MV_BM_REG_BASE + 0x50)

#define MV_BM_CAUSE_FREE_FAIL_BIT(p)    (0 + ((p) * 6))
#define MV_BM_CAUSE_FREE_FAIL_MASK(p)   (1 << MV_BM_CAUSE_FREE_FAIL_BIT(p))

#define MV_BM_CAUSE_ALLOC_FAIL_BIT(p)   (1 + ((p) * 6))
#define MV_BM_CAUSE_ALLOC_FAIL_MASK(p)  (1 << MV_BM_CAUSE_ALLOC_FAIL_BIT(p))

#define MV_BM_CAUSE_POOL_EMPTY_BIT(p)   (2 + ((p) * 6))
#define MV_BM_CAUSE_POOL_EMPTY_MASK(p)  (1 << MV_BM_CAUSE_POOL_EMPTY_BIT(p))

#define MV_BM_CAUSE_POOL_FULL_BIT(p)    (3 + ((p) * 6))
#define MV_BM_CAUSE_POOL_FULL_MASK(p)   (1 << MV_BM_CAUSE_POOL_FULL_BIT(p))

#define MV_BM_CAUSE_INT_PAR_ERR_BIT     27
#define MV_BM_CAUSE_INT_PAR_ERR_MASK    (1 << MV_BM_CAUSE_INT_PAR_ERR_MASK)

#define MV_BM_CAUSE_XBAR_PAR_ERR_BIT    28
#define MV_BM_CAUSE_XBAR_PAR_ERR_MASK   (1 << MV_BM_CAUSE_XBAR_PAR_ERR_MASK)

#define MV_BM_CAUSE_STOPPED_BIT         29
#define MV_BM_CAUSE_STOPPED_MASK        (1 << MV_BM_CAUSE_STOPPED_MASK)

#define MV_BM_CAUSE_PAUSED_BIT          30
#define MV_BM_CAUSE_PAUSED_MASK         (1 << MV_BM_CAUSE_PAUSED_MASK)

#define MV_BM_CAUSE_SUMMARY_BIT         31
#define MV_BM_CAUSE_SUMMARY_MASK        (1 << MV_BM_CAUSE_SUMMARY_MASK)

/* BM interrupt Mask Register */
#define MV_BM_INTR_MASK_REG             (MV_BM_REG_BASE + 0x54)

#define MV_BM_DEBUG_REG                 (MV_BM_REG_BASE + 0x60)
#define MV_BM_READ_PTR_REG              (MV_BM_REG_BASE + 0x64)
#define MV_BM_WRITE_PTR_REG             (MV_BM_REG_BASE + 0x68)



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvBmRegs_h__ */
