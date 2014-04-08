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
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

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
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	*   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

	*   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

	*   Neither the name of Marvell nor the names of its contributors may be
	used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************************
* mv_cph_app.c
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) application module to implement
*              CPH main logic and handle application packets such as OMCI, eOAM,
*              IGMP packets.
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 22Jan2013
*
* FILE REVISION NUMBER:
*               Revision: 1.0
*
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_cph_header.h"


/******************************************************************************
* Variable Definition
******************************************************************************/
/* CPH global trace flag */
unsigned int g_cph_global_trace = CPH_ERR_LEVEL|CPH_WARN_LEVEL|CPH_INFO_LEVEL;

struct MV_ENUM_ENTRY_T g_enum_map_profile_id[] = {
	{ TPM_PON_WAN_DUAL_MAC_INT_SWITCH,  "TPM_PON_WAN_DUAL_MAC_INT_SWITCH"},
	{ TPM_PON_WAN_G0_INT_SWITCH,        "TPM_PON_WAN_G0_INT_SWITCH"},
	{ TPM_PON_WAN_G1_LAN_G0_INT_SWITCH, "TPM_PON_WAN_G1_LAN_G0_INT_SWITCH"},
	{ TPM_G0_WAN_G1_INT_SWITCH,         "TPM_G0_WAN_G1_INT_SWITCH"},
	{ TPM_G1_WAN_G0_INT_SWITCH,         "TPM_G1_WAN_G0_INT_SWITCH"},
	{ TPM_PON_G1_WAN_G0_INT_SWITCH,     "TPM_PON_G1_WAN_G0_INT_SWITCH"},
	{ TPM_PON_G0_WAN_G1_INT_SWITCH,     "TPM_PON_G0_WAN_G1_INT_SWITCH"},
	{ TPM_PON_WAN_DUAL_MAC_EXT_SWITCH,  "TPM_PON_WAN_DUAL_MAC_EXT_SWITCH"},
	{ TPM_PON_WAN_G1_MNG_EXT_SWITCH,    "TPM_PON_WAN_G1_MNG_EXT_SWITCH"},
	{ TPM_PON_WAN_G0_SINGLE_PORT,       "TPM_PON_WAN_G0_SINGLE_PORT"},
	{ TPM_PON_WAN_G1_SINGLE_PORT,       "TPM_PON_WAN_G1_SINGLE_PORT"},
	{ TPM_PON_G1_WAN_G0_SINGLE_PORT,    "TPM_PON_G1_WAN_G0_SINGLE_PORT"},
	{ TPM_PON_G0_WAN_G1_SINGLE_PORT,    "TPM_PON_G0_WAN_G1_SINGLE_PORTg"},
	{ TPM_PON_WAN_G0_G1_LPBK,           "TPM_PON_WAN_G0_G1_LPBK"},
	{ TPM_PON_WAN_G0_G1_DUAL_LAN,       "TPM_PON_WAN_G0_G1_DUAL_LAN"},
};

static struct MV_ENUM_ARRAY_T g_enum_array_profile_id = {
	sizeof(g_enum_map_profile_id)/sizeof(g_enum_map_profile_id[0]),
	g_enum_map_profile_id
};

static struct MV_ENUM_ENTRY_T g_enum_map_pon_type[] = {
	{ CPH_PON_TYPE_EPON, "EPON"},
	{ CPH_PON_TYPE_GPON, "GPON"},
	{ CPH_PON_TYPE_GBE,  "GBE"},
	{ CPH_PON_TYPE_P2P,  "P2P"},
};

static struct MV_ENUM_ARRAY_T g_enum_array_pon_type = {
	sizeof(g_enum_map_pon_type)/sizeof(g_enum_map_pon_type[0]),
	g_enum_map_pon_type
};

static struct MV_ENUM_ENTRY_T g_enum_map_dir[] = {
	{ CPH_DIR_US,       "US"},
	{ CPH_DIR_DS,       "DS"},
	{ CPH_DIR_NOT_CARE, "Not Care"},
};

static struct MV_ENUM_ARRAY_T g_enum_array_dir = {
	sizeof(g_enum_map_dir)/sizeof(g_enum_map_dir[0]),
	g_enum_map_dir
};

static struct MV_ENUM_ENTRY_T g_enum_map_rx_tx[] = {
	{ CPH_DIR_RX,         "RX"},
	{ CPH_DIR_TX,         "TX"},
	{ CPH_RX_TX_NOT_CARE, "Not Care"},
};

static struct MV_ENUM_ARRAY_T g_enum_array_rx_tx = {
	sizeof(g_enum_map_rx_tx)/sizeof(g_enum_map_rx_tx[0]),
	g_enum_map_rx_tx
};

