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
#include "cls/mvPp2ClsMcHw.h"


static MV_PP2_MC_ENTRY		mc;

static ssize_t mv_mc_help(char *buf)
{
	int off = 0;
	off += scnprintf(buf + off, PAGE_SIZE, "cat             sw_dump      - Dump software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             hw_dump      - Dump all hardware entries.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo i        > hw_write     - Write software entry into hardware <i>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo i        > hw_read      - Read entry <i> from hardware into software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1        > sw_clear     - Clear software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1        > hw_clear_all - Clear all multicast table entries in hardware.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo prio en  > mc_sw_prio   - Set priority enable <en> and value <prio> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo dscp en  > mc_sw_dscp   - Set DSCP enable <en> and value <dscp> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo gpid en  > mc_sw_gpid   - Set GemPortID enable <en> and value <gpid> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo d i      > mc_sw_modif  - Set modification data <d> and command <i> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo q        > mc_sw_queue  - Set Queue <q> value to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo en       > mc_sw_hwf    - Set HWF enabled <en=1> or disable <en=0> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo next     > mc_sw_next   - Set next pointer <next> to sw entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "\n");

	return off;
}

static ssize_t mv_mc_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "sw_dump"))
		mvPp2McSwDump(&mc);
	else if (!strcmp(name, "hw_dump"))
		mvPp2McHwDump();
	else
		off += mv_mc_help(buf);

	return off;
}


static ssize_t mv_mc_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0, d = 0, e = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x %x %x", &a, &b, &c, &d, &e);

	local_irq_save(flags);

	if (!strcmp(name, "hw_read"))
		mvPp2McHwRead(&mc, a);
	else if (!strcmp(name, "hw_write"))
		mvPp2McHwWrite(&mc, a);
	else if (!strcmp(name, "sw_clear"))
		mvPp2McSwClear(&mc);
	else if (!strcmp(name, "hw_clear_all"))
		mvPp2McHwClearAll();
	else if (!strcmp(name, "mc_sw_prio"))
		mvPp2McSwPrioSet(&mc, a, b);
	else if (!strcmp(name, "mc_sw_dscp"))
		mvPp2McSwDscpSet(&mc, a, b);
	else if (!strcmp(name, "mc_sw_gpid"))
		mvPp2McSwGpidSet(&mc, a, b);
	else if (!strcmp(name, "mc_sw_modif"))
		mvPp2McSwModSet(&mc, a, b);
	else if (!strcmp(name, "mc_sw_queue"))
		mvPp2McSwQueueSet(&mc, a);
	else if (!strcmp(name, "mc_sw_hwf"))
		mvPp2McSwForwardEn(&mc, a);
	else if (!strcmp(name, "mc_sw_next"))
		mvPp2McSwNext(&mc, a);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(hw_dump,			S_IRUSR, mv_mc_show, NULL);
static DEVICE_ATTR(sw_dump,			S_IRUSR, mv_mc_show, NULL);
static DEVICE_ATTR(help,			S_IRUSR, mv_mc_show, NULL);

static DEVICE_ATTR(hw_read,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(hw_write,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(sw_clear,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(hw_clear_all,		S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_prio,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_dscp,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_gpid,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_modif,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_queue,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_hwf,			S_IWUSR, NULL, mv_mc_store);
static DEVICE_ATTR(mc_sw_next,			S_IWUSR, NULL, mv_mc_store);


static struct attribute *mc_attrs[] = {
	&dev_attr_sw_dump.attr,
	&dev_attr_hw_dump.attr,
	&dev_attr_help.attr,
	&dev_attr_hw_read.attr,
	&dev_attr_hw_write.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_hw_clear_all.attr,
	&dev_attr_mc_sw_prio.attr,
	&dev_attr_mc_sw_dscp.attr,
	&dev_attr_mc_sw_gpid.attr,
	&dev_attr_mc_sw_modif.attr,
	&dev_attr_mc_sw_queue.attr,
	&dev_attr_mc_sw_hwf.attr,
	&dev_attr_mc_sw_next.attr,
	NULL
};

static struct attribute_group mc_group = {
	.name = "mc",
	.attrs = mc_attrs,
};

int mv_pp2_mc_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &mc_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mc_group.name, err);

	return err;
}

int mv_pp2_mc_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &mc_group);

	return 0;
}

