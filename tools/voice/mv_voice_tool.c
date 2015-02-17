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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "vpapi_lib.h"
#include "tdm/test/tdm_dev.h"
#if defined(ZARLINK_SLIC_VE880)
#include "profile_88266.h"
#elif defined(ZARLINK_SLIC_VE792)
#include "profile_79238.h"
#endif

#define TOOL_PREFIX	">> "
#define TIMEOUT		11000 /* usec */
/* Line calibration increases init time significantly */
#define LINE_CALIBRATION_SUPPORT

/* Defines */
#define GET_DEVICE(line_id)	(line_id/MAX_DEVICE_LINES)
#define GET_LINE(line_id)	(line_id % MAX_DEVICE_LINES)
#define N_A			0
#define ON_HOOK			0
#define OFF_HOOK		1
#define CH_BUFF_SIZE		(80 * pcm_bytes)
#define BUFF_ADDR(buff, line)	((unsigned char*)buff + (line*80*pcm_bytes))

/* VE880 */
#if defined(ZARLINK_SLIC_VE880)

#define MAX_DEVICES		2
#define MAX_DEVICE_LINES	2
#define MAX_LINES		4
#define VP_DEV_SERIES		VP_DEV_880_SERIES
#define DEV_PROFILE		ABS_VBL_FLYBACK
#define DC_COEFF		DC_22MA_CC
#define AC_COEFF		AC_FXS_RF14_DEF
#define WB_AC_COEFF		AC_FXS_RF14_WB_US
#define RING_PROFILE		RING_DEF

/* VE792 */
#elif defined(ZARLINK_SLIC_VE792)

#define MAX_DEVICES		4
#define MAX_DEVICE_LINES	8
#define MAX_LINES		32
#define VP_DEV_SERIES		VP_DEV_792_SERIES
#define DEV_PROFILE		VE792_DEV_PROFILE
#define DC_COEFF		VE792_DC_COEFF
#define AC_COEFF		VE792_AC_COEFF_600
#define WB_AC_COEFF		TBD /* TBD: AC profile for WideBand support */
#define RING_PROFILE		RING_20HZ_SINE

/* Power-supply related parameters */
#define VBH			-50
#define VBL			-25
#define VBP			 50

#endif

/* Extern */
extern int dev_profile_size;
extern int dc_profile_size;
extern int ac_profile_size;
extern int ring_profile_size;

/* GLobals */
static unsigned short total_lines = 0;
static unsigned short total_devs = 0;
static unsigned char pcm_bytes = 0;
static unsigned char cal_devs = 0;
#ifdef LINE_CALIBRATION_SUPPORT
static unsigned short cal_lines = 0;
#endif
static VpOptionCodecType codec = VP_OPTION_ALAW;
static unsigned char time_slot_table[MAX_LINES];
static unsigned char hook_state[MAX_LINES];
static char dev_name[] = "/dev/tdm";
static int buff_size = 0;
static unsigned char aud_buf[2][320 * MAX_LINES];
static unsigned short f1Mem = 0;
static unsigned short f2Mem = 0;
static int offhook_count = 0;
static unsigned int event_count = 0;
static int tdm_fd = 0;
static unsigned char data_buff[MAX_SLIC_RDWR_BUFF_SIZE];
static int slic_init = 0;