static struct MV_ENUM_ENTRY_T g_enum_map_gmac[] = {
	{ MV_APP_GMAC_PORT_0,  "GMAC0"},
	{ MV_APP_GMAC_PORT_1,  "GMAC1"},
	{ MV_APP_PON_MAC_PORT, "PON MAC"},
};

static struct MV_ENUM_ARRAY_T g_enum_array_gmac = {
	sizeof(g_enum_map_gmac)/sizeof(g_enum_map_gmac[0]),
	g_enum_map_gmac
};


/******************************************************************************
* External Declaration
******************************************************************************/

/******************************************************************************
* Function Definition
******************************************************************************/
/******************************************************************************
* cph_app_set_complex_profile()
* _____________________________________________________________________________
*
* DESCRIPTION: Set TPM complex profile ID
*
* INPUTS:
*       profile_id   - TPM complex profile ID
*       active_port  - Active WAN port
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_set_complex_profile(enum tpm_eth_complex_profile_t profile_id, enum MV_APP_GMAC_PORT_E active_port)
{
	MV_STATUS rc = MV_OK;

	/* Check the range of profile_id */
	if (profile_id > TPM_PON_WAN_G0_G1_DUAL_LAN) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "profile_id[%d] is out of range[1~%d]\n",
			profile_id, TPM_PON_WAN_G0_G1_DUAL_LAN);
		return MV_OUT_OF_RANGE;
	}

	/* Check the range of active_port */
	if (active_port > MV_APP_PON_MAC_PORT) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "active_port[%d] is out of range[0~%d]\n",
			active_port, MV_APP_PON_MAC_PORT);
		return MV_OUT_OF_RANGE;
	}

	rc = cph_db_set_param(CPH_DB_PARAM_PROFILE_ID, &profile_id);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_set_param");

	rc = cph_db_set_param(CPH_DB_PARAM_ACTIVE_PORT, &active_port);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_set_param");

	return rc;
}

/******************************************************************************
* cph_app_set_feature_flag()
* _____________________________________________________________________________
*
* DESCRIPTION: Enable or disable feature support in CPH
*
* INPUTS:
*       feature - CPH supported features
*       state   - Enable or disable this feature in CPH
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_set_feature_flag(enum CPH_APP_FEATURE_E feature, bool state)
{
	MV_STATUS rc = MV_OK;

	switch (feature) {
	case CPH_APP_FEATURE_APP:
		cph_db_set_param(CPH_DB_PARAM_APP_SUPPORT, &state);
		break;
	case CPH_APP_FEATURE_IGMP:
		cph_db_set_param(CPH_DB_PARAM_IGMP_SUPPORT, &state);
		break;
	case CPH_APP_FEATURE_BC:
		cph_db_set_param(CPH_DB_PARAM_BC_SUPPORT, &state);
		break;
	case CPH_APP_FEATURE_FLOW:
		cph_db_set_param(CPH_DB_PARAM_FLOW_SUPPORT, &state);
		break;
	case CPH_APP_FEATURE_UDP:
		cph_db_set_param(CPH_DB_PARAM_UDP_SUPPORT, &state);
		break;
	default:
		break;
	}
	return rc;
}

/******************************************************************************
* cph_app_validate_parse_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Validate the parsing filed of CPH rule
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_validate_parse_field(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key)
{
	MV_STATUS rc = MV_OK;

	/* Check the range of parse_bm */
	if (parse_bm >= CPH_APP_PARSE_FIELD_END) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "parse_bm[0x%x] is out of range[0x01~0x%x]\n",
			parse_bm, CPH_APP_PARSE_FIELD_END);
		return MV_OUT_OF_RANGE;
	}

	/* Validate direction */
	if (parse_bm & CPH_APP_PARSE_FIELD_DIR) {
		if (parse_key->dir > CPH_DIR_NOT_CARE) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "dir[%d] is out of range[0~%d]\n",
				parse_key->dir, CPH_DIR_NOT_CARE);
			return MV_OUT_OF_RANGE;
		}
	}

	/* Validate RX/TX direction */
	if (parse_bm & CPH_APP_PARSE_FIELD_RX_TX) {
		if (parse_key->rx_tx > CPH_RX_TX_NOT_CARE) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "rx_tx[%d] is out of range[0~%d]\n",
				parse_key->rx_tx,
				CPH_RX_TX_NOT_CARE);
			return MV_OUT_OF_RANGE;
		}
	}

	/* Could not parse None IPv4 Eth type and IPv4 protocol type at the same ime */
	if ((parse_bm & CPH_APP_PARSE_FIELD_ETH_TYPE) &&
		(parse_bm & CPH_APP_PARSE_FIELD_IPV4_TYPE) &&
		(parse_key->eth_type != MV_CPH_ETH_TYPE_IPV4)) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "parse_bm[0x%x], eth_type[0x%x], does not support parsing None" \
			"IPv4 Eth type and IPv4 protocol type at the same time\n",
				parse_bm, parse_key->eth_type);
		return MV_BAD_VALUE;
	}

	/* Could not parse None IPv6 Eth type and IPv4 protocol type at the same ime */
	if ((parse_bm & CPH_APP_PARSE_FIELD_ETH_TYPE) &&
		((parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH1) ||
		(parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH2) ||
		(parse_bm & CPH_APP_PARSE_FIELD_ICMPV6_TYPE)) &&
		(parse_key->eth_type != MV_CPH_ETH_TYPE_IPV6)) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"parse_bm[0x%x], eth_type[0x%x], does not support parsing None IPv6 Eth type and " \
			"IPv6 NH or ICMP type at the same time\n",
				parse_bm, parse_key->eth_type);
		return MV_BAD_VALUE;
	}

	/* Could not parse Eth subtype and IPv4 the same time */
	if ((parse_bm & CPH_APP_PARSE_FIELD_ETH_SUBTYPE) &&
		(parse_bm & CPH_APP_PARSE_FIELD_IPV4_TYPE)) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"parse_bm[0x%x], does not support parsing Eth subtype and IPv4 type at the same time\n",
			parse_bm);
		return MV_BAD_VALUE;
	}

	/* Could not parse Eth subtype and IPv6 the same time */
	if ((parse_bm & CPH_APP_PARSE_FIELD_ETH_SUBTYPE) &&
		((parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH1) ||
		(parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH2) ||
		(parse_bm & CPH_APP_PARSE_FIELD_ICMPV6_TYPE))) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"parse_bm[0x%x], does not support parsing Eth subtype and IPv6 type at the same time\n",
			parse_bm);
		return MV_BAD_VALUE;
	}

	/* Could not parse IPv4 and IPv6 at the same time */
	if ((parse_bm & CPH_APP_PARSE_FIELD_IPV4_TYPE) &&
		((parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH1) ||
		 (parse_bm & CPH_APP_PARSE_FIELD_IPV6_NH2) ||
		 (parse_bm & CPH_APP_PARSE_FIELD_ICMPV6_TYPE))) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"parse_bm[0x%x], does not support parsing IPv4 and IPv6 type at the same time\n",
			parse_bm);
		return MV_BAD_VALUE;
	}

	/* Validate IGMPv6 type */
	if ((parse_bm & CPH_APP_PARSE_FIELD_ICMPV6_TYPE) &&
		(parse_key->icmpv6_type != MV_ICMPV6_TYPE_MLD)) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"parse_bm[0x%x], icmpv6_type[%d], currently only support ICMPv6 MLD type[%d]\n",
			parse_bm, parse_key->icmpv6_type, MV_ICMPV6_TYPE_MLD);
		return MV_BAD_VALUE;
	}
	return rc;
}

