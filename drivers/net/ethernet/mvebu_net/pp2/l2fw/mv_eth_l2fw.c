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

#include <linux/version.h>

#ifdef CONFIG_MV_PP2_L2FW_XOR
#include "xor/mvXor.h"
#include "xor/mvXorRegs.h"
#include "mv_hal_if/mvSysXorApi.h"
#endif /* CONFIG_MV_PP2_L2FW_XOR */

#include "mv_eth_l2fw.h"
#include "../net_dev/mv_netdev.h"
#include "gbe/mvPp2Gbe.h"
#include "mvDebug.h"

#ifdef CONFIG_MV_PP2_L2SEC
#include "mv_eth_l2sec.h"
#endif

static int numHashEntries;
static int shared;

static struct l2fw_rule **l2fw_hash;
static struct eth_port_l2fw **mv_pp2_ports_l2fw;
static int eth_ports_l2fw_num;

static MV_U32 l2fw_jhash_iv;

#ifdef CONFIG_MV_PP2_L2FW_XOR
static MV_XOR_DESC *eth_xor_desc;
static MV_LONG      eth_xor_desc_phys_addr;
#endif

inline int mv_l2fw_rx(struct eth_port *pp, int rx_todo, int rxq);
inline int mv_l2fw_tx(struct sk_buff *skb, struct eth_port *pp, struct pp2_rx_desc *rx_desc);
inline int mv_l2fw_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl);
static int mv_l2fw_port_init(int port);
static void mv_l2fw_port_free(int port);

static const struct net_device_ops mv_l2fw_netdev_ops;
static const struct net_device_ops *mv_pp2_netdev_ops_ptr;

static struct l2fw_rule *l2fw_lookup(MV_U32 srcIP, MV_U32 dstIP)
{
	MV_U32 hash;
	struct l2fw_rule *rule;

	hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
	hash &= L2FW_HASH_MASK;
	rule = l2fw_hash[hash];

	while (rule) {
		if ((rule->srcIP == srcIP) && (rule->dstIP == dstIP)) {
#ifdef CONFIG_MV_PP2_L2FW_DEBUG
			printk(KERN_INFO "rule is not NULL in %s\n", __func__);
#endif
			return rule;
		}

		rule = rule->next;
	}

#ifdef CONFIG_MV_PP2_L2FW_DEBUG
	printk(KERN_INFO "rule is NULL in %s\n", __func__);
#endif

	return NULL;
}

void l2fw_show_numHashEntries(void)
{
	mvOsPrintf("number of Hash Entries is %d\n", numHashEntries);

}


void mv_l2fw_flush(void)
{
	MV_U32 i = 0;
	mvOsPrintf("\nFlushing L2fw Rule Database:\n");
	mvOsPrintf("*******************************\n");
	for (i = 0; i < L2FW_HASH_SIZE; i++)
		if (l2fw_hash[i]) {
			mvOsFree(l2fw_hash[i]);
			l2fw_hash[i] = NULL;
		}
	numHashEntries = 0;
}


void mv_l2fw_rules_dump(void)
{
	MV_U32 i = 0;
	struct l2fw_rule *currRule;
	MV_U8	  *srcIP, *dstIP;

	mvOsPrintf("\nPrinting L2fw Rule Database:\n");
	mvOsPrintf("*******************************\n");

	for (i = 0; i < L2FW_HASH_SIZE; i++) {
		currRule = l2fw_hash[i];
		srcIP = (MV_U8 *)&(currRule->srcIP);
		dstIP = (MV_U8 *)&(currRule->dstIP);

		while (currRule != NULL) {
			mvOsPrintf("%u.%u.%u.%u->%u.%u.%u.%u     out port=%d (hash=%x)\n",
				MV_IPQUAD(srcIP), MV_IPQUAD(dstIP),
				currRule->port, i);
			currRule = currRule->next;
		}
	}

}

void mv_l2fw_ports_dump(void)
{
	MV_U32 rx_port = 0;
	struct eth_port_l2fw *ppl2fw;

	mvOsPrintf("\nPrinting L2fw ports Database:\n");
	mvOsPrintf("*******************************\n");

	if (!mv_pp2_ports_l2fw)
		return;

	for (rx_port = 0; rx_port < eth_ports_l2fw_num; rx_port++) {
		ppl2fw = mv_pp2_ports_l2fw[rx_port];
		if (ppl2fw)
			mvOsPrintf("rx_port=%d cmd = %d tx_port=%d lookup=%d xor_threshold = %d\n",
					rx_port, ppl2fw->cmd, ppl2fw->txPort, ppl2fw->lookupEn, ppl2fw->xorThreshold);

	}
}


int mv_l2fw_add(MV_U32 srcIP, MV_U32 dstIP, int port)
{
	struct l2fw_rule *rule;
	MV_U8	  *srcIPchr, *dstIPchr;

	MV_U32 hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
	hash &= L2FW_HASH_MASK;
	if (numHashEntries == L2FW_HASH_SIZE) {
		printk(KERN_INFO "cannot add entry, hash table is full, there are %d entires\n", L2FW_HASH_SIZE);
		return MV_ERROR;
	}

	srcIPchr = (MV_U8 *)&(srcIP);
	dstIPchr = (MV_U8 *)&(dstIP);

#ifdef CONFIG_MV_PP2_L2FW_DEBUG
	mvOsPrintf("srcIP=%x dstIP=%x in %s\n", srcIP, dstIP, __func__);
	mvOsPrintf("srcIp = %u.%u.%u.%u in %s\n", MV_IPQUAD(srcIPchr), __func__);
	mvOsPrintf("dstIp = %u.%u.%u.%u in %s\n", MV_IPQUAD(dstIPchr), __func__);
#endif

	rule = l2fw_lookup(srcIP, dstIP);
	if (rule) {
		/* overwite port */
		rule->port = port;
		return MV_OK;
	}

	rule = (struct l2fw_rule *)mvOsMalloc(sizeof(struct l2fw_rule));
	if (!rule) {
		mvOsPrintf("%s: OOM\n", __func__);
		return MV_FAIL;
	}
#ifdef CONFIG_MV_PP2_L2FW_DEBUG
	mvOsPrintf("adding a rule to l2fw hash in %s\n", __func__);
#endif
	rule->srcIP = srcIP;
	rule->dstIP = dstIP;
	rule->port = port;

	rule->next = l2fw_hash[hash];
	l2fw_hash[hash] = rule;
	numHashEntries++;
	return MV_OK;
}

