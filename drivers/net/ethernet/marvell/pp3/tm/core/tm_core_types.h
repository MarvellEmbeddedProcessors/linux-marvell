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

#ifndef	TM_CORE_TYPES_H
#define	TM_CORE_TYPES_H


#include "tm_defs.h"

/** Periodic Scheme configuration
 */
#define TM_FIXED_PERIODIC_SCHEME_DEC_EN			TM_DISABLE
#define TM_FIXED_PERIODIC_SCHEME_PER_EN			1
#define TM_FIXED_PERIODIC_SCHEME_L				55
#define TM_FIXED_PERIODIC_SCHEME_K				1
#define TM_FIXED_PERIODIC_SCHEME_N				23
#define TM_FIXED_10_M_SHAPING_TOKEN				10

/** Port level shaping
 */
#define TM_FIXED_2_5_G_PORT_PERIODIC_SCHEME_PER_INTERVAL	48
#define TM_FIXED_2_5_G_PORT_SHAPING_PERIODS					1
#define TM_FIXED_2_5_G_PORT_SHAPING_BURST_SIZE				10


/** C level shaping
 */
#define TM_FIXED_2_5_G_C_LEVEL_PERIODIC_SCHEME_PER_INTERVAL		768
#define TM_FIXED_10_M_C_LEVEL_SHAPING_TOKEN_RES					4
#define TM_FIXED_2_5_G_C_LEVEL_SHAPING_BURST_SIZE				16

/** B level shaping
 */
#define TM_FIXED_2_5_G_B_LEVEL_PERIODIC_SCHEME_PER_INTERVAL		1536
#define TM_FIXED_10_M_B_LEVEL_SHAPING_TOKEN_RES					5
#define TM_FIXED_2_5_G_B_LEVEL_SHAPING_BURST_SIZE				16

/** A level shaping
 */
#define TM_FIXED_2_5_G_A_LEVEL_PERIODIC_SCHEME_PER_INTERVAL		1536
#define TM_FIXED_10_M_A_LEVEL_SHAPING_TOKEN_RES					5
#define TM_FIXED_2_5_G_A_LEVEL_SHAPING_BURST_SIZE				16

/** Queue level shaping
 */
#define TM_FIXED_2_5_G_QUEUE_PERIODIC_SCHEME_PER_INTERVAL		3072
#define TM_FIXED_10_M_QUEUE_SHAPING_TOKEN_RES					6
#define TM_FIXED_2_5_G_QUEUE_SHAPING_BURST_SIZE					16


/** Internal Constants definitions
 */
#define TM_NUM_QUEUE_DROP_PROF        16 /* was 2048*/
#define TM_NUM_A_NODE_DROP_PROF       8  /* was 256*/
#define TM_NUM_B_NODE_DROP_PROF       8  /* was 64*/
#define TM_NUM_C_NODE_DROP_PROF       2  /* was 64*/ /* Per CoS */
#define TM_NUM_PORT_DROP_PROF         16 /* TBD: get_tm_port_count() */
#define TM_NUM_WRED_QUEUE_CURVES      8  /* Per Color */
#define TM_NUM_WRED_A_NODE_CURVES     8  /* Per Color */
#define TM_NUM_WRED_B_NODE_CURVES     4
#define TM_NUM_WRED_C_NODE_CURVES     2  /* was 4*/ /* Per CoS */
#define TM_NUM_WRED_PORT_CURVES       4
#define TM_NUM_DIVIDERS               8


/** Registers values
 */
#define TM_128M 0x7FFFFFF
#define TM_4M   0x3FFFFF

#define TM_1K	1024


/** Delay Size Multiplier (used to calculate TD Threshold)
 */
#define TM_DELAY_SIZE_MULT   1


/* Physical port capacity (speed in Kbits) */
#define TM_1G_SPEED   1000000  /* 1050000000UL */   /* 1G + 5% */
#define TM_2HG_SPEED  2000000  /* 2625000000UL */   /* 2.5G + 5% */
#define TM_10G_SPEED  10000000 /* 10500000000ULL */ /* 10G + 5% */
#define TM_40G_SPEED  40000000 /* 42000000000ULL */ /* 40G + 5% */
#define TM_50G_SPEED  50000000
#define TM_100G_SPEED 100000000


