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

#ifndef __MV_CLS4_HW_H__
#define __MV_CLS4_HW_H__

#include "mvPp2ClsActHw.h"
#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"


/*-------------------------------------------------------------------------------*/
/*			Classifier C4 Top Registers	    			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_PHY_TO_RL_REG(port)				(MV_PP2_REG_BASE + 0x1E00 + ((port)*4))

#define MV_PP2_CLS4_PHY_TO_RL_GRP				0
#define MV_PP2_CLS4_PHY_TO_RL_GRP_BITS				3
#define MV_PP2_CLS4_PHY_TO_RL_GRP_MASK				(((1 << MV_PP2_CLS4_PHY_TO_RL_GRP_BITS) - 1) << MV_PP2_CLS4_PHY_TO_RL_GRP)

#define MV_PP2_CLS4_PHY_TO_RL_RULE_NUM				4
#define MV_PP2_CLS4_PHY_TO_RL_RULE_NUM_BITS			4
#define MV_PP2_CLS4_PHY_TO_RL_RULE_NUM_MASK			(((1 << MV_PP2_CLS4_PHY_TO_RL_RULE_NUM_BITS) - 1) << MV_PP2_CLS4_PHY_TO_RL_RULE_NUM)

/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_UNI_TO_RL_REG(uni)				(MV_PP2_REG_BASE + 0x1E20 + ((uni)*4))

#define MV_PP2_CLS4_UNI_TO_RL_GRP				0
#define MV_PP2_CLS4_UNI_TO_RL_RULE_NUM				4
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_RL_INDEX_REG				(MV_PP2_REG_BASE + 0x1E40)
#define MV_PP2_CLS4_RL_INDEX_RULE				0
#define MV_PP2_CLS4_RL_INDEX_GRP				3
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_FATTR1_REG					(MV_PP2_REG_BASE + 0x1E50)
#define MV_PP2_CLS4_FATTR2_REG					(MV_PP2_REG_BASE + 0x1E54)
#define MV_PP2_CLS4_FATTR_REG_NUM				2

#define MV_PP2_CLS4_FATTR_ID(field)				(((field) * 9) % 27)
#define MV_PP2_CLS4_FATTR_ID_BITS				6
#define MV_PP2_CLS4_FATTR_ID_MAX				((1 << MV_PP2_CLS4_FATTR_ID_BITS) - 1)
#define MV_PP2_CLS4_FATTR_ID_MASK(field)			(MV_PP2_CLS4_FATTR_ID_MAX << MV_PP2_CLS4_FATTR_ID(field))
#define MV_PP2_CLS4_FATTR_ID_VAL(field, reg_val)		((reg_val & MV_PP2_CLS4_FATTR_ID_MASK(field)) >> MV_PP2_CLS4_FATTR_ID(field))

#define MV_PP2_CLS4_FATTR_OPCODE_BITS				3
#define MV_PP2_CLS4_FATTR_OPCODE(field)				((((field) * 9) % 27) + MV_PP2_CLS4_FATTR_ID_BITS)
#define MV_PP2_CLS4_FATTR_OPCODE_MAX				((1 << MV_PP2_CLS4_FATTR_OPCODE_BITS) - 1)
#define MV_PP2_CLS4_FATTR_OPCODE_MASK(field)			(MV_PP2_CLS4_FATTR_OPCODE_MAX << MV_PP2_CLS4_FATTR_OPCODE(field))
#define MV_PP2_CLS4_FATTR_OPCODE_VAL(field, reg_val)		((reg_val & MV_PP2_CLS4_FATTR_OPCODE_MASK(field)) >> MV_PP2_CLS4_FATTR_OPCODE(field))
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_FDATA1_REG					(MV_PP2_REG_BASE + 0x1E58)
#define MV_PP2_CLS4_FDATA2_REG					(MV_PP2_REG_BASE + 0x1E5C)
#define MV_PP2_CLS4_FDATA3_REG					(MV_PP2_REG_BASE + 0x1E60)
#define MV_PP2_CLS4_FDATA4_REG					(MV_PP2_REG_BASE + 0x1E64)
#define MV_PP2_CLS4_FDATA5_REG					(MV_PP2_REG_BASE + 0x1E68)
#define MV_PP2_CLS4_FDATA6_REG					(MV_PP2_REG_BASE + 0x1E6C)
#define MV_PP2_CLS4_FDATA7_REG					(MV_PP2_REG_BASE + 0x1E70)
#define MV_PP2_CLS4_FDATA8_REG					(MV_PP2_REG_BASE + 0x1E74)
#define MV_PP2_CLS4_FDATA_REG(reg_num)				(MV_PP2_REG_BASE + 0x1E58 + (4*(reg_num)))
#define MV_PP2_CLS4_FDATA_REGS_NUM				8
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS4_FDATA7_L3INFO				16
#define MV_PP2_CLS4_FDATA7_L3INFO_BITS				4
#define MV_PP2_CLS4_L3INFO_MAX					((1 << MV_PP2_CLS4_FDATA7_L3INFO_BITS) - 1)
#define MV_PP2_CLS4_L3INFO_MASK					(MV_PP2_CLS4_L3INFO_MAX << MV_PP2_CLS4_FDATA7_L3INFO)
#define MV_PP2_CLS4_L3INFO_VAL(reg_val)				(((reg_val) & MV_PP2_CLS4_L3INFO_MASK) >> MV_PP2_CLS4_FDATA7_L3INFO)

#define MV_PP2_CLS4_FDATA7_L4INFO				20
#define MV_PP2_CLS4_FDATA7_L4INFO_BITS				4
#define MV_PP2_CLS4_L4INFO_MAX					((1 << MV_PP2_CLS4_FDATA7_L4INFO_BITS) - 1)
#define MV_PP2_CLS4_L4INFO_MASK					(MV_PP2_CLS4_L4INFO_MAX << MV_PP2_CLS4_FDATA7_L4INFO)
#define MV_PP2_CLS4_L4INFO_VAL(reg_val)				(((reg_val) & MV_PP2_CLS4_L4INFO_MASK) >> MV_PP2_CLS4_FDATA7_L4INFO)


#define MV_PP2_CLS4_FDATA7_MACME				24
#define MV_PP2_CLS4_FDATA7_MACME_BITS				2
#define MV_PP2_CLS4_MACME_MAX					((1 << MV_PP2_CLS4_FDATA7_MACME_BITS) - 1)
#define MV_PP2_CLS4_MACME_MASK					(MV_PP2_CLS4_MACME_MAX << MV_PP2_CLS4_FDATA7_MACME)
#define MV_PP2_CLS4_MACME_VAL(reg_val)				(((reg_val) & MV_PP2_CLS4_MACME_MASK) >> MV_PP2_CLS4_FDATA7_MACME)

#define MV_PP2_CLS4_FDATA7_PPPOE				26
#define MV_PP2_CLS4_FDATA7_PPPOE_BITS				2
#define MV_PP2_CLS4_PPPOE_MAX					((1 << MV_PP2_CLS4_FDATA7_PPPOE_BITS) - 1)
#define MV_PP2_CLS4_PPPOE_MASK					(MV_PP2_CLS4_PPPOE_MAX << MV_PP2_CLS4_FDATA7_PPPOE)
#define MV_PP2_CLS4_PPPOE_VAL(reg_val)				(((reg_val) & MV_PP2_CLS4_PPPOE_MASK) >> MV_PP2_CLS4_FDATA7_PPPOE)

#define MV_PP2_CLS4_FDATA7_VLAN					28
#define MV_PP2_CLS4_FDATA7_VLAN_BITS				3
#define MV_PP2_CLS4_VLAN_MAX					((1 << MV_PP2_CLS4_FDATA7_VLAN_BITS) - 1)
#define MV_PP2_CLS4_VLAN_MASK					(MV_PP2_CLS4_VLAN_MAX << MV_PP2_CLS4_FDATA7_VLAN)
#define MV_PP2_CLS4_VLAN_VAL(reg_val)				(((reg_val) & MV_PP2_CLS4_VLAN_MASK) >> MV_PP2_CLS4_FDATA7_VLAN)

/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_ACT_REG					(MV_PP2_REG_BASE + 0x1E80)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_ACT_QOS_ATTR_REG				(MV_PP2_REG_BASE + 0x1E84)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS4_ACT_DUP_ATTR_REG				(MV_PP2_REG_BASE + 0x1E88)
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
#define MV_PP2_V1_CNT_IDX_REG				(MV_PP2_REG_BASE + 0x7040)
#define MV_PP2_V1_CNT_IDX_RULE(rule, set)		((rule) << 3 | (set))

#define MV_PP2_V1_CLS_C4_TBL_HIT_REG			(MV_PP2_REG_BASE + 0x7708)

/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine Public APIs			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_C4_GRP_SIZE					(8)
#define MV_PP2_CLS_C4_GRPS_NUM					(8)
#define MV_PP2_CLS_C4_TBL_WORDS					(10)
#define MV_PP2_CLS_C4_TBL_DATA_WORDS				(8)
#define MV_PP2_CLS_C4_SRAM_WORDS				(3)

/* Field 0- 3 */
#define FLD_FMT							"%4.4x"
#define FLD_VAL(field, p)					((p[field/2] >> (16 * (field % 2))) & 0xFFFF)

