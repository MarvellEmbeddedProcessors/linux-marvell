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
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "tal/tal_dev.h"
#include "libtapi.h"

#define TAPI_DEVICE                     0
#define TAPI_CHANNELS                   2
#define TOOL_PREFIX                     ">> "

#define TIMEOUT                         11000 /* usec */
#define BUFF_ADDR(buff, line)           ((unsigned char *)buff + (line * 80 * pcm_bytes))
#define CH_BUFF_SIZE                    (80 * pcm_bytes)
#define DFEV_DEFAULT_RX_GAIN            (-3)
#define DFEV_DEFAULT_TX_GAIN            (-2)
static const char dev_name[]      = "/dev/tal";
static const char tapi_dev_name[] = "/dev/sdd";

static int tal_fd;
static tapi_handle_t th;
static unsigned char hook_state[TAPI_CHANNELS];
static unsigned int cal_lines;
static int offhook_count;
static int buff_size;
static int slic_init;
static int pcm_bytes;

static unsigned short f1Mem;
static unsigned short f2Mem;
static unsigned char aud_buf[2][320 * TAPI_CHANNELS];

/* sin table, 256 points */
static short sinTbl[] = { 0,      402,    804,    1205,   1606,   2005,   2404,   2801,   3196,   3590,   3981,   4370,   4756,
			  5139,   5519,   5896,   6270,   6639,   7005,   7366,   7723,   8075,   8423,   8765,   9102,   9433,  9759,   10079, 10393,
			  10701,  11002,  11297,  11585,  11865,  12139,  12405,  12664,  12915,  13159,  13394,  13622,  13841, 14052,
			  14255,  14449,  14634,  14810,  14977,  15136,  15285,  15425,  15556,  15678,  15790,  15892,  15985, 16068,
			  16142,  16206,  16260,  16304,  16339,  16363,  16378,  16383,  16378,  16363,  16339,  16304,  16260, 16206,
			  16142,  16068,  15985,  15892,  15790,  15678,  15556,  15425,  15285,  15136,  14977,  14810,  14634, 14449,
			  14255,  14052,  13841,  13622,  13394,  13159,  12915,  12664,  12405,  12139,  11865,  11585,  11297, 11002,
			  10701,  10393,  10079,  9759,   9433,   9102,   8765,   8423,   8075,   7723,   7366,   7005,   6639,  6270,   5896,  5519,
			  5139,   4756,   4370,   3981,   3590,   3196,   2801,   2404,   2005,   1606,   1205,   804,    402,   0,      -402,  -804, -1205, -1606,
			  -2005,  -2404,  -2801,  -3196,  -3590,  -3981,  -4370,  -4756,  -5139,  -5519,  -5896,  -6270,  -6639, -7005,
			  -7366,  -7723,  -8075,  -8423,  -8765,  -9102,  -9433,  -9759,  -10079, -10393, -10701, -11002, -11297,
			  -11585, -11865, -12139, -12405, -12664, -12915, -13159, -13394, -13622, -13841, -14052, -14255,
			  -14449, -14634, -14810, -14977, -15136, -15285, -15425, -15556, -15678, -15790, -15892, -15985,
			  -16068, -16142, -16206, -16260, -16304, -16339, -16363, -16378, -16383, -16378, -16363, -16339,
			  -16304, -16260, -16206, -16142, -16068, -15985, -15892, -15790, -15678, -15556, -15425, -15285,
			  -15136, -14977, -14810, -14634, -14449, -14255, -14052, -13841, -13622, -13394, -13159, -12915,
			  -12664, -12405, -12139, -11865, -11585, -11297, -11002, -10701, -10393, -10079, -9759,  -9433,  -9102,
			  -8765,  -8423,  -8075,  -7723,  -7366,  -7005,  -6639,  -6270,  -5896,  -5519,  -5139,  -4756,  -4370, -3981,
			  -3590,  -3196,  -2801,  -2404,  -2005,  -1606,  -1205,  -804,   -402,   0 };

