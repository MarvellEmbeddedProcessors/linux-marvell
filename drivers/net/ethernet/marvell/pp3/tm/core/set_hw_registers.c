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

#include <linux/delay.h>
#include "set_hw_registers.h"
#include "tm_alias.h"
#include "tm_rw_registers_interface.h"
#include "tm_hw_configuration_interface.h"
#include "rm_internal_types.h"
#include "rm_status.h"


/*#define SHIFT_TABLE */
#define STRUCTS

#define READ_ONLY   0

#if defined(STRUCTS)
	#include "tm_payloads.h"
#elif defined(SHIFT_TABLE)
	#include "tm_registers_description.h"
#else

#endif

#include "tm_registers_processing.h"
#include "tm_errcodes.h"


#define COMPLETE_HW_WRITE								\
	do {												\
		if (0 == rc)									\
			rc = flush_hw_connection(TM_ENV(ctl));		\
		if (rc)											\
			rc = reset_hw_connection(TM_ENV(ctl), rc);	\
	} while (0);

#define TM_WRITE_REGISTER(address, register_name)                   \
	do {                                                            \
		if (sizeof(struct register_name) != 8)                      \
			pr_err("WR size is %d\n", sizeof(struct register_name));\
		rc = tm_register_write(TM_ENV(ctl), (void *)&(address),     \
			TM_REGISTER_VAR_ADDR(register_name));                   \
	} while (0);

#define TM_READ_REGISTER(address, register_name)                    \
	do {                                                            \
		if (sizeof(struct register_name) != 8)                      \
			pr_err("WR size is %d\n", sizeof(struct register_name));\
		rc = tm_register_read(TM_ENV(ctl), (void *)&(address),      \
			TM_REGISTER_VAR_ADDR(register_name));                   \
	} while (0);

#define TM_WRITE_TABLE_REGISTER(address, index, register_name)      \
	do {                                                            \
		if (sizeof(struct register_name) != 8)                      \
			pr_err("WR size is %d\n", sizeof(struct register_name));\
		rc = tm_table_entry_write(TM_ENV(ctl), (void *)&(address),  \
			index, TM_REGISTER_VAR_ADDR(register_name));            \
	} while (0);

#define TM_READ_TABLE_REGISTER(address, index, register_name)       \
	do {                                                            \
		if (sizeof(struct register_name) != 8)                      \
			pr_err("WR size is %d\n", sizeof(struct register_name));\
		rc = tm_table_entry_read(TM_ENV(ctl), (void *)&(address),   \
		index, TM_REGISTER_VAR_ADDR(register_name));                \
	} while (0);


int set_hw_fixed_port_periodic_scheme(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortPerConf)
	TM_REGISTER_VAR(TM_Sched_PortPerRateShpPrms)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_PortPerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_PortPerConf, PerInterval, TM_FIXED_2_5_G_PORT_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_PortPerConf, PerEn, TM_FIXED_PERIODIC_SCHEME_PER_EN);
	TM_WRITE_REGISTER(TM.Sched.PortPerConf, TM_Sched_PortPerConf);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, L, TM_FIXED_PERIODIC_SCHEME_L);
	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, K, TM_FIXED_PERIODIC_SCHEME_K);
	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, N, TM_FIXED_PERIODIC_SCHEME_N);
	TM_WRITE_REGISTER(TM.Sched.PortPerRateShpPrms, TM_Sched_PortPerRateShpPrms);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_fixed_c_level_periodic_scheme(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_ClvlPerConf)
	TM_REGISTER_VAR(TM_Sched_ClvlPerRateShpPrms)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_ClvlPerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_ClvlPerConf, PerInterval, TM_FIXED_2_5_G_C_LEVEL_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_ClvlPerConf, PerEn, TM_FIXED_PERIODIC_SCHEME_PER_EN);
	TM_WRITE_REGISTER(TM.Sched.ClvlPerConf, TM_Sched_ClvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, L, TM_FIXED_PERIODIC_SCHEME_L);
	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, K, TM_FIXED_PERIODIC_SCHEME_K);
	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, N, TM_FIXED_PERIODIC_SCHEME_N);
	TM_WRITE_REGISTER(TM.Sched.ClvlPerRateShpPrms, TM_Sched_ClvlPerRateShpPrms);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_fixed_b_level_periodic_scheme(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_BlvlPerConf)
	TM_REGISTER_VAR(TM_Sched_BlvlPerRateShpPrms)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_BlvlPerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_BlvlPerConf, PerInterval, TM_FIXED_2_5_G_B_LEVEL_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_BlvlPerConf, PerEn, TM_FIXED_PERIODIC_SCHEME_PER_EN);
	TM_WRITE_REGISTER(TM.Sched.BlvlPerConf, TM_Sched_BlvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, L, TM_FIXED_PERIODIC_SCHEME_L);
	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, K, TM_FIXED_PERIODIC_SCHEME_K);
	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, N, TM_FIXED_PERIODIC_SCHEME_N);
	TM_WRITE_REGISTER(TM.Sched.BlvlPerRateShpPrms, TM_Sched_BlvlPerRateShpPrms);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_fixed_a_level_periodic_scheme(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_AlvlPerConf)
	TM_REGISTER_VAR(TM_Sched_AlvlPerRateShpPrms)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_AlvlPerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_AlvlPerConf, PerInterval, TM_FIXED_2_5_G_A_LEVEL_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_AlvlPerConf, PerEn, TM_FIXED_PERIODIC_SCHEME_PER_EN);
	TM_WRITE_REGISTER(TM.Sched.AlvlPerConf, TM_Sched_AlvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, L, TM_FIXED_PERIODIC_SCHEME_L);
	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, K, TM_FIXED_PERIODIC_SCHEME_K);
	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, N, TM_FIXED_PERIODIC_SCHEME_N);
	TM_WRITE_REGISTER(TM.Sched.AlvlPerRateShpPrms, TM_Sched_AlvlPerRateShpPrms);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_fixed_queue_periodic_scheme(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_QueuePerConf)
	TM_REGISTER_VAR(TM_Sched_QueuePerRateShpPrms)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_QueuePerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_QueuePerConf, PerInterval, TM_FIXED_2_5_G_QUEUE_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_QueuePerConf, PerEn, TM_FIXED_PERIODIC_SCHEME_PER_EN);
	TM_WRITE_REGISTER(TM.Sched.QueuePerConf, TM_Sched_QueuePerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, L, TM_FIXED_PERIODIC_SCHEME_L);
	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, K, TM_FIXED_PERIODIC_SCHEME_K);
	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, N, TM_FIXED_PERIODIC_SCHEME_N);
	TM_WRITE_REGISTER(TM.Sched.QueuePerRateShpPrms, TM_Sched_QueuePerRateShpPrms);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_fixed_port_shaping_status(tm_handle hndl, uint8_t shaping_status)
{
	int rc = -ERANGE;

	TM_REGISTER_VAR(TM_Sched_PortPerConf)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	TM_REGISTER_SET(TM_Sched_PortPerConf, DecEn, TM_FIXED_PERIODIC_SCHEME_DEC_EN);
	TM_REGISTER_SET(TM_Sched_PortPerConf, PerInterval, TM_FIXED_2_5_G_PORT_PERIODIC_SCHEME_PER_INTERVAL);
	TM_REGISTER_SET(TM_Sched_PortPerConf, PerEn, shaping_status);

	TM_WRITE_REGISTER(TM.Sched.PortPerConf, TM_Sched_PortPerConf);
	COMPLETE_HW_WRITE
	return rc;
}


#ifdef MV_QMTM_NSS_A0
/**
 */
int set_hw_dwrr_limit(tm_handle hndl)
{
	int rc = 0;
	TM_REGISTER_VAR(TM_Sched_PortDWRRBytesPerBurstsLimit)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_PortDWRRBytesPerBurstsLimit, limit, ctl->dwrr_bytes_burst_limit);
	TM_WRITE_REGISTER(TM.Sched.PortDWRRBytesPerBurstsLimit, TM_Sched_PortDWRRBytesPerBurstsLimit);
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_gen_conf(tm_handle hndl)
{
	int rc = 0;
	TM_REGISTER_VAR(TM_Sched_PortExtBPEn)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_PortExtBPEn, En, ctl->port_ext_bp_en);
	TM_WRITE_REGISTER(TM.Sched.PortExtBPEn, TM_Sched_PortExtBPEn);
	COMPLETE_HW_WRITE
	return rc;
}
#endif

/**
 */
