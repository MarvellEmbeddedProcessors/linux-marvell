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

#include "tm_sched.h"
#include "tm_errcodes.h"
#include "tm_nodes_utils.h"
#include "tm_locking_interface.h"
#include "set_hw_registers.h"
#include "tm_set_local_db_defaults.h"
#include "tm_os_interface.h"
#include "tm_hw_configuration_interface.h"

/**
*/
int tm_get_node_min_quantum(tm_handle hndl, uint16_t *min_quantum)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	*min_quantum = ctl->min_quantum;
	return rc;
}

/**
*/
int tm_mtu_set(tm_handle hndl, uint32_t mtu)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (mtu > TM_MAX_MTU)
		return -EFAULT;

	ctl->mtu = mtu;
	ctl->min_quantum = (mtu + ctl->min_pkg_size)/TM_NODE_QUANTUM_UNIT;
	return rc;
}

/**
*/
int tm_elig_to_prio(tm_handle hndl, enum tm_level level, uint8_t elig)
{
	uint8_t type;
	uint8_t prio;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* check parameters */
	if (level > P_LEVEL)
		return -EFAULT;

	DECODE_ELIGIBLE_FUN(elig, type, prio);
	if (level == Q_LEVEL) {
		if (IS_VALID_Q_TYPE_PRIO(type, prio)) /* valid eligible function */
			return prio;
		else
			return -EBADMSG;
	} else {
		if (IS_VALID_N_TYPE_PRIO(type, prio)) {
			/* valid eligible function */
			if (prio == PRIO_P) /* propagated priority found */
				return -1;
			else
				return prio;
		} else
			return -EBADMSG;
	}
}

#ifdef MV_QMTM_NSS_A0
/**
*/
int tm_sched_general_config(tm_handle hndl, uint8_t port_ext_bp)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)


	rc = tm_glob_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	if ((port_ext_bp != TM_ENABLE) && (port_ext_bp != TM_DISABLE))
	{
		rc = -EFAULT;
		goto out;
	}

	/* update relevant fields in hndl */
	ctl->port_ext_bp_en = port_ext_bp;

	rc = set_hw_gen_conf(hndl);
	if (rc < 0)
		rc = TM_HW_GEN_CONFIG_FAILED;

out:
	if (rc)
	{
		if (rc > 0)
			set_sw_gen_conf_default(hndl);
	}
	tm_glob_unlock(TM_ENV(ctl));
	return rc;
}
#endif


int tm_configure_fixed_periodic_scheme_2_5G(tm_handle hndl, enum tm_level level)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	switch (level) {
	case Q_LEVEL:
		rc = set_hw_fixed_queue_periodic_scheme(hndl);
		break;
	case A_LEVEL:
		rc = set_hw_fixed_a_level_periodic_scheme(hndl);
		break;
	case B_LEVEL:
		rc = set_hw_fixed_b_level_periodic_scheme(hndl);
		break;
	case C_LEVEL:
		rc = set_hw_fixed_c_level_periodic_scheme(hndl);
		break;
	case P_LEVEL:
		rc = set_hw_fixed_port_periodic_scheme(hndl);
		break;
	default:
		rc = -EFAULT;
		goto out;
	}
	if (rc) {
		rc = TM_HW_CONF_PER_SCHEME_FAILED;
		goto out;
	}

	ctl->periodic_scheme_state = TM_ENABLE;
out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}


#ifdef MV_QMTM_NSS_A0
/**
 */
int tm_port_level_set_dwrr_bytes_per_burst_limit(tm_handle hndl, uint8_t bytes)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	rc = tm_sched_lock(TM_ENV(ctl));
	if (rc)
		return rc;

	/* check parameters validity */
	if (bytes > 0x7F)
	{
		rc = -EFAULT;
		goto out;
	}

	ctl->dwrr_bytes_burst_limit = bytes;

	rc = set_hw_dwrr_limit(hndl);
	if (rc < 0)
	{
		/* set to default */
		ctl->dwrr_bytes_burst_limit = 0x40;
		rc = TM_HW_PORT_DWRR_BYTES_PER_BURST_FAILED;
	}

out:
	tm_sched_unlock(TM_ENV(ctl));
	return rc;
}
#endif
