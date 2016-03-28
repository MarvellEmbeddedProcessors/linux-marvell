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
#include "platform/mv_pp3_config.h"
#include "mv_pp3_cpu.h"
#include "mv_pp3_pool.h"
#include "bm/mv_bm.h"
#include "qm/mv_qm.h"
#include "hmac/mv_hmac_bm.h"

static struct pp3_pool	**pp3_pools;
static int		pp3_pools_num;
static struct mv_pp3	*pp3_priv;

/* Once time called pool component initialization function */
int mv_pp3_pools_global_init(struct mv_pp3 *priv, int pools_num)
{
	pp3_pools = kzalloc(pools_num * sizeof(struct pp3_pool *), GFP_KERNEL);
	if (!pp3_pools)
		return -ENOMEM;

	pp3_pools_num = pools_num;
	pp3_priv = priv;
	return 0;
}

/*---------------------------------------------------------------------------*/
const char *mv_pp3_pool_name_get(struct pp3_pool *ppool)
{
	const char *type_str;

	switch (ppool->type) {
	case PP3_POOL_TYPE_DRAM:
		return "QM DRAM ";
	case PP3_POOL_TYPE_GPM:
		return "QM GPM ";
	case PP3_POOL_TYPE_GP:
	default:
		break;
	}
	switch (ppool->mode) {
	case POOL_MODE_FREE:
		type_str = "Free   ";
		break;
	case POOL_MODE_LONG:
		type_str = "Long   ";
		break;
	case POOL_MODE_SHORT:
		type_str = "Short  ";
		break;
	case POOL_MODE_LRO:
		type_str = "LRO    ";
		break;
	case POOL_MODE_TXDONE:
		type_str = "TxDone ";
		break;
	default:
		type_str = "Unknown";
	}
	return type_str;
}
/*---------------------------------------------------------------------------*/

/* Allocate memory for a pool without attach pool ID. */
struct pp3_pool *mv_pp3_pool_alloc(int capacity)
{
	struct pp3_pool *ppool;
	int size;

	if (capacity % 16) {
		pr_err("%s: pool size must be multiple of 16\n", __func__);
		return NULL;
	}

	ppool = kzalloc(sizeof(struct pp3_pool), GFP_KERNEL);
	if (!ppool)
		goto oom;

	ppool->stats = alloc_percpu(struct pp3_pool_stats);
	ppool->mode = POOL_MODE_FREE;
	atomic_set(&ppool->in_use, 0);

	size = sizeof(unsigned int) * capacity;

	ppool->virt_base = dma_alloc_coherent(&pp3_priv->pdev->dev, size, &ppool->phys_base, GFP_KERNEL);
	if (!ppool->virt_base) {
		pr_err("%s: Can't allocate %d bytes of coherent memory for BM pool\n",
			__func__, size);
		goto oom;
	}
	ppool->capacity = capacity;

	return ppool;

oom:
	pr_err("%s: out of memory\n", __func__);
	if (ppool && ppool->virt_base)
		dma_free_coherent(&pp3_priv->pdev->dev, size, ppool->virt_base, ppool->phys_base);

	kfree(ppool);

	return NULL;
}
/*---------------------------------------------------------------------------*/

struct pp3_pool *mv_pp3_pool_get(int pool)
{
	if (pp3_pools == NULL) {
		pr_err("Pool component is not initialized yet\n");
		return NULL;
	}

	if (mv_pp3_max_check(pool, pp3_pools_num, "pool"))
		return NULL;

	return pp3_pools[pool];
}

/* Attach pool structure with pool ID */
int mv_pp3_pool_set_id(struct pp3_pool *ppool, int pool)
{
	if (pp3_pools == NULL) {
		pr_err("Pool component is not initialized yet\n");
		return -1;
	}

	if (mv_pp3_max_check(pool, pp3_pools_num, "pool"))
		return -1;

	if (pp3_pools[pool] != NULL) {
		pr_err("Pool #%d is already exist\n", pool);
		return -1;
	}
	ppool->pool = pool;
	pp3_pools[pool] = ppool;

	return 0;
}

/* Get pool ID from configurator and initialize GP pool
   or update pool fields if pool ID already set */

