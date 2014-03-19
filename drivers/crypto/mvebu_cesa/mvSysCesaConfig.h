/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

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
/*******************************************************************************
* mvSysCesaConfig.h - Marvell Cesa unit specific configurations
*
* DESCRIPTION:
*       None.
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __mvSysCesaConfig_h__
#define __mvSysCesaConfig_h__

/*
** Base address for cesa registers.
*/

extern MV_U32 mv_cesa_base[], mv_cesa_tdma_base[];

/* This enumerator defines the Marvell CESA feature*/
enum cesa_mode {
	CESA_UNKNOWN_M = -1,
	CESA_OCF_M,
	CESA_TEST_M
};

extern enum cesa_mode mv_cesa_mode;

#define MV_CESA_REGS_BASE(chan)		(mv_cesa_base[chan])

#define MV_CESA_TDMA_REGS_BASE(chan)	(mv_cesa_tdma_base[chan])

#define MV_CESA_CHANNELS		(CONFIG_MV_CESA_CHANNELS)

#ifdef CONFIG_MV_CESA_CHAIN_MODE
	#define MV_CESA_CHAIN_MODE
#endif

#ifdef CONFIG_MV_CESA_INT_COALESCING_SUPPORT
	#define MV_CESA_INT_COALESCING_SUPPORT
	#define MV_CESA_INT_COAL_THRESHOLD		\
				(CONFIG_MV_CESA_INT_COAL_THRESHOLD)
	#define MV_CESA_INT_COAL_TIME_THRESHOLD		\
				(CONFIG_MV_CESA_INT_COAL_TIME_THRESHOLD)
#endif

#ifdef CONFIG_MV_CESA_INT_PER_PACKET
	#define MV_CESA_INT_PER_PACKET
#endif

/*
 * Use 2K of SRAM
 */
#define MV_CESA_MAX_BUF_SIZE	1600

#endif /* __mvSysCesaConfig_h__ */