static void wait_for_event(int offhook, int block)
{
	tapi_event_t event;

	if (tapi_event_get(th, TAPI_DEVICE, (block) ? -1 : 0, &event) <= 0)
		return;

	switch (event.event) {
	case TAPI_EVENT_FXS_ONHOOK:
		printf("on-hook(%d)\n", event.channel);
		hook_state[event.channel] = 0;
		offhook_count--;
		if (offhook)
			tapi_line_feed_set(th, event.device, event.channel,
					   TAPI_LINE_FEED_STANDBY);
		else
			tapi_line_feed_set(th, event.device, event.channel,
					   TAPI_LINE_FEED_ONHOOK_ACTIVE);
		break;
	case TAPI_EVENT_FXS_OFFHOOK:
		printf("off-hook(%d)\n", event.channel);
		hook_state[event.channel] = 1;
		offhook_count++;
		tapi_line_feed_set(th, event.device, event.channel,
				   TAPI_LINE_FEED_OFFHOOK_ACTIVE);
		break;
	case TAPI_EVENT_CALIBRATION_END:
		cal_lines += 1;
		break;
	default:
		break;
	}
}

static void release(int signum)
{
	/* Stop SLIC/s */
	if (slic_init) {
		printf("\n%s Stopping SLIC\n", TOOL_PREFIX);
		tapi_dev_stop(th, TAPI_DEVICE);
		printf("\n%s Stopped SLIC\n", TOOL_PREFIX);
		slic_init = 0;
	} else
		printf("\n%s SLIC already stopped\n", TOOL_PREFIX);

	/* Stop TAL */
	if (ioctl(tal_fd, TAL_DEV_EXIT, 0)) {
		printf("\n%s Error, unable to stop TAL\n", TOOL_PREFIX);
		return;
	} else
		printf("\n%s TAL stopped\n", TOOL_PREFIX);

	if (signum) {
		close(tal_fd);
		tapi_close(th);
		exit(0);
	}
}

static void gen_tone(unsigned short freq, unsigned char line_id, unsigned char *tx_buff)
{
	short i;
	short buf[80];
	short sample;

	for (i = 0; i < 80; i++) {
		sample = (sinTbl[f1Mem >> 8] + sinTbl[f2Mem >> 8]) >> 2;
#ifndef CONFIG_CPU_BIG_ENDIAN
		buf[i] = sample;
#else
		buf[i] = ((sample & 0xff) << 8) + (sample >> 8);
#endif
		f1Mem += freq;
		f2Mem += freq;
	}
	memcpy(BUFF_ADDR(tx_buff, line_id), (void *)buf, 160);
}

