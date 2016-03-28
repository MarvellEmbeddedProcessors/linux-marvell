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

#include "platform/mv_pp3.h"
#include "fw/mv_pp3_fw_msg.h"
#include "net_dev/mv_dev_vq.h"
#include "net_dev/mv_netdev.h"
#include "net_dev/mv_dev_dbg.h"
#include "mv_pp3_gnss_api.h"
#include "mv_pp3_gnss.h"
#include <net/gnss/mv_nss_defs.h>

#define MV_PP3_GNSS_STATS_COLLECTOR_CPU_ID	1
#define MV_PP3_GNSS_STATS_USEC_DEF		20000
/*---------------------------------------------------------------------------*/
/*				Globals					     */
/*---------------------------------------------------------------------------*/
static int ext_vport_msec;
static struct mv_pp3_stats_ext_vp *mv_ext_vports[MV_NSS_EXT_PORT_MAX + 1];
static struct mv_pp3_stats_simple_vp mv_simple_vports[MV_NSS_EXT_PORT_MAX + 1];
/*---------------------------------------------------------------------------*/
static int mv_pp3_vq_reset_flag_get(int vport, int vq, int cpu)
{
	struct net_device *dev;
	struct pp3_dev_priv *dev_priv;
	int reset;

	dev = mv_pp3_vport_dev_get(vport);
	MV_PP3_NULL_PTR(dev, err);

	dev_priv = MV_PP3_PRIV(dev);

	if (mv_pp3_ingress_vq_reset_stats_get(dev_priv->cpu_vp[cpu], vq, &reset) < 0)
		goto err;

	return reset;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_vq_reset_flag_set(int vport, int vq, int cpu, int reset)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	MV_PP3_NULL_PTR(dev, err);

	dev_priv = MV_PP3_PRIV(dev);

	if (mv_pp3_ingress_vq_reset_stats_set(dev_priv->cpu_vp[cpu], vq, reset) < 0)
		goto err;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/
static int mv_pp3_ingress_hwq_stats(int vport, int vq, int cpu, struct mv_pp3_fw_hwq_stat *hwq_stat)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);
	unsigned short hwq = dev_priv->cpu_vp[cpu]->rx_vqs[vq]->hwq;

	if (pp3_fw_hwq_stat_get(hwq, false, hwq_stat)) {
		pr_err("%s:Can't read HWQ #%d statistics from FW\n", __func__, hwq);
		return -1;
	}

	return 0;
}
/*---------------------------------------------------------------------------*/

/* return SWQ stats per CPU */
static struct pp3_swq_stats *mv_pp3_ingress_vq_stats(int vport, int vq, int cpu)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct pp3_dev_priv *dev_priv;

	MV_PP3_NULL_PTR(dev, err);

	dev_priv = MV_PP3_PRIV(dev);

	return mv_pp3_ingress_vq_sw_stats(dev_priv->cpu_vp[cpu], vq);

err:
	pr_err("%s: function failed\n", __func__);
	return NULL;
}
/*---------------------------------------------------------------------------*/
static struct mv_pp3_vq_update_stats *mv_pp3_update_stats_get(int vport, int vq, int cpu)
{
	if ((vport > MV_NSS_EXT_PORT_MAX) || (vq > MV_PP3_VQ_NUM) || (cpu > CONFIG_NR_CPUS)) {
		pr_err("%s: function failed\n", __func__);
		return NULL;
	}

	return &mv_ext_vports[vport]->rx_vqs_stats[vq][cpu];
}
/*---------------------------------------------------------------------------*/
static struct mv_pp3_vq_collect_stats *mv_pp3_stats_collect_get(int vport, int vq, int cpu)
{
	if ((vport > MV_NSS_EXT_PORT_MAX) || (vq > MV_PP3_VQ_NUM) || (cpu > CONFIG_NR_CPUS)) {
		pr_err("%s: function failed\n", __func__);
		return NULL;
	}
	if (mv_ext_vports[vport])
		return &mv_ext_vports[vport]->rx_vqs_collect_stats[vq][cpu];

