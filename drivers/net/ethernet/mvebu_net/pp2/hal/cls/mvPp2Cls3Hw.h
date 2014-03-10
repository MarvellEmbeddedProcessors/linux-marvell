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

#ifndef __MV_CLS3_HW_H__
#define __MV_CLS3_HW_H__

#include "mvPp2ClsActHw.h"
#include "mvPp2ClsHw.h"
#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"

/*-------------------------------------------------------------------------------*/
/*			Classifier C3 Top Registers	    			 */
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_KEY_CTRL_REG		(MV_PP2_REG_BASE + 0x1C10)
#define KEY_CTRL_L4				0
#define KEY_CTRL_L4_BITS			3
#define KEY_CTRL_L4_MAX				((1 << KEY_CTRL_L4_BITS) - 1)
#define KEY_CTRL_L4_MASK			(((1 << KEY_CTRL_L4_BITS) - 1) << KEY_CTRL_L4)

/*
  PPv2.1 (feature MAS 3.16) LKP_TYPE size and offset changed
*/
#ifdef CONFIG_MV_ETH_PP2_1
#define KEY_CTRL_LKP_TYPE			4
#define KEY_CTRL_LKP_TYPE_BITS			6
#else
#define KEY_CTRL_LKP_TYPE			8
#define KEY_CTRL_LKP_TYPE_BITS			4
#endif

#define KEY_CTRL_LKP_TYPE_MAX			((1 << KEY_CTRL_LKP_TYPE_BITS) - 1)
#define KEY_CTRL_LKP_TYPE_MASK			(((1 << KEY_CTRL_LKP_TYPE_BITS) - 1) << KEY_CTRL_LKP_TYPE)


#define KEY_CTRL_PRT_ID_TYPE			12
#define KEY_CTRL_PRT_ID_TYPE_BITS		2
#define KEY_CTRL_PRT_ID_TYPE_MAX		((1 << KEY_CTRL_PRT_ID_TYPE_BITS) - 1)
#define KEY_CTRL_PRT_ID_TYPE_MASK		((KEY_CTRL_PRT_ID_TYPE_MAX) << KEY_CTRL_PRT_ID_TYPE)

#define KEY_CTRL_PRT_ID				16
#define KEY_CTRL_PRT_ID_BITS			8
#define KEY_CTRL_PRT_ID_MAX			((1 << KEY_CTRL_PRT_ID_BITS) - 1)
#define KEY_CTRL_PRT_ID_MASK			(((1 << KEY_CTRL_PRT_ID_BITS) - 1) << KEY_CTRL_PRT_ID)

#define KEY_CTRL_HEK_SIZE			24
#define KEY_CTRL_HEK_SIZE_BITS			6
#define KEY_CTRL_HEK_SIZE_MAX			36
#define KEY_CTRL_HEK_SIZE_MASK			(((1 << KEY_CTRL_HEK_SIZE_BITS) - 1) << KEY_CTRL_HEK_SIZE)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_KEY_HEK_REG(reg_num)	(MV_PP2_REG_BASE + 0x1C34 - 4*(reg_num))
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_QRY_ACT_REG			(MV_PP2_REG_BASE + 0x1C40)
#define MV_PP2_CLS3_QRY_ACT			0
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_QRY_RES_HASH_REG(hash)	(MV_PP2_REG_BASE + 0x1C50 + 4*(hash))
#define MV_PP2_CLS3_HASH_BANKS_NUM		8
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_INIT_HIT_CNT_REG	(MV_PP2_REG_BASE + 0x1C80)
#define MV_PP2_CLS3_INIT_HIT_CNT_OFFS	6
#define MV_PP2_CLS3_INIT_HIT_CNT_BITS	18
#define MV_PP2_CLS3_INIT_HIT_CNT_MASK	(((1 << MV_PP2_CLS3_INIT_HIT_CNT_BITS) - 1) << MV_PP2_CLS3_INIT_HIT_CNT_OFFS)
#define MV_PP2_CLS3_INIT_HIT_CNT_MAX	((1 << MV_PP2_CLS3_INIT_HIT_CNT_BITS) - 1)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_HASH_OP_REG			(MV_PP2_REG_BASE + 0x1C84)