static int mv_pp2_poll_l2fw(struct napi_struct *napi, int budget)
{
	int rx_done = 0;
	MV_U32 causeRxTx;
	struct napi_group_ctrl *napi_group;
	struct eth_port *pp = MV_ETH_PRIV(napi->dev);
	int cpu = smp_processor_id();

	STAT_INFO(pp->stats.poll[cpu]++);

	/* Read cause register */
	causeRxTx = mvPp2GbeIsrCauseRxTxGet(pp->port);
	if (causeRxTx & MV_PP2_CAUSE_MISC_SUM_MASK) {
		if (causeRxTx & MV_PP2_CAUSE_FCS_ERR_MASK)
			printk(KERN_ERR "%s: FCS error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_RX_FIFO_OVERRUN_MASK)
			printk(KERN_ERR "%s: RX fifo overrun error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_TX_FIFO_UNDERRUN_MASK)
			printk(KERN_ERR "%s: TX fifo underrun error\n", __func__);

		if (causeRxTx & MV_PP2_CAUSE_MISC_SUM_MASK) {
			printk(KERN_ERR "%s: misc event\n", __func__);
			MV_REG_WRITE(MV_PP2_ISR_MISC_CAUSE_REG, 0);
		}

		causeRxTx &= ~MV_PP2_CAUSE_MISC_SUM_MASK;
		MV_REG_WRITE(MV_PP2_ISR_RX_TX_CAUSE_REG(MV_PPV2_PORT_PHYS(pp->port)), causeRxTx);
	}
	napi_group = pp->cpu_config[smp_processor_id()]->napi_group;
	causeRxTx |= napi_group->cause_rx_tx;

#ifdef CONFIG_MV_PP2_TXDONE_ISR

	/* TODO check this mode */

	if (mvPp2GbeIsrCauseTxDoneIsSet(pp->port, causeRxTx)) {
		int tx_todo = 0, cause_tx_done;

		/* TX_DONE process */
		cause_tx_done = mvPp2GbeIsrCauseTxDoneOffset(pp->port, causeRxTx);
		if (MV_PP2_IS_PON_PORT(pp->port)) {
			mv_pp2_tx_done_pon(pp, &tx_todo);
			mvOsPrintf("enter to mv_pp2_tx_done_pon\n");
		} else
			mv_pp2_tx_done_gbe(pp, cause_tx_done, &tx_todo);
	}
#endif /* CONFIG_MV_PP2_TXDONE_ISR */
	if (MV_PP2_IS_PON_PORT(pp->port))
		causeRxTx &= ~MV_PP2_PON_CAUSE_TXP_OCCUP_DESC_ALL_MASK;
	else
		causeRxTx &= ~MV_PP2_CAUSE_TXQ_OCCUP_DESC_ALL_MASK;

	while ((causeRxTx != 0) && (budget > 0)) {
		int count, rx_queue;

		rx_queue = mv_pp2_rx_policy(causeRxTx);
		if (rx_queue == -1)
			break;

		count = mv_l2fw_rx(pp, budget, rx_queue);
		rx_done += count;
		budget -= count;
		if (budget > 0)
			causeRxTx &= ~((1 << rx_queue) << MV_PP2_CAUSE_RXQ_OCCUP_DESC_OFFS);
	}

	STAT_DIST((rx_done < pp->dist_stats.rx_dist_size) ? pp->dist_stats.rx_dist[rx_done]++ : 0);

#ifdef CONFIG_MV_PP2_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_POLL) {
		printk(KERN_ERR "%s  EXIT: port=%d, cpu=%d, budget=%d, rx_done=%d\n",
			__func__, pp->port, cpu, budget, rx_done);
	}
#endif /* CONFIG_MV_PP2_DEBUG_CODE */

	if (budget > 0) {
		unsigned long flags;

		causeRxTx = 0;

		napi_complete(napi);

		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);

		local_irq_save(flags);
		/* Enable interrupts for all cpus belong to this group */
		mvPp2GbeCpuInterruptsEnable(pp->port, napi_group->cpu_mask);
		local_irq_restore(flags);
	}
	napi_group->cause_rx_tx = causeRxTx;
	return rx_done;
}


static int mv_l2fw_update_napi(struct eth_port *pp, bool l2fw)
{
	int group;
	struct napi_group_ctrl *napi_group;


	for (group = 0; group < 1/*MV_ETH_MAX_NAPI_GROUPS*/; group++) {
		napi_group = pp->napi_group[group];
/*
		if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
			napi_disable(napi_group->napi);
*/
		netif_napi_del(napi_group->napi);

		if (l2fw)
			netif_napi_add(pp->dev, napi_group->napi, mv_pp2_poll_l2fw, pp->weight);
		else
			netif_napi_add(pp->dev, napi_group->napi, mv_pp2_poll, pp->weight);
/*
		if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
			napi_enable(napi_group->napi);
*/
	}
	return MV_OK;
}

