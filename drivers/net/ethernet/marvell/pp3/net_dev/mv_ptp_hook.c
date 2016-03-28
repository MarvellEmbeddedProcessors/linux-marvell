/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

*******************************************************************************/
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/list.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>

#include "mv_netdev_structs.h"

#define DBG_PTP_TS(FMT, VV...)	pr_info(FMT, ##VV)

#define PTP_PORT_EVENT	319 /* for PTP RX and TX */
#define PTP_PORT_SIGNAL	320 /* for PTP TX only */
/* PTP message-IDs having special TX handling */
#define PTP_SYNC	0x00
#define PTP_DELAY_REQ	0x01
#define PTP_PEER_DELAY_REQ	0x02
#define PTP_DELAY_RESP	0x09
#define PTP_ANNOUNCE	0x0b

#define PTP_HEADER_OFFS	(44)
#define PTP_HEADER_MSG_ID_OFFS	(0)
#define PTP_HEADER_RESERVE_1BYTES_OFFS	(5)
#define PTP_HEADER_CORRECTION_FIELD_OFFS	(8) /*size 6+2bytes */
#define PTP_HEADER_RESERVE_4BYTES_OFFS	(16)
#define PTP_CORRECTION_PRIVATE_ID	0xa5

/* PTP check upon MAX-lenght statistically is most effective.
 * The skb->data_len is already known ZERO, so skb->len is enought
 * MAX=Announce_64bytes + MV_MH_SIZE + IPv4_42 + IPv6_20 + VLAN_2 =
 *  64 + 44 + 20 + 2 = 64 + 66
 */
#define MAX_PTP_UDP_LEN	80
#define MAX_PTP_PKT_LEN	(MAX_PTP_UDP_LEN + 66)

#define PTP_TS_CS_CORRECTION_SIZE	2

/* FEATURES */
#define PTP_FILLER_CONTROL
#define PTP_IGNORE_TIMESTAMPING_FLAG_FOR_DEBUG
#define PTP_TS_TRAFFIC_CORRECTION

/**** DEBUG and HW-testing */
/*#define PTP_ALL_RX_TS_DBG_CAPTURE*/
#if defined(PTP_ALL_RX_TS_DBG_CAPTURE) || defined(PTP_DELAY_TRACE)
#include "mv_ptp_hook_dbg.h"
#else
/* Empty fillers for no-debug case */
#define PTP_RX_TS_DBG_CAPTURE(IDX, LEN, TS)
#define PTP_RX_TS_DBG_PRINT(C, RC)
#define PTP_RX_TS_DBG_CFG(val1, val2, val3)	0
#define PTP_TX_TS_DBG(IN_QUEUE)
#define PTP_RX_TS_DBG(pDATA, OFFS, RX32B)
#define PTP_DELAY_TRACE_CFG_DBG(V1, V2, V3, RC)
#endif
/**/

struct pp3_ptp_stats {
	u32 tx;
	u32 tx_capture_in_queue;
	u32 rx;
};

struct pp3_ptp_desc {
	int	emac_num; /* shortcut for ->vport->port.emac.emac_num */
	struct pp3_ptp_stats *stats;
};

static inline struct pp3_dev_priv *mv_pp3_emac_dev_priv_get(int emac_num);

/**** Net-dev hook Utilities (used as static/inline) ******************
 *  mv_pp3_is_pkt_ptp_rx_proc()   - filter PTP & processing
 *  mv_pp3_is_pkt_ptp_tx()        - filter PTP
 *  mv_pp3_send_filler_pkt_cfh()  - processing step 1
 *  mv_pp3_ptp_pkt_proc_tx()      - processing step 1
 *
 *  mv_pp3_pkt_ptp_stats()        - statistic (private PTP)
 *  mv_ptp_hook_enable()          - enable and statistic print
 *  mv_ptp_hook_extra_op()        - extra operations for debug
***********************************************************************/

static struct pp3_ptp_desc	pp3_ptp_desc[MV_PP3_EMAC_NUM];
static struct pp3_ptp_stats	pp3_ptp_stats[MV_PP3_EMAC_NUM];

