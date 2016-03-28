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
*******************************************************************************/

#ifndef __mv_fw_regs_h__
#define __mv_fw_regs_h__

/*
	0 - maintenance PPC
	1 - datapath PPC0
	2 - datapath PPC1
*/

#define MV_PPC_BASE(ppc)		(0x800000 + ((ppc)*0x80000))

/* PPC memories */
#define MV_PPC_IMEM_OFFS		0x000000
#define MV_PPC_IMEM_MASK		0x1FFFFF

#define MV_PPC_SHARED_MEM_OFFS		0x50000

#define MV_PPC_PROF_MEM_OFFS		0x78000
#define MV_PPC_PROF_MEM_MASK		0x0FFFF

/* PPC registers */
#define MV_PPC_IMEM_HOLD_OFF_REG	0x7F034
#define MV_PPC_WAIT_FOR_DEQ_REG		0x7F038
#define MV_PPC_BUSY_REG			0x7F03C

#define MV_NSS_SE_OFFS			0xD7000
#define MV_NSS_SE_MASK			0x00FFF

/* Search engine registers */
#define MV_EC_ENG2APB_DATA_REGS_NUM         4
#define MV_EC_APB2ENG_REQ_OPCODE_REG        0xd7060
#define MV_EC_APB2ENG_REQ_ADDR_CNTRL_REG    0xd7064
#define MV_EC_ENG2APB_RESPONSE_STATUS_REG   0xd7068
#define MV_EC_APB2ENG_REQUEST_DATA3_REG     0xd7070
#define MV_EC_APB2ENG_REQUEST_DATA2_REG     0xd7074
#define MV_EC_APB2ENG_REQUEST_DATA1_REG     0xd7078
#define MV_EC_APB2ENG_REQUEST_DATA0_REG     0xd707C
#define MV_EC_ENG2APB_RESPONSE_DATA3_REG    0xd7080
#define MV_EC_ENG2APB_RESPONSE_DATA2_REG    0xd7084
#define MV_EC_ENG2APB_RESPONSE_DATA1_REG    0xd7088
#define MV_EC_ENG2APB_RESPONSE_DATA0_REG    0xd708C

#endif /* __mv_fw_regs_h__ */
