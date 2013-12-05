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

#include "mvOs.h"
#include "mvCommon.h"
#include "mv802_3.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#include "gbe/mvNetaRegs.h"
#include "gbe/mvEthRegs.h"

#include "mvPnc.h"
#include "mvTcam.h"

/*
 * PNC debug
 */
/*#define PNC_DBG mvOsPrintf*/
#define PNC_DBG(X...)

/*
 * PNC errors
 */
#define PNC_ERR mvOsPrintf
/*#define PNC_ERR(X...)*/

/*
 * Local variables
 */
static int   pnc_inited = 0;
static int rxq_mac_bc = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_mac_mc = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_vlan = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_ip6 = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_ip4 = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_ip4_tcp = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_ip4_udp = CONFIG_MV_ETH_RXQ_DEF;
static int rxq_arp 		= CONFIG_MV_ETH_RXQ_DEF;


#ifdef CONFIG_ARCH_FEROCEON_KW2
int pnc_port_map(int pnc_port)
{
	switch (pnc_port) {
	case 2:
		return 0;

	case 4:
		return 1;

	case 0:
		return 2;

	default:
		mvOsPrintf("%s: pnc_port=%d is out of range\n", __func__, pnc_port);
		return -1;
	}
}

int pnc_eth_port_map(int eth_port)
{
	switch (eth_port) {
	case 0:
		return 2;

	case 1:
		return 4;

	case 2:
		return 0;

	default:
		mvOsPrintf("%s: eth_port=%d is out of range\n", __func__, eth_port);
		return -1;
	}
}
#else /* CONFIG_ARCH_ARMADA_XP */
int pnc_port_map(int pnc_port)
{
	switch (pnc_port) {
	case 0:
	case 1:
		return 0;

	case 4:
		return 1;

	case 2:
		return 2;

	case 3:
		return 3;

	default:
		mvOsPrintf("%s: pnc_port=%d is out of range\n", __func__, pnc_port);
		return -1;
	}
}

int pnc_eth_port_map(int eth_port)
{
	switch (eth_port) {
	case 0:
		return 0;

	case 1:
		return 4;

	case 2:
		return 2;

	case 3:
		return 3;

	default:
		mvOsPrintf("%s: eth_port=%d is out of range\n", __func__, eth_port);
		return -1;
	}
}
#endif /* MV_ETH_PNC_NEW */

int pnc_te_del(unsigned int tid)
{
	PNC_DBG("%s [%d]\n", __func__, tid);

	tcam_hw_inv(tid);

	return 0;
}

/* pnc port setting: data: 0 for all bits, mask: 0 - for accepted ports, 1 - for rejected ports */
int pnc_port_mask_check(unsigned int mask, int eth_port)
{
	int pnc_port = pnc_eth_port_map(eth_port);

	if (pnc_port < 0)
		return 0;

	if (mask & (1 << pnc_port))
		return 0;

	return 1;
}

unsigned int pnc_port_mask_update(unsigned int mask, int eth_port, int add)
{
	int pnc_port = pnc_eth_port_map(eth_port);

	if (pnc_port < 0)
		return mask;

	if (add)
		mask &= ~(1 << pnc_port);
	else
		mask |= (1 << pnc_port);

	return mask;
}

unsigned int pnc_port_mask(int eth_port)
{
	unsigned int mask;
	int pnc_port = pnc_eth_port_map(eth_port);

	if (pnc_port < 0)
		return 0;

	mask = (~(1 << pnc_port)) & PORT_MASK;
	return mask;
}

/* Get TCAM entry if valid, NULL if invalid */
struct tcam_entry *pnc_tcam_entry_get(int tid)
{
	struct tcam_entry *te;

	te = tcam_sw_alloc(0);

	tcam_hw_read(te, tid);

	if (te->ctrl.flags & TCAM_F_INV) {
		tcam_sw_free(te);
		return NULL;
	}
	return te;
}

int pnc_tcam_port_update(int tid, int eth_port, int add)
{
	struct tcam_entry *te;
	unsigned int data, mask;

	te = pnc_tcam_entry_get(tid);
	if (te == NULL) {
		mvOsPrintf("%s: TCAM entry #%d is invalid\n", __func__, tid);
		return -1;
	}
	tcam_sw_get_port(te, &data, &mask);
	mask = pnc_port_mask_update(mask, eth_port, add);
	tcam_sw_set_port(te, data, mask);
	tcam_hw_write(te, tid);
	tcam_sw_free(te);

	return 0;
}

/******************************************************************************
 *
 * MAC Address Section
 *
 ******************************************************************************
 */

/*
 * pnc_mac_fc_drop - Add Flow Control MAC address match rule to the MAC section
 * to drop PAUSE frames arriving without Marvell Header on all ports
 */
static void pnc_mac_fc_drop(void)
{
	struct tcam_entry *te = NULL;
	unsigned char da[6] = { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x01 };
	unsigned int len = MV_MAC_ADDR_SIZE;

	te = tcam_sw_alloc(TCAM_LU_MAC);

	/* set match on DA */
	while (len--)
		tcam_sw_set_byte(te, len, da[len]);

	/* port id match */
	tcam_sw_set_port(te, 0, 0);	/* all ports */

	/* result info bit */
	sram_sw_set_rinfo(te, RI_DROP, RI_DROP);

	tcam_sw_text(te, "flow control");
	sram_sw_set_lookup_done(te, 1);

	tcam_hw_write(te, TE_MAC_FLOW_CTRL);
	tcam_sw_free(te);
}

/*
 * pnc_mac_da - Add DA MAC address match rule to the MAC section
 * @da: destination MAC address
 * @len: destination MAC address length to match on: 0..6
 * @port_mask: source port id: 0..1F or ANY
 * @rxq: rx queue
 * @rinfo: result info bits to set
 */
