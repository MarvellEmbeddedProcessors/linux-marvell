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


#ifndef __mvPmt_h__
#define __mvPmt_h__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "mvTypes.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"

#define PMT_TEXT    16

#define PMT_PRINT_VALID_FLAG    0x01


typedef union mv_neta_pmt_t {
	MV_U32	word;

} MV_NETA_PMT;

#define MV_NETA_PMT_DATA_OFFS       0
#define MV_NETA_PMT_DATA_BITS       16
#define MV_NETA_PMT_DATA_MASK       (((1 << MV_NETA_PMT_DATA_BITS) - 1) << MV_NETA_PMT_DATA_OFFS)

#define MV_NETA_PMT_CTRL_OFFS       16
#define MV_NETA_PMT_CTRL_BITS       16
#define MV_NETA_PMT_CTRL_MASK       (((1 << MV_NETA_PMT_CTRL_BITS) - 1) << MV_NETA_PMT_CTRL_OFFS)

#define MV_NETA_PMT_CMD_OFFS        16
#define MV_NETA_PMT_CMD_BITS        5
#define MV_NETA_PMT_CMD_ALL_MASK    (((1 << MV_NETA_PMT_CMD_BITS) - 1) << MV_NETA_PMT_CMD_OFFS)
#define MV_NETA_PMT_CMD_MASK(cmd)   ((cmd) << MV_NETA_PMT_CMD_OFFS)

enum {
    MV_NETA_CMD_NONE        = 0,
    MV_NETA_CMD_ADD_2B,
    MV_NETA_CMD_CFG_VLAN,
    MV_NETA_CMD_ADD_VLAN,
    MV_NETA_CMD_CFG_DSA_1,
    MV_NETA_CMD_CFG_DSA_2,
    MV_NETA_CMD_ADD_DSA,
    MV_NETA_CMD_DEL_BYTES,
    MV_NETA_CMD_REPLACE_2B,
    MV_NETA_CMD_REPLACE_LSB,
    MV_NETA_CMD_REPLACE_MSB,
    MV_NETA_CMD_REPLACE_VLAN,
    MV_NETA_CMD_DEC_LSB,
    MV_NETA_CMD_DEC_MSB,
    MV_NETA_CMD_ADD_CALC_LEN,
    MV_NETA_CMD_REPLACE_LEN,
    MV_NETA_CMD_IPV4_CSUM,
    MV_NETA_CMD_L4_CSUM,
    MV_NETA_CMD_SKIP,
    MV_NETA_CMD_JUMP,
    MV_NETA_CMD_JUMP_SKIP,
    MV_NETA_CMD_JUMP_SUB,
    MV_NETA_CMD_PPPOE,
    MV_NETA_CMD_STORE,
};

#define MV_NETA_PMT_IP4_CSUM_BIT    21
#define MV_NETA_PMT_IP4_CSUM_MASK   (1 << MV_NETA_PMT_IP4_CSUM_BIT)

#define MV_NETA_PMT_L4_CSUM_BIT     22
#define MV_NETA_PMT_L4_CSUM_MASK    (1 << MV_NETA_PMT_L4_CSUM_BIT)

#define MV_NETA_PMT_LAST_BIT        23
#define MV_NETA_PMT_LAST_MASK       (1 << MV_NETA_PMT_LAST_BIT)


/*********** Command special defines ************/

/* Bits for MV_NETA_CMD_DEL_BYTES command */
/* [7:0] - number of words (2 bytes) to delete */
#define MV_NETA_PMT_DEL_SHORTS_OFFS         0
#define MV_NETA_PMT_DEL_SHORTS_MASK         (0xFF << MV_NETA_PMT_DEL_SHORTS_OFFS)
#define MV_NETA_PMT_DEL_SHORTS(size)        (((size) << MV_NETA_PMT_DEL_SHORTS_OFFS) & MV_NETA_PMT_DEL_SHORTS_MASK)

/* [11:8] - number of words (2 bytes) to skip before the delete command */
#define MV_NETA_PMT_DEL_SKIP_B_OFFS         8
#define MV_NETA_PMT_DEL_SKIP_B_MASK         (0xF << MV_NETA_PMT_DEL_SKIP_B_OFFS)
#define MV_NETA_PMT_DEL_SKIP_B(size)        (((size) << MV_NETA_PMT_DEL_SKIP_B_OFFS) & MV_NETA_PMT_DEL_SKIP_B_MASK)

