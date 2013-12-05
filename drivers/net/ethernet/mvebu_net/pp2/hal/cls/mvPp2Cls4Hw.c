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
#include "mvPp2Cls4Hw.h"

/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine Public APIs			 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC4HwPortToRulesSet(int port, int set, int rules)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS-1);
	POS_RANGE_VALIDATE(set, MV_PP2_CLS_C4_GRPS_NUM-1);
	RANGE_VALIDATE(rules, 1, MV_PP2_CLS_C4_GRP_SIZE);

	regVal = (set << MV_PP2_CLS4_PHY_TO_RL_GRP) | (rules << MV_PP2_CLS4_PHY_TO_RL_RULE_NUM);
	mvPp2WrReg(MV_PP2_CLS4_PHY_TO_RL_REG(port), regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwUniToRulesSet(int uni, int set, int rules)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(uni, UNI_MAX);
	POS_RANGE_VALIDATE(set, MV_PP2_CLS_C4_GRPS_NUM-1);
	RANGE_VALIDATE(rules, 1, MV_PP2_CLS_C4_GRP_SIZE);

	regVal = (set << MV_PP2_CLS4_PHY_TO_RL_GRP) | (rules << MV_PP2_CLS4_PHY_TO_RL_RULE_NUM);
	mvPp2WrReg(MV_PP2_CLS4_UNI_TO_RL_REG(uni), regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwPortToRulesGet(int port, int *set, int *rules)
{
	unsigned int regVal;

	PTR_VALIDATE(set);
	PTR_VALIDATE(rules);

	regVal = mvPp2RdReg(MV_PP2_CLS4_PHY_TO_RL_REG(port));

	*rules = (regVal & MV_PP2_CLS4_PHY_TO_RL_RULE_NUM_MASK) >> MV_PP2_CLS4_PHY_TO_RL_RULE_NUM;
	*set = (regVal & MV_PP2_CLS4_PHY_TO_RL_GRP_MASK) >> MV_PP2_CLS4_PHY_TO_RL_GRP;

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwUniToRulesGet(int uni, int *set, int *rules)
{
	unsigned int regVal;

	PTR_VALIDATE(set);
	PTR_VALIDATE(rules);


	regVal = mvPp2RdReg(MV_PP2_CLS4_UNI_TO_RL_REG(uni));

	*rules = (regVal & MV_PP2_CLS4_PHY_TO_RL_RULE_NUM_MASK) >> MV_PP2_CLS4_PHY_TO_RL_RULE_NUM;
	*set = (regVal & MV_PP2_CLS4_PHY_TO_RL_GRP_MASK) >> MV_PP2_CLS4_PHY_TO_RL_GRP;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwRead(MV_PP2_CLS_C4_ENTRY *C4, int rule, int set)
{
	unsigned int regVal = 0;
	int regInd;

	PTR_VALIDATE(C4);

	POS_RANGE_VALIDATE(rule, (MV_PP2_CLS_C4_GRP_SIZE-1));
	POS_RANGE_VALIDATE(set, (MV_PP2_CLS_C4_GRPS_NUM-1));

	/* write index reg */
	regVal = (set << MV_PP2_CLS4_RL_INDEX_GRP) | (rule << MV_PP2_CLS4_RL_INDEX_RULE);
	mvPp2WrReg(MV_PP2_CLS4_RL_INDEX_REG, regVal);

	C4->ruleIndex = rule;
	C4->setIndex = set;
	/* read entry rule fields*/
	C4->rules.regs.attr[0] = mvPp2RdReg(MV_PP2_CLS4_FATTR1_REG);
	C4->rules.regs.attr[1] = mvPp2RdReg(MV_PP2_CLS4_FATTR2_REG);

	for (regInd = 0; regInd < MV_PP2_CLS_C4_TBL_DATA_WORDS; regInd++)
		C4->rules.regs.fdataArr[regInd] = mvPp2RdReg(MV_PP2_CLS4_FDATA_REG(regInd));

	/* read entry from action table */
	C4->sram.regs.actions = mvPp2RdReg(MV_PP2_CLS4_ACT_REG);
	C4->sram.regs.qos_attr = mvPp2RdReg(MV_PP2_CLS4_ACT_QOS_ATTR_REG);
	C4->sram.regs.dup_attr = mvPp2RdReg(MV_PP2_CLS4_ACT_DUP_ATTR_REG);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwWrite(MV_PP2_CLS_C4_ENTRY *C4, int rule, int set)
{
	unsigned int regVal = 0;
	int regInd;

	PTR_VALIDATE(C4);


	POS_RANGE_VALIDATE(rule, (MV_PP2_CLS_C4_GRP_SIZE-1));
	POS_RANGE_VALIDATE(set, (MV_PP2_CLS_C4_GRPS_NUM-1));

	/* write index reg */
	regVal = (set << MV_PP2_CLS4_RL_INDEX_GRP) | (rule << MV_PP2_CLS4_RL_INDEX_RULE);
	mvPp2WrReg(MV_PP2_CLS4_RL_INDEX_REG, regVal);

	mvPp2WrReg(MV_PP2_CLS4_FATTR1_REG, C4->rules.regs.attr[0]);
	mvPp2WrReg(MV_PP2_CLS4_FATTR2_REG, C4->rules.regs.attr[1]);

	for (regInd = 0; regInd < MV_PP2_CLS_C4_TBL_DATA_WORDS; regInd++)
		mvPp2WrReg(MV_PP2_CLS4_FDATA_REG(regInd), C4->rules.regs.fdataArr[regInd]);

	/* read entry from action table */
	mvPp2WrReg(MV_PP2_CLS4_ACT_REG, C4->sram.regs.actions);
	mvPp2WrReg(MV_PP2_CLS4_ACT_QOS_ATTR_REG, C4->sram.regs.qos_attr);
	mvPp2WrReg(MV_PP2_CLS4_ACT_DUP_ATTR_REG, C4->sram.regs.dup_attr);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwDump(MV_PP2_CLS_C4_ENTRY *C4)
{
	int index;

	PTR_VALIDATE(C4);

	mvOsPrintf("SET: %d	RULE: %d\n", C4->setIndex, C4->ruleIndex);
	mvOsPrintf("FIELD  ID  OP	DATA\n");

	/*------------------------------*/
	/*	   fields 0-2		*/
	/*------------------------------*/

	for (index = 0; index <  4; index++) {
		mvOsPrintf("%d       %d  %d	",
				index,
				MV_PP2_CLS4_FATTR_ID_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]),
				MV_PP2_CLS4_FATTR_OPCODE_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]));

		mvOsPrintf(FLD_FMT, FLD_VAL(index, C4->rules.regs.fdataArr));
		mvOsPrintf("\n");
	}

	/*------------------------------*/
	/*	   field 4		*/
	/*------------------------------*/

	/* index = 4 after loop */
	mvOsPrintf("%d       %d  %d	",
			index,
			MV_PP2_CLS4_FATTR_ID_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]),
			MV_PP2_CLS4_FATTR_OPCODE_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]));
	mvOsPrintf(FLD4_FMT, FLD4_VAL(C4->rules.regs.fdataArr));
	mvOsPrintf("\n");

	/*------------------------------*/
	/*	   field 5		*/
	/*------------------------------*/
	index++;

	mvOsPrintf("%d       %d  %d	",
			index,
			MV_PP2_CLS4_FATTR_ID_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]),
			MV_PP2_CLS4_FATTR_OPCODE_VAL(index, C4->rules.regs.attr[GET_FIELD_ATRR(index)]));
	mvOsPrintf(FLD5_FMT, FLD5_VAL(C4->rules.regs.fdataArr));
	mvOsPrintf("\n");
	mvOsPrintf("\n");
	mvOsPrintf("VLAN: %d  PPPOE: %d  MACME: %d  L4INFO: %d  L3INFO: %d\n",
			MV_PP2_CLS4_VLAN_VAL(C4->rules.regs.fdataArr[6]),
			MV_PP2_CLS4_PPPOE_VAL(C4->rules.regs.fdataArr[6]),
			MV_PP2_CLS4_MACME_VAL(C4->rules.regs.fdataArr[6]),
			MV_PP2_CLS4_L4INFO_VAL(C4->rules.regs.fdataArr[6]),
			MV_PP2_CLS4_L3INFO_VAL(C4->rules.regs.fdataArr[6]));
	mvOsPrintf("\n");
	/*------------------------------*/
	/*	actions	0x1E80		*/
	/*------------------------------*/