static struct tcam_entry *pnc_mac_da(unsigned char *da, unsigned int len,
				     unsigned int port_mask, int rxq, unsigned int rinfo)
{
	struct tcam_entry *te = NULL;

	if (len > MV_MAC_ADDR_SIZE)
		goto out;

	if (rinfo >= BIT24)
		goto out;

	te = tcam_sw_alloc(TCAM_LU_MAC);

	/* set match on DA */
	while (len--)
		tcam_sw_set_byte(te, MV_ETH_MH_SIZE + len, da[len]);

	/* port id match */
	tcam_sw_set_port(te, 0, port_mask);

	/* result info bit */
	sram_sw_set_rinfo(te, rinfo, rinfo);

	/* set rx queue */
	sram_sw_set_rxq(te, rxq, 0);

	/* shift to ethertype */
	sram_sw_set_shift_update(te, 0, MV_ETH_MH_SIZE + 2 * MV_MAC_ADDR_SIZE);
	sram_sw_set_next_lookup(te, TCAM_LU_L2);
out:
	return te;
}

/*
 * pnc_mac_me - Add DA MAC address of port
 * @mac: destination MAC address or NULL for promiscuous
 * @port: ingress giga port number
 */
int pnc_mac_me(unsigned int port, unsigned char *mac, int rxq)
{
	struct tcam_entry *te;
	int len = MV_MAC_ADDR_SIZE;
	char text[TCAM_TEXT];
	unsigned int port_mask = pnc_port_mask(port);

	if (port_mask < 0)
		return 1;

	if (!mac)
		len = 0;

	te = pnc_mac_da(mac, len, port_mask, rxq, RI_DA_ME);
	sprintf(text, "%s%d", "ucast_me", port);
	tcam_sw_text(te, text);

	tcam_hw_write(te, TE_MAC_ME + port);
	tcam_sw_free(te);

	return 0;
}

/*
 * pnc_mcast_all - Accept all MAC multicast of port
 * @port: ingress giga port number.
 * @en: 1 - Accept ALL MCAST, 0 - Discard ALL MCAST
 */
int pnc_mcast_all(unsigned int port, int en)
{
	struct tcam_entry *te;
	unsigned int data, mask;

	te = pnc_tcam_entry_get(TE_MAC_MC_ALL);
	if (te == NULL) {
		mvOsPrintf("%s: MC_ALL entry (tid=%d) is invalid\n", __func__, TE_MAC_MC_ALL);
		return 1;
	}

	/* Update port mask */
	tcam_sw_get_port(te, &data, &mask);
	mask = pnc_port_mask_update(mask, port, en);

	tcam_sw_set_port(te, data, mask);
	tcam_sw_text(te, "mcast_all");

	tcam_hw_write(te, TE_MAC_MC_ALL);
	tcam_sw_free(te);

	return 0;
}

/*
 * pnc_mcast_me - Add DA MAC address of port
 * @mac: Multicast MAC DA or NULL to delete all Multicast DAs for this port
 * @port: ingress giga port number
 */
int pnc_mcast_me(unsigned int port, unsigned char *mac)
{
	struct tcam_entry *te;
	int tid, empty = -1;
	unsigned int data, mask;

	if (mac == NULL) {
		/* Delete all Multicast addresses for this port */
		for (tid = (TE_MAC_MC_ALL + 1); tid <= TE_MAC_MC_L; tid++) {
			/* Check TCAM entry */
			te = pnc_tcam_entry_get(tid);
			if (te != NULL) {
				/* delete entry if belong specific port */
				tcam_sw_get_port(te, &data, &mask);
				mask = pnc_port_mask_update(mask, port, 0);
				if (mask == PORT_MASK) {	/* No valid ports */
					tcam_hw_inv(tid);
				} else {
					tcam_sw_set_port(te, data, mask);
					tcam_hw_write(te, tid);
				}
				tcam_sw_free(te);
			}
		}
		return 0;
	}

	/* Add new Multicast DA for this port */
	for (tid = (TE_MAC_MC_ALL + 1); tid <= TE_MAC_MC_L; tid++) {
		te = pnc_tcam_entry_get(tid);

		/* Remember first Empty entry */
		if (te == NULL) {
			if (empty == -1)
				empty = tid;

			continue;
		}

		/* Find existing TCAM entry with this DA */
		if (tcam_sw_cmp_bytes(te, MV_ETH_MH_SIZE, MV_MAC_ADDR_SIZE, mac) == 0) {
			/* check and update port mask */
			tcam_sw_get_port(te, &data, &mask);
			mask = pnc_port_mask_update(mask, port, 1);
			tcam_sw_set_port(te, data, mask);

			tcam_hw_write(te, tid);
			tcam_sw_free(te);
			return 0;
		}
		tcam_sw_free(te);
	}

	/* Not found existing entry and no free TCAM entry - Failed */
	if (empty == -1)
		return 1;

	/* Not found existing entry - add to free TCAM entry */
	te = pnc_mac_da(mac, MV_MAC_ADDR_SIZE, pnc_port_mask(port), rxq_mac_mc, RI_DA_MC);
	tcam_sw_text(te, "mcast_me");

	tcam_hw_write(te, empty);
	tcam_sw_free(te);
	return 0;
}

/*
 * pnc_mac_init - GE init phase configuration
 */
