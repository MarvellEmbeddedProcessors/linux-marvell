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

#ifndef TM_NODES_CTL_H
#define TM_NODES_CTL_H

#include "tm_core_types.h"

/**  Delete node from scheduling hierarchy.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     level       Scheduling level: Queue/A/B/C-node/Port.
 *   @param[in]     node_id     Node index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -ERANGE if level is out of range.
 *   @retval -EFAULT if node index is out of range.
 *   @retval -ENODATA if node is not in use (free).
 *   @retval -EBUSY if any of children is still in use.
 *
 *   @retval TM_HW_QUEUE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_A_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_B_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_C_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_PORT_CONFIG_FAIL if download to HW fails.
 */
int tm_delete_node(tm_handle hndl, enum tm_level level, uint32_t node_id);


/**  Delete port and all its subtree from scheduling hierarchy.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     index       Port index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port index is out of range.
 *
 *   @retval TM_HW_QUEUE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_A_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_B_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_C_NODE_CONFIG_FAIL if download to HW fails.
 */
int tm_delete_trans_port(tm_handle hndl, uint8_t index);


/***************************************************************************
 * Reshuffling
 ***************************************************************************/

/** Read next tree index/range change after reshuffling.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[out]    change          Change structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -@EINVAL if hndl is NULL.
 *   @retval -@EBADF if hndl is invalid.
 *   @retval -@ENOBUFS if list is empty.
 */
int tm_nodes_read_next_change(tm_handle hndl, struct tm_tree_change *change);


/** Empty list of reshuffling changes.
 *
 *   @param[in]     hndl            TM lib handle.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -@EINVAL if hndl is NULL.
 *   @retval -@EBADF if hndl is invalid.
 */
int tm_clean_list(tm_handle hndl);


#endif   /* TM_NODES_CTL_H */

