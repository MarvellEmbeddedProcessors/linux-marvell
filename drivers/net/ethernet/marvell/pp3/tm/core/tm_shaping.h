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

#ifndef   	TM_SHAPING_H
#define   	TM_SHAPING_H

#include "tm_core_types.h"

/** Enable/Disable shaping for a given port:
 * @param[in]	hndl	TM lib handle
 * @param[in]	status  Enable/Disable shaping
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if  tm_hndl is an invalid handle
 * @retval -EFAULT if level is invalid
 * @retval TM_HW_CHANGE_SHAPING_STATUS_FAILED if download to HW fails
*/
int tm_fixed_port_shaping_change_status(tm_handle hndl, uint8_t status);


/** Set shaping (CIR & EIR):
 * @param[in]	hndl		TM lib handle
 * @param[in]	level		TM level.
 * @param[in]	index		Port/node/queue index.
 * @param[in]	cbw			Committed shaping BW [in resolution of 1Mb, in steps of 10Mb].
 * @param[in]	ebw			Effective shaping BW [in resolution of 1Mb, in steps of 10Mb].
 *
 * @note: eligible function will be changed accordingly.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if tm_hndl is an invalid handle
 * @retval -EFAULT if cbw/ebw is more than max allowed shaping bw
 * @retval TM_HW_SHAPING_PROF_FAILED if download to HW fails
*/
int tm_set_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw,
					uint32_t ebw);


/** Set minimal shaping (CIR):
 * @param[in]	hndl		TM lib handle
 * @param[in]	level		TM level.
 * @param[in]	index		Port/node/queue index.
 * @param[in]	cbw			Committed shaping BW [in resolution of 1Mb, in steps of 10Mb].
 *
 * @note: eligible function will be changed accordingly.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if tm_hndl is an invalid handle
 * @retval -EFAULT if cbw is more than max allowed shaping bw
 * @retval TM_HW_SHAPING_PROF_FAILED if download to HW fails
*/
int tm_set_min_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw);

/** Set node shapingless:
 * @param[in]	hndl		TM lib handle
 * @param[in]	level		TM level.
 * @param[in]	index		Port/node/queue index.
 *
 * @note: eligible function will be changed accordingly.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if tm_hndl is an invalid handle
 * @retval TM_HW_SHAPING_PROF_FAILED if download to HW fails
*/
int tm_set_no_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index);

/** Read shaping:
 * @param[in]	hndl		TM lib handle
 * @param[in]	level		TM level.
 * @param[in]	index		Port/node/queue index.
 * @param[out]	cir			Committed shaping rate in steps of 10Mb.
 * @param[out]	eir			Extra shaping rate in steps of 10Mb.
 * @param[out]	cbs			Committed burst size in kB.
 * @param[out]	ebs			Extra burst size in kB.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if tm_hndl is an invalid handle
 * @retval -EACCES if level is out of range
 * @retval -EBADMSG if index is out of range
 * @retval -EPERM if port/node/queue doesn't exist
*/
int tm_read_shaping(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t *cir,
					uint32_t *eir,
					uint32_t *cbs,
					uint32_t *ebs);

/** Set shaping (CIR & EIR CBS & EBS):
 * @param[in]	hndl		TM lib handle
 * @param[in]	level		TM level.
 * @param[in]	index		Port/node/queue index.
 * @param[in]	cbw			Committed shaping BW [in resolution of 1Mb, in steps of 10Mb].
 * @param[in]	ebw			Extra shaping BW [in resolution of 1Mb, in steps of 10Mb].
 * @param[in]	pcbs		(pointer to) Committed BurstSize.
 * @param[in]	pcbs		(pointer to) Extra BurstSize.
 *
 * @note: eligible function will be changed accordingly.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if tm_hndl is an invalid handle
 * @retval -EFAULT if cbw/ebw is more than max allowed shaping bw
 * @retval -TM_CONF_MIN_TOKEN_TOO_LARGE if cbs or ebs is too small to provide required b/w
 * @retval TM_HW_SHAPING_PROF_FAILED if download to HW fails
 comment :
	1. If cbs/ebs is less than minimal possible for required b/w - the TM_CONF_MIN_TOKEN_TOO_LARGE
	error is returned & *pcbs  and *ebs are updated to minimal possible values.
	2. If Setting  pcbs/pebs is NULL  the appropriate cbs/ebs parameter is set to default(minimal possible) value
*/

int tm_set_shaping_ex(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t cbw,
					uint32_t ebw,
					uint32_t *pcbs,
					uint32_t *pebs
					);


#endif   /* TM_SHAPING_H */

