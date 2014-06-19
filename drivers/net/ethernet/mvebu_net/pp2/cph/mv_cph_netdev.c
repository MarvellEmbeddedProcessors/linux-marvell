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
********************************************************************************
* mv_cph_netdev.c
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) network device part definition
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 22Jan2013
*
* FILE REVISION NUMBER:
*               Revision: 1.1
*
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/icmpv6.h>

#include <ctrlEnv/mvCtrlEnvLib.h>

#include "mvDebug.h"
#include "mv_cph_header.h"


/******************************************************************************
*                           Global Definition
******************************************************************************/
#define MV_MUX_SKB_TAG_VAL		(0xabcd)

/* Total Eth port number */
static int gs_mv_eth_port_num;


#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
static struct mv_port_tx_spec    udp_port_spec_cfg[MV_APP_ETH_PORTS_NUM];
#define PORT_ENTRIES        CPH_TBL_ENTRY_NUM(udp_port_spec_cfg)
#endif

/******************************************************************************
*                           External Declarations
******************************************************************************/

/******************************************************************************
*                           Function Definitions
******************************************************************************/
/******************************************************************************
* cph_rec_skb()
* _____________________________________________________________________________
*
* DESCRIPTION: Send SKB packet to linux network and increse counter
*
* INPUTS:
*       port    - Gmac port the packet from
*       skb     - SKB buffer to receive packet
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*******************************************************************************/
void cph_rec_skb(int port, struct sk_buff *skb)
{
	unsigned int rx_status    = 0;
	struct eth_port *pp = NULL;

	rx_status = netif_receive_skb(skb);
	pp        = mv_eth_port_by_id(port);
	if (rx_status)
		STAT_DBG(pp->stats.rx_drop_sw++);
}

#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
static inline void cph_copy_tx_spec(struct mv_eth_tx_spec *tx_spec,
					uint8_t txp, uint8_t txq,
					uint16_t flags, uint32_t hw_cmd)
{
	tx_spec->txp = txp;
	tx_spec->txq = txq;
	tx_spec->hw_cmd[0] = hw_cmd;
	tx_spec->flags = flags;
}

int cph_udp_spec_print(int port)
{
	int i;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct mv_udp_port_tx_spec *udp_spec;

	if (!pp)
		return -ENODEV;

	udp_spec = &(udp_port_spec_cfg[port].udp_dst[0]);

	pr_err("\n**** port #%d - TX UDP Dest Port configuration *****\n", port);
	pr_err("----------------------------------------------------\n");
	pr_err("ID udp_dst   txp    txq    flags    hw_cmd     func_add\n");
	for (i = 0; i < sizeof(udp_port_spec_cfg[port].udp_dst)/sizeof(udp_port_spec_cfg[port].udp_dst[0]); i++) {
		if (udp_spec[i].tx_spec.txq != MV_ETH_TXQ_INVALID)
			pr_err("%2d   %04d      %d      %d     0x%04lx   0x%08x   0x%p\n",
			       i, ntohs(udp_spec[i].udp_port),
			       udp_spec[i].tx_spec.txp, udp_spec[i].tx_spec.txq,
			       udp_spec[i].tx_spec.flags, udp_spec[i].tx_spec.hw_cmd[0],
			       udp_spec[i].tx_spec.tx_func);
	}
	pr_err("-----------------------------------------------------\n");

	udp_spec = &(udp_port_spec_cfg[port].udp_src[0]);

	pr_err("**** port #%d - TX UDP Source Port configuration *****\n", port);
	pr_err("-----------------------------------------------------\n");
	pr_err("ID udp_src   txp    txq     flags    hw_cmd     func_add\n");
	for (i = 0; i < sizeof(udp_port_spec_cfg[port].udp_src)/sizeof(udp_port_spec_cfg[port].udp_src[0]); i++) {
		if (udp_spec[i].tx_spec.txq != MV_ETH_TXQ_INVALID)
			pr_err("%2d   %04d      %d      %d     0x%04lx   0x%08x   0x%p\n",
			       i, ntohs(udp_spec[i].udp_port),
			       udp_spec[i].tx_spec.txp, udp_spec[i].tx_spec.txq,
			       udp_spec[i].tx_spec.flags, udp_spec[i].tx_spec.hw_cmd[0],
			       udp_spec[i].tx_spec.tx_func);
	}
	pr_err("**************************************************************\n");

	return 0;
}


