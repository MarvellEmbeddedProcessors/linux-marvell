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

#include "net_complex/mv_net_complex_a39x.h"
#include "mv_pp3.h"


#define PP3_DEBUG_ARRAY_SIZE	512

static ssize_t pp3_debug_help(char *b)
{
	int o = 0;
	int p = PAGE_SIZE;

	o += sprintf(b+o, "\n");

	o += scnprintf(b+o, p-o, "cat                  reset_nss      - Reset NSS sub system\n");
	o += scnprintf(b+o, p-o, "echo [o] [v] [c]   > write_nss_reg  - Write value [v] to [c] NSS registers from offset [o]\n");
	o += scnprintf(b+o, p-o, "echo [o] [c]       > read_nss_reg   - Read [c] NSS registers from offset [o]\n");
	o += scnprintf(b+o, p-o, "echo [o] [s:e] [v] > modify_nss_reg - Write value [v] to NSS register offset [o] start from bit [s] to bit [e]\n");
	o += scnprintf(b+o, p-o, "echo [a] [v] [c]   > write_u32      - Write value [v] to [c] virtual address [a]\n");
	o += scnprintf(b+o, p-o, "echo [a] [c]       > read_u32       - Read [c] values from virtual address [a]\n");
	o += scnprintf(b+o, p-o, "echo [a] [s:e] [v] > modify_u32     - Write value [v] to virtual address [a] start from bit [s] to bit [e]\n");
	o += scnprintf(b+o, p-o, "echo [a] [v] [c]   > write_u32_le   - Write LE value [v] to [c] virtual addresses start from [a]\n");
	o += scnprintf(b+o, p-o, "echo [a] [c]       > read_u32_le    - Read [c] LE values start from virtual address [a]\n");
	o += scnprintf(b+o, p-o, "echo [a] [s:e] [v] > modify_u32_le  - Write LE value [v] to virtual address [a] start from bit [s] to bit [e]\n");
	o += scnprintf(b+o, p-o, "parameters:\n");
	o += scnprintf(b+o, p-o, "      [c] number of registers for read/write; must be < 0x80\n");
	o += scnprintf(b+o, p-o, "      All inputs in hex.");
	o += scnprintf(b+o, p-o, "\n");

	return o;
}

