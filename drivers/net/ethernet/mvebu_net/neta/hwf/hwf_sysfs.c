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
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "gbe/mvNeta.h"
#include "net_dev/mv_netdev.h"

static ssize_t hwf_help(char *buf)
{
	int off = 0;

	off += mvOsSPrintf(buf+off, "cat                      help  - print this help\n");
#ifndef CONFIG_MV_ETH_BM_CPU
	off += mvOsSPrintf(buf+off, "cat                      bm    - print HWF BM information\n");
#endif
	off += mvOsSPrintf(buf+off, "echo rxp p txp         > regs  - print HWF registers of port <p>\n");
	off += mvOsSPrintf(buf+off, "echo rxp p txp         > cntrs - print HWF counters of port <p>\n");
	off += mvOsSPrintf(buf+off, "echo rxp p txp txq en  > en    - enable HWF from <rxp> to specific <txq>\n");
	off += mvOsSPrintf(buf+off, "echo rxp p txp txq a b > drop  - set HWF drop threshold <a> and Random bits <b>\n");

	return off;
}

static ssize_t hwf_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char   *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return hwf_help(buf);
#ifndef CONFIG_MV_ETH_BM_CPU
	else if (!strcmp(name, "bm"))
		mv_hwf_bm_dump();
#endif /* CONFIG_MV_ETH_BM_CPU */

	return 0;
}
static ssize_t hwf_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, rxp = 0, p = 0, txp = 0, txq = 0, a = 0, b = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %d %d %d %d", &rxp, &p, &txp, &txq, &a, &b);

	local_irq_save(flags);

	if (!strcmp(name, "regs")) {
		mvNetaHwfRxpRegs(rxp);
		mvNetaHwfTxpRegs(rxp, p, txp);
	} else if (!strcmp(name, "cntrs")) {
		mvNetaHwfTxpCntrs(rxp, p, txp);
	} else if (!strcmp(name, "en")) {
		if (a)        {
			/* Set txp/txq ownership to HWF */
			if (mv_eth_ctrl_txq_hwf_own(p, txp, txq, rxp)) {
				printk(KERN_ERR "%s failed: p=%d, txp=%d, txq=%d\n",
					__func__, p, txp, txq);
				return -EINVAL;
			}
		} else
			mv_eth_ctrl_txq_hwf_own(p, txp, txq, -1);

		mvNetaHwfTxqEnable(rxp, p, txp, txq, a);
	} else if (!strcmp(name, "drop")) {
		mvNetaHwfTxqDropSet(rxp, p, txp, txq, a, b);
	} else
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(regs,  S_IWUSR, hwf_show, hwf_store);
static DEVICE_ATTR(cntrs, S_IWUSR, hwf_show, hwf_store);
static DEVICE_ATTR(en,    S_IWUSR, hwf_show, hwf_store);
static DEVICE_ATTR(drop,  S_IWUSR, hwf_show, hwf_store);
static DEVICE_ATTR(bm,    S_IRUSR, hwf_show, hwf_store);
static DEVICE_ATTR(help,  S_IRUSR, hwf_show, hwf_store);

static struct attribute *hwf_attrs[] = {
	&dev_attr_regs.attr,
	&dev_attr_cntrs.attr,
	&dev_attr_en.attr,
	&dev_attr_drop.attr,
	&dev_attr_bm.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group hwf_group = {
	.name = "hwf",
	.attrs = hwf_attrs,
};

int mv_neta_hwf_sysfs_init(struct kobject *neta_kobj)
{
	int err;

	err = sysfs_create_group(neta_kobj, &hwf_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", hwf_group.name, err);

	return err;
}

int mv_neta_hwf_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &hwf_group);

	return 0;
}