#define MV_PP2_CLS3_HASH_OP_TBL_ADDR		0
#define MV_PP2_CLS3_HASH_OP_TBL_ADDR_BITS	12
#define MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX	((1 << MV_PP2_CLS3_HASH_OP_TBL_ADDR_BITS) - 1)
#define MV_PP2_CLS3_HASH_OP_TBL_ADDR_MASK	((MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX) << MV_PP2_CLS3_HASH_OP_TBL_ADDR)

/*PPv2.1 (feature MAS 3.16) MISS_PTR is new field (one bit) at HASH_OP_REG */
#define MV_PP2_CLS3_MISS_PTR			12
#define MV_PP2_CLS3_MISS_PTR_MASK		(1 << MV_PP2_CLS3_MISS_PTR)

#define MV_PP2_CLS3_HASH_OP_DEL			14
#define MV_PP2_CLS3_HASH_OP_ADD			15

#define MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR	16
#define MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_BITS	8
#define MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_MAX	((1 << MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_BITS) - 1)
#define MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_MASK	\
		((MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR_MAX) << MV_PP2_CLS3_HASH_OP_EXT_TBL_ADDR)

/*PPv2.1 (feature MAS 3.16) INIT_CNT_VAL field removed*/
#define MV_PP2_CLS3_HASH_OP_INIT_CTR_VAL	24
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_STATE_REG			(MV_PP2_REG_BASE + 0x1C8C)
#define MV_PP2_CLS3_STATE_CPU_DONE		0
#define MV_PP2_CLS3_STATE_CPU_DONE_MASK		(1 << MV_PP2_CLS3_STATE_CPU_DONE)

#define MV_PP2_CLS3_STATE_CLEAR_CTR_DONE	1
#define MV_PP2_CLS3_STATE_CLEAR_CTR_DONE_MASK	(1 << MV_PP2_CLS3_STATE_CLEAR_CTR_DONE)

#define MV_PP2_CLS3_STATE_SC_DONE		2
#define MV_PP2_CLS3_STATE_SC_DONE_MASK		(1 << MV_PP2_CLS3_STATE_SC_DONE)

#define MV_PP2_CLS3_STATE_OCCIPIED		8
#define MV_PP2_CLS3_STATE_OCCIPIED_BITS		8
#define MV_PP2_CLS3_STATE_OCCIPIED_MASK		\
		(((1 << MV_PP2_CLS3_STATE_OCCIPIED_BITS) - 1) << MV_PP2_CLS3_STATE_OCCIPIED)

#define MV_PP2_CLS3_STATE_SC_STATE		16
#define MV_PP2_CLS3_STATE_SC_STATE_BITS		2
#define MV_PP2_CLS3_STATE_SC_STATE_MASK		\
		(((1 << MV_PP2_CLS3_STATE_SC_STATE_BITS) - 1) << MV_PP2_CLS3_STATE_SC_STATE)
/*
SCAN STATUS
0 - scan compleat
1 -	hit counter clear
3 - scan wait
4 - scan in progress
*/

#define MV_PP2_CLS3_STATE_NO_OF_SC_RES		20
#define MV_PP2_CLS3_STATE_NO_OF_SC_RES_BITS	9
#define MV_PP2_CLS3_STATE_NO_OF_SC_RES_MASK	\
		(((1 << MV_PP2_CLS3_STATE_NO_OF_SC_RES_BITS) - 1) << MV_PP2_CLS3_STATE_NO_OF_SC_RES)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_DB_INDEX_REG		(MV_PP2_REG_BASE + 0x1C90)
