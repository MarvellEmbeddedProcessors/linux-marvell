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

#ifndef __mv_net_config_h__
#define __mv_net_config_h__

#define MV_ETH_MAX_PORTS                4

#if defined(CONFIG_MV_ETH_PP2) || defined(CONFIG_MV_ETH_PP2_MODULE)

#define INTER_REGS_PHYS_BASE		0xF1000000
#define INTER_REGS_VIRT_BASE		0xFBC00000

#define PP2_CPU0_VIRT_BASE		0xFEC00000
#define PP2_CPU1_VIRT_BASE		0xFEC10000

#define MV_PP2_PON_EXIST
#define MV_PP2_PON_PORT_ID              7
#define MV_PP2_MAX_RXQ                  16      /* Maximum number of RXQs can be mapped to each port */
#define MV_PP2_MAX_TXQ                  8
#define MV_PP2_RXQ_TOTAL_NUM            32      /* Total number of RXQs for usage by all ports */
#define MV_PP2_MAX_TCONT                16      /* Maximum number of TCONTs supported by PON port */
#define MV_PP2_TX_CSUM_MAX_SIZE         1790

#define IRQ_GLOBAL_GOP			82 /* Group of Ports (GOP) */
#define IRQ_GLOBAL_NET_WAKE_UP		112 /* WOL interrupt */

#define MV_ETH_TCLK			(166666667)

#ifdef CONFIG_OF
extern int pp2_vbase;
extern int eth_vbase;
extern int pp2_port_vbase[MV_ETH_MAX_PORTS];
#define MV_PP2_REG_BASE                 (pp2_vbase)
#define MV_ETH_BASE_ADDR                (eth_vbase)
#define GOP_REG_BASE(port)		(pp2_port_vbase[port])
#else
#define MV_PP2_REG_BASE                 (0xF0000)
#define MV_ETH_BASE_ADDR                (0xC0000)
#define GOP_REG_BASE(port)		(MV_ETH_BASE_ADDR + 0x4000 + ((port) / 2) * 0x3000 + ((port) % 2) * 0x1000)
#endif

#define LMS_REG_BASE                    (MV_ETH_BASE_ADDR)
#define MIB_COUNTERS_REG_BASE           (MV_ETH_BASE_ADDR + 0x1000)
#define GOP_MNG_REG_BASE                (MV_ETH_BASE_ADDR + 0x3000)
#define MV_PON_REGS_OFFSET              (MV_ETH_BASE_ADDR + 0x8000)

#endif /* PP2 */

#if defined(CONFIG_MV_ETH_NETA) || defined(CONFIG_MV_ETH_NETA_MODULE)

#define MV_PON_PORT(p)			MV_FALSE
#define MV_ETH_MAX_TCONT		1

#define MV_ETH_MAX_RXQ			8
#define MV_ETH_MAX_TXQ			8
#define MV_ETH_TX_CSUM_MAX_SIZE		9800
#define MV_PNC_TCAM_LINES		1024    /* TCAM num of entries */
#define MV_BM_WIN_ID		12
#define MV_PNC_WIN_ID		11
#define MV_BM_WIN_ATTR		0x4
#define MV_PNC_WIN_ATTR		0x4

/* New GMAC module is used */
#define MV_ETH_GMAC_NEW
/* New WRR/EJP module is used */
#define MV_ETH_WRR_NEW
/* IPv6 parsing support for Legacy parser */
#define MV_ETH_LEGACY_PARSER_IPV6
#define MV_ETH_PNC_NEW
#define MV_ETH_PNC_LB

#endif /* CONFIG_MV_ETH_NETA */

#endif /* __mv_net_config_h__ */
