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

#ifndef __MV_PRS_HW_H__
#define __MV_PRS_HW_H__

#include "mvTypes.h"
#include "mvCommon.h"
#include "mvOs.h"
#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"
/************************** Parser HW Configuration ***********************/

/************************** Parser Registers ******************************/

#define MV_PP2_PRS_INIT_LOOKUP_REG              (MV_PP2_REG_BASE + 0x1000)

#define MV_PP2_PRS_PORT_LU_BITS                 4
#define MV_PP2_PRS_PORT_LU_MAX			((1 << MV_PP2_PRS_PORT_LU_BITS) - 1)
#define MV_PP2_PRS_PORT_LU_MASK(port)           (MV_PP2_PRS_PORT_LU_MAX << ((port) * MV_PP2_PRS_PORT_LU_BITS))
#define MV_PP2_PRS_PORT_LU_VAL(port, val)       ((val) << ((port) * MV_PP2_PRS_PORT_LU_BITS))

/*-------------------------------------------------------------------------------*/

#define MV_PP2_PRS_INIT_OFFS_0_REG              (MV_PP2_REG_BASE + 0x1004)
#define MV_PP2_PRS_INIT_OFFS_1_REG              (MV_PP2_REG_BASE + 0x1008)

#define MV_PP2_PRS_INIT_OFFS_REG(port)         	(MV_PP2_PRS_INIT_OFFS_0_REG + ((port) & 4))
#define MV_PP2_PRS_INIT_OFF_BITS					6
#define MV_PP2_PRS_INIT_OFF_FIXED_BITS		8 /*only for offsets calculations*/
#define MV_PP2_PRS_INIT_OFF_MAX			((1 << MV_PP2_PRS_INIT_OFF_BITS) - 1)
#define MV_PP2_PRS_INIT_OFF_MASK(port)		(MV_PP2_PRS_INIT_OFF_MAX << (((port) % 4) * MV_PP2_PRS_INIT_OFF_FIXED_BITS))
#define MV_PP2_PRS_INIT_OFF_VAL(port, val)	((val) << (((port) % 4) * MV_PP2_PRS_INIT_OFF_FIXED_BITS))

/*-------------------------------------------------------------------------------*/

#define MV_PP2_PRS_MAX_LOOP_0_REG		(MV_PP2_REG_BASE + 0x100c)
#define MV_PP2_PRS_MAX_LOOP_1_REG		(MV_PP2_REG_BASE + 0x1010)

#define MV_PP2_PRS_MAX_LOOP_REG(port)		(MV_PP2_PRS_MAX_LOOP_0_REG + ((port) & 4))
#define MV_PP2_PRS_MAX_LOOP_BITS                8
#define MV_PP2_PRS_MAX_LOOP_MAX			((1 << MV_PP2_PRS_MAX_LOOP_BITS) - 1) /*MAX VALID VALUE*/
#define MV_PP2_PRS_MAX_LOOP_MIN			1 /*MIN VALID VALUE*/
#define MV_PP2_PRS_MAX_LOOP_MASK(port)		(MV_PP2_PRS_MAX_LOOP_MAX << (((port) % 4) * MV_PP2_PRS_MAX_LOOP_BITS))
#define MV_PP2_PRS_MAX_LOOP_VAL(port, val)      ((val) << (((port) % 4) * MV_PP2_PRS_MAX_LOOP_BITS))

/*-------------------------------------------------------------------------------*/

#define MV_PP2_PRS_INTR_CAUSE_REG		(MV_PP2_REG_BASE + 0x1020)
#define MV_PP2_PRS_INTR_MASK_REG		(MV_PP2_REG_BASE + 0x1024)

#define PRS_INTR_MISS				0
#define PRS_INTR_MAX_LOOPS			1
#define PRS_INTR_INV_OFF			2
#define PRS_INTR_PARITY				4
#define PRS_INTR_SRAM_PARITY			5

#define PRS_INTR_MISS_MASK			(1 << PRS_INTR_MISS)
#define PRS_INTR_MAX_LOOP_MASK			(1 << PRS_INTR_MAX_LOOPS)
#define PRS_INTR_INV_OFF_MASK			(1 << PRS_INTR_INV_OFF)
#define PRS_INTR_PARITY_MASK			(1 << PRS_INTR_PARITY)
#define PRS_INTR_SRAM_PARITY_MASK		(1 << PRS_INTR_SRAM_PARITY)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_PRS_TCAM_IDX_REG			(MV_PP2_REG_BASE + 0x1100)
#define MV_PP2_PRS_TCAM_DATA_REG(idx)		(MV_PP2_REG_BASE + 0x1104 + (idx) * 4)
#define MV_PP2_PRS_TCAM_DATA0_REG		(MV_PP2_REG_BASE + 0x1104)
#define MV_PP2_PRS_TCAM_DATA1_REG		(MV_PP2_REG_BASE + 0x1108)
#define MV_PP2_PRS_TCAM_DATA2_REG		(MV_PP2_REG_BASE + 0x110c)
#define MV_PP2_PRS_TCAM_DATA3_REG		(MV_PP2_REG_BASE + 0x1110)
#define MV_PP2_PRS_TCAM_DATA4_REG		(MV_PP2_REG_BASE + 0x1114)
#define MV_PP2_PRS_TCAM_DATA5_REG		(MV_PP2_REG_BASE + 0x1118)

