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

#ifndef TM_NODES_UPDATE_H
#define TM_NODES_UPDATE_H

#include "tm_core_types.h"

/***************************************************************************
 * Parameters Update
 ***************************************************************************/

/** Update queue parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @note wred_profile_ref parameter will be updated in any case,
 *   set it's value to be the same as in DB if you don't want to change it.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     queue_index     Queue index to be updated.
 *   @param[in]     q_params        Queue parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if queue_index is out of range.
 *   @retval -ENODATA if queue_index is not in use.
 *
 *   @retval TM_CONF_Q_SHAPING_PROF_REF_OOR.
 *   @retval TM_CONF_Q_PRIORITY_OOR.
 *   @retval TM_CONF_Q_QUANTUM_OOR.
 *   @retval TM_CONF_Q_WRED_PROF_REF_OOR.
 *
 *   @retval TM_CONF_ELIG_PRIO_FUNC_ID_OOR if eligible function id is oor
 *   @retval TM_HW_ELIG_PRIO_FUNC_FAILED if queue eligible function
 *   pointer download to HW failed .
 *
 *   @retval TM_HW_QUEUE_CONFIG_FAIL if download to HW fails..
 */
int tm_update_queue(tm_handle hndl, uint32_t queue_index,
					struct tm_queue_params *q_params);


/** Update A-node parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     a_node_index    A-node index to be updated.
 *   @param[in]     a_params        A-node parameters structure pointer.
 *
 *   @note 'num_of_children' cann't be updated.
 *   @note wred_profile_ref parameter will be updated in any case,
 *   set it's value to be the same as in DB if you don't want to change it.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if a_node_index is out of range.
 *   @retval -ENODATA if a_node_index is not in use.
 *
 *   @retval TM_CONF_A_SHAPING_PROF_REF_OOR.
 *   @retval TM_CONF_A_PRIORITY_OOR.
 *   @retval TM_CONF_A_QUANTUM_OOR.
 *   @retval TM_CONF_A_DWRR_PRIO_OOR.
 *   @retval TM_CONF_A_WRED_PROF_REF_OOR.
 *
 *   @retval TM_CONF_ELIG_PRIO_FUNC_ID_OOR if eligible function id is oor
 *   @retval TM_HW_A_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_ELIG_PRIO_FUNC_FAILED if A node eligible function
 *   pointer download to HW failed .
 */
int tm_update_a_node(tm_handle hndl, uint32_t a_node_index,
					 struct tm_a_node_params *a_params);


/** Update B-node parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     b_node_index    B-node index to be updated.
 *   @param[in]     b_params        B-node parameters structure pointer.
 *
 *   @note 'num_of_children' cann't be updated.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if b_node_index is out of range.
 *   @retval -ENODATA if b_node_index is not in use.
 *
 *   @retval TM_CONF_B_SHAPING_PROF_REF_OOR.
 *   @retval TM_CONF_B_PRIORITY_OOR.
 *   @retval TM_CONF_B_QUANTUM_OOR.
 *   @retval TM_CONF_B_DWRR_PRIO_OOR.
 *   @retvak TM_CONF_B_WRED_PROF_REF_OOR.
 *   @retval TM_CONF_ELIG_PRIO_FUNC_ID_OOR if eligible function id is oor
 *
 *   @retval TM_HW_B_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_ELIG_PRIO_FUNC_FAILED if B node eligible function
 *   pointer download to HW failed .
 */
int tm_update_b_node(tm_handle hndl, uint32_t b_node_index,
					 struct tm_b_node_params *b_params);


/** Update C-node parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *   @note  wred_cos can only be updated together with wred_profile_ref.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     c_node_index    C-node index to be updated.
 *   @param[in]     c_params        C-node parameters structure pointer.
 *
 *   @note 'num_of_children' cann't be updated.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if c_node_index is out of range.
 *   @retval -ENODATA if c_node_index is not in use.
 *
 *   @retval TM_CONF_C_SHAPING_PROF_REF_OOR.
 *   @retval TM_CONF_C_PRIORITY_OOR.
 *   @retval TM_CONF_C_QUANTUM_OOR.
 *   @retval TM_CONF_C_DWRR_PRIO_OOR.
 *   @retval TM_CONF_C_WRED_PROF_REF_OOR.
 *   @retval TM_CONF_C_WRED_COS_OOR.
 *   @retval TM_CONF_ELIG_PRIO_FUNC_ID_OOR if eligible function id is oor
 *
 *   @retval TM_HW_C_NODE_CONFIG_FAIL if download to HW fails.
 *   @retval TM_HW_ELIG_PRIO_FUNC_FAILED if C node eligible function
 *   pointer download to HW failed .
 */
int tm_update_c_node(tm_handle hndl, uint32_t c_node_index,
					 struct tm_c_node_params *c_params);


/** Update Port Scheduling parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     port_id         Port index to be updated.
 *   @param[in]     elig_prio_func  Eligible Priority Function pointer.
 *   @param[in]     quantum         Port quantum 8 cells array.
 *   @param[in]     dwrr_priority   Port dwrr priority pointer for C-level.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_index is out of range.
 *   @retval -ENODATA if port_index is not in use.
 *
 *   @retval TM_CONF_PORT_PRIORITY_OOR.
 *   @retval TM_CONF_PORT_QUANTUM_OOR.
 *   @retval TM_CONF_PORT_DWRR_PRIO_OOR.
 *   @retval TM_CONF_ELIG_PRIO_FUNC_ID_OOR if eligible function id is oor
 *
 *   @retval TM_HW_ELIG_PRIO_FUNC_FAILED if port eligible function
 *   pointer download to HW failed .
 *   @retval TM_HW_PORT_CONFIG_FAIL if download to HW fails.
 */
int tm_update_port_scheduling(tm_handle hndl, uint8_t port_id,
							uint8_t elig_prio_func_ptr,
#ifdef MV_QMTM_NSS_A0
							uint16_t *quantum,
#endif
							uint8_t *dwrr_priority);


/** Update Port Drop parameters.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @param[in]     hndl                TM lib handle.
 *   @param[in]     port_index          Port index to be updated.
 *   @param[in]     wred_profile_ref    Port Drop Profile reference.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_index is out of range.
 *   @retval -ENODATA if port_index is not in use.
 *
 *   @retval TM_CONF_P_WRED_PROF_REF_OOR.
 *
 *   @retval TM_HW_PORT_CONFIG_FAIL if download to HW fails.
 */
int tm_update_port_drop(tm_handle hndl,
						uint8_t port_index,
						uint8_t wred_profile_ref);

/** Update Port Drop parameters per Cos.
 *
 *   @brief when error occurs, the entry is considered inconsistent.
 *
 *   @param[in]     hndl                TM lib handle.
 *   @param[in]     port_index          Port index to be updated.
 *   @param[in]     params              Port Drop parameters structure per Cos.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_index is out of range.
 *   @retval -ENODATA if port_index is not in use.
 *
 *   @retval TM_CONF_P_WRED_PROF_REF_OOR.
 *   @retval TM_HW_PORT_CONFIG_FAIL if download to HW fails.
 */
int tm_update_port_drop_cos(tm_handle hndl,
							uint8_t index,
							struct tm_port_drop_per_cos *params);

#endif   /* TM_NODES_UPDATE_H */

