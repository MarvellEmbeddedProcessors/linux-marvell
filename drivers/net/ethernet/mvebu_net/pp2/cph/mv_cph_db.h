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
* mv_cph_db.h
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) data base implementation
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
#ifndef _MV_CPH_DB_H_
#define _MV_CPH_DB_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Data type Definition
 ******************************************************************************/
#define US_CPH_XML_CFG_FILE          "/etc/xml_params/cph_xml_cfg_file.xml"
#define US_ONU_PROFILE_XML_CFG_FILE  "/etc/xml_params/onu_profile_xml_cfg_file.xml"

#define XML_PROFILE_ELM_CAPABILITY   "Capability"
#define XML_PROFILE_ELM_ATTRIB       "attrib"
#define XML_PROFILE_ATTR_NAME        "name"
#define XML_PROFILE_ATTR_VALUE       "value"
#define XML_PROFILE_NAME_PROFILE     "Complex profile"
#define XML_PROFILE_NAME_ACTIVE_PORT "Active wan"

#define XML_CPH_ELM_APP_SUPPORT      "app_support"
#define XML_CPH_ELM_IGMP_SUPPORT     "igmp_support"
#define XML_CPH_ELM_BC_SUPPORT       "bc_support"
#define XML_CPH_ELM_FLOW_SUPPORT     "flow_support"
#define XML_CPH_ELM_UDP_SUPPORT      "udp_support"


/* CPH rule definition for application packet handling
------------------------------------------------------------------------------*/
struct CPH_APP_RULE_T {
	bool                  valid;
	enum CPH_APP_PARSE_FIELD_E parse_bm;
	struct CPH_APP_PARSE_T       parse_key;
	enum CPH_APP_MOD_FIELD_E   mod_bm;
	struct CPH_APP_MOD_T         mod_value;
	enum CPH_APP_FRWD_FIELD_E  frwd_bm;
	struct CPH_APP_FRWD_T        frwd_value;
	unsigned int                count;
};

/* CPH data base for application packet handling
------------------------------------------------------------------------------*/
#define CPH_APP_MAX_RULE_NUM  (64)
struct CPH_APP_DB_T {
	enum tpm_eth_complex_profile_t  profile_id;       /* Complex profile ID, see enum tpm_eth_complex_profile_t  */
	enum MV_APP_GMAC_PORT_E         active_port;      /* Current active WAN GE port, see enum MV_APP_GMAC_PORT_E */
	bool                       app_support;      /* Whether support generic application handling       */
	bool                       igmp_support;     /* Whether support IGMP/MLD packet handling           */
	bool                       bc_support;       /* Whether support U/S broadcast packet handling      */
	bool                       flow_support;     /* Whether support flow mapping handling in CPH       */
	bool                       udp_support;      /* Whether support UDP port mapping in CPH            */
	unsigned int                     rule_num;         /* Current application rule number                    */
	struct CPH_APP_RULE_T             cph_rule[CPH_APP_MAX_RULE_NUM]; /* CPH application rules                */
	spinlock_t                 app_lock;         /* Spin lock for application rule operation           */
	unsigned int                     bc_count;         /* Counter for mis-matched packets, usually is bc     */
	bool                       tcont_state[MV_TCONT_LLID_NUM];/* T-CONT state used to control SWF      */
};

/* CPH database parameter enum
------------------------------------------------------------------------------*/
enum CPH_DB_PARAM_E {
	CPH_DB_PARAM_PROFILE_ID = 0,
	CPH_DB_PARAM_ACTIVE_PORT,
	CPH_DB_PARAM_APP_SUPPORT,
	CPH_DB_PARAM_IGMP_SUPPORT,
	CPH_DB_PARAM_BC_SUPPORT,
	CPH_DB_PARAM_FLOW_SUPPORT,
	CPH_DB_PARAM_UDP_SUPPORT,
	CPH_DB_PARAM_BC_COUNTER
};

