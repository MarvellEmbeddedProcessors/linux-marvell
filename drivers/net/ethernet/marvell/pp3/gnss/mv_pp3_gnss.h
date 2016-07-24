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

/* SW-only structure mv_pp3_vq_update_stats (no sync to HW) */
struct mv_pp3_vq_update_stats {
	unsigned int	pkts_fill_lvl;       /* Current queue fill level, packets */
	unsigned int	pkts_fill_lvl_max;   /* Maximum queue fill level, packets */
	unsigned int	bytes_fill_lvl;      /* Current queue fill level, octets */
	unsigned int	bytes_fill_lvl_max;  /* Maximum queue fill level, octets */
	u64		pkts_fill_lvl_sum;   /* Sum of pkts_fill_lvl */
	u64		bytes_fill_lvl_sum;  /* Sum of bytes_fill_lvl */
	u64		pkts_sum;
	u64		bytes_sum;
};

struct mv_pp3_vq_collect_stats {
	struct pp3_swq_stats swq_ext_stats_base;
	struct pp3_swq_stats swq_ext_stats_curr;
	struct mv_pp3_fw_hwq_stat fw_ext_stats_curr;
};

/* Extension statistics DB */
struct mv_pp3_stats_ext_vp {
	int vport;
	struct mv_pp3_vq_update_stats rx_vqs_stats[MV_PP3_VQ_NUM][CONFIG_NR_CPUS];
	struct mv_pp3_vq_collect_stats rx_vqs_collect_stats[MV_PP3_VQ_NUM][CONFIG_NR_CPUS];
	struct mv_pp3_timer	stats_timer;  /* Statistics collection timer */
	struct spinlock		*stats_lock;  /* Spinlock for Statistics collection */
	int time_elapsed;
	int iter;
};

/* Simple statistics DB */
struct mv_pp3_stats_simple_vp {
	int vport;
	struct mv_pp3_fw_hwq_stat hwq_stats_base[MV_PP3_VQ_NUM][CONFIG_NR_CPUS];
};
