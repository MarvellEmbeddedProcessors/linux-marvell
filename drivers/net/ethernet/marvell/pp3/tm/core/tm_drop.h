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

#ifndef   	TM_DROP_H
#define   	TM_DROP_H

#include "tm_core_types.h"	 /* in order to define tm_handle */


/** Create a WRED curve for a level.
 * @param[in]	        	hndl	      TM lib handle
 * @param[in]               level         A nodes level the WRED curve is created for
 * @param[in]               cos	          CoS of RED Curve
 * @param[in]               prob	      Array of 32 probability points (0-100%)
 * @param[out]              curve_index   An index of a created WRED curve
 *
 * @note The cos parameter is relevant for C lvl only, else set TM_INVAL
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL        if hndl is NULL
 * @retval -EBADF         if hndl is an invalid handle
 * @retval -EADDRNOTAVAIL if level is invalid
 * @retval -EPERM         if one of probabilities is out of range
 * @retval -ENOSPC        if the WRED Curves table is fully allocated
 * @retval -EBADMSG       if AQM Mode params are not configured for this lvl
 * @retval TM_HW_WRED_CURVE_FAILED if download to HW fails
 */
int tm_create_wred_curve(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint8_t *prob,
						uint8_t *curve_index);


/** Create a WRED traditional curve for a level.
 * @param[in]	        	hndl	      TM lib handle
 * @param[in]               level         A nodes level the WRED curve is created for
 * @param[in]               cos	          CoS of RED Curve
 * @param[in]               mp  	      Non zero Max probability
 * @param[out]              curve_index   An index of a created WRED curve
 *
 * @note The cos parameter is relevant for C lvl only, else set TM_INVAL.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL        if hndl is NULL
 * @retval -EBADF         if hndl is an invalid handle
 * @retval -EADDRNOTAVAIL if level is invalid
 * @retval -ENOSPC        if the WRED Curves table is fully allocated
 * @retval -EPERM         if max probability is not valid
 * @retval -EBADMSG       if AQM Mode params are not configured for this lvl
 * @retval TM_HW_WRED_CURVE_FAILED if download to HW fails
 */
int tm_create_wred_traditional_curve(tm_handle hndl,
									 enum tm_level level,
									 uint8_t cos,
									 uint8_t mp,
									 uint8_t *curve_index);

/** Create a WRED flat curve for a level.
 * @param[in]               hndl          TM lib handle
 * @param[in]               level         A nodes level the WRED curve is created for
 * @param[in]               cos           CoS of RED Curve
 * @param[in]               cp            Non zero probability
 * @param[out]              curve_index   An index of a created WRED curve
 *
 * @note The cos parameter is relevant for C lvl only, else set TM_INVAL.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL        if hndl is NULL
 * @retval -EBADF         if hndl is an invalid handle
 * @retval -EADDRNOTAVAIL if level is invalid
 * @retval -ENOSPC        if the WRED Curves table is fully allocated
 * @retval -EPERM         if curve probability is not valid
 * @retval -EBADMSG       if AQM Mode params are not configured for this lvl
 * @retval TM_HW_WRED_CURVE_FAILED if download to HW fails
 */
int tm_create_wred_flat_curve(tm_handle hndl,
									 enum tm_level level,
									 uint8_t cos,
									 uint8_t cp,
									 uint8_t *curve_index);


/** Create Drop profile
 * @param[in]       hndl         TM lib handle
 * @param[in]       level        A nodes level to create a Drop profile for
 * @param[in]       cos          CoS
 * @param[in]       profile      Drop Profile configuration structure pointer
 * @param[out]      prof_index   An index of the created Drop profile
 *
 * @note In case of Color Blind TD only set wred_catd_bw=0 and wred_catd_mode=CBTD_ONLY.
 * @note In case of Color Blind TD disabled set cbtd_bw=TM_MAX_BW, cbtd_rtt_ratio=0.
 * @note Cos of Drop Profile matches Cos of given curve.
 * @note The CoS parameter is relevant for C and P level only,
 *       else set TM_INVAL.
 * @note For P level in Global mode set 'cos' = TM_INVAL, else
 *       profile will be created for CoS mode.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL         if either hndl or profile is NULL
 * @retval -EBADF          hndl is an invalid handle
 * @retval -EPERM          if the level is invalid
 * @retval -EACCES         if the wred_catd_bw or cbtd_bw is out of range
 * @retval -EDOM           if wred/catd are not configured correctly
 * @retval -EDOM           if cos in the profile is out of range
 * @retval -ENODEV         if RED curve doesn't exist
 * @retval -EFAULT         if exponents or index in the profile are out of range
 * @retval -ENODATA        if wred_catd_mode is invalid
 * @retval -ERANGE         if max_th is less that min_th
 * @retval -ENOSPC         if level's Drop Profiles table is fully allocated
 * @retval -EBADMSG        if AQM Mode params are not configured for this lvl
 * @retval TM_HW_DROP_PROFILE_FAILED if download to HW fails
*/
int tm_create_drop_profile(tm_handle hndl,
						   enum tm_level level,
						   uint8_t cos,
						   struct tm_drop_profile_params *profile,
						   uint16_t *prof_index);


