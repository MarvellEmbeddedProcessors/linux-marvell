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

static ssize_t mv_pp2_hwf_help(char *buf)
{
	int o = 0;

	o += scnprintf(buf+o, PAGE_SIZE-o, "id     - is hex number\n");
	o += scnprintf(buf+o, PAGE_SIZE-o, "others - are dec numbers\n\n");

	o += scnprintf(buf+o, PAGE_SIZE-o, "cat                    regs    - show SWF to HWF switching registers\n");
	o += scnprintf(buf+o, PAGE_SIZE-o, "cat                    status  - show SWF to HWF switching status\n");
	o += scnprintf(buf+o, PAGE_SIZE-o, "echo msec            > timeout - set SWF to HWF switching timeout\n");
	o += scnprintf(buf+o, PAGE_SIZE-o, "echo id txq rxq msec > switch  - start SWF to HWF switching process\n");
#ifdef CONFIG_MV_PP2_SWF_HWF_CORRUPTION_WA
	o += scnprintf(buf+o, PAGE_SIZE-o, "echo en              > c_inv   - on/off L1 and L2 cache invalidation\n");
#endif
	return o;
}

static ssize_t mv_pp2_hwf_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;
	const char      *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "regs"))
		mvPp2FwdSwitchRegs();
	else if (!strcmp(name, "status")) {
		int state, status, msec;

		status = mvPp2FwdSwitchStatus(&state, &msec);
		pr_info("\n[FWD Switch status]\n");
		pr_info("\t status=%d, hwState=%d, msec=%d\n", status, state, msec);
	} else
		off = mv_pp2_hwf_help(buf);

	return off;
}

static unsigned int fwd_switch_msec = 3;

static ssize_t mv_pp2_hwf_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             num, err;
	unsigned int    id, rxq, txq, msec;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = id = txq = rxq = msec = 0;
	num = sscanf(buf, "%x %d %d %d", &id, &txq, &rxq, &msec);
	if (num < 4)
		msec = fwd_switch_msec;

	local_irq_save(flags);

	if (!strcmp(name, "switch")) {
		err = mvPp2FwdSwitchCtrl(id, txq, rxq, msec);
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_pp2_hwf_dec_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned int    val;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = val = 0;
	sscanf(buf, "%d ", &val);

	local_irq_save(flags);

	if (!strcmp(name, "timeout")) {
		fwd_switch_msec = val;
#ifdef CONFIG_MV_PP2_SWF_HWF_CORRUPTION_WA
	} else if (!strcmp(name, "c_inv")) {
		mv_pp2_cache_inv_wa_ctrl(val);
#endif
	} else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, mv_pp2_hwf_show, NULL);
static DEVICE_ATTR(regs,		S_IRUSR, mv_pp2_hwf_show, NULL);
static DEVICE_ATTR(status,		S_IRUSR, mv_pp2_hwf_show, NULL);
static DEVICE_ATTR(switch,		S_IWUSR, NULL, mv_pp2_hwf_store);
static DEVICE_ATTR(timeout,		S_IWUSR, NULL, mv_pp2_hwf_dec_store);
#ifdef CONFIG_MV_PP2_SWF_HWF_CORRUPTION_WA
static DEVICE_ATTR(c_inv,		S_IWUSR, NULL, mv_pp2_hwf_dec_store);
#endif

static struct attribute *mv_pp2_hwf_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_status.attr,
	&dev_attr_switch.attr,
	&dev_attr_timeout.attr,
#ifdef CONFIG_MV_PP2_SWF_HWF_CORRUPTION_WA
	&dev_attr_c_inv.attr,
#endif
	NULL
};

static struct attribute_group mv_pp2_hwf_group = {
	.name = "hwf",
	.attrs = mv_pp2_hwf_attrs,
};


int mv_pp2_gbe_hwf_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_pp2_hwf_group);
	if (err)
		printk(KERN_INFO "sysfs group failed %d\n", err);

	return err;
}

int mv_pp2_gbe_hwf_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_pp2_hwf_group);

	return 0;
}

