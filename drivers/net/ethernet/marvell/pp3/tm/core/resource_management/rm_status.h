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

#ifndef RM_STATUS_H
#define RM_STATUS_H

#include "rm_interface.h"
#include "tm/core/tm_defs.h"

/** Get Node status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		lvl			Level of node.
 *   @param[in]		node_ind	Node index.
 *   @param[out]	status		Node status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if node_ind is out of range.
 *   @retval -ERANGE if lvl is out of range.
*/
int rm_node_status(rmctl_t hndl, enum tm_level lvl, uint32_t node_ind,
					uint8_t *status);


/** Get WRED Queue Curve status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_wred_queue_curve_status(rmctl_t hndl, uint8_t entry_ind,
								uint8_t *status);


/** Get WRED A-node Curve status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_wred_a_node_curve_status(rmctl_t hndl, uint8_t entry_ind,
								uint8_t *status);


/** Get WRED B-node Curve status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_wred_b_node_curve_status(rmctl_t hndl, uint8_t entry_ind,
								uint8_t *status);


/** Get WRED C-node Curve status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos	        CoS of RED Curve.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 */
int rm_wred_c_node_curve_status(rmctl_t hndl, uint8_t cos, uint8_t entry_ind,
								uint8_t *status);


/** Get WRED Port Curve status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_wred_port_curve_status(rmctl_t hndl,
							uint8_t entry_ind,
							uint8_t *status);


/* not used for HX/AX */
/** Get WRED Port Curve status per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos	        CoS of RED Curve.
 *   @param[in]		entry_ind	Curve index.
 *   @param[out]	status		Curve status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 */
int rm_wred_port_curve_status_cos(rmctl_t hndl,
								uint8_t cos,
								uint8_t entry_ind,
								uint8_t *status);


/** Get Queue Drop Profile status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_queue_drop_profile_status(rmctl_t hndl, uint16_t entry_ind,
								uint8_t *status);


/** Get A-node Drop Profile status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_a_node_drop_profile_status(rmctl_t hndl, uint16_t entry_ind,
								uint8_t *status);


/** Get B-node Drop Profile status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_b_node_drop_profile_status(rmctl_t hndl, uint16_t entry_ind,
								uint8_t *status);


/** Get C-node Drop Profile status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos	        CoS of RED Curve.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_c_node_drop_profile_status(rmctl_t hndl, uint8_t cos, uint16_t entry_ind,
								uint8_t *status);


/** Get Port Drop Profile status.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind is out of range.
 */
int rm_port_drop_profile_status(rmctl_t hndl,
								uint16_t entry_ind,
								uint8_t *status);



/* not used for Hx/AX*/
/** Get Port Drop Profile status per Cos.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *   @param[in]		cos	        CoS of RED Curve.
 *   @param[in]		entry_ind	Profile index.
 *   @param[out]	status		Profile status.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if entry_ind or cos is out of range.
 */
int rm_port_drop_profile_status_cos(rmctl_t hndl,
									uint8_t cos,
									uint16_t entry_ind,
									uint8_t *status);


#endif   /* RM_STATUS_H */
