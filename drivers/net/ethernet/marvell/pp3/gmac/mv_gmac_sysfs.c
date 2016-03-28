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
#include "gmac/mv_gmac.h"

static ssize_t mv_gmac_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > regs       - show GMAC port registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > mib_cntrs  - show GMAC port MIB counters\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u] [v] > reg_write  - write register: gmac [p], address [u], value [v]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u]     > reg_read   - read register: gmac [p], address [u]\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [p]  - gmac number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t mv_gmac_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_gmac_help(buf);

	return off;
}

static ssize_t mv_gmac_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, u, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = u = v = 0;
	sscanf(buf, "%x %x %x", &p, &u, &v);

	local_irq_save(flags);

	if (!strcmp(name, "regs"))
		pp3_gmac_port_regs(p);
	else if (!strcmp(name, "mib_cntrs"))
		pp3_gmac_mib_counters_show(p);
	else if (!strcmp(name, "reg_write"))
		pp3_gmac_reg_write(p, u, v);
	else if (!strcmp(name, "reg_read"))
		pp3_gmac_reg_read(p, u);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help, S_IRUSR, mv_gmac_show, NULL);
static DEVICE_ATTR(regs, S_IWUSR, NULL, mv_gmac_3_hex_store);
static DEVICE_ATTR(reg_write, S_IWUSR, NULL, mv_gmac_3_hex_store);
static DEVICE_ATTR(reg_read, S_IWUSR, NULL, mv_gmac_3_hex_store);
static DEVICE_ATTR(mib_cntrs, S_IWUSR, NULL, mv_gmac_3_hex_store);


static struct attribute *mv_gmac_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_mib_cntrs.attr,
	NULL
};

static struct attribute_group mv_gmac_group = {
	.name = "gmac",
	.attrs = mv_gmac_attrs,
};


int mv_pp3_gmac_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_gmac_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", mv_gmac_group.name, err);
		return err;
	}

	return err;
}

int mv_pp3_gmac_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &mv_gmac_group);

	return 0;
}
