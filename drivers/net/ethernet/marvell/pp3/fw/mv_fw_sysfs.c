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
#include <linux/platform_device.h>
#include "mv_fw.h"
#include "mv_pp3_fw_msg.h"

static ssize_t mv_fw_help(char *b)
{
	int o = 0;
	int p = PAGE_SIZE;

	o += scnprintf(b + o, p - o, "\n");

	o += scnprintf(b + o, p - o, "cat                        version      - show FW version\n");
	o += scnprintf(b + o, p - o, "echo [path]              > fw_path      - define FW images folder\n");
	o += scnprintf(b + o, p - o, "echo [num]               > ppc_active   - number of active PPCs\n");
	o += scnprintf(b + o, p - o, "echo [ppc] [ppn] [e] [n] > inf_lg_dump  - informational logger dump\n");
	o += scnprintf(b + o, p - o, "echo [ppc] [ppn] [e] [n] > cr_lg_dump   - critical logger dump\n");
	o += scnprintf(b + o, p - o, "echo [ppc]               > ka_dump      - dump keep alive array\n");
	o += scnprintf(b + o, p - o, "echo [ppc_map]           > ka_get       - get keep alive status\n");
	o += scnprintf(b + o, p - o, "echo [port]              > emac_vp_show - show EMAC virtual port status\n");
	o += scnprintf(b + o, p - o, "echo [port]              > vp_stats     - show virtual port FW statistics\n");
	o += scnprintf(b + o, p - o, "echo [queue]             > hwq_stats    - show HW queue FW statistics\n");
	o += scnprintf(b + o, p - o, "echo [queue]             > swq_stats    - show SW queue FW statistics\n");
	o += scnprintf(b + o, p - o, "echo [pool]              > bmpool_stats - show BM pool FW statistics\n");
	o += scnprintf(b + o, p - o, "echo [channel]           > chan_stats   - show message channel FW statistics\n");
	o += scnprintf(b + o, p - o, "echo [st] [ind]          > clear_stats  - clear FW statistics by type, 0-all\n");

	o += scnprintf(b + o, p - o, "\n");
	o += scnprintf(b + o, p - o, "parameters:\n");
	o += scnprintf(b + o, p - o, "      [ppc_map] - bit map of PPC clusters (decimal)\n");
	o += scnprintf(b + o, p - o, "      [ppc]     - PPC number (decimal)\n");
	o += scnprintf(b + o, p - o, "      [ppn]     - PPN number in current PPC (decimal)\n");
	o += scnprintf(b + o, p - o, "      [entry]   - start entry number (decimal)\n");
	o += scnprintf(b + o, p - o, "      [numb]    - number of entries to print (decimal)\n");
	o += scnprintf(b + o, p - o, "      [port]    - virtual port number\n");
	o += scnprintf(b + o, p - o, "      [queue]   - queue number\n");
	o += scnprintf(b + o, p - o, "      [pool]    - pool number\n");
	o += scnprintf(b + o, p - o, "      [channel] - message channel number\n");
	o += scnprintf(b + o, p - o, "      [st]      - stats type: 0-all, 1-vport, 2-hwq, 3-swq, 4-bm pool, 5-chan\n");
	o += scnprintf(b + o, p - o, "      [ind]     - index according to statistics type\n");
	o += scnprintf(b + o, p - o, "      [num]     - number of active PPCs (decimal)\n");

	o += scnprintf(b + o, p - o, "\n");

	return o;
}

static ssize_t mv_fw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             rc, off = 0;
	struct mv_pp3_version fw_ver;
	char version_name[MV_PP3_VERSION_NAME_SIZE + 1];

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "version")) {
		rc = pp3_fw_version_get(&fw_ver);
		if (rc) {
			pr_err("FW version is unknown. rc = %d\n", rc);
			return 0;
		}
		memcpy(version_name, fw_ver.name, MV_PP3_VERSION_NAME_SIZE);
		version_name[MV_PP3_VERSION_NAME_SIZE] = '\0';
		pr_info("FW version:     %s:%d.%d.%d.%d\n",
				version_name, fw_ver.major_x, fw_ver.minor_y, fw_ver.local_z, fw_ver.debug_d);
	} else
		off = mv_fw_help(buf);

	return off;
}

