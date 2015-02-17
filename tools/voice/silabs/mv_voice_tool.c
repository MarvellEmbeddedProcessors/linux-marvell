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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tal/tal_dev.h"

#include "silabs_lib.h"
#include "pesqmain.h"
#include <dirent.h>
#if defined(SI3217x)
/* SILABS si3217x */
#include "si3217x_constants.h"
#define MAX_DEVICES		1
#define MAX_DEVICE_LINES	1
#define MAX_LINES		1

#elif defined(SI3226x)
/* SILABS si3226x */
#include "si3226x_constants.h"
#define MAX_DEVICES		4
#define MAX_DEVICE_LINES	2
#define MAX_LINES		8
#endif

#define TOOL_PREFIX		">> "
#define TIMEOUT			11000 /* usec */
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

/* GLobals */
static unsigned short total_lines = 0;
static unsigned short total_devs = 0;
static unsigned char pcm_bytes = 0;
static unsigned char cal_devs = 0;
#ifdef LINE_CALIBRATION_SUPPORT
static unsigned short cal_lines = 0;
#endif
static unsigned char time_slot_table[MAX_LINES];
static unsigned char hook_state[MAX_LINES];
static const char dev_name[] = "/dev/tal";
static int buff_size = 0;
static unsigned char aud_buf[2][320 * MAX_LINES];
static unsigned short f1Mem = 0;
static unsigned short f2Mem = 0;
static int offhook_count = 0;
static unsigned int event_count = 0;
static int tdm_fd = 0;
static unsigned char data_buff[MAX_SLIC_RDWR_BUFF_SIZE];
static unsigned int data_buff_ram[3];
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
static inline void wait_for_silabs_event(void);
static int silabs_init(void);
static void silabs_release(void);
static void release(int signum);
static void sw_tone_test(int tdm_fd, unsigned char line_id);
static void gen_tone(unsigned short freq, unsigned char line_id, unsigned char* tx_buff);
static void sw_loopback(int tdm_fd, unsigned char line_id);
static void sw_loopback_two_phones_test(int tdm_fd, unsigned char line0, unsigned char line1);
static void slic_digital_loopback(int tdm_fd, unsigned long int iterations);
static void sw_loopback_multi_phones_test(int tdm_fd, unsigned char start_line, unsigned char end_line);
static void channel_balancing_test(int tdm_fd, unsigned long int iterations);
static inline int slic_dl_data_compare(int offset);
static void set_tdm_clk_config(void);
#if defined(MV_TDM_USE_DCO)
static int get_tdm_clk_correction(void);
static void set_tdm_clk_correction(int correction);
#endif
static int analog_loopback_test(unsigned char line_id, char *str, char *dir);
static void play_pcm_file_on_phone(int tdm_fd, unsigned char line_id, char
		*filename);
static int enable_disable_loopback(ProslicLoopbackModes loopMode, uInt8 newLineFeed);
static int  read_wav_header(FILE *filename, char *output_name);
static int read_from_wav(FILE *filename, int line_id, long leftsize);
static void record_input(int tdm_fd, unsigned char line);
static int analog_loopback_out(int cmp_status, FILE *or, FILE *dg, char
		*or_name, char *dg_name);
static void finish_al_read(fd_set rd_fds, int *cmp_status_ptr, unsigned char
		line_id, FILE *output_file);
