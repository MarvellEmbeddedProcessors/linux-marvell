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
#include "cls/mvPp2Cls2Hw.h"


static MV_PP2_CLS_C2_QOS_ENTRY		qos_entry;
static MV_PP2_CLS_C2_ENTRY		act_entry;




static ssize_t mv_cls_help(char *buf)
{
	int off = 0;

	off += scnprintf(buf + off, PAGE_SIZE, "cat  qos_sw_dump  - dump QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  prio_hw_dump - dump all QoS priority tables from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  dscp_hw_dump - dump all QoS dscp tables from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  act_sw_dump  - dump action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  act_hw_dump  - dump all action table enrties from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  hw_regs      - dump classifier C2 registers.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat  cnt_dump     - dump all hit counters that are not zeroed.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1             > qos_sw_clear           - clear QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1             > act_sw_clear           - clear action table SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id s ln       > qos_hw_write           - write QoS table SW entry into HW <id,s,ln>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id s ln       > qos_hw_read            - read QoS table entry from HW <id,s,ln>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo prio          > qos_sw_prio            - set priority <prio> value to QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo dscp          > qos_sw_dscp            - set DSCP <dscp> value to QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo color         > qos_sw_color           - set color value to QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id            > qos_sw_gemid           - set GemPortId <id> value to QoS table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo q             > qos_sw_queue           - set queue number <q> value to QoS table SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx           > act_hw_write           - write action table SW entry into HW <idx>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx           > act_hw_read            - read action table entry from HW <idx> into SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx           > act_hw_inv             - invalidate C2 entry <idx> in hw.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo               > act_hw_inv_all         - invalidate all C2 entries in HW.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo o d m         > act_sw_byte            - set byte <d,m> to TCAM offset <o> to action table SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo id sel        > act_sw_qos             - set QoS table <id,sel> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd from      > act_sw_color           - set color command <cmd> to action table SW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              <from> - source for color command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd prio from > act_sw_prio            - set priority command <cmd> and value <prio> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table SW entry. <from> - source for priority command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd prio from > act_sw_dscp            - set DSCP command <cmd> and value <dscp> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table SW entry. <from> - source for DSCP command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd gpid from > act_sw_gpid            - set GemPortID command <cmd> and value <gpid> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table SW entry. <from> - source for GemPortID command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q from    > act_sw_qh              - set queue high command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table software entry. <from>-source for Queue High command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q from    > act_sw_ql              - set queue low command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table software entry. <from> -source for Queue Low command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q from    > act_sw_queue           - set full queue command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              table software entry.  <from> -source for Queue command.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd           > act_sw_hwf             - set Forwarding command <cmd> to action table SW entry.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id bank   > act_sw_pol             - set PolicerID command <cmd> bank and number <id> to action table SW entry.\n");
#else
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id        > act_sw_pol             - set PolicerID command <cmd> and number <id> to action table SW entry.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "echo en            > act_sw_flowid          - set FlowID enable/disable <1/0> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo d i cs        > act_sw_mdf             - set modification parameters to action table SW entry\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              data pointer <d>, instruction pointrt <i>,\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                              <cs> enable L4 checksum generation.\n");

#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx           > act_sw_mtu             - set MTU index to action table SW entry\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo miss id       > act_sw_sq              - set miss bit and instruction ID to action table SW entry\n");
#endif

/*TODO ppv2.1: ADD sysfs command for mvPp2ClsC2SeqSet */
	off += scnprintf(buf + off, PAGE_SIZE, "echo id cnt        > act_sw_dup             - set packet duplication parameters <id,cnt> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo               > cnt_clr_all            - clear all hit counters from action tabe.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx           > cnt_read               - show hit counter for action table entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	return off;
}