int set_hw_max_dp_mode(tm_handle hndl)
{
	int rc = 0;
	int i;
	uint8_t port = 0;
	uint8_t c_lvl = 0;
	uint8_t b_lvl = 0;
	uint8_t a_lvl = 0;
	uint8_t queue = 0;

	TM_REGISTER_VAR(TM_Drop_WREDMaxProbModePerColor)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < 3; i++) {
		port  = port  | (ctl->dp_unit.local[P_LEVEL].max_p_mode[i] << (i*2));
		c_lvl = c_lvl | (ctl->dp_unit.local[C_LEVEL].max_p_mode[i] << (i*2));
		b_lvl = b_lvl | (ctl->dp_unit.local[B_LEVEL].max_p_mode[i] << (i*2));
		a_lvl = a_lvl | (ctl->dp_unit.local[A_LEVEL].max_p_mode[i] << (i*2));
		queue = queue | (ctl->dp_unit.local[Q_LEVEL].max_p_mode[i] << (i*2));
	}

	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Port, port)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Clvl, c_lvl)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Blvl, b_lvl)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Alvl, a_lvl)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Queue, queue)

	TM_WRITE_REGISTER(TM.Drop.WREDMaxProbModePerColor, TM_Drop_WREDMaxProbModePerColor)
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int __set_hw_queues_wred_curve(tm_handle hndl, uint8_t *prob_array, uint8_t curve_ind)
{
	int rc = -ERANGE;
	int i;
	int j;
	int ind;
	TM_REGISTER_VAR(TM_Drop_QueueREDCurve_Color)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		TM_REGISTER_SET(TM_Drop_QueueREDCurve_Color, Prob, prob_array[i])
		ind = curve_ind*32 + i;
		for (j = 0; j < 3; j++) {
			/* the same curve for each color */
			rc = tm_table_entry_write(TM_ENV(ctl), (void *)&(TM.Drop.QueueREDCurve.Color[j]), ind, TM_REGISTER_VAR_ADDR(TM_Drop_QueueREDCurve_Color));
			if (rc)
				goto out;
		}
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_queues_wred_curve(tm_handle hndl, uint8_t curve_ind)
{
	int rc = -ERANGE;
	uint8_t *prob_array;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (curve_ind < TM_NUM_WRED_QUEUE_CURVES) {
		prob_array = (uint8_t *)(ctl->tm_wred_q_lvl_curves[curve_ind].prob);
		return __set_hw_queues_wred_curve(hndl, prob_array, curve_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_queues_default_wred_curve(tm_handle hndl, uint8_t *prob_array)
{
	return __set_hw_queues_wred_curve(hndl, prob_array, 0);
}

/*****************************/


int __set_hw_a_nodes_wred_curve(tm_handle hndl, uint8_t *prob_array, uint8_t curve_ind)
{
	int rc = -ERANGE;
	int i;
	int j;
	int ind;
	TM_REGISTER_VAR(TM_Drop_AlvlREDCurve_Color)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
	{
		TM_REGISTER_SET(TM_Drop_AlvlREDCurve_Color, Prob, prob_array[i])
		ind = curve_ind*32 + i;
		for (j = 0; j < 3; j++)
		{
			rc = tm_table_entry_write(TM_ENV(ctl), (void *)&(TM.Drop.AlvlREDCurve.Color[j]), ind, TM_REGISTER_VAR_ADDR(TM_Drop_AlvlREDCurve_Color));
			if (rc)
				goto out;
		}
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_a_nodes_wred_curve(tm_handle hndl, uint8_t curve_ind)
{
	int rc = -ERANGE;
	uint8_t *prob_array;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (curve_ind < TM_NUM_WRED_A_NODE_CURVES)
	{
		prob_array = (uint8_t *)(ctl->tm_wred_a_lvl_curves[curve_ind].prob);
		return __set_hw_a_nodes_wred_curve(hndl,prob_array,curve_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_a_nodes_default_wred_curve(tm_handle hndl, uint8_t *prob_array)
{
	return __set_hw_a_nodes_wred_curve(hndl, prob_array, 0);
}

/*****************************/


int __set_hw_b_nodes_wred_curve(tm_handle hndl, uint8_t *prob_array, uint8_t curve_ind)
{
	int rc = -ERANGE;
	int i;
	TM_REGISTER_VAR(TM_Drop_BlvlREDCurve_Table)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		TM_REGISTER_SET(TM_Drop_BlvlREDCurve_Table, Prob, prob_array[i]);
		rc = tm_table_entry_write(TM_ENV(ctl), (void *)&TM.Drop.BlvlREDCurve[curve_ind].Table, i, TM_REGISTER_VAR_ADDR(TM_Drop_BlvlREDCurve_Table));
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_b_nodes_wred_curve(tm_handle hndl, uint8_t curve_ind)
{
	int rc = -ERANGE;
	uint8_t *prob_array;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (curve_ind < TM_NUM_WRED_B_NODE_CURVES)
	{
		prob_array = (uint8_t *)(ctl->tm_wred_b_lvl_curves[curve_ind].prob);
		return __set_hw_b_nodes_wred_curve(hndl, prob_array,curve_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_b_nodes_default_wred_curve(tm_handle hndl, uint8_t *prob_array)
{
	return __set_hw_b_nodes_wred_curve(hndl, prob_array, 0);
}


/**
 */
int __set_hw_c_nodes_wred_curve(tm_handle hndl, uint8_t *prob_array, uint8_t cos, uint8_t curve_ind)
{
	int rc = -EFAULT;
	int i;
	int ind;
	TM_REGISTER_VAR(TM_Drop_ClvlREDCurve_CoS)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		TM_REGISTER_SET(TM_Drop_ClvlREDCurve_CoS, Prob, prob_array[i]);
		ind = curve_ind*32 + i;
		rc = tm_table_entry_write(TM_ENV(ctl), (void *)&(TM.Drop.ClvlREDCurve.CoS[cos]), ind, TM_REGISTER_VAR_ADDR(TM_Drop_ClvlREDCurve_CoS));
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_c_nodes_wred_curve(tm_handle hndl, uint8_t cos, uint8_t curve_ind)
{
	uint8_t *prob_array;
	int rc = -EFAULT;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (curve_ind < TM_NUM_WRED_C_NODE_CURVES)
	{
		prob_array = (uint8_t *)(ctl->tm_wred_c_lvl_curves[cos][curve_ind].prob);
		return __set_hw_c_nodes_wred_curve(hndl, prob_array, cos, curve_ind);
	}

	COMPLETE_HW_WRITE
	return rc;
}
int set_hw_c_nodes_default_wred_curve(tm_handle hndl, uint8_t cos, uint8_t *prob_array)
{
	return __set_hw_c_nodes_wred_curve(hndl, prob_array, cos, 0);
}

/**
 */
int __set_hw_ports_wred_curve(tm_handle hndl, uint8_t *prob_array)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Drop_PortREDCurve)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		TM_REGISTER_SET(TM_Drop_PortREDCurve, Prob, prob_array[i]);
		TM_WRITE_TABLE_REGISTER(TM.Drop.PortREDCurve, i, TM_Drop_PortREDCurve)
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_wred_curve(tm_handle hndl, uint8_t curve_ind)
{
	int rc = -EFAULT;
	uint8_t *prob_array;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (curve_ind < TM_NUM_WRED_PORT_CURVES)
	{
		prob_array = (uint8_t *)(ctl->tm_wred_ports_curves[curve_ind].prob);
		return __set_hw_ports_wred_curve(hndl,prob_array);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_default_wred_curve(tm_handle hndl, uint8_t *prob_array)
{
	return	__set_hw_ports_wred_curve(hndl,prob_array);
}


int __set_hw_ports_wred_curve_cos(tm_handle hndl, uint8_t *prob_array, uint8_t cos)
{
	int rc = 0;
	int i;
	TM_REGISTER_VAR(TM_Drop_PortREDCurve)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++) {
		TM_REGISTER_SET(TM_Drop_PortREDCurve, Prob, prob_array[i]);
		TM_WRITE_TABLE_REGISTER(TM.Drop.PortREDCurve_CoS[cos], i, TM_Drop_PortREDCurve)
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_wred_curve_cos(tm_handle hndl, uint8_t cos, uint8_t curve_ind)
{
	int rc = -EFAULT;
	uint8_t *prob_array;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (curve_ind < TM_NUM_WRED_PORT_CURVES)
	{
		prob_array = (uint8_t *)(ctl->tm_wred_ports_curves_cos[cos][curve_ind].prob);
		return __set_hw_ports_wred_curve_cos( hndl, prob_array, cos);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_default_wred_curve_cos(tm_handle hndl, uint8_t cos, uint8_t *prob_array)
{
	return __set_hw_ports_wred_curve_cos( hndl, prob_array, cos);
}


/**
 */
int __set_hw_queue_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint32_t prof_ind)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDDPRatio)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDParams, CurveIndexColor2, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDParams, prof_ind, TM_Drop_QueueDropPrfWREDParams);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDScaleRatio, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDScaleRatio, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDScaleRatio, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDScaleRatio, prof_ind, TM_Drop_QueueDropPrfWREDScaleRatio);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDMinThresh, MinTHColor0, profile->min_threshold[0].thresh);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDMinThresh, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDMinThresh, MinTHColor2, profile->min_threshold[2].thresh);
	TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDMinThresh, prof_ind, TM_Drop_QueueDropPrfWREDMinThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_QueueDropPrfTailDrpThresh, TailDropThreshRes, profile->td_thresh_res);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfTailDrpThresh, TailDropThresh, profile->td_threshold);
	TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfTailDrpThresh, prof_ind, TM_Drop_QueueDropPrfTailDrpThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDDPRatio, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDDPRatio, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_QueueDropPrfWREDDPRatio, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDDPRatio, prof_ind, TM_Drop_QueueDropPrfWREDDPRatio);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_queue_drop_profile(tm_handle hndl, uint32_t prof_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_QUEUE_DROP_PROF)
	{
		profile = &(ctl->tm_q_lvl_drop_profiles[prof_ind]);
		return __set_hw_queue_drop_profile(hndl, profile, prof_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_queue_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile)
{
	return __set_hw_queue_drop_profile(hndl, profile, 0);
}

/**
 */
int __set_hw_a_nodes_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint32_t prof_ind)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDDPRatio)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDParams, CurveIndexColor2, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDParams, prof_ind, TM_Drop_AlvlDropPrfWREDParams);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDScaleRatio, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDScaleRatio, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDScaleRatio, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDScaleRatio, prof_ind, TM_Drop_AlvlDropPrfWREDScaleRatio);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDMinThresh, MinTHColor2, profile->min_threshold[2].thresh);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDMinThresh, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDMinThresh, MinTHColor0, profile->min_threshold[0].thresh);
	TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDMinThresh, prof_ind, TM_Drop_AlvlDropPrfWREDMinThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_AlvlDropPrfTailDrpThresh, TailDropThreshRes, profile->td_thresh_res);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfTailDrpThresh, TailDropThresh, profile->td_threshold);
	TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfTailDrpThresh, prof_ind, TM_Drop_AlvlDropPrfTailDrpThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDDPRatio, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDDPRatio, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_AlvlDropPrfWREDDPRatio, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDDPRatio, prof_ind, TM_Drop_AlvlDropPrfWREDDPRatio);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_a_nodes_drop_profile(tm_handle hndl, uint32_t prof_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;


	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_A_NODE_DROP_PROF)
	{
		profile = &(ctl->tm_a_lvl_drop_profiles[prof_ind]);
		return __set_hw_a_nodes_drop_profile(hndl,profile,prof_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}
int set_hw_a_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile)
{
	return __set_hw_a_nodes_drop_profile(hndl, profile, 0);
}


/**
 */
int __set_hw_b_nodes_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint32_t prof_ind)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDDPRatio)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDParams, CurveIndexColor2, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDParams, prof_ind, TM_Drop_BlvlDropPrfWREDParams);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDScaleRatio, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDScaleRatio, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDScaleRatio, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDScaleRatio, prof_ind, TM_Drop_BlvlDropPrfWREDScaleRatio);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDMinThresh, MinTHColor2, profile->min_threshold[2].thresh);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDMinThresh, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDMinThresh, MinTHColor0, profile->min_threshold[0].thresh);
	TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDMinThresh, prof_ind, TM_Drop_BlvlDropPrfWREDMinThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_BlvlDropPrfTailDrpThresh, TailDropThreshRes, profile->td_thresh_res);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfTailDrpThresh, TailDropThresh, profile->td_threshold);
	TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfTailDrpThresh, prof_ind, TM_Drop_BlvlDropPrfTailDrpThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDDPRatio, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDDPRatio, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_BlvlDropPrfWREDDPRatio, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDDPRatio, prof_ind, TM_Drop_BlvlDropPrfWREDDPRatio);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_b_nodes_drop_profile(tm_handle hndl, uint32_t prof_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_B_NODE_DROP_PROF)
	{
		profile = &(ctl->tm_b_lvl_drop_profiles[prof_ind]);
		return __set_hw_b_nodes_drop_profile(hndl, profile, prof_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_b_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile)
{
	return __set_hw_b_nodes_drop_profile(hndl, profile, 0);
}


/**
 */
int __set_hw_c_nodes_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint8_t cos, uint32_t prof_ind)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDParams_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDScaleRatio_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDMinThresh_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfTailDrpThresh_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDDPRatio_CoS)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDParams_CoS, CurveIndexColor2, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDParams.CoS[cos], prof_ind, TM_Drop_ClvlDropPrfWREDParams_CoS);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDScaleRatio_CoS, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDScaleRatio_CoS, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDScaleRatio_CoS, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[cos], prof_ind,
		TM_Drop_ClvlDropPrfWREDScaleRatio_CoS);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDMinThresh_CoS, MinTHColor0, profile->min_threshold[0].thresh);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDMinThresh_CoS, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDMinThresh_CoS, MinTHColor2, profile->min_threshold[2].thresh);

	TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDMinThresh.CoS[cos], prof_ind, TM_Drop_ClvlDropPrfWREDMinThresh_CoS);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_ClvlDropPrfTailDrpThresh_CoS, TailDropThreshRes, profile->td_thresh_res);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfTailDrpThresh_CoS, TailDropThresh, profile->td_threshold);
	TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfTailDrpThresh.CoS[cos], prof_ind, TM_Drop_ClvlDropPrfTailDrpThresh_CoS);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDDPRatio_CoS, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDDPRatio_CoS, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_ClvlDropPrfWREDDPRatio_CoS, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDDPRatio.CoS[cos], prof_ind,
		TM_Drop_ClvlDropPrfWREDDPRatio_CoS);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_c_nodes_drop_profile(tm_handle hndl, uint8_t cos, uint32_t prof_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_C_NODE_DROP_PROF)
	{
		profile = &(ctl->tm_c_lvl_drop_profiles[cos][prof_ind]);
		return __set_hw_c_nodes_drop_profile(hndl, profile, cos, prof_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_c_nodes_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint8_t cos)
{
	return __set_hw_c_nodes_drop_profile(hndl, profile, cos, 0);
}


/**
 */
int __set_hw_ports_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint32_t prof_ind, uint8_t port_ind)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDDPRatio)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	profile = &(ctl->tm_p_lvl_drop_profiles[prof_ind]);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor2, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDParams, port_ind, TM_Drop_PortDropPrfWREDParams);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDScaleRatio, port_ind, TM_Drop_PortDropPrfWREDScaleRatio);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor0, profile->min_threshold[0].thresh);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor2, profile->min_threshold[2].thresh);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDMinThresh, port_ind, TM_Drop_PortDropPrfWREDMinThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfTailDrpThresh, TailDropThreshRes, profile->td_thresh_res);
	TM_REGISTER_SET(TM_Drop_PortDropPrfTailDrpThresh, TailDropThresh, profile->td_threshold);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfTailDrpThresh, port_ind, TM_Drop_PortDropPrfTailDrpThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDDPRatio, port_ind, TM_Drop_PortDropPrfWREDDPRatio);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_drop_profile(tm_handle hndl, uint32_t prof_ind, uint8_t port_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_PORT_DROP_PROF)
	{
		profile = &(ctl->tm_p_lvl_drop_profiles[prof_ind]);
		return __set_hw_ports_drop_profile(hndl, profile, prof_ind,port_ind);
	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_default_drop_profile(tm_handle hndl, struct tm_drop_profile *profile, uint8_t port_ind)
{
	return __set_hw_ports_drop_profile(hndl, profile, 0, port_ind);
}


/**
 */
int __set_hw_ports_drop_profile_cos(tm_handle hndl, struct tm_drop_profile *profile, uint8_t cos, uint32_t port_ind)
{
	int rc = 0;
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDDPRatio)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, AQLExp, profile->aql_exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ColorTDEn, profile->color_td_en);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor2, profile->scale_exp[2].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor1, profile->scale_exp[1].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, ScaleExpColor0, profile->scale_exp[0].exp);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor0, profile->curve_id[0].index);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor1, profile->curve_id[1].index);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDParams, CurveIndexColor0, profile->curve_id[2].index);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDParams_CoSRes[cos], port_ind, TM_Drop_PortDropPrfWREDParams);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor0, profile->scale_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor1, profile->scale_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDScaleRatio, ScaleRatioColor2, profile->scale_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[cos], port_ind,
		TM_Drop_PortDropPrfWREDScaleRatio);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor0, profile->min_threshold[0].thresh);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor1, profile->min_threshold[1].thresh);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDMinThresh, MinTHColor2, profile->min_threshold[2].thresh);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDMinThresh_CoSRes[cos], port_ind,
		TM_Drop_PortDropPrfWREDMinThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfTailDrpThresh, TailDropThresh, profile->td_threshold);
	TM_REGISTER_SET(TM_Drop_PortDropPrfTailDrpThresh, TailDropThreshRes, profile->td_thresh_res);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfTailDrpThresh_CoSRes[cos], port_ind,
		TM_Drop_PortDropPrfTailDrpThresh);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio0, profile->dp_ratio[0].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio1, profile->dp_ratio[1].ratio);
	TM_REGISTER_SET(TM_Drop_PortDropPrfWREDDPRatio, DPRatio2, profile->dp_ratio[2].ratio);
	TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDDPRatio_CoSRes[cos], port_ind, TM_Drop_PortDropPrfWREDDPRatio);
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_drop_profile_cos(tm_handle hndl, uint8_t cos, uint32_t prof_ind, uint8_t port_ind)
{
	int rc = -EFAULT;
	struct tm_drop_profile *profile;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (prof_ind < TM_NUM_PORT_DROP_PROF)
	{
		profile = &(ctl->tm_p_lvl_drop_profiles_cos[cos][prof_ind]);
		return __set_hw_ports_drop_profile_cos(hndl, profile, cos, port_ind);

	}
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_ports_default_drop_profile_cos(tm_handle hndl, struct tm_drop_profile *profile,
	uint8_t cos, uint8_t port_ind)
{
	return __set_hw_ports_drop_profile_cos(hndl, profile, cos, port_ind);
}


/****************************************************************************************************
* common macros for shaping functions
*
****************************************************************************************************/
#define SET_NODE_SHAPING_MAC(level, condition, resolution)	\
	do {\
		int rc = -EFAULT;\
	\
		TM_REGISTER_VAR(TM_Sched_##level##TokenBucketTokenEnDiv)\
		TM_REGISTER_VAR(TM_Sched_##level##TokenBucketBurstSize)\
		DECLARE_TM_CTL_PTR(ctl, hndl)\
		CHECK_TM_CTL_PTR(ctl)\
	\
		if (condition) {\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketTokenEnDiv, MaxTokenRes, resolution);\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketTokenEnDiv, MaxToken,\
							ebw/TM_FIXED_10_M_SHAPING_TOKEN);\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketTokenEnDiv, MinTokenRes, resolution);\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketTokenEnDiv, MinToken,\
							cbw/TM_FIXED_10_M_SHAPING_TOKEN);\
			TM_WRITE_TABLE_REGISTER(TM.Sched.level##TokenBucketTokenEnDiv, index,\
					TM_Sched_##level##TokenBucketTokenEnDiv);\
			if (rc)\
				goto out;\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketBurstSize, MaxBurstSz, cbs);\
			TM_REGISTER_SET(TM_Sched_##level##TokenBucketBurstSize, MinBurstSz, ebs);\
			TM_WRITE_TABLE_REGISTER(TM.Sched.level##TokenBucketBurstSize, index,\
					TM_Sched_##level##TokenBucketBurstSize);\
			if (rc)\
				goto out;\
		} \
out:\
		COMPLETE_HW_WRITE\
		return rc;\
	} while (0)

#define SET_BURSTS(min_cbs, min_ebs) \
	do {\
		cbs = min_cbs;\
		ebs = min_ebs;\
		if (pcbs) {\
			if (*pcbs >= cbs)\
				cbs = *pcbs;\
			else {\
				*pcbs = cbs;\
				rc = 1;\
			} \
		} \
		if (pebs) {\
			if (*pebs >= ebs)\
				ebs = *pebs;\
			else {\
				*pebs = ebs;\
				rc = 1;\
			} \
		} \
	} while (0)

#define GET_NODE_SHAPING_MAC(level, condition)	\
	do {\
		int rc = -EFAULT;\
		uint32_t value;\
		TM_REGISTER_VAR(TM_Sched_##level##TokenBucketTokenEnDiv)\
		TM_REGISTER_VAR(TM_Sched_##level##TokenBucketBurstSize)\
	\
		DECLARE_TM_CTL_PTR(ctl, hndl)\
		CHECK_TM_CTL_PTR(ctl)\
	\
		if (condition) {\
			TM_READ_TABLE_REGISTER(TM.Sched.level##TokenBucketTokenEnDiv, index,\
					TM_Sched_##level##TokenBucketTokenEnDiv);\
			if (rc)\
				return rc;\
			TM_REGISTER_GET(TM_Sched_##level##TokenBucketTokenEnDiv, MaxToken, value, (uint32_t));\
			if (pebw)\
				*pebw = value * TM_FIXED_10_M_SHAPING_TOKEN;\
			TM_REGISTER_GET(TM_Sched_##level##TokenBucketTokenEnDiv, MinToken, value, (uint32_t));\
			if (pcbw)\
				*pcbw = value * TM_FIXED_10_M_SHAPING_TOKEN;\
			TM_READ_TABLE_REGISTER(TM.Sched.level##TokenBucketBurstSize, index,\
					TM_Sched_##level##TokenBucketBurstSize);\
			if (rc)\
				return rc;\
			if (pebs)\
				TM_REGISTER_GET(TM_Sched_##level##TokenBucketBurstSize, MaxBurstSz, *pebs, (uint32_t));\
			if (pcbs)\
				TM_REGISTER_GET(TM_Sched_##level##TokenBucketBurstSize, MinBurstSz, *pcbs, (uint32_t));\
		} \
		return rc;\
	} while (0)

int __set_hw_queue_shaping(tm_handle hndl, uint32_t index, uint32_t cbw, uint32_t ebw, uint32_t cbs, uint32_t ebs)
{
	SET_NODE_SHAPING_MAC(Queue, index < get_tm_queues_count(), TM_FIXED_10_M_QUEUE_SHAPING_TOKEN_RES);
}

int set_hw_queue_shaping_ex(tm_handle hndl, uint32_t node_ind,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs)
{
	int cbs, ebs;
	int rc = 0;
/*
	cbs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_QUEUE_SHAPING_TOKEN_RES) /1024;
	ebs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_QUEUE_SHAPING_TOKEN_RES) /1024;
*/
	SET_BURSTS(TM_FIXED_2_5_G_QUEUE_SHAPING_BURST_SIZE, TM_FIXED_2_5_G_QUEUE_SHAPING_BURST_SIZE);

	if (rc == 0)
		return __set_hw_queue_shaping(hndl, node_ind, cbw, ebw, cbs, ebs);
	else
		return TM_CONF_MIN_TOKEN_TOO_LARGE;
}
int set_hw_queue_shaping_def(tm_handle hndl, uint32_t node_ind)
{
	return set_hw_queue_shaping_ex(hndl, node_ind, 10000, 10000, NULL, NULL);
}

int __set_hw_a_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t cbs, uint32_t ebs)
{
	SET_NODE_SHAPING_MAC(Alvl, index < get_tm_a_nodes_count(), TM_FIXED_10_M_A_LEVEL_SHAPING_TOKEN_RES);
}

int set_hw_a_node_shaping_ex(tm_handle hndl, uint32_t node_ind,
							uint32_t cbw, uint32_t ebw, uint32_t  *pcbs, uint32_t *pebs)
{
	int cbs, ebs;
	int rc = 0;
/*
	cbs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_A_LEVEL_SHAPING_TOKEN_RES) /1024;
	ebs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_A_LEVEL_SHAPING_TOKEN_RES) /1024;
*/
	SET_BURSTS(TM_FIXED_2_5_G_A_LEVEL_SHAPING_BURST_SIZE, TM_FIXED_2_5_G_A_LEVEL_SHAPING_BURST_SIZE);

	if (rc == 0)
		return __set_hw_a_node_shaping(hndl, node_ind, cbw, ebw, cbs, ebs);
	else
		return TM_CONF_MIN_TOKEN_TOO_LARGE;
}

int set_hw_a_node_shaping_def(tm_handle hndl, uint32_t node_ind)
{
	return set_hw_a_node_shaping_ex(hndl, node_ind, 10000, 10000, 0, 0);
}

int __set_hw_b_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t cbs, uint32_t ebs)
{
	SET_NODE_SHAPING_MAC(Blvl, index < get_tm_b_nodes_count(), TM_FIXED_10_M_B_LEVEL_SHAPING_TOKEN_RES);
}

int set_hw_b_node_shaping_ex(tm_handle hndl, uint32_t node_ind,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs)
{
	int cbs, ebs;
	int rc = 0;
/*
	cbs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_B_LEVEL_SHAPING_TOKEN_RES) /1024;
	ebs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_B_LEVEL_SHAPING_TOKEN_RES) /1024;
*/
	SET_BURSTS(TM_FIXED_2_5_G_B_LEVEL_SHAPING_BURST_SIZE, TM_FIXED_2_5_G_B_LEVEL_SHAPING_BURST_SIZE);

	if (rc == 0)
		return __set_hw_b_node_shaping(hndl, node_ind, cbw, ebw, cbs, ebs);
	else
		return TM_CONF_MIN_TOKEN_TOO_LARGE;
}

int set_hw_b_node_shaping_def(tm_handle hndl, uint32_t node_ind)
{
	return set_hw_b_node_shaping_ex(hndl, node_ind, 10000, 10000, NULL, NULL);
}


int __set_hw_c_node_shaping(tm_handle hndl, uint32_t index,
							uint32_t cbw, uint32_t ebw, uint32_t cbs, uint32_t ebs)
{
	SET_NODE_SHAPING_MAC(Clvl, index < get_tm_c_nodes_count(), TM_FIXED_10_M_C_LEVEL_SHAPING_TOKEN_RES);
}

int set_hw_c_node_shaping_ex(tm_handle hndl, uint32_t node_ind,
							uint32_t cbw, uint32_t ebw, uint32_t *pcbs, uint32_t *pebs)
{
	int cbs, ebs;
	int rc = 0;
/*
	cbs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_C_LEVEL_SHAPING_TOKEN_RES) /1024;
	ebs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) * (1 << TM_FIXED_10_M_C_LEVEL_SHAPING_TOKEN_RES) /1024;
*/
	SET_BURSTS(TM_FIXED_2_5_G_C_LEVEL_SHAPING_BURST_SIZE, TM_FIXED_2_5_G_C_LEVEL_SHAPING_BURST_SIZE);

	if (rc == 0)
		return __set_hw_c_node_shaping(hndl, node_ind, cbw, ebw, cbs, ebs);
	else
		return TM_CONF_MIN_TOKEN_TOO_LARGE;
}

int set_hw_c_node_shaping_def(tm_handle hndl, uint32_t node_ind)
{
	return set_hw_c_node_shaping_ex(hndl, node_ind, 10000, 10000, NULL, NULL);
}


int get_hw_queue_shaping(tm_handle hndl, uint32_t index,
						uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs)
{
	GET_NODE_SHAPING_MAC(Queue, index < get_tm_queues_count());
}
int get_hw_a_node_shaping(tm_handle hndl, uint32_t index,
						uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs)
{
	GET_NODE_SHAPING_MAC(Alvl, index < get_tm_a_nodes_count());
}
int get_hw_b_node_shaping(tm_handle hndl, uint32_t index, uint32_t *pcbw,
						uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs)
{
	GET_NODE_SHAPING_MAC(Blvl, index < get_tm_b_nodes_count());
}
int get_hw_c_node_shaping(tm_handle hndl, uint32_t index,
						uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs)
{
	GET_NODE_SHAPING_MAC(Clvl, index < get_tm_c_nodes_count());
}

/**
 */
int set_hw_drop_aqm_mode(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_WREDDropProbMode)
	TM_REGISTER_VAR(TM_Drop_DPSource)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Port, 1);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Clvl, 1);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Blvl, 1);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Alvl, 1);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Queue, 1);

	if ((ctl->dp_unit.local[Q_LEVEL].color_num == TM_1_COLORS) ||
		(ctl->dp_unit.local[Q_LEVEL].color_num == TM_2_COLORS))
	{
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Queue, 0);
	}

	if ((ctl->dp_unit.local[A_LEVEL].color_num == TM_1_COLORS) ||
		(ctl->dp_unit.local[A_LEVEL].color_num == TM_2_COLORS))
	{
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Alvl, 0);
	}
	if ((ctl->dp_unit.local[B_LEVEL].color_num == TM_1_COLORS) ||
		(ctl->dp_unit.local[B_LEVEL].color_num == TM_2_COLORS))
	{
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Blvl, 0);
	}
	if ((ctl->dp_unit.local[C_LEVEL].color_num == TM_1_COLORS) ||
		(ctl->dp_unit.local[C_LEVEL].color_num == TM_2_COLORS))
	{
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Clvl, 0);
	}

	if ((ctl->dp_unit.local[P_LEVEL].color_num == TM_1_COLORS) ||
		(ctl->dp_unit.local[P_LEVEL].color_num == TM_2_COLORS))
	{
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Port, 0);
	}
	TM_WRITE_REGISTER(TM.Drop.WREDDropProbMode, TM_Drop_WREDDropProbMode)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_DPSource, PortSrc, ctl->dp_unit.local[P_LEVEL].dp_src[0] +
		ctl->dp_unit.local[P_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[P_LEVEL].dp_src[2]*4);
	TM_REGISTER_SET(TM_Drop_DPSource, ClvlSrc, ctl->dp_unit.local[C_LEVEL].dp_src[0]);
	TM_REGISTER_SET(TM_Drop_DPSource, BlvlSrc, ctl->dp_unit.local[B_LEVEL].dp_src[0]);
	TM_REGISTER_SET(TM_Drop_DPSource, AlvlSrc, ctl->dp_unit.local[A_LEVEL].dp_src[0]);
	TM_REGISTER_SET(TM_Drop_DPSource, QueueSrc, ctl->dp_unit.local[Q_LEVEL].dp_src[0]);
	TM_WRITE_REGISTER(TM.Drop.DPSource, TM_Drop_DPSource)
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_drop_color_num(tm_handle hndl)
{
	int rc = -EFAULT;
	TM_REGISTER_VAR(TM_Drop_WREDDropProbMode)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Port, 0);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Clvl, 0);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Blvl, 0);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Alvl, 0);
	TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Queue, 0);

	if (ctl->dp_unit.local[Q_LEVEL].color_num > 2)
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Queue, 1);
	if (ctl->dp_unit.local[A_LEVEL].color_num > 2)
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Alvl, 1);
	if (ctl->dp_unit.local[B_LEVEL].color_num > 2)
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Blvl, 1);
	if (ctl->dp_unit.local[C_LEVEL].color_num > 2)
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Clvl, 1);
	if (ctl->dp_unit.local[P_LEVEL].color_num > 2)
		TM_REGISTER_SET(TM_Drop_WREDDropProbMode, Port, 1);
	TM_WRITE_REGISTER(TM.Drop.WREDDropProbMode, TM_Drop_WREDDropProbMode)
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_map(tm_handle hndl, enum tm_level lvl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortRangeMap)
	TM_REGISTER_VAR(TM_Sched_ClvltoPortAndBlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_BLvltoClvlAndAlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)
	TM_REGISTER_VAR(TM_Sched_QueueAMap)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (lvl)
	{
	case P_LEVEL:
		TM_REGISTER_SET(TM_Sched_PortRangeMap, Lo, ctl->tm_port_array[index].first_child_c_node);
		TM_REGISTER_SET(TM_Sched_PortRangeMap, Hi, ctl->tm_port_array[index].last_child_c_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortRangeMap, index, TM_Sched_PortRangeMap);
		break;
	case C_LEVEL:
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, ctl->tm_c_node_array[index].parent_port);
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlLo,
			ctl->tm_c_node_array[index].first_child_b_node);
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlHi,
			ctl->tm_c_node_array[index].last_child_b_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvltoPortAndBlvlRangeMap, index, TM_Sched_ClvltoPortAndBlvlRangeMap);
		break;
	case B_LEVEL:
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, Clvl, ctl->tm_b_node_array[index].parent_c_node);
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlLo,
			ctl->tm_b_node_array[index].first_child_a_node);
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlHi,
			ctl->tm_b_node_array[index].last_child_a_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BLvltoClvlAndAlvlRangeMap, index, TM_Sched_BLvltoClvlAndAlvlRangeMap);
		break;
	case A_LEVEL:
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, Blvl, ctl->tm_a_node_array[index].parent_b_node);
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo,
			ctl->tm_a_node_array[index].first_child_queue;);
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi,
			ctl->tm_a_node_array[index].last_child_queue);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, index,
			TM_Sched_ALvltoBlvlAndQueueRangeMap);
		break;
	case Q_LEVEL:
		TM_REGISTER_SET(TM_Sched_QueueAMap, Alvl, ctl->tm_queue_array[index].parent_a_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueAMap, index, TM_Sched_QueueAMap);
		break;
	}

	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_queue(tm_handle hndl, uint32_t queue_ind)
{
	int rc = -EFAULT;

	int entry;
	int base_ind;
	struct tm_queue *queue = NULL;

	TM_REGISTER_VAR(TM_Sched_QueueAMap)
	TM_REGISTER_VAR(TM_Sched_QueueQuantum)
	TM_REGISTER_VAR(TM_Drop_QueueDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (queue_ind < ((struct rmctl *)(ctl->rm))->rm_total_queues)
	{
		queue = &(ctl->tm_queue_array[queue_ind]);
		TM_REGISTER_SET(TM_Sched_QueueAMap, Alvl, queue->parent_a_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueAMap, queue_ind, TM_Sched_QueueAMap);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_QueueEligPrioFuncPtr, Ptr, queue->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, queue_ind, TM_Sched_QueueEligPrioFuncPtr);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_QueueQuantum, Quantum, queue->dwrr_quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueQuantum, queue_ind, TM_Sched_QueueQuantum);
		if (rc)
			goto out;

		/* Set drop profile pointer entry with data exisitng in SW image
		 * to avoid read-modify-write from HW */
		/* Entry in the table */
		entry = queue_ind/TM_Q_DRP_PROF_PER_ENTRY;
		base_ind = entry*TM_Q_DRP_PROF_PER_ENTRY;

		TM_REGISTER_SET(TM_Drop_QueueDropProfPtr, ProfPtr0, ctl->tm_q_lvl_drop_prof_ptr[base_ind+0]);
		TM_REGISTER_SET(TM_Drop_QueueDropProfPtr, ProfPtr1, ctl->tm_q_lvl_drop_prof_ptr[base_ind+1]);
		TM_REGISTER_SET(TM_Drop_QueueDropProfPtr, ProfPtr2, ctl->tm_q_lvl_drop_prof_ptr[base_ind+2]);
		TM_REGISTER_SET(TM_Drop_QueueDropProfPtr, ProfPtr3, ctl->tm_q_lvl_drop_prof_ptr[base_ind+3]);
		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropProfPtr, entry, TM_Drop_QueueDropProfPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int get_hw_queue(tm_handle hndl, uint32_t index, struct tm_queue *queue)
{
	int rc = -EFAULT;

	int entry;
	int ind;

	TM_REGISTER_VAR(TM_Sched_QueueAMap)
	TM_REGISTER_VAR(TM_Sched_QueueQuantum)
	TM_REGISTER_VAR(TM_Drop_QueueDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *)(ctl->rm))->rm_total_queues) {
		TM_READ_TABLE_REGISTER(TM.Sched.QueueAMap, index, TM_Sched_QueueAMap);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_QueueAMap, Alvl, queue->parent_a_node, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, index, TM_Sched_QueueEligPrioFuncPtr);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFuncPtr, Ptr, queue->elig_prio_func_ptr, (uint8_t));

		TM_READ_TABLE_REGISTER(TM.Sched.QueueQuantum, index, TM_Sched_QueueQuantum);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_QueueQuantum, Quantum, queue->dwrr_quantum, (uint16_t));

		/* Entry in the table */
		entry = index/TM_Q_DRP_PROF_PER_ENTRY;
		ind = index - entry*TM_Q_DRP_PROF_PER_ENTRY;

		TM_READ_TABLE_REGISTER(TM.Drop.QueueDropProfPtr, entry, TM_Drop_QueueDropProfPtr);
		if (rc)
			return rc;
		switch (ind) {
		case 0:
			TM_REGISTER_GET(TM_Drop_QueueDropProfPtr, ProfPtr0, queue->wred_profile_ref, (uint8_t));
			break;
		case 1:
			TM_REGISTER_GET(TM_Drop_QueueDropProfPtr, ProfPtr1, queue->wred_profile_ref, (uint8_t));
			break;
		case 2:
			TM_REGISTER_GET(TM_Drop_QueueDropProfPtr, ProfPtr2, queue->wred_profile_ref, (uint8_t));
			break;
		case 3:
			TM_REGISTER_GET(TM_Drop_QueueDropProfPtr, ProfPtr3, queue->wred_profile_ref, (uint8_t));
			break;
		}
	}
	return rc;
}


