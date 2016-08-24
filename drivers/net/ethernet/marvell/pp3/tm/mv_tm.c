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

#include "mv_tm.h"
#include "tm_os_interface.h"
#include "tm_core_types.h"
#include "tm_alias.h"
#include "tm_locking_interface.h"
#include "tm_ctl.h"
#include "tm_rw_registers_interface.h"
#include "tm_drop.h"
#include "tm_sched.h"
#include "tm_shaping.h"
#include "tm_nodes_create.h"
#include "tm_nodes_status.h"
#include "tm_nodes_update.h"
#include "set_hw_registers.h"

struct qmtm *qmtm_hndl;
enum mv_tm_config mv_active_config;
void __iomem *tm_regs_base;
const char *tm_prod_str;


static const char *tm_config_str(enum mv_tm_config config)
{
	const char *str;

	switch (config) {
	case TM_DEFAULT_CONFIG:
		str = "default";
		break;
	case TM_CFG1_CONFIG:
		str = "cfg1";
		break;
	case TM_2xPPC_CONFIG:
		str = "2xppc";
		break;
	case TM_CFG3_CONFIG:
		str = "cfg3";
		break;
	default:
		str = "Unknown";
	}
	return str;
}

int tm_global_init(void __iomem *base, const char *prod_str)
{
	if (tm_regs_base || tm_prod_str) {
		pr_info("%s is already called\n", __func__);
		return -1;
	}
	if (!base || !prod_str) {
		pr_info("%s: invalid parameters\n", __func__);
		return -1;
	}
	tm_regs_base = base;
	tm_prod_str = prod_str;

	return 0;
}

int tm_open(void)
{
	struct qmtm * henv;
	int rc = 0;

	if (!tm_regs_base || !tm_prod_str) {
		pr_err("%s: tm_global_init is not called yet\n", __func__);

	}
	if (qmtm_hndl) {
		pr_info("tm_open is already called\n");
		return 0;
	}

	henv = (struct qmtm *)tm_malloc(sizeof(struct qmtm));
	if (henv == NULL) {
		pr_err("malloc of TM handler failed");
		return -1;
	}

	mv_active_config = TM_INVALID_CONFIG;


	tm_memset(henv, 0, sizeof(struct qmtm));
	henv->magic = TM_MAGIC;
	qmtm_hndl = henv;
	init_tm_alias_struct(tm_regs_base);
	init_tm_init_offset_struct();
	rc = tm_create_locking_staff(henv);
	if (rc)
		goto out;

	tm_debug_on = TM_DISABLE;

	rc = tm_lib_open(tm_prod_str, henv, (tm_handle *)(&(henv->tmctl)));
	if (rc) {
		rc = tm_to_qmtm_errcode(rc);
		pr_err("TM lib open failed");
		goto out;
	}

	rc = set_hw_connection(henv);
	if (rc)
		goto out;

	rc = tm_lib_init_hw(henv->tmctl);
	if (rc) {
		rc = tm_to_qmtm_errcode(rc);
		pr_err("TM lib init failed");
		goto out;
	}

out:
	if (rc) {
		tm_lib_close(henv->tmctl);
		tm_free(henv);
		qmtm_hndl = NULL;
		pr_err("TM open failed");
	} else
		pr_info("TM open completed successfuly\n");

	return rc;
}
EXPORT_SYMBOL(tm_open);

int tm_close(void)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	/* close library handle, if opened  */
	if (ctl) {
		/* release tm API library resources*/
		rc = tm_lib_close(ctl);
		if (rc) {
			rc = tm_to_qmtm_errcode(rc);
			goto out;
		}
	}

	/* Attempt close anyway
	 */
	close_hw_connection(henv);

	/* Free environment
	 */
	tm_destroy_locking_staff(henv);

	tm_free(henv);
	qmtm_hndl = NULL;
out:
	TM_WRAPPER_END(hndl);
}
EXPORT_SYMBOL(tm_close);

int tm_check_args(struct qmtm *hndl, struct tm_ctl **ctl, struct qmtm **env)
{
	if (hndl == NULL)
		return -EINVAL;
	if (hndl->magic != TM_MAGIC)
		return -EBADF;
	if ((hndl->tmctl) && (hndl->tmctl->magic != TM_MAGIC))
		return -EBADF;

	*ctl = hndl->tmctl;
	*env = NULL;

	return 0;
}

int tm_drop_profiles_config(tm_handle hndl)
{
	uint16_t index; /* drop profile index */
	uint8_t i, j;
	int rc;

	for (i = 0; i < 15/*(TM_NUM_PORT_DROP_PROF-1)*/; i++) { /* Drop Profile #0 reserved for NO_DROP */
		rc = tm_create_drop_profile_2_5G(hndl, P_LEVEL, (uint8_t)TM_INVAL, &index);
		if (rc) {
			pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
			return rc;
		}

		for (j = 0; j < TM_WRED_COS; j++) {
			rc = tm_create_drop_profile_2_5G(hndl, P_LEVEL, j, &index);
			if (rc) {
				pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
				return rc;
			}
		}
	}

	for (i = 0; i < (TM_NUM_C_NODE_DROP_PROF-1); i++) { /* Drop Profile #0 reserved for NO_DROP */
		for (j = 0; j < TM_WRED_COS; j++) {
			rc = tm_create_drop_profile_2_5G(hndl, C_LEVEL, j, &index);
			if (rc) {
				pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
				return rc;
			}
		}
	}

	for (i = 0; i < (TM_NUM_B_NODE_DROP_PROF-1); i++) { /* Drop Profile #0 reserved for NO_DROP */
		rc = tm_create_drop_profile_2_5G(hndl, B_LEVEL, j, &index);
		if (rc) {
			pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
			return rc;
		}
	}

	for (i = 0; i < (TM_NUM_A_NODE_DROP_PROF-1); i++) { /* Drop Profile #0 reserved for NO_DROP */
		rc = tm_create_drop_profile_2_5G(hndl, A_LEVEL, j, &index);
		if (rc) {
			pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
			return rc;
		}
	}

	for (i = 0; i < (TM_NUM_QUEUE_DROP_PROF-1); i++) { /* Drop Profile #0 reserved for NO_DROP */
		rc = tm_create_drop_profile_2_5G(hndl, Q_LEVEL, j, &index);
		if (rc) {
			pr_info("*** (TM_SCN) ***: tm_drop_profiles_config: rc %d.\n", rc);
			return rc;
		}
	}

	return 0;
}

