// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/inet.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>
#include <linux/socket.h>
#include <linux/parser.h>
#include <linux/netdevice.h>
#include <linux/errno.h>
#include <net/ip.h>
#include <linux/ipv6.h>

#include "rvu.h"
#include "pan_tuple.h"
#include "pan_fl_tbl.h"
#include "pan_rvu.h"
#include "pan_test.h"

static struct pan_fl_tbl_node tn;
static struct pan_tuple *tuple;

static char src_ip_str[64];
static char dst_ip_str[64];

static int pan_test_simple_llu_get(void *data, u64 *val)
{
	char *str = (char *)data;

	if (!strncmp(str, "sport", strlen("sport")))
		*val = ntohs(tuple->sport);
	else if (!strncmp(str, "dport", strlen("dport")))
		*val = ntohs(tuple->dport);
	else if (!strncmp(str, "l4proto", strlen("l4proto")))
		*val = tuple->l4proto;

	return 0;
}

static int pan_test_simple_llu_set(void *data, u64 val)
{
	char *str = (char *)data;

	if (!strncmp(str, "sport", strlen("sport")))
		tuple->sport = htons((u16)val);
	else if (!strncmp(str, "dport", strlen("dport")))
		tuple->dport = htons((u16)val);
	else if (!strncmp(str, "l4proto", strlen("l4proto")))
		tuple->l4proto = (u8)val;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(pan_test_simple_llu_fops, pan_test_simple_llu_get,
			pan_test_simple_llu_set, "%llu\n");

static int pan_test_simple_llx_get(void *data, u64 *val)
{
	struct pan_rvu_gbl_t *gbl;
	char *str = (char *)data;

	if (!strncmp(str, "l3proto", strlen("l3proto"))) {
		*val = ntohs(tuple->l3proto);
	} else if (!strncmp(str, "pcifunc", strlen("pcifunc"))) {
		gbl = pan_rvu_get_gbl();
		*val = gbl->sqoff2pcifunc[tn.res.pcifuncoff];
	}

	return 0;
}

static int pan_test_simple_llx_set(void *data, u64 val)
{
	char *str = (char *)data;

	if (!strncmp(str, "l3proto", strlen("l3proto")))
		tuple->l3proto = htons((u16)val);
	else if (!strncmp(str, "pcifunc", strlen("pcifunc")))
		tn.res.pcifuncoff = pan_rvu_pcifunc2_sq_off((u16)val);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(pan_test_simple_llx_fops, pan_test_simple_llx_get,
			pan_test_simple_llx_set, "%#llx\n");

static int pan_test_debugfs_simple_node_create(struct dentry *parent, char *name)
{
	struct dentry *file;

	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_simple_llu_fops);
	if (!file) {
		pr_err("Failed to create sport debugfs node");
		return -EFAULT;
	}

	return 0;
}

static ssize_t pan_test_ip_read(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char *str  = file->private_data;
	unsigned int len;
	u8 *ptr;

	if (!strncmp(str, "src_ip", sizeof("src_ip")))
		ptr = (u8 *)src_ip_str;
	else
		ptr = (u8 *)dst_ip_str;

	len = strlen(ptr) + 1;
	return simple_read_from_buffer(user_buf, count, ppos, ptr, len);
}

static ssize_t pan_test_ip_write(struct file *file, const char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	char *str = file->private_data;
	u32 tmp[4] = { 0, 0, 0, 0};
	char buf[64];
	bool is_ipv4 = false;
	unsigned int len;
	int ret;
	u8 *ptr;

	tuple->flags = 0;

	len = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);
	buf[len] = '\0';

	ret = in4_pton(buf, strlen(buf), (u8 *)&tmp[0], '\0', NULL);
	if (ret) {
		tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;
		is_ipv4 = true;
		goto done;
	}

	ret = in6_pton(buf, strlen(buf), (u8 *)&tmp[0], '\0', NULL);
	if (ret) {
		tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V6;
		goto done;
	}

	if (!(tuple->flags & (PAN_TUPLE_FLAG_L3_PROTO_V4 | PAN_TUPLE_FLAG_L3_PROTO_V6))) {
		pr_err("ip address is not ipv4 or ipv6\n");
		return -EINVAL;
	}

done:
	if (is_ipv4) {
		if (!strncmp(str, "dst_ip", sizeof("dst_ip")))
			ptr = (u8 *)&tuple->dst_ip6;
		else
			ptr = (u8 *)&tuple->src_ip6;
	} else {
		if (!strncmp(str, "dst_ip", sizeof("dst_ip")))
			ptr = (u8 *)&tuple->dst_ip6;
		else
			ptr = (u8 *)&tuple->src_ip6;
	}

	memcpy(ptr, tmp, sizeof(struct in6_addr));

	if (!strncmp(str, "dst_ip", sizeof("dst_ip")))
		strscpy_pad(dst_ip_str, buf, sizeof(dst_ip_str));
	else
		strscpy_pad(src_ip_str, buf, sizeof(src_ip_str));

	return len;
}

