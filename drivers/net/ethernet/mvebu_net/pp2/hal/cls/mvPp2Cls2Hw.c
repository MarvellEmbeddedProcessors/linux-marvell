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
#include "mvPp2Cls2Hw.h"

/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine QoS table Public APIs			 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosHwRead(int tbl_id, int tbl_sel, int tbl_line, MV_PP2_CLS_C2_QOS_ENTRY *qos)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(qos);

	POS_RANGE_VALIDATE(tbl_sel, 1); /* one bit */
	if (tbl_sel == 1) {
		/*dscp*/
		/* TODO define 8=DSCP_TBL_NUM  64=DSCP_TBL_LINES */
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_DSCP);
		POS_RANGE_VALIDATE(tbl_line, QOS_TBL_LINE_NUM_DSCP);
	} else {
		/*pri*/
		/* TODO define 64=PRI_TBL_NUM  8=PRI_TBL_LINES */
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_PRI);
		POS_RANGE_VALIDATE(tbl_line, QOS_TBL_LINE_NUM_PRI);
	}

	qos->tbl_id = tbl_id;
	qos->tbl_sel = tbl_sel;
	qos->tbl_line = tbl_line;

	/* write index reg */
	regVal |= (tbl_line << MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |= (tbl_sel << MV_PP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (tbl_id << MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);

	mvPp2WrReg(MV_PP2_CLS2_DSCP_PRI_INDEX_REG, regVal);

	/* read data reg*/
	qos->data = mvPp2RdReg(MV_PP2_CLS2_QOS_TBL_REG);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosHwWrite(int tbl_id, int tbl_sel, int tbl_line, MV_PP2_CLS_C2_QOS_ENTRY *qos)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(qos);

	POS_RANGE_VALIDATE(tbl_sel, 1); /* one bit */
	if (tbl_sel == 1) {
		/*dscp*/
		/* TODO define 8=DSCP_TBL_NUM  64=DSCP_TBL_LINES */
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_DSCP);
		POS_RANGE_VALIDATE(tbl_line, QOS_TBL_LINE_NUM_DSCP);
	} else {
		/*pri*/
		/* TODO define 64=PRI_TBL_NUM  8=PRI_TBL_LINES */
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_PRI);
		POS_RANGE_VALIDATE(tbl_line, QOS_TBL_LINE_NUM_PRI);
	}
	/* write index reg */
	regVal |= (tbl_line << MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |= (tbl_sel << MV_PP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (tbl_id << MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);

	mvPp2WrReg(MV_PP2_CLS2_DSCP_PRI_INDEX_REG, regVal);

	/* write data reg*/
	mvPp2WrReg(MV_PP2_CLS2_QOS_TBL_REG, qos->data);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosSwDump(MV_PP2_CLS_C2_QOS_ENTRY *qos)
{
	int int32bit;
	int status = 0;

	PTR_VALIDATE(qos);

	mvOsPrintf("TABLE	SEL	LINE	PRI	DSCP	COLOR	GEM_ID	QUEUE\n");

	/* table id */
	mvOsPrintf("0x%2.2x\t", qos->tbl_id);

	/* table sel */
	mvOsPrintf("0x%1.1x\t", qos->tbl_sel);

	/* table line */
	mvOsPrintf("0x%2.2x\t", qos->tbl_line);

	/* priority */
	status |= mvPp2ClsC2QosPrioGet(qos, &int32bit);
	mvOsPrintf("0x%1.1x\t", int32bit);

	/* dscp */
	status |= mvPp2ClsC2QosDscpGet(qos, &int32bit);
	mvOsPrintf("0x%2.2x\t", int32bit);

	/* color */
	status |= mvPp2ClsC2QosColorGet(qos, &int32bit);
	mvOsPrintf("0x%1.1x\t", int32bit);

	/* gem port id */
	status |= mvPp2ClsC2QosGpidGet(qos, &int32bit);
	mvOsPrintf("0x%3.3x\t", int32bit);

	/* queue */
	status |= mvPp2ClsC2QosQueueGet(qos, &int32bit);
	mvOsPrintf("0x%2.2x", int32bit);

	mvOsPrintf("\n");

	return status;
}
/*-------------------------------------------------------------------------------*/
void 	mvPp2ClsC2QosSwClear(MV_PP2_CLS_C2_QOS_ENTRY *qos)
{

	memset(qos, 0, sizeof(MV_PP2_CLS_C2_QOS_ENTRY));
}
/*-------------------------------------------------------------------------------*/
void 	mvPp2ClsC2QosHwClearAll()
{
	int tbl_id, tbl_line;

	MV_PP2_CLS_C2_QOS_ENTRY c2;

	mvPp2ClsC2QosSwClear(&c2);

	/* clear DSCP tables */
	for (tbl_id = 0; tbl_id < MV_PP2_CLS_C2_QOS_DSCP_TBL_NUM; tbl_id++)
		for (tbl_line = 0; tbl_line < MV_PP2_CLS_C2_QOS_DSCP_TBL_SIZE; tbl_line++)
			mvPp2ClsC2QosHwWrite(tbl_id, 1/*DSCP*/, tbl_line, &c2);

	/* clear PRIO tables */
	for (tbl_id = 0; tbl_id < MV_PP2_CLS_C2_QOS_PRIO_TBL_NUM; tbl_id++)
		for (tbl_line = 0; tbl_line < MV_PP2_CLS_C2_QOS_PRIO_TBL_SIZE; tbl_line++)
			mvPp2ClsC2QosHwWrite(tbl_id, 0/*PRIO*/, tbl_line, &c2);



}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosDscpHwDump(void)
{
	int tbl_id, tbl_line, int32bit;
	MV_PP2_CLS_C2_QOS_ENTRY qos;

	for (tbl_id = 0; tbl_id < MV_PP2_CLS_C2_QOS_DSCP_TBL_NUM; tbl_id++) {

		mvOsPrintf("\n------------ DSCP TABLE %d ------------\n", tbl_id);
		mvOsPrintf("LINE	DSCP	COLOR	GEM_ID	QUEUE\n");
		for (tbl_line = 0; tbl_line < MV_PP2_CLS_C2_QOS_DSCP_TBL_SIZE; tbl_line++) {
			mvPp2ClsC2QosHwRead(tbl_id, 1/*DSCP*/, tbl_line, &qos);
			mvOsPrintf("0x%2.2x\t", qos.tbl_line);
			mvPp2ClsC2QosDscpGet(&qos, &int32bit);
			mvOsPrintf("0x%2.2x\t", int32bit);
			mvPp2ClsC2QosColorGet(&qos, &int32bit);
			mvOsPrintf("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosGpidGet(&qos, &int32bit);
			mvOsPrintf("0x%3.3x\t", int32bit);
			mvPp2ClsC2QosQueueGet(&qos, &int32bit);
			mvOsPrintf("0x%2.2x", int32bit);
			mvOsPrintf("\n");
		}
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosPrioHwDump(void)
{
	int tbl_id, tbl_line, int32bit;

	MV_PP2_CLS_C2_QOS_ENTRY qos;

	for (tbl_id = 0; tbl_id < MV_PP2_CLS_C2_QOS_PRIO_TBL_NUM; tbl_id++) {

		mvOsPrintf("\n-------- PRIORITY TABLE %d -----------\n", tbl_id);
		mvOsPrintf("LINE	PRIO	COLOR	GEM_ID	QUEUE\n");

		for (tbl_line = 0; tbl_line < MV_PP2_CLS_C2_QOS_PRIO_TBL_SIZE; tbl_line++) {
			mvPp2ClsC2QosHwRead(tbl_id, 0/*PRIO*/, tbl_line, &qos);
			mvOsPrintf("0x%2.2x\t", qos.tbl_line);
			mvPp2ClsC2QosPrioGet(&qos, &int32bit);
			mvOsPrintf("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosColorGet(&qos, &int32bit);
			mvOsPrintf("0x%1.1x\t", int32bit);
			mvPp2ClsC2QosGpidGet(&qos, &int32bit);
			mvOsPrintf("0x%3.3x\t", int32bit);
			mvPp2ClsC2QosQueueGet(&qos, &int32bit);
			mvOsPrintf("0x%2.2x", int32bit);
			mvOsPrintf("\n");
		}
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosPrioSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int prio)

{
	PTR_VALIDATE(qos);
	POS_RANGE_VALIDATE(prio, (QOS_TBL_LINE_NUM_PRI-1));

	qos->data &= ~QOS_TBL_PRI_MASK;
	qos->data |= (prio << QOS_TBL_PRI);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosDscpSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int dscp)
{
	PTR_VALIDATE(qos);
	POS_RANGE_VALIDATE(dscp, (QOS_TBL_LINE_NUM_DSCP-1));

	qos->data &= ~QOS_TBL_DSCP_MASK;
	qos->data |= (dscp << QOS_TBL_DSCP);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosColorSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int color)
{
	PTR_VALIDATE(qos);
	POS_RANGE_VALIDATE(color, COLOR_RED_AND_LOCK);

	qos->data &= ~QOS_TBL_COLOR_MASK;
	qos->data |= (color << QOS_TBL_COLOR);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosGpidSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int gpid)
{
	PTR_VALIDATE(qos);
	POS_RANGE_VALIDATE(gpid, ACT_QOS_ATTR_GEM_ID_MAX);

	qos->data &= ~QOS_TBL_GEM_ID_MASK;
	qos->data |= (gpid << QOS_TBL_GEM_ID);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosQueueSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int queue)
{
	PTR_VALIDATE(qos);
	POS_RANGE_VALIDATE(queue, QOS_TBL_Q_NUM_MAX);

	qos->data &= ~QOS_TBL_Q_NUM_MASK;
	qos->data |= (queue << QOS_TBL_Q_NUM);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosPrioGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *prio)
{
	PTR_VALIDATE(qos);
	PTR_VALIDATE(prio);

	*prio = (qos->data & QOS_TBL_PRI_MASK) >> QOS_TBL_PRI ;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosDscpGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *dscp)
{
	PTR_VALIDATE(qos);
	PTR_VALIDATE(dscp);

	*dscp = (qos->data & QOS_TBL_DSCP_MASK) >> QOS_TBL_DSCP;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosColorGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *color)
{
	PTR_VALIDATE(qos);
	PTR_VALIDATE(color);

	*color = (qos->data & QOS_TBL_COLOR_MASK) >> QOS_TBL_COLOR;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosGpidGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *gpid)
{
	PTR_VALIDATE(qos);
	PTR_VALIDATE(gpid);

	*gpid = (qos->data & QOS_TBL_GEM_ID_MASK) >> QOS_TBL_GEM_ID;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosQueueGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *queue)
{
	PTR_VALIDATE(qos);
	PTR_VALIDATE(queue);

	*queue = (qos->data & QOS_TBL_Q_NUM_MASK) >> QOS_TBL_Q_NUM;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine TCAM table Public APIs	    		 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2HwWrite(int index, MV_PP2_CLS_C2_ENTRY *c2)
{
	int TcmIdx;

	PTR_VALIDATE(c2);

	POS_RANGE_VALIDATE(index, (MV_PP2_CLS_C2_TCAM_SIZE-1));

	c2->index = index;

	/* write index reg */
	mvPp2WrReg(MV_PP2_CLS2_TCAM_IDX_REG, index);

	/* write valid bit*/
	c2->inv = 0;
	mvPp2WrReg(MV_PP2_CLS2_TCAM_INV_REG, ((c2->inv) << MV_PP2_CLS2_TCAM_INV_INVALID));

	for (TcmIdx = 0; TcmIdx < MV_PP2_CLS_C2_TCAM_WORDS; TcmIdx++)
		mvPp2WrReg(MV_PP2_CLS2_TCAM_DATA_REG(TcmIdx), c2->tcam.words[TcmIdx]);

	/* write action_tbl 0x1B30 */
	mvPp2WrReg(MV_PP2_CLS2_ACT_DATA_REG, c2->sram.regs.action_tbl);

	/* write actions 0x1B60 */
	mvPp2WrReg(MV_PP2_CLS2_ACT_REG, c2->sram.regs.actions);

	/* write qos_attr 0x1B64 */
	mvPp2WrReg(MV_PP2_CLS2_ACT_QOS_ATTR_REG, c2->sram.regs.qos_attr);

	/* write hwf_attr 0x1B68 */
	mvPp2WrReg(MV_PP2_CLS2_ACT_HWF_ATTR_REG, c2->sram.regs.hwf_attr);

	/* write dup_attr 0x1B6C */
	mvPp2WrReg(MV_PP2_CLS2_ACT_DUP_ATTR_REG, c2->sram.regs.dup_attr);
#ifdef CONFIG_MV_ETH_PP2_1
	/* write seq_attr 0x1B70 */
	mvPp2WrReg(MV_PP2_CLS2_ACT_SEQ_ATTR_REG, c2->sram.regs.seq_attr);
#endif
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

/*
 note: error is not returned if entry is invalid
 user should check c2->valid afer returned from this func
*/
int mvPp2ClsC2HwRead(int index, MV_PP2_CLS_C2_ENTRY *c2)
{
	unsigned int regVal;
	int	TcmIdx;

	PTR_VALIDATE(c2);

	c2->index = index;

	/* write index reg */
	mvPp2WrReg(MV_PP2_CLS2_TCAM_IDX_REG, index);

	/* read inValid bit*/
	regVal = mvPp2RdReg(MV_PP2_CLS2_TCAM_INV_REG);
	c2->inv = (regVal & MV_PP2_CLS2_TCAM_INV_INVALID_MASK) >> MV_PP2_CLS2_TCAM_INV_INVALID;

	if (c2->inv)
		return MV_OK;

	for (TcmIdx = 0; TcmIdx < MV_PP2_CLS_C2_TCAM_WORDS; TcmIdx++)
		c2->tcam.words[TcmIdx] = mvPp2RdReg(MV_PP2_CLS2_TCAM_DATA_REG(TcmIdx));

	/* read action_tbl 0x1B30 */
	c2->sram.regs.action_tbl = mvPp2RdReg(MV_PP2_CLS2_ACT_DATA_REG);

	/* read actions 0x1B60 */
	c2->sram.regs.actions = mvPp2RdReg(MV_PP2_CLS2_ACT_REG);

	/* read qos_attr 0x1B64 */
	c2->sram.regs.qos_attr = mvPp2RdReg(MV_PP2_CLS2_ACT_QOS_ATTR_REG);

	/* read hwf_attr 0x1B68 */
	c2->sram.regs.hwf_attr = mvPp2RdReg(MV_PP2_CLS2_ACT_HWF_ATTR_REG);

	/* read dup_attr 0x1B6C */
	c2->sram.regs.dup_attr = mvPp2RdReg(MV_PP2_CLS2_ACT_DUP_ATTR_REG);

#ifdef CONFIG_MV_ETH_PP2_1
	/* read seq_attr 0x1B70 */
	c2->sram.regs.seq_attr = mvPp2RdReg(MV_PP2_CLS2_ACT_SEQ_ATTR_REG);
#endif

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2SwWordsDump(MV_PP2_CLS_C2_ENTRY *c2)
{
	int i;

	PTR_VALIDATE(c2);

	/* TODO check size */
	/* hw entry id */
	mvOsPrintf("[0x%3.3x] ", c2->index);

	i = MV_PP2_CLS_C2_TCAM_WORDS - 1 ;

	while (i >= 0)
		mvOsPrintf("%4.4x ", (MV_32BIT_LE_FAST(c2->tcam.words[i--])) & 0xFFFF);

	mvOsPrintf("| ");

	mvOsPrintf(C2_SRAM_FMT, C2_SRAM_VAL(c2->sram.words));

	/*tcam inValid bit*/
	mvOsPrintf(" %s", (c2->inv == 1) ? "[inv]" : "[valid]");

	mvOsPrintf("\n        ");

	i = MV_PP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		mvOsPrintf("%4.4x ", ((MV_32BIT_LE_FAST(c2->tcam.words[i--]) >> 16)  & 0xFFFF));

	mvOsPrintf("\n");

	return MV_OK;
}


/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2SwDump(MV_PP2_CLS_C2_ENTRY *c2)
{
	int id, sel, type, gemid, low_q, high_q, color, int32bit;

	PTR_VALIDATE(c2);

	mvPp2ClsC2SwWordsDump(c2);
	mvOsPrintf("\n");

	/*------------------------------*/
	/*	action_tbl 0x1B30	*/
	/*------------------------------*/

	id =  ((c2->sram.regs.action_tbl & (ACT_TBL_ID_MASK)) >> ACT_TBL_ID);
	sel =  ((c2->sram.regs.action_tbl & (ACT_TBL_SEL_MASK)) >> ACT_TBL_SEL);
	type =	((c2->sram.regs.action_tbl & (ACT_TBL_PRI_DSCP_MASK)) >> ACT_TBL_PRI_DSCP);
	gemid = ((c2->sram.regs.action_tbl & (ACT_TBL_GEM_ID_MASK)) >> ACT_TBL_GEM_ID);
	low_q = ((c2->sram.regs.action_tbl & (ACT_TBL_LOW_Q_MASK)) >> ACT_TBL_LOW_Q);
	high_q = ((c2->sram.regs.action_tbl & (ACT_TBL_HIGH_Q_MASK)) >> ACT_TBL_HIGH_Q);
	color =  ((c2->sram.regs.action_tbl & (ACT_TBL_COLOR_MASK)) >> ACT_TBL_COLOR);

	mvOsPrintf("FROM_QOS_%s_TBL[%2.2d]:  ", sel ? "DSCP" : "PRI", id);
	type ? mvOsPrintf("%s	", sel ? "DSCP" : "PRIO") : 0;
	color ? mvOsPrintf("COLOR	") : 0;
	gemid ? mvOsPrintf("GEMID	") : 0;
	low_q ? mvOsPrintf("LOW_Q	") : 0;
	high_q ? mvOsPrintf("HIGH_Q	") : 0;
	mvOsPrintf("\n");

	mvOsPrintf("FROM_ACT_TBL:		");
	(type == 0) ? mvOsPrintf("%s 	", sel ? "DSCP" : "PRI") : 0;
	(gemid == 0) ? mvOsPrintf("GEMID	") : 0;
	(low_q == 0) ? mvOsPrintf("LOW_Q	") : 0;
	(high_q == 0) ? mvOsPrintf("HIGH_Q	") : 0;
	(color == 0) ? mvOsPrintf("COLOR	") : 0;
	mvOsPrintf("\n\n");

	/*------------------------------*/
	/*	actions 0x1B60		*/
	/*------------------------------*/

	mvOsPrintf("ACT_CMD:		COLOR	PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	FWD	POLICER	FID\n");
	mvOsPrintf("			");

	mvOsPrintf("%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t%1.1d\t",
			((c2->sram.regs.actions & ACT_COLOR_MASK) >> ACT_COLOR),
			((c2->sram.regs.actions & ACT_PRI_MASK) >> ACT_PRI),
			((c2->sram.regs.actions & ACT_DSCP_MASK) >> ACT_DSCP),
			((c2->sram.regs.actions & ACT_GEM_ID_MASK) >> ACT_GEM_ID),
			((c2->sram.regs.actions & ACT_LOW_Q_MASK) >> ACT_LOW_Q),
			((c2->sram.regs.actions & ACT_HIGH_Q_MASK) >> ACT_HIGH_Q),
			((c2->sram.regs.actions & ACT_FWD_MASK) >> ACT_FWD),
			((c2->sram.regs.actions & ACT_POLICER_SELECT_MASK) >> ACT_POLICER_SELECT),
			((c2->sram.regs.actions & ACT_FLOW_ID_EN_MASK) >> ACT_FLOW_ID_EN));
	mvOsPrintf("\n\n");


	/*------------------------------*/
	/*	qos_attr 0x1B64		*/
	/*------------------------------*/
	mvOsPrintf("ACT_ATTR:		PRIO	DSCP	GEMID	LOW_Q	HIGH_Q	QUEUE\n");
	mvOsPrintf("		");
	/* modify priority */
	int32bit =  ((c2->sram.regs.qos_attr & ACT_QOS_ATTR_MDF_PRI_MASK) >> ACT_QOS_ATTR_MDF_PRI);
	mvOsPrintf("	%1.1d\t", int32bit);

	/* modify dscp */
	int32bit =  ((c2->sram.regs.qos_attr & ACT_QOS_ATTR_MDF_DSCP_MASK) >> ACT_QOS_ATTR_MDF_DSCP);
	mvOsPrintf("0x%2.2d\t", int32bit);

	/* modify gemportid */
	int32bit =  ((c2->sram.regs.qos_attr & ACT_QOS_ATTR_MDF_GEM_ID_MASK) >> ACT_QOS_ATTR_MDF_GEM_ID);
	mvOsPrintf("0x%4.4x\t", int32bit);

	/* modify low Q */
	int32bit =  ((c2->sram.regs.qos_attr & ACT_QOS_ATTR_MDF_LOW_Q_MASK) >> ACT_QOS_ATTR_MDF_LOW_Q);
	mvOsPrintf("0x%1.1d\t", int32bit);

	/* modify high Q */
	int32bit =  ((c2->sram.regs.qos_attr & ACT_QOS_ATTR_MDF_HIGH_Q_MASK) >> ACT_QOS_ATTR_MDF_HIGH_Q);
	mvOsPrintf("0x%2.2x\t", int32bit);

	/*modify queue*/
	int32bit = ((c2->sram.regs.qos_attr & (ACT_QOS_ATTR_MDF_LOW_Q_MASK | ACT_QOS_ATTR_MDF_HIGH_Q_MASK)));
	int32bit >>= ACT_QOS_ATTR_MDF_LOW_Q;

	mvOsPrintf("0x%2.2x\t", int32bit);
	mvOsPrintf("\n\n");



	/*------------------------------*/
	/*	hwf_attr 0x1B68		*/
	/*------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
	mvOsPrintf("HWF_ATTR:		IPTR	DPTR	CHKSM   MTU_IDX\n");
#else
	mvOsPrintf("HWF_ATTR:		IPTR	DPTR	CHKSM\n");
#endif
	mvOsPrintf("			");

	/* HWF modification instraction pointer */
	int32bit =  ((c2->sram.regs.hwf_attr & ACT_HWF_ATTR_IPTR_MASK) >> ACT_HWF_ATTR_IPTR);
	mvOsPrintf("0x%1.1x\t", int32bit);

	/* HWF modification data pointer */
	int32bit =  ((c2->sram.regs.hwf_attr & ACT_HWF_ATTR_DPTR_MASK) >> ACT_HWF_ATTR_DPTR);
	mvOsPrintf("0x%4.4x\t", int32bit);

	/* HWF modification instraction pointer */
	int32bit =  ((c2->sram.regs.hwf_attr & ACT_HWF_ATTR_CHKSM_EN_MASK) >> ACT_HWF_ATTR_CHKSM_EN);
	mvOsPrintf("%s\t", int32bit ? "ENABLE " : "DISABLE");

#ifdef CONFIG_MV_ETH_PP2_1
	/* mtu index */
	int32bit =  ((c2->sram.regs.hwf_attr & ACT_HWF_ATTR_MTU_INX_MASK) >> ACT_HWF_ATTR_MTU_INX);
	mvOsPrintf("0x%1.1x\t", int32bit);
#endif
	mvOsPrintf("\n\n");

	/*------------------------------*/
	/*	dup_attr 0x1B6C		*/
	/*------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
	mvOsPrintf("DUP_ATTR:		FID	COUNT	POLICER [id    bank]\n");
	mvOsPrintf("			0x%2.2x\t0x%1.1x\t\t[0x%2.2x   0x%1.1x]\n",
		((c2->sram.regs.dup_attr & ACT_DUP_FID_MASK) >> ACT_DUP_FID),
		((c2->sram.regs.dup_attr & ACT_DUP_COUNT_MASK) >> ACT_DUP_COUNT),
		((c2->sram.regs.dup_attr & ACT_DUP_POLICER_MASK) >> ACT_DUP_POLICER_ID),
		((c2->sram.regs.dup_attr & ACT_DUP_POLICER_BANK_MASK) >> ACT_DUP_POLICER_BANK_BIT));
	mvOsPrintf("\n");
	/*------------------------------*/
	/*	seq_attr 0x1B70		*/
	/*------------------------------*/
	/*PPv2.1 new feature MAS 3.14*/
	mvOsPrintf("SEQ_ATTR:		ID	MISS\n");
	mvOsPrintf("			0x%2.2x    0x%2.2x\n",
			((c2->sram.regs.seq_attr & ACT_SEQ_ATTR_ID_MASK) >> ACT_SEQ_ATTR_ID),
			((c2->sram.regs.seq_attr & ACT_SEQ_ATTR_MISS_MASK) >> ACT_SEQ_ATTR_MISS));

	mvOsPrintf("\n\n");

#else
	mvOsPrintf("DUP_ATTR:		FID	COUNT	POLICER\n");
	mvOsPrintf("	0x%2.2x\t0x%1.1x\t0x%2.2x",
		((c2->sram.regs.dup_attr & ACT_DUP_FID_MASK) >> ACT_DUP_FID),
		((c2->sram.regs.dup_attr & ACT_DUP_COUNT_MASK) >> ACT_DUP_COUNT),
		((c2->sram.regs.dup_attr & ACT_DUP_POLICER_MASK) >> ACT_DUP_POLICER_ID));

	mvOsPrintf("\n\n");
#endif

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
void 	mvPp2ClsC2SwClear(MV_PP2_CLS_C2_ENTRY *c2)
{

	memset(c2, 0, sizeof(MV_PP2_CLS_C2_ENTRY));
}
/*-------------------------------------------------------------------------------*/
void 	mvPp2ClsC2HwClearAll()
{
	int index;

	MV_PP2_CLS_C2_ENTRY c2;

	mvPp2ClsC2SwClear(&c2);

	for (index = 0; index < MV_PP2_CLS_C2_TCAM_SIZE; index++) {
		mvPp2ClsC2HwWrite(index, &c2);
		mvPp2ClsC2HwInv(index);
	}
}
/*-------------------------------------------------------------------------------*/
int 	mvPp2ClsC2HwDump()
{
	int index;
	unsigned cnt;

	MV_PP2_CLS_C2_ENTRY c2;

	mvPp2ClsC2SwClear(&c2);

	for (index = 0; index < MV_PP2_CLS_C2_TCAM_SIZE; index++) {
		mvPp2ClsC2HwRead(index, &c2);
		if (c2.inv == 0) {
			mvPp2ClsC2SwDump(&c2);
			mvPp2ClsC2HitCntrRead(index, &cnt);
			mvOsPrintf("HITS: %d\n", cnt);
			mvOsPrintf("-----------------------------------------------------------------\n");
		}
	}
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2SwTcam(int enable)
{
	POS_RANGE_VALIDATE(enable, 1);

	mvPp2WrReg(MV_PP2_CLS2_TCAM_CTRL_REG, enable);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2TcamByteSet(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char byte, unsigned char enable)
{
	PTR_VALIDATE(c2);

	POS_RANGE_VALIDATE(offs, MV_PP2_CLS_C2_TCAM_DATA_BYTES);

	c2->tcam.bytes[TCAM_DATA_BYTE_OFFS(offs)] = byte;
	c2->tcam.bytes[TCAM_DATA_MASK_OFFS(offs)] = enable;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2TcamByteGet(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char *byte, unsigned char *enable)
{
	PTR_VALIDATE(c2);
	PTR_VALIDATE(byte);
	PTR_VALIDATE(enable);

	POS_RANGE_VALIDATE(offs, 8);

	*byte = c2->tcam.bytes[TCAM_DATA_BYTE_OFFS(offs)];
	*enable = c2->tcam.bytes[TCAM_DATA_MASK_OFFS(offs)];
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*
return EQUALS if tcam_data[off]&tcam_mask[off] = byte
*/
int mvPp2ClsC2TcamByteCmp(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char byte)
	{
	unsigned char tcamByte, tcamMask;

	PTR_VALIDATE(c2);

	if (mvPp2ClsC2TcamByteGet(c2, offs, &tcamByte, &tcamMask) != MV_OK)
		return MV_CLS2_ERR;

	if ((tcamByte & tcamMask) == (byte & tcamMask))
		return EQUALS;

	return NOT_EQUALS;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2TcamBytesCmp(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offset, unsigned int size, unsigned char *bytes)
{
	int status, index;

	PTR_VALIDATE(c2);

	if ((sizeof(bytes) < size) || ((offset + size) > (MV_PP2_CLS_C2_TCAM_WORDS * 4))) {
		mvOsPrintf("mvCls2Hw %s: value is out of range.\n", __func__);
		return MV_CLS2_ERR;
	}

	for (index = 0; index < size; index++) {
		status = mvPp2ClsC2TcamByteCmp(c2, offset, bytes[index]);
		if (status != EQUALS)
			return status;
	}
	return EQUALS;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QosTblSet(MV_PP2_CLS_C2_ENTRY *c2, int tbl_id, int tbl_sel)
{

	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(tbl_sel, 1);

	if (tbl_sel == 1) {
		/*dscp*/
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_DSCP);
	} else {
		/*pri*/
		POS_RANGE_VALIDATE(tbl_id, QOS_TBL_NUM_PRI);
	}
	c2->sram.regs.action_tbl = (tbl_id << ACT_TBL_ID) | (tbl_sel << ACT_TBL_SEL);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2ColorSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int from)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, COLOR_RED_AND_LOCK);

	c2->sram.regs.actions &= ~ACT_COLOR_MASK;
	c2->sram.regs.actions |= (cmd << ACT_COLOR);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_COLOR);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_COLOR);


	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2PrioSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int prio, int from)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(prio, (QOS_TBL_LINE_NUM_PRI-1));

	/*set command*/
	c2->sram.regs.actions &= ~ACT_PRI_MASK;
	c2->sram.regs.actions |= (cmd << ACT_PRI);

	/*set modify priority value*/
	c2->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_PRI_MASK;
	c2->sram.regs.qos_attr |= (prio << ACT_QOS_ATTR_MDF_PRI);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_PRI_DSCP);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_PRI_DSCP);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2DscpSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int dscp, int from)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(dscp, (QOS_TBL_LINE_NUM_DSCP-1));

	/*set command*/
	c2->sram.regs.actions &= ~ACT_DSCP_MASK;
	c2->sram.regs.actions |= (cmd << ACT_DSCP);

	/*set modify DSCP value*/
	c2->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_DSCP_MASK;
	c2->sram.regs.qos_attr |= (dscp << ACT_QOS_ATTR_MDF_DSCP);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_PRI_DSCP);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_PRI_DSCP);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2GpidSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int gpid, int from)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(gpid, ACT_QOS_ATTR_GEM_ID_MAX);

	/*set command*/
	c2->sram.regs.actions &= ~ACT_GEM_ID_MASK;
	c2->sram.regs.actions |= (cmd << ACT_GEM_ID);

	/*set modify DSCP value*/
	c2->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_GEM_ID_MASK;
	c2->sram.regs.qos_attr |= (gpid << ACT_QOS_ATTR_MDF_GEM_ID);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_GEM_ID);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_GEM_ID);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2QueueHighSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from)
{
	PTR_VALIDATE(c2);


	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_HIGH_Q_MAX);

	/*set command*/
	c2->sram.regs.actions &= ~ACT_HIGH_Q_MASK;
	c2->sram.regs.actions |= (cmd << ACT_HIGH_Q);

	/*set modify High queue value*/
	c2->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_HIGH_Q_MASK;
	c2->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_HIGH_Q);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_HIGH_Q);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_HIGH_Q);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2QueueLowSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from)
{
	PTR_VALIDATE(c2);

	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_MDF_LOW_Q_MAX);

	/*set command*/
	c2->sram.regs.actions &= ~ACT_LOW_Q_MASK;
	c2->sram.regs.actions |= (cmd << ACT_LOW_Q);

	/*set modify High queue value*/
	c2->sram.regs.qos_attr &= ~ACT_QOS_ATTR_MDF_LOW_Q_MASK;
	c2->sram.regs.qos_attr |= (queue << ACT_QOS_ATTR_MDF_LOW_Q);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 << ACT_TBL_LOW_Q);
	else
		c2->sram.regs.action_tbl &= ~(1 << ACT_TBL_LOW_Q);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2QueueSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from)
{
	int status = MV_OK;
	int qHigh, qLow;

	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(queue, ACT_QOS_ATTR_Q_MAX);

	/* cmd validation in set functions */

	qHigh = (queue & ACT_QOS_ATTR_MDF_HIGH_Q_MASK) >> ACT_QOS_ATTR_MDF_HIGH_Q;
	qLow = (queue & ACT_QOS_ATTR_MDF_LOW_Q_MASK) >> ACT_QOS_ATTR_MDF_LOW_Q;

	status |= mvPp2ClsC2QueueLowSet(c2, cmd, qLow, from);
	status |= mvPp2ClsC2QueueHighSet(c2, cmd, qHigh, from);

	return status;

}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2ForwardSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, HWF_AND_LOW_LATENCY_AND_LOCK);

	c2->sram.regs.actions &= ~ACT_FWD_MASK;
	c2->sram.regs.actions |= (cmd << ACT_FWD);
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
int mvPp2ClsC2PolicerSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int policerId, int bank)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);
	BIT_RANGE_VALIDATE(bank);

	c2->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	c2->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	c2->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	c2->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);

	if (bank)
		c2->sram.regs.dup_attr |= ACT_DUP_POLICER_BANK_MASK;
	else
		c2->sram.regs.dup_attr &= ~ACT_DUP_POLICER_BANK_MASK;

	return MV_OK;

}

