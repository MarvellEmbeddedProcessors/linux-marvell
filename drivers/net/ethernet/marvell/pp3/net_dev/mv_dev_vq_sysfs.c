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
#include "mv_netdev.h"
#include "mv_dev_vq.h"


static ssize_t pp3_dev_vq_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]                              > rx_cos_show  - show ingress CoS values to virtual queues mapping\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]                              > rx_vq_show   - show ingress virtual queues configuration for network interface\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [prio]                  > rx_vq_prio   - set scheduling priority [prio] for ingress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [weight]                > rx_vq_wrr    - set WRR [weight] for ingress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [length]                > rx_vq_length - set [length] in packets for ingress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [td] [red]              > rx_vq_drop   - set [td] and [red] thresholds for ingress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [cir] [eir] [cbs] [ebs] > rx_vq_limit  - set limit rates and burst sizes for ingress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [cos] [vq]                   > rx_cos_vq    - set ingress virtual queue [vq] for [cos] value\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]                              > tx_cos_show  - show egress CoS values to virtual queues mapping\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]                              > tx_vq_show   - show egress virtual queues configuration for network interface\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [prio]                  > tx_vq_prio   - set scheduling priority [prio] for egress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [weight]                > tx_vq_wrr    - set WRR weight [weight] for egress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [length]                > tx_vq_length - set [length] in packets for egress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [td] [red]              > tx_vq_drop   - set [td] and [red] thresholds for egress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [cir] [eir] [cbs] [ebs] > tx_vq_limit  - set limit rates and burst sizes for egress virtual queue [vq]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [cos] [vq]                   > tx_cos_vq    - set ingress virtual queue [vq] for [cos] value\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters: all values are decimal\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[ifname]    - network interface name e.g. nic0, nic1, nss0\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[vq]        - virtual queue number in range of [0..16]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[cos]       - class of service value in range of [0..16]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[length]    - virtual queue length in [pkts]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[td]        - tail drop threshold in [KBytes]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[red]       - random early drop threshold in [KBytes]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[prio]      - scheduling priority in range of [0..7]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[weight]    - WRR weight\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[cir]       - committed information rate in [Mbps] units, granularity of [10 Mbps]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[eir]       - excessive information rate in [Mbps] units, granularity of [10 Mbps]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[cbs]       - committed burst size in [KBytes]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "	[ebs]       - excessive burst size in [KBytes]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t pp3_dev_vq_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = pp3_dev_vq_help(buf);

	return off;
}


static ssize_t pp3_dev_vq_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    vq, a, b, c, d;
	unsigned long   flags;
	char		if_name[10];
	struct net_device *netdev;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = a = b = vq = 0;

	if (sscanf(buf, "%s %d %d %d %d %d", if_name, &vq, &a, &b, &c, &d) <= 0) {
		err = 1;
		goto exit;
	}

	netdev = dev_get_by_name(&init_net, if_name);
	if (!mv_pp3_dev_is_valid(netdev)) {
		pr_err("%s in not pp3 device\n", if_name);
		if (netdev)
			dev_put(netdev);
		return -EINVAL;
	}

	local_irq_save(flags);

	if (!strcmp(name, "rx_vq_show")) {
		pp3_dbg_ingress_vqs_print(netdev);
		pp3_dbg_ingress_vqs_show(netdev);
	} else if (!strcmp(name, "tx_vq_show")) {
		pp3_dbg_egress_vqs_print(netdev);
		pp3_dbg_egress_vqs_show(netdev);
	} else if (!strcmp(name, "rx_cos_show"))
		err = mv_pp3_dev_ingress_cos_show(netdev);
	else if (!strcmp(name, "tx_cos_show"))
		err = mv_pp3_dev_egress_cos_show(netdev);
	else if (!strcmp(name, "rx_vq_prio"))
		err = mv_pp3_dev_ingress_vq_prio_set(netdev, vq, a);
	else if (!strcmp(name, "rx_vq_wrr"))
		err = mv_pp3_dev_ingress_vq_weight_set(netdev, vq, a);
	else if (!strcmp(name, "rx_vq_drop")) {
		struct mv_nss_drop drop;

		drop.td = a;
		drop.red = b;
		drop.enable = true;
		err = mv_pp3_dev_ingress_vq_drop_set(netdev, vq, &drop);
	} else if (!strcmp(name, "rx_cos_vq"))
		err = mv_pp3_dev_ingress_cos_to_vq_set(netdev, vq, a);
	else if (!strcmp(name, "tx_vq_prio"))
		err = mv_pp3_dev_egress_vq_prio_set(netdev, vq, a);
	else if (!strcmp(name, "tx_vq_wrr"))
		err = mv_pp3_dev_egress_vq_weight_set(netdev, vq, a);
	else if (!strcmp(name, "tx_vq_drop")) {
		struct mv_nss_drop drop;

		drop.td = a;
		drop.red = b;
		drop.enable = true;
		err = mv_pp3_dev_egress_vq_drop_set(netdev, vq, &drop);
	} else if (!strcmp(name, "tx_cos_vq"))
		err = mv_pp3_dev_egress_cos_to_vq_set(netdev, vq, a);