void cph_udp_spec_print_all(void)
{
	int port;

	for (port = 0; port < MV_APP_ETH_PORTS_NUM; port++)
		cph_udp_spec_print(port);
}

MV_STATUS  cph_udp_int_spec_set(struct mv_udp_port_tx_spec *udp_spec, uint16_t udp_port, int table_size,
				uint8_t txp, uint8_t txq, uint16_t flags, uint32_t hw_cmd)
{
	int i;

	/* Check if already exists */
	for (i = 0; i < table_size; i++) {
		if (udp_spec[i].udp_port == htons(udp_port) &&
		    udp_spec[i].tx_spec.txq != MV_ETH_TXQ_INVALID) {
			cph_copy_tx_spec(&(udp_spec[i].tx_spec), txp, txq, flags, hw_cmd);
			return MV_OK;
		}
	}
	/* Check empty */
	for (i = 0; i < table_size; i++) {
		if (udp_spec[i].tx_spec.txq == MV_ETH_TXQ_INVALID) {
			udp_spec[i].udp_port = htons(udp_port);
			cph_copy_tx_spec(&(udp_spec[i].tx_spec), txp, txq, flags, hw_cmd);
			return MV_OK;
		}
	}

	return MV_FULL;
}


MV_STATUS  cph_udp_src_spec_set(int tx_port, uint16_t udp_src_port, uint8_t txp,
	uint8_t txq, uint16_t flags, uint32_t hw_cmd)
{
	struct eth_port *pp = mv_eth_port_by_id(tx_port);
	struct mv_udp_port_tx_spec *udp_src_spec = udp_port_spec_cfg[tx_port].udp_src;
	MV_STATUS mv_status;

	if (!pp)
		return -ENODEV;

	mv_status = cph_udp_int_spec_set(udp_src_spec, udp_src_port,
					MV_CPH_MAS_UDP_SRC_PORT,
					txp, txq, flags, hw_cmd);

	if (mv_status != MV_OK)
		pr_err("%s: UDP Special Source Port Table is full\n", __func__);

	return mv_status;
}
EXPORT_SYMBOL(cph_udp_src_spec_set);


MV_STATUS  cph_udp_dest_spec_set(int tx_port, uint16_t udp_dest_port, uint8_t txp,
	uint8_t txq, uint16_t flags, uint32_t hw_cmd)
{
	struct eth_port *pp = mv_eth_port_by_id(tx_port);
	struct mv_udp_port_tx_spec *udp_dst_spec = udp_port_spec_cfg[tx_port].udp_dst;
	MV_STATUS mv_status;

	if (!pp)
		return -ENODEV;

	mv_status = cph_udp_int_spec_set(udp_dst_spec, udp_dest_port,
					MV_CPH_MAS_UDP_DST_PORT,
					txp, txq, flags, hw_cmd);

	if (mv_status != MV_OK)
		pr_err("%s: UDP Special Dest. Port Table is full\n", __func__);

	return mv_status;
}
EXPORT_SYMBOL(cph_udp_dest_spec_set);


void cph_udp_table_init(void)
{
	int num_ports = PORT_ENTRIES;
	int tx_port, i;

	if (num_ports > gs_mv_eth_port_num)
		num_ports = gs_mv_eth_port_num;

	for (tx_port = 0; tx_port < num_ports; tx_port++) {
		/* Invalidate UDP Dest ports, set txq=invalid  */
		for (i = 0; i < MV_CPH_MAS_UDP_DST_PORT; i++) {
			memset(&(udp_port_spec_cfg[tx_port].udp_dst[i]), 0, sizeof(struct mv_udp_port_tx_spec));
			udp_port_spec_cfg[tx_port].udp_dst[i].tx_spec.txq = MV_ETH_TXQ_INVALID;
		}

		/* Invalidate UDP Source ports, , set txq=invalid */
		for (i = 0; i < MV_CPH_MAS_UDP_SRC_PORT; i++) {
			memset(&(udp_port_spec_cfg[tx_port].udp_src[i]), 0, sizeof(struct mv_udp_port_tx_spec));
			udp_port_spec_cfg[tx_port].udp_src[i].tx_spec.txq = MV_ETH_TXQ_INVALID;
		}

	}
	return;
}

int cph_udp_port_tx(int port, struct net_device *dev, struct sk_buff *skb,
		struct mv_eth_tx_spec *tx_spec_out)
{
	struct iphdr  *iphdrp   = NULL;
	struct udphdr *udphdrp  = NULL;
	int i;