/******************************************************************************
* cph_app_validate_mod_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Validate the modification filed of CPH rule
*
* INPUTS:
*       mod_bm     - Modification bitmap
*       mod_value  - Modification value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_validate_mod_field(
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value)
{
	MV_STATUS rc = MV_OK;

	/* Check the range of mod_bm */
	if (mod_bm >= CPH_APP_MOD_FIELD_END) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "mod_bm[0x%x] is out of range[0x01~0x%x]\n", mod_bm, CPH_APP_MOD_FIELD_END);
		return MV_OUT_OF_RANGE;
	}

	/* Does not support adding GMAC information and strip MH at the same time */
	if ((mod_bm & CPH_APP_RX_MOD_ADD_GMAC) &&
	    (mod_bm & CPH_APP_RX_MOD_STRIP_MH)) {
		MV_CPH_PRINT(CPH_ERR_LEVEL,
			"mod_bm[0x%x], does not support adding GMAC information and stripping MH at the same time\n",
			mod_bm);
		return MV_BAD_VALUE;
	}

	return rc;
}

/******************************************************************************
* cph_app_validate_frwd_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Validate the forwarding filed of CPH rule
*
* INPUTS:
*       frwd_bm    - Forwarding bitmap
*       frwd_value - Forwarding value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_validate_frwd_field(
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	/* Check the range of frwd_bm */
	if (frwd_bm >= CPH_APP_FRWD_FIELD_END) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "frwd_bm[0x%x] is out of range[0x01~0x%x]\n",
			frwd_bm, CPH_APP_FRWD_FIELD_END);
		return MV_OUT_OF_RANGE;
	}

	return rc;
}

