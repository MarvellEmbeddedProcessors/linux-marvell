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

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "platform/mv_pp3_config.h"
#include "hmac/mv_hmac.h"

#include "mv_pp3_swq.h"


#define MV_PP3_DBG_SWQ_CNT_DUMP(swq, num, cpu, name, name_pref, pr_name, pr_zero)\
	mv_pp3_swq_cnt(swq, num, cpu, offsetof(struct pp3_swq_stats, name)/4, #name, name_pref, pr_name, pr_zero)

struct pp3_swq *mv_pp3_swq_alloc(void)
{
	struct pp3_swq *swq = kzalloc(sizeof(struct pp3_swq), GFP_KERNEL);

	return swq;
}
/*---------------------------------------------------------------------------*/


int mv_pp3_rx_swq_sw_init(struct pp3_swq *rx_swq, int frame, int swq, int hwq, int irq_group)
{
	MV_PP3_NULL_PTR(rx_swq, err);

	rx_swq->frame_num = frame;
	rx_swq->swq = swq;
	rx_swq->queue.rx.irq_group = irq_group;

	if (mv_pp3_cfg_rx_bp_node_get(hwq, &rx_swq->queue.rx.node_type, &rx_swq->queue.rx.node_id) < 0) {
		pr_err("%s: can't get back pressure parameters (hwq #%d)\n", __func__, hwq);
		goto err;
	}
	if (!rx_swq->cfh_dg_size)
		rx_swq->cfh_dg_size = MV_PP3_CFH_DG_MAX_NUM;

	if (!rx_swq->cur_size)
		rx_swq->cur_size = CONFIG_MV_PP3_RXQ_SIZE;

	if (!rx_swq->queue.rx.pkt_coal)
		rx_swq->queue.rx.pkt_coal = CONFIG_MV_PP3_RX_COAL_PKTS;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_tx_swq_sw_init(struct pp3_swq *tx_swq, int frame, int swq, int hwq)
{

	MV_PP3_NULL_PTR(tx_swq, err);

	tx_swq->frame_num = frame;
	tx_swq->swq = swq;
	tx_swq->queue.tx.hwq = hwq;

	if (!tx_swq->cur_size)
		tx_swq->cur_size = CONFIG_MV_PP3_TXQ_SIZE;

	if (!tx_swq->cfh_dg_size)
		tx_swq->cfh_dg_size = MV_PP3_CFH_DG_MAX_NUM;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_rx_swq_hw_init(struct pp3_swq *rx_swq)
{
	MV_PP3_NULL_PTR(rx_swq, err);

	/* HMAC RXQ allocated accordingly with maximum possible number of dg per packet */
	if (mv_pp3_hmac_rxq_init(rx_swq->frame_num, rx_swq->swq, rx_swq->cur_size * MV_PP3_CFH_DG_MAX_NUM) == NULL)
		goto err;

	mv_pp3_hmac_rxq_pkt_coal_set(rx_swq->frame_num, rx_swq->swq, rx_swq->queue.rx.pkt_coal * rx_swq->cfh_dg_size);
	mv_pp3_hmac_rxq_bp_node_set(rx_swq->frame_num, rx_swq->swq, rx_swq->queue.rx.node_type,
		rx_swq->queue.rx.node_id);

	if (mv_pp3_hmac_rxq_bp_thresh_set(rx_swq->frame_num, rx_swq->swq, rx_swq->cur_size * rx_swq->cfh_dg_size))
		goto err;

	mv_pp3_hmac_rxq_event_cfg(rx_swq->frame_num, rx_swq->swq, 0, rx_swq->queue.rx.irq_group);
	mv_pp3_hmac_rxq_enable(rx_swq->frame_num, rx_swq->swq);

	return 0;

err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_tx_swq_hw_init(struct pp3_swq *tx_swq)
{
	int dg_num;

	MV_PP3_NULL_PTR(tx_swq, err);

	/* HMAC TXQ allocated accordingly with maximum possible number of dg per packet */
	dg_num = tx_swq->cur_size * MV_PP3_CFH_DG_MAX_NUM;
	if (mv_pp3_hmac_txq_init(tx_swq->frame_num, tx_swq->swq, dg_num, 0) == NULL)
		goto err;

	mv_pp3_hmac_queue_qm_mode_cfg(tx_swq->frame_num, tx_swq->swq, tx_swq->queue.tx.hwq);
	mv_pp3_hmac_txq_enable(tx_swq->frame_num, tx_swq->swq);

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;

}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* set RX/TX SWQ CFH size in datagrames					     */
/*---------------------------------------------------------------------------*/
int mv_pp3_swq_cfh_size_set(struct pp3_swq *swq, int dg_size)
{
	MV_PP3_NULL_PTR(swq, err);

	if ((dg_size > MV_PP3_CFH_DG_MAX_NUM)  || (dg_size < 0)) {
		pr_err("%s: invalid CFH size %d, valid range in datagrames is [0, %d]\n",
			__func__, dg_size, MV_PP3_CFH_DG_MAX_NUM);
			goto err;
	}

	swq->cfh_dg_size = dg_size;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_swq_rx_pkt_coal_set(struct pp3_swq *swq, int pkts_num)
{
	mv_pp3_hmac_rxq_pkt_coal_set(swq->frame_num, swq->swq, pkts_num * swq->cfh_dg_size);

	swq->queue.rx.pkt_coal = pkts_num;
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_swq_rx_time_prof_set(struct pp3_swq *swq, int prof)
{
	mv_pp3_hmac_rxq_time_coal_profile_set(swq->frame_num, swq->swq, prof);

	swq->queue.rx.time_prof = prof;
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_swq_rx_time_coal_set(struct pp3_swq *swq, int usec)
{
	mv_pp3_hmac_frame_time_coal_set(swq->frame_num, swq->queue.rx.time_prof, usec);

	return 0;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* set RX/TX SWQ size in packets					     */
/*---------------------------------------------------------------------------*/
int mv_pp3_rx_swq_size_set(struct pp3_swq *swq, int pkts)
{
	int dg_num, rc;

	if (pkts < MV_PP3_RX_SWQ_SIZE_MIN) {
		pr_err("%s: RX SWQ size #%d [pkts] is too small. minimum is #%d [pkts]\n",
			__func__, pkts, MV_PP3_RX_SWQ_SIZE_MIN);
		return -1;
	}
	dg_num = pkts * swq->cfh_dg_size;

	rc = mv_pp3_hmac_rxq_bp_thresh_set(swq->frame_num, swq->swq, dg_num);
	if (rc)
		return -1;

	swq->cur_size = pkts;
	return 0;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_tx_swq_size_set(struct pp3_swq *swq, int pkts)
{
	int dg_num, rc;

	dg_num = pkts * swq->cfh_dg_size;
	rc = mv_pp3_hmac_txq_capacity_cfg(swq->frame_num, swq->swq, dg_num);
	if (rc)
		return -1;

	swq->cur_size = pkts;
	return 0;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_swq_delete(struct pp3_swq *rx_swq)
{
	kfree(rx_swq);
	/* TODO - free rxq memory, split HMAC function */
}
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
mv_pp3_queue_cnt
	print counter values from swq list
	calculate and print sum value for swq_num > 1
Inputs:
	swq        - array of VQueues pointers
	swq_num    - size of array
	cnt_index - counter index
	name      - counter name
	name_pref - preffix to be added to counter name
	pr_name   - if true, print counter name
	pr_zero   - if true, print zero counter values
---------------------------------------------------------------------------*/
static void mv_pp3_swq_cnt(struct pp3_swq **swq, int swq_num, int cpu, int cnt_index,
	const char *name, const char *name_pref, bool pr_name, bool pr_zero)
{
	int q, str_len = 0, sum = 0;
	unsigned int *stats;
	char str[200];
	int str_size = 200;
	bool print_flag;

	if (pr_name)
		str_len = snprintf(str + str_len, str_size - str_len, "%s%-12s%d%-8s", name_pref, name, cpu, "");
	else
		str_len = snprintf(str + str_len, str_size - str_len, "%-15s%d%-8s", "", cpu, "");

	print_flag = (pr_zero) ? true : false;

	for (q = 0; q < swq_num; q++) {
		if (swq[q]) {
			stats = (unsigned int *)&swq[q]->stats;
			sum += stats[cnt_index];
			str_len += snprintf(str + str_len, str_size - str_len, "%-10u     ", stats[cnt_index]);
			if (stats[cnt_index])
				print_flag |= true;
		} else
			str_len += snprintf(str + str_len, str_size - str_len, "%-10s     ", "NA");
	}

	if (swq_num > 1)
		str_len += snprintf(str + str_len, str_size - str_len, "%u\n", sum);
	else
		str_len += snprintf(str + str_len, str_size - str_len, "\n");

	if (print_flag)
		pr_cont("%s", str);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
mv_pp3_swq_cnt_dump_header
	print SWQ counters header
		swq_num - number of SWQs to be dump
---------------------------------------------------------------------------*/
void mv_pp3_swq_cnt_dump_header(int swq_num)
{
	char line1[200];
	char line2[200];
	int q;
	int str_len1;
	int str_len2;
	int str_size = 200;

	str_len1 = str_len2 = 0;
	str_len1 += sprintf(line1, "%-14scpu%-7s", "", "");
	str_len2 += sprintf(line2, "----------------------------------");

	for (q = 0; q < swq_num; q++) {
		str_len1 += snprintf(line1 + str_len1, str_size - str_len1, "swq%d %-10s", q, "");
		str_len2 += snprintf(line2 + str_len2, str_size - str_len2, "-----------------");
	}

	pr_cont("%s", line1);
	if (swq_num > 1)
		pr_cont("SUM");
	pr_info("%s\n", line2);
}

/*---------------------------------------------------------------------------
mv_pp3_swq_cnt_dump
	print swq counters according to number of swq_num
	print only if counter is not zeroed
	calculate and print sum value for swq_num > 1
	inputs:
		swq - array of CPU SWQs pointers
		cpu_swq_num - size of array
		cnt_index - stat counter index
		name - counter name
---------------------------------------------------------------------------*/
void mv_pp3_swq_cnt_dump(const char *cntr_pref, int cpu, struct pp3_swq **swq, int swq_num, bool pr_cntr_name)
{
	MV_PP3_DBG_SWQ_CNT_DUMP(swq, swq_num, cpu, pkts, cntr_pref, pr_cntr_name, true);
	MV_PP3_DBG_SWQ_CNT_DUMP(swq, swq_num, cpu, suspend, cntr_pref, pr_cntr_name, false);
	MV_PP3_DBG_SWQ_CNT_DUMP(swq, swq_num, cpu, resumed, cntr_pref, pr_cntr_name, false);
	MV_PP3_DBG_SWQ_CNT_DUMP(swq, swq_num, cpu, pkts_drop, cntr_pref, pr_cntr_name, false);
	MV_PP3_DBG_SWQ_CNT_DUMP(swq, swq_num, cpu, pkts_errors, cntr_pref, pr_cntr_name, false);
}
/*---------------------------------------------------------------------------*/

void mv_pp3_swq_stats_clear(struct pp3_swq *swq)
{
	memset(&swq->stats, 0, sizeof(struct pp3_swq_stats));
}
/*---------------------------------------------------------------------------*/

