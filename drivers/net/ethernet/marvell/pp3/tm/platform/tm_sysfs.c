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

#include "tm_sysfs_debug.h"
#include "tm_sysfs_drop.h"
#include "tm_sysfs_shaping.h"
#include "mv_tm_drop.h"
#include "mv_tm_sched.h"
#include "mv_tm_shaping.h"
#include "mv_tm_scheme.h"

#define PR_ERR_CODE(_rc)	\
{							\
	pr_err("%s: operation failed (rc=%d)\n", __func__, _rc);	\
}

#define PR_INFO_CALLED		\
{							\
	pr_info("%s is called\n", attr->attr.name);	\
}

static struct qmtm **hndl;

static ssize_t mv_tm_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "cd                         debug               - move to TM Debug sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                         drop                - move to TM Drop sysfs directory\n");
	o += scnprintf(b+o, s-o, "cd                         shaping             - move to TM Shaping sysfs directory\n");
	o += scnprintf(b+o, s-o, "echo [sc]                > config              - create default tree configuration\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]             > node_show           - show node's parameters (SW DB)\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]             > node_show_hw        - show node's parameters (HW)\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [pr]        > prio_set            - set node's priority\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]             > prio_set_propagated - set node's propagated priority\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [w]         > dwrr_weight         - set node's DWRR weight\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [pr] [0|1]  > dwrr_enable         - enable/disable DWRR for the priority on node\n");
	o += scnprintf(b+o, s-o, "echo [0|1]               > dequeue             - enable/disable DeQ tree status\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "parameters:\n");
	o += scnprintf(b+o, s-o, "        [l]       - level: 4-Port, 3-C level, 2-B level, 1-A level, 0-Queue\n");
	o += scnprintf(b+o, s-o, "        [i]       - index\n");
	o += scnprintf(b+o, s-o, "        [sc]      - scenario: 0 - defcon, 1 - cfg1, 2 - 2xPPC, 3 - cfg3 tree\n");
	o += scnprintf(b+o, s-o, "        [w]       - DWRR weight [in 256 bytes units]\n");
	o += scnprintf(b+o, s-o, "        [pr]      - priority [0-7]\n");
	o += scnprintf(b+o, s-o, "\n");

	return o;
}


static ssize_t mv_tm_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		off = mv_tm_help(buf);
	else if (!strcmp(name, "debug")) {
		pr_info("debug\n");
	} else if (!strcmp(name, "drop")) {
		pr_info("drop\n");
	} else if (!strcmp(name, "shaping")) {
		pr_info("shaping\n");
	} else
		off = mv_tm_help(buf);

	return off;
}