static int pnc_mac_init(void)
{
	struct tcam_entry *te;
	unsigned char da_mc[6] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
	unsigned char da_bc[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

	PNC_DBG("%s\n", __func__);

	/* broadcast - for all ports */
	te = pnc_mac_da(da_bc, 6, 0, rxq_mac_bc, RI_DA_BC);
	tcam_sw_text(te, "bcast");

	tcam_hw_write(te, TE_MAC_BC);
	tcam_sw_free(te);

	/* flow control PAUSE frames - discard for all ports by default */
	pnc_mac_fc_drop();

	/* All Multicast - no ports by default */
	te = pnc_mac_da(da_mc, 1, PORT_MASK, rxq_mac_mc, RI_DA_MC);
	tcam_sw_text(te, "mcast_all");

	tcam_hw_write(te, TE_MAC_MC_ALL);
	tcam_sw_free(te);

	/* end of section */
	te = tcam_sw_alloc(TCAM_LU_MAC);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_MAC, FLOWID_CTRL_LOW_HALF_MASK);

	/* Non-promiscous mode - DROP unknown packets */
	sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
	sram_sw_set_lookup_done(te, 1);
	tcam_sw_text(te, "mac_eof");

	tcam_hw_write(te, TE_MAC_EOF);
	tcam_sw_free(te);

	return 0;
}

/******************************************************************************
 *
 * L2 Section
 *
 ******************************************************************************
 */

/*
 * Helper: match ethertype
 */
static void pnc_match_etype(struct tcam_entry *te, unsigned short ethertype)
{
	tcam_sw_set_byte(te, 0, ethertype >> 8);
	tcam_sw_set_byte(te, 1, ethertype & 0xFF);
}

/* Set VLAN priority entry */
int pnc_vlan_prio_set(int port, int prio, int rxq)
{
#if (CONFIG_MV_ETH_PNC_VLAN_PRIO > 0)
	struct tcam_entry *te;
	unsigned int pdata, pmask;
	int q, tid, empty = -1;

	PNC_DBG("%s\n", __func__);

	/* check validity */
	if ((prio < 0) || (prio > 7))
		return 1;

	if ((rxq < -1) || (rxq > CONFIG_MV_ETH_RXQ))
		return 1;

	/* Find match TCAM entry */
	for (tid = TE_VLAN_PRIO; tid <= TE_VLAN_PRIO_END; tid++) {
		te = pnc_tcam_entry_get(tid);
		/* Remember first Empty entry */
		if (te == NULL) {
			if (empty == -1)
				empty = tid;
			continue;
		}
		/* find VLAN entry with the same priority */
		if (tcam_sw_cmp_byte(te, 2, ((unsigned char)prio << 5)) == 0) {
			tcam_sw_get_port(te, &pdata, &pmask);
			if (rxq == -1) {
				if (!pnc_port_mask_check(pmask, port)) {
					tcam_sw_free(te);
					continue;
				}
				pmask = pnc_port_mask_update(pmask, port, 0);
				if (pmask == PORT_MASK) {	/* No valid ports */
					tcam_hw_inv(tid);
				} else {
					tcam_sw_set_port(te, pdata, pmask);
					tcam_hw_write(te, tid);
				}
			} else {
				q = sram_sw_get_rxq(te, NULL);
				if (rxq == q) {
					/* Add port to this entry */
					pmask = pnc_port_mask_update(pmask, port, 1);
					tcam_sw_set_port(te, pdata, pmask);
					tcam_hw_write(te, tid);
				} else {
					/* Update RXQ */
					pmask = pnc_port_mask_update(pmask, port, 0);
					if (pmask == PORT_MASK) {
						/* No valid ports - use the same entry */
						pmask = pnc_port_mask_update(pmask, port, 1);
						tcam_sw_set_port(te, pdata, pmask);
						sram_sw_set_rxq(te, rxq, 0);
						tcam_hw_write(te, tid);
						tcam_sw_free(te);
					} else {
						tcam_sw_free(te);
						continue;
					}
				}
			}
			tcam_sw_free(te);
			return 0;
		}
		tcam_sw_free(te);
	}
	if (rxq == -1) {
		mvOsPrintf("%s: Entry not found - vprio=%d, rxq=%d\n",
					__func__, prio, rxq);
		return 1;
	}

	/* Not found existing entry and no free TCAM entry - Failed */
	if (empty == -1) {
		mvOsPrintf("%s: No free place - vprio=%d, rxq=%d\n",
					__func__, prio, rxq);
		return 1;
	}

	/* Not found existing entry - add to free TCAM entry */
	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_VLAN_TYPE);
	tcam_sw_set_byte(te, 2, prio << 5);
	tcam_sw_set_mask(te, 2, 7 << 5);
	tcam_sw_text(te, "vlan_prio");

	sram_sw_set_rinfo(te, RI_VLAN, RI_VLAN);
	sram_sw_set_next_lookup(te, TCAM_LU_L2);
	sram_sw_set_shift_update(te, 0, MV_VLAN_HLEN);
	sram_sw_set_rxq(te, rxq, 0);

	/* single port mask */
	pmask = pnc_port_mask(port);
	tcam_sw_set_port(te, 0, pmask);

	tcam_sw_text(te, "vlan_prio");

	tcam_hw_write(te, empty);
	tcam_sw_free(te);

	return 0;
#else
	return -1;
#endif /* CONFIG_MV_ETH_PNC_VLAN_PRIO > 0 */
}

int pnc_vlan_init(void)
{
	struct tcam_entry *te;
	int tid;

	PNC_DBG("%s\n", __func__);

	/* Set default VLAN entry */
	tid = TE_VLAN_EOF;
	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_VLAN_TYPE);
	tcam_sw_set_mask(te, 2, 0);
	tcam_sw_text(te, "vlan_def");

	sram_sw_set_rxq(te, rxq_vlan, 0);

	sram_sw_set_rinfo(te, RI_VLAN, RI_VLAN);
	sram_sw_set_next_lookup(te, TCAM_LU_L2);
	sram_sw_set_shift_update(te, 0, MV_VLAN_HLEN);

	tcam_hw_write(te, tid);
	tcam_sw_free(te);

	return 0;
}

