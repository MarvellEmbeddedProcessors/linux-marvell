/*
 * AHCI glue platform driver for Marvell EBU SOCs
 *
 * Copyright (C) 2014 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 * Marcin Wojtas <mw@semihalf.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/ahci_platform.h>
#include <linux/kernel.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/libata.h>
#include "ahci.h"
#include <linux/mv_soc_info.h>

#define DRV_NAME "ahci-mvebu"

#define AHCI_VENDOR_SPECIFIC_0_ADDR  0xa0
#define AHCI_VENDOR_SPECIFIC_0_DATA  0xa4

#define AHCI_WINDOW_CTRL(win)	(0x60 + ((win) << 4))
#define AHCI_WINDOW_BASE(win)	(0x64 + ((win) << 4))
#define AHCI_WINDOW_SIZE(win)	(0x68 + ((win) << 4))

#define SATA3_VENDOR_ADDRESS			0xA0
#define SATA3_VENDOR_ADDR_OFSSET		0
#define SATA3_VENDOR_ADDR_MASK			(0xFFFFFFFF << SATA3_VENDOR_ADDR_OFSSET)
#define SATA3_VENDOR_DATA			0xA4

#define SATA_CONTROL_REG			0x0
#define SATA3_CTRL_SATA0_PD_OFFSET		6
#define SATA3_CTRL_SATA0_PD_MASK		(1 << SATA3_CTRL_SATA0_PD_OFFSET)
#define SATA3_CTRL_SATA1_PD_OFFSET		14
#define SATA3_CTRL_SATA1_PD_MASK		(1 << SATA3_CTRL_SATA1_PD_OFFSET)
#define SATA3_CTRL_SATA1_ENABLE_OFFSET		22
#define SATA3_CTRL_SATA1_ENABLE_MASK		(1 << SATA3_CTRL_SATA1_ENABLE_OFFSET)
#define SATA3_CTRL_SATA_SSU_OFFSET		23
#define SATA3_CTRL_SATA_SSU_MASK		(1 << SATA3_CTRL_SATA_SSU_OFFSET)

#define SATA_MBUS_SIZE_SELECT_REG		0x4
#define SATA_MBUS_REGRET_EN_OFFSET		7
#define SATA_MBUS_REGRET_EN_MASK		(0x1 << SATA_MBUS_REGRET_EN_OFFSET)

/*
 * MVEBU AHCI does not support Enclosure Management registers, so it can not
 * control LED through Enclosure Management registers; but it can turn on/off
 * LED though a GPIO.
 * MVEBU AHCI implements software activity LED blinking under Enclosure
 * Management framework by overwriting transmit_led_message callback function:
 *     - after a EM message is parsed, MVEBU AHCI does not set EM registers but
 *       set the GPIO state to turn on/off LED.
 * The software activity LED blinking only works when the SATA disk is
 * connected to the controller directly; it does not work when a PMP card is
 * attached to the controller and disks are connected to the PMP card since
 * in that case disks are not controlled by MVEBU AHCI controller but by PM
 * card actually.
 */
struct ahci_mvebu_priv {
	struct device		*dev;
	u32			led_gpio;
	enum of_gpio_flags	flags;
};

static void reg_set(void __iomem *addr, u32 data, u32 mask)
{
	u32 reg_data;

	reg_data = readl(addr);
	reg_data &= ~mask;
	reg_data |= data;
	writel(reg_data, addr);
}

static void ahci_mvebu_mbus_config(struct ahci_host_priv *hpriv,
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
		writel(cs->base >> 16, hpriv->mmio + AHCI_WINDOW_BASE(i));
		writel(((cs->size - 1) & 0xffff0000),
		       hpriv->mmio + AHCI_WINDOW_SIZE(i));
	}
}

