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

#ifndef RM_FREE_H
#define RM_FREE_H

#include "rm_interface.h"

/** Free Queue.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		queue_ind	Queue index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if queue_ind is out of range.
 *   @retval -ENOMSG if queue_ind is already free.
 */
int rm_free_queue(rmctl_t hndl, uint32_t queue_ind);


/** Free A-node.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		a_node_ind	A-node index.
 *   @param[in]		range	    Number of children Queues in range.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if a_node_ind is out of range.
 *   @retval -ENOMSG if a_node_ind is already free.
 *   @retval –EBUSY if any of children is still in use.
 */
int rm_free_a_node(rmctl_t hndl, uint32_t a_node_ind, uint32_t range);


/** Free B-node.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		b_node_ind	B-node index.
 *   @param[in]		range	    Number of children A-nodes in range.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if b_node_ind is out of range.
 *   @retval -ENOMSG if b_node_ind is already free.
 *   @retval –EBUSY if any of children is still in use.
 */
int rm_free_b_node(rmctl_t hndl, uint32_t b_node_ind, uint32_t range);


/** Free C-node.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		c_node_ind	C-node index.
 *   @param[in]		range	    Number of children B-nodes in range.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if c_node_ind is out of range.
 *   @retval -ENOMSG if c_node_ind is already free.
 *   @retval –EBUSY if any of children is still in use.
 */
int rm_free_c_node(rmctl_t hndl, uint32_t c_node_ind, uint32_t range);


/** Free Port.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		port_ind	Port index.
 *   @param[in]		range	    Number of children C-nodes in range.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if port_ind is out of range.
 *   @retval -ENOMSG if port_ind is already free.
 *	 @retval -ENOMEM when out of memory space.
 *   @retval –EBUSY if any of children is still in use.
 */
int rm_free_port(rmctl_t hndl, uint8_t port_ind, uint32_t range);


/** Free WRED Queue Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_queue_curve(rmctl_t hndl, uint16_t entry_ind);


/** Free WRED A-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_a_node_curve(rmctl_t hndl, uint16_t entry_ind);


/** Free WRED B-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_b_node_curve(rmctl_t hndl, uint16_t entry_ind);


/** Free WRED C-node Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos			CoS of RED Curve.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_c_node_curve(rmctl_t hndl, uint8_t cos, uint16_t entry_ind);


/** Free WRED Port Curve entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_port_curve(rmctl_t hndl, uint16_t entry_ind);

/* not used for HX/AX*/
/** Free WRED Port Curve entry per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_wred_port_curve_cos(rmctl_t hndl, uint8_t cos, uint16_t entry_ind);



/** Free Queue Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_queue_drop_profile(rmctl_t hndl, uint16_t entry_ind);


/** Free A-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_a_node_drop_profile(rmctl_t hndl, uint16_t entry_ind);


/** Free B-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_b_node_drop_profile(rmctl_t hndl, uint16_t entry_ind);


/** Free C-node Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos			CoS of RED Curve.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_c_node_drop_profile(rmctl_t hndl, uint8_t cos, uint16_t entry_ind);


/** Free Port Drop Profile entry.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_port_drop_profile(rmctl_t hndl, uint16_t entry_ind);


/* not used for HX/AX */
/** Free Port Drop Profile entry per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos		    CoS of RED Curve.
 *   @param[in]		entry_ind	Entry index.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 *   @retval -ENOMSG if entry_ind is already free.
 */
int rm_free_port_drop_profile_cos(rmctl_t hndl, uint8_t cos, uint16_t entry_ind);


#endif   /* RM_FREE_H */
