/**
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_mdio.h>
#include <linux/of_address.h>

#define TIMEOUT_MIN		150	/* = 150us */
#define TIMEOUT_MAX		160	/* = 160us */
#define NTRIES			5
#define XBUSY			BIT(30)
#define XREAD_VALID		BIT(29)
#define XCLKDIV_OFFS		0
#define XCLKDIV_MASK		(0x3 << XCLKDIV_OFFS)
#define XOPCODE_OFFS		26
#define XOPCODE_ADDR_READ	(7 << XOPCODE_OFFS)
#define XOPCODE_ADDR_WRITE	(5 << XOPCODE_OFFS)
#define XPHYADDR_OFFS		16 /* Phy port addr offset */
#define XPORT_ADDR(x)		((x & 0x1f) << XPHYADDR_OFFS)
#define XDEVADDR_OFFS		21	/* Phy device addr offset */
#define XDEV_ADDR(x)		((x & 0x1f) << XDEVADDR_OFFS)
#define XSMI_ADDR_REG_OFFS	0x8

struct xmdio_controller {
	void __iomem *xmdio_mngmnt;	/* XSMI Management Register */
	void __iomem *xmdio_addr;	/* XSMI Address Register */
};

/* Check if XSMI bus is idle */
static int xmdio_check_idle(struct xmdio_controller __iomem *regs)
{
	return !(readl(regs->xmdio_mngmnt) & XBUSY);
}

/* Wait until XSMI bus is free */
static int xmdio_wait_free(struct device *dev, struct xmdio_controller __iomem *regs)
{
	int ntries = NTRIES;

	while (ntries > 0) {
		if (xmdio_check_idle(regs))
			return 0;

		usleep_range(TIMEOUT_MIN, TIMEOUT_MAX);
		ntries--;
	}

	dev_err(dev, "timeout while waiting for bus to be free\n");
	return -ETIMEDOUT;
}

/* Check if XSMI bus read operaton is done */
static int xmdio_check_read_done(struct xmdio_controller __iomem *regs)
{
	return readl(regs->xmdio_mngmnt) & XREAD_VALID;
}

/* Wait until XSMI bus read operation is done */
static int xmdio_wait_read_done(struct device *dev, struct xmdio_controller __iomem *regs)
{
	int ntries = NTRIES;

	while (ntries > 0) {
		if (xmdio_check_read_done(regs))
			return 0;

		usleep_range(TIMEOUT_MIN, TIMEOUT_MAX);
		ntries--;
	}

	dev_err(dev, "timeout while waiting for bus read operation is done\n");
	return -ETIMEDOUT;
}

static int xmdio_read(struct mii_bus *bus, int phy_id, int regnum)
{
	struct xmdio_controller *regs = bus->priv;
	u32 reg_val;
	u16 dev_addr = regnum >> 16;
	u16 data;
	int ret;

	ret = xmdio_wait_free(&bus->dev, regs);
	if (ret)
		return ret;

	/* Write phy reg addr */
	writel(regnum & 0xffff, regs->xmdio_addr);

	/* Set phy port and device addrs, and read opcode */
	reg_val = XPORT_ADDR(phy_id) | XDEV_ADDR(dev_addr) | XOPCODE_ADDR_READ;

	/* Initiate the read operation */
	writel(reg_val, regs->xmdio_mngmnt);

	ret = xmdio_wait_read_done(&bus->dev, regs);
	if (ret)
		return ret;

	data = readl(regs->xmdio_mngmnt) & 0xffff;

	return data;
}

static int xmdio_write(struct mii_bus *bus, int phy_id, int regnum, u16 value)
{
	struct xmdio_controller __iomem *regs = bus->priv;
	u32 reg_val;
	u16 dev_addr = regnum >> 16;
	int ret;

	ret = xmdio_wait_free(&bus->dev, regs);
	if (ret)
		return ret;

	/* Write phy reg addr */
	writel(regnum & 0xffff, regs->xmdio_addr);

	/* Set phy port and device addrs, write opcode, and value */
	reg_val = XPORT_ADDR(phy_id) | XDEV_ADDR(dev_addr) | XOPCODE_ADDR_WRITE | value;

	/* Initiate the write operation */
	writel(reg_val, regs->xmdio_mngmnt);

	ret = xmdio_wait_free(&bus->dev, regs);
	if (ret)
		return ret;

	return 0;
}

static int xmdio_reset(struct mii_bus *bus)
{
	return 0;
}

static int xmdio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mii_bus *bus;
	struct resource *res;
	int ret;
	struct xmdio_controller *regs;

	regs = devm_kzalloc(&pdev->dev, sizeof(struct xmdio_controller), GFP_KERNEL);

	if (!regs)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	regs->xmdio_mngmnt = devm_ioremap(&pdev->dev, res->start, resource_size(res));

	if (IS_ERR(regs->xmdio_mngmnt))
		return PTR_ERR(regs->xmdio_mngmnt);

	regs->xmdio_addr = regs->xmdio_mngmnt + XSMI_ADDR_REG_OFFS;

	bus = mdiobus_alloc_size(PHY_MAX_ADDR * sizeof(int));
	if (!bus)
		return -ENOMEM;

	bus->name = "Marvell XSMI MDIO Bus";
	bus->read = xmdio_read;
	bus->write = xmdio_write;
	bus->reset = xmdio_reset;
	bus->parent = &pdev->dev;
	snprintf(bus->id, MII_BUS_ID_SIZE, "%llx", (unsigned long long)res->start);

	/* Set the PHY base address */
	bus->priv = regs;

	if (!bus->priv) {
		ret = -ENOMEM;
		goto err_ioremap;
	}

	ret = of_mdiobus_register(bus, np);
	if (ret) {
		dev_err(&pdev->dev, "cannot register MDIO bus\n");
		goto err_registration;
	}

	platform_set_drvdata(pdev, bus);

	return 0;

err_registration:
	iounmap(bus->priv);

err_ioremap:
	mdiobus_free(bus);

	return ret;
}

static int xmdio_remove(struct platform_device *pdev)
{
	struct mii_bus *bus = platform_get_drvdata(pdev);

	devm_kfree(&pdev->dev, bus->priv);

	mdiobus_unregister(bus);
	iounmap(bus->priv);
	mdiobus_free(bus);

	return 0;
}

static const struct of_device_id xmdio_match[] = {
	{ .compatible = "marvell,xmdio", },
	{},
};
MODULE_DEVICE_TABLE(of, xmdio_match);

static struct platform_driver xmdio_driver = {
	.driver = {
		.name = "marvell-xmdio",
		.of_match_table = xmdio_match,
	},
	.probe = xmdio_probe,
	.remove = xmdio_remove,
};

module_platform_driver(xmdio_driver);

MODULE_DESCRIPTION("Marvell XSMI MDIO interface driver");
MODULE_AUTHOR("Victor Axelrod <victora@marvell.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:marvell-xmdio");