/* sin table, 256 points */
static short sinTbl[] = {0,402,804,1205,1606,2005,2404,2801,3196,3590,3981,4370,4756,
5139,5519,5896,6270,6639,7005,7366,7723,8075,8423,8765,9102,9433,9759,10079,10393,
10701,11002,11297,11585,11865,12139,12405,12664,12915,13159,13394,13622,13841,14052,
14255,14449,14634,14810,14977,15136,15285,15425,15556,15678,15790,15892,15985,16068,
16142,16206,16260,16304,16339,16363,16378,16383,16378,16363,16339,16304,16260,16206,
16142,16068,15985,15892,15790,15678,15556,15425,15285,15136,14977,14810,14634,14449,
14255,14052,13841,13622,13394,13159,12915,12664,12405,12139,11865,11585,11297,11002,
10701,10393,10079,9759,9433,9102,8765,8423,8075,7723,7366,7005,6639,6270,5896,5519,
5139,4756,4370,3981,3590,3196,2801,2404,2005,1606,1205,804,402,0,-402,-804,-1205,-1606,
-2005,-2404,-2801,-3196,-3590,-3981,-4370,-4756,-5139,-5519,-5896,-6270,-6639,-7005,
-7366,-7723,-8075,-8423,-8765,-9102,-9433,-9759,-10079,-10393,-10701,-11002,-11297,
-11585,-11865,-12139,-12405,-12664,-12915,-13159,-13394,-13622,-13841,-14052,-14255,
-14449,-14634,-14810,-14977,-15136,-15285,-15425,-15556,-15678,-15790,-15892,-15985,
-16068,-16142,-16206,-16260,-16304,-16339,-16363,-16378,-16383,-16378,-16363,-16339,
-16304,-16260,-16206,-16142,-16068,-15985,-15892,-15790,-15678,-15556,-15425,-15285,
-15136,-14977,-14810,-14634,-14449,-14255,-14052,-13841,-13622,-13394,-13159,-12915,
-12664,-12405,-12139,-11865,-11585,-11297,-11002,-10701,-10393,-10079,-9759,-9433,-9102,
-8765,-8423,-8075,-7723,-7366,-7005,-6639,-6270,-5896,-5519,-5139,-4756,-4370,-3981,
-3590,-3196,-2801,-2404,-2005,-1606,-1205,-804,-402,0};

/* Static APIs */
static void vpapi_dev_init(VpDeviceIdType dev_id);
static inline void wait_for_vpapi_event(void);
static int vpapi_init(void);
static void vpapi_init_done(void);
static void vpapi_release(void);
static void release(int signum);
static void sw_tone_test(int tdm_fd, unsigned char line_id);
static void sw_loopback_two_phones_test(int tdm_fd, unsigned char line0, unsigned char line1);
static void sw_loopback_multi_phones_test(int tdm_fd, unsigned char start_line, unsigned char end_line);
static void gen_tone(unsigned short freq, unsigned char line_id, unsigned char* tx_buff);
static void sw_loopback(int tdm_fd, unsigned char line_id);
static void slic_digital_loopback(int tdm_fd, unsigned long int iterations);
static void channel_balancing_test(int tdm_fd, unsigned long int iterations);
static inline int slic_dl_data_compare(int offset);