static const struct file_operations pan_test_ip_ops = {
	.open           = simple_open,
	.read		= pan_test_ip_read,
	.write		= pan_test_ip_write,
};

static ssize_t pan_test_mac_read(struct file *file, char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	char *str = file->private_data;
	char buf[32];
	int len;
	u8 *mac;
	int rc;

	if (!strncmp(str, "smac", sizeof("smac")))
		mac = tuple->smac;
	else
		mac = tuple->dmac;

	rc = snprintf(buf, sizeof(buf), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
		      (int)*mac, (int)*(mac + 1), (int)*(mac + 2),
		      (int)*(mac + 3), (int)*(mac + 4), (int)*(mac + 5));
	if (!rc) {
		pr_err("Reading mac failed\n");
		return -EFAULT;
	}

	len = strlen(buf) + 1;
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t pan_test_mac_write(struct file *file, const char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	char *str = file->private_data;
	char buf[64];
	u8 *mac;
	int len;

	if (!strncmp(str, "smac", sizeof("smac")))
		mac = tuple->smac;
	else
		mac = tuple->dmac;

	len = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);

	if (!mac_pton(buf, mac)) {
		pr_err("Setting mac address failed %s\n", buf);
		return -EINVAL;
	}

	return len;
}

static const struct file_operations pan_test_mac_ops = {
	.open		= simple_open,
	.read		= pan_test_mac_read,
	.write		= pan_test_mac_write,
};

