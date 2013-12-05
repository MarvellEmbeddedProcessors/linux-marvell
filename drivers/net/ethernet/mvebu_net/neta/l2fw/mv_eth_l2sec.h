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

#ifndef L2SEC_MV_ETH_L2SEC_H
#define L2SEC_MV_ETH_L2SEC_H
#include "cesa/mvCesa.h"

extern u32 mv_crypto_virt_base_get(void);

/* IPSec defines */
#define MV_L2FW_SEC_MAX_PACKET		1540
#define MV_L2FW_SEC_ENC_BLOCK_SIZE	16
#define MV_L2FW_SEC_ESP_OFFSET		34

#define L2SEC_CESA_BUF_NUM	1	/* CESA_DEF_BUF_NUM */
#define L2SEC_CESA_BUF_SIZE  1500	/* CESA_DEF_BUF_SIZE */


/* IPSec Enumerators */
typedef enum {
	MV_L2FW_SEC_TUNNEL = 0,
	MV_L2FW_SEC_TRANSPORT,
} MV_L2FW_SEC_PROT;

typedef enum {
	MV_L2FW_SEC_ESP = 0,
	MV_L2FW_SEC_AH,
} MV_L2FW_SEC_ENCAP;


typedef enum {
	MV_L2FW_SEC_ENCRYPT = 0,
	MV_L2FW_SEC_DECRYPT,
} MV_L2FW_SEC_OP;

struct mv_l2fw_sa_stats {
	MV_U32 encrypt;
	MV_U32 decrypt;
	MV_U32 rejected;	/* slow path */
	MV_U32 dropped;		/* packet drop */
	MV_U32 bytes;
} MV_L2FW_SA_STATS;

/* IPSec Structures */
struct mv_l2fw_sec_tunnel_hdr {
	MV_U32 sIp;			/* BE */
	MV_U32 dIp;			/* BE */
	/* dstMac should be 2 byte aligned */
	MV_U8 dstMac[MV_MAC_ADDR_SIZE];	/* BE */
	MV_U8 srcMac[MV_MAC_ADDR_SIZE];	/* BE */
	MV_U8 outIfIndex;
} MV_L2FW_SEC_TUNNEL_HDR;

struct mv_l2fw_sec_sa_entry {
	MV_U32 spi;			/* BE */
	MV_L2FW_SEC_PROT tunProt;
	MV_L2FW_SEC_ENCAP encap;
	MV_U16 sid;
	MV_U32 seqNum;			/* LE  */
	struct mv_l2fw_sec_tunnel_hdr tunnelHdr;
	MV_U32 lifeTime;
	MV_U8 ivSize;
	MV_U8 cipherBlockSize;
	MV_U8 digestSize;
	MV_L2FW_SEC_OP secOp;
	struct mv_l2fw_sa_stats stats;
} MV_L2FW_SEC_SA_ENTRY;


#define CESA_0    0
#define CESA_1    1

/* define number of channels */
#ifdef CONFIG_ARMADA_XP
#define CESA_CHAN 2
#else
#define CESA_CHAN 1
#endif


#define MV_L2FW_SEC_REQ_Q_SIZE   1000
#define CESA_DEF_REQ_SIZE       (256*4)

struct mv_l2fw_sec_cesa_priv {
	struct mv_l2fw_sec_sa_entry *pSaEntry;
	MV_BUF_INFO *pBufInfo;
	MV_U8 orgDigest[MV_CESA_MAX_DIGEST_SIZE];
	MV_CESA_COMMAND *pCesaCmd;
	struct eth_pbuf *pPkt;
	int ifout;
	int ownerId;
	int inPort;
} MV_L2FW_SEC_CESA_PRIV;

MV_STATUS mv_l2sec_handle_esp(struct eth_pbuf *pkt, struct neta_rx_desc *rx_desc, struct eth_port  *new_pp, int inPort);
int mv_l2sec_cesa_init(void);
void mv_l2sec_stats(void);
int mv_l2sec_set_cesa_chan(int port, int cesaChan);
#endif /*L2SEC_MV_ETH_L2SEC_H*/
