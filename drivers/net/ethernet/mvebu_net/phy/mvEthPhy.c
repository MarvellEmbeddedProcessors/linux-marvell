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

#include "mvCommon.h"
#include "mvOs.h"
#include "mvEthPhyRegs.h"
#include "mvEthPhy.h"

#include "mvNetConfig.h"

#undef DEBUG
#ifdef DEBUG
#define DB(x) x
#define DB2(x) x
#else
#define DB(x)
#define DB2(x)
#endif /* DEBUG */

static MV_U32 mvEthPhySmiReg;

MV_STATUS mvEthPhySmiAddrSet(MV_U32 smi_addr)
{
	mvEthPhySmiReg = smi_addr;
	return MV_OK;
}

/*******************************************************************************
* mvEthPhyRegRead - Read from ethernet phy register.
*
* DESCRIPTION:
*       This function reads ethernet phy register.
*
* INPUT:
*       phyAddr - Phy address.
*       regOffs - Phy register offset.
*
* OUTPUT:
*       None.
*
* RETURN:
*       16bit phy register value, or 0xffff on error
*
*******************************************************************************/
MV_STATUS mvEthPhyRegRead(MV_U32 phyAddr, MV_U32 regOffs, MV_U16 *data)
{
	MV_U32 			smiReg;
	volatile MV_U32 timeout;

	/* check parameters */
	if ((phyAddr << ETH_PHY_SMI_DEV_ADDR_OFFS) & ~ETH_PHY_SMI_DEV_ADDR_MASK) {
		mvOsPrintf("mvEthPhyRegRead: Err. Illegal PHY device address %d\n",
				phyAddr);
		return MV_FAIL;
	}
	if ((regOffs <<  ETH_PHY_SMI_REG_ADDR_OFFS) & ~ETH_PHY_SMI_REG_ADDR_MASK) {
		mvOsPrintf("mvEthPhyRegRead: Err. Illegal PHY register offset %d\n",
				regOffs);
		return MV_FAIL;
	}

	timeout = ETH_PHY_TIMEOUT;
	/* wait till the SMI is not busy*/
	do {
		/* read smi register */
		smiReg = MV_REG_READ(mvEthPhySmiReg);
		if (timeout-- == 0) {
			mvOsPrintf("mvEthPhyRegRead: SMI busy timeout\n");
			return MV_FAIL;
		}
	} while (smiReg & ETH_PHY_SMI_BUSY_MASK);

	/* fill the phy address and regiser offset and read opcode */
	smiReg = (phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) | (regOffs << ETH_PHY_SMI_REG_ADDR_OFFS)|
			   ETH_PHY_SMI_OPCODE_READ;

	/* write the smi register */
	MV_REG_WRITE(mvEthPhySmiReg, smiReg);

	timeout = ETH_PHY_TIMEOUT;

	/*wait till readed value is ready */
	do {
		/* read smi register */
		smiReg = MV_REG_READ(mvEthPhySmiReg);

		if (timeout-- == 0) {
			mvOsPrintf("mvEthPhyRegRead: SMI read-valid timeout\n");
			return MV_FAIL;
		}
	} while (!(smiReg & ETH_PHY_SMI_READ_VALID_MASK));

	/* Wait for the data to update in the SMI register */
	for (timeout = 0; timeout < ETH_PHY_TIMEOUT; timeout++)
		;

	*data = (MV_U16)(MV_REG_READ(mvEthPhySmiReg) & ETH_PHY_SMI_DATA_MASK);

	return MV_OK;
}

MV_STATUS mvEthPhyRegPrint(MV_U32 phyAddr, MV_U32 regOffs)
{
	MV_U16      data;
	MV_STATUS   status;

	status = mvEthPhyRegRead(phyAddr, regOffs, &data);
	if (status != MV_OK)
		mvOsPrintf("Read failed - status = %d\n", status);

	mvOsPrintf("phy=0x%x, reg=0x%x: 0x%04x\n", phyAddr, regOffs, data);

	return status;
}

void mvEthPhyRegs(int phyAddr)
{
	mvOsPrintf("[ETH-PHY #%d registers]\n\n", phyAddr);

	mvEthPhyRegPrint(phyAddr, ETH_PHY_CTRL_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_STATUS_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_AUTONEGO_AD_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_LINK_PARTNER_CAP_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_1000BASE_T_CTRL_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_1000BASE_T_STATUS_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_EXTENDED_STATUS_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_SPEC_CTRL_REG);
	mvEthPhyRegPrint(phyAddr, ETH_PHY_SPEC_STATUS_REG);
}

