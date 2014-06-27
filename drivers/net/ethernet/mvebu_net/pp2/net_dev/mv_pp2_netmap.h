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
/* mv_pp2_netmap.h */

#ifndef __MV_PP2_NETMAP_H__
#define __MV_PP2_NETMAP_H__

#include <bsd_glue.h>
#include <netmap.h>
#include <netmap_kern.h>

#define SOFTC_T	eth_port

static int pool_buf_num[MV_BM_POOLS];
static struct bm_pool *pool_short[MV_ETH_MAX_PORTS];
/*
 * Register/unregister
 *	adapter is pointer to eth_port
 */
static int mv_pp2_netmap_reg(struct ifnet *ifp, int onoff)
{
	struct eth_port *adapter = MV_ETH_PRIV(ifp);
	struct netmap_adapter *na = NA(ifp);
	int error = 0, txq, rxq;

	if (na == NULL)
		return -EINVAL;

	if (!(ifp->flags & IFF_UP)) {
		/* mv_pp2_eth_open has not been called yet, so resources
		 * are not allocated */
		printk(KERN_ERR "Interface is down!");
		return -EINVAL;
	}

	/* stop current interface */
	if (mv_pp2_eth_stop(ifp)) {
		printk(KERN_ERR "%s: stop interface failed\n", ifp->name);
		return -EINVAL;
	}

	if (onoff) { /* enable netmap mode */
		u32 port_map;

		mv_pp2_rx_reset(adapter->port);
		ifp->if_capenable |= IFCAP_NETMAP;
		na->if_transmit = (void *)ifp->netdev_ops;
		ifp->netdev_ops = &na->nm_ndo;

		/* check that long pool is not shared with other ports */
		port_map =  mv_pp2_ctrl_pool_port_map_get(adapter->pool_long->pool);
		if (port_map != (1 << adapter->port)) {
			printk(KERN_ERR "%s: BM pool %d not initialized or shared with other ports.\n",
			__func__, adapter->pool_long->pool);
			return -EINVAL;
		}

		/* Keep old number of long pool buffers */
		pool_buf_num[adapter->pool_long->pool] = adapter->pool_long->buf_num;
		mv_pp2_pool_free(adapter->pool_long->pool, adapter->pool_long->buf_num);

		/* set same pool number for short and long packets */
		for (rxq = 0; rxq < CONFIG_MV_PP2_RXQ; rxq++)
			mvPp2RxqBmShortPoolSet(adapter->port, rxq, adapter->pool_long->pool);

		/* update short pool in software */
		pool_short[adapter->port] = adapter->pool_short;
		adapter->pool_short = adapter->pool_long;

		set_bit(MV_ETH_F_IFCAP_NETMAP_BIT, &(adapter->flags));

	} else {
		unsigned long flags = 0;
		u_int pa, i;

		ifp->if_capenable &= ~IFCAP_NETMAP;
		ifp->netdev_ops = (void *)na->if_transmit;
		mv_pp2_rx_reset(adapter->port);

		/* TODO: handle SMP - each CPU must call this loop */
		/*
		for (txq = 0; txq < CONFIG_MV_PP2_TXQ; txq++)
			mvPp2TxqSentDescProc(adapter->port, 0, txq);
		*/

		i = 0;
		MV_ETH_LOCK(&adapter->pool_long->lock, flags);
		do {
			mvBmPoolGet(adapter->pool_long->pool, &pa);
			i++;
		} while (pa != 0);

		MV_ETH_UNLOCK(&adapter->pool_long->lock, flags);
		printk(KERN_ERR "NETMAP: free %d buffers from pool %d\n", i, adapter->pool_long->pool);
		mv_pp2_pool_add(adapter, adapter->pool_long->pool, pool_buf_num[adapter->pool_long->pool]);

		/* set port's short pool for Linux driver */
		for (rxq = 0; rxq < CONFIG_MV_PP2_RXQ; rxq++)
			mvPp2RxqBmShortPoolSet(adapter->port, rxq, adapter->pool_short->pool);

		/* update short pool in software */
		adapter->pool_short = pool_short[adapter->port];

		clear_bit(MV_ETH_F_IFCAP_NETMAP_BIT, &(adapter->flags));
	}

	if (mv_pp2_start(ifp)) {
		printk(KERN_ERR "%s: start interface failed\n", ifp->name);
		return -EINVAL;
	}
	return error;
}