	if (port >= MV_APP_ETH_PORTS_NUM) {
		pr_err("Port Error\n");
		return 0;
	}

	if (skb->protocol == MV_CPH_ETH_TYPE_IPV4) {
		/* Get UDP Port */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
		iphdrp = skb->nh.iph;
#else
		iphdrp = ip_hdr(skb);
#endif

	if ((iphdrp) && (iphdrp->protocol == IPPROTO_UDP)) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 22)
		udphdrp = skb->h.uh;
#else
		udphdrp = udp_hdr(skb);
#endif
			if (udphdrp) {
				if (udphdrp == (struct udphdr *)iphdrp)
					udphdrp = (struct udphdr *)((char *)udphdrp + (4*(iphdrp->ihl)));
				/* Find configured UDP Source Port*/
				for (i = 0; i < MV_CPH_MAS_UDP_SRC_PORT; i++) {
					if ((udphdrp->source == udp_port_spec_cfg[port].udp_src[i].udp_port) &&
					(udp_port_spec_cfg[port].udp_src[i].tx_spec.txq != MV_ETH_TXQ_INVALID)) {
						memcpy(tx_spec_out, &(udp_port_spec_cfg[port].udp_src[i].tx_spec),
							sizeof(struct mv_eth_tx_spec));
						MV_CPH_PRINT(CPH_DEBUG_LEVEL, "found udp_src 0x(%04x)\n",
							ntohs(udphdrp->source));
						return 1;
					}
				}
				/* Find configured UDP Dest. Port*/
				for (i = 0; i < MV_CPH_MAS_UDP_DST_PORT; i++) {
					if ((udphdrp->dest == udp_port_spec_cfg[port].udp_dst[i].udp_port) &&
					(udp_port_spec_cfg[port].udp_src[i].tx_spec.txq != MV_ETH_TXQ_INVALID)) {
						memcpy(tx_spec_out, &(udp_port_spec_cfg[port].udp_dst[i].tx_spec),
							sizeof(struct mv_eth_tx_spec));
						MV_CPH_PRINT(CPH_DEBUG_LEVEL, "found udp_dst 0x(%04x)\n",
							ntohs(udphdrp->dest));
						return 1;
					}
				}

				if (port == MV_CPH_PON_PORT_IDX) {
					MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Packet UDP, udp source or dest port not found");
					MV_CPH_PRINT(CPH_DEBUG_LEVEL, " udp_src(%x)x udp_dst(%x)x\n",
						ntohs(udphdrp->source), ntohs(udphdrp->dest));
				}
			}
		} else if (port == MV_CPH_PON_PORT_IDX)
			MV_CPH_PRINT(CPH_DEBUG_LEVEL, "NOT UDP, ip_proto(%d)\n", iphdrp->protocol);
	} else if (port == MV_CPH_PON_PORT_IDX)
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "NOT IP, proto(%d)\n", skb->protocol);

	return 0;
}
#endif


#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
/******************************************************************************
* cph_data_flow_rx()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received application packets
*
* INPUTS:
*       port    - Gmac port the packet from
*       dev     - Net device
*       skb     - SKB buffer to receive packet
*       rx_desc - RX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
static int cph_data_flow_rx(int port, struct net_device *dev, struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	struct CPH_FLOW_ENTRY_T flow_rule;
	int            offset = 0;
	bool             state  = FALSE;
	MV_STATUS        rc     = MV_OK;

	if (MV_CPH_PON_PORT_IDX != port)
		return 0;

	cph_db_get_param(CPH_DB_PARAM_FLOW_SUPPORT, &state);

	if (state == TRUE) {
		/* Parse packets */
		rc = cph_flow_parse_packet(port, skb->data, TRUE, TRUE, &flow_rule);
		if (rc != MV_OK) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "%s():fail to call cph_flow_parse_packet, rc(%d)\n", __func__, rc);
			return 0;
		}

		/* Get None default CPH data flow rule at first */
		flow_rule.is_default = FALSE;
		rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);
		if (rc != MV_OK) {
			/* Get default CPH data flow rule secondly */
			flow_rule.is_default = TRUE;
			rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);
			/* Do nothing */
			if (rc != MV_OK)
				return 0;
		}

		/* modify packet */
		rc = cph_flow_mod_packet(skb, TRUE, &flow_rule, &offset);
		if (rc != MV_OK) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "%s():fail to call cph_flow_mod_rx_packet, rc(%d)\n", __func__, rc);
			return 0;
		}

		return 1;
	}

	return 0;
}