static void sw_tone_test(int tal_fd, unsigned char line_id)
{
	fd_set wr_fds;
	struct timeval timeout = { 0, TIMEOUT };
	int msg_len, x;
	char str[4];

	if (tal_fd <= 0) {
		printf("%s Device is not accessible\n", TOOL_PREFIX);
		return;
	}

	while (1) {
		printf("%s Choose frequency: (1) 300HZ (2) 630HZ (3) 1000HZ (4) 1000HZ ON-HOOK (5) Back to main menu: ",
			TOOL_PREFIX);
		fgets(str, sizeof(str), stdin);

		if (str[0] == '1')
			x = 2457;
			/* printf("%s Generating 300HZ tone\n", TOOL_PREFIX); */
		else if (str[0] == '2')
			x = 5161;
			/* printf("%s Generating 630HZ tone\n", TOOL_PREFIX); */
		else if (str[0] == '3')
			x = 8192;
		else if (str[0] == '4')
			x = 8192;
			/* printf("%s Generating 1000HZ tone\n", TOOL_PREFIX); */
		else if (str[0] == '5')
			return;
		else {
			printf("%s Input error - try again\n", TOOL_PREFIX);
			continue;
		}

		if (str[0] == '4') {

			printf("%s Waiting for on-hook...\n", TOOL_PREFIX);

			/* Wait until both lines goes on-hook */
			while (hook_state[line_id] == 1)
				wait_for_event(0, 1);

			/* Active the line */
			tapi_line_feed_set(th, TAPI_DEVICE, line_id,
					   TAPI_LINE_FEED_ONHOOK_ACTIVE);

			if (ioctl(tal_fd, TAL_DEV_PCM_START, 0)) {
				printf("Error, unable to start pcm bus\n");
				return;
			}

			printf("%s Waiting for off-hook to return to menu.\n", TOOL_PREFIX);

			while (hook_state[line_id] == 0) {
				FD_ZERO(&wr_fds);
				FD_SET(tal_fd, &wr_fds);

				/* Wait for event  */
				if (select(tal_fd + 1, NULL, &wr_fds, NULL, &timeout) == 0) {
					printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
					return;
				}

				/* Write */
				if (FD_ISSET(tal_fd, &wr_fds)) {
					gen_tone(x, line_id, aud_buf[1]);
					if (pcm_bytes == 4)
						gen_tone(x, line_id, (aud_buf[1] + 160));

					msg_len = write(tal_fd, aud_buf[1], buff_size);
					if (msg_len <= 0) {
						printf("write() failed\n");
						return;
					}
				}

				/* Check hook state */
				wait_for_event(0, 0);

				/* Reload timeout */
				timeout.tv_usec = TIMEOUT;
			}

			if (ioctl(tal_fd, TAL_DEV_PCM_STOP, 0)) {
				printf("Error, unable to stop pcm bus\n");
				return;
			}
		} else {
			printf("%s Waiting for off-hook...\n", TOOL_PREFIX);

			/* Wait until both lines go off-hook */
			while (hook_state[line_id] == 0)
				wait_for_event(1, 1);

			if (ioctl(tal_fd, TAL_DEV_PCM_START, 0)) {
				printf("Error, unable to start pcm bus\n");
				return;
			}

			printf("%s Waiting for on-hook to return to menu.\n", TOOL_PREFIX);

			while (hook_state[line_id] == 1) {
				FD_ZERO(&wr_fds);
				FD_SET(tal_fd, &wr_fds);

				/* Wait for event  */
				if (select(tal_fd + 1, NULL, &wr_fds, NULL, &timeout) == 0) {
					printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
					return;
				}

				/* Write */
				if (FD_ISSET(tal_fd, &wr_fds)) {
					gen_tone(x, line_id, aud_buf[1]);
					if (pcm_bytes == 4)
						gen_tone(x, line_id, (aud_buf[1] + 160));

					msg_len = write(tal_fd, aud_buf[1], buff_size);
					if (msg_len <= 0) {
						printf("write() failed\n");
						return;
					}
				}

				/* Check hook state */
				wait_for_event(1, 0);

				/* Reload timeout */
				timeout.tv_usec = TIMEOUT;
			}

			if (ioctl(tal_fd, TAL_DEV_PCM_STOP, 0)) {
				printf("Error, unable to stop pcm bus\n");
				return;
			}
		}
	}
}