/*
 * Reconcile kernel and user view of the transmit ring.
 */
static int
mv_pp2_netmap_txsync(struct ifnet *ifp, u_int ring_nr, int do_lock)
{
	struct SOFTC_T *adapter = MV_ETH_PRIV(ifp);
	struct netmap_adapter *na = NA(ifp);
	struct netmap_kring *kring = &na->tx_rings[ring_nr];
	struct netmap_ring *ring = kring->ring;
	u_int j, k, n = 0, lim = kring->nkr_num_slots - 1;
	u_int sent_n, cpu = 0;

	struct pp2_tx_desc *tx_desc;
	struct aggr_tx_queue *aggr_txq_ctrl = NULL;

	/* generate an interrupt approximately every half ring */
	/*int report_frequency = kring->nkr_num_slots >> 1;*/
	/* take a copy of ring->cur now, and never read it again */
	k = ring->cur;
	if (k > lim)
		return netmap_ring_reinit(kring);

	aggr_txq_ctrl = &aggr_txqs[cpu];

	if (do_lock)
		mtx_lock(&kring->q_lock);

	rmb();
	/*
	 * Process new packets to send. j is the current index in the
	 * netmap ring, l is the corresponding index in the NIC ring.
	 */
	j = kring->nr_hwcur;
	if (j != k) {	/* we have new packets to send */
		for (n = 0; j != k; n++) {
			/* slot is the current slot in the netmap ring */
			struct netmap_slot *slot = &ring->slot[j];

			uint64_t paddr;
			void *addr = PNMB(slot, &paddr);
			u_int len = slot->len;

			if (addr == netmap_buffer_base || len > NETMAP_BUF_SIZE) {
				if (do_lock)
					mtx_unlock(&kring->q_lock);
				return netmap_ring_reinit(kring);
			}

			slot->flags &= ~NS_REPORT;

			/* check aggregated TXQ resource */
			if (mv_pp2_aggr_desc_num_check(aggr_txq_ctrl, 1)) {
				if (do_lock)
					mtx_unlock(&kring->q_lock);
				return netmap_ring_reinit(kring);
			}

			tx_desc = mvPp2AggrTxqNextDescGet(aggr_txq_ctrl->q);
			tx_desc->physTxq = MV_PPV2_TXQ_PHYS(adapter->port, 0, ring_nr);
			tx_desc->bufPhysAddr = (uint32_t)(paddr);
			tx_desc->dataSize = len;
			tx_desc->pktOffset = slot->data_offs;
			tx_desc->command = PP2_TX_L4_CSUM_NOT | PP2_TX_F_DESC_MASK | PP2_TX_L_DESC_MASK;
			mv_pp2_tx_desc_flush(adapter, tx_desc);
			aggr_txq_ctrl->txq_count++;

			if (slot->flags & NS_BUF_CHANGED)
				slot->flags &= ~NS_BUF_CHANGED;

			j = (j == lim) ? 0 : j + 1;
		}
		kring->nr_hwcur = k; /* the saved ring->cur */
		kring->nr_hwavail -= n;

		wmb(); /* synchronize writes to the NIC ring */

		/* Enable transmit */
		sent_n = n;
		while (sent_n > 0xFF) {
			mvPp2AggrTxqPendDescAdd(0xFF);
			sent_n -= 0xFF;
		}
		mvPp2AggrTxqPendDescAdd(sent_n);
	}

	if (n == 0 || kring->nr_hwavail < 1) {
		int delta;

		delta = mvPp2TxqSentDescProc(adapter->port, 0, ring_nr);
		if (delta)
			kring->nr_hwavail += delta;
	}
	/* update avail to what the kernel knows */
	ring->avail = kring->nr_hwavail;

	if (do_lock)
		mtx_unlock(&kring->q_lock);

	return 0;
}


/*
 * Reconcile kernel and user view of the receive ring.
 */