static void ahci_mvebu_regret_option(struct ahci_host_priv *hpriv)
{
	/*
	 * Enable the regret bit to allow the SATA unit to regret a
	 * request that didn't receive an acknowlegde and avoid a
	 * deadlock
	 */
	writel(0x4, hpriv->mmio + AHCI_VENDOR_SPECIFIC_0_ADDR);
	writel(0x80, hpriv->mmio + AHCI_VENDOR_SPECIFIC_0_DATA);
}

/**
 * ahci_mvebu_pll_power_up
 *
 * @pdev:	A pointer to ahci platform device
 * @hpriv:	A pointer to achi host private structure
 * @pd_polarity: 1 or 0 to power down the PLL
 *
 * This function configures corresponding comphy to SATA mode.
 * AHCI driver acquires an handle to the corresponding PHY from
 * the device-tree (In ahci_platform_get_resources).
 * Mvebu SATA require the following sequence:
 *	1. Power down AHCI macs
 *	2. Configure the corresponding comphy (comphy driver).
 *	3. Power up AHCI macs
 *	4. Check if comphy PLL was locked
 *
 * Return: 0 on success; Error code otherwise.
 */
static int ahci_mvebu_pll_power_up(struct platform_device *pdev,
				   struct ahci_host_priv *hpriv,
				   u32 pd_polarity)
{
	u32 mask, data, i;
	int err = 0;

	/* Power off AHCI macs */
	reg_set(hpriv->mmio + SATA3_VENDOR_ADDRESS,
		SATA_CONTROL_REG << SATA3_VENDOR_ADDR_OFSSET,
		SATA3_VENDOR_ADDR_MASK);
	/* SATA port 0 power down */
	mask = SATA3_CTRL_SATA0_PD_MASK;
	/*
	 * Marvell SoC have different power down polarity.
	 * For Armada 3700, 0 means that power down the PLL, 1 means power up.
	 * but for CP110, 1 means to power down the PLL while 0 for power up.
	 */
	if (pd_polarity)
		data = 0x1 << SATA3_CTRL_SATA0_PD_OFFSET;
	else
		data = 0x0 << SATA3_CTRL_SATA0_PD_OFFSET;
	if (hpriv->nports > 1) {
		/* SATA port 1 power down */
		mask |= SATA3_CTRL_SATA1_PD_MASK;
		data |= 0x1 << SATA3_CTRL_SATA1_PD_OFFSET;
		/* SATA SSU disable */
		mask |= SATA3_CTRL_SATA1_ENABLE_MASK;
		data |= 0x0 << SATA3_CTRL_SATA1_ENABLE_OFFSET;
		/* SATA port 1 disable
		 * There's no option to disable SATA port 0, so we power down both
		 * ports (during previous steps) but disable only SATA port 1
		 */
		mask |= SATA3_CTRL_SATA_SSU_MASK;
		data |= 0x0 << SATA3_CTRL_SATA_SSU_OFFSET;
	}
	reg_set(hpriv->mmio + SATA3_VENDOR_DATA, data, mask);

	/* Configure corresponding comphy
	 * First we need to call phy_power_off because the phy_power_on
	 * was called by generic AHCI code.
	 * Next, we call phy_power_on in order to configure the comphy
	 * while AHCI is powered down.
	 */
	for (i = 0; i < hpriv->nports; i++) {
		err = phy_power_off(hpriv->phys[i]);
		if (err) {
			dev_err(&pdev->dev, "unable to power off SATA comphy\n");
			return -EINVAL;
		}
		err = phy_power_on(hpriv->phys[i]);
		if (err) {
			dev_err(&pdev->dev, "unable to power on SATA comphy\n");
			return -EINVAL;
		}
	}

	/* Power up AHCI macs */
	reg_set(hpriv->mmio + SATA3_VENDOR_ADDRESS,
		SATA_CONTROL_REG << SATA3_VENDOR_ADDR_OFSSET,
		SATA3_VENDOR_ADDR_MASK);
	/* SATA port 0 power up */
	mask = SATA3_CTRL_SATA0_PD_MASK;
	/*
	 * Marvell SoC have different power down polarity.
	 * For Armada 3700, 0 means that power down the PLL, 1 means power up.
	 * but for CP110, 1 means to power down the PLL while 0 for power down.
	 */
	if (pd_polarity)
		data = 0x0 << SATA3_CTRL_SATA0_PD_OFFSET;
	else
		data = 0x1 << SATA3_CTRL_SATA0_PD_OFFSET;
	if (hpriv->nports > 1) {
		/* SATA port 1 power up */
		mask |= SATA3_CTRL_SATA1_PD_MASK;
		data |= 0x0 << SATA3_CTRL_SATA1_PD_OFFSET;
		/* SATA SSU enable */
		mask |= SATA3_CTRL_SATA1_ENABLE_MASK;
		data |= 0x1 << SATA3_CTRL_SATA1_ENABLE_OFFSET;
		/* SATA port 1 enable */
		mask |= SATA3_CTRL_SATA_SSU_MASK;
		data |= 0x1 << SATA3_CTRL_SATA_SSU_OFFSET;
	}
	reg_set(hpriv->mmio + SATA3_VENDOR_DATA, data, mask);

	/* MBUS request size and interface select register */
	reg_set(hpriv->mmio + SATA3_VENDOR_ADDRESS,
		SATA_MBUS_SIZE_SELECT_REG << SATA3_VENDOR_ADDR_OFSSET,
		SATA3_VENDOR_ADDR_MASK);
	/* Mbus regret enable */
	reg_set(hpriv->mmio + SATA3_VENDOR_DATA,
		0x1 << SATA_MBUS_REGRET_EN_OFFSET,
		SATA_MBUS_REGRET_EN_MASK);

	/* Check if comphy PLL is locked */
	for (i = 0; i < hpriv->nports; i++) {
		err = phy_is_pll_locked(hpriv->phys[i]);
		if (err) {
			dev_err(&pdev->dev, "port %d: comphy PLL is not locked for SATA. Unable to power on SATA comphy\n",
				i);
			return err;
		}
	}

	return err;
}

