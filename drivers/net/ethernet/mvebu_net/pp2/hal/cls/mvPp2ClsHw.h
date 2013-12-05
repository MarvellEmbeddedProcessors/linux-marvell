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

#ifndef __MV_CLS_HW_H__
#define __MV_CLS_HW_H__

#include "../common/mvPp2ErrCode.h"
#include "../common/mvPp2Common.h"
#include "../gbe/mvPp2GbeRegs.h"
#include "../gbe/mvPp2Gbe.h"
/*-------------------------------------------------------------------------------*/
/*			Classifier Top Registers	    			 */
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_MODE_REG			(MV_PP2_REG_BASE + 0x1800)

#define MV_PP2_CLS_MODE_ACTIVE_BIT		0
#define MV_PP2_CLS_MODE_ACTIVE_MASK		(1 << MV_PP2_CLS_MODE_ACTIVE_BIT)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_PORT_WAY_REG			(MV_PP2_REG_BASE + 0x1810)

#define MV_PP2_CLS_PORT_WAY_OFFS		0
#define MV_PP2_CLS_PORT_WAY_MASK(port)		(1 << ((port) + MV_PP2_CLS_PORT_WAY_OFFS))
#define WAY_MAX					1
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_LKP_INDEX_REG		(MV_PP2_REG_BASE + 0x1814)

#define MV_PP2_CLS_LKP_INDEX_LKP_OFFS		0
#define MV_PP2_CLS_LKP_INDEX_WAY_OFFS		6
#define MV_PP2_CLS_LKP_INDEX_BITS		7
#define MV_PP2_CLS_LKP_INDEX_MASK		((1 << MV_PP2_CLS_LKP_INDEX_BITS) - 1)
#define MV_PP2_CLS_LKP_WAY_MASK			(1 << MV_PP2_CLS_LKP_INDEX_WAY_OFFS)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_LKP_TBL_REG			(MV_PP2_REG_BASE + 0x1818)

/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_FLOW_INDEX_REG		(MV_PP2_REG_BASE + 0x1820)

#define MV_PP2_CLS_FLOW_INDEX_BITS		9
#define MV_PP2_CLS_FLOW_INDEX_MASK		((1 << MV_PP2_CLS_FLOW_INDEX_BITS) - 1)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_FLOW_TBL0_REG		(MV_PP2_REG_BASE + 0x1824)
#define MV_PP2_CLS_FLOW_TBL1_REG		(MV_PP2_REG_BASE + 0x1828)
#define MV_PP2_CLS_FLOW_TBL2_REG		(MV_PP2_REG_BASE + 0x182c)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_PORT_SPID_REG		(MV_PP2_REG_BASE + 0x1830)

#define MV_PP2_CLS_PORT_SPID_BITS		2
#define MV_PP2_CLS_PORT_SPID_MAX		((1 << MV_PP2_CLS_PORT_SPID_BITS) - 1)
#define MV_PP2_CLS_PORT_SPID_MASK(port)		((MV_PP2_CLS_PORT_SPID_MAX) << ((port) * MV_PP2_CLS_PORT_SPID_BITS))
#define MV_PP2_CLS_PORT_SPID_VAL(port, val)	((val) << ((port) * MV_PP2_CLS_PORT_SPID_BITS));

/* PORT - SPID types */
#define PORT_SPID_MH				0
#define PORT_SPID_EXT_SWITCH			1
#define PORT_SPID_CAS_SWITCH			2
#define PORT_SPID_PORT_TRUNK			3
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_SPID_UNI_BASE_REG		(MV_PP2_REG_BASE + 0x1840)
#define MV_PP2_CLS_SPID_UNI_REG(spid)		(MV_PP2_CLS_SPID_UNI_BASE_REG + (((spid) >> 3) * 4))

