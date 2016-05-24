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
#include "common/mv_sw_if.h"
#include "gop/mv_gop_if.h"
#include "gop/mv_ptp_if.h"
#include "gop/mv_tai_regs.h"
#include "gop/pcs/mv_xpcs_if.h"
#include "gop/serdes/mv_serdes_if.h"
#ifdef CONFIG_ARCH_MVEBU
#include "net_complex/mv_net_complex_a39x.h"
#include "gop/mac/mv_xlg_mac_if.h"
#endif

static ssize_t mv_gop_help(char *b)
{
	int o = 0;

	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"cd                     ptp         - go to PTP and TAI units configuration sub directory\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [p]             > status      - show GOP port configuration\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [p]             > regs        - show GOP port registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [lane]          > xpcs_regs   - show XPCS lane registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [lane]          > serdes_regs - show Serdes lane registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [p]             > mib_cntrs   - show port MIB counters\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [p]             > clear_cntrs - clear port MIB counters\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [u] [v]         > reg_write   - write register: address [u], value [v]\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [u]             > reg_read    - read register: address [u]\n");
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [u] [s:e] [v]   > reg_modify  - read, modify, write gop register\n");
#ifdef CONFIG_ARCH_MVEBU
	o += scnprintf(b + o, PAGE_SIZE - o,
		"echo [p] [mode]      > port_mode   - change mode of lanes 6,5 connected to port [p]\n");
#endif
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "parameters:\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "      [p]	- mac number\n");
#ifdef CONFIG_ARCH_MVEBU
	o += scnprintf(b + o, PAGE_SIZE - o, "      [mode]	- rxaui:    switch to RXAUI\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "		- sgmii:    switch to SGMII\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "		- sgmii2_5: switch to SGMII 2.5\n");
#endif
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");

	return o;
}

static ssize_t mv_gop_ptp_help(char *b)
{
	int o = 0;
	/* NOTE: the sysfs-show limited with PAGE_SIZE. Current help-size is about 1.3kB */
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "cat              tai_regs  - show TAI unit registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "cat              tai_tod   - show TAI time capture values\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "cat              tai_clock - show TAI clock status\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [p]       > ptp_regs  - show PTP unit registers\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [p] [0/1] > ptp_en    - enable(1) / disable(0) PTP unit\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [p]       > ptp_reset - reset given port PTP unit\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     [p] - mac (port) number\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "----\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [h] [l] [n] > tai_tod_load_value  - set TAI TOD with DECimal\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "         [h] hig16bit sec, [l] low32bit sec, [n] - nanosec\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");

	o += scnprintf(b + o, PAGE_SIZE - o, "--- TAI TOD operationS (HEX parameters)---\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "echo [o] [h] [l] [n] > tai_op\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "         [h] high sec, [l] low sec, [n] nanosec (HEX)\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     [o] OPeration (HEX all parameters)\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "   ToD time:      [h]=0 must be present\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     1c -increment[l+n], 1c0 -graceful inc[l+n]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     dc -decrement[l+n], 1d0 -graceful dec[l+n]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "   FREQ:\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     F1c / Fdc - inc/dec by value [h]\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "   SYNC ToD time from/to linux or Sys/kernel:\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     41 - from linux, 21 - to linux\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     45 - from Sys/kernel, 47,46 -print ToD and System time\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "   Tai-Clock cfg:\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     CE1 - Clock External Increment [h] seconds\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     CED - Clock External Decrement [h] seconds\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     CEA - Clock External Absolute set [h] seconds\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     CEC - Clock External Check stability & counter\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     C1  - Clock Internal (free-running)\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     C0  - Clock Off\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     CEB11 - Blink led on gpio=11\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "   DEBUG:\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "     deb h l n  - DEBug-op with up to 3 parameters\n");
	o += scnprintf(b + o, PAGE_SIZE - o, "\n");

	return o;
}

