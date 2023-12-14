/* SPDX-License-Identifier: GPL-2.0 */
/* Marvell PAN driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#ifndef PAN_TUPLE_H_
#define PAN_TUPLE_H_

#include <linux/in.h>
#include <linux/in6.h>
#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>

#define PAN_TUPLE_ENCAP_MAX 2

#ifdef PAN_TUPLE_DEBUG
#define PAN_TUPLE_DUMP pan_tuple_dump2console
#else
#define PAN_TUPLE_DUMP(...)
#endif

enum pan_tuple_flag {
	PAN_TUPLE_FLAG_IFF		= BIT_ULL(0),

	PAN_TUPLE_FLAG_L2_SRC_MAC	= BIT_ULL(1),
	PAN_TUPLE_FLAG_L2_DST_MAC	= BIT_ULL(2),
	PAN_TUPLE_FLAG_L2_PROTO		= BIT_ULL(3),
	PAN_TUPLE_FLAG_L2_VLAN		= BIT_ULL(4),
	PAN_TUPLE_FLAG_L2_DOUBLE_VLAN	= BIT_ULL(5),
	PAN_TUPLE_FLAG_L2_PPP		= BIT_ULL(6),

	PAN_TUPLE_FLAG_L3_SRC_IP	= BIT_ULL(7),
	PAN_TUPLE_FLAG_L3_DST_IP	= BIT_ULL(8),
	PAN_TUPLE_FLAG_L3_PROTO		= BIT_ULL(9),
	PAN_TUPLE_FLAG_L3_PROTO_V4	= BIT_ULL(10),
	PAN_TUPLE_FLAG_L3_PROTO_V6	= BIT_ULL(11),

	PAN_TUPLE_FLAG_L4_PROTO		= BIT_ULL(12),
	PAN_TUPLE_FLAG_L4_PROTO_TCP	= BIT_ULL(13),
	PAN_TUPLE_FLAG_L4_PROTO_UDP	= BIT_ULL(14),
	PAN_TUPLE_FLAG_L4_SRC_PORT	= BIT_ULL(15),
	PAN_TUPLE_FLAG_L4_DST_PORT	= BIT_ULL(16),

	PAN_TUPLE_FLAG_ENCAP_HDR1	= BIT_ULL(17),
	PAN_TUPLE_FLAG_ENCAP_HDR2	= BIT_ULL(18),
	PAN_TUPLE_FLAG_HASH		= BIT_ULL(19),
	PAN_TUPLE_FLAG_MAX,
};

#define PAN_TUPLE_FLAGS_L2  \
	(PAN_TUPLE_FLAG_L2_PROTO | \
	PAN_TUPLE_FLAG_L2_SRC_MAC | \
	PAN_TUPLE_FLAG_L2_DST_MAC)

#define PAN_TUPLE_FLAGS_L3  \
	(PAN_TUPLE_FLAG_L3_SRC_IP | \
	PAN_TUPLE_FLAG_L3_DST_IP | \
	PAN_TUPLE_FLAG_L3_PROTO)

#define PAN_TUPLE_FLAGS_L3_IPV4 \
	(PAN_TUPLE_FLAGS_L3 | PAN_TUPLE_FLAG_L3_PROTO_V4)

#define PAN_TUPLE_FLAGS_L3_IPV6 \
	(PAN_TUPLE_FLAGS_L3 | PAN_TUPLE_FLAG_L3_PROTO_V6)

#define PAN_TUPLE_FLAGS_L4 \
	(PAN_TUPLE_FLAG_L4_PROTO | \
	 PAN_TUPLE_FLAG_L4_SRC_PORT | \
	 PAN_TUPLE_FLAG_L4_DST_PORT)

#define PAN_TUPLE_FLAGS_L4_TCP \
	(PAN_TUPLE_FLAGS_L4 | PAN_TUPLE_FLAG_L4_PROTO_TCP)

#define PAN_TUPLE_FLAGS_L4_UDP (PAN_TUPLE_FLAGS_L4 | \
			PAN_TUPLE_FLAG_L4_PROTO_UDP)

enum pan_tuple_dir {
	FLOW_OFFLOAD_DIR_ORIGINAL = IP_CT_DIR_ORIGINAL,
	FLOW_OFFLOAD_DIR_REPLY = IP_CT_DIR_REPLY,
};

struct pan_tuple_hdr {
	u8 *l2hdr;
	u8 *l3hdr;
	u8 *l4hdr;
	u64 flags;
};

struct pan_tuple {
	union {
		struct in_addr		src_ip4;	//Big Endian
		struct in6_addr		src_ip6;	//Big Endian
	};
	union {
		struct in_addr		dst_ip4;	//Big endian
		struct in6_addr		dst_ip6;	//Big endian
	};

	struct {
		__be16			sport;		//Big endian
		__be16			dport;		//Big endian
	};

	u8				l4proto;
	u16				l3proto;

	u8				smac[ETH_ALEN];	//Big endian
	u8				dmac[ETH_ALEN];	//Big endian
	struct { } __end;		// Packet tuple ends.

	u64 flags;
	u32 hash;
};

static inline bool
pan_tuple_flags_is_set(const struct pan_tuple *t, enum pan_tuple_flag flag)
{
	return !!(t->flags & flag);
}

static inline void
pan_tuple_flags_set(struct pan_tuple *t, enum pan_tuple_flag flag)
{
	t->flags |= flag;
}

static inline void
pan_tuple_flags_clear(struct pan_tuple *t, enum pan_tuple_flag flag)
{
	t->flags &= ~flag;
}

static inline void pan_tuple_flags_reset(struct pan_tuple *t)
{
	t->flags = 0;
}

static inline u64 pan_tuple_flags_get(struct pan_tuple *t)
{
	return t->flags;
}

static inline u32 pan_tuple_hash_get(const struct pan_tuple *t)
{
	return t->hash;
}

static inline void pan_tuple_hash_set(struct pan_tuple *t, u32 hash)
{
	t->hash = hash;
	t->flags |= PAN_TUPLE_FLAG_HASH;
}

void pan_tuple_dump2sysfs(struct seq_file *m, struct pan_tuple *tuple,
			  int index);
void pan_tuple_dump2console(struct pan_tuple *tuple);

#endif // PAN_TUPLE_H_