/* [15:12] - number of words (2 bytes) to skip after the delete command */
#define MV_NETA_PMT_DEL_SKIP_A_OFFS         12
#define MV_NETA_PMT_DEL_SKIP_A_MASK         (0xF << MV_NETA_PMT_DEL_SKIP_A_OFFS)
#define MV_NETA_PMT_DEL_SKIP_A(size)        (((size) << MV_NETA_PMT_DEL_SKIP_A_OFFS) & MV_NETA_PMT_DEL_SKIP_A_MASK)
/*-----------------------------------------------------------------------------------------------------------------*/

/* Bits for Add Calculated length operation */
/* Used for commands: Add Calculated length, Replace length, Skip, */
#define MV_NETA_PMT_ZERO_ADD                0
#define MV_NETA_PMT_DATA_ADD                2
#define MV_NETA_PMT_DATA_SUB                3

#define MV_NETA_PMT_CALC_LEN_0_OFFS         14
#define MV_NETA_PMT_CALC_LEN_0_MASK         (3 << MV_NETA_PMT_CALC_LEN_0_OFFS)
#define MV_NETA_PMT_CALC_LEN_0_ZERO         (0 << MV_NETA_PMT_CALC_LEN_0_OFFS)
#define MV_NETA_PMT_CALC_LEN_0_TX_DESC      (1 << MV_NETA_PMT_CALC_LEN_0_OFFS)
#define MV_NETA_PMT_CALC_LEN_0_TX_PKT       (2 << MV_NETA_PMT_CALC_LEN_0_OFFS)
#define MV_NETA_PMT_CALC_LEN_0_STORE        (3 << MV_NETA_PMT_CALC_LEN_0_OFFS)

#define MV_NETA_PMT_CALC_LEN_1_OFFS         12
#define MV_NETA_PMT_CALC_LEN_1_MASK         (3 << MV_NETA_PMT_CALC_LEN_1_OFFS)
#define MV_NETA_PMT_CALC_LEN_1(op)          ((op) << MV_NETA_PMT_CALC_LEN_1_OFFS)

#define MV_NETA_PMT_CALC_LEN_2_OFFS         10
#define MV_NETA_PMT_CALC_LEN_2_MASK         (3 << MV_NETA_PMT_CALC_LEN_2_OFFS)
#define MV_NETA_PMT_CALC_LEN_2(op)          ((op) << MV_NETA_PMT_CALC_LEN_2_OFFS)

#define MV_NETA_PMT_CALC_LEN_3_BIT          9
#define MV_NETA_PMT_CALC_LEN_3_ADD_MASK     (0 << MV_NETA_PMT_CALC_LEN_3_BIT)
#define MV_NETA_PMT_CALC_LEN_3_SUB_MASK     (1 << MV_NETA_PMT_CALC_LEN_3_BIT)

#define MV_NETA_PMT_CALC_LEN_DATA_OFFS      0
#define MV_NETA_PMT_CALC_LEN_DATA_MASK      (0x1FF << MV_NETA_PMT_CALC_LEN_DATA_OFFS)
#define MV_NETA_PMT_CALC_LEN_DATA(data)     ((data) << MV_NETA_PMT_CALC_LEN_DATA_OFFS)
/*-----------------------------------------------------------------------------------------------------------------*/

/* Bits for MV_NETA_CMD_DEC_LSB and MV_NETA_CMD_DEC_MSB commands */
#define MV_NETA_PMT_DEC_SKIP_A_OFFS         0
#define MV_NETA_PMT_DEC_SKIP_A_MASK         (0xFF << MV_NETA_PMT_DEC_SKIP_A_OFFS)
#define MV_NETA_PMT_DEC_SKIP_A(size)        (((size) << MV_NETA_PMT_DEC_SKIP_A_OFFS) & MV_NETA_PMT_DEC_SKIP_A_MASK)

#define MV_NETA_PMT_DEC_SKIP_B_OFFS         8
#define MV_NETA_PMT_DEC_SKIP_B_MASK         (0xFF << MV_NETA_PMT_DEC_SKIP_B_OFFS)
#define MV_NETA_PMT_DEC_SKIP_B(size)        (((size) << MV_NETA_PMT_DEC_SKIP_B_OFFS) & MV_NETA_PMT_DEC_SKIP_B_MASK)
/*-----------------------------------------------------------------------------------------------------------------*/

