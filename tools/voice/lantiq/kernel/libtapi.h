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
#ifndef _LIBTAPI_H
#define _LIBTAPI_H

struct tapi_handle;
typedef struct tapi_handle * tapi_handle_t;

typedef enum {
	TAPI_LINE_TYPE_FXS_NB,
	TAPI_LINE_TYPE_FXS_WB,
	TAPI_LINE_TYPE_FXS_AUTO,
	TAPI_LINE_TYPE_FXO_NB,
} tapi_line_type_t;

typedef enum {
	TAPI_LINE_FEED_ACTIVE,
	TAPI_LINE_FEED_ACTIVE_REV,
	TAPI_LINE_FEED_STANDBY,
	TAPI_LINE_FEED_HIGH_IMPEDANCE,
	TAPI_LINE_FEED_DISABLED,
} tapi_line_feed_t;

typedef enum {
	TAPI_LINE_ONHOOK,
	TAPI_LINE_OFFHOOK,
} tapi_line_hook_t;

typedef enum {
	TAPI_EVENT_NONE,
	TAPI_EVENT_IO_GENERAL_NONE,
	TAPI_EVENT_IO_INTERRUPT_NONE,
	TAPI_EVENT_FXS_NONE,
	TAPI_EVENT_FXS_RING,
	TAPI_EVENT_FXS_RINGBURST_END,
	TAPI_EVENT_FXS_RINGING_END,
	TAPI_EVENT_FXS_ONHOOK,
	TAPI_EVENT_FXS_OFFHOOK,
	TAPI_EVENT_FXS_FLASH,
	TAPI_EVENT_FXS_ONHOOK_INT,
	TAPI_EVENT_FXS_OFFHOOK_INT,
	TAPI_EVENT_CONTMEASUREMENT,
	TAPI_EVENT_FXS_RAW_ONHOOK,
	TAPI_EVENT_FXS_RAW_OFFHOOK,
	TAPI_EVENT_FXS_LINE_MODE,
	TAPI_EVENT_FXS_COMTEL_END,
	TAPI_EVENT_FXO_NONE,
	TAPI_EVENT_FXO_BAT_FEEDED,
	TAPI_EVENT_FXO_BAT_DROPPED,
	TAPI_EVENT_FXO_POLARITY,
	TAPI_EVENT_FXO_RING_START,
	TAPI_EVENT_FXO_RING_STOP,
	TAPI_EVENT_FXO_OSI,
	TAPI_EVENT_FXO_APOH,
	TAPI_EVENT_FXO_NOPOH,
	TAPI_EVENT_LT_GR909_RDY,
	TAPI_EVENT_NLT_END,
	TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY,
	TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT,
	TAPI_EVENT_LINE_MEASURE_CAPACITANCE_START_INT,
	TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY,
	TAPI_EVENT_PULSE_NONE,
	TAPI_EVENT_PULSE_DIGIT,
	TAPI_EVENT_PULSE_START,
	TAPI_EVENT_DTMF_NONE,
	TAPI_EVENT_DTMF_DIGIT,
	TAPI_EVENT_DTMF_END,
	TAPI_EVENT_CALIBRATION_NONE,
	TAPI_EVENT_CALIBRATION_END,
	TAPI_EVENT_CALIBRATION_END_INT,
	TAPI_EVENT_CALIBRATION_END_SINT,
	TAPI_EVENT_METERING_NONE,
	TAPI_EVENT_METERING_END,
	TAPI_EVENT_CID_TX_NONE,
	TAPI_EVENT_CID_TX_SEQ_START,
	TAPI_EVENT_CID_TX_SEQ_END,
	TAPI_EVENT_CID_TX_INFO_START,
	TAPI_EVENT_CID_TX_INFO_END,
	TAPI_EVENT_CID_TX_NOACK_ERR,
	TAPI_EVENT_CID_TX_RINGCAD_ERR,
	TAPI_EVENT_CID_TX_UNDERRUN_ERR,
	TAPI_EVENT_CID_TX_NOACK2_ERR,
	TAPI_EVENT_CIDSM_END,
	TAPI_EVENT_CID_TX_END,
	TAPI_EVENT_CID_RX_NONE,
	TAPI_EVENT_CID_RX_CAS,
	TAPI_EVENT_CID_RX_END,
	TAPI_EVENT_CID_RX_CD,
	TAPI_EVENT_CID_RX_ERROR_READ,
	TAPI_EVENT_CID_RX_ERROR1,
	TAPI_EVENT_CID_RX_ERROR2,
	TAPI_EVENT_TONE_GEN_NONE,
	TAPI_EVENT_TONE_GEN_BUSY,
	TAPI_EVENT_TONE_GEN_END,
	TAPI_EVENT_TONE_GEN_END_RAW,
	TAPI_EVENT_TONE_DET_NONE,
	TAPI_EVENT_TONE_DET_RECEIVE,
	TAPI_EVENT_TONE_DET_TRANSMIT,
	TAPI_EVENT_TONE_DET_CPT,
	TAPI_EVENT_TONE_DET_CPT_END,
	TAPI_EVENT_TONE_DET_MF_R2_START,
	TAPI_EVENT_TONE_DET_MF_R2_END,
	TAPI_EVENT_FAXMODEM_NONE,
	TAPI_EVENT_FAXMODEM_DIS,
	TAPI_EVENT_FAXMODEM_CED,
	TAPI_EVENT_FAXMODEM_PR,
	TAPI_EVENT_FAXMODEM_AM,
	TAPI_EVENT_FAXMODEM_CNGFAX,
	TAPI_EVENT_FAXMODEM_CNGMOD,
	TAPI_EVENT_FAXMODEM_V21L,
	TAPI_EVENT_FAXMODEM_V18A,
	TAPI_EVENT_FAXMODEM_V27,
	TAPI_EVENT_FAXMODEM_BELL,
	TAPI_EVENT_FAXMODEM_V22,
	TAPI_EVENT_FAXMODEM_V22ORBELL,
	TAPI_EVENT_FAXMODEM_V32AC,
	TAPI_EVENT_FAXMODEM_V8BIS,
	TAPI_EVENT_FAXMODEM_HOLDEND,
	TAPI_EVENT_FAXMODEM_CEDEND,
	TAPI_EVENT_FAXMODEM_CAS_BELL,
	TAPI_EVENT_FAXMODEM_V21H,
	TAPI_EVENT_FAXMODEM_VMD,
	TAPI_EVENT_LIN_NONE,
	TAPI_EVENT_LIN_UNDERFLOW,
	TAPI_EVENT_COD_NONE,
	TAPI_EVENT_COD_DEC_CHG,
	TAPI_EVENT_COD_ROOM_NOISE,
	TAPI_EVENT_COD_ROOM_SILENCE,
	TAPI_EVENT_COD_ANNOUNCE_END,
	TAPI_EVENT_COD_MOS,
	TAPI_EVENT_RTP_NONE,
	TAPI_EVENT_RTP_FIRST,
	TAPI_EVENT_RTP_EXT_BROKEN,
	TAPI_EVENT_RTP_EXT_SSRC_CHANGED,
	TAPI_EVENT_AAL_NONE,
	TAPI_EVENT_RFC2833_NONE,
	TAPI_EVENT_RFC2833_EVENT,
	TAPI_EVENT_KPI_NONE,
	TAPI_EVENT_KPI_INGRESS_FIFO_FULL,
	TAPI_EVENT_KPI_SOCKET_FAILURE,
	TAPI_EVENT_T38_NONE,
	TAPI_EVENT_T38_ERROR_GEN,
	TAPI_EVENT_T38_ERROR_OVLD,
	TAPI_EVENT_T38_ERROR_READ,
	TAPI_EVENT_T38_ERROR_WRITE,
	TAPI_EVENT_T38_ERROR_DATA,
	TAPI_EVENT_T38_ERROR_SETUP,
	TAPI_EVENT_T38_FDP_REQ,
	TAPI_EVENT_T38_STATE_CHANGE,
	TAPI_EVENT_JB_NONE,
	TAPI_EVENT_DOWNLOAD_NONE,
	TAPI_EVENT_INFO_NONE,
	TAPI_EVENT_INFO_MBX_CONGESTION,
	TAPI_EVENT_DEBUG_NONE,
	TAPI_EVENT_DEBUG_CERR,
	TAPI_EVENT_GPIO_HL,
	TAPI_EVENT_GPIO_LH,
	TAPI_EVENT_LL_DRIVER_NONE,
	TAPI_EVENT_LL_DRIVER_WD_FAIL,
	TAPI_EVENT_FAULT_GENERAL_NONE,
	TAPI_EVENT_FAULT_GENERAL,
	TAPI_EVENT_FAULT_GENERAL_CHINFO,
	TAPI_EVENT_FAULT_GENERAL_DEVINFO,
	TAPI_EVENT_TYPE_FAULT_GENERAL,
	TAPI_EVENT_FAULT_LINE_NONE,
	TAPI_EVENT_FAULT_LINE_GK_POS,
	TAPI_EVENT_FAULT_LINE_GK_NEG,
	TAPI_EVENT_FAULT_LINE_GK_LOW,
	TAPI_EVENT_FAULT_LINE_GK_HIGH,
	TAPI_EVENT_FAULT_LINE_OVERTEMP,
	TAPI_EVENT_FAULT_LINE_OVERCURRENT,
	TAPI_EVENT_FAULT_LINE_GK_LOW_INT,
	TAPI_EVENT_FAULT_LINE_GK_HIGH_INT,
	TAPI_EVENT_FAULT_LINE_GK_LOW_END,
	TAPI_EVENT_FAULT_LINE_GK_HIGH_END,
	TAPI_EVENT_FAULT_LINE_OVERTEMP_END,
	TAPI_EVENT_FAULT_HW_NONE,
	TAPI_EVENT_FAULT_HW_SPI_ACCESS,
	TAPI_EVENT_FAULT_HW_CLOCK_FAIL,
	TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END,
	TAPI_EVENT_FAULT_HW_FAULT,
	TAPI_EVENT_FAULT_HW_SYNC,
	TAPI_EVENT_FAULT_HW_RESET,
	TAPI_EVENT_FAULT_HW_SSI_ERR,
	TAPI_EVENT_FAULT_HW_SSI_ERR_END,
	TAPI_EVENT_FAULT_FW_NONE,
	TAPI_EVENT_FAULT_FW_EBO_UF,
	TAPI_EVENT_FAULT_FW_EBO_OF,
	TAPI_EVENT_FAULT_FW_CBO_UF,
	TAPI_EVENT_FAULT_FW_CBO_OF,
	TAPI_EVENT_FAULT_FW_CBI_OF,
	TAPI_EVENT_FAULT_FW_WATCHDOG,
	TAPI_EVENT_FAULT_SW_NONE,
	TAPI_EVENT_FAULT_HDLC_NONE,
	TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH,
	TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH,
	TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW,
	TAPI_EVENT_FAULT_HDLC_DISABLED,
} tapi_event_id_t;

