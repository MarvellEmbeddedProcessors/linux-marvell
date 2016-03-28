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

#ifndef	TM_DEFS_H
#define	TM_DEFS_H

#include "common/mv_sw_if.h"


/** BW resolution in Kbits */
#define TM_KBITS   1000

/** Maximal MTU */
#define TM_MAX_MTU	MV_MTU_MAX	/* 0x4000 */

/*---------------- Constants definitions-------------*/

/** Number of Linecards */
#define TM_LC_NUM       10

/** Round Trip Time */
#define TM_RTT          200    /* milliseconds */

/** Node Quantum Unit in units of 256 bytes */
#define TM_NODE_QUANTUM_UNIT  256
/** Port Quantum Unit in units of 64 bytes */
#define TM_PORT_QUANTUM_UNIT  64

/** No Drop  */
#define TM_NO_DROP_PROFILE  0

/** Infinite Shaping = 0xFFF (12 bits) * TM_FIXED_10_M_SHAPING_MAX_TOKEN */
#define TM_MAX_SHAPING_BW	0x9FF6

/* TM INVALID constants */
/** 32 bit Invalid data indicator */
#define TM_INVAL         0xFFFFFFFF
/** 64 bit Invalid data indicator */
#define TM_64BIT_INVAL   0xFFFFFFFFFFFFFFFFULL


/* Status constants */
/** Enable indicator */
#define TM_ENABLE  1
/** Disable indicator */
#define TM_DISABLE 0


/** Maximum Bandwidth (in Kbits/sec) */
#define TM_MAX_BW 100000000 /* 100GBit/sec */

enum {
	/** Eligible function for dequeue disable (the node is not eligible)
	*  - the last function in eligible function array - reserved for queues and nodes**/
	TM_ELIG_DEQ_DISABLE = 63,
	/** The size of  eligible functions array  **/
};

#define FIXED_PRIORITY		0
#define MINTB_SHAPING		1
#define MINMAXTB_SHAPING	2

#define PRIO_0		0
#define PRIO_1		1
#define PRIO_2		2
#define PRIO_3		3
#define PRIO_4		4
#define PRIO_5		5
#define PRIO_6		6
#define PRIO_7		7
#define PRIO_P		8

#define ENCODE_ELIGIBLE_FUN(type, prio)				(type * 10 + prio)
#define DECODE_ELIGIBLE_FUN(elig_fun, type, prio)	do { type = elig_fun / 10 ; prio = elig_fun % 10; } while (0)

#define IS_VALID_Q_TYPE_PRIO(type, prio)			((type <= MINMAXTB_SHAPING) && (prio <= PRIO_7))
#define IS_VALID_N_TYPE_PRIO(type, prio)			((type <= MINMAXTB_SHAPING) && (prio <= PRIO_P))




enum elig_func_node {
	/** Eligible function priority 0 **/
	TM_ELIG_N_FIXED_P0 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_0),
	/** Eligible function priority 1 **/
	TM_ELIG_N_FIXED_P1 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_1),
	/** Eligible function priority 2 **/
	TM_ELIG_N_FIXED_P2 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_2),
	/** Eligible function priority 3 **/
	TM_ELIG_N_FIXED_P3 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_3),
	/** Eligible function priority 4 **/
	TM_ELIG_N_FIXED_P4 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_4),
	/** Eligible function priority 5 **/
	TM_ELIG_N_FIXED_P5 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_5),
	/** Eligible function priority 6 **/
	TM_ELIG_N_FIXED_P6 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_6),
	/** Eligible function priority 7 **/
	TM_ELIG_N_FIXED_P7 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_7),
	/** Eligible function priority propagated **/
	TM_ELIG_N_FIXED_PP = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_P),

	/** Eligible function min shaping priority 0 **/
	TM_ELIG_N_MIN_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_0),
	/** Eligible function min shaping priority 1 **/
	TM_ELIG_N_MIN_SHP_P1 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_1),
	/** Eligible function min shaping priority 2 **/
	TM_ELIG_N_MIN_SHP_P2 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_2),
	/** Eligible function min shaping priority 3 **/
	TM_ELIG_N_MIN_SHP_P3 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_3),
	/** Eligible function min shaping priority 4 **/
	TM_ELIG_N_MIN_SHP_P4 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_4),
	/** Eligible function min shaping priority 5 **/
	TM_ELIG_N_MIN_SHP_P5 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_5),
	/** Eligible function min shaping priority 6 **/
	TM_ELIG_N_MIN_SHP_P6 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_6),
	/** Eligible function min shaping priority 7 **/
	TM_ELIG_N_MIN_SHP_P7 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_7),
	/** Eligible function min shaping priority propagated **/
	TM_ELIG_N_MIN_SHP_PP = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_P),

	/** Eligible function min shaping priority 0, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P0_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_0),
	/** Eligible function min shaping priority 1, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P1_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_1),
	/** Eligible function min shaping priority 2, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P2_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_2),
	/** Eligible function min shaping priority 3, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P3_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_3),
	/** Eligible function min shaping priority 4, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P4_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_4),
	/** Eligible function min shaping priority 5, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P5_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_5),
	/** Eligible function min shaping priority 6, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P6_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_6),
	/** Eligible function min shaping priority 7, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_P7_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_7),
	/** Eligible function min shaping priority propagated, max shaping priority 0 **/
	TM_ELIG_N_SHP_MIN_SHP_PP_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_P),

	TM_ELIG_N_DEFAULT = TM_ELIG_N_FIXED_P0


};


