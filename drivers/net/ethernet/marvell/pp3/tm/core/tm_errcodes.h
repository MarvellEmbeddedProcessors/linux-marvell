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

#ifndef   	TM_ERRCODES_H
#define   	TM_ERRCODES_H

/** HW error codes */
enum tm_hw_err_codes
{
	TM_HW_GEN_CONFIG_FAILED = 1,            /**< 1 */
	TM_HW_WRED_CURVE_FAILED,                /**< 2 */
	TM_HW_DROP_PROFILE_FAILED,              /**< 3 */
	TM_HW_CONF_PER_SCHEME_FAILED,           /**< 4 */
	TM_HW_TREE_CONFIG_FAIL,                 /**< 5 */
	TM_HW_AGING_CONFIG_FAIL,                /**< 6 */
	TM_HW_PORT_CONFIG_FAIL,                 /**< 7 */
	TM_HW_C_NODE_CONFIG_FAIL,               /**< 8 */
	TM_HW_B_NODE_CONFIG_FAIL,               /**< 9 */
	TM_HW_A_NODE_CONFIG_FAIL,               /**< 10 */
	TM_HW_QUEUE_CONFIG_FAIL,				/**< 11 */
	TM_HW_CHANGE_SHAPING_STATUS_FAILED,     /**< 12 */
	TM_HW_PORT_DWRR_BYTES_PER_BURST_FAILED, /**< 13 */
	TM_HW_QMR_GET_ERRORS_FAILED,            /**< 14 */
	TM_HW_BAP_GET_ERRORS_FAILED,            /**< 15 */
	TM_HW_RCB_GET_ERRORS_FAILED,            /**< 16 */
	TM_HW_SCHED_GET_ERRORS_FAILED,          /**< 17 */
	TM_HW_DROP_GET_ERRORS_FAILED,           /**< 18 */
	TM_HW_SHAPING_PROF_FAILED,              /**< 19 */
	TM_HW_READ_PORT_STATUS_FAIL,            /**< 20 */
	TM_HW_READ_C_NODE_STATUS_FAIL,          /**< 21 */
	TM_HW_READ_B_NODE_STATUS_FAIL,          /**< 22 */
	TM_HW_READ_A_NODE_STATUS_FAIL,          /**< 23 */
	TM_HW_READ_QUEUE_STATUS_FAIL,           /**< 24 */
	TM_HW_GET_QUEUE_LENGTH_FAIL,            /**< 25 */
	TM_HW_GET_QMR_PKT_STAT_FAILED,          /**< 26 */
	TM_HW_GET_RCB_PKT_STAT_FAILED,          /**< 27 */
	TM_HW_SET_SMS_PORT_ATTR_FAILED,         /**< 28 */
	TM_HW_ELIG_PRIO_FUNC_FAILED,            /**< 29 */
	TM_HW_AQM_CONFIG_FAIL,                  /**< 30 */
	TM_HW_COLOR_NUM_CONFIG_FAIL,            /**< 31 */
	TM_HW_DP_QUERY_RESP_CONF_FAILED,        /**< 32 */
	TM_HW_QUEUE_COS_CONF_FAILED,            /**< 33 */
	TM_HW_TM2TM_GLOB_CONF_FAILED,           /**< 34 */
	TM_HW_TM2TM_CHANNEL_CONF_FAILED,        /**< 35 */
	TM_HW_TM2TM_LC_CONF_FAILED,             /**< 36 */
	TM_HW_TM2TM_ENABLE_FAILED,              /**< 37 */

	TM_HW_MAX_ERROR						/**< 38 */
};