	return NULL;
}
/*---------------------------------------------------------------------------*/
static struct mv_pp3_fw_hwq_stat *mv_pp3_stats_hwq_base_get(int vport, int vq, int cpu)
{
	if ((vport > MV_NSS_EXT_PORT_MAX) || (vq > MV_PP3_VQ_NUM) || (cpu > CONFIG_NR_CPUS)) {
		pr_err("%s: function failed\n", __func__);
		return NULL;
	}
	return &mv_simple_vports[vport].hwq_stats_base[vq][cpu];
}
/*---------------------------------------------------------------------------*/
/* Update base for simple statistics */
static int mv_pp3_gnss_ingress_vq_stats_clean(int vport, int vq, int cpu)
{
	struct mv_pp3_fw_hwq_stat *hwq_stats_base;

	hwq_stats_base = mv_pp3_stats_hwq_base_get(vport, vq, cpu);

	/* overrite simple stats base */
	if (mv_pp3_ingress_hwq_stats(vport, vq, cpu, hwq_stats_base) < 0) {
		pr_err("%s: failed to collect fw vport %d vq %d statistics\n", __func__, vport, vq);
		return -1;
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_ingress_vq_stats_update_after_dev_reset(int vport, int vq, int cpu)
{
	struct mv_pp3_vq_collect_stats *collect_stats = mv_pp3_stats_collect_get(vport, vq, cpu);
	struct pp3_swq_stats *swq_stats;

	/* update ext stats base */
	if (collect_stats) {
		swq_stats = mv_pp3_ingress_vq_stats(vport, vq, cpu);
		memcpy(&collect_stats->swq_ext_stats_base, swq_stats, sizeof(struct pp3_swq_stats));
	}

	if (mv_pp3_gnss_ingress_vq_stats_clean(vport, vq, cpu) < 0) {
		pr_err("%s: function failed\n", __func__);
		return -1;
	}

	pr_info("vp#%d vq#%d, cpu#%d: Sync statistics DB after reset\n", vport, vq, cpu);
	return 0;
}

/*---------------------------------------------------------------------------*/
/* update counters base after reset */


static int mv_pp3_ingress_vq_stats_collect(int vport, int vq, int cpu)
{
	struct mv_pp3_vq_collect_stats *collect_stats = mv_pp3_stats_collect_get(vport, vq, cpu);
	struct pp3_swq_stats *swq_stats = mv_pp3_ingress_vq_stats(vport, vq, cpu);

	collect_stats->swq_ext_stats_curr.pkts = swq_stats->pkts;
	collect_stats->swq_ext_stats_curr.bytes = swq_stats->bytes;
	collect_stats->swq_ext_stats_curr.pkts_drop = swq_stats->pkts_drop;
	collect_stats->swq_ext_stats_curr.pkts_errors = swq_stats->pkts_errors;

	if (mv_pp3_ingress_hwq_stats(vport, vq, cpu, &collect_stats->fw_ext_stats_curr) < 0) {
		pr_err("%s: failed to collect fw vport %d vq %d statistics\n", __func__, vport, vq);
		return -1;
	}
	if (mv_pp3_vq_reset_flag_get(vport, vq, cpu)) {
		if (mv_pp3_ingress_vq_stats_update_after_dev_reset(vport, vq, cpu) < 0) {
			pr_err("%s: function failed\n", __func__);
			return -1;
		}
		mv_pp3_vq_reset_flag_set(vport, vq, cpu, false);
	}
	return 0;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_ingress_vq_stats_update(int vport, int vq, int cpu)
{
	struct mv_pp3_vq_update_stats *ext_stats = mv_pp3_update_stats_get(vport, vq, cpu);
	struct mv_pp3_vq_collect_stats *collect_stats = mv_pp3_stats_collect_get(vport, vq, cpu);
	unsigned long flags = 0;
	struct spinlock	*stats_lock = mv_ext_vports[vport]->stats_lock;
	uint64_t temp64;

	MV_LOCK(stats_lock, flags);

	/* pkts calculation */
	ext_stats->pkts_sum = collect_stats->swq_ext_stats_curr.pkts - collect_stats->swq_ext_stats_base.pkts;

	temp64 = (((uint64_t)collect_stats->fw_ext_stats_curr.hwq_pkt_high << 31)
			| (uint64_t)collect_stats->fw_ext_stats_curr.hwq_pkt_low);

	ext_stats->pkts_fill_lvl = (unsigned int)(temp64 & 0xFFFFFFFF) - collect_stats->swq_ext_stats_curr.pkts;

	ext_stats->pkts_fill_lvl_sum += ext_stats->pkts_fill_lvl;
	ext_stats->pkts_fill_lvl_max = MV_MAX(ext_stats->pkts_fill_lvl_max, ext_stats->pkts_fill_lvl);

	ext_stats->bytes_sum = collect_stats->swq_ext_stats_curr.bytes - collect_stats->swq_ext_stats_base.bytes;

	temp64 = (((uint64_t)collect_stats->fw_ext_stats_curr.hwq_oct_high << 31)
			| (uint64_t)collect_stats->fw_ext_stats_curr.hwq_oct_low);

	ext_stats->bytes_fill_lvl = (unsigned int)(temp64 & 0xFFFFFFFF) - collect_stats->swq_ext_stats_curr.bytes;

	ext_stats->bytes_fill_lvl_sum += ext_stats->bytes_fill_lvl;
	ext_stats->bytes_fill_lvl_max = MV_MAX(ext_stats->bytes_fill_lvl_max, ext_stats->bytes_fill_lvl);
	MV_UNLOCK(stats_lock, flags);
	return 0;
}
/*---------------------------------------------------------------------------*/
static void mv_pp3_stats_callback(unsigned long data)
{
	struct mv_pp3_stats_ext_vp *vp_priv = (struct mv_pp3_stats_ext_vp *)data;
	int vport = vp_priv->vport;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct mv_pp3_timer *stats_timer = &vp_priv->stats_timer;
	int cpu, vq, vq_num;

	vp_priv->time_elapsed += stats_timer->usec;
	vp_priv->iter++;

	for_each_possible_cpu(cpu) {
		if (!mv_pp3_dev_cpu_inuse(dev, cpu))
			continue;
		vq_num = mv_pp3_dev_rxvq_num_get(dev, cpu);

		for (vq = 0; vq < vq_num; vq++) {
			mv_pp3_ingress_vq_stats_collect(vport, vq, cpu);
			mv_pp3_ingress_vq_stats_update(vport, vq, cpu);

		}
	}
	mv_pp3_timer_complete(stats_timer);

	if (stats_timer->usec)
		mv_pp3_timer_add(stats_timer);
}

/*---------------------------------------------------------------------------*/
/* Set msec for all external (WLAN) vports statistic timers		     */
/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ingress_msec_all_set(unsigned int msec)
{
	int vport;

	if (msec == ext_vport_msec) {
		pr_info("No change in extended statistics configuration\n");
		return -1;
	}

	if ((msec != 0) && (msec < jiffies_to_msecs(1))) {
		pr_err("Invalid time interval, must be >= %d msec\n", jiffies_to_msecs(1));
		return -1;
	}

	pr_info("Reset extended statistics\n");

	for (vport = MV_NSS_EXT_PORT_MIN; vport < MV_NSS_EXT_PORT_MAX + 1; vport++)
		if (mv_pp3_vport_dev_get(vport))
			mv_pp3_gnss_ingress_vport_stats_init(vport, msec);

	if (!ext_vport_msec && msec)
		pr_info("Enable extended staistics, set time interval to %d msec\n", msec);

	else if (ext_vport_msec && msec)
		pr_info("Set extended staistics time interval to %d msec\n", msec);

	else
		pr_info("Disable extended statistics\n");

	ext_vport_msec = msec;

	return 0;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_msec_all_set);
/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ext_vport_msec_get(void)
{
	return ext_vport_msec;
}
/*---------------------------------------------------------------------------*/
static struct mv_pp3_stats_ext_vp *mv_pp3_gnss_ingress_vport_stats_alloc(int vport)
{
	struct mv_pp3_stats_ext_vp *vp_priv;
	int rc;

	if (vport > MV_NSS_EXT_PORT_MAX) {
		pr_err("%s: virtual port %d is out of range\n", __func__, vport);
		return NULL;
	}
	vp_priv = kzalloc(sizeof(struct mv_pp3_stats_ext_vp), GFP_KERNEL);

	if (!vp_priv) {
		pr_err("%s: Out of memory\n", __func__);
		return NULL;
	}

	vp_priv->stats_lock = kzalloc(sizeof(struct spinlock), GFP_KERNEL);

	if (!vp_priv->stats_lock) {
		pr_err("%s: Out of memory\n", __func__);
		return NULL;
	}

	spin_lock_init(vp_priv->stats_lock);

	rc = mv_pp3_timer_init(&vp_priv->stats_timer,
				MV_PP3_GNSS_STATS_COLLECTOR_CPU_ID,
				MV_PP3_GNSS_STATS_USEC_DEF,
				MV_PP3_WORKQUEUE, mv_pp3_stats_callback,
				(unsigned long)vp_priv);
	if (rc < 0)
		return NULL;

	vp_priv->vport = vport;

	/*pr_info("Initialized vport #%d statistic extenstion\n", vport);*/

	return vp_priv;
}

/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ingress_vport_stats_init(int vport, unsigned int msec)
{
	struct mv_pp3_stats_ext_vp *vp_priv;
	int rc;


	if ((msec != 0) && (msec < jiffies_to_msecs(1))) {
		pr_err("Invalid time period, must be >= %d msec\n", jiffies_to_msecs(1));
		return -1;
	}

	if (vport > MV_NSS_EXT_PORT_MAX) {
		pr_err("%s: Invalid virtual port %d\n", __func__, vport);
		return -1;
	}

	if (!mv_ext_vports[vport]) {
		mv_ext_vports[vport] = mv_pp3_gnss_ingress_vport_stats_alloc(vport);

		if (!mv_ext_vports[vport])
			return -1;
	}

	vp_priv = mv_ext_vports[vport];

	/* if msec = 0 timer will stop */
	rc = mv_pp3_timer_usec_set(&vp_priv->stats_timer, msec * 1000);

	if (msec) {
		mv_pp3_gnss_ingress_vport_ext_stats_clean(vport);
		mv_pp3_timer_add(&vp_priv->stats_timer);
	}

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vport_stats_init);

/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ingress_vport_stats_clean(int vport)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	int cpu, vq, vq_num = 0;

	if (!dev)
		goto err;

	if (vport > MV_NSS_EXT_PORT_MAX) {
		pr_err("%s: Invalid virtual port %d\n", __func__, vport);
		return -1;
	}

	for_each_possible_cpu(cpu) {

		if (!mv_pp3_dev_cpu_inuse(dev, cpu))
			continue;

		vq_num = mv_pp3_dev_rxvq_num_get(dev, cpu);

		for (vq = 0; vq < vq_num; vq++) {
			/* clean vq advanced statistics */
			if  (mv_pp3_gnss_ingress_vq_stats_clean(vport, vq, cpu))
				goto err;
		}
	}
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vport_stats_clean);

/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ingress_vport_stats_get(int vport, bool clean, int size, struct mv_nss_vq_stats res_stats[])
{
	int cpu, vq, vq_num, rc = 0;
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct mv_pp3_fw_hwq_stat hwq_stats, *hwq_stats_base;
	uint64_t temp64, temp64_base;

	 if (!dev) {
		pr_err("%s: function failed\n", __func__);
		return -1;
	}
	/* clear input */
	memset(res_stats, 0, sizeof(struct mv_nss_vq_stats) * size);

	for_each_possible_cpu(cpu) {

		if (!mv_pp3_dev_cpu_inuse(dev, cpu))
			continue;

		vq_num = mv_pp3_dev_rxvq_num_get(dev, cpu);

		if (size > vq_num) {
			pr_err("%s: vport %d have only %d rx virtual queues\n",
						__func__, vport, vq_num);
			return -1;
		}

		for (vq = 0; vq < size; vq++) {
			if (mv_pp3_vq_reset_flag_get(vport, vq, cpu)) {
					if (mv_pp3_ingress_vq_stats_update_after_dev_reset(vport, vq, cpu) < 0) {
						pr_err("%s: function failed\n", __func__);
						return -1;
					}
					mv_pp3_vq_reset_flag_set(vport, vq, cpu, false);
			}
			if (mv_pp3_ingress_hwq_stats(vport, vq, cpu, &hwq_stats) < 0) {
				pr_err("%s: failed to read fw vport %d vq %d statistics\n", __func__, vport, vq);
				return -1;
			}
			hwq_stats_base  = mv_pp3_stats_hwq_base_get(vport, vq, cpu);
			/* calc packets */
			temp64_base = (((uint64_t)hwq_stats_base->hwq_pkt_high << 31) |
							(uint64_t)hwq_stats_base->hwq_pkt_low);
			temp64 = (((uint64_t)hwq_stats.hwq_pkt_high << 31) |
							(uint64_t)hwq_stats.hwq_pkt_low);
			res_stats[vq].pkts += (unsigned int)((temp64 - temp64_base) & 0xFFFFFFFF);

			/* octets packets */
			temp64_base = (((uint64_t)hwq_stats_base->hwq_oct_high << 31) |
							(uint64_t)hwq_stats_base->hwq_oct_low);
			temp64 = (((uint64_t)hwq_stats.hwq_oct_high << 31) |
							(uint64_t)hwq_stats.hwq_oct_low);
			res_stats[vq].octets += (unsigned int)((temp64 - temp64_base) & 0xFFFFFFFF);

			/* drops packets */
			temp64_base = (((uint64_t)hwq_stats_base->hwq_pkt_drop_high << 31) |
							(uint64_t)hwq_stats_base->hwq_pkt_drop_low);
			temp64 = (((uint64_t)hwq_stats.hwq_pkt_drop_high << 31) |
								(uint64_t)hwq_stats.hwq_pkt_drop_low);
			res_stats[vq].drops += (unsigned int)((temp64 - temp64_base) & 0xFFFFFFFF);

			/* errors packets - not supported, FW and SW do not count errors */
		}
	}
	if (clean)
		rc = mv_pp3_gnss_ingress_vport_stats_clean(vport);

	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vport_stats_get);

/*---------------------------------------------------------------------------*/
static int mv_pp3_gnss_ingress_vq_ext_stats_clean(int vport, int vq, int cpu)
{
	struct mv_pp3_vq_update_stats *ext_stats;
	struct mv_pp3_vq_collect_stats *collect_stats = mv_pp3_stats_collect_get(vport, vq, cpu);
	struct pp3_swq_stats *swq_stats = mv_pp3_ingress_vq_stats(vport, vq, cpu);

	ext_stats = mv_pp3_update_stats_get(vport, vq, cpu);

	if (!ext_stats)
		return -1;

	/* clean vq advanced statistics */
	ext_stats->pkts_fill_lvl = 0;
	ext_stats->pkts_fill_lvl_max = 0;
	ext_stats->pkts_fill_lvl_sum = 0;
	ext_stats->pkts_sum = 0;
	ext_stats->bytes_fill_lvl = 0;
	ext_stats->bytes_fill_lvl_max = 0;
	ext_stats->bytes_fill_lvl_sum = 0;
	ext_stats->bytes_sum = 0;
	memcpy(&collect_stats->swq_ext_stats_base, swq_stats, sizeof(struct pp3_swq_stats));
	return 0;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_gnss_ingress_vport_ext_stats_clean(int vport)
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct mv_pp3_stats_ext_vp *vp_priv;
	int cpu, vq, vq_num = 0;

	if (!dev)
		goto err;


	if (vport > MV_NSS_EXT_PORT_MAX) {
		pr_err("%s: Invalid virtual port %d\n", __func__, vport);
		return -1;
	}

	vp_priv = mv_ext_vports[vport];

	if (!vp_priv)
		return -1;

	for_each_possible_cpu(cpu) {

		if (!mv_pp3_dev_cpu_inuse(dev, cpu))
			continue;

		vq_num = mv_pp3_dev_rxvq_num_get(dev, cpu);

		for (vq = 0; vq < vq_num; vq++) {

			/* clean timer time elapsed counter */
			vp_priv->time_elapsed = 0;

			/* clean timer iterations counter */
			vp_priv->iter = 0;

			/* clean vq advanced statistics */
			if  (mv_pp3_gnss_ingress_vq_ext_stats_clean(vport, vq, cpu))
				goto err;
		}
	}
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vport_ext_stats_clean);

/*---------------------------------------------------------------------------*/

int mv_pp3_gnss_ingress_vport_ext_stats_get(int vport, bool clean, int size,
						struct mv_nss_vq_advance_stats res_stats[])
{
	struct net_device *dev = mv_pp3_vport_dev_get(vport);
	struct mv_pp3_stats_ext_vp *vp_priv;
	struct mv_pp3_vq_update_stats *ext_stats;
	int cpu, i, vq_num, rc = 0;
	unsigned long flags = 0;
	uint64_t temp64;

	 if (!dev) {
		pr_err("%s: function failed\n", __func__);
		return -1;
	}

	if (vport > MV_NSS_EXT_PORT_MAX) {
		pr_err("%s: Invalid virtual port %d\n", __func__, vport);
		return -1;
	}

	vp_priv = mv_ext_vports[vport];

	if (!vp_priv) {
		pr_err("%s: statistics extension not initialized\n", __func__);
		return -1;

	}

	/* clear input */
	memset(res_stats, 0, sizeof(struct mv_nss_vq_advance_stats) * size);

	MV_LOCK(vp_priv->stats_lock, flags);

	for_each_possible_cpu(cpu) {

		if (!mv_pp3_dev_cpu_inuse(dev, cpu))
			continue;

		vq_num = mv_pp3_dev_rxvq_num_get(dev, cpu);

		if (size > vq_num) {
			pr_err("%s: vport %d have only %d rx virtual queues\n",
						__func__, vport, vq_num);
			MV_UNLOCK(vp_priv->stats_lock, flags);
			return -1;
		}
		for (i = 0; i < size; i++) {
			ext_stats = mv_pp3_update_stats_get(vport, i, cpu);
			/* time elapsed in msec */
			res_stats[i].time_elapsed = vp_priv->time_elapsed / 1000;

			res_stats[i].pkts_fill_lvl += ext_stats->pkts_fill_lvl;
			res_stats[i].pkts_fill_lvl_max =
				MV_MAX(res_stats[i].pkts_fill_lvl_max, ext_stats->pkts_fill_lvl_max);

			res_stats[i].bytes_fill_lvl += ext_stats->bytes_fill_lvl;
			res_stats[i].bytes_fill_lvl_max =
				MV_MAX(res_stats[i].bytes_fill_lvl_max, ext_stats->bytes_fill_lvl_max);


			res_stats[i].pkts_fill_lvl_avg += ext_stats->pkts_fill_lvl_sum;
			res_stats[i].bytes_fill_lvl_avg += ext_stats->bytes_fill_lvl_sum;

			res_stats[i].pkts_rate += ext_stats->pkts_sum;
			res_stats[i].bytes_rate += ext_stats->bytes_sum;
		}
	}

	for (i = 0; i < size; i++) {
		if (vp_priv->iter) {
			res_stats[i].pkts_fill_lvl_avg = res_stats[i].pkts_fill_lvl_avg / vp_priv->iter;
			res_stats[i].bytes_fill_lvl_avg = res_stats[i].bytes_fill_lvl_avg / vp_priv->iter;
		}

		if (vp_priv->time_elapsed) {
			temp64 = ((uint64_t)res_stats[i].pkts_rate) *  1000000;
			do_div(temp64, vp_priv->time_elapsed);
			res_stats[i].pkts_rate = (unsigned int) (temp64 & 0xFFFFFFFF);

			temp64 = ((uint64_t)res_stats[i].bytes_rate) *  1000000;
			do_div(temp64, vp_priv->time_elapsed);
			res_stats[i].bytes_rate = (unsigned int) (temp64 & 0xFFFFFFFF);
		}
	}
	if (clean)
		rc = mv_pp3_gnss_ingress_vport_ext_stats_clean(vport);

	MV_UNLOCK(vp_priv->stats_lock, flags);


	return rc;
}
EXPORT_SYMBOL(mv_pp3_gnss_ingress_vport_ext_stats_get);
#if 0
/*---------------------------------------------------------------------------*/

void mv_pp3_gnss_ingress_vport_ext_stats_show(int vport, bool clean)
{
	int rc, vq = 0;
	struct mv_nss_vq_advance_stats res_stats[CONFIG_MV_PP3_RXQ_NUM];

	rc = mv_pp3_gnss_ingress_vport_ext_stats_get(vport, clean, CONFIG_MV_PP3_RXQ_NUM, res_stats);

	if (rc < 0) {
		pr_info("Failed read vport #%d advanced statistics\n", vport);
		return;
	}
	pr_info("Vport %d Advanced statistics\n", vport);

	for (vq = 0; vq < CONFIG_MV_PP3_RXQ_NUM; vq++) {
		pr_info("\nvq = %d\n", vq);
		pr_info("-------------------\n");
		pr_info("pkts_fill_lvl     = %d\n", res_stats[vq].pkts_fill_lvl);
		pr_info("pkts_fill_lvl_max = %d\n", res_stats[vq].pkts_fill_lvl_max);
		pr_info("pkts_fill_lvl_avg = %d\n", res_stats[vq].pkts_fill_lvl_avg);
		pr_info("pkts_rate = %d pps\n", res_stats[vq].pkts_rate);
		pr_info("bytes_fill_lvl     = %d\n", res_stats[vq].bytes_fill_lvl);
		pr_info("bytes_fill_lvl_max = %d\n", res_stats[vq].bytes_fill_lvl_max);
		pr_info("bytes_fill_lvl_avg = %d\n", res_stats[vq].bytes_fill_lvl_avg);
		pr_info("bytes_rate = %d bps\n", res_stats[vq].bytes_rate);
		pr_info("time_elapsed = %d msec\n", res_stats[vq].time_elapsed);
	}
}
/*---------------------------------------------------------------------------*/

void mv_pp3_gnss_ingress_vport_stats_show(int vport, bool clean)
{
	int rc, vq;
	struct mv_nss_vq_stats res_stats[CONFIG_MV_PP3_RXQ_NUM];

	rc = mv_pp3_gnss_ingress_vport_stats_get(vport, clean, CONFIG_MV_PP3_RXQ_NUM, res_stats);

	if (rc < 0) {
		pr_info("Failed read vport #%d statistics\n", vport);
		return;
	}
	pr_info("Vport %d statistics\n", vport);

	for (vq = 0; vq < CONFIG_MV_PP3_RXQ_NUM; vq++) {
		pr_info("\nvq = %d\n", vq);
		pr_info("-------------------\n");
		pr_info("pkts   = %10llu\n", res_stats[vq].pkts);
		pr_info("octets = %10llu\n", res_stats[vq].octets);
		pr_info("errors = %10llu\n", res_stats[vq].errors);
		pr_info("drops  = %10llu\n",  res_stats[vq].drops);
	}
}
#endif