/** Eligible functions for queue nodes enumerator */
enum elig_func_queue {
	/** Eligible function priority 0 **/
	TM_ELIG_Q_FIXED_P0 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_0),
	/** Eligible function priority 1 **/
	TM_ELIG_Q_FIXED_P1 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_1),
	/** Eligible function priority 2 **/
	TM_ELIG_Q_FIXED_P2 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_2),
	/** Eligible function priority 3 **/
	TM_ELIG_Q_FIXED_P3 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_3),
	/** Eligible function priority 4 **/
	TM_ELIG_Q_FIXED_P4 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_4),
	/** Eligible function priority 5 **/
	TM_ELIG_Q_FIXED_P5 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_5),
	/** Eligible function priority 6 **/
	TM_ELIG_Q_FIXED_P6 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_6),
	/** Eligible function priority 7 **/
	TM_ELIG_Q_FIXED_P7 = ENCODE_ELIGIBLE_FUN(FIXED_PRIORITY, PRIO_7),

	/** Eligible function min shaping priority 0 **/
	TM_ELIG_Q_MIN_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_0),
	/** Eligible function min shaping priority 1 **/
	TM_ELIG_Q_MIN_SHP_P1 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_1),
	/** Eligible function min shaping priority 2 **/
	TM_ELIG_Q_MIN_SHP_P2 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_2),
	/** Eligible function min shaping priority 3 **/
	TM_ELIG_Q_MIN_SHP_P3 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_3),
	/** Eligible function min shaping priority 4 **/
	TM_ELIG_Q_MIN_SHP_P4 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_4),
	/** Eligible function min shaping priority 5 **/
	TM_ELIG_Q_MIN_SHP_P5 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_5),
	/** Eligible function min shaping priority 6 **/
	TM_ELIG_Q_MIN_SHP_P6 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_6),
	/** Eligible function min shaping priority 7 **/
	TM_ELIG_Q_MIN_SHP_P7 = ENCODE_ELIGIBLE_FUN(MINTB_SHAPING, PRIO_7),

	/** Eligible function min shaping priority 0, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P0_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_0),
	/** Eligible function min shaping priority 1, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P1_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_1),
	/** Eligible function min shaping priority 2, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P2_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_2),
	/** Eligible function min shaping priority 3, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P3_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_3),
	/** Eligible function min shaping priority 4, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P4_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_4),
	/** Eligible function min shaping priority 5, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P5_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_5),
	/** Eligible function min shaping priority 6, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P6_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_6),
	/** Eligible function min shaping priority 7, max shaping priority 0 **/
	TM_ELIG_Q_SHP_MIN_SHP_P7_MAX_SHP_P0 = ENCODE_ELIGIBLE_FUN(MINMAXTB_SHAPING, PRIO_7),

	TM_ELIG_Q_DEFAULT = TM_ELIG_Q_FIXED_P0
};

/*---------------------- Enumerated Types---------------- */

/** TM levels */
enum tm_level {
	Q_LEVEL = 0, /**< Queue Level */
	A_LEVEL,     /**< A-nodes Level */
	B_LEVEL,     /**< B-nodes Level */
	C_LEVEL,     /**< C-nodes Level */
	P_LEVEL      /**< Ports Level */
};

/** Port's physical bandwidth */
enum tm_port_bw {
	TM_1G_PORT = 0, /**< 1G bit/sec */
	TM_2HG_PORT,    /**< 2.5G bit/sec*/
	TM_10G_PORT,    /**< 10G bit/sec */
	TM_40G_PORT,    /**< 40G bit/sec */
	TM_50G_PORT,    /**< 50G bit/sec */
	TM_100G_PORT    /**< 100G bit/sec */
};