static ssize_t mv_fw_store(struct device *dev,
			   struct device_attribute *attr, const char *buf,
			   size_t len)
{
	const char *name = attr->attr.name;
	unsigned int    a, b, c, d;
	int err;
	int fields;
	char str[128] = "/0";

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = a = b = c = 0;

	if (!strcmp(name, "fw_path")) {
		sscanf(buf, "%s", str);
		mv_pp3_fw_path_set(str);
	} else if (!strcmp(name, "inf_lg_dump")) {
		fields = sscanf(buf, "%d %d %d %d", &a, &b, &c, &d);
		if (fields == 4)
			mv_fw_inf_logger_dump(a, b, c, d);
		else
			err = 1;
	} else if (!strcmp(name, "ppc_active")) {
		fields = sscanf(buf, "%d", &a);
		if (fields == 1)
			err = mv_pp3_fw_ppc_num_set(a);
		else
			err = 1;
	} else if (!strcmp(name, "cr_lg_dump")) {
		fields = sscanf(buf, "%d %d %d %d", &a, &b, &c, &d);
		if (fields == 4)
			mv_fw_critical_logger_dump(a, b, c, d);
		else
			err = 1;
	} else if (!strcmp(name, "ka_dump")) {
		if (!kstrtoint(buf, 10, &a))
			mv_fw_keep_alive_dump(a);
		else
			err = 1;
	} else if (!strcmp(name, "ka_get")) {
		if (!kstrtoint(buf, 10, &a)) {
			int ppc;

			for (ppc = 0; ppc < MV_PP3_PPC_MAX_NUM; ppc++) {
				if ((a & (1 << ppc)) && mv_fw_keep_alive_get(ppc))
					pr_info("ppc #%d: keep alive is OK\n", ppc);
			}
		} else {
			err = 1;
		}
	} else if (!strcmp(name, "emac_vp_show")) {
		int i;
		unsigned char valid_macs;
		unsigned char macs_list[MV_PP3_MAC_ADDR_NUM][MV_MAC_ADDR_SIZE];
		char prefix[16];

		fields = sscanf(buf, "%d", &a);
		if (fields == 1) {
			if (a > MV_NSS_ETH_PORT_MAX) {
				pr_info("Not supported for vp=%d. Only for EMAC virtual ports [%d..%d]\n",
					a, MV_NSS_ETH_PORT_MIN, MV_NSS_ETH_PORT_MAX);
				goto out;
			}
			err = pp3_fw_emac_vport_msg_show(a);
			if (err)
				goto out;

			err = pp3_fw_vport_mac_list_get(a, MV_PP3_MAC_ADDR_NUM, &macs_list[0][0],
						&valid_macs);
			if (!err) {
				pr_info("%d valid MAC addresses\n", valid_macs);
				for (i = 0; i < valid_macs; i++) {
					sprintf(prefix, "%2d: ", i);
					mv_mac_addr_print(prefix, &macs_list[i][0], NULL);
				}
			}
		} else
			err = 1;
	} else if (!strcmp(name, "vp_stats")) {
		fields = sscanf(buf, "%d", &a);
		if (fields == 1)
			mv_pp3_vport_fw_stats_dump(a);
		else
			err = 1;
	} else if (!strcmp(name, "hwq_stats")) {
		struct mv_pp3_fw_hwq_stat hwq_stats;

		sscanf(buf, "%d", &a);
		if (pp3_fw_hwq_stat_get((unsigned short)a, false, &hwq_stats) == 0)
			pp3_fw_hwq_stat_print(&hwq_stats);

	} else if (!strcmp(name, "swq_stats")) {
		struct mv_pp3_fw_swq_stat swq_stats;

		sscanf(buf, "%d", &a);
		if (pp3_fw_swq_stat_get(a, &swq_stats) == 0)
			/* print statistic */
			pp3_fw_swq_stat_print(&swq_stats);

	} else if (!strcmp(name, "bmpool_stats")) {
		struct mv_pp3_fw_bm_pool_stat pool_stat;
		unsigned char param;

		sscanf(buf, "%d", &a);
		param = (unsigned char)a;
		if (pp3_fw_bm_pool_stat_get(param, &pool_stat) == 0)
			pp3_fw_bmpool_stat_print(&pool_stat);
	} else if (!strcmp(name, "chan_stats")) {
		struct mv_pp3_fw_msg_chan_stat msg_stat;
		unsigned char param;

		sscanf(buf, "%d", &a);
		param = (unsigned char)a;
		if (pp3_fw_channel_stat_get(param, &msg_stat) == 0)
			/* print statistic */
			pp3_fw_msg_stat_print(&msg_stat);
	} else if (!strcmp(name, "clear_stats")) {

		sscanf(buf, "%d %d", &a, &b);
		if (pp3_fw_clear_stat_set((unsigned char) a, b) != 0)
			pr_info("Command failed");
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__,
		       attr->attr.name);
	}
