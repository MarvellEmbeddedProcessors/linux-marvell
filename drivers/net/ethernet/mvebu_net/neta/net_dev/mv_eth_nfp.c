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

#include "mvCommon.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/inetdevice.h>
#include <linux/mv_nfp.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "eth-phy/mvEthPhy.h"
#include "mvSysEthPhyApi.h"
#include "mvSysNetaApi.h"

#include "gbe/mvNeta.h"
#include "bm/mvBm.h"
#include "pnc/mvPnc.h"
#include "pnc/mvTcam.h"
#include "pmt/mvPmt.h"

#include "mv_switch.h"
#include "mv_netdev.h"
#include "mv_eth_tool.h"
#include "cpu/mvCpuCntrs.h"


#ifdef CONFIG_MV_ETH_NFP_EXT
int                mv_ctrl_nfp_ext_port[NFP_EXT_NUM];
int                mv_ctrl_nfp_ext_en[NFP_EXT_NUM];
struct net_device *mv_ctrl_nfp_ext_netdev[NFP_EXT_NUM];

static void mv_eth_nfp_ext_skb_destructor(struct sk_buff *skb)
{
	consume_skb(skb_shinfo(skb)->destructor_arg);
}

static int mv_eth_nfp_ext_tx(struct eth_port *pp, struct eth_pbuf *pkt, MV_NFP_RESULT *res);
#endif /* CONFIG_MV_ETH_NFP_EXT */

static INLINE int mv_eth_nfp_need_fragment(MV_NFP_RESULT *res)
{
	if (res->flags & MV_NFP_RES_IP_INFO_VALID)
		return (res->ipInfo.ipLen > res->mtu);

	return 0;
}

/* Enable NFP */
int mv_eth_nfp_ctrl(struct net_device *dev, int en)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);

	if (pp == NULL)
		return 1;

	if (en) {
		pp->flags |= MV_ETH_F_NFP_EN;
		printk(KERN_INFO "%s: NFP enabled\n", dev->name);
	} else {
		pp->flags &= ~MV_ETH_F_NFP_EN;
		printk(KERN_INFO "%s: NFP disabled\n", dev->name);
	}
	return 0;
}
EXPORT_SYMBOL(mv_eth_nfp_ctrl);

#ifdef CONFIG_MV_ETH_NFP_EXT
int mv_eth_nfp_ext_add(struct net_device *dev, int port)
{
	int i;

	/* find free place in mv_ctrl_nfp_ext_netdev */
	for (i = 0; i < NFP_EXT_NUM; i++) {
		if (mv_ctrl_nfp_ext_netdev[i] == NULL) {
			mv_ctrl_nfp_ext_netdev[i] = dev;
			mv_ctrl_nfp_ext_port[i] = port;
			mv_ctrl_nfp_ext_en[i] = 0;
			return 0;
		}
	}
	printk(KERN_INFO "External interface %s can't be bound to NFP\n", dev->name);
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_add);

int mv_eth_nfp_ext_del(struct net_device *dev)
{
	int i;

	/* find free place in mv_ctrl_nfp_ext_netdev */
	for (i = 0; i < NFP_EXT_NUM; i++) {
		if (mv_ctrl_nfp_ext_netdev[i] == dev) {
			mv_ctrl_nfp_ext_netdev[i] = NULL;
			return 0;
		}
	}
	printk(KERN_INFO "External interface %s is not bound to NFP\n", dev->name);
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_del);

int mv_eth_nfp_ext_ctrl(struct net_device *dev, int en)
{
	int i;

	/* find net_device in mv_ctrl_nfp_ext_netdev */
	for (i = 0; i < NFP_EXT_NUM; i++) {
		if (mv_ctrl_nfp_ext_netdev[i] == dev) {
			if (en)
				printk(KERN_INFO "%s: NFP enabled for external interface\n", dev->name);
			 else
				printk(KERN_INFO "%s: NFP disabled for external interface\n", dev->name);

			mv_ctrl_nfp_ext_en[i] = en;
			return 0;
		}
	}
	printk(KERN_INFO "External interface %s is not bind to NFP\n", dev->name);
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_ctrl);
#else
int mv_eth_nfp_ext_add(struct net_device *dev, int port)
{
	printk(KERN_INFO "NFP doesn't support external interfaces\n");
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_add);

int mv_eth_nfp_ext_del(struct net_device *dev)
{
	printk(KERN_INFO "NFP doesn't support external interfaces\n");
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_del);

int mv_eth_nfp_ext_ctrl(struct net_device *dev, int en)
{
	printk(KERN_INFO "NFP doesn't support external interfaces\n");
	return 1;
}
EXPORT_SYMBOL(mv_eth_nfp_ext_ctrl);
#endif /* CONFIG_MV_ETH_NFP_EXT */


