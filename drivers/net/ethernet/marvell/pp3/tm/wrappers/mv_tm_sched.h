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

#ifndef MV_TM_SCHED__H
#define MV_TM_SCHED__H

#include "common/mv_sw_if.h"
#include "tm/mv_tm.h"
#include "tm_sched.h"

/* Number of priorities */
#define MV_TM_NUM_OF_PRIO			8

/* get Node's minimal & maximal quantum size */
int mv_tm_quantum_range_get(uint32_t mtu, uint32_t *min, uint32_t *max);

/* set Maximal Transmission Unit size */
/* note: should be called once at system initialization to set default values */
/* User's responsibility to keep updated all already configured quantums */
int mv_tm_mtu_set(uint32_t mtu);

/* set the tree DeQ status */
int mv_tm_tree_status_set(int status);

/* set Queue/Node/Port priority */
int mv_tm_prio_set(enum mv_tm_level level, uint32_t index, uint8_t prio);

/* set  propagated priority  to Node/Port*/
int mv_tm_prio_set_propagated(enum mv_tm_level level, uint32_t index);

/* set DWRR weight (quantum) to Queue/Node */
int mv_tm_dwrr_weight(enum mv_tm_level level, uint32_t index, uint32_t quantum);

/* enable/disable DWRR to the given priority on the Queue/Node */
int mv_tm_dwrr_enable(enum mv_tm_level level, uint32_t index, uint8_t prio, int en);

#endif /* MV_TM_SCHED__H */