/*******************************************************************************
* mvEthPhyRegWrite - Write to ethernet phy register.
*
* DESCRIPTION:
*       This function write to ethernet phy register.
*
* INPUT:
*       phyAddr - Phy address.
*       regOffs - Phy register offset.
*       data    - 16bit data.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK if write succeed, MV_BAD_PARAM on bad parameters , MV_ERROR on error .
*		MV_TIMEOUT on timeout
*
*******************************************************************************/
MV_STATUS mvEthPhyRegWrite(MV_U32 phyAddr, MV_U32 regOffs, MV_U16 data)
{
	MV_U32		smiReg;
	volatile MV_U32 timeout;

	/* check parameters */
	if ((phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) & ~ETH_PHY_SMI_DEV_ADDR_MASK) {
		mvOsPrintf("mvEthPhyRegWrite: Err. Illegal phy address 0x%x\n", phyAddr);
		return MV_BAD_PARAM;
	}
	if ((regOffs <<  ETH_PHY_SMI_REG_ADDR_OFFS) & ~ETH_PHY_SMI_REG_ADDR_MASK) {
		mvOsPrintf("mvEthPhyRegWrite: Err. Illegal register offset 0x%x\n", regOffs);
		return MV_BAD_PARAM;
	}

	timeout = ETH_PHY_TIMEOUT;

	/* wait till the SMI is not busy*/
	do {
		/* read smi register */
		smiReg = MV_REG_READ(mvEthPhySmiReg);
		if (timeout-- == 0) {
			mvOsPrintf("mvEthPhyRegWrite: SMI busy timeout\n");
		return MV_TIMEOUT;
		}
	} while (smiReg & ETH_PHY_SMI_BUSY_MASK);

	/* fill the phy address and regiser offset and write opcode and data*/
	smiReg = (data << ETH_PHY_SMI_DATA_OFFS);
	smiReg |= (phyAddr <<  ETH_PHY_SMI_DEV_ADDR_OFFS) | (regOffs << ETH_PHY_SMI_REG_ADDR_OFFS);
	smiReg &= ~ETH_PHY_SMI_OPCODE_READ;

	/* write the smi register */
	DB(printf("%s: phyAddr=0x%x offset = 0x%x data=0x%x\n", __func__, phyAddr, regOffs, data));
	DB(printf("%s: mvEthPhySmiReg = 0x%x smiReg=0x%x\n", __func__, mvEthPhySmiReg, smiReg));
	MV_REG_WRITE(mvEthPhySmiReg, smiReg);

	return MV_OK;
}


/*******************************************************************************
* mvEthPhyReset - Reset ethernet Phy.
*
* DESCRIPTION:
*       This function resets a given ethernet Phy.
*
* INPUT:
*       phyAddr - Phy address.
*       timeout - in millisec
*
* OUTPUT:
*       None.
*
* RETURN:   MV_OK       - Success
*           MV_TIMEOUT  - Timeout
*
*******************************************************************************/
MV_STATUS mvEthPhyReset(MV_U32 phyAddr, int timeout)
{
	MV_U16  phyRegData;

	/* Reset the PHY */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &phyRegData) != MV_OK)
		return MV_FAIL;

	/* Set bit 15 to reset the PHY */
	phyRegData |= ETH_PHY_CTRL_RESET_MASK;
	mvEthPhyRegWrite(phyAddr, ETH_PHY_CTRL_REG, phyRegData);

	/* Wait untill Reset completed */
	while (timeout > 0) {
		mvOsSleep(100);
		timeout -= 100;

		if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &phyRegData) != MV_OK)
			return MV_FAIL;

		if ((phyRegData & ETH_PHY_CTRL_RESET_MASK) == 0)
			return MV_OK;
	}
	return MV_TIMEOUT;
}