#define MV_PP2_PRS_TCAM_DATA_OFFS		0
#define MV_PP2_PRS_TCAM_MASK_OFFS		16
/*-------------------------------------------------------------------------------*/

#define MV_PP2_PRS_SRAM_IDX_REG			(MV_PP2_REG_BASE + 0x1200)
#define MV_PP2_PRS_SRAM_DATA_REG(idx)		(MV_PP2_REG_BASE + 0x1204 + (idx) * 4)
#define MV_PP2_PRS_SRAM_DATA0_REG		(MV_PP2_REG_BASE + 0x1204)
#define MV_PP2_PRS_SRAM_DATA1_REG		(MV_PP2_REG_BASE + 0x1208)
#define MV_PP2_PRS_SRAM_DATA2_REG		(MV_PP2_REG_BASE + 0x120c)
#define MV_PP2_PRS_SRAM_DATA3_REG		(MV_PP2_REG_BASE + 0x1210)
/*-------------------------------------------------------------------------------*/
#define MV_PP2_PRS_EXP_REG			(MV_PP2_REG_BASE + 0x1214)
#define MV_PP2_PRS_EXP_MISS			0
#define MV_PP2_PRS_EXP_EXEED			1
#define MV_PP2_PRS_EXP_OF			2

/*-------------------------------------------------------------------------------*/
#define MV_PP2_PRS_TCAM_CTRL_REG		(MV_PP2_REG_BASE + 0x1230)
#define MV_PP2_PRS_TCAM_CTRL_EN			0

/*-------------------------------------------------------------------------------*/
/*PPv2.1 MASS 3.20 new feature */
#define MV_PP2_PRS_TCAM_HIT_IDX_REG		(MV_PP2_REG_BASE + 0x1240)
/*-------------------------------------------------------------------------------*/
/*PPv2.1 MASS 3.20 new feature */
#define MV_PP2_PRS_TCAM_HIT_CNT_REG		(MV_PP2_REG_BASE + 0x1244)
#define MV_PP2_PRS_TCAM_HIT_CNT_BITS		16
#define MV_PP2_PRS_TCAM_HIT_CNT_OFFS		0
#define MV_PP2_PRS_TCAM_HIT_CNT_MASK		\
	(((1 << MV_PP2_PRS_TCAM_HIT_CNT_BITS) - 1) << MV_PP2_PRS_TCAM_HIT_CNT_OFFS)

/*-------------------------------------------------------------------------------*/
/*				TCAM 						*/
/*-------------------------------------------------------------------------------*/
#define AI_BITS  				8
#define AI_MASK					((1 << AI_BITS) - 1)
#define AI_DONE_BIT				7
#define AI_DONE					(1 << 7)

#define PORT_BITS  				8
#define PORT_MASK				((1 << PORT_BITS) - 1)

#define LU_BITS  				4
#define LU_MASK					((1 << LU_BITS) - 1)

/****************************************************************/
/*			endians support				*/
/****************************************************************/

/* TODO: share endians support defenitions between parser and cls2  */

#if defined(MV_CPU_LE)
	#define PRS_HW_BYTE_OFFS(_offs_)	(_offs_)
	#define TCAM_MASK_OFFS(_offs_)		((_offs_) + 2)
#else
	#define PRS_HW_BYTE_OFFS(_offs_)	((3 - ((_offs_) % 4)) + (((_offs_) / 4) * 4))
	#define TCAM_MASK_OFFS(_offs_)		((_offs_) - 2)

#endif

/************************* TCAM structure **********************/
/*little endian */
#define TCAM_DATA_BYTE_OFFS_LE(_offs_)		(((_offs_) - ((_offs_) % 2)) * 2 + ((_offs_) % 2))
#define TCAM_DATA_MASK_OFFS_LE(_offs_)		(((_offs_) * 2) - ((_offs_) % 2)  + 2)

#define TCAM_BYTE_OFFS(_offs_)			PRS_HW_BYTE_OFFS(_offs_)
#define TCAM_DATA_BYTE(_offs_)			(PRS_HW_BYTE_OFFS(TCAM_DATA_BYTE_OFFS_LE(_offs_)))
#define TCAM_DATA_MASK(_offs_)			(PRS_HW_BYTE_OFFS(TCAM_DATA_MASK_OFFS_LE(_offs_)))

/*
______________________________________________
|  LKP ID  | PORT ID |    AI  | HEADER DATA  |
| 4 bits   | 1 byte  | 1 byte |   8 byte     |
----------------------------------------------
reg 5 --> reg 0
*/

#define TCAM_DATA_OFFS				0
#define TCAM_DATA_SIZE				8 /*bytes*/
#define TCAM_DATA_MAX				(TCAM_DATA_SIZE - 1) /*bytes*/
#define TCAM_DATA_WORD_MAX			((TCAM_DATA_SIZE / 4) - 1) /*words*/
#define TCAM_AI_BYTE				TCAM_BYTE_OFFS(16)