static ssize_t mv_cls_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "qos_sw_dump"))
		off += mvPp2ClsC2QosSwDump(&qos_entry);
	else if (!strcmp(name, "prio_hw_dump"))
		off += mvPp2ClsC2QosPrioHwDump();
	else if (!strcmp(name, "dscp_hw_dump"))
		off += mvPp2ClsC2QosDscpHwDump();
	else if (!strcmp(name, "act_sw_dump"))
		off += mvPp2ClsC2SwDump(&act_entry);
	else if (!strcmp(name, "act_hw_dump"))
		off += mvPp2ClsC2HwDump();
	else if (!strcmp(name, "cnt_dump"))
		off += mvPp2ClsC2HitCntrsDump();
	else if (!strcmp(name, "hw_regs"))
		off += mvPp2ClsC2RegsDump();
	else
		off += mv_cls_help(buf);

	return off;
}


static ssize_t mv_cls_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0, d = 0, e = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x %x %x", &a, &b, &c, &d, &e);

	local_irq_save(flags);

	if (!strcmp(name, "act_hw_inv_all"))
		mvPp2ClsC2HwInvAll();
	else if (!strcmp(name, "act_hw_inv"))
		mvPp2ClsC2HwInv(a);
	else if (!strcmp(name, "qos_sw_clear"))
		mvPp2ClsC2QosSwClear(&qos_entry);
	else if (!strcmp(name, "qos_hw_write"))
		mvPp2ClsC2QosHwWrite(a, b, c, &qos_entry);
	else if (!strcmp(name, "qos_hw_read"))
		mvPp2ClsC2QosHwRead(a, b, c, &qos_entry);
	else if (!strcmp(name, "qos_sw_prio"))
		mvPp2ClsC2QosPrioSet(&qos_entry, a);
	else if (!strcmp(name, "qos_sw_dscp"))
		mvPp2ClsC2QosDscpSet(&qos_entry, a);
	else if (!strcmp(name, "qos_sw_color"))
		mvPp2ClsC2QosColorSet(&qos_entry, a);
	else if (!strcmp(name, "qos_sw_gemid"))
		mvPp2ClsC2QosGpidSet(&qos_entry, a);
	else if (!strcmp(name, "qos_sw_queue"))
		mvPp2ClsC2QosQueueSet(&qos_entry, a);
	else if (!strcmp(name, "act_sw_clear"))
		mvPp2ClsC2SwClear(&act_entry);
	else if (!strcmp(name, "act_hw_write"))
		mvPp2ClsC2HwWrite(a, &act_entry);
	else if (!strcmp(name, "act_hw_read"))
		mvPp2ClsC2HwRead(a, &act_entry);
	else if (!strcmp(name, "act_sw_byte"))
		mvPp2ClsC2TcamByteSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_qos"))
		mvPp2ClsC2QosTblSet(&act_entry, a, b);
	else if (!strcmp(name, "act_sw_color"))
		mvPp2ClsC2ColorSet(&act_entry, a, b);
	else if (!strcmp(name, "act_sw_prio"))
		mvPp2ClsC2PrioSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_dscp"))
		mvPp2ClsC2DscpSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_gpid"))
		mvPp2ClsC2GpidSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_qh"))
		mvPp2ClsC2QueueHighSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_ql"))
		mvPp2ClsC2QueueLowSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_queue"))
		mvPp2ClsC2QueueSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_hwf"))
		mvPp2ClsC2ForwardSet(&act_entry, a);
	else if (!strcmp(name, "act_sw_pol"))
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2ClsC2PolicerSet(&act_entry, a, b, c);
#else
		mvPp2ClsC2PolicerSet(&act_entry, a, b);