/**
 */
int set_hw_a_node(tm_handle hndl, uint32_t node_ind)
{
	int rc = -EFAULT;

	int entry;
	int base_ind;
	struct tm_a_node *node = NULL;

	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)
	TM_REGISTER_VAR(TM_Sched_AlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_AlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_AlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (node_ind < ((struct rmctl *)(ctl->rm))->rm_total_a_nodes)
	{
		node = &(ctl->tm_a_node_array[node_ind]);
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, Blvl, node->parent_b_node);
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi, node->last_child_queue);
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo, node->first_child_queue);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, node_ind,
									TM_Sched_ALvltoBlvlAndQueueRangeMap);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFuncPtr, node_ind, TM_Sched_AlvlEligPrioFuncPtr);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlQuantum, Quantum, node->dwrr_quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlQuantum, node_ind, TM_Sched_AlvlQuantum);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlDWRRPrioEn, En, node->dwrr_priority);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlDWRRPrioEn, node_ind, TM_Sched_AlvlDWRRPrioEn);
		if (rc)
			goto out;

		/* Set drop profile pointer entry with data exisitng in SW image
		 * to avoid read-modify-write from HW */
		/* Entry in the table */
		entry = node_ind/TM_A_DRP_PROF_PER_ENTRY;
		base_ind = entry*TM_A_DRP_PROF_PER_ENTRY;

		TM_REGISTER_SET(TM_Drop_AlvlDropProfPtr, ProfPtr0, ctl->tm_a_lvl_drop_prof_ptr[base_ind+0]);
		TM_REGISTER_SET(TM_Drop_AlvlDropProfPtr, ProfPtr1, ctl->tm_a_lvl_drop_prof_ptr[base_ind+1]);
		TM_REGISTER_SET(TM_Drop_AlvlDropProfPtr, ProfPtr2, ctl->tm_a_lvl_drop_prof_ptr[base_ind+2]);
		TM_REGISTER_SET(TM_Drop_AlvlDropProfPtr, ProfPtr3, ctl->tm_a_lvl_drop_prof_ptr[base_ind+3]);

		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropProfPtr, entry, TM_Drop_AlvlDropProfPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int get_hw_a_node(tm_handle hndl, uint32_t index, struct tm_a_node *node)
{
	int rc = -EFAULT;

	int entry;
	int ind;

	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)
	TM_REGISTER_VAR(TM_Sched_AlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_AlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_AlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *)(ctl->rm))->rm_total_a_nodes) {
		TM_READ_TABLE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, index, TM_Sched_ALvltoBlvlAndQueueRangeMap);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_ALvltoBlvlAndQueueRangeMap, Blvl, node->parent_b_node, (uint16_t));
		TM_REGISTER_GET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi, node->last_child_queue, (uint16_t));
		TM_REGISTER_GET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo, node->first_child_queue, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.AlvlEligPrioFuncPtr, index, TM_Sched_AlvlEligPrioFuncPtr);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_AlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr, (uint8_t));

		TM_READ_TABLE_REGISTER(TM.Sched.AlvlQuantum, index, TM_Sched_AlvlQuantum);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_AlvlQuantum, Quantum, node->dwrr_quantum, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.AlvlDWRRPrioEn, index, TM_Sched_AlvlDWRRPrioEn);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_AlvlDWRRPrioEn, En, node->dwrr_priority, (uint8_t));

		/* Entry in the table */
		entry = index/TM_A_DRP_PROF_PER_ENTRY;
		ind = index - entry*TM_A_DRP_PROF_PER_ENTRY;

		TM_READ_TABLE_REGISTER(TM.Drop.AlvlDropProfPtr, entry, TM_Drop_AlvlDropProfPtr);
		if (rc)
			return rc;
		switch (ind) {
		case 0:
			TM_REGISTER_GET(TM_Drop_AlvlDropProfPtr, ProfPtr0, node->wred_profile_ref, (uint8_t));
			break;
		case 1:
			TM_REGISTER_GET(TM_Drop_AlvlDropProfPtr, ProfPtr1, node->wred_profile_ref, (uint8_t));
			break;
		case 2:
			TM_REGISTER_GET(TM_Drop_AlvlDropProfPtr, ProfPtr2, node->wred_profile_ref, (uint8_t));
			break;
		case 3:
			TM_REGISTER_GET(TM_Drop_AlvlDropProfPtr, ProfPtr3, node->wred_profile_ref, (uint8_t));
			break;
		}
	}
	return rc;
}


/**
 */
int set_hw_b_node(tm_handle hndl, uint32_t node_ind)
{
	int rc = -EFAULT;
	int entry;
	int base_ind;
	struct tm_b_node *node = NULL;

	TM_REGISTER_VAR(TM_Sched_BLvltoClvlAndAlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_BlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_BlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_BlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (node_ind < ((struct rmctl *)(ctl->rm))->rm_total_b_nodes)
	{
		node = &(ctl->tm_b_node_array[node_ind]);

		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, Clvl, node->parent_c_node);
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlHi, node->last_child_a_node);
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlLo, node->first_child_a_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BLvltoClvlAndAlvlRangeMap, node_ind,
			TM_Sched_BLvltoClvlAndAlvlRangeMap);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFuncPtr, node_ind, TM_Sched_BlvlEligPrioFuncPtr);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlQuantum, Quantum, node->dwrr_quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlQuantum, node_ind, TM_Sched_BlvlQuantum);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlDWRRPrioEn, En, node->dwrr_priority);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlDWRRPrioEn, node_ind, TM_Sched_BlvlDWRRPrioEn);
		if (rc)
			goto out;

		/* Set drop profile pointer entry with data exisitng in SW image
		 * to avoid read-modify-write from HW */
		/* Entry in the table */
		entry = node_ind/TM_B_DRP_PROF_PER_ENTRY;
		base_ind = entry*TM_B_DRP_PROF_PER_ENTRY;

		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr0, ctl->tm_b_lvl_drop_prof_ptr[base_ind+0]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr1, ctl->tm_b_lvl_drop_prof_ptr[base_ind+1]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr2, ctl->tm_b_lvl_drop_prof_ptr[base_ind+2]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr3, ctl->tm_b_lvl_drop_prof_ptr[base_ind+3]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr4, ctl->tm_b_lvl_drop_prof_ptr[base_ind+4]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr5, ctl->tm_b_lvl_drop_prof_ptr[base_ind+5]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr6, ctl->tm_b_lvl_drop_prof_ptr[base_ind+6]);
		TM_REGISTER_SET(TM_Drop_BlvlDropProfPtr, ProfPtr7, ctl->tm_b_lvl_drop_prof_ptr[base_ind+7]);

		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropProfPtr, entry, TM_Drop_BlvlDropProfPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int get_hw_b_node(tm_handle hndl, uint32_t index, struct tm_b_node *node)
{
	int rc = -EFAULT;

	int entry;
	int ind;

	TM_REGISTER_VAR(TM_Sched_BLvltoClvlAndAlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_BlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_BlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_BlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *)(ctl->rm))->rm_total_b_nodes) {
		TM_READ_TABLE_REGISTER(TM.Sched.BLvltoClvlAndAlvlRangeMap, index, TM_Sched_BLvltoClvlAndAlvlRangeMap);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_BLvltoClvlAndAlvlRangeMap, Clvl, node->parent_c_node, (uint16_t));
		TM_REGISTER_GET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlHi, node->last_child_a_node, (uint16_t));
		TM_REGISTER_GET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlLo, node->first_child_a_node, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.BlvlEligPrioFuncPtr, index, TM_Sched_BlvlEligPrioFuncPtr);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_BlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr, (uint8_t));

		TM_READ_TABLE_REGISTER(TM.Sched.BlvlQuantum, index, TM_Sched_BlvlQuantum);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_BlvlQuantum, Quantum, node->dwrr_quantum, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.BlvlDWRRPrioEn, index, TM_Sched_BlvlDWRRPrioEn);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_BlvlDWRRPrioEn, En, node->dwrr_priority, (uint8_t));

		/* Entry in the table */
		entry = index/TM_B_DRP_PROF_PER_ENTRY;
		ind = index - entry*TM_B_DRP_PROF_PER_ENTRY;

		TM_READ_TABLE_REGISTER(TM.Drop.BlvlDropProfPtr, entry, TM_Drop_BlvlDropProfPtr);
		if (rc)
			return rc;
		switch (ind) {
		case 0:
			TM_REGISTER_GET(TM_Drop_BlvlDropProfPtr, ProfPtr0, node->wred_profile_ref, (uint8_t));
			break;
		case 1:
			TM_REGISTER_GET(TM_Drop_BlvlDropProfPtr, ProfPtr1, node->wred_profile_ref, (uint8_t));
			break;
		case 2:
			TM_REGISTER_GET(TM_Drop_BlvlDropProfPtr, ProfPtr2, node->wred_profile_ref, (uint8_t));
			break;
		case 3:
			TM_REGISTER_GET(TM_Drop_BlvlDropProfPtr, ProfPtr3, node->wred_profile_ref, (uint8_t));
			break;
		}
	}
	return rc;
}


/**
 */
