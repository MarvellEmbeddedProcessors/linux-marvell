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

#include "rm_list.h"
#include "tm/core/tm_os_interface.h"

/* needed for RM_HANDLE  but not nesessary for the code ???*/
#include "rm_internal_types.h"


/**
 */
int rm_list_create(rmctl_t hndl, struct rm_list **list_ptr)
{
	struct rm_list *list = NULL;

	list = (struct rm_list *)tm_malloc(sizeof(struct rm_list));
	if (list == NULL)
		return -ENOMEM;

	list->next_ptr = NULL;
	list->first = NULL;

	*list_ptr = list;

	return 0;
}


/**
 */
int rm_list_add_index(rmctl_t hndl, struct rm_list *list, uint32_t index,
	uint8_t level)
{
	struct rm_list_elem *elem = NULL;

	if (!list)
		return -EFAULT;

	elem = tm_malloc(sizeof(struct rm_list_elem));
	if (elem == NULL)
		return -ENOMEM;

	elem->index = index;
	elem->level = level;
	elem->next = list->first;

	list->next_ptr = elem;
	list->first = elem;

	return 0;
}


/**
 */
int rm_list_next_index(rmctl_t hndl, struct rm_list *list, uint32_t *index,
	uint8_t *level)
{
	if (!list)
		return -EFAULT;

	if (list->next_ptr == NULL) {
		*index = 0xFFFFFFFF;
		*level = 0xFF;
		/* reset next pointer to start if the end of the list have
			* been reached */
		list->next_ptr = list->first;
		return 0;
	}

	*index = list->next_ptr->index;
	*level = list->next_ptr->level;
	list->next_ptr = list->next_ptr->next;

	return 0;
}


/**
 */
int rm_list_reset_to_start(rmctl_t hndl, struct rm_list *list, uint32_t *index, uint8_t *level)
{
	if (!list)
		return -EFAULT;

	list->next_ptr = list->first;

	if (list->first) {
		*index = list->first->index;
		*level = list->first->level;
		list->next_ptr = list->first->next;
	} else {
		*index = 0xFFFFFFFF;
		*level = 0xFF;
	}
	return 0;
}


/**
 */
int rm_list_del_index(rmctl_t hndl, struct rm_list *list, uint32_t index,
	uint8_t level)
{
	struct rm_list_elem *elem = NULL;
	struct rm_list_elem *prev = NULL;

	if (!list)
		return -EFAULT;

	/* find element */
	list->next_ptr = list->first;
	while (list->next_ptr) {
		if ((list->next_ptr->index == index)
			&& (list->next_ptr->level == level)) {
			elem = list->next_ptr;
			if (prev == NULL) /* first in the list */
				list->first = elem->next;
			else /* not a first */
				prev->next = elem->next;
			tm_free(elem);
			list->next_ptr = list->first; /* reset to start */
			return 0;
		}
		prev = list->next_ptr;
		list->next_ptr = list->next_ptr->next;
	}

	if (!elem)
		return -EBADMSG;

	return 0;
}


/**
 */
int rm_list_delete(rmctl_t hndl, struct rm_list *list)
{
	struct rm_list_elem *elem = NULL;

	if (!list)
		return -EFAULT;

	while (list->first) {
		elem = list->first;
		list->first = elem->next;
		tm_free(elem);
	}
	tm_free(list);
	return 0;
}