int main(void)
{
	int ret = 0, cmd = 0, val = 0, tdm_init = 0;
	int proc_fd, fdflags, cmd_len, i;
	char str[32];
	unsigned char line0_id, line1_id;
	tdm_dev_params_t tdm_params;
	unsigned long int iterations;

	event_count = 0;
	slic_init = 0;

	/* open tdm device */
	tdm_fd = open(dev_name, O_RDWR);
	if (tdm_fd <= 0) {
		printf("%s Cannot open %s device\n", TOOL_PREFIX, dev_name);
		return 1;
	}

	/* set some flags */
	fdflags = fcntl(tdm_fd, F_GETFL, 0);
	fdflags |= O_NONBLOCK;
	fcntl(tdm_fd, F_SETFL, fdflags);

	printf("\n%s Please enter total lines number: ", TOOL_PREFIX);
	gets(str);
	total_lines = atoi(str);

	printf("%s Please enter PCM sample size(1/2/4): ",TOOL_PREFIX);
	gets(str);
	pcm_bytes = atoi(str);

	/* Calculate total lines buffer size */
	buff_size = (80 * pcm_bytes * total_lines);

	/* Fill TDM info */
	tdm_params.pcm_format = pcm_bytes;
	tdm_params.total_lines = total_lines;

	/* Start TDM */
	if(ioctl(tdm_fd, TDM_DEV_TDM_START, &tdm_params)) {
		printf("%s Error, unable to init TDM\n", TOOL_PREFIX);
		return 1;
	}

	total_devs = (total_lines/MAX_DEVICE_LINES);
	if((total_lines % MAX_DEVICE_LINES))
		total_devs++;

	if(vpapi_open_device()) {
		printf("%s Error, could not open vpapi device\n", TOOL_PREFIX);
		return 1;
	}

	if(vpapi_init()) {
		printf("%s Error, init failed\n", TOOL_PREFIX);
		ret = 1;
		goto vpapi_out;
	}
	slic_init = 1;

	/* Wait to device/s and line/s calibration to finish */
#ifdef LINE_CALIBRATION_SUPPORT
	while((cal_devs < total_devs) || (cal_lines < total_lines)) {
#else
	while(cal_devs < total_devs) {
#endif
		wait_for_vpapi_event();
	}

	/* Handle termination gracefully */
	if (signal (SIGINT, release) == SIG_IGN)
		signal (SIGINT, SIG_IGN);

	/* Issue main menu */
	while(1) {
		printf("\n  Marvell Voice Tool:\n");
		printf("  0. Read from SLIC register(VE880 only)\n");
		printf("  1. Write to SLIC register(VE880 only)\n");
		printf("  2. Start ring\n");
		printf("  3. Stop ring\n");
                printf("  4. Start SW tone\n");
                printf("  5. Self echo on local phone\n");
                printf("  6. Loopback two local phones\n");
		printf("  7. Multiple local phone pairs loopback\n");
		printf("  8. Digital Loopback(incremental pattern)\n");
		printf("  9. Channel balancing\n");
		printf("  q. Quit\n");
		printf("\n%s Please select option: ", TOOL_PREFIX);

		/* Clear write buffer */
		memset(aud_buf[1], 0, buff_size);
		gets(str);
		switch(str[0])
		{
                        case '0':
#if defined(ZARLINK_SLIC_VE880)
                                printf("%s Enter line id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC register command(decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				printf("%s Enter SLIC register command size: ",TOOL_PREFIX);
				gets(str);
				cmd_len = atoi(str);
				vpapi_slic_reg_read(line0_id, cmd, cmd_len, data_buff);
				printf("\n%s Sent command 0x%x to line(%d)\n", TOOL_PREFIX, cmd, line0_id);
				printf("%s Return value: ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++)
					printf("0x%x ", data_buff[i]);
				printf("\n");
#else
				printf("%s operation not supported\n",TOOL_PREFIX);
#endif
				break;

			case '1':
#if defined(ZARLINK_SLIC_VE880)
				printf("%s Enter line id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC register command(decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				printf("%s Enter SLIC register command size: ",TOOL_PREFIX);
				gets(str);
				cmd_len = atoi(str);
				printf("%s Enter data(press Enter after each byte): ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++) {
					gets(str);
					data_buff[i] = atoi(str);
				}
				vpapi_slic_reg_write(line0_id, cmd, cmd_len, data_buff);
				printf("\n%s Sent command 0x%x to line(%d)\n", TOOL_PREFIX, cmd, line0_id);
#else
				printf("%s operation not supported\n",TOOL_PREFIX);
#endif
				break;


			case '2':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Start ringing on line %d\n", line0_id);
				vpapi_set_line_state(line0_id, VP_LINE_RINGING);
				break;

			case '3':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Stop ringing on line %d\n", line0_id);
				vpapi_set_line_state(line0_id, VP_LINE_STANDBY);
				break;

			case '4':
				if(pcm_bytes < 2) {
					printf("Test is supported for linear mode only - try again\n");
					break;
				}
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				sw_tone_test(tdm_fd, line0_id);
				break;

			case '5':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				printf("%s Waiting for off-hook...\n", TOOL_PREFIX);
				line0_id = atoi(str);
				sw_loopback(tdm_fd, line0_id);
				break;

			case '6':
				printf("%s Enter line #0: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter line #1: ", TOOL_PREFIX);
				gets(str);
				printf("Waiting for off-hook...\n");
				line1_id = atoi(str);
				if(line0_id >= MAX_LINES || line1_id >= MAX_LINES) {
					printf("%s Error, line must be in the range of 0-%d\n", TOOL_PREFIX, (MAX_LINES-1));
					break;
				}
				sw_loopback_two_phones_test(tdm_fd, line0_id, line1_id);
				break;

			case '7':
				printf("%s Enter starting line range: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter ending line range: ", TOOL_PREFIX);
				gets(str);
				printf("Waiting for off-hook...\n");
				line1_id = atoi(str);
				if((line0_id >= MAX_LINES) ||
				   (line1_id >= MAX_LINES) ||
				   ((line1_id-line0_id) % 2 == 0)) {
					printf("%s Error, lines range must be even and \
							between 0-%d\n", TOOL_PREFIX, (MAX_LINES-1));
					break;
				}
				sw_loopback_multi_phones_test(tdm_fd, line0_id, line1_id);
				break;

			case '8':
				printf("%s Enter number of iterations(must be greater than 3): ", TOOL_PREFIX);
				gets(str);
				iterations = (unsigned long int)atoi(str);
				if(iterations < 4) {
					printf("Requires at least 4 iterations  - try again\n");
					break;
				}
				slic_digital_loopback(tdm_fd, iterations);
				break;

			case '9':
				printf("%s Enter number of iterations('0' - for infinite loop): ", TOOL_PREFIX);
				gets(str);
				iterations = (unsigned long int)atoi(str);
				channel_balancing_test(tdm_fd, iterations);
				break;

			case 'q':
				goto vpapi_out;

			default:
				printf("Option is not supported - try again\n");
				break;
		}
	}

vpapi_out:
	release(0);

	return ret;
}

void release(int signum)
{
	printf("\n%s Exit\n", TOOL_PREFIX);
	sleep(1);

	/* Stop SLIC/s */
	if(slic_init)
		vpapi_release();

	/* Stop TDM */
	if(ioctl(tdm_fd, TDM_DEV_TDM_STOP, 0)) {
		printf("Error, unable to stop TDM\n");
		return;
	}

	close(tdm_fd);

	if(vpapi_close_device()) {
		printf("## Error, could not close vpapi device ##\n");
		return;
	}

	exit(0);
}

static void channel_balancing_test(int tdm_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch, cb_loop = 0, i;
	unsigned long int loops = 0, index;
	VpOptionLoopbackType lp = VP_OPTION_LB_TIMESLOT;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Fill Tx buffer with incremental pattern */
	for(ch = 0; ch < total_lines; ch++) {
		for(index = 0; index < (80 * pcm_bytes); index+=2)
			*((unsigned short*)&aud_buf[1][(80 * pcm_bytes * ch) + index]) = (((index+3) << 8)+ (index+1));
	}

	/* Put SLIC/s in loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_option(1, ch, GET_DEVICE(ch), VP_OPTION_ID_LOOPBACK, &lp);

	/* Wait a bit */
	sleep(1);

	/* Put SLIC/s in TALK mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_line_state(ch, VP_LINE_TALK);

	/* Wait a bit */
	sleep(1);

	if (iterations == 0)
		iterations = (unsigned long int)(-1); /* Assume infinite */

	while (loops < iterations) {

	  cb_loop = 0;
	  i = 0;

	  if (ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	  }

	  while (cb_loop == 0) {

		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			goto cb_out;
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds))
		{
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len < buff_size) {
				printf("write() failed\n");
				goto cb_out;
			}
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds))
		{
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len < buff_size) {
				printf("read() failed\n");
				goto cb_out;
			}

			if(i > 3) {

				for(ch = 1; ch < total_lines; ch++) {
					if(memcmp(aud_buf[0], &aud_buf[0][(ch * pcm_bytes * 80)], (pcm_bytes * 80))) {
						printf("\nERROR - data miscompare(ch=%d) !!!\n", ch);
						cmp_status = 1;
						goto cb_out;
					}
				}

				cb_loop = 1;
			}
			i++;
		}

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	  }

	  loops++;
	  if (ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	  }
	  printf("loop #%u\n", loops);
	  sleep(1);
	}

