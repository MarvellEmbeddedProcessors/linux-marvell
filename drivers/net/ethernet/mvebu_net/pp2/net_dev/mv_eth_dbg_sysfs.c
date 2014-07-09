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
#include "mv_eth_sysfs.h"


static ssize_t mv_pp2_dbg_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat              clean     - Clean all ports\n");
	off += sprintf(buf+off, "cat              init      - Clean and init all ports\n");
	off += sprintf(buf+off, "echo offs      > regRead   - Read PPv2 register [ offs]\n");
	off += sprintf(buf+off, "echo offs hex  > regWrite  - Write value [hex] to PPv2 register [offs]\n");

	return off;
}

static ssize_t mv_pp2_dbg_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "clean"))
		mv_pp2_all_ports_cleanup();
	else if (!strcmp(name, "init")) {
		if (mv_pp2_all_ports_cleanup() == 0)
			/* probe only if all ports are clean */
			mv_pp2_all_ports_probe();
	} else
		off = mv_pp2_dbg_help(buf);

	return off;
}

static ssize_t mv_pp2_dbg_reg_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    r, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = r = v = 0;
	sscanf(buf, "%x %x", &r, &v);

	local_irq_save(flags);

	if (!strcmp(name, "regRead")) {
		v = mvPp2RdReg(r);
		pr_info("regRead val: 0x%08x\n", v);
	}  else if (!strcmp(name, "regWrite")) {
		mvPp2WrReg(r, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,          S_IRUSR, mv_pp2_dbg_show, NULL);
static DEVICE_ATTR(clean,         S_IRUSR, mv_pp2_dbg_show, NULL);
static DEVICE_ATTR(init,          S_IRUSR, mv_pp2_dbg_show, NULL);
static DEVICE_ATTR(regRead,       S_IWUSR, NULL, mv_pp2_dbg_reg_store);
static DEVICE_ATTR(regWrite,      S_IWUSR, NULL, mv_pp2_dbg_reg_store);


static struct attribute *mv_pp2_dbg_attrs[] = {
	&dev_attr_clean.attr,
	&dev_attr_init.attr,
	&dev_attr_help.attr,
	&dev_attr_regRead.attr,
	&dev_attr_regWrite.attr,
	NULL
};


static struct attribute_group mv_pp2_dbg_group = {
	.name = "dbg",
	.attrs = mv_pp2_dbg_attrs,
};

int mv_pp2_dbg_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &mv_pp2_dbg_group);
	if (err)
		pr_err("sysfs group i%s failed %d\n", mv_pp2_dbg_group.name, err);

	return err;
}

int mv_pp2_dbg_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mv_pp2_dbg_group);

	return 0;
}