static void slic_analog_loopback(char *dir_name, unsigned char line);
static int analog_loopback_pesq_check(char *or_name, char *dg_name);
int main(void)
{
	int ret = 0, cmd = 0, val = 0, tdm_init = 0, toneEna = 0, preset = 0;
	int proc_fd, fdflags, cmd_len, i, status;
	char str[32];
	unsigned char line0_id, line1_id;
	tal_dev_params_t tal_params;
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
	tal_params.pcm_format = pcm_bytes;
	tal_params.total_lines = total_lines;

	total_devs = (total_lines/MAX_DEVICE_LINES);
	if((total_lines % MAX_DEVICE_LINES))
		total_devs++;

	/* Handle termination gracefully */
	if (signal (SIGINT, release) == SIG_IGN)
		signal (SIGINT, SIG_IGN);

	printf("\n !!! Remember to start phone devices before performing any action !!!\n", TOOL_PREFIX);

	/* Issue main menu */
	while(1) {
		printf("\n  Marvell Voice Tool (Silabs Edition):\n");
		printf("  0. Read from SLIC register\n");
		printf("  1. Write to SLIC register\n");
		printf("  2. Start ring\n");
		printf("  3. Stop ring\n");
		printf("  4. Start/Stop HW Dial tone\n");
		printf("  5. Start SW Dial tone\n");
		printf("  6. Self echo on local phone\n");
		printf("  7. Loopback two local phones\n");
		printf("  8. Digital Loopback\n");
		printf("  9. Channel balancing\n");
		printf("  c. Config TDM PCLK\n");
		printf("  m. Multiple local phone pairs loopback\n");
#if defined(DEBUG)
		printf("  r. Read from SLIC RAM\n");
		printf("  w. Write to SLIC RAM\n");
#endif
#if defined(MV_TDM_USE_DCO)
		printf("  g. Get current TDM PCLK frequency correction (DCO)\n");
		printf("  s. Set TDM PCLK frequency correction (DCO)\n");
#endif
		printf("  t. Start Phone devices\n");
		printf("  u. Stop Phone devices\n");
		printf("  a. Analog Looopback\n");
		printf("  b. record from phone and replay\n");
		printf("  q. Quit\n");
		printf("\n%s Please select option: ", TOOL_PREFIX);

		/* Clear write buffer */
		memset(aud_buf[1], 0, buff_size);
		gets(str);
		switch(str[0]) {
			case '0':
				printf("%s Enter channel id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC register address (decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				cmd_len = 1;
				silabs_slic_reg_read(line0_id, cmd, cmd_len, data_buff);
				printf("%s Return value: ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++)
					printf("0x%x ", data_buff[i]);
				printf("\n");
				break;

			case '1':
				printf("%s Enter channel id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC register address (decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				cmd_len = 1;
				printf("%s Enter data: ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++) {
					gets(str);
					data_buff[i] = atoi(str);
				}
				silabs_slic_reg_write(line0_id, cmd, cmd_len, data_buff);
				break;

			case '2':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Start ringing on line %d\n", line0_id);
				silabs_channel_operation(SI_CHANNEL_OP_RING_START, line0_id);
				break;

			case '3':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("Stop ringing on line %d\n", line0_id);
				silabs_channel_operation(SI_CHANNEL_OP_RING_STOP, line0_id);
				break;

			case '4':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				toneEna = 1-toneEna;
				if (toneEna) {
					printf("%s Enter preset: ", TOOL_PREFIX);
					gets(str);
					preset = atoi(str);
					status = silabs_channel_setup(SI_CHANNEL_SETUP_TONEGEN, line0_id, preset);
					if (status != RC_NONE) {
						printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_TONEGEN) failed (status=%d) ##\n", status);
						return -1;
					}
					status = silabs_channel_operation(SI_CHANNEL_OP_TONE_GEN_START, line0_id);
				} else {
					status = silabs_channel_operation(SI_CHANNEL_OP_TONE_GEN_STOP, line0_id);
				}

				if (status != RC_NONE) {
					printf("## Error, silabs_channel_operation (SI_CHANNEL_OP_TONE_GEN_START) failed (status=%d) ##\n", status);
					return -1;
				}
				break;

			case '5':
				if(pcm_bytes < 2) {
					printf("Test is supported for linear mode only - try again\n");
					break;
				}
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				sw_tone_test(tdm_fd, line0_id);
				break;

			case '6':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				printf("%s Waiting for off-hook...\n", TOOL_PREFIX);
				line0_id = atoi(str);
				sw_loopback(tdm_fd, line0_id);
				break;

			case '7':
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
#if defined(DEBUG)
			case 'r':
				printf("%s Enter channel id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC RAM address (decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				cmd_len = 1;
				data_buff_ram[0] = 0;
				silabs_slic_ram_read(line0_id, cmd, cmd_len, data_buff_ram);
				printf("%s Return value: ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++)
					printf("0x%x ", data_buff_ram[i]);
				printf("\n");
				wait_for_silabs_event();
				break;

			case 'w':
				printf("%s Enter channel id: ",TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				printf("%s Enter SLIC RAM address (decimal): ",TOOL_PREFIX);
				gets(str);
				cmd = atoi(str);
				cmd_len = 1;
				printf("%s Enter data: ",TOOL_PREFIX);
				for(i = 0; i < cmd_len; i++) {
					gets(str);
					data_buff_ram[i] = atoi(str);
				}
				silabs_slic_ram_write(line0_id, cmd, cmd_len, data_buff_ram);
				break;
#endif
			case 'm':
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
				sw_loopback_multi_phones_test(tdm_fd, line0_id, line1_id);
				break;

			case 'c':
				set_tdm_clk_config();
				break;

			case 'g':
				printf("%s Current PPM correction is (+/-1000): %d", TOOL_PREFIX, get_tdm_clk_correction());
				break;

			case 's':
				printf("%s Enter number of PPM for correction (+/-1000, 0 to disable correction): ", TOOL_PREFIX);
				gets(str);
				set_tdm_clk_correction((int)atoi(str));
				break;

			case 't':
				/* Start Telephony */
				if (ioctl(tdm_fd, TAL_DEV_INIT, &tal_params)) {
					printf("%s Error, unable to init TDM\n", TOOL_PREFIX);
					return 1;
				}

				if(silabs_open_device()) {
					printf("%s Error, could not open vpapi device\n", TOOL_PREFIX);
					return 1;
				}

				if(silabs_init()) {
					printf("%s Error, init failed\n", TOOL_PREFIX);
					ret = 1;
					goto voice_out;
				}
				slic_init = 1;

				wait_for_silabs_event();

				break;

			case 'u':
				/* Stop Telephony */
				release(0);
				break;
			case 'a':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				if (line0_id >= total_lines) {
					printf("Error invalid line number -"\
							"Please Try Again\n");
					continue;
				}
				printf("%s Enter path to the directory "\
						"containing the files to be"\
						"tested:", TOOL_PREFIX);
				gets(str);
				slic_analog_loopback(str,        line0_id);
				break;
			case 'b':
				printf("%s Enter line id: ", TOOL_PREFIX);
				gets(str);
				line0_id = atoi(str);
				record_input(tdm_fd, line0_id);
				printf("press something to hear the recording\n");
				gets(str);
				play_pcm_file_on_phone(tdm_fd, line0_id,
						"record-out.pcm");
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

	return ret;
}

void release(int signum)
{
	int i, status;

	if (signum) {
		printf("\n%s Stopping Phone devices and exit\n", TOOL_PREFIX);
		sleep(1);
	} else {
		printf("\n%s Stopping Phone devices\n", TOOL_PREFIX);
	}

	/* Stop SLIC/s */
	if (slic_init) {
		/* Destroy ProSLIC channel objects. */
		for (i = 0; i < total_lines; i++) {
			status = silabs_channel_init(SI_CHANNEL_DESTROY, i);
			if (status != RC_NONE) {
				printf("## Error, silabs_channel_init (SI_CHANNEL_DESTROY) " \
					"failed (line=%d, status=%d) ##\n", i, status);
				return;
			}
		}

		/* Destroy ProSLIC Device Objects */
		for (i = 0; i < total_devs; i++) {
			status = silabs_device_init(SI_DEVICE_DESTROY, i);
			if (status != RC_NONE) {
				printf("## Error, silabs_device_init (SI_DEVICE_CREATE) "\
					"failed (device=%d, status=%d) ##\n", i, status);
				return;
			}
		}

		/* Destroy ProSLIC Control Interface Object  */
		for (i = 0; i < total_devs; i++) {
			status = silabs_control_interface(SI_IF_DESTROY, i);
			if (status != RC_NONE) {
				printf("## Error, silabs_control_interface (SI_IF_DESTROY) "\
					"failed (line=%d, status=%d) ##\n", i, status);
				return;
			}
		}
		slic_init = 0;
	} else {
		printf("\n%s SLIC already stopped\n", TOOL_PREFIX);
	}

	/* Stop TDM */
	if (ioctl(tdm_fd, TAL_DEV_EXIT, 0)) {
		printf("\n%s Error, unable to stop TDM\n", TOOL_PREFIX);
		return;
	} else {
		printf("\n%s TDM stopped\n", TOOL_PREFIX);
	}

	if (signum)
		close(tdm_fd);

	if(silabs_close_device()) {
		printf("\n%s Error, could not close SLIC device ##\n", TOOL_PREFIX);
		return;
	} else {
		printf("\n%s SLIC device closed\n", TOOL_PREFIX);
	}

	if (signum)
		exit(signum);
}

static void channel_balancing_test(int tdm_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch, cb_loop = 0, i;
	unsigned long int loops = 0, index;
	int status;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Fill Tx buffer with incremental pattern */
	for(ch = 0; ch < total_lines; ch++) {
		for(index = 0; index < (80 * pcm_bytes); index+=2)
			*((unsigned short*)&aud_buf[1][(80 * pcm_bytes * ch) + index]) = (((index+3) << 8)+ (index+1));
	}

	/* Put SLIC/s in loopback mode and On-Hook transmission */
	for(ch = 0; ch < total_lines; ch++) {
		status = silabs_channel_set_loopback(ch, PROSLIC_LOOPBACK_DIG);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (PROSLIC_LOOPBACK_DIG) failed (status=%d) ##\n", status);
			return;
		}

		status = silabs_channel_set_line_feed(ch, LF_FWD_OHT);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed "\
					"(LF_FWD_ACTIVE) failed (line=%d,"\
					"status=%d) ##\n", ch, status);
			return;
		}
	}

	/* Wait a bit */
	sleep(1);

	if (iterations == 0)
		iterations = (unsigned long int)(-1); /* Assume infinite */

	while (loops < iterations) {
		cb_loop = 0;
		i = 0;

		if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
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
			if (FD_ISSET(tdm_fd, &wr_fds)) {
				msg_len = write(tdm_fd, aud_buf[1], buff_size);
				if (msg_len < buff_size) {
					printf("write() failed\n");
					goto cb_out;
				}
			}

			/* Read */
			if (FD_ISSET(tdm_fd, &rd_fds)) {
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
		if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
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

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	/*  Return SLIC/s to no-loopback mode and Off-Hook transmission */
	for(ch = 0; ch < total_lines; ch++) {
		status = silabs_channel_set_loopback(ch, PROSLIC_LOOPBACK_NONE);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (PROSLIC_LOOPBACK_DIG) failed (status=%d) ##\n", status);
			return;
		}

		status = silabs_channel_set_line_feed(ch, LF_FWD_ACTIVE);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed (LF_FWD_ACTIVE) failed (line=%d, status=%d) ##\n", i, status);
			return;
		}
	}
}

static void slic_digital_loopback(int tdm_fd, unsigned long int iterations)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch;
	unsigned long int loops = 0, index;
	int i, status;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Put SLIC/s in loopback mode and On-Hook transmission */
	for(ch = 0; ch < total_lines; ch++) {
		status = silabs_channel_set_loopback(ch, PROSLIC_LOOPBACK_DIG);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (PROSLIC_LOOPBACK_DIG) failed (status=%d) ##\n", status);
			return;
		}

		status = silabs_channel_set_line_feed(ch, LF_FWD_OHT);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed (LF_FWD_ACTIVE) "\
				"failed (line=%d, status=%d) ##\n", ch, status);
			return;
		}
	}

	/* Wait a bit */
	sleep(1);

	/* Fill Tx buffer with incremental pattern */
	for(ch = 0; ch < total_lines; ch++) {
		for(index = 0; index < (80 * pcm_bytes); index++)
			aud_buf[1][index + (80 * pcm_bytes * ch)] = (index+ch+2);
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
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
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len < buff_size) {
				printf("write() failed\n");
				goto slic_dl_out;
			}
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len < buff_size) {
				printf("read() failed\n");
				goto slic_dl_out;
			}

			if (++loops > 3) {
				for(ch = 0; ch < total_lines; ch++) {
					if(slic_dl_data_compare(ch)) {
						printf("\nERROR - data miscompare(ch=%d,loops=%d) !!!\n", ch, loops);
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

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}

	/*  Return SLIC/s to no-loopback mode and Off-Hook transmission */
	for(ch = 0; ch < total_lines; ch++) {
		status = silabs_channel_set_loopback(ch, PROSLIC_LOOPBACK_NONE);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (PROSLIC_LOOPBACK_DIG) failed (status=%d) ##\n", status);
			return;
		}

		status = silabs_channel_set_line_feed(ch, LF_FWD_ACTIVE);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed (LF_FWD_ACTIVE) failed (line=%d, status=%d) ##\n", i, status);
			return;
		}
	}
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
	} else {
		return 0;
	}
}

