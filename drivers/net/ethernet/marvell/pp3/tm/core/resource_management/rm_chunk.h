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

#ifndef RM_CHUNK_H
#define RM_CHUNK_H

#include "rm_interface.h"


/** Find index of matching chunk and update internal DB.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]		lvl		          Hierarchy level.
 *   @param[in]		size		      Length of needed range.
 *   @param[out]	index		      Index of node.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOMEM if no free space in needed size.
 */
int rm_get_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t size, uint32_t *index);


/** Add chunk of the nodes that became free and update internal DB.
 *
 *   @param[in]		hndl			  Resource Manager handle.
 *   @param[in]		lvl		          Hierarchy level.
 *   @param[in]		size		      Length of needed range.
 *   @param[in]		index		      Index of node.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_release_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t size, uint32_t index);


/** Find free chunk that includes index and get from it node to expand used nodes range.
 *
 *   @param[in]		hndl				Resource Manager handle.
 *   @param[in]		lvl					Hierarchy level.
 *   @param[in]		index				Index of node.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 *   @retval -ENOMEM if index not found.
 */
int rm_expand_chunk(rmctl_t hndl, enum rm_level lvl, uint32_t index);


/** Create new chunk and add it to free chunk list ( list elements are stay in decending order of chunk size)
 *
 *   @param[in]	    index		  Index of starting node.
 *   @param[in]	    length		  length of chunk
 *   @param[in]	    chunk_list	  Pointer to chunk list to add to
 *
 *   @return  new pointer to updated chunk list
 */
chunk_ptr rm_new_chunk(uint32_t start_index, uint32_t length, struct rm_chunk *chunk_list);


/** Deallocate chunk list and releases allocated memory
 *
 *   @param[in]	    chunk_list	  Pointer to chunk list to free *
 */
void clear_chunk_list(chunk_ptr list);


#endif   /* RM_CHUNK_H */