static struct sk_buff *pan_test_v6_skb_build(void)
{
	u8 smac[ETH_ALEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	u8 dmac[ETH_ALEN] = {0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
	int tot_len, data_len = 256;
	struct ipv6hdr *ip6h;
	struct sk_buff *skb;
	struct udphdr *udph;
	struct ethhdr *eth;
	u8 *data;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (!skb)
		return NULL;
	tot_len = sizeof(struct udphdr) + data_len;

	skb_reset_mac_header(skb);
	eth = skb_put(skb, sizeof(struct ethhdr));
	ether_addr_copy(eth->h_dest, dmac);
	ether_addr_copy(eth->h_source, smac);
	eth->h_proto = htons(ETH_P_IPV6);
	skb->protocol = htons(ETH_P_IPV6);

	skb_set_network_header(skb, skb->len);
	ip6h = skb_put(skb, sizeof(struct ipv6hdr));
	ip6h->version = 0x6;
	ip6h->payload_len = htons(tot_len);
	ip6h->nexthdr = IPPROTO_UDP;
	in6_pton("2001::100", -1, (u8 *)&ip6h->saddr, '\0', NULL);
	in6_pton("2001::200", -1, (u8 *)&ip6h->daddr, '\0', NULL);

	skb_set_transport_header(skb, skb->len);
	udph = skb_put_zero(skb, sizeof(struct udphdr));
	udph->source = htons(1000);
	udph->dest = htons(2000);

	udph->len = htons(sizeof(struct udphdr) + data_len);

	data = skb_put_zero(skb, data_len);
	strscpy(data, "Hello IPv6 world\n",  sizeof("Hello IPv6 world\n"));

	return skb;
}

static struct sk_buff *pan_test_v4_skb_build(void)
{
	int tot_len, data_len = 256;
	u8 smac[ETH_ALEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	u8 dmac[ETH_ALEN] = {0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
	struct sk_buff *skb;
	struct udphdr *udph;
	struct ethhdr *eth;
	struct iphdr *iph;
	u8 *data;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_ATOMIC);
	if (!skb)
		return NULL;
	tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + data_len;

	skb_reset_mac_header(skb);
	eth = skb_put(skb, sizeof(struct ethhdr));
	ether_addr_copy(eth->h_dest, dmac);
	ether_addr_copy(eth->h_source, smac);
	eth->h_proto = htons(ETH_P_IP);
	skb->protocol = htons(ETH_P_IP);

	skb_set_network_header(skb, skb->len);
	iph = skb_put(skb, sizeof(struct iphdr));
	iph->protocol = IPPROTO_UDP;
	iph->saddr = in_aton("192.168.8.100");
	iph->daddr = in_aton("192.168.8.200");
	iph->version = 0x4;
	iph->frag_off = 0;
	iph->ihl = 0x5;
	iph->tot_len = htons(tot_len);
	iph->ttl = 100;
	iph->check = 0;
	iph->check = ip_fast_csum((unsigned char *)iph, iph->ihl);

	skb_set_transport_header(skb, skb->len);
	udph = skb_put_zero(skb, sizeof(struct udphdr));
	udph->source = htons(1000);
	udph->dest = htons(2000);
	udph->len = htons(sizeof(struct udphdr) + data_len);

	data = skb_put_zero(skb, data_len);
	strscpy(data, "Hello IPv4 world\n",  sizeof("Hello IPv4 world\n"));

	return skb;
}

/* Commands -  echo "<cmd>" > <debugfs>/pan/test/cmd
 * tuple add
 * tuple del
 * tuple find
 */
enum {
	opt_cmd_tuple,
	opt_cmd_ingress,
	opt_cmd_egress,
	opt_cmd_modify_mcam,
	opt_add,
	opt_del,
	opt_find,
	opt_reset,
	opt_hooknum,
	opt_dev,
	opt_inject,
	opt_v4,
	opt_v6,
	opt_reg,
	opt_lookup_perf,
	opt_tbl_sz,
	opt_lookup_sz,
	opt_mcam_idx,
	opt_qidx,
	opt_err,
};

enum {
	pan_test_cmd_tuple_add = BIT_ULL(opt_cmd_tuple) | BIT_ULL(opt_add),
	pan_test_cmd_tuple_del = BIT_ULL(opt_cmd_tuple) | BIT_ULL(opt_del),
	pan_test_cmd_tuple_find = BIT_ULL(opt_cmd_tuple) | BIT_ULL(opt_find),
	pan_test_cmd_tuple_reset = BIT_ULL(opt_cmd_tuple) | BIT_ULL(opt_reset),
	pan_test_cmd_ingress_add = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_add) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_ingress_del = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_del) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_ingress_find = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_find) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_ingress_inject_stack = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_inject) |
					 BIT_ULL(opt_dev),
	pan_test_cmd_ingress_inject_hook = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_inject) |
					 BIT_ULL(opt_dev) |  BIT_ULL(opt_hooknum),
	pan_test_cmd_ingress_reg = BIT_ULL(opt_cmd_ingress) | BIT_ULL(opt_reg),
	pan_test_cmd_egress_add = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_add) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_egress_del = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_del) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_egress_find = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_find) |
				BIT_ULL(opt_hooknum) | BIT_ULL(opt_dev),
	pan_test_cmd_egress_inject_stack = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_inject) |
				BIT_ULL(opt_dev),
	pan_test_cmd_egress_inject_hook = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_inject) |
				BIT_ULL(opt_dev) | BIT_ULL(opt_hooknum),
	pan_test_cmd_egress_reg = BIT_ULL(opt_cmd_egress) | BIT_ULL(opt_reg),
	pan_test_cmd_lookup_perf = BIT_ULL(opt_lookup_perf) | BIT_ULL(opt_tbl_sz) |
					BIT_ULL(opt_lookup_sz),
	pan_test_cmd_redirect2pan = BIT_ULL(opt_cmd_modify_mcam) |
			BIT_ULL(opt_mcam_idx) | BIT_ULL(opt_qidx),
};

