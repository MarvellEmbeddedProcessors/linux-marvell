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

#include "tm_set_local_db_defaults.h"
#include "tm_core_types.h"
#include "rm_list.h"
#include "rm_internal_types.h"


/**
 */
int set_sw_sched_conf_default(void * hndl)
{
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)


	/* HW defaults configuration */
	ctl->periodic_scheme_state = TM_DISABLE; /* periodic scheme
											  * configured - yes/no */

#ifdef MV_QMTM_NSS_A0
	ctl->dwrr_bytes_burst_limit = 0x20;
	ctl->tree_dwrr_priority = 0;
#endif
	ctl->tree_deq_status = TM_ENABLE;

	return 0;
}


/**
 */
int set_sw_gen_conf_default(void * hndl)
{
	DECLARE_TM_CTL_PTR(ctl, hndl)
	CHECK_TM_CTL_PTR(ctl)

#ifdef MV_QMTM_NSS_A0
	ctl->port_ext_bp_en = 1;
#endif
	return 0;
}


/**
 */
int set_sw_drop_profile_default(struct tm_drop_profile *profile,
							uint32_t prof_index)
{
	int i;

	for (i = 0; i < 3; i++) {
		profile[prof_index].scale_exp[i].exp = 0; /* 1 Burst unit */
		profile[prof_index].scale_ratio[i].ratio = 0;
		profile[prof_index].curve_id[i].index = 0;
		profile[prof_index].dp_ratio[i].ratio = 0;
		profile[prof_index].min_threshold[i].thresh = 0x0; /* 0 Bursts  */
	}
	profile[prof_index].out_bw = 0;
	profile[prof_index].cbtd_bw = 0;
	profile[prof_index].aql_exp = 0; /* Forget Factor = 1, AQL=QL */
	profile[prof_index].color_td_en = 0; /* Disabled */
	profile[prof_index].td_thresh_res = 0; /* 16B */
	profile[prof_index].td_threshold = 0; /* 0 Bursts */
	profile[prof_index].use_counter = 0;
	profile[prof_index].use_list = NULL;
	return 0;
}

/*
 Used for all levels WRED queues
*/
/**
 */
int set_sw_wred_curve_default(struct tm_wred_curve *curve, uint16_t curve_index)
{
	int i;

	for (i = 0; i < TM_WRED_CURVE_POINTS; i++)
		curve[curve_index].prob[i] = 0;

	return 0;
}

/**
 */
int set_sw_queue_default(struct tm_queue *array,
									uint32_t queue_ind,
									struct rmctl *rm)
{
	(void)rm;

	array[queue_ind].installed = TM_DISABLE;
	array[queue_ind].dwrr_quantum = 0x40;
	array[queue_ind].wred_profile_ref = 0;
	array[queue_ind].elig_prio_func_ptr = 0;
	return 0;
}


/**
 */
int set_sw_a_node_default(struct tm_a_node *array,
										uint32_t node_ind,
										struct rmctl *rm)
{
	(void)rm;

	array[node_ind].dwrr_quantum = 0x40;
	array[node_ind].dwrr_priority = 0;
	array[node_ind].wred_profile_ref = 0;
	array[node_ind].elig_prio_func_ptr = 0;
	return 0;
}


/**
 */
int set_sw_b_node_default(struct tm_b_node *array,
										uint32_t node_ind,
										struct rmctl *rm)
{
	(void)rm;

	array[node_ind].dwrr_quantum = 0x40;
	array[node_ind].dwrr_priority = 0;
	array[node_ind].wred_profile_ref = 0;
	array[node_ind].elig_prio_func_ptr = 0;
	return 0;
}


/**
 */
int set_sw_c_node_default(struct tm_c_node *array,
										uint32_t node_ind,
										struct rmctl *rm)
{
	(void)rm;

	array[node_ind].dwrr_quantum = 0x40;
	array[node_ind].dwrr_priority = 0;
	array[node_ind].wred_cos = 0;
	array[node_ind].wred_profile_ref[0] = 0;
	array[node_ind].wred_profile_ref[1] = 0;
	array[node_ind].wred_profile_ref[2] = 0;
	array[node_ind].wred_profile_ref[3] = 0;
	array[node_ind].wred_profile_ref[4] = 0;
	array[node_ind].wred_profile_ref[5] = 0;
	array[node_ind].wred_profile_ref[6] = 0;
	array[node_ind].wred_profile_ref[7] = 0;
	array[node_ind].elig_prio_func_ptr = 0;
	return 0;
}


