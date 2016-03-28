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

#ifndef TM_NODES_READ_H
#define TM_NODES_READ_H

#include "tm_core_types.h"


/***************************************************************************
 * Read Configuration
 ***************************************************************************/

/** Read queue software configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     queue_index     Queue index.
 *   @param[out]    q_params        Queue parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if queue_index is out of range.
 *   @retval -ENODATA if queue_index is not in use.
 */
int tm_read_queue_configuration(tm_handle hndl, uint32_t queue_index,
								struct tm_queue_params *q_params);


/** Read A-node software configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     a_node_index    A-node index.
 *   @param[out]    a_params        A-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if a_node_index is out of range.
 *   @retval -ENODATA if a_node_index is not in use.
 */
int tm_read_a_node_configuration(tm_handle hndl, uint32_t a_node_index,
								struct tm_a_node_params *a_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read B-node software configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     b_node_index    B-node index.
 *   @param[out]    b_params        B-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if b_node_index is out of range.
 *   @retval -ENODATA if b_node_index is not in use.
 */
int tm_read_b_node_configuration(tm_handle hndl, uint32_t b_node_index,
								struct tm_b_node_params *b_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read C-node software configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     c_node_index    C-node index.
 *   @param[out]    c_params        C-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if c_node_index is out of range.
 *   @retval -ENODATA if c_node_index is not in use.
 */
int tm_read_c_node_configuration(tm_handle hndl, uint32_t c_node_index,
								struct tm_c_node_params *c_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read Port software configuration.
 *
 *   @param[in]     hndl          TM lib handle.
 *   @param[in]     port_index    Port index.
 *   @param[out]    params        Port parameters structure pointer.
 *
 *   @note The CIR and EIR bw may deviate from the originally configured
 *   by tm_create_port or tm_update_port by the
 *   bw accuracy parameter for P_LEVEL provisioned in tm_configure_periodic_scheme API.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_index is out of range.
 *   @retval -ENODATA if port_index is not in use.
 */
int tm_read_port_configuration(tm_handle hndl, uint32_t port_index,
							struct tm_port_params *params,
							struct tm_port_drop_per_cos *cos_params,
							uint32_t *p_first_child,
							uint32_t *p_last_child);


/** Read queue hardware configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     queue_index     Queue index.
 *   @param[out]    q_params        Queue parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if queue_index is out of range.
 */
int tm_read_queue_configuration_hw(tm_handle hndl, uint32_t queue_index,
								struct tm_queue_params *params);


/** Read A-node hardware configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     a_node_index    A-node index.
 *   @param[out]    a_params        A-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if a_node_index is out of range.
 */
int tm_read_a_node_configuration_hw(tm_handle hndl, uint32_t a_node_index,
								struct tm_a_node_params *a_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read B-node hardware configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     b_node_index    B-node index.
 *   @param[out]    b_params        B-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if b_node_index is out of range.
 */
int tm_read_b_node_configuration_hw(tm_handle hndl, uint32_t b_node_index,
								struct tm_b_node_params *b_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read C-node hardware configuration.
 *
 *   @param[in]     hndl            TM lib handle.
 *   @param[in]     c_node_index    C-node index.
 *   @param[out]    c_params        C-node parameters structure pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if c_node_index is out of range.
 */
int tm_read_c_node_configuration_hw(tm_handle hndl, uint32_t c_node_index,
								struct tm_c_node_params *c_params,
								uint32_t *p_first_child,
								uint32_t *p_last_child);


/** Read Port hardware configuration.
 *
 *   @param[in]     hndl          TM lib handle.
 *   @param[in]     port_index    Port index.
 *   @param[out]    params        Port parameters structure pointer.
 *
 *   @note Drop Profile references only still are taken from SW.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_index is out of range.
 */
int tm_read_port_configuration_hw(tm_handle hndl, uint32_t port_index,
							struct tm_port_params *params,
							struct tm_port_drop_per_cos *cos_params,
							uint32_t *p_first_child,
							uint32_t *p_last_child);

#endif   /* TM_NODES_READ_H */

