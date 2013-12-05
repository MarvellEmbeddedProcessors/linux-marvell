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
#include "pp2/prs/mvPp2PrsHw.h"
#include "pp2/prs/mvPp2Prs.h"


static  MV_PP2_PRS_ENTRY pe;


static ssize_t mv_prs_low_help(char *buf)
{
	int off = 0;

	off += scnprintf(buf + off, PAGE_SIZE - off, "cat          sw_dump       - dump parser SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat          hw_dump       - dump all valid HW entries\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat          hw_regs       - dump parser registers.\n");
#ifdef CONFIG_MV_ETH_PP2_1
	off += scnprintf(buf + off, PAGE_SIZE - off, "cat          hw_hits       - dump non zeroed hit counters and the associated HW entries\n");
#endif
	off += scnprintf(buf + off, PAGE_SIZE - off, "\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "echo id      > hw_write    - write parser SW entry into HW place <id>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo id      > hw_read     - read parser entry <id> into SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo 1       > sw_clear    - clear parser SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo id      > hw_inv      - invalidate parser entry <id> in hw.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo         > hw_inv_all  - invalidate all parser entries in HW.\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p m     > t_port      - add<m=1> or delete<m=0> port<p> in SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo pmap    > t_port_map  - set port map <pmap> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v m     > t_ai        - update ainfo value <v> with mask <m> in SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo o d m   > t_byte      - set byte of data <d> with mask <m> and offset <o> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v       > t_lu        - set lookup id <v> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v m     > s_ri        - set result info value <v> with mask <m> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v m     > s_ai        - set ainfo value <v> with mask <m> to sw entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v       > s_next_lu   - set next lookup id value <v> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v       > s_shift     - set packet shift value <v> for next lookup to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo t v     > s_offs      - set offset value <v> for type <t> to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v       > s_lu_done   - set (v=1) or clear (v=0) lookup done bit to SW entry.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "echo v       > s_fid_gen   - set (v=1) or clear (v=0) flowid generate bit in SW entry.\n");

	off += scnprintf(buf + off, PAGE_SIZE - off, "echo p l m o > hw_frst_itr - set values for first iteration port <p>, lookupid <l>, \n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "                             max loops <m>, init offs <o>.\n");
	off += scnprintf(buf + off, PAGE_SIZE - off, "\n");

	return off;
}


static ssize_t mv_prs_low_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;
	if (!strcmp(name, "hw_dump"))
		mvPp2PrsHwDump();
	else if (!strcmp(name, "sw_dump"))
		mvPp2PrsSwDump(&pe);
	else if (!strcmp(name, "hw_regs"))
		mvPp2PrsHwRegsDump();
	else if (!strcmp(name, "hw_hits"))
		mvPp2V1PrsHwHitsDump();
	else
		off += mv_prs_low_help(buf);

	return off;
}