static int mv_l2fw_check(int port, bool l2fw)
{
	if (!l2fw) {
		/* user try to exit form l2fw */
		if (!mv_pp2_ports_l2fw) {
			mvOsPrintf("port #%d l2fw already disabled\n", port);
			return MV_ERROR;
		}

		if (!mv_pp2_ports_l2fw[port]) {
			mvOsPrintf("port #%d l2fw already disabled\n", port);
			return MV_ERROR;
		}

	/* user try to enter into l2fw */
	} else if (mv_pp2_ports_l2fw && mv_pp2_ports_l2fw[port]) {
			mvOsPrintf("port #%d l2fw already enabled\n", port);
			return MV_ERROR;
	}

	return MV_OK;
}

int mv_l2fw_set(int port, bool l2fw)
{
	struct eth_port *pp = mv_pp2_port_by_id(port);
	int status = MV_OK;

	if (mv_l2fw_check(port, l2fw))
		return MV_ERROR;

	if (!pp) {
		mvOsPrintf("pp is NULL in setting L2FW (%s)\n", __func__);
		return MV_ERROR;
	}

	if (pp->flags & MV_ETH_F_STARTED) {
		pr_err("%s: port %d must be stopped before\n", __func__, port);
		return -EINVAL;
	}

	/* for multiBuffer validation */
	/*mvGmacMaxRxSizeSet(port, 9000);*/

	if (!mv_pp2_netdev_ops_ptr) {
		/* enter only once - save eth ops */
		mv_pp2_netdev_ops_ptr = pp->dev->netdev_ops;
		/* set maximum number of ports */
		eth_ports_l2fw_num = pp->plat_data->max_port;
	}

	if (mv_l2fw_update_napi(pp, l2fw))
		return MV_ERROR;

	if (l2fw) {
		status = mv_l2fw_port_init(port);
		pp->dev->netdev_ops  = &mv_l2fw_netdev_ops;

	} else {
		pp->dev->netdev_ops = mv_pp2_netdev_ops_ptr;
		mv_l2fw_port_free(port);
	}

	return status;
}

int mv_l2fw_port(int rx_port, int tx_port, int cmd)
{
	struct eth_port_l2fw *ppl2fw;

	if (!mv_pp2_ports_l2fw) {
		mvOsPrintf("%s: ports are not in l2fw mode\n", __func__);
		return MV_ERROR;
	}
	if (mvPp2MaxCheck(rx_port, eth_ports_l2fw_num, "rx_port"))
		return MV_ERROR;

	if (mvPp2MaxCheck(tx_port, eth_ports_l2fw_num, "tx_port"))
		return MV_ERROR;

	if (!mv_pp2_ports_l2fw[rx_port]) {
		mvOsPrintf("%s: port #%d is not in l2fw mode\n", __func__, rx_port);
		return MV_ERROR;
	}

	if (!mv_pp2_ports_l2fw[tx_port]) {
		mvOsPrintf("%s: port #%d is not in l2fw mode\n", __func__, tx_port);
		return MV_ERROR;
	}

	if (cmd > CMD_L2FW_LAST) {
		mvOsPrintf("Error: invalid command %d\n", cmd);
		return MV_ERROR;
	}

	ppl2fw = mv_pp2_ports_l2fw[rx_port];
	ppl2fw->cmd = cmd;
	ppl2fw->txPort = tx_port;

	return MV_OK;

}

inline unsigned char *l2fw_swap_mac(unsigned char *buff)
{
	MV_U16 *pSrc;
	int i;
	MV_U16 swap;
	pSrc = (MV_U16 *)(buff + MV_ETH_MH_SIZE);

	for (i = 0; i < 3; i++) {
		swap = pSrc[i];
		pSrc[i] = pSrc[i+3];
		pSrc[i+3] = swap;
		}

	return  buff;
}

inline void l2fw_copy_mac(unsigned char *rx_buff, unsigned char *tx_buff)
{
	/* copy 30 bytes (start after MH header) */
	/* 12 for SA + DA */
	/* 18 for the rest */
	MV_U16 *pSrc;
	MV_U16 *pDst;
	int i;
	pSrc = (MV_U16 *)(rx_buff);
	pDst = (MV_U16 *)(tx_buff);

	/* swap mac SA and DA */
	for (i = 0; i < 3; i++) {
		pDst[i]   = pSrc[i+3];
		pDst[i+3] = pSrc[i];
		}
	for (i = 6; i < 15; i++)
		pDst[i] = pSrc[i];
	}

inline void l2fw_copy_and_swap_mac(unsigned char *rx_buff, unsigned char *tx_buff)
{
	MV_U16 *pSrc;
	MV_U16 *pDst;
	int i;

	pSrc = (MV_U16 *)(rx_buff);
	pDst = (MV_U16 *)(tx_buff);
	for (i = 0; i < 3; i++) {
		pDst[i]   = pSrc[i+3];
		pDst[i+3] = pSrc[i];
	}
}

inline struct sk_buff *eth_l2fw_copy_packet_withOutXor(struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	MV_U8 *pSrc;
	MV_U8 *pDst;
	int poolId;
	struct sk_buff *skb_new;
	int  bytes = rx_desc->dataSize - MV_ETH_MH_SIZE;
	/* 12 for SA + DA */
	int mac = 2 * MV_MAC_ADDR_SIZE;

	mvOsCacheInvalidate(NULL, skb->data, bytes);

	poolId = mvPp2RxBmPoolId(rx_desc);

	skb_new = (struct sk_buff *)mv_pp2_pool_get(poolId);

	if (!skb_new) {
		mvOsPrintf("skb == NULL in %s\n", __func__);
		return NULL;
	}

	pSrc = skb->data + MV_ETH_MH_SIZE;
	pDst = skb_new->data + MV_ETH_MH_SIZE;

	memcpy(pDst + mac, pSrc + mac, bytes - mac);
	l2fw_copy_and_swap_mac(pSrc, pDst);
	mvOsCacheFlush(NULL, skb_new->data, bytes);

	return skb_new;
}