static void sw_loopback(int tal_fd, unsigned char line_id)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = { 0, TIMEOUT };
	int msg_len;

	if (tal_fd <= 0) {
		printf("%s Device is not accessible\n", TOOL_PREFIX);
		return;
	}

	/* Wait until line goes off-hook */
	while (hook_state[line_id] == 0)
		wait_for_event(1, 1);

	if (ioctl(tal_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while (hook_state[line_id] == 1) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd + 1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			return;
		}

		/* Read */
		if (FD_ISSET(tal_fd, &rd_fds)) {
			/* printf("Rd\n"); */
			msg_len = read(tal_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tal_fd, &wr_fds)) {
			/* printf("Wr\n"); */
			msg_len = write(tal_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_event(1, 0);

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tal_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void sw_loopback_two_phones_test(int tal_fd, unsigned char line0, unsigned char line1)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = { 0, TIMEOUT };
	int msg_len;

	if (tal_fd <= 0) {
		printf("%s Device is not accessible\n", TOOL_PREFIX);
		return;
	}

	/* Wait until both lines go off-hook */
	while ((hook_state[line0] == 0) || (hook_state[line1] == 0))
		wait_for_event(1, 1);

	if (ioctl(tal_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while ((hook_state[line0] == 1) && (hook_state[line1] == 1)) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd + 1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			return;
		}

		/* Read */
		if (FD_ISSET(tal_fd, &rd_fds)) {
			msg_len = read(tal_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line0), BUFF_ADDR(aud_buf[0], line1), CH_BUFF_SIZE);
			memcpy(BUFF_ADDR(aud_buf[1], line1), BUFF_ADDR(aud_buf[0], line0), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tal_fd, &wr_fds)) {
			msg_len = write(tal_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_event(1, 0);

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tal_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

int main(int argc, char *argv[])
{
	int fdflags, rxgain, txgain;
	tal_dev_params_t tal_params;
	unsigned int i, j;
	char str[8];

	/* Open TAL device */
	tal_fd = open(dev_name, O_RDWR);
	if (tal_fd < 0) {
		printf("%s Cannot open TAL device.\n", TOOL_PREFIX);
		return 1;
	}

	fdflags = fcntl(tal_fd, F_GETFL, 0);
	fdflags |= O_NONBLOCK;
	fcntl(tal_fd, F_SETFL, fdflags);

	printf("\n%s Please enter PCM sample size (2/4): ", TOOL_PREFIX);
	fgets(str, sizeof(str), stdin);
	pcm_bytes = atoi(str);

	/* Calculate total lines buffer size */
	buff_size = (80 * pcm_bytes * TAPI_CHANNELS);

	/* Fill TAL info */
	tal_params.pcm_format = pcm_bytes;
	tal_params.total_lines = TAPI_CHANNELS;

	/* Open TAPI device */
	th = tapi_open(tapi_dev_name);
	if (th == NULL) {
		printf("%s Error, could not open tapi device.\n", TOOL_PREFIX);
		return 1;
	}

	/* Handle termination gracefully */
	if (signal(SIGINT, release) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	/* Issue main menu */
	printf("\n !!! Remember to start phone devices before performing any action !!!\n", TOOL_PREFIX);
	while (1) {
		printf("\n  Marvell Voice Tool (DFEV Edition):\n");
		printf("  2. Start ring\n");
		printf("  3. Stop ring\n");
		printf("  4. Start SW Dial tone\n");
		printf("  5. Self echo on local phone\n");
		printf("  6. Loopback two local phones\n");
		printf("  a. Start Phone devices\n");
		printf("  b. Stop Phone devices\n");
		printf("  g. Set line gain\n");
		printf("  q. Quit\n");
		printf("\n%s Please select option: ", TOOL_PREFIX);

		fgets(str, sizeof(str), stdin);
		switch (str[0]) {
		case '2':
			printf("%s Enter line id: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			i = atoi(str);
			printf("%s Start ringing on line %d\n", TOOL_PREFIX, i);
			if (tapi_ring_start(th, TAPI_DEVICE, i) < 0)
				printf("%s Error, could not change line state.\n", TOOL_PREFIX);
			break;

		case '3':
			printf("%s Enter line id: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			i = atoi(str);
			printf("%s Stop ringing on line %d\n", TOOL_PREFIX, i);
			if (tapi_ring_stop(th, TAPI_DEVICE, i) < 0)
				printf("%s Error, could not change line state.\n", TOOL_PREFIX);
			break;

		case '4':
			printf("%s Enter line id: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			i = atoi(str);
			sw_tone_test(tal_fd, i);
			break;

		case '5':
			printf("%s Enter line id: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			printf("%s Waiting for off-hook...\n", TOOL_PREFIX);
			i = atoi(str);
			sw_loopback(tal_fd, i);
			break;
		case '6':
			printf("%s Enter line #0: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			i = atoi(str);
			printf("%s Enter line #1: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			printf("Waiting for off-hook...\n");
			j = atoi(str);
			if (i >= TAPI_CHANNELS || j >= TAPI_CHANNELS) {
				printf("%s Error, line must be in the range of 0-%d\n", TOOL_PREFIX, (TAPI_CHANNELS - 1));
				break;
			}

			sw_loopback_two_phones_test(tal_fd, i, j);
			break;

		case 'a':
			/* Start Telephony */
			if (ioctl(tal_fd, TAL_DEV_INIT, &tal_params)) {
				printf("%s Error, unable to init TAL\n", TOOL_PREFIX);
				return 1;
			}

			if (tapi_fw_download(th, TAPI_DEVICE, NULL) < 0) {
				printf("%s Error, could not load tapi firmware.\n", TOOL_PREFIX);
				break;
			}

			if (tapi_dev_start(th, TAPI_DEVICE) < 0) {
				printf("%s Error, could not start tapi device.\n", TOOL_PREFIX);
				break;
			}

			if (tapi_bbd_download(th, TAPI_DEVICE, 0, NULL) < 0) {
				printf("%s Error, could not load tapi configuration.\n", TOOL_PREFIX);
				break;
			}

			usleep(1000);

			if (tapi_bbd_download(th, TAPI_DEVICE, 1, NULL) < 0) {
				printf("%s Error, could not load tapi configuration.\n", TOOL_PREFIX);
				break;
			}

			for (i = 0; i < TAPI_CHANNELS; i++) {
				if (tapi_event_enable(th, TAPI_DEVICE, i, TAPI_EVENT_FXS_ONHOOK) < 0) {
					printf("%s Error, could enable event reporting.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_event_enable(th, TAPI_DEVICE, i, TAPI_EVENT_FXS_OFFHOOK) < 0) {
					printf("%s Error, could enable event reporting.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_event_enable(th, TAPI_DEVICE, i, TAPI_EVENT_CALIBRATION_END) < 0) {
					printf("%s Error, could enable event reporting.\n", TOOL_PREFIX);
					break;
				}
			}

			if (i != TAPI_CHANNELS)
				break;

			/* Wait to device/s and line/s calibration to finish */
			cal_lines = 0;
			while (cal_lines < TAPI_CHANNELS) {
				printf("%s SLIC: Calibrating line %u ...\n", TOOL_PREFIX, cal_lines);

				if (tapi_line_calibrate(th, TAPI_DEVICE, cal_lines) < 0) {
					printf("%s Error, could not calibrate line.\n", TOOL_PREFIX);
					break;
				}

				wait_for_event(1, 1);
			}

			if (cal_lines != TAPI_CHANNELS)
				break;

			for (i = 0; i < TAPI_CHANNELS; i++) {
				if (tapi_line_feed_set(th, TAPI_DEVICE, i, TAPI_LINE_FEED_STANDBY) < 0) {
					printf("%s Error, could not set line feed.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_phone_volume_set(th, TAPI_DEVICE, i, DFEV_DEFAULT_RX_GAIN, DFEV_DEFAULT_TX_GAIN) < 0) {
					printf("%s Error, could not set line gain.\n", TOOL_PREFIX);
					break;
				}
			}

			if (i != TAPI_CHANNELS)
				break;

			slic_init = 1;
			break;

		case 'b':
			release(0);
			break;

		case 'g':
			printf("%s Enter line id: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			i = atoi(str);

			printf("%s Enter RX gain [dB]: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			rxgain = atoi(str);

			printf("%s Enter TX gain [dB]: ", TOOL_PREFIX);
			fgets(str, sizeof(str), stdin);
			txgain = atoi(str);

			if (tapi_phone_volume_set(th, TAPI_DEVICE, i, rxgain, txgain) < 0)
				printf("%s Error, could not set line gain.\n", TOOL_PREFIX);
			break;

		case 'q':
			goto voice_out;

		default:
			printf("Option is not supported - try again\n");
			break;
		}
	}

voice_out:
	release(1);

	return 0;
}