/******************************************************************************
* cph_app_add_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Add CPH rule
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*       mod_bm     - Modification bitmap
*       mod_value  - Modification value
*       frwd_bm    - Forwarding bitmap
*       frwd_value - Forwarding value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_add_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_validate_parse_field(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid parsing field");

	rc = cph_app_validate_mod_field(mod_bm, mod_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid modification field");

	rc = cph_app_validate_frwd_field(frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid forwarding field");

	rc = cph_db_add_app_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_add_app_rule");

	return rc;
}

/******************************************************************************
* cph_app_del_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Delete CPH rule
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_del_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_validate_parse_field(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid parsing field");

	rc = cph_db_del_app_rule(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_del_app_rule");

	return rc;
}

/******************************************************************************
* cph_app_update_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Update CPH rule
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*       mod_bm     - Modification bitmap
*       mod_value  - Modification value
*       frwd_bm    - Forwarding bitmap
*       frwd_value - Forwarding value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_update_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_validate_parse_field(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid parsing field");

	rc = cph_app_validate_mod_field(mod_bm, mod_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid modification field");

	rc = cph_app_validate_frwd_field(frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid forwarding field");

	rc = cph_db_update_app_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_update_app_rule");

	return rc;
}

/******************************************************************************
* cph_app_get_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Get CPH rule
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*
* OUTPUTS:
*       mod_bm     - Modification bitmap
*       mod_value  - Modification value
*       frwd_bm    - Forwarding bitmap
*       frwd_value - Forwarding value
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_get_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E  *mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E *frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_validate_parse_field(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid parsing field");

	rc = cph_db_get_app_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	if (rc != MV_OK)
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "fail to call cph_app_get_rule\n");

	return rc;
}

/******************************************************************************
* cph_app_get_rule_by_dir_proto()
* _____________________________________________________________________________
*
* DESCRIPTION: Get CPH rule according to protocol type
*
* INPUTS:
*       dir        - Direction
*       proto_type - SKB protocol type
*
* OUTPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*       mod_bm     - Modification bitmap
*       mod_value  - Modification value
*       frwd_bm    - Forwarding bitmap
*       frwd_value - Forwarding value
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_get_rule_by_dir_proto(
	enum CPH_DIR_E              dir,
	unsigned short                 proto_type,
	enum CPH_APP_PARSE_FIELD_E *parse_bm,
	struct CPH_APP_PARSE_T       *parse_key,
	enum CPH_APP_MOD_FIELD_E   *mod_bm,
	struct CPH_APP_MOD_T         *mod_value,
	enum CPH_APP_FRWD_FIELD_E  *frwd_bm,
	struct CPH_APP_FRWD_T        *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_db_get_app_rule_by_dir_proto(dir, proto_type, parse_bm,
		parse_key, mod_bm, mod_value, frwd_bm, frwd_value);

	return rc;
}

/******************************************************************************
* cph_app_increase_counter()
* _____________________________________________________________________________
*
* DESCRIPTION: Increase RX counter
*
* INPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_increase_counter(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_validate_parse_field(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to valid parsing field");

	rc = cph_db_increase_counter(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_increase_counter");

	return rc;
}

/******************************************************************************
* cph_app_increase_counter_by_dir_proto()
* _____________________________________________________________________________
*
* DESCRIPTION:  Increase RX counter according to protocol type
*
* INPUTS:
*       dir        - Direction
*       proto_type - SKB protocol type
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_increase_counter_by_dir_proto(
	enum CPH_DIR_E dir,
	unsigned short    proto_type)
{
	MV_STATUS rc = MV_OK;

	rc = cph_db_increase_counter_by_dir_proto(dir, proto_type);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_increase_counter_by_dir_proto");

	return rc;
}

/******************************************************************************
* cph_app_parse_ge_port_type()
* _____________________________________________________________________________
*
* DESCRIPTION: Get GEMAC port type by profile ID
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       port_type   - Modification bitmap
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_parse_ge_port_type(struct CPH_PORT_STATE_T *port_type)
{
	enum tpm_eth_complex_profile_t  profile_id  = 0;
	enum MV_APP_GMAC_PORT_E         active_port = 0;
	MV_STATUS                  rc          = MV_OK;

	/* Get Profile ID and active WAN port */
	cph_db_get_param(CPH_DB_PARAM_PROFILE_ID,  &profile_id);
	cph_db_get_param(CPH_DB_PARAM_ACTIVE_PORT, &active_port);

	switch (profile_id) {
	case TPM_PON_WAN_DUAL_MAC_INT_SWITCH:
	case TPM_PON_WAN_G1_LAN_G0_INT_SWITCH:
	case TPM_PON_WAN_DUAL_MAC_EXT_SWITCH:
	case TPM_PON_WAN_G0_G1_DUAL_LAN:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_PON_MAC_PORT].port_type  = MV_APP_PORT_WAN;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_ACTIVE;
		break;
	case TPM_PON_WAN_G0_INT_SWITCH:
	case TPM_PON_WAN_G0_SINGLE_PORT:
	case TPM_PON_WAN_G0_G1_LPBK:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_INVALID;
		port_type[MV_APP_PON_MAC_PORT].port_type  = MV_APP_PORT_WAN;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_ACTIVE;
		break;
	case TPM_G0_WAN_G1_INT_SWITCH:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_WAN;
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_INVALID;
		break;
	case TPM_G1_WAN_G0_INT_SWITCH:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_WAN;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_INVALID;
		break;
	case TPM_PON_G1_WAN_G0_INT_SWITCH:
	case TPM_PON_G1_WAN_G0_SINGLE_PORT:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_WAN;
		port_type[MV_APP_PON_MAC_PORT].port_type  = MV_APP_PORT_WAN;

		if (active_port == MV_APP_GMAC_PORT_1) {
			port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
			port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_INACTIVE;
		} else {
			port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_INACTIVE;
			port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_ACTIVE;
		}
		break;
	case TPM_PON_G0_WAN_G1_INT_SWITCH:
	case TPM_PON_G0_WAN_G1_SINGLE_PORT:
		port_type[MV_APP_GMAC_PORT_0].port_type   = MV_APP_PORT_WAN;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_PON_MAC_PORT].port_type  = MV_APP_PORT_WAN;
		if (active_port == MV_APP_GMAC_PORT_0) {
			port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_ACTIVE;
			port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_INACTIVE;
		} else {
			port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_INACTIVE;
			port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_ACTIVE;
		}
		break;
	case TPM_PON_WAN_G1_MNG_EXT_SWITCH:
	case TPM_PON_WAN_G1_SINGLE_PORT:
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_INVALID;
		port_type[MV_APP_GMAC_PORT_1].port_type   = MV_APP_PORT_LAN;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_ACTIVE;
		port_type[MV_APP_PON_MAC_PORT].port_type  = MV_APP_PORT_WAN;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_ACTIVE;
		break;
	default:
		port_type[MV_APP_GMAC_PORT_0].port_state  = MV_GE_PORT_INVALID;
		port_type[MV_APP_GMAC_PORT_1].port_state  = MV_GE_PORT_INVALID;
		port_type[MV_APP_PON_MAC_PORT].port_state = MV_GE_PORT_INVALID;
		break;
	}
	/* loopback port will be LAN side port by default */
	port_type[MV_APP_LPBK_PORT].port_type  = MV_APP_PORT_LAN;
	port_type[MV_APP_LPBK_PORT].port_state = MV_GE_PORT_ACTIVE;

	return rc;
}

