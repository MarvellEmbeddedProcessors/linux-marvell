/*#include <mvCopyright.h>*/

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
#include <linux/mv_pp3.h>
#include "common/mv_hw_if.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_bm.h"
#include "mv_netdev.h"
#include "mv_netdev_structs.h"

/* global data */
struct pp3_dev_priv **pp3_ports;
struct pp3_group **pp3_groups;
struct pp3_cpu **pp3_cpus;
static int pp3_ports_num;
static int pp3_initialized;

/* functions */
static int mv_pp3_poll(struct napi_struct *napi, int budget);

/****************************************************************
 * mv_pp3_netdev_init						*
 *	Allocate and initialize net_device structures		*
 ***************************************************************/

struct net_device *mv_pp3_netdev_init(int mtu, u8 *mac, struct platform_device *pdev)
{
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	struct net_device *dev;
	struct resource *res;

	dev = alloc_etherdev_mqs(sizeof(struct pp3_dev_priv), plat_data->tx_queue_count, plat_data->tx_queue_count);
	if (!dev)
		return NULL;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	BUG_ON(!res);
	dev->irq = res->start;

	dev->mtu = mtu;
	memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);

	dev->tx_queue_len = plat_data->tx_queue_size;
	dev->watchdog_timeo = 5 * HZ;

	/* TODO: init eth_tools */

	SET_NETDEV_DEV(dev, &pdev->dev);

	return dev;

}

/****************************************************************
 * mv_pp3_priv_init						*
 *	Allocate and initialize net_device private structures	*
 ***************************************************************/

static int mv_pp3_dev_priv_init(int index, struct net_device *dev)
{
	struct pp3_dev_priv *dev_priv;
	int cpu, num, first, frame, i;

	dev_priv = MV_PP3_PRIV(dev);

	memset(dev_priv, 0, sizeof(struct pp3_dev_priv));

	dev_priv->dev = dev;
	dev_priv->index = index;

	/* create group per each cpu */
	for_each_possible_cpu(cpu) {
		struct pp3_group *group;

		dev_priv->groups[cpu] = kmalloc(sizeof(struct pp3_dev_priv), GFP_KERNEL);
		memset(dev_priv->groups[cpu], 0, sizeof(struct pp3_dev_priv));

		group = dev_priv->groups[cpu];

		/* init group rxqs */
		/*pp3_config_mngr_rxq(emac_map, cpu, &first, &num, &frame);*/
		group->rxqs_num = num;
		group->rxqs = kmalloc(sizeof(struct pp3_rxq *) * num, GFP_KERNEL);
		memset(group->rxqs, 0, sizeof(struct pp3_rxq *) * num);

		for (i = 0; i < num; i++) {
			group->rxqs[i] = kmalloc(sizeof(struct pp3_rxq), GFP_KERNEL);
			memset(group->rxqs[i], 0, sizeof(struct pp3_rxq) * num);
			group->rxqs[i]->frame_num = frame;
			group->rxqs[i]->logic_q = i;
			group->rxqs[i]->phys_q = first + i;
			group->rxqs[i]->type = PP3_Q_TYPE_QM;
			/*mv_pp3_hmac_rxq_init(frame, first + i);*/
			/*group->rxqs[i]->hmac_queue = mv_pp3_hmac_rxq_get(frame, firts + i);*/
			group->rxqs[i]->dev_priv = dev_priv;
			group->rxqs[i]->pkt_coal = CONFIG_PP3_RX_COAL_PKTS;
			group->rxqs[i]->time_coal = CONFIG_PP3_RX_COAL_USEC;
		}

		/* get emac bitmap */
		/*pp3_config_mngr_emac_map(dev_priv->index, &dev_priv->emac_map);*/

		/* init group txqs */
		/*pp3_config_mngr_txq(dev_priv->index, cpu, &first, &num, &frame);*/
		group->txqs_num = num;
		group->txqs = kmalloc(sizeof(struct pp3_txq *) * num, GFP_KERNEL);
		memset(group->txqs, 0, sizeof(struct pp3_txq *) * num);

		for (i = 0; i < num; i++) {
			group->txqs[i] = kmalloc(sizeof(struct pp3_txq) * num, GFP_KERNEL);
			memset(group->txqs[i], 0, sizeof(struct pp3_txq) * num);
			group->txqs[i]->frame_num = frame;
			group->txqs[i]->logic_q = i;
			group->txqs[i]->phys_q = first + i;
			group->txqs[i]->type = PP3_Q_TYPE_QM;
			/*mv_pp3_hmac_txq_init(frame, first + i);*/
			/*group->txqs[i]->hmac_queue = mv_pp3_hmac_txq_get(frame, firts + i);*/
			group->txqs[i]->dev_priv = dev_priv;
		}

		/* set group cpu control */
		group->cpu_ctrl = pp3_cpus[cpu];

		/* init group napi */
		group->napi = kmalloc(sizeof(struct napi_struct), GFP_KERNEL);

		if (!group->napi) {
			/* TODO: call cleanup function */
			return -EIO;
		}

		memset(group->napi, 0, sizeof(struct napi_struct));
		netif_napi_add(dev, group->napi, mv_pp3_poll, CONFIG_MV_ETH_RX_POLL_WEIGHT);


		/* TODO: init pools */

	} /* for */

	return 0;
}


