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

#ifndef __MV_CLS2_HW_H__
#define __MV_CLS2_HW_H__

#include "mvPp2ClsActHw.h"
#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"

/*-------------------------------------------------------------------------------*/
/*			Classifier C2 Top Registers	    			 */
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_TCAM_IDX_REG			(MV_PP2_REG_BASE + 0x1B00)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_TCAM_DATA_REG(idx)			(MV_PP2_REG_BASE + 0x1B10 + (idx) * 4)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_TCAM_INV_REG			(MV_PP2_REG_BASE + 0x1B24)
#define MV_PP2_CLS2_TCAM_INV_INVALID			31
#define MV_PP2_CLS2_TCAM_INV_INVALID_MASK		(1 << MV_PP2_CLS2_TCAM_INV_INVALID)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_ACT_DATA_REG			(MV_PP2_REG_BASE + 0x1B30)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_DSCP_PRI_INDEX_REG			(MV_PP2_REG_BASE + 0x1B40)

#define MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_OFF		0
#define MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_BITS		6
#define MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_MASK		((1 << MV_PP2_CLS2_DSCP_PRI_INDEX_LINE_BITS) - 1)

#define MV_PP2_CLS2_DSCP_PRI_INDEX_SEL_OFF		6
#define MV_PP2_CLS2_DSCP_PRI_INDEX_SEL_MASK		(1 << MV_PP2_CLS2_DSCP_PRI_INDEX_SEL_OFF)

#define MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF		8
#define MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_BITS		6
#define MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_MASK		((1 << MV_PP2_CLS2_DSCP_PRI_INDEX_TBL_ID_BITS) - 1)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_QOS_TBL_REG				(MV_PP2_REG_BASE + 0x1B44)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_HIT_CTR_REG				(MV_PP2_REG_BASE + 0x1B50)
#define MV_PP2_CLS2_HIT_CTR_OFF				0

#ifdef CONFIG_MV_ETH_PP2_1
#define MV_PP2_CLS2_HIT_CTR_BITS			32
#else
#define MV_PP2_CLS2_HIT_CTR_BITS			24
#endif
#define MV_PP2_CLS2_HIT_CTR_MASK			((1  << MV_PP2_CLS2_HIT_CTR_BITS) - 1)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_HIT_CTR_CLR_REG			(MV_PP2_REG_BASE + 0x1B54)

#define MV_PP2_CLS2_HIT_CTR_CLR_CLR			0
#define MV_PP2_CLS2_HIT_CTR_CLR_CLR_MASK		(1 << MV_PP2_CLS2_HIT_CTR_CLR_CLR)

#define MV_PP2_CLS2_HIT_CTR_CLR_DONE			1
#define MV_PP2_CLS2_HIT_CTR_CLR_DONE_MASK		(1 << MV_PP2_CLS2_HIT_CTR_CLR_DONE)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_ACT_REG				(MV_PP2_REG_BASE + 0x1B60)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_ACT_QOS_ATTR_REG			(MV_PP2_REG_BASE + 0x1B64)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_ACT_HWF_ATTR_REG			(MV_PP2_REG_BASE + 0x1B68)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_ACT_DUP_ATTR_REG			(MV_PP2_REG_BASE + 0x1B6C)

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.14) SEQ_ATTR new register in action table
 */
#define MV_PP2_CLS2_ACT_SEQ_ATTR_REG			(MV_PP2_REG_BASE + 0x1B70)

#define ACT_SEQ_ATTR_ID					0
#define ACT_SEQ_ATTR_ID_BITS				8
#define ACT_SEQ_ATTR_ID_MASK				(((1 << ACT_SEQ_ATTR_ID_BITS) - 1) << ACT_SEQ_ATTR_ID)
#define ACT_SEQ_ATTR_ID_MAX				((1 << ACT_SEQ_ATTR_ID_BITS) - 1)

#define ACT_SEQ_ATTR_MISS				8
#define ACT_SEQ_ATTR_MISS_MASK				(1 << ACT_SEQ_ATTR_MISS)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS2_TCAM_CTRL_REG			(MV_PP2_REG_BASE + 0x1B90)
#define MV_PP2_CLS2_TCAM_CTRL_EN			0
/*-------------------------------------------------------------------------------*/
/*		Classifier C2 QOS Table	(DSCP/PRI Table)			 */
/*-------------------------------------------------------------------------------*/
#define QOS_TBL_LINE_NUM_PRI				(8)
#define QOS_TBL_NUM_PRI					(64)

