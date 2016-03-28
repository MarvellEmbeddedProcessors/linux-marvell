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

#ifndef	TM_SCHED_H
#define	TM_SCHED_H

#include "tm_core_types.h"

/** Get Node's minimal quantum size
 * @param[in]	hndl			TM lib handle
 * @param[out]	min_quantum		Min quantum size
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if hndl is an invalid handle
*/
int tm_get_node_min_quantum(tm_handle hndl, uint16_t *min_quantum);


/** Set Maximal Transmission Unit size
 * @param[in]	hndl			TM lib handle
 * @param[in]	mtu				MTU size
 *
 * @note should be called once at system initialization to set default values
 * User's responsibility to keep updated all already configured quantums
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if hndl is an invalid handle
 * @retval -EFAULT if mtu is out of range
*/
int tm_mtu_set(tm_handle hndl, uint32_t mtu);

/** Convert eligible function to priority
 * @param[in]	hndl			TM lib handle
 * @param[in]	level			TM level
 * @param[in]	elig			eligible function
 *
 * @return an integer return code
 * @retval priority on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if hndl is an invalid handle
 * @retval -EFAULT if level is out of range
 * @retval -EBADMSG if elig can't be converted to prio
*/
int tm_elig_to_prio(tm_handle hndl, enum tm_level level, uint8_t elig);

#ifdef MV_QMTM_NSS_A0
/** Configure TM general registers.
 * @param[in]	hndl	           TM lib handle
 * @param[in]	port_ext_bp        En/Dis port external BP
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if hndl is an invalid handle
 * @retval -EFAULT if one of the parameters is out of range
 * @retval  TM_HW_GEN_CONFIG_FAILED if download to HW fails
*/
int tm_sched_general_config(tm_handle hndl, uint8_t port_ext_bp);
#endif


/** Configure Periodic Scheme for level for fixed shaper of 2.5Giga
 * @param[in]	hndl			TM lib handle
 * @param[in]	level			Scheduling level: Queue/A/B/C/Port.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF if hndl is an invalid handle
 * @retval -EFAULT if level is out of range
 * @retval  TM_HW_CONF_PER_SCHEME_FAILED if download to HW fails
*/
int tm_configure_fixed_periodic_scheme_2_5G(tm_handle hndl, enum tm_level level);


#ifdef MV_QMTM_NSS_A0
/** Set the number of DWRR bytes per busrt limit for all ports.
 * @param [in]      hndl	TM lib handle
 * @param [in]      bytes   Number of bytes per burst limit
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL if hndl is NULL
 * @retval -EBADF  if  hndl is an invalid handle
 * @retval -EFAULT if number of bytes is out of range
 * @retval TM_HW_PORT_SET_DWRR_BYTES_BURST_FAILED if download to HW fails.
*/
int tm_port_level_set_dwrr_bytes_per_burst_limit(tm_handle hndl, uint8_t bytes);
#endif

#endif   /* TM_SCHED_H */