/**
 * ahci_mvebu_stop_engine
 *
 * @ap:	Target ata port
 *
 * Errata Ref#226 - SATA Disk HOT swap issue when connected through
 * Port Multiplier in FIS-based Switching mode.
 *
 * To avoid the issue, according to design, the bits[11:8, 0] of
 * register PxFBS are cleared when Port Command and Status (0x18) bit[0]
 * changes its value from 1 to 0, i.e. falling edge of Port
 * Command and Status bit[0] sends PULSE that resets PxFBS
 * bits[11:8; 0].
 *
 * This function is used to override function of "ahci_stop_engine"
 * from libahci.c by adding the mvebu work around(WA) to save PxFBS
 * value before the PxCMD ST write of 0, then restore PxFBS value.
 *
 * Return: 0 on success; Error code otherwise.
 */
int ahci_mvebu_stop_engine(struct ata_port *ap)
{
	void __iomem *port_mmio = ahci_port_base(ap);
	u32 tmp, port_fbs;

	tmp = readl(port_mmio + PORT_CMD);

	/* check if the HBA is idle */
	if ((tmp & (PORT_CMD_START | PORT_CMD_LIST_ON)) == 0)
		return 0;

	/* save the port PxFBS register for later restore */
	port_fbs = readl(port_mmio + PORT_FBS);

	/* setting HBA to idle */
	tmp &= ~PORT_CMD_START;
	writel(tmp, port_mmio + PORT_CMD);

	/*
	 * bit #15 PxCMD signal doesn't clear PxFBS,
	 * restore the PxFBS register right after clearing the PxCMD ST,
	 * no need to wait for the PxCMD bit #15.
	 */
	writel(port_fbs, port_mmio + PORT_FBS);

	/* wait for engine to stop. This could be as long as 500 msec */
	tmp = ata_wait_register(ap, port_mmio + PORT_CMD,
				PORT_CMD_LIST_ON, PORT_CMD_LIST_ON, 1, 500);
	if (tmp & PORT_CMD_LIST_ON)
		return -EIO;

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int ahci_mvebu_suspend(struct platform_device *pdev, pm_message_t state)
{
	int err;
	struct ata_host *host = platform_get_drvdata(pdev);
	struct ahci_host_priv *hpriv = host->private_data;

	err = ahci_platform_suspend_host(&pdev->dev);
	if (err)
		return err;

	/* AHCI resources, such as PHY, clock, etc. should be disabled */
	ahci_platform_disable_resources(hpriv);

	return 0;
}

static int ahci_mvebu_resume(struct platform_device *pdev)
{
	struct ata_host *host = platform_get_drvdata(pdev);
	struct ahci_host_priv *hpriv = host->private_data;
	const struct mbus_dram_target_info *dram;
	int err;

	/* AHCI resources, such as PHY, clock, etc. should be enabled first */
	err = ahci_platform_enable_resources(hpriv);
	if (err)
		return err;

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-380-ahci")) {
		dram = mv_mbus_dram_info();
		if (dram)
			ahci_mvebu_mbus_config(hpriv, dram);

		ahci_mvebu_regret_option(hpriv);
	}

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-cp110-ahci"))
		ahci_mvebu_pll_power_up(pdev, hpriv, 1);

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-3700-ahci"))
		ahci_mvebu_pll_power_up(pdev, hpriv, 0);

	return ahci_platform_resume_host(&pdev->dev);
}
#else
#define ahci_mvebu_suspend NULL
#define ahci_mvebu_resume NULL
#endif