static inline int mv_eth_frag_build_hdr_desc(struct eth_port *priv, struct tx_queue *txq_ctrl,
					MV_U8 *pktData, int mac_hdr_len, int ip_hdr_len,
					     int frag_size, int left_len, int frag_offset)
{
	struct neta_tx_desc *tx_desc;
	struct iphdr        *iph;
	MV_U8               *data;
	int                 align;
	MV_U16              frag_ctrl;

	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
	if (tx_desc == NULL)
		return -1;

	txq_ctrl->txq_count++;

	data = mv_eth_extra_pool_get(priv);
	if (data == NULL)
		return -1;

	tx_desc->command = mvNetaTxqDescCsum(mac_hdr_len, MV_16BIT_BE(MV_IP_TYPE), ip_hdr_len, 0);
	tx_desc->command |= NETA_TX_F_DESC_MASK;
	tx_desc->dataSize = mac_hdr_len + ip_hdr_len;

	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((MV_ULONG)data | MV_ETH_SHADOW_EXT);
	mv_eth_shadow_inc_put(txq_ctrl);

	/* Check for IP header alignment */
	align = 4 - (mac_hdr_len & 3);
	data += align;
	memcpy(data, pktData, mac_hdr_len + ip_hdr_len);

	iph = (struct iphdr *)(data + mac_hdr_len);

	iph->tot_len = htons(frag_size + ip_hdr_len);

	/* update frag_offset and MF flag in IP header - packet can be already fragmented */
	frag_ctrl = ntohs(iph->frag_off);
	frag_offset += ((frag_ctrl & IP_OFFSET) << 3);
	frag_ctrl &= ~IP_OFFSET;
	frag_ctrl |= ((frag_offset >> 3) & IP_OFFSET);

	if (((frag_ctrl & IP_MF) == 0) && (left_len != frag_size))
		frag_ctrl |= IP_MF;

	iph->frag_off = htons(frag_ctrl);

	/* if it was PPPoE, update the PPPoE payload fields  */
	if ((*((char *)iph - MV_PPPOE_HDR_SIZE - 1) == 0x64) &&
		(*((char *)iph - MV_PPPOE_HDR_SIZE - 2) == 0x88)) {
		PPPoE_HEADER *pPPPNew = (PPPoE_HEADER *)((char *)iph - MV_PPPOE_HDR_SIZE);
		pPPPNew->len = htons(frag_size + ip_hdr_len + MV_PPP_HDR_SIZE);
	}
	tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, data, tx_desc->dataSize);
	mv_eth_tx_desc_flush(tx_desc);

	return 0;
}

static inline int mv_eth_frag_build_data_desc(struct tx_queue *txq_ctrl, MV_U8 *frag_ptr, int frag_size,
						int data_left, struct eth_pbuf *pkt)
{
	struct neta_tx_desc *tx_desc;

	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
	if (tx_desc == NULL)
		return -1;

	txq_ctrl->txq_count++;
	tx_desc->dataSize = frag_size;
	tx_desc->bufPhysAddr = pkt->physAddr + (frag_ptr - pkt->pBuf);
	tx_desc->command = (NETA_TX_L_DESC_MASK | NETA_TX_Z_PAD_MASK);

	if (frag_size == data_left)
		txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) pkt;
	else
		txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = 0;

	mv_eth_shadow_inc_put(txq_ctrl);
	mv_eth_tx_desc_flush(tx_desc);

	return 0;
}

static int mv_eth_nfp_fragment_tx(struct eth_port *pp, struct net_device *dev, MV_NFP_RESULT *res,
					   struct tx_queue *txq_ctrl, struct eth_pbuf *pkt)
{
	MV_IP_HEADER_INFO *pIpInfo = &res->ipInfo;
	int   pkt_offset = (pkt->offset + res->shift);
	int   ip_offset = (pIpInfo->ipOffset - res->shift);
	int   frag_size = MV_ALIGN_DOWN((res->mtu - res->ipInfo.ipHdrLen), 8);
	int   data_left = pIpInfo->ipLen - res->ipInfo.ipHdrLen;
	int   pktNum = (data_left / frag_size) + ((data_left % frag_size) ? 1 : 0);
	MV_U8 *pData = pkt->pBuf + pkt_offset;
	MV_U8 *payloadStart = pData + ip_offset + pIpInfo->ipHdrLen;
	MV_U8 *frag_ptr = payloadStart;
	int   i, total_bytes = 0;
	int   save_txq_count = txq_ctrl->txq_count;

	if ((txq_ctrl->txq_count + (pktNum * 2)) >= txq_ctrl->txq_size) {
		/*
		printk(KERN_ERR "%s: no TX descriptors - txq_count=%d, len=%d, frag_size=%d\n",
					__func__, txq_ctrl->txq_count, data_left, frag_size);
		*/
		STAT_ERR(txq_ctrl->stats.txq_err++);
		goto outNoTxDesc;
	}

