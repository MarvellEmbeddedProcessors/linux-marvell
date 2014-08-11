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
* mv_cph_netdev.h
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) network device part definition
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 11Dec2011
*
* FILE REVISION NUMBER:
*               Revision: 1.0
*
*
*******************************************************************************/
#ifndef _MV_CPH_NETDEV_H_
#define _MV_CPH_NETDEV_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/ip.h>

#include <mvCommon.h>
#include <mvOs.h>
#include <../net_dev/mv_netdev.h>


#define MV_CPH_MAS_UDP_SRC_PORT          8
#define MV_CPH_MAS_UDP_DST_PORT          8
#define MV_CPH_NUM_LLID                  8
#define MV_CPH_PON_PORT_IDX              3

#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
struct mv_udp_port_tx_spec {
	__be16    udp_port;
	struct mv_pp2_tx_spec tx_spec;
};

struct mv_port_tx_spec {
	struct mv_udp_port_tx_spec udp_src[MV_CPH_MAS_UDP_SRC_PORT];
	struct mv_udp_port_tx_spec udp_dst[MV_CPH_MAS_UDP_DST_PORT];
};

void cph_udp_spec_print_all(void);
MV_STATUS  cph_udp_src_spec_set(int tx_port, uint16_t udp_src_port,
	uint8_t txp, uint8_t txq, uint16_t flags, uint32_t hw_cmd);
MV_STATUS  cph_udp_dest_spec_set(int tx_port, uint16_t udp_dest_port,
	uint8_t txp, uint8_t txq, uint16_t flags, uint32_t hw_cmd);
#endif

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
void cph_rec_skb(int port, struct sk_buff *skb);

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
int cph_netdev_init(void);

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
*       0: the packet will not be forwarded to linux stack and mv_pp2_rx() needs to continue to handle it
*******************************************************************************/
int cph_rx_func(int port, int rxq, struct net_device *dev,
		struct sk_buff *skb, struct pp2_rx_desc *rx_desc);

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
		struct mv_pp2_tx_spec *tx_spec_out);

#ifdef __cplusplus
}
#endif

#endif /* _MV_CPH_NETDEV_H_ */
