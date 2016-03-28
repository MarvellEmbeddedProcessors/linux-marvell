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

#ifndef __mv_pp3_cpu_h__
#define __mv_pp3_cpu_h__

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"

/* TBD. Temporary externals. Remove later */
extern struct pp3_cpu	**pp3_cpus;
extern int		mv_pp3_cpus_num;

/************************/
/*    CPU structures   */
/************************/

/* Masks used for pp3_cpu flags */
#define MV_PP3_CPU_F_DBG_BUF_PUSH_BIT	0
#define MV_PP3_CPU_F_DBG_BUF_POP_BIT	1

#define MV_PP3_CPU_F_DBG_BUF_PUSH		(1 << MV_PP3_CPU_F_DBG_BUF_PUSH_BIT)
#define MV_PP3_CPU_F_DBG_BUF_POP		(1 << MV_PP3_CPU_F_DBG_BUF_POP_BIT)

/* CPU statistics */
struct pp3_cpu_stats {
	unsigned int lnx_fw_irq;
	unsigned int lnx_fw_irq_err;
};

/* CPU control structure shared for all network devices */
struct pp3_cpu {
	int	cpu;      /* CPU number [0..nr_cpu_ids) for this CPU */
	int	ref_cnt;  /* reference count of active CPU vports */
	int	bm_frame; /* HMAC frame used to access BM */
	int	bm_swq;   /* HMAC SWQ used to access BM*/
	struct pp3_cpu_stats	stats;
	unsigned long		flags;
#ifdef CONFIG_MV_PP3_DEBUG_CODE
	int			debug_txdone_occ;
	int			*occ_debug_buf;
	u32			occ_cur_buf;
#endif
};

/* CPU shared per port structure*/
struct pp3_cpu_shared {
	struct mv_nss_ops	*gnss_ops;		/* Pointer to gnss_ops structure. If NULL use Linux functions */
	struct pp3_pool		*long_pool;		/* BM pool used for long buffers - must be valid */
	struct pp3_pool		*short_pool;		/* BM pool used for short buffers */
	struct pp3_pool		*lro_pool;		/* BM pool used for LRO - page size buffers */
	struct pp3_pool		*txdone_pool;           /* TX Done pool */
	enum mv_pp3_pkt_mode	rx_pkt_mode;		/* RX mode used for this device */
};

/********************************/
/*        GLOBAL CPU API        */
/********************************/
int mv_pp3_cpus_global_init(struct mv_pp3 *priv, int num_cpus);
int mv_pp3_cpu_close(struct mv_pp3 *priv, int cpu);
struct pp3_cpu *mv_pp3_cpu_alloc(int cpu);
struct pp3_cpu *mv_pp3_cpu_get(int cpu);
int mv_pp3_cpu_sw_init(struct pp3_cpu *cpu_ctrl);
int mv_pp3_cpu_hw_init(struct pp3_cpu *cpu_ctrl);
void mv_pp3_cpu_delete(struct pp3_cpu *cpu_ctrl);
/*********************************/
/*  SHARED PRE PORT CPU API      */
/*********************************/
struct pp3_cpu_shared *mv_pp3_cpu_shared_alloc(struct mv_pp3 *priv);
int mv_pp3_cpu_shared_sw_init(struct pp3_cpu_shared *cpu_shared, int max_pkt_size);
int mv_pp3_cpu_shared_hw_init(struct pp3_cpu_shared *cpu_shared);
void mv_pp3_cpu_shared_delete(struct pp3_cpu_shared *cpu_shared);
int mv_pp3_cpu_shared_fw_set_pools(struct pp3_cpu_shared *shared);

#endif /* __mv_pp3_cpu_h__ */