#else
int mvPp2ClsC2PolicerSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int policerId)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(cmd, UPDATE_AND_LOCK);
	POS_RANGE_VALIDATE(policerId, ACT_DUP_POLICER_MAX);

	c2->sram.regs.actions &= ~ACT_POLICER_SELECT_MASK;
	c2->sram.regs.actions |= (cmd << ACT_POLICER_SELECT);

	c2->sram.regs.dup_attr &= ~ACT_DUP_POLICER_MASK;
	c2->sram.regs.dup_attr |= (policerId << ACT_DUP_POLICER_ID);
	return MV_OK;
}
#endif /*CONFIG_MV_ETH_PP2_1*/
 /*-------------------------------------------------------------------------------*/

int mvPp2ClsC2FlowIdEn(MV_PP2_CLS_C2_ENTRY *c2, int flowid_en)
{
	PTR_VALIDATE(c2);

	/*set Flow ID enable or disable*/
	if (flowid_en)
		c2->sram.regs.actions |= (1 << ACT_FLOW_ID_EN);
	else
		c2->sram.regs.actions &= ~(1 << ACT_FLOW_ID_EN);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2ModSet(MV_PP2_CLS_C2_ENTRY *c2, int data_ptr, int instr_offs, int l4_csum)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(data_ptr, ACT_HWF_ATTR_DPTR_MAX);
	POS_RANGE_VALIDATE(instr_offs, ACT_HWF_ATTR_IPTR_MAX);
	POS_RANGE_VALIDATE(l4_csum, 1);

	c2->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_DPTR_MASK;
	c2->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_IPTR_MASK;
	c2->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_CHKSM_EN_MASK;

	c2->sram.regs.hwf_attr |= (data_ptr << ACT_HWF_ATTR_DPTR);
	c2->sram.regs.hwf_attr |= (instr_offs << ACT_HWF_ATTR_IPTR);
	c2->sram.regs.hwf_attr |= (l4_csum << ACT_HWF_ATTR_CHKSM_EN);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

/*  PPv2.1 (feature MAS 3.7) new feature - set mtu index */

int mvPp2ClsC2MtuSet(MV_PP2_CLS_C2_ENTRY *c2, int mtu_inx)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(mtu_inx, ACT_HWF_ATTR_MTU_INX_MAX);

	c2->sram.regs.hwf_attr &= ~ACT_HWF_ATTR_MTU_INX_MASK;
	c2->sram.regs.hwf_attr |= (mtu_inx << ACT_HWF_ATTR_MTU_INX);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2DupSet(MV_PP2_CLS_C2_ENTRY *c2, int dupid, int count)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(count, ACT_DUP_COUNT_MAX);
	POS_RANGE_VALIDATE(dupid, ACT_DUP_FID_MAX);

	/*set flowid and count*/
	c2->sram.regs.dup_attr &= ~(ACT_DUP_FID_MASK | ACT_DUP_COUNT_MASK);
	c2->sram.regs.dup_attr |= (dupid << ACT_DUP_FID);
	c2->sram.regs.dup_attr |= (count << ACT_DUP_COUNT);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.14) SEQ_ATTR new register in action table
 */