/**
 */
int set_sw_port_default(struct tm_port *array,
									  uint8_t port_ind,
									  struct rmctl *rm)
{
	int i;

	(void)rm;

#ifdef MV_QMTM_NSS_A0
	array[port_ind].dwrr_quantum[0].quantum = 0x10;
	array[port_ind].dwrr_quantum[1].quantum = 0x10;
	array[port_ind].dwrr_quantum[2].quantum = 0x10;
	array[port_ind].dwrr_quantum[3].quantum = 0x10;
	array[port_ind].dwrr_quantum[4].quantum = 0x10;
	array[port_ind].dwrr_quantum[5].quantum = 0x10;
	array[port_ind].dwrr_quantum[6].quantum = 0x10;
	array[port_ind].dwrr_quantum[7].quantum = 0x10;
#endif
	array[port_ind].dwrr_priority = 0;
	array[port_ind].port_speed = TM_1G_PORT;
	array[port_ind].wred_profile_ref = 0;
	for (i = 0; i < TM_WRED_COS; i++)
		array[port_ind].wred_profile_ref_cos[i] = 0;
	array[port_ind].wred_cos = 0;   /* bit map */
	array[port_ind].elig_prio_func_ptr = 0;
	return 0;
}


#define	SP_0		0
#define	SP_1		1
#define	SP_2		2
#define	SP_3		3
#define	SP_4		4
#define	SP_5		5
#define	SP_6		6
#define	SP_7		7

#define	PP_0		0
#define	PP_1		1
#define	PP_2		2
#define	PP_3		3
#define	PP_4		4
#define	PP_5		5
#define	PP_6		6
#define	PP_7		7


#define	FIX_PRIO_0		0
#define	FIX_PRIO_1		1
#define	FIX_PRIO_2		2
#define	FIX_PRIO_3		3
#define	FIX_PRIO_4		4
#define	FIX_PRIO_5		5
#define	FIX_PRIO_6		6
#define	FIX_PRIO_7		7


#define	USE_MIN_TB		1
#define	USE_MAX_TB		1
#define	NOT_USE_MIN_TB	0
#define	NOT_USE_MAX_TB	0

#define	POS_MIN_TB		0
#define	POS_MAX_TB		0
#define	NEG_MIN_TB		1
#define	NEG_MAX_TB		1


#define	ELIGIBLE(scheduled_priority, propagated_priority, use_minTB, useMaxTB) \
		((1 << 8) | (scheduled_priority << 5) | (propagated_priority << 2) | (use_minTB << 1) | useMaxTB)
#define	NOT_ELIGIBLE	0

#define	STRICT_PRIORITY(scheduled_priority, propagated_priority) \
	ELIGIBLE(scheduled_priority, propagated_priority, NOT_USE_MIN_TB, NOT_USE_MAX_TB)

#define	FIX_PRIORITY(priority)  ELIGIBLE(priority, priority, NOT_USE_MIN_TB, NOT_USE_MAX_TB)


#define Q_ENTRY(min_flag, max_flag)	tbl_entry.func_out[2*min_flag+max_flag]
/*	assert((int)function_ID < (int)TM_ELIG_FUNC_TABLE_SIZE); \*/

#define QUEUE_ELIG_FUNCTION(function_ID, p0, p1, p2, p3) \
do {\
	func_table[function_ID].p0;\
	func_table[function_ID].p1;\
	func_table[function_ID].p2;\
	func_table[function_ID].p3;\
} while (0)

#define QUEUE_ELIG_FUN_FIXED_PRIORITY(function_ID, priority) \
	{\
		QUEUE_ELIG_FUNCTION(function_ID,\
		Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = priority,\
		Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = priority,\
		Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = priority,\
		Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = priority);\
	}

#define QUEUE_DEQ_DISABLE(function_ID) QUEUE_ELIG_FUN_FIXED_PRIORITY(function_ID, NOT_ELIGIBLE)

#define N_ENTRY(min_flag, max_flag, priority)	tbl_entry[min_flag*4 + max_flag*2 + priority/4].func_out[priority & 3]
/*	assert((int)function_ID < (int)TM_ELIG_FUNC_TABLE_SIZE); \*/

#define NODE_ELIG_FUNCTION(function_ID, p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15,\
	p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29, p30, p31) \