static inline void mv_pp3_pkt_ptp_stats(int port, int reset)
{
	struct pp3_ptp_stats *p = &pp3_ptp_stats[port];
	if (reset) {
		memset(p, 0, sizeof(struct pp3_ptp_stats));
	} else {
		/* STATS print-form optimization:
		 * MasterOnly should always have capture_in_queue=0
		 * SlaveOnly  should always have tx == capture_in_queue
		 * BoundaryClock=Master+Slave and so tx != capture_in_queue
		 */
		if ((p->tx == p->tx_capture_in_queue) || !p->tx_capture_in_queue)
			pr_info("ptp port %d stats: tx=%u ; rx=%u\n",
				port, p->tx, p->rx);
		else
			pr_info("ptp port %d stats: tx=%u (%u:Qcapture) ; rx=%u\n",
				port, p->tx, p->tx_capture_in_queue, p->rx);
	}
}

#ifdef PTP_FILLER_CONTROL
static u32 pp3_ptp_filler_enable; /* DISABLE by default*/
#endif


void mv_ptp_hook_extra_op(u32 val1, u32 val2, u32 val3)
{
	int i, rc = -1, clear_stats = 0;
#ifdef PTP_FILLER_CONTROL
	if (val1 == 0xf/*Filler*/) {
		pp3_ptp_filler_enable = val2;
		if (val3) {
			/* silent settting requested. Don't print but exit */
			return;
		}
		rc = 0;
	}
	pr_info("echo deb f .. > [F]iller enable = %d (1:without, 2/3:with TS, 0xFn:ADDRs save)\n",
		pp3_ptp_filler_enable);
#endif
	PTP_DELAY_TRACE_CFG_DBG(val1, val2, val3, &rc);

	if (val1 == 0xc/*Clear statistic*/) {
		clear_stats = 1;
		rc = 1;
	}
	pr_info("echo deb c > [C]lear statistic %d\n", clear_stats);

	rc |= PTP_RX_TS_DBG_CFG(val1, val2, val3);

	if (rc) {
		for (i = 0; i < MV_PP3_EMAC_NUM; i++)
			mv_pp3_pkt_ptp_stats(i, clear_stats);
#ifdef PTP_TS_TRAFFIC_CORRECTION
		pr_info(" PTP_TS_TRAFFIC_CORRECTION is enabled\n");
#endif
		PTP_RX_TS_DBG_PRINT(clear_stats, rc);
	}
}

void mv_ptp_hook_enable(int port, bool enable)
{
	struct pp3_dev_priv *dev_priv;

	if (port >= MV_PP3_EMAC_NUM)
		return;

	dev_priv = mv_pp3_emac_dev_priv_get(port);
	if (!dev_priv)
		return;
	/* New Request handling depends upon current state */
	if (!dev_priv->ptp_desc) {
		/* Currently disabled */
		if (enable) {
			mv_pp3_pkt_ptp_stats(port, 1); /* new session, reset only */
			dev_priv->ptp_desc = &pp3_ptp_desc[port]; /* hook enabling */
			dev_priv->ptp_desc->emac_num = port;
			dev_priv->ptp_desc->stats = &pp3_ptp_stats[port];
		}
	} else {
		/* Currently enabled */
		mv_pp3_pkt_ptp_stats(port, 0); /* print out accumulated */
		if (!enable)
			dev_priv->ptp_desc = NULL; /* hook disabling */
	}
}


/***************************************************************************
 **  Real-Time used utilities
 ***************************************************************************
 */
static inline int ptp_get_emac_num(struct pp3_dev_priv *dev_priv)
{
	return dev_priv->ptp_desc->emac_num;
}

static inline u32 ptp_MV_CFH_HWQ_SET(struct pp3_vq  *tx_vq, bool high_queue)
{
	u32 tag1 = 0;
	/** Currently not supported
	if (high_queue)
		tag1 = MV_CFH_HWQ_SET( tx_vq -> .. ->to_emac_hwq );
	**/
	return tag1;
}

static inline void ptp_raise_tx_q_priority(struct pp3_dev_priv *dev_priv,
	struct pp3_vport *cpu_vp, struct pp3_vq  **p_tx_vq, struct pp3_swq **p_tx_swq)
{
	/** Currently not supported
	*p_tx_vq = cpu_vp->tx_vqs[dev_priv->vport->tx_vqs_num - 1];
	*p_tx_swq = (*p_tx_vq)->swq;
	**/
}


