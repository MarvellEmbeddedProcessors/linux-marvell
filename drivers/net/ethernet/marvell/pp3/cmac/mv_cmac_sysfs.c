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
#include <linux/netdevice.h>
#include "cmac/mv_cmac.h"

static ssize_t mv_cmac_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat            regs           - Dump CMAC unit registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat            eip197_regs    - Dump EIP197 units registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "cat            cmac_cfg       - Run default CMAC configuration\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [v]       > debug        - 0 disable, bit0 enable read, bit1 enable write debug outputs\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t mv_cmac_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int err = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		return mv_cmac_help(buf);
	else if (!strcmp(name, "regs"))
		mv_cmac_top_regs_dump();
	else if (!strcmp(name, "eip197_regs"))
		mv_cmac_eip197_regs_dump();
	else if (!strcmp(name, "cmac_cfg"))
		mv_pp3_cmac_config();
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	return err;
}

static ssize_t mv_cmac_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	unsigned int    p, err;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read parameters */
	p = err = 0;
	sscanf(buf, "%d", &p);

	local_irq_save(flags);

	if (!strcmp(name, "debug"))
		mv_pp3_cmac_debug_cfg(p);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,	S_IRUSR, mv_cmac_show, NULL);
static DEVICE_ATTR(regs,	S_IRUSR, mv_cmac_show, NULL);
static DEVICE_ATTR(eip197_regs, S_IRUSR, mv_cmac_show, NULL);
static DEVICE_ATTR(cmac_cfg,	S_IRUSR, mv_cmac_show, NULL);
static DEVICE_ATTR(debug,	S_IWUSR, NULL, mv_cmac_store);

static struct attribute *mv_cmac_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_eip197_regs.attr,
	&dev_attr_cmac_cfg.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group mv_cmac_group = {
	.name = "cmac",
	.attrs = mv_cmac_attrs,
};

int mv_pp3_cmac_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_cmac_group);
	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_cmac_sysfs_exit(struct kobject *cmac_kobj)
{
	sysfs_remove_group(cmac_kobj, &mv_cmac_group);
	return 0;
}
