/*********************************************************************
 * DebugFS for Marvell PPv2 network controller for Armada 7k/8k SoC.
 *
 * Copyright (C) 2018 Marvell
 *
 * Yan Markman <ymarkman@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef CONFIG_DEBUG_FS
#error CONFIG_DEBUG_FS is not defined
#endif

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <asm/setup.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/if_ether.h>
#include <linux/bitops.h>

#include "mvpp2_dbgfs.h"

#define MAX_CPN		8
#define ARGNUM		3
#define MAXETH		5

enum mvpp2_dbgfs_ops {
	ARG_RO,
	ARG_W1,
	ARG_W2,
	ARG_W3,
	SETETH = 8,
	UCADD,
	UCDEL,
	UCDELA,
};

struct mvpp2_f_desc {
	/* CONST part */
	const char	*name;
	const int	arg_num_cfg;
	const u32	arg_limit[ARGNUM];
	int (*show)(struct seq_file *, void *);
	const char	*help;

	/* VARIABLE part */
	int	arg_num;
	u32	arg[ARGNUM];

	/* if(!show) this is child directory but not a file */
	struct dentry *dentry;
};

struct mvpp2_glb_cnfg {
	unsigned long flags;
	struct dentry *root_dir;
	const char *plat_dev_name[MAX_CPN];
	char	ifname[14];
	int	eth_no;
	int	gop_id;
	int	cpn; /* cp110-0, cp110-1 */
	struct net_device	*netdev;
	struct mvpp2_port	*port;
	struct platform_device	*plat_dev;
	struct mvpp2		*priv;
	struct mvpp2		*hw; // hw=priv:  hw->swth_base[cpu]
};

struct mvpp2_glb_cnfg	mvpp2_global_config = {
	.plat_dev_name = {"f2000000.ethernet", "f4000000.ethernet",
		"8100000000.ethernet", "8100000000.ethernet",
		"8100000000.ethernet", "8800000000.ethernet",
		"8f00000000.ethernet", "9600000000.ethernet"},
};

static struct mvpp2_f_desc			f_root[];
static const struct file_operations	mvpp2_dbgfs_fops;

static inline bool IS_UCSET(const int cmd)
{
	return (cmd >= UCADD && cmd <= UCDEL);
}

static inline void mvpp2_write(struct mvpp2 *hw, u32 offset, u32 data)
{
	writel(data, hw->swth_base[smp_processor_id()] + offset);
}

static inline u32 mvpp2_read(struct mvpp2 *hw, u32 offset)
{
	return readl(hw->swth_base[smp_processor_id()] + offset);
}

static inline void mvpp2_percpu_write(struct mvpp2 *priv, int cpu,
				      u32 offset, u32 data)
{
	writel(data, priv->swth_base[cpu] + offset);
}

static inline u32 mvpp2_percpu_read(struct mvpp2 *priv, int cpu,
				    u32 offset)
{
	return readl(priv->swth_base[cpu] + offset);
}

static void mvpp2_show_reg(struct seq_file *m, struct mvpp2 *hw,
			   unsigned int reg_addr, char *reg_name)
{
	seq_printf(m, "  %-32s: 0x%04x = 0x%08x\n",
		   reg_name, reg_addr, mvpp2_read(hw, reg_addr));
}

static int mvpp2_range_validate(struct seq_file *m, int value, int min, int max)
{
	if (((value) > (max)) || ((value) < (min))) {
		seq_printf(m, "%s: value 0x%X (%d) is out of range [0x%X , 0x%X].\n",
			   __func__, (value), (value), (min), (max));
		return -EINVAL;
	}
	return 0;
}

static void mvpp2_prt_reg(struct seq_file *m, struct mvpp2 *priv,
			  unsigned int addr, char *name)
{
	seq_printf(m, "  %-32s: 0x%04x = 0x%08x\n",
		   name, addr,
		   mvpp2_read(priv, addr));
}

/* Get net-dev with checking it belongs to PP2.2 */
static struct net_device *mvpp2_dbgfs_get_netdev(int eth_no, int *cpn_no,
						 bool with_put)
{
	struct net_device *netdev;
	struct device	*pp2dev;
	struct mvpp2_port *port;
	char ifname[14];
	int cpn;

	sprintf(ifname, "eth%d", eth_no);
	netdev = dev_get_by_name(&init_net, ifname);
	if (!netdev)
		return netdev;
	port = netdev_priv(netdev);
	if (!port || !port->priv)
		goto err;
	pp2dev = netdev->dev.parent;
	if (!pp2dev)
		goto err;
	cpn = 0;
	while (strcmp(dev_name(pp2dev), mvpp2_global_config.plat_dev_name[cpn]))
		if (++cpn == MAX_CPN)
			goto err;
	if (with_put)
		dev_put(netdev);
	*cpn_no = cpn;
	return netdev;
err:
	dev_put(netdev);
	return NULL;
}

