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
#include "mvEthGmacApi.h"
#include "gbe/mvPp2Gbe.h"

void mvGmacPortEnable(int port)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_0_REG(port));
	regVal |= ETH_GMAC_PORT_EN_MASK;
	regVal |= ETH_GMAC_MIB_CNTR_EN_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_0_REG(port), regVal);
}

void mvGmacPortDisable(int port)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_0_REG(port));
	regVal &= ~(ETH_GMAC_PORT_EN_MASK);
	MV_REG_WRITE(ETH_GMAC_CTRL_0_REG(port), regVal);
}

void mvGmacPortMhSet(int port, int enable)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_4_REG(port));

	if (enable)
		regVal |= ETH_GMAC_MH_ENABLE_MASK;
	else
		regVal &= ~ETH_GMAC_MH_ENABLE_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_4_REG(port), regVal);
}

static void mvGmacPortRgmiiSet(int port, int enable)
{
	MV_U32  regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_2_REG(port));
	if (enable)
		regVal |= ETH_GMAC_PORT_RGMII_MASK;
	else
		regVal &= ~ETH_GMAC_PORT_RGMII_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_2_REG(port), regVal);
}

static void mvGmacPortSgmiiSet(int port, int enable)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_2_REG(port));

	if (enable)
		regVal |= (ETH_GMAC_PCS_ENABLE_MASK | ETH_GMAC_INBAND_AN_MASK);
	else
		regVal &= ~(ETH_GMAC_PCS_ENABLE_MASK | ETH_GMAC_INBAND_AN_MASK);

	MV_REG_WRITE(ETH_GMAC_CTRL_2_REG(port), regVal);
}

void mvGmacPortPeriodicXonSet(int port, int enable)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_1_REG(port));

	if (enable)
		regVal |= ETH_GMAC_PERIODIC_XON_EN_MASK;
	else
		regVal &= ~ETH_GMAC_PERIODIC_XON_EN_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_1_REG(port), regVal);
}

void mvGmacPortLbSet(int port, int isGmii, int isPcsEn)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_1_REG(port));

	if (isGmii)
		regVal |= ETH_GMAC_GMII_LB_EN_MASK;
	else
		regVal &= ~ETH_GMAC_GMII_LB_EN_MASK;

	if (isPcsEn)
		regVal |= ETH_GMAC_PCS_LB_EN_MASK;
	else
		regVal &= ~ETH_GMAC_PCS_LB_EN_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_1_REG(port), regVal);
}

void mvGmacPortResetSet(int port, MV_BOOL setReset)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_CTRL_2_REG(port));
	regVal &= ~ETH_GMAC_PORT_RESET_MASK;

	if (setReset == MV_TRUE)
		regVal |= ETH_GMAC_PORT_RESET_MASK;
	else
		regVal &= ~ETH_GMAC_PORT_RESET_MASK;

	MV_REG_WRITE(ETH_GMAC_CTRL_2_REG(port), regVal);

	if (setReset == MV_FALSE)
		while (MV_REG_READ(ETH_GMAC_CTRL_2_REG(port) &
		       ETH_GMAC_PORT_RESET_MASK));
}

void mvGmacPortPowerUp(int port, MV_BOOL isSgmii, MV_BOOL isRgmii)
{
	mvGmacPortSgmiiSet(port, isSgmii);
	mvGmacPortRgmiiSet(port, isRgmii);
	mvGmacPortPeriodicXonSet(port, MV_FALSE);
	mvGmacPortResetSet(port, MV_FALSE);
}

void mvGmacDefaultsSet(int port)
{
	MV_U32 regVal;

	/* Update TX FIFO MIN Threshold */
	regVal = MV_REG_READ(GMAC_PORT_FIFO_CFG_1_REG(port));
	regVal &= ~GMAC_TX_FIFO_MIN_TH_ALL_MASK;
	/* Minimal TX threshold must be less than minimal packet length */
	regVal |= GMAC_TX_FIFO_MIN_TH_MASK(64 - 4 - 2);
	MV_REG_WRITE(GMAC_PORT_FIFO_CFG_1_REG(port), regVal);
}

