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
#include "mv_netdev.h"

static ssize_t mv_eth_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "p, txp, txq, d                        - are dec numbers\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "echo p txp         > wrr_regs      - show WRR registers for <p/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp {0|1}   > ejp           - enable/disable EJP mode for <port/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp d       > txp_rate      - set outgoing rate <d> in [kbps] for <port/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp d       > txp_burst     - set maximum burst <d> in [Bytes] for <port/txp>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_rate      - set outgoing rate <d> in [kbps] for <port/txp/txq>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_burst     - set maximum burst <d> in [Bytes] for <port/txp/txq>\n");
	o += scnprintf(b+o, s-o, "echo p txp txq d   > txq_wrr       - set outgoing WRR weight for <port/txp/txq>. <d=0> - fixed\n");

	return o;
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
static ssize_t mv_eth_3_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, i, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %d", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "txp_rate")) {
		err = mvNetaTxpRateSet(p, i, v);
	} else if (!strcmp(name, "txp_burst")) {
		err = mvNetaTxpBurstSet(p, i, v);
	} else if (!strcmp(name, "ejp")) {
		err = mvNetaTxpEjpSet(p, i, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_4_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, txp, txq, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = txp = txq = v = 0;
	sscanf(buf, "%d %d %d %d", &p, &txp, &txq, &v);

	local_irq_save(flags);

	if (!strcmp(name, "wrr_regs")) {
		mvEthTxpWrrRegs(p, txp);
	} else if (!strcmp(name, "txq_rate")) {
		err = mvNetaTxqRateSet(p, txp, txq, v);
	} else if (!strcmp(name, "txq_burst")) {
		err = mvNetaTxqBurstSet(p, txp, txq, v);
	} else if (!strcmp(name, "txq_wrr")) {
		if (v == 0)
			err = mvNetaTxqFixPrioSet(p, txp, txq);
		else
			err = mvNetaTxqWrrPrioSet(p, txp, txq, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,           S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(wrr_regs,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(ejp,            S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(txp_rate,       S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(txp_burst,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(txq_rate,       S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_burst,      S_IWUSR, NULL, mv_eth_4_store);
static DEVICE_ATTR(txq_wrr,        S_IWUSR, NULL, mv_eth_4_store);

static struct attribute *mv_eth_tx_sched_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_wrr_regs.attr,
	&dev_attr_ejp.attr,
	&dev_attr_txp_rate.attr,
	&dev_attr_txp_burst.attr,
	&dev_attr_txq_rate.attr,
	&dev_attr_txq_burst.attr,
	&dev_attr_txq_wrr.attr,
	NULL
};

static struct attribute_group mv_eth_tx_sched_group = {
	.name = "tx_sched",
	.attrs = mv_eth_tx_sched_attrs,
};

int mv_neta_tx_sched_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_tx_sched_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_tx_sched_group.name, err);

	return err;
}

int mv_neta_tx_sched_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_tx_sched_group);

	return 0;
}

