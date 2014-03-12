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
#ifndef __mv_gmac_h__
#define __mv_gmac_h__

#include "gmac/mv_gmac_regs.h"

struct pp3_gmac_ctrl {
	u32 base;
	u32 flags;
};

/* mv_pp3_gmac_ctrl flags */


#define MV_PP3_GMAC_F_DEBUG_BIT		0
#define MV_PP3_GMAC_F_ATTACH_BIT	1

#define MV_PP3_GMAC_F_DEBUG		(1 << MV_PP3_GMAC_F_DEBUG_BIT)
#define MV_PP3_GMAC_F_ATTACH		(1 << MV_PP3_GMAC_F_ATTACH_BIT)

enum pp3_port_speed {
	GMAC_SPEED_AN,
	GMAC_SPEED_10,
	GMAC_SPEED_100,
	GMAC_SPEED_1000,
	GMAC_SPEED_2000,
};

enum pp3_port_duplex {
	GMAC_DUPLEX_AN,
	GMAC_DUPLEX_HALF,
	GMAC_DUPLEX_FULL
};

enum pp3_port_fc {
	GMAC_FC_AN_NO,
	GMAC_FC_AN_SYM,
	GMAC_FC_AN_ASYM,
	GMAC_FC_DISABLE,
	GMAC_FC_ENABLE,
	GMAC_FC_ACTIVE
};

struct pp3_port_link_status {
	int			linkup; /*flag*/
	enum pp3_port_speed	speed;
	enum pp3_port_duplex	duplex;
	enum pp3_port_fc		rx_fc;
	enum pp3_port_fc		tx_fc;
};

/***************************************************************************/
/*                          regs access functions                          */
/***************************************************************************/
void pp3_gmac_unit_base(int index, u32 base);
u32  pp3_gmac_reg_read(int port, u32 reg);
void pp3_gmac_reg_write(int port, u32 reg, u32 data);

static inline u32 mv_fpga_gop_base_addr_get(void)
{
	return 0xa0000000;
}


/***************************************************************************/
/*                          inline functions                               */
/***************************************************************************/
static inline void pp3_gmac_isr_summary_mask(void)
{
	mv_pp3_hw_reg_write(mv_fpga_gop_base_addr_get() + ISR_SUM_MASK_REG, 0);
}

static inline void pp3_gmac_isr_summary_unmask(void)
{
	mv_pp3_hw_reg_write(mv_fpga_gop_base_addr_get() + ISR_SUM_MASK_REG,
			ISR_SUM_PORT0_MASK | ISR_SUM_PORT1_MASK | 0x20 /* magic bit */);
}

static inline u32 pp3_gmac_isr_summary_cause_get(void)
{
	return mv_pp3_hw_reg_read(mv_fpga_gop_base_addr_get() + ISR_SUM_CAUSE_REG);
}

static inline u32 pp3_gmac_port_isr_cause_get(int port)
{
	return pp3_gmac_reg_read(port, GMAC_PORT_ISR_CAUSE_REG);
}

static inline void pp3_gmac_port_isr_mask(int port)
{
	pp3_gmac_reg_write(port, GMAC_PORT_ISR_MASK_REG, 0);
}

static inline void pp3_gmac_port_isr_unmask(int port)
{
	pp3_gmac_reg_write(port, GMAC_PORT_ISR_MASK_REG, GMAC_PORT_LINK_CHANGE_MASK);
}

void pp3_gmac_def_set(int port);
void pp3_gmac_port_enable(int port);
void pp3_gmac_port_diable(int port);
void pp3_gmac_port_periodic_xon_set(int port, int enable);
bool pp3_gmac_port_is_link_up(int port);
int pp3_gmac_lins_status(int port, struct pp3_port_link_status *pstatus);
void pp3_gmac_port_lb_set(int port, int is_gmii, int is_pcs_en);
void pp3_gmac_port_reset_set(int port, bool set_reset);
void pp3_gmac_port_power_up(int port, bool is_sgmii, bool isRgmii);
void pp3_gmac_port_power_down(int port);
char *pp3_gmac_speed_str_get(enum pp3_port_speed speed);

/******************************************************************************/
/*                          Port Configuration functions                      */
/******************************************************************************/
int pp3_gmac_max_rx_size_set(int port, int max_rx_size);
int pp3_gmac_force_link_mode_set(int port_num, bool force_link_up, bool force_link_down);
int pp3_gmac_speed_duplex_set(int port_num, enum pp3_port_speed speed, enum pp3_port_duplex duplex);
int pp3_gmac_speed_duplex_get(int port_num, enum pp3_port_speed *speed, enum pp3_port_duplex *duplex);
int pp3_gmac_fc_set(int port, enum pp3_port_fc fc);
void pp3_gmac_fc_get(int port, enum pp3_port_fc *fc);
int pp3_gmac_port_link_speed_fc(int port, enum pp3_port_speed speed,
				     int force_link_up);

/******************************************************************************/
/*                         PHY Control Functions                              */
/******************************************************************************/
void pp3_gmac_phy_addr_set(int port, int phy_addr);
int pp3_gmac_phy_addr_get(int port);

/****************************************/
/*	MIB counters			*/
/****************************************/
u32 pp3_gmac_mib_counter_read(int port, u32 offset, u32 *p_high_32);
void pp3_gmac_mib_counters_clear(int port);
void pp3_gmac_mib_counters_show(int port);
void pp3_gmac_port_regs(int port);
/*
void pp3_gmac_lms_regs(void);
*/
#endif /* __mv_gmac_h__ */
