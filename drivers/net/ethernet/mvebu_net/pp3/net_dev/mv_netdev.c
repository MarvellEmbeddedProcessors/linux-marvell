/*#include <mvCopyright.h>*/
#define MV_FPGA

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>
#include <asm/setup.h>
#include <net/ip.h>
#include <net/ipv6.h>

#ifdef MV_FPGA
#include <linux/pci.h>
#endif

#include <linux/mv_pp3.h>
#include <linux/dma-mapping.h>
#include "common/mv_hw_if.h"
#include "common/mv_pp3_config.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "emac/mv_emac.h"
#include "fw/mv_channel_if.h"
#include "gmac/mv_gmac.h"
#include "mv_netdev.h"
#include "mv_netdev_structs.h"

/* global data */
struct pp3_group *pp3_groups[CONFIG_NR_CPUS][MAX_ETH_DEVICES];
struct pp3_frame **pp3_frames;
struct pp3_pool **pp3_pools;
struct pp3_dev_priv **pp3_ports;
struct pp3_cpu **pp3_cpus;
static int pp3_ports_num;
static int pp3_hw_initialized;
static struct  platform_device *pp3_sysfs;

/* functions */
static int mv_pp3_poll(struct napi_struct *napi, int budget);
static int mv_pp3_dev_open(struct net_device *dev);
static int mv_pp3_hw_shared_start(void);

/*---------------------------------------------------------------------------*/
static u32 mv_vbase_address;

u32 mv_hw_silicon_base_addr_get(void)
{
	return mv_vbase_address;
}

static int pp3_sysfs_init(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");

	if (!pd) {
		pp3_sysfs = platform_device_register_simple("pp3", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");
	}

	if (!pd) {
		pr_err("%s: cannot find pp3 device\n", __func__);
		return -1;
	}

	mv_pp3_emac_sysfs_init(&pd->kobj);
	mv_pp3_hmac_sysfs_init(&pd->kobj);
	pp3_dev_sysfs_init(&pd->kobj);

	return 0;
}

/*---------------------------------------------------------------------------*/

static void pp3_sysfs_exit(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "pp3");
	if (!pd) {
		pr_err("%s: cannot find pp3 device\n", __func__);
		return;
	}

	pp3_dev_sysfs_exit(&pd->kobj);
	mv_pp3_emac_sysfs_exit(&pd->kobj);
	platform_device_unregister(pp3_sysfs);
}

/*---------------------------------------------------------------------------*/
/* Trigger tx done in MV_CPU_TX_DONE_TIMER_PERIOD msecs			     */
/*---------------------------------------------------------------------------*/
static void mv_pp3_add_tx_done_timer(struct pp3_cpu *cpu_ctrl)
{
	if (test_and_set_bit(MV_CPU_F_TX_DONE_TIMER, &cpu_ctrl->flags) == 0) {
		cpu_ctrl->tx_done_timer.expires = jiffies +
			msecs_to_jiffies(MV_CPU_TX_DONE_TIMER_PERIOD);
		add_timer_on(&cpu_ctrl->tx_done_timer, cpu_ctrl->cpu);
	}
}

/*---------------------------------------------------------------------------*/
static void mv_pp3_tx_done_timer_callback(unsigned long data)
{
	struct pp3_cpu *cpu_ctrl = (struct pp3_cpu *)data;
	struct	pp3_pool *tx_done_pool = cpu_ctrl->tx_done_pool;
	struct	pp3_queue *bm_msg_queue = cpu_ctrl->bm_msg_queue;

	clear_bit(MV_CPU_F_TX_DONE_TIMER_BIT, &cpu_ctrl->flags);

	mv_pp3_hmac_bm_buff_request(bm_msg_queue->frame, bm_msg_queue->rxq.sw_q,
					tx_done_pool->pool, 100 /*TODO - request according to counter value*/);

	/* TODO: update counter */

	if (cpu_ctrl->tx_done_cnt - 100 > 0)
		mv_pp3_add_tx_done_timer(cpu_ctrl);
}

/*---------------------------------------------------------------------------*/
/* rx events , group interrupt handle					     */
/*---------------------------------------------------------------------------*/
irqreturn_t mv_pp3_isr(void *data)
{
	struct pp3_group *group = (struct pp3_group *)data;
	struct napi_struct *napi = group->napi;

	STAT_INFO(group->stats.irq++);

	/* TODO: disable group interrupt */

	/* Verify that the device not already on the polling list */
	if (napi_schedule_prep(napi)) {
		/* schedule the work (rx+txdone+link) out of interrupt contxet */
		__napi_schedule(napi);
	} else {
		STAT_INFO(group->stats.irq_err++);
	}

	/* TODO: enable group interrupt */

	return IRQ_HANDLED;
}

/*---------------------------------------------------------------------------*/
/* call to mv_pp3_rx for group's rxqs					     */
/*---------------------------------------------------------------------------*/

static int mv_pp3_poll(struct napi_struct *napi, int budget)
{
	int rx_done = 0;
	struct pp3_dev_priv *priv = MV_PP3_PRIV(napi->dev);
	struct pp3_group *group = priv->groups[smp_processor_id()];

	if (!test_bit(MV_ETH_F_STARTED_BIT, &(priv->flags))) {
		napi_complete(napi);
		return rx_done;
	}

	STAT_INFO(group->stats.rx_poll++);

	while (budget > 0) { /* && group rxqs are not empty */
		/*
		for all rxqs in the group
			example from ppv2
			call to mv_pp3_rx()
			update counters and budget
		*/
	}

	if (budget > 0)
		napi_complete(napi);

	return rx_done;
}