/******************************************************************************
 *
 * Ethertype Section
 *
 ******************************************************************************
 */
/* match arp */
void pnc_etype_arp(int rxq)
{
	struct tcam_entry *te;

	rxq_arp = rxq;
	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_IP_ARP_TYPE);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_rxq(te, rxq_arp, 0);
	tcam_sw_text(te, "etype_arp");

	tcam_hw_write(te, TE_ETYPE_ARP);
	tcam_sw_free(te);
}

/* match ip4 */
static void pnc_etype_ip4(void)
{
	struct tcam_entry *te;

	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_IP_TYPE);
	sram_sw_set_shift_update(te, 0, MV_ETH_TYPE_LEN);
	sram_sw_set_next_lookup(te, TCAM_LU_IP4);
	tcam_sw_text(te, "etype_ipv4");

	tcam_hw_write(te, TE_ETYPE_IP4);
	tcam_sw_free(te);
}

/* match ip6 */
static void pnc_etype_ip6(void)
{
	struct tcam_entry *te;

	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_IP6_TYPE);
	sram_sw_set_shift_update(te, 0, MV_ETH_TYPE_LEN);
	sram_sw_set_next_lookup(te, TCAM_LU_IP6);
	tcam_sw_text(te, "etype_ipv6");

	tcam_hw_write(te, TE_ETYPE_IP6);
	tcam_sw_free(te);
}

/* match pppoe */
static void pnc_etype_pppoe(void)
{
	struct tcam_entry *te;

	/* IPv4 over PPPoE */
	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_PPPOE_TYPE);
	tcam_sw_set_byte(te, MV_PPPOE_HDR_SIZE, 0x00);
	tcam_sw_set_byte(te, MV_PPPOE_HDR_SIZE + 1, 0x21);

	sram_sw_set_shift_update(te, 0, MV_ETH_TYPE_LEN + MV_PPPOE_HDR_SIZE);
	sram_sw_set_next_lookup(te, TCAM_LU_IP4);
	sram_sw_set_rinfo(te, RI_PPPOE, RI_PPPOE);
	tcam_sw_text(te, "pppoe_ip4");

	tcam_hw_write(te, TE_PPPOE_IP4);
	tcam_sw_free(te);

	/* IPv6 over PPPoE */
	te = tcam_sw_alloc(TCAM_LU_L2);
	pnc_match_etype(te, MV_PPPOE_TYPE);

	tcam_sw_set_byte(te, MV_PPPOE_HDR_SIZE, 0x00);
	tcam_sw_set_byte(te, MV_PPPOE_HDR_SIZE + 1, 0x57);

	sram_sw_set_shift_update(te, 0, MV_ETH_TYPE_LEN + MV_PPPOE_HDR_SIZE);
	sram_sw_set_next_lookup(te, TCAM_LU_IP6);
	sram_sw_set_rinfo(te, RI_PPPOE, RI_PPPOE);
	tcam_sw_text(te, "pppoe_ip6");

	tcam_hw_write(te, TE_PPPOE_IP6);
	tcam_sw_free(te);
}

/*
 * pnc_etype_init - match basic ethertypes
 */
static int pnc_etype_init(void)
{
	struct tcam_entry *te;
	int tid;

	PNC_DBG("%s\n", __func__);

	pnc_etype_arp(CONFIG_MV_ETH_RXQ_DEF);
	pnc_etype_ip4();
	pnc_etype_ip6();
	pnc_etype_pppoe();

	/* add custom ethertypes here */
	tid = TE_ETYPE;

	ERR_ON_OOR(--tid >= TE_ETYPE_EOF);

	/* end of section */
	te = tcam_sw_alloc(TCAM_LU_L2);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_L2, FLOWID_CTRL_LOW_HALF_MASK);
	sram_sw_set_rxq(te, CONFIG_MV_ETH_RXQ_DEF, 0);
	sram_sw_set_lookup_done(te, 1);
	tcam_sw_text(te, "etype_eof");

	tcam_hw_write(te, TE_ETYPE_EOF);
	tcam_sw_free(te);

	return 0;
}

/******************************************************************************
 *
 * IPv4 Section
 *
 ******************************************************************************
 */

static void pnc_ip4_flow_next_lookup_set(struct tcam_entry *te)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	sram_sw_set_next_lookup(te, TCAM_LU_FLOW_IP4);
#else
	sram_sw_set_next_lookup(te, TCAM_LU_L4);
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}

/*
 * pnc_ip4_tos - Add TOS priority rules
 */