#define MV_PP2_CLS_SPID_MAX			31
#define MV_PP2_CLS_SPID_UNI_REGS		4
#define MV_PP2_CLS_SPID_UNI_BITS		3
#define MV_PP2_CLS_SPID_UNI_FIXED_BITS		4
#define MV_PP2_CLS_SPID_UNI_MAX			((1 << MV_PP2_CLS_SPID_UNI_BITS) - 1)
#define MV_PP2_CLS_SPID_UNI_OFFS(spid)		(((spid) % 8) * MV_PP2_CLS_SPID_UNI_FIXED_BITS)
#define MV_PP2_CLS_SPID_UNI_MASK(spid)		((MV_PP2_CLS_SPID_UNI_MAX) << (MV_PP2_CLS_SPID_UNI_OFFS(spid)))
#define MV_PP2_CLS_SPID_UNI_VAL(spid, val)	((val) << (MV_PP2_CLS_SPID_UNI_OFFS(spid)))
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_GEM_VIRT_INDEX_REG		(MV_PP2_REG_BASE + 0x1A00)
#define MV_PP2_CLS_GEM_VIRT_INDEX_BITS		(7)
#define MV_PP2_CLS_GEM_VIRT_INDEX_MAX		(((1 << MV_PP2_CLS_GEM_VIRT_INDEX_BITS) - 1) << 0)
/*-------------------------------------------------------------------------------*/
#ifdef CONFIG_MV_ETH_PP2_1
/* indirect rd/wr via index GEM_VIRT_INDEX */
#define MV_PP2_CLS_GEM_VIRT_REGS_NUM		128
#define MV_PP2_CLS_GEM_VIRT_REG			(MV_PP2_REG_BASE + 0x1A04)
#else
/* direct rd/wr */
#define MV_PP2_CLS_GEM_VIRT_REGS_NUM		64
#define MV_PP2_CLS_GEM_VIRT_BASE_REG		(MV_PP2_REG_BASE + 0x1A00)
#define MV_PP2_CLS_GEM_VIRT_REG(index)		(MV_PP2_CLS_GEM_VIRT_BASE_REG + ((index) * 4))
#endif

#define MV_PP2_CLS_GEM_VIRT_BITS		12
#define MV_PP2_CLS_GEM_VIRT_MAX			((1 << MV_PP2_CLS_GEM_VIRT_BITS) - 1)
#define MV_PP2_CLS_GEM_VIRT_MASK		(((1 << MV_PP2_CLS_GEM_VIRT_BITS) - 1) << 0)

/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_UDF_BASE_REG			(MV_PP2_REG_BASE + 0x1860)
#define MV_PP2_CLS_UDF_REG(index)		(MV_PP2_CLS_UDF_BASE_REG + ((index) * 4)) /*index <=63*/
#define MV_PP2_CLS_UDF_REGS_NUM			64

#define MV_PP2_CLS_UDF_BASE_REGS		8
#define MV_PP2_CLS_UDF_OFFSET_ID_OFFS		0
#define MV_PP2_CLS_UDF_OFFSET_ID_BITS		4
#define MV_PP2_CLS_UDF_OFFSET_ID_MAX		((1 << MV_PP2_CLS_UDF_OFFSET_ID_BITS) - 1)
#define MV_PP2_CLS_UDF_OFFSET_ID_MASK		((MV_PP2_CLS_UDF_OFFSET_ID_MAX) << MV_PP2_CLS_UDF_OFFSET_ID_OFFS)

#define MV_PP2_CLS_UDF_REL_OFFSET_OFFS		4
#define MV_PP2_CLS_UDF_REL_OFFSET_BITS		11
#define MV_PP2_CLS_UDF_REL_OFFSET_MAX		((1 << MV_PP2_CLS_UDF_REL_OFFSET_BITS) - 1)
#define MV_PP2_CLS_UDF_REL_OFFSET_MASK		((MV_PP2_CLS_UDF_REL_OFFSET_MAX) << MV_PP2_CLS_UDF_REL_OFFSET_OFFS)

#define MV_PP2_CLS_UDF_SIZE_OFFS		16
#define MV_PP2_CLS_UDF_SIZE_BITS		8
#define MV_PP2_CLS_UDF_SIZE_MAX			((1 << MV_PP2_CLS_UDF_SIZE_BITS) - 1)
#define MV_PP2_CLS_UDF_SIZE_MASK		(((1 << MV_PP2_CLS_UDF_SIZE_BITS) - 1) << MV_PP2_CLS_UDF_SIZE_OFFS)
/*-------------------------------------------------------------------------------*/