#define MV_PP2_CLS3_DB_MISS_OFFS		12
#define MV_PP2_CLS3_DB_MISS_MASK		(1 << MV_PP2_CLS3_DB_MISS_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_HASH_DATA_REG(num)		(MV_PP2_REG_BASE + 0x1CA0 + 4*(num)) /* 0-3 valid val*/
#define MV_PP2_CLS3_HASH_DATA_REG_NUM		4
#define MV_PP2_CLS3_HASH_EXT_DATA_REG(num)	(MV_PP2_REG_BASE + 0x1CC0 + 4*(num))
#define MV_PP2_CLS3_HASH_EXT_DATA_REG_NUM	7
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_CLEAR_COUNTERS_REG		(MV_PP2_REG_BASE + 0x1D00)
#define MV_PP2_CLS3_CLEAR_COUNTERS		0
/*
  PPv2.1 (feature MAS 3.16)  CLEAR_COUNTERS size changed, clear all code changed from 0x1f to 0x3f
*/
#define MV_PP2_V1_CLS3_CLEAR_COUNTERS_BITS	7
#define MV_PP2_V1_CLS3_CLEAR_ALL		0x3f
#define MV_PP2_V1_CLS3_CLEAR_COUNTERS_MAX	0x3F
#define MV_PP2_V1_CLS3_CLEAR_COUNTERS_MASK	((MV_PP2_V1_CLS3_CLEAR_COUNTERS_MAX) << MV_PP2_V1_CLS3_CLEAR_COUNTERS)

#define MV_PP2_V0_CLS3_CLEAR_COUNTERS_BITS	5
#define MV_PP2_V0_CLS3_CLEAR_ALL		0x1f
#define MV_PP2_V0_CLS3_CLEAR_COUNTERS_MAX	0x1F
#define MV_PP2_V0_CLS3_CLEAR_COUNTERS_MASK	((MV_PP2_V0_CLS3_CLEAR_COUNTERS_MAX)  << MV_PP2_V0_CLS3_CLEAR_COUNTERS)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_HIT_COUNTER_REG		(MV_PP2_REG_BASE + 0x1D08)
#define MV_PP2_CLS3_HIT_COUNTER			0
/*ppv2.1 his counter field size changed from 14 bits to 24 bits*/
#define MV_PP2_V0_CLS3_HIT_COUNTER_BITS		14
#define MV_PP2_V0_CLS3_HIT_COUNTER_MAX		((1 << MV_PP2_V0_CLS3_HIT_COUNTER_BITS) - 1)
#define MV_PP2_V0_CLS3_HIT_COUNTER_MASK		((MV_PP2_V0_CLS3_HIT_COUNTER_MAX) << MV_PP2_CLS3_HIT_COUNTER)

#define MV_PP2_V1_CLS3_HIT_COUNTER_BITS		24
#define MV_PP2_V1_CLS3_HIT_COUNTER_MAX		((1 << MV_PP2_V1_CLS3_HIT_COUNTER_BITS) - 1)
#define MV_PP2_V1_CLS3_HIT_COUNTER_MASK		((MV_PP2_V1_CLS3_HIT_COUNTER_MAX) << MV_PP2_CLS3_HIT_COUNTER)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_SC_PROP_REG			(MV_PP2_REG_BASE + 0x1D10)

#define MV_PP2_CLS3_SC_PROP_TH_MODE		0
#define MV_PP2_CLS3_SC_PROP_TH_MODE_MASK	(1 << MV_PP2_CLS3_SC_PROP_TH_MODE)

#define MV_PP2_CLS3_SC_PROP_CLEAR		1
#define MV_PP2_CLS3_SC_PROP_CLEAR_MASK		(1 << MV_PP2_CLS3_SC_PROP_CLEAR)

#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN		3
#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN_MASK	(1 << MV_PP2_CLS3_SC_PROP_LKP_TYPE_EN)

#define MV_PP2_CLS3_SC_PROP_LKP_TYPE		4
/*
  PPv2.1 (feature MAS 3.16) LKP_TYPE size and offset changed
*/

#ifdef CONFIG_MV_ETH_PP2_1
#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_BITS	6
#else
#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_BITS	4
#endif

