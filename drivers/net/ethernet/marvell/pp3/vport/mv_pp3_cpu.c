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

#include "hmac/mv_hmac_bm.h"
#include "mv_pp3_cpu.h"
#include "mv_pp3_pool.h"
#include "fw/mv_pp3_fw_msg.h"

struct pp3_cpu	**pp3_cpus;
int		mv_pp3_cpus_num;

/*---------------------------------------------------------------------------*/
/*			Global CPU APIs					     */
/*---------------------------------------------------------------------------*/

int mv_pp3_cpus_global_init(struct mv_pp3 *priv, int cpus_num)
{
	pp3_cpus = kzalloc(cpus_num * sizeof(struct pp3_cpu *), GFP_KERNEL);
	if (!pp3_cpus)
		return -ENOMEM;

	mv_pp3_cpus_num = cpus_num;
	return 0;
}

int mv_pp3_cpu_close(struct mv_pp3 *priv, int cpu)
{
	if (pp3_cpus == NULL) {
		pr_err("CPU component is not initialized yet\n");
		return -1;
	}
	if (mv_pp3_max_check(cpu, mv_pp3_cpus_num, "cpu"))
		return -1;

	if (pp3_cpus[cpu] == NULL) {
		pr_err("CPU %d is not allocated yet\n", cpu);
		return -1;
	}
	mv_pp3_hmac_rxq_flush(pp3_cpus[cpu]->bm_frame, pp3_cpus[cpu]->bm_swq);
	mv_pp3_hmac_rxq_delete(pp3_cpus[cpu]->bm_frame, pp3_cpus[cpu]->bm_swq);

	return 0;
}

struct pp3_cpu *mv_pp3_cpu_get(int cpu)
{
	if (pp3_cpus == NULL) {
		pr_err("CPU component is not initialized yet\n");
		return NULL;
	}
	if (mv_pp3_max_check(cpu, mv_pp3_cpus_num, "cpu"))
		return NULL;

	return pp3_cpus[cpu];
}

struct pp3_cpu *mv_pp3_cpu_alloc(int cpu)
{
	struct pp3_cpu *cpu_ctrl;

	if (mv_pp3_max_check(cpu, mv_pp3_cpus_num, "cpu"))
		return NULL;

	cpu_ctrl = kzalloc(sizeof(struct pp3_cpu), GFP_KERNEL);
	MV_PP3_NULL_PTR(cpu_ctrl, oom);

	if (pp3_cpus[cpu] != NULL) {
		pr_err("CPU #%d is already exist\n", cpu);
		return NULL;
	}
	cpu_ctrl->cpu = cpu;
	pp3_cpus[cpu] = cpu_ctrl;

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	pr_info("\t  o Debug buffer: %d bytes of memory allocated\n", MV_PP3_DEBUG_BUFFER*4);
	cpu_ctrl->occ_debug_buf = kzalloc(MV_PP3_DEBUG_BUFFER*4, GFP_KERNEL);
	MV_PP3_NULL_PTR(cpu_ctrl->occ_debug_buf, oom);
#endif

	return cpu_ctrl;
oom:
	mv_pp3_cpu_delete(cpu_ctrl);
	pr_err("%s: Out of memory\n", __func__);
	return NULL;
}
/*---------------------------------------------------------------------------*/