enum pkt_type {
	PKT_TYPE_NONE,
	PKT_TYPE_V4,
	PKT_TYPE_V6,
	PKT_TYPE_V4_VLAN,
	PKT_TYPE_V4_DOUBLE_VLAN,
};

static const match_table_t tokens = {
	{opt_cmd_tuple, "tuple"},
	{opt_cmd_ingress, "ingress"},
	{opt_cmd_egress, "egress"},
	{opt_cmd_modify_mcam, "modify"},
	{opt_add, "add"},
	{opt_del, "del"},
	{opt_find, "find"},
	{opt_inject, "inject=%s"},
	{opt_hooknum, "hooknum=%u"},
	{opt_dev, "dev=%s"},
	{opt_v4, "v4"},
	{opt_v6, "v6"},
	{opt_reg, "reg"},
	{opt_lookup_perf, "lookup_perf"},
	{opt_tbl_sz, "tbl_sz=%u"},
	{opt_lookup_sz, "lookup_sz=%u"},
	{opt_lookup_sz, "lookup_sz=%u"},
	{opt_mcam_idx, "mcam_idx=%u"},
	{opt_qidx, "qidx=%u"},
	{opt_err, NULL},
};

enum inject_point {
	INJECT_POINT_HOOK,
	INJECT_POINT_STACK,
};

#define MAX_LOOKUP_SIZE 64
static struct  pan_tuple stuple[MAX_LOOKUP_SIZE];

struct pan_test_cmd_params {
	int hook, ifindex;
	struct sk_buff *skb;
	unsigned int lookup_sz;
	unsigned int tbl_sz;
	unsigned int mcam_idx;
	unsigned int qidx;
};

static int pan_test_cmd_parse(char *options, u64 *res,
			      struct pan_test_cmd_params *params)
{
	enum pkt_type type = PKT_TYPE_V4;
	substring_t args[MAX_OPT_ARGS];
	struct net_device *dev;
	char hook_or_stack[32];
	char ifname[IFNAMSIZ];
	char *p, *last, len;
	int inj_pt = 0;
	u64 cmd = 0;

	if (!options)
		return 1;

	options[strcspn(options, "\r\n")] = 0;
	last = strrchr(options, ' ');

