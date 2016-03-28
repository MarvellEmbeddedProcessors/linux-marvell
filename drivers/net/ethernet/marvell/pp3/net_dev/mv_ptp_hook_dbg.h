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

/* This is DEBUG file for PTP HW verification but never in real system
 * Used as INCLUDE into the "mv_ptp_hook.c" file!
 *
 * Refer mv_ptp_hook_extra_op() for debug configuration & control
 *
 * PTP_DELAY_TRACE
 * PTP_ALL_RX_TS_DBG_CAPTURE
 * PTP_PACKET_DUMP
 */

#ifndef PTP_DELAY_TRACE
#define PTP_TX_TS_DBG(IN_QUEUE)
#define PTP_RX_TS_DBG(pDATA, OFFS, RX32B)
#define PTP_DELAY_TRACE_CFG_DBG(V1, V2, V3, RC)
#endif

#ifndef PTP_ALL_RX_TS_DBG_CAPTURE
#define PTP_RX_TS_DBG_CAPTURE(IDX, LEN, TS)
#define PTP_RX_TS_DBG_PRINT(C, RC)
#define PTP_RX_TS_DBG_CFG(val1, val2, val3)	0
#endif


/*******************************************************************************
 * Delay Req/Resp roundtrip measurement;
 * trace: TX-TimeStamp, RX-packet-TS, RX-TS
 */
#ifdef PTP_DELAY_TRACE
static bool pp3_ptp_dbg_promisc;
static int pp3_ptp_dbg_tai_reset;
static u32 pp3_ptp_dbg_enable_cntr;
static struct mv_pp3_tai_tod ts_tx;
/*static u64 kclock_tx;*/

static inline void PTP_DELAY_TRACE_CFG_DBG(u32 val1, u32 val2, u32 val3, int *rc)
{
	if (val1 == 0xd/*d=Delay measurement*/) {
		pp3_ptp_dbg_enable_cntr = val2 * 2; /* Req and Resp */
		pp3_ptp_dbg_promisc = val3 & 1;
		pp3_ptp_dbg_tai_reset = val3 >> 1;
		/* DELAY_REQ rate=16/sec, SYNC rate=64/sec */
		if (pp3_ptp_dbg_promisc)
			pp3_ptp_dbg_enable_cntr += val2 * 4;
		*rc = 0;
	}
	pr_info("echo deb d NN [resetCntr|sync] > [D]elay Req/Resp 0x%x 0x%x\n",
		pp3_ptp_dbg_enable_cntr, pp3_ptp_dbg_promisc | (pp3_ptp_dbg_tai_reset << 1));
}

static inline void PTP_TX_TS_DBG(int in_queue)
{
	if (!pp3_ptp_dbg_enable_cntr || !in_queue)
		return;
	/*kclock_tx = local_clock();*/
	if (pp3_ptp_dbg_tai_reset) {
		pp3_ptp_dbg_tai_reset--;
		memset(&ts_tx, 0, sizeof(ts_tx));
		mv_pp3_tai_tod_op(MV_TAI_SET_UPDATE, &ts_tx, 0);
	} else {
		mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, &ts_tx, 0);
	}
	pp3_ptp_dbg_enable_cntr--;
}

static inline void PTP_RX_TS_DBG(u8 *pkt_data, int ptp_ts_offs, u32 rx32b)
{
	u8 msg_type = pkt_data[ptp_ts_offs - 16] & 0x0f;
	static struct mv_pp3_tai_tod ts_rx;
	bool is_delay_resp;

	if (!pp3_ptp_dbg_enable_cntr)
		return;
	is_delay_resp = (msg_type == PTP_DELAY_RESP);
	if (!is_delay_resp && !pp3_ptp_dbg_promisc)
		return;
	/* u32 d = (u32)(local_clock() - kclock_tx);  * get before TAI clock */
	mv_pp3_tai_tod_op(MV_TAI_GET_CAPTURE, &ts_rx, 0);
	if (!is_delay_resp) {
		pr_info("%x_%s *****.********* (%u.%09u) %u.%09u\n",
			msg_type, (msg_type == PTP_SYNC) ? "SYNC" : "misc",
			/*FromGOP*/ rx32b >> 30, rx32b & 0x3fffffff,
			ts_rx.sec_lsb_32b & 0xffff, ts_rx.nsec);
	} else {
		pr_info(" DELAY %5u.%09u (%u.%09u) %u.%09u\n",
			ts_tx.sec_lsb_32b & 0xffff, ts_tx.nsec,
			/*FromGOP*/ rx32b >> 30, rx32b & 0x3fffffff,
			ts_rx.sec_lsb_32b & 0xffff, ts_rx.nsec);
	}
	pp3_ptp_dbg_enable_cntr--;
}
#endif /*PTP_DELAY_TRACE*/


