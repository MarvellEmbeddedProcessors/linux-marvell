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

#include "mv_netdev.h"

static ssize_t pp3_dev_help(char *b)
{
	int o = 0;
	o += sprintf(b+o, "echo [netif]                > ifStatus         - print net interface status\n");
	o += sprintf(b+o, "echo [netif] [cpu]          > groupStatus      - print group status\n");
	o += sprintf(b+o, "echo [pool]                 > poolStatus       - print BM pool status\n");
	o += sprintf(b+o, "echo [cpu]                  > cpuStatus        - print cpu status\n");
	o += sprintf(b+o, "echo [netif] [cpu] [q]      > rxqStatus        - print rxq status\n");
	o += sprintf(b+o, "echo [netif] [cpu] [q]      > txqStatus        - print rxq status\n");

	o += sprintf(b+o, "\nAll inputs in decimal\n");

	return o;
}

static ssize_t pp3_dev_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	/*const char	*name = attr->attr.name;*/
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = pp3_dev_help(buf);

	return off;
}

static ssize_t pp3_dev_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    a, b, c;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = a = b = c = 0;
	sscanf(buf, "%d %d %d", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "ifStatus")) {
		pp3_netdev_dev_status_print(a);
	} else if (!strcmp(name, "groupStatus")) {
		pp3_netdev_group_status_print(a, b);
	} else if (!strcmp(name, "poolStatus")) {
		pp3_netdev_pool_status_print(a);
	} else if (!strcmp(name, "cpuStatus")) {
		pp3_netdev_cpu_status_print(a);
	} else if (!strcmp(name, "rxqStatus")) {
		pp3_netdev_rxq_status_print(a, b, c);
	} else if (!strcmp(name, "txqStatus")) {
		pp3_netdev_txq_status_print(a, b, c);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(ifStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(groupStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(poolStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(cpuStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(rxqStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(txqStatus,		S_IWUSR, NULL, pp3_dev_store);
static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_show, NULL);

static struct attribute *pp3_dev_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_ifStatus.attr,
	&dev_attr_groupStatus.attr,
	&dev_attr_poolStatus.attr,
	&dev_attr_cpuStatus.attr,
	&dev_attr_rxqStatus.attr,
	&dev_attr_txqStatus.attr,
	NULL
};

static struct attribute_group pp3_dev_group = {
	.name = "dev",
	.attrs = pp3_dev_attrs,
};

int pp3_dev_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &pp3_dev_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", pp3_dev_group.name, err);
		return err;
	}

	return err;
}

int pp3_dev_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &pp3_dev_group);

	return 0;
}