#define MV_PP2_CLS_MTU_BASE_REG			(MV_PP2_REG_BASE + 0x1900)
/*
  in PPv2.1 (feature MAS 3.7) num indicate an mtu reg index
  in PPv2.0 num (<=31) indicate eport number , 0-15 pon txq,  16-23 ethernet
*/
#define MV_PP2_CLS_MTU_REG(num)			(MV_PP2_CLS_MTU_BASE_REG + ((num) * 4))
#define MV_PP2_CLS_MTU_OFFS			0
#define MV_PP2_CLS_MTU_BITS			16
#define MV_PP2_CLS_MTU_MAX			((1 << MV_PP2_CLS_MTU_BITS) - 1)
#define MV_PP2_CLS_MTU_MASK			(((1 << MV_PP2_CLS_MTU_BITS) - 1) << MV_PP2_CLS_MTU_OFFS)
/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.7) MV_PP2_V0_CLS_OVERSIZE_RXQ_BASE_REG removed
*/
#define MV_PP2_V0_CLS_OVERSIZE_RXQ_BASE_REG	(MV_PP2_REG_BASE + 0x1980)
#define MV_PP2_CLS_OVERSIZE_RXQ_REG(eport)	(MV_PP2_V0_CLS_OVERSIZE_RXQ_BASE_REG + (4 * (eport))) /*eport <=23*/
#define MV_PP2_CLS_OVERSIZE_RXQ_BITS		8
#define MV_PP2_CLS_OVERSIZE_RXQ_MAX		((1 << MV_PP2_CLS_OVERSIZE_RXQ_BITS) - 1)
#define MV_PP2_CLS_OVERSIZE_RXQ_OFFS		0
#define MV_PP2_CLS_OVERSIZE_RX_MASK		((MV_PP2_CLS_OVERSIZE_RXQ_MAX) << MV_PP2_CLS_OVERSIZE_RXQ_OFFS)

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 (feature MAS 3.7) new registers
*/
#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_BASE_REG	(MV_PP2_REG_BASE + 0x1980)
#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_REG(port)	(MV_PP2_CLS_OVERSIZE_RXQ_LOW_BASE_REG + ((port) * 4))

#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_OFF		0
#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_BITS	3
#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_MAX		((1 << MV_PP2_CLS_OVERSIZE_RXQ_LOW_BITS) - 1)
#define MV_PP2_CLS_OVERSIZE_RXQ_LOW_MASK	((MV_PP2_CLS_OVERSIZE_RXQ_LOW_MAX) << MV_PP2_CLS_OVERSIZE_RXQ_LOW_OFF)

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 len changed table moved to tx general
*/
#define MV_PP2_V0_CLS_LEN_CHANGE_INDEX_REG	(MV_PP2_REG_BASE + 0x19A0)
#define MV_PP2_V1_CLS_LEN_CHANGE_INDEX_REG	(MV_PP2_REG_BASE + 0x8808)

/*-------------------------------------------------------------------------------*/
/*
  PPv2.1 len changed table moved to tx general
*/
#define MV_PP2_V0_CLS_LEN_CHANGE_TBL_REG	(MV_PP2_REG_BASE + 0x19A4)
#define MV_PP2_V1_CLS_LEN_CHANGE_TBL_REG	(MV_PP2_REG_BASE + 0x880c)


/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.5*/
#define MV_PP2_CLS_SWFWD_P2HQ_BASE_REG		(MV_PP2_REG_BASE + 0x19B0)
#define MV_PP2_CLS_SWFWD_P2HQ_REG(eport)	(MV_PP2_CLS_SWFWD_P2HQ_BASE_REG + ((eport) * 4))

#define MV_PP2_CLS_SWFWD_P2HQ_QUEUE_OFF		0
#define MV_PP2_CLS_SWFWD_P2HQ_QUEUE_BITS	5
#define MV_PP2_CLS_SWFWD_P2HQ_QUEUE_MAX		((1 << MV_PP2_CLS_SWFWD_P2HQ_QUEUE_BITS) - 1)
#define MV_PP2_CLS_SWFWD_P2HQ_QUEUE_MASK	((MV_PP2_CLS_SWFWD_P2HQ_QUEUE_MAX) << MV_PP2_CLS_SWFWD_P2HQ_QUEUE_OFF)
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.5*/
#define MV_PP2_CLS_SWFWD_PCTRL_REG		(MV_PP2_REG_BASE + 0x19D0)
#define MV_PP2_CLS_SWFWD_PCTRL_OFF		0
#define MV_PP2_CLS_SWFWD_PCTRL_MASK(port)	(1 << ((port) + MV_PP2_CLS_SWFWD_PCTRL_OFF))

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.14*/
#define MV_PP2_CLS_SEQ_SIZE_REG			(MV_PP2_REG_BASE + 0x19D4)
#define MV_PP2_CLS_SEQ_SIZE_BITS		4
#define MV_PP2_CLS_SEQ_INDEX_MAX		7
#define MV_PP2_CLS_SEQ_SIZE_MAX			8
#define MV_PP2_CLS_SEQ_SIZE_MASK(index)		\
		(((1 << MV_PP2_CLS_SEQ_SIZE_BITS) - 1) << (MV_PP2_CLS_SEQ_SIZE_BITS * (index)))
