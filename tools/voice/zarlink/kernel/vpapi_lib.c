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

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "vpapi_lib.h"

/* Locals */
static char dev_name[] = "/dev/vpapi";
static int dev_fd = 0;
static char lib_str[] = "[vpapi_lib]:";

int vpapi_open_device(void)
{
	int fdflags;

	/* open the device */
	dev_fd = open(dev_name, O_RDWR);
	if (dev_fd <= 0) {
		printf("Cannot open %s device\n", dev_name);
		return -1;
	}

	/* set some flags */
	fdflags = fcntl(dev_fd, F_GETFL, 0);
	fdflags |= O_NONBLOCK;
	fcntl(dev_fd, F_SETFL, fdflags);

	return 0;
}

int vpapi_close_device(void)
{
	if(dev_fd > 0)
		close(dev_fd);

	return 0;
}

VpStatusType vpapi_make_dev_object(VpDeviceType dev_type, VpDeviceIdType dev_id)
{
	VpApiModMkDevObjType data;

	data.deviceType = dev_type;
	data.deviceId = dev_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_MK_DEV_OBJ, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_MK_DEV_OBJ) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_make_line_object(VpTermType term_type, VpLineIdType line_id)
{
	VpApiModMkLnObjType data;

	data.termType = term_type;
	data.lineId = line_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_MK_LN_OBJ, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_MK_LN_OBJ) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_map_line_id(VpLineIdType line_id)
{
	VpApiModMapLnIdType data;

	data.lineId = line_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_MAP_LN_ID, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_MAP_LN_ID) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_map_slac_id(VpDeviceIdType dev_id, unsigned char slac_id)
{
	VpApiModMapSlacIdType data;

	data.deviceId = dev_id;
	data.slacId = slac_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_MAP_SLAC_ID, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_MAP_SLAC_ID) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_free_line_context(VpLineIdType line_id)
{
	VpApiModFreeLnCtxType data;

	data.lineId = line_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_FREE_LN_CTX, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_FREE_LN_CTX) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_init_device(VpDeviceIdType dev_id, VpProfilePtrType dev_profile_ptr,
				VpProfilePtrType ac_profile_ptr, VpProfilePtrType dc_profile_ptr,
				VpProfilePtrType ring_profile_ptr, VpProfilePtrType fxo_ac_profile_ptr,
				VpProfilePtrType fxo_cfg_profile_ptr, vpapi_init_device_params_t *params_ptr)
{
	VpApiModInitDeviceType data;

	data.deviceId = dev_id;
	data.pDevProfile = dev_profile_ptr;
	data.pAcProfile = ac_profile_ptr;
	data.pDcProfile = dc_profile_ptr;
	data.pRingProfile = ring_profile_ptr;
	data.pFxoAcProfile = fxo_ac_profile_ptr;
	data.pFxoCfgProfile = fxo_cfg_profile_ptr;
	data.devProfileSize = params_ptr->dev_size;
	data.acProfileSize = params_ptr->ac_size;
	data.dcProfileSize = params_ptr->dc_size;
	data.ringProfileSize = params_ptr->ring_size;
	data.fxoAcProfileSize = params_ptr->fxo_ac_size;
	data.fxoCfgProfileSize = params_ptr->fxo_cfg_size;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_INIT_DEV, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_INIT_DEV) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_cal_line(VpLineIdType line_id)
{
	VpApiModCalLnType data;

	data.lineId = line_id;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_CAL_LN, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_CAL_LN) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_set_line_state(VpLineIdType line_id, VpLineStateType state)
{
	VpApiModSetLnStType data;

	data.lineId = line_id;
	data.state = state;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_SET_LN_ST, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_SET_LN_ST) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

VpStatusType vpapi_set_option(unsigned char line_request, VpLineIdType line_id,
				VpDeviceIdType dev_id, VpOptionIdType option, void *value_p)
{
	VpApiModSetOptionType data;

	data.lineRequest = line_request;
	data.lineId = line_id;
	data.deviceId = dev_id;
	data.option = option;
	data.pValue = value_p;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return VP_STATUS_FAILURE;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_SET_OPTION, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_SET_OPTION) failed\n");
		return VP_STATUS_FAILURE;
	}

	return data.status;
}

bool vpapi_get_event(VpDeviceIdType dev_id, VpEventType *event_p)
{
	VpApiModGetEventType data;

	data.deviceId = dev_id;
	data.pEvent = event_p;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return false;
	}


	if (ioctl(dev_fd, VPAPI_MOD_IOX_GET_EVENT, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_GET_EVENT) failed\n");
		return false;
	}

	return data.newEvent;
}

int vpapi_check_event(void)
{
	fd_set ex_fds;
	struct timeval timeout = {0, 1};
	int ret;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return 1;
	}

	FD_ZERO(&ex_fds);
	FD_SET(dev_fd, &ex_fds);

	/* Wait for event  */
	ret = select(dev_fd+1, NULL, NULL, &ex_fds, &timeout);

	if(FD_ISSET(dev_fd, &ex_fds))
		return 0;
	else
		return 1;
}

int vpapi_battary_on(int vbh, int vbl, int vbp)
{
	VpModBatteryOnType data;

	data.vbh = vbh;
	data.vbl = vbl;
	data.vbp = vbp;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, VPAPI_MOD_IOX_BATT_ON, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_BATT_ON) failed\n");
		return -1;
	}

	return data.status;
}

int vpapi_battery_off(void)
{
	VpModBatteryOffType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, VPAPI_MOD_IOX_BATT_OFF, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_BATT_OFF) failed\n");
		return -1;
	}

	return data.status;
}

int vpapi_slic_reg_read(VpLineIdType line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff)
{
	VpModRegOpType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.lineId = line_id;
	data.cmd = cmd;
	data.cmdLen = cmd_len;

	if (ioctl(dev_fd, VPAPI_MOD_IOX_REG_READ, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_REG_READ) failed\n");
		return -1;
	}

	memcpy(buff, data.buff, cmd_len);

	return 0;
}

int vpapi_slic_reg_write(VpLineIdType line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff)
{
	VpModRegOpType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.lineId = line_id;
	data.cmd = cmd;
	data.cmdLen = cmd_len;
	memcpy(data.buff, buff, cmd_len);

	if (ioctl(dev_fd, VPAPI_MOD_IOX_REG_WRITE, &data) < 0) {
		printf("ioctl(VPAPI_MOD_IOX_REG_WRITE) failed\n");
		return -1;
	}

	return 0;
}