/*---------------------------------------------------------------------------*/
/* linux pool full interrupt handler					     */
/*---------------------------------------------------------------------------*/

irqreturn_t pp3_linux_pool_isr(unsigned int data)
{
	struct pp3_cpu *cpu_ctrl = (struct pp3_cpu *)data;

	STAT_INFO(cpu_ctrl->stats.lnx_pool_irq++);

	/* TODO: interrupts Mask */

	tasklet_schedule(cpu_ctrl->bm_msg_tasklet);

	/* TODO: interrupts UnMask */

	return IRQ_HANDLED;
}

/*---------------------------------------------------------------------------*/

void mv_pp3_bm_tasklet(unsigned long data)
{
	int pool;
	unsigned int  ph_addr, vr_addr;
	struct	pp3_cpu *cpu_ctrl = (struct pp3_cpu *)data;
	struct	pp3_pool *tx_done_pool = cpu_ctrl->tx_done_pool;
	struct	pp3_queue *bm_msg_queue = cpu_ctrl->bm_msg_queue;

	while (mv_pp3_hmac_bm_buff_get(bm_msg_queue->frame, bm_msg_queue->rxq.sw_q,
						&pool, &ph_addr, &vr_addr) != -1) {

		if (pool == tx_done_pool->pool) {
			dev_kfree_skb_any((struct sk_buff *)(&vr_addr));
			cpu_ctrl->tx_done_cnt--;
		}
		/* talk with yelena ... if we use the same HMAC Q */
		/* TODO: registration mechanisem */
		/* TODO: else call calback function */
	}
}

/*---------------------------------------------------------------------------*/

static struct sk_buff *pp3_skb_alloc(struct pp3_pool *ppool, unsigned long *phys_addr, gfp_t gfp_mask)
{
	struct sk_buff *skb;

	skb = __dev_alloc_skb(ppool->buf_size, gfp_mask);

	if (!skb)
		return NULL;

	if (phys_addr)
		*phys_addr = dma_map_single(NULL, skb->head, ppool->buf_size, DMA_FROM_DEVICE);

	return skb;
}

/*---------------------------------------------------------------------------*/

static int pp3_pool_release(int pool)
{
	if ((pp3_pools == NULL) | (pp3_pools[pool] == NULL))
		return 0;

	if (pp3_pools[pool]->buf_num != 0) {
		pr_err("%s: pool %d is not empty\n", __func__, pool);
		return -EINVAL;
	}

	kfree(pp3_pools[pool]->virt_base);
	kfree(pp3_pools[pool]);

	pr_info("%s: pool %d released\n", __func__, pool);

	return 0;
}

/*---------------------------------------------------------------------------*/

static struct pp3_pool *pp3_pool_alloc(int pool, int capacity)
{
	struct pp3_pool *ppool;
	int size;

	if (capacity % 16) {
		pr_err("%s: pool size must be multiple of 16\n", __func__);
		return NULL;
	}

	if ((pool < 0) || (pool >= MV_PP3_BM_POOLS)) {
		pr_err("%s: pool=%d is out of range\n", __func__, pool);
		return NULL;
	}

	if (pp3_pools[pool] != NULL) {
		pr_err("%s: pool=%d already exist\n", __func__, pool);
		return NULL;
	}

	/* init group napi */
	pp3_pools[pool] = kmalloc(sizeof(struct pp3_pool), GFP_KERNEL);

	ppool = pp3_pools[pool];

	if (!ppool)
		goto oom;

	memset(ppool, 0, sizeof(struct pp3_pool));

	ppool->pool = pool;
	ppool->capacity = capacity;
	ppool->flags = POOL_F_FREE;

	size = sizeof(unsigned int) * capacity;
/*
	TODO: example in mainline driver, firt param ?
	ppool->virt_base = dma_alloc_coherent(NULL, size, &ppool->phys_base, GFP_KERNEL);
*/
	if (!ppool->virt_base)
		goto oom;

	return ppool;

oom:
	pp3_pool_release(pool);

	pr_err("%s: out of memory\n", __func__);

	return NULL;

}

/*---------------------------------------------------------------------------*/

static int pp3_pool_init_complete(int pool)
{
	int count = 0;
	unsigned int completed;

	do {
		if (count++ >= MV_PP3_POOL_INIT_TIMEOUT_MSEC) {
			pr_warn("TIMEOUT for pool #%d init complete\n", pool);
			return -1;
		}

		mdelay(1);

		/*bm_pool_quick_init_status_get(pool, &completed);*/

	} while (!completed);

	return 0;
}

/*---------------------------------------------------------------------------*/

static struct pp3_pool *pp3_pool_gp_create(int pool, int capacity)
{
	struct pp3_pool *ppool;
	unsigned int ret_val;

	if (pp3_pools[pool]) {
		if (pp3_pools[pool]->capacity != capacity) {
			pr_err("%s: pool #%d already exist with capacity < %d\n", __func__, pool, capacity);
			return NULL;
		}

		if (pp3_pools[pool]->type != PP3_POOL_TYPE_GP) {
			pr_err("%s: pool #%d reserved for QM\n", __func__, pool);
			return NULL;
		}

		return pp3_pools[pool];
	}

	ppool = pp3_pool_alloc(pool, 2 * capacity);

	if (ppool == NULL) {
		pr_err("%s: out of memory\n", __func__);
		return NULL;
	}

	ppool->type = PP3_POOL_TYPE_GP;

