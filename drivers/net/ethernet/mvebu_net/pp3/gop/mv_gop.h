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
*******************************************************************************/
#ifndef __mv_gop_h__
#define __mv_gop_h__

/*--------------------------------------------------------------*/
/*------------------------- LINK -------------------------------*/
/*--------------------------------------------------------------*/
enum mv_link_speed {
	MV_ETH_SPEED_AN,
	MV_ETH_SPEED_10,
	MV_ETH_SPEED_100,
	MV_ETH_SPEED_1000,
	MV_ETH_SPEED_2000,
};

enum mv_link_duplex {
	MV_ETH_DUPLEX_AN,
	MV_ETH_DUPLEX_HALF,
	MV_ETH_DUPLEX_FULL
};

enum mv_link_fc {
	MV_ETH_FC_AN_ADV_DIS,
	MV_ETH_FC_AN_ADV_SYM,
	MV_ETH_FC_DISABLE,
	MV_ETH_FC_ENABLE
};

struct mv_link_status {
	/* TODO: change is_up val to bool */
	int is_up;
	enum mv_link_speed speed;
	enum mv_link_duplex duplex;
	enum mv_link_fc fc;
};

int	mv_gop_is_link_up(int port); /* TODO: change ret val to bool */
int	mv_gop_link_status(int port, struct mv_link_status *pStatus);
char	*mv_gop_link_speed_str_get(enum mv_link_speed speed);

/*--------------------------------------------------------------*/
/*---------------------- Interrupts ----------------------------*/
/*--------------------------------------------------------------*/
static inline void mv_gop_isr_sum_mask(void)
{
	/* TODO */
}

static inline void mv_gop_isr_sum_unmask(void)
{
	/* TODO */
}

static inline void mv_pp3_gop_cpu_interrupt_enable(int port, int cpu_mask)
{
	/* TODO */
}

static inline void mv_pp3_gop_cpu_interrupt_disable(int port, int cpu_mask)
{
	/* TODO */
}

static inline unsigned int mv_gop_isr_cause_get(int port)
{
	/* TODO */
	return 0;
}

static inline unsigned int mv_gop_isr_sum_get_cause_get(void)
{
	/* TODO */
	return 0;
}

static inline void mv_gop_isr_unmask(int port)
{
	/* TODO */
}

static inline void mv_gop_isr_mask(int port)
{
	/* TODO */
}
/*--------------------------------------------------------------*/
/*-------------------------- RX --------------------------------*/
/*--------------------------------------------------------------*/

/*--------------------------------------------------------------*/
/*-------------------------- TX --------------------------------*/
/*--------------------------------------------------------------*/

#endif /* __mv_gop_h__ */