	for (i = 0; i < pktNum; i++) {

		if (mv_eth_frag_build_hdr_desc(pp, txq_ctrl, pData, ip_offset, pIpInfo->ipHdrLen,
					frag_size, data_left, frag_ptr - payloadStart))
			goto outNoTxDesc;

		total_bytes += (ip_offset + pIpInfo->ipHdrLen);

		if (mv_eth_frag_build_data_desc(txq_ctrl, frag_ptr, frag_size, data_left, pkt))
			goto outNoTxDesc;

		total_bytes += frag_size;
		frag_ptr += frag_size;
		data_left -= frag_size;
		frag_size = MV_MIN(frag_size, data_left);
	}
	/* Flush + Invalidate cache for MAC + IP header + L4 header */
	pData = pkt->pBuf + pkt->offset;
	if (res->shift < 0)
		pData += res->shift;

	mvOsCacheMultiLineFlushInv(NULL, pData, (res->pWrite - pData));

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		mvNetaPonTxqBytesAdd(pp->port, txq_ctrl->txp, txq_ctrl->txq, total_bytes);
#endif /* CONFIG_MV_PON */

	dev->stats.tx_packets += pktNum;
	dev->stats.tx_bytes += total_bytes;
	STAT_DBG(txq_ctrl->stats.txq_tx += (pktNum * 2));

	mvNetaTxqPendDescAdd(pp->port, txq_ctrl->txp, txq_ctrl->txq, pktNum * 2);

	return pktNum * 2;

outNoTxDesc:
	while (save_txq_count < txq_ctrl->txq_count) {
		txq_ctrl->txq_count--;
		mv_eth_shadow_dec_put(txq_ctrl);
		mvNetaTxqPrevDescGet(txq_ctrl->q);
	}
	/* Invalidate cache for MAC + IP header + L4 header */
	pData = pkt->pBuf + pkt->offset;
	if (res->shift < 0)
		pData += res->shift;

	mvOsCacheMultiLineInv(NULL, pData, (res->pWrite - pData));

	return 0;
}


static MV_STATUS mv_eth_nfp_tx(struct eth_pbuf *pkt, MV_NFP_RESULT *res)
{
	struct net_device *dev = (struct net_device *)res->dev;
	struct eth_port *pp = MV_ETH_PRIV(dev);
	struct neta_tx_desc *tx_desc;
	u32 tx_cmd, physAddr;
	MV_STATUS status = MV_OK;
	struct tx_queue *txq_ctrl;
	int use_bm, pkt_offset, frags = 1;

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		STAT_INFO(pp->stats.netdev_stop++);
#ifdef CONFIG_MV_NETA_DEBUG_CODE
		if (pp->flags & MV_ETH_F_DBG_TX)
			printk(KERN_ERR "%s: STARTED_BIT = 0 , packet is dropped.\n", __func__);
#endif /* CONFIG_MV_NETA_DEBUG_CODE */
		return MV_DROPPED;
	}

	/* Get TxQ to send packet */
	/* Check TXQ classification */
	if ((res->flags & MV_NFP_RES_TXQ_VALID) == 0)
		res->txq = pp->cpu_config[smp_processor_id()]->txq;

	if ((res->flags & MV_NFP_RES_TXP_VALID) == 0)
		res->txp = pp->txp;

	txq_ctrl = &pp->txq_ctrl[res->txp * CONFIG_MV_ETH_TXQ + res->txq];

	if (txq_ctrl->flags & MV_ETH_F_TX_SHARED)
		spin_lock(&txq_ctrl->queue_lock);

	/* Do fragmentation if needed */
	if (mv_eth_nfp_need_fragment(res)) {
		frags = mv_eth_nfp_fragment_tx(pp, dev, res, txq_ctrl, pkt);
		if (frags == 0) {
			dev->stats.tx_dropped++;
			status = MV_DROPPED;
		}
		STAT_INFO(pp->stats.tx_fragment++);
		goto out;
	}

	/* Get next descriptor for tx, single buffer, so FIRST & LAST */
	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
	if (tx_desc == NULL) {

		/* No resources: Drop */
		dev->stats.tx_dropped++;
		status = MV_DROPPED;
		goto out;
	}

	if (res->flags & MV_NFP_RES_L4_CSUM_NEEDED) {
		MV_U8 *pData = pkt->pBuf + pkt->offset;

		if (res->shift < 0)
			pData += res->shift;

		mvOsCacheMultiLineFlushInv(NULL, pData, (res->pWrite - pData));
	}

	txq_ctrl->txq_count++;

	/* tx_cmd - word accumulated by NFP processing */
	tx_cmd = res->tx_cmd;

	if (res->flags & MV_NFP_RES_IP_INFO_VALID) {
		if (res->ipInfo.family == MV_INET) {
			tx_cmd |= NETA_TX_L3_IP4 | NETA_TX_IP_CSUM_MASK |
				((res->ipInfo.ipOffset - res->shift) << NETA_TX_L3_OFFSET_OFFS) |
				((res->ipInfo.ipHdrLen >> 2) << NETA_TX_IP_HLEN_OFFS);
		} else {
			tx_cmd |= NETA_TX_L3_IP6 |
				((res->ipInfo.ipOffset - res->shift) << NETA_TX_L3_OFFSET_OFFS) |
				((res->ipInfo.ipHdrLen >> 2) << NETA_TX_IP_HLEN_OFFS);
		}
	}