void mvGmacPortPowerDown(int port)
{
	mvGmacPortDisable(port);
	mvGmacMibCountersClear(port);
	mvGmacPortResetSet(port, MV_TRUE);
}

MV_BOOL mvGmacPortIsLinkUp(int port)
{
	return (MV_REG_READ(ETH_GMAC_STATUS_REG(port)) & ETH_GMAC_LINK_UP_MASK);
}

MV_STATUS mvGmacLinkStatus(int port, MV_ETH_PORT_STATUS *pStatus)
{
	MV_U32 regVal;

	if (MV_PON_PORT(port)) {
		pStatus->linkup = MV_TRUE;
		pStatus->speed = MV_ETH_SPEED_1000;
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
		pStatus->rxFc = MV_ETH_FC_DISABLE;
		pStatus->txFc = MV_ETH_FC_DISABLE;
		return MV_OK;
	}

	regVal = MV_REG_READ(ETH_GMAC_STATUS_REG(port));

	if (regVal & ETH_GMAC_SPEED_1000_MASK)
		pStatus->speed = MV_ETH_SPEED_1000;
	else if (regVal & ETH_GMAC_SPEED_100_MASK)
		pStatus->speed = MV_ETH_SPEED_100;
	else
		pStatus->speed = MV_ETH_SPEED_10;

	if (regVal & ETH_GMAC_LINK_UP_MASK)
		pStatus->linkup = MV_TRUE;
	else
		pStatus->linkup = MV_FALSE;

	if (regVal & ETH_GMAC_FULL_DUPLEX_MASK)
		pStatus->duplex = MV_ETH_DUPLEX_FULL;
	else
		pStatus->duplex = MV_ETH_DUPLEX_HALF;

	if (regVal & ETH_TX_FLOW_CTRL_ACTIVE_MASK)
		pStatus->txFc = MV_ETH_FC_ACTIVE;
	else if (regVal & ETH_TX_FLOW_CTRL_ENABLE_MASK)
		pStatus->txFc = MV_ETH_FC_ENABLE;
	else
		pStatus->txFc = MV_ETH_FC_DISABLE;

	if (regVal & ETH_RX_FLOW_CTRL_ACTIVE_MASK)
		pStatus->rxFc = MV_ETH_FC_ACTIVE;
	else if (regVal & ETH_RX_FLOW_CTRL_ENABLE_MASK)
		pStatus->rxFc = MV_ETH_FC_ENABLE;
	else
		pStatus->rxFc = MV_ETH_FC_DISABLE;

	return MV_OK;
}