#ifdef CONFIG_MV_PP2_L2FW_XOR
inline struct sk_buff *eth_l2fw_copy_packet_withXor(struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	struct sk_buff *skb_new = NULL;
	MV_U8 *pSrc;
	MV_U8 *pDst;
	int poolId;
	unsigned int bufPhysAddr;
	int  bytes = rx_desc->dataSize - MV_ETH_MH_SIZE;

	poolId = mvPp2RxBmPoolId(rx_desc);

	skb_new = (struct sk_buff *)mv_pp2_pool_get(poolId);

	if (!skb_new) {
		mvOsPrintf("skb == NULL in %s\n", __func__);
		return NULL;
	}

	/* sync between giga and XOR to avoid errors (like checksum errors in TX)
	   when working with IOCC */

	mvOsCacheIoSync(NULL);

	bufPhysAddr =  mvOsCacheFlush(NULL, skb->data, bytes);
	eth_xor_desc->srcAdd0    = bufPhysAddr + skb_headroom(skb) + MV_ETH_MH_SIZE + 30;

	bufPhysAddr =  mvOsCacheFlush(NULL, skb_new->data, bytes);
	eth_xor_desc->srcAdd0    = bufPhysAddr + skb_headroom(skb_new) + MV_ETH_MH_SIZE + 30;

	eth_xor_desc->byteCnt    = bytes - 30;

	eth_xor_desc->phyNextDescPtr = 0;
	eth_xor_desc->status         = BIT31;
	/* we had changed only the first part of eth_xor_desc, so flush only one
	 line of cache */
	mvOsCacheLineFlush(NULL, eth_xor_desc);
	MV_REG_WRITE(XOR_NEXT_DESC_PTR_REG(1, XOR_CHAN(0)), eth_xor_desc_phys_addr);

	MV_REG_WRITE(XOR_ACTIVATION_REG(1, XOR_CHAN(0)), XEXACTR_XESTART_MASK);

	mvOsCacheLineInv(NULL, skb->data);

	pSrc = skb->data + MV_ETH_MH_SIZE;
	pDst = skb_new->data + MV_ETH_MH_SIZE;

	l2fw_copy_mac(pSrc, pDst);
	mvOsCacheLineFlush(NULL, skb_new->data);

	return skb_new;
}

void setXorDesc(void)
{
	unsigned int mode;
	eth_xor_desc = mvOsMalloc(sizeof(MV_XOR_DESC) + XEXDPR_DST_PTR_DMA_MASK + 32);
	eth_xor_desc = (MV_XOR_DESC *)MV_ALIGN_UP((MV_U32)eth_xor_desc, XEXDPR_DST_PTR_DMA_MASK+1);
	eth_xor_desc_phys_addr = mvOsIoVirtToPhys(NULL, eth_xor_desc);
	mvSysXorInit();

	mode = MV_REG_READ(XOR_CONFIG_REG(1, XOR_CHAN(0)));
	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_DMA;
	MV_REG_WRITE(XOR_CONFIG_REG(1, XOR_CHAN(0)), mode);
	MV_REG_WRITE(XOR_NEXT_DESC_PTR_REG(1, XOR_CHAN(0)), eth_xor_desc_phys_addr);
	/* TODO mask xor intterupts*/
}


inline int xorReady(void)
{
	int timeout = 0;

	while (!(MV_REG_READ(XOR_CAUSE_REG(1)) & XOR_CAUSE_DONE_MASK(XOR_CHAN(0)))) {
		if (timeout > 0x100000) {
			mvOsPrintf("XOR timeout\n");
			return 0;
			}
		timeout++;
	}

	/* Clear int */
	MV_REG_WRITE(XOR_CAUSE_REG(1), ~(XOR_CAUSE_DONE_MASK(XOR_CHAN(0))));

	return 1;
}

void mv_l2fw_xor(int rx_port, int threshold)
{
	if (mvPp2MaxCheck(rx_port, eth_ports_l2fw_num, "rx_port"))
		return;

	mvOsPrintf("setting port %d threshold to %d in %s\n", rx_port, threshold, __func__);
	mv_pp2_ports_l2fw[rx_port]->xorThreshold = threshold;
}
#endif /* CONFIG_MV_PP2_L2FW_XOR */

void mv_l2fw_lookupEn(int rx_port, int enable)
{
	if (mvPp2MaxCheck(rx_port, eth_ports_l2fw_num, "rx_port"))
		return;

	mvOsPrintf("setting port %d lookup mode to %s\n", rx_port, (enable == 1) ? "enable" : "disable");
	mv_pp2_ports_l2fw[rx_port]->lookupEn = enable;
}

void mv_l2fw_stats(void)
{
	int i;

	if (!mv_pp2_ports_l2fw)
		return;

	for (i = 0; i < eth_ports_l2fw_num; i++) {
		if (mv_pp2_ports_l2fw[i]) {
			mvOsPrintf("number of errors in port[%d]=%d\n", i, mv_pp2_ports_l2fw[i]->statErr);
			mvOsPrintf("number of drops  in port[%d]=%d\n", i, mv_pp2_ports_l2fw[i]->statDrop);
		}
	}

#ifdef CONFIG_MV_PP2_L2SEC
	mv_l2sec_stats();
#endif

}