	/*ret_val = bm_gp_pool_def_basic_init(pool, 2 * capacity, 0, ppool->phys_base, 1);*/

	if (!ret_val)
		goto out;

	if (pp3_pool_init_complete(pool))
		goto out;

	return ppool;
out:
	pp3_pool_release(pool);
	pr_err("%s: pool %d creation failed\n", __func__, pool);
	return NULL;
}

/*---------------------------------------------------------------------------*/

static int pp3_pools_gpm_init(int capacity)
{
	struct pp3_pool *ppool_0, *ppool_1;
	int ret_val;

	ppool_0 = pp3_pool_alloc(MV_PP3_GPM_POOL_0, capacity);
	ppool_1 = pp3_pool_alloc(MV_PP3_GPM_POOL_1, capacity);
	ppool_0->type = PP3_POOL_TYPE_GPM;
	ppool_1->type = PP3_POOL_TYPE_GPM;

	/*reg_val = bm_qm_gpm_pools_def_quick_init(capacity, 0, ppool_0->phys_base, 0, ppool_1->phys_base);*/

	if (!ret_val)
		return ret_val;

	return pp3_pool_init_complete(MV_PP3_GPM_POOL_0) || pp3_pool_init_complete(MV_PP3_GPM_POOL_1);
}

/*---------------------------------------------------------------------------*/

/* initialize pool 2, 3 */
static int pp3_pools_dram_init(int capacity)
{
	struct pp3_pool *ppool_0, *ppool_1;
	int ret_val;

	ppool_0 = pp3_pool_alloc(MV_PP3_DRAM_POOL_0, capacity);
	ppool_1 = pp3_pool_alloc(MV_PP3_DRAM_POOL_1, capacity);
	ppool_0->type = PP3_POOL_TYPE_DRAM;
	ppool_1->type = PP3_POOL_TYPE_DRAM;

	/*ret_val = bm_qm_dram_pools_def_quick_init (capacity, 0, ppool_0->phys_base, 0, ppool_1->phys_base);*/

	if (!ret_val)
		return ret_val;

	return pp3_pool_init_complete(MV_PP3_DRAM_POOL_0) || pp3_pool_init_complete(MV_PP3_DRAM_POOL_1);
}

/*---------------------------------------------------------------------------*/

static int pp3_pool_add(int pool, int buf_num, int frame, int queue)
{
	struct pp3_pool *ppool;
	unsigned long phys_addr;
	void *virt;
	int size, i;

	if ((pool < 0) || (pool >= MV_PP3_BM_POOLS)) {
		pr_err("%s: pool=%d is out of range\n", __func__, pool);
		return -EINVAL;
	}

	if ((pp3_pools == NULL) || (pp3_pools[pool] == NULL)) {
		pr_err("%s: pool=%d is not initialized\n", __func__, pool);
		return -EINVAL;
	}

	ppool = pp3_pools[pool];
	size = ppool->buf_size;

	if (size == 0) {
		pr_err("%s: invalid pool #%d state: buf_size = %d\n", __func__, pool, size);
		return -EINVAL;
	}

	for (i = 0; i < buf_num; i++) {
		/* alloc char * and add skb only in TX func */
		virt =  (void *)pp3_skb_alloc(ppool, &phys_addr, GFP_KERNEL);

		if (!virt)
			break;

		mv_pp3_hmac_bm_buff_put(frame, queue, pool, (unsigned int)virt, phys_addr);
	}

	ppool->buf_num += i;

	pr_info("%s %s %s pool #%d:  buf_size=%4d - %d of %d buffers added\n",
		(ppool->flags & POOL_F_SHORT) ? "short" : "",
		(ppool->flags & POOL_F_LONG) ? "long" : "",
		(ppool->flags & POOL_F_LRO) ? "lro" : "",
		 pool, size, i, buf_num);
}
/*---------------------------------------------------------------------------*/
/* channel callback function						     */
/*---------------------------------------------------------------------------*/
void pp3_chan_callback(int chan, void *msg, int size)
{
	/* TODO: lock / release*/
}

/*---------------------------------------------------------------------------*/

static const struct net_device_ops mv_pp3_netdev_ops = {
	.ndo_open            = mv_pp3_dev_open,
/*
	.ndo_stop            = mv_pp3_stop,
	.ndo_start_xmit      = mv_pp3_tx,
	.ndo_set_rx_mode     = mv_pp3_set_rx_mode,
	.ndo_set_mac_address = mv_pp3_set_mac_addr,
	.ndo_change_mtu      = mv_pp3_change_mtu,
	.ndo_tx_timeout      = mv_pp3_tx_timeout,
	.ndo_get_stats64     = mvneta_get_stats64,
*/
};

/*---------------------------------------------------------------------------*/
/* Allocate and initialize net_device structures			     */
/*---------------------------------------------------------------------------*/
struct net_device *mv_pp3_netdev_init(struct platform_device *pdev)
{
	struct mv_pp3_port_data *port_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;
	struct pp3_dev_priv *dev_priv;
	struct net_device *dev;
	struct resource *res;

	int rxqs_num = port_data->group_rx_queue_count * nr_cpu_ids;
	int txqs_num = port_data->group_tx_queue_count * nr_cpu_ids;

	/* tx_queue_count from core.c */
	dev = alloc_etherdev_mqs(sizeof(struct pp3_dev_priv), txqs_num, rxqs_num);
	if (!dev)
		return NULL;

	/*SET IN CORE.C to BASE*/
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	BUG_ON(!res);
	dev->irq = res->start;