static int
mv_pp2_netmap_rxsync(struct ifnet *ifp, u_int ring_nr, int do_lock)
{
	struct SOFTC_T *adapter = MV_ETH_PRIV(ifp);
	struct netmap_adapter *na = NA(ifp);

	MV_PP2_PHYS_RXQ_CTRL *rxr = adapter->rxq_ctrl[ring_nr].q;

	struct netmap_kring *kring = &na->rx_rings[ring_nr];
	struct netmap_ring *ring = kring->ring;
	u_int j, l, n;

	int force_update = do_lock || kring->nr_kflags & NKR_PENDINTR;

	uint16_t strip_crc = (1) ? 4 : 0; /* TBD :: remove CRC or not */

	u_int lim   = kring->nkr_num_slots - 1;
	u_int k     = ring->cur;
	u_int resvd = ring->reserved;
	u_int rx_done;

	if (k > lim)
		return netmap_ring_reinit(kring);

	if (do_lock)
		mtx_lock(&kring->q_lock);

	/* hardware memory barrier that prevents any memory read access from being moved */
	/* and executed on the other side of the barrier */
	rmb();

	/*
	 * Import newly received packets into the netmap ring.
	 * j is an index in the netmap ring, l in the NIC ring.
	*/
	l = rxr->queueCtrl.nextToProc;
	j = netmap_idx_n2k(kring, l); /* map NIC ring index to netmap ring index */

	if (netmap_no_pendintr || force_update) { /* netmap_no_pendintr = 1, see netmap.c */
		/* Get number of received packets */
		rx_done = mvPp2RxqBusyDescNumGet(adapter->port, ring_nr);
		mvOsCacheIoSync();
		rx_done = (rx_done >= lim) ? lim - 1 : rx_done;
		for (n = 0; n < rx_done; n++) {
			PP2_RX_DESC *curr = (PP2_RX_DESC *)MV_PP2_QUEUE_DESC_PTR(&rxr->queueCtrl, l);

#if defined(MV_CPU_BE)
			mvPPv2RxqDescSwap(curr);
#endif /* MV_CPU_BE */

			/* TBD : check for ERRORs */
			ring->slot[j].len = (curr->dataSize) - strip_crc - MV_ETH_MH_SIZE;
			ring->slot[j].data_offs = NET_SKB_PAD + MV_ETH_MH_SIZE;
			ring->slot[j].buf_idx = curr->bufCookie;
			ring->slot[j].flags |= NS_BUF_CHANGED;

			j = (j == lim) ? 0 : j + 1;
			l = (l == lim) ? 0 : l + 1;
		}
		if (n) { /* update the state variables */
			struct napi_group_ctrl *napi_group;

			rxr->queueCtrl.nextToProc = l;
			kring->nr_hwavail += n;
			mvPp2RxqOccupDescDec(adapter->port, ring_nr, n);

			/* enable interrupts */
			wmb();
			napi_group = adapter->cpu_config[smp_processor_id()]->napi_group;
			mvPp2GbeCpuInterruptsEnable(adapter->port, napi_group->cpu_mask);
		}
		kring->nr_kflags &= ~NKR_PENDINTR;
	}

	/* skip past packets that userspace has released */
	j = kring->nr_hwcur; /* netmap ring index */
	if (resvd > 0) {
		if (resvd + ring->avail >= lim + 1) {
			printk(KERN_ERR "XXX invalid reserve/avail %d %d", resvd, ring->avail);
			ring->reserved = resvd = 0;
		}
		k = (k >= resvd) ? k - resvd : k + lim + 1 - resvd;
	}

	if (j != k) { /* userspace has released some packets. */
		l = netmap_idx_k2n(kring, j); /* NIC ring index */
		for (n = 0; j != k; n++) {
			struct netmap_slot *slot = &ring->slot[j];
			PP2_RX_DESC *curr = (PP2_RX_DESC *)MV_PP2_QUEUE_DESC_PTR(&rxr->queueCtrl, l);
			uint64_t paddr;
			uint32_t *addr = PNMB(slot, &paddr);

			/*
			In big endian mode:
			we do not need to swap descriptor here, allready swapped before
			*/

			slot->data_offs = NET_SKB_PAD + MV_ETH_MH_SIZE;
			if (addr == (uint32_t *)netmap_buffer_base) { /* bad buf */
				if (do_lock)
					mtx_unlock(&kring->q_lock);

				return netmap_ring_reinit(kring);
			}
			if (slot->flags & NS_BUF_CHANGED) {
				slot->flags &= ~NS_BUF_CHANGED;

				mvBmPoolPut(adapter->pool_long->pool, (uint32_t)paddr, curr->bufCookie);
			}
			curr->status = 0;
			j = (j == lim) ? 0 : j + 1;
			l = (l == lim) ? 0 : l + 1;
		}
		kring->nr_hwavail -= n;
		kring->nr_hwcur = k;
		/* hardware memory barrier that prevents any memory write access from being moved and */
		/* executed on the other side of the barrier.*/
		wmb();
		/*
		 * IMPORTANT: we must leave one free slot in the ring,
		 * so move l back by one unit
		 */
		l = (l == 0) ? lim : l - 1;
		mvPp2RxqNonOccupDescAdd(adapter->port, ring_nr, n);
	}
	/* tell userspace that there are new packets */
	ring->avail = kring->nr_hwavail - resvd;

	if (do_lock)
		mtx_unlock(&kring->q_lock);

	return 0;
}


