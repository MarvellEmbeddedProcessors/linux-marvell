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

#ifndef TM_ELIG_PRIO_H
#define TM_ELIG_PRIO_H

#include "tm_core_types.h"

/** Configure the Eligible Priority Function according
*   to the User Application parameters
*
*	The new Eligible Priority Function is written to the
*	proper Queue or Node level Eligible Table.
*	A pointer in the API is provided to the location of the new Eligible Priority Function in the table.
*
*	Note:	The use of Eligible Function Pointer 63 on both Queue and Node level is forbidden.
*			Eligible Function 63 is reserved to indicate that Queue/Node is not eligible.
*
*	@param[in]	hndl					TM lib handle
*	@param[in]	elig_prio_func_ptr		The new configured eligible function pointer
*	@param[in]	level					A level to configure the Eligible function with
*	@param[in]	func_out_arr			The Eligible Priority Function structure array pointer
*
*   @retval zero on success.
*   @retval -EINVAL if hndl is NULL.
*   @retval -EBADF if hndl is an invalid handle.
*	@retval -EDOM  if (elig_prio_func_ptr > 63)
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/
int tm_elig_prio_func_config(tm_handle hndl,
							 uint16_t elig_prio_func_ptr,
							 enum tm_level level,
							 union tm_elig_prio_func *func_out_arr);


/** Configure the Eligible Priority Function according
*   to the User Application parameters
*
*	The following API configures the same Eligible Priority Functions
*	at all nodes (A, B, C, and Port) levels Elig. Prio. Tables
*	according to the user?s parameters.
*	It has the same functionality as tm_elig_prio_func_config()
*	and can be used at the user convenience to configure the
*	same eligible function to all the Nodes levels (except for Q level)
*
*	Note:	The use of Eligible Function Pointer 63 on both Queue and Node level is forbidden.
*			Eligible Function 63 is reserved to indicate that Queue/Node is not eligible.
*
*	@param[in]	hndl					TM lib handle
*	@param[in]	elig_prio_func_ptr		The new configured eligible function pointer
*	@param[in]	func_out_arr			The Eligible Priority Function structure array pointer
*
*   @retval zero on success.
*   @retval -EINVAL if hndl is NULL.
*   @retval -EBADF if hndl is an invalid handle.
*	@retval -EDOM  if (elig_prio_func_ptr > 63)
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/
int tm_elig_prio_func_config_all_levels(tm_handle hndl,
										uint16_t elig_prio_func_ptr,
										union tm_elig_prio_func *func_out_arr);


/** Configure the Eligible Priority Function to Node
*
*	@param[in]	hndl					TM lib handle
*	@param[in]	level					Node level (P/C/B/A)
*	@param[in]	index					Node index
*	@param[in]	prio					The Eligible Priority Function pointer
*
*   @retval zero on success.
*   @retval -EINVAL  if hndl is NULL.
*   @retval -EBADF   if hndl is an invalid handle.
*	@retval -EACCES  if level/prio is out of range
*	@retval -EBADMSG if index is out of range
*	@retval -EPERM if node is not in use
*	@retval -ENODATA if unsupported case
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/
int tm_node_elig_set(tm_handle hndl, enum tm_level level, uint32_t index, uint8_t prio);


/** Configure the Eligible Priority Function to Queue
*
*	@param[in]	hndl					TM lib handle
*	@param[in]	index					Queue index
*	@param[in]	prio					The Eligible Priority Function pointer
*
*   @retval zero on success.
*   @retval -EINVAL  if hndl is NULL.
*   @retval -EBADF   if hndl is an invalid handle.
*	@retval -EACCES  if prio is out of range
*	@retval -EBADMSG if index is out of range
*	@retval -EPERM if queue is not in use
*	@retval -ENODATA if unsupported case
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/
int tm_queue_elig_set(tm_handle hndl, uint32_t index, uint8_t prio);


/** Configure the Eligible Node Priority as propagated priority (without shaping)
*
*	@param[in]	hndl					TM lib handle
*	@param[in]	level					Node level (P/C/B/A)
*	@param[in]	index					Node index

*   @retval zero on success.
*   @retval -EINVAL  if hndl is NULL.
*   @retval -EBADF   if hndl is an invalid handle.
*	@retval -EACCES  if level/prio is out of range
*	@retval -EBADMSG if index is out of range
*	@retval -EPERM if node is not in use
*	@retval -ENODATA if unsupported case
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/

int tm_node_elig_set_propagated(tm_handle hndl, enum tm_level level, uint32_t index);


int tm_get_node_elig_prio_fun_info(tm_handle hndl, enum tm_level level, uint32_t index, uint8_t *prio, uint8_t *mask);



int is_queue_elig_fun_uses_shaper(struct tm_elig_prio_func_queue *queue_func_table,
									uint8_t func_index);

int is_node_elig_fun_uses_shaper(struct tm_elig_prio_func_node *node_func_table,
									uint8_t func_index);


/* Auxiliary function
uint16_t convert_elig_func_to_value(struct tm_elig_prio_func_out *elig_func);

void convert_value_to_elig_func(uint16_t elig_val, struct tm_elig_prio_func_out *elig_func);

 */
#endif   /* TM_ELIG_PRIO_H */