	dev->mtu = port_data->mtu;
	memcpy(dev->dev_addr, port_data->mac_addr, MV_MAC_ADDR_SIZE);

	dev->tx_queue_len = port_data->tx_queue_size;
	dev->watchdog_timeo = 5 * HZ;

	SET_NETDEV_DEV(dev, &pdev->dev);

	dev_priv = MV_PP3_PRIV(dev);

	memset(dev_priv, 0, sizeof(struct pp3_dev_priv));

	dev_priv->dev = dev;
	dev_priv->index = pdev->id;
	dev_priv->rxqs_num = rxqs_num;
	dev_priv->txqs_num = txqs_num;

/*
	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(32))) {
		pr_warning( "mydev: No suitable DMA available.\n");
		free_netdev(dev);
		return NULL;
	}
*/
	return dev;
}

/*---------------------------------------------------------------------------*/

static struct pp3_rxq *pp3_rxq_priv_init(int emac, int cpu)
{
	int sw_q, hw_q, frame;
	struct pp3_rxq *priv_rxq;

	/*mv_pp3_cfg_dp_nic_rxq_params_get(emac, cpu, &frame, &sw_q, &hw_q);*/
	priv_rxq = kzalloc(sizeof(struct pp3_rxq), GFP_KERNEL);

	if (priv_rxq) {
		priv_rxq->frame_num = frame;
		priv_rxq->sw_q = sw_q;
		priv_rxq->hw_q = hw_q;
		priv_rxq->type = PP3_Q_TYPE_QM;
		priv_rxq->size = MV_PP3_DP_RXQ_SIZE;
		priv_rxq->pkt_coal = CONFIG_PP3_RX_COAL_PKTS;
		priv_rxq->time_coal_profile = MV_PP3_RXQ_TIME_COAL_DEF_PROF;
	}
	return priv_rxq;
}

/*---------------------------------------------------------------------------*/

static struct pp3_txq *pp3_txq_priv_init(int emac, int cpu)
{
	int sw_q, hw_q, frame;
	struct pp3_txq *priv_txq;

	/*mv_pp3_cfg_dp_nic_txq_params_get(emac, cpu, &frame, &sw_q, &hw_q)*/
	priv_txq = kmalloc(sizeof(struct pp3_txq), GFP_KERNEL);

	if (priv_txq) {
		priv_txq->frame_num = frame;
		priv_txq->sw_q = sw_q;
		priv_txq->hw_q = hw_q;
		priv_txq->size = MV_PP3_DP_TXQ_SIZE;
		priv_txq->type = PP3_Q_TYPE_QM;
	}

	return priv_txq;
}

/*---------------------------------------------------------------------------*/
/* Release net_device private structures				     */
/*---------------------------------------------------------------------------*/

static void mv_pp3_dev_priv_release(struct pp3_dev_priv *dev_priv)
{
	int cpu, i;
	struct pp3_group *group;

	for_each_possible_cpu(cpu) {

		group = dev_priv->groups[cpu];

		if (!group)
			continue;

		if (group->rxqs)
			for (i = 0; i < group->rxqs_num; i++)
				kfree(group->rxqs[i]);

		if (group->txqs)
			for (i = 0; i < group->txqs_num; i++)
				kfree(group->txqs[i]);

		kfree(group->rxqs);
		kfree(group->txqs);
		kfree(group->napi);
		kfree(group);
	}
}

/*---------------------------------------------------------------------------*/
/* Allocate and initialize net_device private structures		     */
/*---------------------------------------------------------------------------*/
static int mv_pp3_dev_priv_init(struct pp3_dev_priv *dev_priv)
{
	int cpu, i, index;
	/*int logic_q, phys_q, size, frame;*/

	/* indicate emac number */
	index = dev_priv->index;

	/* create group per each cpu */
	for_each_possible_cpu(cpu) {
		struct pp3_group *group;

		dev_priv->groups[cpu] = kzalloc(sizeof(struct pp3_dev_priv), GFP_KERNEL);

		group = dev_priv->groups[cpu];

		if (!group)
			goto oom;

		group->rxqs_num = dev_priv->rxqs_num / nr_cpu_ids;
		group->rxqs = kzalloc(sizeof(struct pp3_rxq *) * group->rxqs_num, GFP_KERNEL);

		if (!group->rxqs)
			goto oom;


		group->txqs_num = dev_priv->txqs_num / nr_cpu_ids;
		group->txqs = kzalloc(sizeof(struct pp3_txq *) * group->txqs_num, GFP_KERNEL);

		if (!group->txqs)
			goto oom;

		for (i = 0; i < group->rxqs_num; i++) {
			group->rxqs[i] = pp3_rxq_priv_init(index, cpu);

			if (!group->rxqs[i])
				goto oom;

			group->rxqs[i]->dev_priv = dev_priv;
		}

		for (i = 0; i < group->txqs_num; i++) {
			group->txqs[i] = pp3_txq_priv_init(index, cpu);

			if (!group->txqs[i])
				goto oom;

			group->txqs[i]->dev_priv = dev_priv;
		}

		/* set group cpu control */
		group->cpu_ctrl = pp3_cpus[cpu];

		/* init group napi */
		group->napi = kzalloc(sizeof(struct napi_struct), GFP_KERNEL);

		if (!group->napi)
			goto oom;

		netif_napi_add(dev_priv->dev, group->napi, mv_pp3_poll, CONFIG_MV_ETH_RX_POLL_WEIGHT);

		pp3_groups[cpu][index] = group;
		pp3_cpus[cpu]->dev_priv[index] = dev_priv;

	} /* for */

	return 0;
oom:
	mv_pp3_dev_priv_release(dev_priv);

	pr_err("%s: out of memory\n", __func__);

	return -ENOMEM;

}