#define TCAM_PORT_BYTE				TCAM_BYTE_OFFS(17)

#define TCAM_LU_BYTE				TCAM_BYTE_OFFS(20)

/* Special bit in the TCAM register */
#define TCAM_INV_BIT				31
#define TCAM_INV_MASK				(1 << TCAM_INV_BIT)
#define TCAM_VALID				0
#define TCAM_INVALID				1
#define TCAM_INV_WORD				5


/************************* SRAM structure **********************/
/* convert bit offset to byte offset */
#define SRAM_BIT_TO_BYTE(_bit_)			PRS_HW_BYTE_OFFS((_bit_) / 8)


#define SRAM_RI_OFFS  					0
#define SRAM_RI_BITS  					32
#define SRAM_RI_MASK  					((1 << SRAM_RI_BITS) - 1)
#define SRAM_RI_WORD  					(SRAM_RI_OFFS / DWORD_BITS_LEN)

#define SRAM_RI_CTRL_OFFS  				32
#define SRAM_RI_CTRL_BITS  				32
#define SRAM_RI_CTRL_WORD  				(SRAM_RI_CTRL_OFFS / DWORD_BITS_LEN)

#define SRAM_SHIFT_OFFS  				64 /*NEXT_LKP*/
#define SRAM_SHIFT_BITS					8
#define SRAM_SHIFT_MASK					((1 << SRAM_SHIFT_BITS) - 1)

#define SRAM_SHIFT_SIGN_BIT  				72 /*NEXT_LKP_SIGN*/

#define SRAM_OFFSET_OFFS				73 /*UDF_OFF*/
#define SRAM_OFFSET_BITS				8
#define SRAM_OFFSET_MASK				((1 << SRAM_OFFSET_BITS) - 1)

#define SRAM_OFFSET_SIGN_BIT  				81 /*UDF_SIGN*/

#define SRAM_OFFSET_TYPE_OFFS  				82 /*UDF_TYPE*/
#define SRAM_OFFSET_TYPE_BITS  				3
#define SRAM_OFFSET_TYPE_MASK  				((1 << SRAM_OFFSET_TYPE_BITS) - 1)
#define SRAM_OFFSET_TYPE_PKT				0
#define SRAM_OFFSET_TYPE_L3				1
#define SRAM_OFFSET_TYPE_L4				4

#define SRAM_OP_SEL_OFFS  				85
#define SRAM_OP_SEL_BITS  				5
#define SRAM_OP_SEL_MASK  				((1 << SRAM_OP_SEL_BITS) - 1)

#define SRAM_OP_SEL_SHIFT_OFFS				85
#define SRAM_OP_SEL_SHIFT_BITS				2
#define SRAM_OP_SEL_SHIFT_MASK  			((1 << SRAM_OP_SEL_SHIFT_BITS) - 1)
#define SRAM_OP_SEL_SHIFT_ADD				1
#define SRAM_OP_SEL_SHIFT_IP4_ADD			2
#define SRAM_OP_SEL_SHIFT_IP6_ADD			3

#define SRAM_OP_SEL_OFFSET_OFFS				87
#define SRAM_OP_SEL_OFFSET_BITS				2
#define SRAM_OP_SEL_OFFSET_MASK  			((1 << SRAM_OP_SEL_OFFSET_BITS) - 1)
#define SRAM_OP_SEL_OFFSET_ADD				0
#define SRAM_OP_SEL_OFFSET_LKP_ADD			1
#define SRAM_OP_SEL_OFFSET_IP4_ADD			2
#define SRAM_OP_SEL_OFFSET_IP6_ADD			3

#define SRAM_OP_SEL_BASE_OFFS				89
#define SRAM_OP_SEL_BASE_BITS				1
#define SRAM_OP_SEL_BASE_MASK				((1 << SRAM_OP_SEL_BASE_BITS) - 1)
#define SRAM_OP_SEL_BASE_CURRENT			0
#define SRAM_OP_SEL_BASE_INIT				1

#define SRAM_AI_OFFS					90
#define SRAM_AI_BITS					8
#define SRAM_AI_MASK					((1 << SRAM_AI_BITS) - 1)
#define SRAM_AI_WORD					(SRAM_AI_OFFS / DWORD_BITS_LEN)

#define SRAM_AI_CTRL_OFFS				98
#define SRAM_AI_CTRL_BITS				8
#define SRAM_AI_CTRL_MASK				((1 << SRAM_AI_CTRL_BITS) - 1)
#define SRAM_AI_CTRL_WORD				(SRAM_AI_CTRL_OFFS / DWORD_BITS_LEN)

#define SRAM_NEXT_LU_OFFS				106 /*LOOKUP ID*/
#define SRAM_NEXT_LU_BITS				4
#define SRAM_NEXT_LU_MASK				((1 << SRAM_NEXT_LU_BITS) - 1)

