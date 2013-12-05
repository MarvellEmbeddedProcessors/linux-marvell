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

#ifndef L2FW_MV_ETH_L2FW_H
#define L2FW_MV_ETH_L2FW_H

#include "mvOs.h"
#include "mv_pp2/net_dev/mv_netdev.h"

#define	L2FW_HASH_SIZE   (1 << 17)
#define	L2FW_HASH_MASK   (L2FW_HASH_SIZE - 1)

extern struct aggr_tx_queue *aggr_txqs;

/* L2fw defines */

#define CMD_L2FW_AS_IS				0
#define CMD_L2FW_SWAP_MAC			1
#define CMD_L2FW_COPY_SWAP			2
#define CMD_L2FW_CESA				3
#define CMD_L2FW_LAST				4

#define XOR_CAUSE_DONE_MASK(chan) ((BIT0|BIT1) << (chan * 16))
#define XOR_THRESHOLD_DEF			2000;

struct eth_port_l2fw {
	int cmd;
	int lookupEn;
	int xorThreshold;
	int txPort;
	/* stats */
	int statErr;
	int statDrop;
};

struct l2fw_rule {
	MV_U32 srcIP;
	MV_U32 dstIP;
	MV_U8 port;
	struct l2fw_rule *next;
};

int mv_l2fw_add(MV_U32 srcIP, MV_U32 dstIP, int port);
int mv_l2fw_set(int port, bool l2fw);
int mv_l2fw_port(int rx_port, int tx_port, int cmd);
void mv_l2fw_xor(int rx_port, int threshold);
void mv_l2fw_lookupEn(int rx_port, int enable);
void mv_l2fw_flush(void);
void mv_l2fw_rules_dump(void);
void mv_l2fw_ports_dump(void);
void mv_l2fw_stats(void);

#endif