#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_MAX	((1 << MV_PP2_CLS3_SC_PROP_LKP_TYPE_BITS) - 1)
#define MV_PP2_CLS3_SC_PROP_LKP_TYPE_MASK	((MV_PP2_CLS3_SC_PROP_LKP_TYPE_MAX) << MV_PP2_CLS3_SC_PROP_LKP_TYPE)

#define MV_PP2_CLS3_SC_PROP_START_ENTRY		16
#define MV_PP2_CLS3_SC_PROP_START_ENTRY_MASK	((MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX) << MV_PP2_CLS3_SC_PROP_START_ENTRY)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_SC_PROP_VAL_REG		(MV_PP2_REG_BASE + 0x1D14)

/* ppv2.1 field removed from this reg */
#define MV_PP2_V0_CLS3_SC_PROP_VAL_TH		0
#define MV_PP2_V0_CLS3_SC_PROP_VAL_TH_BITS	13
#define MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MAX	((1 << MV_PP2_V0_CLS3_SC_PROP_VAL_TH_BITS) - 1)
#define MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MASK	((MV_PP2_V0_CLS3_SC_PROP_VAL_TH_MAX) << MV_PP2_V0_CLS3_SC_PROP_VAL_TH)

/* ppv2.1 field offsett changed */
#define MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY	16
#define MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY	0
#define MV_PP2_CLS3_SC_PROP_VAL_DELAY_BITS	16
#define MV_PP2_CLS3_SC_PROP_VAL_DELAY_MAX	((1 << MV_PP2_CLS3_SC_PROP_VAL_DELAY_BITS) - 1)
#define MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY_MASK	(MV_PP2_CLS3_SC_PROP_VAL_DELAY_MAX << MV_PP2_V0_CLS3_SC_PROP_VAL_DELAY)
#define MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY_MASK	(MV_PP2_CLS3_SC_PROP_VAL_DELAY_MAX << MV_PP2_V1_CLS3_SC_PROP_VAL_DELAY)


/*-------------------------------------------------------------------------------*/
/* PPv2.1 new reg in cls3 */
#define MV_PP2_CLS3_SC_TH_REG			(MV_PP2_REG_BASE + 0x1D18)
#define MV_PP2_CLS3_SC_TH			4
#define MV_PP2_CLS3_SC_TH_BITS			20
#define MV_PP2_CLS3_SC_TH_MAX			((1 << MV_PP2_CLS3_SC_TH_BITS) - 1)
#define MV_PP2_CLS3_SC_TH_MASK			(((1 << MV_PP2_CLS3_SC_TH_BITS) - 1) << MV_PP2_CLS3_SC_TH)



/*-------------------------------------------------------------------------------*/
/* ppv2.1 TIMER REG ADDRESS changed */
#define MV_PP2_V0_CLS3_SC_TIMER_REG		(MV_PP2_REG_BASE + 0x1D18)
#define MV_PP2_V1_CLS3_SC_TIMER_REG		(MV_PP2_REG_BASE + 0x1D1c)

#define MV_PP2_CLS3_SC_TIMER			0
#define MV_PP2_CLS3_SC_TIMER_BITS		16
#define MV_PP2_CLS3_SC_TIMER_MASK		(((1 << MV_PP2_CLS3_SC_TIMER_BITS) - 1) << MV_PP2_CLS3_SC_TIMER)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_SC_ACT_REG			(MV_PP2_REG_BASE + 0x1D20)
#define MV_PP2_CLS3_SC_ACT			0
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_SC_INDEX_REG		(MV_PP2_REG_BASE + 0x1D28)
#define MV_PP2_CLS3_SC_INDEX			0
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_SC_RES_REG			(MV_PP2_REG_BASE + 0x1D2C)
#define MV_PP2_CLS3_SC_RES_ENTRY		0
#define MV_PP2_CLS3_SC_RES_ENTRY_MASK		((MV_PP2_CLS3_HASH_OP_TBL_ADDR_MAX) << MV_PP2_CLS3_SC_RES_ENTRY)

