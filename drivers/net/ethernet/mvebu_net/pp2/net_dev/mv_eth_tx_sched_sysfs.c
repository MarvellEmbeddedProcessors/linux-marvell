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

static ssize_t mv_eth_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "echo [p] [txp]               > txSchedRegs   - show TXP Scheduler registers for egress port <p/txp>\n");
	off += sprintf(buf+off, "echo [p] [txp] [v]           > txpRate       - set outgoing rate <v> in [kbps] for <port/txp>\n");
	off += sprintf(buf+off, "echo [p] [txp] [v]           > txpBurst      - set maximum burst <v> in [Bytes] for <port/txp>\n");
	off += sprintf(buf+off, "echo [p] [txp] [txq] [v]     > txqRate       - set outgoing rate <v> in [kbps] for <port/txp/txq>\n");
	off += sprintf(buf+off, "echo [p] [txp] [txq] [v]     > txqBurst      - set maximum burst <v> in [Bytes] for <port/txp/txq>\n");
	off += sprintf(buf+off, "echo [p] [txp] [txq] [v]     > txqWrr        - set outgoing WRR weight for <port/txp/txq>. <v=0> - fixed\n");

	return off;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_eth_help(buf);

	return off;
}

static ssize_t mv_eth_port_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, v, a, b;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = a = b = 0;
	sscanf(buf, "%d %d %d %d", &p, &v, &a, &b);

	local_irq_save(flags);

	if (!strcmp(name, "txSchedRegs")) {
		mvPp2TxSchedRegs(p, v);
	} else if (!strcmp(name, "txpRate")) {
		err = mvPp2TxpRateSet(p, v, a);
	} else if (!strcmp(name, "txpBurst")) {
		err = mvPp2TxpBurstSet(p, v, a);
	} else if (!strcmp(name, "txqRate")) {
		err = mvPp2TxqRateSet(p, v, a, b);
	} else if (!strcmp(name, "txqBurst")) {
		err = mvPp2TxqBurstSet(p, v, a, b);
	} else if (!strcmp(name, "txqWrr")) {
		if (b == 0)
			err = mvPp2TxqFixPrioSet(p, v, a);
		else
			err = mvPp2TxqWrrPrioSet(p, v, a, b);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,         S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(txSchedRegs,  S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txpRate,      S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txpBurst,     S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txqRate,      S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txqBurst,     S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(txqWrr,       S_IWUSR, NULL, mv_eth_port_store);

static struct attribute *mv_eth_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_txSchedRegs.attr,
	&dev_attr_txpRate.attr,
	&dev_attr_txpBurst.attr,
	&dev_attr_txqRate.attr,
	&dev_attr_txqBurst.attr,
	&dev_attr_txqWrr.attr,
	NULL
};

static struct attribute_group mv_eth_tx_sched_group = {
	.name = "tx_sched",
	.attrs = mv_eth_attrs,
};

int mv_pp2_tx_sched_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_tx_sched_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_tx_sched_group.name, err);

	return err;
}

int mv_pp2_tx_sched_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_tx_sched_group);
	return 0;
}
