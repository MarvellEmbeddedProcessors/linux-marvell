/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.


********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/
#include "mvCommon.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/mbus.h>
#include <linux/inetdevice.h>
#include <linux/interrupt.h>

#include "mvTypes.h"

#include "mvEthPhy.h"

static ssize_t phy_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "echo a       > status        - print phy status <a-phy address>.\n");
	off += sprintf(buf+off, "echo a r     > read_reg      - read phy <a-phy address> register <r-hex>\n");
	off += sprintf(buf+off, "echo a r v   > write_reg     - write value <v-hex> to  phy (a-phy address) register <r-hex>\n");
	off += sprintf(buf+off, "echo a       > restart_an    - restart phy <a-phy address> Auto-Negotiation.\n");

	return off;
}

static ssize_t phy_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int          err = 0;
	const char   *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return phy_help(buf);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	return err;
}

static ssize_t phy_store_hex(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, p = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x", &p);


	if (!strcmp(name, "status")) {
		err = mvEthPhyPrintStatus(p);
	} else if (!strcmp(name, "restart_an")) {
		err = mvEthPhyRestartAN(p, 0 /* time out */);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return err ? -EINVAL : len;
}


static ssize_t phy_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, reg, val;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = reg = val = 0;
	sscanf(buf, "%x %x %x", &p, &reg, &val);

	local_irq_save(flags);

	if (!strcmp(name, "read_reg")) {
		err = mvEthPhyRegPrint(p, reg);
	} else if (!strcmp(name, "write_reg")) {
		err = mvEthPhyRegWrite(p, reg, val);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}



static DEVICE_ATTR(status, S_IWUSR, NULL, phy_store_hex);
static DEVICE_ATTR(restart_an, S_IWUSR, NULL, phy_store_hex);
static DEVICE_ATTR(read_reg, S_IWUSR, NULL, phy_3_hex_store);
static DEVICE_ATTR(write_reg, S_IWUSR, NULL, phy_3_hex_store);
static DEVICE_ATTR(help,   S_IRUSR, phy_show, NULL);


static struct attribute *phy_attrs[] = {
	&dev_attr_status.attr,
	&dev_attr_read_reg.attr,
	&dev_attr_write_reg.attr,
	&dev_attr_restart_an.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group phy_group = {
	.name = "mv_phy",
	.attrs = phy_attrs,
};

int __init phy_sysfs_init(void)
{
		int err;
		struct device *pd;

		pd = &platform_bus;

		err = sysfs_create_group(&pd->kobj, &phy_group);
		if (err) {
			printk(KERN_INFO "sysfs group failed %d\n", err);
			goto out;
		}
out:
		return err;
}

module_init(phy_sysfs_init);

MODULE_AUTHOR("Uri Eliyahu");
MODULE_DESCRIPTION("Phy sysfs commands");
MODULE_LICENSE("GPL");
