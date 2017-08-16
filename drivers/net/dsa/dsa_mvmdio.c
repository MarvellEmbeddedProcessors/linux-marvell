/*
 * ***************************************************************************
 * Copyright (C) 2016 Marvell International Ltd.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/of_mdio.h>

#define MV_PHY_CMD_REG 0
#define MV_PHY_DATA_REG 1
#define MV_SMIBUSY_OFFSET    15
#define MV_SMIFUNC_OFFSET    13
#define MV_SMIFUNC_INT    0
#define MV_SMIFUNC_EXT    1
#define MV_SMIFUNC_SIZE   2
#define MV_SMIMODE_OFFSET    12
#define MV_SMIOP_OFFSET      10
#define MV_SMIOP_SIZE     2
#define MV_SMIOP_READ     2
#define MV_SMIOP_WRITE    1
#define MV_DEVAD_OFFSET      5
#define MV_DEVAD_SIZE     5
#define MV_DEVAD_MASK     0x1F
#define MV_REGAD_OFFSET      0
#define MV_REGAD_SIZE     5
#define MV_REGAD_MASK     0x1F
#define MV_MAX_REGS_PER_PORT 0x20
#define MV_SMI_PHY_CMD    0x18
#define MV_SMI_PHY_DATA   0x19
#define MV_GLOBAL1    0x1B
#define MV_GLOBAL2    0x1C
#define MV_MAX_CHARS 1024
#define MV_INVALID_PHY_ADDR 0xFF

static struct mii_bus *mv_mii_bus;
static struct mii_bus *mv_xmii_bus;
static unsigned int mv_phy_addr;

enum mv_reg_type {
	MV_REG_TYPE_SWITCH,
	MV_REG_TYPE_PHY_INT,
	MV_REG_TYPE_PHY_EXT,
	MV_REG_TYPE_MDIO,
	MV_REG_TYPE_XMDIO
};

/* Read regular phy register that connected on mdio bus.
 * Returns: register value on success or error value on failure.
 */
static int dsa_mvmdio_read_mdio(unsigned char phy, unsigned char reg)
{
	return mv_mii_bus->read(mv_mii_bus, phy, reg);
}

/* Write regular phy register that is connected on mdio bus.
 * Returns: 0 on success or an error value on failure
 */
static int dsa_mvmdio_write_mdio(unsigned char phy, unsigned char reg, unsigned short val)
{
	return mv_mii_bus->write(mv_mii_bus, phy, reg, val);
}

/* Read extended phy register that connected on xmdio bus.
 * Returns: register value on success or error value on failure.
 */
static int dsa_mvmdio_read_xmdio(unsigned char phy, unsigned char dev, unsigned char reg)
{
	return mv_xmii_bus->read(mv_xmii_bus, phy, (dev << 16) | reg);
}

/* Write extended phy register that is connected on xmdio bus.
 * Returns: 0 on success or an error value on failure
 */
static int dsa_mvmdio_write_xmdio(unsigned char phy, unsigned char dev, unsigned char reg, unsigned short val)
{
	return mv_xmii_bus->write(mv_xmii_bus, phy, (dev << 16) | reg, val);
}

/* Read switch register that is connected on mdio bus.
 * Uses direct access if the switch is configured in siglechip addressing mode.
 * Otherwise in multichip addressing mode it uses indirect acces through command and data registers of switch.
 * Returns: register value on success or error value on failure.
 */
static int dsa_mvmdio_read_register(unsigned char dev, unsigned char reg)
{
	int ret;
	unsigned short cmd_data;

	if (mv_phy_addr == 0)
		return mv_mii_bus->read(mv_mii_bus, dev, reg);

	/* Write to SMI Command Register */
	cmd_data  = (1 << MV_SMIBUSY_OFFSET) | (MV_SMIFUNC_INT << MV_SMIFUNC_OFFSET) |
		(1 << MV_SMIMODE_OFFSET) | (MV_SMIOP_READ << MV_SMIOP_OFFSET) |
		((dev & MV_DEVAD_MASK) << MV_DEVAD_OFFSET) | ((reg & MV_REGAD_MASK) << MV_REGAD_OFFSET);

	ret = mv_mii_bus->write(mv_mii_bus, mv_phy_addr, MV_PHY_CMD_REG, cmd_data);
	if (ret < 0)
		return ret;

	/* Read from SMI Data Register */
	ret = mv_mii_bus->read(mv_mii_bus, mv_phy_addr, MV_PHY_DATA_REG);

	return ret;
}

/* Write switch register that is connected on mdio bus.
 * Uses direct access if the switch is configured in siglechip addressing mode.
 * Otherwise in multichip addressing mode it uses indirect acces through command and data registers of switch.
 * Returns: 0 on success or an error value on failure.
 */