/** WRED CoS number */
#define TM_WRED_COS              8

/** WRED curve points number */
#define TM_WRED_CURVE_POINTS     32

/** Drop Profile Pointer Table values */
#define TM_Q_DRP_PROF_PER_ENTRY  4
#define TM_A_DRP_PROF_PER_ENTRY  4
#define TM_B_DRP_PROF_PER_ENTRY  8
#define TM_C_DRP_PROF_PER_ENTRY  8


/** Max number of Scrubbing slots */
#define TM_SCRUB_SLOTS 64

/** Max Probability Mode */
enum tm_max_prob_mode {
	TM_MAX_PROB_100 = 0,   /* 100%  */
	TM_MAX_PROB_50,        /*  50%  */
	TM_MAX_PROB_25,        /*  25%  */
	TM_MAX_PROB_12H       /* 12.5% */
};


/*********************************/
/* Internal Databases Structures */
/*********************************/
#define TM_ELIG_FUNC_TABLE_SIZE		64

#define	TM_NODE_DISABLED_FUN		62

#define VALIDATE_ELIG_FUNCTION(elig_fun) \
do { \
	if ((elig_fun >= TM_ELIG_FUNC_TABLE_SIZE) || (elig_fun == TM_NODE_DISABLED_FUN))	{\
		/* maximum function id 0..63 */\
		rc = TM_CONF_ELIG_PRIO_FUNC_ID_OOR;\
		goto out;\
	} \
} while (0)


/** Global arrays structures definitions */


/** WRED curve
 */
struct tm_wred_curve {
	uint8_t prob[32];
};

struct curve_ind {
	uint8_t index:3; /* 3 bits for Q/A-level, 2 bits - B/C/P levels */
};

struct min_thresh {
	uint16_t thresh:10;
};

struct dwrr_quantum {
	uint16_t quantum:9;
};

struct scale_exp {
	uint8_t exp:5;
};

struct scale_ratio {
	uint16_t ratio:10;
};


struct dp_ratio {
	uint8_t ratio:6;
};


/** Range of children nodes in symetric distribution
 */
struct ranges {
	uint32_t norm_range[3];
	uint32_t last_range[3];
};

/** Drop profile
 */
struct tm_drop_profile {
	uint32_t out_bw;                /**< CATD/WRED Out BW in Kbits/sec */
	uint32_t cbtd_bw;               /**< CBTD BW in Kbits/sec */
	uint8_t aql_exp:4;              /**< Forget factor exponent */
	uint8_t color_td_en:1;          /**< Colored Tail Drop Enable */

	struct scale_exp scale_exp[3];     /**< Used for scaling of AQL range */
	struct scale_ratio scale_ratio[3]; /**< Used for scaling of DP range */
	struct curve_ind curve_id[3];      /**< RED curve index per color[0..2] */
	struct dp_ratio dp_ratio[3];       /**< Used for scaling of DP */

	struct min_thresh min_threshold[3];/**< RED curve Min threshold per color [0..2] */
	uint8_t td_thresh_res:1;     /**< Tail Drop Threshold resolution - 16B or 16KB */
	uint32_t td_threshold:19;    /**< Hard Limit on queue length */
	struct rm_list *use_list;    /**< List of ports that use this profile, for Port lvl only */
	uint32_t use_counter;

	/* For read issues */
	uint32_t min_th_sw[3];        /**< Min Threshold ratio from RTT in % per color */
	uint32_t max_th_sw[3];        /**< Max Threshold ratio from RTT in % per color */
} __ATTRIBUTE_PACKED__;


/** Queue data structure
 */
struct tm_queue {
	uint8_t installed:1;
	uint16_t dwrr_quantum:14;
	uint16_t parent_a_node:14;
	uint8_t wred_profile_ref;
	uint8_t elig_prio_func_ptr;
} __ATTRIBUTE_PACKED__;