static inline void ahci_mvebu_led_on(struct ahci_mvebu_priv *priv)
{
	gpio_set_value(priv->led_gpio,
		       (priv->flags & OF_GPIO_ACTIVE_LOW) ? 0 : 1);
}

static inline void ahci_mvebu_led_off(struct ahci_mvebu_priv *priv)
{
	gpio_set_value(priv->led_gpio,
		       (priv->flags & OF_GPIO_ACTIVE_LOW) ? 1 : 0);
}

static ssize_t ahci_mvebu_transmit_led_message(struct ata_port *ap,
					       u32 state,
					       ssize_t size)
{
	struct ahci_host_priv *hpriv =  ap->host->private_data;
	struct ahci_mvebu_priv *priv = hpriv->plat_data;
	struct ahci_port_priv *pp = ap->private_data;
	int pmp;
	struct ahci_em_priv *emp;

	dev_dbg(priv->dev, "Tx led message: state 0x%x\n", state);

	/* get the slot number from the message */
	pmp = (state & EM_MSG_LED_PMP_SLOT) >> 8;
	if (pmp < EM_MAX_SLOTS)
		emp = &pp->em_priv[pmp];
	else
		return -EINVAL;

	if (!(hpriv->em_msg_type & EM_MSG_TYPE_LED))
		return size;

	if (state & EM_MSG_LED_VALUE_ON)
		ahci_mvebu_led_on(priv);
	else
		ahci_mvebu_led_off(priv);

	/* save off new led state for port/slot */
	emp->led_state = state;

	return size;
}

static int ahci_mvebu_init_em_led(struct platform_device *pdev,
			     struct ahci_host_priv *hpriv)
{
	struct ahci_mvebu_priv *priv = NULL;
	int rc;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	hpriv->plat_data = priv;
	priv->dev = &pdev->dev;
	priv->led_gpio = of_get_named_gpio_flags(pdev->dev.of_node,
						 "marvell,led-gpio",
						 0,
						 &priv->flags);