/******************************************************************************
* Function Declaration
******************************************************************************/
/******************************************************************************
* cph_db_set_param()
* _____________________________________________________________________________
*
* DESCRIPTION: Set CPH DB parameter
*
* INPUTS:
*       param   - The parameter type
*       value   - Parameter value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_db_set_param(enum CPH_DB_PARAM_E param, void *value);

/******************************************************************************
* cph_db_get_param()
* _____________________________________________________________________________
*
* DESCRIPTION: Get CPH DB parameter
*
* INPUTS:
*       param   - The parameter type
*
* OUTPUTS:
*       value   - Parameter value
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_db_get_param(enum CPH_DB_PARAM_E param, void *value);

/******************************************************************************
* cph_db_add_app_rule()
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
MV_STATUS cph_db_add_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value);

/******************************************************************************
* cph_db_del_app_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Deletes CPH rule
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
MV_STATUS cph_db_del_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key);

/******************************************************************************
* cph_db_update_app_rule()
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
MV_STATUS cph_db_update_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value);

/******************************************************************************
* cph_db_get_app_rule()
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
MV_STATUS cph_db_get_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E  *mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E *frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value);

/******************************************************************************
* cph_db_get_app_rule_by_dir_proto()
* _____________________________________________________________________________
*
* DESCRIPTION: Get application type CPH rule from data base by protocol type
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
MV_STATUS cph_db_get_app_rule_by_dir_proto(
	enum CPH_DIR_E              dir,
	unsigned short                 proto_type,
	enum CPH_APP_PARSE_FIELD_E *parse_bm,
	struct CPH_APP_PARSE_T       *parse_key,
	enum CPH_APP_MOD_FIELD_E   *mod_bm,
	struct CPH_APP_MOD_T         *mod_value,
	enum CPH_APP_FRWD_FIELD_E  *frwd_bm,
	struct CPH_APP_FRWD_T        *frwd_value);

/******************************************************************************
* cph_db_increase_counter()
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
MV_STATUS cph_db_increase_counter(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key);

/******************************************************************************
* cph_db_increase_counter_by_dir_proto()
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
MV_STATUS cph_db_increase_counter_by_dir_proto(enum CPH_DIR_E dir,
	unsigned short    proto_type);

/******************************************************************************
* cph_db_get_xml_param()
* _____________________________________________________________________________
*
* DESCRIPTION: Get the XML parameter
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
MV_STATUS cph_db_get_xml_param(void);

/*******************************************************************************
**
** cph_db_get_tcont_state
** ___________________________________________________________________________
**
** DESCRIPTION: The function get T-CONT state
**
** INPUTS:
**   tcont - T-CONT
**
** OUTPUTS:
**   None.
**
** RETURNS:
**   state - State of T-CONT, enabled or disabled.
**
*******************************************************************************/
bool cph_db_get_tcont_state(unsigned int tcont);

/*******************************************************************************
**
** cph_db_set_tcont_state
** ___________________________________________________________________________
**
** DESCRIPTION: The function sets T-CONT state in mv_cph
**
** INPUTS:
**   tcont - T-CONT
**   state - State of T-CONT, enabled or disabled.
**
** OUTPUTS:
**   None.
**
** RETURNS:
**  On success, the function returns (MV_OK). On error different types are
**  returned according to the case.
**
*******************************************************************************/
MV_STATUS cph_db_set_tcont_state(unsigned int tcont, bool state);

/******************************************************************************
* cph_db_display_parse_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Display CPH rule parsing field
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
MV_STATUS cph_db_display_parse_field(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key);

/******************************************************************************
* cph_db_display_mod_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Display CPH rule modification field
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
MV_STATUS cph_db_display_mod_field(
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value);

/******************************************************************************
* cph_db_display_frwd_field()
* _____________________________________________________________________________
*
* DESCRIPTION: Display CPH rule forwarding field
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
MV_STATUS cph_db_display_frwd_field(
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value);


/******************************************************************************
* cph_db_display_all()
* _____________________________________________________________________________
*
* DESCRIPTION: Display CPH data base
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
MV_STATUS cph_db_display_all(void);

/******************************************************************************
* cph_db_init()
* _____________________________________________________________________________
*
* DESCRIPTION: Initialize CPH data base
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
MV_STATUS cph_db_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _MV_CPH_DB_H_ */
