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

	off += sprintf(buf+off, "echo [p]                     > napiShow      - show port's napi groups info\n");
	off += sprintf(buf+off, "echo [p] [group]             > napiCreate    - create an empty napi group (cpu_mask = rxq_mask = 0)\n");
	off += sprintf(buf+off, "echo [p] [group]             > napiDelete    - delete an existing empty napi group\n");
	off += sprintf(buf+off, "echo [p] [group] [cpus]      > cpuGroup      - set <cpus mask> for <port/napi group>\n");
	off += sprintf(buf+off, "echo [p] [group] [rxqs]      > rxqGroup      - set <rxqs mask> for <port/napi group>\n");

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
	unsigned int    p, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = v = 0;
	sscanf(buf, "%d %d", &p, &v);

	local_irq_save(flags);

	if (!strcmp(name, "napiShow")) {
		mv_eth_napi_groups_print(p);
	} else if (!strcmp(name, "napiCreate")) {
		mv_eth_port_napi_group_create(p, v);
	} else if (!strcmp(name, "napiDelete")) {
		mv_eth_port_napi_group_delete(p, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_eth_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, i, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %x", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "cpuGroup")) {
		err = mv_eth_napi_set_cpu_affinity(p, i, v);
	} else if (!strcmp(name, "rxqGroup")) {
		err = mv_eth_napi_set_rxq_affinity(p, i, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help,         S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(napiCreate,   S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(napiDelete,   S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(napiShow,     S_IWUSR, NULL, mv_eth_port_store);
static DEVICE_ATTR(cpuGroup,     S_IWUSR, NULL, mv_eth_3_hex_store);
static DEVICE_ATTR(rxqGroup,     S_IWUSR, NULL, mv_eth_3_hex_store);

static struct attribute *mv_eth_attrs[] = {
	&dev_attr_napiCreate.attr,
	&dev_attr_napiDelete.attr,
	&dev_attr_cpuGroup.attr,
	&dev_attr_rxqGroup.attr,
	&dev_attr_help.attr,
	&dev_attr_napiShow.attr,
	NULL
};

static struct attribute_group mv_eth_napi_group = {
	.name = "napi",
	.attrs = mv_eth_attrs,
};

int mv_pp2_napi_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_napi_group);
	if (err)
		pr_err("sysfs group i%s failed %d\n", mv_eth_napi_group.name, err);

	return err;
}

int mv_pp2_napi_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_napi_group);

	return 0;
}