cb_out:

	if(cmp_status == 0) {
		printf("\nChannel balancing test PASSED !!!\n");
	}
	else {
		printf("Dump Rx buffer:\n");
		for(ch = 0; ch < total_lines; ch++) {
			printf("Buffer #%d: ", ch);
			for(i = 0; i < (pcm_bytes * 80); i++) {
				printf("0x%x ", aud_buf[0][(ch * pcm_bytes * 80) + i]);
			}
			printf("\n\n");
			sleep(1);
		}
	}

	if (ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	lp = VP_OPTION_LB_OFF;

	/* Disable loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_option(1, ch, GET_DEVICE(ch), VP_OPTION_ID_LOOPBACK, &lp);

	/* Put SLIC/s in STANDBY mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_line_state(ch, VP_LINE_STANDBY);
}

static void slic_digital_loopback(int tdm_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch;
	unsigned long int loops = 0, index;
	VpOptionLoopbackType lp = VP_OPTION_LB_TIMESLOT;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Put SLIC/s in loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_option(1, ch, GET_DEVICE(ch), VP_OPTION_ID_LOOPBACK, &lp);

	/* Wait a bit */
	sleep(1);

	/* Put SLIC/s in TALK mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_line_state(ch, VP_LINE_TALK);

	/* Fill Tx buffer with incremental pattern */
	for(ch = 0; ch < total_lines; ch++) {
		for(index = 0; index < (80 * pcm_bytes); index++)
			aud_buf[1][index + (80 * pcm_bytes * ch)] = (index+ch+2);
	}

	if (ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while (loops < iterations) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			goto slic_dl_out;
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds))
		{
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len < buff_size) {
				printf("write() failed\n");
				goto slic_dl_out;
			}
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds))
		{
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tdm_fd, aud_buf[0], buff_size);										     if (msg_len < buff_size) {
				printf("read() failed\n");
				goto slic_dl_out;
			}

			if(loops++ > 3) {
				for(ch = 0; ch < total_lines; ch++) {
					if(slic_dl_data_compare(ch)) {
						printf("\nERROR - data miscompare(loops=%d) !!!\n",loops);
						cmp_status = 1;
						goto slic_dl_out;
					}
				}
			}
		}

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;

	}

