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
#include "mvPp2ClsHw.h"

/* TODO: chage to char arrays */
int mvClsLkpShadowTbl[2 * MV_PP2_CLS_LKP_TBL_SIZE];
int mvClsFlowShadowTbl[MV_PP2_CLS_FLOWS_TBL_SIZE];


/******************************************************************************/
/**************** Classifier Top Public initialization APIs *******************/
/******************************************************************************/
int mvPp2ClsInit()
{
	int rc;
	/*TODO - SET MTU */
	/* Enabled Classifier */
	rc = mvPp2ClsHwEnable(1);
	if (rc)
		return rc;
	/* clear cls flow table and shadow */
	mvPp2ClsHwFlowClearAll();

	/* clear cls lookup table and shadow */
	mvPp2ClsHwLkpClearAll();

	return MV_OK;
}
void mvPp2ClsShadowInit()
{
	memset(mvClsLkpShadowTbl, NOT_IN_USE, 2 * MV_PP2_CLS_LKP_TBL_SIZE * sizeof(int));
	memset(mvClsFlowShadowTbl, NOT_IN_USE, MV_PP2_CLS_FLOWS_TBL_SIZE * sizeof(int));
}
void mvPp2ClsHwLastBitWorkAround()
{
	/* workaround to hw bug - set last bit in flow entry */
	mvPp2WrReg(MV_PP2_CLS_FLOW_INDEX_REG, 0);
	mvPp2WrReg(MV_PP2_CLS_FLOW_TBL0_REG, 1);
}