out:
	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,		S_IRUSR, mv_fw_show, NULL);
static DEVICE_ATTR(version,		S_IRUSR, mv_fw_show, NULL);
static DEVICE_ATTR(fw_path,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(inf_lg_dump,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(ppc_active,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(cr_lg_dump,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(ka_dump,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(ka_get,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(emac_vp_show,	S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(vp_stats,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(hwq_stats,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(swq_stats,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(bmpool_stats,	S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(chan_stats,		S_IWUSR, NULL, mv_fw_store);
static DEVICE_ATTR(clear_stats,		S_IWUSR, NULL, mv_fw_store);


static struct attribute *mv_fw_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_version.attr,
	&dev_attr_fw_path.attr,
	&dev_attr_inf_lg_dump.attr,
	&dev_attr_ppc_active.attr,
	&dev_attr_cr_lg_dump.attr,
	&dev_attr_ka_dump.attr,
	&dev_attr_ka_get.attr,
	&dev_attr_emac_vp_show.attr,
	&dev_attr_vp_stats.attr,
	&dev_attr_hwq_stats.attr,
	&dev_attr_swq_stats.attr,
	&dev_attr_bmpool_stats.attr,
	&dev_attr_chan_stats.attr,
	&dev_attr_clear_stats.attr,

	NULL
};

static struct attribute_group mv_fw_group = {
	.attrs = mv_fw_attrs,
};


static ssize_t mv_fw_debug_help(char *b)
{
	int o = 0;
	int p = PAGE_SIZE;

	o += scnprintf(b + o, p - o,
		       "echo [ppc] [path]                > imem_dnld     - Download PPC IMEM from file\n");
	o += scnprintf(b + o, p - o,
		       "echo [ppc] [path]                > profile_dnld  - Download PPC profile table from file\n");
	o += scnprintf(b + o, p - o,
		       "echo [path]                      > se_dnld       - Download SE from file\n");
	o += scnprintf(b + o, p - o,
		       "echo                             > ppn_run       - Run PPN\n");
	o += scnprintf(b + o, p - o,
		       "echo [ppc][ppn][ind][adr][words] > sp_dump       - Print SP contents\n");
	o += scnprintf(b + o, p - o,
		       "echo [ppc][adr][words]           > rec_dump      - Print messages/packets buffer contents\n");
	o += scnprintf(b + o, p - o, "parameters:\n");
	o += scnprintf(b + o, p - o, "      [ppc]   - PPC number (decimal)\n");
	o += scnprintf(b + o, p - o, "      [ppn]   - PPN number in current PPC (decimal)\n");
	o += scnprintf(b + o, p - o, "      [ind]   - index of SP_image array (decimal)\n");
	o += scnprintf(b + o, p - o, "      [adr]   - offset to print (hex)\n");
	o += scnprintf(b + o, p - o, "      [words] - words number to print (decimal)\n");


	return o;
}

static ssize_t mv_fw_debug_show(struct device *dev,
			  struct device_attribute *attr, char *buf)
{
	/* TODO: const char      *name = attr->attr.name; */
	int off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	off = mv_fw_debug_help(buf);

	return off;
}

static ssize_t mv_fw_debug_store(struct device *dev,
			   struct device_attribute *attr, const char *buf,
			   size_t len)
{
	const char *name = attr->attr.name;
	unsigned int    a, b, c, d, e;
	int err;
	int fields;
	char str[128] = "/0";

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = a = b = c = d = 0;

	if (!strcmp(name, "imem_dnld")) {
		fields = sscanf(buf, "%d %s", &a, str);
		err = (fields != 2) ? 1 : 0;
		mv_pp3_ppc_fw_image_download(a, str, PPC_MEM_IMEM);
	} else if (!strcmp(name, "profile_dnld")) {
		fields = sscanf(buf, "%d %s", &a, str);
		err = (fields != 2) ? 1 : 0;
		mv_pp3_ppc_fw_image_download(a, str, PPC_MEM_PROF);
	} else if (!strcmp(name, "se_dnld")) {
		fields = sscanf(buf, "%s", str);
		err = (fields != 1) ? 1 : 0;
		mv_pp3_se_fw_image_download(str);
	} else if (!strcmp(name, "ppn_run")) {
		mv_pp3_ppc_run_all();
	} else if (!strcmp(name, "rec_dump")) {
		fields = sscanf(buf, "%d %x %d", &a, &b, &c);
		err = (fields != 3) ? 1 : 0;
		mv_fw_pkts_rec_dump(a, b, c);
	} else if (!strcmp(name, "sp_dump")) {
		fields = sscanf(buf, "%d %d %d %x %d", &a, &b, &c, &d, &e);
		err = (fields != 5) ? 1 : 0;
		mv_fw_sp_dump(a, b, c, d, e);
	} else {
		err = 1;
		pr_err("%s: illegal operation <%s>\n", __func__,
		       attr->attr.name);
	}

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help_debug,		S_IRUSR, mv_fw_debug_show, NULL);
static DEVICE_ATTR(imem_dnld,		S_IWUSR, NULL, mv_fw_debug_store);
static DEVICE_ATTR(se_dnld,		    S_IWUSR, NULL, mv_fw_debug_store);
static DEVICE_ATTR(profile_dnld,	S_IWUSR, NULL, mv_fw_debug_store);
static DEVICE_ATTR(ppn_run,		    S_IWUSR, NULL, mv_fw_debug_store);
static DEVICE_ATTR(sp_dump,		S_IWUSR, NULL, mv_fw_debug_store);
static DEVICE_ATTR(rec_dump,		S_IWUSR, NULL, mv_fw_debug_store);

static struct attribute *mv_fw_debug_attrs[] = {
	&dev_attr_help_debug.attr,
	&dev_attr_imem_dnld.attr,
	&dev_attr_se_dnld.attr,
	&dev_attr_profile_dnld.attr,
	&dev_attr_ppn_run.attr,
	&dev_attr_sp_dump.attr,
	&dev_attr_rec_dump.attr,

	NULL
};
static struct attribute_group mv_fw_debug_group = {
	.name = "debug",
	.attrs = mv_fw_debug_attrs,
};


int mv_pp3_fw_sysfs_init(struct kobject *neta_kobj)
{
	int err;
	struct kobject *fw_kobj;


	fw_kobj = kobject_create_and_add("fw", neta_kobj);
	if (!fw_kobj) {
		printk(KERN_ERR"%s: cannot create fw kobject\n", __func__);
		return -ENOMEM;
	}

	err = sysfs_create_group(fw_kobj, &mv_fw_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for fw %d\n", err);
		return err;
	}

	err = sysfs_create_group(fw_kobj, &mv_fw_debug_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for bm debug%d\n", err);
		return err;
	}


	return err;
}

int mv_pp3_fw_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &mv_fw_debug_group);
	sysfs_remove_group(neta_kobj, &mv_fw_group);
	return 0;
}

