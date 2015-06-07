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

/*******************************************************************************
* mvSysTdmConfig.h - Marvell TDM unit specific configurations
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/
#ifndef __INCmvSysTdmConfigh
#define __INCmvSysTdmConfigh

#include "mvOs.h"
#include "voiceband/mvSysTdmSpi.h"
#include "voiceband/common/mvTdmComm.h"

/****************************************************************/
/*************** Telephony configuration ************************/
/****************************************************************/
extern int tdm_base, use_pclk_external;
#define MV_TDM_REGS_BASE	(tdm_base)

/* Core DivClk Control Register */

/* DCO clock apply/reset bits */
#define DCO_CLK_DIV_MOD_OFFS			24
#define DCO_CLK_DIV_APPLY			(0x1 << DCO_CLK_DIV_MOD_OFFS)
#define DCO_CLK_DIV_RESET_OFFS			25
#define DCO_CLK_DIV_RESET			(0x1 << DCO_CLK_DIV_RESET_OFFS)

/* DCO clock ratio is 24Mhz/x */
#define DCO_CLK_DIV_RATIO_OFFS			26
#define DCO_CLK_DIV_RATIO_MASK			0xfc000000
#define DCO_CLK_DIV_RATIO_8M			(0x3 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_4M			(0x6 << DCO_CLK_DIV_RATIO_OFFS)
#define DCO_CLK_DIV_RATIO_2M			(0xc << DCO_CLK_DIV_RATIO_OFFS)

/* TDM PLL configuration registers */
#define TDM_PLL_CONF_REG0			0x0
#define TDM_PLL_FB_CLK_DIV_OFFSET		10
#define TDM_PLL_FB_CLK_DIV_MASK			0x7fc00

#define TDM_PLL_CONF_REG1			0x4
#define TDM_PLL_FREQ_OFFSET_MASK		0xffff
#define TDM_PLL_FREQ_OFFSET_VALID		BIT16
#define TDM_PLL_SW_RESET			BIT31

#define TDM_PLL_CONF_REG2			0x8
#define TDM_PLL_POSTDIV_MASK			0x7f

/* Debug-trace stub */
#define TRC_INIT(...)
#define TRC_RELEASE(...)
#define TRC_START(...)
#define TRC_STOP(...)
#define TRC_REC(...)
#define TRC_OUTPUT(...)

#ifdef CONFIG_MV_TDM_EXT_STATS
	#define MV_TDM_EXT_STATS
#endif

/* TDM control/SPI registers used for suspend/resume */
#define TDM_CTRL_REGS_NUM			36
#define TDM_SPI_REGS_OFFSET			0x3100
#define TDM_SPI_REGS_NUM			16

struct mv_phone_dev {
	void __iomem *tdm_base;
	void __iomem *pll_base;
	void __iomem *dco_div_reg;
	MV_TDM_PARAMS *tdm_params;
	struct platform_device *parent;
	struct device_node *np;
	struct clk *clk;
	u32 pclk_freq_mhz;
	int irq;

	/* Used to preserve TDM registers across suspend/resume */
	u32 tdm_ctrl_regs[TDM_CTRL_REGS_NUM];
	u32 tdm_spi_regs[TDM_SPI_REGS_NUM];
	u32 tdm_spi_mux_reg;
	u32 tdm_mbus_config_reg;
	u32 tdm_misc_reg;
};

/* This enumerator defines the Marvell Units ID */
typedef enum {
	SLIC_EXTERNAL_ID,
	SLIC_ZARLINK_ID,
	SLIC_SILABS_ID,
	SLIC_LANTIQ_ID
} MV_SLIC_UNIT_TYPE;

typedef enum {
	MV_TDM_UNIT_NONE,
	MV_TDM_UNIT_TDM2C,
	MV_TDM_UNIT_TDMMC
} MV_TDM_UNIT_TYPE;

enum {
	SPI_TYPE_FLASH = 0,
	SPI_TYPE_SLIC_ZARLINK_SILABS,
	SPI_TYPE_SLIC_LANTIQ,
	SPI_TYPE_SLIC_ZSI,
	SPI_TYPE_SLIC_ISI
};

typedef enum _devBoardSlicType {
	MV_BOARD_SLIC_DISABLED,
	MV_BOARD_SLIC_SSI_ID, /* Lantiq Integrated SLIC */
	MV_BOARD_SLIC_ISI_ID, /* Silicon Labs ISI Bus */
	MV_BOARD_SLIC_ZSI_ID, /* Zarlink ZSI Bus */
	MV_BOARD_SLIC_EXTERNAL_ID /* Cross vendor external SLIC */
} MV_BOARD_SLIC_TYPE;

/* Declarations */
u32 mvBoardSlicUnitTypeGet(void);
u32 mvCtrlTdmUnitIrqGet(void);
MV_TDM_UNIT_TYPE mvCtrlTdmUnitTypeGet(void);
int mvSysTdmInit(MV_TDM_PARAMS *tdmParams);

#endif /* __INCmvSysTdmConfigh */