/*
  PPv2.1 (feature MAS 3.9) Add forwarding command to C4
*/

#ifdef CONFIG_MV_ETH_PP2_1
	mvOsPrintf("ACT_TBL:COLOR	PRIO	DSCP	GPID	LOW_Q	HIGH_Q	POLICER		FWD\n");
	mvOsPrintf("CMD:    [%1d]	[%1d]	[%1d]	[%1d]	[%1d]	[%1d]	[%1d]		[%1d]\n",
			((C4->sram.regs.actions & (ACT_COLOR_MASK)) >> ACT_COLOR),
			((C4->sram.regs.actions & (ACT_PRI_MASK)) >> ACT_PRI),
			((C4->sram.regs.actions & (ACT_DSCP_MASK)) >> ACT_DSCP),
			((C4->sram.regs.actions & (ACT_GEM_ID_MASK)) >> ACT_GEM_ID),
			((C4->sram.regs.actions & (ACT_LOW_Q_MASK)) >> ACT_LOW_Q),
			((C4->sram.regs.actions & (ACT_HIGH_Q_MASK)) >> ACT_HIGH_Q),
			((C4->sram.regs.actions & (ACT_POLICER_SELECT_MASK)) >> ACT_POLICER_SELECT),
			((C4->sram.regs.actions & ACT_FWD_MASK) >> ACT_FWD));


	/*------------------------------*/
	/*	qos_attr 0x1E84		*/
	/*------------------------------*/
	/*mvOsPrintf("ACT_TBL:COLOR	PRIO	DSCP	GPID	LOW_Q	HIGH_Q	POLICER		FWD\n");*/

	/*mvOsPrintf("VAL:		PRIO	DSCP	GPID	LOW_Q	HIGH_Q	 POLICER\n");*/

	mvOsPrintf("VAL:		[%1d]	[%1d]	[%1d]	[%1d]	[0x%x]	[id 0x%2.2x bank %1.1x]\n",
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_PRI_MASK)) >> ACT_QOS_ATTR_MDF_PRI),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_DSCP_MASK)) >> ACT_QOS_ATTR_MDF_DSCP),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_GEM_ID_MASK)) >> ACT_QOS_ATTR_MDF_GEM_ID),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_LOW_Q_MASK)) >> ACT_QOS_ATTR_MDF_LOW_Q),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_HIGH_Q_MASK)) >> ACT_QOS_ATTR_MDF_HIGH_Q),
			((C4->sram.regs.dup_attr & (ACT_DUP_POLICER_MASK)) >> ACT_DUP_POLICER_ID),
			((C4->sram.regs.dup_attr & ACT_DUP_POLICER_BANK_MASK) >> ACT_DUP_POLICER_BANK_BIT));