/******************************************************************************
* cph_app_parse_peer_port()
* _____________________________________________________________________________
*
* DESCRIPTION: Get peer GEMAC port
*
* INPUTS:
*       port        - Original port
*
* OUTPUTS:
*       peer_port   - Peer port
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_parse_peer_port(
	int    port,
	int   *peer_port)
{
	unsigned int            idx = 0;
	struct CPH_PORT_STATE_T  port_type[MV_APP_GMAC_PORT_NUM];
	MV_STATUS         rc  = MV_FAIL;

	/* Verify port */
	if (port > MV_APP_PON_MAC_PORT) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "port[%d] is out of range[0~%d]\n", port, MV_APP_PON_MAC_PORT);
		return MV_OUT_OF_RANGE;
	}

	/* Get port type */
	rc = cph_app_parse_ge_port_type(&port_type[0]);

	/* Search for peer port */
	if (port_type[port].port_type == MV_APP_PORT_LAN) {
		for (idx = 0; idx < MV_APP_GMAC_PORT_NUM; idx++) {
			if (idx == port)
				continue;
			if ((port_type[idx].port_type  == MV_APP_PORT_WAN) &&
			    (port_type[idx].port_state == MV_GE_PORT_ACTIVE)) {
				*peer_port = idx;
				rc  = MV_OK;
				break;
			}
		}
	} else if (port_type[port].port_type == MV_APP_PORT_WAN) {
		for (idx = 0; idx < MV_APP_GMAC_PORT_NUM; idx++) {
			if (idx == port)
				continue;
			if (port_type[idx].port_type  == MV_APP_PORT_LAN) {
				*peer_port = idx;
				rc  = MV_OK;
				break;
			}
		}
	}

	return rc;
}

