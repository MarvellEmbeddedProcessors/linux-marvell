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

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "mv_bm.h"

static ssize_t pp3_dev_bm_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat                   > regs         - show BM registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat                   > err_regs     - show BM erorr registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat                   > idle_regs    - show BM idle mode registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [pool] [0|1]     > pool_regs    - show BM pool registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [pool] [0|1]     > pool_enable  - enable/disable BM pool\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [bank]           > bank_regs    - show BM bank registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [bank]           > bank_dump    - show BM bank cache memory\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [mask]           > debug        - Registers read and write debug outputs\n");


	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [pool]  - pool number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [bank]  - bank number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [mask]  - b0:read, b1:write\n");

	return o;
}

static ssize_t pp3_dev_bm_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char	*name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "regs"))
		bm_global_registers_dump();
	else if (!strcmp(name, "err_regs"))
		bm_error_dump();
	else if (!strcmp(name, "idle_regs"))
		bm_idle_status_dump();
	else
		off = pp3_dev_bm_help(buf);

	return off;
}



static ssize_t pp3_dev_bm_store(struct device *dev,
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

	if (!strcmp(name, "pool_enable")) {
		(b == 0) ? bm_pool_disable(a) : bm_pool_enable(a);
	} else if (!strcmp(name, "pool_regs")) {
		(b == 0) ? bm_pool_registers_dump(a) :  bm_pool_registers_parse(a);
	} else if (!strcmp(name, "bank_regs")) {
		bm_bank_registers_dump(a);
	} else if (!strcmp(name, "bank_dump")) {
		bm_bank_cache_dump(a);
	} else if (!strcmp(name, "debug")) {
		bm_dbg_flags(BM_F_DBG_RD, a & 0x1);
		bm_dbg_flags(BM_F_DBG_WR, a & 0x2);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(pool_regs,		S_IWUSR, NULL, pp3_dev_bm_store);
static DEVICE_ATTR(pool_enable,		S_IWUSR, NULL, pp3_dev_bm_store);
static DEVICE_ATTR(bank_regs,		S_IWUSR, NULL, pp3_dev_bm_store);
static DEVICE_ATTR(bank_dump,		S_IWUSR, NULL, pp3_dev_bm_store);
static DEVICE_ATTR(debug,		S_IWUSR, NULL, pp3_dev_bm_store);
static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_bm_show, NULL);
static DEVICE_ATTR(err_regs,		S_IRUSR, pp3_dev_bm_show, NULL);
static DEVICE_ATTR(idle_regs,		S_IRUSR, pp3_dev_bm_show, NULL);
static DEVICE_ATTR(regs,		S_IRUSR, pp3_dev_bm_show, NULL);

static struct attribute *pp3_dev_bm_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_err_regs.attr,
	&dev_attr_idle_regs.attr,
	&dev_attr_bank_regs.attr,
	&dev_attr_bank_dump.attr,
	&dev_attr_regs.attr,
	&dev_attr_pool_regs.attr,
	&dev_attr_pool_enable.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group pp3_dev_bm_group = {
	.attrs = pp3_dev_bm_attrs,
};

static struct kobject *bm_kobj;

int mv_pp3_bm_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	bm_kobj = kobject_create_and_add("bm", pp3_kobj);
	if (!bm_kobj) {
		printk(KERN_ERR"%s: cannot create bm kobject\n", __func__);
		return -ENOMEM;
	}
	err = sysfs_create_group(bm_kobj, &pp3_dev_bm_group);

	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}
/*
	TODO .... fix
	mv_pp3_bm_debug_sysfs_init(bm_kobj);
*/
	return err;
}

int mv_pp3_bm_sysfs_exit(struct kobject *dev_kobj)
{
	sysfs_remove_group(bm_kobj, &pp3_dev_bm_group);

	return 0;
}