static int mv_pp3_config_get(struct platform_device *pdev, unsigned char *mac_addr, int *index)
{
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	if (mac_addr)
		memcpy(mac_addr, plat_data->mac_addr, MV_MAC_ADDR_SIZE);

	if (index)
		*index = pdev->id;

	return plat_data->mtu;
}

static int mv_pp3_load_network_interfaces(struct platform_device *pdev)
{
	int mtu, ret, index;
	struct net_device *dev;
	u8 mac[MV_MAC_ADDR_SIZE];

	/* TODO: move function to configure block */
	mtu = mv_pp3_config_get(pdev, mac, &index);

	pr_info("  o Loading network interface(s) for port #%d: mtu=%d\n", pdev->id, mtu);

	dev = mv_pp3_netdev_init(mtu, mac, pdev);

	if (dev == NULL) {
		pr_err("\to %s: can't create netdevice\n", __func__);
		return -EIO;
	}

	ret = mv_pp3_dev_priv_init(pdev->id, dev);

	if (ret)
		return ret;

	pp3_ports[pdev->id] = MV_PP3_PRIV(dev);

	return 0;
}

/* Support per port for platform driver */
static int mv_pp3_probe(struct platform_device *pdev)
{
	if (mv_pp3_load_network_interfaces(pdev))
		return -ENODEV;

	pr_info("Probing Marvell PPv3 Network Driver\n");
	return 0;
}

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

static int mv_pp3_remove(struct platform_device *pdev)
{
	pr_info("Removing Marvell PPv3 Network Driver\n");
	return 0;
}

static void mv_pp3_shutdown(struct platform_device *pdev)
{
	pr_info("Shutting Down Marvell PPv3 Network Driver\n");
}

