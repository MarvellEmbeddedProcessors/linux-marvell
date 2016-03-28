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
#include "common/mv_sw_if.h"
#include "qm/mv_qm.h"
#include "qm/mv_qm_regs.h"


#define PR_ERR_CODE(_rc)	\
{							\
	pr_err("%s: operation failed. probably wrong input (rc=%d)\n", __func__, _rc);	\
}

#define PR_INFO_CALLED		\
{							\
	pr_info("%s is called\n", attr->attr.name);	\
}

static ssize_t mv_qm_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "cat                                  errors_regs    - Print error registers\n");
	o += scnprintf(b+o, s-o, "cat                                  global_regs    - Print global registers\n");
	o += scnprintf(b+o, s-o, "cat                                  nempty_queues  - Print length of non-empty queues\n");
	o += scnprintf(b+o, s-o, "cat                                  bpi_groups     - Print all valids (xon < xoff) internal back pressure groups\n");
	o += scnprintf(b+o, s-o, "echo [q]                           > queue_regs     - Print Q registers\n");
	o += scnprintf(b+o, s-o, "echo [p]                           > dqf_port_regs  - Print dequeue fifo registers\n");
	o += scnprintf(b+o, s-o, "echo [pr]                          > profile_show   - Print profile configuration parameters\n");
	o += scnprintf(b+o, s-o, "echo [pr] [low] [pause] [high]     > profile_set    - Set profile parameters\n");
	o += scnprintf(b+o, s-o, "echo [gr]                          > bpi_show       - Print internal back pressure group\n");
	o += scnprintf(b+o, s-o, "echo [gr] [low] [high] [node] [id] > bpi_set        - Set internal back pressure group parameters\n");

	o += scnprintf(b+o, s-o, "\n");
	o += scnprintf(b+o, s-o, "parameters: [p]     port\n");
	o += scnprintf(b+o, s-o, "            [pr]    profile 0:emac0, 1:emac1, 2:emac2: 3:emac3 6:hamc\n");
	o += scnprintf(b+o, s-o, "            [q]     queue\n");
	o += scnprintf(b+o, s-o, "            [low]   low threshold in KB\n");
	o += scnprintf(b+o, s-o, "            [pause] pause threshold in KB\n");
	o += scnprintf(b+o, s-o, "            [high]  high threshold in KB\n");
	o += scnprintf(b+o, s-o, "            [gr]    internal back pressure group\n");
	o += scnprintf(b+o, s-o, "            [node]  1: A node, 2: B node, 3: C node\n");
	o += scnprintf(b+o, s-o, "            [id]    node id\n");


	o += scnprintf(b+o, s-o, "\n");

	return o;
}

static ssize_t mv_qm_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help")) {
		off = mv_qm_help(buf);
	} else if (!strcmp(name, "errors_dump")) {
		qm_errors_dump();
	} else if (!strcmp(name, "global_dump")) {
		qm_global_dump();
	} else if (!strcmp(name, "nempty_queues")) {
		qm_nempty_queue_len_dump();
	} else if (!strcmp(name, "bpi_groups")) {
		qm_ql_group_bpi_show_all();
	} else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t mv_qm_config(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int a, b, c, d, e, err;

	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input parameters */
	err = a = b = c = d = e = 0;

	if (sscanf(buf, "%d %d %d %d %d", &a, &b, &c, &d, &e) <= 0) {
		err = 1;
		goto exit;
	}

	local_irq_save(flags);

	if (!strcmp(name, "queue_dump")) {
		qm_queue_dump(a);
	} else if (!strcmp(name, "dqf_port_regs")) {
		qm_dqf_port_dump(a);
	} else if (!strcmp(name, "profile_show")) {
		qm_ql_profile_show(a);
	} else if (!strcmp(name, "profile_set")) {
		qm_ql_profile_cfg(a, b, c, d);
	} else if (!strcmp(name, "bpi_show")) {
		qm_ql_group_bpi_show(a);
	} else if (!strcmp(name, "bpi_set")) {
		qm_ql_group_bpi_set(a, b, c, d, e);
	} else if (!strcmp(name, "debug")) {
		qm_dbg_flags(QM_F_DBG_RD, a & 0x1);
		qm_dbg_flags(QM_F_DBG_WR, a & 0x2);
	} else {
		err = 1;
		pr_err("%s: wrong name of QM function <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);
exit:
	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,                      S_IRUSR, mv_qm_show, NULL);
static DEVICE_ATTR(errors_dump,               S_IRUSR, mv_qm_show, NULL);
static DEVICE_ATTR(global_dump,               S_IRUSR, mv_qm_show, NULL);
static DEVICE_ATTR(nempty_queues,             S_IRUSR, mv_qm_show, NULL);
static DEVICE_ATTR(bpi_groups,                S_IRUSR, mv_qm_show, NULL);
static DEVICE_ATTR(queue_dump,                S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(dqf_port_regs,             S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(profile_show,              S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(profile_set,               S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(bpi_show,                  S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(bpi_set,                   S_IWUSR, NULL, mv_qm_config);
static DEVICE_ATTR(debug,                   S_IWUSR, NULL, mv_qm_config);

static struct attribute *mv_qm_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_errors_dump.attr,
	&dev_attr_global_dump.attr,
	&dev_attr_queue_dump.attr,
	&dev_attr_nempty_queues.attr,
	&dev_attr_bpi_groups.attr,
	&dev_attr_dqf_port_regs.attr,
	&dev_attr_profile_show.attr,
	&dev_attr_profile_set.attr,
	&dev_attr_bpi_show.attr,
	&dev_attr_bpi_set.attr,
	&dev_attr_debug.attr,

	NULL
};

static struct attribute_group mv_qm_group = {
	.attrs = mv_qm_attrs,
};

int mv_pp3_qm_sysfs_init(struct kobject *pp3_kobj)
{
	int err;
	struct kobject *qm_kobj;

	qm_kobj = kobject_create_and_add("qm", pp3_kobj);
	if (!qm_kobj) {
		printk(KERN_ERR"%s: cannot create qm kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(qm_kobj, &mv_qm_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for qm%d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_qm_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &mv_qm_group);
	return 0;
}