/*ppv2.1 field offset and size changed */
#define MV_PP2_V0_CLS3_SC_RES_CTR		16
#define MV_PP2_V0_CLS3_SC_RES_CTR_MASK		((MV_PP2_V0_CLS3_HIT_COUNTER_MAX) << MV_PP2_V0_CLS3_SC_RES_CTR)
#define MV_PP2_V1_CLS3_SC_RES_CTR		12
#define MV_PP2_V1_CLS3_SC_RES_CTR_MASK		((MV_PP2_V1_CLS3_HIT_COUNTER_MAX) << MV_PP2_V1_CLS3_SC_RES_CTR)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_ACT_REG			(MV_PP2_REG_BASE + 0x1D40)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_ACT_QOS_ATTR_REG		(MV_PP2_REG_BASE + 0x1D44)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_ACT_HWF_ATTR_REG		(MV_PP2_REG_BASE + 0x1D48)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS3_ACT_DUP_ATTR_REG		(MV_PP2_REG_BASE + 0x1D4C)
/*-------------------------------------------------------------------------------*/
/*ppv2.1: 0x1D50 0x1D54 are new registers, additional fields for action table*/
#define MV_PP2_CLS3_ACT_SEQ_L_ATTR_REG		(MV_PP2_REG_BASE + 0x1D50)
#define MV_PP2_CLS3_ACT_SEQ_H_ATTR_REG		(MV_PP2_REG_BASE + 0x1D54)
#define MV_PP2_CLS3_ACT_SEQ_SIZE		38
/*-------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------*/
/*		Classifier C3 offsets in hash table		    		 */
/*-------------------------------------------------------------------------------*/
/* PPv2.1 (feature MAS 3.16) LKP_TYPE size and offset changed */
#ifdef CONFIG_MV_ETH_PP2_1

#define KEY_OCCUPIED				116
#define KEY_FORMAT				115
#define KEY_PTR_EXT				107

#define KEY_PRT_ID(ext_mode)			((ext_mode == 1) ? (99) : (107))
#define KEY_PRT_ID_MASK(ext_mode)		(((1 << KEY_CTRL_PRT_ID_BITS) - 1) << (KEY_PRT_ID(ext_mode) % 32))

#define KEY_PRT_ID_TYPE(ext_mode)		((ext_mode == 1) ? (97) : (105))
#define KEY_PRT_ID_TYPE_MASK(ext_mode)		((KEY_CTRL_PRT_ID_TYPE_MAX) << (KEY_PRT_ID_TYPE(ext_mode) % 32))

#else

#define KEY_OCCUPIED				114
#define KEY_FORMAT				113
#define KEY_PTR_EXT				105

#define KEY_PRT_ID(ext_mode)			((ext_mode == 1) ? (97) : (105))
#define KEY_PRT_ID_MASK(ext_mode)		(((1 << KEY_CTRL_PRT_ID_BITS) - 1) << (KEY_PRT_ID(ext_mode) % 32))

#define KEY_PRT_ID_TYPE(ext_mode)		((ext_mode == 1) ? (95) : (103))
#define KEY_PRT_ID_TYPE_MASK(ext_mode)		((KEY_CTRL_PRT_ID_TYPE_MAX) << (KEY_PRT_ID_TYPE(ext_mode) % 32))

#endif /* CONFIG_MV_ETH_PP2_1 */

#define KEY_LKP_TYPE(ext_mode)			((ext_mode == 1) ? (91) : (99))
#define KEY_LKP_TYPE_MASK(ext_mode)		(((1 << KEY_CTRL_LKP_TYPE_BITS) - 1) << (KEY_LKP_TYPE(ext_mode) % 32))

#define KEY_L4_INFO(ext_mode)			((ext_mode == 1) ? (88) : (96))
#define KEY_L4_INFO_MASK(ext_mode)		(((1 << KEY_CTRL_L4_BITS) - 1) << (KEY_L4_INFO(ext_mode) % 32))


/*-------------------------------------------------------------------------------*/
/*		Classifier C3 engine Key public APIs		    		 */
/*-------------------------------------------------------------------------------*/

