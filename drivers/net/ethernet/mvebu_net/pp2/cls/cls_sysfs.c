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
#include "cls/mvPp2ClsHw.h"

static MV_PP2_CLS_LKP_ENTRY	lkp_entry;
static MV_PP2_CLS_FLOW_ENTRY	flow_entry;


static ssize_t mv_cls_help(char *buf)
{
	int off = 0;
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             lkp_sw_dump          - dump lookup ID table sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             flow_sw_dump         - dump flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             lkp_hw_dump          - dump lookup ID tabel from hardware.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             flow_hw_hits         - dump non zeroed hit counters  and the associated flow tabel entries from hardware.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             lkp_hw_hits          - dump non zeroed hit counters and the associated lookup ID entires from hardware.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             flow_hw_dump         - dump flow table from hardware.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             len_change_hw_dump   - lkp dump sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "cat             hw_regs              - dump classifier top registers.\n");

	off += scnprintf(buf + off, PAGE_SIZE,  "\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo 1          >lkp_sw_clear        - clear lookup ID table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo 1          >flow_sw_clear       - clear flow table SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE,  "\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo en         >hw_enable           - classifier enable/disable <en = 1/0>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p w        >hw_port_way         - set lookup way <w> for physical port <p>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p mode     >hw_port_spid        - set SPID extraction mode <mode> for physical port <p>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo uni spid   >hw_uni_spid         - set port <uni> for spid <spid>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo virt gpid  >hw_virt_gpid        - set virtual port number <virt> for GemPortId <gpid>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo a b c d    >hw_udf              - set UDF field <a> as: base <b>, offset <c> bits, size<d> bits.\n");

#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p q        >hw_over_rxq_low     - set oversize rx low queue <q> for ingress port <p>.\n");
#else
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p q        >hw_over_rxq         - set oversize rxq <q> for ingress port <p>.\n");
#endif
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p from q   >hw_qh               - set rx high queue source <from> and queue <q> for ingress port <p>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo idx m      >hw_mtu              - set MTU value <m> for index <idx>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p v u mh   >hw_mh               - set port <p> enable/disable port Id generation for.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "                                       virtual <v=0,1> and uni <u=0,1> ports, set default Marvell header <mh>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo idx size   >hw_sq_size          - set sequence id number <idx> size to flow table software entry\n");
#else
	off += scnprintf(buf + off, PAGE_SIZE,  "echo p txp m    >hw_mtu              - set MTU value <m> for egress port <p, txp>.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE,  "\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo idx way    >lkp_hw_write        - write lookup ID table SW entry HW <idx,way>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo idx way    >lkp_hw_read         - read lookup ID table entry from HW <idx,way>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo rxq        >lkp_sw_rxq          - set default RXQ <rxq> to lookup ID table.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo f          >lkp_sw_flow         - set index of firs insruction <f> in flow table\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "                                       to lookup ID SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo m          >lkp_sw_mod          - set modification instruction offset <m> to lookup ID SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo e          >lkp_sw_en           - Enable <e=1> or disable <e=0> lookup ID table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo id         >flow_hw_write       - write flow table SW entry to HW <id>.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo id         >flow_hw_read        - read flow table entry <id> from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo t id       >flow_sw_port        - set port type <t> and id <p> to flow table SW entry\n");
#ifdef CONFIG_MV_ETH_PP2_1
	/*PPv2.1 new feature MAS 3.18*/
	off += scnprintf(buf + off, PAGE_SIZE,  "echo from       >flow_sw_portid      - set cls to recive portid via packet <from=1>\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "                                       or via user configurration <from=0>  to flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo mode       >flow_sw_pppoe       - Set PPPoE lookup skip mode <mode> to flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo mode       >flow_sw_vlan        - Set VLAN lookup skip mode <mode> to flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo mode       >flow_sw_macme       - Set MAC ME lookup skip mode <mode> to flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo mode       >flow_sw_udf7        - Set UDF7 lookup skip mode <mode> to flow table SW entry.\n");
	/*PPv2.1 new feature MAS 3.14*/
	off += scnprintf(buf + off, PAGE_SIZE,  "echo mode       >flow_sw_sq          - Set sequence type <mode> to flow table SW entry.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE,  "echo e l        >flow_sw_engin       - set engine <e> nember to flow table SW entry.  <l> - last bit.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo l p        >flow_sw_extra       - set lookup type <l> and priority <p> to flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo idx id     >flow_sw_hek         - set HEK field <idx, id> flow table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo n          >flow_sw_num_of_heks - set number of HEK fields <n> to flow table SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE,  "\n");
	off += scnprintf(buf + off, PAGE_SIZE,  "echo i len      >len_change_hw_set   - set signed length <len> (in decimal) change for modification index <idx> (in hex).\n");

	return off;
}