slic_dl_out:

	if(cmp_status == 0)
		printf("\nDigital loopback test(%d lines) - PASS !!!\n",total_lines);

	if (ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	lp = VP_OPTION_LB_OFF;

	/* Disable loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_option(1, ch, GET_DEVICE(ch), VP_OPTION_ID_LOOPBACK, &lp);

	/* Put SLIC/s in STANDBY mode */
	for(ch = 0; ch < total_lines; ch++)
		vpapi_set_line_state(ch, VP_LINE_STANDBY);
}

static inline int slic_dl_data_compare(int ch)
{
	int i = 0, offset = (ch * pcm_bytes * 80);

	/* Align Tx & Rx data start */
	while((aud_buf[1][offset] != aud_buf[0][offset+i]) && (i < (pcm_bytes * 80)))
		i++;

	if(i >= (offset + (pcm_bytes * 80))) {
		printf("\nError, first Tx byte not found inside Rx buffer\n");
		return -1;
	}

	if(memcmp(&aud_buf[0][offset+i], &aud_buf[1][offset], ((pcm_bytes * 80) - i))) {
			printf("\nDump buffers:\n");
			for(i = offset; i < (offset +(pcm_bytes * 80)); i++)
				printf("write[%d] = 0x%x, read[%d] = 0x%x\n", i, aud_buf[1][i], i, aud_buf[0][i]);
			return -1;
	}
	else
		return 0;
}