do {\
	func_table[function_ID].p0;\
	func_table[function_ID].p1;\
	func_table[function_ID].p2;\
	func_table[function_ID].p3;\
	func_table[function_ID].p4;\
	func_table[function_ID].p5;\
	func_table[function_ID].p6;\
	func_table[function_ID].p7;\
	func_table[function_ID].p8;\
	func_table[function_ID].p9;\
	func_table[function_ID].p10;\
	func_table[function_ID].p11;\
	func_table[function_ID].p12;\
	func_table[function_ID].p13;\
	func_table[function_ID].p14;\
	func_table[function_ID].p15;\
	func_table[function_ID].p16;\
	func_table[function_ID].p17;\
	func_table[function_ID].p18;\
	func_table[function_ID].p19;\
	func_table[function_ID].p20;\
	func_table[function_ID].p21;\
	func_table[function_ID].p22;\
	func_table[function_ID].p23;\
	func_table[function_ID].p24;\
	func_table[function_ID].p25;\
	func_table[function_ID].p26;\
	func_table[function_ID].p27;\
	func_table[function_ID].p28;\
	func_table[function_ID].p29;\
	func_table[function_ID].p30;\
	func_table[function_ID].p31;\
} while (0)

#define NODE_ELIG_FUN_FIXED_PRIORITY(function_ID, priority) \
	{\
		NODE_ELIG_FUNCTION(function_ID,\
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = priority, \
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = priority, \
			/*  ---------------------------------- */\
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = priority, \
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = priority, \
			/*  ---------------------------------- */\
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = priority, \
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = priority, \
			/*  ---------------------------------- */\
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = priority, \
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = priority);\
	}

#define NODE_DEQ_DISABLE(function_ID)	{NODE_ELIG_FUN_FIXED_PRIORITY(function_ID, NOT_ELIGIBLE); }

/*   Queues default EligibleFunction   table */
/**
 */
void set_default_queue_elig_prio_func_table(struct tm_elig_prio_func_queue *func_table)
{
	int i;
	/* initialize memory */
	for (i = 0; i < TM_ELIG_FUNC_TABLE_SIZE ; i++)
		QUEUE_DEQ_DISABLE(i);

	/*  setup default functions  */
	/*
	===================================================================================================
	   shapeless functions
	===================================================================================================
	*/
	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_0),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_0),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_0),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_0));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P1,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_1),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_1),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_1),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_1));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P2,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_2),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_2),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_2),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_2));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P3,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_3),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_3),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_3),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_3));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P4,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_4),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_4),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_4),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_4));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P5,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_5),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_5),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_5),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_5));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P6,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_6),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_6),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_6),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_6));

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_FIXED_P7,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_7),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_7),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = FIX_PRIORITY(FIX_PRIO_7),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = FIX_PRIORITY(FIX_PRIO_7));

	/*
	===================================================================================================
	   eligible functions uses only min shaper
	===================================================================================================
	*/
	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_0, PP_0 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P1,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_1, PP_1 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_1, PP_1 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P2,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_2, PP_2 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_2, PP_2 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P3,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_3, PP_3 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_3, PP_3 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P4,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_4, PP_4 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_4, PP_4 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P5,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_5, PP_5 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_5, PP_5 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P6,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_6, PP_7 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_6, PP_7 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_MIN_SHP_P7,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_7, PP_7 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_7, PP_7 , USE_MIN_TB, NOT_USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = NOT_ELIGIBLE,
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	/*
	===================================================================================================
	   eligible functions using min & max shaping
	===================================================================================================
	*/
	/* fixed priority functions*/
	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P0_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_0, PP_0 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P1_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_1, PP_1 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_1, PP_1 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P2_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_2, PP_2 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_2, PP_2 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P3_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_3, PP_3 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_3, PP_3 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P4_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_4, PP_4 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_4, PP_4 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P5_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_5, PP_5 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_5, PP_5 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P6_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_6, PP_6 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_6, PP_6 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);

	QUEUE_ELIG_FUNCTION(TM_ELIG_Q_SHP_MIN_SHP_P7_MAX_SHP_P0,
			Q_ENTRY(POS_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_7, PP_7 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(POS_MIN_TB, NEG_MAX_TB) = ELIGIBLE(SP_7, PP_7 , USE_MIN_TB,     USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, POS_MAX_TB) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, USE_MAX_TB),
			Q_ENTRY(NEG_MIN_TB, NEG_MAX_TB) = NOT_ELIGIBLE);


	/* reserved   DeqDisable function - don't change*/
	QUEUE_DEQ_DISABLE(TM_ELIG_DEQ_DISABLE);
	/* reduced macro example - with assert : function Id < TM_ELIG_FUNC_TABLE_SIZE  failed */
	/* QUEUE_ELIG_FUN_FIXED_PRIORITY(65 ,FIX_PRIORITY(FIX_PRIO_7));	*/
}


