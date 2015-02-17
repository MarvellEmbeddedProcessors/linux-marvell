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

#include "tal/tal_dev.h"
#include "libtapi.h"

#define	TAPI_DEVICE			0
#define TAPI_CHANNELS			2
#define TAPI_IRQ			0	/* Polling mode */

#define MAX_LINES			TAPI_CHANNELS
#define	MAX_DEVICE_LINES		TAPI_CHANNELS

#define TIMEOUT				11000 /* usec */
#define TOOL_PREFIX			">> "

#define MAX_SLIC_RDWR_BUFF_SIZE         128
#define CH_BUFF_SIZE			(80 * pcm_bytes)
#define BUFF_ADDR(buff, line)		((unsigned char*)buff + (line*80*pcm_bytes))

/* GLobals */
static unsigned short total_lines = 0;
static unsigned short total_devs = 0;
static unsigned char pcm_bytes = 0;
static unsigned short cal_lines = 0;
static unsigned char time_slot_table[MAX_LINES];
static unsigned char hook_state[MAX_LINES];
static const char dev_name[] = "/dev/tal";
static const char tapi_name[] = "/dev/tapi";
static int buff_size = 0;
static unsigned char aud_buf[2][320 * MAX_LINES];
static unsigned short f1Mem = 0;
static unsigned short f2Mem = 0;
static int offhook_count = 0;
static unsigned int event_count = 0;
static int tal_fd;
static unsigned char data_buff[MAX_SLIC_RDWR_BUFF_SIZE];
static int slic_init = 0;
static tapi_handle_t th;

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

/* Function declarations */
static void sw_tone_test(int tal_fd, unsigned char line_id);
static void sw_loopback(int tal_fd, unsigned char line_id);
static void sw_loopback_two_phones_test(int tal_fd, unsigned char line0, unsigned char line1);
static void sw_loopback_multi_phones_test(int tal_fd, unsigned char start_line, unsigned char end_line);
static void slic_digital_loopback(int tal_fd, unsigned long int iterations);
static void channel_balancing_test(int tal_fd, unsigned long int iterations);
static void wait_for_event(int block);
static void release(int signum);
#if defined(MV_TDM_USE_DCO)
static void set_tdm_clk_config(void);
static int get_tdm_clk_correction(void);
static void set_tdm_clk_correction(int correction);
#endif

