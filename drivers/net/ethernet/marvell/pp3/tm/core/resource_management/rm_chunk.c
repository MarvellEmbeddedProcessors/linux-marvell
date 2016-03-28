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

#include "rm_internal_types.h"
#include "rm_chunk.h"
#include "tm/core/tm_os_interface.h"


/* Chunk list is always in decending order - the biggest is first */
/* Chunk list always have member - if no free nodes so it has one chunk with 0 size */


void clear_chunk_list(chunk_ptr list)
{
	chunk_ptr tmp;
	while (list) {
		tmp = list->next_free;
		tm_free(list);
		list = tmp;
	}
}


static void prv_swap_chunk_content(chunk_ptr a, chunk_ptr b)
{
	uint32_t tmp;
	tmp = a->index; a->index = b->index ; b->index = tmp;
	tmp = a->size; a->size = b->size ; b->size = tmp;
}


static void prv_reorder_decreased(chunk_ptr ptr)
{
	while ((ptr->next_free) && (ptr->next_free->size > ptr->size)) {
		prv_swap_chunk_content(ptr, ptr->next_free);
		ptr = ptr->next_free;
	}
	/* remove extra zero sizes chunks */
	if (ptr->size == 0) {
		/* this is first zero size chunk - all chunks after must be also zero-sized and should be removed */
		clear_chunk_list(ptr->next_free);
		/* terminate  list */
		ptr->next_free = 0;
	}
}


static void prv_reorder_increased(chunk_ptr list, chunk_ptr ptr)
{
	while (list != ptr) {
		if (list->size < ptr->size)
			prv_swap_chunk_content(list, ptr);
		list = list->next_free;
	}
}


chunk_ptr rm_new_chunk(uint32_t start_index, uint32_t length, struct rm_chunk *chunk_list)
{
	struct rm_chunk *pchunk = (struct rm_chunk *)tm_malloc(sizeof(struct rm_chunk));
	pchunk->index = start_index;
	pchunk->size = length;
	pchunk->next_free = chunk_list;
	prv_reorder_decreased(pchunk);
	return pchunk;
}


int rm_release_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t size, uint32_t index)
{
	chunk_ptr ptr;
	uint32_t end_index = index + size;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	/* find if the chunk can be merged with already existing free chunk */
	for (ptr = ctl->rm_free_nodes[lvl]; ptr ; ptr = ptr->next_free) {
		if (ptr->index+ptr->size == index) /* chunk is merged after existing free space */
			break;
		if (ptr->index == end_index) { /* chunk is merged before existing free space */
			ptr->index = index;
			break;
		}
	}
	if (ptr) {
		ptr->size += size;
		/* if next chunk starts immediately after this - let merge them */
		if ((ptr->next_free) && (ptr->next_free->index == ptr->index+ptr->size)) {
			ptr->size += ptr->next_free->size;
			ptr->next_free->size = 0;
			prv_reorder_decreased(ptr->next_free);
		}
		/* chunk merged, chunk size increased, need reorder */
		prv_reorder_increased(ctl->rm_free_nodes[lvl], ptr);
	} else {
		/* new free chunk added, reordering inside chunk creation function */
		ctl->rm_free_nodes[lvl] = rm_new_chunk(index, size, ctl->rm_free_nodes[lvl]);
	}
	return 0;
}


int rm_get_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t size, uint32_t *index)
{
	chunk_ptr	ptr;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)

	ptr = ctl->rm_free_nodes[lvl];
	if (ptr->size < size)
		return -ENOMEM; /* no enought continuous nodes pool */
	/* if we want to utilize smallest possible chunk  -  launch string below */
	while ((ptr->next_free) && (ptr->next_free->size > size))
		ptr = ptr->next_free;
	/* here ptr is the last chunk with size >= required size */
	/* otherwize we will get resources from biggest chunk; */
	/* let get pool from chunk and return it's index */
	ptr->size -= size;
	/* we can extract pool from start of chunk */
	*index = ptr->index;
	ptr->index += size;
	/* or extract it fom the end */
	/* *index=ptr->index+ptr->size; */
	prv_reorder_decreased(ptr);
	return 0;
}


int rm_expand_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t index)
{
	chunk_ptr	ptr;

	DECLARE_RM_HANDLE(ctl, hndl)
	CHECK_RM_HANDLE(ctl)


	ptr = ctl->rm_free_nodes[lvl];
	while ((ptr) && (ptr->index != index))
		ptr = ptr->next_free;
	if (ptr && ptr->size) { /* found */
		ptr->index += 1;
		ptr->size -= 1;
		prv_reorder_decreased(ptr);
		return 0;
	} else
		return -ENOMEM;
}