/*******************************************************************************
* mvEthPhyRestartAN - Restart ethernet Phy Auto-Negotiation.
*
* DESCRIPTION:
*       This function resets a given ethernet Phy.
*
* INPUT:
*       phyAddr - Phy address.
*       timeout - in millisec; 0 - no timeout (don't wait)
*
* OUTPUT:
*       None.
*
* RETURN:   MV_OK       - Success
*           MV_TIMEOUT  - Timeout
*
*******************************************************************************/
MV_STATUS mvEthPhyRestartAN(MV_U32 phyAddr, int timeout)
{
	MV_U16  phyRegData;

	/* Reset the PHY */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &phyRegData) != MV_OK)
		return MV_FAIL;

	/* Set bit 12 to Enable autonegotiation of the PHY */
	phyRegData |= ETH_PHY_CTRL_AN_ENABLE_MASK;

	/* Set bit 9 to Restart autonegotiation of the PHY */
	phyRegData |= ETH_PHY_CTRL_AN_RESTART_MASK;
	mvEthPhyRegWrite(phyAddr, ETH_PHY_CTRL_REG, phyRegData);

	if (timeout == 0)
		return MV_OK;

	/* Wait untill Auotonegotiation completed */
	while (timeout > 0) {
		mvOsSleep(100);
		timeout -= 100;

		if (mvEthPhyRegRead(phyAddr, ETH_PHY_STATUS_REG, &phyRegData) != MV_OK)
			return MV_FAIL;

		if (phyRegData & ETH_PHY_STATUS_AN_DONE_MASK)
			return MV_OK;
	}
	return MV_TIMEOUT;
}


/*******************************************************************************
* mvEthPhyDisableAN - Disable Phy Auto-Negotiation and set forced Speed and Duplex
*
* DESCRIPTION:
*       This function disable AN and set duplex and speed.
*
* INPUT:
*       phyAddr - Phy address.
*       speed   - port speed. 0 - 10 Mbps, 1-100 Mbps, 2 - 1000 Mbps
*       duplex  - port duplex. 0 - Half duplex, 1 - Full duplex
*
* OUTPUT:
*       None.
*
* RETURN:   MV_OK   - Success
*           MV_FAIL - Failure
*
*******************************************************************************/
MV_STATUS mvEthPhyDisableAN(MV_U32 phyAddr, int speed, int duplex)
{
	MV_U16  phyRegData;

	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &phyRegData) != MV_OK)
		return MV_FAIL;

	switch (speed) {
	case 0: /* 10 Mbps */
			phyRegData &= ~ETH_PHY_CTRL_SPEED_LSB_MASK;
			phyRegData &= ~ETH_PHY_CTRL_SPEED_MSB_MASK;
			break;

	case 1: /* 100 Mbps */
			phyRegData |= ETH_PHY_CTRL_SPEED_LSB_MASK;
			phyRegData &= ~ETH_PHY_CTRL_SPEED_MSB_MASK;
			break;

	case 2: /* 1000 Mbps */
			phyRegData &= ~ETH_PHY_CTRL_SPEED_LSB_MASK;
			phyRegData |= ETH_PHY_CTRL_SPEED_MSB_MASK;
			break;

	default:
			mvOsOutput("Unexpected speed = %d\n", speed);
			return MV_FAIL;
	}

	switch (duplex) {
	case 0: /* half duplex */
			phyRegData &= ~ETH_PHY_CTRL_DUPLEX_MASK;
			break;

	case 1: /* full duplex */
			phyRegData |= ETH_PHY_CTRL_DUPLEX_MASK;
			break;

	default:
			mvOsOutput("Unexpected duplex = %d\n", duplex);
			return MV_FAIL;
	}
	/* Clear bit 12 to Disable autonegotiation of the PHY */
	phyRegData &= ~ETH_PHY_CTRL_AN_ENABLE_MASK;

	/* Clear bit 9 to DISABLE, Restart autonegotiation of the PHY */
	phyRegData &= ~ETH_PHY_CTRL_AN_RESTART_MASK;
	mvEthPhyRegWrite(phyAddr, ETH_PHY_CTRL_REG, phyRegData);

	return MV_OK;
}