int tm_build_tree_under_port(tm_handle hndl, uint8_t port_index,
				uint16_t num_of_c_nodes,
				uint16_t num_of_b_nodes,
				uint16_t num_of_a_nodes,
				uint32_t num_of_queues)
{
	struct tm_c_node_params c_params;
	struct tm_b_node_params b_params;
	struct tm_a_node_params a_params;
	struct tm_queue_params  q_params;

	int rc = 0;
	uint32_t i, n;
	/* relative node index in children range under some parent node */
	uint32_t queue, a_node, b_node, c_node;

	uint32_t q_ind;              /* Queue index pointer  */
	uint32_t a_ind;              /* A-node index pointer */
	uint32_t b_ind;              /* B-node index pointer */
	uint32_t c_ind;              /* C-node index pointer */

	q_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	q_params.quantum = 0x40;

	a_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	a_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	a_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		a_params.dwrr_priority[i] = TM_DISABLE;

	b_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	b_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	b_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		b_params.dwrr_priority[i] = TM_DISABLE;

	c_params.wred_cos = 0;
	c_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		c_params.wred_profile_ref[i] = TM_NO_DROP_PROFILE;
	c_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		c_params.dwrr_priority[i] = TM_DISABLE;

	a_params.num_of_children = num_of_queues;
	b_params.num_of_children = num_of_a_nodes;
	c_params.num_of_children = num_of_b_nodes;

	for (c_node = 0; c_node < num_of_c_nodes; c_node++) {
		/* Connects C nodes to the port (port_index), returns the created C node index */
		rc = tm_create_c_node_to_port(hndl, port_index, &c_params, &c_ind);
		if (rc) {
			pr_info("*** (TM_SCN) *** tm_create_c_node_to_port: rc %d.\n", rc);
			return rc;
		}

		for (b_node = 0; b_node < num_of_b_nodes; b_node++) {
			if (mv_active_config == TM_2xPPC_CONFIG) {
				if (port_index == TM_A0_PORT_PPC0_1) {
					if ((c_node == 0) && (b_node == 0)) /* B1 */
						b_params.num_of_children = 5;
					else if ((c_node == 0) && (b_node == 1)) /* B2 */
						b_params.num_of_children = 6;
					else
						b_params.num_of_children = num_of_a_nodes;
				}
			}
			/* Connects B nodes to C nodes (c_ind), returns the created B node index */
			rc = tm_create_b_node_to_c_node(hndl, c_ind, &b_params, &b_ind);
			if (rc) {
				pr_info("*** (TM_SCN) *** tm_create_b_node_to_c_node: rc %d.\n", rc);
				return rc;
			}

			for (a_node = 0; a_node < b_params.num_of_children; a_node++) {
				/* Connects A nodes to B nodes (b_ind), returns the created A node index */
				rc = tm_create_a_node_to_b_node(hndl, b_ind, &a_params, &a_ind);
				if (rc) {
					pr_info("*** (TM_SCN) *** tm_create_a_node_to_b_node: rc %d.\n", rc);
					return rc;
				}

				for (queue = 0; queue < num_of_queues; queue++) {
					n = queue%4;
					switch (n) {
					case 0: /* P0 */
						q_params.elig_prio_func_ptr = TM_ELIG_Q_FIXED_P0;
						break;

					case 1: /* P1 */
						q_params.elig_prio_func_ptr = TM_ELIG_Q_FIXED_P1;
						break;

					case 2: /* P2 */
						q_params.elig_prio_func_ptr = TM_ELIG_Q_FIXED_P2;
						break;

					case 3: /* P3 */
						q_params.elig_prio_func_ptr = TM_ELIG_Q_FIXED_P3;
						break;

					default:
						return -3;
					}

					/* Connects queue to A nodes (a_ind), returns the created queue index */
					/* In quantum of 4 queues per A node                                  */
					rc = tm_create_queue_to_a_node(hndl, a_ind, &q_params, &q_ind);
					if (rc) {
						pr_info("*** (TM_SCN) *** tm_create_queue_to_a_node. rc %d\n", rc);
						return rc;
					}
				 }
			}
		}
	}

	return rc;
}

int tm_defzero(void)
{
	struct tm_ctl *hndl = NULL;
	int rc = 0;

	pr_info("*** TM DefZero Scenario ***\n");

	/* Write default registers */
	rc = tm_lib_init_hw_def(hndl);
	if (rc == 0)
		pr_info("TM DefZero Scenario Completed Successfuly\n");
	else
		pr_info("TM DefZero Scenario Completed with Error %d\n", rc);
	return rc;
}