static void sw_loopback(int tdm_fd, unsigned char line_id)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, status;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until line goes off-hook */
	while(hook_state[line_id] == 0) {
		wait_for_silabs_event();
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
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
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			printf("Rd\n");
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line_id), BUFF_ADDR(aud_buf[0], line_id), CH_BUFF_SIZE);
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			printf("Wr\n");
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_silabs_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static void gen_tone(unsigned short freq, unsigned char line_id, unsigned char* tx_buff)
{
	short i;
	short buf[80];
	short sample;

	for (i = 0; i < 80; i++) {
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
	int msg_len, x, status;
	char str[4];

	if (tdm_fd <= 0) {
		printf("%s Device %s is not accessible\n", TOOL_PREFIX, dev_name);
		return;
	}

	while(1) {
		if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
			printf("Error, unable to stop pcm bus\n");
			return;
		}

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
			wait_for_silabs_event();
		}

		if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
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
			if (FD_ISSET(tdm_fd, &wr_fds)) {
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
			wait_for_silabs_event();

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
		wait_for_silabs_event();
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
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
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
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
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_silabs_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
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

	/* Wait until both lines go off-hook */
	while((hook_state[line0] == 0) || (hook_state[line1] == 0)) {
		wait_for_silabs_event();
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
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
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				return;
			}
			memcpy(BUFF_ADDR(aud_buf[1], line0), BUFF_ADDR(aud_buf[0], line1), CH_BUFF_SIZE);
			memcpy(BUFF_ADDR(aud_buf[1], line1), BUFF_ADDR(aud_buf[0], line0), CH_BUFF_SIZE);

		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_silabs_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
}

static int silabs_init(void)
{
	int i, status;

	/* Set lines status to on-hook */
	memset(hook_state, 0, MAX_LINES);

	/* Fill time slot table */
	memset(time_slot_table, 0, MAX_LINES);
	for (i = 0; i < total_lines; i++)
		time_slot_table[i] = ((i+1) * pcm_bytes);

	/* Create ProSLIC Control Interface Object  */
	for (i = 0; i < total_devs; i++) {
		status = silabs_control_interface(SI_IF_CREATE, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_control_interface (SI_IF_CREATE) failed (line=%d, status=%d) ##\n",
				i, status);
			return -1;
		}
	}

	/* Create ProSLIC Device Objects */
	for (i = 0; i < total_devs; i++) {
		status = silabs_device_init(SI_DEVICE_CREATE, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_device_init (SI_DEVICE_CREATE) failed (device=%d, status=%d) ##\n",
				i, status);
			return -1;
		}
	}

	/* Create and initialize ProSLIC channel objects. Also initialize array pointers to user’s proslic
	channel object members to simplify initialization process. */
	for (i = 0; i < total_lines; i++) {
		status = silabs_channel_init(SI_CHANNEL_CREATE, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_init (SI_CHANNEL_CREATE) failed (line=%d, status=%d) ##\n",
				i, status);
			return -1;
		}
		status = silabs_channel_init(SI_CHANNEL_SW_INIT, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_init (SI_CHANNEL_SW_INIT) failed (line=%d, status=%d) ##\n",
				i, status);
			return -1;
		}
	}

	/* Establish linkage between host objects/functions and ProSLIC API */
	for (i = 0; i < total_devs; i++) {
		status = silabs_control_interface(SI_IF_SET_FUNCS, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_control_interface (SI_IF_SET_FUNCS) failed (status=%d) ##\n", status);
			return -1;
		}
	}

	/* Assert hardware reset to ensure VDD, PCLK, and FSYNC are present and stable before releasing reset */
	for (i = 0; i < total_lines; i++) {
		status = silabs_channel_operation(SI_CHANNEL_OP_RESET, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (SI_CHANNEL_OP_RESET)"\
				"failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
	}

	/* Initialize device (loading of general parameters, calibrations, dc-dc powerup, etc.) */
	status = silabs_channel_all(SI_CHANNEL_ALL_INIT);
	if (status != RC_NONE) {
		printf("## Error, silabs_channel_all (SI_CHANNEL_ALL_INIT) failed (status=%d) ##\n", status);
		return -1;
	}

	/* Execute longitudinal balance calibration or reload coefficients from factory LB cal
	Note: all batteries should be up and stable prior to executing the lb cal */
	status = silabs_channel_all(SI_CHANNEL_ALL_LBCAL);
	if (status != RC_NONE) {
		printf("## Error, silabs_channel_all (SI_CHANNEL_ALL_LBCAL) failed (status=%d) ##\n", status);
		return -1;
	}

	/* Load custom configuration presets (generated using ProSLIC API Config Tool) */
	for (i = 0; i < total_lines; i++) {
		status = silabs_channel_setup(SI_CHANNEL_SETUP_DC_FEED, i, DCFEED_48V_20MA);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_DC_FEED) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
		status = silabs_channel_setup(SI_CHANNEL_SETUP_RING, i, RING_F20_45VRMS_0VDC_BAL);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_RING) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
		status = silabs_channel_setup(SI_CHANNEL_SETUP_ZSYNTH, i, ZSYN_370_620_310_30_0);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_ZSYNTH) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
		status = silabs_channel_setup(SI_CHANNEL_SETUP_TONEGEN, i, TONEGEN_FCC_DIAL);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_TONEGEN) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}

		/* Extract PCM format */
		switch(pcm_bytes) {
		case 1:
			status = silabs_channel_setup(SI_CHANNEL_SETUP_PCM, i, PCM_8ULAW);
			break;
		case 2:
			status = silabs_channel_setup(SI_CHANNEL_SETUP_PCM, i, PCM_16LIN);
			break;
		case 4:
			status = silabs_channel_setup(SI_CHANNEL_SETUP_PCM, i, PCM_16LIN_WB);
			break;
		default:
			status = silabs_channel_setup(SI_CHANNEL_SETUP_PCM, i, PCM_8ULAW);
			printf("## Warning, wrong PCM size - set to default(ULAW) ##\n");
			break;
		}
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_setup (SI_CHANNEL_SETUP_PCM) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
	}

	/* Set Time slot parameters and line feed.*/
	for (i = 0; i < total_lines; i++) {
		/* Configure PCM timeslots */
		//printf("## INFO: line(%d): rx-slot(%d) , tx-slot(%d) ##\n",i ,(time_slot_table[i] * 8) + 1, (time_slot_table[i] * 8) + 1);
		status = silabs_PCM_TS_setup(i, (time_slot_table[i] * 8) + 1, (time_slot_table[i] * 8) + 1);
		if (status != RC_NONE) {
			printf("## Error, silabs_PCM_TS_setup failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}

		status = silabs_channel_set_line_feed(i, LF_FWD_ACTIVE);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed (LF_FWD_ACTIVE) failed (line=%d, status=%d) ##\n", i, status);
			return -1;
		}
	}

	/*  Enable Interrupts. */
	for (i = 0; i < total_lines; i++) {
		status = silabs_channel_operation(SI_CHANNEL_OP_ENA_INT, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (SI_CHANNEL_OP_ENA_INT) failed (status=%d) ##\n", status);
			return -1;
		}
	}

	/* PCM Start. */
	for (i = 0; i < total_lines; i++)  {
		status = silabs_channel_operation(SI_CHANNEL_OP_PCM_START, i);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation (SI_CHANNEL_OP_PCM_START) failed (status=%d) ##\n", status);
			return;
		}
	}
	return 0;
}