#endif
	else if (!strcmp(name, "act_sw_mdf"))
		mvPp2ClsC2ModSet(&act_entry, a, b, c);
	else if (!strcmp(name, "act_sw_mtu"))/*PPv2.1 new feature MAS 3.7*/
		mvPp2ClsC2MtuSet(&act_entry, a);
	else if (!strcmp(name, "act_sw_dup"))
		mvPp2ClsC2DupSet(&act_entry, a, b);
	else if (!strcmp(name, "act_sw_sq"))/*PPv2.1 new feature MAS 3.14*/
		mvPp2ClsC2SeqSet(&act_entry, a, b);
	else if (!strcmp(name, "cnt_clr_all"))
		mvPp2ClsC2HitCntrsClearAll();
	else if (!strcmp(name, "act_sw_flowid"))
		mvPp2ClsC2FlowIdEn(&act_entry, a);
	else if (!strcmp(name, "cnt_read"))
		mvPp2ClsC2HitCntrRead(a, NULL);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(prio_hw_dump,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(dscp_hw_dump,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(qos_sw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(act_sw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(act_hw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(cnt_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(hw_regs,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(help,			S_IRUSR, mv_cls_show, NULL);

static DEVICE_ATTR(qos_sw_clear,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_hw_write,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_hw_read,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_sw_prio,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_sw_dscp,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_sw_color,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_sw_gemid,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(qos_sw_queue,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_hw_inv,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_hw_inv_all,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_clear,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_hw_write,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_hw_read,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_byte,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_color,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_prio,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_dscp,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_gpid,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_qh,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_ql,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_queue,		S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_hwf,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_pol,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_mdf,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_mtu,			S_IWUSR, mv_cls_show, mv_cls_store);/*PPv2.1 new feature MAS 3.7*/
static DEVICE_ATTR(act_sw_dup,			S_IWUSR, mv_cls_show, mv_cls_store);/*PPv2.1 new feature MAS 3.14*/
static DEVICE_ATTR(act_sw_sq,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(cnt_clr_all,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_qos,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(cnt_read,			S_IWUSR, mv_cls_show, mv_cls_store);
static DEVICE_ATTR(act_sw_flowid,		S_IWUSR, mv_cls_show, mv_cls_store);


static struct attribute *cls2_attrs[] = {
	&dev_attr_prio_hw_dump.attr,
	&dev_attr_dscp_hw_dump.attr,
	&dev_attr_qos_sw_dump.attr,
	&dev_attr_act_sw_dump.attr,
	&dev_attr_act_hw_dump.attr,
	&dev_attr_cnt_dump.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_help.attr,
	&dev_attr_qos_sw_clear.attr,
	&dev_attr_qos_hw_write.attr,
	&dev_attr_qos_hw_read.attr,
	&dev_attr_qos_sw_prio.attr,
	&dev_attr_qos_sw_dscp.attr,
	&dev_attr_qos_sw_color.attr,
	&dev_attr_qos_sw_gemid.attr,
	&dev_attr_qos_sw_queue.attr,
	&dev_attr_act_hw_inv.attr,
	&dev_attr_act_hw_inv_all.attr,
	&dev_attr_act_sw_clear.attr,
	&dev_attr_act_hw_write.attr,
	&dev_attr_act_hw_read.attr,
	&dev_attr_act_sw_byte.attr,
	&dev_attr_act_sw_color.attr,
	&dev_attr_act_sw_prio.attr,
	&dev_attr_act_sw_dscp.attr,
	&dev_attr_act_sw_gpid.attr,
	&dev_attr_act_sw_qh.attr,
	&dev_attr_act_sw_ql.attr,
	&dev_attr_act_sw_queue.attr,
	&dev_attr_act_sw_hwf.attr,
	&dev_attr_act_sw_pol.attr,
	&dev_attr_act_sw_mdf.attr,
	&dev_attr_act_sw_mtu.attr,/*PPv2.1 new feature MAS 3.7*/
	&dev_attr_act_sw_dup.attr,
	&dev_attr_act_sw_sq.attr,/*PPv2.1 new feature MAS 3.14*/
	&dev_attr_cnt_clr_all.attr,
	&dev_attr_act_sw_qos.attr,
	&dev_attr_cnt_read.attr,
	&dev_attr_act_sw_flowid.attr,
	NULL
};

static struct attribute_group cls2_group = {
	.name = "cls2",
	.attrs = cls2_attrs,
};

int mv_pp2_cls2_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &cls2_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", cls2_group.name, err);

	return err;
}

int mv_pp2_cls2_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &cls2_group);
	return 0;
}