int main(void)
{
	int ret = 0, cmd = 0, val = 0, tdm_init = 0;
	int proc_fd, fdflags, cmd_len, i;
	char str[32];
	unsigned char line0_id, line1_id;
	tdm_dev_params_t tdm_params;
	unsigned long int iterations;
	tapi_pcm_coding_t codec;

	event_count = 0;
	slic_init = 0;

	/* open tdm device */
	tal_fd = open(dev_name, O_RDWR);
	if (tal_fd <= 0) {
		printf("%s Cannot open %s device\n", TOOL_PREFIX, dev_name);
		return 1;
	}

	th = tapi_open(tapi_name);
	if (th == NULL) {
		printf("%s Error, could not open tapi device.\n", TOOL_PREFIX);
		return 1;
	}

	/* set some flags */
	fdflags = fcntl(tal_fd, F_GETFL, 0);
	fdflags |= O_NONBLOCK;
	fcntl(tal_fd, F_SETFL, fdflags);

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

	total_devs = (total_lines/MAX_DEVICE_LINES);
	if((total_lines % MAX_DEVICE_LINES))
		total_devs++;

	/* Handle termination gracefully */
	if (signal (SIGINT, release) == SIG_IGN)
		signal (SIGINT, SIG_IGN);

	printf("\n !!! Remember to start phone devices before performing any action !!!\n", TOOL_PREFIX);

	/* Issue main menu */
	while(1) {
		printf("\n  Marvell Voice Tool (Lantiq Edition):\n");
		printf("  2. Start ring\n");
		printf("  3. Stop ring\n");
		printf("  4. Start SW Dial tone\n");
		printf("  5. Self echo on local phone\n");
		printf("  6. Loopback two local phones\n");
		printf("  7. Multiple local phone pairs loopback\n");
		printf("  8. Digital Loopback (incremental pattern)\n");
		printf("  9. Channel balancing\n");
		printf("  a. Start Phone devices\n");
		printf("  b. Stop Phone devices\n");
#if defined(MV_TDM_USE_DCO)
		printf("  c. Config TDM PCLK\n");
		printf("  d. Get current TDM PCLK frequency correction (DCO)\n");
		printf("  e. Set TDM PCLK frequency correction (DCO)\n");
#endif
		printf("  q. Quit\n");
		printf("\n%s Please select option: ", TOOL_PREFIX);

		/* Clear write buffer */
		memset(aud_buf[1], 0, buff_size);
		gets(str);
		switch(str[0]) {
			case '2':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Start ringing on line %d\n", line0_id);
				tapi_ring_start(th, TAPI_DEVICE, line0_id);
				break;

			case '3':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Stop ringing on line %d\n", line0_id);
				tapi_ring_stop(th, TAPI_DEVICE, line0_id);
				break;

			case '4':
				if(pcm_bytes < 2) {
					printf("Test is supported for linear mode only - try again\n");
					break;
				}
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				sw_tone_test(tal_fd, line0_id);
				break;

			case '5':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				printf("%s Waiting for off-hook...\n", TOOL_PREFIX);
				line0_id = atoi(str);
				sw_loopback(tal_fd, line0_id);
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
				sw_loopback_two_phones_test(tal_fd, line0_id, line1_id);
				break;

			case '7':
				printf("%s Enter starting line range: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter ending line range: ", TOOL_PREFIX);
				gets(str);
				printf("Waiting for off-hook...\n");
				line1_id = atoi(str);
				if((line0_id >= MAX_LINES) || (line1_id >= MAX_LINES) || ((line1_id-line0_id) % 2 == 0)) {
					printf("%s Error, lines range must be even and \
						between 0-%d\n", TOOL_PREFIX, (MAX_LINES-1));
					break;
				}
				sw_loopback_multi_phones_test(tal_fd, line0_id, line1_id);
				break;

			case '8':
				printf("%s Enter number of iterations(must be greater than 3): ", TOOL_PREFIX);
				gets(str);
				iterations = (unsigned long int)atoi(str);
				if(iterations < 4) {
					printf("Requires at least 4 iterations  - try again\n");
					break;
				}
				slic_digital_loopback(tal_fd, iterations);
				break;

			case '9':
				printf("%s Enter number of iterations('0' - for infinite loop): ", TOOL_PREFIX);
				gets(str);
				iterations = (unsigned long int)atoi(str);
				channel_balancing_test(tal_fd, iterations);
				break;

			case 'a':
				/* Start Telephony */
				if (ioctl(tal_fd, TAL_DEV_INIT, &tdm_params)) {
					printf("%s Error, unable to init TDM.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_basic_init(th, TAPI_DEVICE, TAPI_IRQ) < 0) {
					printf("%s Error, could not initialize tapi device.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_fw_download(th, TAPI_DEVICE, NULL) < 0) {
					printf("%s Error, could not load tapi firmware.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_dev_start(th, TAPI_DEVICE) < 0) {
					printf("%s Error, could not start tapi device.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_bbd_download(th, TAPI_DEVICE, -1, NULL) < 0) {
					printf("%s Error, could not load tapi configuration.\n", TOOL_PREFIX);
					break;
				}

				if (tapi_pcm_if_config(th, TAPI_DEVICE,
				    TAPI_PCM_IF_MODE_SLAVE, TAPI_PCM_IF_FREQ_8192KHZ,
				    TAPI_PCM_IF_SLOPE_FALL, TAPI_PCM_IF_SLOPE_FALL) < 0) {
					printf("%s Error, could not configure PCM interface.\n", TOOL_PREFIX);
					break;
				}

				switch(pcm_bytes) {
				case 1:
					codec = TAPI_PCM_CODING_NB_ALAW_8BIT;
					break;
				case 2:
					codec = TAPI_PCM_CODING_NB_LINEAR_16BIT;
					break;
				case 4:
					codec = TAPI_PCM_CODING_WB_LINEAR_SPLIT_16BIT;
					break;
				default:
					codec = TAPI_PCM_CODING_NB_ALAW_8BIT;
					printf("## Warning, wrong PCM size - set to default(ALAW) ##\n");
					break;
				}

				for (i = 0; i < TAPI_CHANNELS; i++) {
					if (tapi_line_type_set(th, TAPI_DEVICE, i, TAPI_LINE_TYPE_FXS_AUTO) < 0) {
						printf("%s Error, could not set line type.\n", TOOL_PREFIX);
						break;
					}

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
				while((cal_lines < total_lines)) {
					printf("SLIC: Calibrating line %u ...\n", cal_lines);

					if (tapi_line_calibrate(th, TAPI_DEVICE, cal_lines) < 0) {
						printf("%s Error, could not calibrate line.\n", TOOL_PREFIX);
						break;
					}

					wait_for_event(1);
				}

				if (cal_lines != total_lines)
					break;

				for (i = 0; i < TAPI_CHANNELS; i++) {
					if (tapi_line_feed_set(th, TAPI_DEVICE, i, TAPI_LINE_FEED_STANDBY) < 0) {
						printf("%s Error, could not set line feed.\n", TOOL_PREFIX);
						break;
					}

					if (tapi_pcm_channel_config(th, TAPI_DEVICE, i, ((i+1)*pcm_bytes), ((i+1)*pcm_bytes), codec) < 0) {
						printf("%s Error, could not configure PCM channel.\n", TOOL_PREFIX);
						break;
					}

					if (tapi_pcm_channel_activate(th, TAPI_DEVICE, i) < 0) {
						printf("%s Error, could not activate PCM channel.\n", TOOL_PREFIX);
						break;
					}
				}

				if (i != TAPI_CHANNELS)
					break;

				slic_init = 1;
				break;

			case 'b':
				/* Stop Telephony */
				release(0);
				break;

#if defined(MV_TDM_USE_DCO)
			case 'c':
				set_tdm_clk_config();
				break;

			case 'd':
				printf("%s Current PPM correction is (+/-1000): %d", TOOL_PREFIX, get_tdm_clk_correction());
				break;

			case 'e':
				printf("%s Enter number of PPM for correction (+/-1000, 0 to disable correction): ", TOOL_PREFIX);
				gets(str);
				set_tdm_clk_correction((int)atoi(str));
				break;
#endif

			case 'q':
				goto voice_out;

			default:
				printf("Option is not supported - try again\n");
				break;
		}
	}

voice_out:
	release(1);

	return ret;
}

static void release(int signum)
{
	if (signum) {
		printf("\n%s Stopping Phone devices and exit\n", TOOL_PREFIX);
		sleep(1);
	} else {
		printf("\n%s Stopping Phone devices\n", TOOL_PREFIX);
	}

	/* Stop SLIC/s */
	if(slic_init) {
		printf("\n%s Stopping SLIC\n", TOOL_PREFIX);
		tapi_dev_stop(th, TAPI_DEVICE);
		printf("\n%s Stopped SLIC\n", TOOL_PREFIX);
		slic_init = 0;
	} else {
		printf("\n%s SLIC already stopped\n", TOOL_PREFIX);
	}

	/* Stop TDM */
	if (ioctl(tal_fd, TDM_DEV_TDM_STOP, 0)) {
		printf("\n%s Error, unable to stop TDM\n", TOOL_PREFIX);
		return;
	} else {
		printf("\n%s TDM stopped\n", TOOL_PREFIX);
	}

	if (signum) {
		tapi_close(th);
		close(tal_fd);
		exit(signum);
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

static void sw_tone_test(int tal_fd, unsigned char line_id)
{
	fd_set wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, x;
	char str[4];

	if (tal_fd <= 0) {
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
			wait_for_event(1);
		}

		if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
			printf("Error, unable to start pcm bus\n");
			return;
		}

		printf("%s Waiting for on-hook to return to menu.\n", TOOL_PREFIX);

		while(hook_state[line_id] == 1) {
			FD_ZERO(&wr_fds);
			FD_SET(tal_fd, &wr_fds);

			/* Wait for event  */
			if (select(tal_fd+1, NULL, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				return;
			}

			/* Write */
			if (FD_ISSET(tal_fd, &wr_fds)) {
				gen_tone(x, line_id, aud_buf[1]);
				if (pcm_bytes == 4)
					gen_tone(x, line_id, (aud_buf[1]+160));

				msg_len = write(tal_fd, aud_buf[1], buff_size);
				if (msg_len <= 0) {
					printf("write() failed\n");
					return;
				}
			}

			/* Check hook state */
			wait_for_event(0);

			/* Reload timeout */
			timeout.tv_usec = TIMEOUT;
		}

		if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
			printf("Error, unable to stop pcm bus\n");
			return;
		}

	}
}

static void sw_loopback(int tal_fd, unsigned char line_id)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;

	if (tal_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until line goes off-hook */
	while(hook_state[line_id] == 0) {
		wait_for_event(1);
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while(hook_state[line_id] == 1) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			return;
		}

		/* Read */
		if (FD_ISSET(tal_fd, &rd_fds)) {
			// printf("Rd\n");
			msg_len = read(tal_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tal_fd, &wr_fds)) {
			// printf("Wr\n");
			msg_len = write(tal_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_event(0);

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void sw_loopback_two_phones_test(int tal_fd, unsigned char line0, unsigned char line1)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;

	if (tal_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until both lines go off-hook */
	while((hook_state[line0] == 0) || (hook_state[line1] == 0)) {
		wait_for_event(1);
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while((hook_state[line0] == 1) && (hook_state[line1] == 1)) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
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
		wait_for_event(0);

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void sw_loopback_multi_phones_test(int tal_fd, unsigned char start_line, unsigned char end_line)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len;
	unsigned char line_id;

	if (tal_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until at least one line goes off-hook */
	while(offhook_count == 0) {
		wait_for_event(1);
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while(offhook_count) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
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

			for(line_id = start_line; line_id < end_line; line_id+=2) {
				memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], (line_id+1)), CH_BUFF_SIZE);
				memcpy(BUFF_ADDR(aud_buf[1], (line_id+1)), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
			}
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
		wait_for_event(0);

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void wait_for_event(int block)
{
	tapi_event_t event;

	if (tapi_event_get(th, TAPI_DEVICE, (block) ? -1 : 0, &event) <= 0)
		return;

	switch (event.event) {
	case TAPI_EVENT_FXS_ONHOOK:
		printf("on-hook(%d)\n", event.channel);
		hook_state[event.channel] = 0;
		offhook_count--;
		tapi_line_feed_set(th, event.device, event.channel,
							TAPI_LINE_FEED_STANDBY);
		break;
	case TAPI_EVENT_FXS_OFFHOOK:
		printf("off-hook(%d)\n", event.channel);
		hook_state[event.channel] = 1;
		offhook_count++;
		tapi_line_feed_set(th, event.device, event.channel,
							TAPI_LINE_FEED_ACTIVE);
		break;
	case TAPI_EVENT_CALIBRATION_END:
		cal_lines += 1;
		break;
	default:
		break;
	}
}

static int slic_dl_data_compare(int ch)
{
	int i = 0, offset = (ch * pcm_bytes * 80);

	/* Align Tx & Rx data start */
	while((aud_buf[1][offset] != aud_buf[0][offset+i]) && (i < (pcm_bytes * 80)))
		i++;

	if (i >= (offset + (pcm_bytes * 80))) {
		printf("\nError, first Tx byte not found inside Rx buffer\n");
		return -1;
	}

	if (memcmp(&aud_buf[0][offset+i], &aud_buf[1][offset], ((pcm_bytes * 80) - i))) {
		printf("\nDump buffers:\n");
		for(i = offset; i < (offset +(pcm_bytes * 80)); i++)
			printf("write[%d] = 0x%x, read[%d] = 0x%x\n", i, aud_buf[1][i], i, aud_buf[0][i]);
		return -1;
	} else {
		return 0;
	}
}

static void slic_digital_loopback(int tal_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch;
	unsigned long int loops = 0, index;

	if (tal_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Put SLIC/s in loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_test_loop(th, TAPI_DEVICE, ch, 1);

	/* Wait a bit */
	sleep(1);

	/* Put SLIC/s in TALK mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_line_feed_set(th, TAPI_DEVICE, ch, TAPI_LINE_FEED_ACTIVE);

	/* Fill Tx buffer with incremental pattern */
	for(ch = 0; ch < total_lines; ch++) {
		for(index = 0; index < (80 * pcm_bytes); index++)
			aud_buf[1][index + (80 * pcm_bytes * ch)] = (index+ch+2);
	}

	if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	while (loops < iterations) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tal_fd, &rd_fds);
		FD_SET(tal_fd, &wr_fds);

		/* Wait for event  */
		if (select(tal_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			goto slic_dl_out;
		}

		/* Write */
		if (FD_ISSET(tal_fd, &wr_fds)) {
			msg_len = write(tal_fd, aud_buf[1], buff_size);
			if (msg_len < buff_size) {
				printf("write() failed\n");
				goto slic_dl_out;
			}
		}

		/* Read */
		if (FD_ISSET(tal_fd, &rd_fds)) {
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tal_fd, aud_buf[0], buff_size);
			if (msg_len < buff_size) {
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

	if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	/* Disable loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_test_loop(th, TAPI_DEVICE, ch, 0);

	/* Put SLIC/s in STANDBY mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_line_feed_set(th, TAPI_DEVICE, ch, TAPI_LINE_FEED_STANDBY);
}

static void channel_balancing_test(int tal_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch, cb_loop = 0, i;
	unsigned long int loops = 0, index;

	if (tal_fd <= 0) {
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
		tapi_test_loop(th, TAPI_DEVICE, ch, 1);

	/* Wait a bit */
	sleep(1);

	/* Put SLIC/s in TALK mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_line_feed_set(th, TAPI_DEVICE, ch, TAPI_LINE_FEED_ACTIVE);

	/* Wait a bit */
	sleep(1);

	if (iterations == 0)
		iterations = (unsigned long int)(-1); /* Assume infinite */

	while (loops < iterations) {
		cb_loop = 0;
		i = 0;

		if (ioctl(tal_fd, TDM_DEV_PCM_START, 0)) {
			printf("Error, unable to start pcm bus\n");
			return;
		}

		while (cb_loop == 0) {
			FD_ZERO(&rd_fds);
			FD_ZERO(&wr_fds);
			FD_SET(tal_fd, &rd_fds);
			FD_SET(tal_fd, &wr_fds);

			/* Wait for event  */
			if (select(tal_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
				printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
				goto cb_out;
			}

			/* Write */
			if (FD_ISSET(tal_fd, &wr_fds)) {
				msg_len = write(tal_fd, aud_buf[1], buff_size);
				if (msg_len < buff_size) {
					printf("write() failed\n");
					goto cb_out;
				}
			}

			/* Read */
			if (FD_ISSET(tal_fd, &rd_fds)) {
				memset(aud_buf[0], 0, buff_size);
				msg_len = read(tal_fd, aud_buf[0], buff_size);
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
		if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
			printf("Error, unable to stop pcm bus\n");
			return;
		}
		printf("loop #%u\n", loops);
		sleep(1);
	}

cb_out:
	if(cmp_status == 0) {
		printf("\nChannel balancing test PASSED !!!\n");
	} else {
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

	if (ioctl(tal_fd, TDM_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	/* Disable loopback mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_test_loop(th, TAPI_DEVICE, ch, 0);

	/* Put SLIC/s in STANDBY mode */
	for(ch = 0; ch < total_lines; ch++)
		tapi_line_feed_set(th, TAPI_DEVICE, ch, TAPI_LINE_FEED_STANDBY);
}

#if defined(MV_TDM_USE_DCO)
static void set_tdm_clk_config(void)
{
	tdm_dev_clk_t tdm_dev_clk;

	/* Config TDM clock */
	if (ioctl(tal_fd, TDM_DEV_TDM_CLK_CONFIG, &tdm_dev_clk))
		printf("%s Error, unable to config TDM clock.\n", TOOL_PREFIX);
}

static int get_tdm_clk_correction(void)
{
	tdm_dev_clk_t tdm_dev_clk;

	/* Get TDM clock */
	if (ioctl(tal_fd, TDM_DEV_TDM_CLK_GET, &tdm_dev_clk)) {
		printf("%s Error, unable to get TDM clock.\n", TOOL_PREFIX);
		return 0;
	}

	return tdm_dev_clk.correction;
}

static void set_tdm_clk_correction(int correction)
{
	tdm_dev_clk_t tdm_dev_clk;

	tdm_dev_clk.correction = correction;

	/* Set TDM clock */
	if (ioctl(tal_fd, TDM_DEV_TDM_CLK_SET, &tdm_dev_clk))
		printf("%s Error, unable to set TDM clock.\n", TOOL_PREFIX);

}
#endif
