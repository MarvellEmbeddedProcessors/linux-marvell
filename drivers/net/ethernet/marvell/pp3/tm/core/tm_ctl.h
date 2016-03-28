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

#ifndef   	TM_CTL_H
#define   	TM_CTL_H

#include "tm_core_types.h"	 /* in order to define tm_handle */


/** Initialize TM configuration library.
 *
 *   @param[in]		cProductName	Product Name.
 *   @param[in]		hEnv			TM handle storage.
 *   @param[out]	htm				Pointer to TM lib handle.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if any of handles is NULL.
 *   @retval -EBADF if any of handles is invalid.
 *   @retval -ENOMEM when out of memory space.
 *
 *   @retval TM_CONF_INVALID_PROD_NAME.
 */
int tm_lib_open(const char * cProductName, tm_handle hEnv, tm_handle *htm);


/**
 * @brief   Initiate TM related H/W resources.
 *
 * @param[in]    hndl    TM lib handle
 *
 * @return an integer return code.
 * @retval zero on success.
 * @retval -EINVAL if any of handles is NULL.
 * @retval -EBADF if any of handles is invalid.
 *
 * @retval TM_HW_GEN_CONFIG_FAILED.
 */
int tm_lib_init_hw(tm_handle hndl);


/** Close TM configuration library.
 *
 *   @param[in]		hndl		TM lib handle
 *
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is an invalid handle.
 */
int tm_lib_close(tm_handle hndl);


/**
 * @brief   Initiate TM H/W resources to default.
 *
 * @param[in]    hndl    TM lib handle
 *
 * @return an integer return code.
 * @retval zero on success.
 * @retval -EINVAL if any of handles is NULL.
 * @retval -EBADF if any of handles is invalid.
 *
 * @retval TM_HW_GEN_CONFIG_FAILED.
 */
int tm_lib_init_hw_def(tm_handle hndl);


#endif   /* TM_CTL_H */