static inline void wait_for_silabs_event(void)
{
	bool status;
	SiEventType event;
	uInt8 hookStatus;

	while(silabs_get_event(&event) == true) {
		while(event.eventsNum > 0) {
			event.eventsNum--;
			switch (event.irqs[event.eventsNum]) {
			case IRQ_LOOP_STATUS:
				silabs_channel_read_hook_status(event.chanNum, &hookStatus);
				hook_state[event.chanNum] = hookStatus;
				if (hook_state[event.chanNum]) {
					printf("off-hook(%d)\n", event.chanNum);
					offhook_count++;
				} else {
					printf("on-hook(%d)\n", event.chanNum);
					offhook_count--;
				}
				break;
			default:
				break;
			}
		}
	}
}

static void set_tdm_clk_config(void)
{
	tdm_dev_clk_t tdm_dev_clk;

	/* Config TDM clock */
	if (ioctl(tdm_fd, TDM_DEV_TDM_CLK_CONFIG, &tdm_dev_clk))
		printf("%s Error, unable to config TDM clock.\n", TOOL_PREFIX);
}

static int get_tdm_clk_correction(void)
{
	tdm_dev_clk_t tdm_dev_clk;

	/* Get TDM clock */
	if (ioctl(tdm_fd, TDM_DEV_TDM_CLK_GET, &tdm_dev_clk)) {
		printf("%s Error, unable to get TDM clock.\n", TOOL_PREFIX);
		return 0;
	}

	return tdm_dev_clk.correction;
}