#ifdef CONFIG_MV_ETH_BM_CPU
	use_bm = 1;
#else
	use_bm = 0;
#endif /* CONFIG_MV_ETH_BM_CPU */

	pkt_offset = pkt->offset + res->shift;
	physAddr = pkt->physAddr;
	if (pkt_offset > NETA_TX_PKT_OFFSET_MAX) {
		use_bm = 0;
		physAddr += pkt_offset;
		pkt_offset = 0;
	}

	if ((pkt->pool >= 0) && (pkt->pool < MV_ETH_BM_POOLS)) {
		if (use_bm) {
			tx_cmd |= NETA_TX_BM_ENABLE_MASK | NETA_TX_BM_POOL_ID_MASK(pkt->pool);
			txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) NULL;
		} else
			txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) pkt;
	} else {
		/* skb from external interface */
		txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = ((u32)pkt->osInfo | MV_ETH_SHADOW_SKB);
	}

	mv_eth_shadow_inc_put(txq_ctrl);

	tx_cmd |= NETA_TX_PKT_OFFSET_MASK(pkt_offset);

	tx_desc->command = tx_cmd | NETA_TX_FLZ_DESC_MASK;
	tx_desc->dataSize = pkt->bytes;
	tx_desc->bufPhysAddr = physAddr;

	/* FIXME: PON only? --BK */
	tx_desc->hw_cmd = pp->hw_cmd;

#ifdef CONFIG_MV_NETA_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_TX) {
		printk(KERN_ERR "%s - nfp_tx_%lu: port=%d, txp=%d, txq=%d\n",
		       dev->name, dev->stats.tx_packets, pp->port, res->txp, res->txq);
		mv_eth_tx_desc_print(tx_desc);
		mv_eth_pkt_print(pkt);
	}
#endif /* CONFIG_MV_NETA_DEBUG_CODE */

	mv_eth_tx_desc_flush(tx_desc);

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		mvNetaPonTxqBytesAdd(pp->port, res->txp, res->txq, pkt->bytes);
#endif /* CONFIG_MV_PON */

	/* Enable transmit by update PENDING counter */
	mvNetaTxqPendDescAdd(pp->port, res->txp, res->txq, 1);

	/* FIXME: stats includes MH --BK */
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += pkt->bytes;
	STAT_DBG(txq_ctrl->stats.txq_tx++);

out:
#ifndef CONFIG_MV_ETH_TXDONE_ISR
	if (txq_ctrl->txq_count >= mv_ctrl_txdone) {
		u32 tx_done = mv_eth_txq_done(pp, txq_ctrl);

		STAT_DIST((tx_done < pp->dist_stats.tx_done_dist_size) ? pp->dist_stats.tx_done_dist[tx_done]++ : 0);
	}
	/* If after calling mv_eth_txq_done, txq_ctrl->txq_count equals frags, we need to set the timer */
	if ((txq_ctrl->txq_count == frags) && (frags > 0))
		mv_eth_add_tx_done_timer(pp->cpu_config[smp_processor_id()]);

#endif /* CONFIG_MV_ETH_TXDONE_ISR */

	if (txq_ctrl->flags & MV_ETH_F_TX_SHARED)
		spin_unlock(&txq_ctrl->queue_lock);

	return status;
}

/* Main NFP function returns the following error codes:
 *  MV_OK - packet processed and sent successfully by NFP
 *  MV_TERMINATE - packet can't be processed by NFP - pass to Linux processing
 *  MV_DROPPED - packet processed by NFP, but not sent (dropped)
 */
MV_STATUS mv_eth_nfp(struct eth_port *pp, int rxq, struct neta_rx_desc *rx_desc,
				struct eth_pbuf *pkt, struct bm_pool *pool)
{
	MV_STATUS       status;
	MV_NFP_RESULT   res;
	bool            tx_external = false;

#ifdef CONFIG_MV_NETA_DEBUG_CODE
	if (pp->flags & MV_ETH_F_DBG_RX) {
		mv_eth_rx_desc_print(rx_desc);
		mv_eth_pkt_print(pkt);
	}
#endif /* CONFIG_MV_NETA_DEBUG_CODE */

	status = nfp_core_p->nfp_rx(pp->port, rx_desc, pkt, &res);
	tx_external = (res.flags & MV_NFP_RES_NETDEV_EXT);