int set_hw_c_node(tm_handle hndl, uint32_t node_ind)
{
	int rc = -EFAULT;
	int i;
	int entry;
	int base_ind;
	struct tm_c_node *node = NULL;

	TM_REGISTER_VAR(TM_Sched_ClvltoPortAndBlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_ClvlQuantum)
	TM_REGISTER_VAR(TM_Sched_ClvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_ClvlDropProfPtr_CoS)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (node_ind < ((struct rmctl *)(ctl->rm))->rm_total_c_nodes)
	{
		node = &(ctl->tm_c_node_array[node_ind]);
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, node->parent_port);
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlHi, node->last_child_b_node);
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlLo, node->first_child_b_node);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvltoPortAndBlvlRangeMap, node_ind,
			TM_Sched_ClvltoPortAndBlvlRangeMap);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFuncPtr, node_ind, TM_Sched_ClvlEligPrioFuncPtr);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlQuantum, Quantum, node->dwrr_quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlQuantum, node_ind, TM_Sched_ClvlQuantum);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlDWRRPrioEn, En, node->dwrr_priority);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlDWRRPrioEn, node_ind, TM_Sched_ClvlDWRRPrioEn);
		if (rc)
			goto out;

		/* Set drop profile pointer entry with data exisitng in SW image
		 * to avoid read-modify-write from HW */
		/* Entry in the table */
		entry = node_ind/TM_C_DRP_PROF_PER_ENTRY;
		base_ind = entry*TM_C_DRP_PROF_PER_ENTRY;

		for (i = 0; i < TM_WRED_COS; i++) {
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr0,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+0]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr1,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+1]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr2,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+2]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr3,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+3]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr4,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+4]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr5,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+5]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr6,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+6]);
			TM_REGISTER_SET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr7,
				ctl->tm_c_lvl_drop_prof_ptr[i][base_ind+7]);

			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropProfPtr_CoS[i], entry, TM_Drop_ClvlDropProfPtr_CoS);
			if (rc)
				goto out;
		}
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int get_hw_c_node(tm_handle hndl, uint32_t index, struct tm_c_node *node)
{
	int rc = -EFAULT;
	int i;
	int entry;
	int ind;

	TM_REGISTER_VAR(TM_Sched_ClvltoPortAndBlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_ClvlQuantum)
	TM_REGISTER_VAR(TM_Sched_ClvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Drop_ClvlDropProfPtr_CoS)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *)(ctl->rm))->rm_total_c_nodes) {
		TM_READ_TABLE_REGISTER(TM.Sched.ClvltoPortAndBlvlRangeMap, index, TM_Sched_ClvltoPortAndBlvlRangeMap);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, node->parent_port, (uint8_t));
		TM_REGISTER_GET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlHi, node->last_child_b_node, (uint16_t));
		TM_REGISTER_GET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlLo, node->first_child_b_node, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.ClvlEligPrioFuncPtr, index, TM_Sched_ClvlEligPrioFuncPtr);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_ClvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr, (uint8_t));

		TM_READ_TABLE_REGISTER(TM.Sched.ClvlQuantum, index, TM_Sched_ClvlQuantum);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_ClvlQuantum, Quantum, node->dwrr_quantum, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.ClvlDWRRPrioEn, index, TM_Sched_ClvlDWRRPrioEn);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_ClvlDWRRPrioEn, En, node->dwrr_priority, (uint8_t));

		/* Entry in the table */
		entry = index/TM_C_DRP_PROF_PER_ENTRY;
		ind = index - entry*TM_C_DRP_PROF_PER_ENTRY;

		for (i = 0; i < TM_WRED_COS; i++) {
			TM_READ_TABLE_REGISTER(TM.Drop.ClvlDropProfPtr_CoS[i], entry, TM_Drop_ClvlDropProfPtr_CoS);
			if (rc)
				return rc;
			switch (ind) {
			case 0:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr0,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 1:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr1,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 2:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr2,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 3:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr3,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 4:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr4,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 5:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr5,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 6:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr6,
					node->wred_profile_ref[i], (uint8_t));
				break;
			case 7:
				TM_REGISTER_GET(TM_Drop_ClvlDropProfPtr_CoS, ProfPtr7,
					node->wred_profile_ref[i], (uint8_t));
				break;
			}
		}
	}
	return rc;
}


/**
 */
int set_hw_queue_elig_prio_func_ptr(tm_handle hndl, uint32_t ind)
{
	int rc = -EFAULT;

	struct tm_queue *node;
	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (ind < ((struct rmctl *) (ctl->rm))->rm_total_queues) {
		node = &(ctl->tm_queue_array[ind]);
		TM_REGISTER_SET(TM_Sched_QueueEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, ind, TM_Sched_QueueEligPrioFuncPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_a_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind)
{
	int rc = -EFAULT;

	struct tm_a_node *node;
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (ind < ((struct rmctl *) (ctl->rm))->rm_total_a_nodes) {
		node = &(ctl->tm_a_node_array[ind]);
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFuncPtr, ind, TM_Sched_AlvlEligPrioFuncPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_b_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind)
{
	int rc = -EFAULT;

	struct tm_b_node *node;
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (ind < ((struct rmctl *) (ctl->rm))->rm_total_b_nodes) {
		node = &(ctl->tm_b_node_array[ind]);
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFuncPtr, ind, TM_Sched_BlvlEligPrioFuncPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_c_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind)
{
	int rc = -EFAULT;

	struct tm_c_node *node;
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (ind < ((struct rmctl *) (ctl->rm))->rm_total_c_nodes) {
		node = &(ctl->tm_c_node_array[ind]);
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFuncPtr, Ptr, node->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFuncPtr, ind, TM_Sched_ClvlEligPrioFuncPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_port_elig_prio_func_ptr(tm_handle hndl, uint8_t port_ind)
{
	int rc = -EFAULT;

	struct tm_port *port;
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		port = &(ctl->tm_port_array[port_ind]);
		TM_REGISTER_SET(TM_Sched_PortEligPrioFuncPtr, Ptr, port->elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFuncPtr, port_ind, TM_Sched_PortEligPrioFuncPtr);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}



#define GET_ELIG_PRIO_HW_MAC(level, max_nodes)	\
	do {\
		int rc = -EFAULT;\
		TM_REGISTER_VAR(TM_Sched_##level##EligPrioFuncPtr)\
\
		DECLARE_TM_CTL_PTR(ctl, hndl)\
		CHECK_TM_CTL_PTR(ctl)\
\
		if (ind < max_nodes) {\
			TM_READ_TABLE_REGISTER(TM.Sched.level##EligPrioFuncPtr, ind,\
									TM_Sched_##level##EligPrioFuncPtr);\
			if (rc)\
				goto out;\
			if (pfunc)\
				TM_REGISTER_GET(TM_Sched_##level##EligPrioFuncPtr, Ptr, *pfunc, (uint8_t));\
		} \
out:\
		COMPLETE_HW_WRITE\
		return rc;\
	} while (0)

int get_hw_queue_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc)
{
	GET_ELIG_PRIO_HW_MAC(Queue, ((struct rmctl *) (ctl->rm))->rm_total_queues);
}
int get_hw_a_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc)
{
	GET_ELIG_PRIO_HW_MAC(Alvl, ((struct rmctl *) (ctl->rm))->rm_total_a_nodes);
}
int get_hw_b_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc)
{
	GET_ELIG_PRIO_HW_MAC(Blvl, ((struct rmctl *) (ctl->rm))->rm_total_b_nodes);
}
int get_hw_c_node_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc)
{
	GET_ELIG_PRIO_HW_MAC(Clvl, ((struct rmctl *) (ctl->rm))->rm_total_c_nodes);
}
int get_hw_port_elig_prio_func_ptr(tm_handle hndl, uint32_t ind, uint8_t *pfunc)
{
	GET_ELIG_PRIO_HW_MAC(Port, ((struct rmctl *) (ctl->rm))->rm_total_ports);
}


/**
 */

int __set_hw_port_shaping(tm_handle hndl, uint8_t port_ind,
						uint32_t cbw, uint32_t ebw, uint32_t cbs, uint32_t ebs)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortTokenBucketTokenEnDiv)
	TM_REGISTER_VAR(TM_Sched_PortTokenBucketBurstSize)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, Periods, TM_FIXED_2_5_G_PORT_SHAPING_PERIODS);
		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, MinToken, cbw/TM_FIXED_10_M_SHAPING_TOKEN);
		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, MaxToken, ebw/TM_FIXED_10_M_SHAPING_TOKEN);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortTokenBucketTokenEnDiv, port_ind,
			TM_Sched_PortTokenBucketTokenEnDiv);
		if (rc)
			goto out;
		TM_REGISTER_SET(TM_Sched_PortTokenBucketBurstSize, MaxBurstSz, cbs);
		TM_REGISTER_SET(TM_Sched_PortTokenBucketBurstSize, MinBurstSz, ebs);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortTokenBucketBurstSize, port_ind,
			TM_Sched_PortTokenBucketBurstSize);
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}

/**
 */
int set_hw_port_shaping_ex(tm_handle hndl, uint8_t port_ind,
						uint32_t cbw, uint32_t ebw ,  uint32_t *pcbs, uint32_t *pebs)
{
	int cbs, ebs;
	int rc = 0;
/*
	cbs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) / 1024;
	ebs= 2 * ( (min_bw/TM_FIXED_10_M_SHAPING_TOKEN) / 1024;
*/
	SET_BURSTS(TM_FIXED_2_5_G_PORT_SHAPING_BURST_SIZE, TM_FIXED_2_5_G_PORT_SHAPING_BURST_SIZE);

	if (rc == 0)
		return __set_hw_port_shaping(hndl, port_ind, cbw, ebw, cbs, ebs);
	else
		return TM_CONF_MIN_TOKEN_TOO_LARGE;

}

int set_hw_port_shaping_def(tm_handle hndl, uint8_t port_ind)
{
	int rc = -EFAULT;
	struct tm_port *port = NULL;


	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if ((port_ind < get_tm_port_count()) && (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports)) {
		port = &(ctl->tm_port_array[port_ind]);
		switch (port->port_speed) {
		case TM_1G_PORT:
			return set_hw_port_shaping_ex(hndl, port_ind, 1000, 0, NULL, NULL);
			break;
		case TM_2HG_PORT:
			return set_hw_port_shaping_ex(hndl, port_ind, 2500, 0, NULL, NULL);
			break;
		case TM_10G_PORT:
			return set_hw_port_shaping_ex(hndl, port_ind, 10000, 0, NULL, NULL);
			break;
			/* TBD */
		case TM_40G_PORT:
		case TM_50G_PORT:
		case TM_100G_PORT:
		default:
			break;
		}
	}
	COMPLETE_HW_WRITE
	return rc;
}

int get_hw_port_shaping(tm_handle hndl, uint32_t index,
						uint32_t *pcbw, uint32_t *pebw, uint32_t *pcbs, uint32_t *pebs)
{
	GET_NODE_SHAPING_MAC(Port, index < ((struct rmctl *) (ctl->rm))->rm_total_ports);
}

/**
 */
int set_hw_port_scheduling(tm_handle hndl, uint8_t port_ind)
{
	int rc = -EFAULT;
	struct tm_port *port = NULL;
#ifdef MV_QMTM_NSS_A0
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosLo)
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosHi)
#endif
	TM_REGISTER_VAR(TM_Sched_PortDWRRPrioEn)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		port = &(ctl->tm_port_array[port_ind]);

#ifdef MV_QMTM_NSS_A0
		/* DWRR for Port */
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum0, port->dwrr_quantum[0].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum1, port->dwrr_quantum[1].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum2, port->dwrr_quantum[2].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum3, port->dwrr_quantum[3].quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortQuantumsPriosLo, port_ind, TM_Sched_PortQuantumsPriosLo);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum4, port->dwrr_quantum[4].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum5, port->dwrr_quantum[5].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum6, port->dwrr_quantum[6].quantum);
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum7, port->dwrr_quantum[7].quantum);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortQuantumsPriosHi, port_ind, TM_Sched_PortQuantumsPriosHi);
		if (rc)
			goto out;
#endif
		/* DWRR for C-nodes in Port's range */
		TM_REGISTER_SET(TM_Sched_PortDWRRPrioEn, En, port->dwrr_priority);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDWRRPrioEn, port_ind, TM_Sched_PortDWRRPrioEn);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_port_drop(tm_handle hndl, uint8_t port_ind)
{
	int rc = -EFAULT;
	struct tm_port *port = NULL;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		port = &(ctl->tm_port_array[port_ind]);
		rc = set_hw_ports_drop_profile(hndl, port->wred_profile_ref, port_ind);
	}
	return rc;
}


int set_hw_port_drop_cos(tm_handle hndl, uint8_t port_ind, uint8_t cos)
{
	int rc = 0;
	struct tm_port *port ;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		port = &(ctl->tm_port_array[port_ind]);
		rc = set_hw_ports_drop_profile_cos(hndl, cos, port->wred_profile_ref_cos[cos], port_ind);
	}
	return rc;
}


/**
 */
int set_hw_port(tm_handle hndl, uint8_t port_ind)
{
	int rc = -EFAULT;
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (port_ind < ((struct rmctl *) (ctl->rm))->rm_total_ports)
	{
		rc = set_hw_map(hndl, P_LEVEL, port_ind);
		if (rc < 0)
			goto out;

		rc = set_hw_port_shaping_def(hndl, port_ind);
		if (rc < 0)
			goto out;
#ifdef MV_QMTM_NSS_A0
		rc = set_hw_port_scheduling(hndl, port_ind);
		if (rc < 0)
			goto out;
#endif

		rc = set_hw_port_drop(hndl, port_ind);
		if (rc < 0)
			goto out;

		rc = set_hw_port_elig_prio_func_ptr(hndl, port_ind);
	}
out:
	return rc;
}


/**
 */
int get_hw_port(tm_handle hndl, uint8_t index, struct tm_port *port)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortRangeMap)
#ifdef MV_QMTM_NSS_A0
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosLo)
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosHi)
#endif
	TM_REGISTER_VAR(TM_Sched_PortDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFuncPtr)
	/* No Drop Profile Ptr for Port */

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *)(ctl->rm))->rm_total_ports) {
		TM_READ_TABLE_REGISTER(TM.Sched.PortRangeMap, index, TM_Sched_PortRangeMap);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_PortRangeMap, Hi, port->last_child_c_node, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortRangeMap, Lo, port->first_child_c_node, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.PortEligPrioFuncPtr, index, TM_Sched_PortEligPrioFuncPtr);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_PortEligPrioFuncPtr, Ptr, port->elig_prio_func_ptr, (uint8_t));

#ifdef MV_QMTM_NSS_A0
		/* DWRR for Port */
		TM_READ_TABLE_REGISTER(TM.Sched.PortQuantumsPriosLo, index, TM_Sched_PortQuantumsPriosLo);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosLo, Quantum0, port->dwrr_quantum[0].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosLo, Quantum1, port->dwrr_quantum[1].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosLo, Quantum2, port->dwrr_quantum[2].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosLo, Quantum3, port->dwrr_quantum[3].quantum, (uint16_t));

		TM_READ_TABLE_REGISTER(TM.Sched.PortQuantumsPriosHi, index, TM_Sched_PortQuantumsPriosHi);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosHi, Quantum4, port->dwrr_quantum[4].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosHi, Quantum5, port->dwrr_quantum[5].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosHi, Quantum6, port->dwrr_quantum[6].quantum, (uint16_t));
		TM_REGISTER_GET(TM_Sched_PortQuantumsPriosHi, Quantum7, port->dwrr_quantum[7].quantum, (uint16_t));
#endif

		TM_READ_TABLE_REGISTER(TM.Sched.PortDWRRPrioEn, index, TM_Sched_PortDWRRPrioEn);
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Sched_PortDWRRPrioEn, En, port->dwrr_priority, (uint8_t));

		/* No Drop Profile Ptr for Port */
	}
	return rc;
}


/**
 */
int set_hw_tree_deq_status(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_TreeDeqEn)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_TreeDeqEn, En, ctl->tree_deq_status);
	TM_WRITE_REGISTER(TM.Sched.TreeDeqEn, TM_Sched_TreeDeqEn)
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


#ifdef MV_QMTM_NSS_A0
/**
 */
int set_hw_tree_dwrr_priority(tm_handle hndl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_TreeDWRRPrioEn)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_TreeDWRRPrioEn, PrioEn, ctl->tree_dwrr_priority);
	TM_WRITE_REGISTER(TM.Sched.TreeDWRRPrioEn, TM_Sched_TreeDWRRPrioEn)
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}
#endif


/**
 */
int set_hw_deq_status(tm_handle hndl, enum tm_level lvl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (lvl)
	{
	case P_LEVEL:
		TM_REGISTER_SET(TM_Sched_PortEligPrioFuncPtr, Ptr, ctl->tm_port_array[index].elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFuncPtr, index, TM_Sched_PortEligPrioFuncPtr);
		break;
	case C_LEVEL:
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFuncPtr, Ptr, ctl->tm_c_node_array[index].elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFuncPtr, index, TM_Sched_ClvlEligPrioFuncPtr);
		break;
	case B_LEVEL:
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFuncPtr, Ptr, ctl->tm_b_node_array[index].elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFuncPtr, index, TM_Sched_BlvlEligPrioFuncPtr);
		break;
	case A_LEVEL:
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFuncPtr, Ptr, ctl->tm_a_node_array[index].elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFuncPtr, index, TM_Sched_AlvlEligPrioFuncPtr);
		break;
	case Q_LEVEL:
		TM_REGISTER_SET(TM_Sched_QueueEligPrioFuncPtr, Ptr, ctl->tm_queue_array[index].elig_prio_func_ptr);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, index, TM_Sched_QueueEligPrioFuncPtr);
		break;
	}
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_disable_ports(tm_handle hndl, uint32_t total_ports)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_PortEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* Disable Ports */
	for (i = 0; i < (int)total_ports; i++)
	{
		TM_REGISTER_SET(TM_Sched_PortEligPrioFuncPtr, Ptr, 63); /* DeQ disable function ID */
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFuncPtr, i, TM_Sched_PortEligPrioFuncPtr);
		if (rc)
			goto out;
	}

out:
	COMPLETE_HW_WRITE
	return rc;
}




/**
*  Configure user Q level Eligible Priority Function
*/
int set_hw_q_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset)
{
	int rc = -EFAULT;
	struct tm_elig_prio_func_queue *params;

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFunc)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	params = &(ctl->tm_elig_prio_q_lvl_tbl[func_offset]);
	/* assign register fields */
	TM_REGISTER_SET(TM_Sched_QueueEligPrioFunc , FuncOut0 , params->tbl_entry.func_out[0])
	TM_REGISTER_SET(TM_Sched_QueueEligPrioFunc , FuncOut1 , params->tbl_entry.func_out[1])
	TM_REGISTER_SET(TM_Sched_QueueEligPrioFunc , FuncOut2 , params->tbl_entry.func_out[2])
	TM_REGISTER_SET(TM_Sched_QueueEligPrioFunc , FuncOut3 , params->tbl_entry.func_out[3])
	/* write register */
	TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFunc, func_offset, TM_Sched_QueueEligPrioFunc)
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}

/**
 *  Configure user Node level Eligible Priority Function
 */