char *mvGmacSpeedStrGet(MV_ETH_PORT_SPEED speed)
{
	char *str;

	switch (speed) {
	case MV_ETH_SPEED_10:
		str = "10 Mbps";
		break;
	case MV_ETH_SPEED_100:
		str = "100 Mbps";
		break;
	case MV_ETH_SPEED_1000:
		str = "1 Gbps";
		break;
	case MV_ETH_SPEED_2000:
		str = "2 Gbps";
		break;
	case MV_ETH_SPEED_AN:
		str = "AutoNeg";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

/******************************************************************************/
/*                          Port Configuration functions                      */
/******************************************************************************/

/*******************************************************************************
* mvNetaMaxRxSizeSet -
*
* DESCRIPTION:
*       Change maximum receive size of the port. This configuration will take place
*       imidiately.
*
* INPUT:
*
* RETURN:
*******************************************************************************/
MV_STATUS mvGmacMaxRxSizeSet(int port, int maxRxSize)
{
	MV_U32		regVal;

	if (MV_PON_PORT(port))
		return MV_ERROR;

	regVal =  MV_REG_READ(ETH_GMAC_CTRL_0_REG(port));
	regVal &= ~ETH_GMAC_MAX_RX_SIZE_MASK;
	regVal |= (((maxRxSize - MV_ETH_MH_SIZE) / 2) << ETH_GMAC_MAX_RX_SIZE_OFFS);
	MV_REG_WRITE(ETH_GMAC_CTRL_0_REG(port), regVal);
	return MV_OK;
}

/*******************************************************************************
* mvGmacForceLinkModeSet -
*
* DESCRIPTION:
*       Sets "Force Link Pass" and "Do Not Force Link Fail" bits.
* 	Note: This function should only be called when the port is disabled.
*
* INPUT:
* 	int		portNo			- port number
* 	MV_BOOL force_link_pass	- Force Link Pass
* 	MV_BOOL force_link_fail - Force Link Failure
*		0, 0 - normal state: detect link via PHY and connector
*		1, 1 - prohibited state.
*
* RETURN:
*******************************************************************************/
MV_STATUS mvGmacForceLinkModeSet(int portNo, MV_BOOL force_link_up, MV_BOOL force_link_down)
{
	MV_U32 regVal;

	/* Can't force link pass and link fail at the same time */
	if ((force_link_up) && (force_link_down))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(ETH_GMAC_AN_CTRL_REG(portNo));

	if (force_link_up)
		regVal |= ETH_FORCE_LINK_PASS_MASK;
	else
		regVal &= ~ETH_FORCE_LINK_PASS_MASK;

	if (force_link_down)
		regVal |= ETH_FORCE_LINK_FAIL_MASK;
	else
		regVal &= ~ETH_FORCE_LINK_FAIL_MASK;

	MV_REG_WRITE(ETH_GMAC_AN_CTRL_REG(portNo), regVal);

	return MV_OK;
}

/*******************************************************************************
* mvGmacSpeedDuplexSet -
*
* DESCRIPTION:
*       Sets port speed to Auto Negotiation / 1000 / 100 / 10 Mbps.
*	Sets port duplex to Auto Negotiation / Full / Half Duplex.
*
* INPUT:
* 	int portNo - port number
* 	MV_ETH_PORT_SPEED speed - port speed
*	MV_ETH_PORT_DUPLEX duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvGmacSpeedDuplexSet(int portNo, MV_ETH_PORT_SPEED speed, MV_ETH_PORT_DUPLEX duplex)
{
	MV_U32 regVal;

	/* Check validity */
	if ((speed == MV_ETH_SPEED_1000) && (duplex == MV_ETH_DUPLEX_HALF))
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(ETH_GMAC_AN_CTRL_REG(portNo));

	switch (speed) {
	case MV_ETH_SPEED_AN:
		regVal |= ETH_ENABLE_SPEED_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_SPEED_1000:
		regVal &= ~ETH_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal |= ETH_SET_GMII_SPEED_1000_MASK;
		/* the 100/10 bit doesn't matter in this case */
		break;
	case MV_ETH_SPEED_100:
		regVal &= ~ETH_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal &= ~ETH_SET_GMII_SPEED_1000_MASK;
		regVal |= ETH_SET_MII_SPEED_100_MASK;
		break;
	case MV_ETH_SPEED_10:
		regVal &= ~ETH_ENABLE_SPEED_AUTO_NEG_MASK;
		regVal &= ~ETH_SET_GMII_SPEED_1000_MASK;
		regVal &= ~ETH_SET_MII_SPEED_100_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Speed value %d\n", speed);
		return MV_BAD_PARAM;
	}

	switch (duplex) {
	case MV_ETH_DUPLEX_AN:
		regVal  |= ETH_ENABLE_DUPLEX_AUTO_NEG_MASK;
		/* the other bits don't matter in this case */
		break;
	case MV_ETH_DUPLEX_HALF:
		regVal &= ~ETH_ENABLE_DUPLEX_AUTO_NEG_MASK;
		regVal &= ~ETH_SET_FULL_DUPLEX_MASK;
		break;
	case MV_ETH_DUPLEX_FULL:
		regVal &= ~ETH_ENABLE_DUPLEX_AUTO_NEG_MASK;
		regVal |= ETH_SET_FULL_DUPLEX_MASK;
		break;
	default:
		mvOsPrintf("Unexpected Duplex value %d\n", duplex);
		return MV_BAD_PARAM;
	}

	MV_REG_WRITE(ETH_GMAC_AN_CTRL_REG(portNo), regVal);
	return MV_OK;
}

/*******************************************************************************
* mvGmacSpeedDuplexGet -
*
* DESCRIPTION:
*       Gets port speed
*	Gets port duplex
*
* INPUT:
* 	int portNo - port number
* OUTPUT:
* 	MV_ETH_PORT_SPEED *speed - port speed
*	MV_ETH_PORT_DUPLEX *duplex - port duplex mode
*
* RETURN:
*******************************************************************************/
MV_STATUS mvGmacSpeedDuplexGet(int portNo, MV_ETH_PORT_SPEED *speed, MV_ETH_PORT_DUPLEX *duplex)
{
	MV_U32 regVal;

	/* Check validity */
	if (!speed || !duplex)
		return MV_BAD_PARAM;

	regVal = MV_REG_READ(ETH_GMAC_AN_CTRL_REG(portNo));
	if (regVal & ETH_ENABLE_SPEED_AUTO_NEG_MASK)
		*speed = MV_ETH_SPEED_AN;
	else if (regVal & ETH_SET_GMII_SPEED_1000_MASK)
		*speed = MV_ETH_SPEED_1000;
	else if (regVal & ETH_SET_MII_SPEED_100_MASK)
		*speed = MV_ETH_SPEED_100;
	else
		*speed = MV_ETH_SPEED_10;

	if (regVal & ETH_ENABLE_DUPLEX_AUTO_NEG_MASK)
		*duplex = MV_ETH_DUPLEX_AN;
	else if (regVal & ETH_SET_FULL_DUPLEX_MASK)
		*duplex = MV_ETH_DUPLEX_FULL;
	else
		*duplex = MV_ETH_DUPLEX_HALF;

	return MV_OK;
}

/*******************************************************************************
* mvGmacFlowCtrlSet - Set Flow Control of the port.
*
* DESCRIPTION:
*       This function configures the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*       MV_ETH_PORT_FC  flowControl - Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*       MV_BAD_VALUE    - Value flowControl parameters is not valid
*
*******************************************************************************/
MV_STATUS mvGmacFlowCtrlSet(int port, MV_ETH_PORT_FC flowControl)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_AN_CTRL_REG(port));

	switch (flowControl) {
	case MV_ETH_FC_AN_NO:
		regVal |= ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal &= ~ETH_FLOW_CONTROL_ADVERTISE_MASK;
		regVal &= ~ETH_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_AN_SYM:
		regVal |= ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= ETH_FLOW_CONTROL_ADVERTISE_MASK;
		regVal &= ~ETH_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_AN_ASYM:
		regVal |= ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= ETH_FLOW_CONTROL_ADVERTISE_MASK;
		regVal |= ETH_FLOW_CONTROL_ASYMETRIC_MASK;
		break;

	case MV_ETH_FC_DISABLE:
		regVal &= ~ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal &= ~ETH_SET_FLOW_CONTROL_MASK;
		break;

	case MV_ETH_FC_ENABLE:
		regVal &= ~ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK;
		regVal |= ETH_SET_FLOW_CONTROL_MASK;
		break;

	default:
		mvOsPrintf("ethDrv: Unexpected FlowControl value %d\n", flowControl);
		return MV_BAD_VALUE;
	}

	MV_REG_WRITE(ETH_GMAC_AN_CTRL_REG(port), regVal);

	return MV_OK;
}

