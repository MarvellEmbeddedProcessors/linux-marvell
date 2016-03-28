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

#include "tm_sysfs_shaping.h"
#include "mv_tm_shaping.h"
#include "tm/mv_tm.h"
#include "tm_shaping.h"
#include "tm_hw_configuration_interface.h"

static char *level_names_arr[] = { "Q", "A", "B", "C", "P"};

int tm_sysfs_read_shaping(void)
{
	uint32_t profiles_num = 0;
	int level;
	uint32_t cir = 0;
	uint32_t eir = 0;
	int i;
	uint32_t total_nodes = 0;

	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	for (level = P_LEVEL; level >= Q_LEVEL; level--) {
		switch (level) {
		case P_LEVEL:
			total_nodes = get_tm_port_count();
			break;
		case C_LEVEL:
			total_nodes = get_tm_c_nodes_count();
			break;
		case B_LEVEL:
			total_nodes = get_tm_b_nodes_count();
			break;
		case A_LEVEL:
			total_nodes = get_tm_a_nodes_count();
			break;
		case Q_LEVEL:
			total_nodes = get_tm_queues_count();
			break;
		default:
			rc = -1;
			TM_WRAPPER_END(tm_hndl);
		}
		rc = 0;
		profiles_num = 0;
		cir = 0;
		eir = 0;
		for (i = 0; i < total_nodes; i++) {
			rc = tm_read_shaping(ctl,
				TM_LEVEL(level),
				i,
				&cir,
				&eir,
				NULL,
				NULL);
			if (rc)
				continue;
			if ((cir != TM_MAX_SHAPING_BW) || (eir != TM_MAX_SHAPING_BW))
				profiles_num++;
		}
		if (profiles_num != 0)
			pr_info("TM %s Level Shaping Configurations: %d\n",
					level_names_arr[level], profiles_num);
		else
			pr_info("TM %s Level Shaping Configurations: None\n",
					level_names_arr[level]);
	}

	rc = 0;

	TM_WRAPPER_END(tm_hndl);
}

