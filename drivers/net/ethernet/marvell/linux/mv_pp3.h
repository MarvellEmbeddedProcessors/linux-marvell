/******************************************************************************
*Copyright (C) Marvell International Ltd. and its affiliates
*
*This software file (the "File") is owned and distributed by Marvell
*International Ltd. and/or its affiliates ("Marvell") under the following
*alternative licensing terms.  Once you have made an election to distribute the
*File under one of the following license alternatives, please (i) delete this
*introductory statement regarding license alternatives, (ii) delete the two
*license alternatives that you have not elected to use and (iii) preserve the
*Marvell copyright notice above.
*
******************************************************************************
*Marvell GPL License Option
*
*If you received this File from Marvell, you may opt to use, redistribute and/or
*modify this File in accordance with the terms and conditions of the General
*Public License Version 2, June 1991 (the "GPL License"), a copy of which is
*available along with the File in the license.txt file or on the worldwide web
*at http://www.gnu.org/licenses/gpl.txt.
*
*THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
*WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
*DISCLAIMED.  The GPL License provides additional details about this warranty
*disclaimer.
******************************************************************************/

/*  mv_pp3.h */

#ifndef LINUX_MV_PP3_H
#define LINUX_MV_PP3_H

#define MV_PP3_PORT_NAME	"mv_pp3_port"
#define MV_PP3_SHARED_NAME	"mv_pp3_shared"

struct mv_pp3_mac_data {
	/* Whether a PHY is present, and if yes, at which address. */
	int	phy_addr;
	int	port_mode;
	int	link_irq;
	bool	force_link;
};

/* PP3 system shared data */
struct mv_pp3_plat_data {
	/* Global parameters common for all ports */
	unsigned int		tclk;
	int			max_port;

	/* Controller Model (Device ID) and Revision */
	unsigned int		ctrl_model;
	unsigned int		ctrl_rev;
	unsigned int		nss_mac_mask;
	struct mv_pp3_mac_data	macs_data[4];
};

/* PP3 per port data */
struct mv_pp3_port_data {
	int           id;
	unsigned int  cpu_mask;
	int           mtu;

	/* Use this MAC address if it is valid */
	u8       mac_addr[6];

	/* How many RX/TX queues to use per cpu.*/
	int      num_rxqs_per_cpu;
	int      num_txqs_per_cpu;

	/* Override default RX/TX queue sizes if nonzero.*/
	int      rx_queue_size;
	int      tx_queue_size;
	unsigned int flags;
};

/* mv_pp3_port_data flags */
#define MV_PP3_PORT_DATA_F_NSS_BIT	0
#define MV_PP3_PORT_DATA_F_NSS		BIT(MV_PP3_PORT_DATA_F_NSS_BIT)

#endif  /* LINUX_MV_PP3_H */