/******************************************************************************
* cph_data_flow_tx()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the trasmiting application packets
*
* INPUTS:
*       port        - Gmac port the packet from
*       dev         - Net device
*       skb         - SKB buffer to receive packet
*       tx_spec_out - TX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
int cph_data_flow_tx(int port, struct net_device *dev, struct sk_buff *skb,
			bool mh, struct mv_eth_tx_spec *tx_spec_out)
{
	struct CPH_FLOW_ENTRY_T flow_rule;
	int            offset = 0;
	bool             l_mh;
	bool             state  = FALSE;
	MV_STATUS        rc     = MV_OK;

	if (MV_CPH_PON_PORT_IDX != port)
		return 0;

	cph_db_get_param(CPH_DB_PARAM_FLOW_SUPPORT, &state);

	if (state != TRUE)
		return 0;

	/* Decide whether need to handle Marvell header */
	l_mh = mh;
	if (TRUE == mutils_is_frwd_broadcast_packet(skb->data))
		l_mh = TRUE;

	/* Parse packets */
	rc = cph_flow_parse_packet(port, skb->data, FALSE, l_mh, &flow_rule);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "%s():fail to call cph_flow_parse_packet, rc<%d>\n", __func__, rc);
		return 0;
	}

	/* Get CPH data flow rule */
	flow_rule.is_default = FALSE;
	rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);
	if (rc != MV_OK) {
		/* Handle multicat packets as unicast one */
		if (flow_rule.parse_bm & CPH_FLOW_PARSE_MC_PROTO) {
			flow_rule.parse_bm &= ~CPH_FLOW_PARSE_MC_PROTO;
			rc = cph_flow_db_get_rule_by_vid(&flow_rule);
			if (rc != MV_OK) {
				rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);

				if (rc != MV_OK) {
					flow_rule.is_default = TRUE;
					flow_rule.parse_outer_tci.vid   = MV_CPH_VID_NOT_CARE_VALUE;
					flow_rule.parse_outer_tci.pbits = MV_CPH_PBITS_NOT_CARE_VALUE;
					rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);
					if (rc != MV_OK) {
						MV_CPH_PRINT(CPH_DEBUG_LEVEL,
							"%s():fail to call cph_flow_get_rule, rc<%d>\n",
							__func__, rc);
						return 0;
					}
				}
			}
		} else {
			flow_rule.is_default = TRUE;
			flow_rule.parse_outer_tci.vid   = MV_CPH_VID_NOT_CARE_VALUE;
			flow_rule.parse_outer_tci.pbits = MV_CPH_PBITS_NOT_CARE_VALUE;
			rc = cph_flow_db_get_rule(&flow_rule, MV_TRUE);
			if (rc != MV_OK) {
				MV_CPH_PRINT(CPH_DEBUG_LEVEL, "%s():fail to call cph_flow_get_rule, rc<%d>\n",
					__func__, rc);
				return 0;
			}
		}
	}

	/* modify packet */
	rc = cph_flow_mod_packet(skb, l_mh, &flow_rule, &offset);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "%s():fail to call cph_flow_mod_rx_packet, rc<%d>\n", __func__, rc);
		return 0;
	}

	/* Strip TX MH */
	if (TRUE == l_mh) {
		skb->data  += MV_ETH_MH_SIZE;
		skb->len   -= MV_ETH_MH_SIZE;
	}

	/* modify packet */
	rc = cph_flow_mod_frwd(&flow_rule, tx_spec_out);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "%s():fail to call cph_flow_mod_frwd, rc<%d>\n", __func__, rc);
		return 0;
	}
	return 1;
}
#endif

