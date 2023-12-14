// SPDX-License-Identifier: GPL-2.0:
/* Marvell RVU Admin Function driver
 *
 * Copyright (C) 2025 Marvell.
 *
 */
#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/errno.h>
#include <linux/if_vlan.h>
#include <linux/if_pppox.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <net/ip.h>

#include "pan_tuple.h"
#include "pan_parse_buf.h"

static int pan_parse_buf_l2(u8 *data, struct pan_tuple *tuple)
{
	struct ethhdr *eth;

	eth = (struct ethhdr *)data;
	/* TODO: check if we have ethhdr */

	ether_addr_copy(tuple->smac, eth->h_source);
	ether_addr_copy(tuple->dmac, eth->h_dest);
	tuple->l3proto = eth->h_proto;
	tuple->flags |= PAN_TUPLE_FLAGS_L2;

	/* TODO: handle vlan */
	return 0;
}

static int pan_parse_buf_ipv6(u8 **ptr, struct pan_tuple *tuple)
{
	struct ipv6hdr *ip6h;
	struct tcphdr *th;
	struct udphdr *uh;

	/* TODO: check we can pull ipv6 hdr */

	ip6h = (struct ipv6hdr *)ptr;

	memcpy(&tuple->src_ip6, &ip6h->saddr, sizeof(tuple->src_ip6));
	memcpy(&tuple->dst_ip6, &ip6h->daddr, sizeof(tuple->dst_ip6));
	tuple->flags |= PAN_TUPLE_FLAGS_L3_IPV6;

	tuple->l4proto = ip6h->nexthdr;

	*ptr += sizeof(struct ipv6hdr);
	switch (ip6h->nexthdr) {
	case IPPROTO_TCP:
		/* TODO: can we pull tcp hdr ? */
		th = (struct tcphdr *)ptr;
		tuple->sport = th->source;
		tuple->dport = th->dest;
		tuple->flags |= PAN_TUPLE_FLAGS_L4_TCP;
		break;
	case IPPROTO_UDP:
		/* TODO: can we pull udp hdr ? */
		uh = (struct udphdr *)ptr;
		tuple->sport = uh->source;
		tuple->dport = uh->dest;
		tuple->flags |= PAN_TUPLE_FLAGS_L4_UDP;
		break;
	default:
		break;
	}

	return 0;
}

static int pan_parse_buf_ipv4(u8 **ptr, struct pan_tuple *tuple)
{
	struct tcphdr *th;
	struct udphdr *uh;
	struct iphdr *iph;
	u8 ipproto;

	/* TODO: check we have header size available */

	iph = (struct iphdr *)*ptr;

	if (ip_is_fragment(iph) ||
	    unlikely((iph->ihl * 4) != sizeof(*iph))) /* IP has options */
		return 0; // since we processed L2.

	tuple->src_ip4.s_addr = iph->saddr;
	tuple->dst_ip4.s_addr = iph->daddr;
	tuple->flags |= PAN_TUPLE_FLAGS_L3_IPV4;

	ipproto = iph->protocol;
	tuple->l4proto = ipproto;

	*ptr += sizeof(*iph);
	switch (ipproto) {
	case IPPROTO_TCP:
		/* TODO: we can pull tcp header ? */

		th = (struct tcphdr *)*ptr;
		tuple->sport = th->source;
		tuple->dport = th->dest;
		tuple->flags |= PAN_TUPLE_FLAGS_L4_TCP;
		break;
	case IPPROTO_UDP:
		/* TODO: we can pull UDP header ? */
		uh = (struct udphdr *)*ptr;
		tuple->sport = uh->source;
		tuple->dport = uh->dest;
		tuple->flags |= PAN_TUPLE_FLAGS_L4_UDP;
		break;
	default:
		break;
	}

	return 0;
}

int pan_parse_buf(u8 *data, struct pan_tuple *tuple, struct pan_tuple_hdr *hdr)
{
	struct ethhdr *eth;
	struct iphdr *iph;
	u8 *ptr;
	int rc;

	eth = (struct ethhdr *)data;

	switch (eth->h_proto) {
	case htons(ETH_P_8021Q):
	case htons(ETH_P_PPP_SES):
	case htons(ETH_P_IP):
	case htons(ETH_P_IPV6):
		break;

	default:
		pr_debug("packet protocol cannot be handled (%#x)\n", ntohs(eth->h_proto));
		return -1;
	}

	ptr = data;
	rc = pan_parse_buf_l2(data, tuple);
	if (rc) {
		pr_err("Error in parsing l2 header err=%u\n", rc);
		return rc;
	}

	hdr->l2hdr = ptr;

	tuple->flags |= PAN_TUPLE_FLAGS_L2;

	ptr += sizeof(*eth);
	hdr->l3hdr = ptr;

	iph = (struct iphdr *)ptr;
	switch (iph->version) {
	case IPVERSION:
		rc = pan_parse_buf_ipv4(&ptr, tuple);
		break;
	case 6:
		rc = pan_parse_buf_ipv6(&ptr, tuple);
		break;
	default:
		pr_err("IP version (%u) is not v4 or v6(ret=%u)\n",  iph->version, rc);
		return 0;
	}

	hdr->l4hdr = ptr;
	return 0;
}