	if (gpio_is_valid(priv->led_gpio)) {
		rc = gpio_request(priv->led_gpio, "Marvell AHCI LED");
		if (rc)
			dev_err(&pdev->dev,
				"gpio_request %d failed: %d\n",
				priv->led_gpio, rc);
	} else if (priv->led_gpio == -EPROBE_DEFER) {
		rc = -EPROBE_DEFER;
	} else {
		dev_err(&pdev->dev, "Failed to get <marvell,led-gpio>!\n");
		rc = -ENODEV;
	}
	if (rc)
		return rc;

	/*
	 * Set enclosure management location to be 0 although mvebu AHCI
	 * overwrite transmit_led_message and will not use it.
	 */
	hpriv->em_loc = 0;
	/*
	 * Set enclosure management buffer size to be 4 bytes as standard
	 * EM LED message size although mvebu AHCI overwrite
	 * transmit_led_message and will not use it
	 */
	hpriv->em_buf_sz = 4;
	/* Set enclosure management message type to support EM LED */
	hpriv->em_msg_type = EM_MSG_TYPE_LED;

	return 0;
}

/*
 * This function is used to set LED blink policy and link flags by force for
 * all ATA links connected to the AHCI controller to support software
 * activity LED blinking since mvebu AHCI does not support Enclosure
 * Management registers actually;
 * ATA links are created with ATA host, so this function must be called
 * after ahci_platform_init_host() which creates ATA host, ports and links.
 */
static void ahci_mvebu_set_em_blink_policy(struct platform_device *pdev,
			      enum sw_activity blink_policy)
{
	/* Set LED blink policy for all connected ATA links */
	struct ata_host *host = platform_get_drvdata(pdev);
	struct ata_link *link;
	int i;

	for (i = 0; i < host->n_ports; i++) {
		struct ata_port *ap = host->ports[i];
		struct ahci_port_priv *pp = ap->private_data;

		ata_for_each_link(link, ap, EDGE) {
			struct ahci_em_priv *emp;

			link->flags |= ATA_LFLAG_SW_ACTIVITY;
			emp = &pp->em_priv[link->pmp];
			emp->blink_policy = blink_policy;
		}
	}
}

static void ahci_mvebu_postreset(struct ata_link *link, unsigned int *class)
{
	/*
	 * ahci_platform_ops does not define postreset() but inherit it
	 * from ahci_ops.
	 */
	ahci_ops.postreset(link, class);

	/* Turn off LED when the ata link is not on line */
	if (!ata_link_online(link)) {
		struct ata_port *ap = link->ap;
		struct ahci_host_priv *hpriv =  ap->host->private_data;
		struct ahci_mvebu_priv *priv = hpriv->plat_data;

		ahci_mvebu_led_off(priv);
	}
}

static const struct ata_port_info ahci_mvebu_port_info = {
	.flags	   = AHCI_FLAG_COMMON,
	.pio_mask  = ATA_PIO4,
	.udma_mask = ATA_UDMA6,
	.port_ops  = &ahci_platform_ops,
};

/*
 * The below ata port operations and port info are for Enclosure Management;
 * for more detailed, please refer to explanation above struct ahci_mvebu_priv.
 */
struct ata_port_operations ahci_mvebu_em_ops = {
	.inherits		= &ahci_platform_ops,
	.transmit_led_message	= ahci_mvebu_transmit_led_message,
	.postreset		= ahci_mvebu_postreset,
};

static const struct ata_port_info ahci_mvebu_em_port_info = {
	.flags	   = AHCI_FLAG_COMMON | ATA_FLAG_EM | ATA_FLAG_SW_ACTIVITY,
	.pio_mask  = ATA_PIO4,
	.udma_mask = ATA_UDMA6,
	.port_ops  = &ahci_mvebu_em_ops,
};

static struct scsi_host_template ahci_platform_sht = {
	AHCI_SHT(DRV_NAME),
};