int mvpp2_dbgfs_eth_set_show(int eth_no, bool set, bool kmsg,
			     struct seq_file *m, const char *str)
{
	struct mvpp2_glb_cnfg *gbl = &mvpp2_global_config;
	struct net_device *netdev;
	struct mvpp2_port *port;
	char ifname[14], msg[100];
	int cpn;

	netdev = mvpp2_dbgfs_get_netdev(eth_no, &cpn, true);
	if (!netdev)
		return -EINVAL;

	sprintf(ifname, "eth%d", eth_no);
	port = netdev_priv(netdev);
	if (set) {
		strcpy(gbl->ifname, ifname);
		gbl->eth_no = eth_no;
		gbl->netdev = netdev;
		gbl->gop_id = port->gop_id;
		gbl->port = port;
		gbl->priv = port->priv;
		gbl->hw = port->priv;
		gbl->cpn = cpn;
		gbl->plat_dev = NULL;
	}

	sprintf(msg, "%s %s: port=%d, cp110-%d {%s}\n",
		(str) ? str : " ", ifname, port->gop_id,
		cpn, gbl->plat_dev_name[cpn]);
	if (kmsg)
		pr_info("%s", msg);
	if (m)
		seq_printf(m, "%s", msg);

	return 0;
}

static int mvpp2_dbgfs_gbl_init(void)
{
	int id = 0;

	sprintf(mvpp2_global_config.ifname, "ethN");
	while (mvpp2_dbgfs_eth_set_show(id, true, true, NULL, "pp2-dbgfs:"))
		if (++id > MAXETH)
			return -ENODEV;
	return 0;
}

static int mvpp2_show_gbl_all(struct seq_file *m, void *v)
{
	int id = mvpp2_global_config.eth_no;

	if (mvpp2_dbgfs_eth_set_show(id, false, false, m, "pp2 CURR:"))
		seq_puts(m, "pp2-dbgfs: no device configured\n");

	seq_puts(m, "------- MVPP2 interface/device mapping ------\n");
	for (id = 0; id <= MAXETH; id++)
		mvpp2_dbgfs_eth_set_show(id, false, false, m, NULL);
	return 0;
}

static void mvpp2_show_help_1(struct seq_file *m, struct mvpp2_f_desc *f,
			      int err)
{
	char buf[120], op;
	int i, offs;

	if (!m)
		return;
	if (f->name[0] == '\n') {
		seq_printf(m, "%s", f->name);
		return;
	}
	if (!f->arg_num_cfg) {
		sprintf(buf, "cat");
		op = ' ';
	} else if (f->arg_num_cfg == SETETH) {
		sprintf(buf, "cat   or   echo N");
		op = '>';
	} else if (f->arg_num_cfg == UCDELA) {
		sprintf(buf, "echo ethN");
		op = '>';
	} else if (IS_UCSET(f->arg_num_cfg)) {
		sprintf(buf, "echo ethN a:b:c:d:e:f");
		op = '>';
	} else {
		/*10:<echo a b c>*/
		offs = sprintf(buf, "echo");
		for (i = 0; i < f->arg_num_cfg; i++)
			offs += sprintf(buf + offs, " %u", f->arg_limit[i]);
		op = '>';
	}
	if (err)
		seq_puts(m, "ERROR: ");
	seq_printf(m, " %-21s %c %-17s  @%s\n", buf, op, f->name, f->help);
}

static int mvpp2_show_help_root(struct seq_file *m, void *v)
{
	struct mvpp2_f_desc *f = &f_root[0];

	mvpp2_show_gbl_all(m, NULL);
	seq_puts(m, "--- MVPP2  DUMP/Print commands -------------\n");
	while (f->name) {
		mvpp2_show_help_1(m, f, 0);
		f++;
	}
	return 0;
}

static bool mvpp2_f_arg_is_valid(struct seq_file *m, struct mvpp2_f_desc *f)
{
	int err = 0, i;
	int arg_num_cfg;

	arg_num_cfg = (f->arg_num_cfg == SETETH) ? 1 : f->arg_num_cfg;
	if (arg_num_cfg) {
		if (f->arg_num != arg_num_cfg)
			err++;
		for (i = 0; i < arg_num_cfg; i++) {
			if (f->arg_limit[i] && f->arg[i] > f->arg_limit[i])
				err++;
		}
	}
	if (err && m)
		mvpp2_show_help_1(m, f, err);
	return !err;
}

