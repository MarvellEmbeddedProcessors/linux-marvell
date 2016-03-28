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
#include "hmac/mv_hmac.h"

static ssize_t mv_hmac_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f]         > f_regs      - dump global unit frame registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [q]     > rxq_regs    - dump frame RX queue registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [q]     > txq_regs    - dump frame TX queue registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [q] [m] > rxq_show    - show RX queue status\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [q] [m] > txq_show    - show TX queue status\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [q] [c] > txq_cap     - change TX queue capacity in datagrams\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [u]         > reg_read    - read global unit register\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [u] [v]     > reg_write   - write global unit register\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [u]     > f_reg_read  - read frame unit register\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [f] [u] [v] > f_reg_write - write frame unit register\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [v]         > debug       - 0 disable, bit0 enable read, bit1 enable write debug outputs\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [f] frame number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [q] queue number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [u] hex register address\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [v] hex value\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [c] max number of datagrams\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [m] mode: 0 - brief (status only) display, 1 - full (status + dump) display\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t mv_hmac_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* const char      *name = attr->attr.name; */
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_hmac_help(buf);

	return off;
}

static ssize_t mv_hmac_3_hex_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, u, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read first 3 parameters */
	err = p = u = v = 0;
	sscanf(buf, "%x %x %x", &p, &u, &v);

	local_irq_save(flags);

	if (!strcmp(name, "reg_write"))
		mv_pp3_hmac_gl_reg_write(p, u);
	else if (!strcmp(name, "reg_read")) {
		v = mv_pp3_hmac_gl_reg_read(p);
		pr_info("0x%x = 0x%x\n", p, v);
	} else if (!strcmp(name, "f_reg_write"))
		mv_pp3_hmac_frame_reg_write(p, u, v);
	else if (!strcmp(name, "f_reg_read")) {
		v = mv_pp3_hmac_frame_reg_read(p, u);
		pr_info("0x%x = 0x%x\n", u, v);
	} else if (!strcmp(name, "debug"))
		mv_pp3_hmac_debug_cfg(p);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static ssize_t mv_hmac_3_dec_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    p, u, v;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read first 3 parameters */
	err = p = u = v = 0;
	sscanf(buf, "%d %d %d", &p, &u, &v);

	local_irq_save(flags);

	if (!strcmp(name, "f_regs")) {
		mv_pp3_hmac_global_regs_dump();
		mv_pp3_hmac_frame_regs_dump(p);
	} else if (!strcmp(name, "rxq_regs"))
		mv_pp3_hmac_rxq_regs_dump(p, u);
	else if (!strcmp(name, "txq_regs"))
		mv_pp3_hmac_txq_regs_dump(p, u);
	else if (!strcmp(name, "txq_cap")) {
		if (mv_pp3_hmac_txq_capacity_cfg(p, u, v) != 0)
			pr_err("Bad queue capacity value %d. Must be large than 8 and less than queue size\n", v);
	} else if (!strcmp(name, "rxq_show")) {
		mv_pp3_hmac_rx_queue_show(p, u);
		if (v > 0) {
			bool print_all;

			print_all = (v == 2) ? true : false;
			mv_pp3_hmac_rx_queue_dump(p, u, print_all);
		}
	} else if (!strcmp(name, "txq_show")) {
		mv_pp3_hmac_tx_queue_show(p, u);
		if (v > 0) {
			bool print_all;

			print_all = (v == 2) ? true : false;
			mv_pp3_hmac_tx_queue_dump(p, u, print_all);
		}
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help, S_IRUSR, mv_hmac_show, NULL);
static DEVICE_ATTR(f_regs, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(rxq_regs, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(txq_regs, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(txq_cap, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(rxq_show, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(txq_show, S_IWUSR, NULL, mv_hmac_3_dec_store);
static DEVICE_ATTR(reg_write, S_IWUSR, NULL, mv_hmac_3_hex_store);
static DEVICE_ATTR(reg_read, S_IWUSR, NULL, mv_hmac_3_hex_store);
static DEVICE_ATTR(f_reg_write, S_IWUSR, NULL, mv_hmac_3_hex_store);
static DEVICE_ATTR(f_reg_read, S_IWUSR, NULL, mv_hmac_3_hex_store);
static DEVICE_ATTR(debug, S_IWUSR, NULL, mv_hmac_3_hex_store);


static struct attribute *mv_hmac_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_f_regs.attr,
	&dev_attr_rxq_regs.attr,
	&dev_attr_txq_regs.attr,
	&dev_attr_txq_cap.attr,
	&dev_attr_rxq_show.attr,
	&dev_attr_txq_show.attr,
	&dev_attr_reg_write.attr,
	&dev_attr_reg_read.attr,
	&dev_attr_f_reg_write.attr,
	&dev_attr_f_reg_read.attr,
	&dev_attr_debug.attr,
	NULL
};

static struct attribute_group mv_hmac_group = {
	.name = "hmac",
	.attrs = mv_hmac_attrs,
};

int mv_pp3_hmac_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_hmac_group);
	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_hmac_sysfs_exit(struct kobject *hmac_kobj)
{
	sysfs_remove_group(hmac_kobj, &mv_hmac_group);
	return 0;
}
