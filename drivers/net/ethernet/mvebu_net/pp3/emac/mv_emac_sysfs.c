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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#include <linux/module.h>
#include <linux/netdevice.h>
#include "emac/mv_emac.h"

static ssize_t mv_emac_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p]         > regs     - Dump registers\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u]     > regRead  - Read register, address [u]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u] [v] > regWrite - Write register, address [u], value [v]\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [u] [v] > qmMapSet - Set QM mapping, [u] QM port, [v] QM queue\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [0|1]   > rxEn     - Enable / Disable rx\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [p] [0|1]   > debug    - Enable / Disable debug outputs\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters: [p] emac number\n");

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
	else if (!strcmp(name, "regWrite"))
		mv_pp3_emac_reg_write(p, u, v);
	else if (!strcmp(name, "regRead"))
		mv_pp3_emac_reg_write(p, u, v);
	else if (!strcmp(name, "debug"))
		mv_pp3_emac_debug(p, u);
	else if (!strcmp(name, "qmMapSet"))
		mv_pp3_emac_qm_mapping(p, u, v);
	else if (!strcmp(name, "rxEn"))
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
static DEVICE_ATTR(regWrite, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(regRead, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(debug, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(qmMapSet, S_IWUSR, NULL, mv_emac_3_hex_store);
static DEVICE_ATTR(rxEn, S_IWUSR, NULL, mv_emac_3_hex_store);


static struct attribute *mv_emac_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_regs.attr,
	&dev_attr_regWrite.attr,
	&dev_attr_regRead.attr,
	&dev_attr_debug.attr,
	&dev_attr_qmMapSet.attr,
	&dev_attr_rxEn.attr,
	NULL
};

static struct attribute_group mv_emac_group = {
	.attrs = mv_emac_attrs,
};

static struct kobject *emac_kobj;

int mv_pp3_emac_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	emac_kobj = kobject_create_and_add("emac", pp3_kobj);
	if (!emac_kobj) {
		pr_err("%s: cannot create emac kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(emac_kobj, &mv_emac_group);
	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_emac_sysfs_exit(struct kobject *emac_kobj)
{
	/*TODO*/
	return 0;
}
