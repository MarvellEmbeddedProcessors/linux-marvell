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

#ifndef __INCMVNFCREGSH
#define __INCMVNFCREGSH

#ifdef __cplusplus
extern "C" {
#endif

#include "mvSysNfcConfig.h"

/* NAND Flash Control Register */
#define	NFC_CONTROL_REG			(MV_NFC_REGS_BASE + 0x0)
#define	NFC_CTRL_WRCMDREQ_MASK		(0x1 << 0)
#define NFC_CTRL_RDDREQ_MASK		(0x1 << 1)
#define NFC_CTRL_WRDREQ_MASK		(0x1 << 2)
#define NFC_CTRL_CORRERR_MASK		(0x1 << 3)
#define NFC_CTRL_UNCERR_MASK		(0x1 << 4)
#define NFC_CTRL_CS1_BBD_MASK		(0x1 << 5)
#define NFC_CTRL_CS0_BBD_MASK		(0x1 << 6)
#define NFC_CTRL_CS1_CMDD_MASK		(0x1 << 7)
#define NFC_CTRL_CS0_CMDD_MASK		(0x1 << 8)
#define NFC_CTRL_CS1_PAGED_MASK		(0x1 << 9)
#define NFC_CTRL_CS0_PAGED_MASK		(0x1 << 10)
#define NFC_CTRL_RDY_MASK		(0x1 << 11)
#define NFC_CTRL_ND_ARB_EN_MASK		(0x1 << 12)
#define NFC_CTRL_PG_PER_BLK_OFFS	13
#define NFC_CTRL_PG_PER_BLK_MASK	(0x3 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_32		(0x0 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_64		(0x2 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_128		(0x1 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_PG_PER_BLK_256		(0x3 << NFC_CTRL_PG_PER_BLK_OFFS)
#define NFC_CTRL_RA_START_MASK		(0x1 << 15)
#define NFC_CTRL_RD_ID_CNT_OFFS		16
#define NFC_CTRL_RD_ID_CNT_MASK		(0x7 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_RD_ID_CNT_SP		(0x2 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_RD_ID_CNT_LP		(0x4 << NFC_CTRL_RD_ID_CNT_OFFS)
#define NFC_CTRL_CLR_PG_CNT_MASK	(0x1 << 20)
#define NFC_CTRL_FORCE_CSX_MASK		(0x1 << 21)
#define NFC_CTRL_ND_STOP_MASK		(0x1 << 22)
#define NFC_CTRL_SEQ_DIS_MASK		(0x1 << 23)
#define NFC_CTRL_PAGE_SZ_OFFS		24
#define NFC_CTRL_PAGE_SZ_MASK		(0x3 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_PAGE_SZ_512B		(0x0 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_PAGE_SZ_2KB		(0x1 << NFC_CTRL_PAGE_SZ_OFFS)
#define NFC_CTRL_DWIDTH_M_MASK		(0x1 << 26)
#define NFC_CTRL_DWIDTH_C_MASK		(0x1 << 27)
#define NFC_CTRL_ND_RUN_MASK		(0x1 << 28)
#define NFC_CTRL_DMA_EN_MASK		(0x1 << 29)
#define NFC_CTRL_ECC_EN_MASK		(0x1 << 30)
#define NFC_CTRL_SPARE_EN_MASK		(0x1 << 31)

/* NAND Interface Timing Parameter 0 Register */
#define NFC_TIMING_0_REG		(MV_NFC_REGS_BASE + 0x4)
#define NFC_TMNG0_TRP_OFFS		0
#define NFC_TMNG0_TRP_MASK		(0x7 << NFC_TMNG0_TRP_OFFS)
#define NFC_TMNG0_TRH_OFFS		3
#define NFC_TMNG0_TRH_MASK		(0x7 << NFC_TMNG0_TRH_OFFS)
#define NFC_TMNG0_ETRP_OFFS		6
#define NFC_TMNG0_SEL_NRE_EDGE_OFFS	7
#define NFC_TMNG0_TWP_OFFS		8
#define NFC_TMNG0_TWP_MASK		(0x7 << NFC_TMNG0_TWP_OFFS)
#define NFC_TMNG0_TWH_OFFS		11
#define NFC_TMNG0_TWH_MASK		(0x7 << NFC_TMNG0_TWH_OFFS)
#define NFC_TMNG0_TCS_OFFS		16
#define NFC_TMNG0_TCS_MASK		(0x7 << NFC_TMNG0_TCS_OFFS)
#define NFC_TMNG0_TCH_OFFS		19
#define NFC_TMNG0_TCH_MASK		(0x7 << NFC_TMNG0_TCH_OFFS)
#define NFC_TMNG0_RD_CNT_DEL_OFFS	22
#define NFC_TMNG0_RD_CNT_DEL_MASK	(0xF << NFC_TMNG0_RD_CNT_DEL_OFFS)
#define NFC_TMNG0_SEL_CNTR_OFFS		26
#define NFC_TMNG0_TADL_OFFS		27
#define NFC_TMNG0_TADL_MASK		(0x1F << NFC_TMNG0_TADL_OFFS)

/* NAND Interface Timing Parameter 1 Register */
#define NFC_TIMING_1_REG		(MV_NFC_REGS_BASE + 0xC)
#define NFC_TMNG1_TAR_OFFS		0
#define NFC_TMNG1_TAR_MASK		(0xF << NFC_TMNG1_TAR_OFFS)
#define NFC_TMNG1_TWHR_OFFS		4
#define NFC_TMNG1_TWHR_MASK		(0xF << NFC_TMNG1_TWHR_OFFS)
#define NFC_TMNG1_TRHW_OFFS		8
#define NFC_TMNG1_TRHW_MASK		(0x3 << NFC_TMNG1_TRHW_OFFS)
#define NFC_TMNG1_PRESCALE_OFFS		14
#define NFC_TMNG1_WAIT_MODE_OFFS	15
#define NFC_TMNG1_TR_OFFS		16
#define NFC_TMNG1_TR_MASK		(0xFFFF << NFC_TMNG1_TR_OFFS)

/* NAND Controller Status Register - NDSR */
#define NFC_STATUS_REG			(MV_NFC_REGS_BASE + 0x14)
#define NFC_SR_WRCMDREQ_MASK		(0x1 << 0)
#define NFC_SR_RDDREQ_MASK		(0x1 << 1)
#define NFC_SR_WRDREQ_MASK		(0x1 << 2)
#define NFC_SR_CORERR_MASK		(0x1 << 3)
#define NFC_SR_UNCERR_MASK		(0x1 << 4)
#define NFC_SR_CS1_BBD_MASK		(0x1 << 5)
#define NFC_SR_CS0_BBD_MASK		(0x1 << 6)
#define NFC_SR_CS1_CMDD_MASK		(0x1 << 7)
#define NFC_SR_CS0_CMDD_MASK		(0x1 << 8)
#define NFC_SR_CS1_PAGED_MASK		(0x1 << 9)
#define NFC_SR_CS0_PAGED_MASK		(0x1 << 10)
#define NFC_SR_RDY0_MASK		(0x1 << 11)
#define NFC_SR_RDY1_MASK		(0x1 << 12)
#define NFC_SR_ALLIRQ_MASK		(0x1FFF << 0)
#define NFC_SR_TRUSTVIO_MASK		(0x1 << 15)
#define NFC_SR_ERR_CNT_OFFS		16
#define NFC_SR_ERR_CNT_MASK		(0x1F << NFC_SR_ERR_CNT_OFFS)

/* NAND Controller Page Count Register */
#define NFC_PAGE_COUNT_REG		(MV_NFC_REGS_BASE + 0x18)
#define NFC_PCR_PG_CNT_0_OFFS		0
#define NFC_PCR_PG_CNT_0_MASK		(0xFF << NFC_PCR_PG_CNT_0_OFFS)
#define NFC_PCR_PG_CNT_1_OFFS		16
#define NFC_PCR_PG_CNT_1_MASK		(0xFF << NFC_PCR_PG_CNT_1_OFFS)

/* NAND Controller Bad Block 0 Register */
#define NFC_BAD_BLOCK_0_REG		(MV_NFC_REGS_BASE + 0x1C)

/* NAND Controller Bad Block 1 Register */
#define NFC_BAD_BLOCK_1_REG		(MV_NFC_REGS_BASE + 0x20)

/* NAND ECC Controle Register */
#define NFC_ECC_CONTROL_REG		(MV_NFC_REGS_BASE + 0x28)
#define NFC_ECC_BCH_EN_MASK		(0x1 << 0)
#define NFC_ECC_THRESHOLD_OFFS		1
#define NFC_ECC_THRESHOLF_MASK		(0x3F << NFC_ECC_THRESHOLD_OFFS)
#define NFC_ECC_SPARE_OFFS		7
#define NFC_ECC_SPARE_MASK		(0xFF << NFC_ECC_SPARE_OFFS)

/* NAND Busy Length Count */
#define NFC_BUSY_LEN_CNT_REG		(MV_NFC_REGS_BASE + 0x2C)
#define NFC_BUSY_CNT_0_OFFS		0
#define NFC_BUSY_CNT_0_MASK		(0xFFFF << NFC_BUSY_CNT_0_OFFS)
#define NFC_BUSY_CNT_1_OFFS		16
#define NFC_BUSY_CNT_1_MASK		(0xFFFF << NFC_BUSY_CNT_1_OFFS)

/* NAND Mutex Lock */
#define NFC_MUTEX_LOCK_REG		(MV_NFC_REGS_BASE + 0x30)
#define NFC_MUTEX_LOCK_MASK		(0x1 << 0)

/* NAND Partition Command Match */
#define NFC_PART_CMD_MACTH_1_REG	(MV_NFC_REGS_BASE + 0x34)
#define NFC_PART_CMD_MACTH_2_REG	(MV_NFC_REGS_BASE + 0x38)
#define NFC_PART_CMD_MACTH_3_REG	(MV_NFC_REGS_BASE + 0x3C)
#define NFC_CMDMAT_CMD1_MATCH_OFFS	0
#define NFC_CMDMAT_CMD1_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD1_MATCH_OFFS)
#define NFC_CMDMAT_CMD1_ROWADD_MASK	(0x1 << 8)
#define NFC_CMDMAT_CMD1_NKDDIS_MASK	(0x1 << 9)
#define NFC_CMDMAT_CMD2_MATCH_OFFS	10
#define NFC_CMDMAT_CMD2_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD2_MATCH_OFFS)
#define NFC_CMDMAT_CMD2_ROWADD_MASK	(0x1 << 18)
#define NFC_CMDMAT_CMD2_NKDDIS_MASK	(0x1 << 19)
#define NFC_CMDMAT_CMD3_MATCH_OFFS	20
#define NFC_CMDMAT_CMD3_MATCH_MASK	(0xFF << NFC_CMDMAT_CMD3_MATCH_OFFS)
#define NFC_CMDMAT_CMD3_ROWADD_MASK	(0x1 << 28)
#define NFC_CMDMAT_CMD3_NKDDIS_MASK	(0x1 << 29)
#define NFC_CMDMAT_VALID_CNT_OFFS	30
#define NFC_CMDMAT_VALID_CNT_MASK	(0x3 << NFC_CMDMAT_VALID_CNT_OFFS)

/* NAND Controller Data Buffer */
#define NFC_DATA_BUFF_REG_4PDMA		(MV_NFC_REGS_OFFSET + 0x40)
#define NFC_DATA_BUFF_REG		(MV_NFC_REGS_BASE + 0x40)

/* NAND Controller Command Buffer 0 */
#define NFC_COMMAND_BUFF_0_REG_4PDMA	(MV_NFC_REGS_OFFSET + 0x48)
#define NFC_COMMAND_BUFF_0_REG		(MV_NFC_REGS_BASE + 0x48)
#define NFC_CB0_CMD1_OFFS		0
#define NFC_CB0_CMD1_MASK		(0xFF << NFC_CB0_CMD1_OFFS)
#define NFC_CB0_CMD2_OFFS		8
#define NFC_CB0_CMD2_MASK		(0xFF << NFC_CB0_CMD2_OFFS)
#define NFC_CB0_ADDR_CYC_OFFS		16
#define NFC_CB0_ADDR_CYC_MASK		(0x7 << NFC_CB0_ADDR_CYC_OFFS)
#define NFC_CB0_DBC_MASK			(0x1 << 19)
#define NFC_CB0_NEXT_CMD_MASK		(0x1 << 20)
#define NFC_CB0_CMD_TYPE_OFFS		21
#define NFC_CB0_CMD_TYPE_MASK		(0x7 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_READ		(0x0 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_WRITE		(0x1 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_ERASE		(0x2 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_READ_ID	(0x3 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_STATUS		(0x4 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_RESET		(0x5 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_NAKED_CMD	(0x6 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CMD_TYPE_NAKED_ADDR	(0x7 << NFC_CB0_CMD_TYPE_OFFS)
#define NFC_CB0_CSEL_MASK		(0x1 << 24)
#define NFC_CB0_AUTO_RS_MASK		(0x1 << 25)
#define NFC_CB0_ST_ROW_EN_MASK		(0x1 << 26)
#define NFC_CB0_RDY_BYP_MASK		(0x1 << 27)
#define NFC_CB0_LEN_OVRD_MASK		(0x1 << 28)
#define NFC_CB0_CMD_XTYPE_OFFS		29
#define NFC_CB0_CMD_XTYPE_MASK		(0x7 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_MONOLITHIC	(0x0 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_LAST_NAKED	(0x1 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_MULTIPLE	(0x4 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_NAKED		(0x5 << NFC_CB0_CMD_XTYPE_OFFS)
#define NFC_CB0_CMD_XTYPE_DISPATCH	(0x6 << NFC_CB0_CMD_XTYPE_OFFS)

/* NAND Controller Command Buffer 1 */
#define NFC_COMMAND_BUFF_1_REG		(MV_NFC_REGS_BASE + 0x4C)
#define NFC_CB1_ADDR1_OFFS		0
#define NFC_CB1_ADDR1_MASK		(0xFF << NFC_CB1_ADDR1_OFFS)
#define NFC_CB1_ADDR2_OFFS		8
#define NFC_CB1_ADDR2_MASK		(0xFF << NFC_CB1_ADDR2_OFFS)
#define NFC_CB1_ADDR3_OFFS		16
#define NFC_CB1_ADDR3_MASK		(0xFF << NFC_CB1_ADDR3_OFFS)
#define NFC_CB1_ADDR4_OFFS		24
#define NFC_CB1_ADDR4_MASK		(0xFF << NFC_CB1_ADDR4_OFFS)

/* NAND Controller Command Buffer 2 */
#define NFC_COMMAND_BUFF_2_REG		(MV_NFC_REGS_BASE + 0x50)
#define NFC_CB2_ADDR5_OFFS		0
#define NFC_CB2_ADDR5_MASK		(0xFF << NFC_CB2_ADDR5_OFFS)
#define NFC_CB2_CS_2_3_SELECT_MASK	(0x80 << NFC_CB2_ADDR5_OFFS)
#define NFC_CB2_PAGE_CNT_OFFS		8
#define NFC_CB2_PAGE_CNT_MASK		(0xFF << NFC_CB2_PAGE_CNT_OFFS)
#define NFC_CB2_ST_CMD_OFFS		16
#define NFC_CB2_ST_CMD_MASK		(0xFF << NFC_CB2_ST_CMD_OFFS)
#define NFC_CB2_ST_MASK_OFFS		24
#define NFC_CB2_ST_MASK_MASK		(0xFF << NFC_CB2_ST_MASK_OFFS)

/* NAND Controller Command Buffer 3 */
#define NFC_COMMAND_BUFF_3_REG		(MV_NFC_REGS_BASE + 0x54)
#define NFC_CB3_NDLENCNT_OFFS		0
#define NFC_CB3_NDLENCNT_MASK		(0xFFFF << NFC_CB3_NDLENCNT_OFFS)
#define NFC_CB3_ADDR6_OFFS		16
#define NFC_CB3_ADDR6_MASK		(0xFF << NFC_CB3_ADDR6_OFFS)
#define NFC_CB3_ADDR7_OFFS		24
#define NFC_CB3_ADDR7_MASK		(0xFF << NFC_CB3_ADDR7_OFFS)

/* NAND Arbitration Control */
#define NFC_ARB_CONTROL_REG		(MV_NFC_REGS_BASE + 0x5C)
#define NFC_ARB_CNT_OFFS		0
#define NFC_ARB_CNT_MASK		(0xFFFF << NFC_ARB_CNT_OFFS)

/* NAND Partition Table for Chip Select */
#define NFC_PART_TBL_4CS_REG(x)		(MV_NFC_REGS_BASE + (x * 0x4))
#define NFC_PT4CS_BLOCKADD_OFFS		0
#define NFC_PT4CS_BLOCKADD_MASK		(0xFFFFFF << NFC_PT4CS_BLOCKADD_OFFS)
#define NFC_PT4CS_TRUSTED_MASK		(0x1 << 29)
#define NFC_PT4CS_LOCK_MASK		(0x1 << 30)
#define NFC_PT4CS_VALID_MASK		(0x1 << 31)

/* NAND XBAR2AXI Bridge Configuration Register */
#define NFC_XBAR2AXI_BRDG_CFG_REG	(MV_NFC_REGS_BASE + 0x1022C)
#define NFC_XBC_CS_EXPAND_EN_MASK	(0x1 << 2)

#ifdef __cplusplus
}
#endif


#endif /* __INCMVNFCREGSH */