	if (status == MV_OK) {

		if (res.flags & MV_NFP_RES_L4_CSUM_NEEDED) {
			MV_IP_HEADER_INFO *pIpInfo = &res.ipInfo;
			MV_U8 *pIpHdr = pIpInfo->ip_hdr.l3;

			if (pIpInfo->ipProto == MV_IP_PROTO_TCP) {
				MV_TCP_HEADER *pTcpHdr = (MV_TCP_HEADER *) ((char *)pIpHdr + pIpInfo->ipHdrLen);

				pTcpHdr->chksum = csum_fold(csum_partial((char *)res.diffL4, sizeof(res.diffL4),
									~csum_unfold(pTcpHdr->chksum)));
				res.pWrite = (MV_U8 *)pTcpHdr + sizeof(MV_TCP_HEADER);
			} else {
				MV_UDP_HEADER *pUdpHdr = (MV_UDP_HEADER *) ((char *)pIpHdr + pIpInfo->ipHdrLen);

				pUdpHdr->check = csum_fold(csum_partial((char *)res.diffL4, sizeof(res.diffL4),
									~csum_unfold(pUdpHdr->check)));
				res.pWrite = (MV_U8 *)pUdpHdr + sizeof(MV_UDP_HEADER);
			}
		}

#ifdef CONFIG_MV_ETH_NFP_EXT
		if  (tx_external) {
			/* INT RX -> EXT TX */
			mv_eth_nfp_ext_tx(pp, pkt, &res);
			status = MV_OK;
		} else
#endif /* CONFIG_MV_ETH_NFP_EXT */
			/* INT RX -> INT TX */
			status = mv_eth_nfp_tx(pkt, &res);
	}
	if (status == MV_OK) {
		STAT_DBG(pp->stats.rx_nfp++);

		/* Packet transmited - refill now */
		if (!tx_external && mv_eth_pool_bm(pool)) {
			/* BM - no refill */
			mvOsCacheLineInv(NULL, rx_desc);
			return MV_OK;
		}

		if (!tx_external || mv_eth_is_recycle())
			pkt = NULL;

		if (mv_eth_refill(pp, rxq, pkt, pool, rx_desc)) {
			printk(KERN_ERR "Linux processing - Can't refill\n");
			pp->rxq_ctrl[rxq].missed++;
			mv_eth_add_cleanup_timer(pp->cpu_config[smp_processor_id()]);
			return MV_FAIL;
		}
		return MV_OK;
	}
	if (status == MV_DROPPED) {
		/* Refill the same buffer */
		STAT_DBG(pp->stats.rx_nfp_drop++);
		mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
		return MV_OK;
	}
	return status;
}

#ifdef CONFIG_MV_ETH_NFP_EXT
static int mv_eth_nfp_ext_tx_fragment(struct net_device *dev, struct sk_buff *skb, MV_NFP_RESULT *res)
{
	unsigned int      dlen, doff, error, flen, fsize, l, max_dlen, max_plen;
	unsigned int      hdrlen, offset;
	struct iphdr      *ip, *nip;
	struct sk_buff    *new;
	struct page       *page;
	int               mac_header_len;
	MV_IP_HEADER_INFO *pIpInfo = &res->ipInfo;

	max_plen = dev->mtu + dev->hard_header_len;

	SKB_LINEAR_ASSERT(skb);

	mac_header_len = (pIpInfo->ipOffset - res->shift);
	ip = (struct iphdr *)(skb->data + mac_header_len);

	hdrlen = mac_header_len + res->ipInfo.ipHdrLen;

	doff = hdrlen;
	dlen = skb_headlen(skb) - hdrlen;
	offset = ntohs(ip->frag_off) & IP_OFFSET;
	max_dlen = (max_plen - hdrlen) & ~0x07;

	do {
		new = dev_alloc_skb(hdrlen);
		if (!new)
			break;

		/* Setup new packet metadata */
		new->protocol = IPPROTO_IP;
		new->ip_summed = CHECKSUM_PARTIAL;
		skb_set_network_header(new, mac_header_len);

		/* Copy original IP header */
		memcpy(skb_put(new, hdrlen), skb->data, hdrlen);

		/* Append data portion */
		fsize = flen = min(max_dlen, dlen);

		skb_get(skb);
		skb_shinfo(new)->destructor_arg = skb;
		new->destructor = mv_eth_nfp_ext_skb_destructor;

		while (fsize) {
			l = PAGE_SIZE - ((unsigned long)(skb->data + doff) & ~PAGE_MASK);
			if (l > fsize)
				l = fsize;

			page = virt_to_page(skb->data + doff);
			get_page(page);
			skb_add_rx_frag(new, skb_shinfo(new)->nr_frags, page,
					(unsigned long)(skb->data + doff) &
								~PAGE_MASK, l);
			dlen -= l;
			doff += l;
			fsize -= l;
		}

		/* Fixup IP header */
		nip = ip_hdr(new);
		nip->tot_len = htons((4 * ip->ihl) + flen);
		nip->frag_off = htons(offset |
				(dlen ? IP_MF : (IP_MF & ntohs(ip->frag_off))));

		/* if it was PPPoE, update the PPPoE payload fields
		adapted from  mv_eth_frag_build_hdr_desc */
		if ((*((char *)nip - MV_PPPOE_HDR_SIZE - 1) == 0x64) &&
			(*((char *)nip - MV_PPPOE_HDR_SIZE - 2) == 0x88)) {
			PPPoE_HEADER *pPPPNew = (PPPoE_HEADER *)((char *)nip - MV_PPPOE_HDR_SIZE);
			pPPPNew->len = htons(flen + 4*ip->ihl + MV_PPP_HDR_SIZE);
	    }

		offset += flen / 8;

		/* Recalculate IP checksum */
		new->ip_summed = CHECKSUM_NONE;
		nip->check = 0;
		nip->check = ip_fast_csum(nip, nip->ihl);

		/* TX packet */
		error = dev->netdev_ops->ndo_start_xmit(new, dev);
		if (error)
			break;
	} while (dlen);

	if (!new)
		return -ENOMEM;

	if (error) {
		consume_skb(new);
		return error;
	}

	/* We are no longer use original skb */
	consume_skb(skb);
	return 0;
}