/* field 4 */
#define FLD4_FMT						"%8.8x %8.8x %8.8x %8.8x"
#define FLD4_VAL(p)						p[2], p[3], p[4], p[5]
/* field 5 */
#define FLD5_FMT						"%4.4x %8.8x"
#define FLD5_VAL(p)						p[6] & 0xFFFF, p[7]

#define MV_PP2_CLS_C4_FIELDS_NUM				6
#define GET_FIELD_ATRR(field)					((field) / 3)

typedef struct mvPp2ClsC4RuleEntry {
	unsigned int ruleIndex;
	unsigned int setIndex;
	union {
		MV_U32	words[MV_PP2_CLS_C4_TBL_WORDS];
		struct {
			MV_U32 attr[MV_PP2_CLS4_FATTR_REG_NUM];
			MV_U32 fdataArr[MV_PP2_CLS_C4_TBL_DATA_WORDS];
		} regs;
	} rules;
	union {
		MV_U32 words[MV_PP2_CLS_C4_SRAM_WORDS];
		struct {
			MV_U32 actions;/* 0x1E80 */
			MV_U32 qos_attr;/* 0x1E84*/
			MV_U32 dup_attr;/* 0x1E88 */
		} regs;
	} sram;
} MV_PP2_CLS_C4_ENTRY;