#else
	mvOsPrintf("ACT_TBL:    COLOR   PRIO    DSCP    GPID    LOW_Q   HIGH_Q  POLICER\n");
	mvOsPrintf("CMD:                [%1d]   [%1d]   [%1d]   [%1d]   [%1d]   [%1d]   [%1d]\n",
			((C4->sram.regs.actions & (ACT_COLOR_MASK)) >> ACT_COLOR),
			((C4->sram.regs.actions & (ACT_PRI_MASK)) >> ACT_PRI),
			((C4->sram.regs.actions & (ACT_DSCP_MASK)) >> ACT_DSCP),
			((C4->sram.regs.actions & (ACT_GEM_ID_MASK)) >> ACT_GEM_ID),
			((C4->sram.regs.actions & (ACT_LOW_Q_MASK)) >> ACT_LOW_Q),
			((C4->sram.regs.actions & (ACT_HIGH_Q_MASK)) >> ACT_HIGH_Q),
			((C4->sram.regs.actions & (ACT_POLICER_SELECT_MASK)) >> ACT_POLICER_SELECT));


	/*------------------------------*/
	/*	qos_attr 0x1E84		*/
	/*------------------------------*/
	/*mvOsPrintf("VAL:		PRIO	DSCP	GPID	LOW_Q	HIGH_Q	 POLICER\n");*/

	mvOsPrintf("VAL:                    [%1d]	[%1d]	[%1d]	[%1d]	[0x%x]	[%1d]\n",
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_PRI_MASK)) >> ACT_QOS_ATTR_MDF_PRI),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_DSCP_MASK)) >> ACT_QOS_ATTR_MDF_DSCP),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_GEM_ID_MASK)) >> ACT_QOS_ATTR_MDF_GEM_ID),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_LOW_Q_MASK)) >> ACT_QOS_ATTR_MDF_LOW_Q),
			((C4->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_HIGH_Q_MASK)) >> ACT_QOS_ATTR_MDF_HIGH_Q),
			((C4->sram.regs.dup_attr & (ACT_DUP_POLICER_MASK)) >> ACT_DUP_POLICER_ID));

#endif



	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/* PPv2.1 MASS 3.20 new feature */