static void set_tdm_clk_correction(int correction)
{
	tdm_dev_clk_t tdm_dev_clk;

	tdm_dev_clk.correction=correction;

	/* Set TDM clock */
	if (ioctl(tdm_fd, TDM_DEV_TDM_CLK_SET, &tdm_dev_clk))
		printf("%s Error, unable to set TDM clock.\n", TOOL_PREFIX);
}

struct Wav_Header {
	char chunk_id[4];
	int chunk_size;
	char format[4];
	char subchunk1_id[4];
	int subchunk1_size;
	short audio_format;
	short num_channels;
	int sample_rate;
	int byte_rate;
	short block_align;
	short bits_per_sample;
	char subchunk2_id[4];
	int subchunk2_size;
};


/*
 *@func: prepare_al: preparations for the analog loopback
 *@param: input_file_name: the name of the file to be looped back, should be
 *	  ending with .pcm or .wav, the function fails otherwise.
 *@param: output_name: the address to which we'll write the name of the
 *	  output file to
 *@param: input:  the address of the variable that will store the input file
 *	  object
 *@param: output: the address of the variable that will store the output file
 *	  object
 *@param: size: the address of the variable that will store the size of the
 *	 input file
 *@param: dir: the name of the directory containing the files to be tested.
 *
 * @return value: 0 in success -1 otherwise.
 */
