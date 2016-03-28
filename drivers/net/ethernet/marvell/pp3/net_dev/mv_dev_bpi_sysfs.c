/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#include "mv_netdev.h"
#include "mv_dev_vq.h"


static ssize_t pp3_dev_bpi_help(char *b)
{
	int o = 0;
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname]                     > tx_bpi_show   - Show BPI configuration for network interface\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [emac]         > tx_vq_emac    - Set egress virtual queue to send packets to [emac]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq] [on] [off]     > tx_bpi_thresh - Set x[on] and x[off] BPI thresholds for egress packets\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [ifname] [vq]                > tx_vq_del     - Delete egress virtual queue BPI configuration\n");

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters: all values are decimal\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "    [ifname]    - network interface name e.g. nic0, nic1, nss0\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "    [emac]      - EMAC number\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "    [vq]        - virtual queue number in range of [0..16]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "    [on] [off]  - internal back pressure Xon and Xoff thresholds in units of bytes\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "    [l]         - BPI level: 0 - emac2hmac, 1 - cmac2hmac,  2 - cmac2cmac   3 - emac2cmac\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "                  level 0 valid only for VQs that send packets to EMAC directly\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "                  level 1-3 valid only for VQss that send packets to EMAC via CMAC\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");

	return o;
}

static ssize_t pp3_dev_bpi_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help")) {
		off = pp3_dev_bpi_help(buf);

	} else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
	return off;
}


static ssize_t pp3_dev_bpi_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    vq, a, b, c;
	unsigned long   flags;
	char		if_name[10];
	struct net_device *netdev;
	struct	pp3_dev_priv *dev_priv;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = a = b = c = vq = 0;

	if (sscanf(buf, "%s %d %d %d %d", if_name, &vq, &a, &b, &c) <= 0) {
		err = 1;
		goto exit;
	}

	netdev = dev_get_by_name(&init_net, if_name);

	if (!netdev) {
		pr_err("%s: illegal interface <%s>\n", __func__, if_name);
		return -EINVAL;
	}

	dev_priv = MV_PP3_PRIV(netdev);

	local_irq_save(flags);

	if (!strcmp(name, "tx_vq_emac"))
		err = mv_pp3_egress_vq_emac_set(netdev, vq, a);
	else if (!strcmp(name, "tx_bpi_thresh"))
		err = mv_pp3_egress_vq_bpi_thresh_set(netdev, vq, a, b);
	else if (!strcmp(name, "tx_vq_del"))
		pr_info("not supported yet\n");
	else if (!strcmp(name, "tx_bpi_show"))
		mv_pp3_egress_bpi_dump(netdev);
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

static DEVICE_ATTR(help,		S_IRUSR, pp3_dev_bpi_show, NULL);
static DEVICE_ATTR(tx_bpi_show,		S_IWUSR, NULL, pp3_dev_bpi_store);
static DEVICE_ATTR(tx_vq_emac,		S_IWUSR, NULL, pp3_dev_bpi_store);
static DEVICE_ATTR(tx_bpi_thresh,	S_IWUSR, NULL, pp3_dev_bpi_store);
static DEVICE_ATTR(tx_vq_del,		S_IWUSR, NULL, pp3_dev_bpi_store);


static struct attribute *pp3_dev_bpi_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_tx_bpi_show.attr,
	&dev_attr_tx_vq_emac.attr,
	&dev_attr_tx_bpi_thresh.attr,
	&dev_attr_tx_vq_del.attr,
	NULL
};


static struct attribute_group pp3_dev_bpi_group = {
	.name = "bpi",
	.attrs = pp3_dev_bpi_attrs,
};

int mv_pp3_dev_bpi_sysfs_init(struct kobject *dev_kobj)
{
	int err;

	err = sysfs_create_group(dev_kobj, &pp3_dev_bpi_group);

	if (err)
		pr_err("sysfs group failed for bpi path\n");

	return err;
}

int mv_pp3_dev_bpi_sysfs_exit(struct kobject *dev_kobj)
{
	sysfs_remove_group(dev_kobj, &pp3_dev_bpi_group);

	return 0;
}