static int dsa_mvmdio_write_register(unsigned char dev, unsigned char reg, unsigned short data)
{
	int ret;
	unsigned short cmd_data;

	if (mv_phy_addr == 0)
		return mv_mii_bus->write(mv_mii_bus, dev, reg, data);

	/* Write data to SMI Data Register */
	ret = mv_mii_bus->write(mv_mii_bus, mv_phy_addr, MV_PHY_DATA_REG, data);
	if (ret < 0)
		return ret;

	/* Write to SMI Command Register */
	cmd_data  = (1 << MV_SMIBUSY_OFFSET) | (MV_SMIFUNC_INT << MV_SMIFUNC_OFFSET) |
		(1 << MV_SMIMODE_OFFSET) | (MV_SMIOP_WRITE << MV_SMIOP_OFFSET) |
		((dev & MV_DEVAD_MASK) << MV_DEVAD_OFFSET) | ((reg & MV_REGAD_MASK) << MV_REGAD_OFFSET);

	ret = mv_mii_bus->write(mv_mii_bus, mv_phy_addr, MV_PHY_CMD_REG, cmd_data);

	return ret;
}

/* Read switch internal phy register when smi_func = 0.
 * Read external phy register that is connected to the switch port when smi_func = 1.
 * Returns: register value on success or error value on failure.
 */
static int dsa_mvmdio_phy_read_register(unsigned char dev, unsigned char reg, unsigned char smi_func)
{
	int ret;
	unsigned short cmd_data;

	/* Write to SMI Command Register */
	cmd_data  = (1 << MV_SMIBUSY_OFFSET) | (smi_func << MV_SMIFUNC_OFFSET) |
		(1 << MV_SMIMODE_OFFSET) | (MV_SMIOP_READ << MV_SMIOP_OFFSET) |
		((dev & MV_DEVAD_MASK) << MV_DEVAD_OFFSET) | ((reg & MV_REGAD_MASK) << MV_REGAD_OFFSET);

	ret = dsa_mvmdio_write_register(MV_GLOBAL2, MV_SMI_PHY_CMD, cmd_data);
	if (ret < 0)
		return ret;

	/* Read from SMI Data Register */
	ret = dsa_mvmdio_read_register(MV_GLOBAL2, MV_SMI_PHY_DATA);
	return ret;
}

/* Write switch internal phy register when smi_func = 0.
 * Write external phy register that is connected to the switch port when smi_func = 1.
 * Returns: 0 on success or an error value on failure.
 */
static int dsa_mvmdio_phy_write_register(unsigned char dev, unsigned char reg,
					 unsigned short data, unsigned char smi_func)
{
	int ret;
	unsigned short cmd_data;

	/* Write data to SMI Data Register */
	ret = dsa_mvmdio_write_register(MV_GLOBAL2, MV_SMI_PHY_DATA, data);
	if (ret < 0)
		return ret;

	/* Write to SMI Command Register */
	cmd_data  = (1 << MV_SMIBUSY_OFFSET) | (smi_func << MV_SMIFUNC_OFFSET) |
		(1 << MV_SMIMODE_OFFSET) | (MV_SMIOP_WRITE << MV_SMIOP_OFFSET) |
		((dev & MV_DEVAD_MASK) << MV_DEVAD_OFFSET) | ((reg & MV_REGAD_MASK) << MV_REGAD_OFFSET);

	ret = dsa_mvmdio_write_register(MV_GLOBAL2, MV_SMI_PHY_CMD, cmd_data);

	return ret;
}

/* Processing "read" command in sysfs.
 * Returns: register value on success or error value on failure for a given register type.
 */
static int dsa_mvmdio_read(unsigned char port, unsigned char dev_addr, unsigned char reg,
			   unsigned char type, unsigned int *value)
{
	int ret = 0;

	if (type == MV_REG_TYPE_SWITCH)
		ret = dsa_mvmdio_read_register(port, reg);
	else if (type == MV_REG_TYPE_PHY_INT)
		ret = dsa_mvmdio_phy_read_register(port, reg, MV_SMIFUNC_INT);
	else if (type == MV_REG_TYPE_PHY_EXT)
		ret = dsa_mvmdio_phy_read_register(port, reg, MV_SMIFUNC_EXT);
	else if (type == MV_REG_TYPE_MDIO)
		ret = dsa_mvmdio_read_mdio(port, reg);
	else if (type == MV_REG_TYPE_XMDIO)
		ret = dsa_mvmdio_read_xmdio(port, dev_addr, reg);

	if (ret < 0)
		return ret;

	*value = ret;

	return 0;
}

