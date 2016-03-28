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

#ifndef TM_NODES_TREE_H
#define TM_NODES_TREE_H

#include "tm_core_types.h"


/**  Change the tree DeQ status.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     status      Tree status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if status is out of range.
 *   @retval TM_HW_TREE_CONFIG_FAIL if download to HW fails.
 */
int tm_tree_change_status(tm_handle hndl, uint8_t status);

#ifdef MV_QMTM_NSS_A0
/**  Change the tree DWRR priority.
 *
 *   @brief: set prios[i] = TM_DISABLE, if DWRR for prio i is disabled,
 *           set prios[i] = TM_ENABLE if DWRR for prio i is enabled
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     prios           Priority array pointer structure.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval TM_HW_TREE_CONFIG_FAIL if download to HW fails.
 */
int tm_tree_set_dwrr_prio(tm_handle hndl, uint8_t *prios);
#endif


#endif   /* TM_NODES_TREE_H */