int mv_pp3_cpu_sw_init(struct pp3_cpu *cpu_ctrl)
{
	MV_PP3_NULL_PTR(cpu_ctrl, err);

	mv_pp3_cfg_dp_bmq_params_get(cpu_ctrl->cpu, &cpu_ctrl->bm_frame, &cpu_ctrl->bm_swq, NULL);
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/
int mv_pp3_cpu_hw_init(struct pp3_cpu *cpu_ctrl)
{
	MV_PP3_NULL_PTR(cpu_ctrl, err);

	if (mv_pp3_hmac_bm_queue_init(cpu_ctrl->bm_frame, cpu_ctrl->bm_swq, MV_PP3_HMAC_BM_Q_SIZE)) {
		pr_err("%s: bm queue (frame #%d, queue %d) initialization failed\n",
				__func__, cpu_ctrl->bm_frame, cpu_ctrl->bm_swq);
		goto err;
	}

	mv_pp3_hmac_rxq_enable(cpu_ctrl->bm_frame, cpu_ctrl->bm_swq);
	mv_pp3_hmac_txq_enable(cpu_ctrl->bm_frame, cpu_ctrl->bm_swq);
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/
void mv_pp3_cpu_delete(struct pp3_cpu *cpu_ctrl)
{
	if (!cpu_ctrl)
		return;

#ifdef CONFIG_MV_PP3_DEBUG_CODE
	kfree(cpu_ctrl->occ_debug_buf);
#endif

	pr_info("%s: delete global cpu %d\n", __func__, cpu_ctrl->cpu);
	kfree(cpu_ctrl);
}

/*---------------------------------------------------------------------------*/
/*			Shared CPU APIs					     */
/*---------------------------------------------------------------------------*/
struct pp3_cpu_shared *mv_pp3_cpu_shared_alloc(struct mv_pp3 *priv)
{
	/* allocate netdev shared CPUs structure*/
	struct pp3_cpu_shared *cpu_shared = kzalloc(sizeof(struct pp3_cpu_shared), GFP_KERNEL);

	MV_PP3_NULL_PTR(cpu_shared, oom);

	/* double size allocation - pools in pair mode */
	cpu_shared->long_pool = mv_pp3_pool_alloc(2 * CONFIG_MV_PP3_BM_RX_POOL_CAPACITY);
	MV_PP3_NULL_PTR(cpu_shared->long_pool, oom);
	cpu_shared->long_pool->capacity = CONFIG_MV_PP3_BM_RX_POOL_CAPACITY;

	if (priv->short_pool_buf_size > (NET_SKB_PAD + 32)) {
		cpu_shared->short_pool = mv_pp3_pool_alloc(2 * CONFIG_MV_PP3_BM_RX_POOL_CAPACITY);
		MV_PP3_NULL_PTR(cpu_shared->short_pool, oom);
		cpu_shared->short_pool->capacity = CONFIG_MV_PP3_BM_RX_POOL_CAPACITY;
	}

	cpu_shared->txdone_pool = mv_pp3_pool_alloc(2 * CONFIG_MV_PP3_BM_LINUX_POOL_CAPACITY);
	cpu_shared->txdone_pool->capacity = CONFIG_MV_PP3_BM_LINUX_POOL_CAPACITY;

	cpu_shared->rx_pkt_mode = MV_PP3_PKT_LAST;

	return cpu_shared;

oom:
	mv_pp3_cpu_shared_delete(cpu_shared);
	pr_err("%s: Out of memory\n", __func__);
	return NULL;

}
/*---------------------------------------------------------------------------*/
int mv_pp3_cpu_shared_sw_init(struct pp3_cpu_shared *cpu_shared, int max_pkt_size)
{
	MV_PP3_NULL_PTR(cpu_shared, err);

	if (mv_pp3_pool_long_sw_init(cpu_shared->long_pool, NET_SKB_PAD, max_pkt_size) < 0)
		goto err;

	if (cpu_shared->short_pool) {
		if (mv_pp3_pool_short_sw_init(cpu_shared->short_pool, NET_SKB_PAD,
				pp3_device->short_pool_buf_size) < 0)
			goto err;
	}

	if (mv_pp3_pool_txdone_sw_init(cpu_shared->txdone_pool) < 0)
		goto err;

	/* Init only if not changed by user */
	if (cpu_shared->rx_pkt_mode ==  MV_PP3_PKT_LAST)
		cpu_shared->rx_pkt_mode = MV_PP3_PKT_DRAM;

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
/*---------------------------------------------------------------------------*/
int mv_pp3_cpu_shared_hw_init(struct pp3_cpu_shared *cpu_shared)
{
	MV_PP3_NULL_PTR(cpu_shared, err);

	if (cpu_shared->long_pool)
		if (mv_pp3_pool_hw_init(cpu_shared->long_pool) < 0)
			goto err;

	if (cpu_shared->short_pool)
		if (mv_pp3_pool_hw_init(cpu_shared->short_pool) < 0)
			goto err;

	if (cpu_shared->lro_pool)
		if (mv_pp3_pool_hw_init(cpu_shared->lro_pool) < 0)
			goto err;

	if (cpu_shared->txdone_pool)
		if (mv_pp3_pool_hw_init(cpu_shared->txdone_pool) < 0)
			goto err;
	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}

/*---------------------------------------------------------------------------*/
void mv_pp3_cpu_shared_delete(struct pp3_cpu_shared *cpu_shared)
{
	if (!cpu_shared)
		return;

	mv_pp3_pool_delete(cpu_shared->long_pool);
	mv_pp3_pool_delete(cpu_shared->short_pool);
	mv_pp3_pool_delete(cpu_shared->lro_pool);
	mv_pp3_pool_delete(cpu_shared->txdone_pool);
	kfree(cpu_shared);
}

/*---------------------------------------------------------------------------
description:
	Update FW with netdev if pools

return values:
		success: 0
		fail: -1
---------------------------------------------------------------------------*/
int mv_pp3_cpu_shared_fw_set_pools(struct pp3_cpu_shared *shared)
{
	struct pp3_pool *pool;

	pool = shared->long_pool;
	if (pool) {
		pp3_fw_bm_pool_set(pool);
	} else {
		pr_err("%s: long pool must be initialized\n", __func__);
		goto err;
	}

	pool = shared->short_pool;
	if (pool)
		pp3_fw_bm_pool_set(pool);

	pool = shared->lro_pool;
	if (pool)
		pp3_fw_bm_pool_set(pool);

	return 0;
err:
	pr_err("%s: function failed\n", __func__);
	return -1;
}
