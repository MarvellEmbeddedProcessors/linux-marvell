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
#include "silabs_lib.h"

/* Locals */
static char dev_name[] = "/dev/silabs";
static int dev_fd = 0;
static char lib_str[] = "[silabs_lib]:";

int silabs_open_device(void)
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

int silabs_close_device(void)
{
	if(dev_fd > 0)
		close(dev_fd);

	return 0;
}

int silabs_slic_reg_read(int line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff)
{
	SilabsRegObjType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.chanNum = line_id;
	data.func = SI_REG_READ;
	data.regAddr = cmd;
	data.value = 0;
	data.ramValue = 0;

	if (ioctl(dev_fd, SILABS_MOD_IOX_REG_CTRL, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_REG_CTRL) failed\n");
		return -1;
	}

	*buff = data.value;

	return 0;
}

int silabs_slic_reg_write(int line_id, unsigned char cmd, unsigned char cmd_len, unsigned char *buff)
{
	SilabsRegObjType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.chanNum = line_id;
	data.func = SI_REG_WRITE;
	data.regAddr = cmd;
	data.value = buff[0];
	data.ramValue = 0;

	if (ioctl(dev_fd, SILABS_MOD_IOX_REG_CTRL, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_REG_CTRL) failed\n");
		return -1;
	}

	return 0;
}

int silabs_slic_ram_read(int line_id, uInt16 cmd, unsigned char cmd_len, unsigned int *buff)
{
	SilabsRegObjType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.chanNum = line_id;
	data.func = SI_RAM_READ;
	data.ramAddr = cmd;
	data.ramValue = 0;
	data.value = 0;

	if (ioctl(dev_fd, SILABS_MOD_IOX_REG_CTRL, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_REG_CTRL) failed\n");
		return -1;
	}

	*buff = data.ramValue;

	return 0;
}

int silabs_slic_ram_write(int line_id, uInt16 cmd, unsigned char cmd_len, unsigned int *buff)
{
	SilabsRegObjType data;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	data.chanNum = line_id;
	data.func = SI_RAM_WRITE;
	data.ramAddr = cmd;
	data.ramValue = buff[0];
	data.value = 0;

	if (ioctl(dev_fd, SILABS_MOD_IOX_REG_CTRL, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_REG_CTRL) failed\n");
		return -1;
	}

	return 0;
}

int silabs_control_interface(ControlFuncType func, int devNum)
{
	SilabsModCtrlObjType data;

	data.func = func;
	data.devNum = devNum;
	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CTRL_IF, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CTRL_IF) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_device_init(DeviceFuncType func, int devNum)
{
	SilabsModDevObjType data;

	data.func = func;
	data.devNum = devNum;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_DEVICE_INIT, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_DEVICE_INIT) failed\n");
		return -1;
	}

	return data.status;
}


int silabs_channel_init(ChannelFuncType func, int chanNum)
{
	SilabsModChannelObjType data;

	data.func = func;
	data.chanNum = chanNum;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_INIT, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_INIT) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_channel_all(ChannelAllFuncType func)
{
	SilabsModChannelAllObjType data;

	data.func = func;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_ALL, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_ALL) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_channel_setup(ChannelSetupFuncType func, int chanNum, int preset)
{
	SilabsModChannelSetupObjType data;

	data.chanNum = chanNum;
	data.func = func;
	data.preset = preset;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_SETUP, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_SETUP) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_channel_operation(ChannelOPFuncType func, int chanNum)
{
	SilabsModChannelOpObjType data;

	data.func = func;
	data.chanNum = chanNum;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_OP, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_OP) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_PCM_TS_setup(int chanNum, uInt16 rxcount, uInt16 txcount)
{
	SilabsModPCMTSSetupObjType data;

	data.chanNum = chanNum;
	data.rxcount = rxcount;
	data.txcount = txcount;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_PCM_TS_SETUP, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_PCM_TS_SETUP) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_channel_set_line_feed(int chanNum, uInt8 newLineFeed)
{
	SilabsModChannelLineFeedObjType data;

	data.chanNum = chanNum;
	data.newLineFeed = newLineFeed;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_LINE_FEED, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_LINE_FEED) failed\n");
		return -1;
	}

	return data.status;
}

int silabs_channel_read_hook_status(int chanNum, uInt8 *pHookStatus)
{
	SilabsModChannelReadHookStatObjType data;

	data.chanNum = chanNum;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_HOOK_STATUS, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_HOOK_STATUS) failed\n");
		return -1;
	}

	*pHookStatus = data.hookStatus;

	return data.status;
}

int silabs_channel_set_loopback(int chanNum, ProslicLoopbackModes newMode)
{
	SilabsModChannelSetLoopbackObjType data;

	data.chanNum = chanNum;
	data.newMode = newMode;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(dev_fd, SILABS_MOD_IOX_CHAN_LOOPBACK, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_CHAN_LOOPBACK) failed\n");
		return -1;
	}

	return data.status;
}

bool silabs_get_event(SiEventType *event_p)
{
	SilabsModGetEventType data;

	data.pEvent = event_p;

	if (dev_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return false;
	}


	if (ioctl(dev_fd, SILABS_MOD_IOX_GET_EVENT, &data) < 0) {
		printf("ioctl(SILABS_MOD_IOX_GET_EVENT) failed\n");
		return false;
	}

	return data.newEvent;
}

int silabs_check_event(void)
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
