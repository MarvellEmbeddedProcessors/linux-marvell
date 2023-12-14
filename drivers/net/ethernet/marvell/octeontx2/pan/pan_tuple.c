// SPDX-License-Identifier: GPL-2.0
/* Marvell RVU PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/seq_file.h>

#include "pan_tuple.h"

enum pan_tuple_fld_type {
	PAN_TUPLE_FLD_TYPE_SIP4,
	PAN_TUPLE_FLD_TYPE_DIP4,
	PAN_TUPLE_FLD_TYPE_SIP6,
	PAN_TUPLE_FLD_TYPE_DIP6,
	PAN_TUPLE_FLD_TYPE_SPORT,
	PAN_TUPLE_FLD_TYPE_DPORT,
	PAN_TUPLE_FLD_TYPE_IFINDEX,
	PAN_TUPLE_FLD_TYPE_SMAC,
	PAN_TUPLE_FLD_TYPE_DMAC,
	PAN_TUPLE_FLD_TYPE_L4PROTO,
	PAN_TUPLE_FLD_TYPE_L3PROTO,
	PAN_TUPLE_FLD_TYPE_MAX,
};

void pan_tuple_dump2console(struct pan_tuple *tuple)
{
	bool v4, v6;
	u64 flags;

	flags = tuple->flags;
	if (!flags) {
		pr_info("Tuple is not populated\n");
		return;
	}

	v4 = !!(flags & PAN_TUPLE_FLAG_L3_PROTO_V4);
	v6 = !!(flags & PAN_TUPLE_FLAG_L3_PROTO_V6);

	if (!v4 && !v6)
		pr_info("not v4 or v6 tuple\n");

	pr_info("(smac %pM),", tuple->smac);
	pr_info("(dmac %pM),", tuple->dmac);

	if (v4) {
		pr_info("(sip %pI4b),", &tuple->src_ip4);
		pr_info("(dip %pI4b),", &tuple->dst_ip4);
	} else {
		pr_info("(sip %pI6b),", &tuple->src_ip6);
		pr_info("(dip %pI6b),", &tuple->dst_ip6);
	}

	pr_info("(sport %u),", ntohs(tuple->sport));
	pr_info("(dport %u),", ntohs(tuple->dport));

	pr_info("(l3proto 0x%x),", ntohs(tuple->l3proto));
	pr_info("(l4proto %u),", tuple->l4proto);

	pr_info("\n");
}

void pan_tuple_dump2sysfs(struct seq_file *m, struct pan_tuple *tuple,
			  int index)
{
	bool v4, v6;
	u64 flags;

	seq_printf(m, "%u: ", index);

	flags = tuple->flags;
	if (!flags) {
		seq_puts(m, "Tuple is not populated\n");
		return;
	}

	v4 = !!(flags & PAN_TUPLE_FLAG_L3_PROTO_V4);
	v6 = !!(flags & PAN_TUPLE_FLAG_L3_PROTO_V6);

	if (!v4 && !v6) {
		seq_puts(m, "not v4 or v6 tuple\n");
		return;
	}

	seq_printf(m, "(smac %pM),", tuple->smac);
	seq_printf(m, "(dmac %pM),", tuple->dmac);

	if (v4) {
		seq_printf(m, "(sip %pI4b),", &tuple->src_ip4);
		seq_printf(m, "(dip %pI4b),", &tuple->dst_ip4);
	} else {
		seq_printf(m, "(sip %pI6b),", &tuple->src_ip6);
		seq_printf(m, "(dip %pI6b),", &tuple->dst_ip6);
	}

	seq_printf(m, "(sport %u),", ntohs(tuple->sport));
	seq_printf(m, "(dport %u),", ntohs(tuple->dport));

	seq_printf(m, "(l3proto 0x%x),", ntohs(tuple->l3proto));
	seq_printf(m, "(l4proto %u),", tuple->l4proto);
}