static int mvPp2V1ClsC4HwCntDump(int rule, int set, unsigned int *cnt)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(rule, (MV_PP2_CLS_C4_GRP_SIZE-1));
	POS_RANGE_VALIDATE(set, (MV_PP2_CLS_C4_GRPS_NUM-1));

	/* write index */
	regVal =  MV_PP2_V1_CNT_IDX_RULE(rule, set);
	mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, regVal);

	/*read hit counter*/
	regVal = mvPp2RdReg(MV_PP2_V1_CLS_C4_TBL_HIT_REG);

	if (cnt)
		*cnt = regVal;
	else
		mvOsPrintf("HIT COUNTER: %d\n", regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4HwDumpAll()
{
	int set, rule;
	MV_PP2_CLS_C4_ENTRY C4;

	for (set = 0; set < MV_PP2_CLS_C4_GRPS_NUM; set++)
		for (rule = 0; rule <  MV_PP2_CLS_C4_GRP_SIZE; rule++) {
			mvPp2ClsC4HwRead(&C4, rule, set);
			mvPp2ClsC4SwDump(&C4);
#ifdef CONFIG_MV_ETH_PP2_1
			mvPp2V1ClsC4HwCntDump(rule, set, NULL);
#endif
			mvOsPrintf("--------------------------------------------------------------------\n");
		}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/* mvPp2V1ClsC4HwHitsDump - dump all non zeroed hit counters and the associated  HWentries */
/* PPv2.1 MASS 3.20 new feature */
int mvPp2V1ClsC4HwHitsDump()
{
	int set, rule;
	unsigned int cnt;
	MV_PP2_CLS_C4_ENTRY C4;

	for (set = 0; set < MV_PP2_CLS_C4_GRPS_NUM; set++)
		for (rule = 0; rule <  MV_PP2_CLS_C4_GRP_SIZE; rule++) {
			mvPp2V1ClsC4HwCntDump(rule, set, &cnt);
			if (cnt == 0)
				continue;

			mvPp2ClsC4HwRead(&C4, rule, set);
			mvPp2ClsC4SwDump(&C4);
			mvOsPrintf("HITS: %d\n", cnt);
			mvOsPrintf("--------------------------------------------------------------------\n");
		}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4RegsDump()
{
	int i = 0;
	char reg_name[100];


	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS4_PHY_TO_RL_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS4_PHY_TO_RL_REG(i), reg_name);
	}

	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS4_UNI_TO_RL_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS4_UNI_TO_RL_REG(i), reg_name);
	}

	mvPp2PrintReg(MV_PP2_CLS4_FATTR1_REG, "MV_PP2_CLS4_FATTR1_REG");
	mvPp2PrintReg(MV_PP2_CLS4_FATTR2_REG, "MV_PP2_CLS4_FATTR2_REG");

	for (i = 0; i < MV_PP2_CLS4_FDATA_REGS_NUM; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS4_FDATA_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS4_FDATA_REG(i), reg_name);
	}

	mvPp2PrintReg(MV_PP2_CLS4_RL_INDEX_REG, "MV_PP2_CLS4_RL_INDEX_REG");
	mvPp2PrintReg(MV_PP2_CLS4_ACT_REG, "MV_PP2_CLS4_ACT_REG");
	mvPp2PrintReg(MV_PP2_CLS4_ACT_QOS_ATTR_REG, "MV_PP2_CLS4_ACT_QOS_ATTR_REG");
	mvPp2PrintReg(MV_PP2_CLS4_ACT_DUP_ATTR_REG, "MV_PP2_CLS4_ACT_DUP_ATTR_REG");

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
void mvPp2ClsC4SwClear(MV_PP2_CLS_C4_ENTRY *C4)
{
	memset(C4, 0, sizeof(MV_PP2_CLS_C4_ENTRY));
}

/*-------------------------------------------------------------------------------*/
void mvPp2ClsC4HwClearAll()
{
	int set, rule;
	MV_PP2_CLS_C4_ENTRY C4;

	mvPp2ClsC4SwClear(&C4);

	for (set = 0; set < MV_PP2_CLS_C4_GRPS_NUM; set++)
		for (rule = 0; rule <  MV_PP2_CLS_C4_GRP_SIZE; rule++)
			mvPp2ClsC4HwWrite(&C4, rule, set);
}
/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine rules APIs	 			 */
/*-------------------------------------------------------------------------------*/

