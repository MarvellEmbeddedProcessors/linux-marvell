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

#ifndef __mv_pp3_pool_h__
#define __mv_pp3_pool_h__

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"

#define PPOOL_STATS(ppool, cpu)		per_cpu_ptr((ppool)->stats, (cpu))
#define PPOOL_BUF_MISSED(ppool, cpu)	(*per_cpu_ptr((ppool->buf_missed), (cpu)))
#define PPOOL_BUF_TXDONE(ppool, cpu)	(*per_cpu_ptr((ppool->buf_txdone), (cpu)))

/************************/
/*   pool structures    */
/************************/
enum  pp3_pool_type {
	PP3_POOL_TYPE_GP = 0,
	PP3_POOL_TYPE_DRAM,
	PP3_POOL_TYPE_GPM,
	PP3_POOL_TYPE_LAST
};

enum pp3_pool_mode {
	POOL_MODE_FREE = 0,
	POOL_MODE_LONG,
	POOL_MODE_SHORT,
	POOL_MODE_LRO,
	POOL_MODE_TXDONE,
	POOL_MODE_LAST
};

struct pp3_pool_stats {
	unsigned int buff_rx;
	unsigned int buff_get_request;
	unsigned int buff_get;
	unsigned int buff_get_zero;
	unsigned int buff_put;
	unsigned int buff_get_timeout_err;
	unsigned int buff_alloc_err;
	unsigned int buff_alloc;
	unsigned int buff_free;
	unsigned int buff_recycled_ok;
	unsigned int buff_recycled_err;
	unsigned int buff_get_dummy;
	unsigned int buff_free_tso;
};

struct pp3_pool {
	int pool;
	int capacity;
	int pool_size;
	int buf_num;
	int headroom;
	int buf_size;
	int pkt_max_size;
	atomic_t in_use;
	int in_use_thresh;
	void __iomem *virt_base;
	dma_addr_t phys_base;
	unsigned int flags;
	enum pp3_pool_mode mode;
	enum pp3_pool_type type;
	int __percpu *buf_missed; /* ~ allocation failed */
	int __percpu *buf_txdone; /* ~ wait for release */
	struct pp3_pool_stats __percpu *stats;
};


/************************/
/*   user pools APIs    */
/************************/
int mv_pp3_pool_buff_put(int pool, void __iomem *virt, dma_addr_t phys_addr);
const char *mv_pp3_pool_name_get(struct pp3_pool *ppool);
int mv_pp3_pools_global_init(struct mv_pp3 *priv, int pools_num);
struct pp3_pool *mv_pp3_pool_get(int pool);
int mv_pp3_pool_set_id(struct pp3_pool *ppool, int pool);
struct pp3_pool *mv_pp3_pool_alloc(int capacity);
int mv_pp3_pool_long_sw_init(struct pp3_pool *ppool, int headroom, int max_pkt_size);
int mv_pp3_pool_short_sw_init(struct pp3_pool *ppool, int headroom, int buf_size);
int mv_pp3_pool_txdone_sw_init(struct pp3_pool *ppool);
int mv_pp3_pool_hw_init(struct pp3_pool *ppool);
void mv_pp3_pool_delete(struct pp3_pool *ppool);

/************************/
/*   QM/GPM pools APIs  */
/************************/
void mv_pp3_pools_qm_gpm_sw_init(struct pp3_pool *pool0, struct pp3_pool *pool1);
void mv_pp3_pools_qm_dram_sw_init(struct pp3_pool *pool0, struct pp3_pool *pool1);
int mv_pp3_pools_qm_hw_init(struct pp3_pool *ppool0, struct pp3_pool *ppool1);

/************************/
/*   Debug pool APIs    */
/************************/
void pp3_dbg_pool_status_print(int pool);
void pp3_dbg_pool_stats_print(int pool);
void pp3_dbg_pool_stats_clear(int pool);
int pp3_dbg_pool_dump(int pool, int v);

#endif /* __mv_pp3_pool_h__ */