#ifdef PTP_TS_TRAFFIC_CORRECTION
/* Under traffic long packets have transmission latency ~12us (on 1Gb link)
 * causing for PTP delay and TimeStamp-deviation 0..12us
 * Handle the case in a statistical algorithm and correct TS.
 * Use the fact that EVERY Ingress has TS-32bits in CFH
 * and pass the info to upper application.
 * The information is placed into "correction field".
 * This requires special PTP application to handle this non-standard.
 * Since the Standard correction-field is used for TransparentClock devices,
 * we need to place traffic-info only if received correction==0 and mark 0xA5
 * in 1byte-reserve to distinct standard/non-standard
 */
struct ptp_correction_field { /* correctionField not standard refill */
	u8 tx_factor;
	u8 rx_handled_burst_sz; /* Pkts handled before PTP in same napi-budget */
	u16 rx_prev_sz;
	u32 rx_prev_ts;
};

struct ptp_taffic_stats {
	int last_sz;
	u32 last_ts;
};
static struct ptp_taffic_stats ptp_taffic_rx_stats[MV_PP3_EMAC_NUM];

static inline void mv_pp3_rx_traffic_stats(int emac_num, int pkt_len, u32 ts)
{
	ptp_taffic_rx_stats[emac_num].last_ts = ts;
	ptp_taffic_rx_stats[emac_num].last_sz = pkt_len;
}

static inline void mv_pp3_rx_traffic_handle(int emac_num, int pkt_len,
	u8 *ptp_data, int rx_pkt_done)
{
	struct ptp_correction_field *cf;
	struct ptp_taffic_stats *s;
	u16 *p16 = (u16 *)(ptp_data + PTP_HEADER_CORRECTION_FIELD_OFFS + 4);
	if (*p16)
		return; /* Field is not empty. Do not touch */

	ptp_data[PTP_HEADER_RESERVE_1BYTES_OFFS] = PTP_CORRECTION_PRIVATE_ID;

	cf = (void *)(ptp_data + PTP_HEADER_CORRECTION_FIELD_OFFS);
	s = &ptp_taffic_rx_stats[emac_num];
	cf->rx_handled_burst_sz = (u8)rx_pkt_done;
	cf->rx_prev_sz = (u16)s->last_sz;
	cf->rx_prev_ts = s->last_ts;
}
#else
#define mv_pp3_rx_traffic_stats(IDX, LEN, TS)
#define mv_pp3_rx_traffic_handle(IDX, LEN, DATA, RX_DONE)
#endif/*PTP_TS_TRAFFIC_CORRECTION*/