static int prepare_al(char *input_file_name, char *output_name, FILE **input, FILE
		**output , int *size, char *dir)
{
	FILE *input_file, *output_file;
	char flag[5], in_buf[100] , out_buf[100], *ptr;

	snprintf(out_buf, 100, "%s/out/%s", dir, input_file_name);
	snprintf(in_buf, 100, "%s/%s", dir, input_file_name);

	strcpy(input_file_name, in_buf);
	strcpy(output_name, out_buf);

	input_file = fopen(input_file_name, "r");
	if (input_file == NULL) {
		printf("error while opening\n");
		return -1;
	}

	ptr = strrchr(input_file_name, '.');
	if (ptr == NULL) {
		printf("Error, %s is invalid file name-please enter a *.wav/*.pcm file\n", input_file_name);
		return -1;
	}
	if (strcmp(".wav", ptr) == 0) {
		*size = read_wav_header(input_file, output_name);
		if (*size == -1)
			return -1;
		strcpy(flag, "a");
	} else if (strcmp(".pcm", ptr) == 0) {
		strcpy(flag, "w");
		fseek(input_file, 0L, SEEK_END);
		*size = ftell(input_file);
		fseek(input_file, 0L, SEEK_SET);
	} else {
		printf("Error unknown fromat\n");
		return -1;
	}

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return -1;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return -1;
	}

	output_file = fopen(output_name, flag);
	if (output_file == NULL) {
		printf(" error cannot open output file : %s\n ", output_name);
		fclose(input_file);
		return -1;
	}
	*input = input_file;
	*output = output_file;
	return 0;
}

static void finish_al_read(fd_set rd_fds, int *cmp_status_ptr, unsigned char
		line_id, FILE *output_file)
{
	int i, msg_len;
	struct timeval timeout = {0, TIMEOUT};

	for (i = 0 ; i < 3 ; ++i) {
		FD_ZERO(&rd_fds);
		FD_SET(tdm_fd, &rd_fds);

		if (select(tdm_fd+1, &rd_fds, NULL, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			*cmp_status_ptr = 1;
			return;
		}
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len < buff_size) {
				printf("read() failed\n");
				*cmp_status_ptr = 1;
				return;
			}
			fwrite(BUFF_ADDR(&aud_buf[0][0], line_id), 1, CH_BUFF_SIZE, output_file);
		} else {
			printf("why are you here ?\n");
		}
		timeout.tv_usec = TIMEOUT;
	}
}