/*******************************************************************************
* mvGmacFlowCtrlGet - Get Flow Control configuration of the port.
*
* DESCRIPTION:
*       This function returns the port's Flow Control properties.
*
* INPUT:
*       int				port		- Port number
*
* OUTPUT:
*       MV_ETH_PORT_FC  *flowCntrl	- Flow control of the port.
*
* RETURN:   MV_STATUS
*       MV_OK           - Success
*       MV_OUT_OF_RANGE - Failed. Port is out of valid range
*
*******************************************************************************/
MV_STATUS mvGmacFlowCtrlGet(int port, MV_ETH_PORT_FC *pFlowCntrl)
{
	MV_U32 regVal;

	regVal = MV_REG_READ(ETH_GMAC_AN_CTRL_REG(port));

	if (regVal & ETH_ENABLE_FLOW_CONTROL_AUTO_NEG_MASK) {
		/* Auto negotiation is enabled */
		if (regVal & ETH_FLOW_CONTROL_ADVERTISE_MASK) {
			if (regVal & ETH_FLOW_CONTROL_ASYMETRIC_MASK)
				*pFlowCntrl = MV_ETH_FC_AN_ASYM;
			else
				*pFlowCntrl = MV_ETH_FC_AN_SYM;
		} else
			*pFlowCntrl = MV_ETH_FC_AN_NO;
	} else {
		/* Auto negotiation is disabled */
		if (regVal & ETH_SET_FLOW_CONTROL_MASK)
			*pFlowCntrl = MV_ETH_FC_ENABLE;
		else
			*pFlowCntrl = MV_ETH_FC_DISABLE;
	}
	return MV_OK;
}