#define SRAM_LU_DONE_BIT				110
#define SRAM_LU_GEN_BIT					111
/*-------------------------------------------------------------------------------*/

/* Result info bits assigment */
#define RI_MAC_ME_BIT					0
#define RI_MAC_ME_MASK					(1 << RI_MAC_ME_BIT)

#define RI_DSA_BIT                 			1
#define RI_DSA_MASK        				(1 << RI_DSA_BIT)

/* bits 2 - 3 */
#define RI_VLAN_OFFS					2
#define RI_VLAN_BITS					2
#define RI_VLAN_MASK					(((1 << RI_VLAN_BITS) - 1) << RI_VLAN_OFFS)
#define RI_VLAN_NONE          				(0 << RI_VLAN_OFFS)
#define RI_VLAN_SINGLE          			(1 << RI_VLAN_OFFS)
#define RI_VLAN_DOUBLE          			(2 << RI_VLAN_OFFS)
#define RI_VLAN_TRIPLE          			(3 << RI_VLAN_OFFS)

/* bits 4 - 6 */
#define RI_CPU_CODE_OFFS           			4 /* bits 4 - 6 */
#define RI_CPU_CODE_BITS				3
#define RI_CPU_CODE_MASK				(((1 << RI_CPU_CODE_BITS) - 1) << RI_CPU_CODE_OFFS)
#define RI_CPU_CODE_RX_SPEC_VAL				1
#define RI_CPU_CODE_RX_SPEC				(RI_CPU_CODE_RX_SPEC_VAL << RI_CPU_CODE_OFFS)

/* bits 7 - 8 */
#define RI_L2_VER_OFFS					7
#define RI_L2_VER_BITS					2
#define RI_L2_VER_MASK					(((1 << RI_L2_VER_BITS) - 1) << RI_L2_VER_OFFS)
#define RI_L2_LLC               			(0 << RI_L2_VER_OFFS)
#define RI_L2_LLC_SNAP          			(1 << RI_L2_VER_OFFS)
#define RI_L2_ETH2             				(2 << RI_L2_VER_OFFS)
#define RI_L2_OTHER             			(3 << RI_L2_VER_OFFS)

/* bits 9 - 10 */
#define RI_L2_CAST_OFFS					9
#define RI_L2_CAST_BITS					2
#define RI_L2_CAST_MASK					(((1 << RI_L2_CAST_BITS) - 1) << RI_L2_CAST_OFFS)
#define RI_L2_UCAST					(0 << RI_L2_CAST_OFFS)
#define RI_L2_MCAST					(1 << RI_L2_CAST_OFFS)
#define RI_L2_BCAST					(2 << RI_L2_CAST_OFFS)
#define RI_L2_RESERVED					(3 << RI_L2_CAST_OFFS)

/* bit 11 */
#define RI_PPPOE_BIT					11
#define RI_PPPOE_MASK					(1 << RI_PPPOE_BIT)

/* bits 12 - 14 */
#define RI_L3_PROTO_OFFS				12
#define RI_L3_PROTO_BITS				3
#define RI_L3_PROTO_MASK				(((1 << RI_L3_PROTO_BITS) - 1) << RI_L3_PROTO_OFFS)
#define RI_L3_UN              				(0 << RI_L3_PROTO_OFFS)
#define RI_L3_IP4            				(1 << RI_L3_PROTO_OFFS)
#define RI_L3_IP4_OPT          				(2 << RI_L3_PROTO_OFFS)
#define RI_L3_IP4_OTHER       				(3 << RI_L3_PROTO_OFFS)
#define RI_L3_IP6         				(4 << RI_L3_PROTO_OFFS)
#define RI_L3_IP6_EXT          				(5 << RI_L3_PROTO_OFFS)
#define RI_L3_ARP					(6 << RI_L3_PROTO_OFFS)
#define RI_L3_RESERVED					(7 << RI_L3_PROTO_OFFS)


/* bits 15 - 16 */
#define RI_L3_ADDR_OFFS       				15
#define RI_L3_ADDR_BITS       				2
#define RI_L3_ADDR_MASK					(((1 << RI_L3_ADDR_BITS) - 1) << RI_L3_ADDR_OFFS)
#define RI_L3_UCAST            				(0 << RI_L3_ADDR_OFFS)
#define RI_L3_MCAST            				(1 << RI_L3_ADDR_OFFS)
#define RI_L3_BCAST            				(2 << RI_L3_ADDR_OFFS)
#define RI_L3_ANYCAST           			(3 << RI_L3_ADDR_OFFS)

/* bit 17 */
#define RI_IP_FRAG_BIT					17
#define RI_IP_FRAG_MASK					(1 << RI_IP_FRAG_BIT)

/* Bits 18 - 19 */
#define RI_UDF2_OFFS					18
#define RI_UDF2_BITS					2
#define RI_UDF2_MASK					(((1 << RI_UDF2_BITS) - 1) << RI_UDF2_OFFS)

/* Bits 20 - 21 */
#define RI_UDF3_OFFS					20
#define RI_UDF3_BITS					2
#define RI_UDF3_MASK					(((1 << RI_UDF3_BITS) - 1) << RI_UDF3_OFFS)

