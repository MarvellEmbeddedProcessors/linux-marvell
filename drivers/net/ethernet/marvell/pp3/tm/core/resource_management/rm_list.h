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

#ifndef RM_LIST_H
#define RM_LIST_H

#include "rm_interface.h"


/** Create list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[out]	list		      Pointer to list.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOMEM if no free space.
 */
int rm_list_create(rmctl_t hndl, struct rm_list **list);


/** Add index to list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]	    list		      Pointer to list.
 *   @param[in]	    index		      Index of element to be added.
 *   @param[in]	    level		      Level of element to be added.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if list is an invalid pointer.
 *   @retval -ENOMEM if no free space.
 */
int rm_list_add_index(rmctl_t hndl, struct rm_list *list,
						uint32_t index,
						uint8_t level);


/** Next index in the list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]	    list		      Pointer to list.
 *   @param[out]	index		      Next index in list.
 *   @param[out]	level		      Level of next index in list.
 *
 *   @note   At each call the next index in list will be returned. If
 *   other list function called in process (add or delete), next index will start from
 *   the head of list. When end of the list index will get value NULL.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if list is an invalid pointer.
 */
int rm_list_next_index(rmctl_t hndl, struct rm_list *list,
						uint32_t *index,
						uint8_t *level);


/** Reset the pointer of next index in the list to start of the list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]	    list		      Pointer to list.
 *   @param[out]	index		      First index in list.
 *   @param[out]	level		      Level of first index in list.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if list is an invalid pointer.
 */
int rm_list_reset_to_start(rmctl_t hndl, struct rm_list *list, uint32_t *index, uint8_t *level);


/** Delete index from the list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]	    list		      Pointer to list.
 *   @param[in]	    index		      Index in list to be deleted.
 *   @param[in]	    level		      Level in list to be deleted.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if list is an invalid pointer.
 *   @retval -EBADMSG if index is not in list.
 */
int rm_list_del_index(rmctl_t hndl, struct rm_list *list,
						uint32_t index,
						uint8_t level);


/** Forced delete of the list.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]	    list		      Pointer to list.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -EFAULT if list is an invalid pointer.
 */
int rm_list_delete(rmctl_t hndl, struct rm_list *list);


#endif   /* RM_LIST_H */