/******************************************************************************
* cph_app_packet_rx()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received application packets
*
* INPUTS:
*       port    - Gmac port the packet from
*       dev     - Net device
*       skb     - SKB buffer to receive packet
*       rx_desc - RX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
static int cph_app_packet_rx(int port, struct net_device *dev, struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	enum CPH_APP_PARSE_FIELD_E parse_bm = 0;
	struct CPH_APP_PARSE_T       parse_key;
	int                 rc       = MV_OK;
	enum CPH_APP_MOD_FIELD_E   mod_bm   = 0;
	struct CPH_APP_MOD_T         mod_value;
	enum CPH_APP_FRWD_FIELD_E  frwd_bm  = 0;
	struct CPH_APP_FRWD_T        frwd_value;
	bool                  state    = FALSE;

	/* Check whether need to handle generic application packet */
	cph_db_get_param(CPH_DB_PARAM_APP_SUPPORT, &state);
	if (state == FALSE)
		return 0;

	memset(&parse_key,  0, sizeof(parse_key));
	memset(&mod_value,  0, sizeof(mod_value));
	memset(&frwd_value, 0, sizeof(frwd_value));

	/* Parse application packet */
	rc = cph_app_parse_packet(port, skb->data, &parse_bm, &parse_key);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_parse_packet, rc(%d)\n", rc);
		return 0;
	}

	/* Get CPH application rule */
	rc = cph_app_get_rule(parse_bm, &parse_key, &mod_bm, &mod_value, &frwd_bm, &frwd_value);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Fail to call cph_app_get_rule, rc(%d)\n", rc);
		return 0;
	}

	/* Increase counter */
	rc = cph_app_increase_counter(parse_bm, &parse_key);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_increase_counter, rc(%d)\n", rc);
		return 0;
	}

	/* Apply modification */
	rc = cph_app_mod_rx_packet(port, dev, skb, rx_desc, mod_bm, &mod_value);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_mod_rx_packet, rc(%d)\n", rc);
		return 0;
	}

	/* Send to Linux Network Stack */
	cph_rec_skb(port, skb);

	return 1;
}

/******************************************************************************
* cph_app_packet_tx()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the trasmiting application packets
*
* INPUTS:
*       port        - Gmac port the packet from
*       dev         - Net device
*       skb         - SKB buffer to receive packet
*       tx_spec_out - TX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
int cph_app_packet_tx(int port, struct net_device *dev, struct sk_buff *skb,
			struct mv_eth_tx_spec *tx_spec_out)
{
	enum CPH_DIR_E             dir;
	unsigned short                proto_type = 0;
	enum CPH_APP_PARSE_FIELD_E parse_bm   = 0;
	struct CPH_APP_PARSE_T       parse_key;
	enum CPH_APP_MOD_FIELD_E   mod_bm     = 0;
	struct CPH_APP_MOD_T         mod_value;
	enum CPH_APP_FRWD_FIELD_E  frwd_bm    = 0;
	struct CPH_APP_FRWD_T        frwd_value;
	int                 rc         = MV_OK;
	bool                  state      = FALSE;

	/* Check whether need to handle generic application packet */
	cph_db_get_param(CPH_DB_PARAM_APP_SUPPORT, &state);
	if (state == FALSE)
		return 0;

	memset(&parse_key,  0, sizeof(parse_key));
	memset(&mod_value,  0, sizeof(mod_value));
	memset(&frwd_value, 0, sizeof(frwd_value));
	tx_spec_out->tx_func = NULL;
	tx_spec_out->flags   = 0;

	/* Get direction of packet */
	dir = cph_app_parse_dir(port, FALSE);
	if (dir == CPH_DIR_INVALID) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "dir[%d] is invalid\n", dir);
		return 0;
	}

	/* Get the protocol type application packet */
	proto_type = skb->protocol;

	/* Get CPH application rule by protocol type */
	rc = cph_app_get_rule_by_dir_proto(dir, proto_type, &parse_bm,
					&parse_key, &mod_bm, &mod_value, &frwd_bm, &frwd_value);
	if (rc != MV_OK)
		return 0;

	/* Increase counter */
	rc = cph_app_increase_counter_by_dir_proto(dir, proto_type);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_increase_counter_by_dir_proto, rc(%d)\n", rc);
		return 0;
	}

	/* Sepcial handling for IGMP and MLD */
	if (((parse_bm & CPH_APP_PARSE_FIELD_IPV4_TYPE) &&
		(parse_key.ipv4_type == MV_IP_PROTO_IGMP))  ||
		((parse_bm & CPH_APP_PARSE_FIELD_ICMPV6_TYPE) &&
		(parse_key.icmpv6_type == MV_ICMPV6_TYPE_MLD))) {
		/* only handle U/S multicast protocol packet in U/S */
		if (dir == CPH_DIR_US) {
			if (cph_data_flow_tx(port, dev, skb, TRUE, tx_spec_out))
				return 1;
		}
	}

	/* Apply modification and set forwarding information */
	rc = cph_app_mod_tx_packet(skb, tx_spec_out, mod_bm, &mod_value);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_mod_tx_packet, rc(%d)\n", rc);
		return 0;
	}

	/* Set forwarding information */
	rc = cph_app_set_frwd(skb, tx_spec_out, frwd_bm, &frwd_value);
	if (rc != MV_OK) {
		pr_err("Fail to call cph_app_set_frwd, rc(%d)\n", rc);
		return 0;
	}

	/* mark skb as tagged skb */
	if (dir == CPH_DIR_DS)
		skb->skb_iif = MV_MUX_SKB_TAG_VAL;
	return 1;
}