int pnc_ip4_dscp(int port, unsigned char dscp, unsigned char mask, int rxq)
{
#if (CONFIG_MV_ETH_PNC_DSCP_PRIO > 0)
	struct tcam_entry *te;
	unsigned int pdata, pmask;
	int tid, q, empty = -1;

	if ((rxq < -1) || (rxq > CONFIG_MV_ETH_RXQ))
		return 1;

	for (tid = TE_IP4_DSCP; tid <= TE_IP4_DSCP_END; tid++) {
		PNC_DBG("%s: tid=%d, dscp=0x%02x, mask=0x%02x, rxq=%d\n", __func__, tid, dscp, mask, rxq);

		te = pnc_tcam_entry_get(tid);
		/* Remember first Empty entry */
		if (te == NULL) {
			if (empty == -1)
				empty = tid;
			continue;
		}
		/* Find existing entry for this TOS */
		if (tcam_sw_cmp_bytes(te, 1, 1, &dscp) == 0) {
			tcam_sw_get_port(te, &pdata, &pmask);
			if (rxq == -1) {
				if (!pnc_port_mask_check(pmask, port)) {
					tcam_sw_free(te);
					continue;
				}
				pmask = pnc_port_mask_update(pmask, port, 0);
				if (pmask == PORT_MASK) {	/* No valid ports */
					tcam_hw_inv(tid);
				} else {
					tcam_sw_set_port(te, pdata, pmask);
					tcam_hw_write(te, tid);
				}
			} else {
				q = sram_sw_get_rxq(te, NULL);
				if (rxq == q) {
					/* Add port to this entry */
					pmask = pnc_port_mask_update(pmask, port, 1);
					tcam_sw_set_port(te, pdata, pmask);
					tcam_hw_write(te, tid);
				} else {
					/* Update RXQ */
					pmask = pnc_port_mask_update(pmask, port, 0);
					if (pmask == PORT_MASK) {
						/* No valid ports - use the same entry */
						pmask = pnc_port_mask_update(pmask, port, 1);
						tcam_sw_set_port(te, pdata, pmask);
						sram_sw_set_rxq(te, rxq, 0);
						tcam_hw_write(te, tid);
					} else {
						tcam_sw_free(te);
						continue;
					}
				}
			}
			tcam_sw_free(te);
			return 0;
		}
		tcam_sw_free(te);
	}

	if (rxq == -1) {
		mvOsPrintf("%s: Entry not found - tos=0x%x, rxq=%d\n",
					__func__, dscp, rxq);
		return 1;
	}

	/* Not found existing entry and no free TCAM entry - Failed */
	if (empty == -1) {
		mvOsPrintf("%s: No free place - tos=0x%x, rxq=%d\n",
					__func__, dscp, rxq);
		return 1;
	}

	/* Not found existing entry - add to free TCAM entry */
	te = tcam_sw_alloc(TCAM_LU_IP4);
	tcam_sw_set_byte(te, 1, (MV_U8) dscp);
	tcam_sw_set_mask(te, 1, (MV_U8) mask);
	sram_sw_set_rxq(te, rxq, 0);
	pmask = pnc_port_mask(port);
	tcam_sw_set_port(te, 0, pmask);
	sram_sw_set_next_lookup(te, TCAM_LU_IP4);
	tcam_sw_set_ainfo(te, 0, AI_DONE_MASK);
	sram_sw_set_ainfo(te, AI_DONE_MASK, AI_DONE_MASK);

	tcam_sw_text(te, "ipv4_tos");

	tcam_hw_write(te, empty);
	tcam_sw_free(te);
	return 0;
#else
	return -1;
#endif /* (CONFIG_MV_ETH_PNC_DSCP_PRIO > 0) */
}


/* IPv4/TCP header parsing for fragmentation and L4 offset.  */
void pnc_ip4_tcp(int rxq)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);
	rxq_ip4_tcp = rxq;

	/* TCP, FRAG=0 normal */
	te = tcam_sw_alloc(TCAM_LU_IP4);
	tcam_sw_set_byte(te, 9, MV_IP_PROTO_TCP);
	tcam_sw_set_byte(te, 6, 0x00);
	tcam_sw_set_mask(te, 6, 0x3F);
	tcam_sw_set_byte(te, 7, 0x00);
	tcam_sw_set_mask(te, 7, 0xFF);
	sram_sw_set_shift_update(te, 1, SHIFT_IP4_HLEN);
	sram_sw_set_rinfo(te, (RI_L3_IP4 | RI_L4_TCP), (RI_L3_IP4 | RI_L4_TCP));
	sram_sw_set_rxq(te, rxq_ip4_tcp, 0);
	sram_sw_set_ainfo(te, 0, AI_DONE_MASK);
	pnc_ip4_flow_next_lookup_set(te);

	tcam_sw_text(te, "ipv4_tcp");

	tcam_hw_write(te, TE_IP4_TCP);
	tcam_sw_free(te);

	/* TCP, FRAG=1 any */
	te = tcam_sw_alloc(TCAM_LU_IP4);
	tcam_sw_set_byte(te, 9, MV_IP_PROTO_TCP);
	sram_sw_set_shift_update(te, 1, SHIFT_IP4_HLEN);
	sram_sw_set_rinfo(te, (RI_L3_IP4_FRAG | RI_L4_TCP), (RI_L3_IP4_FRAG | RI_L4_TCP));
	sram_sw_set_rxq(te, rxq_ip4_tcp, 0);
	sram_sw_set_ainfo(te, 0, AI_DONE_MASK);
	pnc_ip4_flow_next_lookup_set(te);
	tcam_sw_text(te, "ipv4_tcp_fr");

	tcam_hw_write(te, TE_IP4_TCP_FRAG);
	tcam_sw_free(te);
}

/* IPv4/UDP header parsing for fragmentation and L4 offset. */
void pnc_ip4_udp(int rxq)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);
	rxq_ip4_udp = rxq;

	/* UDP, FRAG=0 normal */
	te = tcam_sw_alloc(TCAM_LU_IP4);
	tcam_sw_set_byte(te, 9, MV_IP_PROTO_UDP);
	tcam_sw_set_byte(te, 6, 0x00);
	tcam_sw_set_mask(te, 6, 0x3F);
	tcam_sw_set_byte(te, 7, 0x00);
	tcam_sw_set_mask(te, 7, 0xFF);
	sram_sw_set_shift_update(te, 1, SHIFT_IP4_HLEN);
	sram_sw_set_rinfo(te, (RI_L3_IP4 | RI_L4_UDP), (RI_L3_IP4 | RI_L4_UDP));
	sram_sw_set_rxq(te, rxq_ip4_udp, 0);
	sram_sw_set_ainfo(te, 0, AI_DONE_MASK);
	pnc_ip4_flow_next_lookup_set(te);
	tcam_sw_text(te, "ipv4_udp");

	tcam_hw_write(te, TE_IP4_UDP);
	tcam_sw_free(te);

	/* UDP, FRAG=1 any */
	te = tcam_sw_alloc(TCAM_LU_IP4);
	tcam_sw_set_byte(te, 9, MV_IP_PROTO_UDP);
	sram_sw_set_shift_update(te, 1, SHIFT_IP4_HLEN);
	sram_sw_set_rinfo(te, (RI_L3_IP4_FRAG | RI_L4_UDP), (RI_L3_IP4_FRAG | RI_L4_UDP));
	sram_sw_set_rxq(te, rxq_ip4_udp, 0);
	sram_sw_set_ainfo(te, 0, AI_DONE_MASK);
	pnc_ip4_flow_next_lookup_set(te);
	tcam_sw_text(te, "ipv4_udp_fr");

	tcam_hw_write(te, TE_IP4_UDP_FRAG);
	tcam_sw_free(te);
}