/* Processing "write" command in sysfsi.
 * Returns: 0 on success or error value on failure.
*/
static int dsa_mvmdio_write(unsigned char port, unsigned char dev_addr, unsigned char reg,
			    unsigned char type, unsigned short value)
{
	int ret = 0;

	if (type == MV_REG_TYPE_SWITCH)
		ret = dsa_mvmdio_write_register(port, reg, value);
	else if (type == MV_REG_TYPE_PHY_INT)
		ret = dsa_mvmdio_phy_write_register(port, reg, value, MV_SMIFUNC_INT);
	else if (type == MV_REG_TYPE_PHY_EXT)
		ret = dsa_mvmdio_phy_write_register(port, reg, value, MV_SMIFUNC_EXT);
	else if (type == MV_REG_TYPE_MDIO)
		ret = dsa_mvmdio_write_mdio(port, reg, value);
	else if (type == MV_REG_TYPE_XMDIO)
		ret = dsa_mvmdio_write_xmdio(port, dev_addr, reg, value);

	if (ret < 0)
		return ret;

	return 0;
}

/* Processing "dump" command in sysfs.
 * Print 0 to 32 registers values of a given register type.
 * Returns: 0 on success or error value on failure.
 */
static int dsa_mvmdio_dump(unsigned char port, unsigned char dev_addr, unsigned char type)
{
	int i;
	int max_regs = MV_MAX_REGS_PER_PORT;
	int off = 0;
	char buf[MV_MAX_CHARS];

	for (i = 0; i < max_regs; i++) {
		if (i % 4 == 0)
			off += sprintf(buf + off, "(%02X-%02X)  ", i, i + 3);
		if (type == MV_REG_TYPE_SWITCH)
			off += sprintf(buf + off, "%04X ", dsa_mvmdio_read_register(port, i));
		else if (type == MV_REG_TYPE_PHY_INT)
			off += sprintf(buf + off, "%04X ", dsa_mvmdio_phy_read_register(port, i, MV_SMIFUNC_INT));
		else if (type == MV_REG_TYPE_PHY_EXT)
			off += sprintf(buf + off, "%04X ", dsa_mvmdio_phy_read_register(port, i, MV_SMIFUNC_EXT));
		else if (type == MV_REG_TYPE_MDIO)
			off += sprintf(buf + off, "%04X ", dsa_mvmdio_read_mdio(port, i));
		else if (type == MV_REG_TYPE_XMDIO)
			off += sprintf(buf + off, "%04X ", dsa_mvmdio_read_xmdio(port, dev_addr, i));
		if (i % 4 == 3)
			off += sprintf(buf + off, "\n");
	}

	pr_err("%s", buf);
	return 0;
}

/* Processing "help" command in sysfs */
static ssize_t dsa_mvmdio_help(char *buf)
{
	int off = 0;

	off += scnprintf(buf + off, PAGE_SIZE - off, "cat help                         - print help\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo [t] [p] [x] [r]     > read  - read register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo [t] [p] [x] [r] [v] > write - write register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo [t] [p] [x]         > dump  - dump 32 registers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "parameters (in hexadecimal):\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "    [t] type. 0-switch, 1-internal phy, 2-external phy regs\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "              3-regular phy, 4-extended phy\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "    [p] port addr or phy-id.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "    [x] device address. valid only for extended phy.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "    [r] register address.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "    [v] value.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "Examples:\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  1. echo 0  1 0  3   > read  - read switch register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  2. echo 0 1b 0 1c   > read  - read switch global1 register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  3. echo 1  3 0  2   > read  - read internal phy register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  4. echo 2  0 0  2   > read  - read external phy register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  5. echo 3  1 0  2   > read  - read regular phy register, phyid=1\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  6. echo 4  0 7 3c   > read  - read xmdio phy, EEE advertisement register\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  7. echo 0  2 0  7 5 > write - write switch register, set vlan id\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  8. echo 1  3 0      > dump  - dump internal switch phy registers\n");
	off += scnprintf(buf + off, PAGE_SIZE - off,
			 "  9. echo 4  0 7      > dump  - dump xmdio phy registers for dev-addr=7");
	return off;
}

static ssize_t dsa_mvmdio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int off = 0;
	ssize_t len = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	len = dsa_mvmdio_help(buf);
	pr_err("%s\n", buf);

	return off;
}