/******************************************************************************
* cph_app_parse_dir()
* _____________________________________________________________________________
*
* DESCRIPTION: Parse application packet to output parse bitmap and value
*
* INPUTS:
*       port  - GE MAC port
*       rx    - Whether RX path
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       Return direction.
*******************************************************************************/
enum CPH_DIR_E cph_app_parse_dir(
	int    port,
	bool     rx)
{
	struct CPH_PORT_STATE_T port_type[MV_APP_GMAC_PORT_NUM];
	MV_STATUS        rc  = MV_OK;
	enum CPH_DIR_E        dir = CPH_DIR_INVALID;

	/* Parse port */
	if (port > MV_APP_PON_MAC_PORT) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "port[%d] is out of range[0~%d]\n", port, MV_APP_PON_MAC_PORT);
		return dir;
	}

	/* Get GMAC WAN/LAN type */
	rc = cph_app_parse_ge_port_type(&port_type[0]);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "fail to call cph_app_parse_ge_port_type()\n");
		return dir;
	}

	/* Rx dir */
	if (rx == TRUE) {
		if ((port_type[port].port_type  == MV_APP_PORT_WAN) &&
		    (port_type[port].port_state == MV_GE_PORT_ACTIVE))
			dir = CPH_DIR_DS;
		else if ((port_type[port].port_type  == MV_APP_PORT_LAN) &&
			(port_type[port].port_state == MV_GE_PORT_ACTIVE))
			dir = CPH_DIR_US;
		else {
			dir = CPH_DIR_INVALID;
			MV_CPH_PRINT(CPH_ERR_LEVEL, "RX dir[%d] is invalid\n", dir);
		}
	} else {/* Tx dir */
		if ((port_type[port].port_type  == MV_APP_PORT_WAN) &&
		    (port_type[port].port_state == MV_GE_PORT_ACTIVE))
			dir = CPH_DIR_US;
		else if ((port_type[port].port_type  == MV_APP_PORT_LAN) &&
			(port_type[port].port_state == MV_GE_PORT_ACTIVE))
			dir = CPH_DIR_DS;
		else {
			dir = CPH_DIR_INVALID;
			MV_CPH_PRINT(CPH_ERR_LEVEL, "TX dir[%d] is invalid\n", dir);
		}
	}

	return dir;
}

/******************************************************************************
* cph_app_parse_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: Parse application packet to output parse bitmap and value
*
* INPUTS:
*       port       - GE MAC port
*       skb_data   - Pointer to SKB data holding application packet
*
* OUTPUTS:
*       parse_bm   - Parsing bitmap
*       parse_key  - Parsing key
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_parse_packet(
	int                  port,
	unsigned char                 *skb_data,
	enum CPH_APP_PARSE_FIELD_E *parse_bm,
	struct CPH_APP_PARSE_T       *parse_key)
{
	unsigned short                  eth_type     = 0;
	struct ipv6hdr         *p_ipv6_hdr   = NULL;
	struct ipv6_hopopt_hdr *p_hopopt_hdr = NULL;
	struct icmp6hdr        *p_icmp_hdr   = NULL;
	unsigned char                  *p_field      = NULL;
	MV_STATUS               rc           = MV_OK;

	*parse_bm = 0;
	memset(parse_key, 0, sizeof(struct CPH_APP_PARSE_T));

	/* Parse dir */
	parse_key->dir = cph_app_parse_dir(port, TRUE);
	if (parse_key->dir == CPH_DIR_INVALID) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "dir[%d] is invalid\n", parse_key->dir);
		return MV_BAD_VALUE;
	}
	*parse_bm |= CPH_APP_PARSE_FIELD_DIR;

	/* Parse RX/TX */
	parse_key->rx_tx = CPH_DIR_RX;
	*parse_bm |= CPH_APP_PARSE_FIELD_RX_TX;

	/* Parse Marvell header */
	if (parse_key->dir == CPH_DIR_US)
		parse_key->mh = (ntohs(*(unsigned short *)skb_data) & MV_VALID_MH_MASK);
	else
		parse_key->mh = (ntohs(*(unsigned short *)skb_data) & MV_VALID_GH_MASK);

	*parse_bm |= CPH_APP_PARSE_FIELD_MH;

	/* Parse Eth type */
	p_field  = skb_data + MV_ETH_MH_SIZE + ETH_ALEN + ETH_ALEN;
	eth_type = ntohs(*(unsigned short *)p_field);
	while (eth_type == MV_TPID_8100 || eth_type == MV_TPID_88A8 || eth_type == MV_TPID_9100) {
		p_field += VLAN_HLEN;
		eth_type = ntohs(*(unsigned short *)p_field);
	}
	parse_key->eth_type = eth_type;
	*parse_bm |= CPH_APP_PARSE_FIELD_ETH_TYPE;

	/* Parse IPv4 type */
	if (eth_type == ETH_P_IP) {
		p_field += MV_CPH_ETH_TYPE_LEN;
		p_field += MV_IPV4_PROTO_OFFSET;
		parse_key->ipv4_type = *(unsigned char *)p_field;
		*parse_bm |= CPH_APP_PARSE_FIELD_IPV4_TYPE;
	} else if (eth_type == ETH_P_IPV6) {/* Parse IPv6 type */
		p_ipv6_hdr = (struct ipv6hdr *)(p_field + MV_CPH_ETH_TYPE_LEN);
		parse_key->ipv6_nh1 = p_ipv6_hdr->nexthdr;
		*parse_bm |= CPH_APP_PARSE_FIELD_IPV6_NH1;

		if (p_ipv6_hdr->nexthdr != NEXTHDR_HOP)
			return rc;

		p_hopopt_hdr = (struct ipv6_hopopt_hdr *)((unsigned char *)p_ipv6_hdr + sizeof(struct ipv6hdr));

		parse_key->ipv6_nh2 = p_hopopt_hdr->nexthdr;
		*parse_bm |= CPH_APP_PARSE_FIELD_IPV6_NH2;

		if (p_hopopt_hdr->nexthdr != IPPROTO_ICMPV6)
			return rc;

		p_icmp_hdr =  (struct icmp6hdr *)((unsigned char *)p_hopopt_hdr + ipv6_optlen(p_hopopt_hdr));

		switch (p_icmp_hdr->icmp6_type) {
		case ICMPV6_MGM_QUERY:
		case ICMPV6_MGM_REPORT:
		case ICMPV6_MGM_REDUCTION:
		case ICMPV6_MLD2_REPORT:
			parse_key->icmpv6_type = MV_ICMPV6_TYPE_MLD;
			*parse_bm |= CPH_APP_PARSE_FIELD_ICMPV6_TYPE;
			break;
		default:
			break;
		}
	} else {/* Parse Ethenet subtype */
		parse_key->eth_subtype = (*(unsigned char *)(p_field + MV_CPH_ETH_TYPE_LEN));
		*parse_bm |= CPH_APP_PARSE_FIELD_ETH_SUBTYPE;
	}

	return rc;
}