/* IPv4 - end of section  */
static void pnc_ip4_end(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	te = tcam_sw_alloc(TCAM_LU_IP4);
	sram_sw_set_rinfo(te, (RI_L3_IP4 | RI_L4_UN), (RI_L3_IP4 | RI_L4_UN));
	sram_sw_set_rxq(te, rxq_ip4, 0);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_IP4, FLOWID_CTRL_LOW_HALF_MASK);

	tcam_sw_text(te, "ipv4_eof");

	tcam_hw_write(te, TE_IP4_EOF);
	tcam_sw_free(te);
}

int pnc_ip4_init(void)
{
	PNC_DBG("%s\n", __func__);

	pnc_ip4_tcp(CONFIG_MV_ETH_RXQ_DEF);
	pnc_ip4_udp(CONFIG_MV_ETH_RXQ_DEF);
	/*pnc_ip4_esp();*/
	pnc_ip4_end();

	return 0;
}

/******************************************************************************
 *
 * IPv6 Section
 *
 *******************************************************************************
 */

static void pnc_ip6_flow_next_lookup_set(struct tcam_entry *te)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	sram_sw_set_next_lookup(te, TCAM_LU_FLOW_IP6_A);
#else
	sram_sw_set_next_lookup(te, TCAM_LU_L4);
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}

/* IPv6 - detect TCP */
static void pnc_ip6_tcp(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	/* TCP without extension headers */
	te = tcam_sw_alloc(TCAM_LU_IP6);
	tcam_sw_set_byte(te, 6, MV_IP_PROTO_TCP);
	sram_sw_set_shift_update(te, 1, sizeof(MV_IP6_HEADER));
	pnc_ip6_flow_next_lookup_set(te);
	sram_sw_set_rinfo(te, (RI_L3_IP6 | RI_L4_TCP), (RI_L3_IP6 | RI_L4_TCP));
	sram_sw_set_rxq(te, rxq_ip6, 0);
	tcam_sw_text(te, "ipv6_tcp");

	tcam_hw_write(te, TE_IP6_TCP);
	tcam_sw_free(te);
}

/* IPv6 - detect UDP */
static void pnc_ip6_udp(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	/* UDP without extension headers */
	te = tcam_sw_alloc(TCAM_LU_IP6);
	tcam_sw_set_byte(te, 6, MV_IP_PROTO_UDP);
	sram_sw_set_shift_update(te, 1, sizeof(MV_IP6_HEADER));
	pnc_ip6_flow_next_lookup_set(te);

	sram_sw_set_rinfo(te, (RI_L3_IP6 | RI_L4_UDP), (RI_L3_IP6 | RI_L4_UDP));
	sram_sw_set_rxq(te, rxq_ip6, 0);
	tcam_sw_text(te, "ipv6_udp");

	tcam_hw_write(te, TE_IP6_UDP);
	tcam_sw_free(te);
}

/* IPv6 - end of section  */
static void pnc_ip6_end(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	te = tcam_sw_alloc(TCAM_LU_IP6);
	sram_sw_set_shift_update(te, 1, sizeof(MV_IP6_HEADER));
	sram_sw_set_rinfo(te, (RI_L3_IP6 | RI_L4_UN), (RI_L3_IP6 | RI_L4_UN));
	sram_sw_set_rxq(te, rxq_ip6, 0);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_IP6, FLOWID_CTRL_LOW_HALF_MASK);
	tcam_sw_text(te, "ipv6_eof");

	tcam_hw_write(te, TE_IP6_EOF);
	tcam_sw_free(te);
}

int pnc_ip6_init(void)
{
	PNC_DBG("%s\n", __func__);

	pnc_ip6_tcp();
	pnc_ip6_udp();

	pnc_ip6_end();

	return 0;
}

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
/******************************************************************************
 *
 * L3 Flows Section
 *
 ******************************************************************************
 */
static int pnc_flow_init(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	/* end of section for IPv4 */
	te = tcam_sw_alloc(TCAM_LU_FLOW_IP4);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_FLOW_IP4, FLOWID_CTRL_LOW_HALF_MASK);
	tcam_sw_text(te, "flow_ip4_eof");

	tcam_hw_write(te, TE_FLOW_IP4_EOF);
	tcam_sw_free(te);

	/* end of section for IPv6_A */
	te = tcam_sw_alloc(TCAM_LU_FLOW_IP6_A);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_FLOW_IP6_A, FLOWID_CTRL_LOW_HALF_MASK);
	tcam_sw_text(te, "flow_ip6_A_eof");

	tcam_hw_write(te, TE_FLOW_IP6_A_EOF);
	tcam_sw_free(te);

	/* end of section for IPv6_B */
	te = tcam_sw_alloc(TCAM_LU_FLOW_IP6_B);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_FLOW_IP6_B, FLOWID_CTRL_LOW_HALF_MASK);
	tcam_sw_text(te, "flow_ip6_B_eof");

	tcam_hw_write(te, TE_FLOW_IP6_B_EOF);
	tcam_sw_free(te);

	return 0;
}