static ssize_t pp3_debug_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help")) {
		off = pp3_debug_help(buf);
	} else if (!strcmp(name, "reset_nss")) {
		pr_err("%s operation not supported yet\n", attr->attr.name);
		/*if (mv_pp3_nss_drain(NULL))
			off = -EINVAL;
		else
			mv_nss_sw_reset();*/
	} else {
		off = -EINVAL;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t pp3_hex_debug_modify(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	unsigned long   flags;
	u32 p, u, start_bit, end_bit, val;
	u32 ret, mask;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	ret = sscanf(buf, "%x %d:%d %x", &p, &start_bit, &end_bit, &u);
	if (ret < 4)
		return -EINVAL;

	if ((end_bit < start_bit) || ((end_bit - start_bit + 1) > 32))
		return -EINVAL;

	if ((end_bit - start_bit + 1) == 32)
		mask = 0xFFFFFFFF;
	else
		mask = (1 << (end_bit - start_bit + 1)) - 1;

	local_irq_save(flags);
	if (!strcmp(name, "modify_nss_reg")) {

		val = mv_pp3_hw_reg_read(p + mv_pp3_nss_regs_vaddr_get());
		MV_U32_SET_FIELD(val, (mask << start_bit), ((u & mask) << start_bit));
		mv_pp3_hw_reg_write(p + mv_pp3_nss_regs_vaddr_get(), val);

	} else if (!strcmp(name, "modify_u32_le")) {

		val = mv_pp3_hw_reg_read((void __iomem *)p);
		MV_U32_SET_FIELD(val, (mask << start_bit), ((u & mask) << start_bit));
		mv_pp3_hw_reg_write((void __iomem *)p, val);

	} else if (!strcmp(name, "modify_u32")) {

		val = *(u32 *)p;
		MV_U32_SET_FIELD(val, (mask << start_bit), ((u & mask) << start_bit));
		*(u32 *)p = val;
	}

	local_irq_restore(flags);

	return len;
}

static ssize_t pp3_hex_debug_store(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    a, b, c;
	unsigned long   flags;
	u32             *arr;
	int i, ret;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read port and value */
	err = a = b = c = 0;
	ret = sscanf(buf, "%x %x %x", &a, &b, &c);

	local_irq_save(flags);

	arr = kzalloc(sizeof(unsigned int) * PP3_DEBUG_ARRAY_SIZE, GFP_KERNEL);

	if (!strcmp(name, "write_nss_reg")) {
		if (c > PP3_DEBUG_ARRAY_SIZE) {
			pr_info("can't write more than %d values", PP3_DEBUG_ARRAY_SIZE);
			err = 1;
			goto end;
		}
		for (i = 0; i < c; i++)
			arr[i] = b;
		mv_pp3_hw_write(a + mv_pp3_nss_regs_vaddr_get(), c, arr);
	} else if (!strcmp(name, "read_nss_reg")) {
		if (b > PP3_DEBUG_ARRAY_SIZE) {
			pr_info("can't read more than %d values", PP3_DEBUG_ARRAY_SIZE);
			err = 1;
			goto end;
		}
		mv_pp3_hw_read(a + mv_pp3_nss_regs_vaddr_get(), b, arr);
		for (i = 0; i < b; i++)
			pr_info("0x%x = 0x%08x\n", a+i*4, arr[i]);
	} else if (!strcmp(name, "write_u32_le")) {
		if (c > PP3_DEBUG_ARRAY_SIZE) {
			pr_info("can't write more than %d values", PP3_DEBUG_ARRAY_SIZE);
			err = 1;
			goto end;
		}
		for (i = 0; i < c; i++)
			arr[i] = b;
		mv_pp3_hw_write((void __iomem *)a, c, arr);
	} else if (!strcmp(name, "read_u32_le")) {
		if (b > PP3_DEBUG_ARRAY_SIZE) {
			pr_info("can't read more than %d values", PP3_DEBUG_ARRAY_SIZE);
			err = 1;
			goto end;
		}
		mv_pp3_hw_read((void __iomem *)a, b, arr);
		for (i = 0; i < b; i++)
			pr_info("0x%x = 0x%08x\n", a + i*4, arr[i]);

	} else if (!strcmp(name, "read_u32")) {
		for (i = 0; i < b; i++)
			pr_info("0x%x = 0x%08x\n", a + i*4, *(u32 *)(a + 4*i));

	} else if (!strcmp(name, "write_u32")) {
		for (i = 0; i < c; i++)
			*(u32 *)(a + 4*i) = b;
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}
end:
	kfree(arr);

	local_irq_restore(flags);

	if (err)
		pr_err("%s: error %d\n", __func__, err);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(write_nss_reg,	S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(read_nss_reg,	S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(read_u32_le,		S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(write_u32_le,	S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(read_u32,		S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(write_u32,		S_IWUSR, NULL, pp3_hex_debug_store);
static DEVICE_ATTR(modify_nss_reg,	S_IWUSR, NULL, pp3_hex_debug_modify);
static DEVICE_ATTR(modify_u32,		S_IWUSR, NULL, pp3_hex_debug_modify);
static DEVICE_ATTR(modify_u32_le,	S_IWUSR, NULL, pp3_hex_debug_modify);
static DEVICE_ATTR(help,		S_IRUSR, pp3_debug_show, NULL);
static DEVICE_ATTR(reset_nss,		S_IRUSR, pp3_debug_show, NULL);

static struct attribute *pp3_debug_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_reset_nss.attr,
	&dev_attr_write_nss_reg.attr,
	&dev_attr_read_nss_reg.attr,
	&dev_attr_write_u32_le.attr,
	&dev_attr_read_u32_le.attr,
	&dev_attr_write_u32.attr,
	&dev_attr_read_u32.attr,
	&dev_attr_modify_nss_reg.attr,
	&dev_attr_modify_u32_le.attr,
	&dev_attr_modify_u32.attr,
	NULL
};

static struct attribute_group pp3_debug_group = {
	.attrs = pp3_debug_attrs,
};

static struct kobject *dev_kobj;

int mv_pp3_debug_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	dev_kobj = kobject_create_and_add("debug", pp3_kobj);
	if (!dev_kobj) {
		pr_err("%s: cannot create dev kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_update_group(dev_kobj, &pp3_debug_group);
	if (err) {
		pr_err("sysfs group %s failed %d\n", pp3_debug_group.name, err);
		return err;
	}

	return err;
}


int mv_pp3_debug_sysfs_exit(struct kobject *pp3_kobj)
{
	sysfs_remove_group(pp3_kobj, &pp3_debug_group);

	return 0;
}


