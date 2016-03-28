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

#ifndef RM_INTERFACE_H
#define RM_INTERFACE_H

#include "common/mv_sw_if.h"


/** An rm instance handle type.
 */
#define rmctl_t void *

#define RM_FALSE 0
#define RM_TRUE  1


/**
 */
enum rm_level {
	RM_C_LVL = 0,
	RM_B_LVL,
	RM_A_LVL,
	RM_Q_LVL
};


/** RM list element data structure.
 */
struct rm_list_elem {
	uint32_t  index;
	uint8_t   level;
	struct rm_list_elem *next;
};


/** RM list data structure.
 */
struct rm_list {
	struct rm_list_elem *next_ptr;
	struct rm_list_elem *first;
};


#endif   /* RM_INTERFACE_H */
