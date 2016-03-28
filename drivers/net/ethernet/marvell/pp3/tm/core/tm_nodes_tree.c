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

#include "tm_nodes_tree.h"

#include "tm_locking_interface.h"
#include "tm_errcodes.h"

#include "set_hw_registers.h"


/**
 */
int tm_tree_change_status(tm_handle hndl, uint8_t status)
{
	int rc;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)


	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	if ((status != TM_ENABLE) && (status != TM_DISABLE))
	{
		rc = -EFAULT;
		goto out;
	}

	ctl->tree_deq_status = status;

	rc = set_hw_tree_deq_status(hndl);
	if (rc < 0)
		rc = TM_HW_TREE_CONFIG_FAIL;

out:
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}


#ifdef MV_QMTM_NSS_A0
/**
 */
int tm_tree_set_dwrr_prio(tm_handle hndl, uint8_t * prios)
{
	int rc ;
	int i;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)


	rc = tm_nodes_lock(TM_ENV(ctl));
	if (rc)
		return rc;
	ctl->tree_dwrr_priority = 0;
	for (i = 0; i < 8; i++)
	{
		if ((prios[i] != TM_ENABLE) && (prios[i] != TM_DISABLE))
		{
			rc = -EFAULT;
			goto out;
		}
		ctl->tree_dwrr_priority = ctl->tree_dwrr_priority | (prios[i] << i);
	}

	rc = set_hw_tree_dwrr_priority(hndl);
	if (rc)
		rc = TM_HW_TREE_CONFIG_FAIL;

out:
	if (rc < 0)
		ctl->tree_dwrr_priority = 0;
	tm_nodes_unlock(TM_ENV(ctl));
	return rc;
}
#endif