inline int mv_l2fw_tx(struct sk_buff *skb, struct eth_port *pp, struct pp2_rx_desc *rx_desc)
{
	struct pp2_tx_desc *tx_desc;
	u32 tx_cmd = 0;
	struct mv_pp2_tx_spec *tx_spec_ptr = NULL;
	struct tx_queue *txq_ctrl;
	struct aggr_tx_queue *aggr_txq_ctrl = NULL;
	struct txq_cpu_ctrl *txq_cpu_ptr;
	int qset, grntd;
	int cpu = smp_processor_id(), poolId, frags = 1;
	tx_spec_ptr = &pp->tx_spec;
	tx_spec_ptr->txq = pp->cpu_config[cpu]->txq;
	aggr_txq_ctrl = &aggr_txqs[cpu];

	txq_ctrl = &pp->txq_ctrl[tx_spec_ptr->txp * CONFIG_MV_PP2_TXQ + tx_spec_ptr->txq];
	txq_cpu_ptr = &(txq_ctrl->txq_cpu[cpu]);

#ifdef CONFIG_MV_ETH_PP2_1
	if (mv_pp2_reserved_desc_num_proc(pp, tx_spec_ptr->txp, tx_spec_ptr->txq, frags) ||
		mv_pp2_aggr_desc_num_check(aggr_txq_ctrl, frags)) {
		frags = 0;
		goto out;
	}
#else
	if (mv_pp2_aggr_desc_num_check(aggr_txq_ctrl, frags))
		goto out;
#endif /*CONFIG_MV_ETH_PP2_1*/

	/* Get next descriptor for tx, single buffer, so FIRST & LAST */
	tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);

	if (tx_desc == NULL) {
		pp->dev->stats.tx_dropped++;
		return MV_DROPPED;
		/* TODO wait until xor is ready */
	}

	/* check if buffer header is used */
	if (rx_desc->status & PP2_RX_BUF_HDR_MASK)
		tx_cmd |= PP2_TX_BUF_HDR_MASK | PP2_TX_DESC_PER_PKT;

	if (tx_spec_ptr->flags & MV_ETH_TX_F_NO_PAD)
		tx_cmd |= PP2_TX_PADDING_DISABLE_MASK;

	poolId = mvPp2RxBmPoolId(rx_desc);

	/* buffers released by HW */
	tx_cmd |= (poolId << PP2_TX_POOL_INDEX_OFFS) | PP2_TX_BUF_RELEASE_MODE_MASK |
			PP2_TX_F_DESC_MASK | PP2_TX_L_DESC_MASK |
			PP2_TX_L4_CSUM_NOT | PP2_TX_IP_CSUM_DISABLE_MASK;

	tx_desc->command = tx_cmd;

#ifdef CONFIG_MV_ETH_PP2_1
	qset = (rx_desc->bmQset & PP2_RX_BUFF_QSET_NUM_MASK) >> PP2_RX_BUFF_QSET_NUM_OFFS;
	grntd = (rx_desc->bmQset & PP2_RX_BUFF_TYPE_MASK) >> PP2_RX_BUFF_TYPE_OFFS;
	tx_desc->hwCmd[1] = (qset << PP2_TX_MOD_QSET_OFFS) | (grntd << PP2_TX_MOD_GRNTD_BIT);
#endif

	tx_desc->physTxq = MV_PPV2_TXQ_PHYS(pp->port, tx_spec_ptr->txp, tx_spec_ptr->txq);

	txq_ctrl = &pp->txq_ctrl[tx_spec_ptr->txp * CONFIG_MV_PP2_TXQ + tx_spec_ptr->txq];

	if (txq_ctrl == NULL) {
		printk(KERN_ERR "%s: invalidate txp/txq (%d/%d)\n",
			__func__, tx_spec_ptr->txp, tx_spec_ptr->txq);
		pp->dev->stats.tx_dropped++;
		return MV_DROPPED;
	}

	txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];

	if (txq_cpu_ptr->txq_count >= mv_ctrl_pp2_txdone)
		mv_l2fw_txq_done(pp, txq_ctrl);

	if (MV_PP2_IS_PON_PORT(pp->port)) {
		tx_desc->dataSize  = rx_desc->dataSize;
		tx_desc->pktOffset = skb_headroom(skb);
	} else {
		tx_desc->dataSize  = rx_desc->dataSize - MV_ETH_MH_SIZE;
		tx_desc->pktOffset = skb_headroom(skb) + MV_ETH_MH_SIZE;
	}

	tx_desc->bufCookie = (MV_U32)skb;
	tx_desc->bufPhysAddr = mvOsCacheFlush(NULL, skb->head, tx_desc->dataSize);
	mv_pp2_tx_desc_flush(pp, tx_desc);

	/* TODO - XOR ready check */

#ifdef CONFIG_MV_ETH_PP2_1
	txq_cpu_ptr->reserved_num--;
#endif
	txq_cpu_ptr->txq_count++;
	aggr_txq_ctrl->txq_count++;

#ifdef CONFIG_MV_PP2_DEBUG_CODE
	if (pp->dbg_flags & MV_ETH_F_DBG_TX) {
		printk(KERN_ERR "\n");
		printk(KERN_ERR "%s - eth_l2fw_tx_%lu: cpu=%d, in_intr=0x%lx, port=%d, txp=%d, txq=%d\n",
			pp->dev->name, pp->dev->stats.tx_packets, smp_processor_id(), in_interrupt(),
			pp->port, tx_spec_ptr->txp, tx_spec_ptr->txq);

		mv_pp2_tx_desc_print(tx_desc);
		mvDebugMemDump(skb->data, 64, 1);
	}