typedef struct {
	unsigned int	device;
	unsigned int	channel;
	tapi_event_id_t	event;
} tapi_event_t;

typedef enum {
	TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ,
	TAPI_PCM_IF_MODE_SLAVE,
	TAPI_PCM_IF_MODE_MASTER,
} tapi_pcm_if_mode_t;

typedef enum {
	TAPI_PCM_IF_FREQ_512KHZ,
	TAPI_PCM_IF_FREQ_1024KHZ,
	TAPI_PCM_IF_FREQ_1536KHZ,
	TAPI_PCM_IF_FREQ_2048KHZ,
	TAPI_PCM_IF_FREQ_4096KHZ,
	TAPI_PCM_IF_FREQ_8192KHZ,
	TAPI_PCM_IF_FREQ_16384KHZ,
} tapi_pcm_if_freq_t;

typedef enum {
	TAPI_PCM_IF_SLOPE_RISE,
	TAPI_PCM_IF_SLOPE_FALL,
} tapi_pcm_if_slope_t;

typedef enum {
	TAPI_PCM_CODING_NB_ALAW_8BIT,
	TAPI_PCM_CODING_NB_ULAW_8BIT,
	TAPI_PCM_CODING_NB_LINEAR_16BIT,
	TAPI_PCM_CODING_WB_ALAW_8BIT,
	TAPI_PCM_CODING_WB_ULAW_8BIT,
	TAPI_PCM_CODING_WB_LINEAR_16BIT,
	TAPI_PCM_CODING_WB_G722,
	TAPI_PCM_CODING_NB_G726_16,
	TAPI_PCM_CODING_NB_G726_24,
	TAPI_PCM_CODING_NB_G726_32,
	TAPI_PCM_CODING_NB_G726_40,
	TAPI_PCM_CODING_WB_LINEAR_SPLIT_16BIT,
} tapi_pcm_coding_t;