#ifdef MV_QMTM_NOT_NSS
/** Token bucket usage */
enum token_bucket {
	MIN_TOKEN_BUCKET = 0, /**< Use Min token bucket */
	MAX_TOKEN_BUCKET      /**< Use Max tiken bucket */
};

/** TM Port Basic Periodic update rate */
enum tm_port_update_rate {
	TM_PORT_RATE_1M = 1, /**< 1MHz */
	TM_MIN_PORT_RATE = TM_PORT_RATE_1M, /**< Port Minimum update rate */
	TM_PORT_RATE_2M,     /**< 2MHz */
	TM_PORT_RATE_3M,     /**< 3MHz */
	TM_PORT_RATE_4M,     /**< 4MHz */
	TM_PORT_RATE_5M,     /**< 5MHz */
	TM_MAX_PORT_RATE = TM_PORT_RATE_5M  /**< Port Maximum update rate */
};
#endif
/** Drop WRED/CATD mode */
enum tm_drop_mode {
	WRED = 0, /**< WRED */
	CATD,     /**< Color Aware TD */
	DISABLED  /**< Both modes are disabled */
};

/** TM2TM channel */
enum tm2tm_channel {
	TM2TM_NODE_CH = 0,
	TM2TM_PORT_CH
};

#ifdef MV_QMTM_NOT_NSS
/** TM2TM Port/Node channel's mode */
enum tm2tm_mode {
TM2TM_WRED = 0,
TM2TM_BP
};
#endif
/** Number of colors */
enum tm_color_num {
	TM_1_COLORS = 0,
	TM_2_COLORS,
	TM_3_COLORS
};

/** Drop Probability Source */
enum tm_dp_source {
	TM_AQL = 0,
	TM_QL
};

/*------------------- Global Paramteres Data structures-----------------*/

/** Drop configuration profile */
struct tm_drop_profile_params {
	uint32_t wred_catd_bw;     /**< WRED/Color Aware TD BW in Kbits/sec */
	uint32_t cbtd_bw;          /**< Color Blind TD BW in Kbits/sec */
	uint32_t cbtd_rtt_ratio;   /**< Color Blind TD BW ratio from RTT in % */
	uint8_t aql_exp;           /**< Forget factor exponent */

	enum tm_drop_mode wred_catd_mode;   /**< Color Aware TD/WRED */
	uint8_t curve_id[3];       /**< RED curve index per color[0..2] */
	uint8_t dp_ratio[3];       /**< DP ratio per color[0..2] */
	uint16_t min_th[3];        /**< Min Threshold ratio from RTT in % per color */
	uint16_t max_th[3];        /**< Max Threshold ratio from RTT in % per color */
};

/*----------------- Nodes Parameters Data Structures---------------*/
/* Note: only drop mode 0 is supported in current version.
 *  Drop profile reference fields are present only for queues and
 *  ports
*/

/** Queue Parameters Data Structure */
struct tm_queue_params {
	uint16_t quantum;              /**< Queue DWRR Quantum in TM_NODE_QUANTUM_UNIT */
	uint8_t wred_profile_ref;     /**< Index of Drop profile */
	uint8_t elig_prio_func_ptr;    /**< Eligible Priority Function pointer */
};


/** A-Node Parameters Data Structure */
struct tm_a_node_params {
	uint16_t quantum;              /**< DWRR Quantum in TM_NODE_QUANTUM_UNIT */
	uint8_t dwrr_priority[8];      /**< DWRR Priority for Queue Scheduling */
	uint8_t wred_profile_ref;     /**< Index of Drop profile */
	uint8_t elig_prio_func_ptr;    /**< Eligible Priority Function pointer */
	uint32_t num_of_children;      /**< Number of children nodes */
};


/** B-Node Parameters Data Structure */
struct tm_b_node_params {
	uint16_t quantum;              /**< DWRR Quantum in TM_NODE_QUANTUM_UNIT */
	uint8_t dwrr_priority[8];      /**< DWRR Priority for A-Node Scheduling */
	uint8_t wred_profile_ref;      /**< Index of Drop profile */
	uint8_t elig_prio_func_ptr;    /**< Eligible Priority Function pointer */
	uint16_t num_of_children;      /**< Number of children nodes */
};