static inline int mvpp2_show_f_empty(struct seq_file *m, void *v)
{
	struct mvpp2_f_desc *f = m->private;
	char buf[ARGNUM * 10 + 1];
	int i, offs;

	if (!mvpp2_f_arg_is_valid(m, f))
		return 0;
	if (!f->arg_num) {
		sprintf(buf, " ");
	} else {
		offs = 0;
		for (i = 0; i < f->arg_num; i++)
			offs += sprintf(buf + offs, "%u,", f->arg[i]);
		buf[offs - 1] = 0;
	}
	seq_printf(m, "Proc show_%s(%s) is empty\n", f->name, buf);
	return 0;
}

#include "mvpp2_dbgfs_utils.h"

/*static*/ void mvpp2_dbgfs_setup_bm_pool(void)
{
	/* Short pool */
	mvpp2_pools[MVPP2_BM_SHORT].buf_num  = MVPP2_BM_SHORT_BUF_NUM;
	mvpp2_pools[MVPP2_BM_SHORT].pkt_size = MVPP2_BM_SHORT_PKT_SIZE;

	/* Long pool */
	mvpp2_pools[MVPP2_BM_LONG].buf_num  = MVPP2_BM_LONG_BUF_NUM;
	mvpp2_pools[MVPP2_BM_LONG].pkt_size = MVPP2_BM_LONG_PKT_SIZE;

	/* Jumbo pool */
	mvpp2_pools[MVPP2_BM_JUMBO].buf_num  = MVPP2_BM_JUMBO_BUF_NUM;
	mvpp2_pools[MVPP2_BM_JUMBO].pkt_size = MVPP2_BM_JUMBO_PKT_SIZE;
}

/*"fName",	args num,  limit[3] , show-function, "Help description"*/
static struct mvpp2_f_desc f_root[] = {
{ "_eth",		SETETH, {MAXETH}, mvpp2_show_gbl_all,	"Show/Set active ethN (and port+cp110n)"},
{ "help",		ARG_RO, { 0 }, mvpp2_show_help_root,	"HELP"},
{ "\n" },
{ "cls_lkp_hw_dump",	ARG_RO, { 0 }, mvpp2_cls_hw_lkp_dump,	      "CLS  lookup ID table from HW" },
{ "cls_lkp_hw_hits",	ARG_RO, { 0 }, mvpp2_cls_hw_lkp_hits_dump,  "CLS  nonZero hits lookup ID entires" },
{ "cls_flow_hw_dump",	ARG_RO, { 0 }, mvpp2_cls_hw_flow_dump,      "CLS  flow table from HW" },
{ "cls_flow_hw_hits",	ARG_RO, { 0 }, mvpp2_cls_hw_flow_hits_dump, "CLS  nonZero hits flow tab entries" },
{ "cls_hw_regs",	ARG_RO, { 0 }, mvpp2_cls_hw_regs_dump,      "CLS  classifier top registers" },
{ "\n" },
{ "cls2_act_hw_dump",	ARG_RO, { 0 }, mvpp2_cls2_hw_dump,          "CLS2 action table enrties" },
{ "cls2_dscp_hw_dump",	ARG_RO, { 0 }, mvpp2_cls2_qos_dscp_hw_dump, "CLS2 QoS dscp tables" },
{ "cls2_prio_hw_dump",	ARG_RO, { 0 }, mvpp2_cl2_qos_prio_hw_dump,  "CLS2 QoS priority tables" },
{ "cls2_cnt_dump",	ARG_RO, { 0 }, mvpp2_cls2_hit_cntr_dump,    "CLS2 hit counters" },
{ "cls2_hw_regs",	ARG_RO, { 0 }, mvpp2_cls2_regs_dump,        "CLS2 C2-classifier registers" },
{ "\n" },
{ "prs_dump",		ARG_RO, { 0 }, mvpp2_show_prs_hw,	 "PRS  dump all valid HW entries" },
{ "prs_hits",		ARG_RO, { 0 }, mvpp2_show_prs_hw_hits, "PRS  dump hit counters with HW entries" },
{ "prs_regs",		ARG_RO, { 0 }, mvpp2_show_prs_hw_regs, "PRS  dump parser registers" },
{ "\n" },
{ "rss_hw_dump",	ARG_RO, { 0 }, mvpp2_rss_hw_dump,	 "RSS  dump rxq in rss-table entry" },
{ "rss_hw_rxq_tbl",	ARG_RO, { 0 }, mvpp2_rss_hw_rxq_tbl,	 "RSS  dump rxq table assignment" },
{ "\n" },
{ "uc_filter_add",	UCADD,  { 0 }, mvpp2_uc_filter_dump,	"UC MAC  add       to given ifname" },
{ "uc_filter_del",	UCDEL,  { 0 }, mvpp2_uc_filter_dump,	"UC MAC delete   from given ifname" },
{ "uc_filter_delall",	UCDELA, { 0 }, mvpp2_uc_filter_dump,	"UC MAC delete ALL (flush) uc-macs" },
{ "uc_filter_show",	ARG_RO, { 0 }, mvpp2_uc_filter_dump,	"UC MAC list on Pre-Set ethN interface" },
{ "\n" },
{ NULL } /* NULL terminator must be present */
};

