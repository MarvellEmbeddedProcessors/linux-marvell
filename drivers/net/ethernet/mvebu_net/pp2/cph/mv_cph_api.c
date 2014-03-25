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
* mv_cph_api.c
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) API definition
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

#include "mv_cph_header.h"


/******************************************************************************
* Variable Definition
******************************************************************************/


/******************************************************************************
* Function Definition
******************************************************************************/
/******************************************************************************
* cph_set_complex_profile()
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
MV_STATUS cph_set_complex_profile(enum tpm_eth_complex_profile_t profile_id, enum MV_APP_GMAC_PORT_E active_port)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_set_complex_profile(profile_id, active_port);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_app_set_complex_profile");

	return rc;
}
EXPORT_SYMBOL(cph_set_complex_profile);

/******************************************************************************
* cph_set_feature_flag()
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
MV_STATUS cph_set_feature_flag(enum CPH_APP_FEATURE_E feature, bool state)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_set_feature_flag(feature, state);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_app_set_feature_flag");

	return rc;
}
EXPORT_SYMBOL(cph_set_feature_flag);

/******************************************************************************
* cph_add_app_rule()
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
MV_STATUS cph_add_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_add_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_app_add_rule");

	return rc;
}
EXPORT_SYMBOL(cph_add_app_rule);

/******************************************************************************
* cph_del_app_rule()
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
MV_STATUS cph_del_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_del_rule(parse_bm, parse_key);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_app_del_rule");

	return rc;
}
EXPORT_SYMBOL(cph_del_app_rule);

/******************************************************************************
* cph_update_app_rule()
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
MV_STATUS cph_update_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E   mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E  frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_update_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_app_update_rule");

	return rc;
}
EXPORT_SYMBOL(cph_update_app_rule);

/******************************************************************************
* cph_get_app_rule()
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
MV_STATUS cph_get_app_rule(
	enum CPH_APP_PARSE_FIELD_E parse_bm,
	struct CPH_APP_PARSE_T      *parse_key,
	enum CPH_APP_MOD_FIELD_E  *mod_bm,
	struct CPH_APP_MOD_T        *mod_value,
	enum CPH_APP_FRWD_FIELD_E *frwd_bm,
	struct CPH_APP_FRWD_T       *frwd_value)
{
	MV_STATUS rc = MV_OK;

	rc = cph_app_get_rule(parse_bm, parse_key, mod_bm, mod_value, frwd_bm, frwd_value);
	if (rc != MV_OK)
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "fail to call cph_app_get_rule\n");

	return rc;
}
EXPORT_SYMBOL(cph_get_app_rule);

/******************************************************************************
* cph_add_flow_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Sets flow mapping rule
*
* INPUTS:
*       cph_flow - VLAN ID, 802.1p value, pkt_fwd information.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_add_flow_rule(struct CPH_FLOW_ENTRY_T *cph_flow)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_add_rule(cph_flow);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_add_rule");

	return rc;
}
EXPORT_SYMBOL(cph_add_flow_rule);

/******************************************************************************
* cph_del_flow_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Deletes flow mapping rule
*
* INPUTS:
*       cph_flow - VLAN ID, 802.1p value, pkt_fwd information.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_del_flow_rule(struct CPH_FLOW_ENTRY_T *cph_flow)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_del_rule(cph_flow);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_del_rule");

	return rc;
}
EXPORT_SYMBOL(cph_del_flow_rule);

/******************************************************************************
* cph_get_flow_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Gets flow mapping rule for tagged frames.
*
* INPUTS:
*       cph_flow - Input vid, pbits, dir
*
* OUTPUTS:
*       cph_flow - output packet forwarding information, including GEM port,
*                   T-CONT, queue and packet modification for VID, P-bits.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_get_flow_rule(struct CPH_FLOW_ENTRY_T *cph_flow)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_get_rule(cph_flow);
	if (rc != MV_OK)
		MV_CPH_PRINT(CPH_DEBUG_LEVEL, "fail to call cph_flow_get_rule\n");

	return rc;
}
EXPORT_SYMBOL(cph_get_flow_rule);

/******************************************************************************
* cph_clear_flow_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Clears all flow mapping rules
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
int cph_clear_flow_rule(void)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_clear_rule();
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_clear_rule");

	return rc;
}
EXPORT_SYMBOL(cph_clear_flow_rule);

/******************************************************************************
* cph_clear_flow_rule_by_mh()
* _____________________________________________________________________________
*
* DESCRIPTION: Clears flow mapping rules by MH
*
* INPUTS:
*       mh   -  Marvell header.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_clear_flow_rule_by_mh(unsigned short mh)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_clear_rule_by_mh(mh);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_clear_rule_by_mh");

	return rc;
}

/******************************************************************************
* cph_set_flow_dscp_map()
* _____________________________________________________________________________
*
* DESCRIPTION: Sets DSCP to P-bits mapping rules
*
* INPUTS:
*       dscp_map  - DSCP to P-bits mapping rules.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_set_flow_dscp_map(struct CPH_DSCP_PBITS_T *dscp_map)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_set_dscp_map(dscp_map);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_set_dscp_map");

	return rc;
}
EXPORT_SYMBOL(cph_set_flow_dscp_map);

/******************************************************************************
* cph_del_flow_dscp_map()
* _____________________________________________________________________________
*
* DESCRIPTION: Deletes DSCP to P-bits mapping rules
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
int cph_del_flow_dscp_map(void)
{
	MV_STATUS rc = MV_OK;

	rc = cph_flow_del_dscp_map();
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "Fail to call cph_flow_del_dscp_map");

	return rc;
}
EXPORT_SYMBOL(cph_del_flow_dscp_map);

/*******************************************************************************
**
** cph_get_tcont_state
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
bool cph_get_tcont_state(unsigned int tcont)
{
	return cph_db_get_tcont_state(tcont);
}
EXPORT_SYMBOL(cph_get_tcont_state);

/*******************************************************************************
**
** cph_set_tcont_state
** ___________________________________________________________________________
**
** DESCRIPTION: The function sets T-CONT state in mv_cust
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
int cph_set_tcont_state(unsigned int tcont, bool state)
{
	return cph_db_set_tcont_state(tcont, state);
}
EXPORT_SYMBOL(cph_set_tcont_state);