/*******************************************************************************
 * All Ingress packets have 32bits timestamp in CFH->tag2
 * Captire all packets from all ports into buffer to check timestamp consistency
 */
#ifdef PTP_ALL_RX_TS_DBG_CAPTURE
#warning PTP_ALL_RX_TS_DBG_CAPTURE enabled

#define SNIFF_TS_INCONSISTENCY_ON_RANGE

#ifdef SNIFF_TS_INCONSISTENCY_ON_RANGE
#define RX_TS_DBG_SIZE	128

struct rx_ts_dbg_s {
	u32 prev_ts;
	u32 ts;
	u32 diff;
};
static struct rx_ts_dbg_s rx_ts_dbg[RX_TS_DBG_SIZE];
static u32 sniff_ts_inconsistency_port_no = 1; /* on port 1 */
static int post_prt_cntr;
static int sniff_min = 12276; /* This is for 100% 1Gb packets 1518 */
static int sniff_max = 12360;

#else /*SNIFF_TS_INCONSISTENCY_ON_RANGE*/

#define RX_TS_DBG_SIZE	1024

struct rx_ts_dbg_s {
	u16 port;
	u16 len;
	u32 ts;
};
static struct rx_ts_dbg_s rx_ts_dbg[RX_TS_DBG_SIZE];
static u32 sniff_ts_inconsistency_port_no;

#endif /*SNIFF_TS_INCONSISTENCY_ON_RANGE*/

static u32 rx_ts_dbg_idx;



static inline u32 convert2nnsec(u32 sec, u32 nsec)
{
	/*     avoid warning    { 0, 1000000000, 2000000000, 3000000000 */
	const u32 sec2nsec[4] = { 0, 0x3b9aca00, 0x77359400, 0xb2d05e00 };
	u32 nnsec = sec2nsec[sec] + nsec;
	return nnsec;
}

static int PTP_RX_TS_DBG_CFG(u32 val1, u32 val2, u32 val3)
{
	int rc = 0;
	if (val1 == 0x5) {
		sniff_ts_inconsistency_port_no = val2;
		rc = 1;
	}
	pr_info("echo deb 5 <1/3/0>  :%d=Sniff timestamp inconsistency on port 1/3\n",
		sniff_ts_inconsistency_port_no);
#ifdef SNIFF_TS_INCONSISTENCY_ON_RANGE
	if ((val1 == 0x5a) && val2 && val3) {
		sniff_min = val2;
		sniff_max = val3;
		rc = 1;
	}
	pr_info("echo deb 5a <min_nsec> <max_nsec> :Sniff on given range\n");
#endif
	return rc;
}

static inline int sniff_ts_inconsistency_f(int emac_idx, u32 ts)
{
#ifdef SNIFF_TS_INCONSISTENCY_ON_RANGE
	static u32 prev_ts, prev_nnsec;
	u32 sec, nsec, nnsec, diff;
	if (!sniff_ts_inconsistency_port_no || (emac_idx != sniff_ts_inconsistency_port_no))
		return 0;
	sec = ts >> 30;
	nsec = ts & 0x3fffffff;
	nnsec =  convert2nnsec(sec, nsec);
	diff = nnsec - prev_nnsec;
	if (post_prt_cntr) {
		post_prt_cntr--;
		if (rx_ts_dbg_idx < RX_TS_DBG_SIZE) {
			rx_ts_dbg[rx_ts_dbg_idx].prev_ts = ts;
			rx_ts_dbg_idx++;
		}
		goto exit;
	}
	if ((diff >= sniff_min) && (diff <= sniff_max))
		goto exit;
	if ((sec == 0) && ((prev_ts >> 30) == 3))
		goto exit;
	/* Do not print but save for print by sysfs command */
	if (rx_ts_dbg_idx < RX_TS_DBG_SIZE) {
		rx_ts_dbg[rx_ts_dbg_idx].prev_ts = prev_ts;
		rx_ts_dbg[rx_ts_dbg_idx].ts = ts;
		rx_ts_dbg[rx_ts_dbg_idx].diff = diff;
		rx_ts_dbg_idx++;
	}
	post_prt_cntr = 2;
exit:
	prev_ts = ts;
	prev_nnsec = nnsec;
#else
	static u32 prev_ts;
	if (!sniff_ts_inconsistency_port_no || (emac_idx != sniff_ts_inconsistency_port_no))
		return 0;
	if (prev_ts < ts)
		goto exit; /* all is ok */
	if (((prev_ts >> 30) == 3) && ((ts >> 30) == 0))
		goto exit; /*wrap 3.xxx to 0.xxx is ok */

	pr_err("PREV: %08x=%u.%09u, CURR: %08x=%u.%09u\n",
		prev_ts, prev_ts>>30, prev_ts & 0x3fffffff,
		     ts,      ts>>30,      ts & 0x3fffffff);
exit:
	prev_ts = ts;
#endif /*SNIFF_TS_INCONSISTENCY_ON_RANGE*/
	return 1;
}