/** Create 1G CBTD Drop profile
 * @param[in] 		hndl      	 TM lib handle
 * @param[in]       level        A nodes level to create a Drop profile for
 * @param[in]       cos	         CoS
 * @param[out]      prof_index	 An index of the created Drop profile
 *
 * @note The CoS parameter is relevant for C and P level only,
 *       else set TM_INVAL.
 * @note For P level in Global mode set 'cos' = TM_INVAL, else
 *       profile will be created for CoS mode.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL         if either hndl or profile is NULL
 * @retval -EBADF          hndl is an invalid handle
 * @retval -EPERM          if the level is invalid
 * @retval -EDOM           if cos in the profile is out of range
 * @retval -ENOSPC         if level's Drop Profiles table is fully allocated
 * @retval TM_HW_DROP_PROFILE_FAILED if download to HW fails
*/
int tm_create_drop_profile_1G(tm_handle hndl,
						   enum tm_level level,
						   uint8_t cos,
						   uint16_t *prof_index);


/** Create 2.5G CBTD Drop profile
 * @param[in] 		hndl      	 TM lib handle
 * @param[in]       level        A nodes level to create a Drop profile for
 * @param[in]       cos	         CoS
 * @param[out]      prof_index	 An index of the created Drop profile
 *
 * @note The CoS parameter is relevant for C and P level only,
 *       else set TM_INVAL.
 * @note For P level in Global mode set 'cos' = TM_INVAL, else
 *       profile will be created for CoS mode.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL         if either hndl or profile is NULL
 * @retval -EBADF          hndl is an invalid handle
 * @retval -EPERM          if the level is invalid
 * @retval -EDOM           if cos in the profile is out of range
 * @retval -ENOSPC         if level's Drop Profiles table is fully allocated
 * @retval TM_HW_DROP_PROFILE_FAILED if download to HW fails
*/
int tm_create_drop_profile_2_5G(tm_handle hndl,
						   enum tm_level level,
						   uint8_t cos,
						   uint16_t *prof_index);


/** Delete Drop profile
 * @param[in] 	   hndl 	    TM lib handle
 * @param[in]      level        A nodes level to delete a Drop profile for
 * @param[in]      cos	        CoS
 * @param[in]	   prof_index	An index to a profile to be deleted
 *
 * @note The CoS parameter is relevant for C and P level
 *      only, else set TM_INVAL.
 * @note For P level in Global mode set 'cos' = TM_INVAL, else
 *       profile will be deleted for CoS mode.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL          if hndl is NULL
 * @retval -EBADF           if hndl is an invalid handle
 * @retval -EDOM            if the level is invalid
 * @retval -EADDRNOTAVAIL   if prof_index is out of range
 * @retval -EPERM           if profile is in use or reserved
 * @retval -EFAULT          if no existing profile with this index
 * @retval TM_HW_DROP_PROFILE_FAILED if download to HW fails
*/
int tm_delete_drop_profile(tm_handle hndl,
						   enum tm_level level,
						   uint8_t cos,
						   uint16_t prof_index);


/** Read Drop profile
 * @param[in] 	   hndl	        TM lib handle
 * @param[in]      level        A nodes level to read a Drop profile for
 * @param[in]      cos	        CoS
 * @param[in]	   prof_index	An index to a profile to be read out
 * @param[in]      profile      Drop profile configuration struct pointer
 *
 * @note The cbtd_rtt_ratio calculated aproximately from the register's values
 * @note The CoS parameter is relevant for C and P level only,
 *       else set TM_INVAL.
 * @note For P level in Global mode set 'cos' = TM_INVAL, else
 *       profile will be read for CoS mode.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL           if hndl is NULL
 * @retval -EBADF            if hndl is an invalid handle
 * @retval -EPERM            if the level is invalid
 * @retval -EADDRNOTAVAIL    if prof_index is out of range
 * @retval -EFAULT           if no existing profile with this index
 * @retval -EBADMSG          if AQM Mode params are not configured for this lvl
 */