MV_STATUS mvGmacPortLinkSpeedFlowCtrl(int port, MV_ETH_PORT_SPEED speed,
				     int forceLinkUp)
{
	if (forceLinkUp) {
		if (mvGmacSpeedDuplexSet(port, speed, MV_ETH_DUPLEX_FULL)) {
			mvOsPrintf("mvGmacSpeedDuplexSet failed\n");
			return MV_FAIL;
		}
		if (mvGmacFlowCtrlSet(port, MV_ETH_FC_ENABLE)) {
			mvOsPrintf("mvGmacFlowCtrlSet failed\n");
			return MV_FAIL;
		}
		if (mvGmacForceLinkModeSet(port, 1, 0)) {
			mvOsPrintf("mvGmacForceLinkModeSet failed\n");
			return MV_FAIL;
		}
	} else {
		if (mvGmacForceLinkModeSet(port, 0, 0)) {
			mvOsPrintf("mvGmacForceLinkModeSet failed\n");
			return MV_FAIL;
		}
		if (mvGmacSpeedDuplexSet(port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN)) {
			mvOsPrintf("mvGmacSpeedDuplexSet failed\n");
			return MV_FAIL;
		}
		if (mvGmacFlowCtrlSet(port, MV_ETH_FC_AN_SYM)) {
			mvOsPrintf("mvGmacFlowCtrlSet failed\n");
			return MV_FAIL;
		}
	}

	return MV_OK;
}

/******************************************************************************/
/*                         PHY Control Functions                              */
/******************************************************************************/
void mvGmacPhyAddrSet(int port, int phyAddr)
{
	unsigned int regData;

	regData = MV_REG_READ(ETH_PHY_ADDR_REG);

	regData &= ~ETH_PHY_ADDR_MASK(port);
	regData |= ((phyAddr << ETH_PHY_ADDR_OFFS(port)) & ETH_PHY_ADDR_MASK(port));

	MV_REG_WRITE(ETH_PHY_ADDR_REG, regData);

	return;
}

int mvGmacPhyAddrGet(int port)
{
	unsigned int 	regData;

	regData = MV_REG_READ(ETH_PHY_ADDR_REG);

	return ((regData & ETH_PHY_ADDR_MASK(port)) >> ETH_PHY_ADDR_OFFS(port));
}

void mvGmacPrintReg(unsigned int reg_addr, char *reg_name)
{
	mvOsPrintf("  %-32s: 0x%x = 0x%08x\n", reg_name, reg_addr, MV_REG_READ(reg_addr));
}

void mvGmacLmsRegs(void)
{
	mvOsPrintf("\n[GoP LMS registers]\n");

	mvGmacPrintReg(ETH_PHY_ADDR_REG, "MV_GOP_LMS_PHY_ADDR_REG");
	mvGmacPrintReg(ETH_PHY_AN_CFG0_REG, "MV_GOP_LMS_PHY_AN_CFG0_REG");
}

