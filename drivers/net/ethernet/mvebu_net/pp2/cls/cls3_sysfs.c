/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This SW file (the "File") is owned and distributed by Marvell
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
SW Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
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
#include "cls/mvPp2Cls3Hw.h"

static MV_PP2_CLS_C3_ENTRY		c3;


static ssize_t mv_cls3_help(char *buf)
{
	int off = 0;
	off += scnprintf(buf + off, PAGE_SIZE, "cat             hw_dump        - Dump all occupied entries from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             hw_ext_dump    - Dump all occupied extension table entries from HW.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "cat             hw_ms_dump     - Dump all miss table entires from HW.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "cat             sw_dump        - Dump SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             sc_res_dump    - Dump all valid scan results from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             sc_regs        - Dump scan registers.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             hw_query       - Get query for HEK in the SW entry and show result.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "cat             cnt_read_all   - Dump all hit counters for all changed indices and miss entries\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo lkp_type   > hw_ms_add    - Write entry from SW into HW miss table <lkp_type>\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "echo depth      > hw_query_add - Get query for HEK in the SW entry and Write entry into HW hash entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                 free entry search depth <depth>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx        > hw_read      - Read entry from HW <idx> into SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx extIdx > hw_add       - Write entry from SW into HW hash table <idx>\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                 external table entry index <extIdx> optionally used for long entries.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx        > hw_del       - Delete entry from HW <idx>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1          > hw_del_all   - Delete all c3 entries from HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1          > sw_clear     - Clear SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo val        > sw_init_cnt  - Set initial hit counter value <val> (in units of 64 hits) to SW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo info       > key_sw_l4    - Set L4 information <info> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo type       > key_sw_lkp_type - Set key lookup type to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id type    > key_sw_port  - Set key port ID <id> and port ID type to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo size       > key_sw_size  - Set key HEK size port ID <id> and port ID type to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo o d        > key_sw_byte  - Set byte of HEK data <d> and offset <o> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo o d        > key_sw_word  - Set byte of HEK data <d> and offset <o> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd         > act_sw_color - Set color command <cmd> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd qh      > act_sw_qh    - Set Queue High command <cmd> and value <qh> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd ql      > act_sw_ql    - Set Queue Low command <cmd> and value <ql> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd q       > act_sw_queue - Set full Queue command <cmd> and value <q> to action table SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd         > act_sw_fwd   - Set Forwarding command <cmd> to action table SW entry.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id bnk  > act_sw_pol   - Set PolicerID command <cmd> bank <bnk> and number <id> to action table SW entry.\n");
#else
	off += scnprintf(buf + off, PAGE_SIZE, "echo cmd id      > act_sw_pol   - Set PolicerID command <cmd> and number <id> to action table SW entry.\n");
#endif

	off += scnprintf(buf + off, PAGE_SIZE, "echo en          > act_sw_flowid- Set FlowID enable/disable <1/0> to action table SW entry.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx         > act_sw_mtu   - Set MTU index to action table SW entry\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "echo d i cs      > act_sw_mdf   - Set modification parameters to action table SW entry data pointer <d>\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                  instruction offset <i>, <cs> enable L4 checksum generation\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id cnt      > act_sw_dup   - Set packet duplication parameters <id, cnt> to action SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo id off bits > act_sw_sq    - Write sequence id <id> to offset <off> (in bits), id bits size <bits>\n");
	off += scnprintf(buf + off, PAGE_SIZE, "                                  to action SW entry\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx         > cnt_read     - Show hit counter for action table entry <idx>.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE, "echo lkp_type    > cnt_ms_read  - Show hit counter for action table miss entry <lkp_type>.\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1           > cnt_clr_all  - Clear hit counters for all action table entries.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo t           > cnt_clr_lkp  - Clear hit counters for all action table entries with lookup type <t>.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo 1           > sc_start     - Start new multi-hash scanning.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo m t         > sc_thresh    - Set scan threshold <t> and mode to above <m=1> or below <m=0> thresh.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo e           > sc_clear_before - clear hit counter before scan enable <e=1> or disable<e=0> in HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo t           > sc_lkp       - Set lookup type <t> for scan operation, <t=-1> all entries are scanned\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo idx         > sc_start_idx - Set scan start entry <idx> in HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo time        > sc_delay     - Set scan delay <time> in HW.\n");
	off += scnprintf(buf + off, PAGE_SIZE, "echo  idx        > sc_res_read  - Show result entry <idx> form scan result table in HW.\n");

	return off;
}


static ssize_t mv_cls3_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (!strcmp(name, "hw_dump"))
		mvPp2ClsC3HwDump();
	else if (!strcmp(name, "hw_ms_dump"))
		mvPp2ClsC3HwMissDump();
	else if (!strcmp(name, "hw_ext_dump"))
		mvPp2ClsC3HwExtDump();
	else if (!strcmp(name, "sw_dump"))
		mvPp2ClsC3SwDump(&c3);
	else if (!strcmp(name, "sc_res_dump"))
		mvPp2ClsC3ScanResDump();
	else if (!strcmp(name, "sc_regs"))
		mvPp2ClsC3ScanRegs();
	else if (!strcmp(name, "hw_query"))
		mvPp2ClsC3HwQuery(&c3, NULL, NULL);
	else if (!strcmp(name, "cnt_read_all"))
		mvPp2ClsC3HitCntrsReadAll();
	else
		off += mv_cls3_help(buf);

	return off;
}



static ssize_t mv_cls3_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0, d = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x %x", &a, &b, &c, &d);

	local_irq_save(flags);

	if (!strcmp(name, "hw_read"))
		mvPp2ClsC3HwRead(&c3, a);
	else if (!strcmp(name, "hw_query_add"))
		mvPp2ClsC3HwQueryAdd(&c3, a);
	else if (!strcmp(name, "hw_add"))
		mvPp2ClsC3HwAdd(&c3, a, b);
	else if (!strcmp(name, "hw_ms_add"))/*PPv2.1 new feature MAS 3.12*/
		mvPp2ClsC3HwMissAdd(&c3, a);
	else if (!strcmp(name, "hw_del"))
		mvPp2ClsC3HwDel(a);
	else if (!strcmp(name, "hw_del_all"))
		mvPp2ClsC3HwDelAll();
	else if (!strcmp(name, "sw_clear"))
		mvPp2ClsC3SwClear(&c3);
	else if (!strcmp(name, "sw_init_cnt"))
		mvPp2ClsC3HwInitCtrSet(a);
	else if (!strcmp(name, "key_sw_l4"))
		 mvPp2ClsC3SwL4infoSet(&c3, a);
	else if (!strcmp(name, "key_sw_lkp_type"))
		mvPp2ClsC3SwLkpTypeSet(&c3, a);
	else if (!strcmp(name, "key_sw_port"))
		mvPp2ClsC3SwPortIDSet(&c3, b, a);
	else if (!strcmp(name, "key_sw_size"))
		mvPp2ClsC3SwHekSizeSet(&c3, a);
	else if (!strcmp(name, "key_sw_byte"))
		mvPp2ClsC3SwHekByteSet(&c3, a, b);
	else if (!strcmp(name, "key_sw_word"))
		mvPp2ClsC3SwHekWordSet(&c3, a, b);
	else if (!strcmp(name, "act_sw_color"))
		mvPp2ClsC3ColorSet(&c3, a);
	else if (!strcmp(name, "act_sw_qh"))
		mvPp2ClsC3QueueHighSet(&c3, a, b);
	else if (!strcmp(name, "act_sw_ql"))
		mvPp2ClsC3QueueLowSet(&c3, a, b);
	else if (!strcmp(name, "act_sw_queue"))
		mvPp2ClsC3QueueSet(&c3, a, b);
	else if (!strcmp(name, "act_sw_fwd"))
		mvPp2ClsC3ForwardSet(&c3, a);
	else if (!strcmp(name, "act_sw_pol"))
#ifdef CONFIG_MV_ETH_PP2_1
		mvPp2ClsC3PolicerSet(&c3, a, b, c);
#else
		mvPp2ClsC3PolicerSet(&c3, a, b);
#endif
	else if (!strcmp(name, "act_sw_flowid"))
		mvPp2ClsC3FlowIdEn(&c3, a);
	else if (!strcmp(name, "act_sw_mdf"))
		mvPp2ClsC3ModSet(&c3, a, b, c);
	else if (!strcmp(name, "act_sw_mtu"))/*PPv2.1 new feature MAS 3.7*/
		mvPp2ClsC3MtuSet(&c3, a);
	else if (!strcmp(name, "act_sw_dup"))
		mvPp2ClsC3DupSet(&c3, a, b);
	else if (!strcmp(name, "act_sw_sq"))/*PPv2.1 new feature MAS 3.4*/
		mvPp2ClsC3SeqSet(&c3, a, b, c);
	else if (!strcmp(name, "cnt_read"))
		mvPp2ClsC3HitCntrsRead(a, NULL);
	else if (!strcmp(name, "cnt_ms_read"))
		mvPp2ClsC3HitCntrsMissRead(a, NULL);
	else if (!strcmp(name, "cnt_clr_all"))
		mvPp2ClsC3HitCntrsClearAll();
	else if (!strcmp(name, "cnt_clr_lkp"))
		mvPp2ClsC3HitCntrsClear(a);
	else if (!strcmp(name, "sc_start"))
		mvPp2ClsC3ScanStart();
	else if (!strcmp(name, "sc_thresh"))
		mvPp2ClsC3ScanThreshSet(a, b);
	else if (!strcmp(name, "sc_clear_before"))
		mvPp2ClsC3ScanClearBeforeEnSet(a);
	else if (!strcmp(name, "sc_start_idx"))
		mvPp2ClsC3ScanStartIndexSet(a);
	else if (!strcmp(name, "sc_delay"))
		mvPp2ClsC3ScanDelaySet(a);
	else if (!strcmp(name, "sc_res_read"))
		mvPp2ClsC3ScanResRead(a, NULL, NULL);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}

