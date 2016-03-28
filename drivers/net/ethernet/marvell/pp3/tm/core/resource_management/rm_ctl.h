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

#ifndef RM_CTL_H
#define RM_CTL_H

#include "rm_interface.h"

/** Create RM handle.
 *
 *   @param[in]     hlog			         generic log handle.
 *   @param[in]		total_ports		         Total num Ports.
 *   @param[in]		total_c_nodes		     Total num C-nodes.
 *   @param[in]		total_b_nodes		     Total num B-nodes.
 *   @param[in]		total_a_nodes		     Total num A-nodes.
 *   @param[in]		total_queues		     Total num Queues.
 *   @param[out]    hndl					RM handle pointer.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -ENOMEM when out of memory space.
 */
int rm_open(uint8_t total_ports,
			uint16_t total_c_nodes,
			uint16_t total_b_nodes,
			uint16_t total_a_nodes,
			uint32_t total_queues,
			rmctl_t *hndl);


/** Close RM handle.
 *
 *   @param[in]		hndl		Resource Manager handle.
 *
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle
 */
int rm_close(rmctl_t hndl);


#endif   /* RM_CTL_H */