/* Bits 22 - 24 */
#define RI_L4_PROTO_OFFS				22
#define RI_L4_PROTO_BITS				3
#define RI_L4_PROTO_MASK				(((1 << RI_L4_PROTO_BITS) - 1) << RI_L4_PROTO_OFFS)
#define RI_L4_UN					(0 << RI_L4_PROTO_OFFS)
#define RI_L4_TCP					(1 << RI_L4_PROTO_OFFS)
#define RI_L4_UDP					(2 << RI_L4_PROTO_OFFS)
#define RI_L4_OTHER					(3 << RI_L4_PROTO_OFFS)
									/* 3-7 user defined */
/* Bits 25 - 26 */
#define RI_UDF5_OFFS					25
#define RI_UDF5_BITS					2
#define RI_UDF5_MASK					(((1 << RI_UDF5_BITS) - 1) << RI_UDF5_OFFS)

/* Bits 27 - 28 */
#define RI_UDF6_OFFS					27
#define RI_UDF6_BITS					2
#define RI_UDF6_MASK					(((1 << RI_UDF6_BITS) - 1) << RI_UDF6_OFFS)

/* Bits 29 - 30 */
#define RI_UDF7_OFFS					29
#define RI_UDF7_BITS					2
#define RI_UDF7_MASK					(((1 << RI_UDF7_BITS) - 1) << RI_UDF7_OFFS)

/* bit 31 - drop */
#define RI_DROP_BIT					31
#define RI_DROP_MASK					(1 << RI_DROP_BIT)
/*---------------------------------------------------------------------------*/

/************* Offfset types *****************/
#define MV_PP2_PKT_OFFSET				0
#define MV_PP2_L3_OFFSET				1
#define MV_PP2_IP6_OFFSET				2
#define MV_PP2_UDF3_OFFSET				3
#define MV_PP2_L4_OFFSET				4
#define MV_PP2_UDF5_OFFSET				5
#define MV_PP2_UDF6_OFFSET				6
#define MV_PP2_UDF7_OFFSET				7


/*-------------------------------------------------------------------------------*/
/* 				Parser Shadow					 */
/*-------------------------------------------------------------------------------*/

#define PRS_TEXT_SIZE					20

typedef struct {
	int             valid;
	int		lu;
	unsigned char   text[PRS_TEXT_SIZE];
	int		udf;
	unsigned	ri;
	unsigned	riMask;
	MV_BOOL		finish;
} PRS_SHADOW_ENTRY;


void mvPp2PrsShadowSet(int index, int lu, char *text);
void mvPp2PrsShadowLuSet(int index, int lu);
int mvPp2PrsShadowUdf(int index);
void mvPp2PrsShadowUdfSet(int index, int udf);
unsigned int mvPp2PrsShadowRi(int index);
unsigned int mvPp2PrsShadowRiMask(int index);
void mvPp2PrsShadowRiSet(int index, unsigned int ri, unsigned int riMask);
void mvPp2PrsShadowFinSet(int index, MV_BOOL finish); /* set bit 111 (GEN_BIT) in SRAM */
MV_BOOL mvPp2PrsShadowFin(int index);
void mvPp2PrsShadowClear(int index);
void mvPp2PrsShadowClearAll(void);
int mvPp2PrsShadowLu(int index);
int mvPp2PrsShadowIsValid(int index);
int mvPp2PrsTcamFirstFree(int start, int end);


/*-------------------------------------------------------------------------------*/
/* 				Parser SW entry 				 */
/*-------------------------------------------------------------------------------*/

/* Parser Public TCAM APIs */
#define MV_PP2_PRS_TCAM_SIZE 				(256)

#define MV_PP2_PRC_TCAM_WORDS				6
#define MV_PP2_PRC_SRAM_WORDS				4

#define PRS_SRAM_FMT					"%4.4x %8.8x %8.8x %8.8x"
#define PRS_SRAM_VAL(p)					p[3] & 0xFFFF, p[2], p[1], p[0]

typedef union mvPp2TcamEntry {
	MV_U32 word[MV_PP2_PRC_TCAM_WORDS];
	MV_U8  byte[MV_PP2_PRC_TCAM_WORDS * 4];
} MV_PP2_TCAM_ENTRY;

typedef union mvPp2SramEntry {
	MV_U32 word[MV_PP2_PRC_SRAM_WORDS];
	MV_U8  byte[MV_PP2_PRC_SRAM_WORDS * 4];
} MV_PP2_SRAM_ENTRY;


typedef struct mvPp2PrsEntry {
	unsigned int index;
	MV_PP2_TCAM_ENTRY tcam;
	MV_PP2_SRAM_ENTRY sram;
} MV_PP2_PRS_ENTRY;


/*-------------------------------------------------------------------------------*/
/* 			Parser Public initialization APIs			 */
/*-------------------------------------------------------------------------------*/

/*
 *mvPp2PrsHwInvAll - sign all tcam entries as invalid
*/
int mvPp2PrsHwInvAll(void);