int set_hw_a_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFunc)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < 8; i++)	{
		/* assign register fields */
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFunc , FuncOut0 ,
			ctl->tm_elig_prio_a_lvl_tbl[func_offset].tbl_entry[i].func_out[0])
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFunc , FuncOut1 ,
			ctl->tm_elig_prio_a_lvl_tbl[func_offset].tbl_entry[i].func_out[1])
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFunc , FuncOut2 ,
			ctl->tm_elig_prio_a_lvl_tbl[func_offset].tbl_entry[i].func_out[2])
		TM_REGISTER_SET(TM_Sched_AlvlEligPrioFunc , FuncOut3 ,
			ctl->tm_elig_prio_a_lvl_tbl[func_offset].tbl_entry[i].func_out[3])
		/* write register */
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFunc, func_offset + i*64 , TM_Sched_AlvlEligPrioFunc)
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_b_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFunc)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < 8; i++)	{
		/* assign register fields */
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFunc , FuncOut0 ,
			ctl->tm_elig_prio_b_lvl_tbl[func_offset].tbl_entry[i].func_out[0])
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFunc , FuncOut1 ,
			ctl->tm_elig_prio_b_lvl_tbl[func_offset].tbl_entry[i].func_out[1])
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFunc , FuncOut2 ,
			ctl->tm_elig_prio_b_lvl_tbl[func_offset].tbl_entry[i].func_out[2])
		TM_REGISTER_SET(TM_Sched_BlvlEligPrioFunc , FuncOut3 ,
			ctl->tm_elig_prio_b_lvl_tbl[func_offset].tbl_entry[i].func_out[3])
		/* write register */
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFunc, func_offset + i*64 , TM_Sched_BlvlEligPrioFunc)
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_c_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFunc)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < 8; i++)	{
		/* assign register fields */
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFunc , FuncOut0 ,
			ctl->tm_elig_prio_c_lvl_tbl[func_offset].tbl_entry[i].func_out[0]);
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFunc , FuncOut1 ,
			ctl->tm_elig_prio_c_lvl_tbl[func_offset].tbl_entry[i].func_out[1]);
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFunc , FuncOut2 ,
			ctl->tm_elig_prio_c_lvl_tbl[func_offset].tbl_entry[i].func_out[2]);
		TM_REGISTER_SET(TM_Sched_ClvlEligPrioFunc , FuncOut3 ,
			ctl->tm_elig_prio_c_lvl_tbl[func_offset].tbl_entry[i].func_out[3]);
		/* write register */
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFunc, func_offset + i*64 , TM_Sched_ClvlEligPrioFunc);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE;
	return rc;
}

int set_hw_p_lvl_elig_prio_func_entry(tm_handle hndl, uint16_t func_offset)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_PortEligPrioFunc)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	for (i = 0; i < 8; i++)	{/* Entry ID */
		/* assign register fields */
		TM_REGISTER_SET(TM_Sched_PortEligPrioFunc , FuncOut0 ,
			ctl->tm_elig_prio_p_lvl_tbl[func_offset].tbl_entry[i].func_out[0])
		TM_REGISTER_SET(TM_Sched_PortEligPrioFunc , FuncOut1 ,
			ctl->tm_elig_prio_p_lvl_tbl[func_offset].tbl_entry[i].func_out[1])
		TM_REGISTER_SET(TM_Sched_PortEligPrioFunc , FuncOut2 ,
			ctl->tm_elig_prio_p_lvl_tbl[func_offset].tbl_entry[i].func_out[2])
		TM_REGISTER_SET(TM_Sched_PortEligPrioFunc , FuncOut3 ,
			ctl->tm_elig_prio_p_lvl_tbl[func_offset].tbl_entry[i].func_out[3])
		/* write register */
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFunc , func_offset + i*64 , TM_Sched_PortEligPrioFunc);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_elig_prio_func_tbl_q_level(tm_handle hndl)
{
	int j;
	int rc;

	for (j = 0; j < TM_ELIG_FUNC_TABLE_SIZE; j++) {
		rc = set_hw_q_elig_prio_func_entry(hndl, j);
		if (rc)
			break;
	}
	return rc;
}


/**
 */
int set_hw_elig_prio_func_tbl_a_level(tm_handle hndl)
{
	int rc =  -EFAULT;
	int j;
	for (j = 0; j < TM_ELIG_FUNC_TABLE_SIZE; j++) {
		rc = set_hw_a_lvl_elig_prio_func_entry(hndl, j);
		if (rc)
			break;
	}
	return rc;
}


/**
 */
int set_hw_elig_prio_func_tbl_b_level(tm_handle hndl)
{
	int rc =  -EFAULT;
	int j;
	for (j = 0; j < TM_ELIG_FUNC_TABLE_SIZE; j++) {
		rc = set_hw_b_lvl_elig_prio_func_entry(hndl, j);
		if (rc)
			break;
	}
	return rc;
}

/**
 */
int set_hw_elig_prio_func_tbl_c_level(tm_handle hndl)
{
	int rc =  -EFAULT;
	int j;
	for (j = 0; j < TM_ELIG_FUNC_TABLE_SIZE; j++) {
		rc = set_hw_c_lvl_elig_prio_func_entry(hndl, j);
		if (rc)
			break;
	}
	return rc;
}

/**
 */
int set_hw_elig_prio_func_tbl_p_level(tm_handle hndl)
{
	int rc =  -EFAULT;
	int j;
	for (j = 0; j < TM_ELIG_FUNC_TABLE_SIZE; j++) {
		rc = set_hw_p_lvl_elig_prio_func_entry(hndl, j);
		if (rc)
			break;
	}
	return rc;
}

/**
 */
int set_hw_port_deficit_clear(tm_handle hndl, uint8_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortDefPrioHi)
	TM_REGISTER_VAR(TM_Sched_PortDefPrioLo)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *) (ctl->rm))->rm_total_ports) {
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit0, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit1, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit2, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit3, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDefPrioHi, index, TM_Sched_PortDefPrioHi);
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit0, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit1, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit2, 0x1);
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit3, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDefPrioLo, index, TM_Sched_PortDefPrioLo);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_c_node_deficit_clear(tm_handle hndl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_CLvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *) (ctl->rm))->rm_total_c_nodes) {
		TM_REGISTER_SET(TM_Sched_CLvlDef, Deficit, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.CLvlDef, index, TM_Sched_CLvlDef);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_b_node_deficit_clear(tm_handle hndl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_BlvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *) (ctl->rm))->rm_total_b_nodes) {
		TM_REGISTER_SET(TM_Sched_BlvlDef, Deficit, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlDef, index, TM_Sched_BlvlDef);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_a_node_deficit_clear(tm_handle hndl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_AlvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)
	if (index < ((struct rmctl *) (ctl->rm))->rm_total_a_nodes) {
		TM_REGISTER_SET(TM_Sched_AlvlDef, Deficit, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlDef, index, TM_Sched_AlvlDef);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_queue_deficit_clear(tm_handle hndl, uint32_t index)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_QueueDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *) (ctl->rm))->rm_total_queues) {
		TM_REGISTER_SET(TM_Sched_QueueDef, Deficit, 0x1);
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueDef, index, TM_Sched_QueueDef);
		if (rc)
			goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_dp_local_resp(tm_handle hndl, uint8_t port_dp, enum tm_level local_lvl)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_RespLocalDPSel)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	(void)port_dp;

	TM_REGISTER_SET(TM_Drop_RespLocalDPSel, DPSel, local_lvl);
	TM_WRITE_REGISTER(TM.Drop.RespLocalDPSel, TM_Drop_RespLocalDPSel)
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_port_sms_attr_pbase(tm_handle hndl, uint8_t index)
{
	int rc = -EFAULT;
	(void) hndl;
	(void)index;
	/*
	check if it exists in SN
	struct TM_RCB_SMSPortAttrPBase p_attr;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	p_attr.PBase = ctl->tm_port_sms_attr_pbase[index].pbase;
	p_attr.PShift = ctl->tm_port_sms_attr_pbase[index].pshift;
	*//* debug fields *//*
	p_attr.AddTiRxHead = 0;
	p_attr.RxChanFieldEn = 0;

	p_attr.RxTimeFieldEn = 0;
	rc = tm_table_entry_write(TM_ENV(ctl), (void *)&TM.RCB.SMSPortAttrPBase, index, &p_attr);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
*/
	return rc;
}


/**
 */
int set_hw_port_sms_attr_qmap_pars(tm_handle hndl, uint8_t index)
{
	int rc = -EFAULT;
	(void) hndl;
	(void) index;
	/*
	check if it exists in SN
	struct TM_RCB_SMSPortAttrQmapPars q_map ;

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	q_map.ParseMode = ctl->tm_port_sms_attr_qmap_pars[index].mode;
	q_map.BaseQ = ctl->tm_port_sms_attr_qmap_pars[index].base_q;
	q_map.DfltColor = ctl->tm_port_sms_attr_qmap_pars[index].dcolor;
	*//* debug fields *//*
	q_map.EType = 0x8100;
	q_map.QmapStrip = 0;
	q_map.Offs = 0;
	rc = tm_table_entry_write(TM_ENV(ctl), (void *)&TM.RCB.SMSPortAttrQmapPars, index, &q_map);
	if (rc)
		goto out;

out:
	COMPLETE_HW_WRITE
*/
	return rc;
}




int set_hw_dp_source(tm_handle hndl)
{
	int rc;
	TM_REGISTER_VAR(TM_Drop_DPSource)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Drop_DPSource, PortSrc, ctl->dp_unit.local[P_LEVEL].dp_src[0] +
	ctl->dp_unit.local[P_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[P_LEVEL].dp_src[2]*4)
	TM_REGISTER_SET(TM_Drop_DPSource, ClvlSrc, ctl->dp_unit.local[C_LEVEL].dp_src[0] +
	ctl->dp_unit.local[C_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[C_LEVEL].dp_src[2]*4)
	TM_REGISTER_SET(TM_Drop_DPSource, BlvlSrc, ctl->dp_unit.local[B_LEVEL].dp_src[0] +
	ctl->dp_unit.local[B_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[B_LEVEL].dp_src[2]*4)
	TM_REGISTER_SET(TM_Drop_DPSource, AlvlSrc, ctl->dp_unit.local[A_LEVEL].dp_src[0] +
	ctl->dp_unit.local[A_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[A_LEVEL].dp_src[2]*4)
	TM_REGISTER_SET(TM_Drop_DPSource, QueueSrc, ctl->dp_unit.local[Q_LEVEL].dp_src[0] +
	ctl->dp_unit.local[Q_LEVEL].dp_src[1]*2 + ctl->dp_unit.local[Q_LEVEL].dp_src[2]*4)
	TM_WRITE_REGISTER(TM.Drop.DPSource, TM_Drop_DPSource)
	if (rc)
		goto out;
out:
	COMPLETE_HW_WRITE
	return rc;
}


int set_hw_queue_cos(tm_handle hndl, uint32_t index)
{
	int rc = -EFAULT;
	uint32_t entry = index/4;
	uint32_t base = entry*4;

	TM_REGISTER_VAR(TM_Drop_QueueCoSConf)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if (index < ((struct rmctl *) (ctl->rm))->rm_total_queues) {
		TM_REGISTER_SET(TM_Drop_QueueCoSConf, QueueCos0, ctl->tm_q_cos[base]);
		TM_REGISTER_SET(TM_Drop_QueueCoSConf, QueueCos1, ctl->tm_q_cos[base + 1]);
		TM_REGISTER_SET(TM_Drop_QueueCoSConf, QueueCos2, ctl->tm_q_cos[base + 2]);
		TM_REGISTER_SET(TM_Drop_QueueCoSConf, QueueCos3, ctl->tm_q_cos[base + 3]);
		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueCoSConf, entry, TM_Drop_QueueCoSConf);
	}

	COMPLETE_HW_WRITE
	return rc;
}


/**
 */
int set_hw_register_db_default(tm_handle hndl)
{
	int rc = -EFAULT;
	uint32_t i, j;
	uint32_t cos;
	uint32_t max_entries;

	/* Drop */
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDDPRatio)

	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_AlvlDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_AlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Drop_AlvlREDCurve_Color)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDDPRatio)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_BlvlDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_BlvlDropProfPtr)
	TM_REGISTER_VAR(TM_Drop_BlvlREDCurve_Table)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfTailDrpThresh_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDDPRatio_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDMinThresh_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDParams_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropPrfWREDScaleRatio_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlDropProfPtr_CoS)
	TM_REGISTER_VAR(TM_Drop_ClvlREDCurve_CoS)
	TM_REGISTER_VAR(TM_Drop_ExcMask)
	TM_REGISTER_VAR(TM_Drop_DPSource)
	TM_REGISTER_VAR(TM_Drop_Drp_Decision_hierarchy_to_Query_debug)/*NEW*/
	TM_REGISTER_VAR(TM_Drop_Drp_Decision_to_Query_debug)/*NEW*/
	TM_REGISTER_VAR(TM_Drop_EccConfig)/*NEW*/
	TM_REGISTER_VAR(TM_Drop_PortDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfTailDrpThresh_CoSRes)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDDPRatio)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDDPRatio_CoSRes)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDMinThresh_CoSRes)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDParams_CoSRes)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_PortDropPrfWREDScaleRatio_CoSRes)
	TM_REGISTER_VAR(TM_Drop_PortREDCurve)
	TM_REGISTER_VAR(TM_Drop_PortREDCurve_CoS)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfTailDrpThresh)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDDPRatio)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDMinThresh)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDParams)
	TM_REGISTER_VAR(TM_Drop_QueueDropPrfWREDScaleRatio)
	TM_REGISTER_VAR(TM_Drop_QueueCoSConf)
	TM_REGISTER_VAR(TM_Drop_QueueDropProfPtr)
	TM_REGISTER_VAR(TM_Drop_QueueREDCurve_Color)
	TM_REGISTER_VAR(TM_Drop_RespLocalDPSel)
	TM_REGISTER_VAR(TM_Drop_WREDDropProbMode)
	TM_REGISTER_VAR(TM_Drop_WREDMaxProbModePerColor)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Drop_AlvlDropProb)
	TM_REGISTER_VAR(TM_Drop_AlvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_BlvlDropProb)
	TM_REGISTER_VAR(TM_Drop_BlvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_ClvlDropProb)
	TM_REGISTER_VAR(TM_Drop_ClvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_EccMemParams)/*NEW*/
	TM_REGISTER_VAR(TM_Drop_ErrCnt)
	TM_REGISTER_VAR(TM_Drop_ErrStus)
	TM_REGISTER_VAR(TM_Drop_ExcCnt)
	TM_REGISTER_VAR(TM_Drop_FirstExc)
	TM_REGISTER_VAR(TM_Drop_ForceErr)
	TM_REGISTER_VAR(TM_Drop_Id)
	TM_REGISTER_VAR(TM_Drop_PortDropProb)
	TM_REGISTER_VAR(TM_Drop_PortDropProbPerCoS_CoS)
	TM_REGISTER_VAR(TM_Drop_PortInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_PortInstAndAvgQueueLengthPerCoS_CoS)
	TM_REGISTER_VAR(TM_Drop_QueueAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_QueueDropProb)
#endif

	/* Sched */
	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)
	TM_REGISTER_VAR(TM_Sched_AlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_AlvlDef)
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_AlvlPerConf)
	TM_REGISTER_VAR(TM_Sched_AlvlPerRateShpPrms)
	TM_REGISTER_VAR(TM_Sched_AlvlPerRateShpPrmsInt)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_AlvlTokenBucketBurstSize)
	TM_REGISTER_VAR(TM_Sched_AlvlTokenBucketTokenEnDiv)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_ALevelShaperBucketNeg)
	TM_REGISTER_VAR(TM_Sched_AlvlBankEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL0ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL0ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL1ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL1ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL2ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlL2ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlMyQ)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlMyQEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlNodeState)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlPerStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlRRDWRRStatus01)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlRRDWRRStatus23)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlRRDWRRStatus45)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlRRDWRRStatus67)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_AlvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_AlvlWFS)/*NEW*/
#endif
	TM_REGISTER_VAR(TM_Sched_BLvltoClvlAndAlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_BlvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_BlvlDef)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_BlvlPerConf)
	TM_REGISTER_VAR(TM_Sched_BlvlPerRateShpPrms)
	TM_REGISTER_VAR(TM_Sched_BlvlPerRateShpPrmsInt)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlQuantum)
	TM_REGISTER_VAR(TM_Sched_BlvlTokenBucketBurstSize)
	TM_REGISTER_VAR(TM_Sched_BlvlTokenBucketTokenEnDiv)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_BLevelShaperBucketNeg)
	TM_REGISTER_VAR(TM_Sched_BlvlBankEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlL0ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlL0ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlL1ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlL1ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlMyQ)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlMyQEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlNodeState)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlPerStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlRRDWRRStatus01)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlRRDWRRStatus23)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlRRDWRRStatus45)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlRRDWRRStatus67)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_BlvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_BlvlWFS)/*NEW*/
#endif
	TM_REGISTER_VAR(TM_Sched_CLvlDef)
	TM_REGISTER_VAR(TM_Sched_ClvlDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_ClvlPerConf)
	TM_REGISTER_VAR(TM_Sched_ClvlPerRateShpPrms)
	TM_REGISTER_VAR(TM_Sched_ClvlPerRateShpPrmsInt)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlQuantum)
	TM_REGISTER_VAR(TM_Sched_ClvlTokenBucketBurstSize)
	TM_REGISTER_VAR(TM_Sched_ClvlTokenBucketTokenEnDiv)
	TM_REGISTER_VAR(TM_Sched_ClvltoPortAndBlvlRangeMap)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_CLevelShaperBucketNeg)
	TM_REGISTER_VAR(TM_Sched_ClvlBPFromSTF)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlBankEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlL0ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlL0ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlMyQ)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlMyQEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlNodeState)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlPerStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlRRDWRRStatus01)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlRRDWRRStatus23)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlRRDWRRStatus45)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlRRDWRRStatus67)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ClvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_ClvlWFS)/*NEW*/
#endif
	TM_REGISTER_VAR(TM_Sched_PortDWRRBytesPerBurstsLimit)
	TM_REGISTER_VAR(TM_Sched_PortDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_PortDefPrioHi)
	TM_REGISTER_VAR(TM_Sched_PortDefPrioLo)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_PortExtBPEn)
	TM_REGISTER_VAR(TM_Sched_PortPerConf)
	TM_REGISTER_VAR(TM_Sched_PortPerRateShpPrms)
	TM_REGISTER_VAR(TM_Sched_PortPerRateShpPrmsInt)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosHi)
	TM_REGISTER_VAR(TM_Sched_PortQuantumsPriosLo)
	TM_REGISTER_VAR(TM_Sched_PortRangeMap)
	TM_REGISTER_VAR(TM_Sched_PortTokenBucketBurstSize)
	TM_REGISTER_VAR(TM_Sched_PortTokenBucketTokenEnDiv)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_PortShaperBucketNeg)
	TM_REGISTER_VAR(TM_Sched_PortBPFromQMgr)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortBPFromSTF)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortBankEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortMyQ)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortNodeState)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortPerStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortRRDWRRStatus01)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortRRDWRRStatus23)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortRRDWRRStatus45)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortRRDWRRStatus67)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortWFS)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_PortShpBucketLvls)
#endif
	TM_REGISTER_VAR(TM_Sched_QueueAMap)
	TM_REGISTER_VAR(TM_Sched_QueueDef)
	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)
	TM_REGISTER_VAR(TM_Sched_QueuePerConf)
	TM_REGISTER_VAR(TM_Sched_QueuePerRateShpPrms)
	TM_REGISTER_VAR(TM_Sched_QueuePerRateShpPrmsInt)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueQuantum)
	TM_REGISTER_VAR(TM_Sched_QueueTokenBucketBurstSize)
	TM_REGISTER_VAR(TM_Sched_QueueTokenBucketTokenEnDiv)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_QueueBank0EccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueBank1EccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueBank2EccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueBank3EccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL0ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL0ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL1ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL1ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL2ClusterStateHi)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueL2ClusterStateLo)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueNodeState)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueuePerStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueWFS)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_QueueShaperBucketNeg)
	TM_REGISTER_VAR(TM_Sched_QueueShpBucketLvls)