static struct platform_driver mv_pp3_driver = {
	.probe = mv_pp3_probe,
	.remove = mv_pp3_remove,
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

/*
 * Global units (hmac, qm/bm and etc.) init functions
*/
static int mv_pp3_shared_probe(struct platform_device *pdev)
{
	int size, ret;
	struct mv_pp3_plat_data *plat_data = (struct mv_pp3_plat_data *)pdev->dev.platform_data;

	pp3_ports_num = plat_data->max_port;

	/* TODO:
		init sysfs
		init window */

	size = pp3_ports_num * sizeof(struct pp3_dev_priv *);
	pp3_ports = kzalloc(size, GFP_KERNEL);
	if (!pp3_ports)
		goto out;

	memset(pp3_ports, 0, size);

	/* if (mv_eth_bm_pools_init())
		goto oom;*/

	/* TODO: set links interrupt */

	pp3_initialized = 1;


	return 0;

out:
	return ret;
}

static int mv_pp3_shared_remove(struct platform_device *pdev)
{
	/* free all shared resources */
	return 0;
}

static struct platform_driver mv_pp3_shared_driver = {
	.probe		= mv_pp3_shared_probe,
	.remove		= mv_pp3_shared_remove,
	.driver = {
		.name	= MV_PP3_SHARED_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init mv_pp3_init_module(void)
{
	int rc;

	rc = platform_driver_register(&mv_pp3_shared_driver);
	if (!rc) {
		rc = platform_driver_register(&mv_pp3_driver);
		if (rc)
			platform_driver_unregister(&mv_pp3_shared_driver);
	}

	return rc;
}
module_init(mv_pp3_init_module);

static void __exit mv_pp3_cleanup_module(void)
{
	platform_driver_unregister(&mv_pp3_driver);
	platform_driver_unregister(&mv_pp3_shared_driver);
}
module_exit(mv_pp3_cleanup_module);

/****************************************************************
 * mv_pp3_isr							*
 *	rx events , group interrupt handle			*
 ***************************************************************/
irqreturn_t mv_pp3_isr(int irq, int group_id)
{
	int cpu = smp_processor_id();
	struct pp3_group *group = &pp3_groups[cpu][group_id];
	struct napi_struct *napi = group->napi;

	STAT_INFO(group->stats.irq++);

	/* TODO: interrupts Mask */

	/* Verify that the device not already on the polling list */
	if (napi_schedule_prep(napi)) {
		/* schedule the work (rx+txdone+link) out of interrupt contxet */
		__napi_schedule(napi);
	} else {
		STAT_INFO(group->stats.irq_err++);
	}

	/* TODO: interrupts unMask */

	return IRQ_HANDLED;
}

/****************************************************************
 * mv_pp3_poll							*
 *	napi func - call to mv_pp3_rx for group's rxqs		*
 ***************************************************************/
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

	/* TODO */


	while (budget > 0 /* && group rxqs are not empty */) {

		/* TODO
			select rx_queue
			call to mv_pp3_rx()
			update counters and budget
		*/
	}

	if (budget > 0)
		napi_complete(napi);


	return rx_done;
}

/****************************************************************
 * mv_pp3_linux_pool_isr					*
 *	linux poll full interrupt handler			*
 ***************************************************************/
irqreturn_t mv_pp3_linux_pool_isr(int irq, int group_id)
{
	int cpu = smp_processor_id();
	struct pp3_cpu *cpu_ctrl = pp3_cpus[cpu];

	STAT_INFO(cpu_ctrl->stats.lnx_pool_irq++);

	/* TODO: interrupts Mask */

	tasklet_schedule(&cpu_ctrl->bm_msg_tasklet);

	/* TODO: interrupts UnMask */

	return IRQ_HANDLED;

}

void mv_pp3_bm_tasklet(unsigned long data)
{
	/* TODO
		while (pool is not empty)
		1 - read cfh from bm_msg_queue
		2 - get pool ID from cfh
		3 - get buffer ptr from cfh
		4 - if pool ID is linux_pool_tx
			free buffer
		    else
			use callback function, send buffer ptr to application
	*/
}

/****************************************************************
 * mv_pp3_fw_isr					*
 *	linux poll full interrupt handler			*
 ***************************************************************/
irqreturn_t mv_pp3_fw_isr(int irq, int group_id)
{
	int cpu = smp_processor_id();
	struct pp3_cpu *cpu_ctrl = pp3_cpus[cpu];

	STAT_INFO(cpu_ctrl->stats.lnx_fw_irq++);

	/* TODO: interrupts Mask */

	tasklet_schedule(&cpu_ctrl->fw_msg_tasklet);

	/* TODO: interrupts UnMask */

	return IRQ_HANDLED;

}

void mv_pp3_fw_tasklet(unsigned long data)
{
	/* TODO */
}


MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MV_PP3_SHARED_NAME);
MODULE_ALIAS("platform:" MV_PP3_PORT_NAME);

