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
* mv_cph_mng_if.h
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) management interface definition
*              for ioctl
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 11Dec2011
*
* FILE REVISION NUMBER:
*               Revision: 1.1
*
*
*******************************************************************************/
#ifndef _MV_CPH_MNG_IF_H_
#define _MV_CPH_MNG_IF_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Include Files
------------------------------------------------------------------------------*/
#include <linux/cdev.h>

/* Definitions
------------------------------------------------------------------------------*/
#define MV_CPH_IOCTL_SET_COMPLEX_PROFILE      _IOW(MV_CPH_IOCTL_MAGIC,  1,  unsigned int)
#define MV_CPH_IOCTL_SET_FEATURE_FLAG         _IOW(MV_CPH_IOCTL_MAGIC,  2,  unsigned int)
#define MV_CPH_IOCTL_APP_ADD_RULE             _IOW(MV_CPH_IOCTL_MAGIC,  3,  unsigned int)
#define MV_CPH_IOCTL_APP_DEL_RULE             _IOW(MV_CPH_IOCTL_MAGIC,  4,  unsigned int)
#define MV_CPH_IOCTL_APP_UPDATE_RULE          _IOW(MV_CPH_IOCTL_MAGIC,  5,  unsigned int)
#define MV_CPH_IOCTL_APP_GET_RULE             _IOR(MV_CPH_IOCTL_MAGIC,  6,  unsigned int)
#define MV_CPH_IOCTL_FLOW_ADD_RULE            _IOW(MV_CPH_IOCTL_MAGIC,  7,  unsigned int)
#define MV_CPH_IOCTL_FLOW_DEL_RULE            _IOW(MV_CPH_IOCTL_MAGIC,  8,  unsigned int)
#define MV_CPH_IOCTL_FLOW_GET_RULE            _IOR(MV_CPH_IOCTL_MAGIC,  9,  unsigned int)
#define MV_CPH_IOCTL_FLOW_CLEAR_RULE          _IOW(MV_CPH_IOCTL_MAGIC,  10, unsigned int)
#define MV_CPH_IOCTL_FLOW_CLEAR_RULE_BY_MH    _IOW(MV_CPH_IOCTL_MAGIC,  11, unsigned int)
#define MV_CPH_IOCTL_FLOW_SET_DSCP_MAP        _IOW(MV_CPH_IOCTL_MAGIC,  12, unsigned int)
#define MV_CPH_IOCTL_FLOW_DEL_DSCP_MAP        _IOW(MV_CPH_IOCTL_MAGIC,  13, unsigned int)
#define MV_CPH_IOCTL_SET_TCONT_LLID_STATE     _IOW(MV_CPH_IOCTL_MAGIC,  14, unsigned int)
#define MV_CPH_IOCTL_SETUP                    _IOW(MV_CPH_IOCTL_MAGIC,  15, unsigned int)

/* Typedefs
------------------------------------------------------------------------------*/
struct CPH_IOCTL_APP_RULE_T {
	enum CPH_APP_PARSE_FIELD_E parse_bm;
	struct CPH_APP_PARSE_T       parse_key;
	enum CPH_APP_MOD_FIELD_E   mod_bm;
	struct CPH_APP_MOD_T         mod_value;
	enum CPH_APP_FRWD_FIELD_E  frwd_bm;
	struct CPH_APP_FRWD_T        frwd_value;
};

struct CPH_IOCTL_FLOW_MAP_T {
	struct CPH_FLOW_ENTRY_T flow_map;
};

struct CPH_IOCTL_DSCP_MAP_T {
	struct CPH_DSCP_PBITS_T dscp_map;
};

struct CPH_IOCTL_MISC_T {
	enum tpm_eth_complex_profile_t profile_id;
	enum MV_APP_GMAC_PORT_E        active_port;
	enum CPH_APP_FEATURE_E         feature_type;
	bool                      feature_flag;
};

struct CPH_IOCTL_TCONT_STATE_T {
	unsigned int  tcont;
	bool    state;
};

/* MV CPH Char Device Structure */
struct CPH_CDEV_T {
	struct CPH_IOCTL_APP_RULE_T    cph_ioctl_app_rule;
	struct CPH_IOCTL_FLOW_MAP_T    cph_ioctl_flow_map;
	struct CPH_IOCTL_DSCP_MAP_T    cph_ioctl_dscp_map;
	struct CPH_IOCTL_MISC_T        cph_ioctl_misc;
	struct CPH_IOCTL_TCONT_STATE_T cph_ioctl_tcont;

	struct cdev             cdev;
};

/* Global variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _MV_CPH_MNG_IF_H_ */