/* nodes default EligibleFunction  table*/
/**
 */
void set_default_node_elig_prio_func_table(struct tm_elig_prio_func_node *func_table)
{
	int i;
	/* initialize memory */
	for (i = 0; i < TM_ELIG_FUNC_TABLE_SIZE ; i++)
		NODE_DEQ_DISABLE(i);

	/*  setup default functions  */

	/*
	===================================================================================================
	   shapeless functions
	===================================================================================================
	*/
	/* fixed priority functions*/
	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_0),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_0),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_0),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_0),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_0));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P1,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P2,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_2),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_2),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_2),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_2),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_2));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P3,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_3),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_3),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_3),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_3),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_3));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P4,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_4),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_4),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_4),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_4),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_4));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P5,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P6,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_6),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_6),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_6),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_6),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_6));

	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_P7,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_7),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_7),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_7),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_7),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_7));

	/* shapeless propagated priority  function*/
	NODE_ELIG_FUNCTION(TM_ELIG_N_FIXED_PP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 , NOT_USE_MIN_TB, NOT_USE_MAX_TB));

	/*
	===================================================================================================
	   functions uses min shaper only
	===================================================================================================
	*/
	/* fixed priority min shaper functions */
	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P1,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P2,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P3,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P4,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P5,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P6,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_P7,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);
	/* propagated priority  min shaper function*/
	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP_PP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	/*
	===================================================================================================
	   functions uses min & max shapers
	===================================================================================================
	*/
	/* fixed priority min & max shaper functions */
	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P0_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P1_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P2_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P3_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P4_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P5_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P6_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_P7_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	/* propagated priority min & max shaper function */
	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_MIN_SHP_PP_MAX_SHP_P0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_6, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	/*
	===================================================================================================
	   obsolette functions here
	===================================================================================================
	*/
#if 0
	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_PP_MAX_TB_0,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MAX_INC_MIN_SHP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);







	NODE_ELIG_FUNCTION(TM_ELIG_N_PRIO1,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_1),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_1));

	NODE_ELIG_FUNCTION(TM_ELIG_N_PRIO5,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = FIX_PRIORITY(FIX_PRIO_5),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = FIX_PRIORITY(FIX_PRIO_5));

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_MIN_SHP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA_SP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 , NOT_USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA_SHP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA_SP_MIN_SHP,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA_SHP_IGN,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_1, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_2, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_3, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_4, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_PPA_MIN_SHP_SP_IGN,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_5, PP_0 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_1 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_5, PP_2 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_5, PP_3 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);



	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_4P_MIN_4P_MAX,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

	NODE_ELIG_FUNCTION(TM_ELIG_N_SHP_PP_TB,
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, POS_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_4) = ELIGIBLE(SP_4, PP_4 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_5) = ELIGIBLE(SP_5, PP_5 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_6) = ELIGIBLE(SP_6, PP_6 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			N_ENTRY(POS_MIN_TB, NEG_MAX_TB, PP_7) = ELIGIBLE(SP_7, PP_7 ,     USE_MIN_TB, NOT_USE_MAX_TB),
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_0) = ELIGIBLE(SP_0, PP_0 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_1) = ELIGIBLE(SP_1, PP_1 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_2) = ELIGIBLE(SP_2, PP_2 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_3) = ELIGIBLE(SP_3, PP_3 , NOT_USE_MIN_TB,     USE_MAX_TB),
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, POS_MAX_TB, PP_7) = NOT_ELIGIBLE,
			/*  ---------------------------------- */
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_0) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_1) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_2) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_3) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_4) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_5) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_6) = NOT_ELIGIBLE,
			N_ENTRY(NEG_MIN_TB, NEG_MAX_TB, PP_7) = NOT_ELIGIBLE);

#endif

	NODE_DEQ_DISABLE(TM_ELIG_DEQ_DISABLE);
}