static ssize_t mv_gop_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		off = mv_gop_help(buf);
	else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t mv_gop_ptp_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help_ptp"))
		off = mv_gop_ptp_help(buf);
	else if (!strcmp(name, "tai_regs"))
		mv_pp3_tai_reg_dump();
	else if (!strcmp(name, "tai_tod"))
		mv_pp3_tai_tod_dump();
	else if (!strcmp(name, "tai_clock"))
		off = mv_pp3_tai_clock_status_get_sysfs(buf);
	else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t mv_gop_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, u, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "reg_modify")) {
		u32	ret;
		u32 start_bit, end_bit, mask;

		ret = sscanf(buf, "%x %d:%d %x", &p, &start_bit, &end_bit, &u);

		if ((end_bit < start_bit) || ((end_bit - start_bit + 1) > 32))
			return -EINVAL;

		if ((end_bit - start_bit + 1) == 32)
			mask = 0xFFFFFFFF;
		else
			mask = (1 << (end_bit - start_bit + 1)) - 1;

		local_irq_save(flags);

		v = mv_gop_reg_read(p);
		MV_U32_SET_FIELD(v, (mask << start_bit), ((u & mask) << start_bit));
		mv_gop_reg_write(p, v);

		local_irq_restore(flags);
		return len;
	}

	/* TAI TOD commands: tai_tod_load_value, tai_op */
	if (!strcmp(name, "tai_tod_load_value")) {
		u32	ret;
		u32 h, l, n;

		n = 0;
		ret = sscanf(buf, "%d %d %d", &h, &l, &n);
		if (ret < 2)
			return -EINVAL;
		/* Case (!op & !h && !l && !n) is valid to set TAI-TOD ZERO */
		local_irq_save(flags);
		mv_pp3_tai_tod_load_set(h, l, n, 0);
		local_irq_restore(flags);
		return len;
	}
	if (!strcmp(name, "tai_op")) {
		u32	ret;
		u32 h, l, n, op;

		h = l = n = 0;
		ret = sscanf(buf, "%x %x %x %x", &op, &h, &l, &n);
		if (ret < 1)
			return -EINVAL;
		if (!op)
			return -EINVAL; /* likely wrong OP */
		local_irq_save(flags);
		mv_pp3_tai_tod_load_set(h, l, n, op);
		local_irq_restore(flags);
		return len;
	}

#ifdef CONFIG_ARCH_MVEBU
	if (!strcmp(name, "port_mode")) {
		char	str_mode[10];
		u32	lane_mode; /* mode for SERDES 6, 5 */
		u32	ret;
		int	mac_num;
		enum mv_port_mode mode;

		ret = sscanf(buf, "%d %9s", &mac_num, str_mode);
		if (ret != 2)
			return -EINVAL;

		local_irq_save(flags);

		if (!strcmp(str_mode, "rxaui")) {
			pr_info("\nSwitch to RXAUI\n");
			mode = MV_PORT_RXAUI;
			lane_mode = MV_NETCOMP_GE_MAC0_2_RXAUI;
		} else {
			if (mac_num == 0)
				lane_mode = MV_NETCOMP_GE_MAC0_2_SGMII_L6;
			else if (mac_num == 3)
				lane_mode = MV_NETCOMP_GE_MAC3_2_SGMII_L6;
			else
				return -EINVAL;

			if (!strcmp(str_mode, "sgmii")) {
				pr_info("\nSwitch to SGMII\n");
				mode = MV_PORT_SGMII;
			} else if (!strcmp(str_mode, "sgmii2_5")) {
				pr_info("\nSwitch to SGMII 2.5G\n");
				mode = MV_PORT_SGMII2_5;
			} else
				return -EINVAL;
		}

		mv_pp3_gop_port_reset(mac_num);
		mv_net_complex_dynamic_init(lane_mode);
		mv_pp3_gop_port_init(mac_num, mode);

		local_irq_restore(flags);
		return len;
	}