#endif
	TM_REGISTER_VAR(TM_Sched_EccConfig)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ExcMask)
	TM_REGISTER_VAR(TM_Sched_ScrubDisable)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ScrubSlotAlloc)
	TM_REGISTER_VAR(TM_Sched_TreeDWRRPrioEn)
	TM_REGISTER_VAR(TM_Sched_TreeDeqEn)
#if READ_ONLY
	TM_REGISTER_VAR(TM_Sched_EccMemParams)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_ErrCnt)
	TM_REGISTER_VAR(TM_Sched_ErrStus)
	TM_REGISTER_VAR(TM_Sched_ExcCnt)
	TM_REGISTER_VAR(TM_Sched_FirstExc)
	TM_REGISTER_VAR(TM_Sched_ForceErr)
	TM_REGISTER_VAR(TM_Sched_GeneralEccErrStatus)/*NEW*/
	TM_REGISTER_VAR(TM_Sched_Id)
	TM_REGISTER_VAR(TM_Sched_TreeRRDWRRStatus)/*NEW*/
#endif

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	/* Drop */
	/* A level */
	for (i = 0; i < TM_NUM_A_NODE_DROP_PROF; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfTailDrpThresh, i, TM_Drop_AlvlDropPrfTailDrpThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDDPRatio, i, TM_Drop_AlvlDropPrfWREDDPRatio)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDMinThresh, i, TM_Drop_AlvlDropPrfWREDMinThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDParams, i, TM_Drop_AlvlDropPrfWREDParams)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropPrfWREDScaleRatio, i, TM_Drop_AlvlDropPrfWREDScaleRatio)
		if (rc)
			goto out;
	}

	max_entries = get_tm_a_nodes_count()/TM_A_DRP_PROF_PER_ENTRY;
	for (i = 0; i < max_entries; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlDropProfPtr, i, TM_Drop_AlvlDropProfPtr)
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_WRED_A_NODE_CURVES; i++)
		/* TBD  __set_hw_queues_wred_curve(tm_handle hndl, uint8_t *prob_array, uint8_t curve_ind) */
		for (j = 0; j < 3; j++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.AlvlREDCurve.Color[j], i, TM_Drop_AlvlREDCurve_Color)
			if (rc)
				goto out;
		}

	/* B level */
	for (i = 0; i < TM_NUM_B_NODE_DROP_PROF; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfTailDrpThresh, i, TM_Drop_BlvlDropPrfTailDrpThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDDPRatio, i, TM_Drop_BlvlDropPrfWREDDPRatio)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDMinThresh, i, TM_Drop_BlvlDropPrfWREDMinThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDParams, i, TM_Drop_BlvlDropPrfWREDParams)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropPrfWREDScaleRatio, i, TM_Drop_BlvlDropPrfWREDScaleRatio)
		if (rc)
			goto out;
	}

	max_entries = get_tm_b_nodes_count()/TM_B_DRP_PROF_PER_ENTRY;
	for (i = 0; i < max_entries; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlDropProfPtr, i, TM_Drop_BlvlDropProfPtr)
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_WRED_B_NODE_CURVES; i++)
		for (j = 0; j < 4; j++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.BlvlREDCurve[j].Table, i, TM_Drop_BlvlREDCurve_Table)
			if (rc)
				goto out;
		}

	/* C level */
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		for (i = 0; i < TM_NUM_C_NODE_DROP_PROF; i++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfTailDrpThresh.CoS[cos], i,
				TM_Drop_ClvlDropPrfTailDrpThresh_CoS)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDDPRatio.CoS[cos], i,
				TM_Drop_ClvlDropPrfWREDDPRatio_CoS)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDMinThresh.CoS[cos], i,
				TM_Drop_ClvlDropPrfWREDMinThresh_CoS)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDParams.CoS[cos], i,
				TM_Drop_ClvlDropPrfWREDParams_CoS)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropPrfWREDScaleRatio.CoS[cos], i,
				TM_Drop_ClvlDropPrfWREDScaleRatio_CoS)
			if (rc)
				goto out;
		}

		max_entries = get_tm_c_nodes_count()/TM_C_DRP_PROF_PER_ENTRY;
		for (i = 0; i < max_entries; i++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlDropProfPtr_CoS[cos], i, TM_Drop_ClvlDropProfPtr_CoS)
			if (rc)
				goto out;
		}

		for (i = 0; i < TM_NUM_WRED_C_NODE_CURVES; i++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.ClvlREDCurve.CoS[cos], i, TM_Drop_ClvlREDCurve_CoS)
			if (rc)
				goto out;
		}
	}

	/* Port level */
	/* Global */
	for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfTailDrpThresh, i, TM_Drop_PortDropPrfTailDrpThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDDPRatio, i, TM_Drop_PortDropPrfWREDDPRatio)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDMinThresh, i, TM_Drop_PortDropPrfWREDMinThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDParams, i, TM_Drop_PortDropPrfWREDParams)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDScaleRatio, i, TM_Drop_PortDropPrfWREDScaleRatio)
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.PortREDCurve, i, TM_Drop_PortREDCurve)
		if (rc)
			goto out;
	}

	/* CoS */
	for (cos = 0; cos < TM_WRED_COS; cos++) {
		for (i = 0; i < TM_NUM_PORT_DROP_PROF; i++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfTailDrpThresh_CoSRes[cos], i,
				TM_Drop_PortDropPrfTailDrpThresh_CoSRes)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDDPRatio_CoSRes[cos], i,
				TM_Drop_PortDropPrfWREDDPRatio_CoSRes)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDMinThresh_CoSRes[cos], i,
				TM_Drop_PortDropPrfWREDMinThresh_CoSRes)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDParams_CoSRes[cos], i,
				TM_Drop_PortDropPrfWREDParams_CoSRes)
			if (rc)
				goto out;

			TM_WRITE_TABLE_REGISTER(TM.Drop.PortDropPrfWREDScaleRatio_CoSRes[cos], i,
				TM_Drop_PortDropPrfWREDScaleRatio_CoSRes)
			if (rc)
				goto out;
		}

		for (i = 0; i < TM_NUM_WRED_PORT_CURVES; i++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.PortREDCurve_CoS[cos], i, TM_Drop_PortREDCurve_CoS)
			if (rc)
				goto out;
		}
	}

	/* Queue level */
	max_entries = get_tm_queues_count()/4;
	for (i = 0; i < max_entries; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueCoSConf, i, TM_Drop_QueueCoSConf)
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_NUM_QUEUE_DROP_PROF; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfTailDrpThresh, i, TM_Drop_QueueDropPrfTailDrpThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDDPRatio, i, TM_Drop_QueueDropPrfWREDDPRatio)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDMinThresh, i, TM_Drop_QueueDropPrfWREDMinThresh)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDParams, i, TM_Drop_QueueDropPrfWREDParams)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropPrfWREDScaleRatio, i, TM_Drop_QueueDropPrfWREDScaleRatio)
		if (rc)
			goto out;
	}

	max_entries = get_tm_queues_count()/TM_Q_DRP_PROF_PER_ENTRY;
	for (i = 0; i < max_entries; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Drop.QueueDropProfPtr, i, TM_Drop_QueueDropProfPtr)
		if (rc)
			goto out;
	}
	for (i = 0; i < TM_NUM_WRED_QUEUE_CURVES; i++)
		for (j = 0; j < 3; j++) {
			TM_WRITE_TABLE_REGISTER(TM.Drop.QueueREDCurve.Color[j], i, TM_Drop_QueueREDCurve_Color)
			if (rc)
				goto out;
		}

	/* General */
	TM_WRITE_REGISTER(TM.Drop.DPSource, TM_Drop_DPSource)
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Drop.RespLocalDPSel, TM_Drop_RespLocalDPSel)
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Drop.WREDDropProbMode, TM_Drop_WREDDropProbMode)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Port, 0x2A)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Clvl, 0x2A)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Blvl, 0x2A)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Port, 0x2A)
	TM_REGISTER_SET(TM_Drop_WREDMaxProbModePerColor, Alvl, 0x2A)
	TM_WRITE_REGISTER(TM.Drop.WREDMaxProbModePerColor, TM_Drop_WREDMaxProbModePerColor)
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Drop.Drp_Decision_hierarchy_to_Query_debug,
		TM_Drop_Drp_Decision_hierarchy_to_Query_debug)/*NEW*/
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Drop.Drp_Decision_to_Query_debug, TM_Drop_Drp_Decision_to_Query_debug)/*NEW*/
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Drop.EccConfig, TM_Drop_EccConfig)/*NEW*/
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Drop_ExcMask, ForcedErr, 0x1)
	TM_REGISTER_SET(TM_Drop_ExcMask, CorrECCErr, 0x1)
	TM_REGISTER_SET(TM_Drop_ExcMask, UncECCErr, 0x1)
	TM_REGISTER_SET(TM_Drop_ExcMask, QuesryRespSyncFifoFull, 0x1)
	TM_REGISTER_SET(TM_Drop_ExcMask, QueryReqFifoOverflow, 0x1)
	TM_REGISTER_SET(TM_Drop_ExcMask, AgingFifoOverflow, 0x1)
	TM_WRITE_REGISTER(TM.Drop.ExcMask, TM_Drop_ExcMask)
	if (rc)
		goto out;


	/* Sched */
	/* A level */
	max_entries = get_tm_a_nodes_count();
	for (i = 0; i < max_entries; i++) {
		TM_REGISTER_SET(TM_Sched_AlvlDef, Deficit, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlDef, i, TM_Sched_AlvlDef)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlDWRRPrioEn, i, TM_Sched_AlvlDWRRPrioEn)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFuncPtr, i, TM_Sched_AlvlEligPrioFuncPtr)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlQuantum, Quantum, 0x40)
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlQuantum, i, TM_Sched_AlvlQuantum)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlTokenBucketBurstSize, MinBurstSz, 0xFFF)
		TM_REGISTER_SET(TM_Sched_AlvlTokenBucketBurstSize, MaxBurstSz, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlTokenBucketBurstSize, i, TM_Sched_AlvlTokenBucketBurstSize)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_AlvlTokenBucketTokenEnDiv, MinToken, 0xFFF)
		TM_REGISTER_SET(TM_Sched_AlvlTokenBucketTokenEnDiv, MaxToken, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlTokenBucketBurstSize, i, TM_Sched_AlvlTokenBucketTokenEnDiv)
		if (rc)
			goto out;

		/* TBD */
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo, i*4)
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi, i*4 + 3)
		TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, Blvl, i/4)
		TM_WRITE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, TM_Sched_ALvltoBlvlAndQueueRangeMap)
		if (rc)
			goto out;
	}

	for (i = 0; i < 512; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.AlvlEligPrioFunc, i, TM_Sched_AlvlEligPrioFunc)
		if (rc)
			goto out;
	}

	TM_REGISTER_SET(TM_Sched_AlvlPerConf, PerInterval, 0xe39)
	TM_WRITE_REGISTER(TM.Sched.AlvlPerConf, TM_Sched_AlvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, N, 0x2)
	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, K, 0x66F)
	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrms, L, 0x16E)
	TM_WRITE_REGISTER(TM.Sched.AlvlPerRateShpPrms, TM_Sched_AlvlPerRateShpPrms)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrmsInt, B, 0x1)
	TM_REGISTER_SET(TM_Sched_AlvlPerRateShpPrmsInt, I, 0x1)
	TM_WRITE_REGISTER(TM.Sched.AlvlPerRateShpPrmsInt, TM_Sched_AlvlPerRateShpPrmsInt)/*NEW*/
	if (rc)
		goto out;


	/* B level */
	max_entries = get_tm_b_nodes_count();
	for (i = 0; i < max_entries; i++) {
		TM_REGISTER_SET(TM_Sched_BlvlDef, Deficit, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlDef, i, TM_Sched_BlvlDef)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlDWRRPrioEn, i, TM_Sched_BlvlDWRRPrioEn)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFuncPtr, i, TM_Sched_BlvlEligPrioFuncPtr)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlQuantum, Quantum, 0x40)
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlQuantum, i, TM_Sched_BlvlQuantum)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlTokenBucketBurstSize, MinBurstSz, 0xFFF)
		TM_REGISTER_SET(TM_Sched_BlvlTokenBucketBurstSize, MaxBurstSz, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlTokenBucketBurstSize, i, TM_Sched_BlvlTokenBucketBurstSize)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_BlvlTokenBucketTokenEnDiv, MinToken, 0xFFF)
		TM_REGISTER_SET(TM_Sched_BlvlTokenBucketTokenEnDiv, MaxToken, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlTokenBucketTokenEnDiv, i, TM_Sched_BlvlTokenBucketTokenEnDiv)
		if (rc)
			goto out;

		/* TBD */
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlLo, i*4)
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlHi, i*4 + 3)
		TM_REGISTER_SET(TM_Sched_BLvltoClvlAndAlvlRangeMap, Clvl, i/2)
		TM_WRITE_REGISTER(TM.Sched.BLvltoClvlAndAlvlRangeMap, TM_Sched_BLvltoClvlAndAlvlRangeMap)
		if (rc)
			goto out;
	}

	for (i = 0; i < 512; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.BlvlEligPrioFunc, i, TM_Sched_BlvlEligPrioFunc)
		if (rc)
			goto out;
	}

	TM_REGISTER_SET(TM_Sched_BlvlPerConf, PerInterval, 0xe39)
	TM_WRITE_REGISTER(TM.Sched.BlvlPerConf, TM_Sched_BlvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, N, 0x2)
	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, K, 0x66F)
	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrms, L, 0x16E)
	TM_WRITE_REGISTER(TM.Sched.BlvlPerRateShpPrms, TM_Sched_BlvlPerRateShpPrms)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrmsInt, B, 0x1)
	TM_REGISTER_SET(TM_Sched_BlvlPerRateShpPrmsInt, I, 0x1)
	TM_WRITE_REGISTER(TM.Sched.BlvlPerRateShpPrmsInt, TM_Sched_BlvlPerRateShpPrmsInt)/*NEW*/
	if (rc)
		goto out;


	/* C level */
	max_entries = get_tm_c_nodes_count();
	for (i = 0; i < max_entries; i++) {
		TM_REGISTER_SET(TM_Sched_CLvlDef, Deficit, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.CLvlDef, i, TM_Sched_CLvlDef)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlDWRRPrioEn, i, TM_Sched_ClvlDWRRPrioEn)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFuncPtr, i, TM_Sched_ClvlEligPrioFuncPtr)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlQuantum, Quantum, 0x40)
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlQuantum, i, TM_Sched_ClvlQuantum)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlTokenBucketBurstSize, MinBurstSz, 0xFFF)
		TM_REGISTER_SET(TM_Sched_ClvlTokenBucketBurstSize, MaxBurstSz, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlTokenBucketBurstSize, i, TM_Sched_ClvlTokenBucketBurstSize)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_ClvlTokenBucketTokenEnDiv, MinToken, 0xFFF)
		TM_REGISTER_SET(TM_Sched_ClvlTokenBucketTokenEnDiv, MaxToken, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlTokenBucketTokenEnDiv, i, TM_Sched_ClvlTokenBucketTokenEnDiv)
		if (rc)
			goto out;

		/* TBD */
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlLo, i*2)
		TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlHi, i*2 + 1)
		if (i < 4)
			TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, 0)
		else
			if (i > 13)
				TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, i)
			else
				TM_REGISTER_SET(TM_Sched_ClvltoPortAndBlvlRangeMap, Port, i+3)
		TM_WRITE_REGISTER(TM.Sched.ClvltoPortAndBlvlRangeMap, TM_Sched_ClvltoPortAndBlvlRangeMap)
		if (rc)
			goto out;
	}

	for (i = 0; i < 512; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.ClvlEligPrioFunc, i, TM_Sched_ClvlEligPrioFunc)
		if (rc)
			goto out;
	}

	TM_REGISTER_SET(TM_Sched_ClvlPerConf, PerInterval, 0x71c)
	TM_WRITE_REGISTER(TM.Sched.ClvlPerConf, TM_Sched_ClvlPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, N, 0x2)
	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, K, 0x66F)
	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrms, L, 0x16E)
	TM_WRITE_REGISTER(TM.Sched.ClvlPerRateShpPrms, TM_Sched_ClvlPerRateShpPrms)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrmsInt, B, 0x1)
	TM_REGISTER_SET(TM_Sched_ClvlPerRateShpPrmsInt, I, 0x1)
	TM_WRITE_REGISTER(TM.Sched.ClvlPerRateShpPrmsInt, TM_Sched_ClvlPerRateShpPrmsInt)/*NEW*/
	if (rc)
		goto out;


	/* Port level */
	max_entries = get_tm_port_count();
	for (i = 0; i < max_entries; i++) {
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit0, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit1, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit2, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioHi, Deficit3, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDefPrioHi, i, TM_Sched_PortDefPrioHi)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit0, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit1, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit2, 0x1)
		TM_REGISTER_SET(TM_Sched_PortDefPrioLo, Deficit3, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDefPrioLo, i, TM_Sched_PortDefPrioLo)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.PortDWRRPrioEn, i, TM_Sched_PortDWRRPrioEn)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFuncPtr, i, TM_Sched_PortEligPrioFuncPtr)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum0, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum1, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum2, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosLo, Quantum3, 0x10)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortQuantumsPriosLo, i, TM_Sched_PortQuantumsPriosLo)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum4, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum5, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum6, 0x10)
		TM_REGISTER_SET(TM_Sched_PortQuantumsPriosHi, Quantum7, 0x10)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortQuantumsPriosHi, i, TM_Sched_PortQuantumsPriosHi)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortTokenBucketBurstSize, MinBurstSz, 0x1FFFF)
		TM_REGISTER_SET(TM_Sched_PortTokenBucketBurstSize, MaxBurstSz, 0x1FFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortTokenBucketBurstSize, i, TM_Sched_PortTokenBucketBurstSize)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, Periods, 0x1)
		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, MinToken, 0xFFF)
		TM_REGISTER_SET(TM_Sched_PortTokenBucketTokenEnDiv, MaxToken, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortTokenBucketTokenEnDiv, i, TM_Sched_PortTokenBucketTokenEnDiv)
		if (rc)
			goto out;

		/* TBD */
		TM_REGISTER_SET(TM_Sched_PortRangeMap, Lo, i*2)
		TM_REGISTER_SET(TM_Sched_PortRangeMap, Hi, i*2 + 1)
		if (i == 0) {
			TM_REGISTER_SET(TM_Sched_PortRangeMap, Lo, 0)
			TM_REGISTER_SET(TM_Sched_PortRangeMap, Hi, 3)
		} else {
			if (i < 11) {
				TM_REGISTER_SET(TM_Sched_PortRangeMap, Lo, i+3)
				TM_REGISTER_SET(TM_Sched_PortRangeMap, Hi, i+3)
			} else if (i > 13) {
					TM_REGISTER_SET(TM_Sched_PortRangeMap, Lo, i)
					TM_REGISTER_SET(TM_Sched_PortRangeMap, Hi, i)
				}
		}
		TM_WRITE_REGISTER(TM.Sched.PortRangeMap, TM_Sched_PortRangeMap)
		if (rc)
			goto out;
	}

	for (i = 0; i < 512; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.PortEligPrioFunc, i, TM_Sched_PortEligPrioFunc)
		if (rc)
			goto out;
	}

	TM_REGISTER_SET(TM_Sched_PortDWRRBytesPerBurstsLimit, limit, 0x20)
	TM_WRITE_REGISTER(TM.Sched.PortDWRRBytesPerBurstsLimit, TM_Sched_PortDWRRBytesPerBurstsLimit)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_PortPerConf, PerInterval, 0x50)
	TM_WRITE_REGISTER(TM.Sched.PortPerConf, TM_Sched_PortPerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_PortExtBPEn, En, 0x1)
	TM_WRITE_REGISTER(TM.Sched.PortExtBPEn, TM_Sched_PortExtBPEn);
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, N, 0x2)
	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, K, 0x66F)
	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrms, L, 0x16E)
	TM_WRITE_REGISTER(TM.Sched.PortPerRateShpPrms, TM_Sched_PortPerRateShpPrms)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrmsInt, B, 0x1)
	TM_REGISTER_SET(TM_Sched_PortPerRateShpPrmsInt, I, 0x1)
	TM_WRITE_REGISTER(TM.Sched.PortPerRateShpPrmsInt, TM_Sched_PortPerRateShpPrmsInt)/*NEW*/
	if (rc)
		goto out;

	/* Queue level */
	max_entries = get_tm_queues_count();
	for (i = 0; i < max_entries; i++) {
		TM_REGISTER_SET(TM_Sched_QueueDef, Deficit, 0x1)
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueDef, i, TM_Sched_QueueDef)
		if (rc)
			goto out;

		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, i, TM_Sched_QueueEligPrioFuncPtr)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_QueueQuantum, Quantum, 0x40)
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueQuantum, i, TM_Sched_QueueQuantum)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_QueueTokenBucketBurstSize, MinBurstSz, 0xFFF)
		TM_REGISTER_SET(TM_Sched_QueueTokenBucketBurstSize, MaxBurstSz, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueTokenBucketBurstSize, i, TM_Sched_QueueTokenBucketBurstSize)
		if (rc)
			goto out;

		TM_REGISTER_SET(TM_Sched_QueueTokenBucketTokenEnDiv, MinToken, 0xFFF)
		TM_REGISTER_SET(TM_Sched_QueueTokenBucketTokenEnDiv, MaxToken, 0xFFF)
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueTokenBucketTokenEnDiv, i, TM_Sched_QueueTokenBucketTokenEnDiv)
		if (rc)
			goto out;

		/* TBD */
		TM_REGISTER_SET(TM_Sched_QueueAMap, Alvl, i/4)
		TM_WRITE_REGISTER(TM.Sched.QueueAMap, TM_Sched_QueueAMap)
		if (rc)
			goto out;
	}

	for (i = 0; i < TM_ELIG_FUNC_TABLE_SIZE; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFunc, i, TM_Sched_QueueEligPrioFunc)
		if (rc)
			goto out;
	}

	TM_REGISTER_SET(TM_Sched_QueuePerConf, PerInterval, 0xe39)
	TM_WRITE_REGISTER(TM.Sched.QueuePerConf, TM_Sched_QueuePerConf)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, N, 0x2)
	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, K, 0x66F)
	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrms, L, 0x16E)
	TM_WRITE_REGISTER(TM.Sched.QueuePerRateShpPrms, TM_Sched_QueuePerRateShpPrms)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrmsInt, B, 0x1)
	TM_REGISTER_SET(TM_Sched_QueuePerRateShpPrmsInt, I, 0x1)
	TM_WRITE_REGISTER(TM.Sched.QueuePerRateShpPrmsInt, TM_Sched_QueuePerRateShpPrmsInt)/*NEW*/
	if (rc)
		goto out;


	/* General */
	TM_WRITE_REGISTER(TM.Sched.ScrubDisable, TM_Sched_ScrubDisable)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_ScrubSlotAlloc, QueueSlots, 0x20)
	TM_REGISTER_SET(TM_Sched_ScrubSlotAlloc, AlvlSlots, 0x10)
	TM_REGISTER_SET(TM_Sched_ScrubSlotAlloc, BlvlSlots, 0x8)
	TM_REGISTER_SET(TM_Sched_ScrubSlotAlloc, ClvlSlots, 0x4)
	TM_REGISTER_SET(TM_Sched_ScrubSlotAlloc, PortSlots, 0x4)
	TM_WRITE_REGISTER(TM.Sched.ScrubSlotAlloc, TM_Sched_ScrubSlotAlloc)
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Sched.TreeDWRRPrioEn, TM_Sched_TreeDWRRPrioEn)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_TreeDeqEn, En, 0x1)
	TM_WRITE_REGISTER(TM.Sched.TreeDeqEn, TM_Sched_TreeDeqEn)
	if (rc)
		goto out;

	TM_WRITE_REGISTER(TM.Sched.EccConfig, TM_Sched_EccConfig)
	if (rc)
		goto out;

	TM_REGISTER_SET(TM_Sched_ExcMask, ForcedErr, 0x1)
	TM_REGISTER_SET(TM_Sched_ExcMask, CorrECCErr, 0x1)
	TM_REGISTER_SET(TM_Sched_ExcMask, UncECCErr, 0x1)
	TM_REGISTER_SET(TM_Sched_ExcMask, BPBSat, 0x0)
	TM_REGISTER_SET(TM_Sched_ExcMask, TBNegSat, 0x1)
	TM_REGISTER_SET(TM_Sched_ExcMask, FIFOOvrflowErr, 0x1)
	TM_WRITE_REGISTER(TM.Sched.ExcMask, TM_Sched_ExcMask)