/******************************************************************************
* cph_rx_func()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received special packets
*              from network driver
*
* INPUTS:
*       port    - Gmac port the packet from
*       rxq     - CPU received queue
*       dev     - Net device
*       skb     - Marvell packet information
*       rx_desc - RX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       1: the packet will be handled and forwarded to linux stack in CPH
*       0: the packet will not be forwarded to linux stack and mv_eth_rx() needs to continue to handle it
*******************************************************************************/
int cph_rx_func(int port, int rxq, struct net_device *dev,
		struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	MV_CPH_CLEAN_PRINT(CPH_DEBUG_LEVEL, "\n");
	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

	if (dev == NULL || skb == NULL || rx_desc == NULL) {
		pr_err("%s: NULL Pointer dev(%p) skb(%p) rx_desc(%p)\n",
			__func__, dev, skb, rx_desc);
	}

	if (mvPp2IsRxSpecial(rx_desc->parserInfo)) {
		/* Receive application packets */
		if (cph_app_packet_rx(port, dev, skb, rx_desc))
			return 1;
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Failed to receive application packet\n");

		/* Handle the broadcast packet in case it is enabled */
#ifdef CONFIG_MV_CPH_BC_HANDLE
		if (cph_app_rx_bc(port, dev, skb, rx_desc))
			return 1;
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "BC packet failure\n");
#endif
		/* deliver to upper layer */
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Deliver to upper layer\n");
	} else {/* Handle received data flow packets */
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
		if (cph_data_flow_rx(port, dev, skb, rx_desc)) {
			MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Flow mapping\n");
			return 0;
		}
#endif
	}

	return 0;
}

/******************************************************************************
* cph_tx_func()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle tranmitting special packets
*              to network driver
*
* INPUTS:
*       port        - Gmac port the packet from
*       dev         - Net device
*       skb         - SKB buffer to receive packet
*       tx_spec_out - TX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*******************************************************************************/
int cph_tx_func(int port, struct net_device *dev, struct sk_buff *skb,
		struct mv_eth_tx_spec *tx_spec_out)
{
	/* Transmit application packets */
	if (cph_app_packet_tx(port, dev, skb, tx_spec_out))
		return 1;

	/* Transmit data flow packets */
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	if (cph_data_flow_tx(port, dev, skb, FALSE, tx_spec_out))
		return 1;
#endif

	/* Transmit data flow packets by UDP Source Port or Dest Port */
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	if (cph_udp_port_tx(port, dev, skb, tx_spec_out))
		return 1;
#endif

	return 0;
}

/******************************************************************************
* cph_netdev_init()
* _____________________________________________________________________________
*
* DESCRIPTION: Initialize CPH network device
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_netdev_init(void)
{
	unsigned int idx;

	/* Retrieve Eth port number, as in mv_eth_init_module */
	gs_mv_eth_port_num = mvCtrlEthMaxPortGet();

	if (gs_mv_eth_port_num > MV_ETH_MAX_PORTS)
		gs_mv_eth_port_num = MV_ETH_MAX_PORTS;

	/* Initialize application packet handling */
	cph_app_init();

	/* Initialize UDP port mapping feature */
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	cph_udp_table_init();
#endif

	/* Initialize data flow mapping feature */
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	cph_flow_init();
#endif

	/* Register special receive check function */
#ifdef CONFIG_MV_ETH_RX_SPECIAL
	for (idx = 0; idx < gs_mv_eth_port_num; idx++)
		mv_eth_rx_special_proc_func(idx, cph_rx_func);
#endif /* CONFIG_MV_ETH_RX_SPECIAL */

	/* Register special transmit check function */
#ifdef CONFIG_MV_ETH_TX_SPECIAL
	for (idx = 0; idx < gs_mv_eth_port_num; idx++)
		mv_eth_tx_special_check_func(idx, cph_tx_func);
#endif /* CONFIG_MV_ETH_TX_SPECIAL */

	/* enable all T-CONT by default, whill remove it once callback implmented*/
	for (idx = 0; idx < MV_TCONT_LLID_NUM; idx++)
		cph_set_tcont_state(idx, true);

	return MV_OK;
}