MV_STATUS   mvEthPhyLoopback(MV_U32 phyAddr, MV_BOOL isEnable)
{
	MV_U16      regVal, ctrlVal;
	MV_STATUS   status;

	/* Set loopback speed and duplex accordingly with current */
	/* Bits: 6, 8, 13 */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &ctrlVal) != MV_OK)
		return MV_FAIL;

	if (isEnable) {
		/* Select page 2 */
		mvEthPhyRegWrite(phyAddr, 22, 2);

		mvEthPhyRegRead(phyAddr, 21, &regVal);
		regVal &= ~(ETH_PHY_CTRL_DUPLEX_MASK | ETH_PHY_CTRL_SPEED_LSB_MASK |
				ETH_PHY_CTRL_SPEED_MSB_MASK | ETH_PHY_CTRL_AN_ENABLE_MASK);
		regVal |= (ctrlVal & (ETH_PHY_CTRL_DUPLEX_MASK | ETH_PHY_CTRL_SPEED_LSB_MASK |
					ETH_PHY_CTRL_SPEED_MSB_MASK | ETH_PHY_CTRL_AN_ENABLE_MASK));
		mvEthPhyRegWrite(phyAddr, 21, regVal);

		/* Select page 0 */
		mvEthPhyRegWrite(phyAddr, 22, 0);

		/* Disable Energy detection   R16[9:8] = 00 */
		/* Disable MDI/MDIX crossover R16[6:5] = 00 */
		mvEthPhyRegRead(phyAddr, ETH_PHY_SPEC_CTRL_REG, &regVal);
		regVal &= ~(BIT5 | BIT6 | BIT8 | BIT9);
		mvEthPhyRegWrite(phyAddr, ETH_PHY_SPEC_CTRL_REG, regVal);

		status = mvEthPhyReset(phyAddr, 1000);
		if (status != MV_OK) {
			mvOsPrintf("mvEthPhyReset failed: status=0x%x\n", status);
			return status;
		}

		/* Set loopback */
		ctrlVal |= ETH_PHY_CTRL_LOOPBACK_MASK;
		mvEthPhyRegWrite(phyAddr, ETH_PHY_CTRL_REG, ctrlVal);
	} else {
		/* Cancel Loopback */
		ctrlVal &= ~ETH_PHY_CTRL_LOOPBACK_MASK;
		mvEthPhyRegWrite(phyAddr, ETH_PHY_CTRL_REG, ctrlVal);

		status = mvEthPhyReset(phyAddr, 1000);
		if (status != MV_OK) {
			mvOsPrintf("mvEthPhyReset failed: status=0x%x\n", status);
			return status;
		}

		/* Enable Energy detection   R16[9:8] = 11 */
		/* Enable MDI/MDIX crossover R16[6:5] = 11 */
		mvEthPhyRegRead(phyAddr, ETH_PHY_SPEC_CTRL_REG, &regVal);
		regVal |= (BIT5 | BIT6 | BIT8 | BIT9);
		mvEthPhyRegWrite(phyAddr, ETH_PHY_SPEC_CTRL_REG, regVal);

		status = mvEthPhyReset(phyAddr, 1000);
		if (status != MV_OK) {
			mvOsPrintf("mvEthPhyReset failed: status=0x%x\n", status);
			return status;
		}
	}

	return MV_OK;
}

/*******************************************************************************
* mvEthPhyCheckLink -
*
* DESCRIPTION:
*	check link in phy port
*
* INPUT:
*       phyAddr - Phy address.
*
* OUTPUT:
*       None.
*
* RETURN:   MV_TRUE if link is up, MV_FALSE if down
*
*******************************************************************************/
MV_BOOL mvEthPhyCheckLink(MV_U32 phyAddr)
{
	MV_U16 val_st, val_ctrl, val_spec_st;

	/* read status reg */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_STATUS_REG, &val_st) != MV_OK)
		return MV_FALSE;

	/* read control reg */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &val_ctrl) != MV_OK)
		return MV_FALSE;

	/* read special status reg */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_SPEC_STATUS_REG, &val_spec_st) != MV_OK)
		return MV_FALSE;

	/* Check for PHY exist */
	if ((val_ctrl == ETH_PHY_SMI_DATA_MASK) && (val_st & ETH_PHY_SMI_DATA_MASK))
		return MV_FALSE;


	if (val_ctrl & ETH_PHY_CTRL_AN_ENABLE_MASK) {
		if (val_st & ETH_PHY_STATUS_AN_DONE_MASK)
			return MV_TRUE;
		else
			return MV_FALSE;
	} else {
		if (val_spec_st & ETH_PHY_SPEC_STATUS_LINK_MASK)
			return MV_TRUE;
	}
	return MV_FALSE;
}

