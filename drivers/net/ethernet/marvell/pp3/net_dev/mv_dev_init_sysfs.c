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
#include "gnss/mv_pp3_gnss_api.h"
#include "mv_netdev.h"

static ssize_t pp3_dev_init_sysfs_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]         > dev_show       - show initialization time parameters for network interface\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [n]     > vq_rxqs_num    - set number of ingress VQs [n] for network interface\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [n]     > vq_txqs_num    - set number of egress VQs [n] for network interface\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [mask]  > rx_cpus        - set CPUs [mask] can RX on the network interface\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters:\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [ifname]    - network interface name e.g. nic0, nic1, nic2, etc\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "                  nss: any interface which name stared with nss e.g. nss16, nss17, etc\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [n]         - number of queues in decimal [0..16]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "      [mask]      - CPUs mask in decimal: bit0 = cpu0, bit1 = cpu1, etc\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t pp3_dev_init_sysfs_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	const char *name = attr->attr.name;
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "show"))
		pr_info("Not supported yet\n");
	else
		off = pp3_dev_init_sysfs_help(buf);

	return off;
}

static ssize_t pp3_dev_init_netdev_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    a;
	unsigned long   flags;
	char		if_name[10];
	struct net_device *netdev = NULL;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read interface name and value */
	err = a = 0;

	if (sscanf(buf, "%s %d", if_name, &a) <= 0) {
		err = 1;
		goto exit;
	}

	if (strcmp(if_name, "nss")) {
		netdev = dev_get_by_name(&init_net, if_name);
		if (!mv_pp3_dev_is_valid(netdev)) {
			pr_err("%s in not pp3 device\n", if_name);
			if (netdev)
				dev_put(netdev);

			return -EINVAL;
		}
	}

	local_irq_save(flags);

	if (!strcmp(name, "dev_show"))
		if (netdev)
			mv_pp3_dev_init_show(netdev);
		else
			mv_pp3_gnss_dev_init_show();
	else if (!strcmp(name, "vq_rxqs_num"))
		if (netdev)
			err = mv_pp3_dev_rxqs_set(netdev, a);
		else
			err = mv_pp3_gnss_dev_rxqs_set(a);
	else if (!strcmp(name, "vq_txqs_num"))
		if (netdev)
			err = mv_pp3_dev_txqs_set(netdev, a);
		else
			err = mv_pp3_gnss_dev_txqs_set(a);
	else if (!strcmp(name, "rx_cpus"))
		if (netdev)
			err = mv_pp3_dev_rx_cpus_set(netdev, a);
		else
			err = mv_pp3_gnss_dev_rx_cpus_set(a);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	local_irq_restore(flags);
	if (netdev)
		dev_put(netdev);
exit:
	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_init_sysfs_show, NULL);
static DEVICE_ATTR(dev_show,		S_IWUSR, NULL, pp3_dev_init_netdev_store);
static DEVICE_ATTR(vq_rxqs_num,		S_IWUSR, NULL, pp3_dev_init_netdev_store);
static DEVICE_ATTR(vq_txqs_num,		S_IWUSR, NULL, pp3_dev_init_netdev_store);
static DEVICE_ATTR(rx_cpus,		S_IWUSR, NULL, pp3_dev_init_netdev_store);

static struct attribute *pp3_dev_init_sysfs_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_dev_show.attr,
	&dev_attr_vq_rxqs_num.attr,
	&dev_attr_vq_txqs_num.attr,
	&dev_attr_rx_cpus.attr,
	NULL
};


static struct attribute_group pp3_dev_init_sysfs_group = {
	.name = "init",
	.attrs = pp3_dev_init_sysfs_attrs,
};

int mv_pp3_dev_init_sysfs_init(struct kobject *dev_kobj)
{
	int err;

	err = sysfs_create_group(dev_kobj, &pp3_dev_init_sysfs_group);

	if (err)
		pr_err("sysfs group failed for vq path\n");

	return err;
}

int mv_pp3_dev_init_sysfs_exit(struct kobject *dev_kobj)
{
	sysfs_remove_group(dev_kobj, &pp3_dev_init_sysfs_group);

	return 0;
}
