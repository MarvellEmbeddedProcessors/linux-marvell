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
#ifndef __mv_xlg_mac_if_h__
#define __mv_xlg_mac_if_h__

#include "common/mv_sw_if.h"
#include "gop/mv_gop_if.h"


/* print value of unit registers */
void mv_xlg_mac_regs_dump(int port);
/* Set the MAC to reset or exit from reset */
int mv_xlg_mac_reset(int mac_num, enum mv_reset reset);
/* Set the internal mux's to the required MAC in the GOP */
int mv_xlg_mac_mode_cfg(int mac_num, int num_of_lanes);
/* Configure MAC loopback */
int mv_xlg_mac_loopback_cfg(int mac_num, enum mv_lb_type type);
/* Get MAC link status */
bool mv_xlg_mac_link_status_get(int mac_num);
/* Enable port and MIB counters */
void mv_xlg_mac_port_enable(int mac_num);
/* Disable port */
void mv_xlg_mac_port_disable(int mac_num);
void mv_xlg_mac_port_periodic_xon_set(int mac_num, int enable);
int mv_xlg_mac_link_status(int mac_num, struct mv_port_link_status *pstatus);
/* Change maximum receive size of the port */
int mv_xlg_mac_max_rx_size_set(int mac_num, int max_rx_size);
int mv_xlg_mac_force_link_mode_set(int mac_num, bool force_link_up, bool force_link_down);
int mv_xlg_mac_speed_duplex_set(int mac_num, enum mv_port_speed speed, enum mv_port_duplex duplex);
/* Gets port speed and duplex */
int mv_xlg_mac_speed_duplex_get(int mac_num, enum mv_port_speed *speed, enum mv_port_duplex *duplex);
/* Configure the port's Flow Control properties */
int mv_xlg_mac_fc_set(int mac_num, enum mv_port_fc fc);
/* Get Flow Control configuration of the port */
void mv_xlg_mac_fc_get(int mac_num, enum mv_port_fc *fc);
int mv_xlg_mac_port_link_speed_fc(int port, enum mv_port_speed speed, int force_link_up);
/* Link Interrupt Handle */
void mv_xlg_port_link_event_unmask(int mac_num);
void mv_xlg_port_link_event_mask(int mac_num);
void mv_xlg_port_external_event_unmask(int mac_num, int bit_2_open);
void mv_xlg_port_link_event_clear(int mac_num);
void mv_xlg_2_gig_mac_cfg(int mac_num);

#endif /* __mv_xlg_mac_if_h__ */