#endif /* CONFIG_MV_PP2_DEBUG_CODE */

	/* Enable transmit */
	wmb();
	mvPp2AggrTxqPendDescAdd(frags);

	STAT_DBG(aggr_txq_ctrl->stats.txq_tx++);
	STAT_DBG(txq_ctrl->txq_cpu[cpu].stats.txq_tx++);

	pp->dev->stats.tx_packets++;
	pp->dev->stats.tx_bytes += rx_desc->dataSize - MV_ETH_MH_SIZE;

out:
#ifndef CONFIG_MV_PP2_TXDONE_ISR
	if (txq_cpu_ptr->txq_count >= mv_ctrl_pp2_txdone)
		mv_l2fw_txq_done(pp, txq_ctrl);
#endif /* CONFIG_MV_PP2_STAT_DIST */

	return NETDEV_TX_OK;
}


inline int mv_l2fw_txq_done(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	struct txq_cpu_ctrl *txq_cpu_ptr = &txq_ctrl->txq_cpu[smp_processor_id()];
	int tx_done = mvPp2TxqSentDescProc(pp->port, txq_ctrl->txp, txq_ctrl->txq);

	if (!tx_done)
		return tx_done;

	txq_cpu_ptr->txq_count -= tx_done;
	STAT_DBG(txq_cpu_ptr->stats.txq_txdone += tx_done);
	return tx_done;
}

static int mv_l2fw_txq_done_force(struct eth_port *pp, struct tx_queue *txq_ctrl)
{
	int cpu, tx_done = 0;
	struct txq_cpu_ctrl *txq_cpu_ptr;

	for_each_possible_cpu(cpu) {
		txq_cpu_ptr = &txq_ctrl->txq_cpu[cpu];
		tx_done += txq_cpu_ptr->txq_count;
		txq_cpu_ptr->txq_count = 0;
	}
	return tx_done;
}

static int mv_l2fw_txq_clean(int port, int txp, int txq)
{
	struct eth_port *pp;
	struct tx_queue *txq_ctrl;
	int msec, pending, tx_done;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	pp = mv_pp2_port_by_id(port);
	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if (mvPp2MaxCheck(txq, CONFIG_MV_PP2_TXQ, "txq"))
		return -EINVAL;

	txq_ctrl = &pp->txq_ctrl[txp * CONFIG_MV_PP2_TXQ + txq];
	if (txq_ctrl->q) {
		/* Enable TXQ drain */
		mvPp2TxqDrainSet(port, txp, txq, MV_TRUE);

		/* Wait for all packets to be transmitted */
		msec = 0;
		do {
			if (msec >= 1000 /*timeout*/) {
				pr_err("port=%d, txp=%d txq=%d: timeout for transmit pending descriptors\n",
					port, txp, txq);
				break;
			}
			mdelay(1);
			msec++;

			pending = mvPp2TxqPendDescNumGet(port, txp, txq);
		} while (pending);

		/* Disable TXQ Drain */
		mvPp2TxqDrainSet(port, txp, txq, MV_FALSE);

		/* release all transmitted packets */
		tx_done = mv_l2fw_txq_done(pp, txq_ctrl);
		if (tx_done > 0)
			mvOsPrintf(KERN_INFO "%s: port=%d, txp=%d txq=%d: Free %d transmitted descriptors\n",
				__func__, port, txp, txq, tx_done);

		/* release all untransmitted packets */
		tx_done = mv_l2fw_txq_done_force(pp, txq_ctrl);
		if (tx_done > 0)
			mvOsPrintf(KERN_INFO "%s: port=%d, txp=%d txq=%d: Free %d untransmitted descriptors\n",
				__func__, port, txp, txq, tx_done);
	}
	return 0;
}

static int mv_l2fw_txp_clean(int port, int txp)
{
	struct eth_port *pp;
	int txq;

	if (mvPp2TxpCheck(port, txp))
		return -EINVAL;

	pp = mv_pp2_port_by_id(port);
	if ((pp == NULL) || (pp->txq_ctrl == NULL))
		return -ENODEV;

	if (pp->flags & MV_ETH_F_STARTED) {
		printk(KERN_ERR "Port %d must be stopped before\n", port);
		return -EINVAL;
	}

	/* Flush TX FIFO */
	mvPp2TxPortFifoFlush(port, MV_TRUE);

	/* free the skb's in the hal tx ring */
	for (txq = 0; txq < CONFIG_MV_PP2_TXQ; txq++)
		mv_l2fw_txq_clean(port, txp, txq);

	mvPp2TxPortFifoFlush(port, MV_FALSE);

	mvPp2TxpReset(port, txp);

	return 0;
}




inline void mv_l2fw_pool_refill(struct eth_port *pp,
				     struct bm_pool *pool, struct pp2_rx_desc *rx_desc)
{
	if ((rx_desc->status & PP2_RX_BUF_HDR_MASK) == MV_FALSE) {
		__u32 bm = mv_pp2_bm_cookie_build(rx_desc);
		mv_pp2_pool_refill(pool, bm, rx_desc->bufPhysAddr, rx_desc->bufCookie);
	} else
		/* multiBuffer mode */
		mv_pp2_buff_hdr_rx(pp, rx_desc);
}