/*
 *mvPp2PrsHwClearAll - clear all tcam and sram entries
*/
int mvPp2PrsHwClearAll(void);

/*
 * mvPp2PrsHwPortInit - set first lookup fileds per port
 * @port: port number
 * @lu_first: first lookup id
 * @lu_max: max number of lookups
 * @offs: initial offset in packet
*/
int mvPp2PrsHwPortInit(int port, int lu_first, int lu_max, int offs);

/*
 * mvPrsSwAlloc - allocate new prs entry
 * @id: tcam lookup id
 */
MV_PP2_PRS_ENTRY *mvPp2PrsSwAlloc(unsigned int luId);

/*
 * mvPp2PrsSwFree
 * @pe: entry to free
*/
void mvPp2PrsSwFree(MV_PP2_PRS_ENTRY *pe);

/*-------------------------------------------------------------------------------*/
/* 			Parser internal initialization functions		 */
/*-------------------------------------------------------------------------------*/

/*
 * mvPrsHwLkpFirstSet - set first lookup id per port
 * @port: port number
 * @lu_first: first lookup id
*/
int mvPrsHwLkpFirstSet(int port, int lu_first);

/*
 * mvPrsHwLkpMaxSet - set max number of lookups per port
 * @port: port number
 * @lu_max: max number of lookups
*/
int mvPrsHwLkpMaxSet(int port, int lu_max);

/*
 * mvPrsHwLkpMaxSet - set first lookup initial packet offset
 * @port: port number
 * @offs: initial offset in packet
*/
int mvPrsHwLkpFirstOffsSet(int port, int off);

/*-------------------------------------------------------------------------------*/
/* 			Parser Public TCAM APIs 				*/
/*-------------------------------------------------------------------------------*/

/*
 * mvPp2PrsHwRead - read prs entry
*/
int mvPp2PrsHwRead(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsHwWrite - write prs entry
*/
int mvPp2PrsHwWrite(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsHwInv - invalidate prs entry
 * @tid: entry id
*/
int mvPp2PrsHwInv(int tid);

/*
 * mvPp2PrsHwRegsDump - dump all prs registers into buffer
*/
int mvPp2PrsHwRegsDump(void);

/*
 * mvPp2PrsSwDump - dump sw prs entry
 * @pe: sw prs entry
*/
int mvPp2PrsSwDump(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsSwClear - clear prs sw entry
 * @pe: sw prs entry
*/
void mvPp2PrsSwClear(MV_PP2_PRS_ENTRY *pe);
/*
 * mvPp2PrsHwDump - dump all valid hw entries
*/

int mvPp2PrsHwDump(void);

/*
	mvPp2V1PrsHwHitsDump - dump all entries with non zeroed hit counters
*/
int mvPp2V1PrsHwHitsDump(void);

/*
	enable - Tcam Ebable/Disable
*/

int mvPp2PrsSwTcam(int enable);

/*
 * mvPp2PrsSwTcamWordGet - get byte form tcam data and tcam mask
 * @pe: sw prs entry
 * @offs: offset in tcam data, valid value 0 - 7
 * @woed: data from tcam
 * @enablek: data from tcam mask
*/
int mvPp2PrsSwTcamWordGet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int *word, unsigned int *enable);
/*
 * mvPp2PrsSwTcamWordSet - set byte in tcam data and tcam mask
 * @pe: sw prs entry
 * @offs: offset in tcam data, valid value 0, 1
 * @word: data to tcam
 * enable: data to tcam mask
*/
int mvPp2PrsSwTcamWordSet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int word, unsigned int mask);

/*
 * mvPp2PrsSwTcamByteGet - get byte form tcam data and tcam mask
 * @pe: sw prs entry
 * @offs: offset in tcam data, valid value 0, 1
 * @byte: data from tcam
 * @mask: data from tcam mask
*/
int mvPp2PrsSwTcamByteGet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char *byte, unsigned char *enable);
/*
 * mvPp2PrsSwTcamByteSet - set byte in tcam data and tcam mask
 * @pe: sw prs entry
 * @offs: offset in tcam data, valid value 0 - 7
 * @byte: data to tcam
 * enable: data to tcam mask
*/
int mvPp2PrsSwTcamByteSet(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char byte, unsigned char mask);

/*
 * mvPp2PrsSwTcamByteCmp - compare one byte in tcam data of prs sw entery
 * @pe: sw prs entry
 * @offs: offset in tcam data, valid value 0 - 7
 * @byte: data to compare
 * return value: tcam[off] & tcam_mask[off] == byte & tcam_mask[off]
*/
int mvPp2PrsSwTcamByteCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned char byte);

/*
 * mvPp2PrsSwTcamBytesCmp - compare bytes sequence in tcam data of prs sw entery
 * call to  mvPp2PrsSwTcamByteCmp fpr each byte in the sequence.
 * @pe: sw prs entry
 * @offs: bytes sequence start offset in tcam dat
 * @size: number of bytes to compare
 * return value: tcam[off] & tcam_mask[off] == byte & tcam_mask[off] for all bytes
*/
int mvPp2PrsSwTcamBytesCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offset, unsigned int size, unsigned char *bytes);