/*---------------------------------------------------------------------------*/

static int mv_pp3_sw_probe(struct platform_device *pdev)
{
	struct net_device *dev;

	dev = mv_pp3_netdev_init(pdev);

	pr_info("  o Loading network interface(s) for port #%d: mtu=%d\n", pdev->id, dev->mtu);

	if (dev == NULL) {
		pr_err("\to %s: can't create netdevice\n", __func__);
		return -ENOMEM;
	}

	pp3_ports[pdev->id] = MV_PP3_PRIV(dev);

	mv_pp3_emac_unit_base(pdev->id, mv_hw_silicon_base_addr_get() + MV_PP3_EMAC_BASE(pdev->id));
	pp3_gmac_unit_base(pdev->id, GMAC_REG_BASE(pdev->id) + mv_fpga_gop_base_addr_get());

	pr_info("Probing Marvell PPv3 Network Driver\n");
	return 0;
}

/*---------------------------------------------------------------------------*/

#ifdef CONFIG_CPU_IDLE
int mv_pp3_suspend(struct platform_device *pdev, pm_message_t state)
{
/* TBD */
	return 0;
}

int mv_pp3_resume(struct platform_device *pdev)
{
/* TBD */
	return 0;
}
#endif	/* CONFIG_CPU_IDLE */

/*---------------------------------------------------------------------------*/

static int mv_pp3_sw_remove(struct platform_device *pdev)
{
	pr_info("Removing Marvell PPv3 Network Driver\n");
	return 0;
}

/*---------------------------------------------------------------------------*/

static void mv_pp3_shutdown(struct platform_device *pdev)
{
	pr_info("Shutting Down Marvell PPv3 Network Driver\n");
}

/*---------------------------------------------------------------------------*/

static struct platform_driver mv_pp3_driver = {
	.probe = mv_pp3_sw_probe,
	/*.remove = mv_pp3_remove,*/
	.shutdown = mv_pp3_shutdown,
#ifdef CONFIG_CPU_IDLE
	.suspend = mv_pp3_suspend,
	.resume = mv_pp3_resume,
#endif /* CONFIG_CPU_IDLE */
	.driver = {
		.name = MV_PP3_PORT_NAME,
		.owner	= THIS_MODULE,
	},
};
/*---------------------------------------------------------------------------*/
/* Support per port for platform driver */
static int mv_pp3_hw_netif_start(struct pp3_dev_priv *dev_priv)
{
	int i, cpu, pool;
	struct pp3_group *group_ctrl;

	/* init EMAC */
	for (i = 0; i < MV_PP3_EMAC_NUM; i++)
		/* TODO: suppot NSS mode */
		mv_pp3_emac_init(dev_priv->index);

	/* int HMAC RXQs and TXQs */
	for_each_possible_cpu(cpu) {
		struct pp3_rxq *rxq_ctrl;
		struct pp3_txq *txq_ctrl;
		group_ctrl = pp3_groups[cpu][dev_priv->index];

		/*pool = mv_pp3_cfg_dp_long_bpid(dev_priv->index)*/
		group_ctrl->long_pool = pp3_pool_gp_create(pool, MV_PP3_LONG_POOL_SIZE);
		/*pool = mv_pp3_cfg_dp_short_bpid(dev_priv->index)*/
		group_ctrl->short_pool = pp3_pool_gp_create(pool, MV_PP3_SHORT_POOL_SIZE);
		/*pool = mv_pp3_cfg_dp_lro_bpid(dev_priv->index)*/
		group_ctrl->lro_pool = pp3_pool_gp_create(pool, MV_PP3_LRO_POOL_SIZE);

		for (i = 0; i < group_ctrl->rxqs_num; i++) {
			rxq_ctrl =  group_ctrl->rxqs[i];
			mv_pp3_hmac_rxq_init(rxq_ctrl->frame_num, rxq_ctrl->sw_q, rxq_ctrl->size);
		}

		for (i = 0; i < group_ctrl->txqs_num; i++) {
			txq_ctrl =  group_ctrl->txqs[i];
			mv_pp3_hmac_txq_init(txq_ctrl->frame_num, txq_ctrl->sw_q, txq_ctrl->size, 0);
		}
	}

	return 0;
}

/*---------------------------------------------------------------------------*/