out:
	COMPLETE_HW_WRITE
	return rc;
}

/**
 */
int get_hw_port_status(tm_handle hndl,
					uint8_t index,
					struct tm_port_status *tm_status)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_PortShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_PortDefPrioHi)
	TM_REGISTER_VAR(TM_Sched_PortDefPrioLo)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_TABLE_REGISTER(TM.Sched.PortShpBucketLvls, index, TM_Sched_PortShpBucketLvls);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_PortShpBucketLvls, MinLvl, tm_status->min_bucket_level, (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortShpBucketLvls, MaxLvl, tm_status->max_bucket_level, (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.PortDefPrioHi, index, TM_Sched_PortDefPrioHi);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_PortDefPrioHi, Deficit0, tm_status->deficit[0], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioHi, Deficit1, tm_status->deficit[1], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioHi, Deficit2, tm_status->deficit[2], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioHi, Deficit3, tm_status->deficit[3], (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.PortDefPrioLo, index, TM_Sched_PortDefPrioLo);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_PortDefPrioLo, Deficit0, tm_status->deficit[4], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioLo, Deficit1, tm_status->deficit[5], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioLo, Deficit2, tm_status->deficit[6], (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortDefPrioLo, Deficit3, tm_status->deficit[7], (uint32_t));
	return rc;
}


/**
 */
int get_hw_c_node_status(tm_handle hndl,
					uint32_t index,
					struct tm_node_status *tm_status)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_ClvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_CLvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_TABLE_REGISTER(TM.Sched.ClvlShpBucketLvls, index, TM_Sched_ClvlShpBucketLvls);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_ClvlShpBucketLvls, MinLvl, tm_status->min_bucket_level, (uint32_t));
	TM_REGISTER_GET(TM_Sched_ClvlShpBucketLvls, MaxLvl, tm_status->max_bucket_level, (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.CLvlDef, index, TM_Sched_CLvlDef);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_CLvlDef, Deficit, tm_status->deficit, (uint32_t));
	return rc;
}


/**
 */
int get_hw_b_node_status(tm_handle hndl,
					uint32_t index,
					struct tm_node_status *tm_status)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_BlvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_BlvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_TABLE_REGISTER(TM.Sched.BlvlShpBucketLvls, index, TM_Sched_BlvlShpBucketLvls);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_BlvlShpBucketLvls, MinLvl, tm_status->min_bucket_level, (uint32_t));
	TM_REGISTER_GET(TM_Sched_BlvlShpBucketLvls, MaxLvl, tm_status->max_bucket_level, (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.BlvlDef, index, TM_Sched_BlvlDef);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_BlvlDef, Deficit, tm_status->deficit, (uint32_t));
	return rc;
}


/**
 */
int get_hw_a_node_status(tm_handle hndl,
					uint32_t index,
					struct tm_node_status *tm_status)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_AlvlShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_AlvlDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_TABLE_REGISTER(TM.Sched.AlvlShpBucketLvls, index, TM_Sched_AlvlShpBucketLvls);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_AlvlShpBucketLvls, MinLvl, tm_status->min_bucket_level, (uint32_t));
	TM_REGISTER_GET(TM_Sched_AlvlShpBucketLvls, MaxLvl, tm_status->max_bucket_level, (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.AlvlDef, index, TM_Sched_AlvlDef);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_AlvlDef, Deficit, tm_status->deficit, (uint32_t));
	return rc;
}


/**
 */
int get_hw_queue_status(tm_handle hndl,
					uint32_t index,
					struct tm_node_status *tm_status)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_QueueShpBucketLvls)
	TM_REGISTER_VAR(TM_Sched_QueueDef)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_TABLE_REGISTER(TM.Sched.QueueShpBucketLvls, index, TM_Sched_QueueShpBucketLvls);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_QueueShpBucketLvls, MinLvl, tm_status->min_bucket_level, (uint32_t));
	TM_REGISTER_GET(TM_Sched_QueueShpBucketLvls, MaxLvl, tm_status->max_bucket_level, (uint32_t));

	TM_READ_TABLE_REGISTER(TM.Sched.QueueDef, index, TM_Sched_QueueDef);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_QueueDef, Deficit, tm_status->deficit, (uint32_t));
	return rc;
}


/**
 */
int get_hw_queue_length(tm_handle hndl,
					enum tm_level level,
					uint32_t index,
					uint32_t *av_q_length)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_QueueAvgQueueLength)		/* average */
	TM_REGISTER_VAR(TM_Drop_AlvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_BlvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_ClvlInstAndAvgQueueLength)
	TM_REGISTER_VAR(TM_Drop_PortInstAndAvgQueueLength)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (level) {
	case Q_LEVEL:
		TM_READ_TABLE_REGISTER(TM.Drop.QueueAvgQueueLength, index, TM_Drop_QueueAvgQueueLength)
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Drop_QueueAvgQueueLength, AQL, *av_q_length, (uint32_t)); /* casting to uint32_t */
		break;
	case A_LEVEL:
		TM_READ_TABLE_REGISTER(TM.Drop.AlvlInstAndAvgQueueLength, index, TM_Drop_AlvlInstAndAvgQueueLength)
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Drop_AlvlInstAndAvgQueueLength, AQL, *av_q_length, (uint32_t));
		break;
	case B_LEVEL:
		TM_READ_TABLE_REGISTER(TM.Drop.BlvlInstAndAvgQueueLength, index, TM_Drop_BlvlInstAndAvgQueueLength)
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Drop_BlvlInstAndAvgQueueLength, AQL, *av_q_length, (uint32_t));
		break;
	case C_LEVEL:
		TM_READ_TABLE_REGISTER(TM.Drop.ClvlInstAndAvgQueueLength, index, TM_Drop_ClvlInstAndAvgQueueLength)
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Drop_ClvlInstAndAvgQueueLength, AQL, *av_q_length, (uint32_t));
		break;
	case P_LEVEL:
		TM_READ_TABLE_REGISTER(TM.Drop.PortInstAndAvgQueueLength, index, TM_Drop_PortInstAndAvgQueueLength)
		if (rc)
			return rc;
		TM_REGISTER_GET(TM_Drop_PortInstAndAvgQueueLength, AQL, *av_q_length, (uint32_t));
		break;
	}
	return rc;
}


/**
 */
int get_hw_sched_errors(tm_handle hndl, struct tm_error_info *info)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_ErrCnt)
	TM_REGISTER_VAR(TM_Sched_ExcCnt)


	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_REGISTER(TM.Sched.ErrCnt, TM_Sched_ErrCnt);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_ErrCnt, Cnt, info->error_counter, (uint16_t)); /* casting to uint16_t */

	TM_READ_REGISTER(TM.Sched.ExcCnt, TM_Sched_ExcCnt);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Sched_ExcCnt, Cnt, info->exception_counter, (uint16_t)); /* casting to uint16_t */

	return rc;
}


/**
 */
int get_hw_drop_errors(tm_handle hndl, struct tm_error_info *info)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Drop_ErrCnt)
	TM_REGISTER_VAR(TM_Drop_ExcCnt)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_READ_REGISTER(TM.Drop.ErrCnt, TM_Drop_ErrCnt);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Drop_ErrCnt, Cnt, info->error_counter, (uint16_t)); /* casting to uint16_t */

	TM_READ_REGISTER(TM.Drop.ExcCnt, TM_Drop_ExcCnt);
	if (rc)
		return rc;
	TM_REGISTER_GET(TM_Drop_ExcCnt, Cnt, info->exception_counter, (uint16_t)); /* casting to uint16_t */
	return rc;
}