static inline int mv_pp3_send_filler_pkt_cfh(struct pp3_dev_priv *dev_priv, u8 *pkt_data,
				int cpu, struct pp3_vq  **p_tx_vq, struct pp3_swq **p_tx_swq)
{
	/* Use filler with MAX possible len "built into" the CFH = 64bytes
	 * but with MaxMax FIFO = 16k-2
	 * It could have "any contents" but let's use UDP/PTP_319 DELAY_REQUEST
	 * cut into len 64 bytes.
	 * The MAC addresses should be any invalid Non Broadcust/Multicast; for debug
	 * could save valid MAC and IPv4 addresses from given PTP packet
	 * For debug only we could save-and-use real MACs and IPs from the packet
	 */
	#define DBG_FILLER_REPLACEABLE_SIZE	((12 + 2)/*l2sz*/ + 20/*IPv4sz*/)
	static u8 pkt_l2[62] = { 0x00, 0x00,  /* marvell header */
	/*02*/ 0x3c, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0xff, 0xff, 0xff, 0xff, 0xff, /*MAC dst/src*/
	/*14*/ 0x08, 0x00, 0x45, 0x00,	0x00, 0x30/*IPlen*/,
	/*20*/ 0xb6, 0x3e, 0x40, 0x00, 0x40, 0x11/*UDP*/, 0x2f, 0xc6,
	/*28*/ 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,	0xc0, 0xc0, /* IPv4 addr src/dst: 192.192.192.192 */
	/*36  =  (MV_MH_SIZE + 34:DBG_FILLER_REPLACEABLE_SIZE) */
	/*36*/ 0x01, 0x3f, 0x01, 0x3f, 0x00, 0x1c, 0xbf, 0x3b, /* UDP Port=319/319, Lenght(2), CS(2) */
	/*     0x_F -- PTP but Non-Standard MsgId */
	/*44*/ 0x0f, 0x02, 0x00, 0x12, 0x04, 0x00, /* TimeStamp may be placed below this ...*/
	/*50*/ 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xff, 0xff
	/*62*/
	};
	static u16 ptp_fp_len = 0x4000 - 2; /* 16k-2 */

	/* cpu = smp_processor_id() */
	/* p_tx_vq/p_tx_swq updated if raising priority */
	/* tx_swq = tx_vq->swq */
	struct pp3_swq *tx_swq = *p_tx_swq;
	struct pp3_vport *cpu_vp = dev_priv->cpu_vp[cpu];

	struct mv_cfh_common *cfh;
	unsigned char *cfh_pdata;
	int cfh_len_dg, pkt_len, rc = 0;

#ifdef PTP_FILLER_CONTROL
	if (!pp3_ptp_filler_enable)
		goto exit;
#endif

	cfh_len_dg = 96 / MV_PP3_CFH_DG_SIZE;

	/* get cfh */
	cfh = (struct mv_cfh_common *)mv_pp3_hmac_txq_next_cfh(tx_swq->frame_num,
								tx_swq->swq, cfh_len_dg);
	if (!cfh) {
		STAT_ERR(tx_swq->stats.pkts_errors++);
		rc = -1;
		goto exit;
	}
	cfh->plen_order = MV_CFH_PKT_LEN_SET(ptp_fp_len) | MV_CFH_REORDER_SET(REORD_NEW) |
		MV_CFH_LAST_BIT_SET;

	cfh->ctrl = MV_CFH_LEN_SET(cfh_len_dg * MV_PP3_CFH_DG_SIZE) |
		MV_CFH_MODE_SET(HMAC_CFH) | MV_CFH_PP_MODE_SET(PP_TX_PACKET);

	cfh->vm_bp = 0; /*Pool=0 but not MV_CFH_BPID_SET(1) */
	cfh->marker_l = 0;
	cfh->phys_l = 0;
	cfh->l3_l4_info = 0;

	cfh->tag1 = ptp_MV_CFH_HWQ_SET(*p_tx_vq, false) |
			MV_CFH_ADD_CRC_BIT_SET | MV_CFH_L2_PAD_BIT_SET;

#ifdef PTP_FILLER_CONTROL
	if (pp3_ptp_filler_enable & 0xf0) {
		/* Construct special DEBUG filler-buffer "pkt_l2" (IPv4 only)
		 * for HW-TS checking and measurement
		 */
		u8 tmp[4], *pf;

		pf = pkt_l2 + MV_MH_SIZE;
		/* Save MAC/IP addresses of original PTP into filler-buffer "pkt_l2" */
		memcpy(pf, pkt_data + MV_MH_SIZE, DBG_FILLER_REPLACEABLE_SIZE);

		if (pp3_ptp_filler_enable & 0x20) {
			/* Set MAC-dst Broadcast */
			/*pf = pkt_l2 + MV_MH_SIZE; already done*/
			memset(pf, 0xff, 6);
		}
		if (pp3_ptp_filler_enable & 0x40) {
			/* Swap IPv4 src/dst */
			pf = pkt_l2 + MV_MH_SIZE + 26;
			memcpy(tmp, pf, 4);
			memcpy(pf, pf + 4, 4);
			memcpy(pf + 4, tmp, 4);
		}
		if (pp3_ptp_filler_enable & 0x80) {
			/* Set real 62 lenght instead filler's huge size */
			ptp_fp_len = sizeof(pkt_l2);
		}
		pp3_ptp_filler_enable &= ~0xf0;
	}

	if (pp3_ptp_filler_enable > 1) {
		/* filler with TS and CS/CUE avoids latency ~5uSec */
		/*  TSE, QS, TS_off, CS_off, CUE, PACT, PF, WC, DE, ETS */
		/*   1,  0,    50,    60,    1(!), 4/6,  0,  0,  0,   0 */
		cfh->tag1 |= MV_CFH_PTP_TSE_SET;
		cfh->tag2 = MV_CFH_PTP_TS_OFF_SET(50 - MV_MH_SIZE); /*TS_off*/
		/* CUE: UDP-CS-Enable and CS_off (2bytes used) if needed *
		cfh->tag2 |= MV_CFH_PTP_CUE_SET(1) | MV_CFH_PTP_CS_OFF_SET(60 - MV_MH_SIZE);
		*/
		if (pp3_ptp_filler_enable > 2)
			cfh->tag2 |= MV_CFH_PTP_PACT_SET(6); /*PACT=AddTime2packet+Capture*/
		else
			cfh->tag2 |= MV_CFH_PTP_PACT_SET(4); /*PACT=AddTime2packet*/
	} else {
		cfh->tag2 = 0;
	}
#else
	cfh->tag2 = 0;
#endif

	/* copy packet to CFH */
	pkt_len = sizeof(pkt_l2) - 2;  /* not including MV_MH_SIZE */
	cfh_pdata = (unsigned char *)cfh + MV_PP3_CFH_HDR_SIZE;
	memcpy(cfh_pdata, pkt_l2, sizeof(pkt_l2));

	/* transmit CFH */
	wmb();
	mv_pp3_hmac_txq_send(tx_swq->frame_num, tx_swq->swq, cfh_len_dg);

	DEV_PRIV_STATS(dev_priv, cpu)->tx_pkt_dev++;
	DEV_PRIV_STATS(dev_priv, cpu)->tx_bytes_dev += pkt_len;

	STAT_DBG(tx_swq->stats.pkts++);
	STAT_DBG(cpu_vp->port.cpu.stats.tx_bytes += pkt_len);
exit:
	ptp_raise_tx_q_priority(dev_priv, cpu_vp, p_tx_vq, p_tx_swq);
	return rc;
}

