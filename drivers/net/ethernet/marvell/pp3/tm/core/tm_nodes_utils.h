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

#ifndef TM_NODES_H
#define TM_NODES_H

#include "tm_core_types.h"

/** Configure the default Eligible Priority Functions
*   to the Queue and Nodes (A...Port) Elig. Prio. Tables
*
*   The tm_config_elig_prio_func_table() configures the
*   default Eligible Priority functions to be usable by the user
*   The configuration is done to all Nodes levels (A...Port) with
*   the same default Eligible functions.
*   Queue level Table is configured with different default functions
*   which are suitable to its operation.
*
*   @param[in]		hndl		TM lib handle
*   @param[in]		updateHW	if 0 - not update HW / otherwise update HW
*
*   @retval zero on success.
*   @retval -EINVAL if hndl is NULL.
*   @retval -EBADF if hndl is an invalid handle.
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/

int tm_config_elig_prio_func_table(tm_handle hndl, int updateHW);


#endif   /* TM_NODES_H */