/** SW (configuration) error codes */
enum tm_conf_err_codes
{
	TM_CONF_INVALID_PROD_NAME = TM_HW_MAX_ERROR + 1, /**< 39 */ /* GT_BAD_VALUE */
	TM_CONF_PER_RATE_L_K_N_NOT_FOUND,      /**< 40 */ /* GT_BAD_VALUE */
	TM_CONF_RES_INC_BW_TS_NOT_FOUND,       /**< 41 */ /* GT_BAD_VALUE */
	TM_CONF_RES_ACC_NOT_FOUND,             /**< 42 */ /* GT_BAD_VALUE */
	TM_CONF_NON_RES_ACC_NOT_FOUND,         /**< 43 */ /* GT_BAD_VALUE */
	TM_CONF_RES_INC_BW_TS_LESS_ONE,        /**< 44 */ /* GT_BAD_VALUE */
	TM_CONF_BANK_UPD_RATE_NOT_FOUND,       /**< 45 */ /* GT_BAD_VALUE */
	TM_CONF_UPD_RATE_NOT_FOUND,            /**< 46 */ /* GT_BAD_VALUE */
	TM_CONF_NON_RES_INC_BW_TS_LESS_ONE,    /**< 47 */ /* GT_BAD_VALUE */
	TM_CONF_PORT_IND_OOR,                  /**< 48 */ /* GT_OUT_OF_RANGE */
	TM_CONF_PORT_QUANTUM_OOR,              /**< 49 */ /* GT_OUT_OF_RANGE */
	TM_CONF_PORT_DWRR_PRIO_OOR,            /**< 50 */ /* GT_OUT_OF_RANGE */
	TM_CONF_P_WRED_PROF_REF_OOR,           /**< 51 */ /* GT_OUT_OF_RANGE */
	TM_CONF_PORT_BW_OOR,                   /**< 52 */ /* GT_OUT_OF_RANGE */
	TM_CONF_PORT_BS_OOR,                   /**< 53 */ /* GT_OUT_OF_RANGE */
	TM_CONF_Q_SHAPING_PROF_REF_OOR,        /**< 54 */ /* GT_OUT_OF_RANGE */
	TM_CONF_Q_QUANTUM_OOR,                 /**< 55 */ /* GT_OUT_OF_RANGE */
	TM_CONF_Q_WRED_PROF_REF_OOR,           /**< 56 */ /* GT_OUT_OF_RANGE */
	TM_CONF_A_NODE_IND_OOR,                /**< 57 */ /* GT_OUT_OF_RANGE */
	TM_CONF_A_SHAPING_PROF_REF_OOR,        /**< 58 */ /* GT_OUT_OF_RANGE */
	TM_CONF_A_QUANTUM_OOR,                 /**< 59 */ /* GT_OUT_OF_RANGE */
	TM_CONF_A_DWRR_PRIO_OOR,               /**< 60 */ /* GT_OUT_OF_RANGE */
	TM_CONF_A_WRED_PROF_REF_OOR,           /**< 61 */ /* GT_OUT_OF_RANGE */
	TM_CONF_B_NODE_IND_OOR,                /**< 62 */ /* GT_OUT_OF_RANGE */
	TM_CONF_B_SHAPING_PROF_REF_OOR,        /**< 63 */ /* GT_OUT_OF_RANGE */
	TM_CONF_B_QUANTUM_OOR,                 /**< 64 */ /* GT_OUT_OF_RANGE */
	TM_CONF_B_DWRR_PRIO_OOR,               /**< 65 */ /* GT_OUT_OF_RANGE */
	TM_CONF_B_WRED_PROF_REF_OOR,           /**< 66 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_NODE_IND_OOR,                /**< 67 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_SHAPING_PROF_REF_OOR,        /**< 68 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_QUANTUM_OOR,                 /**< 69 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_DWRR_PRIO_OOR,               /**< 70 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_WRED_PROF_REF_OOR,           /**< 71 */ /* GT_OUT_OF_RANGE */
	TM_CONF_C_WRED_COS_OOR,                /**< 72 */ /* GT_OUT_OF_RANGE */
	TM_CONF_ELIG_PRIO_FUNC_ID_OOR,         /**< 73 */ /* GT_OUT_OF_RANGE */
	TM_CONF_DP_COS_SEL_OOR,                /**< 74 */ /* GT_OUT_OF_RANGE */
	TM_CONF_EXT_HDR_SIZE_OOR,              /**< 75 */ /* GT_OUT_OF_RANGE */
	TM_CONF_CTRL_PKT_STR_OOR,              /**< 76 */ /* GT_OUT_OF_RANGE */
	TM_CONF_DELTA_RANGE_OOR,               /**< 77 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_EGR_ELEMS_OOR,           /**< 78 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_SRC_LVL_OOR,             /**< 79 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_BP_LVL_OOR,              /**< 80 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_BP_THRESH_OOR,           /**< 81 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_DP_LVL_OOR,              /**< 82 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_CTRL_HDR_OOR,            /**< 83 */ /* GT_OUT_OF_RANGE */
	TM_CONF_TM2TM_PORT_FOR_CTRL_PKT_OOR,   /**< 84 */ /* GT_OUT_OF_RANGE */
	TM_CONF_PORT_SPEED_OOR,                /**< 85 */ /* GT_BAD_PARAM */
	TM_CONF_PORT_MIN_SHAP_NOT_INC_BW_MULT, /**< 86 */ /* GT_BAD_PARAM */
	TM_CONF_PORT_MAX_SHAP_NOT_INC_BW_MULT, /**< 87 */ /* GT_BAD_PARAM */
	TM_CONF_PORT_SHAP_MAX_NOT_MULT_MIN,    /**< 88 */ /* GT_BAD_PARAM */
	TM_CONF_PORT_BW_OUT_OF_SPEED,          /**< 89 */ /* GT_BAD_PARAM */
	TM_CONF_INVALID_NUM_OF_C_NODES,        /**< 90 */ /* GT_BAD_PARAM */
	TM_CONF_INVALID_NUM_OF_B_NODES,        /**< 91 */ /* GT_BAD_PARAM */
	TM_CONF_INVALID_NUM_OF_A_NODES,        /**< 92 */ /* GT_BAD_PARAM */
	TM_CONF_INVALID_NUM_OF_QUEUES,         /**< 93 */ /* GT_BAD_PARAM */
	TM_CONF_LVL_MIN_BW_VIOLATION,          /**< 94 */ /* GT_BAD_PARAM */
	TM_CONF_LVL_MAX_BW_VIOLATION,          /**< 95 */ /* GT_BAD_PARAM */
	TM_CONF_LVL_MIN_INC_VIOLATION,         /**< 96 */ /* GT_BAD_PARAM */
	TM_CONF_LVL_MIN_NOT_MULT_INC,          /**< 97 */ /* GT_BAD_PARAM */
	TM_CONF_LVL_MAX_NOT_MULT_INC,          /**< 98 */ /* GT_BAD_PARAM */
	TM_CONF_TM2TM_AQM_INVALID_COLOR_NUM,   /**< 99 */ /* GT_BAD_PARAM */
	TM_CONF_MIN_SHAP_NOT_INC_BW_MULT,      /**< 100 */ /* GT_BAD_PARAM */
	TM_CONF_MAX_SHAP_NOT_INC_BW_MULT,      /**< 101 */ /* GT_BAD_PARAM */
	TM_CONF_SHAP_MAX_NOT_MULT_MIN,         /**< 102 */ /* GT_BAD_PARAM */
	TM_CONF_SHAP_MIN_NOT_MULT_MIN,         /**< 103 */ /* GT_BAD_PARAM */
	TM_CONF_REORDER_NODES_NOT_ADJECENT,    /**< 104 */ /* GT_BAD_PARAM */
	TM_CONF_MIN_TOKEN_TOO_LARGE,           /**< 105 */ /* GT_BAD_SIZE */
	TM_CONF_MAX_TOKEN_TOO_LARGE,           /**< 106 */ /* GT_BAD_SIZE */
	TM_CONF_PORT_MIN_TOKEN_TOO_LARGE,      /**< 107 */ /* GT_BAD_SIZE */
	TM_CONF_PORT_MAX_TOKEN_TOO_LARGE,      /**< 108 */ /* GT_BAD_SIZE */
	TM_CONF_MAX_BW_TS_TOO_LARGE,           /**< 109 */ /* GT_BAD_SIZE */
	TM_CONF_REORDER_CHILDREN_NOT_AVAIL,    /**< 110 */ /* GT_BAD_SIZE */
	TM_CONF_PORT_IND_NOT_EXIST,            /**< 111 */ /* GT_BAD_STATE */
	TM_CONF_A_NODE_IND_NOT_EXIST,          /**< 112 */ /* GT_BAD_STATE */
	TM_CONF_B_NODE_IND_NOT_EXIST,          /**< 113 */ /* GT_BAD_STATE */
	TM_CONF_C_NODE_IND_NOT_EXIST,          /**< 114 */ /* GT_BAD_STATE */
	TM_CONF_CANNT_GET_LAD_FREQUENCY,       /**< 115 */ /* GT_GET_ERROR/? possibly not rel for CPSS */
	TM_CONF_UPD_RATE_NOT_CONF_FOR_LEVEL,   /**< 116 */ /* GT_NOT_INITIALIZED */
	TM_CONF_TM2TM_CHANNEL_NOT_CONFIGURED,  /**< 117 */ /* GT_NOT_INITIALIZED */
	TM_CONF_PORT_IND_USED,                 /**< 118 */ /* GT_ALREADY_EXIST */
	TM_CONF_NULL_LOGICAL_NAME,             /**< 119 */ /* GT_BAD_VALUE */
	TM_CONF_WRONG_LOGICAL_NAME,            /**< 120 */ /* GT_BAD_VALUE */
	TM_CONF_MAX_ERROR					   /**<121 */
};


#endif   /* TM_ERRCODES_H */

