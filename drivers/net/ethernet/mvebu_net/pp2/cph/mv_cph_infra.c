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
* mv_cph_infra.c
*
* DESCRIPTION: Include user space infrastructure modules definitions
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
#include <linux/version.h>
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
char g_cph_unknown_str[] = "<unknown>";

/******************************************************************************
* Function Definition
******************************************************************************/
/******************************************************************************
* mindex_tpm_src_to_app_port()
*
* DESCRIPTION:Convert TPM source port to application UNI port
*
* INPUTS:
*       src_port    - TPM source port
*
* OUTPUTS:
*       Application UNI port index
*
* RETURNS:
*       On success, the function returns application UNI port index.
*       On error return invalid application UNI port index.
*******************************************************************************/
enum MV_APP_ETH_PORT_UNI_E mindex_tpm_src_to_app_port(enum tpm_src_port_type_t src_port)
{
	enum MV_APP_ETH_PORT_UNI_E app_port = MV_APP_ETH_PORT_INVALID;

	/* Should modify below code in case support more than four UNI ports */
	if (src_port <= TPM_SRC_PORT_UNI_3)
		app_port = MV_APP_ETH_PORT_INDEX_MIN + (src_port - TPM_SRC_PORT_UNI_0);

	return app_port;
}

/******************************************************************************
* mindex_mh_to_app_llid()
*
* DESCRIPTION:Convert Marvell header to application LLID
*
* INPUTS:
*       mh  - Marvell header
*
* OUTPUTS:
*       Application LLID
*
* RETURNS:
*       On success, the function returns application LLID.
*       On error return invalid application LLID.
*******************************************************************************/
enum MV_TCONT_LLID_E mindex_mh_to_app_llid(unsigned short mh)
{
	enum MV_TCONT_LLID_E llid       = MV_TCONT_LLID_INVALID;
	unsigned char           llid_index = 0;

	llid_index = (mh >> 8) & 0x0f;

	if (llid_index > 0) {
		if (0x0f == llid_index) {
			llid = MV_TCONT_LLID_BROADCAST;
		} else {
			llid = llid_index - 1;
			if (llid > MV_TCONT_LLID_7)
				llid = MV_TCONT_LLID_INVALID;
		}
	}

	return llid;
}

/******************************************************************************
* mtype_get_digit_num()
*
* DESCRIPTION:Convert character string to digital number
*
* INPUTS:
*       str   - Character string
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Digital numbe
*******************************************************************************/
unsigned int mtype_get_digit_num(const char  *str)
{
	unsigned int  val = 0;

	if ((str[1] == 'x') || (str[1] == 'X'))
		sscanf(&str[2], "%x", &val);
	else
		val = simple_strtoul(str, NULL, 10);

	return val;
}

/******************************************************************************
* mtype_lookup_enum_str()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup enum string according to enum value
*
* INPUTS:
*       p_enum_array   - Pointer to enum array
*       enum_value     - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *mtype_lookup_enum_str(struct MV_ENUM_ARRAY_T *p_enum_array, int enum_value)
{
	int idx;

	for (idx = 0; idx < p_enum_array->enum_num; idx++) {
		if (enum_value == p_enum_array->enum_array[idx].enum_value)
			return p_enum_array->enum_array[idx].enum_str;
	}
	return g_cph_unknown_str;
}

/******************************************************************************
* mutils_is_frwd_broadcast_packet()
* _____________________________________________________________________________
*
* DESCRIPTION:Check whether packet is directly forwarded broadcast one
*
* INPUTS:
*       data   - packet data
*
* OUTPUTS:
*       None
*
* RETURNS:
*       TRUE: broadcast packet, FALSE:none broadcast packet
*******************************************************************************/
bool mutils_is_frwd_broadcast_packet(char *data)
{
	char bc_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	char *p_data;

	p_data = data + MV_ETH_MH_SIZE;

	if (!memcmp(p_data, &bc_mac[0], sizeof(bc_mac)))
		return TRUE;
	else
		return FALSE;
}
