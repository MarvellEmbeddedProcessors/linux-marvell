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
* mv_cph_app.h
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
#ifndef _MV_CPH_APP_H_
#define _MV_CPH_APP_H_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
* Type Definition
******************************************************************************/
#define MV_VALID_MH_MASK  0x007F  /* Valid MH filed in CPH */
#define MV_VALID_GH_MASK  0x0FFF  /* Valid MH filed in CPH */


/* CPH application packets parsing field definition
------------------------------------------------------------------------------*/
enum CPH_APP_PARSE_FIELD_E {
	CPH_APP_PARSE_FIELD_DIR          = 0x1,
	CPH_APP_PARSE_FIELD_RX_TX        = 0x2,
	CPH_APP_PARSE_FIELD_MH           = 0x4,
	CPH_APP_PARSE_FIELD_ETH_TYPE     = 0x8,
	CPH_APP_PARSE_FIELD_ETH_SUBTYPE  = 0x10,
	CPH_APP_PARSE_FIELD_IPV4_TYPE    = 0x20,
	CPH_APP_PARSE_FIELD_IPV6_NH1     = 0x40,
	CPH_APP_PARSE_FIELD_IPV6_NH2     = 0x80,
	CPH_APP_PARSE_FIELD_ICMPV6_TYPE  = 0x100,
	CPH_APP_PARSE_FIELD_END          = 0x200
};

enum CPH_DIR_E {
	CPH_DIR_US       = 0,
	CPH_DIR_DS       = 1,
	CPH_DIR_NOT_CARE = 2,
	CPH_DIR_NUM      = 2,
	CPH_DIR_INVALID  = 3
};

enum CPH_RX_TX_E {
	CPH_DIR_RX         = 0,
	CPH_DIR_TX         = 1,
	CPH_RX_TX_NOT_CARE = 2,
};

struct CPH_APP_PARSE_T {
	enum CPH_DIR_E   dir;
	enum CPH_RX_TX_E rx_tx;
	unsigned short      mh;
	unsigned short      eth_type;
	unsigned char       eth_subtype;
	unsigned char       ipv4_type;
	unsigned char       ipv6_nh1;
	unsigned char       ipv6_nh2;
	unsigned int      icmpv6_type;
};

/* CPH application packets modification field definition
------------------------------------------------------------------------------*/
enum CPH_APP_MOD_FIELD_E {
	CPH_APP_RX_MOD_ADD_GMAC           = 0x1,
	CPH_APP_RX_MOD_REPLACE_PROTO_TYPE = 0x2,
	CPH_APP_RX_MOD_STRIP_MH           = 0x4,
	CPH_APP_TX_MOD_ADD_MH_BY_DRIVER   = 0x8,
	CPH_APP_TX_MOD_NO_PAD             = 0x10,
	CPH_APP_MOD_CHANGE_STATE          = 0x20,
	CPH_APP_MOD_FIELD_END             = 0x40
};

struct CPH_APP_MOD_T {
	unsigned short   proto_type;
	unsigned short   mh;
	bool     state;
};

/* CPH application packets forwarding field definition
------------------------------------------------------------------------------*/
enum CPH_APP_FRWD_FIELD_E {
	CPH_APP_FRWD_SET_TRG_PORT     = 0x1,
	CPH_APP_FRWD_SET_TRG_QUEUE    = 0x2,
	CPH_APP_FRWD_SET_GEM_PORT     = 0x4,
	CPH_APP_FRWD_FIELD_END        = 0x8
};

struct CPH_APP_FRWD_T {
	unsigned char   trg_port;
	unsigned char   trg_queue;
	unsigned short  gem_port;
};

/* TPM connection profile definition from "tpm_types.h"
------------------------------------------------------------------------------*/
enum tpm_eth_complex_profile_t {
	TPM_PON_WAN_DUAL_MAC_INT_SWITCH = 1,
	TPM_PON_WAN_G0_INT_SWITCH,
	TPM_PON_WAN_G1_LAN_G0_INT_SWITCH,
	TPM_G0_WAN_G1_INT_SWITCH,
	TPM_G1_WAN_G0_INT_SWITCH,
	TPM_PON_G1_WAN_G0_INT_SWITCH,
	TPM_PON_G0_WAN_G1_INT_SWITCH,
	TPM_PON_WAN_DUAL_MAC_EXT_SWITCH,
	TPM_PON_WAN_G1_MNG_EXT_SWITCH,
	TPM_PON_WAN_G0_SINGLE_PORT,
	TPM_PON_WAN_G1_SINGLE_PORT,
	TPM_PON_G1_WAN_G0_SINGLE_PORT,
	TPM_PON_G0_WAN_G1_SINGLE_PORT,
	TPM_PON_WAN_G0_G1_LPBK,
	TPM_PON_WAN_G0_G1_DUAL_LAN
};

