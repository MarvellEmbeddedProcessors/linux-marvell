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
/*#include <linux/mv_pp3.h>*/
#include <asm/setup.h>
#include <net/ip.h>
#include <net/ipv6.h>

#define MV_PP3_PORT_NAME        "mv_pp3_port"

/* Support for platform driver */
static int mv_pp3_probe(struct platform_device *pdev)
{
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
	},
};

static int __init mv_pp3_init_module(void)
{
	return platform_driver_register(&mv_pp3_driver);
}
module_init(mv_pp3_init_module);

static void __exit mv_pp3_cleanup_module(void)
{
	platform_driver_unregister(&mv_pp3_driver);
}
module_exit(mv_pp3_cleanup_module);


MODULE_DESCRIPTION("Marvell PPv3 Network Driver - www.marvell.com");
MODULE_AUTHOR("Dmitri Epshtein <dima@marvell.com>");
MODULE_LICENSE("GPL");

