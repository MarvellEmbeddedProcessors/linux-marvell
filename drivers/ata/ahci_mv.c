/*
 * ahci_mv.c - Marvell AHCI SATA platform support
 *
 * Copyright 2013: Marvell Corporation, all rights reserved.
 *
 * based on the AHCI SATA platform driver by Jeff Garzik and Anton Vorontsov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>
#include <linux/ahci_platform.h>
#include <linux/ata_platform.h>
#include <linux/mbus.h>
#include "ahci.h"

#define AHCI_WINDOW_CTRL(win)	(0x60 + ((win) << 4))
#define AHCI_WINDOW_BASE(win)	(0x64 + ((win) << 4))
#define AHCI_WINDOW_SIZE(win)	(0x68 + ((win) << 4))

static void ahci_mv_windows_config(struct ahci_host_priv *hpriv,
				   const struct mbus_dram_target_info *dram)
{
	int i;

	for (i = 0; i < 4; i++) {
		writel(0, hpriv->mmio + AHCI_WINDOW_CTRL(i));
		writel(0, hpriv->mmio + AHCI_WINDOW_BASE(i));
		writel(0, hpriv->mmio + AHCI_WINDOW_SIZE(i));
	}

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->mbus_attr << 8) |
		       (dram->mbus_dram_target_id << 4) | 1,
		       hpriv->mmio + AHCI_WINDOW_CTRL(i));
		writel(cs->base, hpriv->mmio + AHCI_WINDOW_BASE(i));
		writel(((cs->size - 1) & 0xffff0000),
		       hpriv->mmio + AHCI_WINDOW_SIZE(i));
	}
}

static void ahci_mv_host_stop(struct ata_host *host);

static struct ata_port_operations ahci_mv_ops = {
	.inherits = &ahci_ops,
	.host_stop = ahci_mv_host_stop,
};

static const struct ata_port_info ahci_mv_port_info = {
	.flags	   = AHCI_FLAG_COMMON,
	.pio_mask  = ATA_PIO4,
	.udma_mask = ATA_UDMA6,
	.port_ops  = &ahci_mv_ops,
};

static struct scsi_host_template ahci_mv_platform_sht = {
	AHCI_SHT("ahci_mv_platform"),
};

static const struct of_device_id ahci_mv_of_match[] = {
	{ .compatible = "marvell,ahci-sata", },
	{},
};
MODULE_DEVICE_TABLE(of, ahci_of_match);

static int ahci_mv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct mbus_dram_target_info *dram;
	struct ata_port_info pi = ahci_mv_port_info;
	const struct ata_port_info *ppi[] = { &pi, NULL };
	struct ahci_host_priv *hpriv;
	struct ata_host *host;
	struct resource *mem;
	int irq;
	int n_ports;
	int i;
	int rc;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(dev, "no mmio space\n");
		return -EINVAL;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "no irq\n");
		return -EINVAL;
	}

	hpriv = devm_kzalloc(dev, sizeof(*hpriv), GFP_KERNEL);
	if (!hpriv) {
		dev_err(dev, "can't alloc ahci_host_priv\n");
		return -ENOMEM;
	}

	hpriv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(hpriv->clk)) {
		dev_err(dev, "can't get clock\n");
		return PTR_ERR(hpriv->clk);
	}

	rc = clk_prepare_enable(hpriv->clk);
	if (rc < 0) {
		dev_err(dev, "can't enable clock\n");
		return rc;
	}

	hpriv->flags |= (unsigned long)pi.private_data;

	hpriv->mmio = devm_request_and_ioremap(dev, mem);
	if (!hpriv->mmio) {
		dev_err(dev, "can't map %pR\n", mem);
		clk_disable_unprepare(hpriv->clk);
		return -ENOMEM;
	}

	/*
	 * (Re-)program MBUS remapping windows if we are asked to.
	 */
	dram = mv_mbus_dram_info();
	if (dram)
		ahci_mv_windows_config(hpriv, dram);

	ahci_save_initial_config(dev, hpriv, 0, 0);

	/* prepare host */
	if (hpriv->cap & HOST_CAP_NCQ)
		pi.flags |= ATA_FLAG_NCQ;

	if (hpriv->cap & HOST_CAP_PMP)
		pi.flags |= ATA_FLAG_PMP;

	ahci_set_em_messages(hpriv, &pi);

	/*
	 * CAP.NP sometimes indicate the index of the last enabled
	 * port, at other times, that of the last possible port, so
	 * determining the maximum port number requires looking at
	 * both CAP.NP and port_map.
	 */
	n_ports = max(ahci_nr_ports(hpriv->cap), fls(hpriv->port_map));

	host = ata_host_alloc_pinfo(dev, ppi, n_ports);
	if (!host) {
		clk_disable_unprepare(hpriv->clk);
		return -ENOMEM;
	}

	host->private_data = hpriv;

	if (!(hpriv->cap & HOST_CAP_SSS) || ahci_ignore_sss)
		host->flags |= ATA_HOST_PARALLEL_SCAN;
	else
		printk(KERN_INFO "ahci: SSS flag set, parallel bus scan disabled\n");

	if (pi.flags & ATA_FLAG_EM)
		ahci_reset_em(host);

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];

		ata_port_desc(ap, "mmio %pR", mem);
		ata_port_desc(ap, "port 0x%x", 0x100 + ap->port_no * 0x80);

		/* set enclosure management message type */
		if (ap->flags & ATA_FLAG_EM)
			ap->em_message_type = hpriv->em_msg_type;

		/* disabled/not-implemented port */
		if (!(hpriv->port_map & (1 << i)))
			ap->ops = &ata_dummy_port_ops;
	}

	rc = ahci_reset_controller(host);
	if (rc) {
		clk_disable_unprepare(hpriv->clk);
		return rc;
	}

	ahci_init_controller(host);
	ahci_print_info(host, "platform");

	rc = ata_host_activate(host, irq, ahci_interrupt, IRQF_SHARED,
			       &ahci_mv_platform_sht);
	if (rc) {
		clk_disable_unprepare(hpriv->clk);
		return rc;
	}

	return 0;
}