static void sw_loopback(int tdm_fd, unsigned char line_id)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until line goes off-hook */
	while(hook_state[line_id] == 0) {
		wait_for_vpapi_event();
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while(hook_state[line_id] == 1) {

		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				return;
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds))
		{
			printf("Rd\n");
			msg_len = read(tdm_fd, aud_buf[0], buff_size);										     if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds))
		{
			printf("Wr\n");
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_vpapi_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void gen_tone(unsigned short freq, unsigned char line_id, unsigned char* tx_buff)
{
	short i;
	short buf[80];
	short sample;

	for(i = 0; i < 80; i++) {
		sample = (sinTbl[f1Mem >> 8] + sinTbl[f2Mem >> 8]) >> 2;
#ifndef CONFIG_CPU_BIG_ENDIAN
		buf[i] = sample;
#else
		buf[i] = ((sample & 0xff) << 8)+ (sample >> 8);
#endif
		f1Mem += freq;
		f2Mem += freq;
	}
	memcpy(BUFF_ADDR(tx_buff, line_id), (void *)buf, 160);
}

static void sw_tone_test(int tdm_fd, unsigned char line_id)
{
	fd_set wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, x;
	char str[4];

	if (tdm_fd <= 0) {
			printf("%s Device %s is not accessible\n", TOOL_PREFIX, dev_name);
			return;
	}

	while(1) {
		printf("%s Choose frequency: (1) 300HZ (2) 630HZ (3) 1000HZ (4) Back to main menu: ", TOOL_PREFIX);
		gets(str);
		printf("%s Waiting for off-hook...\n", TOOL_PREFIX);

		if(str[0] == '1') {
			x = 2457;
			//printf("%s Generating 300HZ tone\n", TOOL_PREFIX);
		}
		else if (str[0] == '2') {
			x = 5161;
			//printf("%s Generating 630HZ tone\n", TOOL_PREFIX);
		}
		else if (str[0] == '3') {
			x = 8192;
			//printf("%s Generating 1000HZ tone\n", TOOL_PREFIX);
		}
		else if (str[0] == '4') {
			return;
	        }
		else {
			printf("%s Input error - try again\n", TOOL_PREFIX);
			continue;
		}

		/* Wait until both lines go off-hook */
		while(hook_state[line_id] == 0) {
			wait_for_vpapi_event();
		}

		if(ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
			printf("Error, unable to start pcm bus\n");
			return;
		}

		printf("%s Waiting for on-hook to return to menu.\n", TOOL_PREFIX);

		while(hook_state[line_id] == 1) {
			FD_ZERO(&wr_fds);
			FD_SET(tdm_fd, &wr_fds);

			/* Wait for event  */
			if (select(tdm_fd+1, NULL, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				return;
			}

			/* Write */
			if (FD_ISSET(tdm_fd, &wr_fds))
			{
				gen_tone(x, line_id, aud_buf[1]);
				if (pcm_bytes == 4)
					gen_tone(x, line_id, (aud_buf[1]+160));

				msg_len = write(tdm_fd, aud_buf[1], buff_size);
				if (msg_len <= 0) {
					printf("write() failed\n");
					return;
				}
			}

			/* Check hook state */
			wait_for_vpapi_event();

			/* Reload timeout */
			timeout.tv_usec = TIMEOUT;
		}
	}
}

static void sw_loopback_multi_phones_test(int tdm_fd, unsigned char start_line, unsigned char end_line)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;
	unsigned char line_id;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until at least one line goes off-hook */
	while(offhook_count == 0) {
		wait_for_vpapi_event();
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while(offhook_count) {

		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				return;
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds))
		{
			msg_len = read(tdm_fd, aud_buf[0], buff_size);										     if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}

			for(line_id = start_line; line_id < end_line; line_id+=2) {
				memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], (line_id+1)), CH_BUFF_SIZE);
				memcpy(BUFF_ADDR(aud_buf[1], (line_id+1)), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
			}
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds))
		{
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_vpapi_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void sw_loopback_two_phones_test(int tdm_fd, unsigned char line0, unsigned char line1)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	/* Wait until both lines go off-hook */
	while((hook_state[line0] == 0) || (hook_state[line1] == 0)) {
		wait_for_vpapi_event();
	}


	while((hook_state[line0] == 1) && (hook_state[line1] == 1)) {

		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				return;
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds))
		{
			msg_len = read(tdm_fd, aud_buf[0], buff_size);										     if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line0), BUFF_ADDR(aud_buf[0], line1), CH_BUFF_SIZE);
			memcpy(BUFF_ADDR(aud_buf[1], line1), BUFF_ADDR(aud_buf[0], line0), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds))
		{
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_vpapi_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if(ioctl(tdm_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static int vpapi_init(void)
{
	int i = 0;
	VpDeviceIdType dev_id = 0;
	VpLineIdType line_id = 0;
	VpStatusType status;
	vpapi_init_device_params_t params;

	/* Check params */
	if(total_lines > MAX_LINES) {
		printf("## Error, total number of lines(%d) exceeded maximum(%d) ##\n", total_lines, MAX_LINES);
		return -1;
	}

	/* Set lines status to on-hook */
	memset(hook_state, 0, MAX_LINES);

	cal_devs = 0;
#ifdef LINE_CALIBRATION_SUPPORT
	cal_lines = 0;
#endif
	/* Fill time slot table */
	memset(time_slot_table, 0, MAX_LINES);
	for(i = 0; i < total_lines; i++)
		time_slot_table[i] = ((i+1) * pcm_bytes); /* skip slot #0 */

	/* Extract PCM format */
	switch(pcm_bytes)
	{
		case 1:
			codec = VP_OPTION_ALAW;
			break;
		case 2:
			codec = VP_OPTION_LINEAR;
			break;
		case 4:
			codec = VP_OPTION_WIDEBAND;
			break;
		default:
			codec = VP_OPTION_ALAW;
			printf("## Warning, wrong PCM size - set to default(ALAW) ##\n");
			break;
	}

#if defined(ZARLINK_SLIC_VE792)
	/* Bring up the power supply */
	if(vpapi_battary_on(VBH, VBL, VBP)) {
		printf("## Error, VE792 power supply could not initialized properly ##\n");
                return -1;
	}
#endif
	/* Create max device objects */
	for(dev_id = 0; dev_id < total_devs; dev_id++) {

		status = vpapi_make_dev_object(VP_DEV_SERIES, dev_id);
		if (status != VP_STATUS_SUCCESS) {
			printf("## Error, device %d could not initialized properly(status=%d) ##\n", dev_id, status);
			return -1;
		}

#if defined(ZARLINK_SLIC_VE792)
		status = vpapi_map_slac_id(dev_id, 0);
		if (status != VP_STATUS_SUCCESS) {
			printf("## Error, SLAC %d could not be mapped(status=%d) ##\n", dev_id, status);
			return -1;
		}
#endif
		/* Create requested channels for each device */
		while((line_id < ((dev_id+1)*MAX_DEVICE_LINES)) && (line_id < total_lines)) {
			status = vpapi_make_line_object(VP_TERM_FXS_GENERIC, line_id);
			if (status != VP_STATUS_SUCCESS) {
				printf("## Error, line %d of device %d could not initialized \
						properly(status=%d) ##\n", line_id, dev_id, status);
				return -1;
			}


			/* Map unique LineId to LineCtx */
			status = vpapi_map_line_id(line_id);

			if (status != VP_STATUS_SUCCESS) {
				printf("## Error, line %d for device %d could not \
						mapped(status=%d) ##\n", line_id, dev_id, status);
				return -1;
			}
			line_id++;
		}

		params.dev_size = dev_profile_size;
		params.ac_size = ac_profile_size;
		params.dc_size = dc_profile_size;
		params.ring_size = ring_profile_size;
		params.fxo_ac_size = 0;
		params.fxo_cfg_size = 0;

		if (pcm_bytes < 4)
			status = vpapi_init_device(dev_id, DEV_PROFILE, AC_COEFF, DC_COEFF, RING_PROFILE, NULL, NULL, &params);
		else
			status = vpapi_init_device(dev_id, DEV_PROFILE, WB_AC_COEFF, DC_COEFF, RING_PROFILE, NULL, NULL, &params);

		if (status != VP_STATUS_SUCCESS)
		{
			printf("## Error, device(%d) init failed(status=%d)\n", dev_id, status);
			return -1;
		}
	}

	return 0;
}

static inline void wait_for_vpapi_event(void)
{
	bool status;
	VpEventType event;
	VpDeviceIdType dev_id;

	for(dev_id = 0; dev_id < total_devs ; dev_id++) {
		while(vpapi_get_event(dev_id, &event) == true) {
			switch(event.eventCategory) {
				case VP_EVCAT_SIGNALING:
					switch(event.eventId) {
						case VP_LINE_EVID_HOOK_OFF:
							if(cal_devs == total_devs) {
								printf("off-hook(%d)\n", event.lineId);
								hook_state[event.lineId] = 1;
								offhook_count++;
								vpapi_set_line_state(event.lineId, VP_LINE_TALK);
							}
							break;

						case VP_LINE_EVID_HOOK_ON:
							if(cal_devs == total_devs) {
								printf("on-hook(%d)\n", event.lineId);
								hook_state[event.lineId] = 0;
								offhook_count--;
								vpapi_set_line_state(event.lineId, VP_LINE_STANDBY);
							}
							break;

						default:
							/*printf("Unknown SIGNALING event[id-0x%x][lineId-%d]\n",event.eventId, event.lineId);*/
							break;
					}
					break;

				case VP_EVCAT_RESPONSE:
					switch(event.eventId) {
						case VP_DEV_EVID_DEV_INIT_CMP:
							printf("Zarlink telephony device(%d) initialized successfully\n", event.deviceId);
							vpapi_dev_init(event.deviceId);
							cal_devs++;
							break;

						case VP_EVID_CAL_CMP:
#ifdef LINE_CALIBRATION_SUPPORT
							cal_lines++;
							if(cal_lines == total_lines) {
								/*VpOptionLoopbackType lp = VP_OPTION_LB_TIMESLOT;
								vpapi_set_option(1, 0, 0, VP_OPTION_ID_LOOPBACK, &lp); */
								printf("Zarlink telephony lines(%d) calibrated successfully\n", total_lines);
								vpapi_init_done();
							}
#endif
							break;

						default:
							/*printf("Unknown RESPONSE event[id-0x%x][lineId-%d]\n",event.eventId, event.lineId);*/
							break;
					}
					break;

				case VP_EVCAT_FAULT:
					printf("Got FAULT event[id-0x%x][lineId-%d]\n",event.eventId, event.lineId);
					break;

				default:
					printf("Got event[category-0x%x][id-%d]\n",event.eventCategory,event.eventId);
					break;
			}
		}
	}
}

static void vpapi_init_done(void)
{
	VpStatusType status;
	VpDeviceIdType dev_id;
	VpLineIdType line_id, base_line_id;

	for (dev_id = 0; dev_id < total_devs; dev_id++)
	{
		base_line_id = (dev_id * MAX_DEVICE_LINES);
		for (line_id = base_line_id; line_id < (base_line_id + MAX_DEVICE_LINES); line_id++)
		{
			/* Set CODEC options */
			status = vpapi_set_option(1, line_id, dev_id, VP_OPTION_ID_CODEC, &codec);
			if(status != VP_STATUS_SUCCESS)
			{
				printf("## Error setting VP_OPTION_ID_CODEC (%d) ##\n", status);
				return;
			}
		}
	}

	return;
}

static void vpapi_dev_init(VpDeviceIdType dev_id)
{
	VpOptionEventMaskType event_mask;
	VpStatusType status;
	VpOptionTimeslotType time_slot;
	VpLineIdType line_id;

	/* Clear all events */
	memset(&event_mask, 0xff, sizeof(VpOptionEventMaskType));

	event_mask.faults = (unsigned short)VP_EVCAT_FAULT_UNMASK_ALL;
	event_mask.signaling = (unsigned short)(~(VP_LINE_EVID_HOOK_OFF | VP_LINE_EVID_HOOK_ON));
#ifdef LINE_CALIBRATION_SUPPORT
	event_mask.response = (unsigned short)(~VP_EVID_CAL_CMP);
#endif
	status = vpapi_set_option(0, N_A, dev_id, VP_OPTION_ID_EVENT_MASK, &event_mask);
	if(status != VP_STATUS_SUCCESS)
	{
		printf("## Error while setting VP_OPTION_ID_EVENT_MASK (%d) ##\n", status);
		return;
	}

	line_id = (dev_id * MAX_DEVICE_LINES);
	while((line_id < ((dev_id+1)*MAX_DEVICE_LINES)) && (line_id < total_lines)) {

		vpapi_set_line_state(line_id, VP_LINE_STANDBY);

		/* Configure PCM timeslots */
		time_slot.tx = time_slot.rx = time_slot_table[line_id];

		/*printf("## INFO: line(%d): rx-slot(%d) , tx-slot(%d) ##\n", line_id, time_slot.rx, time_slot.tx);*/

		status = vpapi_set_option(1, line_id, dev_id, VP_OPTION_ID_TIMESLOT, &time_slot);
		if(status != VP_STATUS_SUCCESS)
		{
			printf("## Error setting VP_OPTION_ID_TIMESLOT (%d) ##\n", status);
			return;
		}
#if 0
		/* Set CODEC options */
		status = vpapi_set_option(1, line_id, dev_id, VP_OPTION_ID_CODEC, &codec);
		if(status != VP_STATUS_SUCCESS)
		{
			printf("## Error setting VP_OPTION_ID_CODEC (%d) ##\n", status);
			return;
		}
#endif
#ifdef LINE_CALIBRATION_SUPPORT
		/* Start line calibration */
		vpapi_cal_line(line_id);
#endif
		line_id++;
	}
}

static void vpapi_release(void)
{
	VpDeviceIdType dev_id = 0;
	VpLineIdType line_id = 0;
	VpOptionEventMaskType event_mask;
	VpStatusType status;


	/* Clear all events */
	memset(&event_mask, 0xff, sizeof(VpOptionEventMaskType));

	for(dev_id = 0; dev_id < total_devs; dev_id++) {
		/* Mask all interrupts */
		status = vpapi_set_option(0, N_A, dev_id, VP_OPTION_ID_EVENT_MASK, &event_mask);

		if(status != VP_STATUS_SUCCESS) {
			printf("Error while setting VP_OPTION_ID_EVENT_MASK (%d)\n", status);
			continue;
		}

		while((line_id < ((dev_id+1)*MAX_DEVICE_LINES)) && (line_id < total_lines)) {
			/* Place each line in DISCONNECT state */
			vpapi_set_line_state(line_id, VP_LINE_DISCONNECT);

			/* Free line context */
			status = vpapi_free_line_context(line_id);
			if(status != VP_STATUS_SUCCESS)
			{
				printf("Error while free line %d context\n", line_id);
				continue;
			}
			line_id++;
		}
	}
#if defined(ZARLINK_SLIC_VE792)
	/* Shut down the power supply */
	if(vpapi_battery_off()) {
		printf("## Error while shutting down VE792 power supply ##\n");
		sleep(1);
	}
#endif
}