static ssize_t mv_prs_low_store_signed(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	int  err = 0, a = 0, b = 0, c = 0, d = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%d %d %d %d", &a, &b, &c, &d);
	local_irq_save(flags);

	if (!strcmp(name, "s_shift"))
		mvPp2PrsSwSramShiftSet(&pe, a, SRAM_OP_SEL_SHIFT_ADD);
	else if (!strcmp(name, "s_offs"))
		mvPp2PrsSwSramOffsetSet(&pe, a, b, SRAM_OP_SEL_SHIFT_ADD);
	else if (!strcmp(name, "hw_frst_itr"))
		mvPp2PrsHwPortInit(a, b, c, d);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static ssize_t mv_prs_low_store_unsigned(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char    *name = attr->attr.name;
	unsigned int  err = 0, a = 0, b = 0, c = 0;
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	sscanf(buf, "%x %x %x", &a, &b, &c);

	local_irq_save(flags);

	if (!strcmp(name, "hw_write")) {
		pe.index = a;
		mvPp2PrsHwWrite(&pe);
	} else if (!strcmp(name, "hw_read")) {
		pe.index = a;
		mvPp2PrsHwRead(&pe);
	} else if (!strcmp(name, "sw_clear"))
		mvPp2PrsSwClear(&pe);
	else if (!strcmp(name, "hw_inv"))
		mvPp2PrsHwInv(a);
	else if (!strcmp(name, "hw_inv_all"))
		mvPp2PrsHwInvAll();
	else if (!strcmp(name, "t_port"))
		mvPp2PrsSwTcamPortSet(&pe, a, b);
	else if (!strcmp(name, "t_port_map"))
		mvPp2PrsSwTcamPortMapSet(&pe, a);
	else if (!strcmp(name, "t_lu"))
		mvPp2PrsSwTcamLuSet(&pe, a);
	else if (!strcmp(name, "t_ai"))
		mvPp2PrsSwTcamAiUpdate(&pe, a, b);
	else if (!strcmp(name, "t_byte"))
		mvPp2PrsSwTcamByteSet(&pe, a, b, c);
	else if (!strcmp(name, "s_ri"))
		mvPp2PrsSwSramRiUpdate(&pe, a, b);
	else if (!strcmp(name, "s_ai"))
		mvPp2PrsSwSramAiUpdate(&pe, a, b);
	else if (!strcmp(name, "s_next_lu"))
		mvPp2PrsSwSramNextLuSet(&pe, a);
	else if (!strcmp(name, "s_lu_done"))
		(a == 1) ? mvPp2PrsSwSramLuDoneSet(&pe) : mvPp2PrsSwSramLuDoneClear(&pe);
	else if (!strcmp(name, "s_fid_gen"))
		(a == 1) ? mvPp2PrsSwSramFlowidGenSet(&pe) : mvPp2PrsSwSramFlowidGenClear(&pe);
	else {
		err = 1;
		printk(KERN_ERR "%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);

	if (err)
		printk(KERN_ERR "%s: <%s>, error %d\n", __func__, attr->attr.name, err);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(hw_dump,		S_IRUSR, mv_prs_low_show, NULL);
static DEVICE_ATTR(sw_dump,		S_IRUSR, mv_prs_low_show, NULL);
static DEVICE_ATTR(help,		S_IRUSR, mv_prs_low_show, NULL);
static DEVICE_ATTR(hw_regs,		S_IRUSR, mv_prs_low_show, NULL);
static DEVICE_ATTR(hw_hits,		S_IRUSR, mv_prs_low_show, NULL);
static DEVICE_ATTR(sw_clear,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(hw_write,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(hw_read,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(hw_inv,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(hw_inv_all,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(t_byte,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(t_port,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(t_port_map,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(t_ai,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(t_lu,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(s_ri,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(s_ai,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(s_next_lu,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(s_shift,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_signed);
static DEVICE_ATTR(s_offs,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_signed);
static DEVICE_ATTR(s_lu_done,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(s_fid_gen,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_unsigned);
static DEVICE_ATTR(hw_frst_itr,		S_IWUSR, mv_prs_low_show, mv_prs_low_store_signed);



static struct attribute *prs_low_attrs[] = {
	&dev_attr_hw_dump.attr,
	&dev_attr_sw_dump.attr,
	&dev_attr_hw_hits.attr,
	&dev_attr_hw_regs.attr,
	&dev_attr_hw_write.attr,
	&dev_attr_hw_read.attr,
	&dev_attr_hw_inv.attr,
	&dev_attr_hw_inv_all.attr,
	&dev_attr_sw_clear.attr,
	&dev_attr_t_byte.attr,
	&dev_attr_t_port.attr,
	&dev_attr_t_port_map.attr,
	&dev_attr_t_ai.attr,
	&dev_attr_t_lu.attr,
	&dev_attr_s_ri.attr,
	&dev_attr_s_ai.attr,
	&dev_attr_s_next_lu.attr,
	&dev_attr_s_shift.attr,
	&dev_attr_s_offs.attr,
	&dev_attr_s_lu_done.attr,
	&dev_attr_s_fid_gen.attr,
	&dev_attr_hw_frst_itr.attr,
	&dev_attr_help.attr,
	NULL
};

static struct attribute_group prs_low_group = {
	.name = "debug",
	.attrs = prs_low_attrs,
};

int mv_pp2_prs_low_sysfs_init(struct kobject *pp2_kobj)
{
	int err;

	err = sysfs_create_group(pp2_kobj, &prs_low_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", prs_low_group.name, err);

	return err;
}

int mv_pp2_prs_low_sysfs_exit(struct kobject *pp2_kobj)
{
	sysfs_remove_group(pp2_kobj, &prs_low_group);

	return 0;
}