static struct mvpp2_f_desc *mvpp2_f_desc_get(char *fname,
					     struct mvpp2_f_desc *f)
{
	while (f->name) {
		if (!strcmp(fname, f->name))
			return f;
		f++;
	}
	return NULL;
}

static void mvpp2_f_create(struct mvpp2_f_desc *f, struct dentry *parent_dir)
{
	if (f->name[0] == '\n')
		return;
	if (!f->show) {
		f->dentry = debugfs_create_dir(f->name, parent_dir);
		return;
	}
	debugfs_create_file(f->name,
			    (f->arg_num_cfg) ? 0644 : 0444,
			    parent_dir,
			    f, /* DATA passed into file-command */
			    &mvpp2_dbgfs_fops
			   );
}

static ssize_t mvpp2_seq_write(struct file *file, const char __user *buf,
			       size_t size,
			       loff_t *ppos)
{
	char lbuf[size + 1];
	int i, num;
	u32 arg[3];
	struct mvpp2_f_desc *f;
	int arg_num_cfg;

	if (size > (ARGNUM * 9))
		return -EINVAL;
	f = mvpp2_f_desc_get(file->f_path.dentry->d_iname, &f_root[0]);
	if (!f || !f->arg_num_cfg || size > 80)
		return -EINVAL;

	memcpy(lbuf, buf, size);
	lbuf[size] = 0;

	/* Input-ARG is row-string parsed by cmd-handler */
	if (IS_UCSET(f->arg_num_cfg))
		return mvpp2_uc_filter_set((f->arg_num_cfg == UCADD),
					   lbuf, size);
	if (f->arg_num_cfg == UCDELA)
		return mvpp2_uc_filter_flush_delall(lbuf, size);

	/* Input-ARGs are Integer Numeric (up to 3 args) */
	num = sscanf(lbuf, "%u %u %u", &arg[0], &arg[1], &arg[2]); /*ARGNUM*/
	f->arg_num = num;
	arg_num_cfg = (f->arg_num_cfg == SETETH) ? 1 : f->arg_num_cfg;

	for (i = 0; i < arg_num_cfg; i++)
		f->arg[i] = arg[i];
	if (!mvpp2_f_arg_is_valid(NULL, f))
		return -EINVAL;

	if (f->arg_num_cfg == SETETH &&
	    mvpp2_dbgfs_eth_set_show(f->arg[0], true, false, NULL, NULL))
		return -EINVAL;

	return size;
}

static int mvpp2_dbgfs_open(struct inode *inode, struct file *file)
{
	struct mvpp2_f_desc *f = inode->i_private;

	return single_open(file, f->show, inode->i_private);
}

static const struct file_operations mvpp2_dbgfs_fops = {
	.open		= mvpp2_dbgfs_open,
	.write		= mvpp2_seq_write, /* Input parser for ALL commands */
	.read		= seq_read, /* read method for sequential files. */
	.llseek		= seq_lseek, /* llseek method for sequential files */
	.release	= single_release,
};

static int __init mvpp2_dbgfs_root_init(void)
{
	struct dentry *d;
	struct mvpp2_f_desc *f;

	d = debugfs_create_dir("mvpp2", NULL);
	if (!d)
		return -ENOMEM;
	if (d == ERR_PTR(-ENODEV))
		return -ENODEV;
	mvpp2_global_config.root_dir = d;
	f = &f_root[0];
	while (f->name) {
		mvpp2_f_create(f, d);
		f++;
	}
	return 0;
}

static void mvpp2_dbgfs_root_exit(void)
{
	if (!mvpp2_global_config.root_dir)
		return;
	debugfs_remove_recursive(mvpp2_global_config.root_dir);
}

static int __init mvpp2_dbgfs_module_init(void)
{
	int ret;

	ret = mvpp2_dbgfs_root_init();
	if (ret)
		return ret;
	ret = mvpp2_dbgfs_gbl_init();
	if (ret)
		mvpp2_dbgfs_root_exit();
	return ret;
}

static void __exit mvpp2_dbgfs_module_exit(void)
{
	mvpp2_dbgfs_root_exit();
}

module_init(mvpp2_dbgfs_module_init);
module_exit(mvpp2_dbgfs_module_exit);

MODULE_DESCRIPTION("Marvell PPv2 DebugFS commands - www.marvell.com");
MODULE_LICENSE("GPL v2");
