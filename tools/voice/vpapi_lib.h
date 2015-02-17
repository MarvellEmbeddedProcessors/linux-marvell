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

*******************************************************************************/

#ifndef _VPAPI_LIB_H
#define _VPAPI_LIB_H

#include <stdbool.h>
#include <slic/vpapi_dev.h>

typedef struct vpapi_init_device_params {
	unsigned short dev_size;
	unsigned short ac_size;
	unsigned short dc_size;
	unsigned short ring_size;
	unsigned short fxo_ac_size;
	unsigned short fxo_cfg_size;
} vpapi_init_device_params_t;

int vpapi_open_device(void);
int vpapi_close_device(void);
int vpapi_check_event(void);
VpStatusType vpapi_make_dev_object(VpDeviceType dev_type, VpDeviceIdType dev_id);
VpStatusType vpapi_make_line_object(VpTermType term_type, VpLineIdType line_id);
VpStatusType vpapi_map_line_id(VpLineIdType line_id);
VpStatusType vpapi_map_slac_id(VpDeviceIdType dev_id, unsigned char slac_id);
VpStatusType vpapi_free_line_context(VpLineIdType line_id);
VpStatusType vpapi_init_device(VpDeviceIdType dev_id, VpProfilePtrType dev_profile_ptr,
				VpProfilePtrType ac_profile_ptr, VpProfilePtrType dc_profile_ptr,
				VpProfilePtrType ring_profile_ptr, VpProfilePtrType fxo_ac_profile_ptr,
				VpProfilePtrType fxo_cfg_profile_ptr, vpapi_init_device_params_t *params_ptr);
VpStatusType vpapi_cal_line(VpLineIdType line_id);
VpStatusType vpapi_set_line_state(VpLineIdType line_id, VpLineStateType state);
VpStatusType vpapi_set_option(unsigned char line_request, VpLineIdType line_id, VpDeviceIdType dev_id,
				VpOptionIdType option, void *value_p);
bool vpapi_get_event(VpDeviceIdType dev_id, VpEventType *event_p);
int vpapi_battary_on(int vbh, int vbl, int vbp);
int vpapi_battery_off(void);
int vpapi_slic_reg_read(VpLineIdType line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff);
int vpapi_slic_reg_write(VpLineIdType line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff);

#endif /* _VPAPI_LIB_H */
