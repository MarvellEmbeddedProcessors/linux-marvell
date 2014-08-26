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

#include "mvOs.h"
#include "mvCommon.h"
#ifndef CONFIG_ARCH_MVEBU
#include "ctrlEnv/mvCtrlEnvLib.h"
#endif

#include "gbe/mvNeta.h"
#include "pnc/mvPnc.h"

#include "net_dev/mv_netdev.h"

static char	wol_data[MV_PNC_TOTAL_DATA_SIZE];
static int  wol_data_size = 0;
static char	wol_mask[MV_PNC_TOTAL_DATA_SIZE];
static int  wol_mask_size = 0;

extern void mv_eth_wol_wakeup(int port);
extern int mv_eth_wol_sleep(int port);

static ssize_t wol_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat                  dump_all      - dump all wol rules\n");
	off += sprintf(buf+off, "echo idx           > dump          - dump rule <idx>\n");
	off += sprintf(buf+off, "echo port          > sleep         - enter WoL mode on <port>\n");
	off += sprintf(buf+off, "echo port          > wakeup        - exit WoL mode on <port>\n");
	off += sprintf(buf+off, "echo str           > data          - set data string\n");
	off += sprintf(buf+off, "echo str           > mask          - set mask string\n");
	off += sprintf(buf+off, "echo port          > add           - add new rule with <data> and <mask> on <port>\n");
	off += sprintf(buf+off, "echo idx           > del           - delete existing WoL rule <idx>\n");
	off += sprintf(buf+off, "echo port          > del_all       - delete all WoL rules added to <port>\n");

	return off;
}

static ssize_t wol_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char   *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "dump_all"))
		mv_pnc_wol_dump();
	else if (!strcmp(name, "help"))
		return wol_help(buf);

	return 0;
}

static ssize_t wol_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  p, size, err = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d", &p);

	local_irq_save(flags);

	if (!strcmp(name, "sleep"))
		mv_eth_wol_sleep(p);
	else if (!strcmp(name, "wakeup"))
		mv_eth_wol_wakeup(p);
	else if (!strcmp(name, "dump")) {
		if (mv_pnc_wol_rule_dump(p))
			printk(KERN_INFO "WoL rule #%d doesn't exist\n", p);
	} else if (!strcmp(name, "data")) {
		memset(wol_data, 0, sizeof(wol_data));
		size = strlen(buf) / 2;
		if (size > sizeof(wol_data))
			size = sizeof(wol_data);
		mvHexToBin(buf, wol_data, size);
		wol_data_size = size;
	} else if (!strcmp(name, "mask")) {
		memset(wol_mask, 0, sizeof(wol_mask));
		size = strlen(buf) / 2;
		if (size > sizeof(wol_mask))
			size = sizeof(wol_mask);
		mvHexToBin(buf, wol_mask, size);
		wol_mask_size = size;
	} else if (!strcmp(name, "add")) {
		int idx;
		idx = mv_pnc_wol_rule_set(p, wol_data, wol_mask, MV_MIN(wol_data_size, wol_mask_size));
		if (idx < 0)
			err = 1;
	} else if (!strcmp(name, "del")) {
		err = mv_pnc_wol_rule_del(p);
	} else if (!strcmp(name, "del_all")) {
		err = mv_pnc_wol_rule_del_all(p);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, wol_show, wol_store);
static DEVICE_ATTR(dump_all,    S_IRUSR, wol_show, wol_store);
static DEVICE_ATTR(dump,        S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(data,        S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(mask,        S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(add,         S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(del,         S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(del_all,     S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(sleep,       S_IWUSR, wol_show, wol_store);
static DEVICE_ATTR(wakeup,      S_IWUSR, wol_show, wol_store);

static struct attribute *wol_attrs[] = {
    &dev_attr_help.attr,
    &dev_attr_dump_all.attr,
    &dev_attr_dump.attr,
    &dev_attr_data.attr,
    &dev_attr_mask.attr,
    &dev_attr_add.attr,
    &dev_attr_del.attr,
    &dev_attr_del_all.attr,
    &dev_attr_sleep.attr,
    &dev_attr_wakeup.attr,
    NULL
};

static struct attribute_group wol_group = {
	.name = "wol",
	.attrs = wol_attrs,
};

int mv_neta_wol_sysfs_init(struct kobject *neta_kobj)
{
	int err;

	err = sysfs_create_group(neta_kobj, &wol_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", wol_group.name, err);

	return err;
}

int mv_neta_wol_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &wol_group);

	return 0;
}