	while ((p = strsep(&options, " ,")) != NULL) {
		int token, n;

		if (last + 1 == p) {
			len = strlen(p);
			p[len - 1] = '\0';
			p[len - 2] = '\0';
			p[len - 3] = '\0';
		}

		if (!*p)
			continue;

		token = match_token(p, tokens, args);
		switch (token) {
		case opt_cmd_tuple:
		case opt_cmd_ingress:
		case opt_cmd_egress:
		case opt_cmd_modify_mcam:
		case opt_add:
		case opt_del:
		case opt_find:
		case opt_reg:
		case opt_lookup_perf:
			cmd |= BIT_ULL(token);
			break;

		case opt_mcam_idx:
			cmd |= BIT_ULL(token);
			if (match_uint(&args[0], &n)) {
				pr_err("Please enter valid number for mcam_idx\n");
				return -EINVAL;
			}
			params->mcam_idx = n;
			break;

		case opt_qidx:
			cmd |= BIT_ULL(token);
			if (match_uint(&args[0], &n)) {
				pr_err("Please enter valid number for qidx\n");
				return -EINVAL;
			}
			params->qidx = n;
			break;

		case opt_lookup_sz:
			cmd |= BIT_ULL(token);
			if (match_uint(&args[0], &n)) {
				pr_err("Please enter valid number for hook\n");
				return -EINVAL;
			}
			params->lookup_sz = n;
			break;

		case opt_tbl_sz:
			cmd |= BIT_ULL(token);
			if (match_uint(&args[0], &n)) {
				pr_err("Please enter valid number for hook\n");
				return -EINVAL;
			}
			params->tbl_sz = n;
			break;

		case opt_hooknum:
			cmd |= BIT_ULL(token);
			if (match_uint(&args[0], &n)) {
				pr_err("Please enter valid number for hook\n");
				return -EINVAL;
			}
			params->hook = n;
			break;

		case opt_dev:
			cmd |= BIT_ULL(token);
			if (!match_strlcpy(ifname, &args[0], sizeof(ifname))) {
				pr_err("Please enter valid netdev\n");
				return -EINVAL;
			}

			dev = dev_get_by_name(&init_net, ifname);
			if (!dev) {
				pr_err("Please enter valid netdev=%s\n", ifname);
				return -EINVAL;
			}

			params->ifindex = dev->ifindex;
			dev_put(dev);
			break;

		case opt_v4:
			type = PKT_TYPE_V4;
			break;

		case opt_v6:
			type = PKT_TYPE_V6;
			break;

		case opt_inject:
			cmd |= BIT_ULL(token);
			if (!match_strlcpy(hook_or_stack, &args[0],
					   sizeof(hook_or_stack))) {
				pr_err("Please enter inject point (hook or stack)");
				return -EINVAL;
			}

			if (!strncmp(hook_or_stack, "hook", sizeof(hook_or_stack))) {
				inj_pt = INJECT_POINT_HOOK;
			} else if (!strncmp(hook_or_stack, "stack",
					    sizeof(hook_or_stack))) {
				inj_pt = INJECT_POINT_STACK;
				params->hook = -1;
			} else {
				pr_err("Please specify proper inject point(hook/stack)\n");
				return -EINVAL;
			}

			if (type == PKT_TYPE_V4) {
				params->skb = pan_test_v4_skb_build();
			} else if (type == PKT_TYPE_V6) {
				params->skb = pan_test_v6_skb_build();
			} else {
				pr_err("Can Test only v4 or v6 packet inject\n");
				return -EINVAL;
			}

			pr_debug("Inject pt value is %d\n", inj_pt);

			break;

		default:
			pr_debug("Unrecognized option %s\n", p);
		}
	}
	*res = cmd;
	return 0;
}

static int pan_test_cb_ingress(struct pan_tuple *tuple, struct sk_buff *skb, void *arg)
{
	pr_info("ingress cb called\n");
	PAN_TUPLE_DUMP(tuple);
	return NF_ACCEPT;
}

static int pan_test_cb_egress(struct pan_tuple *tuple, struct sk_buff *skb, void *arg)
{
	pr_info("egress cb called\n");
	PAN_TUPLE_DUMP(tuple);
	return NF_ACCEPT;
}

