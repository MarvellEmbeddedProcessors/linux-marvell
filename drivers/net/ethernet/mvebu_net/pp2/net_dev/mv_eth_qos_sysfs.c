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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_eth_sysfs.h"
#include "gbe/mvPp2Gbe.h"
#include "mv_netdev.h"

static ssize_t mv_pp2_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "echo [p]                     > dscp          - show RX and TX DSCP map for port <p>\n");
	off += sprintf(buf+off, "echo [p] [txq] [dscp]        > txqDscp       - set <txq> for outgoing IP packets with <dscp>\n");

	return off;
}

static ssize_t mv_pp2_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_pp2_help(buf);

	return off;
}

static ssize_t mv_pp2_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = 0;
	sscanf(buf, "%d", &p);

	local_irq_save(flags);

	if (!strcmp(name, "dscp")) {
		mv_pp2_dscp_map_show(p);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_pp2_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, i, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %x", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txqDscp")) {
		err = mv_pp2_txq_dscp_map_set(p, i, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,         S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(dscp,         S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(txqDscp,      S_IWUSR, NULL, mv_pp2_3_hex_store);


static struct attribute *mv_pp2_attrs[] = {
	&dev_attr_dscp.attr,
	&dev_attr_txqDscp.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group mv_pp2_qos_group = {
	.name = "qos",
	.attrs = mv_pp2_attrs,
};

int mv_pp2_qos_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_pp2_qos_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_pp2_qos_group.name, err);

	return err;
}

int mv_pp2_qos_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_pp2_qos_group);
	return 0;
}