int tm_read_drop_profile(tm_handle hndl,
						 enum tm_level level,
						 uint8_t cos,
						 uint16_t prof_index,
						 struct tm_drop_profile_params *profile);


/** Set Drop (Egress) Colors number per level.
 * @param[in] 	   hndl	        TM lib handle
 * @param[in]      lvl          A nodes level to set colors number for
 * @param[in]      num          Colors number
 *
 * @note This API should be called before all the rest Drop APIs (if
 * need). By default there are two colors per each level.
 * @note In case TM2TM usage, instead if this API should be called 'tm2tm_set_egress_drop_aqm_mode'.
 *
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL           if hndl is NULL
 * @retval -EBADF            if hndl is an invalid handle
 * @retval -EPERM            if level is invalid
 * @retval -EFAULT           if num is invalid
 * @retval TM_HW_COLOR_NUM_CONFIG_FAIL if download to HW fails
 */
int tm_set_drop_color_num(tm_handle hndl,
						  enum tm_level lvl,
						  enum tm_color_num num);


/**  Change Drop Probability (DP) source.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     lvl         A nodes level to set source for
 *   @param[in]     color       A color to set source for (0,1,2)
 *   @param[in]     source      QL or AQL (0 - use AQL, 1 - use QL to calculate DP)
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF  if hndl is invalid.
 *   @retval -EPERM  if level is invalid.
 *   @retval -ENODEV if color is invalid.
 *   @retval -EFAULT if status is out of range.
 *   @retval TM_HW_AQM_CONFIG_FAIL if download to HW fails
 */
int tm_dp_source_set(tm_handle hndl,
						enum tm_level lvl,
						uint8_t color,
						enum tm_dp_source source);


/** Drop Query Response Select.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     port_dp     0 - Global, 1 - CoS.
 *   @param[in]     lvl         Local response level (Q/A/B/C/Port).
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if port_dp is invalid.
 *   @retval -EPERM if lvl out of range.
 *   @retval TM_HW_DP_QUERY_RESP_CONF_FAILED if download to HW
 *           fails.
 */
int tm_set_drop_query_responce(tm_handle hndl,
							   uint8_t port_dp,
							   enum tm_level lvl);


/** Drop Queue Cos Select.
 *
 *   @param[in]     hndl        TM lib handle.
 *   @param[in]     index       Queue index.
 *   @param[in]     cos         Cos.
 *
 *   @return an integer return code.
 *   @retval zero on success.
 *   @retval -EINVAL if hndl is NULL.
 *   @retval -EBADF if hndl is invalid.
 *   @retval -EFAULT if index or cos is invalid.
 *   @retval TM_HW_QUEUE_COS_CONF_FAILED if download to HW fails.
 */
int tm_set_drop_queue_cos(tm_handle hndl,
						  uint32_t index,
						  uint8_t cos);


/****************************************************************************************/
/* functions for internal usage */

/** Set default Drop configuration (curves & profiles) per level in database
 * @param[in] 	   hndl	        TM lib handle
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL           if hndl is NULL
 * @retval -EBADF            if hndl is an invalid handle
 * @retval -ENOSPC           if level's Drop Profiles/WRED Curves table is fully allocated
 */
int _tm_config_default_drop_sw(tm_handle hndl);


/** Set default values to drop related registers
 * @param[in] 	   hndl	        TM lib handle
 * @return an integer return code
 * @retval 0 on success
 * @retval -EINVAL           if hndl is NULL
 * @retval -EBADF            if hndl is an invalid handle
 * @retval TM_HW_WRED_CURVE_FAILED if download to HW Curves fails
 * @retval TM_HW_DROP_PROFILE_FAILED if download to HW Drop Profiles fails
 */
int _tm_config_default_drop_hw(tm_handle hndl);


/* Predefined Drop profiles */
int tm_create_drop_profile_cbtd_100Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index);


int tm_create_drop_profile_wred_10Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index);


int tm_create_drop_profile_mixed_cbtd_100Mb_wred_10Mb(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t *prof_index);


int tm_update_drop_profile(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t index,
						struct tm_drop_profile_params *profile);


int tm_drop_profile_hw_set(tm_handle hndl,
						enum tm_level level,
						uint8_t cos,
						uint16_t index);


#endif   /* TM_DROP_H */