/* Default scenario configuration */
int tm_defcon(void)
{
	struct tm_ctl *hndl;
	struct tm_port_params p_params;
	enum tm_port_bw port_speed = TM_10G_PORT;

	uint32_t cbw = MV_TM_2HG_BW; /* shaping rate 2.5G */
	uint16_t dp_index = 1; /* drop profile index */
	uint8_t port_index;
	uint8_t i;
	int rc;
	uint16_t num_of_c_nodes = 0, num_of_b_nodes = 0, num_of_a_nodes = 0, num_queues = 0;
	enum tm_level lvl;

	if (!qmtm_hndl) {
		rc = tm_open();
		if (rc)
			goto err_out;
	}

	if (mv_active_config != TM_INVALID_CONFIG) {
		pr_warn("Warning: TM already configured to %s, cannot reconfig TM\n", tm_config_str(mv_active_config));
		return 0;
	}

	pr_info("*** TM DefCon Scenario ***\n");
	hndl = qmtm_hndl->tmctl;
	mv_active_config = TM_DEFAULT_CONFIG;

	/* Drop configuration - Drop profiles: out_BW = 2.5Gbps, Color blind TD */
	rc = tm_drop_profiles_config(hndl);
	if (rc) {
		rc = -3;
		goto err_out;
	}

	for (lvl = Q_LEVEL; lvl <= P_LEVEL; lvl++) {
		rc = tm_configure_fixed_periodic_scheme_2_5G(hndl, lvl);
		if (rc) {
			rc = -3;
			goto err_out;
		}
	}
	p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	p_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		p_params.dwrr_priority[i] = TM_DISABLE;

	/* Build TM Tree */
	for (port_index = TM_A0_PORT_PPC0_0; port_index <= TM_A0_PORT_DROP1; port_index++) {
		num_of_c_nodes = 1;
		if ((port_index == TM_A0_PORT_PPC0_0) || (port_index == TM_A0_PORT_HMAC)) {
			/* 128 queues */
			num_of_b_nodes = 8;
			num_of_a_nodes = 4;
			num_queues     = 4;
		} else if (port_index == TM_A0_PORT_PPC0_1) {
			/* 64 queues */
			num_of_b_nodes = 4;
			num_of_a_nodes = 4;
			num_queues     = 4;
		} else if ((port_index == TM_A0_PORT_PPC1_MNT0) || (port_index == TM_A0_PORT_PPC1_MNT1)) {
			num_of_b_nodes = 0;
			num_of_a_nodes = 0;
			num_queues     = 0;
		} else if (port_index == TM_A0_PORT_EMAC0) {
			/* 32 queues */
			num_of_b_nodes = 2;
			num_of_a_nodes = 4;
			num_queues     = 4;
		} else {
			/* All other ports has 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			num_queues     = 4;
		}

		switch (port_index) {
		case TM_A0_PORT_PPC0_0:
		case TM_A0_PORT_PPC0_1:
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_PPC1_MNT0:
		case TM_A0_PORT_PPC1_MNT1:
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_EMAC0:
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_EMAC1:
		case TM_A0_PORT_EMAC2:
		case TM_A0_PORT_EMAC3:
		case TM_A0_PORT_EMAC4:
		case TM_A0_PORT_EMAC_LPB:
			port_speed = TM_1G_PORT;
			break;
		case TM_A0_PORT_CMAC_IN:
		case TM_A0_PORT_CMAC_LA:
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_HMAC:
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_UNUSED0:
			/* These ports do not exists */
			continue;
		case TM_A0_PORT_DROP0:
		case TM_A0_PORT_DROP1:
			port_speed	 = TM_10G_PORT;
			break;
		}

		if (port_index == TM_A0_PORT_EMAC0) {
			/* 2.5G "shaping" drop */
			p_params.wred_profile_ref = (uint8_t) dp_index;
			rc = tm_create_port(hndl, port_index, port_speed, &p_params,
				num_of_c_nodes,
				(num_of_b_nodes*num_of_c_nodes),
				(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
				(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		} else {
			/* No shaping */
			p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
			if ((port_index == TM_A0_PORT_PPC1_MNT0) || (port_index == TM_A0_PORT_PPC1_MNT1))
				rc = tm_create_asym_port(hndl, port_index, port_speed, &p_params);
			else
				rc = tm_create_port(hndl, port_index, port_speed, &p_params,
					num_of_c_nodes,
					(num_of_b_nodes*num_of_c_nodes),
					(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
					(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		}
		if (rc) {
			pr_info("*** (TM_SCN) *** tm_create_port: rc %d.\n", rc);
			pr_info("*** (TM_SCN) *** port_index              : %d\n", port_index);
			pr_info("*** (TM_SCN) *** port_speed              : %d\n", port_speed);
			pr_info("*** (TM_SCN) *** num_of_c_nodes          : %d\n", num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_b_c_nodes        : %d\n", num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_nodes      : %d\n",
				num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_queue_nodes: %d\n",
				num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			goto err_out;
		}

		if ((port_index == TM_A0_PORT_PPC1_MNT0) || (port_index == TM_A0_PORT_PPC1_MNT1))
			continue;

		rc = tm_build_tree_under_port(hndl, port_index,
			num_of_c_nodes, num_of_b_nodes, num_of_a_nodes, num_queues);
		if (rc)
			goto err_out;
	}

	/* 2.5G shaping on port EMAC0 */
	rc = tm_set_min_shaping(hndl, P_LEVEL, TM_A0_PORT_EMAC0, cbw);

err_out:
	if (rc == 0)
		pr_info("TM DefCon Scenario Completed Successfuly\n");
	else
		pr_info("TM DefCon Scenario Completed with Error %d\n", rc);
	return rc;
}
EXPORT_SYMBOL(tm_defcon);

/* cfg1 scenario configuration */
int tm_cfg1(void)
{
	struct tm_ctl *hndl;
	struct tm_port_params p_params;
	enum tm_port_bw port_speed = TM_10G_PORT;

	uint8_t port_index;
	uint8_t i;
	int rc;
	uint16_t num_of_c_nodes = 0, num_of_b_nodes = 0, num_of_a_nodes = 0, num_queues = 0;
	enum tm_level lvl;

	if (!qmtm_hndl) {
		rc = tm_open();
		if (rc)
			goto err_out;
	}

	if (mv_active_config != TM_INVALID_CONFIG) {
		pr_warn("Warning: TM already configured to %s, cannot reconfig TM\n", tm_config_str(mv_active_config));
		return 0;
	}

	pr_info("*** TM cfg1 Scenario ***\n");
	hndl = qmtm_hndl->tmctl;
	mv_active_config = TM_CFG1_CONFIG;

	/* Drop configuration - Drop profiles: out_BW = 2.5Gbps, Color blind TD */
	rc = tm_drop_profiles_config(hndl);
	if (rc) {
		rc = -3;
		goto err_out;
	}

	for (lvl = Q_LEVEL; lvl <= P_LEVEL; lvl++) {
		rc = tm_configure_fixed_periodic_scheme_2_5G(hndl, lvl);
		if (rc) {
			rc = -3;
			goto err_out;
		}
	}
	p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	p_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		p_params.dwrr_priority[i] = TM_DISABLE;

	/* Build TM Tree */
	for (port_index = TM_A0_PORT_PPC0_0; port_index <= TM_A0_PORT_DROP1; port_index++) {
		num_of_c_nodes = 1;

		switch (port_index) {
		case TM_A0_PORT_PPC0_0:
			/* 128 queues */
			num_of_b_nodes = 8;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_PPC0_1:
		case TM_A0_PORT_PPC1_MNT0:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_PPC1_MNT1:
			/* 32 queues */
			num_of_b_nodes = 2;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_EMAC0:
			/* 32 queues */
			num_of_b_nodes = 2;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_EMAC1:
		case TM_A0_PORT_EMAC2:
		case TM_A0_PORT_EMAC3:
		case TM_A0_PORT_EMAC4:
		case TM_A0_PORT_EMAC_LPB:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_1G_PORT;
			break;
		case TM_A0_PORT_CMAC_IN:
			/* 64 queues */
			num_of_b_nodes = 4;
			num_of_a_nodes = 4;
			num_queues     = 4;
			break;
		case TM_A0_PORT_CMAC_LA:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_HMAC:
			/* 80 queues */
			num_of_b_nodes = 5;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_UNUSED0:
		case TM_A0_PORT_DROP0:
		case TM_A0_PORT_DROP1:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			num_queues     = 4;
			port_speed	 = TM_10G_PORT;
			break;
		}

		if (port_index == TM_A0_PORT_EMAC0) {
			p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
			rc = tm_create_port(hndl, port_index, port_speed, &p_params,
				num_of_c_nodes,
				(num_of_b_nodes*num_of_c_nodes),
				(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
				(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		} else {
			/* No shaping */
			p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
			if (port_index == TM_A0_PORT_PPC0_1)
				rc = tm_create_asym_port(hndl, port_index, port_speed, &p_params);
			else
				rc = tm_create_port(hndl, port_index, port_speed, &p_params,
					num_of_c_nodes,
					(num_of_b_nodes*num_of_c_nodes),
					(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
					(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		}
		if (rc) {
			pr_info("*** (TM_SCN) *** tm_create_port: rc %d.\n", rc);
			pr_info("*** (TM_SCN) *** port_index              : %d\n", port_index);
			pr_info("*** (TM_SCN) *** port_speed              : %d\n", port_speed);
			pr_info("*** (TM_SCN) *** num_of_c_nodes          : %d\n", num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_b_c_nodes        : %d\n", num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_nodes      : %d\n",
				num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_queue_nodes: %d\n",
				num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			goto err_out;
		}
/*
		if (port_index == TM_A0_PORT_PPC0_1)
			continue;
*/
		rc = tm_build_tree_under_port(hndl, port_index,
			num_of_c_nodes, num_of_b_nodes, num_of_a_nodes, num_queues);
		if (rc)
			goto err_out;
	}

err_out:
	if (rc == 0)
		pr_info("TM cfg1 Scenario Completed Successfuly\n");
	else
		pr_info("TM cfg1 Scenario Completed with Error %d\n", rc);
	return rc;
}
EXPORT_SYMBOL(tm_cfg1);


/* 2xPPC scenario configuration */
int tm_2xppc(void)
{
	struct tm_ctl *hndl;
	struct tm_port_params p_params;
	enum tm_port_bw port_speed = TM_10G_PORT;

	uint32_t cbw = MV_TM_2HG_BW; /* shaping rate 2.5G */
	uint16_t dp_index = 1; /* drop profile index */
	uint8_t port_index;
	uint8_t i;
	int rc;
	uint16_t num_of_c_nodes = 0, num_of_b_nodes = 0, num_of_a_nodes = 0, num_queues = 0;
	enum tm_level lvl;

	if (!qmtm_hndl) {
		rc = tm_open();
		if (rc)
			goto err_out;
	}

	if (mv_active_config != TM_INVALID_CONFIG) {
		pr_warn("Warning: TM already configured to %s, cannot reconfig TM\n", tm_config_str(mv_active_config));
		return 0;
	}

	pr_info("*** TM 2xPPC Scenario ***\n");
	hndl = qmtm_hndl->tmctl;
	mv_active_config = TM_2xPPC_CONFIG;

	/* Drop configuration - Drop profiles: out_BW = 2.5Gbps, Color blind TD */
	rc = tm_drop_profiles_config(hndl);
	if (rc) {
		rc = -3;
		goto err_out;
	}

	for (lvl = Q_LEVEL; lvl <= P_LEVEL; lvl++) {
		rc = tm_configure_fixed_periodic_scheme_2_5G(hndl, lvl);
		if (rc) {
			rc = -3;
			goto err_out;
		}
	}
	p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	p_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		p_params.dwrr_priority[i] = TM_DISABLE;

	/* Build TM Tree */
	for (port_index = TM_A0_PORT_PPC0_0; port_index <= TM_A0_PORT_DROP1; port_index++) {
		num_of_c_nodes = 1;
		num_queues     = 4;

		switch (port_index) {
		case TM_A0_PORT_PPC0_0:
			/* 4 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 1;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_PPC0_1:
			/* non-equal queue distribution */
			num_of_b_nodes = 7;
			num_of_a_nodes = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_PPC1_MNT0:
		case TM_A0_PORT_PPC1_MNT1:
			/* 32 queues */
			num_of_b_nodes = 2;
			num_of_a_nodes = 4;
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_EMAC0:
			/* 32 queues */
			num_of_b_nodes = 2;
			num_of_a_nodes = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_EMAC1:
		case TM_A0_PORT_EMAC2:
		case TM_A0_PORT_EMAC3:
		case TM_A0_PORT_EMAC4:
		case TM_A0_PORT_EMAC_LPB:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			port_speed = TM_1G_PORT;
			break;
		case TM_A0_PORT_CMAC_IN:
			/* 64 queues */
			num_of_b_nodes = 4;
			num_of_a_nodes = 4;
			break;
		case TM_A0_PORT_CMAC_LA:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			port_speed = TM_10G_PORT;
			break;
		case TM_A0_PORT_HMAC:
			/* 80 queues */
			num_of_b_nodes = 5;
			num_of_a_nodes = 4;
			port_speed = TM_2HG_PORT;
			break;
		case TM_A0_PORT_UNUSED0:
		case TM_A0_PORT_DROP0:
		case TM_A0_PORT_DROP1:
			/* 16 queues */
			num_of_b_nodes = 1;
			num_of_a_nodes = 4;
			port_speed	 = TM_10G_PORT;
			break;
		}

		if (port_index == TM_A0_PORT_EMAC0) {
			/* 2.5G "shaping" drop */
			p_params.wred_profile_ref = (uint8_t) dp_index;
			rc = tm_create_port(hndl, port_index, port_speed, &p_params,
				num_of_c_nodes,
				(num_of_b_nodes*num_of_c_nodes),
				(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
				(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		} else {
			/* No shaping */
			p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
			if (port_index == TM_A0_PORT_PPC0_1)
				rc = tm_create_asym_port(hndl, port_index, port_speed, &p_params);
			else
				rc = tm_create_port(hndl, port_index, port_speed, &p_params,
					num_of_c_nodes,
					(num_of_b_nodes*num_of_c_nodes),
					(num_of_a_nodes*num_of_b_nodes*num_of_c_nodes),
					(num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes));
		}
		if (rc) {
			pr_info("*** (TM_SCN) *** tm_create_port: rc %d.\n", rc);
			pr_info("*** (TM_SCN) *** port_index              : %d\n", port_index);
			pr_info("*** (TM_SCN) *** port_speed              : %d\n", port_speed);
			pr_info("*** (TM_SCN) *** num_of_c_nodes          : %d\n", num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_b_c_nodes        : %d\n", num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_nodes      : %d\n",
				num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			pr_info("*** (TM_SCN) *** num_of_a_b_c_queue_nodes: %d\n",
				num_queues*num_of_a_nodes*num_of_b_nodes*num_of_c_nodes);
			goto err_out;
		}

		rc = tm_build_tree_under_port(hndl, port_index,
			num_of_c_nodes, num_of_b_nodes, num_of_a_nodes, num_queues);
		if (rc)
			goto err_out;
	}

	/* 2.5G shaping on port EMAC0 */
	rc = tm_set_min_shaping(hndl, P_LEVEL, TM_A0_PORT_EMAC0, cbw);

err_out:
	if (rc == 0)
		pr_info("TM 2xPPC Scenario Completed Successfuly\n");
	else
		pr_info("TM 2xPPC Scenario Completed with Error %d\n", rc);
	return rc;
}
EXPORT_SYMBOL(tm_2xppc);

int __tm_create_default_c_node(tm_handle hndl, uint8_t port_index,
				uint16_t num_of_b_children,
				uint32_t *c_ind_ptr)
{
	struct tm_c_node_params c_params;
	int i;
	int rc = 0;

	c_params.wred_cos = 0;
	c_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		c_params.wred_profile_ref[i] = TM_NO_DROP_PROFILE;
	c_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		c_params.dwrr_priority[i] = TM_DISABLE;
	c_params.num_of_children = num_of_b_children;

	/* Connects C nodes to the port (port_index), returns the created C node index */
	rc = tm_create_c_node_to_port(hndl, port_index, &c_params, c_ind_ptr);
/* */	if (rc)
		pr_info("*** (TM_SCN) *** tm_create_c_node_to_port:  port:%d rc %d.\n", port_index, rc);

	return rc;
}
int __tm_create_default_b_node(tm_handle hndl, uint16_t c_node_index,
				uint16_t num_of_a_children,
				uint32_t *b_ind_ptr)
{
	struct tm_b_node_params b_params;
	int i;
	int rc = 0;
	b_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	b_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	b_params.quantum = 0x40;
	b_params.num_of_children = num_of_a_children;
	for (i = 0; i < 8; i++)
		b_params.dwrr_priority[i] = TM_DISABLE;

	rc = tm_create_b_node_to_c_node(hndl, c_node_index, &b_params, b_ind_ptr);
/* */	if (rc)
		pr_info("*** (TM_SCN) *** tm_create_b_node_to_c_node:  c_node : %d , rc %d.\n", c_node_index , rc);

	return rc;
}

int __tm_create_default_a_node_with_queues(tm_handle hndl, uint16_t b_node_index,
				uint16_t num_of_queues,
				uint32_t *a_ind_ptr, uint32_t *last_queue_ptr)
{
	struct tm_a_node_params a_params;
	struct tm_queue_params  q_params;
	int i;

	int rc = 0;
	/* relative node index in children range under some parent node */
	uint32_t queue;


	a_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	a_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	a_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		a_params.dwrr_priority[i] = TM_DISABLE;
	a_params.num_of_children = num_of_queues;

	q_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	q_params.quantum = 0x40;
	q_params.elig_prio_func_ptr = TM_ELIG_Q_DEFAULT;

	/* Connects A node to B node (b_ind), returns the created A node index */
	rc = tm_create_a_node_to_b_node(hndl, b_node_index, &a_params, a_ind_ptr);
	if (rc) {
		pr_info("*** (TM_SCN) *** tm_create_a_node_to_b_node:  b_node:%d , rc %d.\n", b_node_index, rc);
		return rc;
	}
	/* create default queues to A-node */
	for (queue = 0; queue < num_of_queues; queue++) {
		/* Connects queue to A nodes (a_ind), returns the created queue index */
		rc = tm_create_queue_to_a_node(hndl, *a_ind_ptr, &q_params, last_queue_ptr);
		if (rc) {
			pr_info("*** (TM_SCN) *** tm_create_queue_to_a_node. a_node:%d,   rc %d\n", *a_ind_ptr, rc);
			return rc;
		}
	}
	return rc;
}

int __tm_create_default_p_c_path(tm_handle hndl, uint8_t port_index,
								enum tm_port_bw port_speed,
								uint16_t num_of_b_children,
								uint32_t *c_ind_ptr)
{
	struct tm_port_params p_params;
	struct tm_c_node_params c_params;
	int i;
	int rc = 0;

	p_params.wred_profile_ref = TM_NO_DROP_PROFILE;
	p_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		p_params.dwrr_priority[i] = TM_DISABLE;

	p_params.num_of_children = 1;

	rc = tm_create_asym_port(hndl, port_index, port_speed, &p_params);
	if (rc) {
		pr_info("*** (TM_SCN) *** tm_create_asym_port: port : %d,  rc %d.\n", port_index, rc);
		return rc;
	}

	c_params.wred_cos = 0;
	c_params.elig_prio_func_ptr = TM_ELIG_N_DEFAULT;
	for (i = 0; i < 8; i++)
		c_params.wred_profile_ref[i] = TM_NO_DROP_PROFILE;
	c_params.quantum = 0x40;
	for (i = 0; i < 8; i++)
		c_params.dwrr_priority[i] = TM_DISABLE;
	c_params.num_of_children = num_of_b_children;

	/* Connects C nodes to the port (port_index), returns the created C node index */
	rc = tm_create_c_node_to_port(hndl, port_index, &c_params, c_ind_ptr);
	if (rc)
		pr_info("*** (TM_SCN) *** tm_create_c_node_to_port: port : %d , c_node: %d , rc %d.\n",
				port_index, *c_ind_ptr, rc);

	return rc;
}

int __tm_create_default_b_a_path(tm_handle hndl, uint16_t c_node_index,
								uint16_t num_of_a_children,
								uint16_t num_of_queues,
								uint32_t *b_ind_ptr,
								uint32_t *last_a_ind_ptr,
								uint32_t *last_queue_ptr)
{
	int rc = 0;
	int i;
	rc = __tm_create_default_b_node(hndl, c_node_index, num_of_a_children, b_ind_ptr);
	if (rc)
		return rc;
	for (i = 0; i < num_of_a_children; i++) {
		rc = __tm_create_default_a_node_with_queues(hndl,
													*b_ind_ptr,
													num_of_queues,
													last_a_ind_ptr,
													last_queue_ptr);
		if (rc)
			return rc;
	}
	return rc;
}

#define	CREATE_P_C_PATH(port, port_speed, total_num_of_b_nodes)	\
do {\
	port_index = port;\
	rc = __tm_create_default_p_c_path(hndl, port_index, port_speed, total_num_of_b_nodes , &c_node_index);\
/* pr_info("TM cfg3 tree : port :  %d\n", port); */\
	if (rc)\
		goto err_out;\
} while (0)

#define	CREATE_B_NODE(a_nodes_per_b_node_num)	\
do {\
	rc =  __tm_create_default_b_node(hndl, c_node_index, a_nodes_per_b_node_num, &b_node_index);\
/* pr_info("TM cfg3 tree : b_node :  %d\n", b_node_index); */\
	if (rc)\
		goto err_out;\
} while (0)

#define	CREATE_A_PATH(queues_per_anode)	\
do {\
	rc = __tm_create_default_a_node_with_queues(hndl,\
			b_node_index, queues_per_anode, &a_node_index, &queue_index);\
	if (rc)\
		goto err_out;\
}  while (0)

#define	CREATE_A_PATH_SET(a_nodes_num, queues_per_anode)	\
do {\
	for (i = 0; i < a_nodes_num ; i++) {\
		rc = __tm_create_default_a_node_with_queues(hndl,\
				b_node_index, queues_per_anode, &a_node_index, &queue_index);\
		if (rc)\
			goto err_out;\
	} \
/*pr_info("                             last used Queue index  :  %d\n", queue_index);*/\
} while (0)

#define	CREATE_B_A_PATH(a_nodes_per_b_node_num, queues_per_anode)	\
do {\
		rc = __tm_create_default_b_a_path(hndl, c_node_index,\
				a_nodes_per_b_node_num, queues_per_anode, &b_node_index, &a_node_index, &queue_index);\
		if (rc)\
			goto err_out;\
/*pr_info("                             last used A-node index :  %d\n", a_node_index);*/\
/*pr_info("                             last used Queue index  :  %d\n", queue_index);*/\
} while (0)

#define	CREATE_B_A_PATH_SET(b_nodes_num, a_nodes_per_b_node_num, queues_per_anode)	\
do {\
	for (i = 0; i < b_nodes_num ; i++) {\
		rc = __tm_create_default_b_a_path(hndl, c_node_index,\
				a_nodes_per_b_node_num, queues_per_anode, &b_node_index, &a_node_index, &queue_index);\
		if (rc)\
			goto err_out;\
	} \
/*pr_info("                             last used A-node index :  %d\n", a_node_index);*/\
/*pr_info("                             last used Queue index  :  %d\n", queue_index);*/\
} while (0)

#define PRINT_CREATE_TREE_INFO(port) \
/*
do {\
	pr_info(" PORT :  %s (%d)\n", #port, port);\
	pr_info("                         last used C-node index :  %d\n", c_node_index);\
	pr_info("                         last used B-node index :  %d\n", b_node_index);\
	pr_info("                         last used A-node index :  %d\n", a_node_index);\
	pr_info("                         last used Queue  index :  %d\n", queue_index);\
} while (0)
*/

int tm_cfg3_tree(void)
{
	struct tm_ctl *hndl;

	uint8_t port_index;
	int rc;
	uint32_t	queue_index = 0;
	uint32_t	a_node_index = 0;
	uint32_t	b_node_index = 0;
	uint32_t	c_node_index = 0;
	int		i;
	enum tm_level lvl;

	if (!qmtm_hndl) {
		rc = tm_open();
		if (rc)
			goto err_out;
	}

	if (mv_active_config != TM_INVALID_CONFIG) {
		pr_warn("Warning: TM already configured to %s, cannot reconfig TM\n", tm_config_str(mv_active_config));
		return 0;
	}

	pr_info("*** TM cfg3 Scenario ***\n");
	mv_active_config = TM_CFG3_CONFIG;
	hndl = qmtm_hndl->tmctl;

	/* Drop configuration - Drop profiles: out_BW = 2.5Gbps, Color blind TD */
	rc = tm_drop_profiles_config(hndl);
	if (rc) {
		rc = -3;
		goto err_out;
	}

	for (lvl = Q_LEVEL; lvl <= P_LEVEL; lvl++) {
		rc = tm_configure_fixed_periodic_scheme_2_5G(hndl, lvl);
		if (rc) {
			rc = -3;
			goto err_out;
		}
	}
	/**********************************************************************************/
	/* port TM_A0_PORT_PPC0_0 (0) : 1, 3*4*4+1*(1*8+1*2 +1*2+1*4)+1*(4*8+4*0+8*4)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_PPC0_0, TM_10G_PORT, 8 /*  total b_nodes per c_node */);
		/* create three 4*4 branches */
		CREATE_B_A_PATH_SET(4, 1/*A nodes per B*/, 4/*queues per A*/);
		/* follows belowing assymmetric structure creation */
		CREATE_B_A_PATH(8/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(12/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_PPC0_0);
/**/
#if 0
	rc = tm_update_port_drop(hndl, TM_A0_PORT_PPC0_0, 1 /*drop profile for 2_5G */);
	if (rc)
		goto err_out;
#endif

	/**********************************************************************************/
	/* port TM_A0_PORT_PPC0_1 (1) :  1*4*4+1*(1*8+1*2 +1*2+1*4)+1*(4*8+4*0+8*4)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_PPC0_1, TM_10G_PORT, 1 /*  total b_nodes per c_node */);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_PPC0_1);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_PPC1_MNT0 (2) : 1 , 1*4*4           ( c, b a q nodes mapping)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_PPC1_MNT0, TM_10G_PORT, 1 /* total b_nodes per c_node */);
		/* create two 4*4 branches */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);

/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_PPC1_MNT0);
/**/
	/**********************************************************************************/
	/* port TM_A0_PORT_PPC1_MNT1-3 : 1 , 2*4*4           ( c, b a q nodes mapping)    */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_PPC1_MNT1, TM_10G_PORT, 2 /* total b_nodes per c_node */);
		/* create two 4*4 branches */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);

/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_PPC1_MNT1);
/**/
	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC0-4 : 1 , 1*1*12+ 1*(4*4+1*2+2*1) ( c, b a q nodes mapping)*/
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC0, TM_10G_PORT, 2 /* total b_nodes per c_node */);
		/* create two 4*4 branches */
		CREATE_B_A_PATH(1/*A nodes per B*/, 12/*queues per A*/);
		CREATE_B_NODE(7/*A nodes per B*/);
			CREATE_A_PATH(4/*queues per A*/);
			CREATE_A_PATH(4/*queues per A*/);
			CREATE_A_PATH(4/*queues per A*/);
			CREATE_A_PATH(4/*queues per A*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
	/* 2.5G shaping on port EMAC0
	rc = tm_set_min_shaping(hndl, P_LEVEL, TM_A0_PORT_EMAC0, 2500);
	*/
	/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC0);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC1-5 : 1 , 1*(1*12+1*2+2*1)       ( c, b a q nodes mapping)  */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC1, TM_10G_PORT, 1/* total b_nodes per c_node */);
		CREATE_B_NODE(4/*A nodes per B*/);
			CREATE_A_PATH(12/*queues per A*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC1);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC2-6 : 1 , 1*(1*12+1*2+2*1)       ( c, b a q nodes mapping)  */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC2, TM_10G_PORT, 1/* total b_nodes per c_node */);
		CREATE_B_NODE(4/*A nodes per B*/);
			CREATE_A_PATH(12/*queues per A*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC2);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC3-7 : 1 ,1*(1*12+1*2+2*1)       ( c, b a q nodes mapping)  */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC3, TM_10G_PORT, 1/* total b_nodes per c_node */);
		CREATE_B_NODE(4/*A nodes per B*/);
			CREATE_A_PATH(12/*queues per A*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC3);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC4-8 : 1 , 1*4*4           ( c, b a q nodes mapping)        */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC4, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC4);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_EMAC_LPB-9 : 1 , 1*4*4           ( c, b a q nodes mapping)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_EMAC_LPB, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_EMAC_LPB);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_CMAC_IN-10 : 1 , 4*4*4           ( c, b a q nodes mapping)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_CMAC_IN, TM_10G_PORT, 4/* total b_nodes per c_node */);
		/* create  four  4*4 branches */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_CMAC_IN);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_CMAC_LA-11 : 1 , 1*4*4           ( c, b a q nodes mapping)     */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_CMAC_LA, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);

/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_CMAC_LA);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_HMAC-12 : 1 , 2*1*16 + 2*2*8 + 1*(2*2 + 12*1) ( c, b a q nodes mapping)        */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_HMAC, TM_10G_PORT, 5/* total b_nodes per c_node */);
		/* create  five  4*4 branches */
		CREATE_B_A_PATH(1/*A nodes per B*/, 16/*queues per A*/);
		CREATE_B_A_PATH(1/*A nodes per B*/, 16/*queues per A*/);
		CREATE_B_A_PATH(2/*A nodes per B*/, 8/*queues per A*/);
		CREATE_B_A_PATH(2/*A nodes per B*/, 8/*queues per A*/);
		CREATE_B_NODE(14/*total A nodes per B*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(2/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
			CREATE_A_PATH(1/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_HMAC);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_UNUSED0-13 : 1 ,1*4*4           ( c b a  nodes mapping)        */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_UNUSED0, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);

/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_UNUSED0);
/**/

	/**********************************************************************************/
	/* port TM_A0_PORT_DROP0-14 : 1 , 1*4*4           ( c, b a q nodes mapping)       */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_DROP0, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);

/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_DROP0);
/**/
	/**********************************************************************************/
	/* port TM_A0_PORT_DROP1-15 : 1 , 1*4*4           ( c, b a q nodes mapping)       */
	/**********************************************************************************/
	CREATE_P_C_PATH(TM_A0_PORT_DROP1, TM_10G_PORT, 1/* total b_nodes per c_node */);
		/* create 4*4 branch */
		CREATE_B_A_PATH(4/*A nodes per B*/, 4/*queues per A*/);
/**/
PRINT_CREATE_TREE_INFO(TM_A0_PORT_DROP1);
/**/

/*
rc = tm_dump_port_hw(hndl, TM_A0_PORT_PPC0_0);
rc = tm_dump_port_hw(hndl, TM_A0_PORT_HMAC);
*/

	/* here all C-node, B-node & Queue resources are already used */
	/*
	pr_info("TM cfg3 tree : valid tree created:\n");
	pr_info("                         last used C-node index :  %d\n", c_node_index);
	pr_info("                         last used B-node index :  %d\n", b_node_index);
	pr_info("                         last used A-node index :  %d\n", a_node_index);
	pr_info("                         last used Queue  index :  %d\n", queue_index);
	*/

err_out:
	if (rc == 0)
		pr_info("TM cfg3 tree Completed Successfuly\n");
	else
		pr_info("TM cfg3 Completed with Error %d\n", rc);
	return rc;
}
EXPORT_SYMBOL(tm_cfg3_tree);