static ssize_t mv_tm_config(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned long   flags = 0;

	u32 level = 0;
	u32 index = 0;
	u32 en = 0;
	u32 prio = 0;
	int fields;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	local_irq_save(flags);

	if (!strcmp(name, "config")) {
		u32 sc = 0;
		fields = sscanf(buf, "%u", &sc);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		if (sc == 0) {
			pr_info("Configure default tree scenario\n");
			err = tm_defcon();
		} else if (sc == 1) {
			pr_info("Configure cfg1 scenario\n");
			err = tm_cfg1();
		} else if (sc == 2) {
			pr_info("Configure 2xPPC scenario\n");
			err = tm_2xppc();
		} else if (sc == 3) {
			pr_info("Configure cfg3 scenario\n");
			err = tm_cfg3_tree();
		} else {
			err = 1;
			pr_info("unknown configuration.\n");
		}
		if (err)
			PR_ERR_CODE(err)
		else
			pr_info("Configuration completed successfuly\n");
	} else if (!strcmp(name, "node_show")) {
		int parent;

		fields = sscanf(buf, "%d %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_node_show(%d, %u) is called\n",
				level,
				index);
		err = tm_sysfs_read_node(level, (u16)index);
		if (err)
			PR_ERR_CODE(err)

		if (!mv_tm_scheme_parent_node_get(level, index, &parent))
			pr_info("Parent node is: %s #%d\n", tm_sysfs_level_str(level + 1), parent);
	} else if (!strcmp(name, "node_show_hw")) {
		fields = sscanf(buf, "%d %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_node_show_hw(%d, %u) is called\n",
				level,
				index);
		err = tm_sysfs_read_node_hw(level, (u16)index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "prio_set")) {
		fields = sscanf(buf, "%d %u %u", &level, &index, &prio);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_prio_set(%d, %u, %u) is called\n",
				level,
				index,
				prio);
		err = mv_tm_prio_set(level, (u16)index, prio);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "prio_set_propagated")) {
		fields = sscanf(buf, "%d %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_prio_set_propagated(%d, %u) is called\n",
				level,
				index);
		err = mv_tm_prio_set_propagated(level, (u16)index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "dwrr_weight")) {
		u32 quantum = 0;
		fields = sscanf(buf, "%d %u %u", &level, &index, &quantum);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("dwrr_weight(%d %u %u) is called\n",
				level, index, quantum);
		err = mv_tm_dwrr_weight(level, (u16)index, quantum);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "dwrr_enable")) {
		fields = sscanf(buf, "%d %u %u %u", &level, &index, &prio, &en);
		err = (fields != 4) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("dwrr_enable(%d %u %u %u) is called\n",
				level, index, prio, en);
		err = mv_tm_dwrr_enable(level, (u16)index, (u8)prio, (u8)en);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "dequeue")) {
		fields = sscanf(buf, "%u", &en);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_dequeue( %u) is called\n", en);
		err = mv_tm_tree_status_set((u8)en);
		if (err)
			PR_ERR_CODE(err)
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

/* SYSFS initialization */
static DEVICE_ATTR(help,			S_IRUSR, mv_tm_show, NULL);
static DEVICE_ATTR(config,			S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(node_show,			S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(node_show_hw,		S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(prio_set,			S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(prio_set_propagated,		S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(dwrr_weight,			S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(dwrr_enable,			S_IWUSR, NULL, mv_tm_config);
static DEVICE_ATTR(dequeue,			S_IWUSR, NULL, mv_tm_config);

static struct attribute *mv_tm_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_config.attr,
	&dev_attr_node_show.attr,
	&dev_attr_node_show_hw.attr,
	&dev_attr_prio_set.attr,
	&dev_attr_prio_set_propagated.attr,
	&dev_attr_dwrr_weight.attr,
	&dev_attr_dwrr_enable.attr,
	&dev_attr_dequeue.attr,
	NULL
};

static struct attribute_group mv_tm_group = {
	.attrs = mv_tm_attrs,
};


/*********/
/* Debug */
/*********/
static ssize_t mv_tm_debug_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "cat                      help_debug           - show this help\n");
	o += scnprintf(b+o, s-o, "cat                      open                 - initiate TM module\n");
	o += scnprintf(b+o, s-o, "cat                      close                - close TM module\n");
	o += scnprintf(b+o, s-o, "cat                      ports_name           - print tm ports name\n");
#if 0 /* TBD */
	o += scnprintf(b+o, s-o, "cat                      drop_errors          - dump drop errors\n");
#endif
	o += scnprintf(b+o, s-o, "echo [0|1]             > enable_debug         - enable debug printing\n");
	o += scnprintf(b+o, s-o, "echo [a] [l32] [h32]   > write_tm_reg         - write [l32] [h32] to tm reg [a]\n");
	o += scnprintf(b+o, s-o, "echo [a]               > read_tm_reg          - read tm reg [a]\n");
	o += scnprintf(b+o, s-o, "echo [a] [cnt] [int]   > dump_tm_regs         - dump tm regs\n");
	o += scnprintf(b+o, s-o, "echo [i]               > dump_port_hw         - print tm tree under port from HW\n");
	o += scnprintf(b+o, s-o, "echo [timeout]         > trace_queues         - trace packet path in tm (Qs only)\n");
	o += scnprintf(b+o, s-o, "echo [timeout]         > trace_path           - trace packet path in tm\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]           > dump_elig_func       - print eligible function value\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [elig]    > set_elig             - set node's eligible function\n");
	o += scnprintf(b+o, s-o, "echo [q1] [q2] [elig]  > set_elig_per_q_range - set eligible function per queues range\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "parameters:\n");
	o += scnprintf(b+o, s-o, "        [l]       - level: 4-Port, 3-C level, 2-B level, 1-A level, 0-Queue\n");
	o += scnprintf(b+o, s-o, "        [i]       - index\n");
	o += scnprintf(b+o, s-o, "        [a]       - register address\n");
	o += scnprintf(b+o, s-o, "        [l32]     - register value - low 32 bits\n");
	o += scnprintf(b+o, s-o, "        [h32]     - register value - high 32 bits\n");
	o += scnprintf(b+o, s-o, "        [cnt]     - count\n");
	o += scnprintf(b+o, s-o, "        [int]     - intervals\n");
	o += scnprintf(b+o, s-o, "        [timeout] - timeout in seconds\n");
	o += scnprintf(b+o, s-o, "        [elig]    - eligible priority function [0-63]\n");
	o += scnprintf(b+o, s-o, "\n");

	return o;
}


static ssize_t mv_tm_debug_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help_debug"))
		off = mv_tm_debug_help(buf);
	else if (!strcmp(name, "open")) {
		PR_INFO_CALLED
		off = tm_open();
		if (off != MV_OK)
			PR_ERR_CODE(off)
		else
			pr_info("TM open completed successfuly, handle is (0x%X)\n", (u32)hndl);
	} else if (!strcmp(name, "close")) {
		PR_INFO_CALLED
		off = tm_close();
		if (off != MV_OK)
			PR_ERR_CODE(off)
		else
			pr_info("TM close completed successfuly\n");
	} else if (!strcmp(name, "ports_name")) {
		off = tm_sysfs_print_ports_name();
		if (off != MV_OK)
			PR_ERR_CODE(off)
#if 0 /* TBD */
	} else if (!strcmp(name, "drop_errors")) {
		struct tm_error_info info;
		off = tm_drop_get_errors(hndl, &info);
		if (off != MV_OK)
			PR_ERR_CODE(off)
		else {
			pr_info("Drop Errors:\n");
			pr_info("    error counter		= %d\n", info.error_counter);
			pr_info("    exception counter  = %d\n", info.exception_counter);
		}
#endif
	} else if (!strcmp(name, "help"))
		off = mv_tm_debug_help(buf);
	else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}


static ssize_t mv_tm_debug_config(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned long   flags = 0;

	int l, i;
	u32 en = 0;
	u32 reg_addr = 0;
	u32 reg_val[2] = {0, 0};
	u32 index = 0;
	u32 elig = 0;
	u32 timeout = 5;
	int fields;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	local_irq_save(flags);

	if (!strcmp(name, "enable_debug")) {
		fields = sscanf(buf, "%u", &en);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		err = tm_sysfs_enable_debug(en);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "write_tm_reg")) {
		fields = sscanf(buf, "%x %x %x", &reg_addr, &reg_val[0], &reg_val[1]);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		mv_pp3_hw_write(reg_addr + mv_pp3_nss_regs_vaddr_get(), 2, reg_val);
	} else if (!strcmp(name, "read_tm_reg")) {
		fields = sscanf(buf, "%x", &reg_addr);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		mv_pp3_hw_read(reg_addr + mv_pp3_nss_regs_vaddr_get(), 2, reg_val);
		for (i = 0; i < 2; i++)
			pr_info("0x%x = 0x%08x\n", reg_addr+i*4, reg_val[i]);
	} else if (!strcmp(name, "dump_tm_regs")) {
		u32 cnt = 1, interval = 1;
		fields = sscanf(buf, "%x %u %u", &reg_addr, &cnt, &interval);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("Dumping TM regs: Reg: 0x%x, Count: %u, Interval: %u\n", reg_addr, cnt, interval);
		for (i = 0; i < cnt; i++) {
			mv_pp3_hw_read(reg_addr + mv_pp3_nss_regs_vaddr_get() , 2, reg_val);
			pr_info("0x%x = 0x%08x\n", reg_addr+0*4, reg_val[0]);
			pr_info("0x%x = 0x%08x\n", reg_addr+1*4, reg_val[1]);
			reg_addr += 8 * interval;
		}
	} else if (!strcmp(name, "dump_port_hw")) {
		fields = sscanf(buf, "%u", &index);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_sysfs_dump_port_hw(%u) is called\n", index);
		err = tm_sysfs_dump_port_hw(index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "trace_queues")) {
		fields = sscanf(buf, "%u", &timeout);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		err = tm_sysfs_trace_queues(timeout, 0);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "trace_path")) {
		fields = sscanf(buf, "%u", &timeout);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		err = tm_sysfs_trace_queues(timeout, 1);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "dump_elig_func")) {
		fields = sscanf(buf, "%u %u", &l, &i);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		err = tm_sysfs_show_elig_func(l, i);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "set_elig")) {
		fields = sscanf(buf, "%d %u %u", &l, &index, &elig);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_set_elig(%d, %u, %u) is called\n",
				l,
				index,
				elig);
		err = tm_sysfs_set_elig(l, (u16)index, elig);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "set_elig_per_q_range")) {
		u32 start;
		u32 end;
		fields = sscanf(buf, "%d %u %u", &start, &end, &elig);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		err = tm_sysfs_set_elig_per_queue_range(start, end, elig);
		if (err)
			PR_ERR_CODE(err)
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help_debug,				S_IRUSR, mv_tm_debug_show, NULL);
static DEVICE_ATTR(open,					S_IRUSR, mv_tm_debug_show, NULL);
static DEVICE_ATTR(close,					S_IRUSR, mv_tm_debug_show, NULL);
static DEVICE_ATTR(ports_name,				S_IRUSR, mv_tm_debug_show, NULL);
#if 0 /* TBD */
static DEVICE_ATTR(drop_errors,				S_IRUSR, mv_tm_debug_show, NULL);
#endif
static DEVICE_ATTR(enable_debug,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(write_tm_reg,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(read_tm_reg,				S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(dump_tm_regs,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(dump_port_hw,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(trace_queues,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(trace_path,				S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(dump_elig_func,			S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(set_elig,				S_IWUSR, NULL, mv_tm_debug_config);
static DEVICE_ATTR(set_elig_per_q_range,	S_IWUSR, NULL, mv_tm_debug_config);

static struct attribute *mv_tm_debug_attrs[] = {
	&dev_attr_help_debug.attr,
	&dev_attr_open.attr,
	&dev_attr_close.attr,
	&dev_attr_ports_name.attr,
#if 0 /* TBD */
	&dev_attr_drop_errors.attr,
#endif
	&dev_attr_enable_debug.attr,
	&dev_attr_write_tm_reg.attr,
	&dev_attr_read_tm_reg.attr,
	&dev_attr_dump_tm_regs.attr,
	&dev_attr_dump_port_hw.attr,
	&dev_attr_trace_queues.attr,
	&dev_attr_trace_path.attr,
	&dev_attr_dump_elig_func.attr,
	&dev_attr_set_elig.attr,
	&dev_attr_set_elig_per_q_range.attr,
	NULL
};

static struct attribute_group mv_tm_debug_group = {
	.name = "debug",
	.attrs = mv_tm_debug_attrs,
};


/********/
/* Drop */
/********/
static ssize_t mv_tm_drop_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "cat                          help_drop         - show this help\n");
	o += scnprintf(b+o, s-o, "cat                          profiles          - show all existing Drop profiles\n");
	o += scnprintf(b+o, s-o, "cat                          curves            - show all existing WRED curves\n");
	o += scnprintf(b+o, s-o, "cat                          params_show       - show configured parameters\n");

	o += scnprintf(b+o, s-o, "echo [thr]                 > cbtd_thr_set      - set CBTD threshold\n");
	o += scnprintf(b+o, s-o, "echo [clr][thr]            > catd_thr_set      - set CATD threshold\n");
	o += scnprintf(b+o, s-o, "echo [clr][min][max]       > wred_thr_set      - set WRED thresholds\n");
	o += scnprintf(b+o, s-o, "echo [clr][cur][sc]        > wred_curve_set    - set WRED curve index and scaling\n");

	o += scnprintf(b+o, s-o, "echo [l][i][cos]           > profile_set       - update Drop Profile params to HW\n");
	o += scnprintf(b+o, s-o, "echo [l][i][cos]           > profile_clear     - set Drop Profile to default\n");
	o += scnprintf(b+o, s-o, "echo [l][i][cos]           > profile_show      - show Drop profile's parameters\n");

	/* Configuration by BW */
	o += scnprintf(b+o, s-o, "echo [l][i][cos][cb][wred] > wred_profile_set  - update Drop Profile (CBTD & WRED)\n");
	o += scnprintf(b+o, s-o, "echo [l][i][cos][cb][ca]   > catd_profile_set  - update Drop Profile (CBTD & CATD)\n");
	o += scnprintf(b+o, s-o, "echo [l][i][cos]           > profile_bw_show   - show Drop profile's parameters (in BW)\n");

	o += scnprintf(b+o, s-o, "echo [l][cos][mp]          > wred_curve_create - create traditional WRED curve\n");
	o += scnprintf(b+o, s-o, "echo [l][1|2|3]            > color_num_set     - set number of colors\n");
	o += scnprintf(b+o, s-o, "echo [i][cos]              > queue_cos_set     - queue cos select\n");
	o += scnprintf(b+o, s-o, "echo [l][i][cos][dp]       > profile_ptr_set   - update Node's pointer to Drop Profile\n");
#ifdef MV_QMTM_NSS_A0
	o += scnprintf(b+o, s-o, "echo [l][c][s]             > dp_source_set\n");
	o += scnprintf(b+o, s-o, "echo [l][dp]               > set_drop_query_responce\n");
#endif
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "parameters:\n");
	o += scnprintf(b+o, s-o, "        [l]      - level: 4-Port, 3-C level, 2-B level, 1-A level, 0-Queue\n");
	o += scnprintf(b+o, s-o, "        [i]      - index\n");
	o += scnprintf(b+o, s-o, "        [cos]    - cos [0-7, '-1' - if not relevant]\n");
	o += scnprintf(b+o, s-o, "        [clr]    - color: 0-Green, 1-Yellow, 2-Red, 7-All\n");
	o += scnprintf(b+o, s-o, "        [cur]    - curve index\n");
	o += scnprintf(b+o, s-o, "        [sc]     - curve scaling\n");
	o += scnprintf(b+o, s-o, "        [thr]    - TD threshold [burst = 16 bytes]\n");
	o += scnprintf(b+o, s-o, "        [min]    - min threshold [burst = 16 bytes]\n");
	o += scnprintf(b+o, s-o, "        [max]    - max threshold [burst = 16 bytes]\n");
	o += scnprintf(b+o, s-o, "        [cb]     - color blind TD bw [Kbps]\n");
	o += scnprintf(b+o, s-o, "        [ca]     - color aware TD bw [Kbps]\n");
	o += scnprintf(b+o, s-o, "        [wred]   - WRED bw [Kbps]\n");
	o += scnprintf(b+o, s-o, "        [mp]     - maximum probability [1-100]\n");
	o += scnprintf(b+o, s-o, "        [dp]     - drop profile index\n");

	o += scnprintf(b+o, s-o, "\n");

	return o;
}


static ssize_t mv_tm_drop_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help_drop"))
		off = mv_tm_drop_help(buf);
	else if (!strcmp(name, "profiles")) {
		PR_INFO_CALLED
		off = tm_sysfs_read_drop_profiles();
		if (off != MV_OK)
			PR_ERR_CODE(off)
	} else if (!strcmp(name, "curves")) {
		PR_INFO_CALLED
		off = tm_sysfs_read_wred_curves();
		if (off != MV_OK)
			PR_ERR_CODE(off)
	} else if (!strcmp(name, "params_show")) {
		PR_INFO_CALLED
		off = tm_sysfs_params_show();
		if (off != MV_OK)
			PR_ERR_CODE(off)
	} else if (!strcmp(name, "help"))
		off = mv_tm_debug_help(buf);
	else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}


static ssize_t mv_tm_drop_config(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned long   flags = 0;

	u32 level;
	u32 index;
	int cos;
	int color = 0;
	u32 threshold = 0;
	u32 cb_bw = 0;
	u32 ca_wred_bw = 0;
	int fields;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	local_irq_save(flags);

	if (!strcmp(name, "cbtd_thr_set")) {
		fields = sscanf(buf, "%u", &threshold);
		err = (fields != 1) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_cbtd_thr_set(%u) is called\n",
				threshold);
		err = tm_sysfs_cbtd_thr_set(threshold);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "catd_thr_set")) {
		fields = sscanf(buf, "%d %u", &color, &threshold);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_catd_thr_set(%d, %u) is called\n",
				color,
				threshold);
		err = tm_sysfs_catd_thr_set(color, threshold);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "wred_thr_set")) {
		u32 min_thr = 0;
		u32 max_thr = 0;
		fields = sscanf(buf, "%d %u %u", &color, &min_thr, &max_thr);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_wred_thr_set(%d, %u, %u) is called\n",
				color,
				min_thr,
				max_thr);
		err = tm_sysfs_wred_thr_set(color, min_thr, max_thr);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "wred_curve_set")) {
		u32 scale = 0;
		fields = sscanf(buf, "%d %u %u", &color, &index, &scale);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_wred_curve_set(%d, %u, %u) is called\n",
				color,
				index,
				scale);
		err = tm_sysfs_wred_curve_set(color, index, scale);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "profile_set")) {
		fields = sscanf(buf, "%u %u %d", &level, &index, &cos);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_profile_set(%u, %u, %d) is called\n",
				level,
				index,
				cos);
		err = tm_sysfs_drop_profile_set(level, (u16)index, cos);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "profile_clear")) {
		fields = sscanf(buf, "%u %u %d", &level, &index, &cos);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_profile_clear(%u, %u, %d) is called\n",
				level,
				index,
				cos);
		err = mv_tm_drop_profile_clear(level, index, cos);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "profile_show")) {
		fields = sscanf(buf, "%u %u %d", &level, &index, &cos);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_profile_show(%u, %u, %d) is called\n",
			level,
			index,
			cos);
		err = tm_sysfs_read_drop_profile(level, cos, (u16)index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "wred_profile_set")) {
		fields = sscanf(buf, "%u %u %d %u %u", &level, &index, &cos, &cb_bw, &ca_wred_bw);
		err = (fields != 5) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_wred_profile_set(%u, %u, %d, %u, %u) is called\n",
				level,
				index,
				cos,
				cb_bw,
				ca_wred_bw);
		err = mv_tm_update_drop_profile_wred(level, cos, index, cb_bw, ca_wred_bw);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "catd_profile_set")) {
		fields = sscanf(buf, "%u %u %d %u %u", &level, &index, &cos, &cb_bw, &ca_wred_bw);
		err = (fields != 5) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_catd_profile_set(%u, %u, %d, %u, %u) is called\n",
				level,
				index,
				cos,
				cb_bw,
				ca_wred_bw);
		err = mv_tm_update_drop_profile_catd(level, cos, index, cb_bw, ca_wred_bw);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "profile_bw_show")) {
		fields = sscanf(buf, "%u %u %d", &level, &index, &cos);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("profile_bw_show(%u, %u, %d) is called\n",
			level,
			index,
			cos);
		err = tm_sysfs_read_drop_profile_bw(level, cos, (u16)index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "wred_curve_create")) {
		u32 mp;
		fields = sscanf(buf, "%u %d %u", &level, &cos, &mp);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_create_wred_curve (%u %d %u) is called\n",
			level,
			cos,
			mp);
		err = mv_tm_create_wred_curve(level, cos, mp, (uint8_t *)&index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "color_num_set")) {
		fields = sscanf(buf, "%u %d", &level, &color);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_color_num_set(%u, %d) is called\n",
				level,
				color);
		err = mv_tm_color_num_set(level, color);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "queue_cos_set")) {
		fields = sscanf(buf, "%u %d", &index, &cos);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_queue_cos_set(%u, %d) is called\n",
			index,
			cos);
		err = mv_tm_queue_cos_set(index, cos);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "profile_ptr_set")) {
		u32 dp = 0;
		fields = sscanf(buf, "%u %u %d %u", &level, &index, &cos, &dp);
		err = (fields != 4) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("tm_profile_ptr_set(%u, %u, %d, %u) is called\n",
			level,
			index,
			cos,
			dp);
		err = mv_tm_dp_set(level, index, cos, dp);
		if (err)
			PR_ERR_CODE(err)
#if 0
	} else if (!strcmp(name, "dp_source_set")) {
		/* Read pool and value */
		sscanf(buf, "%d %d %d", &level, &color, &source);
		rc = tm_dp_source_set(hndl, level, color, source);
		if (rc)
			PR_ERR_CODE(rc)
		else
			pr_info("set_drop_color_num: Level %d color %d source %d\n", level, color, source);
	}
	else if (!strcmp(name, "set_drop_query_responce")) {
		/* Read pool and value */
		sscanf(buf, "%d %d", &level, &port_dp);
		rc = tm_set_drop_query_responce(hndl, port_dp, level);
		if (rc)
			PR_ERR_CODE(rc)
		else
			pr_info("set_drop_query_responce: Level %d port_dp %d\n", level, port_dp);
#endif
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help_drop,			S_IRUSR, mv_tm_drop_show, NULL);
static DEVICE_ATTR(profiles,			S_IRUSR, mv_tm_drop_show, NULL);
static DEVICE_ATTR(curves,				S_IRUSR, mv_tm_drop_show, NULL);
static DEVICE_ATTR(params_show,			S_IRUSR, mv_tm_drop_show, NULL);
static DEVICE_ATTR(cbtd_thr_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(catd_thr_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(wred_thr_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(wred_curve_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(profile_set,			S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(profile_clear,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(profile_show,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(wred_profile_set,	S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(catd_profile_set,	S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(profile_bw_show,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(wred_curve_create,	S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(color_num_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(queue_cos_set,		S_IWUSR, NULL, mv_tm_drop_config);
static DEVICE_ATTR(profile_ptr_set,		S_IWUSR, NULL, mv_tm_drop_config);

static struct attribute *mv_tm_drop_attrs[] = {
	&dev_attr_help_drop.attr,
	&dev_attr_profiles.attr,
	&dev_attr_curves.attr,
	&dev_attr_params_show.attr,
	&dev_attr_cbtd_thr_set.attr,
	&dev_attr_catd_thr_set.attr,
	&dev_attr_wred_thr_set.attr,
	&dev_attr_wred_curve_set.attr,
	&dev_attr_profile_set.attr,
	&dev_attr_profile_clear.attr,
	&dev_attr_profile_show.attr,
	&dev_attr_wred_profile_set.attr,
	&dev_attr_catd_profile_set.attr,
	&dev_attr_profile_bw_show.attr,
	&dev_attr_wred_curve_create.attr,
	&dev_attr_color_num_set.attr,
	&dev_attr_queue_cos_set.attr,
	&dev_attr_profile_ptr_set.attr,
	NULL
};

static struct attribute_group mv_tm_drop_group = {
	.name = "drop",
	.attrs = mv_tm_drop_attrs,
};


/***********/
/* Shaping */
/***********/
static ssize_t mv_tm_shaping_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "cat                                  help_shaping    - show this help\n");
	o += scnprintf(b+o, s-o, "cat                                  shaping         - show all existing Shaping configurations\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [cir] [eir]             > set_shaping     - set shaping\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [cir]                   > set_min_shaping - set minimal shaping\n");
	o += scnprintf(b+o, s-o, "echo [l] [i] [cir] [eir] [cbs] [ebs] > set_shaping_ex  - set shaping including burst sizes\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]                         > set_no_shaping  - disables shaping on node\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]                         > show_shaping    - show shaping parameters (CIR & EIR only)\n");
	o += scnprintf(b+o, s-o, "echo [l] [i]                         > show_shaping_ex - show shaping parameters (bw & burst sizes)\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "parameters:\n");
	o += scnprintf(b+o, s-o, "        [l]    - level: 4-Port, 3-C level, 2-B level, 1-A level, 0-Queue\n");
	o += scnprintf(b+o, s-o, "        [i]    - index\n");
	o += scnprintf(b+o, s-o, "        [cir]  - committed shaping rate [in resolution of 1Mb, in steps of 10Mb]\n");
	o += scnprintf(b+o, s-o, "        [eir]  - excessive shaping rate [in resolution of 1Mb, in steps of 10Mb]\n");
	o += scnprintf(b+o, s-o, "        [cbs]  - committed burst size [in KBytes]\n");
	o += scnprintf(b+o, s-o, "        [ebs]  - excessive burst size [in KBytes]\n");
	o += scnprintf(b+o, s-o, "\n");

	return o;
}


static ssize_t mv_tm_shaping_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help_shaping"))
		off = mv_tm_shaping_help(buf);
	else if (!strcmp(name, "shaping")) {
		PR_INFO_CALLED
		off = tm_sysfs_read_shaping();
		if (off != MV_OK)
			PR_ERR_CODE(off)
	} else if (!strcmp(name, "help"))
		off = mv_tm_debug_help(buf);
	else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}


static ssize_t mv_tm_shaping_config(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err = 0;
	unsigned long   flags = 0;

	u32 level = 0;
	u32 index = 0;
	u32 cbw = 0;
	u32 ebw = 0;
	u32 cbs = 0;
	u32 ebs = 0;
	u8 elig_fun = 0;
	u8 mask = 0;

	int fields;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	local_irq_save(flags);
	if (!strcmp(name, "set_shaping_ex")) {
		fields = sscanf(buf, "%u %u %u %u %u %u", &level, &index, &cbw, &ebw, &cbs, &ebs);
		err = (fields != 6) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("set_shaping_ex( %u, %u, %u, %u %u %u) is called\n",
				level,
				index,
				cbw,
				ebw,
				cbs,
				ebs);
		err = mv_tm_set_shaping_ex((enum mv_tm_level)level, index, cbw, ebw, &cbs, &ebs);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "set_shaping")) {
		fields = sscanf(buf, "%u %u %u %u", &level, &index, &cbw, &ebw);
		err = (fields != 4) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("set_shaping( %u, %u, %u, %u) is called\n",
				level,
				index,
				cbw,
				ebw);
		err = mv_tm_set_shaping((enum mv_tm_level)level, index, cbw, ebw);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "set_min_shaping")) {
		fields = sscanf(buf, "%u %u %u", &level, &index, &cbw);
		err = (fields != 3) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("set_min_shaping( %u, %u, %u) is called\n",
				level,
				index,
				cbw);
		err = mv_tm_set_min_shaping((enum mv_tm_level)level, index, cbw);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "set_no_shaping")) {
		fields = sscanf(buf, "%u %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("set_no_shaping( %u, %u) is called\n",
				level,
				index);
		err = mv_tm_set_no_shaping((enum mv_tm_level)level, index);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "show_shaping")) {
		fields = sscanf(buf, "%u %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("show_shaping( %u, %u) is called\n",
				level,
				index);
		err = mv_tm_get_shaping((enum mv_tm_level)level, index, &cbw, &ebw);
		pr_info("cir:	%d\n", cbw);
		pr_info("eir:	%d\n", ebw);
		if (err)
			PR_ERR_CODE(err)
	} else if (!strcmp(name, "show_shaping_ex")) {
		fields = sscanf(buf, "%u %u", &level, &index);
		err = (fields != 2) ? 1 : 0;
		if (err)
			PR_ERR_CODE(err)
		pr_info("show_shaping_ex( %u, %u) is called\n",
				level,
				index);
		err = mv_tm_get_shaping_full_info((enum mv_tm_level)level, index, &elig_fun, &mask, &cbw, &ebw, &cbs, &ebs);
		pr_info("Eligible function  : %d\n", elig_fun);
		if (mask & 2)
			pr_info("  cir:	%8d(Mb/s)     cbs: %4d(KBytes)\n", cbw, cbs);
		else
			pr_info("  cir:	%8d(Mb/s)     cbs: %4d(KBytes)  (shaper not used!)\n", cbw, cbs);
		if (mask & 1)
			pr_info("  eir:	%8d(Mb/s)     ebs: %4d(KBytes)\n", ebw, ebs);
		else
			pr_info("  eir:	%8d(Mb/s)     ebs: %4d(KBytes)  (shaper not used!)\n", ebw, ebs);
		if (err)
			PR_ERR_CODE(err)
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}


static DEVICE_ATTR(help_shaping,			S_IRUSR, mv_tm_shaping_show, NULL);
static DEVICE_ATTR(shaping,					S_IRUSR, mv_tm_shaping_show, NULL);
static DEVICE_ATTR(set_shaping,				S_IWUSR, NULL, mv_tm_shaping_config);
static DEVICE_ATTR(set_min_shaping,			S_IWUSR, NULL, mv_tm_shaping_config);
static DEVICE_ATTR(set_shaping_ex,			S_IWUSR, NULL, mv_tm_shaping_config);
static DEVICE_ATTR(set_no_shaping,			S_IWUSR, NULL, mv_tm_shaping_config);
static DEVICE_ATTR(show_shaping,			S_IWUSR, NULL, mv_tm_shaping_config);
static DEVICE_ATTR(show_shaping_ex,			S_IWUSR, NULL, mv_tm_shaping_config);

static struct attribute *mv_tm_shaping_attrs[] = {
	&dev_attr_help_shaping.attr,
	&dev_attr_shaping.attr,
	&dev_attr_set_shaping.attr,
	&dev_attr_set_min_shaping.attr,
	&dev_attr_set_shaping_ex.attr,
	&dev_attr_set_no_shaping.attr,
	&dev_attr_show_shaping.attr,
	&dev_attr_show_shaping_ex.attr,
	NULL
};

static struct attribute_group mv_tm_shaping_group = {
	.name = "shaping",
	.attrs = mv_tm_shaping_attrs,
};


/*******************************/
/* SysFS Init & Exit functions */
/*******************************/
int mv_pp3_tm_sysfs_init(struct kobject *pp3_kobj)
{
	int err;
	struct kobject *tm_kobj;

	tm_kobj = kobject_create_and_add("tm", pp3_kobj);
	if (!tm_kobj) {
		pr_err(KERN_ERR"%s: cannot create tm kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(tm_kobj, &mv_tm_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for tm%d\n", err);
		return err;
	}

	err = sysfs_create_group(tm_kobj, &mv_tm_debug_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for tm debug%d\n", err);
		return err;
	}

	err = sysfs_create_group(tm_kobj, &mv_tm_drop_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for tm drop%d\n", err);
		return err;
	}

	err = sysfs_create_group(tm_kobj, &mv_tm_shaping_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for tm shaping%d\n", err);
		return err;
	}

	return err;
}


int mv_pp3_tm_sysfs_exit(struct kobject *tm_kobj)
{
	sysfs_remove_group(tm_kobj, &mv_tm_group);

	return 0;
}