static ssize_t mv_cls3_signed_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0;
	unsigned long flags;
	int           a = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d", &a);

	local_irq_save(flags);

	if (!strcmp(name, "sc_lkp"))
		mvPp2ClsC3ScanLkpTypeSet(a);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(hw_dump,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(hw_ms_dump,		S_IRUSR, mv_cls3_show, NULL);/*PPv2.1 new feature MAS 3.7*/
static DEVICE_ATTR(hw_ext_dump,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(sw_dump,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(sc_res_dump,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(sc_regs,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(hw_query,		S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(cnt_read_all,	S_IRUSR, mv_cls3_show, NULL);
static DEVICE_ATTR(help,		S_IRUSR, mv_cls3_show, NULL);

static DEVICE_ATTR(hw_query_add,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(hw_read,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(hw_add,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(hw_ms_add,		S_IWUSR, NULL, mv_cls3_store);/*PPv2.1 new feature MAS 3.12*/
static DEVICE_ATTR(hw_del,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(hw_del_all,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sw_clear,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sw_init_cnt,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_l4,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_lkp_type,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_port,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_size,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_byte,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(key_sw_word,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_color,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_qh,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_ql,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_queue,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_fwd,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_pol,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_mdf,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_flowid,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_mtu,		S_IWUSR, NULL, mv_cls3_store);/*PPv2.1 new feature MAS 3.7*/
static DEVICE_ATTR(act_sw_dup,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(act_sw_sq,		S_IWUSR, NULL, mv_cls3_store);/*PPv2.1 new feature MAS 3.14*/
static DEVICE_ATTR(cnt_read,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(cnt_ms_read,		S_IWUSR, NULL, mv_cls3_store);/*PPv2.1 new feature MAS 3.12*/
static DEVICE_ATTR(cnt_clr_all,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(cnt_clr_lkp,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_start,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_thresh,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_clear_before,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_lkp,		S_IWUSR, NULL, mv_cls3_signed_store);
static DEVICE_ATTR(sc_start_idx,	S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_delay,		S_IWUSR, NULL, mv_cls3_store);
static DEVICE_ATTR(sc_res_read,		S_IWUSR, NULL, mv_cls3_store);



static struct attribute *cls3_attrs[] = {
	&dev_attr_hw_dump.attr,
	&dev_attr_hw_ms_dump.attr,
	&dev_attr_hw_ext_dump.attr,
	&dev_attr_sw_dump.attr,
	&dev_attr_sc_res_dump.attr,
	&dev_attr_sc_regs.attr,
	&dev_attr_hw_query.attr,
	&dev_attr_cnt_read_all.attr,
	&dev_attr_help.attr,
	&dev_attr_hw_query_add.attr,
	&dev_attr_hw_read.attr,
	&dev_attr_hw_add.attr,
	&dev_attr_hw_ms_add.attr,
	&dev_attr_hw_del.attr,
	&dev_attr_hw_del_all.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_sw_init_cnt.attr,
	&dev_attr_key_sw_l4.attr,
	&dev_attr_key_sw_lkp_type.attr,
	&dev_attr_key_sw_port.attr,
	&dev_attr_key_sw_size.attr,
	&dev_attr_key_sw_byte.attr,
	&dev_attr_key_sw_word.attr,
	&dev_attr_act_sw_color.attr,
	&dev_attr_act_sw_qh.attr,
	&dev_attr_act_sw_ql.attr,
	&dev_attr_act_sw_queue.attr,
	&dev_attr_act_sw_fwd.attr,
	&dev_attr_act_sw_pol.attr,
	&dev_attr_act_sw_mdf.attr,
	&dev_attr_act_sw_mtu.attr,
	&dev_attr_act_sw_dup.attr,
	&dev_attr_act_sw_sq.attr,
	&dev_attr_cnt_read.attr,
	&dev_attr_cnt_ms_read.attr,
	&dev_attr_cnt_clr_all.attr,
	&dev_attr_cnt_clr_lkp.attr,
	&dev_attr_act_sw_flowid.attr,
	&dev_attr_sc_start.attr,
	&dev_attr_sc_thresh.attr,
	&dev_attr_sc_clear_before.attr,
	&dev_attr_sc_lkp.attr,
	&dev_attr_sc_start_idx.attr,
	&dev_attr_sc_delay.attr,
	&dev_attr_sc_res_read.attr,
	NULL
};

static struct attribute_group cls3_group = {
	.name = "cls3",
	.attrs = cls3_attrs,
};

int mv_pp2_cls3_sysfs_init(struct kobject *pp2_kobj)
{
	int err = 0;

	err = sysfs_create_group(pp2_kobj, &cls3_group);
	if (err)
		printk(KERN_INFO "sysfs group %s failed %d\n", cls3_group.name, err);

	return err;
}

int mv_pp2_cls3_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &cls3_group);

	return 0;
}

