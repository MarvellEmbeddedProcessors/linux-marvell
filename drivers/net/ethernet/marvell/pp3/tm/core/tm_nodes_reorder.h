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

#ifndef TM_NODES_REORDER_H
#define TM_NODES_REORDER_H

#include "tm_core_types.h"


 /** Move a number of children from one parent node to another.
  *  The two nodes must have children with adjecent indicies.
  *  The from_node must have number_of_children + 1 children.
  *
  *  @param[in]    hndl                 TM lib handle
  *  @param[in]    level                Node level of parents
  *  @param[in]    number_of_children   The number of children to move
  *  @param[in]    from_node            Parent node to move children from
  *  @param[in]    to_node              Parent node to move children to
  *
  *  @return an integer return code
  *  @retval 0 on success
  *  @retval -EINVAL if hndl is NULL
  *  @retval -EBADF if hndl is invalid
  *  @retval -ERANGE if level is invalid
  *  @retval TM_CONF_[LEVEL]_NODE_IND_OOR if node index is not available on level
  *  @retval TM_CONF_REORDER_CHILDREN_NOT_AVAIL if from_node does not have enough children for operation
  *  @retval TM_CONF_REORDER_NODES_NOT_ADJECENT if from_node and to_node do not have adjecent children nodes
  */
int tm_nodes_move(tm_handle hndl,
				  enum tm_level level,
				  uint16_t number_of_children,
				  uint16_t from_node,
				  uint16_t to_node);


 /** Switch children between two nodes.
  *
  *  @param[in]    hndl                 TM lib handle
  *  @param[in]    level                Node level
  *  @param[in]    node_a               Node A index in switch
  *  @param[in]    node_b               Node B index in switch
  *
  *  @return an integer return code
  *  @retval 0 on success
  *  @retval -EINVAL if hndl is NULL
  *  @retval -EBADF if hndl is invalid
  *  @retval -ERANGE if level is invalid
  *  @retval TM_CONF_[LEVEL]_NODE_IND_OOR if node index is not available on level
  */
int tm_nodes_switch(tm_handle hndl,
					enum tm_level level,
					uint16_t node_a,
					uint16_t node_b);


#endif   /* TM_NODES_REORDER_H */

