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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "fw/mv_fw.h"
#include "mv_pp3.h"


static ssize_t pp3_init_help(char *b)
{
	int o = 0;
	int p = PAGE_SIZE;

	o += sprintf(b+o, "\n");

	o += scnprintf(b+o, p-o, "cat          help      - show this help\n");
	o += scnprintf(b+o, p-o, "echo 1     > sys_init  - init FW and HW, run PPNs\n");
	o += scnprintf(b+o, p-o, "\n");

	return o;
}

static ssize_t pp3_init_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = pp3_init_help(buf);

	return off;
}

static ssize_t pp3_init_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err, fields;
	unsigned int    start;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = start = 0;

	if (!strcmp(name, "sys_init")) {
		fields = sscanf(buf, "%d", &start);
		if (fields == 1) {
			if (start)
				err = mv_pp3_shared_start(pp3_device);
		} else
			err = 1;
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__,
		       attr->attr.name);
	}
	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, pp3_init_show, NULL);
static DEVICE_ATTR(sys_init,		S_IWUSR, NULL, pp3_init_store);

static struct attribute *pp3_init_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_sys_init.attr,
	NULL
};

static struct attribute_group pp3_init_group = {
	.attrs = pp3_init_attrs,
};

static struct kobject *dev_kobj;

int mv_pp3_init_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	dev_kobj = kobject_create_and_add("init", pp3_kobj);
	if (!dev_kobj) {
		pr_err("%s: cannot create dev kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_update_group(dev_kobj, &pp3_init_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", pp3_init_group.name, err);
		return err;
	}

	return err;
}


int mv_pp3_init_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &pp3_init_group);

	return 0;
}