static int ahci_mvebu_probe(struct platform_device *pdev)
{
	struct ahci_host_priv *hpriv;
	const struct mbus_dram_target_info *dram;
	enum sw_activity	blink_policy = OFF;
	const struct ata_port_info *port_info;
	int rc;

	hpriv = ahci_platform_get_resources(pdev);
	if (IS_ERR(hpriv))
		return PTR_ERR(hpriv);

	rc = ahci_platform_enable_resources(hpriv);
	if (rc)
		return rc;

	hpriv->stop_engine = ahci_mvebu_stop_engine;

	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-380-ahci")) {
		dram = mv_mbus_dram_info();
		if (!dram)
			return -ENODEV;

		ahci_mvebu_mbus_config(hpriv, dram);
		ahci_mvebu_regret_option(hpriv);
	}

	/* Call comphy initialization flow */
	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-cp110-ahci")) {
		rc = ahci_mvebu_pll_power_up(pdev, hpriv, 1);
		if (rc)
			return rc;
	}

	/* Call comphy initialization flow */
	if (of_device_is_compatible(pdev->dev.of_node,
				    "marvell,armada-3700-ahci")) {
		rc = ahci_mvebu_pll_power_up(pdev, hpriv, 0);
		if (rc)
			return rc;
	}

	if (of_property_read_u32(pdev->dev.of_node, "comwake",
				 &hpriv->comwake))
		hpriv->comwake = 0;

	if (of_property_read_u32(pdev->dev.of_node, "comreset_u",
				 &hpriv->comreset_u))
		hpriv->comreset_u = 0;

	of_property_read_u32(pdev->dev.of_node, "marvell,blink-policy",
			     &blink_policy);
	if (blink_policy != OFF) {
		rc = ahci_mvebu_init_em_led(pdev, hpriv);
		switch (rc) {
		case 0:
			port_info = &ahci_mvebu_em_port_info;
			break;
		case -EPROBE_DEFER:
			dev_dbg(&pdev->dev, "LED GPIO not ready, retry\n");
		case -ENOMEM:
			/* Fatal error */
			goto disable_resources;
		default:
			dev_info(&pdev->dev,
				 "AHCI LED initialization failed!\n");
			dev_info(&pdev->dev,
				 "Turn back to work in no LED mode!\n");
			port_info = &ahci_mvebu_port_info;
		}
	} else {
		port_info = &ahci_mvebu_port_info;
	}

	rc = ahci_platform_init_host(pdev, hpriv, port_info,
				     &ahci_platform_sht);
	if (rc)
		goto disable_resources;

	if (blink_policy != OFF) {
		/*
		 * After ATA host and its ATA ports and links are created, set
		 * LED blink policy by force for its all connected ATA links to
		 * to support software activity LED blinking.
		 */
		ahci_mvebu_set_em_blink_policy(pdev, blink_policy);
	}

	return 0;

disable_resources:
	ahci_platform_disable_resources(hpriv);
	return rc;
}

static const struct of_device_id ahci_mvebu_of_match[] = {
	{ .compatible = "marvell,armada-380-ahci", },
	{ .compatible = "marvell,armada-3700-ahci", },
	{ .compatible = "marvell,armada-cp110-ahci", },
	{ },
};
MODULE_DEVICE_TABLE(of, ahci_mvebu_of_match);

/*
 * We currently don't provide power management related operations,
 * since there is no suspend/resume support at the platform level for
 * Armada 38x for the moment.
 */
static struct platform_driver ahci_mvebu_driver = {
	.probe = ahci_mvebu_probe,
	.remove = ata_platform_remove_one,
	.suspend = ahci_mvebu_suspend,
	.resume = ahci_mvebu_resume,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = ahci_mvebu_of_match,
	},
};
module_platform_driver(ahci_mvebu_driver);

MODULE_DESCRIPTION("Marvell EBU AHCI SATA driver");
MODULE_AUTHOR("Thomas Petazzoni <thomas.petazzoni@free-electrons.com>, Marcin Wojtas <mw@semihalf.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ahci_mvebu");