void mvGmacPortRegs(int port)
{
	if (mvPp2PortCheck(port))
		return;

	if (MV_PON_PORT(port)) {
		mvOsPrintf("Not supported for PON port\n");
		return;
	}

	port = MV_PPV2_PORT_PHYS(port);

	mvOsPrintf("\n[GoP MAC #%d registers]\n", port);

	mvGmacPrintReg(ETH_GMAC_CTRL_0_REG(port), "MV_GMAC_CTRL_0_REG");
	mvGmacPrintReg(ETH_GMAC_CTRL_1_REG(port), "MV_GMAC_CTRL_1_REG");
	mvGmacPrintReg(ETH_GMAC_CTRL_2_REG(port), "MV_GMAC_CTRL_2_REG");
	mvGmacPrintReg(ETH_GMAC_CTRL_3_REG(port), "MV_GMAC_CTRL_3_REG");
	mvGmacPrintReg(ETH_GMAC_CTRL_4_REG(port), "MV_GMAC_CTRL_4_REG");

	mvGmacPrintReg(ETH_GMAC_AN_CTRL_REG(port), "MV_GMAC_AN_CTRL_REG");
	mvGmacPrintReg(ETH_GMAC_STATUS_REG(port), "MV_GMAC_STATUS_REG");

	mvGmacPrintReg(GMAC_PORT_FIFO_CFG_0_REG(port), "MV_GMAC_PORT_FIFO_CFG_0_REG");
	mvGmacPrintReg(GMAC_PORT_FIFO_CFG_1_REG(port), "MV_GMAC_PORT_FIFO_CFG_1_REG");

	mvGmacPrintReg(ETH_PORT_ISR_CAUSE_REG(port), "MV_GMAC_ISR_CAUSE_REG");
	mvGmacPrintReg(ETH_PORT_ISR_MASK_REG(port), "MV_GMAC_ISR_MASK_REG");

#ifdef CONFIG_MV_ETH_PP2_1
	mvGmacPrintReg(ETH_PORT_ISR_SUM_CAUSE_REG(port), "MV_GMAC_ISR_SUM_CAUSE_REG");
	mvGmacPrintReg(ETH_PORT_ISR_SUM_MASK_REG(port), "MV_GMAC_ISR_SUM_MASK_REG");
#endif

}


/******************************************************************************/
/*                      MIB Counters functions                                */
/******************************************************************************/

/*******************************************************************************
* mvGmacMibCounterRead - Read a MIB counter
*
* DESCRIPTION:
*       This function reads a MIB counter of a specific ethernet port.
*       NOTE - Read from ETH_MIB_GOOD_OCTETS_RECEIVED_LOW or
*              ETH_MIB_GOOD_OCTETS_SENT_LOW counters will return 64 bits value,
*              so pHigh32 pointer should not be NULL in this case.
*
* INPUT:
*       port        - Ethernet Port number.
*       mibOffset   - MIB counter offset.
*
* OUTPUT:
*       MV_U32*       pHigh32 - pointer to place where 32 most significant bits
*                             of the counter will be stored.
*
* RETURN:
*       32 low sgnificant bits of MIB counter value.
*
*******************************************************************************/
MV_U32 mvGmacMibCounterRead(int port, unsigned int mibOffset, MV_U32 *pHigh32)
{
	MV_U32 valLow32, valHigh32;

	valLow32 = MV_REG_READ(ETH_MIB_COUNTERS_BASE(port) + mibOffset);

	/* Implement FEr ETH. Erroneous Value when Reading the Upper 32-bits    */
	/* of a 64-bit MIB Counter.                                             */
	if ((mibOffset == ETH_MIB_GOOD_OCTETS_RECEIVED_LOW) || (mibOffset == ETH_MIB_GOOD_OCTETS_SENT_LOW)) {
		valHigh32 = MV_REG_READ(ETH_MIB_COUNTERS_BASE(port) + mibOffset + 4);
		if (pHigh32 != NULL)
			*pHigh32 = valHigh32;
	}
	return valLow32;
}

/*******************************************************************************
* mvGmacMibCountersClear - Clear all MIB counters
*
* DESCRIPTION:
*       This function clears all MIB counters
*
* INPUT:
*       port      - Ethernet Port number.
*
* RETURN:   void
*
*******************************************************************************/
void mvGmacMibCountersClear(int port)
{
	int i;

	if (MV_PON_PORT(port))
		return;

	/* Perform dummy reads from MIB counters */
	/* Read of last counter clear all counter were read before */
	for (i = ETH_MIB_GOOD_OCTETS_RECEIVED_LOW; i <= ETH_MIB_LATE_COLLISION; i += 4)
		MV_REG_READ((ETH_MIB_COUNTERS_BASE(port) + i));
}

