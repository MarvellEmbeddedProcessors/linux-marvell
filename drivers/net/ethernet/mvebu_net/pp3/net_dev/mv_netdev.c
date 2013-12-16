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

/* global data */
static int mv_eth_initialized;
static int mv_eth_ports_num;

struct eth_port **mv_eth_ports;

static int mv_eth_config_get(struct platform_device *pdev, u8 *mac_addr)
{
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	if (mac_addr)
		memcpy(mac_addr, plat_data->mac_addr, MV_MAC_ADDR_SIZE);

	return plat_data->mtu;
}

/***************************************************************
 * mv_eth_netdev_init -- Allocate and initialize net_device    *
 *                   structure                                 *
 ***************************************************************/
struct net_device *mv_eth_netdev_init(int mtu, u8 *mac, struct platform_device *pdev)
{
	struct net_device *dev;
	struct eth_port *dev_priv;
	struct resource *res;

	dev = alloc_etherdev_mq(sizeof(struct eth_port), CONFIG_MV_ETH_TXQ);
	if (!dev)
		return NULL;

	dev_priv = (struct eth_port *)netdev_priv(dev);
	if (!dev_priv)
		return NULL;

	memset(dev_priv, 0, sizeof(struct eth_port));

	dev_priv->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	BUG_ON(!res);
	dev->irq = res->start;

	dev->mtu = mtu;
	memcpy(dev->dev_addr, mac, MV_MAC_ADDR_SIZE);
	dev->tx_queue_len = CONFIG_MV_ETH_TXQ_DESC;
	dev->watchdog_timeo = 5 * HZ;

	/*dev->netdev_ops = &mv_eth_netdev_ops;*/

	/*SET_ETHTOOL_OPS(dev, &mv_eth_tool_ops);*/

	SET_NETDEV_DEV(dev, &pdev->dev);

	return dev;

}

static int mv_eth_load_network_interfaces(struct platform_device *pdev)
{
	u32 port, phys_port;
	int mtu;
	struct eth_port *pp;
	struct net_device *dev;
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;
	u8 mac[MV_MAC_ADDR_SIZE];

	port = pdev->id;
	phys_port = port; /*MV_PPV3_PORT_PHYS(port);*/
	pr_info("  o Loading network interface(s) for port #%d: mtu=%d\n", port, plat_data->mtu);

	mtu = mv_eth_config_get(pdev, mac);

	dev = mv_eth_netdev_init(mtu, mac, pdev);

	if (dev == NULL) {
		pr_err("\to %s: can't create netdevice\n", __func__);
		return -EIO;
	}

	pp = (struct eth_port *)netdev_priv(dev);
	pp->plat_data = plat_data;

	mv_eth_ports[port] = pp;

	return 0;
}

/* Support per port for platform driver */
static int mv_pp3_probe(struct platform_device *pdev)
{
	struct mv_pp3_port_data *plat_data = (struct mv_pp3_port_data *)pdev->dev.platform_data;

	if (mv_eth_load_network_interfaces(pdev))
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
	struct mv_pp3_plat_data *plat_data = (struct mv_pp3_plat_data *)pdev->dev.platform_data;
	struct resource *res;
	int size;

	int ret;

	ret = -EINVAL;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL)
		goto out;
/*
	ret = -ENOMEM;
	msp = kzalloc(sizeof(*msp), GFP_KERNEL);
	if (msp == NULL)
		goto out;
*/
	/*
	 * Check whether the error interrupt is hooked up.
	 */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
/*	if (res != NULL) {
		int err;

		err = request_irq(res->start, mv643xx_eth_err_irq,
				  IRQF_SHARED, "mv643xx_eth", msp);
		if (!err) {
			writel(ERR_INT_SMI_DONE, msp->base + ERR_INT_MASK);
			msp->err_interrupt = res->start;
		}
	}*/

	mv_eth_ports_num = plat_data->max_port;
	/*mv_eth_sysfs_init();*/
	/*mv_eth_win_init();*/
	/*mv_eth_config_show();*/

	size = mv_eth_ports_num * sizeof(struct eth_port *);
	mv_eth_ports = kzalloc(size, GFP_KERNEL);
	if (!mv_eth_ports)
		goto out;

	memset(mv_eth_ports, 0, size);

	/*if (mv_eth_bm_pools_init())
		goto oom;*/

	/* Initialize tasklet for handle link events */
	/*tasklet_init(&link_tasklet, mv_eth_link_tasklet, 0);*/

	/* request IRQ for link interrupts from GOP */
	/*if (request_irq(IRQ_GLOBAL_GOP, mv_eth_link_isr, (IRQF_DISABLED|IRQF_SAMPLE_RANDOM), "mv_eth_link", NULL))
		printk(KERN_ERR "%s: Could not request IRQ for GOP interrupts\n", __func__);*/

	mv_eth_initialized = 1;

	/*platform_set_drvdata(pdev, msp);*/

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

