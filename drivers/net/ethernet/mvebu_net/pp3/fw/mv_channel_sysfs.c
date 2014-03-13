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
#include "mv_channel_if.h"

static ssize_t mv_channel_help(char *b)
{
	int o = 0;

	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "echo [c]         > chan_show    - Show message channel status\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "\n");
	o += scnprintf(b+o, PAGE_SIZE-o, "parameters: [c] channel number\n");

	return o;
}

static ssize_t mv_channel_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_channel_help(buf);

	return off;
}

static ssize_t mv_channel_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int             err;
	unsigned int    c;
	unsigned long   flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read first 3 parameters */
	err = c = 0;
	sscanf(buf, "%x", &c);

	if (!strcmp(name, "chan_show"))
		mv_pp3_channel_show(c);
	else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help, S_IRUSR, mv_channel_show, NULL);
static DEVICE_ATTR(chan_show, S_IWUSR, NULL, mv_channel_store);


static struct attribute *mv_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_chan_show.attr,
	NULL
};

static struct attribute_group mv_chan_group = {
	.name = "messenger",
	.attrs = mv_attrs,
};

int mv_pp3_chan_sysfs_init(struct kobject *pp3_kobj)
{
	int err;

	err = sysfs_create_group(pp3_kobj, &mv_chan_group);
	if (err) {
		pr_err("sysfs group failed %d\n", err);
		return err;
	}

	return err;
}

int mv_pp3_chan_sysfs_exit(struct kobject *hmac_kobj)
{
	sysfs_remove_group(hmac_kobj, &mv_chan_group);
	return 0;
}
