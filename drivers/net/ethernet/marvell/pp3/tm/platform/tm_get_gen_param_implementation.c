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

#include "tm_get_gen_param_interface.h"
#include "tm_set_local_db_defaults.h"
#include "tm_errcodes.h"
#include "set_hw_registers.h"
#include "tm_hw_configuration_interface.h"


int tm_get_gen_params(tm_handle hndl)
{
	int rc = 0;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* get other general parameters */
	ctl->min_pkg_size = 0x40; /* in Chunks (64 bytes), reset value: 0x20 (2kb) */
	ctl->mtu = 2000;          /* bytes, Maximal Transmission Unit */
#ifdef MV_QMTM_NSS_A0
	ctl->port_ch_emit
#endif
	ctl->min_quantum = (ctl->mtu + ctl->min_pkg_size)/TM_NODE_QUANTUM_UNIT;

	return rc;
}