/******************************************************************************
* cph_app_mod_rx_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: Modify RX application packet
*
* INPUTS:
*       port      - Gmac port the packet from
*       dev       - Net device
*       skb       - SKB buffer to receive packet
*       rx_desc   - RX descriptor
*       mod_bm    - Modification bitmap
*       mod_value - Modification value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_mod_rx_packet(
	int                port,
	struct net_device   *dev,
	struct sk_buff      *skb,
	struct pp2_rx_desc  *rx_desc,
	enum CPH_APP_MOD_FIELD_E  mod_bm,
	struct CPH_APP_MOD_T       *mod_value)
{
	unsigned char     *p_data = NULL;
	MV_STATUS  rc     = MV_OK;

	/* Save GMAC Information */
	if (mod_bm & CPH_APP_RX_MOD_ADD_GMAC) {
		p_data     = (unsigned char *)skb->data;
		p_data[0] &= 0x0F;
		p_data[0] |= ((port & 0x0F) << 4);
	}

	if (mod_bm & CPH_APP_RX_MOD_STRIP_MH) {
		skb->data += MV_ETH_MH_SIZE;
		skb->tail -= MV_ETH_MH_SIZE;
		skb->len  -= MV_ETH_MH_SIZE;
	}

	skb->protocol = eth_type_trans(skb, dev);
	if (mod_bm & CPH_APP_RX_MOD_REPLACE_PROTO_TYPE)
		skb->protocol = mod_value->proto_type;

	return rc;
}