#define MV_PP2_CLS_SEQ_SIZE_VAL(index, val)	((val) << ((index) * MV_PP2_CLS_SEQ_SIZE_BITS))
/*-------------------------------------------------------------------------------*/
/*PPv2.1 new register MAS 3.18*/
#define MV_PP2_CLS_PCTRL_BASE_REG		(MV_PP2_REG_BASE + 0x1880)
#define MV_PP2_CLS_PCTRL_REG(port)		(MV_PP2_CLS_PCTRL_BASE_REG + 4 * (port))
#define MV_PP2_CLS_PCTRL_MH_OFFS		0
#define MV_PP2_CLS_PCTRL_MH_BITS		16
#define MV_PP2_CLS_PCTRL_MH_MASK		(((1 << MV_PP2_CLS_PCTRL_MH_BITS) - 1) << MV_PP2_CLS_PCTRL_MH_OFFS)

#define MV_PP2_CLS_PCTRL_VIRT_EN_OFFS		16
#define MV_PP2_CLS_PCTRL_VIRT_EN_MASK		(1 << MV_PP2_CLS_PCTRL_VIRT_EN_OFFS)

#define MV_PP2_CLS_PCTRL_UNI_EN_OFFS		17
#define MV_PP2_CLS_PCTRL_UNI_EN_MASK		(1 << MV_PP2_CLS_PCTRL_UNI_EN_OFFS)

/*-------------------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
#define MV_PP2_V1_CNT_IDX_REG			(MV_PP2_REG_BASE + 0x7040)
#define MV_PP2_V1_CNT_IDX_LKP(lkp, way)		((way) << 6 | (lkp))
#define MV_PP2_V1_CNT_IDX_FLOW(index)		(index)

#define MV_PP2_V1_CLS_LKP_TBL_HIT_REG		(MV_PP2_REG_BASE + 0x7700)
#define MV_PP2_V1_CLS_FLOW_TBL_HIT_REG		(MV_PP2_REG_BASE + 0x7704)

/*-------------------------------------------------------------------------------*/
/*			 lkpid table structure					 */
/*-------------------------------------------------------------------------------*/
#define FLOWID_RXQ				0
#define FLOWID_RXQ_BITS				8
#define FLOWID_RXQ_MASK				(((1 << FLOWID_RXQ_BITS) - 1) << FLOWID_RXQ)

#define FLOWID_MODE				8
#define FLOWID_MODE_BITS			8
#define FLOWID_MODE_MASK			(((1 << FLOWID_MODE_BITS) - 1) << FLOWID_MODE)
#define FLOWID_MODE_MAX				((1 << FLOWID_MODE_BITS) - 1)

#define FLOWID_FLOW				16
#define FLOWID_FLOW_BITS			9
#define FLOWID_FLOW_MASK			(((1 << FLOWID_FLOW_BITS) - 1) << FLOWID_FLOW)

#define FLOWID_EN				25 /*one bit */
#define FLOWID_EN_MASK				(1 << FLOWID_EN)


/*-------------------------------------------------------------------------------*/
/*			 flow table structure					 */
/*-------------------------------------------------------------------------------*/

/*-------------------------------  DWORD 0  ------------------------------------ */

#define FLOW_LAST				0
#define FLOW_LAST_MASK				1 /*one bit*/

#define FLOW_ENGINE				1
#define FLOW_ENGINE_BITS			3
#define FLOW_ENGINE_MASK			(((1 << FLOW_ENGINE_BITS) - 1) << FLOW_ENGINE)
#define FLOW_ENGINE_MAX				5 /* valid value 1 - 5 */

