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
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_eth_sysfs.h"
#include "mv_netdev.h"

static ssize_t mv_eth_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "p, rxq, d                             - are dec numbers\n");
	o += scnprintf(b+o, s-o, "\n");

	o += scnprintf(b+o, s-o, "echo p rxq         > rxq_regs      - show RXQ registers for <p/rxq>\n");
	o += scnprintf(b+o, s-o, "echo p rxq d       > rxq           - show RXQ descriptors ring for <p/rxq>. d=0-brief, d=1-full\n");
	o += scnprintf(b+o, s-o, "echo p rxq d       > rxq_size      - set number of descriptors <d> for <port/rxq>.\n");
	o += scnprintf(b+o, s-o, "echo p d           > rx_weight     - set weight for the poll function; <d> - new weight, max val: 255\n");
	o += scnprintf(b+o, s-o, "echo p             > rx_reset      - reset RX part of the port <p>\n");
	o += scnprintf(b+o, s-o, "echo p rxq d       > rxq_pkts_coal - set RXQ interrupt coalesing. <d> - number of received packets\n");
	o += scnprintf(b+o, s-o, "echo p rxq d       > rxq_time_coal - set RXQ interrupt coalesing. <d> - time in microseconds\n");

	return o;
}

static ssize_t mv_eth_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_eth_help(buf);

	return off;
}

static ssize_t mv_eth_3_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char	*name = attr->attr.name;
	int		err;
	unsigned int	p, i, v;
	unsigned long	flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = p = i = v = 0;
	sscanf(buf, "%d %d %d", &p, &i, &v);

	local_irq_save(flags);

	if (!strcmp(name, "rxq_size"))
		err = mv_eth_ctrl_rxq_size_set(p, i, v);
	else if (!strcmp(name, "rxq_pkts_coal"))
		err = mv_eth_rx_pkts_coal_set(p, i, v);
	else if (!strcmp(name, "rxq_time_coal"))
		err = mv_eth_rx_time_coal_set(p, i, v);
	else if (!strcmp(name, "rxq"))
		mvNetaRxqShow(p, i, v);
	else if (!strcmp(name, "rxq_regs"))
		mvNetaRxqRegs(p, i);
	else if (!strcmp(name, "rx_reset"))
		err = mv_eth_rx_reset(p);
	else if (!strcmp(name, "rx_weight"))
		err = mv_eth_ctrl_set_poll_rx_weight(p, i);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,          S_IRUSR, mv_eth_show, NULL);
static DEVICE_ATTR(rxq_size,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rxq_pkts_coal, S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rxq_time_coal, S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rxq,           S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rxq_regs,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rx_reset,      S_IWUSR, NULL, mv_eth_3_store);
static DEVICE_ATTR(rx_weight,     S_IWUSR, NULL, mv_eth_3_store);


static struct attribute *mv_eth_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_rxq_size.attr,
	&dev_attr_rxq_pkts_coal.attr,
	&dev_attr_rxq_time_coal.attr,
	&dev_attr_rxq.attr,
	&dev_attr_rxq_regs.attr,
	&dev_attr_rx_reset.attr,
	&dev_attr_rx_weight.attr,
	NULL
};

static struct attribute_group mv_eth_rx_group = {
	.name = "rx",
	.attrs = mv_eth_attrs,
};

int mv_neta_rx_sysfs_init(struct kobject *gbe_kobj)
{
	int err;

	err = sysfs_create_group(gbe_kobj, &mv_eth_rx_group);
	if (err)
		pr_err("sysfs group %s failed %d\n", mv_eth_rx_group.name, err);

	return err;
}

int mv_neta_rx_sysfs_exit(struct kobject *gbe_kobj)
{
	sysfs_remove_group(gbe_kobj, &mv_eth_rx_group);

	return 0;
}