/* require 2 TCAM entries for macth */
int pnc_ipv6_2_tuples_add(unsigned int tid1, unsigned int tid2, unsigned int flow_id,
					      MV_U8 unique, MV_U8 *sip, MV_U8 *dip, unsigned int rxq)
{
	struct tcam_entry   *te;
	int                 i;

	if ((tid1 < TE_FLOW_L3) || (tid1 > TE_FLOW_L3_END))
		ERR_ON_OOR(1);

	if ((tid2 < TE_FLOW_L3) || (tid2 > TE_FLOW_L3_END))
		ERR_ON_OOR(2);

	te = tcam_sw_alloc(TCAM_LU_FLOW_IP6_A);
	for (i = 0; i < 16; i++)
		tcam_sw_set_byte(te, 8+i, sip[i]);

	sram_sw_set_shift_update(te, 2, 24);

	sram_sw_set_next_lookup_shift(te, 2);

	sram_sw_set_next_lookup(te, TCAM_LU_FLOW_IP6_B);

	sram_sw_set_ainfo(te, unique, AI_MASK);
	tcam_sw_text(te, "ipv6_2t_A");

	tcam_hw_write(te, tid1);
	tcam_sw_free(te);

	te = tcam_sw_alloc(TCAM_LU_FLOW_IP6_B);
	for (i = 0; i < 16; i++)
		tcam_sw_set_byte(te, i, dip[i]);

	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, flow_id, FLOWID_CTRL_FULL_MASK);

	if (PNC_FLOWID_IS_HWF(flow_id))	{
		/* Overwrite RXQ - FIXME */
		sram_sw_set_rxq(te, rxq, 1);
	} else {
		sram_sw_set_rxq(te, rxq, 0);
	}

	sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);

	tcam_sw_set_ainfo(te, unique, AI_MASK);
	tcam_sw_text(te, "ipv6_2t_B");

	tcam_hw_write(te, tid2);
	tcam_sw_free(te);

	return 0;
}