static ssize_t mv_cls_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "lkp_sw_dump"))
		mvPp2ClsSwLkpDump(&lkp_entry);
	else if (!strcmp(name, "lkp_hw_hits"))
		mvPp2V1ClsHwLkpHitsDump();
	else if (!strcmp(name, "flow_hw_hits"))
		mvPp2V1ClsHwFlowHitsDump();
	else if (!strcmp(name, "flow_sw_dump"))
		mvPp2ClsSwFlowDump(&flow_entry);
	else if (!strcmp(name, "lkp_hw_dump"))
		mvPp2ClsHwLkpDump();
	else if (!strcmp(name, "flow_hw_dump"))
		mvPp2ClsHwFlowDump();
	else if (!strcmp(name, "len_change_hw_dump"))
		mvPp2ClsPktLenChangeDump();
	else if (!strcmp(name, "hw_regs"))
		mvPp2ClsHwRegsDump();
	else
		off += mv_cls_help(buf);

	return off;
}



static ssize_t mv_prs_store_unsigned(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0, d = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x %x", &a, &b, &c, &d);

	local_irq_save(flags);

	if (!strcmp(name, "lkp_sw_clear"))
		mvPp2ClsSwLkpClear(&lkp_entry);
	else if (!strcmp(name, "lkp_hw_write"))
		mvPp2ClsHwLkpWrite(a, b, &lkp_entry);
	else if (!strcmp(name, "lkp_hw_read"))
		mvPp2ClsHwLkpRead(a, b, &lkp_entry);
	else if (!strcmp(name, "lkp_sw_rxq"))
		mvPp2ClsSwLkpRxqSet(&lkp_entry, a);
	else if (!strcmp(name, "lkp_sw_flow"))
		mvPp2ClsSwLkpFlowSet(&lkp_entry, a);
	else if (!strcmp(name, "lkp_sw_mod"))
		mvPp2ClsSwLkpModSet(&lkp_entry, a);
	else if (!strcmp(name, "lkp_sw_en"))
		mvPp2ClsSwLkpEnSet(&lkp_entry, a);
	else if (!strcmp(name, "flow_sw_clear"))
		mvPp2ClsSwFlowClear(&flow_entry);
	else if (!strcmp(name, "flow_hw_write"))
		mvPp2ClsHwFlowWrite(a, &flow_entry);
	else if (!strcmp(name, "flow_hw_read"))
		mvPp2ClsHwFlowRead(a, &flow_entry);
	else if (!strcmp(name, "flow_sw_port"))
		mvPp2ClsSwFlowPortSet(&flow_entry, a, b);
	else if (!strcmp(name, "flow_sw_portid"))
		mvPp2ClsSwPortIdSelect(&flow_entry, a);
	else if (!strcmp(name, "flow_sw_pppoe"))
		mvPp2ClsSwFlowPppoeSet(&flow_entry, a);
	else if (!strcmp(name, "flow_sw_vlan"))
		mvPp2ClsSwFlowVlanSet(&flow_entry, a);
	else if (!strcmp(name, "flow_sw_macme"))
		mvPp2ClsSwFlowMacMeSet(&flow_entry, a);
	else if (!strcmp(name, "flow_sw_udf7"))
		mvPp2ClsSwFlowUdf7Set(&flow_entry, a);
	/*PPv2.1 feature changed MAS 3.14*/
	else if (!strcmp(name, "flow_sw_sq"))
		mvPp2ClsSwFlowSeqCtrlSet(&flow_entry, a);
	else if (!strcmp(name, "flow_sw_engine"))
		mvPp2ClsSwFlowEngineSet(&flow_entry, a, b);
	else if (!strcmp(name, "flow_sw_extra"))
		mvPp2ClsSwFlowExtraSet(&flow_entry, a, b);
	else if (!strcmp(name, "flow_sw_hek"))
		mvPp2ClsSwFlowHekSet(&flow_entry, a, b);
	else if (!strcmp(name, "flow_sw_num_of_heks"))
		mvPp2ClsSwFlowHekNumSet(&flow_entry, a);
	else if (!strcmp(name, "hw_enable"))
		mvPp2ClsHwEnable(a);
	else if (!strcmp(name, "hw_port_way"))
		mvPp2ClsHwPortWaySet(a, b);
	else if (!strcmp(name, "hw_port_spid"))
		mvPp2ClsHwPortSpidSet(a, b);
	else if (!strcmp(name, "hw_uni_spid"))
		mvPp2ClsHwUniPortSet(a, b);
	else if (!strcmp(name, "hw_virt_gpid"))
		mvPp2ClsHwVirtPortSet(a, b);
	else if (!strcmp(name, "hw_udf"))
		mvPp2ClsHwUdfSet(a, b, c, d);
	/*PPv2.1 feature changed MAS 3.7*/
	else if (!strcmp(name, "hw_mtu"))
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2V1ClsHwMtuSet(a, b);
#else
		mvPp2V0ClsHwMtuSet(a, b, c);
#endif
#ifdef CONFIG_MV_ETH_PP2_1
	else if (!strcmp(name, "hw_over_rxq_low"))
		mvPp2ClsHwOversizeRxqLowSet(a, b);
#else
	else if (!strcmp(name, "hw_over_rxq"))
		mvPp2ClsHwOversizeRxqSet(a, b);
#endif
	/*PPv2.1 new feature MAS 3.5*/
	else if (!strcmp(name, "hw_qh"))
		mvPp2ClsHwRxQueueHighSet(a, b, c);
	/*PPv2.1 new feature MAS 3.18*/
	else if (!strcmp(name, "hw_mh"))
		mvPp2ClsHwMhSet(a, b, c, d);
	/*PPv2.1 new feature MAS 3.18*/
	else if (!strcmp(name, "hw_sq_size"))
		mvPp2ClsHwSeqInstrSizeSet(a, b);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static ssize_t mv_prs_store_signed(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0;
	unsigned long flags;
	int b = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %d", &a, &b);

	local_irq_save(flags);

	if (!strcmp(name, "len_change_hw_set"))
		mvPp2ClsPktLenChangeSet(a, b);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(lkp_hw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(lkp_hw_hits,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(flow_hw_hits,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(flow_hw_dump,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(lkp_sw_dump,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(flow_sw_dump,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(len_change_hw_dump,		S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(help,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(hw_regs,			S_IRUSR, mv_cls_show, NULL);
static DEVICE_ATTR(lkp_sw_clear,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_hw_write,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_hw_read,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_sw_rxq,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_sw_flow,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_sw_mod,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(lkp_sw_en,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_clear,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_hw_write,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_hw_read,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_port,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_portid,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(flow_sw_pppoe,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(flow_sw_macme,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(flow_sw_vlan,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(flow_sw_udf7,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(flow_sw_sq,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);/*PPv2.1 new feature MAS 3.14*/
static DEVICE_ATTR(flow_sw_engine,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_extra,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_hek,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(flow_sw_num_of_heks,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(len_change_hw_set,		S_IWUSR, mv_cls_show, mv_prs_store_signed);
static DEVICE_ATTR(hw_enable,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_port_way,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_port_spid,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_uni_spid,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_virt_gpid,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_udf,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
static DEVICE_ATTR(hw_mtu,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
#ifdef CONFIG_MV_ETH_PP2_1
static DEVICE_ATTR(hw_over_rxq_low,		S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
#else
static DEVICE_ATTR(hw_over_rxq,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned);
#endif
static DEVICE_ATTR(hw_qh,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned); /*PPv2.1 new feature MAS 3.5*/
static DEVICE_ATTR(hw_mh,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned); /*PPv2.1 new feature MAS 3.18*/
static DEVICE_ATTR(hw_sq_size,			S_IWUSR, mv_cls_show, mv_prs_store_unsigned); /*PPv2.1 new feature MAS 3.14*/



static struct attribute *cls_attrs[] = {
	&dev_attr_lkp_sw_dump.attr,
	&dev_attr_flow_sw_dump.attr,
	&dev_attr_lkp_hw_hits.attr,
	&dev_attr_flow_hw_hits.attr,
	&dev_attr_lkp_hw_dump.attr,
	&dev_attr_flow_hw_dump.attr,
	&dev_attr_len_change_hw_dump.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_lkp_sw_clear.attr,
	&dev_attr_lkp_hw_write.attr,
	&dev_attr_lkp_hw_read.attr,
	&dev_attr_lkp_sw_rxq.attr,
	&dev_attr_lkp_sw_flow.attr,
	&dev_attr_lkp_sw_mod.attr,
	&dev_attr_lkp_sw_en.attr,
	&dev_attr_flow_sw_clear.attr,
	&dev_attr_flow_hw_write.attr,
	&dev_attr_flow_hw_read.attr,
	&dev_attr_flow_sw_port.attr,
	&dev_attr_flow_sw_portid.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_flow_sw_engine.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_flow_sw_vlan.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_flow_sw_pppoe.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_flow_sw_macme.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_flow_sw_sq.attr,/*PPv2.1 new feature MAS 3.14*/
	&dev_attr_flow_sw_udf7.attr,
	&dev_attr_flow_sw_extra.attr,
	&dev_attr_flow_sw_hek.attr,
	&dev_attr_flow_sw_num_of_heks.attr,
	&dev_attr_len_change_hw_set.attr,
	&dev_attr_hw_enable.attr,
	&dev_attr_hw_port_way.attr,
	&dev_attr_hw_port_spid.attr,
	&dev_attr_hw_uni_spid.attr,
	&dev_attr_hw_virt_gpid.attr,
	&dev_attr_hw_udf.attr,
	&dev_attr_hw_mtu.attr,/*PPv2.1 feature changed MAS 3.7*/
#ifdef CONFIG_MV_ETH_PP2_1
	&dev_attr_hw_over_rxq_low.attr,/*PPv2.1 feature changed MAS 3.7*/
#else
	&dev_attr_hw_over_rxq.attr,
#endif
	&dev_attr_hw_qh.attr,/*PPv2.1 new feature MAS 3.5*/
	&dev_attr_hw_mh.attr,/*PPv2.1 new feature MAS 3.18*/
	&dev_attr_hw_sq_size.attr,/*PPv2.1 new feature MAS 3.14*/
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group cls_group = {
	.name = "cls",
	.attrs = cls_attrs,
};

int mv_pp2_cls_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &cls_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", cls_group.name, err);

	return err;
}

int mv_pp2_cls_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &cls_group);

	return 0;
}