#define FLOW_PORT_ID				4
#define FLOW_PORT_ID_BITS			8
#define FLOW_PORT_ID_MASK			(((1 << FLOW_PORT_ID_BITS) - 1) << FLOW_PORT_ID)
#define FLOW_PORT_ID_MAX			((1 << FLOW_PORT_ID_BITS) - 1)

#define FLOW_PORT_TYPE				12
#define FLOW_PORT_TYPE_BITS			2
#define FLOW_PORT_TYPE_MASK			(((1 << FLOW_PORT_TYPE_BITS) - 1) << FLOW_PORT_TYPE)
#define FLOW_PORT_TYPE_MAX			2 /* valid value 0 - 2 */

/*
  PPv2.1  FLOW_PPPOE new fields in word 0
*/

#define FLOW_PPPOE				14
#define FLOW_PPPOE_BITS				2
#define FLOW_PPPOE_MASK				(((1 << FLOW_PPPOE_BITS) - 1) << FLOW_PPPOE)
#define FLOW_PPPOE_MAX				2 /* valid value 0 - 2 */

/*
  PPv2.1  FLOW_VLAN new fields in word 0
*/
#define FLOW_VLAN				16
#define FLOW_VLAN_BITS				3
#define FLOW_VLAN_MASK				(((1 << FLOW_VLAN_BITS) - 1) << FLOW_VLAN)
#define FLOW_VLAN_MAX				((1 << FLOW_VLAN_BITS) - 1)

/*
  PPv2.1  FLOW_MACME new fields in word 0
*/
#define FLOW_MACME				19
#define FLOW_MACME_BITS				2
#define FLOW_MACME_MASK				(((1 << FLOW_MACME_BITS) - 1) << FLOW_MACME)
#define FLOW_MACME_MAX				2 /* valid value 0 - 2 */

/*
  PPv2.1  FLOW_UDF7 new fields in word 0
*/
#define FLOW_UDF7				21
#define FLOW_UDF7_BITS				2
#define FLOW_UDF7_MASK				(((1 << FLOW_UDF7_BITS) - 1) << FLOW_UDF7)
#define FLOW_UDF7_MAX				((1 << FLOW_UDF7_BITS) - 1)

/*
  PPv2.1  FLOW_PORT_ID_SEL new bit in word 0
*/
#define FLOW_PORT_ID_SEL			23
#define FLOW_PORT_ID_SEL_MASK			(1 << FLOW_PORT_ID_SEL)

/*-------------------------------  DWORD 1  ------------------------------------ */

#define FLOW_FIELDS_NUM				0
#define FLOW_FIELDS_NUM_BITS			3
#define FLOW_FIELDS_NUM_MASK			(((1 << FLOW_FIELDS_NUM_BITS) - 1) << FLOW_FIELDS_NUM)
#define FLOW_FIELDS_NUM_MAX			4 /*valid vaue 0 - 4 */

#define FLOW_LKP_TYPE				3
#define FLOW_LKP_TYPE_BITS			6
#define FLOW_LKP_TYPE_MASK			(((1 << FLOW_LKP_TYPE_BITS) - 1) << FLOW_LKP_TYPE)
#define FLOW_LKP_TYPE_MAX			((1 << FLOW_LKP_TYPE_BITS) - 1)

#define FLOW_FIELED_PRIO			9
#define FLOW_FIELED_PRIO_BITS			6
#define FLOW_FIELED_PRIO_MASK			(((1 << FLOW_FIELED_PRIO_BITS) - 1) << FLOW_FIELED_PRIO)
#define FLOW_FIELED_PRIO_MAX			((1 << FLOW_FIELED_PRIO_BITS) - 1)

/*
  PPv2.1  FLOW_SEQ_CTRL new fields in word 1
*/
#define FLOW_SEQ_CTRL				15
#define FLOW_SEQ_CTRL_BITS			3
#define FLOW_SEQ_CTRL_MASK			(((1 << FLOW_SEQ_CTRL_BITS) - 1) << FLOW_SEQ_CTRL)
#define FLOW_SEQ_CTRL_MAX			4