/*
set two bytes of data in fields
offs - offset in byte resolution
*/
int mvPp2ClsC4FieldsShortSet(MV_PP2_CLS_C4_ENTRY *C4, int field, unsigned int offs, unsigned short data)
{
	PTR_VALIDATE(C4);

	POS_RANGE_VALIDATE(field, MV_PP2_CLS_C4_FIELDS_NUM-1);

	if ((offs % 2) != 0) {
		mvOsPrintf("mvCls4Hw %s: offset should be even , current func write two bytes of data.\n", __func__);
		return MV_CLS4_ERR;
	}

	if (field < 4) {
		/* fields 0,1,2,3 lenght is 2 bytes */
		POS_RANGE_VALIDATE(offs, 0);
		C4->rules.regs.fdataArr[field/2] &= ~(0xFFFF << (16 * (field % 2)));
		C4->rules.regs.fdataArr[field/2] |= (data << (16 * (field % 2)));
	}

	else if (field == 4) {
		/* field 4 lenght is 16 bytes */
		POS_RANGE_VALIDATE(offs, 14);
		C4->rules.regs.fdataArr[5 - offs/4] &= ~(0xFFFF << (16 * ((offs / 2) % 2)));
		C4->rules.regs.fdataArr[5 - offs/4] |= (data << (16 * ((offs / 2) % 2)));
	} else {
		/* field 5 lenght is 6 bytes */
		POS_RANGE_VALIDATE(offs, 4);
		C4->rules.regs.fdataArr[7 - offs/4] &= ~(0xFFFF << (16 * ((offs / 2) % 2)));
		C4->rules.regs.fdataArr[7 - offs/4] |= (data << (16 * ((offs / 2) % 2)));
	}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4FieldsParamsSet(MV_PP2_CLS_C4_ENTRY *C4, int field, unsigned int id, unsigned int op)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(field, MV_PP2_CLS_C4_FIELDS_NUM-1);
	POS_RANGE_VALIDATE(id, MV_PP2_CLS4_FATTR_ID_MAX);
	POS_RANGE_VALIDATE(op, MV_PP2_CLS4_FATTR_OPCODE_MAX);

	/* clear old ID and opcode*/
	C4->rules.regs.attr[GET_FIELD_ATRR(field)] &= ~MV_PP2_CLS4_FATTR_ID_MASK(field);
	C4->rules.regs.attr[GET_FIELD_ATRR(field)] &= ~MV_PP2_CLS4_FATTR_OPCODE_MASK(field);

	/* write new values */
	C4->rules.regs.attr[GET_FIELD_ATRR(field)] |=  (op << MV_PP2_CLS4_FATTR_OPCODE(field));
	C4->rules.regs.attr[GET_FIELD_ATRR(field)] |= 	(id << MV_PP2_CLS4_FATTR_ID(field));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwVlanSet(MV_PP2_CLS_C4_ENTRY *C4, int vlan)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(vlan, MV_PP2_CLS4_VLAN_MAX);

	C4->rules.regs.fdataArr[6] &= ~MV_PP2_CLS4_VLAN_MASK;
	C4->rules.regs.fdataArr[6] |= (vlan << MV_PP2_CLS4_FDATA7_VLAN);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwPppoeSet(MV_PP2_CLS_C4_ENTRY *C4, int pppoe)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(pppoe, MV_PP2_CLS4_PPPOE_MAX);

	C4->rules.regs.fdataArr[6] &= ~MV_PP2_CLS4_PPPOE_MASK;
	C4->rules.regs.fdataArr[6] |= (pppoe << MV_PP2_CLS4_FDATA7_PPPOE);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwMacMeSet(MV_PP2_CLS_C4_ENTRY *C4, int mac)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(mac, MV_PP2_CLS4_MACME_MAX);

	C4->rules.regs.fdataArr[6] &= ~MV_PP2_CLS4_MACME_MASK;
	C4->rules.regs.fdataArr[6] |= (mac << MV_PP2_CLS4_FDATA7_MACME);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwL4InfoSet(MV_PP2_CLS_C4_ENTRY *C4, int info)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(info, MV_PP2_CLS4_L4INFO_MAX);

	C4->rules.regs.fdataArr[6] &= ~MV_PP2_CLS4_L4INFO_MASK;
	C4->rules.regs.fdataArr[6] |= (info << MV_PP2_CLS4_FDATA7_L4INFO);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4SwL3InfoSet(MV_PP2_CLS_C4_ENTRY *C4, int info)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(info, MV_PP2_CLS4_L3INFO_MAX);

	C4->rules.regs.fdataArr[6] &= ~MV_PP2_CLS4_L3INFO_MASK;
	C4->rules.regs.fdataArr[6] |= (info << MV_PP2_CLS4_FDATA7_L3INFO);

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/
/*			Classifier C4 engine Public action table APIs 		 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC4ColorSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, COLOR_RED_AND_LOCK);

	C4->sram.regs.actions &= ~ACT_COLOR_MASK;
	C4->sram.regs.actions |= (cmd << ACT_COLOR);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4PrioSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int prio)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(prio, ACT_QOS_ATTR_PRI_MAX);

	/*set command*/
	C4->sram.regs.actions &= ~ACT_PRI_MASK;
	C4->sram.regs.actions |= (cmd << ACT_PRI);

	/*set modify priority value*/
	C4->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_PRI_MASK;
	C4->sram.regs.qos_attr |= (prio << ACT_QOS_ATTR_MDF_PRI);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4DscpSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int dscp)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(dscp, ACT_QOS_ATTR_DSCP_MAX);

	/*set command*/
	C4->sram.regs.actions &= ~ACT_DSCP_MASK;
	C4->sram.regs.actions |= (cmd << ACT_DSCP);

	/*set modify DSCP value*/
	C4->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_DSCP_MASK;
	C4->sram.regs.qos_attr |= (dscp << ACT_QOS_ATTR_MDF_DSCP);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC4GpidSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int gid)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(gid, ACT_QOS_ATTR_GEM_ID_MAX);

	/*set command*/
	C4->sram.regs.actions &= ~ACT_GEM_ID_MASK;
	C4->sram.regs.actions |= (cmd << ACT_GEM_ID);

	/*set modify DSCP value*/
	C4->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_GEM_ID_MASK;
	C4->sram.regs.qos_attr |= (gid << ACT_QOS_ATTR_MDF_GEM_ID);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
