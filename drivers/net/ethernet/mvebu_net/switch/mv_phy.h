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
#ifndef __mv_phy_h__
#define __mv_phy_h__


int mv_phy_port_power_state_set(unsigned int lport, GT_BOOL state);
int mv_phy_port_power_state_get(unsigned int lport, GT_BOOL *state);
int mv_phy_port_autoneg_mode_set(unsigned int lport, GT_BOOL autoneg_state, GT_PHY_AUTO_MODE autoneg_mode);
int mv_phy_port_autoneg_mode_get(unsigned int lport, GT_BOOL *autoneg_state, GT_PHY_AUTO_MODE *autoneg_mode);
int mv_phy_port_autoneg_restart(unsigned int lport);
int mv_phy_port_pause_mode_set(unsigned int lport, GT_PHY_PAUSE_MODE state);
int mv_phy_port_pause_mode_get(unsigned int lport, GT_PHY_PAUSE_MODE *state);
int mv_phy_port_pause_state_get(unsigned int lport, GT_BOOL *state);
int mv_phy_port_egr_loopback_set(unsigned int lport, GT_BOOL enable);
int mv_phy_port_egr_loopback_get(unsigned int lport, GT_BOOL *enable);
int mv_phy_port_ingr_loopback_set(unsigned int lport, GT_BOOL enable);
int mv_phy_port_ingr_loopback_get(unsigned int lport, GT_BOOL *enable);
int mv_phy_port_speed_set(unsigned int lport, GT_PHY_SPEED speed);
int mv_phy_port_duplex_set(unsigned int lport, GT_BOOL mode);
#endif /* __mv_phy_h__ */