/*----------------------------------  DWORD 2  ---------------------------------- */
#define FLOW_FIELD0_ID				0
#define FLOW_FIELD1_ID				6
#define FLOW_FIELD2_ID				12
#define FLOW_FIELD3_ID				18

#define FLOW_FIELD_ID_BITS			6
#define FLOW_FIELED_ID(num)			(FLOW_FIELD0_ID + (FLOW_FIELD_ID_BITS * (num)))
#define FLOW_FIELED_MASK(num)			(((1 << FLOW_FIELD_ID_BITS) - 1) << (FLOW_FIELD_ID_BITS * (num)))
#define FLOW_FIELED_MAX				((1 << FLOW_FIELD_ID_BITS) - 1)

/*-------------------------------------------------------------------------------*/
/*		  change length table structure					 */
/*-------------------------------------------------------------------------------*/
#define LEN_CHANGE_LENGTH			0
#define LEN_CHANGE_LENGTH_BITS			7
#define LEN_CHANGE_LENGTH_MAX			((1 << LEN_CHANGE_LENGTH_BITS) - 1)
#define LEN_CHANGE_LENGTH_MASK			(((1 << LEN_CHANGE_LENGTH_BITS) - 1) << LEN_CHANGE_LENGTH)

#define LEN_CHANGE_DEC				7 /*1 dec , 0 inc*/
#define LEN_CHANGE_DEC_MASK			(1 << LEN_CHANGE_DEC)
/*-------------------------------------------------------------------------------*/
/*		Classifier Top Public initialization APIs    			 */
/*-------------------------------------------------------------------------------*/
/* workaround for HW bug - set las bit in flow entry 0*/
void mvPp2ClsHwLastBitWorkAround(void);

int mvPp2ClsInit(void);
int mvPp2ClsHwPortDefConfig(int port, int way, int lkpid, int rxq);
int mvPp2ClsHwEnable(int enable);
int mvPp2ClsHwPortWaySet(int port, int way);
int mvPp2ClsHwPortSpidSet(int port, int spid);
int mvPp2ClsHwUniPortSet(int uni_port, int spid);
int mvPp2ClsHwVirtPortSet(int virt_port, int gem_portid);
int mvPp2ClsHwUdfSet(int udf_no, int offs_id, int offs_bits, int size_bits);
int mvPp2V0ClsHwMtuSet(int port, int txp, int mtu);/*PPv2.1 feature changed MAS 3.7*/
int mvPp2V1ClsHwMtuSet(int index, int mtu);/*PPv2.1 feature changed MAS 3.7*/
int mvPp2ClsHwOversizeRxqSet(int port, int rxq);
int mvPp2ClsHwRxQueueHighSet(int port, int from, int queue);/*PPv2.1 new feature MAS 3.5*/
int mvPp2ClsHwMhSet(int port, int virtEn, int uniEn, unsigned short mh);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsHwSeqInstrSizeSet(int index, int size);/*PPv2.1 new feature MAS 3.14*/
void mvPp2ClsShadowInit(void);

/*-------------------------------------------------------------------------------*/
/*		Classifier Top Public lkpid table APIs     			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_LKP_TBL_SIZE				(64)

typedef struct mvPp2ClsLkpEntry {
	unsigned int lkpid;
	unsigned int way;
	unsigned int data;
} MV_PP2_CLS_LKP_ENTRY;

int mvPp2ClsHwLkpWrite(int lkpid, int way, MV_PP2_CLS_LKP_ENTRY *fe);
int mvPp2ClsHwLkpRead(int lkpid, int way, MV_PP2_CLS_LKP_ENTRY *fe);
int mvPp2ClsSwLkpDump(MV_PP2_CLS_LKP_ENTRY *fe);
int mvPp2ClsHwLkpDump(void);
/*PPv2.1 new counters MAS 3.20*/
int mvPp2V1ClsHwLkpHitsDump(void);
void mvPp2ClsSwLkpClear(MV_PP2_CLS_LKP_ENTRY *fe);
void mvPp2ClsHwLkpClearAll(void);