#define QOS_TBL_LINE_NUM_DSCP				(64)
#define QOS_TBL_NUM_DSCP				(8)

#define QOS_TBL_PRI					0
#define QOS_TBL_PRI_MASK				(((1 << ACT_QOS_ATTR_PRI_BITS) - 1) << QOS_TBL_PRI)


#define QOS_TBL_DSCP					3
#define QOS_TBL_DSCP_MASK				(((1 << ACT_QOS_ATTR_DSCP_BITS) - 1) << QOS_TBL_DSCP)

#define QOS_TBL_COLOR					9
#define QOS_TBL_COLOR_BITS				3
#define QOS_TBL_COLOR_MASK				(((1 << QOS_TBL_COLOR_BITS) - 1) << QOS_TBL_COLOR)

#define QOS_TBL_GEM_ID					12
#define QOS_TBL_GEM_ID_MASK				(((1 << ACT_QOS_ATTR_GEM_ID_BITS) - 1) << QOS_TBL_GEM_ID)

#define QOS_TBL_Q_NUM					24
#define QOS_TBL_Q_NUM_BITS				8
#define QOS_TBL_Q_NUM_MAX				((1 << QOS_TBL_Q_NUM_BITS) - 1)
#define QOS_TBL_Q_NUM_MASK				(((1 << QOS_TBL_Q_NUM_BITS) - 1) << QOS_TBL_Q_NUM)
/*-------------------------------------------------------------------------------*/
/*			Classifier C2 engine Public APIs			 */
/*-------------------------------------------------------------------------------*/
int	mvPp2ClsC2Init(void);

/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine QoS table Public APIs			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_C2_QOS_DSCP_TBL_SIZE				(64)
#define MV_PP2_CLS_C2_QOS_PRIO_TBL_SIZE				(8)
#define MV_PP2_CLS_C2_QOS_DSCP_TBL_NUM				(8)
#define MV_PP2_CLS_C2_QOS_PRIO_TBL_NUM				(64)

typedef struct mvPp2ClsC2Qosentry {
	unsigned int tbl_id;
	unsigned int tbl_sel;
	unsigned int tbl_line;
	unsigned int data;
} MV_PP2_CLS_C2_QOS_ENTRY;

int	mvPp2ClsC2QosPrioHwDump(void);
int	mvPp2ClsC2QosDscpHwDump(void);
int	mvPp2ClsC2QosHwRead(int tbl_id, int tbl_sel, int tbl_line, MV_PP2_CLS_C2_QOS_ENTRY *qos);
int	mvPp2ClsC2QosHwWrite(int id, int sel, int line, MV_PP2_CLS_C2_QOS_ENTRY *qos);
int	mvPp2ClsC2QosSwDump(MV_PP2_CLS_C2_QOS_ENTRY *qos);
void 	mvPp2ClsC2QosSwClear(MV_PP2_CLS_C2_QOS_ENTRY *qos);
void	mvPp2ClsC2QosHwClearAll(void);

int	mvPp2ClsC2QosPrioSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int prio);
int	mvPp2ClsC2QosDscpSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int dscp);
int	mvPp2ClsC2QosColorSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int color);
int	mvPp2ClsC2QosGpidSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int gpid);
int	mvPp2ClsC2QosQueueSet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int queue);
int	mvPp2ClsC2QosPrioGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *prio);
int	mvPp2ClsC2QosDscpGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *dscp);
int	mvPp2ClsC2QosColorGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *color);
int	mvPp2ClsC2QosGpidGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *gpid);
int	mvPp2ClsC2QosQueueGet(MV_PP2_CLS_C2_QOS_ENTRY *qos, int *queue);

/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine action table Public APIs	 		 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_C2_TCAM_SIZE				(256)
#define MV_PP2_CLS_C2_TCAM_WORDS			(5)
#define MV_PP2_CLS_C2_TCAM_DATA_BYTES			(10)
#define MV_PP2_CLS_C2_SRAM_WORDS			(5)