static int mv_eth_nfp_ext_tx(struct eth_port *pp, struct eth_pbuf *pkt, MV_NFP_RESULT *res)
{
	struct sk_buff *skb;
	struct net_device *dev = (struct net_device *)res->dev;

	/* prepare SKB for transmit */
	skb = (struct sk_buff *)(pkt->osInfo);

	skb->data += res->shift;
	skb->tail = skb->data + pkt->bytes ;
	skb->len = pkt->bytes;

	skb_reset_mac_header(skb);
	skb_reset_network_header(skb);

	if (res->flags & MV_NFP_RES_IP_INFO_VALID) {

		if (res->ipInfo.family == MV_INET) {
			struct iphdr *iph = (struct iphdr *)res->ipInfo.ip_hdr.ip4;

			if (mv_eth_nfp_need_fragment(res))
				return mv_eth_nfp_ext_tx_fragment(dev, skb, res);

			/* Recalculate IP checksum for IPv4 if necessary */
			skb->ip_summed = CHECKSUM_NONE;
			iph->check = 0;
			iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);
		}
		skb_set_network_header(skb, res->ipInfo.ipOffset - res->shift);
	}

	if (pp) {
		/* ingress port is GBE */
#ifdef ETH_SKB_DEBUG
		mv_eth_skb_check(skb);
#endif /* ETH_SKB_DEBUG */

#ifdef CONFIG_MV_NETA_SKB_RECYCLE
		if (mv_eth_is_recycle()) {
			skb->skb_recycle = mv_eth_skb_recycle;
			skb->hw_cookie = (__u32)pkt;
		}
#endif /* CONFIG_MV_NETA_SKB_RECYCLE */
	}
	return dev->netdev_ops->ndo_start_xmit(skb, dev);
}


static MV_STATUS mv_eth_nfp_ext_rxd_from_info(MV_EXT_PKT_INFO *pktInfo, struct neta_rx_desc *rxd)
{
	if (pktInfo->flags & MV_EXT_VLAN_EXIST_MASK)
		NETA_RX_SET_VLAN(rxd);

	if (pktInfo->flags & MV_EXT_PPP_EXIST_MASK)
		NETA_RX_SET_PPPOE(rxd);

	if (pktInfo->l3_type == ETH_P_IP)
		NETA_RX_L3_SET_IP4(rxd);
	else if (pktInfo->l3_type == ETH_P_IPV6)
		NETA_RX_L3_SET_IP6(rxd);
	else {
		NETA_RX_L3_SET_UN(rxd);
		return MV_OK;
	}

	if (pktInfo->flags & MV_EXT_IP_FRAG_MASK)
		NETA_RX_IP_SET_FRAG(rxd);


	if (!pktInfo->l3_offset || !pktInfo->l3_hdrlen)
		return -1;

	NETA_RX_SET_IPHDR_OFFSET(rxd, pktInfo->l3_offset + MV_ETH_MH_SIZE);
	NETA_RX_SET_IPHDR_HDRLEN(rxd, (pktInfo->l3_hdrlen >> 2));

	if ((pktInfo->flags & MV_EXT_L3_VALID_MASK) == 0) {
		NETA_RX_L3_SET_IP4_ERR(rxd);
		return MV_OK;
	}

	switch (pktInfo->l4_proto) {
	case IPPROTO_TCP:
		NETA_RX_L4_SET_TCP(rxd);
		break;

	case IPPROTO_UDP:
		NETA_RX_L4_SET_UDP(rxd);
		break;

	default:
		NETA_RX_L4_SET_OTHER(rxd);
		break;
	}

	if (pktInfo->flags & MV_EXT_L4_VALID_MASK)
		NETA_RX_L4_CSUM_SET_OK(rxd);

	return MV_OK;
}


