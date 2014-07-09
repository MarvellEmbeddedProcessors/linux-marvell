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

static ssize_t mv_pp2_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "echo [p] [hex]       > modCmd    - set 2 bytes of PME_ATTR filed in TX descriptor (offset: 0x16)\n");
	off += sprintf(buf+off, "echo [p] [hex]       > pmeDptr   - set 2 bytes of PME_DPTR field in TX descriptor (offset: 0x18)\n");
	off += sprintf(buf+off, "echo [p] [hex]       > pmeProg   - set 1 bytes of PME_PROG field in TX descriptor (offset: 0x20)\n");

	return off;
}

static ssize_t mv_pp2_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_pp2_help(buf);

	return off;
}

static ssize_t mv_pp2_port_store(struct device *dev,
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
	sscanf(buf, "%d %x", &p, &v);

	local_irq_save(flags);

	if (!strcmp(name, "modCmd")) {
		err = mv_pp2_ctrl_tx_cmd_mod(p, v);
	} else if (!strcmp(name, "pmeDptr")) {
		err = mv_pp2_ctrl_tx_cmd_pme_dptr(p, v);
	} else if (!strcmp(name, "pmeProgram")) {
		err = mv_pp2_ctrl_tx_cmd_pme_prog(p, v);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,        S_IRUSR, mv_pp2_show, NULL);
static DEVICE_ATTR(modCmd,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(pmeDptr,	S_IWUSR, NULL, mv_pp2_port_store);
static DEVICE_ATTR(pmeProgram,	S_IWUSR, NULL, mv_pp2_port_store);

static struct attribute *mv_pp2_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_modCmd.attr,
	&dev_attr_pmeDptr.attr,
	&dev_attr_pmeProgram.attr,
	NULL
};

static struct attribute_group gbe_pme_group = {
	.name = "pme",
	.attrs = mv_pp2_attrs,
};

int mv_pp2_gbe_pme_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &gbe_pme_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", gbe_pme_group.name, err);

	return err;
}

int mv_pp2_gbe_pme_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &gbe_pme_group);

	return 0;
}
