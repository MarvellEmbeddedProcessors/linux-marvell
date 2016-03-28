/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#ifndef __mv_pp3_swq_h__
#define __mv_pp3_swq_h__

/* Minimum size in packets for RX SWQ */
#define MV_PP3_RX_SWQ_SIZE_MIN	128

/************************/
/* SWQs structures      */
/************************/

struct pp3_swq_stats {
	uint64_t pkts;
	uint64_t bytes;
	uint64_t pkts_drop;
	uint64_t pkts_errors;
	uint64_t suspend;
	uint64_t resumed;
};

/* HMAC RX SWQ structure */
struct pp3_rx_swq {
	int	node_id;	/* node_id for BP from HMAC to QM */
	int	node_type;	/* node type for BP from HMAC to QM */
	int	pkt_coal;	/* RX packet coalecing [pkts] */
	int	time_prof;	/* RX time coalescing profile */
	int	irq_group;	/* RX irq group */
};

/* HMAC TX SWQ structure */
struct pp3_tx_swq {
	int	hwq;		/* connected HWQ */
};


struct pp3_swq {
	int			frame_num;	/* HMAC frame number */
	int			swq;		/* HMAC SWQ number [0..15] */
	int			cur_size;	/* current queue size in packets */
	int			cfh_dg_size;	/* cfh size in DG for one packet */
	struct pp3_swq_stats	stats;		/* SWQ statistics */
	bool			stats_reset_flag;/* statistic counters after reset flag */
	union {
		struct pp3_rx_swq rx;
		struct pp3_tx_swq tx;
	} queue;
};

/************************/
/*   HMAC SWQs APIs     */
/************************/
struct pp3_swq *mv_pp3_swq_alloc(void);

int mv_pp3_rx_swq_sw_init(struct pp3_swq *rx_swq, int frame, int swq, int hwq, int irq_group);
int mv_pp3_tx_swq_sw_init(struct pp3_swq *tx_swq, int frame, int swq, int hwq);

int mv_pp3_rx_swq_hw_init(struct pp3_swq *rx_swq);
int mv_pp3_tx_swq_hw_init(struct pp3_swq *tx_swq);

void mv_pp3_swq_delete(struct pp3_swq *swq);

void mv_pp3_swq_cnt_dump_header(int num);
void mv_pp3_swq_cnt_dump(const char *cntr_pref, int cpu, struct pp3_swq **q, int num, bool pr_cntr_name);
void mv_pp3_swq_stats_clear(struct pp3_swq *swq);

int mv_pp3_swq_cfh_size_set(struct pp3_swq *swq, int dg_size);

int mv_pp3_rx_swq_size_set(struct pp3_swq *swq, int pkts);
int mv_pp3_tx_swq_size_set(struct pp3_swq *swq, int pkts);
int mv_pp3_swq_rx_pkt_coal_set(struct pp3_swq *swq, int pkts_num);
int mv_pp3_swq_rx_time_prof_set(struct pp3_swq *swq, int prof);
int mv_pp3_swq_rx_time_coal_set(struct pp3_swq *swq, int usec);

#endif /* __mv_pp3_swq_h__ */