/* PON type definition
------------------------------------------------------------------------------*/
enum CPH_PON_TYPE_E {
	CPH_PON_TYPE_EPON = 0,
	CPH_PON_TYPE_GPON,
	CPH_PON_TYPE_GBE,
	CPH_PON_TYPE_P2P,
	CPH_PON_TYPE_MAX
};


/* CPH feature flag
------------------------------------------------------------------------------*/
enum CPH_APP_FEATURE_E {
	CPH_APP_FEATURE_APP = 0,
	CPH_APP_FEATURE_IGMP,
	CPH_APP_FEATURE_BC,
	CPH_APP_FEATURE_FLOW,
	CPH_APP_FEATURE_UDP,
};


/* CPH parse/modification field definition for bc/igmp
------------------------------------------------------------------------------*/
struct CPH_PACKET_PARSE_T {
	enum CPH_DIR_E      dir;
	unsigned int         src_port;
	unsigned short         vid;
	unsigned char          pbits;
	unsigned char          dscp;
};


struct CPH_PACKET_MOD_T {
	enum CPH_DIR_E      dir;
	unsigned int         src_port;
	unsigned short         vid;
	unsigned char          pbits;
	unsigned char          dscp;
};

/* GMAC port state definition
------------------------------------------------------------------------------*/
struct CPH_PORT_STATE_T {
	enum MV_APP_PORT_TYPE_E   port_type;
	enum MV_GE_PORT_STATE_E   port_state;
};

/* Debug related definition
------------------------------------------------------------------------------*/
extern unsigned int g_cph_global_trace;

#define CPH_GLOBAL_TRACE g_cph_global_trace
#define CPH_DEBUG_LEVEL  0x00000001
#define CPH_INFO_LEVEL   0x00000002
#define CPH_WARN_LEVEL   0x00000004
#define CPH_ERR_LEVEL    0x00000008

#define MV_CPH_PRINT(level, format, ...) \
	{ \
		if (level & CPH_GLOBAL_TRACE) \
			pr_info("%s(line:%d) "format, __func__, __LINE__, ##__VA_ARGS__); \
	}

#define MV_CPH_CLEAN_PRINT(level, format, ...) \
	{ \
		if (level & CPH_GLOBAL_TRACE) \
			pr_info(format, ##__VA_ARGS__); \
	}

/******************************************************************************
 * Function Declaration
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
MV_STATUS cph_app_set_complex_profile(enum tpm_eth_complex_profile_t profile_id, enum MV_APP_GMAC_PORT_E active_port);

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
MV_STATUS cph_app_set_feature_flag(enum CPH_APP_FEATURE_E feature, bool state);

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
	struct CPH_APP_FRWD_T       *frwd_value);

/******************************************************************************
* cph_app_del_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Del CPH rule
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
	struct CPH_APP_PARSE_T      *parse_key);

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
	struct CPH_APP_FRWD_T       *frwd_value);

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
	struct CPH_APP_FRWD_T       *frwd_value);

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
	struct CPH_APP_FRWD_T        *frwd_value);

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
	struct CPH_APP_PARSE_T      *parse_key);

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
	unsigned short    proto_type);

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
MV_STATUS cph_app_parse_ge_port_type(struct CPH_PORT_STATE_T *port_type);

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
	bool     rx);

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
	struct CPH_APP_PARSE_T       *parse_key);

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
	struct CPH_APP_MOD_T       *mod_value);

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
	struct mv_pp2_tx_spec *tx_spec_out,
	enum CPH_APP_MOD_FIELD_E    mod_bm,
	struct CPH_APP_MOD_T         *mod_value);

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
	struct mv_pp2_tx_spec *tx_spec_out,
	enum CPH_APP_FRWD_FIELD_E   frwd_bm,
	struct CPH_APP_FRWD_T        *frwd_value);

/******************************************************************************
* cph_app_rx_bc()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received broadcast packets
*
* INPUTS:
*       port    - Gmac port the packet from
*       dev     - Net device
*       skb     - Marvell packet information
*       rx_desc - RX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
int cph_app_rx_bc(int port, struct net_device *dev, struct sk_buff *skb, struct pp2_rx_desc *rx_desc);

/******************************************************************************
* cph_app_lookup_profile_id()
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
char *cph_app_lookup_profile_id(int enum_value);

/******************************************************************************
* cph_app_lookup_pon_type()
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
char *cph_app_lookup_pon_type(int enum_value);

/******************************************************************************
* cph_app_lookup_dir()
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
char *cph_app_lookup_dir(int enum_value);

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
char *cph_app_lookup_rx_tx(int enum_value);

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
char *cph_app_lookup_gmac(int enum_value);

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
int  cph_app_init(void);

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
MV_STATUS cph_set_trace_flag(unsigned int flag);

#ifdef __cplusplus
}
#endif

#endif /* _MV_CPH_APP_H_ */
