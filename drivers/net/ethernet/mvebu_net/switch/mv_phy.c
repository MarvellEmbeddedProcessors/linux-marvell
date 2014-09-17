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

#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/mv_switch.h>
#include <linux/module.h>

#include "mvOs.h"
#include "mvSysHwConfig.h"
#include "eth-phy/mvEthPhy.h"
#ifdef MV_INCLUDE_ETH_COMPLEX
#include "ctrlEnv/mvCtrlEthCompLib.h"
#endif /* MV_INCLUDE_ETH_COMPLEX */

#include "msApi.h"
#include "mv_switch.h"
#include "mv_phy.h"
#include "mv_mux/mv_mux_netdev.h"

/*******************************************************************************
* mv_phy_port_power_state_set
*
* DESCRIPTION:
*	The API configures the PHY port state of given switch logical port.
* INPUTS:
*	lport  - logical switch PHY port ID.
*	state  - PHY port power state to set.
*			GT_TRUE: power on
*			GT_FALSE: power down
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_power_state_set(unsigned int lport, GT_BOOL state)
{
	GT_BOOL power_state;
	GT_BOOL pre_power_state;
	GT_STATUS rc = GT_OK;

	if (state == GT_TRUE)
		power_state = GT_FALSE;
	else
		power_state = GT_TRUE;

	/* get the current link status */
	rc = gprtGetPortPowerDown(mv_switch_qd_dev_get(), lport, &pre_power_state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortPowerDown()\n");

	rc = gprtPortPowerDown(mv_switch_qd_dev_get(), lport, power_state);
	SW_IF_ERROR_STR(rc, "failed to call gprtPortPowerDown()\n");

	/* since link change event from HW (via interrupt) does not happen
	   for UP->DOWN link change, only for DOWN->UP after link negotiation,
	   print a port Down state change for this use case */
	if (pre_power_state == GT_FALSE && power_state == GT_TRUE)
		pr_err("Port %d: Link-down\n", lport);

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_power_state_get
*
* DESCRIPTION:
*	The API gets the PHY port state of given switch logical port.
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	state  - PHY port power state to set.
*			GT_TRUE: power on
*			GT_FALSE: power down
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_power_state_get(unsigned int lport, GT_BOOL *state)
{
	GT_BOOL power_state;
	GT_STATUS rc = GT_OK;

	rc = gprtGetPortPowerDown(mv_switch_qd_dev_get(), lport, &power_state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortPowerDown()\n");

	if (power_state == GT_TRUE)
		*state = GT_FALSE;
	else
		*state = GT_TRUE;

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_autoneg_mode_set
*
* DESCRIPTION:
*	The API configures the auto negotiation state of given switch logical port.
* INPUTS:
*	lport          - logical switch PHY port ID.
*	autoneg_state  - autonegotiation state, enabled or disabled.
*	autoneg_mode   - enum:
*			SPEED_AUTO_DUPLEX_AUTO: Auto for both speed and duplex
*			SPEED_1000_DUPLEX_AUTO: 1000Mbps and auto duplex
*			SPEED_100_DUPLEX_AUTO:  100Mbps and auto duplex
*			SPEED_10_DUPLEX_AUTO:   10Mbps and auto duplex
*			SPEED_AUTO_DUPLEX_FULL: Auto for speed only and Full duplex
*			SPEED_AUTO_DUPLEX_HALF: Auto for speed only and Half duplex. (1000Mbps is not supported)
*			SPEED_1000_DUPLEX_FULL: 1000Mbps Full duplex.
*			SPEED_1000_DUPLEX_HALF: 1000Mbps half duplex.
*			SPEED_100_DUPLEX_FULL:  100Mbps Full duplex.
*			SPEED_100_DUPLEX_HALF:  100Mbps half duplex.
*			SPEED_10_DUPLEX_FULL:   10Mbps Full duplex.
*			SPEED_10_DUPLEX_HALF:   10Mbps half duplex.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_autoneg_mode_set(unsigned int lport, GT_BOOL autoneg_state, GT_PHY_AUTO_MODE autoneg_mode)
{
	GT_STATUS rc = GT_OK;

	if (GT_FALSE == autoneg_state) {
		rc = gprtPortAutoNegEnable(mv_switch_qd_dev_get(), lport, GT_FALSE);
		SW_IF_ERROR_STR(rc, "failed to call gprtPortAutoNegEnable()\n");

		return MV_OK;
	} else	{
		rc = gprtSetPortAutoMode(mv_switch_qd_dev_get(), lport, autoneg_mode);
		SW_IF_ERROR_STR(rc, "failed to call gprtSetPortAutoMode()\n");

		rc = gprtPortAutoNegEnable(mv_switch_qd_dev_get(), lport, GT_TRUE);
		SW_IF_ERROR_STR(rc, "failed to call gprtPortAutoNegEnable()\n");
	}

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_autoneg_mode_get
*
* DESCRIPTION:
*	The API gets the auto negotiation state of given switch logical port.
* INPUTS:
*	lport          - logical switch PHY port ID.
*
* OUTPUTS:
*	autoneg_state  - autonegotiation state, enabled or disabled.
*	autoneg_mode   - enum:
*			SPEED_AUTO_DUPLEX_AUTO: Auto for both speed and duplex
*			SPEED_1000_DUPLEX_AUTO: 1000Mbps and auto duplex
*			SPEED_100_DUPLEX_AUTO:  100Mbps and auto duplex
*			SPEED_10_DUPLEX_AUTO:   10Mbps and auto duplex
*			SPEED_AUTO_DUPLEX_FULL: Auto for speed only and Full duplex
*			SPEED_AUTO_DUPLEX_HALF: Auto for speed only and Half duplex. (1000Mbps is not supported)
*			SPEED_1000_DUPLEX_FULL: 1000Mbps Full duplex.
*			SPEED_1000_DUPLEX_HALF: 1000Mbps half duplex.
*			SPEED_100_DUPLEX_FULL:  100Mbps Full duplex.
*			SPEED_100_DUPLEX_HALF:  100Mbps half duplex.
*			SPEED_10_DUPLEX_FULL:   10Mbps Full duplex.
*			SPEED_10_DUPLEX_HALF:   10Mbps half duplex.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_autoneg_mode_get(unsigned int lport, GT_BOOL *autoneg_state, GT_PHY_AUTO_MODE *autoneg_mode)
{
	GT_STATUS rc = GT_OK;

	rc = gprtGetPortAutoNegState(mv_switch_qd_dev_get(), lport, autoneg_state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortAutoNegState()\n");

	rc = gprtGetPortAutoMode(mv_switch_qd_dev_get(), lport, autoneg_mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortAutoMode()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_autoneg_restart
*
* DESCRIPTION:
*	The API restarts the auto negotiation of given switch logical port.
* INPUTS:
*	lport - logical switch PHY port ID.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_autoneg_restart(unsigned int lport)
{
	GT_STATUS rc = GT_OK;

	rc = gprtPortRestartAutoNeg(mv_switch_qd_dev_get(), lport);
	SW_IF_ERROR_STR(rc, "failed to call gprtPortRestartAutoNeg()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_pause_mode_set
*
* DESCRIPTION:
*	This routine will set the pause bit in Autonegotiation Advertisement
*	Register. And restart the autonegotiation.
*
* INPUTS:
*	lport - logical switch PHY port ID.	The physical address, if SERDES device is accessed
*	state - GT_PHY_PAUSE_MODE enum value.
*		GT_PHY_NO_PAUSE		- disable pause
*		GT_PHY_PAUSE		- support pause
*		GT_PHY_ASYMMETRIC_PAUSE	- support asymmetric pause
*		GT_PHY_BOTH_PAUSE	- support both pause and asymmetric pause
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
* COMMENTS:
*	Data sheet register 4.10 Autonegotiation Advertisement Register
*******************************************************************************/
int mv_phy_port_pause_mode_set(unsigned int lport, GT_PHY_PAUSE_MODE state)
{
	GT_STATUS rc = GT_OK;

	rc = gprtSetPause(mv_switch_qd_dev_get(), lport, state);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPause()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_pause_mode_get
*
* DESCRIPTION:
*	This routine will get the pause bit in Autonegotiation Advertisement
*	Register.
*
* INPUTS:
*	lport - logical switch PHY port ID.
*
* OUTPUTS:
*	state - GT_PHY_PAUSE_MODE enum value.
*		GT_PHY_NO_PAUSE		- disable pause
*		GT_PHY_PAUSE		- support pause
*		GT_PHY_ASYMMETRIC_PAUSE	- support asymmetric pause
*		GT_PHY_BOTH_PAUSE	- support both pause and asymmetric pause
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
* COMMENTS:
*	Data sheet register 4.10 Autonegotiation Advertisement Register
*******************************************************************************/
int mv_phy_port_pause_mode_get(unsigned int lport, GT_PHY_PAUSE_MODE *state)
{
	GT_STATUS rc = GT_OK;

	rc = gprtGetPause(mv_switch_qd_dev_get(), lport, state);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPause()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_pause_state_get
*
* DESCRIPTION:
*	This routine will get the current pause state.
*	Register.
*
* INPUTS:
*	lport - logical switch PHY port ID.
*
* OUTPUTS:
*	state - pause state
*		GT_FALSE: MAC pause is not implemented
*		GT_TRUE: MAC pause is implemented
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*******************************************************************************/
int mv_phy_port_pause_state_get(unsigned int lport, GT_BOOL *state)
{
	GT_BOOL force;
	GT_BOOL pause;
	GT_STATUS rc = GT_OK;

	rc = gpcsGetForcedFC(mv_switch_qd_dev_get(), lport, &force);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedFC()\n");

	if (force) {
		rc = gpcsGetFCValue(mv_switch_qd_dev_get(), lport, &pause);
		SW_IF_ERROR_STR(rc, "failed to call gpcsGetFCValue()\n");
	} else {
		rc = gprtGetPauseEn(mv_switch_qd_dev_get(), lport, &pause);
		SW_IF_ERROR_STR(rc, "failed to call gprtGetPauseEn()\n");
	}

	*state = pause;

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_egr_loopback_set
*
* DESCRIPTION:
*	Enable/Disable egress loopback of switch PHY port,
*       and enable/disable force link port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable egree loopback.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Data sheet register 0.14 - Loop_back
*******************************************************************************/
int mv_phy_port_egr_loopback_set(unsigned int lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;
	GT_BOOL link_forced;

	/*clear the PHY detect bit before enable loopback to preven the port from getting locked up due the PPU bug*/
	if (enable == GT_TRUE) {
		rc = gprtSetPHYDetect(mv_switch_qd_dev_get(), lport, GT_FALSE);
		SW_IF_ERROR_STR(rc, "failed to call gprtSetPHYDetect()\n");
	}

	rc = gprtSetPortLoopback(mv_switch_qd_dev_get(), lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortLoopback()\n");

	/*restore the PHY detect bit after disable loopback*/
	if (enable == GT_FALSE) {
		rc = gprtSetPHYDetect(mv_switch_qd_dev_get(), lport, GT_TRUE);
		SW_IF_ERROR_STR(rc, "failed to call gprtSetPHYDetect()\n");
	}

	/* Get port force link statue */
	rc = gpcsGetForcedLink(mv_switch_qd_dev_get(), lport, &link_forced);
	SW_IF_ERROR_STR(rc, "failed to call gpcsGetForcedLink()\n");
	if (((enable == GT_TRUE) && (link_forced == GT_FALSE)) ||
	    ((enable == GT_FALSE) && (link_forced == GT_TRUE))) {
		/* Set force link */
		rc = gpcsSetForcedLink(mv_switch_qd_dev_get(), lport, enable);
		SW_IF_ERROR_STR(rc, "failed to call gpcsSetForcedLink()\n");
		/* Set force link value */
		rc = gpcsSetLinkValue(mv_switch_qd_dev_get(), lport, enable);
		SW_IF_ERROR_STR(rc, "failed to call gpcsSetLinkValue()\n");
	}

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_egr_loopback_get
*
* DESCRIPTION:
*	This API get enabled/disabled state of egress loopback of switch PHY port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable egress loopback.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Data sheet register 0.14 - Loop_back
*******************************************************************************/
int mv_phy_port_egr_loopback_get(unsigned int lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	rc = gprtGetPortLoopback(mv_switch_qd_dev_get(), lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortLoopback()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_ingr_loopback_set
*
* DESCRIPTION:
*	This API sets enabled/disabled state of ingress loopback of switch PHY port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable ingress loopback.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Data sheet register FE:28.4, GE:21_2.14  - Loop_back
********************************************************************************/
int mv_phy_port_ingr_loopback_set(unsigned int lport, GT_BOOL enable)
{
	GT_STATUS rc = GT_OK;

	rc = gprtSetPortLineLoopback(mv_switch_qd_dev_get(), lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortLineLoopback()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_ingr_loopback_get
*
* DESCRIPTION:
*	This API gets enabled/disabled state of ingress loopback of switch PHY port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*
* OUTPUTS:
*	enable - enable/disable ingress loopback.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
*
* COMMENTS:
*	Data sheet register FE:28.4, GE:21_2.14  - Loop_back
********************************************************************************/
int mv_phy_port_ingr_loopback_get(unsigned int lport, GT_BOOL *enable)
{
	GT_STATUS rc = GT_OK;

	rc = gprtGetPortLineLoopback(mv_switch_qd_dev_get(), lport, enable);
	SW_IF_ERROR_STR(rc, "failed to call gprtGetPortLineLoopback()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_speed_set
*
* DESCRIPTION:
*	This API sets the speed of switch PHY port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	enable - enable/disable ingress loopback.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
********************************************************************************/
int mv_phy_port_speed_set(unsigned int lport, GT_PHY_SPEED speed)
{
	GT_STATUS rc = GT_OK;

	rc = gprtSetPortSpeed(mv_switch_qd_dev_get(), lport, speed);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortSpeed()\n");

	return MV_OK;
}

/*******************************************************************************
* mv_phy_port_duplex_set
*
* DESCRIPTION:
*	This API sets the duplex mode of switch PHY port.
*
* INPUTS:
*	lport  - logical switch PHY port ID.
*	mode   - enable or disable duplex mode.
*
* OUTPUTS:
*	None.
*
* RETURNS:
*	On success return MV_OK.
*	On error different types are returned according to the case.
********************************************************************************/
int mv_phy_port_duplex_set(unsigned int lport, GT_BOOL mode)
{
	GT_STATUS rc = GT_OK;

	rc = gprtSetPortDuplexMode(mv_switch_qd_dev_get(), lport, mode);
	SW_IF_ERROR_STR(rc, "failed to call gprtSetPortDuplexMode()\n");

	return MV_OK;
}
