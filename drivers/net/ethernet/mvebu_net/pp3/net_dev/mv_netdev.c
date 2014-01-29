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
struct pp3_group_stats **pp3_groups;
struct pp3_cpu *pp3_cpus;
static int pp3_ports_num;
static int pp3_initialized;


/****************************************************************
 * mv_pp3_netdev_init						*
 *	Allocate and initialize net_device structures		*
 ***************************************************************/

struct net_device *mv_pp3_netdev_init(int mtu, u8 *mac, struct platform_device *pdev)
{
	struct net_device *dev;
	struct pp3_dev_priv *dev_priv;
	struct resource *res;

	dev = alloc_etherdev_mq(sizeof(struct pp3_dev_priv), CONFIG_MV_ETH_TXQ);
	if (!dev)
		return NULL;

	dev_priv = (struct pp3_dev_priv *)netdev_priv(dev);
	if (!dev_priv)
		return NULL;

	memset(dev_priv, 0, sizeof(struct pp3_dev_priv));

	dev_priv->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	BUG_ON(!res);
	dev->irq = res->start;

	dev->mtu = mtu;
	memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);

	dev->tx_queue_len = CONFIG_MV_ETH_TXQ_DESC;
	dev->watchdog_timeo = 5 * HZ;

	/* TODO: init eth_tools */

	SET_NETDEV_DEV(dev, &pdev->dev);

	return dev;

}

static int mv_pp3_config_get(struct platform_device *pdev, unsigned char *mac_addr)
{
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	if (mac_addr)
		memcpy(mac_addr, plat_data->mac_addr, MV_MAC_ADDR_SIZE);

	return plat_data->mtu;
}

static int mv_pp3_load_network_interfaces(struct platform_device *pdev)
{
	int mtu;
	struct pp3_dev_priv *priv;
	struct net_device *dev;
	u8 mac[MV_MAC_ADDR_SIZE];

	mtu = mv_pp3_config_get(pdev, mac);

	pr_info("  o Loading network interface(s) for port #%d: mtu=%d\n", pdev->id, mtu);

	dev = mv_pp3_netdev_init(mtu, mac, pdev);

	if (dev == NULL) {
		pr_err("\to %s: can't create netdevice\n", __func__);
		return -EIO;
	}

	priv = MV_PP3_PRIV(dev);
	priv->plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	pp3_ports[pdev->id] = priv;

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


MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" MV_PP3_SHARED_NAME);
MODULE_ALIAS("platform:" MV_PP3_PORT_NAME);