#define C2_SRAM_FMT					"%8.8x %8.8x %8.8x %8.8x %8.8x"
#define C2_SRAM_VAL(p)					p[4], p[3], p[2], p[1], p[0]

typedef struct mvPp2ClsC2Entry {
	unsigned int index;
	bool         inv;
	union {
		MV_U32	words[MV_PP2_CLS_C2_TCAM_WORDS];
		MV_U8	bytes[MV_PP2_CLS_C2_TCAM_WORDS * 4];
	} tcam;
	union {
		MV_U32 words[MV_PP2_CLS_C2_SRAM_WORDS];
		struct {
			MV_U32 action_tbl; /* 0x1B30 */
			MV_U32 actions;    /* 0x1B60 */
			MV_U32 qos_attr;   /* 0x1B64*/
			MV_U32 hwf_attr;   /* 0x1B68 */
			MV_U32 dup_attr;   /* 0x1B6C */
			/* PPv2.1 (feature MAS 3.14) SEQ_ATTR new register in action table */
			MV_U32 seq_attr;   /* 0x1B70 */
		} regs;
	} sram;
} MV_PP2_CLS_C2_ENTRY;

int 	mvPp2ClsC2SwTcam(int enable);
int 	mvPp2ClsC2HwWrite(int index, MV_PP2_CLS_C2_ENTRY *c2);
int 	mvPp2ClsC2HwRead(int index, MV_PP2_CLS_C2_ENTRY *c2);
int 	mvPp2ClsC2SwDump(MV_PP2_CLS_C2_ENTRY *c2);
int 	mvPp2ClsC2HwDump(void);
void 	mvPp2ClsC2SwClear(MV_PP2_CLS_C2_ENTRY *c2);
void	mvPp2ClsC2HwClearAll(void);
int	mvPp2ClsC2HwInv(int index);
int	mvPp2ClsC2HwInvAll(void);

int	mvPp2ClsC2TcamByteSet(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char byte, unsigned char enable);
int	mvPp2ClsC2TcamByteGet(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char *byte, unsigned char *enable);
int	mvPp2ClsC2TcamByteCmp(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offs, unsigned char byte);
int 	mvPp2ClsC2TcamBytesCmp(MV_PP2_CLS_C2_ENTRY *c2, unsigned int offset, unsigned int size, unsigned char *bytes);

int	mvPp2ClsC2QosTblSet(MV_PP2_CLS_C2_ENTRY *c2, int id, int sel);
int	mvPp2ClsC2ColorSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int from);
int	mvPp2ClsC2PrioSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int prio, int form);
int	mvPp2ClsC2DscpSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int dscp, int from);
int	mvPp2ClsC2GpidSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int gpid, int from);
int	mvPp2ClsC2QueueHighSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from);
int	mvPp2ClsC2QueueLowSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from);
int	mvPp2ClsC2QueueSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int queue, int from);
int	mvPp2ClsC2ForwardSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd);

#ifdef CONFIG_MV_ETH_PP2_1
int	mvPp2ClsC2PolicerSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int policerId, int bank);
#else
int	mvPp2ClsC2PolicerSet(MV_PP2_CLS_C2_ENTRY *c2, int cmd, int policerId);
#endif

int     mvPp2ClsC2FlowIdEn(MV_PP2_CLS_C2_ENTRY *c2, int flowid_en);
int	mvPp2ClsC2ModSet(MV_PP2_CLS_C2_ENTRY *c2, int data_ptr, int instr_offs, int l4_csum);
int	mvPp2ClsC2MtuSet(MV_PP2_CLS_C2_ENTRY *c2, int mtu_inx);
int	mvPp2ClsC2DupSet(MV_PP2_CLS_C2_ENTRY *c2, int dupid, int count);
int	mvPp2ClsC2SeqSet(MV_PP2_CLS_C2_ENTRY *c2, int miss, int id);


/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine Hit counters Public APIs		    	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2HitCntrsIsBusy(void);
int mvPp2ClsC2HitCntrsClearAll(void);
int mvPp2ClsC2HitCntrRead(int index, MV_U32 *cntr);
int mvPp2ClsC2HitCntrsDump(void);

/*-------------------------------------------------------------------------------*/
/*		Classifier C2 engine debug Public APIs			    	 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsC2RegsDump(void);

#endif /* MV_CLS2_HW */