#if 0
	else if (!strcmp(name, "rx_vq_limit"))
		err = mv_pp3_dev_ingress_vq_limit_set(netdev, vq, a, b, c, d);
#endif
	else if (!strcmp(name, "tx_vq_limit")) {
		struct mv_nss_meter meter;

		meter.cir = a;
		meter.eir = b;
		meter.cbs = c;
		meter.ebs = d;
		meter.enable = true;
		err = mv_pp3_dev_egress_vq_rate_limit_set(netdev, vq, &meter);
	} else if (!strcmp(name, "rx_vq_length"))
		err = mv_pp3_dev_ingress_vq_size_set(netdev, vq, a);
	else if (!strcmp(name, "tx_vq_length"))
		err = mv_pp3_dev_egress_vq_size_set(netdev, vq, a);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);
	dev_put(netdev);
exit:
	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_vq_show, NULL);
static DEVICE_ATTR(rx_cos_show,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_show,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_prio,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_wrr,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_drop,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_length,	S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_vq_limit,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(rx_cos_vq,		S_IWUSR, NULL, pp3_dev_vq_store);

static DEVICE_ATTR(tx_cos_show,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_show,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_prio,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_wrr,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_drop,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_length,	S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_vq_limit,		S_IWUSR, NULL, pp3_dev_vq_store);
static DEVICE_ATTR(tx_cos_vq,		S_IWUSR, NULL, pp3_dev_vq_store);


static struct attribute *pp3_dev_vq_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_rx_cos_show.attr,
	&dev_attr_rx_vq_show.attr,
	&dev_attr_rx_vq_prio.attr,
	&dev_attr_rx_vq_wrr.attr,
	&dev_attr_rx_vq_drop.attr,
	&dev_attr_rx_vq_length.attr,
	&dev_attr_rx_vq_limit.attr,
	&dev_attr_rx_cos_vq.attr,

	&dev_attr_tx_cos_show.attr,
	&dev_attr_tx_vq_show.attr,
	&dev_attr_tx_vq_prio.attr,
	&dev_attr_tx_vq_wrr.attr,
	&dev_attr_tx_vq_drop.attr,
	&dev_attr_tx_vq_length.attr,
	&dev_attr_tx_vq_limit.attr,
	&dev_attr_tx_cos_vq.attr,
	NULL
};


static struct attribute_group pp3_dev_vq_group = {
	.name = "vq",
	.attrs = pp3_dev_vq_attrs,
};

int mv_pp3_dev_vq_sysfs_init(struct kobject *dev_kobj)
{
	int err;

	err = sysfs_create_group(dev_kobj, &pp3_dev_vq_group);

	if (err)
		pr_err("sysfs group failed for vq path\n");

	return err;
}

int mv_pp3_dev_vq_sysfs_exit(struct kobject *dev_kobj)
{
	sysfs_remove_group(dev_kobj, &pp3_dev_vq_group);

	return 0;
}
