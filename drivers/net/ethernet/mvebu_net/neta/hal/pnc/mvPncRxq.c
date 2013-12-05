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

#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
int first_2tuple_rule = TE_FLOW_L3_END + 1;
int last_5tuple_rule = TE_FLOW_L3 - 1;

extern int pnc_port_mask_check(unsigned int mask, int eth_port);

static INLINE struct tcam_entry *pnc_create_2t_entry(unsigned int sip, unsigned int dip)
{
	struct tcam_entry *te = tcam_sw_alloc(TCAM_LU_FLOW_IP4);

	tcam_sw_set_byte(te, 12, (sip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 13, (sip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 14, (sip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 15, (sip >> 24) & 0xFF);

	tcam_sw_set_byte(te, 16, (dip >> 0) & 0xFF);
	tcam_sw_set_byte(te, 17, (dip >> 8) & 0xFF);
	tcam_sw_set_byte(te, 18, (dip >> 16) & 0xFF);
	tcam_sw_set_byte(te, 19, (dip >> 24) & 0xFF);

	return te;
}

static INLINE int tcam_sw_cmp_2tuple(struct tcam_entry *te, unsigned int sip, unsigned int dip)
{
	return !((tcam_sw_cmp_bytes(te, 12, 4, (unsigned char *)&sip) == 0)
			&& (tcam_sw_cmp_bytes(te, 16, 4, (unsigned char *)&dip) == 0));
}

static INLINE int tcam_sw_cmp_5tuple(struct tcam_entry *te, unsigned int sip, unsigned int dip,
								unsigned int ports, unsigned int proto)
{
	if (tcam_sw_cmp_2tuple(te, sip, dip) != 0)
		return 1;

	return !((tcam_sw_cmp_bytes(te, 9, 1, (unsigned char *)&proto) == 0) &&
			(tcam_sw_cmp_bytes(te, 20, 2, (unsigned char *)&ports) == 0));
}
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */

static INLINE int pnc_mask_to_port(unsigned int mask)
{
#ifdef CONFIG_ARCH_FEROCEON_KW2
	switch (mask) {
	case 27:
		return 0;
	case 15:
		return 1;
	case 30:
		return 2;
	default:
		return -1;
	}
#else
	switch (mask) {
	case 30:
		return 0;
	case 15:
		return 1;
	case 27:
		return 2;
	case 23:
		return 3;
	default:
		return -1;
	}
#endif /* MV_ETH_PNC_NEW */
}

/*
 * pnc_ip4_2tuple - Add 2-tuple priority rules
 */
int pnc_ip4_2tuple_rxq(unsigned int eth_port, unsigned int sip, unsigned int dip, int rxq)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	struct tcam_entry *te;
	unsigned int pdata, pmask;
	int tid, empty = -1, min_index_occupied = TE_FLOW_L3_END + 1;

	if (rxq < -2 || rxq >= CONFIG_MV_ETH_RXQ || eth_port >= CONFIG_MV_ETH_PORTS_NUM)
		return 1;

	for (tid = TE_FLOW_L3_END; tid > last_5tuple_rule; tid--) {
		te = pnc_tcam_entry_get(tid);
		/* Remember first Empty entry */
		if (te == NULL) {
			if (empty == -1)
				empty = tid;
			continue;
		}

		/* Find existing entry for this rule */
		if (tcam_sw_cmp_2tuple(te, sip, dip) == 0) {
			tcam_sw_get_port(te, &pdata, &pmask);
			if (rxq == -2) { /* delete rule */
				if (!pnc_port_mask_check(pmask, eth_port)) {
					printk(KERN_ERR "%s: rule is not associated with this port (%d)\n", __func__, eth_port);
					tcam_sw_free(te);
					return 1;
				}
				if (first_2tuple_rule == tid)
					first_2tuple_rule = min_index_occupied;
				pnc_te_del(tid);
				tcam_sw_free(te);
				return 0;
			}

			if (!pnc_port_mask_check(pmask, eth_port)) { /* rule is already associated with another port */
				printk(KERN_ERR "%s: rule is already associated with port %d\n",
									__func__, pnc_mask_to_port(pmask));
				return 1;
			}
			if (rxq == -1) { /* set rule to drop mode */
				sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
				sram_sw_set_lookup_done(te, 1);
				tcam_hw_write(te, tid);
			} else { /* update rxq */
				sram_sw_set_rinfo(te, 0, RI_DROP);
				sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
				sram_sw_set_rxq(te, rxq, 0);
				tcam_hw_write(te, tid);
			}

			tcam_sw_free(te);
			return 0;
		}
		min_index_occupied = tid;
		tcam_sw_free(te);
	}

	/* Add rule to PNC */
	if (rxq == -2) {
		mvOsPrintf("%s: Entry not found - sip=0x%x, dip=0x%x, rxq=%d\n", __func__, sip, dip, rxq);
		return 1;
	}
	/* Not found existing entry and no free TCAM entry - Failed */
	if ((empty == -1) || (empty <= last_5tuple_rule)) {
		mvOsPrintf("%s: No free place - sip=0x%x, dip=0x%x, rxq=%d\n", __func__, sip, dip, rxq);
		return 1;
	}

	/* update upper border of 2 tuple rules */
	if (first_2tuple_rule > empty)
		first_2tuple_rule = empty;

	te = pnc_create_2t_entry(sip, dip);
	pmask = pnc_port_mask(eth_port);
	tcam_sw_set_port(te, 0, pmask);
	sram_sw_set_lookup_done(te, 1);
	tcam_sw_text(te, "ipv4_2t");

	if (rxq == -1) {
		sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
		sram_sw_set_lookup_done(te, 1);
	} else {
		sram_sw_set_rinfo(te, 0, RI_DROP);
		sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
		sram_sw_set_rxq(te, rxq, 0);
	}

	tcam_hw_write(te, empty);
	tcam_sw_free(te);

	return 0;
#else
	return -1;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}

/*
 * pnc_ip4_5tuple - Add 5-tuple priority rules
 */
int pnc_ip4_5tuple_rxq(unsigned int eth_port, unsigned int sip, unsigned int dip, unsigned int ports,
						unsigned int proto, int rxq)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	struct tcam_entry *te;
	unsigned int pdata, pmask;
	int tid, empty = -1, max_index_occupied = TE_FLOW_L3 - 1;

	if (rxq < -2 || rxq >= CONFIG_MV_ETH_RXQ || eth_port >= CONFIG_MV_ETH_PORTS_NUM)
		return 1;

	for (tid = TE_FLOW_L3; tid < first_2tuple_rule; tid++) {
		te = pnc_tcam_entry_get(tid);
		/* Remember first Empty entry */
		if (te == NULL) {
			if (empty == -1)
				empty = tid;
			continue;
		}
		/* Find existing entry for this rule */
		if (tcam_sw_cmp_5tuple(te, sip, dip, ports, proto) == 0) {
			tcam_sw_get_port(te, &pdata, &pmask);
			if (rxq == -2) { /* delete rule */
				if (!pnc_port_mask_check(pmask, eth_port)) {
					printk(KERN_ERR "%s: rule is not associated with this port (%d)\n", __func__, eth_port);
					tcam_sw_free(te);
					return 1;
				}
				if (last_5tuple_rule == tid)
					last_5tuple_rule = max_index_occupied;
				pnc_te_del(tid);
				tcam_sw_free(te);
				return 0;
			}

			if (!pnc_port_mask_check(pmask, eth_port)) { /* rule is already associated with another port */
				printk(KERN_ERR "%s: rule is already associated with port %d\n",
									__func__, pnc_mask_to_port(pmask));
				return 1;
			}
			if (rxq == -1) { /* set rule to drop mode */
				sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
				sram_sw_set_lookup_done(te, 1);
				tcam_hw_write(te, tid);
			} else { /* update rxq */
				sram_sw_set_rinfo(te, 0, RI_DROP);
				sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
				sram_sw_set_rxq(te, rxq, 0);
				tcam_hw_write(te, tid);
			}

			tcam_sw_free(te);
			return 0;
		}
		max_index_occupied = tid;
		tcam_sw_free(te);
	}

	/* Add rule to PNC */
	if (rxq == -2) {
		mvOsPrintf("%s: Entry not found - sip=0x%x, dip=0x%x, ports=0x%x, proto=%d, rxq=%d\n",
				__func__, sip, dip, ports, proto, rxq);
		return 1;
	}
	/* Not found existing entry and no free TCAM entry - Failed */
	if ((empty == -1) || (empty >= first_2tuple_rule)) {
		mvOsPrintf("%s: No free place - sip=0x%x, dip=0x%x, ports=0x%x, proto=%d, rxq=%d\n",
				__func__, sip, dip, ports, proto, rxq);
		return 1;
	}

	/* update lower border of 5 tuple rules */
	if (last_5tuple_rule < empty)
		last_5tuple_rule = empty;

	te = pnc_create_2t_entry(sip, dip);

	tcam_sw_set_byte(te, 9, proto);
	tcam_sw_set_byte(te, 20, (ports >> 0) & 0xFF);
	tcam_sw_set_byte(te, 21, (ports >> 8) & 0xFF);
	tcam_sw_set_byte(te, 22, (ports >> 16) & 0xFF);
	tcam_sw_set_byte(te, 23, (ports >> 24) & 0xFF);
	pmask = pnc_port_mask(eth_port);
	tcam_sw_set_port(te, 0, pmask);
	sram_sw_set_lookup_done(te, 1);
	tcam_sw_text(te, "ipv4_5t");

	if (rxq == -1) {
		sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
		sram_sw_set_lookup_done(te, 1);
	} else {
		sram_sw_set_rinfo(te, 0, RI_DROP);
		sram_sw_set_rinfo(te, RI_L3_FLOW, RI_L3_FLOW);
		sram_sw_set_rxq(te, rxq, 0);
	}

	tcam_hw_write(te, empty);
	tcam_sw_free(te);

	return 0;
#else
	return -1;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}


/*
 * pnc_rxq_map_dump - Dump all rules
 */
int pnc_rxq_map_dump()
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	struct tcam_entry *te;
	unsigned int tid, sport, dport, word, shift, rinfo, mask, data;
	unsigned char sip[4], dip[4], sip_buf[16], dip_buf[16], *proto;

	mvOsPrintf(" Tid   Sip               Dip               Sport   Dport   Proto   Rxq    Port   Name\n");
	for (tid = TE_FLOW_L3; tid <= TE_FLOW_L3_END; tid++) {
		te = pnc_tcam_entry_get(tid);
		/* Remember first Empty entry */
		if (te) {
			memset(sip_buf, 0, 16);
			memset(dip_buf, 0, 16);

			sip[0] = *(te->data.u.byte + 12);
			sip[1] = *(te->data.u.byte + 13);
			sip[2] = *(te->data.u.byte + 14);
			sip[3] = *(te->data.u.byte + 15);
			dip[0] = *(te->data.u.byte + 16);
			dip[1] = *(te->data.u.byte + 17);
			dip[2] = *(te->data.u.byte + 18);
			dip[3] = *(te->data.u.byte + 19);
			mvOsSPrintf(sip_buf, "%d.%d.%d.%d", sip[0], sip[1], sip[2], sip[3]);
			mvOsSPrintf(dip_buf, "%d.%d.%d.%d", dip[0], dip[1], dip[2], dip[3]);
			mvOsPrintf(" %-3d   %-15s   %-15s   ", tid, sip_buf, dip_buf);

			if (te->ctrl.text[5] == '5') {
				sport = MV_BYTE_SWAP_16BIT(*((u16 *)(te->data.u.byte + 20)));
				dport = MV_BYTE_SWAP_16BIT(*((u16 *)(te->data.u.byte + 22)));
				proto = (*(te->data.u.byte + 9) == 6) ? "TCP" : "UDP";
				mvOsPrintf("%-5d   %-5d   %-5s   ", sport, dport, proto);
			} else
				mvOsPrintf("-----   -----   -----   ");

			word = RI_VALUE_OFFS / 32;
			shift = RI_VALUE_OFFS % 32;
			rinfo = (te->sram.word[word] >> shift) & ((1 << RI_BITS) - 1);
			if (rinfo & 1)
				mvOsPrintf("DROP   ");
			else
				mvOsPrintf("%-4d   ", sram_sw_get_rxq(te, NULL));

			tcam_sw_get_port(te, &data, &mask);
			mvOsPrintf("%-4d   ", pnc_mask_to_port(mask));
			mvOsPrintf("%s\n", te->ctrl.text);

			tcam_sw_free(te);
		}
	}

	return 0;
#else
	return -1;
#endif /* CONFIG_MV_ETH_PNC_L3_FLOW */
}