static void mvGmacMibPrint(int port, MV_U32 offset, char *mib_name)
{
	MV_U32 regVaLo, regValHi = 0;

	regVaLo = mvGmacMibCounterRead(port, offset, &regValHi);

	if (!regValHi)
		mvOsPrintf("  %-32s: 0x%02x = %u\n", mib_name, offset, regVaLo);
	else
		mvOsPrintf("  %-32s: 0x%02x = 0x%08x%08x\n", mib_name, offset, regValHi, regVaLo);
}

/* Print MIB counters of the Ethernet port */
void mvGmacMibCountersShow(int port)
{
	if (mvPp2PortCheck(port))
		return;

	if (MV_PON_PORT(port)) {
		mvOsPrintf("%s: not supported for PON port\n", __func__);
		return;
	}

	mvOsPrintf("\nMIBs: port=%d, base=0x%x\n", port, ETH_MIB_COUNTERS_BASE(port));

	mvOsPrintf("\n[Rx]\n");
	mvGmacMibPrint(port, ETH_MIB_GOOD_OCTETS_RECEIVED_LOW, "GOOD_OCTETS_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_BAD_OCTETS_RECEIVED, "BAD_OCTETS_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_UNICAST_FRAMES_RECEIVED, "UNCAST_FRAMES_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_BROADCAST_FRAMES_RECEIVED, "BROADCAST_FRAMES_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_MULTICAST_FRAMES_RECEIVED, "MULTICAST_FRAMES_RECEIVED");

	mvOsPrintf("\n[RMON]\n");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_64_OCTETS, "FRAMES_64_OCTETS");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_65_TO_127_OCTETS, "FRAMES_65_TO_127_OCTETS");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_128_TO_255_OCTETS, "FRAMES_128_TO_255_OCTETS");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_256_TO_511_OCTETS, "FRAMES_256_TO_511_OCTETS");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_512_TO_1023_OCTETS, "FRAMES_512_TO_1023_OCTETS");
	mvGmacMibPrint(port, ETH_MIB_FRAMES_1024_TO_MAX_OCTETS, "FRAMES_1024_TO_MAX_OCTETS");

	mvOsPrintf("\n[Tx]\n");
	mvGmacMibPrint(port, ETH_MIB_GOOD_OCTETS_SENT_LOW, "GOOD_OCTETS_SENT");
	mvGmacMibPrint(port, ETH_MIB_UNICAST_FRAMES_SENT, "UNICAST_FRAMES_SENT");
	mvGmacMibPrint(port, ETH_MIB_MULTICAST_FRAMES_SENT, "MULTICAST_FRAMES_SENT");
	mvGmacMibPrint(port, ETH_MIB_BROADCAST_FRAMES_SENT, "BROADCAST_FRAMES_SENT");
	mvGmacMibPrint(port, ETH_MIB_CRC_ERRORS_SENT, "CRC_ERRORS_SENT");

	mvOsPrintf("\n[FC control]\n");
	mvGmacMibPrint(port, ETH_MIB_FC_RECEIVED, "FC_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_FC_SENT, "FC_SENT");

	mvOsPrintf("\n[Errors]\n");
	mvGmacMibPrint(port, ETH_MIB_RX_FIFO_OVERRUN, "ETH_MIB_RX_FIFO_OVERRUN");
	mvGmacMibPrint(port, ETH_MIB_UNDERSIZE_RECEIVED, "UNDERSIZE_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_FRAGMENTS_RECEIVED, "FRAGMENTS_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_OVERSIZE_RECEIVED, "OVERSIZE_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_JABBER_RECEIVED, "JABBER_RECEIVED");
	mvGmacMibPrint(port, ETH_MIB_MAC_RECEIVE_ERROR, "MAC_RECEIVE_ERROR");
	mvGmacMibPrint(port, ETH_MIB_BAD_CRC_EVENT, "BAD_CRC_EVENT");
	mvGmacMibPrint(port, ETH_MIB_COLLISION, "COLLISION");
	/* This counter must be read last. Read it clear all the counters */
	mvGmacMibPrint(port, ETH_MIB_LATE_COLLISION, "LATE_COLLISION");
}