int mvPp2ClsC4HwPortToRulesSet(int port, int set, int rules);
int mvPp2ClsC4HwUniToRulesSet(int uniPort, int set, int rules);
int mvPp2ClsC4HwPortToRulesGet(int port, int *set, int *rules);
int mvPp2ClsC4HwUniToRulesGet(int uni, int *set, int *rules);
int mvPp2ClsC4HwRead(MV_PP2_CLS_C4_ENTRY *C4, int rule, int set);
int mvPp2ClsC4HwWrite(MV_PP2_CLS_C4_ENTRY *C4, int rule, int set);
int mvPp2ClsC4SwDump(MV_PP2_CLS_C4_ENTRY *C4);
void mvPp2ClsC4SwClear(MV_PP2_CLS_C4_ENTRY *C4);
void mvPp2ClsC4HwClearAll(void);
int mvPp2ClsC4RegsDump(void);
int mvPp2V1ClsC4HwHitsDump(void);
int mvPp2ClsC4HwDumpAll(void);


/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine rules APIs	 			 */
/*-------------------------------------------------------------------------------*/


int mvPp2ClsC4FieldsShortSet(MV_PP2_CLS_C4_ENTRY *C4, int field, unsigned int offs, unsigned short data);
int mvPp2ClsC4FieldsParamsSet(MV_PP2_CLS_C4_ENTRY *C4, int field, unsigned int id, unsigned int op);
int mvPp2ClsC4SwVlanSet(MV_PP2_CLS_C4_ENTRY *C4, int vlan);
int mvPp2ClsC4SwPppoeSet(MV_PP2_CLS_C4_ENTRY *C4, int pppoe);
int mvPp2ClsC4SwMacMeSet(MV_PP2_CLS_C4_ENTRY *C4, int mac);
int mvPp2ClsC4SwL4InfoSet(MV_PP2_CLS_C4_ENTRY *C4, int info);
int mvPp2ClsC4SwL3InfoSet(MV_PP2_CLS_C4_ENTRY *C4, int info);


/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine action table APIs 			 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC4ColorSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd);
int mvPp2ClsC4PrioSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int prio);
int mvPp2ClsC4DscpSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int dscp);
int mvPp2ClsC4GpidSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int gpid);
int mvPp2ClsC4ForwardSet(MV_PP2_CLS_C4_ENTRY *c4, int cmd);
#ifdef CONFIG_MV_ETH_PP2_1
int mvPp2ClsC4PolicerSet(MV_PP2_CLS_C4_ENTRY *c2, int cmd, int policerId, int bank);
#else
int mvPp2ClsC4PolicerSet(MV_PP2_CLS_C4_ENTRY *c2, int cmd, int policerId);
#endif
int mvPp2ClsC4QueueHighSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue);
int mvPp2ClsC4QueueLowSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue);
int mvPp2ClsC4QueueSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue);
/*
  PPv2.1 (feature MAS 3.9) Add forwarding command to C4
*/
int mvPp2ClsC4ForwardSet(MV_PP2_CLS_C4_ENTRY *c4, int cmd);

#endif /* MV_CLS4_HW */