static ssize_t dsa_mvmdio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	unsigned long   flags;
	unsigned int    err = 0, dev_addr = 0, port = 0, reg = 0, type = 0;
	unsigned int    val;
	unsigned int  data = 0;
	int ret;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read arguments */
	ret = sscanf(buf, "%x %x %x %x %x", &type, &port, &dev_addr, &reg, &val);

	if ((mv_phy_addr == MV_INVALID_PHY_ADDR) && (type < MV_REG_TYPE_MDIO)) {
		pr_err("\"sw-smi-addr\" property not defined in dts file. Assuming switch not connected\n");
		return len;
	}

	local_irq_save(flags);
	if (!strcmp(name, "read")) {
		err = dsa_mvmdio_read((unsigned char)port, (unsigned char)dev_addr, (unsigned char)reg,
				      (unsigned char)type, &data);
		if (err)
			pr_err("Register read failed, err - %d\n", err);
		else
			pr_err("read:: type:%d, port=0x%X, dev=0x%X, reg=0x%X, val=0x%04X\n",
			       type, port, dev_addr, reg, data);
	} else if (!strcmp(name, "write")) {
		err = dsa_mvmdio_write((unsigned char)port, (unsigned char)dev_addr, (unsigned char)reg,
				       (unsigned char)type, (unsigned short)val);
		if (err)
			pr_err("Register write failed, err - %d\n", err);
		else
			pr_err("write:: type:%d, port=0x%X, dev=0x%X, reg=0x%X, val=0x%X\n",
			       type, port, dev_addr, reg, val);
	} else if (!strcmp(name, "dump")) {
		err = dsa_mvmdio_dump((unsigned char)port, (unsigned char)dev_addr, (unsigned char)type);
		if (err)
			pr_err("Register dump failed, err - %d\n", err);
		else
			pr_err("dump:: type %d, port=0x%X, dev=0x%X\n", type, port, dev_addr);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(read, S_IWUSR, dsa_mvmdio_show, dsa_mvmdio_store);
static DEVICE_ATTR(write, S_IWUSR, dsa_mvmdio_show, dsa_mvmdio_store);
static DEVICE_ATTR(dump, S_IWUSR, dsa_mvmdio_show, dsa_mvmdio_store);
static DEVICE_ATTR(help, S_IRUSR, dsa_mvmdio_show, dsa_mvmdio_store);

static struct attribute *dsa_mvmdio_attrs[] = {
	&dev_attr_read.attr,
	&dev_attr_write.attr,
	&dev_attr_dump.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group dsa_mvmdio_group = {
	.name = "dsa_mvmdio",
	.attrs = dsa_mvmdio_attrs,
};

static int dsa_mvmdio_probe(struct platform_device *pdev)
{
	struct device_node *np;
	struct device_node *mdio;
	struct device_node *xmdio;
	int ret;

	np = pdev->dev.of_node;
	mdio = of_parse_phandle(np, "mii-bus", 0);
	if (!mdio) {
		pr_err("%s : parse mii-bus handle failed\n", __func__);
		return -EINVAL;
	}

	mv_mii_bus = of_mdio_find_bus(mdio);
	if (!mv_mii_bus) {
		pr_err("%s : mdio find bus failed\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "reg", &mv_phy_addr);
	if (ret) {
		pr_err("%s : switch smi addr not defined\n", __func__);
		mv_phy_addr = MV_INVALID_PHY_ADDR;
	}

	xmdio = of_parse_phandle(np, "xmii-bus", 0);
	if (!xmdio) {
		pr_err("%s : parse handle failed\n", __func__);
		return -EINVAL;
	}

	mv_xmii_bus = of_mdio_find_bus(xmdio);
	if (!mv_xmii_bus) {
		pr_err("%s : xmdio find bus failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int dsa_mvmdio_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id dsa_mvmdio_match[] = {
	{ .compatible = "marvell,dsa-mvmdio" },
	{ }
};

MODULE_DEVICE_TABLE(of, dsa_mvmdio_match);

static struct platform_driver dsa_mvmdio_driver = {
	.probe	= dsa_mvmdio_probe,
	.remove = dsa_mvmdio_remove,
	.driver = {
		.name = "dsa-mvmdio",
		.of_match_table = dsa_mvmdio_match,
	},
};

static int dsa_mvmdio_init(void)
{
	int err;
	struct device *pd;

	err = platform_driver_register(&dsa_mvmdio_driver);
	if (err) {
		pr_err("register dsa_mvmdio_driver() failed\n");
		return err;
	}

	pd = &platform_bus;
	err = sysfs_create_group(&pd->kobj, &dsa_mvmdio_group);
	if (err)
		pr_err("init sysfs group %s failed %d\n", dsa_mvmdio_group.name, err);

	return err;
}

static void dsa_mvmdio_exit(void)
{
	platform_driver_unregister(&dsa_mvmdio_driver);
}

late_initcall(dsa_mvmdio_init);
module_exit(dsa_mvmdio_exit);

MODULE_AUTHOR("Ravindra Reddy K. <ravindra@marvell.com>");
MODULE_DESCRIPTION("Mdio access for switch from userspace through sysfs");
MODULE_LICENSE("GPL v2");