#endif /* CONFIG_ARCH_MVEBU */

	/* Read port and value */
	err = p = u = v = 0;
	sscanf(buf, "%x %x %x", &p, &u, &v);

	local_irq_save(flags);

	if (!strcmp(name, "regs"))
		mv_pp3_gop_port_regs(p);
	else if (!strcmp(name, "status"))
		mv_pp3_gop_status_show(p);
	else if (!strcmp(name, "mib_cntrs"))
		mv_pp3_gop_mib_counters_show(p);
	else if (!strcmp(name, "clear_cntrs"))
		mv_pp3_gop_mib_counters_clear(p);
	else if (!strcmp(name, "ptp_en"))
		mv_pp3_ptp_enable(p, (u) ? true : false);
	else if (!strcmp(name, "ptp_reset"))
		mv_pp3_ptp_reset(p);
	else if (!strcmp(name, "ptp_regs"))
		mv_pp3_ptp_reg_dump(p);
	else if (!strcmp(name, "xpcs_regs")) {
		mv_xpcs_gl_regs_dump();
		mv_xpcs_lane_regs_dump(p);
	} else if (!strcmp(name, "serdes_regs"))
		mv_serdes_lane_regs_dump(p);
	else if (!strcmp(name, "reg_write"))
		mv_gop_reg_write(p, u);
	else if (!strcmp(name, "reg_read")) {
		v = mv_gop_reg_read(p);
		pr_info("0x%x = 0x%x\n", p, v);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, mv_gop_show, NULL);
static DEVICE_ATTR(regs,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(xpcs_regs,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(serdes_regs,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(reg_write,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(reg_read,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(reg_modify,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(status,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(mib_cntrs,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(clear_cntrs,		S_IWUSR, NULL, mv_gop_3_hex_store);
#ifdef CONFIG_ARCH_MVEBU
static DEVICE_ATTR(port_mode,	S_IWUSR, NULL, mv_gop_3_hex_store);
#endif

static struct attribute *mv_gop_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_serdes_regs.attr,
	&dev_attr_regs.attr,
	&dev_attr_xpcs_regs.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_reg_modify.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_status.attr,
	&dev_attr_mib_cntrs.attr,
	&dev_attr_clear_cntrs.attr,
#ifdef CONFIG_ARCH_MVEBU
	&dev_attr_port_mode.attr,
#endif
	NULL
};

static DEVICE_ATTR(help_ptp,		S_IRUSR, mv_gop_ptp_show, NULL);
static DEVICE_ATTR(tai_regs,		S_IRUSR, mv_gop_ptp_show, NULL);
static DEVICE_ATTR(tai_clock,		S_IRUSR, mv_gop_ptp_show, NULL);
static DEVICE_ATTR(tai_tod,		S_IRUSR, mv_gop_ptp_show, NULL);
static DEVICE_ATTR(tai_tod_load_value,	S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(tai_op,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(ptp_regs,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(ptp_en,		S_IWUSR, NULL, mv_gop_3_hex_store);
static DEVICE_ATTR(ptp_reset,		S_IWUSR, NULL, mv_gop_3_hex_store);

static struct attribute *mv_gop_ptp_attrs[] = {
	&dev_attr_help_ptp.attr,
	&dev_attr_tai_regs.attr,
	&dev_attr_tai_clock.attr,
	&dev_attr_tai_tod.attr,
	&dev_attr_tai_tod_load_value.attr,
	&dev_attr_tai_op.attr,
	&dev_attr_ptp_regs.attr,
	&dev_attr_ptp_en.attr,
	&dev_attr_ptp_reset.attr,
	NULL
};

static struct attribute_group mv_gop_group = {
	.attrs = mv_gop_attrs,
};

static struct attribute_group mv_gop_ptp_group = {
	.name = "ptp",
	.attrs = mv_gop_ptp_attrs,
};


int mv_pp3_gop_sysfs_init(struct kobject *pp3_kobj)
{
	int err;
	struct kobject *dev_kobj;

	dev_kobj = kobject_create_and_add("gop", pp3_kobj);
	if (!dev_kobj) {
		pr_err("%s: cannot create gop kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(dev_kobj, &mv_gop_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", mv_gop_group.name, err);
		return err;
	}
	err = sysfs_create_group(dev_kobj, &mv_gop_ptp_group);
	if (err) {
		pr_err("sysfs group failed for dev debug%d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_gop_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &mv_gop_group);

	return 0;
}

