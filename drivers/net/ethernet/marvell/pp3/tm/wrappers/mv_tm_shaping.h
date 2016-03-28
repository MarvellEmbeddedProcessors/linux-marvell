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

#ifndef MV_TM_SHAPING__H
#define MV_TM_SHAPING__H

#include "common/mv_sw_if.h"
#include "tm/mv_tm.h"

/* Maximal Shaping Rate [1Mbps]*/
#define MV_TM_MAX_SHAPING_BW	TM_MAX_SHAPING_BW

/*
set shaping (CIR & EIR) [in resolution of 1Mb, in steps of 10Mb]  and CBS & EBS [in KB]
passing NULL to pcbs/pebs causes to set default values to cbs/ebs
*/
int mv_tm_set_shaping_ex(enum mv_tm_level level, uint32_t index,
						uint32_t cir, uint32_t eir, uint32_t *pcbs,	uint32_t *pebs);

/* set shaping (CIR & EIR) [in resolution of 1Mb, in steps of 10Mb] */
int mv_tm_set_shaping(enum mv_tm_level level, uint32_t index, uint32_t cir, uint32_t eir);

/* set minimal shaping (CIR) [in resolution of 1Mb, in steps of 10Mb] */
int mv_tm_set_min_shaping(enum mv_tm_level level, uint32_t index, uint32_t cir);

/* disable shaping for node */
int mv_tm_set_no_shaping(enum mv_tm_level level, uint32_t index);


/* show shaping
get shaping (CIR & EIR) [in resolution of 1Mb, in steps of 10Mb]
passing NULL to parameter prevents it from reading
*/
int mv_tm_get_shaping(enum mv_tm_level level, uint32_t index, uint32_t *cir, uint32_t *eir);


/* retrieve shaping parameters:
shaping (CIR & EIR) [in resolution of 1Mb, in steps of 10Mb]  and CBS & EBS [in KB]
passing NULL to parameter prevents it from reading
*/

int mv_tm_get_shaping_ex(enum mv_tm_level level, uint32_t index,
						uint32_t *cir, uint32_t *eir, uint32_t *pcbs, uint32_t *pebs);

/* retrieve all shaping concerning information:

eligible function :
cir_eir_mask :  if (mask & 1)- cir shaper used, if (mask & 2) - eir shaper used, if mask ==0 - shaping not used
shaping (CIR & EIR) [in resolution of 1Mb, in steps of 10Mb]  and CBS & EBS [in KB]
passing NULL to parameter prevents it from reading
*/


int mv_tm_get_shaping_full_info(enum mv_tm_level level, uint32_t index, uint8_t *elig_fun, uint8_t *cir_eir_mask,
						uint32_t *cir, uint32_t *eir, uint32_t *pcbs, uint32_t *pebs);

#endif /* MV_TM_SHAPING__H */