/*
 * mvPp2PrsSwTcamBytesCmpIgnorMask - compare bytes sequence in tcam data of prs sw entery
 * call to  mvPp2PrsSwTcamByteCmp fpr each byte in the sequence.
 * @pe: sw prs entry
 * @offs: bytes sequence start offset in tcam dat
 * @size: number of bytes to compare
 * return value: tcam[off] == byte for all bytes
*/
int mvPp2PrsSwTcamBytesIgnorMaskCmp(MV_PP2_PRS_ENTRY *pe, unsigned int offs, unsigned int size, unsigned char *bytes);
/*
 * mvPp2PrsSwTcamAiUpdate - update tcam ai bits in prs sw entry.
 * @pe: sw prs entry
 * @bits: bits to set
 * @enable: bits mask
 * tcam AI[i] <-- bits[i] only if  enable[i] is set.
 * tcam_mask AI[i] <--1 only if enable[i] is set.
*/
int mvPp2PrsSwTcamAiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable);

/*
 * mvPp2PrsSwTcamAiGet - get tcam AI and tcam_mask AI from prs sw entry.
 * @pe: sw prs entry
 * @bits: get tcam AI val
 * @enable: get tcam mask AI val
*/
int mvPp2PrsSwTcamAiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable);

/*
 * mvPp2PrsSwTcamAiSetBit - set tcam AI bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset
 * tcam AI[bit] = 1 , tcam mask AI[bit] = 1
*/
int mvPp2PrsSwTcamAiSetBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit);

/*
 * mvPp2PrsSwTcamAiClearBit - clear tcam AI bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset
 * tcam AI[bit] = 0 , tcam mask AI[bit] = 1
*/
int mvPp2PrsSwTcamAiClearBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit);
/*
 * mvPp2PrsSwTcamPortGet - return tcam port status in prs sw entry.
 * @pe: sw prs entry
 * @port: single port
 * @status: 1 - port bit is set, 0 - port bit is not set
*/

int mvPp2PrsSwTcamPortGet(MV_PP2_PRS_ENTRY *pe, unsigned int port, MV_BOOL *status);
/*
 * mvPp2PrsSwTcamPortSet - set tcam port map in prs sw entry.
 * @pe: sw prs entry
 * @port: single port to be add or delete
 * @add: 1 - add port, 0 - delete port
*/
int mvPp2PrsSwTcamPortSet(MV_PP2_PRS_ENTRY *pe, unsigned int port, int add);

/*
 * mvPp2PrsSwTcamPortMapSet - set tcam port map in prs sw entry.
 * @pe: sw prs entry
 * @ports: ports bitmap to be set
*/
int mvPp2PrsSwTcamPortMapSet(MV_PP2_PRS_ENTRY *pe, unsigned int ports);

/*
 * mvPp2PrsSwTcamPortMapGet - get tcam PORT bitmap from prs sw entry.
 * @pe: sw prs entry
 * @port: get tcam PORTS val
*/
int mvPp2PrsSwTcamPortMapGet(MV_PP2_PRS_ENTRY *pe, unsigned int *ports);

/*
 * mvPp2PrsSwTcamLuSet - set tcam lookup id in prs sw entry.
 * @pe: sw prs entry
 * @lu: lookup id
 * set tcam mask LU to 0xff
*/
int mvPp2PrsSwTcamLuSet(MV_PP2_PRS_ENTRY *pe, unsigned int lu);

/*
 * mvPp2PrsSwTcamLuGet - get tcam lookup id from prs sw entry.
 * @pe: sw prs entry
 * @lu: get tcam lookup id
 * @enable: get tcam mask lookup id
*/
int mvPp2PrsSwTcamLuGet(MV_PP2_PRS_ENTRY *pe, unsigned int *lu, unsigned int *enable);


/*
 * mvPp2PrsSwSramRiSetBit - set sram result info bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset in result info
 * set sram RI_EN[bit]
 */
int mvPp2PrsSwSramRiSetBit(MV_PP2_PRS_ENTRY *pe, unsigned int bit);

/*
 * mvPp2PrsSwSramRiClearBit - clear sram result info bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset in result info
 * set sram RI_EN[bit]
 */
int mvPp2PrsSwSramRiClearBit(MV_PP2_PRS_ENTRY *pe, unsigned int bit);

/*
 * mvPp2PrsSwSramRiUpdate - update sram result info bits in prs sw entry.
 * @pe: sw prs entry
 * @bits: bits to set
 * @enable: bits mask
 * sram RI[i] <-- bits[i] only if  sram RI_EN[i] is set.
 * sram RI_EN[i] <--1 only if enable[i] is set.
 */
int mvPp2PrsSwSramRiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable);
/*
 * mvPp2PrsSwSramRiSet - set sram result info bits in prs sw entry.
 * @pe: sw prs entry
 * @bits: bits to set
 * @enable: bits mask
  */

int mvPp2PrsSwSramRiSet(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable);
/*
 * mvPp2PrsSwSramRiGet - get sram result info from prs sw entry.
 * @pe: sw prs entry
 * @bits: get result info bits
 * @enable: get result info update bits
*/
int mvPp2PrsSwSramRiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable);