static MV_STATUS mv_eth_nfp_ext_rxd_from_ipv4(int ofs, struct iphdr *iph, struct sk_buff *skb, struct neta_rx_desc *rxd)
{
	int l4_proto = 0;
	int hdrlen;
	int tmp;

	NETA_RX_L3_SET_IP4(rxd);
	hdrlen = iph->ihl << 2;
	NETA_RX_SET_IPHDR_HDRLEN(rxd, iph->ihl);

	if (ip_fast_csum((unsigned char *)iph, iph->ihl)) {
		NETA_RX_L3_SET_IP4_ERR(rxd);
		return MV_OK;
	}

	switch ((l4_proto = iph->protocol)) {
	case IPPROTO_TCP:
		NETA_RX_L4_SET_TCP(rxd);
		break;
	case IPPROTO_UDP:
		NETA_RX_L4_SET_UDP(rxd);
		break;
	default:
		NETA_RX_L4_SET_OTHER(rxd);
		l4_proto = 0;
		break;
	}

	tmp = ntohs(iph->frag_off);
	if ((tmp & IP_MF) != 0 || (tmp & IP_OFFSET) != 0) {
		NETA_RX_IP_SET_FRAG(rxd);
		return MV_OK; /* cannot checksum fragmented */
	}

	if (!l4_proto)
		return MV_OK; /* can't proceed without l4_proto in {UDP, TCP} */

	if (skb->ip_summed == CHECKSUM_UNNECESSARY) {
		NETA_RX_L4_CSUM_SET_OK(rxd);
		return MV_OK;
	}

	if (l4_proto == IPPROTO_UDP) {
		struct udphdr *uh = (struct udphdr *)((char *)iph + hdrlen);

		if (uh->check == 0)
			NETA_RX_L4_CSUM_SET_OK(rxd);
	}

	/* Complete checksum with pseudo header */
	if (skb->ip_summed == CHECKSUM_COMPLETE) {
		if (!csum_tcpudp_magic(iph->saddr, iph->daddr, skb->len - hdrlen - ofs,
			       l4_proto, skb->csum)) {
			NETA_RX_L4_CSUM_SET_OK(rxd);
			return MV_OK;
		}
	}

	return MV_OK;
}

static MV_STATUS mv_eth_nfp_ext_rxd_from_ipv6(int ofs, struct sk_buff *skb, struct neta_rx_desc *rxd)
{
	struct ipv6hdr *ip6h;
	int l4_proto = 0;
	int hdrlen;
	__u8 nexthdr;

	NETA_RX_L3_SET_IP6(rxd);

	hdrlen = sizeof(struct ipv6hdr);
	NETA_RX_SET_IPHDR_HDRLEN(rxd, (hdrlen >> 2));

	ip6h = (struct ipv6hdr *)(skb->data + ofs);

	nexthdr = ip6h->nexthdr;

	/* No support for extension headers. Only TCP or UDP */
	if (nexthdr == NEXTHDR_TCP) {
		l4_proto = IPPROTO_TCP;
		NETA_RX_L4_SET_TCP(rxd);
	} else if (nexthdr == NEXTHDR_UDP) {
		l4_proto = IPPROTO_UDP;
		NETA_RX_L4_SET_UDP(rxd);
	} else {
		NETA_RX_L4_SET_OTHER(rxd);
		return MV_OK;
	}

	if (skb->ip_summed == CHECKSUM_COMPLETE) {
		if (!csum_ipv6_magic(&ip6h->saddr, &ip6h->daddr, skb->len,
				      l4_proto , skb->csum)) {
			NETA_RX_L4_CSUM_SET_OK(rxd);
			return MV_OK;
		}
	}
	return MV_OK;
}


static MV_STATUS mv_eth_nfp_ext_rxd_build(struct sk_buff *skb, MV_EXT_PKT_INFO *pktInfo, struct neta_rx_desc *rxd)
{
	struct iphdr *iph;
	int l3_proto = 0;
	int ofs = 0;
	MV_U16 tmp;

	rxd->status = 0;
	rxd->pncInfo = 0;

	if (pktInfo)
		return mv_eth_nfp_ext_rxd_from_info(pktInfo, rxd);

	tmp = ntohs(skb->protocol);

 ll:
	switch (tmp) {
	case ETH_P_IP:
	case ETH_P_IPV6:
		l3_proto = tmp;
		break;

	case ETH_P_PPP_SES:
		NETA_RX_SET_PPPOE(rxd);
		ofs += MV_PPPOE_HDR_SIZE;
		switch (tmp = ntohs(*((MV_U16 *)&skb->data[ofs - 2]))) {
		case 0x0021:
			l3_proto = ETH_P_IP;
			break;
		case 0x0057:
			l3_proto = ETH_P_IPV6;
			break;
		default:
			goto non_ip;
		}
		break;

	case ETH_P_8021Q:
		/* Don't support double VLAN for now */
		if (NETA_RX_IS_VLAN(rxd))
			goto non_ip;

		NETA_RX_SET_VLAN(rxd);
		ofs = MV_VLAN_HLEN;

		tmp = ntohs(*((MV_U16 *)&skb->data[2]));
			goto ll;

	default:
	  goto non_ip;
	}

#ifndef CONFIG_MV_ETH_PNC
	rxd->status |= ETH_RX_NOT_LLC_SNAP_FORMAT_MASK;
#endif /* CONFIG_MV_ETH_PNC */

	NETA_RX_SET_IPHDR_OFFSET(rxd, ETH_HLEN + MV_ETH_MH_SIZE + ofs);

	iph = (struct iphdr *)(skb->data + ofs);

	if (l3_proto == ETH_P_IP)
		return mv_eth_nfp_ext_rxd_from_ipv4(ofs, iph, skb, rxd);

	return mv_eth_nfp_ext_rxd_from_ipv6(ofs, skb, rxd);

non_ip:
	 NETA_RX_L3_SET_UN(rxd);
	 return MV_OK;
}