#define MV_NETA_PMT_CLEAR(pmt)          \
		(pmt)->word = 0;

#define MV_NETA_PMT_IS_VALID(pmt)        \
		((((pmt)->word & MV_NETA_PMT_CMD_ALL_MASK) >> MV_NETA_PMT_CMD_OFFS) != MV_NETA_CMD_NONE)

#define MV_NETA_PMT_INVALID_SET(pmt)        \
		((pmt)->word = MV_NETA_PMT_CMD_MASK(MV_NETA_CMD_NONE) | MV_NETA_PMT_LAST_MASK);

#define MV_NETA_PMT_CTRL_GET(pmt)           \
		(MV_U16)(((pmt)->word & MV_NETA_PMT_CTRL_MASK) >> MV_NETA_PMT_CTRL_OFFS)

#define MV_NETA_PMT_CMD_GET(pmt)           \
		(((pmt)->word & MV_NETA_PMT_CMD_ALL_MASK) >> MV_NETA_PMT_CMD_OFFS)

#define MV_NETA_PMT_DATA_GET(pmt)           \
		(MV_U16)(((pmt)->word & MV_NETA_PMT_DATA_MASK) >> MV_NETA_PMT_DATA_OFFS)

#define MV_NETA_PMT_CMD_SET(pmt, cmd)                       \
		(pmt)->word &= ~MV_NETA_PMT_CMD_ALL_MASK;       \
		(pmt)->word |= MV_NETA_PMT_CMD_MASK(cmd);

#define MV_NETA_PMT_DATA_SET(pmt, data)                         \
		(pmt)->word &= ~MV_NETA_PMT_DATA_MASK;              \
		(pmt)->word |= ((data) << MV_NETA_PMT_DATA_OFFS);


MV_STATUS   mvNetaPmtWrite(int port, int idx, MV_NETA_PMT *pEntry);
MV_STATUS   mvNetaPmtRead(int port, int idx, MV_NETA_PMT *pEntry);
MV_STATUS   mvNetaPmtClear(int port);
MV_STATUS   mvNetaPmtInit(int port, MV_NETA_PMT *pBase);
MV_VOID	    mvNetaPmtDestroy(MV_VOID);
MV_VOID	    mvNetaPmtDump(int port, int flags);
MV_VOID        mvNetaPmtRegs(int port, int txp);

MV_VOID        mvNetaPmtEntryPrint(MV_NETA_PMT *pEntry);

MV_VOID        mvNetaPmtAdd2Bytes(MV_NETA_PMT *pEntry, MV_U16 data);
MV_VOID        mvNetaPmtReplace2Bytes(MV_NETA_PMT *pEntry, MV_U16 data);
MV_VOID        mvNetaPmtDelShorts(MV_NETA_PMT *pEntry, MV_U8 bDelete,
				MV_U8 skipBefore, MV_U8 skipAfter);
MV_VOID        mvNetaPmtReplaceLSB(MV_NETA_PMT *pEntry, MV_U8 value, MV_U8 mask);
MV_VOID        mvNetaPmtReplaceMSB(MV_NETA_PMT *pEntry, MV_U8 value, MV_U8 mask);

MV_VOID        mvNetaPmtDecLSB(MV_NETA_PMT *pEntry, MV_U8 skipBefore, MV_U8 skipAfter);
MV_VOID        mvNetaPmtDecMSB(MV_NETA_PMT *pEntry, MV_U8 skipBefore, MV_U8 skipAfter);

MV_VOID        mvNetaPmtLastFlag(MV_NETA_PMT *pEntry, int last);
MV_VOID        mvNetaPmtFlags(MV_NETA_PMT *pEntry, int last, int ipv4, int l4);
MV_VOID        mvNetaPmtSkip(MV_NETA_PMT *pEntry, MV_U16 shorts);
MV_VOID        mvNetaPmtReplaceIPv4csum(MV_NETA_PMT *pEntry, MV_U16 data);
MV_VOID        mvNetaPmtReplaceL4csum(MV_NETA_PMT *pEntry, MV_U16 data);
MV_VOID        mvNetaPmtJump(MV_NETA_PMT *pEntry, MV_U16 target, int type, int cond);

/* High level PMT configurations */
int         mvNetaPmtTtlDec(int port, int idx, int ip_offs, int isLast);
int         mvNetaPmtDataReplace(int port, int idx, int offset,
				 MV_U8 *data, int bytes, int isLast);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __mvPmt_h__ */