static inline void mv_pp3_is_pkt_ptp_rx_proc(struct pp3_dev_priv *dev_priv,
	struct mv_cfh_common *cfh,  int pkt_len, u8 *pkt_data, int rx_pkt_done)
{
	int eth_tag_len, dst_port_offs, ptp_offs, ptp_hdr_offs, emac_num;
	u16 ether_type, l4_port;
	enum mv_pp3_cfh_l3_info_rx l3_info;
	enum mv_pp3_cfh_l4_info_rx l4_info;
	u32 ts;

	if (!dev_priv->ptp_desc)
		return;

	emac_num = ptp_get_emac_num(dev_priv);

	if (pkt_len > MAX_PTP_PKT_LEN)
		goto exit;

	/* Check VLAN - needed for correct offset */
	eth_tag_len = (MV_CFH_VLAN_INFO_GET(cfh->l3_l4_info)) ? 2 : 0;

	/* Check in most-valuable ordering: port -> udpProto -> etherType */
	dst_port_offs = eth_tag_len + MV_MH_SIZE +
		+ MV_CFH_L3_OFFS_GET(cfh->l3_l4_info)
		+ MV_CFH_IPHDR_LEN_GET(cfh->l3_l4_info) * sizeof(u32)
		+ 2; /* +2 for RX only */
	l4_port = ntohs(*(u16 *)(pkt_data + dst_port_offs));
	if ((l4_port != PTP_PORT_EVENT) && (l4_port != PTP_PORT_SIGNAL))
		goto exit;

	l4_info = MV_CFH_L4_INFO_RX_GET(cfh->l3_l4_info);
	if (l4_info != L4_RX_UDP)
		goto exit;

	l3_info = MV_CFH_L3_INFO_RX_GET(cfh->l3_l4_info);
	if ((l3_info != L3_RX_IP4) && (l3_info != L3_RX_IP6))
		goto exit;

	ether_type = ntohs(*(u16 *)(pkt_data + 14 + eth_tag_len));
	if (ether_type == ETH_P_1588)
		goto exit; /*ETH_P_1588=0x88F7 is not supported by upper PTP layers*/