typedef struct {
	/* valid if size > 0 */
	/* size include the extension*/
	int	ext_ptr;
	int	size;
} CLS3_SHADOW_HASH_ENTRY;

#define HEK_EXT_FMT				"%8.8x %8.8x %8.8x | %8.8x %8.8x %8.8x %8.8x %8.8x %8.8x"
#define HEK_EXT_VAL(p)				p[8], p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]

#define HEK_FMT					"%8.8x %8.8x %8.8x"
#define HEK_VAL(p)				p[8], p[7], p[6]

/*-------------------------------------------------------------------------------*/
/*			Classifier C3 engine Public APIs	 		 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_C3_HASH_TBL_SIZE			(4096)
#define MV_PP2_CLS_C3_MISS_TBL_SIZE			(64)
#define MV_PP2_CLS_C3_EXT_HEK_WORDS			(9)
#define MV_PP2_CLS_C3_SRAM_WORDS			(5)
#define MV_PP2_CLS_C3_EXT_TBL_SIZE			(256)
#define MV_PP2_CLS_C3_HEK_WORDS				(3)
#define MV_PP2_CLS_C3_HEK_BYTES				12 /* size in bytes */
#define MV_PP2_CLS_C3_BANK_SIZE				(512)
#define MV_PP2_CLS_C3_MAX_SEARCH_DEPTH			(16)

typedef struct mvPp2Cls3HashPair {
	unsigned short	pair_num;
	unsigned short	old_idx[MV_PP2_CLS_C3_MAX_SEARCH_DEPTH];
	unsigned short	new_idx[MV_PP2_CLS_C3_MAX_SEARCH_DEPTH];
} MV_PP2_CLS3_HASH_PAIR;

typedef struct mvPp2ClsC3Entry {
	unsigned int 	index;
	unsigned int 	ext_index;

	struct {
		union {
			MV_U32	words[MV_PP2_CLS_C3_EXT_HEK_WORDS];
			MV_U8	bytes[MV_PP2_CLS_C3_EXT_HEK_WORDS * 4];
		} hek;
		MV_U32		key_ctrl;/*0x1C10*/
	} key;
	union {
		MV_U32 words[MV_PP2_CLS_C3_SRAM_WORDS];
		struct {
			MV_U32 actions;/*0x1D40*/
			MV_U32 qos_attr;/*0x1D44*/
			MV_U32 hwf_attr;/*0x1D48*/
			MV_U32 dup_attr;/*0x1D4C*/
			/*ppv2.1: 0x1D50 0x1D54 are new registers, additional fields for action table*/
			MV_U32 seq_l_attr;/*0x1D50*/
			MV_U32 seq_h_attr;/*0x1D54*/
		} regs;
	} sram;
} MV_PP2_CLS_C3_ENTRY;