static void ahci_mv_host_stop(struct ata_host *host)
{
	struct ahci_host_priv *hpriv = host->private_data;
	clk_disable_unprepare(hpriv->clk);
}

#ifdef CONFIG_PM_SLEEP
static int ahci_mv_suspend(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	void __iomem *mmio = hpriv->mmio;
	u32 ctl;
	int rc;

	if (hpriv->flags & AHCI_HFLAG_NO_SUSPEND) {
		dev_err(dev, "firmware update required for suspend/resume\n");
		return -EIO;
	}

	/*
	 * AHCI spec rev1.1 section 8.3.3:
	 * Software must disable interrupts prior to requesting a
	 * transition of the HBA to D3 state.
	 */
	ctl = readl(mmio + HOST_CTL);
	ctl &= ~HOST_IRQ_EN;
	writel(ctl, mmio + HOST_CTL);
	readl(mmio + HOST_CTL); /* flush */

	rc = ata_host_suspend(host, PMSG_SUSPEND);
	if (rc)
		return rc;

	return 0;
}

static int ahci_mv_resume(struct device *dev)
{
	struct ata_host *host = dev_get_drvdata(dev);
	struct ahci_host_priv *hpriv = host->private_data;
	const struct mbus_dram_target_info *dram;
	int rc;

	dram = mv_mbus_dram_info();
	if (dram)
		ahci_mv_windows_config(hpriv, dram);

	if (dev->power.power_state.event == PM_EVENT_SUSPEND) {
		rc = ahci_reset_controller(host);
		if (rc)
			return rc;

		ahci_init_controller(host);
	}

	ata_host_resume(host);

	return 0;
}
#endif

static SIMPLE_DEV_PM_OPS(ahci_mv_pm_ops, ahci_mv_suspend, ahci_mv_resume);

static struct platform_driver ahci_mv_driver = {
	.probe = ahci_mv_probe,
	.remove = ata_platform_remove_one,
	.driver = {
		.name = "ahci_mv",
		.owner = THIS_MODULE,
		.of_match_table = ahci_mv_of_match,
		.pm = &ahci_mv_pm_ops,
	},
};

module_platform_driver(ahci_mv_driver);

MODULE_DESCRIPTION("Marvell AHCI SATA platform driver");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci_mv");