	/* Handling PTP packet: fetch TS32bits from cfh and place into PTP header */
	ts = cfh->tag2;

	ptp_hdr_offs = PTP_HEADER_OFFS + eth_tag_len;
	if (l3_info != L3_RX_IP4)
		ptp_hdr_offs += 20; /* IPV6 header +20 bytes */

	ptp_offs = ptp_hdr_offs + PTP_HEADER_RESERVE_4BYTES_OFFS;
	memcpy(pkt_data + ptp_offs, &ts, sizeof(ts));

	/*DBG_PTP_TS("ptp-rx: ts=%08x=%d.%09d\n", ts, ts >> 30, ts & 0x3fffffff);*/
	STAT_INFO(dev_priv->ptp_desc->stats->rx++);
	PTP_RX_TS_DBG(pkt_data, ptp_offs, ts);

	mv_pp3_rx_traffic_handle(emac_num, pkt_len, pkt_data + ptp_hdr_offs, rx_pkt_done);

	return;
exit:
	mv_pp3_rx_traffic_stats(emac_num, pkt_len, cfh->tag2);
	PTP_RX_TS_DBG_CAPTURE(emac_num, pkt_len, cfh->tag2);
}

static inline int mv_pp3_is_pkt_ptp_tx(struct pp3_dev_priv *dev_priv, struct sk_buff *skb, int *tx_ts_queue)
{
	u16 protocol, udp_port;
	int eth_tag_len;
	const int l2_hdr_len = 14;
	int ip_hdr_len; /* 20 or 40 for ipv4 or ipv6 */
	int skb_ip_len_lsb_offs;
	int skb_dst_port_offs/* UDP/L4: on offs=2 out of udpHdrLen=8 */;
	int skb_udp_len_lsb_offs;
	int skb_ptp_header_offs; /* Offset from skb-data beginning */
	int skb_ts_offs; /* OUT result: TimeStamp offset */
	const int ptp_ts_offs = 34; /* Offset from PTP-data beginning */
	u8 msg_type;

#ifndef PTP_IGNORE_TIMESTAMPING_FLAG_FOR_DEBUG
	if (!skb->sk)
		return 0;
	/* User should set for PTP/TX socket the sockopt
	 *  (SOL_SOCKET, SO_TIMESTAMPING, SOCK_TIMESTAMPING_TX_HARDWARE)
	 */
	if (!(skb->sk->sk_flags & (1 << SOCK_TIMESTAMPING_TX_HARDWARE)))
		return 0;
#endif
	if (!dev_priv->ptp_desc)
		return 0;

	if (skb->len > MAX_PTP_PKT_LEN)
		return 0;

	/* Check VLAN to obtain correct offset */
	if (skb->protocol == htons(ETH_P_8021Q)) {
		eth_tag_len = 2;
		protocol = vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
	} else {
		eth_tag_len = 0;
		protocol = skb->protocol;
	}
	/* Check IP v4 vs v6 */
	switch (protocol) {
	case htons(ETH_P_IP):
		ip_hdr_len = ip_hdr(skb)->ihl * sizeof(u32);
		skb_dst_port_offs = MV_MH_SIZE + eth_tag_len + l2_hdr_len + ip_hdr_len + 2;
		udp_port = *(u16 *)(skb->data + skb_dst_port_offs);
		if ((udp_port != htons(PTP_PORT_EVENT)) && (udp_port != htons(PTP_PORT_SIGNAL)))
			return 0;
		if (ip_hdr(skb)->protocol != IPPROTO_UDP)
			return 0;
		break;

	case htons(ETH_P_IPV6):
		ip_hdr_len = 20 + 20;
		skb_dst_port_offs = MV_MH_SIZE + eth_tag_len + l2_hdr_len + ip_hdr_len + 2;
		udp_port = *(u16 *)(skb->data + skb_dst_port_offs);
		if ((udp_port != htons(PTP_PORT_EVENT)) && (udp_port != htons(PTP_PORT_SIGNAL)))
			return 0;
		break;

	default:
		/* PTP_ETHER=0x887F is not supported */
		return 0;
	}
	skb_ptp_header_offs = skb_dst_port_offs + 6;
	skb_ts_offs = skb_ptp_header_offs + ptp_ts_offs;

	/* Capture TS into Queue for TX-Slave event messages only:
	 *   DELAY_REQ=1 and PEER_DELAY_REQ=2
	 */
	msg_type = skb->data[skb_ptp_header_offs] & 0x0f;

	if (udp_port == htons(PTP_PORT_EVENT)) {
		/* Capture TS into Queue for TX-Slave event messages only:
		 *   DELAY_REQ and PEER_DELAY_REQ */
		*tx_ts_queue = ((msg_type == PTP_DELAY_REQ) ||
			(msg_type == PTP_PEER_DELAY_REQ));
	} else if (msg_type == PTP_ANNOUNCE) {
		*tx_ts_queue = 0;
	} else {
		return 0;
	}
	/* PTP TX with FW TimeStamp update impacts the CheckSum.
	 * The FW does not fixes the CS but adds 2 bytes of correction-data
	 * to bring back the CS to be correct.
	 * This requires an additional 2 byte storage after TS-field.
	 * The SKB has enough storage, but the skb, UDP and IPvX lenght
	 * should be extended with these 2=PTP_TS_CS_CORRECTION_SIZE.
	 */
	skb_udp_len_lsb_offs = skb_dst_port_offs + 2 + 1;
	skb_ip_len_lsb_offs = MV_MH_SIZE + eth_tag_len + l2_hdr_len +
		(2 + 1)/*IPv4 TotalLen LSB*/;
	if (protocol == htons(ETH_P_IPV6))
		skb_ip_len_lsb_offs += 2;  /*IPv6 PayloadLen vs IPv4 TotalLen*/
	skb->data[skb_udp_len_lsb_offs] += PTP_TS_CS_CORRECTION_SIZE;
	skb->data[skb_ip_len_lsb_offs] += PTP_TS_CS_CORRECTION_SIZE;
	skb->len += PTP_TS_CS_CORRECTION_SIZE;

	return skb_ts_offs;
}