/*-------------------------------------------------------------------------------*/
/*			Common utilities				   	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3Init(void);
void mvPp2ClsC3ShadowInit(void);
int mvPp2ClsC3ShadowFreeGet(void);
int mvPp2ClsC3ShadowExtFreeGet(void);
void mvPp2C3ShadowClear(int index);

/*-------------------------------------------------------------------------------*/
/*			APIs for Classification C3 engine		   	 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC3HwRead(MV_PP2_CLS_C3_ENTRY *c3, int index);
int mvPp2ClsC3HwAdd(MV_PP2_CLS_C3_ENTRY *c3, int index, int ext_index);
int mvPp2ClsC3HwMissAdd(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type);
int mvPp2ClsC3HwDump(void);
int mvPp2ClsC3HwMissDump(void);
int mvPp2ClsC3HwExtDump(void);
int mvPp2ClsC3HwDel(int index);
int mvPp2ClsC3HwDelAll(void);
int mvPp2ClsC3SwDump(MV_PP2_CLS_C3_ENTRY *c3);
void mvPp2ClsC3SwClear(MV_PP2_CLS_C3_ENTRY *c3);
void mvPp2ClsC3HwInitCtrSet(int cntVal);
int mvPp2ClsC3HwQuery(MV_PP2_CLS_C3_ENTRY *c3, unsigned char *occupied_bmp, int index[]);
int mvPp2ClsC3HwQueryAdd(MV_PP2_CLS_C3_ENTRY *c3, int max_search_depth, MV_PP2_CLS3_HASH_PAIR *hash_pair_arr);

int mvPp2ClsC3HwMissRead(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type);
int mvPp2ClsC3HwMissDump(void);
/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 key fields			   	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3SwL4infoSet(MV_PP2_CLS_C3_ENTRY *c3, int l4info);
int mvPp2ClsC3SwLkpTypeSet(MV_PP2_CLS_C3_ENTRY *c3, int lkp_type);
int mvPp2ClsC3SwPortIDSet(MV_PP2_CLS_C3_ENTRY *c3, int type, int portid);
int mvPp2ClsC3SwHekSizeSet(MV_PP2_CLS_C3_ENTRY *c3, int hek_size);
int mvPp2ClsC3SwHekByteSet(MV_PP2_CLS_C3_ENTRY *c3, unsigned int offs, unsigned char byte);
int mvPp2ClsC3SwHekWordSet(MV_PP2_CLS_C3_ENTRY *c3, unsigned int offs, unsigned int word);

/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 action table fields		   	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3ColorSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd);
int mvPp2ClsC3QueueHighSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int q);
int mvPp2ClsC3QueueLowSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int q);
int mvPp2ClsC3QueueSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int queue);
int mvPp2ClsC3ForwardSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd);
#ifdef CONFIG_MV_ETH_PP2_1
int mvPp2ClsC3PolicerSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int policerId, int bank);
#else
int mvPp2ClsC3PolicerSet(MV_PP2_CLS_C3_ENTRY *c3, int cmd, int policerId);
#endif
int mvPp2ClsC3FlowIdEn(MV_PP2_CLS_C3_ENTRY *c3, int flowid_en);

/* PPv2.1 (feature MAS 3.7) mtu - new field at action table */
int mvPp2ClsC3MtuSet(MV_PP2_CLS_C3_ENTRY *c3, int mtu_inx);
int mvPp2ClsC3ModSet(MV_PP2_CLS_C3_ENTRY *c3, int data_ptr, int instr_offs, int l4_csum);
int mvPp2ClsC3DupSet(MV_PP2_CLS_C3_ENTRY *c3, int dupid, int count);

/* PPv2.1 (feature MAS 3.14) cls sequence */
int mvPp2ClsC3SeqSet(MV_PP2_CLS_C3_ENTRY *c3, int id,  int bits_offs,  int bits);

/*-------------------------------------------------------------------------------*/
/*		APIs for Classification C3 Hit counters management	   	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC3HitCntrsRead(int index, MV_U32 *cntr);
int mvPp2ClsC3HitCntrsClearAll(void);
int mvPp2ClsC3HitCntrsReadAll(void);
int mvPp2ClsC3HitCntrsClear(int lkpType);
int mvPp2ClsC3HitCntrsMissRead(int lkp_type, MV_U32 *cntr);


/*-------------------------------------------------------------------------------*/
/*	 APIs for Classification C3 hit counters scan fields operation 		 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_C3_SC_RES_TBL_SIZE			(256)

int mvPp2ClsC3ScanStart(void);
int mvPp2ClsC3ScanRegs(void);
int mvPp2ClsC3ScanThreshSet(int mode, int thresh);
int mvPp2ClsC3ScanClearBeforeEnSet(int en);
int mvPp2ClsC3ScanLkpTypeSet(int type);
int mvPp2ClsC3ScanStartIndexSet(int idx);
int mvPp2ClsC3ScanDelaySet(int time);
int mvPp2ClsC3ScanResRead(int index, int *addr, int *cnt);
int mvPp2ClsC3ScanNumOfResGet(int *resNum);
int mvPp2ClsC3ScanResDump(void);



#endif /* __MV_CLS3_HW_H__ */