int pnc_ipv4_2_tuples_add(unsigned int tid, unsigned int flow_id, unsigned int sip, unsigned int dip, unsigned int rxq)
{
	struct tcam_entry *te;

	PNC_DBG("%s [%d] flow=%d " MV_IPQUAD_FMT "->" MV_IPQUAD_FMT "\n",
		__func__, tid, flow_id, MV_IPQUAD(((MV_U8 *)&sip)), MV_IPQUAD(((MV_U8 *)&dip)));

	if (tid < TE_FLOW_L3)
		ERR_ON_OOR(1);

	if (tid > TE_FLOW_L3_END)
		ERR_ON_OOR(1);

	te = tcam_sw_alloc(TCAM_LU_FLOW_IP4);

	tcam_sw_set_byte(te, 12, (sip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 13, (sip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 14, (sip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 15, (sip >> 24) & 0xFF);

	tcam_sw_set_byte(te, 16, (dip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 17, (dip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 18, (dip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 19, (dip >> 24) & 0xFF);

	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, flow_id, FLOWID_CTRL_FULL_MASK);
	sram_sw_set_rxq(te, rxq, 0);
	sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
	tcam_sw_text(te, "ipv4_2t");

	tcam_hw_write(te, tid);
	tcam_sw_free(te);

#ifdef MV_ETH_PNC_AGING
	mvPncAgingCntrGroupSet(tid, 3);
#endif

	return 0;
}

int pnc_ipv4_5_tuples_add(unsigned int tid, unsigned int flow_id,
			  unsigned int sip, unsigned int dip, unsigned int proto, unsigned int ports, unsigned int rxq)
{
	struct tcam_entry *te;

	PNC_DBG("%s [%d] flow=%d " MV_IPQUAD_FMT "->" MV_IPQUAD_FMT ", ports=0x%x, proto=%d\n",
		__func__, tid, flow_id, MV_IPQUAD(((MV_U8 *)&sip)), MV_IPQUAD(((MV_U8 *)&dip)), ports, proto);

	if (tid < TE_FLOW_L3)
		ERR_ON_OOR(1);

	if (tid > TE_FLOW_L3_END)
		ERR_ON_OOR(1);

	/* sanity check */

	te = tcam_sw_alloc(TCAM_LU_FLOW_IP4);

	tcam_sw_set_byte(te, 9, proto);

	tcam_sw_set_byte(te, 12, (sip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 13, (sip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 14, (sip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 15, (sip >> 24) & 0xFF);

	tcam_sw_set_byte(te, 16, (dip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 17, (dip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 18, (dip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 19, (dip >> 24) & 0xFF);

	tcam_sw_set_byte(te, 20, (ports >> 0) & 0xFF);
	tcam_sw_set_byte(te, 21, (ports >> 8) & 0xFF);
	tcam_sw_set_byte(te, 22, (ports >> 16) & 0xFF);
	tcam_sw_set_byte(te, 23, (ports >> 24) & 0xFF);

	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, flow_id, FLOWID_CTRL_FULL_MASK);
	sram_sw_set_rxq(te, rxq, 0);
	sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
	tcam_sw_text(te, "ipv4_5t");

	tcam_hw_write(te, tid);
	tcam_sw_free(te);

#ifdef MV_ETH_PNC_AGING
	mvPncAgingCntrGroupSet(tid, 2);
#endif

	return 0;
}
#else
int pnc_l4_end(void)
{
	struct tcam_entry *te;

	PNC_DBG("%s\n", __func__);

	te = tcam_sw_alloc(TCAM_LU_L4);
	sram_sw_set_lookup_done(te, 1);
	sram_sw_set_flowid(te, FLOWID_EOF_LU_L4, FLOWID_CTRL_LOW_HALF_MASK);

	tcam_sw_text(te, "l4_eof");

	tcam_hw_write(te, TE_L4_EOF);
	tcam_sw_free(te);
	return 0;
}
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

/******************************************************************************
 *
 * PnC Init
 *
 ******************************************************************************
 */
int pnc_default_init(void)
{
	int     rc, port;
	MV_U32	regVal = 0;

	PNC_DBG("%s\n", __func__);

	rc = tcam_hw_init();
	if (rc)
		goto out;

	/* Mask all interrupts */
	MV_REG_WRITE(MV_PNC_MASK_REG, 0xffffffff);

	/* Clear all interrupts */
	MV_REG_WRITE(MV_PNC_CAUSE_REG, 0x0);

	/* Always start from lookup = 0 */
	for (port = 0; port < PORT_BITS; port++)
		regVal |= MV_PNC_PORT_LU_INIT_VAL(port, TCAM_LU_MAC);

	MV_REG_WRITE(MV_PNC_INIT_LOOKUP_REG, regVal);

	rc = pnc_mac_init();
	if (rc)
		goto out;

	rc = pnc_vlan_init();
	if (rc)
		goto out;

	rc = pnc_etype_init();
	if (rc)
		goto out;

	rc = pnc_ip4_init();
	if (rc)
		goto out;

	rc = pnc_ip6_init();
	if (rc)
		goto out;

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	rc = pnc_flow_init();
	if (rc)
		goto out;
#else
	rc = pnc_l4_end();
	if (rc)
		goto out;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

#ifdef CONFIG_MV_ETH_PNC_WOL
	mv_pnc_wol_init();
#endif /* CONFIG_MV_ETH_PNC_WOL */

	pnc_inited = 1;
out:
	return rc;
}

static void pnc_port_sprintf(struct tcam_entry *te, char *buf)
{
	int p, offs;
	unsigned int data, mask;

	tcam_sw_get_port(te, &data, &mask);
	if (mask == PORT_MASK)
		mvOsSPrintf(buf, "None");
	else if (mask == 0)
		mvOsSPrintf(buf, "All");
	else {
		offs = 0;
		for (p = 0; p < PORT_BITS; p++) {
			if ((mask & (1 << p)) == 0)
				offs += mvOsSPrintf(buf + offs, " %d,", pnc_port_map(p));
		}
	}
}

void pnc_vlan_prio_show(int port)
{
#if (CONFIG_MV_ETH_PNC_VLAN_PRIO > 0)
	struct tcam_entry *te;
	int tid;
	unsigned char prio;
	char buf[16];

	mvOsPrintf("Prio   Mask       Ports     RXQ    Name\n");
	for (tid = TE_VLAN_PRIO; tid <= TE_VLAN_PRIO_END; tid++) {
		te = pnc_tcam_entry_get(tid);
		if (te) {
			prio = *(te->data.u.byte + 2);
			mvOsPrintf("0x%02x", prio >> 5);
			prio = *(te->mask.u.byte + 2);
			mvOsPrintf("   0x%02x", prio >> 5);
			pnc_port_sprintf(te, buf);
			mvOsPrintf(" %12s", buf);
			mvOsPrintf("     %d", sram_sw_get_rxq(te, NULL));
			mvOsPrintf("     %s\n", te->ctrl.text);
			tcam_sw_free(te);
		}
	}
#endif /* (CONFIG_MV_ETH_PNC_VLAN_PRIO > 0) */
	return;
}
void pnc_ipv4_dscp_show(int port)
{
#if (CONFIG_MV_ETH_PNC_DSCP_PRIO > 0)
	struct tcam_entry *te;
	int tid;
	unsigned char tos;
	char buf[16];

	mvOsPrintf("TOS    Mask       Ports   RXQ    Name\n");
	for (tid = TE_IP4_DSCP; tid <= TE_IP4_DSCP_END; tid++) {
		te = pnc_tcam_entry_get(tid);
		if (te) {
			tos = *(te->data.u.byte + 1);
			mvOsPrintf("0x%02x", tos);
			tos = *(te->mask.u.byte + 1);
			mvOsPrintf("   0x%02x", tos);
			pnc_port_sprintf(te, buf);
			mvOsPrintf(" %10s", buf);
			mvOsPrintf("     %d", sram_sw_get_rxq(te, NULL));
			mvOsPrintf("     %s\n", te->ctrl.text);
			tcam_sw_free(te);
		}
	}
#endif /* CONFIG_MV_ETH_PNC_DSCP_PRIO > 0 */
	return;
}

void pnc_mac_show(void)
{
	int tid;
	struct tcam_entry *te;
	char *mac;
	char buf[16];

	mvOsPrintf("     Addr                   Mask         Ports    RXQ   Name\n");
	for (tid = TE_MAC_BC; tid < TE_MAC_EOF; tid++) {
		te = pnc_tcam_entry_get(tid);
		if (te) {
			mac = te->data.u.byte + MV_ETH_MH_SIZE;
			mvOsPrintf(MV_MACQUAD_FMT, MV_MACQUAD(mac));
			mac = te->mask.u.byte + MV_ETH_MH_SIZE;
			mvOsPrintf("   " MV_MACQUAD_FMT, MV_MACQUAD(mac));

			pnc_port_sprintf(te, buf);
			mvOsPrintf(" %10s", buf);
			mvOsPrintf("     %d", sram_sw_get_rxq(te, NULL));
			mvOsPrintf("     %s\n", te->ctrl.text);
			tcam_sw_free(te);
		}
	}
}

