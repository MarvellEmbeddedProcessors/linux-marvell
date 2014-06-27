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
/*  mv_pp2.h */

#ifndef LINUX_MV_PP2_H
#define LINUX_MV_PP2_H

#define MV_PP2_PORT_NAME	"mv_pp2_port"

/* valid values for flags */
#define MV_PP2_PDATA_F_SGMII		0x1 /* MAC connected to PHY via SGMII, PCS block is active */
#define MV_PP2_PDATA_F_RGMII		0x2 /* MAC connected to PHY via RGMII */
#define MV_PP2_PDATA_F_LB		0x4 /* This port is serve as LoopBack port */
#define MV_PP2_PDATA_F_LINUX_CONNECT	0x8 /* This port is connected to Linux */

struct mv_pp2_pdata {

	/* Global parameters common for all ports */
	unsigned int  tclk;
	int           max_port;

	/* Controller Model (Device ID) and Revision */
	unsigned int  ctrl_model;
	unsigned int  ctrl_rev;

	/* Per port parameters */
	unsigned int  cpu_mask;
	int           mtu;

	/* Whether a PHY is present, and if yes, at which address. */
	int      phy_addr;

	/* Use this MAC address if it is valid */
	u8       mac_addr[6];

	/*
	* If speed is 0, autonegotiation is enabled.
	*   Valid values for speed: 0, SPEED_10, SPEED_100, SPEED_1000.
	*   Valid values for duplex: DUPLEX_HALF, DUPLEX_FULL.
	*/
	int      speed;
	int      duplex;

	int	     is_sgmii;
	int	     is_rgmii;

	/* port interrupt line number */
	int		 irq;

	/*
	* How many RX/TX queues to use.
	*/
	int      rx_queue_count;
	int      tx_queue_count;

	/*
	* Override default RX/TX queue sizes if nonzero.
	*/
	int      rx_queue_size;
	int      tx_queue_size;

	unsigned int flags;
};


#endif  /* LINUX_MV_PP2_H */