/** A-node data structure
 */
struct tm_a_node {
	uint16_t dwrr_quantum:14;
	uint8_t dwrr_priority;
	uint16_t parent_b_node:12;
	uint16_t first_child_queue;
	uint16_t last_child_queue;
	uint8_t wred_profile_ref;
	uint8_t elig_prio_func_ptr;
} __ATTRIBUTE_PACKED__;


/** B-node data structure
 */
struct tm_b_node {
	uint16_t dwrr_quantum:14;
	uint8_t dwrr_priority;
	uint16_t parent_c_node:9;
	uint16_t first_child_a_node:14;
	uint16_t last_child_a_node:14;
	uint8_t wred_profile_ref;
	uint8_t elig_prio_func_ptr;
} __ATTRIBUTE_PACKED__;


/** C-node data structure
 */
struct tm_c_node {
	uint8_t wred_cos;   /* bit map */
	uint16_t dwrr_quantum:14;
	uint8_t dwrr_priority;
	uint8_t parent_port;
	uint16_t first_child_b_node:12;
	uint16_t last_child_b_node:12;
	uint8_t wred_profile_ref[TM_WRED_COS];
	uint8_t elig_prio_func_ptr;
} __ATTRIBUTE_PACKED__;


/** Port data structure
 */
struct tm_port {
#ifdef MV_QMTM_NSS_A0
	struct dwrr_quantum dwrr_quantum[8];
#endif
	uint8_t dwrr_priority;
	uint8_t port_speed:3;

#ifdef MV_QMTM_NOT_NSS
	/* Shaping config */
	uint16_t cir_token:11;
	uint16_t eir_token:11;
	uint32_t cir_burst_size:17;
	uint32_t eir_burst_size:17;
	uint16_t periods:13;
	uint8_t min_token_res:1;
	uint8_t max_token_res:1;
#endif
	uint8_t sym_mode:1;         /* assymetric/symetric tree */
	uint16_t first_child_c_node:9;
	uint16_t last_child_c_node:9;
	struct ranges children_range;   /* for symetric distribution only */

#ifdef MV_QMTM_NOT_NSS
	uint8_t rcb_high_range_limit;
	uint8_t rcb_low_range_limit;
#endif

	/* WRED profile */
	uint8_t wred_profile_ref; /* Global port Drop profile */
	uint8_t wred_cos;   /* bit map */
	uint8_t wred_profile_ref_cos[TM_WRED_COS];
#ifdef MV_QMTM_NOT_NSS
	/*save the values as configured for read api*/
	uint32_t cbs_sw;                  /**< CBS in kbytes */
	uint32_t ebs_sw;                  /**< EBS in kbytes */
#endif
	uint8_t elig_prio_func_ptr;
} __ATTRIBUTE_PACKED__;


/* Eligible Priority Functions structures */

struct tm_elig_prio_func_node_entry {
	uint16_t func_out[4];
};

/* Eligible Priority Functions Definitions */

struct tm_elig_prio_func_node {
	struct tm_elig_prio_func_node_entry tbl_entry[8];
};

struct tm_elig_prio_func_queue {
	struct tm_elig_prio_func_node_entry tbl_entry;
};


#ifdef MV_QMTM_NOT_NSS
/** TM2TM Channel */
struct tm2tm_port_channel {
	uint8_t configured:1;
	uint8_t egr_elems:6;        /* x4, max 64, reset 0x30, 0=all 64 */
	uint8_t _reserved_1:1;
	uint8_t src_lvl:3;          /* A/B/C/Port */
	uint8_t mode:1;             /* WRED/BP */
	/* relevant for BP mode only */
	uint8_t bp_map:1;           /* Queue/C */
	uint16_t bp_offset;
	uint8_t bp_xon:6;           /* BP Xon */
	uint8_t _reserved_2:2;
	uint8_t bp_xoff:6;          /* BP Xoff */
} __ATTRIBUTE_PACKED__;