/*
 * mvPp2PrsSwSramAiSetBit - set sram AI bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset
 * sram AI[bit] = 1 , sram AI_EN[bit] = 1
*/
int mvPp2PrsSwSramAiSetBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit);

/*
 * mvPp2PrsSwSramAiClearBit - clear sram AI bit in prs sw entry.
 * @pe: sw prs entry
 * @bit: bit offset
 * sram AI[bit] = 0 , sram AI_EN[bit] = 1
*/
int mvPp2PrsSwSramAiClearBit(MV_PP2_PRS_ENTRY *pe, unsigned char bit);

/*
 * mvPp2PrsSwSramAiUpdate - update sram ai bits in prs sw entry.
 * @pe: sw prs entry
 * @bits: bits to set
 * @enable: bits mask
 * sram AI[i] <-- bits[i] only if  enable[i] is set.
 * sram AI_EN[i] <--1 only if enable[i] is set.
*/
int mvPp2PrsSwSramAiUpdate(MV_PP2_PRS_ENTRY *pe, unsigned int bits, unsigned int enable);

/*
 * mvPp2PrsSwSramAiGet - get sram AI and AI_EN from prs sw entry.
 * @pe: sw prs entry
 * @bits: get sram AI val
 * @enable: get sram AI_EN val
*/
int mvPp2PrsSwSramAiGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bits, unsigned int *enable);


/*
 * mvPp2PrsSwSramNextLuSet - set prs sram next lookup id.
 * @pe: sw prs entry
 * @lu: next lookup id
*/
int mvPp2PrsSwSramNextLuSet(MV_PP2_PRS_ENTRY *pe, unsigned int lu);

/*
 * mvPp2PrsSwSramNextLuGet - got prs sram next lookup id.
 * @pe: sw prs entry
 * @lu: get next lookup id
*/
int mvPp2PrsSwSramNextLuGet(MV_PP2_PRS_ENTRY *pe, unsigned int *lu);


/*
 * mvPp2PrsSwSramShiftSet - set prs sram shift.
 * @pe: sw prs entry
 * @shift
 * @op
*/
int mvPp2PrsSwSramShiftSet(MV_PP2_PRS_ENTRY *pe, int shift, unsigned int op);

/*
 * mvPp2PrsSwSramShiftGet - get prs sram shift.
 * @pe: sw prs entry
 * @shift: get shift val
 * @op: get shift op
*/
int mvPp2PrsSwSramShiftGet(MV_PP2_PRS_ENTRY *pe, int *shift);

/*
 * mvPp2PrsSwSramShiftAbsUpdate - set sram shift value according to initial offset
 * @pe: sw prs entry
 * @shift: get shift val
 * shift value = @shift + inital value (store in reg 0x1004, 0x1008)
*/
int mvPp2PrsSwSramShiftAbsUpdate(MV_PP2_PRS_ENTRY *pe, int shift, unsigned int op);

/*
 * mvPp2PrsSwSramOffsetSet - set prs sram offset.
 * @pe: sw prs entry
 * @type: offset type
 * @offset: signed offset value
 * @op: offset operation
*/
int mvPp2PrsSwSramOffsetSet(MV_PP2_PRS_ENTRY *pe, unsigned int type, int offset, unsigned int op);

/*
 * mvPp2PrsSwSramOffsetGet - get prs sram offset.
 * @pe: sw prs entry
 * @type: get offset type
 * @offset: get offset
 * @op: get offset operation
*/
int mvPp2PrsSwSramOffsetGet(MV_PP2_PRS_ENTRY *pe, unsigned int *type, int *offset, unsigned int *op);

/*
 * mvPp2PrsSwSramLuDoneSet - set prs sram lookup done bit.
 * @pe: sw prs entry
*/
int mvPp2PrsSwSramLuDoneSet(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsSwSramLuDoneClear - clear prs sram lookup done bit.
 * @pe: sw prs entry
*/
int mvPp2PrsSwSramLuDoneClear(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsSwSramLuDoneGet - get prs sram lookup done bit.
 * @pe: sw prs entry
 * bit: get lookup done bit
*/
int mvPp2PrsSwSramLuDoneGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bit);

/*
 * mvPp2PrsSwSramFlowidGenSet - set prs sram flowid gen bit.
 * @pe: sw prs entry
*/
int mvPp2PrsSwSramFlowidGenSet(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsSwSramFlowidGenClear - clear prs sram flowid gen bit.
 * @pe: sw prs entry
*/
int mvPp2PrsSwSramFlowidGenClear(MV_PP2_PRS_ENTRY *pe);

/*
 * mvPp2PrsSwSramFlowidGenGet - get prs sram flowid gen bit.
 * @pe: sw prs entry
 * bit: get lowid gen bit.
*/
int mvPp2PrsSwSramFlowidGenGet(MV_PP2_PRS_ENTRY *pe, unsigned int *bit);

#endif /* __MV_PRS_HW_H__ */