int mvPp2ClsHwPortDefConfig(int port, int way, int lkpid, int rxq)
{
	MV_PP2_CLS_LKP_ENTRY  le;

	mvPp2ClsHwPortWaySet(port, way);
	/*
	the entry to be accessed in lookupid decoding table
	is acording to way and lkpid
	*/
	le.way = way;
	le.lkpid = lkpid;
	le.data = 0;

	mvPp2ClsSwLkpRxqSet(&le, rxq);

	/* do not use classification engines */
	mvPp2ClsSwLkpEnSet(&le, 0);

	/* write entry */
	mvPp2ClsHwLkpWrite(lkpid, way, &le);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsHwEnable(int enable)
{
	mvPp2WrReg(MV_PP2_CLS_MODE_REG, (unsigned int)(enable << MV_PP2_CLS_MODE_ACTIVE_BIT));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwPortWaySet(int port, int way)
{
	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS-1);
	POS_RANGE_VALIDATE(way, ONE_BIT_MAX);

	if (way == 1)
		MV_REG_BIT_SET(MV_PP2_CLS_PORT_WAY_REG, MV_PP2_CLS_PORT_WAY_MASK(port));
	else
		MV_REG_BIT_RESET(MV_PP2_CLS_PORT_WAY_REG, MV_PP2_CLS_PORT_WAY_MASK(port));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwPortSpidSet(int port, int spid)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(port, ETH_PORTS_NUM-1);
	POS_RANGE_VALIDATE(spid, MV_PP2_CLS_PORT_SPID_MAX);

	regVal = mvPp2RdReg(MV_PP2_CLS_PORT_SPID_REG);
	regVal &= ~MV_PP2_CLS_PORT_SPID_MASK(port);
	regVal |=  MV_PP2_CLS_PORT_SPID_VAL(port, spid);
	mvPp2WrReg(MV_PP2_CLS_PORT_SPID_REG, regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwUniPortSet(int uni_port, int spid)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(uni_port, UNI_MAX);
	POS_RANGE_VALIDATE(spid, MV_PP2_CLS_SPID_MAX);

	regVal = mvPp2RdReg(MV_PP2_CLS_SPID_UNI_REG(spid));
	regVal &= ~MV_PP2_CLS_SPID_UNI_MASK(spid);
	regVal |=  MV_PP2_CLS_SPID_UNI_VAL(spid, uni_port);
	mvPp2WrReg(MV_PP2_CLS_SPID_UNI_REG(spid), regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwVirtPortSet(int index, int gem_portid)
{
	POS_RANGE_VALIDATE(index, MV_PP2_CLS_GEM_VIRT_REGS_NUM - 1);
	POS_RANGE_VALIDATE(gem_portid, MV_PP2_CLS_GEM_VIRT_MAX);
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2WrReg(MV_PP2_CLS_GEM_VIRT_INDEX_REG, index);
	mvPp2WrReg(MV_PP2_CLS_GEM_VIRT_REG, gem_portid);
#else
	mvPp2WrReg(MV_PP2_CLS_GEM_VIRT_REG(index), gem_portid);
#endif /* CONFIG_MV_ETH_PP2_1 */
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwUdfSet(int udf_no, int offs_id, int offs_bits, int size_bits)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(offs_id, MV_PP2_CLS_UDF_OFFSET_ID_MAX);
	POS_RANGE_VALIDATE(offs_bits, MV_PP2_CLS_UDF_REL_OFFSET_MAX);
	POS_RANGE_VALIDATE(size_bits, MV_PP2_CLS_UDF_SIZE_MASK);
	POS_RANGE_VALIDATE(udf_no, MV_PP2_CLS_UDF_REGS_NUM - 1);

	regVal = mvPp2RdReg(MV_PP2_CLS_UDF_REG(udf_no));
	regVal &= ~MV_PP2_CLS_UDF_OFFSET_ID_MASK;
	regVal &= ~MV_PP2_CLS_UDF_REL_OFFSET_MASK;
	regVal &= ~MV_PP2_CLS_UDF_SIZE_MASK;

	regVal |= (offs_id << MV_PP2_CLS_UDF_OFFSET_ID_OFFS);
	regVal |= (offs_bits << MV_PP2_CLS_UDF_REL_OFFSET_OFFS);
	regVal |= (size_bits << MV_PP2_CLS_UDF_SIZE_OFFS);

	mvPp2WrReg(MV_PP2_CLS_UDF_REG(udf_no), regVal);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*
PPv2.1 (feature MAS 3.7) feature update
Note: this function overwrite q_high value that set by mvPp2ClsHwRxQueueHighSet
*/
int mvPp2ClsHwOversizeRxqSet(int port, int rxq)
{

	POS_RANGE_VALIDATE(rxq, MV_PP2_CLS_OVERSIZE_RXQ_MAX);
	/* set oversize rxq */
#ifdef CONFIG_MV_ETH_PP2_1
	mvPp2WrReg(MV_PP2_CLS_OVERSIZE_RXQ_LOW_REG(port), rxq);

	mvPp2WrReg(MV_PP2_CLS_SWFWD_P2HQ_REG(port), (rxq >> MV_PP2_CLS_OVERSIZE_RXQ_LOW_BITS));
#else
	{
		unsigned int regVal;
		regVal = mvPp2RdReg(MV_PP2_CLS_OVERSIZE_RXQ_REG(port));
		regVal &= ~MV_PP2_CLS_OVERSIZE_RX_MASK;
		regVal |= (rxq << MV_PP2_CLS_OVERSIZE_RXQ_OFFS);
		mvPp2WrReg(MV_PP2_CLS_OVERSIZE_RXQ_REG(port), regVal);
	}
#endif /*PPv2_1*/

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*PPv2.1 feature changed MAS 3.7*/
int mvPp2V0ClsHwMtuSet(int port, int txp, int mtu)
{
	int eport;
	unsigned int regVal;

	POS_RANGE_VALIDATE(mtu, MV_PP2_CLS_MTU_MAX);

	if (port == MV_PON_PORT_ID)  {/*pon*/
		POS_RANGE_VALIDATE(txp, MV_ETH_MAX_TCONT - 1); /*txq num in pon*/
		eport = txp; /* regs 0 - 15 for pon txq */
	} else {
		POS_RANGE_VALIDATE(port, ETH_PORTS_NUM - 1);
		eport = 16 + port;/* regs 16 - 23 ethernet port */
	}

	/* set mtu */
	regVal = mvPp2RdReg(MV_PP2_CLS_MTU_REG(eport));
	regVal &= ~MV_PP2_CLS_MTU_MASK;
	regVal |= (mtu << MV_PP2_CLS_MTU_OFFS);
	mvPp2WrReg(MV_PP2_CLS_MTU_REG(eport), regVal);

	return MV_OK;

}

int mvPp2V1ClsHwMtuSet(int index, int mtu)
{
	POS_RANGE_VALIDATE(mtu, MV_PP2_CLS_MTU_MAX);
	POS_RANGE_VALIDATE(index, 15 /* define MAX value */);

	/* set mtu */
	mvPp2WrReg(MV_PP2_CLS_MTU_REG(index), mtu);
	return MV_OK;

}

/*-------------------------------------------------------------------------------
PPv2.1 new feature MAS 3.5
set high queue -
	from = 0 : The value of QueueHigh is as defined by the Classifier
	from = 1 : The value of QueueHigh set to queue
	None: this function overwite rxq value that set by mvPp2ClsHwOversizeRxSet
-------------------------------------------------------------------------------*/

int mvPp2ClsHwRxQueueHighSet(int port, int from, int queue)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS - 1);
	POS_RANGE_VALIDATE(from, 1);

	regVal = mvPp2RdReg(MV_PP2_CLS_SWFWD_PCTRL_REG);

	if (from) {
		POS_RANGE_VALIDATE(queue, MV_PP2_CLS_SWFWD_P2HQ_QUEUE_MASK);
		mvPp2WrReg(MV_PP2_CLS_SWFWD_P2HQ_REG(port), queue);
		regVal |= MV_PP2_CLS_SWFWD_PCTRL_MASK(port);
	} else
		regVal &= ~MV_PP2_CLS_SWFWD_PCTRL_MASK(port);

	mvPp2WrReg(MV_PP2_CLS_SWFWD_PCTRL_REG, regVal);

	return MV_OK;

}

/*-------------------------------------------------------------------------------
PPv2.1 new feature MAS 3.18
	virtEn: port support/not support generation of virtual portId, not relevant for PON port.
	uniEn:  port support/not support generation of UNI portId
	mh: default Marvell header value, used for For UNI and Virtual Port ID generation
	    in case that ETH port do not support Marvell header, not relevant for PON port.
-------------------------------------------------------------------------------*/
int mvPp2ClsHwMhSet(int port, int virtEn, int uniEn, unsigned short mh)
{
	unsigned int regVal = 0;
	int uniDisable = 1 - uniEn;
	int VirtDisable = 1 - virtEn;

	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS - 1);
	BIT_RANGE_VALIDATE(uniEn);
	BIT_RANGE_VALIDATE(virtEn);
	POS_RANGE_VALIDATE(mh, MV_PP2_CLS_PCTRL_MH_MASK);

	if (MV_PON_PORT(port))
		regVal = uniDisable << MV_PP2_CLS_PCTRL_UNI_EN_OFFS;
	else
		regVal = (uniDisable << MV_PP2_CLS_PCTRL_UNI_EN_OFFS) |
			(VirtDisable << MV_PP2_CLS_PCTRL_VIRT_EN_OFFS) |
			(mh << MV_PP2_CLS_PCTRL_MH_OFFS);

	mvPp2WrReg(MV_PP2_CLS_PCTRL_REG(port), regVal);

	return MV_OK;

}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsHwSeqInstrSizeSet(int index, int size)
{
	unsigned int regVal;

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_SEQ_INDEX_MAX);
	POS_RANGE_VALIDATE(size, MV_PP2_CLS_SEQ_SIZE_MAX);

	regVal = mvPp2RdReg(MV_PP2_CLS_SEQ_SIZE_REG);
	regVal &= ~MV_PP2_CLS_SEQ_SIZE_MASK(index);
	regVal |= MV_PP2_CLS_SEQ_SIZE_VAL(index, size);
	mvPp2WrReg(MV_PP2_CLS_SEQ_SIZE_REG, regVal);

	return MV_OK;
}

/******************************************************************************/
/***************** Classifier Top Public lkpid table APIs ********************/
/******************************************************************************/

int mvPp2ClsHwLkpWrite(int lkpid, int way, MV_PP2_CLS_LKP_ENTRY *fe)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(fe);

	BIT_RANGE_VALIDATE(way);
	POS_RANGE_VALIDATE(lkpid, MV_PP2_CLS_LKP_TBL_SIZE);

	/* write index reg */
	regVal = (way << MV_PP2_CLS_LKP_INDEX_WAY_OFFS) | (lkpid << MV_PP2_CLS_LKP_INDEX_LKP_OFFS);
	mvPp2WrReg(MV_PP2_CLS_LKP_INDEX_REG, regVal);

	/* write flowId reg */
	mvPp2WrReg(MV_PP2_CLS_LKP_TBL_REG, fe->data);

	/* update shadow */
	mvClsLkpShadowTbl[regVal] = IN_USE;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwLkpRead(int lkpid, int way, MV_PP2_CLS_LKP_ENTRY *fe)
{
	unsigned int regVal = 0;

	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(way, WAY_MAX);
	POS_RANGE_VALIDATE(lkpid, MV_PP2_CLS_FLOWS_TBL_SIZE);

	/* write index reg */
	regVal = (way << MV_PP2_CLS_LKP_INDEX_WAY_OFFS) | (lkpid << MV_PP2_CLS_LKP_INDEX_LKP_OFFS);
	mvPp2WrReg(MV_PP2_CLS_LKP_INDEX_REG, regVal);

	fe->way = way;
	fe->lkpid = lkpid;

	fe->data = mvPp2RdReg(MV_PP2_CLS_LKP_TBL_REG);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwLkpClear(int lkpid, int way)
{
	unsigned int regVal = 0;
	MV_PP2_CLS_LKP_ENTRY fe;

	POS_RANGE_VALIDATE(lkpid, MV_PP2_CLS_LKP_TBL_SIZE);
	BIT_RANGE_VALIDATE(way);

	/* clear entry */
	mvPp2ClsSwLkpClear(&fe);
	mvPp2ClsHwLkpWrite(lkpid, way, &fe);

	/* clear shadow */
	regVal = (way << MV_PP2_CLS_LKP_INDEX_WAY_OFFS) | (lkpid << MV_PP2_CLS_LKP_INDEX_LKP_OFFS);
	mvClsLkpShadowTbl[regVal] = NOT_IN_USE;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpDump(MV_PP2_CLS_LKP_ENTRY *fe)
{
	int int32bit;
	int status = 0;

	PTR_VALIDATE(fe);

	mvOsPrintf("< ID  WAY >:	RXQ  	EN	FLOW	MODE_BASE\n");

	/* id */
	mvOsPrintf(" 0x%2.2x  %1.1d\t", fe->lkpid, fe->way);

	/*rxq*/
	status |= mvPp2ClsSwLkpRxqGet(fe, &int32bit);
	mvOsPrintf("0x%2.2x\t", int32bit);

	/*enabe bit*/
	status |= mvPp2ClsSwLkpEnGet(fe, &int32bit);
	mvOsPrintf("%1.1d\t", int32bit);

	/*flow*/
	status |= mvPp2ClsSwLkpFlowGet(fe, &int32bit);
	mvOsPrintf("0x%3.3x\t", int32bit);

	/*mode*/
	status |= mvPp2ClsSwLkpModGet(fe, &int32bit);
	mvOsPrintf(" 0x%2.2x\t", int32bit);

	mvOsPrintf("\n");

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpRxqSet(MV_PP2_CLS_LKP_ENTRY *fe, int rxq)
{
	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(rxq, MV_PP2_MAX_RXQS_TOTAL-1);

	fe->data &= ~FLOWID_RXQ_MASK;
	fe->data |= (rxq << FLOWID_RXQ);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpEnSet(MV_PP2_CLS_LKP_ENTRY *fe, int en)
{
	PTR_VALIDATE(fe);

	BIT_RANGE_VALIDATE(en);

	fe->data &= ~FLOWID_EN_MASK;
	fe->data |= (en << FLOWID_EN);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpFlowSet(MV_PP2_CLS_LKP_ENTRY *fe, int flow_idx)
{
	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(flow_idx, MV_PP2_CLS_FLOWS_TBL_SIZE);

	fe->data &= ~FLOWID_FLOW_MASK;
	fe->data |= (flow_idx << FLOWID_FLOW);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpModSet(MV_PP2_CLS_LKP_ENTRY *fe, int mod_base)
{
	PTR_VALIDATE(fe);
	/* TODO: what is the max value of mode base */
	POS_RANGE_VALIDATE(mod_base, FLOWID_MODE_MAX);

	fe->data &= ~FLOWID_MODE_MASK;
	fe->data |= (mod_base << FLOWID_MODE);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpRxqGet(MV_PP2_CLS_LKP_ENTRY *fe, int *rxq)
{

	PTR_VALIDATE(fe);
	PTR_VALIDATE(rxq);

	*rxq =  (fe->data & FLOWID_RXQ_MASK) >> FLOWID_RXQ;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpEnGet(MV_PP2_CLS_LKP_ENTRY *fe, int *en)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(en);

	*en = (fe->data & FLOWID_EN_MASK) >> FLOWID_EN;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpFlowGet(MV_PP2_CLS_LKP_ENTRY *fe, int *flow_idx)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(flow_idx);

	*flow_idx = (fe->data & FLOWID_FLOW_MASK) >> FLOWID_FLOW;
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwLkpModGet(MV_PP2_CLS_LKP_ENTRY *fe, int *mod_base)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(mod_base);

	*mod_base = (fe->data & FLOWID_MODE_MASK) >> FLOWID_MODE;
	return MV_OK;
}

/******************************************************************************/
/***************** Classifier Top Public flows table APIs  ********************/
/******************************************************************************/


int mvPp2ClsHwFlowWrite(int index, MV_PP2_CLS_FLOW_ENTRY *fe)
{
	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_FLOWS_TBL_SIZE);

	fe->index = index;

	/*write index*/
	mvPp2WrReg(MV_PP2_CLS_FLOW_INDEX_REG, index);

	mvPp2WrReg(MV_PP2_CLS_FLOW_TBL0_REG, fe->data[0]);
	mvPp2WrReg(MV_PP2_CLS_FLOW_TBL1_REG, fe->data[1]);
	mvPp2WrReg(MV_PP2_CLS_FLOW_TBL2_REG, fe->data[2]);

	/* update shadow */
	mvClsFlowShadowTbl[index] = IN_USE;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/


int mvPp2ClsHwFlowRead(int index, MV_PP2_CLS_FLOW_ENTRY *fe)
{
	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_FLOWS_TBL_SIZE);

	fe->index = index;

	/*write index*/
	mvPp2WrReg(MV_PP2_CLS_FLOW_INDEX_REG, index);

	fe->data[0] = mvPp2RdReg(MV_PP2_CLS_FLOW_TBL0_REG);
	fe->data[1] = mvPp2RdReg(MV_PP2_CLS_FLOW_TBL1_REG);
	fe->data[2] = mvPp2RdReg(MV_PP2_CLS_FLOW_TBL2_REG);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwFlowClear(int index)
{
	MV_PP2_CLS_FLOW_ENTRY fe;

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_FLOWS_TBL_SIZE);

	/* Clear flow entry */
	mvPp2ClsSwFlowClear(&fe);
	mvPp2ClsHwFlowWrite(index, &fe);

	/* clear shadow */
	mvClsFlowShadowTbl[index] = NOT_IN_USE;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowDump(MV_PP2_CLS_FLOW_ENTRY *fe)
{
	int	int32bit_1, int32bit_2, i;
	int	fieldsArr[MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX];
	int	status = MV_OK;

	PTR_VALIDATE(fe);
	mvOsPrintf("INDEX: F[0] F[1] F[2] F[3] PRT[T  ID] ENG LAST LKP_TYP  PRIO\n");

	/*index*/
	mvOsPrintf("0x%3.3x  ", fe->index);

	/*filed[0] filed[1] filed[2] filed[3]*/
	status |= mvPp2ClsSwFlowHekGet(fe, &int32bit_1, fieldsArr);

	for (i = 0 ; i < MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX; i++)
		if (i < int32bit_1)
			mvOsPrintf("0x%2.2x ", fieldsArr[i]);
		else
			mvOsPrintf(" NA  ");

	/*port_type port_id*/
	status |= mvPp2ClsSwFlowPortGet(fe, &int32bit_1, &int32bit_2);
	mvOsPrintf("[%1d  0x%3.3x]  ", int32bit_1, int32bit_2);

	/* engine_num last_bit*/
	status |= mvPp2ClsSwFlowEngineGet(fe, &int32bit_1, &int32bit_2);
	mvOsPrintf("%1d   %1d    ", int32bit_1, int32bit_2);

	/* lookup_type priority*/
	status |= mvPp2ClsSwFlowExtraGet(fe, &int32bit_1, &int32bit_2);
	mvOsPrintf("0x%2.2x    0x%2.2x", int32bit_1, int32bit_2);

	mvOsPrintf("\n");
#ifdef CONFIG_MV_ETH_PP2_1
	mvOsPrintf("\n");
	mvOsPrintf("       PPPEO   VLAN   MACME   UDF7   SELECT SEQ_CTRL\n");
	mvOsPrintf("         %1d      %1d      %1d       %1d      %1d      %1d\n",
			(fe->data[0] & FLOW_PPPOE_MASK) >> FLOW_PPPOE,
			(fe->data[0] & FLOW_VLAN_MASK) >> FLOW_VLAN,
			(fe->data[0] & FLOW_MACME_MASK) >> FLOW_MACME,
			(fe->data[0] & FLOW_UDF7_MASK) >> FLOW_UDF7,
			(fe->data[0] & FLOW_PORT_ID_SEL_MASK) >> FLOW_PORT_ID_SEL,
			(fe->data[1] & FLOW_SEQ_CTRL_MASK) >> FLOW_SEQ_CTRL);
	mvOsPrintf("\n");

#endif
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsSwFlowHekSet(MV_PP2_CLS_FLOW_ENTRY *fe, int field_index, int field_id)
{
	int num_of_fields;

	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(field_index, MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX);
	POS_RANGE_VALIDATE(field_id, FLOW_FIELED_MAX);

	/* get current num_of_fields */
	num_of_fields = ((fe->data[1] & FLOW_FIELDS_NUM_MASK) >> FLOW_FIELDS_NUM) ;

	if (num_of_fields < (field_index+1)) {
		mvOsPrintf("%s: number of heks = %d , index (%d) is out of range.\n", __func__, num_of_fields, field_index);
		return MV_CLS_OUT_OF_RAGE;
	}

	fe->data[2] &= ~FLOW_FIELED_MASK(field_index);
	fe->data[2] |= (field_id <<  FLOW_FIELED_ID(field_index));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowHekNumSet(MV_PP2_CLS_FLOW_ENTRY *fe, int num_of_fields)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(num_of_fields, MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX);

	fe->data[1] &= ~FLOW_FIELDS_NUM_MASK;
	fe->data[1] |= (num_of_fields << FLOW_FIELDS_NUM);
	return MV_OK;
}
/*-------------------------------------------------------------------------------

int mvPp2ClsSwFlowHekSet(MV_PP2_CLS_FLOW_ENTRY *fe, int num_of_fields, int field_ids[])
{
	int index;

	PTR_VALIDATE(fe);
	PTR_VALIDATE(field_ids);

	POS_RANGE_VALIDATE(num_of_fields, MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX);

	fe->data[1] &= ~FLOW_FIELDS_NUM_MASK;
	fe->data[1] |= (num_of_fields << FLOW_FIELDS_NUM);

	for (index = 0; index < num_of_fields; index++) {
		POS_RANGE_VALIDATE(field_ids[index], FLOW_FIELED_MAX);
		fe->data[2] &= ~FLOW_FIELED_MASK(index);
		fe->data[2] |= (field_ids[index] <<  FLOW_FIELED_ID(index));
	}

	return MV_OK;
}
-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowPortSet(MV_PP2_CLS_FLOW_ENTRY *fe, int type, int portid)
{
	PTR_VALIDATE(fe);

	POS_RANGE_VALIDATE(type, FLOW_PORT_TYPE_MAX);
	POS_RANGE_VALIDATE(portid, FLOW_PORT_ID_MAX);

	fe->data[0] &= ~FLOW_PORT_ID_MASK;
	fe->data[0] &= ~FLOW_PORT_TYPE_MASK;

	fe->data[0] |= (portid << FLOW_PORT_ID);
	fe->data[0] |= (type << FLOW_PORT_TYPE);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwPortIdSelect(MV_PP2_CLS_FLOW_ENTRY *fe, int from)
{
	PTR_VALIDATE(fe);
	BIT_RANGE_VALIDATE(from);

	if (from)
		fe->data[0] |= FLOW_PORT_ID_SEL_MASK;
	else
		fe->data[0] &= ~FLOW_PORT_ID_SEL_MASK;

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowPppoeSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(mode, FLOW_PPPOE_MAX);

	fe->data[0] &= ~FLOW_PPPOE_MASK;
	fe->data[0] |= (mode << FLOW_PPPOE);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowVlanSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(mode, FLOW_VLAN_MAX);

	fe->data[0] &= ~FLOW_VLAN_MASK;
	fe->data[0] |= (mode << FLOW_VLAN);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowMacMeSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(mode, FLOW_MACME_MAX);

	fe->data[0] &= ~FLOW_MACME_MASK;
	fe->data[0] |= (mode << FLOW_MACME);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowUdf7Set(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(mode, FLOW_UDF7_MAX);

	fe->data[0] &= ~FLOW_UDF7_MASK;
	fe->data[0] |= (mode << FLOW_UDF7);
	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
int mvPp2ClsSwFlowEngineSet(MV_PP2_CLS_FLOW_ENTRY *fe, int engine, int is_last)
{
	PTR_VALIDATE(fe);

	BIT_RANGE_VALIDATE(is_last);
	POS_RANGE_VALIDATE(engine, FLOW_ENGINE_MAX);

	fe->data[0] &= ~FLOW_LAST_MASK;
	fe->data[0] &= ~FLOW_ENGINE_MASK;

	fe->data[0] |= is_last;
	fe->data[0] |= (engine << FLOW_ENGINE);

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsSwFlowSeqCtrlSet(MV_PP2_CLS_FLOW_ENTRY *fe, int mode)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(mode, FLOW_ENGINE_MAX);

	fe->data[1] &= ~FLOW_SEQ_CTRL_MASK;
	fe->data[1] |= (mode << FLOW_SEQ_CTRL);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowExtraSet(MV_PP2_CLS_FLOW_ENTRY *fe, int type, int prio)
{
	PTR_VALIDATE(fe);
	POS_RANGE_VALIDATE(type, FLOW_PORT_ID_MAX);
	POS_RANGE_VALIDATE(prio, FLOW_FIELED_MAX);

	fe->data[1] &= ~FLOW_LKP_TYPE_MASK;
	fe->data[1] |= (type << FLOW_LKP_TYPE);

	fe->data[1] &= ~FLOW_FIELED_PRIO_MASK;
	fe->data[1] |= (prio << FLOW_FIELED_PRIO);

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowHekGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *num_of_fields, int field_ids[])
{
	int index;

	PTR_VALIDATE(fe);
	PTR_VALIDATE(num_of_fields);
	PTR_VALIDATE(field_ids);

	*num_of_fields = (fe->data[1] & FLOW_FIELDS_NUM_MASK) >> FLOW_FIELDS_NUM;


	for (index = 0; index < (*num_of_fields); index++)
		field_ids[index] = ((fe->data[2] & FLOW_FIELED_MASK(index)) >>  FLOW_FIELED_ID(index));

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowPortGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *type, int *portid)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(type);
	PTR_VALIDATE(portid);

	*type = (fe->data[0] & FLOW_PORT_TYPE_MASK) >> FLOW_PORT_TYPE;
	*portid = (fe->data[0] & FLOW_PORT_ID_MASK) >> FLOW_PORT_ID;

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowEngineGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *engine, int *is_last)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(engine);
	PTR_VALIDATE(is_last);

	*engine = (fe->data[0] & FLOW_ENGINE_MASK) >> FLOW_ENGINE;
	*is_last = fe->data[0] & FLOW_LAST_MASK;

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/

int mvPp2ClsSwFlowExtraGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *type, int *prio)
{
	PTR_VALIDATE(fe);
	PTR_VALIDATE(type);
	PTR_VALIDATE(prio);

	*type = (fe->data[1] & FLOW_LKP_TYPE_MASK) >> FLOW_LKP_TYPE;
	*prio = (fe->data[1] & FLOW_FIELED_PRIO_MASK) >> FLOW_FIELED_PRIO;

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*	Classifier Top Public length change table APIs   			 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsPktLenChangeWrite(int index, unsigned int data)
{

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_LEN_CHANGE_TBL_SIZE);
#ifdef CONFIG_MV_ETH_PP2_1
	/*write index*/
	mvPp2WrReg(MV_PP2_V1_CLS_LEN_CHANGE_INDEX_REG, index);

	mvPp2WrReg(MV_PP2_V1_CLS_LEN_CHANGE_TBL_REG, data);
#else
	mvPp2WrReg(MV_PP2_V0_CLS_LEN_CHANGE_INDEX_REG, index);

	mvPp2WrReg(MV_PP2_V0_CLS_LEN_CHANGE_TBL_REG, data);
#endif
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsPktLenChangeRead(int index, unsigned int *data)
{
	PTR_VALIDATE(data);

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_LEN_CHANGE_TBL_SIZE);
#ifdef CONFIG_MV_ETH_PP2_1
	/*write index*/
	mvPp2WrReg(MV_PP2_V1_CLS_LEN_CHANGE_INDEX_REG, index);

	*data = mvPp2RdReg(MV_PP2_V1_CLS_LEN_CHANGE_TBL_REG);
#else
	/*write index*/
	mvPp2WrReg(MV_PP2_V0_CLS_LEN_CHANGE_INDEX_REG, index);

	*data = mvPp2RdReg(MV_PP2_V0_CLS_LEN_CHANGE_TBL_REG);
#endif

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/

int mvPp2ClsPktLenChangeDump()
{
	/* print all non-zero length change entries */
	int index, int32bit_1;

	mvOsPrintf("INDEX:\tLENGHT\n");

	for (index = 0 ; index <= LEN_CHANGE_LENGTH_MAX; index++) {
		/* read entry */
		mvPp2ClsPktLenChangeGet(index, &int32bit_1);

		if (int32bit_1 != 0)
			mvOsPrintf("0x%3.3x\t%d\n", index, int32bit_1);
	}

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsPktLenChangeSet(int index, int length)
{
	unsigned int dec = 0, data = 0;

	DECIMAL_RANGE_VALIDATE(length, (0 - LEN_CHANGE_LENGTH_MAX), LEN_CHANGE_LENGTH_MAX);

	if (length < 0) {
		dec =  1;
		length = 0 - length;
	}

	data &= ~(LEN_CHANGE_DEC_MASK | LEN_CHANGE_LENGTH_MASK);
	data |= ((dec << LEN_CHANGE_DEC) |  (length << LEN_CHANGE_LENGTH));

	mvPp2ClsPktLenChangeWrite(index, data);

	return MV_OK;
}

int mvPp2ClsPktLenChangeGet(int index, int *length)
{
	unsigned int data, dec;

	PTR_VALIDATE(length);

	/* read HW entry */
	mvPp2ClsPktLenChangeRead(index, &data);

	dec = ((data & LEN_CHANGE_DEC_MASK) >> LEN_CHANGE_DEC);
	*length = ((data & LEN_CHANGE_LENGTH_MASK) >> LEN_CHANGE_LENGTH);

	if (dec == 1)
		*length = (*length) * (-1);

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
/*			additional cls debug APIs				 */
/*-------------------------------------------------------------------------------*/

int mvPp2ClsHwRegsDump()
{
	int i = 0;
	char reg_name[100];

	mvPp2PrintReg(MV_PP2_CLS_MODE_REG, "MV_PP2_CLS_MODE_REG");
	mvPp2PrintReg(MV_PP2_CLS_PORT_WAY_REG, "MV_PP2_CLS_PORT_WAY_REG");
	mvPp2PrintReg(MV_PP2_CLS_LKP_INDEX_REG, "MV_PP2_CLS_LKP_INDEX_REG");
	mvPp2PrintReg(MV_PP2_CLS_LKP_TBL_REG, "MV_PP2_CLS_LKP_TBL_REG");
	mvPp2PrintReg(MV_PP2_CLS_FLOW_INDEX_REG, "MV_PP2_CLS_FLOW_INDEX_REG");

	mvPp2PrintReg(MV_PP2_CLS_FLOW_TBL0_REG, "MV_PP2_CLS_FLOW_TBL0_REG");
	mvPp2PrintReg(MV_PP2_CLS_FLOW_TBL1_REG, "MV_PP2_CLS_FLOW_TBL1_REG");
	mvPp2PrintReg(MV_PP2_CLS_FLOW_TBL2_REG, "MV_PP2_CLS_FLOW_TBL2_REG");


	mvPp2PrintReg(MV_PP2_CLS_PORT_SPID_REG, "MV_PP2_CLS_PORT_SPID_REG");

	for (i = 0; i < MV_PP2_CLS_SPID_UNI_REGS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_SPID_UNI_%d_REG", i);
		mvPp2PrintReg((MV_PP2_CLS_SPID_UNI_BASE_REG + (4 * i)), reg_name);
	}
#ifdef CONFIG_MV_ETH_PP2_1
	for (i = 0; i < MV_PP2_CLS_GEM_VIRT_REGS_NUM; i++) {
		/* indirect access */
		mvPp2WrReg(MV_PP2_CLS_GEM_VIRT_INDEX_REG, i);
		mvOsSPrintf(reg_name, "MV_PP2_CLS_GEM_VIRT_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_GEM_VIRT_REG, reg_name);
	}
#else
	for (i = 0; i < MV_PP2_CLS_GEM_VIRT_REGS_NUM; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_GEM_VIRT_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_GEM_VIRT_REG(i), reg_name);
	}
#endif
	for (i = 0; i < MV_PP2_CLS_UDF_BASE_REGS; i++)	{
		mvOsSPrintf(reg_name, "MV_PP2_CLS_UDF_REG_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_UDF_REG(i), reg_name);
	}
#ifdef CONFIG_MV_ETH_PP2_1
	for (i = 0; i < 16; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_MTU_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_MTU_REG(i), reg_name);
	}
	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_OVER_RXQ_LOW_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_OVERSIZE_RXQ_LOW_REG(i), reg_name);
	}
	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_SWFWD_P2HQ_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_SWFWD_P2HQ_REG(i), reg_name);
	}

	mvPp2PrintReg(MV_PP2_CLS_SWFWD_PCTRL_REG, "MV_PP2_CLS_SWFWD_PCTRL_REG");
	mvPp2PrintReg(MV_PP2_CLS_SEQ_SIZE_REG, "MV_PP2_CLS_SEQ_SIZE_REG");

	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_PCTRL_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_PCTRL_REG(i), reg_name);
	}
#else
	for (i = 0; i < (MV_ETH_MAX_TCONT + MV_PP2_MAX_PORTS - 1); i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_MTU_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_MTU_REG(i), reg_name);
	}

	for (i = 0; i < MV_PP2_MAX_PORTS; i++) {
		mvOsSPrintf(reg_name, "MV_PP2_CLS_OVER_RXQ_%d_REG", i);
		mvPp2PrintReg(MV_PP2_CLS_OVERSIZE_RXQ_REG(i), reg_name);
	}
#endif

	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
void mvPp2ClsSwLkpClear(MV_PP2_CLS_LKP_ENTRY *fe)
{
	memset(fe, 0, sizeof(MV_PP2_CLS_LKP_ENTRY));
}

/*-------------------------------------------------------------------------------*/
void mvPp2ClsSwFlowClear(MV_PP2_CLS_FLOW_ENTRY *fe)
{
	memset(fe, 0, sizeof(MV_PP2_CLS_FLOW_ENTRY));
}
/*-------------------------------------------------------------------------------*/
void mvPp2ClsHwFlowClearAll()
{
	int index;

	MV_PP2_CLS_FLOW_ENTRY fe;

	mvPp2ClsSwFlowClear(&fe);

	for (index = 0; index < MV_PP2_CLS_FLOWS_TBL_SIZE ; index++)
		mvPp2ClsHwFlowWrite(index, &fe);

	/* clear shadow */
	memset(mvClsFlowShadowTbl, NOT_IN_USE, MV_PP2_CLS_FLOWS_TBL_SIZE * sizeof(int));

}
/*-------------------------------------------------------------------------------*/
static int mvPp2V1ClsHwFlowHitGet(int index,  unsigned int *cnt)
{

	POS_RANGE_VALIDATE(index, MV_PP2_CLS_FLOWS_TBL_SIZE);

	/*set index */
	mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, MV_PP2_V1_CNT_IDX_FLOW(index));

	if (cnt)
		*cnt = mvPp2RdReg(MV_PP2_V1_CLS_FLOW_TBL_HIT_REG);
	else
		mvOsPrintf("HITS = %d\n", mvPp2RdReg(MV_PP2_V1_CLS_FLOW_TBL_HIT_REG));

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/

int mvPp2V1ClsHwLkpHitGet(int lkpid, int way,  unsigned int *cnt)
{

	BIT_RANGE_VALIDATE(way);
	POS_RANGE_VALIDATE(lkpid, MV_PP2_CLS_LKP_TBL_SIZE);

	/*set index */
	mvPp2WrReg(MV_PP2_V1_CNT_IDX_REG, MV_PP2_V1_CNT_IDX_LKP(lkpid, way));

	if (cnt)
		*cnt = mvPp2RdReg(MV_PP2_V1_CLS_LKP_TBL_HIT_REG);
	else
		mvOsPrintf("HITS: %d\n", mvPp2RdReg(MV_PP2_V1_CLS_LKP_TBL_HIT_REG));

	return MV_OK;

}
/*-------------------------------------------------------------------------------*/
int mvPp2ClsHwFlowDump()
{
	int index;

	MV_PP2_CLS_FLOW_ENTRY fe;

	for (index = 0; index < MV_PP2_CLS_FLOWS_TBL_SIZE ; index++) {
		if (mvClsFlowShadowTbl[index] == IN_USE) {
			mvPp2ClsHwFlowRead(index, &fe);
			mvPp2ClsSwFlowDump(&fe);
#ifdef CONFIG_MV_ETH_PP2_1
			mvPp2V1ClsHwFlowHitGet(index, NULL);
#endif
			mvOsPrintf("------------------------------------------------------------------\n");
		}
	}
	return MV_OK;

}

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mvPp2V1ClsHwFlowHitsDump()
{
	int index;
	unsigned int cnt;
	MV_PP2_CLS_FLOW_ENTRY fe;

	for (index = 0; index < MV_PP2_CLS_FLOWS_TBL_SIZE ; index++) {
		if (mvClsFlowShadowTbl[index] == IN_USE) {
			mvPp2V1ClsHwFlowHitGet(index, &cnt);
			if (cnt != 0) {
				mvPp2ClsHwFlowRead(index, &fe);
				mvPp2ClsSwFlowDump(&fe);
				mvOsPrintf("HITS = %d\n", cnt);
				mvOsPrintf("\n");
			}
		}
	}

	return MV_OK;
}

/*-------------------------------------------------------------------------------*/
void mvPp2ClsHwLkpClearAll()
{
	int index;

	MV_PP2_CLS_LKP_ENTRY fe;

	mvPp2ClsSwLkpClear(&fe);

	for (index = 0; index < MV_PP2_CLS_LKP_TBL_SIZE ; index++) {
		mvPp2ClsHwLkpWrite(index, 0, &fe);
		mvPp2ClsHwLkpWrite(index, 1, &fe);
	}
	/* clear shadow */
	memset(mvClsLkpShadowTbl, NOT_IN_USE, 2 * MV_PP2_CLS_LKP_TBL_SIZE * sizeof(int));

}
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mvPp2V1ClsHwLkpHitsDump()
{
	int index, way, entryInd;
	unsigned int cnt;

	mvOsPrintf("< ID  WAY >:	HITS\n");
	for (index = 0; index < MV_PP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			entryInd = (way << MV_PP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			if (mvClsLkpShadowTbl[entryInd] == IN_USE) {
				mvPp2V1ClsHwLkpHitGet(index, way,  &cnt);
				if (cnt != 0)
					mvOsPrintf(" 0x%2.2x  %1.1d\t0x%8.8x\n", index, way, cnt);
			}
	}
	return MV_OK;
}
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mvPp2ClsHwLkpDump()
{
	int index, way, int32bit, ind;

	MV_PP2_CLS_LKP_ENTRY fe;
#ifdef CONFIG_MV_ETH_PP2_1
	mvOsPrintf("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE  HITS\n");
#else
	mvOsPrintf("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE\n");
#endif
	for (index = 0; index < MV_PP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			ind = (way << MV_PP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			if (mvClsLkpShadowTbl[ind] == IN_USE) {
				mvPp2ClsHwLkpRead(index, way, &fe);
				mvOsPrintf(" 0x%2.2x  %1.1d\t", fe.lkpid, fe.way);
				mvPp2ClsSwLkpRxqGet(&fe, &int32bit);
				mvOsPrintf("0x%2.2x\t", int32bit);
				mvPp2ClsSwLkpEnGet(&fe, &int32bit);
				mvOsPrintf("%1.1d\t", int32bit);
				mvPp2ClsSwLkpFlowGet(&fe, &int32bit);
				mvOsPrintf("0x%3.3x\t", int32bit);
				mvPp2ClsSwLkpModGet(&fe, &int32bit);
				mvOsPrintf(" 0x%2.2x\t", int32bit);
#ifdef CONFIG_MV_ETH_PP2_1
				mvPp2V1ClsHwLkpHitGet(index, way, &int32bit);
				mvOsPrintf(" 0x%8.8x\n", int32bit);
#endif
				mvOsPrintf("\n");

			}
		}
	return MV_OK;
}