static int mv_pp3_hw_shared_start(void)
{
	struct pp3_cpu *cpu_ctrl;
	int cpu, pool, frame, queue, q_size, group, irq;
	unsigned int frames_bmp;

	/* load fw */

	/* init cpu's structures */
	for_each_possible_cpu(cpu) {
		cpu_ctrl = kzalloc(sizeof(struct pp3_cpu), GFP_KERNEL);

		if (!cpu_ctrl)
			goto oom;

		pp3_cpus[cpu] = cpu_ctrl;

		/* TODO: call to config manager: get frames bitmap per cpu */
		/*mv_pp3_cfg_dp_frames_bm(int cpu, &frames_bmp);*/
		cpu_ctrl->frame_bmp = frames_bmp;

		/* TODO: call to config manager: get free pool id */
		/* pool = mv_pp3_cfg_dp_linux_bpid(cpu);*/
		cpu_ctrl->tx_done_pool =  pp3_pool_gp_create(pool, MV_PP3_LINUX_POOL_SIZE);

		/* TODO: call to config manager: get frame and queue num in order to manage bm pool */
		/*mv_pp3_cfg_dp_bmq_params_get(cpu, &frames_bmp, &qeueu, &q_size, &group, &irq);*/
		cpu_ctrl->bm_msg_irq = irq;
		cpu_ctrl->bm_msg_group = group;
		mv_pp3_hmac_bm_queue_init(frame, queue, q_size);
		cpu_ctrl->bm_msg_tasklet = kzalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
		tasklet_init(pp3_cpus[cpu]->bm_msg_tasklet, mv_pp3_bm_tasklet, (unsigned long)pp3_cpus[cpu]);

		/* init timer */
		cpu_ctrl->tx_done_timer.function = mv_pp3_tx_done_timer_callback;
		init_timer(&cpu_ctrl->tx_done_timer);
		clear_bit(MV_CPU_F_TX_DONE_TIMER_BIT, &cpu_ctrl->flags);
		cpu_ctrl->tx_done_timer.data = (unsigned long)pp3_cpus[cpu];
	}

	/* TODO: QM  HW init	*/
	/* TODO: HMAC HW unit int */

	pp3_pools_dram_init(BM_DRAM_POOL_CAPACITY);
	pp3_pools_dram_init(BM_GPM_POOL_CAPACITY);
	/*bm_enable();*/

	/* TODO: start fw */
	/* TODO: Channel create */

	cpu_ctrl->chan_id = mv_pp3_chan_create(MV_PP3_CHAN_SIZE, 0, pp3_chan_callback);

	return 0;
oom:
	for_each_possible_cpu(cpu) {
		if (pp3_cpus[cpu]) {
			pp3_pool_release(cpu_ctrl->tx_done_pool->pool);
			kfree(cpu_ctrl->bm_msg_tasklet);
			kfree(pp3_cpus[cpu]);
		}
	}

	return -ENOMEM;
}

/*---------------------------------------------------------------------------*/
/* alloc global structure memory					     */
/*---------------------------------------------------------------------------*/
static int mv_pp3_sw_shared_probe(struct platform_device *pdev)
{
	int i;
	unsigned int silicon_base = mv_hw_silicon_base_addr_get();

	struct mv_pp3_plat_data *plat_data = (struct mv_pp3_plat_data *)pdev->dev.platform_data;

	pp3_ports_num = plat_data->max_port;

	pp3_sysfs_init();

	pp3_ports = kzalloc(pp3_ports_num * sizeof(struct pp3_dev_priv *), GFP_KERNEL);
	if (!pp3_ports)
		goto oom;

	pp3_cpus = kzalloc(nr_cpu_ids * sizeof(struct pp3_cpu *), GFP_KERNEL);
	if (!pp3_cpus)
		goto oom;

	pp3_pools =  kzalloc(MV_PP3_BM_POOLS * sizeof(struct pp3_pool *), GFP_KERNEL);
	if (!pp3_pools)
		goto oom;

	pp3_frames = kzalloc(MV_PP3_FRAMES * sizeof(struct pp3_frame *), GFP_KERNEL);

	mv_pp3_hmac_gl_unit_base(silicon_base + MV_PP3_HMAC_GL_UNIT_OFFSET);
	mv_pp3_hmac_frame_unit_base(silicon_base + MV_PP3_HMAC_FR_UNIT_OFFSET, MV_PP3_HMAC_FR_INST_OFFSET);

	for (i = 0; i < MV_PP3_FRAMES; i++) {
		pp3_frames[i] = kzalloc(sizeof(struct pp3_frame), GFP_KERNEL);
		pp3_frames[i]->frame = i;
		pp3_frames[i]->time_coal[0] = MV_PP3_FRM_TIME_COAL_0;
	}

	/*mv_pp3_bm_unit_base(PP3_BM_BASE);*/
	/*mv_pp3_qm_unit_base(PP3_QM_BASE);*/
	/*mv_pp3_messenger_init();*/
	/*on_each_cpu(mv_pp3_def_chan_create, &size, 1);*/

	return 0;

oom:
	kfree(pp3_ports);
	kfree(pp3_cpus);
	kfree(pp3_pools);
	pr_err("%s: out of memory\n", __func__);
	return -ENOMEM;
}
/*---------------------------------------------------------------------------*/
static int mv_pp3_netif_init(struct pp3_dev_priv *dev_priv)
{
	if (!pp3_hw_initialized) {
		mv_pp3_hw_shared_start();
		pp3_hw_initialized = 1;
	}

	if (!(dev_priv->flags & MV_ETH_F_INIT)) {
		mv_pp3_dev_priv_init(dev_priv);
		mv_pp3_hw_netif_start(dev_priv);
		dev_priv->flags |= MV_ETH_F_INIT;
	}
	/* start seq */
	return 0;
}

/*---------------------------------------------------------------------------*/

static int mv_pp3_dev_open(struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv = MV_PP3_PRIV(dev);

	mv_pp3_netif_init(dev_priv);

	return 0;
}

/*---------------------------------------------------------------------------*/

static int mv_pp3_sw_shared_remove(struct platform_device *pdev)
{
	/* free all shared resources */
	return 0;
}