int mvPp2ClsSwLkpRxqSet(MV_PP2_CLS_LKP_ENTRY *fe, int rxq);
int mvPp2ClsSwLkpEnSet(MV_PP2_CLS_LKP_ENTRY *fe, int en);
int mvPp2ClsSwLkpFlowSet(MV_PP2_CLS_LKP_ENTRY *fe, int flow_idx);
int mvPp2ClsSwLkpModSet(MV_PP2_CLS_LKP_ENTRY *fe, int mod_base);
int mvPp2ClsSwLkpRxqGet(MV_PP2_CLS_LKP_ENTRY *fe, int *rxq);
int mvPp2ClsSwLkpEnGet(MV_PP2_CLS_LKP_ENTRY *fe, int *en);
int mvPp2ClsSwLkpFlowGet(MV_PP2_CLS_LKP_ENTRY *fe, int *flow_idx);
int mvPp2ClsSwLkpModGet(MV_PP2_CLS_LKP_ENTRY *fe, int *mod_base);

/*-------------------------------------------------------------------------------*/
/*		Classifier Top Public flows table APIs   			 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_FLOWS_TBL_SIZE			(512)
#define MV_PP2_CLS_FLOWS_TBL_DATA_WORDS			(3)
#define MV_PP2_CLS_FLOWS_TBL_FIELDS_MAX			(4)

typedef struct mvPp2ClsFlowEntry {
	unsigned int index;
	unsigned int data[MV_PP2_CLS_FLOWS_TBL_DATA_WORDS];
} MV_PP2_CLS_FLOW_ENTRY;

int mvPp2ClsHwFlowWrite(int index, MV_PP2_CLS_FLOW_ENTRY *fe);
int mvPp2ClsHwFlowRead(int index, MV_PP2_CLS_FLOW_ENTRY *fe);
int mvPp2ClsSwFlowDump(MV_PP2_CLS_FLOW_ENTRY *fe);
int mvPp2ClsHwFlowDump(void);
int mvPp2V1ClsHwFlowHitsDump(void);
void mvPp2ClsSwFlowClear(MV_PP2_CLS_FLOW_ENTRY *fe);
void mvPp2ClsHwFlowClearAll(void);

/*
int mvPp2ClsSwFlowHekSet(MV_PP2_CLS_FLOW_ENTRY *fe, int num_of_fields, int field_ids[]);
*/
int mvPp2ClsSwFlowHekSet(MV_PP2_CLS_FLOW_ENTRY *fe, int field_index, int field_id);
int mvPp2ClsSwFlowHekNumSet(MV_PP2_CLS_FLOW_ENTRY *fe, int num_of_fields);
int mvPp2ClsSwFlowPortSet(MV_PP2_CLS_FLOW_ENTRY *fe, int type, int portid);
int mvPp2ClsSwFlowUdf7Set(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowMacMeSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowVlanSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowPppoeSet(MV_PP2_CLS_FLOW_ENTRY *fe,  int mode);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwPortIdSelect(MV_PP2_CLS_FLOW_ENTRY *fe, int from);/*PPv2.1 new feature MAS 3.18*/
int mvPp2ClsSwFlowEngineSet(MV_PP2_CLS_FLOW_ENTRY *fe, int engine, int is_last);
int mvPp2ClsSwFlowSeqCtrlSet(MV_PP2_CLS_FLOW_ENTRY *fe, int mode);/*PPv2.1 new feature MAS 3.14*/
int mvPp2ClsSwFlowExtraSet(MV_PP2_CLS_FLOW_ENTRY *fe, int type, int prio);
int mvPp2ClsSwFlowHekGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *num_of_fields, int field_ids[]);
int mvPp2ClsSwFlowPortGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *type, int *portid);
int mvPp2ClsSwFlowEngineGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *engine, int *is_last);
int mvPp2ClsSwFlowExtraGet(MV_PP2_CLS_FLOW_ENTRY *fe, int *type, int *prio);


/*-------------------------------------------------------------------------------*/
/*		Classifier Top Public length change table APIs  		 */
/*-------------------------------------------------------------------------------*/
#define MV_PP2_CLS_LEN_CHANGE_TBL_SIZE				(256)

int mvPp2ClsPktLenChangeDump(void);
int mvPp2ClsPktLenChangeSet(int index, int length);
int mvPp2ClsPktLenChangeGet(int index, int *length);


/*-------------------------------------------------------------------------------*/
/*			additional cls debug APIs				 */
/*-------------------------------------------------------------------------------*/
int mvPp2ClsHwRegsDump(void);

#endif /* MV_CLS_HW */