/** C-Node Parameters Data Structure */
struct tm_c_node_params {
	uint16_t quantum;              /**< DWRR Quantum in TM_NODE_QUANTUM_UNIT */
	uint8_t dwrr_priority[8];      /**< DWRR Priority for B-Node Scheduling */
	uint8_t wred_cos;              /**< C-node CoS bit map for WRED */
	uint8_t wred_profile_ref[8];   /**< Index of Drop profile per CoS */
	uint8_t elig_prio_func_ptr;    /**< Eligible Priority Function pointer */
	uint16_t num_of_children;      /**< Number of children nodes */
};


/** Port Parameters Data Structure */
struct tm_port_params {
#ifdef MV_QMTM_NSS_A0
	uint16_t quantum[8];           /**< DWRR Quantum for each instance in TM_PORT_QUANTUM_UNIT */
#endif
	uint8_t dwrr_priority[8];      /**< DWRR Priority for C-Node Scheduling */
	uint8_t wred_profile_ref;      /**< Index of Drop profile */
	uint8_t elig_prio_func_ptr;    /**< Eligible Priority Function pointer */
	uint16_t num_of_children;      /**< Number of children nodes */
};

struct tm_port_drop_per_cos {
	uint8_t wred_cos;              /**< Port CoS bit map for WRED */
	uint8_t wred_profile_ref[8];   /**< Index of Drop profile per CoS */
};


/** Port status data structure */
struct tm_port_status {
	uint32_t max_bucket_level;  /**< Maximal Shaper Bucket level */
	uint32_t min_bucket_level;  /**< Minimal Shaper Bucket level */
	uint32_t deficit[8];        /**< DWRR Deficit per instance */
};


/** Node status data structure */
struct tm_node_status {
	uint32_t max_bucket_level;  /**< Maximal Shaper Bucket level */
	uint32_t min_bucket_level;  /**< Minimal Shaper Bucket level */
	uint32_t deficit;           /**< DWRR Deficit */
};

/** QMR Packet Statistics data structure */
/*
struct tm_qmr_pkt_statistics {
	uint64_t num_pkts_to_unins_queue;   *//**< Pkts from SMS that have arrived
											* to not installed Queue *//*
};


*//** RCB Packet Statistics data structure *//*
struct tm_rcb_pkt_statistics {
	uint64_t num_pkts_to_sms;            *//**< Non-error pkts that are passed to SMS *//*
	uint64_t num_crc_err_pkts_to_sms;    *//**< Pkts with CRC error *//*
	uint64_t num_errs_from_sms_to_dram;  *//**< Pkts with error from SMS that has
										* been written to DRAM *//*
};
*/

/** TM Blocks Error Information */
struct tm_error_info {
	uint16_t error_counter; /**< TM Block Error counter */
	uint16_t exception_counter; /**< TM Block Exception Counter */
};


/** TM2TM External Headers */
struct tm_ext_hdr {
	uint8_t size;               /**< only fixed values - 3, 7, 11 or 15 */
	uint8_t	content[32];		/**< header data */
};

/** TM2TM Control Packet Structure */
struct tm_ctrl_pkt_str {
	uint8_t ports;	/**< Ports */
	uint8_t nodes;	/**< Nodes */
};

/** TM2TM Delta Range Mapping to Priority */
struct tm_delta_range {
	uint8_t upper_range0;	/**< Range 0 */
	uint8_t upper_range1;	/**< Range 1 */
	uint8_t upper_range2;	/**< Range 2 */
};

/** Reshuffling index/range change structure */
struct tm_tree_change {
	uint8_t type;   /**< Type of change: index - TM_ENABLE, range - TM_DISABLE */
	uint32_t index; /**< Index of changed parent node */
	uint32_t old_index;   /**< Old index/range */
	uint32_t new_index;   /**< New index/range */
	struct tm_tree_change *next; /**< Pointer to the next change */
};

/** Eligible Priority Function Data structures */
struct tm_elig_prio_func_out {
	uint8_t max_tb;             /**< Use Max Token Bucket   */
	uint8_t min_tb;             /**< Use Min Token Bucket   */
	uint8_t prop_prio;          /**< Propagated priority    */
	uint8_t sched_prio;         /**< Scheduling priority    */
	uint8_t elig;               /**< Eligibility            */
};

/** Eligible Priority Function storage */
union tm_elig_prio_func {
	struct tm_elig_prio_func_out queue_elig_prio_func[4];		/**< Eligible Priority Functions for queues   */
	struct tm_elig_prio_func_out node_elig_prio_func[8][4];		/**< Eligible Priority Functions for intermediate nodes   */
};


#endif   /* TM_DEFS_H */