static inline void mv_pp3_ptp_pkt_proc_tx(struct pp3_dev_priv *dev_priv,
				struct mv_cfh_common *cfh, int skb_len, int skb_ts_offs, int tx_ts_queue)
{
	int pact, cfh_ts_offs, cfh_cs_offs;

	/* Convert skb offset to cfh offset (aka "TS off" field)
	 * Set PACT: 6={AddTime(to packet) + Capture(to egress queue)}
	 *   or only 4=AddTime according to the cfh-offset
	 */
	cfh_ts_offs = skb_ts_offs - MV_MH_SIZE;
	cfh_cs_offs = skb_len - sizeof(short) - MV_MH_SIZE;
	pact = (tx_ts_queue) ? 6 : 4;

	/*DBG_PTP_TS("ptp-tx: ts in_queue=%d, offs=%d\n", tx_ts_queue, cfh_ts_offs);*/
	STAT_INFO(dev_priv->ptp_desc->stats->tx++);
	if (tx_ts_queue)
		STAT_INFO(dev_priv->ptp_desc->stats->tx_capture_in_queue++);

	/* Add PTP related to TX CFH */
	/* TSE, QS, TS_off, CS_off, CUE, PACT, PF, WC, DE, ETS, SEC*/
	/*  1,   0,   76,     86,    1,   4,    0,  0,  0,  0,   0 */
	cfh->tag1 |= MV_CFH_PTP_TSE_SET; /*TSE*/
	/* MV_CFH_PTP_QS_SET(0) QueueSelect=0 */
	/* Reset Word3 with tag2 by "absolute" TS_OFF_SET */
	cfh->tag2 = MV_CFH_PTP_TS_OFF_SET(cfh_ts_offs); /*TS_off (not including MV_MH_SIZE!)*/
	cfh->tag2 |= MV_CFH_PTP_CS_OFF_SET(cfh_cs_offs); /*CS_off*/;
	cfh->tag2 |= MV_CFH_PTP_CUE_SET(1); /*CUE*/
	cfh->tag2 |= MV_CFH_PTP_PACT_SET(pact); /*PACT*/

	PTP_TX_TS_DBG(tx_ts_queue);
}