void mv_eth_nfp_ext_pkt_info_print(MV_EXT_PKT_INFO *pktInfo)
{
	if (pktInfo == NULL)
		return;

	if (pktInfo->flags & MV_EXT_VLAN_EXIST_MASK)
		printk(KERN_INFO "VLAN");

	if (pktInfo->flags & MV_EXT_PPP_EXIST_MASK)
		printk(KERN_INFO " PPPoE");

	if (pktInfo->l3_type == ETH_P_IP)
		printk(KERN_INFO " ipv4");
	else if (pktInfo->l3_type == ETH_P_IPV6)
		printk(KERN_INFO " ipv6");
	else
		printk(KERN_INFO " non-ip");

	if (pktInfo->flags & MV_EXT_IP_FRAG_MASK)
		printk(KERN_INFO " FRAG");

	if (pktInfo->flags & MV_EXT_L3_VALID_MASK)
		printk(KERN_INFO " L3CSUM_OK");

	printk(" offset=%d, hdrlen=%d", pktInfo->l3_offset, pktInfo->l3_hdrlen);

	if (pktInfo->l4_proto == IPPROTO_TCP)
		printk(KERN_INFO " TCP");
	else if (pktInfo->l4_proto == IPPROTO_UDP)
		printk(KERN_INFO " UDP");

	if (pktInfo->flags & MV_EXT_L4_VALID_MASK)
		printk(KERN_INFO " L4CSUM_OK");

	printk(KERN_INFO "\n");
}


/* Return values:   0 - packet successfully processed by NFP (transmitted or dropped) */
/*                  1 - packet can't be processed by NFP  */
/*                  2 - skb is not valid for NFP (not enough headroom or nonlinear) */
/*                  3 - not enough info in pktInfo   */
int mv_eth_nfp_ext(struct net_device *dev, struct sk_buff *skb, MV_EXT_PKT_INFO *pktInfo)
{
	MV_STATUS           status;
	MV_NFP_RESULT       res;
	struct neta_rx_desc rx_desc;
	struct eth_pbuf     pkt;
	int                 err = 1;
	int                 i, port = -1;

#define NEEDED_HEADROOM (MV_PPPOE_HDR_SIZE + MV_VLAN_HLEN)

	/* Check that NFP is enabled for this external interface */
	for (i = 0; i < NFP_EXT_NUM; i++) {
		if ((mv_ctrl_nfp_ext_netdev[i] == dev) && (mv_ctrl_nfp_ext_en[i])) {
			port = mv_ctrl_nfp_ext_port[i];
			break;
		}
	}
	if (port == -1) /* NFP is disabled */
		return 1;

	if (skb_is_nonlinear(skb)) {
		printk(KERN_ERR "%s: skb=%p is nonlinear\n", __func__, skb);
		return 2;
	}

	/* Prepare pkt structure */
	pkt.offset = skb_headroom(skb) - (ETH_HLEN + MV_ETH_MH_SIZE);
	if (pkt.offset < NEEDED_HEADROOM) {
		/* we don't know at this stage if there will be added any of vlans or pppoe or both */
		printk(KERN_ERR "%s: Possible problem: not enough headroom: %d < %d\n",
				__func__, pkt.offset, NEEDED_HEADROOM);
		return 2;
	}

	pkt.pBuf = skb->head;
	pkt.bytes = skb->len + ETH_HLEN + MV_ETH_MH_SIZE;

	/* Set invalid pool to prevent BM usage */
	pkt.pool = MV_ETH_BM_POOLS;
	pkt.physAddr = mvOsIoVirtToPhys(NULL, skb->head);
	pkt.osInfo = (void *)skb;

	/* prepare rx_desc structure */
	status = mv_eth_nfp_ext_rxd_build(skb, pktInfo,  &rx_desc);
	if (status != MV_OK)
		return 3;

/*	read_lock(&nfp_lock);*/
	status = nfp_core_p->nfp_rx(port, &rx_desc, &pkt, &res);

/*	read_unlock(&nfp_lock);*/

	if (status == MV_OK) {
		if  (res.flags & MV_NFP_RES_NETDEV_EXT) {
			/* EXT RX -> EXT TX */
			mv_eth_nfp_ext_tx(NULL, &pkt, &res);
		} else {
			/* EXT RX -> INT TX */
			mvOsCacheFlush(NULL, pkt.pBuf + pkt.offset, pkt.bytes);
			status = mv_eth_nfp_tx(&pkt, &res);
			if (status != MV_OK)
				dev_kfree_skb_any(skb);
		}
		err = 0;
	} else if (status == MV_DROPPED) {
		dev_kfree_skb_any(skb);
		err = 0;
	}
	return err;
}
#endif /* CONFIG_MV_ETH_NFP_EXT */