int mvPp2ClsC4PolicerSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int policerId, int bank)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);
	BIT_RANGE_VALIDATE(bank);

	C4->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	C4->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	C4->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	C4->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);

	if (bank)
		C4->sram.regs.dup_attr |= ACT_DUP_POLICER_BANK_MASK;
	else
		C4->sram.regs.dup_attr &= ~ACT_DUP_POLICER_BANK_MASK;

	return MV_OK;
}
#else
int mvPp2ClsC4PolicerSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int policerId)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);

	C4->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	C4->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	C4->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	C4->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);
	return MV_OK;
}
#endif /*CONFIG_MV_ETH_PP2_1*/


/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4QueueHighSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue)
{
	PTR_VALIDATE(C4);


	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_HIGH_Q_MAX);

	/*set command*/
	C4->sram.regs.actions &= ~ACT_HIGH_Q_MASK;
	C4->sram.regs.actions |= (cmd << ACT_HIGH_Q);

	/*set modify High queue value*/
	C4->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_HIGH_Q_MASK;
	C4->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_HIGH_Q);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC4QueueLowSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue)
{
	PTR_VALIDATE(C4);

	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_LOW_Q_MAX);

	/*set command*/
	C4->sram.regs.actions &= ~ACT_LOW_Q_MASK;
	C4->sram.regs.actions |= (cmd << ACT_LOW_Q);

	/*set modify High queue value*/
	C4->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_LOW_Q_MASK;
	C4->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_LOW_Q);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC4QueueSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd, int queue)
{
	int status = MV_OK;
	int qHigh, qLow;

	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_Q_MAX);

	/* cmd validation in set functions */

	qHigh = (queue & ACT_QOS_ATTR_MDF_HIGH_Q_MASK) >> ACT_QOS_ATTR_MDF_HIGH_Q;
	qLow = (queue & ACT_QOS_ATTR_MDF_LOW_Q_MASK) >> ACT_QOS_ATTR_MDF_LOW_Q;

	status |= mvPp2ClsC4QueueLowSet(C4, cmd, qLow);
	status |= mvPp2ClsC4QueueHighSet(C4, cmd, qHigh);

	return status;

}

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.9) Add forwarding command to C4
*/
int mvPp2ClsC4ForwardSet(MV_PP2_CLS_C4_ENTRY *C4, int cmd)
{
	PTR_VALIDATE(C4);
	POS_RANGE_VALIDATE(cmd, SWF_AND_LOCK);

	C4->sram.regs.actions &= ~ACT_FWD_MASK;
	C4->sram.regs.actions |= (cmd << ACT_FWD);
	return MV_OK;
}