extern tapi_handle_t tapi_open(const char *tapidev);
extern void tapi_close(tapi_handle_t th);
extern int tapi_basic_init(tapi_handle_t th, unsigned int device,
							      unsigned int irq);
extern int tapi_fw_download(tapi_handle_t th, unsigned int device,
							    const char *fwfile);
extern int tapi_dev_start(tapi_handle_t th, unsigned int device);
extern int tapi_dev_stop(tapi_handle_t th, unsigned int device);
extern int tapi_bbd_download(tapi_handle_t th, unsigned int device,
				     unsigned int channel, const char *bbdfile);
extern int tapi_line_calibrate(tapi_handle_t th, unsigned int device,
							  unsigned int channel);
extern int tapi_line_type_set(tapi_handle_t th, unsigned int device,
			       unsigned int channel, tapi_line_type_t linetype);
extern int tapi_line_feed_set(tapi_handle_t th, unsigned int device,
			      unsigned int channel, tapi_line_feed_t linefeed);
extern int tapi_line_feed_get(tapi_handle_t th, unsigned int device,
			      unsigned int channel, tapi_line_feed_t *linefeed);
extern int tapi_ring_start(tapi_handle_t th, unsigned int device,
							  unsigned int channel);
extern int tapi_ring_stop(tapi_handle_t th, unsigned int device,
							  unsigned int channel);
extern tapi_line_hook_t tapi_hook_status_get(tapi_handle_t th,
				     unsigned int device, unsigned int channel);
extern int tapi_event_get(tapi_handle_t th, unsigned int device, int timeout,
							   tapi_event_t *event);
extern int tapi_event_enable(tapi_handle_t th, unsigned int device,
				   unsigned int channel, tapi_event_id_t event);
extern int tapi_pcm_if_config(tapi_handle_t th, unsigned int device,
		tapi_pcm_if_mode_t mode, tapi_pcm_if_freq_t freq,
		tapi_pcm_if_slope_t rxslope, tapi_pcm_if_slope_t txslope);
extern int tapi_pcm_channel_config(tapi_handle_t th, unsigned int device,
		unsigned int channel, unsigned int slotrx, unsigned int slottx,
		tapi_pcm_coding_t coding);
extern int tapi_pcm_channel_activate(tapi_handle_t th, unsigned int device,
							  unsigned int channel);
extern int tapi_test_loop(tapi_handle_t th, unsigned int device,
				     unsigned int channel, unsigned int enable);

#endif /* _LIBTAPI_H */