int mv_pp3_pool_long_sw_init(struct pp3_pool *ppool, int headroom, int max_pkt_size)
{
	MV_PP3_NULL_PTR(ppool, err);

	if (ppool->buf_num) {
		pr_err("%s: none empty pool, buffers number = %d\n",
			__func__,  ppool->buf_num);
		goto err;
	}

	if (!ppool->pool) {
		mv_pp3_cfg_dp_gen_pool_id(&ppool->pool);

		if (mv_pp3_max_check(ppool->pool, pp3_pools_num, "pool ID"))
			return -1;

		ppool->type = PP3_POOL_TYPE_GP;
		ppool->pool_size = 0;
		ppool->mode = POOL_MODE_LONG;
		pp3_pools[ppool->pool] = ppool;
	}

	ppool->buf_size = headroom + max_pkt_size
		+ SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	ppool->headroom = headroom;
	ppool->pkt_max_size = max_pkt_size;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_pool_short_sw_init(struct pp3_pool *ppool, int headroom, int buf_size)
{
	int pool;

	MV_PP3_NULL_PTR(ppool, err);

	mv_pp3_cfg_dp_gen_pool_id(&pool);

	if (mv_pp3_max_check(pool, pp3_pools_num, "pool"))
		return -1;

	ppool->pool = pool;
	ppool->type = PP3_POOL_TYPE_GP;
	ppool->pool_size = 0;
	ppool->buf_size = buf_size;
	ppool->mode = POOL_MODE_SHORT;
	ppool->headroom = headroom;
	ppool->pkt_max_size = ppool->buf_size - ppool->headroom -
				SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	pp3_pools[pool] = ppool;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_pool_txdone_sw_init(struct pp3_pool *ppool)
{
	int pool;

	MV_PP3_NULL_PTR(ppool, err);

	mv_pp3_cfg_dp_gen_pool_id(&pool);

	ppool->pool = pool;
	ppool->mode = POOL_MODE_TXDONE;
	ppool->type = PP3_POOL_TYPE_GP;
	pp3_pools[pool] = ppool;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_pool_hw_init(struct pp3_pool *ppool)
{
	unsigned int ret_val;
	struct mv_a40 pool_addr;

	MV_PP3_NULL_PTR(ppool, err);

	if (bm_gp_pid_validation(ppool->pool)) {
		pr_err("%s: Invalid pool id #%d\n", __func__, ppool->pool);
		goto err;
	}

	pool_addr.virt_lsb = (u32)ppool->virt_base;
	pool_addr.virt_msb = 0;
	pool_addr.dma_lsb = (u32)ppool->phys_base;
	pool_addr.dma_msb = 0;

	ret_val = bm_gp_pool_def_basic_init(ppool->pool, 2 * ppool->capacity, &pool_addr);

	if (ret_val) {
		pr_err("%s: HW pool %d initialization failed\n",  __func__, ppool->pool);
		goto err;
	}

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/

void mv_pp3_pool_delete(struct pp3_pool *ppool)
{
	if (!ppool)
		return;

	/* Free buffers only from general purpose (GP) pools */
	if (ppool->type == PP3_POOL_TYPE_GP)
		if (ppool->buf_num != 0)
			pr_warn("%s: delete none empty pool %d", __func__, ppool->pool);

	pr_info("%s: Free pool %d\n", __func__, ppool->pool);
	kfree(ppool->virt_base);
	free_percpu(ppool->stats);
	kfree(ppool);
}

/*---------------------------------------------------------------------------*/
/* QM GPM pools 0,1 private init					     */
/*---------------------------------------------------------------------------*/
void mv_pp3_pools_qm_gpm_sw_init(struct pp3_pool *pool0, struct pp3_pool *pool1)
{
	pool0->capacity = BM_QM_GPM_POOL_CAPACITY;
	pool1->capacity = BM_QM_GPM_POOL_CAPACITY;
	pool0->type = PP3_POOL_TYPE_GPM;
	pool1->type = PP3_POOL_TYPE_GPM;
}
/*---------------------------------------------------------------------------*/
/* QM GPM pools 2,3 private init                                             */
/*---------------------------------------------------------------------------*/
void mv_pp3_pools_qm_dram_sw_init(struct pp3_pool *pool0, struct pp3_pool *pool1)
{
	pool0->capacity = BM_QM_DRAM_POOL_CAPACITY;
	pool1->capacity = BM_QM_DRAM_POOL_CAPACITY;
	pool0->type = PP3_POOL_TYPE_DRAM;
	pool1->type = PP3_POOL_TYPE_DRAM;
}
/*---------------------------------------------------------------------------*/
/* QM GPM pools HW init init                                                 */
/*---------------------------------------------------------------------------*/
int mv_pp3_pools_qm_hw_init(struct pp3_pool *ppool0, struct pp3_pool *ppool1)
{
	struct mv_a40 pool_0_addr, pool_1_addr;
	int ret_val;

	pool_0_addr.virt_lsb = (u32)ppool0->virt_base;
	pool_0_addr.dma_lsb = (u32)ppool0->phys_base;
	pool_1_addr.virt_lsb = (u32)ppool1->virt_base;
	pool_1_addr.dma_lsb = (u32)ppool1->phys_base;

	pool_0_addr.virt_msb = 0;
	pool_0_addr.dma_msb = 0;
	pool_1_addr.virt_msb = 0;
	pool_1_addr.dma_msb = 0;

	if (ppool0->type != ppool1->type) {
		pr_err("%s: Different pools type\n", __func__);
		goto err;
	}

	if (ppool0->type == PP3_POOL_TYPE_GPM)
		/* QM GPM pools */
		ret_val = bm_qm_gpm_pools_def_quick_init(ppool1->capacity, &pool_0_addr, &pool_1_addr);
	else if (ppool0->type == PP3_POOL_TYPE_DRAM)
		/* QM DRAM pools */
		ret_val = bm_qm_dram_pools_def_quick_init(&pp3_priv->pdev->dev, ppool1->capacity,
									&pool_0_addr, &pool_1_addr);
	else {
		pr_err("%s: Invalid pools type %d\n", __func__, ppool0->type);
		goto err;
	}

	if (ret_val < 0)
		goto err;

	return 0;
err:
	pr_err("%s: Function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
/* push buffer to bm pool                                                    */
/*---------------------------------------------------------------------------*/
int mv_pp3_pool_buff_put(int pool, void __iomem *virt, dma_addr_t phys_addr)
{
	int cpu = smp_processor_id();
	struct pp3_cpu *cpu_priv = mv_pp3_cpu_get(cpu);
	int queue, frame;

	queue = cpu_priv->bm_swq;
	frame = cpu_priv->bm_frame;

	if (mv_pp3_hmac_bm_buff_put(frame, queue, pool, phys_addr, (unsigned int)virt)) {
		mv_pp3_hmac_txq_occ_get(frame, queue);
		if (mv_pp3_hmac_bm_buff_put(frame, queue, pool, phys_addr, (unsigned int)virt))
			return -1;
	}
	STAT_DBG(PPOOL_STATS(pp3_pools[pool], cpu)->buff_put++);

	/* Memory barrier to ensure that CFH written to DRAM before HMAC transmit triggered */
	wmb();

	mv_pp3_hmac_txq_send(frame, queue, 1);

	return 0;
}
/*---------------------------------------------------------------------------*/

/************************/
/*   Debug pool APIs    */
/************************/
void pp3_dbg_pool_status_print(int pool)
{
	struct pp3_pool *ppool;

	if ((pool < 0) || (pool >= BM_POOLS_NUM)) {
		pr_err("%s: Invalid pool number - %d\n", __func__, pool);
		return;
	}

	if (!pp3_pools || (!pp3_pools[pool])) {
		pr_err("Pool #%d not initialized\n", pool);
		return;
	}
	ppool = pp3_pools[pool];

	pr_info("\n----------- BM pool #%d - %s Status -------------\n",
		pool, mv_pp3_pool_name_get(ppool));

	pr_info("size = %d, capacity = %d, virt_base = 0x%08x, phys_base = 0x%08x\n",
		ppool->pool_size, ppool->capacity, (u32)ppool->virt_base,  (u32)ppool->phys_base);

	pr_info("buf size = %d, pkt_max_size = %d\n",
			ppool->buf_size, ppool->pkt_max_size);
	pr_info("buf num = %d, in_use_tresh = %d, in_use = %d\n",
		ppool->buf_num, ppool->in_use_thresh, atomic_read(&ppool->in_use));
}
/*---------------------------------------------------------------------------*/

int pp3_dbg_pool_dump(int pool, int v)
{
	int i;
	struct pp3_pool *ppool;
	u32 *arr;

	if ((pool < 0) || (pool >= BM_POOLS_NUM)) {
		pr_err("%s: pool=%d is out of range\n", __func__, pool);
		return -EINVAL;
	}

	if (v) {

		if ((pp3_pools == NULL) || (pp3_pools[pool] == NULL)) {
			pr_err("%s: pool=%d is not initialized\n", __func__, pool);
			return -EINVAL;
		}

		ppool = pp3_pools[pool];

		arr = (u32 *)ppool->virt_base;

		for (i = 0; i < ppool->capacity; i = i + 2)
			pr_info("%d	virt = 0x%08x	phys = 0x%08x\n", i/2, arr[i+1], arr[i]);
	}

	pr_info("\n");

	bm_pool_status_dump(pool);

	return 0;
}
/*---------------------------------------------------------------------------*/


/* Clear BM pool statistics */
void pp3_dbg_pool_stats_clear(int pool)
{
	struct pp3_pool *ppool;
	int cpu;

	if ((pool < 0) || (pool >= BM_POOLS_NUM)) {
		pr_err("%s: Invalid pool number - %d\n", __func__, pool);
		return;
	}
	if (!pp3_pools || (!pp3_pools[pool])) {
		pr_err("Pool #%d not initialized\n", pool);
		return;
	}
	ppool = pp3_pools[pool];

	for_each_possible_cpu(cpu)
		memset(PPOOL_STATS(ppool, cpu), 0, sizeof(struct pp3_pool_stats));
}
/*---------------------------------------------------------------------------*/

/* Print BM pool statistics */
void pp3_dbg_pool_stats_print(int pool)
{
	struct pp3_pool *ppool;
	struct pp3_pool_stats total_stats;
	int cpu;

	if ((pool < 0) || (pool >= BM_POOLS_NUM)) {
		pr_err("%s: Invalid pool number - %d\n", __func__, pool);
		return;
	}
	if (!pp3_pools || (!pp3_pools[pool])) {
		pr_err("Pool #%d not initialized\n", pool);
		return;
	}
	ppool = pp3_pools[pool];

	pr_info("\n----------- BM pool #%d - %s Statistics ---------\n",
		ppool->pool, mv_pp3_pool_name_get(ppool));

	pr_info("buff_num............................%10d\n", ppool->buf_num);
	pr_info("buff_in_use.........................%10d\n", atomic_read(&ppool->in_use));

	/* Calculate summary for all CPUs */
	memset(&total_stats, 0, sizeof(total_stats));
	for_each_possible_cpu(cpu) {
		total_stats.buff_rx += PPOOL_STATS(ppool, cpu)->buff_rx;
		total_stats.buff_get_request += PPOOL_STATS(ppool, cpu)->buff_get_request;
		total_stats.buff_get_timeout_err += PPOOL_STATS(ppool, cpu)->buff_get_timeout_err;
		total_stats.buff_get_zero += PPOOL_STATS(ppool, cpu)->buff_get_zero;
		total_stats.buff_get_dummy += PPOOL_STATS(ppool, cpu)->buff_get_dummy;
		total_stats.buff_get += PPOOL_STATS(ppool, cpu)->buff_get;
		total_stats.buff_put += PPOOL_STATS(ppool, cpu)->buff_put;
		total_stats.buff_alloc_err += PPOOL_STATS(ppool, cpu)->buff_alloc_err;
		total_stats.buff_alloc += PPOOL_STATS(ppool, cpu)->buff_alloc;
		total_stats.buff_free += PPOOL_STATS(ppool, cpu)->buff_free;
#ifdef CONFIG_MV_PP3_TSO
		total_stats.buff_free_tso += PPOOL_STATS(ppool, cpu)->buff_free_tso;
#endif
#ifdef CONFIG_MV_PP3_SKB_RECYCLE
		total_stats.buff_recycled_ok += PPOOL_STATS(ppool, cpu)->buff_recycled_ok;
		total_stats.buff_recycled_err += PPOOL_STATS(ppool, cpu)->buff_recycled_err;
#endif /* CONFIG_MV_PP3_SKB_RECYCLE */
	}

       /* Print summary only */
	pr_info("buff_rx.............................%10d\n", total_stats.buff_rx);
	pr_info("buff_get_request....................%10d\n", total_stats.buff_get_request);
	pr_info("buff_get_timeout_err................%10d\n", total_stats.buff_get_timeout_err);
	pr_info("buff_get_zero.......................%10d\n", total_stats.buff_get_zero);
	pr_info("buff_get_dummy......................%10d\n", total_stats.buff_get_dummy);
	pr_info("buff_get............................%10d\n", total_stats.buff_get);
	pr_info("buff_put............................%10d\n", total_stats.buff_put);
	pr_info("buff_alloc..........................%10d\n", total_stats.buff_alloc);
	pr_info("buff_free...........................%10d\n", total_stats.buff_free);
	pr_info("buff_alloc_err......................%10d\n", total_stats.buff_alloc_err);
#ifdef CONFIG_MV_PP3_TSO
	pr_info("buff_free_tso.......................%10d\n", total_stats.buff_free_tso);
#endif
#ifdef CONFIG_MV_PP3_SKB_RECYCLE
	pr_info("buff_recycled_ok....................%10d\n", total_stats.buff_recycled_ok);
	pr_info("buff_recycled_err...................%10d\n", total_stats.buff_recycled_err);
#endif /* CONFIG_MV_PP3_SKB_RECYCLE */
}