static inline void PTP_RX_TS_DBG_CAPTURE(int emac_idx, int pkt_len, u32 ts)
{
	if (sniff_ts_inconsistency_f(emac_idx, ts))
		return;
#ifndef SNIFF_TS_INCONSISTENCY_ON_RANGE
	if (rx_ts_dbg_idx < RX_TS_DBG_SIZE) {
		rx_ts_dbg[rx_ts_dbg_idx].port = emac_idx;
		rx_ts_dbg[rx_ts_dbg_idx].len = pkt_len;
		rx_ts_dbg[rx_ts_dbg_idx].ts = ts;
		rx_ts_dbg_idx++;
	}
#endif
}

static void PTP_RX_TS_DBG_PRINT(int clear_stats_only, int rc_code)
{
	u32 i, prev, sec, nsec, nnsec, prev_nnsec;
	struct rx_ts_dbg_s *p;

	if (rc_code >= 0x10)
		return;

	rx_ts_dbg_idx = RX_TS_DBG_SIZE; /* disable capturing */

	if (clear_stats_only)
		goto clear_exit;

	p = &rx_ts_dbg[0];
#ifdef SNIFF_TS_INCONSISTENCY_ON_RANGE
	prev = prev;
	sec = sec;
	nsec = nsec;
	nnsec = nnsec;
	prev_nnsec = prev_nnsec;
	i = 0;
	for (i = 0; i < RX_TS_DBG_SIZE; i++) {
		if (p->prev_ts == 0) {
			pr_err("=== RX TS CAPTURE END on %d ===\n", i);
			goto clear_exit;
		}
		pr_err("%08x=%u.%09u - %08x=%u.%09u = %6u\n",
			p->prev_ts, p->prev_ts >> 30, p->prev_ts & 0x3fffffff,
			p->ts, p->ts >> 30, p->ts & 0x3fffffff,
			p->diff);
		p++;
	}
#else /*SNIFF_TS_INCONSISTENCY_ON_RANGE*/
	i = 0;
	if (rx_ts_dbg[i].len) {
		sec = rx_ts_dbg[i].ts >> 30;
		nsec = rx_ts_dbg[i].ts & 0x3fffffff;
		pr_err("%d: len=%-4d, ts=%08x=%u.%09u\n",
			rx_ts_dbg[i].port, rx_ts_dbg[i].len,
			rx_ts_dbg[i].ts, sec, nsec);
	}
	prev = 0;
	i = 1;
	while (i < RX_TS_DBG_SIZE) {
		if (rx_ts_dbg[i].len == 0) {
			pr_err("=== RX TS CAPTURE END on %d ===\n", i);
			goto clear_exit;
		}
		sec = rx_ts_dbg[prev].ts >> 30;
		nsec = rx_ts_dbg[prev].ts & 0x3fffffff;
		prev_nnsec = convert2nnsec(sec, nsec);

		sec = rx_ts_dbg[i].ts >> 30;
		nsec = rx_ts_dbg[i].ts & 0x3fffffff;
		nnsec =  convert2nnsec(sec, nsec);

		pr_err("%d: len=%-4d, ts=%08x=%u.%09u (%6u)\n",
			rx_ts_dbg[i].port, rx_ts_dbg[i].len,
			rx_ts_dbg[i].ts, sec, nsec,
			nnsec - prev_nnsec);
		i++;
		prev++;
	}
#endif /*SNIFF_TS_INCONSISTENCY_ON_RANGE*/
clear_exit:
	memset(rx_ts_dbg, 0, sizeof(rx_ts_dbg));
	rx_ts_dbg_idx = 0;
}
#endif/*PTP_ALL_RX_TS_DBG_CAPTURE*/


/*******************************************************************************
 *  General purpose packet dump utility
 */
#ifdef PTP_PACKET_DUMP
static void pp3_pack_dump(char *desc, char *data, int offs_begin, int pack_len)
{
	char buf[100];
	int pos, i, k;
	if (!desc)
		desc = " ";
	pr_info("--- packet dump:%s: offs=%d, pack-len=%d ---\n",
		desc, offs_begin, pack_len);
	i = offs_begin;
	do {
		/* Prepare and print 1 line */
		for (pos = 0, k = 0;  k < 16; k++) {
			if (k == 8)
				pos += sprintf(buf + pos, " ");
			pos += sprintf(buf + pos, " %02X", data[i]);
			if (++i == pack_len)
				break;
		}
		pr_info("%s\n", buf);
		/* continue on another line */
	} while (i < pack_len);
}
#endif/*PTP_PACKET_DUMP*/