struct tm2tm_node_channel {
	uint8_t configured:1;
	uint16_t egr_elems;         /* x16, max 64K, reset 0x0, 0=all 64K */
	uint8_t src_lvl:3;          /* Queue/A/B/C */
	uint8_t mode:1;             /* WRED/BP */
	/* relevant for BP mode only */
	uint8_t bp_map:1;           /* Queue/C */
	uint16_t bp_offset;
	uint8_t bp_xon:6;           /* BP Xon */
	uint8_t _reserved_1:2;
	uint8_t bp_xoff:6;          /* BP Xoff */
} __ATTRIBUTE_PACKED__;
#endif
/** TM2TM Egress AQM mode settings */
struct tm_aqm_local_params {
	uint8_t configured:1;
	uint8_t color_num; /* 1 or 2 or 3 */
	enum tm_dp_source dp_src[3]; /* QL/AQL per color [0..2] */
	uint8_t resolution; /* 4 or 6 bits */
	enum tm_max_prob_mode max_p_mode[4]; /* Max Probability Mode - 100/50/25/12.5% */
} __ATTRIBUTE_PACKED__;


/** TM2TM Ingress AQM mode settings - Channels */
struct tm_aqm_remote_params {
	uint8_t configured:1;
	uint8_t color_num; /* 0 or 1 or 2 */
} __ATTRIBUTE_PACKED__;

/** TM2TM Drop Unit structure */
struct dp_unit {
	struct tm_aqm_local_params local[P_LEVEL+1]; /* per Port/C/B/A/Queue level */
	struct tm_aqm_remote_params remote[TM2TM_PORT_CH+1]; /* per Port/Node level */
};


/** TM Scheduler Port SMS pBase Attribute structure */
struct port_sms_attr_pbase {
	uint8_t pbase;
	uint8_t pshift;
};

/** TM Scheduler Port SMS Qmap parsing Attribute structure */
struct port_sms_attr_qmap_parsing {
	uint8_t mode;
	uint16_t base_q;
	uint8_t dcolor;
};



/* typedef void *tm_handle; */
#define tm_handle void *


struct tm_ctl {
	uint32_t magic;
	tm_handle rm;   /**< rm hndl */

	/* Nodes arrays */
	struct tm_queue *tm_queue_array;
	struct tm_a_node *tm_a_node_array;
	struct tm_b_node *tm_b_node_array;
	struct tm_c_node *tm_c_node_array;
	struct tm_port *tm_port_array;

	/* Global arrays */
	struct tm_drop_profile *tm_q_lvl_drop_profiles;
	struct tm_drop_profile *tm_a_lvl_drop_profiles;
	struct tm_drop_profile *tm_b_lvl_drop_profiles;
	struct tm_drop_profile *tm_c_lvl_drop_profiles[TM_WRED_COS];
	struct tm_drop_profile *tm_p_lvl_drop_profiles;
	struct tm_drop_profile *tm_p_lvl_drop_profiles_cos[TM_WRED_COS];

	struct tm_wred_curve *tm_wred_q_lvl_curves;
	struct tm_wred_curve *tm_wred_a_lvl_curves;
	struct tm_wred_curve *tm_wred_b_lvl_curves;
	struct tm_wred_curve *tm_wred_c_lvl_curves[TM_WRED_COS];
	struct tm_wred_curve *tm_wred_ports_curves;
	struct tm_wred_curve *tm_wred_ports_curves_cos[TM_WRED_COS];

	/* Eligible Priority Function Table arrays */
	struct tm_elig_prio_func_node tm_elig_prio_a_lvl_tbl[TM_ELIG_FUNC_TABLE_SIZE];
	struct tm_elig_prio_func_node tm_elig_prio_b_lvl_tbl[TM_ELIG_FUNC_TABLE_SIZE];
	struct tm_elig_prio_func_node tm_elig_prio_c_lvl_tbl[TM_ELIG_FUNC_TABLE_SIZE];
	struct tm_elig_prio_func_node tm_elig_prio_p_lvl_tbl[TM_ELIG_FUNC_TABLE_SIZE];