inline int mv_l2fw_rx(struct eth_port *pp, int rx_todo, int rxq)
{
	struct eth_port  *new_pp;
	struct l2fw_rule *rule;
	MV_PP2_PHYS_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;
	int rx_done, rx_filled, poolId, bytes;
	u32 rx_status;
	struct pp2_rx_desc *rx_desc;
	struct bm_pool *pool;
	MV_STATUS status = MV_OK;
	struct eth_port_l2fw *ppl2fw = mv_pp2_ports_l2fw[pp->port];
	MV_IP_HEADER *pIph = NULL;
	int ipOffset;
	struct sk_buff *skb, *skb_new = NULL;
	MV_U32 bufPhysAddr, bm;

	rx_done = mvPp2RxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync(NULL);

	if ((rx_todo > rx_done) || (rx_todo < 0))
		rx_todo = rx_done;

	if (rx_todo == 0)
		return 0;

	rx_done = 0;
	rx_filled = 0;

	/* Fairness NAPI loop */
	while (rx_done < rx_todo) {
#ifdef CONFIG_MV_PP2_RX_DESC_PREFETCH
		rx_desc = mv_pp2_rx_prefetch(pp, rx_ctrl, rx_done, rx_todo);
#else
		rx_desc = mvPp2RxqNextDescGet(rx_ctrl);
		mvOsCacheLineInv(NULL, rx_desc);
		prefetch(rx_desc);
#endif /* CONFIG_MV_PP2_RX_DESC_PREFETCH */

		if (!rx_desc)
			printk(KERN_INFO "rx_desc is NULL in %s\n", __func__);

		rx_done++;
		rx_filled++;

		rx_status = rx_desc->status;

#ifdef CONFIG_MV_PP2_DEBUG_CODE
		/* check if buffer header is in used */
		if (pp->dbg_flags & MV_ETH_F_DBG_BUFF_HDR)
			if (rx_status & PP2_RX_BUF_HDR_MASK)
				mv_pp2_buff_hdr_rx_dump(pp, rx_desc);

		/* print RX descriptor */
		if (pp->dbg_flags & MV_ETH_F_DBG_RX)
			mv_pp2_rx_desc_print(rx_desc);
#endif /* CONFIG_MV_PP2_DEBUG_CODE */

		skb = (struct sk_buff *)rx_desc->bufCookie;

		if (!skb) {
			printk(KERN_INFO "%s: skb is NULL, rx_done=%d\n", __func__, rx_done);
			return rx_done;
		}

		poolId = mvPp2RxBmPoolId(rx_desc);
		pool = &mv_pp2_pool[poolId];

		if (rx_status & PP2_RX_ES_MASK) {
			printk(KERN_ERR "giga #%d: bad rx status 0x%08x\n", pp->port, rx_status);
			mv_l2fw_pool_refill(pp, pool, rx_desc);
			continue;
		}

		ipOffset = (rx_status & PP2_RX_L3_OFFSET_MASK) >> PP2_RX_L3_OFFSET_OFFS;

		pIph = (MV_IP_HEADER *)(skb->data + ipOffset);

		if (pIph == NULL) {
			printk(KERN_INFO "pIph==NULL in %s\n", __func__);
			continue;
		}
#ifdef CONFIG_MV_PP2_L2FW_DEBUG
		if (pp->dbg_flags & MV_ETH_F_DBG_RX) {

			mvDebugMemDump(skb->data, 64, 1);

			if (pIph) {
				MV_U8 *srcIP, *dstIP;
				srcIP = (MV_U8 *)&(pIph->srcIP);
				dstIP = (MV_U8 *)&(pIph->dstIP);
				printk(KERN_INFO "%u.%u.%u.%u->%u.%u.%u.%u in %s\n",
						MV_IPQUAD(srcIP), MV_IPQUAD(dstIP), __func__);
				printk(KERN_INFO "0x%x->0x%x in %s\n", pIph->srcIP, pIph->dstIP, __func__);
			} else
				printk(KERN_INFO "pIph is NULL in %s\n", __func__);
		}
#endif
		if (ppl2fw->lookupEn) {
			rule = l2fw_lookup(pIph->srcIP, pIph->dstIP);

			new_pp = rule ? mv_pp2_ports[rule->port] : mv_pp2_ports[ppl2fw->txPort];

		} else
			new_pp  = mv_pp2_ports[ppl2fw->txPort];

		bytes = rx_desc->dataSize - MV_ETH_MH_SIZE;

		switch (ppl2fw->cmd) {
		case CMD_L2FW_AS_IS:
			status = mv_l2fw_tx(skb, new_pp, rx_desc);
			break;

		case CMD_L2FW_SWAP_MAC:
			mvOsCacheLineInv(NULL, skb->data);
			l2fw_swap_mac(skb->data);
			mvOsCacheLineFlush(NULL, skb->data);
			status = mv_l2fw_tx(skb, new_pp, rx_desc);
			break;

		case CMD_L2FW_COPY_SWAP:
			if (rx_status & PP2_RX_BUF_HDR_MASK) {
				printk(KERN_INFO "%s: not support copy with multibuffer packets.\n", __func__);
				status = MV_ERROR;
				break;
			}
#ifdef CONFIG_MV_PP2_L2FW_XOR
			if (bytes >= ppl2fw->xorThreshold) {
				skb_new = eth_l2fw_copy_packet_withXor(skb, rx_desc);
				pr_error("%s: xor is not supported\n", __func__);
			}
#endif /* CONFIG_MV_PP2_L2FW_XOR */

			if (skb_new == NULL)
				skb_new = eth_l2fw_copy_packet_withOutXor(skb, rx_desc);

			if (skb_new) {
				bufPhysAddr = rx_desc->bufPhysAddr;

				bm = mv_pp2_bm_cookie_build(rx_desc);
				status = mv_l2fw_tx(skb_new, new_pp, rx_desc);

				mv_pp2_pool_refill(pool, bm, bufPhysAddr, (MV_ULONG)skb);

				/* for refill function */
				skb = skb_new;
			} else
				status = MV_ERROR;
			break;
#ifdef CONFIG_MV_PP2_L2SEC
		case CMD_L2FW_CESA:
			if (rx_status & PP2_RX_BUF_HDR_MASK) {
				printk(KERN_INFO "%s: not support cesa with multibuffer packets.\n", __func__);
				status = MV_ERROR;
				break;
			}
				status = mv_l2sec_handle_esp(pkt, rx_desc, new_pp, pp->port);
			break;
#endif

		default:
			printk(KERN_INFO "WARNING:%s invalid mode %d, rx port %d\n", __func__, ppl2fw->cmd, pp->port);
			status = MV_DROPPED;
		} /*switch*/

		if (status == MV_OK) {
			/* BM - no refill */
			mvOsCacheLineInv(NULL, rx_desc);
			continue;
		}

		/* status is not OK */
		mv_l2fw_pool_refill(pp, pool, rx_desc);

		if (status == MV_DROPPED)
			ppl2fw->statDrop++;

		if (status == MV_ERROR)
			ppl2fw->statErr++;

	} /* of while */

	/* Update RxQ management counters */
	mvOsCacheIoSync(NULL);
	mvPp2RxqDescNumUpdate(pp->port, rxq, rx_done, rx_filled);

	return rx_done;
}

