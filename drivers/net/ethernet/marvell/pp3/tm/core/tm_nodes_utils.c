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

#include "tm_nodes_utils.h"
#include "tm_errcodes.h"
#include "tm_defs.h"
#include "tm_os_interface.h"
#include "tm_elig_prio_func.h"
#include "set_hw_registers.h"
#include "tm_set_local_db_defaults.h"
#include "tm_hw_configuration_interface.h"


/** Configure the default Eligible Priority Functions
*   to the Queue and Nodes (A...Port) Elig. Prio. Tables
*
*   The tm_config_elig_prio_func_table() configures the
*   default Eligible Priority functions to be usable by the user
*   The configuration is done to all Nodes levels (A...Port) with
*   the same default Eligible functions.
*   Queue level Table is configured with different default functions
*   which are suitable to its operation.
*
*   @param[in]		hndl		TM lib handle
*   @param[in]		updateHW	if 0 - not update HW / otherwise update HW
*
*   @retval zero on success.
*   @retval -EINVAL if hndl is NULL.
*   @retval -EBADF if hndl is an invalid handle.
*	@retval TM_HW_ELIG_PRIO_FUNC_FAILED when the configuration to the HW failed
*/
int tm_config_elig_prio_func_table(tm_handle hndl, int updateHW)
{
	int rc = 0;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* Update SW image */
	set_default_queue_elig_prio_func_table(ctl->tm_elig_prio_q_lvl_tbl);

	set_default_node_elig_prio_func_table(ctl->tm_elig_prio_a_lvl_tbl);
	set_default_node_elig_prio_func_table(ctl->tm_elig_prio_b_lvl_tbl);
	set_default_node_elig_prio_func_table(ctl->tm_elig_prio_c_lvl_tbl);
	set_default_node_elig_prio_func_table(ctl->tm_elig_prio_p_lvl_tbl);

	if (updateHW) {
		/* update HW */
		rc = set_hw_elig_prio_func_tbl_q_level(hndl);
		if (rc) return rc;
		rc = set_hw_elig_prio_func_tbl_a_level(hndl);
		if (rc) return rc;
		rc = set_hw_elig_prio_func_tbl_b_level(hndl);
		if (rc) return rc;
		rc = set_hw_elig_prio_func_tbl_c_level(hndl);
		if (rc) return rc;
		rc = set_hw_elig_prio_func_tbl_p_level(hndl);
		if (rc) return rc;
	}
	if (rc)
		return TM_HW_ELIG_PRIO_FUNC_FAILED;
	return rc;
}
