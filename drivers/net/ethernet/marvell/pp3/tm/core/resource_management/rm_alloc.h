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

#ifndef	RM_ALLOC_H
#define	RM_ALLOC_H

#include "rm_interface.h"
#include "tm/core/tm_defs.h"


/** Find free Queue.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		a_node_ind	     A-node index.
 *
 *   @return an integer positive index of found free queue.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if a_node_ind is out of range.
 *   @retval -ENOBUFS when no free queues.
 */
int rm_find_free_queue(rmctl_t hndl, uint32_t a_node_ind);


/** Find free A-node.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		b_node_ind	     B-node index.
 *   @param[in]		num_of_children	 Children nodes number.
 *
 *   @return an integer positive index of found free node.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if b_node_ind is out of range.
 *   @retval -ENOBUFS when no free A-nodes.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_find_free_a_node(rmctl_t hndl, uint32_t b_node_ind, uint32_t num_of_children);


/** Find free B-node.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		c_node_ind	     C-node index.
 *   @param[in]		num_of_children	 Children nodes number.
 *
 *   @return an integer positive index of found free node.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if c_node_ind is out of range.
 *   @retval -ENOBUFS when no free B-nodes.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_find_free_b_node(rmctl_t hndl, uint32_t c_node_ind, uint32_t num_of_children);


/** Find free C-node.
 *
 *   @param[in]		hndl		     Resource Manager handle.
 *   @param[in]		port_ind	     Port index.
 *   @param[in]		num_of_children	 Children nodes number.
 *
 *   @return an integer positive index of found free node.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if port_ind is out of range.
 *   @retval -ENOBUFS when no free C-nodes.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_find_free_c_node(rmctl_t hndl, uint8_t port_ind, uint32_t num_of_children);


/** Init Port in RM.
 *
 *   @param[in]		hndl			Resource Manager handle.
 *   @param[in]		port_id	        Port index.
 *   @param[in]		num_of_children	Children nodes number.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if port_id is out of range.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_init_port(rmctl_t hndl, uint8_t port_id, uint32_t num_of_children);


/** Expand range of the parent by adding the node to it's range.
 *
 *   @param[in]		hndl			Resource Manager handle.
 *   @param[in]		level	                Level of the node.
 *   @param[in]		node	                Index of the node.
 *   @param[in]		parent	                Index of the parent.
 *
 *   @node: node index should be adjacent to the index of the last child node
 *   in range of this parent.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if one of indises is out of range.
 *   @retval -ENOMEM when out of memory space or node index is not found.
 */
int rm_expand_range(rmctl_t hndl, enum tm_level level, uint32_t node, uint32_t parent);


/** Find free WRED Queue Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_queue_curve(rmctl_t hndl);


/** Find free WRED A-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_a_node_curve(rmctl_t hndl);


/** Find free WRED B-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_b_node_curve(rmctl_t hndl);


/** Find free WRED C-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if cos is out of range.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_c_node_curve(rmctl_t hndl, uint8_t cos);


/** Find free WRED Port Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_port_curve(rmctl_t hndl);


/* BC2 only */
/** Find free WRED Port Curve entry per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if cos is out of range.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_wred_port_curve_cos(rmctl_t hndl, uint8_t cos);


/** Find free Queue Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_queue_drop_profile(rmctl_t hndl);


/** Find free A-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_a_node_drop_profile(rmctl_t hndl);


/** Find free B-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_b_node_drop_profile(rmctl_t hndl);


/** Find free C-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if cos is out of range.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_c_node_drop_profile(rmctl_t hndl, uint8_t cos);


/** Find free Port Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_port_drop_profile(rmctl_t hndl);


/* BC2 only */
/** Find free Port Drop Profile entry per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *
 *   @return an integer positive index of found free entry.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if cos is out of range.
 *   @retval -ENOBUFS when no free entry.
 */
int rm_find_free_port_drop_profile_cos(rmctl_t hndl, uint8_t cos);


#endif   /* RM_ALLOC_H */