static void mv_l2fw_shared_cleanup(void)
{
	if (mv_pp2_ports_l2fw)
		mvOsFree(mv_pp2_ports_l2fw);

	if (l2fw_hash) {
		mv_l2fw_flush();
		mvOsFree(l2fw_hash);
	}

	mv_pp2_ports_l2fw = NULL;
	l2fw_hash = NULL;
}


static int mv_l2fw_shared_init(void)
{
	int size, bytes;

	size = eth_ports_l2fw_num * sizeof(struct eth_port_l2fw *);
	mv_pp2_ports_l2fw = mvOsMalloc(size);

	if (!mv_pp2_ports_l2fw)
		goto oom;

	memset(mv_pp2_ports_l2fw, 0, size);

	bytes = sizeof(struct l2fw_rule *) * L2FW_HASH_SIZE;
	get_random_bytes(&l2fw_jhash_iv, sizeof(l2fw_jhash_iv));
	l2fw_hash = (struct l2fw_rule **)mvOsMalloc(bytes);

	if (l2fw_hash == NULL) {
		mvOsPrintf("l2fw hash: not enough memory\n");
		goto oom;
	}

	mvOsMemset(l2fw_hash, 0, bytes);

	mvOsPrintf("L2FW hash init %d entries, %d bytes\n", L2FW_HASH_SIZE, bytes);

#ifdef CONFIG_MV_PP2_L2SEC
	mv_l2sec_cesa_init();
#endif

#ifdef CONFIG_MV_PP2_L2FW_XOR
	setXorDesc();
#endif

	return MV_OK;
oom:
	mv_l2fw_shared_cleanup();
	mvOsPrintf("%s: out of memory in L2FW initialization\n", __func__);

	return -ENOMEM;

}

static int mv_l2fw_port_init(int port)
{
	int status;

	if (!shared) {
		status = mv_l2fw_shared_init();
		if (status)
			return status;
	}

	mv_pp2_ports_l2fw[port] = mvOsMalloc(sizeof(struct eth_port_l2fw));
	if (!mv_pp2_ports_l2fw[port])
		goto oom;

	mv_pp2_ports_l2fw[port]->cmd    = CMD_L2FW_AS_IS;
	mv_pp2_ports_l2fw[port]->txPort = port;
	mv_pp2_ports_l2fw[port]->lookupEn = 0;
	mv_pp2_ports_l2fw[port]->xorThreshold = XOR_THRESHOLD_DEF;
	mv_pp2_ports_l2fw[port]->statErr = 0;
	mv_pp2_ports_l2fw[port]->statDrop = 0;

	shared++;

	return MV_OK;

oom:
	if (!shared)
		mv_l2fw_shared_cleanup();

	return -ENOMEM;
}


static void mv_l2fw_port_free(int port)
{
	if (!mv_pp2_ports_l2fw) {
		mvOsPrintf("in %s: l2fw database is NULL\n", __func__);
		return;
	}

	if (!mv_pp2_ports_l2fw[port]) {
		mvOsPrintf("in %s: l2fw port #%d database is NULL\n", __func__, port);
		return;
	}

	mvOsFree(mv_pp2_ports_l2fw[port]);
	mv_pp2_ports_l2fw[port] = NULL;

	shared--;

	if (!shared)
		mv_l2fw_shared_cleanup();
}

int mv_l2fw_stop(struct net_device *dev)
{
	int txp;
	struct eth_port *pp = MV_ETH_PRIV(dev);

	for (txp = 0; txp < pp->txp_num; txp++)
		if (mv_l2fw_txp_clean(pp->port, txp))
			return MV_ERROR;

	return mv_pp2_eth_stop(dev);
}

static netdev_tx_t mv_l2fw_xmit(struct sk_buff *skb, struct net_device *dev)
{
	return NETDEV_TX_LOCKED;
}

static const struct net_device_ops mv_l2fw_netdev_ops = {
	.ndo_open = mv_pp2_eth_open,
	.ndo_stop = mv_l2fw_stop,
	.ndo_start_xmit = mv_l2fw_xmit,
	.ndo_set_rx_mode = mv_pp2_rx_set_rx_mode,
	.ndo_set_mac_address = mv_pp2_eth_set_mac_addr,
	.ndo_change_mtu = mv_pp2_eth_change_mtu,
};
