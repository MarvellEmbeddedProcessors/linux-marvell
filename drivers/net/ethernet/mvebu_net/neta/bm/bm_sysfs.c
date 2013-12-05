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
#include "bm/mvBm.h"

static ssize_t bm_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat                regs         - show BM registers\n");
	off += sprintf(buf+off, "cat                stat         - show BM status\n");
	off += sprintf(buf+off, "cat                config       - show compile-time BM configuration\n");
	off += sprintf(buf+off, "echo p v           > dump       - dump BM pool <p>. v=0-brief, v=1-full\n");
	off += sprintf(buf+off, "echo p s           > size       - set packet size <s> to BM pool <p>\n");

	return off;
}

static ssize_t bm_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int          err = 0;
	const char   *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return bm_help(buf);
	else if (!strcmp(name, "regs"))
		mvBmRegs();
	else if (!strcmp(name, "stat"))
		mvBmStatus();
	else if (!strcmp(name, "config"))
		mv_eth_bm_config_print();
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	return err;
}
static ssize_t bm_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, pool = 0, val = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d", &pool, &val);

	local_irq_save(flags);

	if (!strcmp(name, "dump")) {
		mvBmPoolDump(pool, val);
	} else if (!strcmp(name, "size")) {
		err = mv_eth_ctrl_pool_size_set(pool, val);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(size,   S_IWUSR, NULL, bm_store);
static DEVICE_ATTR(dump,   S_IWUSR, NULL, bm_store);
static DEVICE_ATTR(config, S_IRUSR, bm_show, NULL);
static DEVICE_ATTR(stat,   S_IRUSR, bm_show, NULL);
static DEVICE_ATTR(regs,   S_IRUSR, bm_show, NULL);
static DEVICE_ATTR(help,   S_IRUSR, bm_show, NULL);

static struct attribute *bm_attrs[] = {
	&dev_attr_size.attr,
	&dev_attr_dump.attr,
	&dev_attr_config.attr,
	&dev_attr_regs.attr,
	&dev_attr_stat.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group bm_group = {
	.name = "bm",
	.attrs = bm_attrs,
};

int mv_neta_bm_sysfs_init(struct kobject *neta_kobj)
{
	int err;

	err = sysfs_create_group(neta_kobj, &bm_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", bm_group.name, err);

	return err;
}

int mv_neta_bm_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &bm_group);

	return 0;
}