#ifdef MV_FPGA
static int mv_pp3_pci_probe(struct pci_dev *pdev,
	const struct pci_device_id *ent)
{
	/* code below relevant fot FPGA only */
	if (pci_enable_device(pdev)) {
		pr_err("Cannot enable PCI device, aborting\n");
		return -1;
	}

	if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
		pr_err("Cannot find proper PCI device base address, aborting\n");
		return -ENODEV;
	}

	/* Override memory size to 1GB */
	/* Disable BARs1 & 2 for FPGA platform - As they are irrelevant. */
	/*pci_resource_start(pdev, 1) = 0x0;
	pci_resource_end(pdev, 1) = 0x0;
	pci_resource_start(pdev, 2) = 0x0;
	pci_resource_end(pdev, 2) = 0x0;*/

	if (pci_request_regions(pdev, "mv_pp3_pci")) {
		pr_err("Cannot obtain PCI resources, aborting\n");
		return -ENODEV;
	}

	if (pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
		pr_err("No usable DMA configuration, aborting\n");
		return -ENODEV;
	}

	mv_vbase_address = (u32)pci_iomap(pdev, 0, 16*1024*1024);
	if (!mv_vbase_address)
		pr_err("Cannot map device registers, aborting\n");

	pr_info("\nSYSTEM VIRTUAL base is 0x%0x.\n", mv_vbase_address);
	return 0;
}

static void mv_pp3_pci_remove(struct pci_dev *pdev)
{
}

/*---------------------------------------------------------------------------*/
static DEFINE_PCI_DEVICE_TABLE(fpga_id_table) = {

	{ 0x1234, 0x1234, PCI_ANY_ID, PCI_ANY_ID, 2, 0, },
};
MODULE_DEVICE_TABLE(pci, fpga_id_table);

static struct pci_driver mv_pp3_pci_driver = {
	.name	= "mv_pp3_pci",
	.id_table = fpga_id_table,
	.probe		= mv_pp3_pci_probe,
	.remove		= mv_pp3_pci_remove,
};
#endif

static struct platform_driver mv_pp3_shared_driver = {
	.probe		= mv_pp3_sw_shared_probe,
	.remove		= mv_pp3_sw_shared_remove,
	.driver = {
		.name	= MV_PP3_SHARED_NAME,
		.owner	= THIS_MODULE,
	},
};


/*---------------------------------------------------------------------------*/

static int __init mv_pp3_init_module(void)
{
	int rc;
#ifdef MV_FPGA
	rc = pci_register_driver(&mv_pp3_pci_driver);
	if (rc < 0)
		pr_err("%sNo cards found, driver not installed.\n", __func__);
#endif
	rc = platform_driver_register(&mv_pp3_shared_driver);
	if (!rc) {
		rc = platform_driver_register(&mv_pp3_driver);
		if (rc)
			platform_driver_unregister(&mv_pp3_shared_driver);
	}

	return rc;
}
module_init(mv_pp3_init_module);

/*---------------------------------------------------------------------------*/
static void __exit mv_pp3_cleanup_module(void)
{
	platform_driver_unregister(&mv_pp3_driver);
	platform_driver_unregister(&mv_pp3_shared_driver);
}
module_exit(mv_pp3_cleanup_module);

/*---------------------------------------------------------------------------*/
/*				dump functions				     */
/*---------------------------------------------------------------------------*/

void pp3_netdev_pool_status_print(int pool)
{
	const char *str;
	struct pp3_pool *ppool;

	if ((pool < 0) || (pool >= MV_PP3_POOL_MAX)) {
		pr_err("%s: Invalid pool number - %d\n", __func__, pool);
		return;
	}

	if (!pp3_pools || (!pp3_pools[pool])) {
		pr_err("Pool #%d not initialized\n", pool);
		return;
	}

	ppool = pp3_pools[pool];

	if (ppool->type == PP3_POOL_TYPE_GPM)
		str = "GPM";
	else if (ppool->type == PP3_POOL_TYPE_DRAM)
		str = "DRAM";
	else if (ppool->flags & POOL_F_FREE)
		str = "FREE";
	else if (ppool->flags & POOL_F_LONG)
		str = "LONG";
	else if (ppool->flags & POOL_F_SHORT)
		str = "SHORT";
	else if (ppool->flags & POOL_F_LRO)
		str = "LRO";
	else
		str = "UNKNOWN";

	pr_info("/n");
	pr_info("pool #%d: pool type = %s, buffers num = %d, buffer size = %d\n",
			pool, str, ppool->buf_num, ppool->buf_size);
	pr_info("virt_base = 0x%p, phys_base = 0x%lu\n", ppool->virt_base, ppool->phys_base);

	return;
}
/*---------------------------------------------------------------------------*/
void pp3_netdev_cpu_status_print(int cpu)
{
	struct pp3_cpu *cpu_ctrl;
	int linux_pool_id;

	if ((cpu < 0) || (cpu >= MAX_CPU_NUM)) {
		pr_err("%s: Invalid cpu number - %d\n", __func__, cpu);
		return;
	}

	if (!pp3_cpus || (!pp3_cpus[cpu])) {
		pr_err("cpu #%d group not initialized\n", cpu);
		return;
	}

	cpu_ctrl = pp3_cpus[cpu];
	pr_info("/n");
	pr_info("cpu #%d: frames bitmap = 0x%x, bm_msg_irq = %d, bm_msg_groeup = %d, chan_id = %d\n", cpu,
			cpu_ctrl->frame_bmp, cpu_ctrl->bm_msg_irq, cpu_ctrl->bm_msg_group, cpu_ctrl->chan_id);

	linux_pool_id = cpu_ctrl->tx_done_pool ? cpu_ctrl->tx_done_pool->pool : -1;
	pr_info("linux_pool = %d, tx_done_counter = %d\n", linux_pool_id, cpu_ctrl->tx_done_cnt);

	return;
}
/*---------------------------------------------------------------------------*/
void pp3_netdev_dev_status_print(int index)
{
	struct pp3_dev_priv *dev_priv;

	if ((index < 0) || (index >= MV_PP3_EMAC_NUM)) {
		pr_err("%s: Invalid interface number - %d\n", __func__, index);
		return;
	}
	if (!pp3_ports || (!pp3_ports[index])) {
		pr_err("interface #%d is not initialized\n", index);
		return;
	}

	dev_priv = pp3_ports[index];

	pr_info("/n");
	pr_info("interface #%d: dev = 0x%p, flags = 0x%lu, rss_id = %d, rxqs_num= %d, txqs_num = %d\n",
		index, dev_priv->dev, dev_priv->flags, dev_priv->rss_id, dev_priv->rxqs_num,  dev_priv->txqs_num);
	return;
}
/*---------------------------------------------------------------------------*/

