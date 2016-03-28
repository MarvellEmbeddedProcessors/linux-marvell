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

#ifndef RM_REORDER_H
#define RM_REORDER_H


#include "rm_interface.h"

/** Move nodes from one parent to other.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		level                Level of the nodes to be moved.
 *   @param[in]		from_node_ind        Parent index from which the nodes are moved.
 *   @param[in]		to_node_ind          Parent index to which the nodes are moved.
 *   @param[in]		number_of_children   Number of the nodes.
 *   @param[in]		first_child_to_move  Index of the first node to be moved.
 *
 *   @return an integer positive index of found free queue.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if from/to_node_ind is out of range.
 *   @retval -ENOBUFS when node is not found in free nodes list.
 */
int rm_nodes_move(rmctl_t hndl,
		  enum rm_level level,
		  uint32_t from_node_ind,
		  uint32_t to_node_ind,
		  uint32_t number_of_children,
		  uint32_t first_child_to_move);


/** Switch children between two nodes.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		level                Level of the nodes to be moved.
 *   @param[in]		node_a_ind           First node index.
 *   @param[in]		node_b_ind           Second node index.
 *   @param[in]		first_a_child        First child in range to first node.
 *   @param[in]		last_a_child         Last child in range to first node.
 *   @param[in]		first_b_child        First child in range to second node.
 *   @param[in]		last_b_child         Last child in range to second node.
 *
 *   @return an integer positive index of found free node.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if node_a/b_ind is out of range.
 */
int rm_nodes_switch(rmctl_t hndl,
			enum rm_level level,
			uint32_t node_a_ind,
			uint32_t node_b_ind,
			uint32_t first_a_child,
			uint32_t last_a_child,
			uint32_t first_b_child,
			uint32_t last_b_child);


#endif   /* RM_REORDER_H */