int mvPp2ClsC2SeqSet(MV_PP2_CLS_C2_ENTRY *c2, int miss, int id)
{
	PTR_VALIDATE(c2);
	POS_RANGE_VALIDATE(miss, 1);
	POS_RANGE_VALIDATE(id, ACT_SEQ_ATTR_ID_MAX);

	c2->sram.regs.seq_attr = 0;
	c2->sram.regs.seq_attr = ((id << ACT_SEQ_ATTR_ID) | (miss << ACT_SEQ_ATTR_MISS));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine Hit counters Public APIs		    	 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2HitCntrsClearAll(void)
{
	int iter = 0;

	/* wrirte clear bit*/
	mvPp2WrReg(MV_PP2_CLS2_HIT_CTR_CLR_REG, (1 << MV_PP2_CLS2_HIT_CTR_CLR_CLR));

	while (mvPp2ClsC2HitCntrsIsBusy())
		if (iter++ >= RETRIES_EXCEEDED) {
			mvOsPrintf("%s:Error - retries exceeded.\n", __func__);
			return MV_CLS2_RETRIES_EXCEEDED;
		}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2HitCntrsIsBusy(void)
{
	unsigned int regVal;

	regVal = mvPp2RdReg(MV_PP2_CLS2_HIT_CTR_REG);
	regVal &= MV_PP2_CLS2_HIT_CTR_CLR_DONE_MASK;
	regVal >>= MV_PP2_CLS2_HIT_CTR_CLR_DONE;

	return (1 - (int)regVal);
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2HitCntrRead(int index, MV_U32 *cntr)
{
	unsigned int value = 0;

	/* write index reg */
	mvPp2WrReg(MV_PP2_CLS2_TCAM_IDX_REG, index);

	value = mvPp2RdReg(MV_PP2_CLS2_HIT_CTR_REG);

	if (cntr)
		*cntr = value;
	else
		mvOsPrintf("INDEX: 0x%8.8X	VAL: 0x%8.8X\n", index, value);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2HitCntrsDump()
{
	int i;
	unsigned int cnt;

	for (i = 0; i < MV_PP2_CLS_C2_TCAM_SIZE; i++) {
		mvPp2ClsC2HitCntrRead(i, &cnt);
		if (cnt != 0)
			mvOsPrintf("INDEX: 0x%8.8X	VAL: 0x%8.8X\n", i, cnt);
	}


	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsC2RegsDump()
{
	int i;
	char reg_name[100];

	mvPp2PrintReg(MV_PP2_CLS2_TCAM_IDX_REG, "MV_PP2_CLS2_TCAM_IDX_REG");

	for (i = 0; i < MV_PP2_CLS_C2_TCAM_WORDS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS2_TCAM_DATA_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS2_TCAM_DATA_REG(i), reg_name);
	}

	mvPp2PrintReg(MV_PP2_CLS2_TCAM_INV_REG, "MV_PP2_CLS2_TCAM_INV_REG");
	mvPp2PrintReg(MV_PP2_CLS2_ACT_DATA_REG, "MV_PP2_CLS2_ACT_DATA_REG");
	mvPp2PrintReg(MV_PP2_CLS2_DSCP_PRI_INDEX_REG, "MV_PP2_CLS2_DSCP_PRI_INDEX_REG");
	mvPp2PrintReg(MV_PP2_CLS2_QOS_TBL_REG, "MV_PP2_CLS2_QOS_TBL_REG");
	mvPp2PrintReg(MV_PP2_CLS2_ACT_REG, "MV_PP2_CLS2_ACT_REG");
	mvPp2PrintReg(MV_PP2_CLS2_ACT_QOS_ATTR_REG, "MV_PP2_CLS2_ACT_QOS_ATTR_REG");
	mvPp2PrintReg(MV_PP2_CLS2_ACT_HWF_ATTR_REG, "MV_PP2_CLS2_ACT_HWF_ATTR_REG");
	mvPp2PrintReg(MV_PP2_CLS2_ACT_DUP_ATTR_REG, "MV_PP2_CLS2_ACT_DUP_ATTR_REG");
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2PrintReg(MV_PP2_CLS2_ACT_SEQ_ATTR_REG, "MV_PP2_CLS2_ACT_SEQ_ATTR_REG");
#endif
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int	mvPp2ClsC2HwInv(int index)
{	/* write index reg */
	mvPp2WrReg(MV_PP2_CLS2_TCAM_IDX_REG, index);

	/* set invalid bit*/
	mvPp2WrReg(MV_PP2_CLS2_TCAM_INV_REG, (1 << MV_PP2_CLS2_TCAM_INV_INVALID));

	/* trigger */
	mvPp2WrReg(MV_PP2_CLS2_TCAM_DATA_REG(4), 0);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int	mvPp2ClsC2HwInvAll(void)
{
	int index;

	for (index = 0; index < MV_PP2_CLS_C2_TCAM_SIZE; index++) {
		/* write index reg */
		mvPp2WrReg(MV_PP2_CLS2_TCAM_IDX_REG, index);

		/* set invalid bit*/
		mvPp2WrReg(MV_PP2_CLS2_TCAM_INV_REG, (1 << MV_PP2_CLS2_TCAM_INV_INVALID));

		/* trigger */
		mvPp2WrReg(MV_PP2_CLS2_TCAM_DATA_REG(4), 0);
	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int	mvPp2ClsC2Init(void)
{
	mvPp2ClsC2QosHwClearAll();
	mvPp2ClsC2HwClearAll();
	mvPp2ClsC2SwTcam(1);
	return MV_OK;
}