static int analog_loopback_test(unsigned char line_id, char *file_name, char *dir)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, cmp_status = 0, ch;
	int i = 0, j = 0, size, res;
	char output_name[36] ;
	FILE *input_file, *output_file;

	res = prepare_al(file_name, output_name, &input_file, &output_file, &size, dir);
	if (res != 0)
		return -1;

	/* Put SLIC/s in loopback mode and On-Hook transmission */
	if (enable_disable_loopback(PROSLIC_LOOPBACK_ANA, LF_FWD_OHT) != 0) {
		cmp_status = 1;
		goto slic_al_out;
	}

	while (feof(input_file) == 0) {
		FD_ZERO(&rd_fds);
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &rd_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			cmp_status = 1;
			goto slic_al_out;
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			/*fill the Tx with data from file */
			size -= read_from_wav(input_file, line_id, size);
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len < buff_size) {
				printf("write() failed\n");
				cmp_status = 1;
				goto slic_al_out;
			}
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			memset(aud_buf[0], 0, buff_size);
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len < buff_size) {
				printf("read() failed\n");
				cmp_status = 1;
				goto slic_al_out;
			}
			if (i > 4)
				fwrite(BUFF_ADDR(&aud_buf[0][0], line_id), 1, CH_BUFF_SIZE, output_file);
		}
		if (size == 0) {
			finish_al_read(rd_fds, &cmp_status, line_id, output_file);
			goto slic_al_out;
		}
		i++;
		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}


	sleep(1);

slic_al_out:
	return analog_loopback_out(cmp_status, input_file,
			output_file, file_name, output_name);
}

static int analog_loopback_out(int cmp_status, FILE *or, FILE *dg, char
		*or_name, char *dg_name)
{
	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0))
		printf("Error, unable to stop pcm bus\n");

	enable_disable_loopback(PROSLIC_LOOPBACK_NONE, LF_FWD_ACTIVE);
	fclose(or);
	fclose(dg);
	if (cmp_status == 0) {
		return analog_loopback_pesq_check(or_name, dg_name);
	} else {
		printf(" Test Failed - Error while analog loopback\n ");
		return -1;
	}
	return 0;
}