#define READ_ELIG_PRIO_FUN_TABLE_MAC(level)	\
do {\
	for (i = 0; i < 8; i++) {\
		TM_READ_TABLE_REGISTER(TM.Sched.level##EligPrioFunc,\
								func_offset + (i * 64),\
								TM_Sched_##level##EligPrioFunc);\
		if (rc)\
			goto out;\
		TM_REGISTER_GET(TM_Sched_##level##EligPrioFunc, FuncOut0, table[i*4], (uint16_t));\
		TM_REGISTER_GET(TM_Sched_##level##EligPrioFunc, FuncOut1, table[i*4+1], (uint16_t));\
		TM_REGISTER_GET(TM_Sched_##level##EligPrioFunc, FuncOut2, table[i*4+2], (uint16_t));\
		TM_REGISTER_GET(TM_Sched_##level##EligPrioFunc, FuncOut3, table[i*4+3], (uint16_t));\
	} \
} while (0)

/**
 * Dump Eligible Priority Function
 */
int get_hw_elig_prio_func(tm_handle hndl, enum tm_level level, uint16_t func_offset, uint16_t *table)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFunc)

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFunc)


	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (level) {
	case Q_LEVEL:
		rc = -EFAULT;
		TM_READ_TABLE_REGISTER(TM.Sched.QueueEligPrioFunc, func_offset, TM_Sched_QueueEligPrioFunc)
		if (rc)
			goto out;
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut0, table[0], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut1, table[1], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut2, table[2], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut3, table[3], (uint16_t));
		goto out;
		break;
	case A_LEVEL:
		READ_ELIG_PRIO_FUN_TABLE_MAC(Alvl);
		break;
	case B_LEVEL:
		READ_ELIG_PRIO_FUN_TABLE_MAC(Blvl);
		break;
	case C_LEVEL:
		READ_ELIG_PRIO_FUN_TABLE_MAC(Clvl);
		break;
	case P_LEVEL:
		READ_ELIG_PRIO_FUN_TABLE_MAC(Port);
		break;
	default:
		goto out;
	}
out:
	COMPLETE_HW_WRITE
	return rc;
}


int show_hw_elig_prio_func(tm_handle hndl, enum tm_level level, uint16_t func_offset)
{
	int rc = -EFAULT;
	int i, j;
	int entry_index;
	int prop_prio;
	uint8_t mask = 0x07;
	uint16_t elig_func_table[32];

	rc = get_hw_elig_prio_func(hndl, level, func_offset, elig_func_table);
	if (rc)
		return rc;
	pr_info("Function: %d\n", func_offset);
	if (level == Q_LEVEL) {
		pr_info("%02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x\n",
			0, elig_func_table[0],
			1, elig_func_table[1],
			2, elig_func_table[2],
			3, elig_func_table[3]);
		/* in - MinTBNeg, MaxTBNeg, PropPrio */
		/* out - Elig, SchdPrio, PropPrio, MinTBUsed, MaxTBUsed */
		pr_info("  Detailed table:\n");
		pr_info("    Input       OutPut            Value\n");
		pr_info("    -----    -----------------    -----\n");
		pr_info("    M   M    E   S   P   M   M\n");
		pr_info("    i   a    l   c   r   i   a\n");
		pr_info("    n   x    i   h   o   n   x\n");
		pr_info("    T   T    g   d   p   T   T\n");
		pr_info("    B   B        P   P   B   B\n");
		pr_info("    N   N        r   r   U   U\n");
		pr_info("    e   e        i   i   s   s\n");
		pr_info("    g   g        o   o   e   e\n");
		pr_info("    -   -    -   -   -   -   -\n");
		for (j = 0; j < 4; j++) {
			entry_index =  j;
			pr_info("%02d: %d   %d    %d   %d   %d   %d   %d    0x%03x\n",
				entry_index,
				entry_index / 2,
				entry_index % 2,
				(elig_func_table[j] >> 8) & 0x01,		/*elig*/
				(elig_func_table[j] >> 2) & mask,		/*sched_prio*/
				(elig_func_table[j] >> 5) & mask,		/*prop_prio*/
				(elig_func_table[j] >> 1) & 0x01,		/*min_tb*/
				elig_func_table[j] & 0x01,			/*max_tb*/
				elig_func_table[j]
				);
		}
	} else {
		for (i = 0; i < 8; i++) {
			pr_info("%02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x\n",
				(i*4) + 0, elig_func_table[i*4 + 0],
				(i*4) + 1, elig_func_table[i*4 + 1],
				(i*4) + 2, elig_func_table[i*4 + 2],
				(i*4) + 3, elig_func_table[i*4 + 3]);
		}

		/* in - MinTBNeg, MaxTBNeg, PropPrio */
		/* out - Elig, SchdPrio, PropPrio, MinTBUsed, MaxTBUsed */
		pr_info("  Detailed table:\n");
		pr_info("      Input            OutPut         Value\n");
		pr_info("    ---------     -----------------   -----\n");
		pr_info("    M   M   P     E   S   P   M   M\n");
		pr_info("    i   a   r     l   c   r   i   a\n");
		pr_info("    n   x   o     i   h   o   n   x\n");
		pr_info("    T   T   p     g   d   p   T   T\n");
		pr_info("    B   B   P         P   P   B   B\n");
		pr_info("    N   N   r         r   r   U   U\n");
		pr_info("    e   e   i         i   i   s   s\n");
		pr_info("    g   g   o         o   o   e   e\n");
		pr_info("    -   -   -     -   -   -   -   -\n");

		for (i = 0; i < 8; i++) {
			for (j = 0; j < 4; j++) {
				entry_index = i*4 + j;
				prop_prio = entry_index & 7;
				pr_info("%02d: %d   %d   %d     %d   %d   %d   %d   %d   0x%03x\n",
						entry_index,
						entry_index / 16,
						(entry_index / 8) % 2,
						prop_prio,
						(elig_func_table[entry_index] >> 8) & 0x01,		/*elig*/
						(elig_func_table[entry_index] >> 2) & mask,		/*sched_prio*/
						(elig_func_table[entry_index] >> 5) & mask,		/*prop_prio*/
						(elig_func_table[entry_index] >> 1) & 0x01,		/*min_tb*/
						elig_func_table[entry_index] & 0x01,			/*max_tb*/
						elig_func_table[entry_index]
						);
			}
		}
	}
	pr_info("---------------------------------------\n");


#if 0
	TM_REGISTER_VAR(TM_Sched_AlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_BlvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_ClvlEligPrioFunc)
	TM_REGISTER_VAR(TM_Sched_PortEligPrioFunc)

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFunc)


	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	switch (level) {
	case Q_LEVEL:
		rc = -EFAULT;
		TM_READ_TABLE_REGISTER(TM.Sched.QueueEligPrioFunc, func_offset, TM_Sched_QueueEligPrioFunc)
		if (rc)
			goto out;
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut0, elig_func_val[0], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut1, elig_func_val[1], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut2, elig_func_val[2], (uint16_t));
		TM_REGISTER_GET(TM_Sched_QueueEligPrioFunc, FuncOut3, elig_func_val[3], (uint16_t));
		pr_info("Function: %d\n", func_offset);
		pr_info("%02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x\n",
			0, elig_func_val[0],
			1, elig_func_val[1],
			2, elig_func_val[2],
			3, elig_func_val[3]);
		/* in - MinTBNeg, MaxTBNeg, PropPrio */
		/* out - Elig, SchdPrio, PropPrio, MinTBUsed, MaxTBUsed */
		pr_info("  Detailed table:\n");
		pr_info("    Input       OutPut            Value\n");
		pr_info("    -----    -----------------    -----\n");
		pr_info("    M   M    E   S   P   M   M\n");
		pr_info("    i   a    l   c   r   i   a\n");
		pr_info("    n   x    i   h   o   n   x\n");
		pr_info("    T   T    g   d   p   T   T\n");
		pr_info("    B   B        P   P   B   B\n");
		pr_info("    N   N        r   r   U   U\n");
		pr_info("    e   e        i   i   s   s\n");
		pr_info("    g   g        o   o   e   e\n");
		pr_info("    -   -    -   -   -   -   -\n");
		for (j = 0; j < 4; j++) {
			entry_index =  j;
			pr_info("%02d: %d   %d    %d   %d   %d   %d   %d    0x%03x\n",
				entry_index,
				entry_index / 2,
				entry_index % 2,
				(elig_func_val[j] >> 8) & 0x01,		/*elig*/
				(elig_func_val[j] >> 2) & mask,		/*sched_prio*/
				(elig_func_val[j] >> 5) & mask,		/*prop_prio*/
				(elig_func_val[j] >> 1) & 0x01,		/*min_tb*/
				elig_func_val[j] & 0x01,			/*max_tb*/
				elig_func_val[j]
				);
		}
		pr_info("---------------------------------------\n");
		goto out;
		break;
	case A_LEVEL:
		for (i = 0; i < 8; i++) {
			rc = tm_table_entry_read(TM_ENV(ctl), (void *)&(TM.Sched.AlvlEligPrioFunc),
				func_offset + (i * 64), TM_REGISTER_VAR_ADDR(TM_Sched_AlvlEligPrioFunc));
			if (rc)
				goto out;

			TM_REGISTER_GET(TM_Sched_AlvlEligPrioFunc, FuncOut0,
				params->tbl_entry[i].func_out0, (uint16_t));
			TM_REGISTER_GET(TM_Sched_AlvlEligPrioFunc, FuncOut1,
				params->tbl_entry[i].func_out1, (uint16_t));
			TM_REGISTER_GET(TM_Sched_AlvlEligPrioFunc, FuncOut2,
				params->tbl_entry[i].func_out2, (uint16_t));
			TM_REGISTER_GET(TM_Sched_AlvlEligPrioFunc, FuncOut3,
				params->tbl_entry[i].func_out3, (uint16_t));
		}
		break;
	case B_LEVEL:
		for (i = 0; i < 8; i++) {
			rc = tm_table_entry_read(TM_ENV(ctl), (void *)&(TM.Sched.BlvlEligPrioFunc),
				func_offset + (i * 64), TM_REGISTER_VAR_ADDR(TM_Sched_BlvlEligPrioFunc));

			TM_REGISTER_GET(TM_Sched_BlvlEligPrioFunc, FuncOut0,
				params->tbl_entry[i].func_out0, (uint16_t));
			TM_REGISTER_GET(TM_Sched_BlvlEligPrioFunc, FuncOut1,
				params->tbl_entry[i].func_out1, (uint16_t));
			TM_REGISTER_GET(TM_Sched_BlvlEligPrioFunc, FuncOut2,
				params->tbl_entry[i].func_out2, (uint16_t));
			TM_REGISTER_GET(TM_Sched_BlvlEligPrioFunc, FuncOut3,
				params->tbl_entry[i].func_out3, (uint16_t));
		}
		break;
	case C_LEVEL:
		for (i = 0; i < 8; i++) {
			rc = tm_table_entry_read(TM_ENV(ctl), (void *)&(TM.Sched.ClvlEligPrioFunc),
				func_offset + (i * 64), TM_REGISTER_VAR_ADDR(TM_Sched_ClvlEligPrioFunc));

			TM_REGISTER_GET(TM_Sched_ClvlEligPrioFunc, FuncOut0,
				params->tbl_entry[i].func_out0, (uint16_t));
			TM_REGISTER_GET(TM_Sched_ClvlEligPrioFunc, FuncOut1,
				params->tbl_entry[i].func_out1, (uint16_t));
			TM_REGISTER_GET(TM_Sched_ClvlEligPrioFunc, FuncOut2,
				params->tbl_entry[i].func_out2, (uint16_t));
			TM_REGISTER_GET(TM_Sched_ClvlEligPrioFunc, FuncOut3,
				params->tbl_entry[i].func_out3, (uint16_t));
		}
		break;
	case P_LEVEL:
		for (i = 0; i < 8; i++) {
			rc = tm_table_entry_read(TM_ENV(ctl), (void *)&(TM.Sched.PortEligPrioFunc),
				func_offset + (i * 64), TM_REGISTER_VAR_ADDR(TM_Sched_PortEligPrioFunc));

			TM_REGISTER_GET(TM_Sched_PortEligPrioFunc, FuncOut0,
				params->tbl_entry[i].func_out0, (uint16_t));
			TM_REGISTER_GET(TM_Sched_PortEligPrioFunc, FuncOut1,
				params->tbl_entry[i].func_out1, (uint16_t));
			TM_REGISTER_GET(TM_Sched_PortEligPrioFunc, FuncOut2,
				params->tbl_entry[i].func_out2, (uint16_t));
			TM_REGISTER_GET(TM_Sched_PortEligPrioFunc, FuncOut3,
				params->tbl_entry[i].func_out3, (uint16_t));
		}
		break;
	default:
		goto out;
	}

	pr_info("Function: %d\n", func_offset);

	for (i = 0; i < 8; i++) {
		elig_func_val[0] = params->tbl_entry[i].func_out0;
		elig_func_val[1] = params->tbl_entry[i].func_out1;
		elig_func_val[2] = params->tbl_entry[i].func_out2;
		elig_func_val[3] = params->tbl_entry[i].func_out3;

		pr_info("%02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x  %02d: 0x%03x\n",
			(i*4), elig_func_val[0],
			(i*4) + 1, elig_func_val[1],
			(i*4) + 2, elig_func_val[2],
			(i*4) + 3, elig_func_val[3]);
	}

	/* in - MinTBNeg, MaxTBNeg, PropPrio */
	/* out - Elig, SchdPrio, PropPrio, MinTBUsed, MaxTBUsed */
	pr_info("  Detailed table:\n");
	pr_info("      Input            OutPut         Value\n");
	pr_info("    ---------     -----------------   -----\n");
	pr_info("    M   M   P     E   S   P   M   M\n");
	pr_info("    i   a   r     l   c   r   i   a\n");
	pr_info("    n   x   o     i   h   o   n   x\n");
	pr_info("    T   T   p     g   d   p   T   T\n");
	pr_info("    B   B   P         P   P   B   B\n");
	pr_info("    N   N   r         r   r   U   U\n");
	pr_info("    e   e   i         i   i   s   s\n");
	pr_info("    g   g   o         o   o   e   e\n");
	pr_info("    -   -   -     -   -   -   -   -\n");

	for (i = 0; i < 8; i++) {
		elig_func_val[0] = params->tbl_entry[i].func_out0;
		elig_func_val[1] = params->tbl_entry[i].func_out1;
		elig_func_val[2] = params->tbl_entry[i].func_out2;
		elig_func_val[3] = params->tbl_entry[i].func_out3;

		for (j = 0; j < 4; j++) {
			entry_index = i*4 + j;
			prop_prio = entry_index & 7;
			pr_info("%02d: %d   %d   %d     %d   %d   %d   %d   %d   0x%03x\n",
					entry_index,
					entry_index / 16,
					(entry_index / 8) % 2,
					prop_prio,
					(elig_func_val[j] >> 8) & 0x01,		/*elig*/
					(elig_func_val[j] >> 2) & mask,		/*sched_prio*/
					(elig_func_val[j] >> 5) & mask,		/*prop_prio*/
					(elig_func_val[j] >> 1) & 0x01,		/*min_tb*/
					elig_func_val[j] & 0x01,			/*max_tb*/
					elig_func_val[j]
					);
		}
	}
	pr_info("-------------------------------------------\n");

out:
	COMPLETE_HW_WRITE
#endif
	return rc;
}


int tm_dump_port_hw(tm_handle hndl, uint32_t portIndex)
{
	uint32_t                   portFirstChild;
	uint32_t                   portLastChild;
	uint32_t                   cFirstChild;
	uint32_t                   cLastChild;
	uint32_t                   bFirstChild;
	uint32_t                   bLastChild;
	uint32_t                   aFirstChild;
	uint32_t                   aLastChild;
	uint32_t                   ic;
	uint32_t                   ib;
	uint32_t                   ia/*, iq*/;
	uint8_t status;
	int rc = 0;

	TM_REGISTER_VAR(TM_Sched_PortRangeMap)
	TM_REGISTER_VAR(TM_Sched_ClvltoPortAndBlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_BLvltoClvlAndAlvlRangeMap)
	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	DECLARE_RM_HANDLE(rm, ctl->rm)
	CHECK_TM_CTL_PTR(ctl)

	if (portIndex >= rm->rm_total_ports)
		return -ERANGE;

	TM_READ_TABLE_REGISTER(TM.Sched.PortRangeMap, portIndex, TM_Sched_PortRangeMap);

	TM_REGISTER_GET(TM_Sched_PortRangeMap, Lo, portFirstChild, (uint32_t));
	TM_REGISTER_GET(TM_Sched_PortRangeMap, Hi, portLastChild, (uint32_t));

	rc = rm_node_status(rm, P_LEVEL, portIndex, &status);
	if ((rc) || (status != RM_TRUE))
		return -ERANGE;

	pr_info("P%d: (c%d-c%d)\n", portIndex, portFirstChild, portLastChild);

	for (ic = portFirstChild; ic <= portLastChild; ic++) {

		rc = rm_node_status(rm, C_LEVEL, ic, &status);
		if (rc)
			return -ERANGE;

		if (status != RM_TRUE)
			continue;

		TM_READ_TABLE_REGISTER(TM.Sched.ClvltoPortAndBlvlRangeMap, ic, TM_Sched_ClvltoPortAndBlvlRangeMap);

		if (portIndex != TM_REGISTER_VAR_NAME(TM_Sched_ClvltoPortAndBlvlRangeMap).Port)
			continue;

		TM_REGISTER_GET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlLo, cFirstChild, (uint32_t));
		TM_REGISTER_GET(TM_Sched_ClvltoPortAndBlvlRangeMap, BlvlHi, cLastChild, (uint32_t));

		pr_info(" C%d: (b%d-b%d)\n", ic, cFirstChild, cLastChild);

		for (ib = cFirstChild; ib <= cLastChild; ib++) {
			rc = rm_node_status(rm, B_LEVEL, ib, &status);
			if (rc)
				return -ERANGE;

			if (status != RM_TRUE)
				continue;

			TM_READ_TABLE_REGISTER(TM.Sched.BLvltoClvlAndAlvlRangeMap, ib,
					TM_Sched_BLvltoClvlAndAlvlRangeMap);

			if (ic != TM_REGISTER_VAR_NAME(TM_Sched_BLvltoClvlAndAlvlRangeMap).Clvl)
				continue;

			TM_REGISTER_GET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlLo, bFirstChild, (uint32_t));
			TM_REGISTER_GET(TM_Sched_BLvltoClvlAndAlvlRangeMap, AlvlHi, bLastChild, (uint32_t));

			pr_info("  B%d: (a%d-a%d)\n", ib, bFirstChild, bLastChild);

			for (ia = bFirstChild ; ia <= bLastChild ; ia++) {
				rc = rm_node_status(rm, A_LEVEL, ia, &status);
				if (rc)
					return -ERANGE;

				if (status != RM_TRUE)
					continue;

				TM_READ_TABLE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, ia,
						TM_Sched_ALvltoBlvlAndQueueRangeMap);

				if (ib != TM_REGISTER_VAR_NAME(TM_Sched_ALvltoBlvlAndQueueRangeMap).Blvl)
					continue;

				TM_REGISTER_GET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo, aFirstChild, (uint32_t));
				TM_REGISTER_GET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi, aLastChild, (uint32_t));

				pr_info("    A%d: (q%d-q%d)\n", ia, aFirstChild, aLastChild);

				/*
				pr_info("	Q: ");

				for (iq = aFirstChild ; iq <= aLastChild ; iq++)
					pr_info("%d ", iq);
				pr_info("\n");
				*/
			}
		}
	}

	return rc;
}


int set_hw_elig_per_queue_range(tm_handle hndl, uint32_t startInd, uint32_t endInd, uint8_t elig)
{
	int rc = -EFAULT;
	int i;

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_QueueEligPrioFuncPtr, Ptr, elig);
	for (i = startInd; i <= endInd; i++) {
		TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, i, TM_Sched_QueueEligPrioFuncPtr);
		if (rc)
			return rc;
	}
	return rc;
}

int check_hw_drop_path(tm_handle hndl, uint32_t timeout, uint8_t full_path)
{
	int rc = -EFAULT;
	int i = 0;
	int k = 0;
	int num_of_queues;
	uint32_t len = 0;
	uint32_t a_ind, b_ind, c_ind, port;
	uint8_t debug_flag = TM_DISABLE;

	TM_REGISTER_VAR(TM_Sched_QueueEligPrioFuncPtr)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	if ((full_path != TM_DISABLE) && (full_path != TM_ENABLE))
		return -ERANGE;

	num_of_queues = get_tm_queues_count();

	/* DeQ disable to all Qs */
	rc = set_hw_elig_per_queue_range(ctl, 0, (num_of_queues - 1), TM_ELIG_DEQ_DISABLE);
	if (rc) {
		pr_info("set_hw_elig_per_queue_range to DeQ disable failed: %d\n", rc);
		return rc;
	}

	/* Disable debug printing if it switched on */
	if (tm_debug_on == TM_ENABLE) {
		debug_flag = TM_ENABLE;
		tm_debug_on = TM_DISABLE;
	}

	while (k < timeout) {
		/* Check which Q is not empty */
		for (i = 0; i < num_of_queues; i++) {
			rc = get_hw_queue_length(ctl, Q_LEVEL, i, &len);
			if (rc) {
				pr_info("get_hw_queue_length failed: %d\n", rc);
				return rc;
			}
			if (len != 0) {
				pr_info("Q%d: LEN=%d\n", i, len*16);
				/* DeQ enable on non empty Q */
				TM_REGISTER_SET(TM_Sched_QueueEligPrioFuncPtr, Ptr, TM_ELIG_Q_DEFAULT);
				TM_WRITE_TABLE_REGISTER(TM.Sched.QueueEligPrioFuncPtr, i,
					TM_Sched_QueueEligPrioFuncPtr);
				if (rc) {
					pr_info("DeQ enable to Q=%d failed: %d\n", i, rc);
					return rc;
				}
				while (len) {/* wait till Q will empty */
					rc = get_hw_queue_length(ctl, Q_LEVEL, i, &len);
					if (rc) {
						pr_info("get_hw_queue_length failed: %d\n", rc);
						return rc;
					}
				}

				if (full_path) {
					a_ind = ctl->tm_queue_array[i].parent_a_node;
					b_ind = ctl->tm_a_node_array[a_ind].parent_b_node;
					c_ind = ctl->tm_b_node_array[b_ind].parent_c_node;
					port = ctl->tm_c_node_array[c_ind].parent_port;
					pr_info("Full Path: A = %d, B = %d, C = %d, P = %d\n",
						a_ind, b_ind, c_ind, port);
				}

				break; /* for (i = 0; i < 512; i++) */
			} /* if (len != 0) */
		}
		msleep(10);
		k += 20; /* adding ~10 for reading length of Qs*/
	}


	/* Restore debug printing state */
	if (debug_flag == TM_ENABLE)
		tm_debug_on = TM_ENABLE;

	/* restore queues eligible function */
	for (i = 0; i < num_of_queues; i++) {
		rc = set_hw_queue_elig_prio_func_ptr(ctl, i);
		if (rc) {
			pr_info("set_hw_queue_elig_prio_func_ptr failed: %d\n", rc);
			return rc;
		}
	}

	return rc;
}


int set_hw_queue_map_directly(tm_handle hndl, uint32_t queue_index, uint32_t parent)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_QueueAMap)

	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_QueueAMap, Alvl, parent);
	TM_WRITE_TABLE_REGISTER(TM.Sched.QueueAMap, queue_index, TM_Sched_QueueAMap);

	COMPLETE_HW_WRITE
	return rc;
}

int set_hw_a_node_map_directly(tm_handle hndl,
					uint32_t a_node_index,
					uint32_t parent,
					uint32_t first_child,
					uint32_t last_child)
{
	int rc = -EFAULT;

	TM_REGISTER_VAR(TM_Sched_ALvltoBlvlAndQueueRangeMap)
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

	TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, Blvl, parent);
	TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueLo, first_child)
	TM_REGISTER_SET(TM_Sched_ALvltoBlvlAndQueueRangeMap, QueueHi, last_child);
	TM_WRITE_TABLE_REGISTER(TM.Sched.ALvltoBlvlAndQueueRangeMap, a_node_index, TM_Sched_ALvltoBlvlAndQueueRangeMap);
	COMPLETE_HW_WRITE
	return rc;
}
