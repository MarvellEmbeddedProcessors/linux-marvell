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
#include "emac/mv_emac.h"

static ssize_t mv_emac_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > regs        - show EMAC port registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > pfc_regs    - show EMAC port PFC registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > config      - show EMAC port configuration\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > cntrs       - show EMAC port counters\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > clear_cntrs - clear EMAC port counters\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [0|1]   > rx_en       - enable/disable RX on EMAC port [p]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [0|1]   > debug       - enable/disable debug outputs registers read/write\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u] [v] > qm_map_set  - set QM mapping, [u] QM port, [v] QM queue\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u] [v] > reg_write   - write register: emac [p], offset [u], value [v]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u]     > reg_read    - read register: emac [p], offset [u]\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [p]  - emac number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t mv_emac_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	/* const char      *name = attr->attr.name; */
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_emac_help(buf);

	return off;
}

static ssize_t mv_emac_3_hex_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, u, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = p = u = v = 0;
	sscanf(buf, "%x %x %x", &p, &u, &v);

	local_irq_save(flags);

	if (!strcmp(name, "regs"))
		mv_pp3_emac_regs(p);
	else if (!strcmp(name, "pfc_regs"))
		mv_pp3_emac_pfc_regs(p);
	else if (!strcmp(name, "config"))
		mv_pp3_emac_status(p);
	else if (!strcmp(name, "cntrs"))
		mv_pp3_emac_counters_show(p);
	else if (!strcmp(name, "clear_cntrs"))
		mv_pp3_emac_counters_clear(p);
	else if (!strcmp(name, "reg_write"))
		mv_pp3_emac_reg_write(p, u, v);
	else if (!strcmp(name, "reg_read"))
		mv_pp3_emac_reg_read(p, u);
	else if (!strcmp(name, "debug"))
		mv_pp3_emac_debug(p, u);
	else if (!strcmp(name, "qm_map_set"))
		mv_pp3_emac_qm_mapping(p, u, v);
	else if (!strcmp(name, "rx_en"))
		mv_pp3_emac_rx_enable(p, u);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help, S_IRUSR, mv_emac_show, NULL);
static DEVICE_ATTR(regs, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(pfc_regs, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(config, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(cntrs, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(clear_cntrs, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(reg_write, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(reg_read, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(debug, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(qm_map_set, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(rx_en, S_IWUSR, NULL, mv_emac_3_hex_store);


static struct attribute *mv_emac_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_pfc_regs.attr,
	&dev_attr_config.attr,
	&dev_attr_cntrs.attr,
	&dev_attr_clear_cntrs.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_debug.attr,
	&dev_attr_qm_map_set.attr,
	&dev_attr_rx_en.attr,
	NULL
};

static struct attribute_group mv_emac_group = {
	.name = "emac",
	.attrs = mv_emac_attrs,
};


int mv_pp3_emac_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_emac_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", mv_emac_group.name, err);
		return err;
	}

	return err;
}

int mv_pp3_emac_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &mv_emac_group);

	return 0;
}
