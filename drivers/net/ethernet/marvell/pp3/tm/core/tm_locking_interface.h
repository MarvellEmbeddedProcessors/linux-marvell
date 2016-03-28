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

#ifndef TM_LOCKING_INTERFACE_H
#define TM_LOCKING_INTERFACE_H


int	tm_create_locking_staff(void * environment_handle);
int	tm_destroy_locking_staff(void * environment_handle);


/** Lock the TM nodes configuration data from other threads.
 *
 * @param[in]   hndl    TM lib handle
 *
 * @return an integer return code.
 * @retval 0 on success.
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 */
int tm_nodes_lock(void * environment_handle);

/** Lock the TM Global configuration data from other threads.
 */
int tm_glob_lock(void * environment_handle);

/** Lock the TM scheduling configuration data from other threads.
 */
int tm_sched_lock(void * environment_handle);

/** Unlock TM nodes configuration data for other threads.
 *
 * @param[in]   hndl    TM lib handle
 *
 * @return an integer return code.
 * @retval 0 on success.
 * @retval -EINVAL if hndl is NULL.
 * @retval -EBADF if hndl is invalid.
 */
int tm_nodes_unlock(void * environment_handle);

/** Unlock the TM Global configuration data from other threads.
 */
int tm_glob_unlock(void * environment_handle);

/** Unlock the TM Scheduling configuration data from other threads.
 */
int tm_sched_unlock(void * environment_handle);


#endif   /* TM_LOCKING_INTERFACE_H */