/*******************************************************************************
* mvEthPhyPrintStatus -
*
* DESCRIPTION:
*	print port Speed, Duplex, Auto-negotiation, Link.
*
* INPUT:
*       phyAddr - Phy address.
*
* OUTPUT:
*       None.
*
* RETURN:   16bit phy register value, or 0xffff on error
*
*******************************************************************************/
MV_STATUS	mvEthPhyPrintStatus(MV_U32 phyAddr)
{
	MV_U16 val;

	/* read control reg */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_CTRL_REG, &val) != MV_OK)
		return MV_ERROR;

	if (val & ETH_PHY_CTRL_AN_ENABLE_MASK)
		mvOsOutput("Auto negotiation: Enabled\n");
	else
		mvOsOutput("Auto negotiation: Disabled\n");


	/* read specific status reg */
	if (mvEthPhyRegRead(phyAddr, ETH_PHY_SPEC_STATUS_REG, &val) != MV_OK)
		return MV_ERROR;

	switch (val & ETH_PHY_SPEC_STATUS_SPEED_MASK) {
	case ETH_PHY_SPEC_STATUS_SPEED_1000MBPS:
			mvOsOutput("Speed: 1000 Mbps\n");
			break;
	case ETH_PHY_SPEC_STATUS_SPEED_100MBPS:
			mvOsOutput("Speed: 100 Mbps\n");
			break;
	case ETH_PHY_SPEC_STATUS_SPEED_10MBPS:
			mvOsOutput("Speed: 10 Mbps\n");
			break;
	default:
			mvOsOutput("Speed: Uknown\n");
			break;

	}

	if (val & ETH_PHY_SPEC_STATUS_DUPLEX_MASK)
		mvOsOutput("Duplex: Full\n");
	else
		mvOsOutput("Duplex: Half\n");


	if (val & ETH_PHY_SPEC_STATUS_LINK_MASK)
		mvOsOutput("Link: up\n");
	else
		mvOsOutput("Link: down\n");

	return MV_OK;
}

/*******************************************************************************
* mvEthPhyAdvertiseSet -
*
* DESCRIPTION:
*	Set advertisement mode
*
* INPUT:
*       phyAddr - Phy address.
*	advertise -	0x1: 10 half
*			0x2: 10 full
*			0x4: 100 half
*			0x8: 100 full
*			0x10: 1000 half
*			0x20: 1000 full
*
* OUTPUT:
*       None.
*
* RETURN:
*
*******************************************************************************/
MV_STATUS mvEthPhyAdvertiseSet(MV_U32 phyAddr, MV_U16 advertise)
{
	MV_U16 regVal, tmp;
	/* 10 - 100 */
	mvEthPhyRegRead(phyAddr, ETH_PHY_AUTONEGO_AD_REG, &regVal);

	regVal = regVal & (~ETH_PHY_10_100_BASE_ADVERTISE_MASK);
	tmp = (advertise & 0xf);
	regVal = regVal | (tmp << ETH_PHY_10_100_BASE_ADVERTISE_OFFSET);
	mvEthPhyRegWrite(phyAddr, ETH_PHY_AUTONEGO_AD_REG, regVal);
	/* 1000 */
	mvEthPhyRegRead(phyAddr, ETH_PHY_1000BASE_T_CTRL_REG, &regVal);
	/* keep only bits 4-5 + shift to the right */
	tmp = ((advertise >> 4) & 0x3);
	regVal = regVal & (~ETH_PHY_1000BASE_ADVERTISE_MASK);
	regVal = regVal | (tmp << ETH_PHY_1000BASE_ADVERTISE_OFFSET);
	mvEthPhyRegWrite(phyAddr, ETH_PHY_1000BASE_T_CTRL_REG, regVal);
	return MV_OK;
}

/*******************************************************************************
* mvEthPhyAdvertiseGet -
*
* DESCRIPTION:
*	Get advertisement mode
*
* INPUT:
*       phyAddr - Phy address.
*
* OUTPUT:
*	advertise -	0x1: 10 half
*			0x2: 10 full
*			0x4: 100 half
*			0x8: 100 full
*			0x10: 1000 half
*			0x20: 1000 full
*
* RETURN:
*
*******************************************************************************/
MV_STATUS mvEthPhyAdvertiseGet(MV_U32 phyAddr, MV_U16 *advertise)
{
	MV_U16 regVal, tmp;

	if (advertise == NULL)
		return MV_BAD_PARAM;

	/* 10 - 100 */
	mvEthPhyRegRead(phyAddr, ETH_PHY_AUTONEGO_AD_REG, &regVal);
	tmp = ((regVal & ETH_PHY_10_100_BASE_ADVERTISE_MASK) >> ETH_PHY_10_100_BASE_ADVERTISE_OFFSET);
	/* 1000 */
	mvEthPhyRegRead(phyAddr, ETH_PHY_1000BASE_T_CTRL_REG, &regVal);
	tmp |= (((regVal & ETH_PHY_1000BASE_ADVERTISE_MASK) >> ETH_PHY_1000BASE_ADVERTISE_OFFSET) << 4);
	*advertise = tmp;

	return MV_OK;
}