/******************************************************************************
* cph_app_mod_tx_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: Modify TX application packet
*
* INPUTS:
*       skb         - Pointer to SKB data hoding application packet
*       tx_spec_out - TX descriptor
*       mod_bm      - Modification bitmap
*       mod_value   - Modification value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_mod_tx_packet(
	struct sk_buff        *skb,
	struct mv_eth_tx_spec *tx_spec_out,
	enum CPH_APP_MOD_FIELD_E    mod_bm,
	struct CPH_APP_MOD_T         *mod_value)
{
	MV_STATUS rc = MV_OK;

	if (mod_bm & CPH_APP_TX_MOD_ADD_MH_BY_DRIVER) {
		tx_spec_out->flags |= MV_ETH_TX_F_MH;
		tx_spec_out->tx_mh = MV_16BIT_BE(mod_value->mh);
	}

	if (mod_bm & CPH_APP_TX_MOD_NO_PAD)
		tx_spec_out->flags |= MV_ETH_TX_F_NO_PAD;

	return rc;
}

/******************************************************************************
* cph_app_set_frwd()
* _____________________________________________________________________________
*
* DESCRIPTION: Set packet forwarding information
*
* INPUTS:
*       skb         - Pointer to SKB data hoding application packet
*       tx_spec_out - TX descriptor
*       frwd_bm     - Forwarding bitmap
*       frwd_value  - Forwarding value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_app_set_frwd(
	struct sk_buff        *skb,
	struct mv_eth_tx_spec *tx_spec_out,
	enum CPH_APP_FRWD_FIELD_E   frwd_bm,
	struct CPH_APP_FRWD_T        *frwd_value)
{
	MV_STATUS rc = MV_OK;

	if (frwd_bm & CPH_APP_FRWD_SET_TRG_PORT)
		tx_spec_out->txp = frwd_value->trg_port;

	if (frwd_bm & CPH_APP_FRWD_SET_TRG_QUEUE)
		tx_spec_out->txq = frwd_value->trg_queue;

	if (frwd_bm & CPH_APP_FRWD_SET_GEM_PORT)
		tx_spec_out->hw_cmd[0] = ((frwd_value->gem_port << 8)|0x0010);

	tx_spec_out->tx_func = NULL;

	return rc;
}

/******************************************************************************
* cph_app_rx_bc()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received broadcast packets
*
* INPUTS:
*       port    - Gmac port the packet from
*       dev     - Net device
*       pkt     - Marvell packet information
*       rx_desc - RX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
int cph_app_rx_bc(int port, struct net_device *dev, struct sk_buff *skb, struct pp2_rx_desc *rx_desc)
{
	struct CPH_FLOW_ENTRY_T      flow_rule;
	int                 peer_port = 0;
	int                 rx_size   = 0;
	int                 offset    = 0;
	bool                  state     = FALSE;
	struct sk_buff       *skb_old   = NULL;
	struct sk_buff       *skb_new   = NULL;
	MV_STATUS             rc        = MV_OK;

	/* Check whether need to handle broadcast packet */
	cph_db_get_param(CPH_DB_PARAM_BC_SUPPORT, &state);
	if (state == FALSE)
		return 0;

	/* Parse packets */
	skb_old = skb;
	skb_new = skb;
	rc = cph_flow_parse_packet(port, skb_old->data, TRUE, TRUE, &flow_rule);
	if (rc != MV_OK) {
		MV_CPH_PRINT(CPH_ERR_LEVEL, "fail to call cph_flow_parse_packet, rc<%d>\n", rc);
		return 0;
	}

	/* U/S */
	if (flow_rule.dir == CPH_DIR_US) {
		/* Forward packet to peer port */
		rc = cph_app_parse_peer_port(port, &peer_port);
		if (rc != MV_OK) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "fail to call cph_app_parse_peer_port, rc<%d>\n", rc);
			return 0;
		}

		/* Forward packet */
		if (netif_running(mv_eth_ports[peer_port]->dev)) {
			/* Copy a new SKB */
			skb_old->tail += rx_desc->dataSize;
			skb_old->len   = rx_desc->dataSize;
			skb_new = skb_copy(skb_old, GFP_ATOMIC);
			if (skb_new == NULL) {
				skb_new = skb_old;
				goto out;
			}
			mv_eth_ports[peer_port]->dev->netdev_ops->ndo_start_xmit(skb_old, mv_eth_ports[peer_port]->dev);
		}
	}
out:
	/* Stripe VLAN tag, then send to Linux network stack */
	offset         = cph_flow_strip_vlan(TRUE, skb_new->data);
	skb_new->data += offset;
	rx_size       -= offset;

	/* Strip MH */
	skb_new->data += MV_ETH_MH_SIZE;
	offset        += MV_ETH_MH_SIZE;

	skb_new->tail    -= offset;
	skb_new->len     -= offset;
	skb_new->protocol = eth_type_trans(skb_new, dev);

	cph_rec_skb(port, skb_new);

	return 1;
}

/******************************************************************************
* cph_app_lookup_profile_id()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup profile ID string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_app_lookup_profile_id(int enum_value)
{
	return mtype_lookup_enum_str(&g_enum_array_profile_id, enum_value);
}

/******************************************************************************
* cph_app_lookup_pon_type()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup PON type string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_app_lookup_pon_type(int enum_value)
{
	return mtype_lookup_enum_str(&g_enum_array_pon_type, enum_value);
}

/******************************************************************************
* cph_app_lookup_dir()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup direction string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_app_lookup_dir(int enum_value)
{
	return mtype_lookup_enum_str(&g_enum_array_dir, enum_value);
}

/******************************************************************************
* cph_app_lookup_rx_tx()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup RX/TX direction string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_app_lookup_rx_tx(int enum_value)
{
	return mtype_lookup_enum_str(&g_enum_array_rx_tx, enum_value);
}

/******************************************************************************
* cph_app_lookup_gmac()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup GMAC string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_app_lookup_gmac(int enum_value)
{
	return mtype_lookup_enum_str(&g_enum_array_gmac, enum_value);
}


/******************************************************************************
* cph_app_init()
* _____________________________________________________________________________
*
* DESCRIPTION: Initializes CPH application module.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int  cph_app_init(void)
{

	cph_db_init();

	return MV_OK;
}

/******************************************************************************
* cph_set_trace_flag()
* _____________________________________________________________________________
*
* DESCRIPTION:sets cph trace flag.
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_set_trace_flag(unsigned int flag)
{
	g_cph_global_trace = flag;

	return MV_OK;
}