static int analog_loopback_pesq_check(char *or_name, char *dg_name)
{
	const char *args_names[5];
	int result;

	args_names[0] = "./PESQ";
	args_names[1] = "+8000";
	args_names[2] = or_name;
	args_names[3] = dg_name;
	args_names[4] = "";
	result = pesq_main(4, args_names);
	if (result == 0) {
		printf("\nAnalog loopback test - PASS !!!\n");
	} else {
		printf("Test Failed  - Bad PESQ result\n");
		return -1;
	}
	return 0;
}
static void play_pcm_file_on_phone(int tdm_fd, unsigned char line_id, char*
		file_name)
{
	fd_set wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, x, status, ch, i, res;
	FILE *wav_file;
	struct Wav_Header h;

	wav_file = fopen(file_name, "r");
	if (wav_file == NULL) {
		printf("error while opening\n");
		return;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	/* Wait a bit */
	sleep(1);

	/* Wait until both lines go off-hook */
	while (hook_state[line_id] == 0)
		wait_for_silabs_event();

	printf("%s Waiting for on-hook to return to menu.\n", TOOL_PREFIX);

	while (hook_state[line_id] == 1 && feof(wav_file) == 0) {
		FD_ZERO(&wr_fds);
		FD_SET(tdm_fd, &wr_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, NULL, &wr_fds, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			return;
		}

		/* Write */
		if (FD_ISSET(tdm_fd, &wr_fds)) {
			read_from_wav(wav_file, line_id, pcm_bytes*80+1);
			msg_len = write(tdm_fd, aud_buf[1], buff_size);
			if (msg_len <= 0) {
				printf("write() failed\n");
				return;
			}
		}

		/* Check hook state */
		wait_for_silabs_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	fclose(wav_file);
	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
	sleep(1);
}

static int enable_disable_loopback(ProslicLoopbackModes loopMode, uInt8 newLineFeed)
{
	int status, ch;

	for (ch = 0; ch < total_lines; ch++) {
		status = silabs_channel_set_loopback(ch, loopMode);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_operation "\
					"(PROSLIC_LOOPBACK_DIG) failed"\
					"(status=%d) ##\n", status);
			return -1;
		}

		status = silabs_channel_set_line_feed(ch, newLineFeed);
		if (status != RC_NONE) {
			printf("## Error, silabs_channel_set_line_feed "\
					"(LF_FWD_ACTIVE) failed (line=%d, "\
					"status=%d) ##\n", ch, status);
			return -1;
		}
	}
	return 0;
}

static int read_from_wav(FILE *filename, int line_id, long leftsize)
{
	int res;
	if (filename == NULL) {
		printf("Error, reading from a NULL wav pointer\n");
		return -1;
	}
	if (leftsize > 80*pcm_bytes) {
		res = fread(BUFF_ADDR(&aud_buf[1][0], line_id), pcm_bytes, 80, filename);
	} else {

		res = fread(BUFF_ADDR(&aud_buf[1][0], line_id), 1, leftsize, filename);
		return res;
	}
	if (res < 0)
		printf("Error reading wav, res = %d\n", res);

	return res*pcm_bytes;
}



/* reads the header and returns the size of the data
 * @filename: an open wav file
 */
static int read_wav_header(FILE *filename, char *output_name)
{
	struct Wav_Header h;
	FILE *wav_file;
	int res = fread(&h, 1 , 44, filename);
	if (res < 44) {
		printf(" Error while reading wav header\n");
		return -1;
	}
#if defined(DEBUG)
	printf(" Chunk-id = %c%c%c%c\n", h.chunk_id[0], h.chunk_id[1], h.chunk_id[2], h.chunk_id[3]);
	printf(" Chunk size = %d\n", h.chunk_size);
	printf(" format = %c%c%c%c\n", h.format[0], h.format[1], h.format[2],
			h.format[3]);
	printf(" subchunk1-id = %c%c%c%c\n", h.subchunk1_id[0], h.subchunk1_id[1], h.subchunk1_id[2],
			h.subchunk1_id[3]);
	printf(" sub-Chunk size = %d\n", h.subchunk1_size);
	printf(" Audio format = %hd\n", h.audio_format);
	printf(" number of channels = %hd\n", h.num_channels);
	printf(" sample rate = %d\n", h.sample_rate);
	printf(" byte rate = %d\n", h.byte_rate);
	printf(" block align = %hd\n", h.block_align);
	printf(" bits per sample = %hd\n", h.bits_per_sample);
	printf(" subchunk2-id = %c%c%c%c\n", h.subchunk2_id[0], h.subchunk2_id[1], h.subchunk2_id[2],
			h.subchunk2_id[3]);
	printf(" subchunk2 size = %d\n", h.subchunk2_size);
#endif
	wav_file = fopen(output_name, "w");
	if (wav_file == NULL) {
		printf(" error cannot open file : %s\n ", filename);
		return -1;
	}
	fwrite(&h, 1, 44, wav_file);
	fclose(wav_file);
	return h.subchunk2_size - 8;
}

static void record_input(int tdm_fd, unsigned char line)
{
	fd_set rd_fds, wr_fds;
	struct timeval timeout = {0, TIMEOUT};
	int msg_len, size, status, i = 0;
	char str[32];
	FILE *wav_file = NULL;

	if (tdm_fd <= 0) {
		printf("Device %s is not accessible\n", dev_name);
		return;
	}

	/* Wait until both lines go off-hook */
	while (hook_state[line] == 0)
		wait_for_silabs_event();

	printf("start recording\n");
	if (ioctl(tdm_fd, TAL_DEV_PCM_START, 0)) {
		printf("Error, unable to start pcm bus\n");
		return;
	}

	/* open the input file*/
	wav_file = fopen("record.pcm", "w");
	if (wav_file == NULL) {
		printf("error while opening\n");
		return;
	}

	memset(aud_buf[0], 0, buff_size);
	while (hook_state[line] == 1) {
		FD_ZERO(&rd_fds);
		FD_SET(tdm_fd, &rd_fds);

		/* Wait for event  */
		if (select(tdm_fd+1, &rd_fds, NULL, NULL, &timeout) == 0) {
			printf("Error, timeout while polling(%dusec)\n", TIMEOUT);
			fclose(wav_file);
			return;
		}

		/* Read */
		if (FD_ISSET(tdm_fd, &rd_fds)) {
			msg_len = read(tdm_fd, aud_buf[0], buff_size);
			if (msg_len <= 0) {
				printf("read() failed\n");
				fclose(wav_file);
				return;
			}
			if (i > 3)
				fwrite(BUFF_ADDR(aud_buf[0], line), 1, CH_BUFF_SIZE, wav_file);
			++i;
		}

		/* Check hook state */
		wait_for_silabs_event();

		/* Reload timeout */
		timeout.tv_usec = TIMEOUT;
	}

	if (ioctl(tdm_fd, TAL_DEV_PCM_STOP, 0)) {
		printf("Error, unable to stop pcm bus\n");
		return;
	}
	fclose(wav_file);
}

/*
 *@func: slic_analog_loopback: the main function for the analog loopback test
 *@param: dir_name: the path to the directory which contains the files to be
 *	  sent to an analog loopback.
 *@param: line: the line id to perform an analog loopback on.
 *
 */
static void slic_analog_loopback(char *dir_name, unsigned char line)
{
	DIR *dir;
	struct dirent *ent;
	char f_name[100], out_dir[100], new_dir[100];
	int x = 0, res = 0;
	struct stat file_inf;
	FILE *input = NULL, *output = NULL;

	dir = opendir(dir_name);
	if (dir != NULL) {
		/* print all the files and directories within directory */
		snprintf(new_dir, 100, "%s/out", dir_name);
		mkdir(new_dir, 0777);
		while ((ent = readdir(dir)) != NULL) {
				snprintf(f_name, 100, "%s/%s", dir_name, ent->d_name);
				if (stat(f_name, &file_inf) == -1) {
					printf("Error with stat\n");
					return;
				}
				if ((file_inf.st_mode & S_IFMT) == S_IFREG) {
					printf("Testing file %s/%s\n", dir_name, ent->d_name);
					res = analog_loopback_test(line, ent->d_name,
							dir_name);
					if (res != 0)
						return;
				wait(1);
				}
		}
	closedir(dir);
	} else {
		/* could not open directory */
		printf("Error - couldn't open directory :\n");
		return;
	}
}