	struct tm_elig_prio_func_queue tm_elig_prio_q_lvl_tbl[TM_ELIG_FUNC_TABLE_SIZE];

	uint16_t *tm_q_lvl_drop_prof_ptr;   /* mirror for Q Drop prof. ptr table
										* to avoid read-modify-write during
										* single pointer update */
	uint16_t *tm_a_lvl_drop_prof_ptr;   /* mirror for A Drop prof. ptr table
										* to avoid read-modify-write during
										* single pointer update */
	uint8_t *tm_b_lvl_drop_prof_ptr;    /* mirror for B Drop prof. ptr table
										* to avoid read-modify-write during
										* single pointer update */
	uint8_t *tm_c_lvl_drop_prof_ptr[TM_WRED_COS];
										/* mirror for C Drop prof. ptr table
										* to avoid read-modify-write during
										* single pointer update */

	/* Abstraction for Port Drop profiles */
	uint8_t *tm_p_lvl_drop_prof_ptr;
	uint8_t *tm_p_lvl_drop_prof_ptr_cos[TM_WRED_COS]; /* CoS mode */

	uint16_t *tm_q_cos;   /* mirror for Q Cos table
						* to avoid read-modify-write during
						* single Queue update */

#ifdef MV_QMTM_NOT_NSS
	/* Scheduling parameters */
	uint32_t freq;                 /**< LAD frequency */
#endif
	/* global update states */
	uint8_t periodic_scheme_state; /**< periodic scheme updated/not updated */
	/* RCB sections */
	uint8_t rcb_section_used[4];   /**< 4 sections, max range 0..0xFF each */

#ifdef MV_QMTM_NOT_NSS
	/* per level data including port level */
	struct level_config level_data[P_LEVEL+1];
#endif
	/* Tree data */
	uint8_t tree_deq_status;       /**< tree DeQ status */
#ifdef MV_QMTM_NSS_A0
	uint8_t tree_dwrr_priority;    /**< tree DWRR priority bitmap for port scheduling */
#endif

	/* TM2TM */
	struct dp_unit dp_unit;
#ifdef MV_QMTM_NOT_NSS
	struct tm2tm_port_channel port_ch;
	struct tm2tm_node_channel node_ch;
#endif
	/* Reshuffling changes */
	struct tm_tree_change list;

	/* Other registers */
#ifdef MV_QMTM_NSS_A0
	uint8_t port_ext_bp_en;
	uint8_t dwrr_bytes_burst_limit;
#endif
	uint32_t mtu;                  /**<  Maximal Transmission Unit */
	uint16_t min_quantum;          /**<  nodes minimum quantum [256B], must be  equal to TM_MSU */
	uint16_t min_pkg_size;         /**<  minimum package size */
#ifdef MV_QMTM_NSS_A0
	uint8_t port_ch_emit;          /**<  port chunks emitted per scheduler selection */
#endif
#ifdef MV_QMTM_NOT_NSS
	uint8_t aging_status;          /**< aging status */
#endif
	/* TM-SMS mapping */
	struct port_sms_attr_pbase        *tm_port_sms_attr_pbase;
	struct port_sms_attr_qmap_parsing *tm_port_sms_attr_qmap_pars;

	/* environment*/
	tm_handle hEnv;
};



/** Internal TM control structure
 */

#define TM_MAGIC 0x24051974
/* following macro declares and checks validity of tmctl*/
#define DECLARE_TM_CTL_PTR(name, value)	struct tm_ctl *name = (struct tm_ctl *)value;

#define CHECK_TM_CTL_PTR(ptr)	\
{ \
	if (!ptr) \
		return -EINVAL; \
	if (ptr->magic != TM_MAGIC) \
		return -EBADF; \
}
/*
#define TM_CTL(name, handle) DECLARE_TM_CTL_PTR(name, handle); CHECK_TM_CTL_PTR(name);
*/

#define TM_ENV(var)	((var)->hEnv)


#endif   /* TM_CORE_TYPES_H */