void pp3_netdev_group_status_print(int index, int cpu)
{
	struct pp3_group  *grp_ctrl;
	int lro_pool, long_pool, short_pool;

	if ((index < 0) || (index >= MV_PP3_EMAC_NUM)) {
		pr_err("%s: Invalid interface number - %d\n", __func__, index);
		return;
	}

	if ((cpu < 0) || (cpu >= MAX_CPU_NUM)) {
		pr_err("%s: Invalid cpu number - %d\n", __func__, cpu);
		return;
	}

	grp_ctrl = pp3_groups[cpu][index];

	if (!grp_ctrl) {
		pr_err("cpu #%d group not initialized\n", cpu);
		return;
	}

	pr_info("/n");
	pr_info("group: cpu = %d, interface = %d, rxqs_num= %d, txqs_num = %d\n",
			cpu, index, grp_ctrl->rxqs_num, grp_ctrl->txqs_num);

	lro_pool = grp_ctrl->lro_pool ? grp_ctrl->lro_pool->pool : -1;
	long_pool = grp_ctrl->long_pool ? grp_ctrl->long_pool->pool : -1;
	short_pool = grp_ctrl->short_pool ? grp_ctrl->short_pool->pool : -1;

	pr_info("short_pool = %d, long_pool = %d, lro_pool = %d\n", short_pool, long_pool, lro_pool);
	return;
}
/*---------------------------------------------------------------------------*/
void pp3_netdev_rxq_status_print(int index, int cpu, int queue)
{
	const char *type;
	struct pp3_group *grp_ctrl;
	struct pp3_rxq *rxq_ctrl;

	if ((index < 0) || (index >= MV_PP3_EMAC_NUM)) {
		pr_err("%s: Invalid interface number - %d\n", __func__, index);
		return;
	}

	if ((cpu < 0) || (cpu >= MAX_CPU_NUM)) {
		pr_err("%s: Invalid cpu number - %d\n", __func__, cpu);
		return;
	}

	grp_ctrl = pp3_groups[cpu][index];

	if (!grp_ctrl) {
		pr_err("cpu #%d group not initialized\n", cpu);
		return;
	}

	if (!grp_ctrl->rxqs || !grp_ctrl->rxqs[queue]) {
		pr_err("queue #%d not initialized\n", queue);
		return;
	}

	rxq_ctrl = grp_ctrl->rxqs[queue];
	type = (rxq_ctrl->type ==  PP3_Q_TYPE_BM) ? "BM" : "QM";
	pr_info("/n");
	pr_info("interface #%d, cpu group #%d, queue #%d:\n", index, cpu, queue);
	pr_info("frame = %d, logic_q = %d, sw_q = %d, hw_q = %d\n",
			rxq_ctrl->frame_num, rxq_ctrl->logic_q, rxq_ctrl->hw_q, rxq_ctrl->sw_q);

	pr_info("size = %d, type = %s, pkt_coal = %d, time_coal_frofile = %d\n",
		rxq_ctrl->size, type, rxq_ctrl->pkt_coal, rxq_ctrl->time_coal_profile);
	return;
}
/*---------------------------------------------------------------------------*/
void pp3_netdev_txq_status_print(int index, int cpu, int queue)
{
	const char *type;
	struct pp3_group *grp_ctrl;
	struct pp3_txq *txq_ctrl;

	if ((index < 0) || (index >= MV_PP3_EMAC_NUM)) {
		pr_err("%s: Invalid interface number - %d\n", __func__, index);
		return;
	}

	if ((cpu < 0) || (cpu >= MAX_CPU_NUM)) {
		pr_err("%s: Invalid cpu number - %d\n", __func__, cpu);
		return;
	}

	grp_ctrl = pp3_groups[cpu][index];

	if (!grp_ctrl) {
		pr_err("cpu #%d group not initialized\n", cpu);
		return;
	}

	if (!grp_ctrl->txqs || !grp_ctrl->txqs[queue]) {
		pr_err("queue #%d not initialized\n", queue);
		return;
	}

	txq_ctrl = grp_ctrl->txqs[queue];
	type = (txq_ctrl->type ==  PP3_Q_TYPE_BM) ? "BM" : "QM";
	pr_info("/n");
	pr_info("interface #%d, cpu group #%d, queue #%d:\n", index, cpu, queue);
	pr_info("frame = %d, logic_q = %d, sw_q = %d, hw_q = %d\n",
			txq_ctrl->frame_num, txq_ctrl->logic_q, txq_ctrl->sw_q, txq_ctrl->hw_q);
	pr_info("size = %d, type = %s\n", txq_ctrl->size, type);
	return;
}
/*---------------------------------------------------------------------------*/

MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MV_PP3_SHARED_NAME);
MODULE_ALIAS("platform:" MV_PP3_PORT_NAME);