static int pan_test_modify_cam_action(u16 pcifunc, int mcam_idx, int qidx)
{
	u64 af_const, af_const2;
	u16 pf_func, idx, op, bank;
	unsigned long long *addr;
	struct pci_dev *pci_dev;
	int depth, banks, width;
	unsigned long long r;
	struct rvu *rvu;
	void __iomem  *io;
	int index;
	u64 reg;
	u8 keyw;

	u64 blk = (6ULL << 28);

	/* TODO: fix in case of DPDK where netdevice is not in kernel */
	pci_dev = pci_get_device(0x177d, 0xA065, NULL);
	if (!pci_dev) {
		pr_err("Could not find pci_dev\n");
		return -1;
	}

	/* TODO: won't work if driver data is not set
	 */
	rvu = (struct rvu *)pci_dev->dev.driver_data;
	if (!rvu) {
		pr_err("Rvu is null\n");
		return -1;
	}

	addr = (u64 *)rvu->afreg_base;

	addr = (u64 *)(((u64)rvu->afreg_base + blk) | NPC_AF_CONST);
	io = (void __iomem    *)addr;
	r = readq(io);
	af_const = r;

	addr = (u64 *)(((u64)rvu->afreg_base + blk) | NPC_AF_CONST2);
	io = (void __iomem    *)addr;
	r = readq(io);
	af_const2 = r;

	banks = FIELD_GET(GENMASK_ULL(47, 44), af_const);
	width = FIELD_GET(GENMASK_ULL(25, 16), af_const);

	depth = FIELD_GET(GENMASK_ULL(15, 0), af_const2);
	idx = mcam_idx % depth;

	reg = rvu_read64(rvu, 6, NPC_AF_INTFX_KEX_CFG(0));
	keyw = FIELD_GET(GENMASK_ULL(34, 32), reg);

	/* keywith 0=116, 1=228, 2=452: */
	/* If keyw == 228, bank 0 and 2 are selected for intf and actions */

	switch (keyw) {
	case 0:
		bank = mcam_idx / depth;
		break;
	case 1:
		bank = (mcam_idx / depth == 1) ? 2 : 0;
		break;
	case 2:
		bank = 0;
		break;
	}

	pr_err("Index=%u depth=%u total_banks=%u width=%u keyw=%u bank=%u\n",
	       idx, depth, banks, width, keyw, bank);

	reg = rvu_read64(rvu, 6, NPC_AF_MCAMEX_BANKX_ACTION(idx, bank));

	/* NIX_RX_ACTION_S 19:4 pf_func, 39:20 index */
	pf_func = FIELD_GET(GENMASK_ULL(19, 4), reg);
	index = FIELD_GET(GENMASK_ULL(39, 20), reg);
	op = FIELD_GET(GENMASK_ULL(3, 0), op);

	reg &= ~(GENMASK_ULL(19, 4) | GENMASK_ULL(39, 20) | GENMASK_ULL(3, 0));

	reg |= FIELD_PREP(GENMASK(19, 4), pcifunc);
	reg |= FIELD_PREP(GENMASK(39, 20), qidx);
	reg |= FIELD_PREP(GENMASK(3, 0), 1);	// Set to UCAST

	pr_err("Modified pcifunc: 0x%x--> 0x%x qidx:0x%x -> 0x%x op:0x%x -> 1\n",
	       pf_func, pcifunc, index, qidx, op);

	rvu_write64(rvu, 6, NPC_AF_MCAMEX_BANKX_ACTION(idx, bank), reg);

	return 0;
}