/* diagnostic routine to catch errors */
static void mv_pp2_no_rx_alloc(struct SOFTC_T *a, int n)
{
	printk("mv_pp2_skb_alloc should not be called");
}

/*
 * Make the rx ring point to the netmap buffers.
 */
static int pp2_netmap_rxq_init_buffers(struct SOFTC_T *adapter, int rxq)
{
	struct ifnet *ifp = adapter->dev; /* struct net_devive */
	struct netmap_adapter *na = NA(ifp);
	struct netmap_slot *slot;
	struct rx_queue   *rxr;

	int i, si;
	uint64_t paddr;
	uint32_t *vaddr;

	if (!(adapter->flags & MV_ETH_F_IFCAP_NETMAP))
		return 0;

	/* initialize the rx ring */
	slot = netmap_reset(na, NR_RX, rxq, 0);
	if (!slot) {
		printk(KERN_ERR "%s: RX slot is null\n", __func__);
		return 1;
	}
	rxr = &(adapter->rxq_ctrl[rxq]);

	for (i = 0; i < rxr->rxq_size; i++) {
		si = netmap_idx_n2k(&na->rx_rings[rxq], i);
		vaddr = PNMB(slot + si, &paddr);
		/* printk(KERN_ERR "paddr = 0x%x, virt = 0x%x\n",
				(uint32_t)paddr,  (uint32_t)((slot+si)->buf_idx));*/

		/* TODO: use mvBmPoolQsetPut in ppv2.1 */
		mvBmPoolPut(adapter->pool_long->pool, (uint32_t)paddr, (uint32_t)((slot+si)->buf_idx));

	}
	rxr->q->queueCtrl.nextToProc = 0;
	/* Force memory writes to complete */
	wmb();
	return 0;
}


/*
 * Make the tx ring point to the netmap buffers.
*/
static int pp2_netmap_txq_init_buffers(struct SOFTC_T *adapter, int txp, int txq)
{
	struct ifnet *ifp = adapter->dev;
	struct netmap_adapter *na = NA(ifp);
	struct netmap_slot *slot;
	int q;

	if (!(adapter->flags & MV_ETH_F_IFCAP_NETMAP))
		return 0;

	q = txp * CONFIG_MV_PP2_TXQ + txq;

	/* initialize the tx ring */
	slot = netmap_reset(na, NR_TX, q, 0);

	if (!slot) {
		printk(KERN_ERR "%s: TX slot is null\n", __func__);
		return 1;
	}

	return 0;
}


static void
mv_pp2_netmap_attach(struct SOFTC_T *adapter)
{
	struct netmap_adapter na;

	bzero(&na, sizeof(na));

	na.ifp = adapter->dev; /* struct net_device */
	na.separate_locks = 0;
	na.num_tx_desc = 256;
	na.num_rx_desc = adapter->rxq_ctrl->rxq_size;
	na.nm_register = mv_pp2_netmap_reg;
	na.nm_txsync = mv_pp2_netmap_txsync;
	na.nm_rxsync = mv_pp2_netmap_rxsync;
	na.num_tx_rings = CONFIG_MV_PP2_TXQ;
	netmap_attach(&na, CONFIG_MV_PP2_RXQ);
}
/* end of file */

#endif  /* __MV_PP2_NETMAP_H__ */
