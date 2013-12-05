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
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "../../../mv_hal/pp2/cls/mvPp2Cls4Hw.h"


static MV_PP2_CLS_C4_ENTRY		C4;



static ssize_t mv_cls_help(char *buf)
{
	int off = 0;
	off += scnprintf(buf + off, PAGE_SIZE, "cat               sw_dump               - Dump software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat               hw_regs               - Dump hardware registers.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat               hw_dump               - Dump all hardware entries.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "cat               hw_hits               - Dump non zeroed hit counters and the associated HW entries\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo p s r      > hw_port_rules         - Set physical port number <p> for rules set <s>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          <rules> - number of rules.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo p s r      > hw_uni_rules          - Set uni port number <p> for rules set <s>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          <rules> - number of rules.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");

	off += scnprintf(buf + off, PAGE_SIZE, "echo s r        > hw_write              - Write software entry into hardware <set=s,rule=r>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo s r        > hw_read               - Read entry <set=s,rule=r> from hardware into software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1          > sw_clear              - Clear software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1          > hw_clear_all          - Clear all C4 rules in hardware.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo f o d      > rule_two_b            - Set two bytes of data <d> in field <f> with offset <o> to\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo f id op    > rule_params           - Set ID <id> and OpCode <op> to filed <f> in software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo mode       > rule_sw_pppoe         - Set PPPOE mode to software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo mode       > rule_sw_vlan          - Set VLAN mode to software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo mode       > rule_sw_mac           - Set mac to me mode to software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo mode       > rule_sw_l4            - Set L4 info mode to software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo mode       > rule_sw_l3            - Set L3 info mode to software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd        > act_sw_color          - Set Color command <cmd> to action table software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd prio   > act_sw_prio           - Set priority command <cmd> and value <prio> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd dscp   > act_sw_dscp           - Set DSCP command <cmd> and value <dscp> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd gpid   > act_sw_gpid           - Set GemPortID command <cmd> and value <gpid> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q      > act_sw_qh             - Set queue high command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q      > act_sw_ql             - Set queue low command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd        > act_sw_fwd            - Set Forwarding command <cmd> to action table software entry\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q      > act_sw_queue          - Set full queue command <cmd> and value <q> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id bnk > act_sw_pol            - Set PolicerId command <cmd> bank <bnk> and numver <id> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
#else
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id     > act_sw_pol            - Set PolicerId command <cmd> and numver <id> to action\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                          table software entry.\n");
#endif
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

	if (!strcmp(name, "sw_dump"))
		mvPp2ClsC4SwDump(&C4);
	else if (!strcmp(name, "hw_regs"))
		mvPp2ClsC4RegsDump();
	else if (!strcmp(name, "hw_dump"))
		mvPp2ClsC4HwDumpAll();
	else if (!strcmp(name, "hw_hits"))
		mvPp2V1ClsC4HwHitsDump();

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

	if (!strcmp(name, "hw_port_rules"))
		mvPp2ClsC4HwPortToRulesSet(a, b, c);
	else if (!strcmp(name, "hw_uni_rules"))
		mvPp2ClsC4HwUniToRulesSet(a, b, c);
	else if (!strcmp(name, "hw_read"))
		mvPp2ClsC4HwRead(&C4, b, a);
	else if (!strcmp(name, "hw_write"))
		mvPp2ClsC4HwWrite(&C4, b, a);
	else if (!strcmp(name, "sw_clear"))
		mvPp2ClsC4SwClear(&C4);
	else if (!strcmp(name, "hw_clear_all"))
		mvPp2ClsC4HwClearAll();
	else if (!strcmp(name, "rule_two_b"))
		mvPp2ClsC4FieldsShortSet(&C4, a, b, (unsigned short) c);
	else if (!strcmp(name, "rule_params"))
		mvPp2ClsC4FieldsParamsSet(&C4, a, b, c);
	else if (!strcmp(name, "rule_sw_vlan"))
		mvPp2ClsC4SwVlanSet(&C4, a);
	else if (!strcmp(name, "rule_sw_pppoe"))
		mvPp2ClsC4SwPppoeSet(&C4, a);
	else if (!strcmp(name, "rule_sw_mac"))
		mvPp2ClsC4SwMacMeSet(&C4, a);
	else if (!strcmp(name, "rule_sw_l4"))
		mvPp2ClsC4SwL4InfoSet(&C4, a);
	else if (!strcmp(name, "rule_sw_l3"))
		mvPp2ClsC4SwL3InfoSet(&C4, a);
	else if (!strcmp(name, "act_sw_color"))
		mvPp2ClsC4ColorSet(&C4, a);
	else if (!strcmp(name, "act_sw_prio"))
		mvPp2ClsC4PrioSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_dscp"))
		mvPp2ClsC4DscpSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_gpid"))
		mvPp2ClsC4GpidSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_qh"))
		mvPp2ClsC4QueueHighSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_ql"))
		mvPp2ClsC4QueueLowSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_fwd"))
		mvPp2ClsC4ForwardSet(&C4, a);
	else if (!strcmp(name, "act_sw_queue"))
		mvPp2ClsC4QueueSet(&C4, a, b);
	else if (!strcmp(name, "act_sw_pol"))
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2ClsC4PolicerSet(&C4, a, b, c);
#else
		mvPp2ClsC4PolicerSet(&C4, a, b);
#endif
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(hw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(sw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(hw_regs,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(hw_hits,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(help,			S_IRUSR, mv_cls_show, NULL);

static DEVICE_ATTR(hw_port_rules,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(hw_uni_rules,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(hw_read,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(hw_write,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(sw_clear,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(hw_clear_all,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_two_b,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_params,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_sw_vlan,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_sw_pppoe,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_sw_mac,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_sw_l4,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(rule_sw_l3,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_color,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_prio,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_dscp,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_gpid,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_qh,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_ql,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_fwd,			S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_queue,		S_IWUSR, NULL, mv_cls_store);
static DEVICE_ATTR(act_sw_pol,			S_IWUSR, NULL, mv_cls_store);




static struct attribute *cls4_attrs[] = {
	&dev_attr_sw_dump.attr,
	&dev_attr_hw_dump.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_hw_hits.attr,
	&dev_attr_help.attr,
	&dev_attr_hw_port_rules.attr,
	&dev_attr_hw_uni_rules.attr,
	&dev_attr_hw_read.attr,
	&dev_attr_hw_write.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_hw_clear_all.attr,
	&dev_attr_rule_two_b.attr,
	&dev_attr_rule_params.attr,
	&dev_attr_rule_sw_vlan.attr,
	&dev_attr_rule_sw_pppoe.attr,
	&dev_attr_rule_sw_mac.attr,
	&dev_attr_rule_sw_l4.attr,
	&dev_attr_rule_sw_l3.attr,
	&dev_attr_act_sw_color.attr,
	&dev_attr_act_sw_prio.attr,
	&dev_attr_act_sw_dscp.attr,
	&dev_attr_act_sw_gpid.attr,
	&dev_attr_act_sw_qh.attr,
	&dev_attr_act_sw_ql.attr,
	&dev_attr_act_sw_fwd.attr,/*ppv2.1 new feature MAS 3.9*/
	&dev_attr_act_sw_queue.attr,
	&dev_attr_act_sw_pol.attr,
	NULL
};

static struct attribute_group cls4_group = {
	.name = "cls4",
	.attrs = cls4_attrs,
};

int mv_pp2_cls4_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &cls4_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", cls4_group.name, err);

	return err;
}

int mv_pp2_cls4_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &cls4_group);

	return 0;
}