static int pan_test_cmd_execute(u64 cmd, struct pan_test_cmd_params *params)
{
	u8 smac[ETH_ALEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
	u8 dmac[ETH_ALEN] = {0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
	unsigned int lookup_sz, tbl_sz;
	struct pan_rvu_dev_priv *pan_priv;
	ktime_t start, end, diff;
	struct net_device *dev;
	struct pan_tuple *t[10];
	int max, min;
	int fail_cnt = 0;
	u16 pcifunc;
	int rc = 0;
	int i, j;

	pr_info("Cmd = %#llx\n", cmd);

	lookup_sz = params->lookup_sz;
	tbl_sz = params->tbl_sz;

	switch (cmd) {
	case pan_test_cmd_tuple_add:

		PAN_TUPLE_DUMP(tuple);
		rc = pan_fl_tbl_add(tuple, &tn.res, NULL);
		break;

	case pan_test_cmd_tuple_del:
		PAN_TUPLE_DUMP(tuple);
		rc = pan_fl_tbl_del(tuple);
		break;

	case pan_test_cmd_tuple_find:
		PAN_TUPLE_DUMP(tuple);
		rc = pan_fl_tbl_lookup(tuple);
		break;

	case pan_test_cmd_tuple_reset:
		PAN_TUPLE_DUMP(tuple);
		memset(tuple, 0, sizeof(struct pan_tuple));
		/* Only l2 fields ends up in v4 table */
		tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;
		break;

	case pan_test_cmd_ingress_reg:
		PAN_TUPLE_DUMP(tuple);
		rc = pan_fl_tbl_lookup(tuple);
		if (rc) {
			pr_err("Could not find tuple for ingress reg\n");
			return -EINVAL;
		}

		rc = pan_fl_tbl_register_ig_cb(tuple, pan_test_cb_ingress, NULL);
		if (rc) {
			pr_err("ingress reg failed %d\n", rc);
			return -EINVAL;
		}
		break;

	case pan_test_cmd_redirect2pan:
		dev = dev_get_by_name(&init_net, PAN_DEV_NAME);
		if (!dev) {
			pr_err("Could not find PAN device\n");
			return -EFAULT;
		}
		dev_put(dev);
		pan_priv = netdev_priv(dev);
		pcifunc = pan_priv->pcifunc;

		pan_test_modify_cam_action(pcifunc, params->mcam_idx, params->qidx);
		break;

	case pan_test_cmd_egress_reg:
		PAN_TUPLE_DUMP(tuple);
		rc = pan_fl_tbl_lookup(tuple);
		if (rc) {
			pr_err("Could not find tuple for egress reg\n");
			return -EINVAL;
		}

		rc = pan_fl_tbl_register_eg_cb(tuple, pan_test_cb_egress, NULL);
		if (rc) {
			pr_err("egress reg failed %d\n", rc);
			return -EINVAL;
		}
		break;

	case pan_test_cmd_lookup_perf:
		if (!lookup_sz || !tbl_sz) {
			pr_err("tbl_sz or lookup_sz can't be NULL\n");
			return -EINVAL;
		}

		if (lookup_sz > MAX_LOOKUP_SIZE) {
			pr_err("Maximum supported search size is 64\n");
			return -EFAULT;
		}

		memset(&stuple, 0, sizeof(stuple));
		min = 1;
		max = tbl_sz + 1;

		ether_addr_copy(tuple->smac, smac);
		ether_addr_copy(tuple->dmac, dmac);
		tuple->l3proto = htons(ETH_P_IP);
		tuple->src_ip4.s_addr = in_aton("192.168.8.100");
		tuple->dst_ip4.s_addr = in_aton("192.168.8.200");
		tuple->sport = htons(1000);
		tuple->dport = htons(2000);

		tuple->l4proto = IPPROTO_UDP;
		tuple->flags = PAN_TUPLE_FLAGS_L3_IPV4;

		for (i = 0; i < tbl_sz; i++) {
			tuple->sport = htons(min + i);
			tuple->dport = htons(max - i);
			rc = pan_fl_tbl_add(tuple, &tn.res, NULL);
			if (rc) {
				pr_err("Tuple addition failed %d\n", i);
				PAN_TUPLE_DUMP(tuple);
			}

			if (i < 10)
				t[i] = tuple;
		}

		for (i = 0; i < lookup_sz; i++) {
			u16 s, d;

			if (i < 10) {
				stuple[i] = *t[i];
				continue;
			}

			stuple[i] = *tuple;
			get_random_bytes(&s, sizeof(s));
			get_random_bytes(&d, sizeof(d));
			stuple[i].sport = s % (tbl_sz + (tbl_sz / 2));
			stuple[i].dport = d % (tbl_sz + (tbl_sz / 2));
		}

		pr_info("Test starting\n");
		start = ktime_get_real();

#define REPEAT 128
		for (j = 0; j < REPEAT; j++)
		for (i = 0; i < lookup_sz; i++)
			fail_cnt += !!pan_fl_tbl_lookup(&stuple[i]);

		end = ktime_get_real();
		diff = ktime_sub(end, start);

		pr_info("Total time taken to search %u (%u times) in tbl sz=%u fail=%u\n",
			lookup_sz, REPEAT, tbl_sz, fail_cnt);

		pr_info("diff = %llu , time/search = %llu ns\n",
			ktime_to_ns(diff),
			ktime_to_ns(diff) / ((lookup_sz * REPEAT)));

		pr_info("start_time=%llu end_time=%llu\n", ktime_to_ns(start), ktime_to_ns(end));

		rc = 0;
		break;

	default:
		pr_err("Invalid command request %#llx\n", cmd);
		return -EINVAL;
	}

	return rc;
}

static ssize_t pan_test_cmd_write(struct file *file, const char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	struct pan_test_cmd_params params;
	char buf[64];
	u64 cmd = 0;
	int len;

	len = simple_write_to_buffer(buf, sizeof(buf), ppos, user_buf, count);

	if (pan_test_cmd_parse(buf, &cmd, &params)) {
		pr_err("Command parsing failed cmd=%s\n", buf);
		return -EFAULT;
	}

	if (pan_test_cmd_execute(cmd, &params)) {
		pr_err("Cmd execution failed cmd=%s (0x%llx)\n", buf, cmd);
		return -EFAULT;
	}

	pr_info("Command executed successfully\n");

	return len;
}

static ssize_t pan_test_cmd_read(struct file *file, char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	int len = 0;
	char buf[1024];

	len += snprintf(buf, sizeof(buf), "%s",
		       "1. tuple add\n2. tuple del\n3. tuple find\n");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations pan_test_cmd_ops = {
	.open           = simple_open,
	.write		= pan_test_cmd_write,
	.read		= pan_test_cmd_read,
};

int pan_test_rx_sock(struct socket *sock, struct sockaddr_in *addr,
		     unsigned char *buf, int len)
{
	struct msghdr msg = { };
	struct kvec iov;
	int sz;

	iov.iov_base = buf;
	iov.iov_len = len;
	sz = kernel_recvmsg(sock, &msg, &iov, 1,
			    iov.iov_len, msg.msg_flags);
	return sz;
}

static struct dentry *pan_test_debugfs_dir(void)
{
	struct dentry *parent;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("%s", "Could not find dir cn10ka or octeontx2 in debugfs\n");
		return NULL;
	}

	/* pan fl tbl would be initialized before pan rvu */
	parent = debugfs_lookup("pan", parent);
	if (!parent)
		return NULL;

	return parent;
}

void pan_test_deinit(void)
{
	struct dentry *parent;

	parent = pan_test_debugfs_dir();
	if (!parent)
		return;

	debugfs_remove(parent);
}

int pan_test_init(void)
{
	struct dentry *parent;
	struct dentry *file;
	char *name;
	int ret;

	tuple = &tn.tuple;
	tuple->flags = PAN_TUPLE_FLAG_L3_PROTO_V4;

	parent = debugfs_lookup("cn10k", NULL);
	if (!parent)
		parent = debugfs_lookup("octeontx2", NULL);

	if (!parent) {
		pr_err("%s", "Could not find dir cn10ka or octeontx2 in debugfs\n");
		return -ESRCH;
	}

	parent = debugfs_lookup("pan", parent);
	if (!parent) {
		pr_err("Could not find pan debugfs directory\n");
		return -ESRCH;
	}

	parent = debugfs_create_dir("test", parent);
	if (!parent) {
		pr_err("Could not create test directory\n");
		return -EFAULT;
	}

	ret = pan_test_debugfs_simple_node_create(parent, "sport");
	if (ret) {
		pr_err("Failed to create sport debugfs node");
		return -EFAULT;
	}

	ret = pan_test_debugfs_simple_node_create(parent, "dport");
	if (ret) {
		pr_err("Failed to create dport debugfs node");
		return -EFAULT;
	}

	file = debugfs_create_file("l3proto", 0600, parent, "l3proto",
				   &pan_test_simple_llx_fops);
	if (!file) {
		pr_err("Failed to create sport debugfs node");
		return -EFAULT;
	}

	file = debugfs_create_file("pcifunc", 0600, parent, "pcifunc",
				   &pan_test_simple_llx_fops);
	if (!file) {
		pr_err("Failed to create sport debugfs node");
		return -EFAULT;
	}

	ret = pan_test_debugfs_simple_node_create(parent, "l4proto");
	if (ret) {
		pr_err("Failed to create l4proto debugfs node");
		return -EFAULT;
	}

	name = "src_ip";
	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_ip_ops);

	if (!file) {
		pr_err("Failed to create src_ip debugfs node");
		return -EFAULT;
	}

	name = "dst_ip";
	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_ip_ops);
	if (!file) {
		pr_err("Failed to create dst_ip debugfs node");
		return -EFAULT;
	}

	name = "smac";
	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_mac_ops);
	if (!file) {
		pr_err("Failed to create smac debugfs node");
		return -EFAULT;
	}

	name = "dmac";
	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_mac_ops);
	if (!file) {
		pr_err("Failed to create dmac debugfs node");
		return -EFAULT;
	}

	name = "cmd";
	file = debugfs_create_file(name, 0600, parent, name,
				   &pan_test_cmd_ops);
	if (!file) {
		pr_err("Failed to create cmd debugfs node");
		return -EFAULT;
	}

	return 0;
}
