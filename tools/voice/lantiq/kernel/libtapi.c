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
#include <malloc.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ifx_types.h>
#include <drv_dxt_io.h>
#include <drv_tapi_io.h>

#include "libtapi.h"

#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

extern unsigned char tapi_firmware_blob[];
extern unsigned int tapi_firmware_blob_size;

extern unsigned char tapi_bbd_blob[];
extern unsigned int tapi_bbd_blob_size;

struct tapi_handle {
	int	fd;
};

static const IFX_TAPI_EVENT_ID_t tapi_event_tanslation_table[] = {
	[TAPI_EVENT_NONE]		= IFX_TAPI_EVENT_NONE,
	[TAPI_EVENT_IO_GENERAL_NONE]	= IFX_TAPI_EVENT_IO_GENERAL_NONE,
	[TAPI_EVENT_IO_INTERRUPT_NONE]	= IFX_TAPI_EVENT_IO_INTERRUPT_NONE,
	[TAPI_EVENT_FXS_NONE]		= IFX_TAPI_EVENT_FXS_NONE,
	[TAPI_EVENT_FXS_RING]		= IFX_TAPI_EVENT_FXS_RING,
	[TAPI_EVENT_FXS_RINGBURST_END]	= IFX_TAPI_EVENT_FXS_RINGBURST_END,
	[TAPI_EVENT_FXS_RINGING_END]	= IFX_TAPI_EVENT_FXS_RINGING_END,
	[TAPI_EVENT_FXS_ONHOOK]		= IFX_TAPI_EVENT_FXS_ONHOOK,
	[TAPI_EVENT_FXS_OFFHOOK]	= IFX_TAPI_EVENT_FXS_OFFHOOK,
	[TAPI_EVENT_FXS_FLASH]		= IFX_TAPI_EVENT_FXS_FLASH,
	[TAPI_EVENT_FXS_ONHOOK_INT]	= IFX_TAPI_EVENT_FXS_ONHOOK_INT,
	[TAPI_EVENT_FXS_OFFHOOK_INT]	= IFX_TAPI_EVENT_FXS_OFFHOOK_INT,
	[TAPI_EVENT_CONTMEASUREMENT]	= IFX_TAPI_EVENT_CONTMEASUREMENT,
	[TAPI_EVENT_FXS_RAW_ONHOOK]	= IFX_TAPI_EVENT_FXS_RAW_ONHOOK,
	[TAPI_EVENT_FXS_RAW_OFFHOOK]	= IFX_TAPI_EVENT_FXS_RAW_OFFHOOK,
	[TAPI_EVENT_FXS_LINE_MODE]	= IFX_TAPI_EVENT_FXS_LINE_MODE,
	[TAPI_EVENT_FXS_COMTEL_END]	= IFX_TAPI_EVENT_FXS_COMTEL_END,
	[TAPI_EVENT_FXO_NONE]		= IFX_TAPI_EVENT_FXO_NONE,
	[TAPI_EVENT_FXO_BAT_FEEDED]	= IFX_TAPI_EVENT_FXO_BAT_FEEDED,
	[TAPI_EVENT_FXO_BAT_DROPPED]	= IFX_TAPI_EVENT_FXO_BAT_DROPPED,
	[TAPI_EVENT_FXO_POLARITY]	= IFX_TAPI_EVENT_FXO_POLARITY,
	[TAPI_EVENT_FXO_RING_START]	= IFX_TAPI_EVENT_FXO_RING_START,
	[TAPI_EVENT_FXO_RING_STOP]	= IFX_TAPI_EVENT_FXO_RING_STOP,
	[TAPI_EVENT_FXO_OSI]		= IFX_TAPI_EVENT_FXO_OSI,
	[TAPI_EVENT_FXO_APOH]		= IFX_TAPI_EVENT_FXO_APOH,
	[TAPI_EVENT_FXO_NOPOH]		= IFX_TAPI_EVENT_FXO_NOPOH,
	[TAPI_EVENT_LT_GR909_RDY]	= IFX_TAPI_EVENT_LT_GR909_RDY,
	[TAPI_EVENT_NLT_END]		= IFX_TAPI_EVENT_NLT_END,
	[TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY] =
			IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY,
	[TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT] =
			IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT,
	[TAPI_EVENT_LINE_MEASURE_CAPACITANCE_START_INT] =
			IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_START_INT,
	[TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY] =
			IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY,
	[TAPI_EVENT_PULSE_NONE]	= IFX_TAPI_EVENT_PULSE_NONE,
	[TAPI_EVENT_PULSE_DIGIT]	= IFX_TAPI_EVENT_PULSE_DIGIT,
	[TAPI_EVENT_PULSE_START]	= IFX_TAPI_EVENT_PULSE_START,
	[TAPI_EVENT_DTMF_NONE]		= IFX_TAPI_EVENT_DTMF_NONE,
	[TAPI_EVENT_DTMF_DIGIT]		= IFX_TAPI_EVENT_DTMF_DIGIT,
	[TAPI_EVENT_DTMF_END]		= IFX_TAPI_EVENT_DTMF_END,
	[TAPI_EVENT_CALIBRATION_NONE]	= IFX_TAPI_EVENT_CALIBRATION_NONE,
	[TAPI_EVENT_CALIBRATION_END]	= IFX_TAPI_EVENT_CALIBRATION_END,
	[TAPI_EVENT_CALIBRATION_END_INT] =
			IFX_TAPI_EVENT_CALIBRATION_END_INT,
	[TAPI_EVENT_CALIBRATION_END_SINT] =
			IFX_TAPI_EVENT_CALIBRATION_END_SINT,
	[TAPI_EVENT_METERING_NONE]	= IFX_TAPI_EVENT_METERING_NONE,
	[TAPI_EVENT_METERING_END]	= IFX_TAPI_EVENT_METERING_END,
	[TAPI_EVENT_CID_TX_NONE]	= IFX_TAPI_EVENT_CID_TX_NONE,
	[TAPI_EVENT_CID_TX_SEQ_START]	= IFX_TAPI_EVENT_CID_TX_SEQ_START,
	[TAPI_EVENT_CID_TX_SEQ_END]	= IFX_TAPI_EVENT_CID_TX_SEQ_END,
	[TAPI_EVENT_CID_TX_INFO_START]	= IFX_TAPI_EVENT_CID_TX_INFO_START,
	[TAPI_EVENT_CID_TX_INFO_END]	= IFX_TAPI_EVENT_CID_TX_INFO_END,
	[TAPI_EVENT_CID_TX_NOACK_ERR]	= IFX_TAPI_EVENT_CID_TX_NOACK_ERR,
	[TAPI_EVENT_CID_TX_RINGCAD_ERR]	= IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR,
	[TAPI_EVENT_CID_TX_UNDERRUN_ERR] =
			IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR,
	[TAPI_EVENT_CID_TX_NOACK2_ERR]	= IFX_TAPI_EVENT_CID_TX_NOACK2_ERR,
	[TAPI_EVENT_CIDSM_END]		= IFX_TAPI_EVENT_CIDSM_END,
	[TAPI_EVENT_CID_TX_END]		= IFX_TAPI_EVENT_CID_TX_END,
	[TAPI_EVENT_CID_RX_NONE]	= IFX_TAPI_EVENT_CID_RX_NONE,
	[TAPI_EVENT_CID_RX_CAS]		= IFX_TAPI_EVENT_CID_RX_CAS,
	[TAPI_EVENT_CID_RX_END]		= IFX_TAPI_EVENT_CID_RX_END,
	[TAPI_EVENT_CID_RX_CD]		= IFX_TAPI_EVENT_CID_RX_CD,
	[TAPI_EVENT_CID_RX_ERROR_READ]	= IFX_TAPI_EVENT_CID_RX_ERROR_READ,
	[TAPI_EVENT_CID_RX_ERROR1]	= IFX_TAPI_EVENT_CID_RX_ERROR1,
	[TAPI_EVENT_CID_RX_ERROR2]	= IFX_TAPI_EVENT_CID_RX_ERROR2,
	[TAPI_EVENT_TONE_GEN_NONE]	= IFX_TAPI_EVENT_TONE_GEN_NONE,
	[TAPI_EVENT_TONE_GEN_BUSY]	= IFX_TAPI_EVENT_TONE_GEN_BUSY,
	[TAPI_EVENT_TONE_GEN_END]	= IFX_TAPI_EVENT_TONE_GEN_END,
	[TAPI_EVENT_TONE_GEN_END_RAW]	= IFX_TAPI_EVENT_TONE_GEN_END_RAW,
	[TAPI_EVENT_TONE_DET_NONE]	= IFX_TAPI_EVENT_TONE_DET_NONE,
	[TAPI_EVENT_TONE_DET_RECEIVE]	= IFX_TAPI_EVENT_TONE_DET_RECEIVE,
	[TAPI_EVENT_TONE_DET_TRANSMIT]	= IFX_TAPI_EVENT_TONE_DET_TRANSMIT,
	[TAPI_EVENT_TONE_DET_CPT]	= IFX_TAPI_EVENT_TONE_DET_CPT,
	[TAPI_EVENT_TONE_DET_CPT_END]	= IFX_TAPI_EVENT_TONE_DET_CPT_END,
	[TAPI_EVENT_TONE_DET_MF_R2_START] =
			IFX_TAPI_EVENT_TONE_DET_MF_R2_START,
	[TAPI_EVENT_TONE_DET_MF_R2_END]	= IFX_TAPI_EVENT_TONE_DET_MF_R2_END,
	[TAPI_EVENT_FAXMODEM_NONE]	= IFX_TAPI_EVENT_FAXMODEM_NONE,
	[TAPI_EVENT_FAXMODEM_DIS]	= IFX_TAPI_EVENT_FAXMODEM_DIS,
	[TAPI_EVENT_FAXMODEM_CED]	= IFX_TAPI_EVENT_FAXMODEM_CED,
	[TAPI_EVENT_FAXMODEM_PR]	= IFX_TAPI_EVENT_FAXMODEM_PR,
	[TAPI_EVENT_FAXMODEM_AM]	= IFX_TAPI_EVENT_FAXMODEM_AM,
	[TAPI_EVENT_FAXMODEM_CNGFAX]	= IFX_TAPI_EVENT_FAXMODEM_CNGFAX,
	[TAPI_EVENT_FAXMODEM_CNGMOD]	= IFX_TAPI_EVENT_FAXMODEM_CNGMOD,
	[TAPI_EVENT_FAXMODEM_V21L]	= IFX_TAPI_EVENT_FAXMODEM_V21L,
	[TAPI_EVENT_FAXMODEM_V18A]	= IFX_TAPI_EVENT_FAXMODEM_V18A,
	[TAPI_EVENT_FAXMODEM_V27]	= IFX_TAPI_EVENT_FAXMODEM_V27,
	[TAPI_EVENT_FAXMODEM_BELL]	= IFX_TAPI_EVENT_FAXMODEM_BELL,
	[TAPI_EVENT_FAXMODEM_V22]	= IFX_TAPI_EVENT_FAXMODEM_V22,
	[TAPI_EVENT_FAXMODEM_V22ORBELL]	= IFX_TAPI_EVENT_FAXMODEM_V22ORBELL,
	[TAPI_EVENT_FAXMODEM_V32AC]	= IFX_TAPI_EVENT_FAXMODEM_V32AC,
	[TAPI_EVENT_FAXMODEM_V8BIS]	= IFX_TAPI_EVENT_FAXMODEM_V8BIS,
	[TAPI_EVENT_FAXMODEM_HOLDEND]	= IFX_TAPI_EVENT_FAXMODEM_HOLDEND,
	[TAPI_EVENT_FAXMODEM_CEDEND]	= IFX_TAPI_EVENT_FAXMODEM_CEDEND,
	[TAPI_EVENT_FAXMODEM_CAS_BELL]	= IFX_TAPI_EVENT_FAXMODEM_CAS_BELL,
	[TAPI_EVENT_FAXMODEM_V21H]	= IFX_TAPI_EVENT_FAXMODEM_V21H,
	[TAPI_EVENT_FAXMODEM_VMD]	= IFX_TAPI_EVENT_FAXMODEM_VMD,
	[TAPI_EVENT_LIN_NONE]		= IFX_TAPI_EVENT_LIN_NONE,
	[TAPI_EVENT_LIN_UNDERFLOW]	= IFX_TAPI_EVENT_LIN_UNDERFLOW,
	[TAPI_EVENT_COD_NONE]		= IFX_TAPI_EVENT_COD_NONE,
	[TAPI_EVENT_COD_DEC_CHG]	= IFX_TAPI_EVENT_COD_DEC_CHG,
	[TAPI_EVENT_COD_ROOM_NOISE]	= IFX_TAPI_EVENT_COD_ROOM_NOISE,
	[TAPI_EVENT_COD_ROOM_SILENCE]	= IFX_TAPI_EVENT_COD_ROOM_SILENCE,
	[TAPI_EVENT_COD_ANNOUNCE_END]	= IFX_TAPI_EVENT_COD_ANNOUNCE_END,
	[TAPI_EVENT_COD_MOS]		= IFX_TAPI_EVENT_COD_MOS,
	[TAPI_EVENT_RTP_NONE]		= IFX_TAPI_EVENT_RTP_NONE,
	[TAPI_EVENT_RTP_FIRST]		= IFX_TAPI_EVENT_RTP_FIRST,
	[TAPI_EVENT_RTP_EXT_BROKEN]	= IFX_TAPI_EVENT_RTP_EXT_BROKEN,
	[TAPI_EVENT_RTP_EXT_SSRC_CHANGED] =
			IFX_TAPI_EVENT_RTP_EXT_SSRC_CHANGED,
	[TAPI_EVENT_AAL_NONE]		= IFX_TAPI_EVENT_AAL_NONE,
	[TAPI_EVENT_RFC2833_NONE]	= IFX_TAPI_EVENT_RFC2833_NONE,
	[TAPI_EVENT_RFC2833_EVENT]	= IFX_TAPI_EVENT_RFC2833_EVENT,
	[TAPI_EVENT_KPI_NONE]		= IFX_TAPI_EVENT_KPI_NONE,
	[TAPI_EVENT_KPI_INGRESS_FIFO_FULL] =
			IFX_TAPI_EVENT_KPI_INGRESS_FIFO_FULL,
	[TAPI_EVENT_KPI_SOCKET_FAILURE]	= IFX_TAPI_EVENT_KPI_SOCKET_FAILURE,
	[TAPI_EVENT_T38_NONE]		= IFX_TAPI_EVENT_T38_NONE,
	[TAPI_EVENT_T38_ERROR_GEN]	= IFX_TAPI_EVENT_T38_ERROR_GEN,
	[TAPI_EVENT_T38_ERROR_OVLD]	= IFX_TAPI_EVENT_T38_ERROR_OVLD,
	[TAPI_EVENT_T38_ERROR_READ]	= IFX_TAPI_EVENT_T38_ERROR_READ,
	[TAPI_EVENT_T38_ERROR_WRITE]	= IFX_TAPI_EVENT_T38_ERROR_WRITE,
	[TAPI_EVENT_T38_ERROR_DATA]	= IFX_TAPI_EVENT_T38_ERROR_DATA,
	[TAPI_EVENT_T38_ERROR_SETUP]	= IFX_TAPI_EVENT_T38_ERROR_SETUP,
	[TAPI_EVENT_T38_FDP_REQ]	= IFX_TAPI_EVENT_T38_FDP_REQ,
	[TAPI_EVENT_T38_STATE_CHANGE]	= IFX_TAPI_EVENT_T38_STATE_CHANGE,
	[TAPI_EVENT_JB_NONE]		= IFX_TAPI_EVENT_JB_NONE,
	[TAPI_EVENT_DOWNLOAD_NONE]	= IFX_TAPI_EVENT_DOWNLOAD_NONE,
	[TAPI_EVENT_INFO_NONE]		= IFX_TAPI_EVENT_INFO_NONE,
	[TAPI_EVENT_INFO_MBX_CONGESTION] =
			IFX_TAPI_EVENT_INFO_MBX_CONGESTION,
	[TAPI_EVENT_DEBUG_NONE]		= IFX_TAPI_EVENT_DEBUG_NONE,
	[TAPI_EVENT_DEBUG_CERR]		= IFX_TAPI_EVENT_DEBUG_CERR,
	[TAPI_EVENT_GPIO_HL]		= IFX_TAPI_EVENT_GPIO_HL,
	[TAPI_EVENT_GPIO_LH]		= IFX_TAPI_EVENT_GPIO_LH,
	[TAPI_EVENT_LL_DRIVER_NONE]	= IFX_TAPI_EVENT_LL_DRIVER_NONE,
	[TAPI_EVENT_LL_DRIVER_WD_FAIL]	= IFX_TAPI_EVENT_LL_DRIVER_WD_FAIL,
	[TAPI_EVENT_FAULT_GENERAL_NONE]	= IFX_TAPI_EVENT_FAULT_GENERAL_NONE,
	[TAPI_EVENT_FAULT_GENERAL]	= IFX_TAPI_EVENT_FAULT_GENERAL,
	[TAPI_EVENT_FAULT_GENERAL_CHINFO] =
			IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO,
	[TAPI_EVENT_FAULT_GENERAL_DEVINFO] =
			IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO,
	[TAPI_EVENT_TYPE_FAULT_GENERAL]	= IFX_TAPI_EVENT_TYPE_FAULT_GENERAL,
	[TAPI_EVENT_FAULT_LINE_NONE]	= IFX_TAPI_EVENT_FAULT_LINE_NONE,
	[TAPI_EVENT_FAULT_LINE_GK_POS]	= IFX_TAPI_EVENT_FAULT_LINE_GK_POS,
	[TAPI_EVENT_FAULT_LINE_GK_NEG]	= IFX_TAPI_EVENT_FAULT_LINE_GK_NEG,
	[TAPI_EVENT_FAULT_LINE_GK_LOW]	= IFX_TAPI_EVENT_FAULT_LINE_GK_LOW,
	[TAPI_EVENT_FAULT_LINE_GK_HIGH]	= IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH,
	[TAPI_EVENT_FAULT_LINE_OVERTEMP] =
			IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP,
	[TAPI_EVENT_FAULT_LINE_OVERCURRENT] =
			IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT,
	[TAPI_EVENT_FAULT_LINE_GK_LOW_INT] =
			IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_INT,
	[TAPI_EVENT_FAULT_LINE_GK_HIGH_INT] =
			IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_INT,
	[TAPI_EVENT_FAULT_LINE_GK_LOW_END] =
			IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END,
	[TAPI_EVENT_FAULT_LINE_GK_HIGH_END] =
			IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END,
	[TAPI_EVENT_FAULT_LINE_OVERTEMP_END] =
			IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END,
	[TAPI_EVENT_FAULT_HW_NONE]	= IFX_TAPI_EVENT_FAULT_HW_NONE,
	[TAPI_EVENT_FAULT_HW_SPI_ACCESS] =
			IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS,
	[TAPI_EVENT_FAULT_HW_CLOCK_FAIL] =
			IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL,
	[TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END] =
			IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END,
	[TAPI_EVENT_FAULT_HW_FAULT]	= IFX_TAPI_EVENT_FAULT_HW_FAULT,
	[TAPI_EVENT_FAULT_HW_SYNC]	= IFX_TAPI_EVENT_FAULT_HW_SYNC,
	[TAPI_EVENT_FAULT_HW_RESET]	= IFX_TAPI_EVENT_FAULT_HW_RESET,
	[TAPI_EVENT_FAULT_HW_SSI_ERR]	= IFX_TAPI_EVENT_FAULT_HW_SSI_ERR,
	[TAPI_EVENT_FAULT_HW_SSI_ERR_END] =
			IFX_TAPI_EVENT_FAULT_HW_SSI_ERR_END,
	[TAPI_EVENT_FAULT_FW_NONE]	= IFX_TAPI_EVENT_FAULT_FW_NONE,
	[TAPI_EVENT_FAULT_FW_EBO_UF]	= IFX_TAPI_EVENT_FAULT_FW_EBO_UF,
	[TAPI_EVENT_FAULT_FW_EBO_OF]	= IFX_TAPI_EVENT_FAULT_FW_EBO_OF,
	[TAPI_EVENT_FAULT_FW_CBO_UF]	= IFX_TAPI_EVENT_FAULT_FW_CBO_UF,
	[TAPI_EVENT_FAULT_FW_CBO_OF]	= IFX_TAPI_EVENT_FAULT_FW_CBO_OF,
	[TAPI_EVENT_FAULT_FW_CBI_OF]	= IFX_TAPI_EVENT_FAULT_FW_CBI_OF,
	[TAPI_EVENT_FAULT_FW_WATCHDOG]	= IFX_TAPI_EVENT_FAULT_FW_WATCHDOG,
	[TAPI_EVENT_FAULT_SW_NONE]	= IFX_TAPI_EVENT_FAULT_SW_NONE,
	[TAPI_EVENT_FAULT_HDLC_NONE]	= IFX_TAPI_EVENT_FAULT_HDLC_NONE,
	[TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH] =
			IFX_TAPI_EVENT_FAULT_HDLC_FRAME_LENGTH,
	[TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH] =
			IFX_TAPI_EVENT_FAULT_HDLC_NO_KPI_PATH,
	[TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW] =
			IFX_TAPI_EVENT_FAULT_HDLC_TX_OVERFLOW,
	[TAPI_EVENT_FAULT_HDLC_DISABLED] =
			IFX_TAPI_EVENT_FAULT_HDLC_DISABLED,
};

tapi_handle_t
tapi_open(const char *tapidev)
{
	tapi_handle_t th;

	th = malloc(sizeof(tapi_handle_t));
	if (th == NULL)
		return (NULL);

	th->fd = open(tapidev, O_RDWR);
	if (th->fd < 0) {
		free(th);
		return (NULL);
	}

	return (th);
}

void
tapi_close(tapi_handle_t th)
{
	close(th->fd);
	free(th);
}

int
tapi_basic_init(tapi_handle_t th, unsigned int device, unsigned int irq)
{
	DXT_BasicDeviceInit_t binit;

	binit.dev	= device;
	binit.nIrqNum	= irq;
	binit.nCfIrqNum	= 0;

	return (ioctl(th->fd, FIO_DXT_BASICDEV_INIT, &binit));
}

int
tapi_fw_download(tapi_handle_t th, unsigned int device, const char *fwfile)
{
	DXT_FW_Download_t fwdl;
	off_t fwsize;
	int fwh, r;
	void *fw;

	if (fwfile != NULL) {
		fwh = open(fwfile, O_RDONLY);
		if (fwh < 0)
			return (-1);

		fwsize = lseek(fwh, 0, SEEK_END);
		if (fwsize == -1) {
			close(fwh);
			return (-1);
		}

		fw = mmap(NULL, fwsize, PROT_READ, MAP_SHARED, fwh, 0);
		if (fw == MAP_FAILED) {
			close(fwh);
			return (-1);
		}

		fwdl.dev	= device;
		fwdl.nEdspFlags	= DXT_NO_ASDSP_DWLD;
		fwdl.pPRAMfw	= fw;
		fwdl.pram_size	= fwsize;
		r = ioctl(th->fd, FIO_DXT_FW_DOWNLOAD, &fwdl);

		munmap(fw, fwsize);
		close(fwh);
	} else {
		fwdl.dev	= device;
		fwdl.nEdspFlags	= DXT_NO_ASDSP_DWLD;
		fwdl.pPRAMfw	= tapi_firmware_blob;
		fwdl.pram_size	= tapi_firmware_blob_size;
		r = ioctl(th->fd, FIO_DXT_FW_DOWNLOAD, &fwdl);
	}

	return (r);
}

int
tapi_dev_start(tapi_handle_t th, unsigned int device)
{
	IFX_TAPI_DEV_START_CFG_t devstart;

	devstart.dev	= device;
	devstart.nMode	= IFX_TAPI_INIT_MODE_DEFAULT;

	return (ioctl(th->fd, IFX_TAPI_DEV_START, &devstart));
}

int
tapi_dev_stop(tapi_handle_t th, unsigned int device)
{
	IFX_TAPI_DEV_START_CFG_t devstart;

	devstart.dev	= device;
	devstart.nMode	= IFX_TAPI_INIT_MODE_DEFAULT;

	return (ioctl(th->fd, IFX_TAPI_DEV_STOP, &devstart));
}

int
tapi_bbd_download(tapi_handle_t th, unsigned int device, unsigned int channel,
							const char *bbdfile)
{
	DXT_BBD_Download_t bbddl;
	off_t bbdsize;
	int bbdh, r;
	void *bbd;

	if (bbdfile != NULL) {
		bbdh = open(bbdfile, O_RDONLY);
		if (bbdh < 0)
			return (-1);

		bbdsize = lseek(bbdh, 0, SEEK_END);
		if (bbdsize == -1) {
			close(bbdh);
			return (-1);
		}

		bbd = mmap(NULL, bbdsize, PROT_READ, MAP_SHARED, bbdh, 0);
		if (bbd == MAP_FAILED) {
			close(bbdh);
			return (-1);
		}

		bbddl.dev	= device;
		bbddl.buf	= bbd;
		bbddl.size	= bbdsize;
		if (channel == -1) {
			bbddl.ch = 0;
			bbddl.bBroadcast= 1;
		} else {
			bbddl.ch = channel;
			bbddl.bBroadcast= 0;
		}
		r = ioctl(th->fd, FIO_DXT_BBD_DOWNLOAD, &bbddl);

		munmap(bbd, bbdsize);
		close(bbdh);
	} else {
		bbddl.dev	= device;
		bbddl.buf	= tapi_bbd_blob;
		bbddl.size	= tapi_bbd_blob_size;
		if (channel == -1) {
			bbddl.ch = 0;
			bbddl.bBroadcast= 1;
		} else {
			bbddl.ch = channel;
			bbddl.bBroadcast= 0;
		}
		r = ioctl(th->fd, FIO_DXT_BBD_DOWNLOAD, &bbddl);
	}

	return (r);
}

int
tapi_line_type_set(tapi_handle_t th, unsigned int device, unsigned int channel,
						     tapi_line_type_t linetype)
{
	IFX_TAPI_LINE_TYPE_CFG_t lt;
	const IFX_TAPI_LINE_TYPE_t ltmap[] = {
		[TAPI_LINE_TYPE_FXS_NB]		= IFX_TAPI_LINE_TYPE_FXS_NB,
		[TAPI_LINE_TYPE_FXS_WB]		= IFX_TAPI_LINE_TYPE_FXS_WB,
		[TAPI_LINE_TYPE_FXS_AUTO]	= IFX_TAPI_LINE_TYPE_FXS_AUTO,
		[TAPI_LINE_TYPE_FXO_NB]		= IFX_TAPI_LINE_TYPE_FXO_NB,
	};

	lt.dev		= device;
	lt.ch		= channel;
	lt.lineType	= ltmap[linetype];
	lt.nDaaCh	= 0;

	return (ioctl(th->fd, IFX_TAPI_LINE_TYPE_SET, &lt));
}

int
tapi_line_feed_set(tapi_handle_t th, unsigned int device, unsigned int channel,
						     tapi_line_feed_t linefeed)
{
	IFX_TAPI_LINE_FEED_t lf;
	const lfmap[] = {
		[TAPI_LINE_FEED_ACTIVE]		= IFX_TAPI_LINE_FEED_ACTIVE,
		[TAPI_LINE_FEED_ACTIVE_REV]	= IFX_TAPI_LINE_FEED_ACTIVE_REV,
		[TAPI_LINE_FEED_STANDBY]	= IFX_TAPI_LINE_FEED_STANDBY,
		[TAPI_LINE_FEED_HIGH_IMPEDANCE]	= IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE,
		[TAPI_LINE_FEED_DISABLED]	= IFX_TAPI_LINE_FEED_DISABLED,
	};

	lf.dev		= device;
	lf.ch		= channel;
	lf.lineMode	= lfmap[linefeed];

	return (ioctl(th->fd, IFX_TAPI_LINE_FEED_SET, &lf));
}

int
tapi_line_calibrate(tapi_handle_t th, unsigned int device, unsigned int channel)
{
	IFX_TAPI_CALIBRATION_t cal;

	cal.dev		= device;
	cal.ch		= channel;

	return (ioctl(th->fd, IFX_TAPI_CALIBRATION_START, &cal));
}

int
tapi_ring_start(tapi_handle_t th, unsigned int device, unsigned int channel)
{
	IFX_TAPI_RING_t ring;

	ring.dev	= device;
	ring.ch		= channel;

	return (ioctl(th->fd, IFX_TAPI_RING_START, &ring));
}

int
tapi_ring_stop(tapi_handle_t th, unsigned int device, unsigned int channel)
{
	IFX_TAPI_RING_t ring;

	ring.dev	= device;
	ring.ch		= channel;

	return (ioctl(th->fd, IFX_TAPI_RING_STOP, &ring));
}

tapi_line_hook_t
tapi_hook_status_get(tapi_handle_t th, unsigned int device, unsigned int channel)
{
	IFX_TAPI_LINE_HOOK_STATUS_GET_t hook;
	const IFX_TAPI_LINE_HOOK_t hmap[] = {
		[IFX_TAPI_LINE_ONHOOK]	= TAPI_LINE_ONHOOK,
		[IFX_TAPI_LINE_OFFHOOK]	= TAPI_LINE_OFFHOOK,
	};
	int r;

	hook.dev	= device;
	hook.ch		= channel;
	r = ioctl(th->fd, IFX_TAPI_LINE_HOOK_STATUS_GET, &hook);
	if (r < 0)
		return (r);

	return (hmap[hook.hookMode]);
}

static tapi_event_id_t
tapi_translate_event_id(IFX_TAPI_EVENT_ID_t ifx_id)
{
	tapi_event_id_t id;
	int i;

	for (i = 0; i < ARRAY_SIZE(tapi_event_tanslation_table); i++) {
		if (ifx_id == tapi_event_tanslation_table[i])
			return (i);
	}

	return (-1);
}

int
tapi_event_enable(tapi_handle_t th, unsigned int device, unsigned int channel,
							tapi_event_id_t event)
{
	IFX_TAPI_EVENT_t ifx_event;

	memset(&ifx_event, 0, sizeof(ifx_event));
	ifx_event.dev		= device;
	ifx_event.ch		= channel;
	ifx_event.id		= tapi_event_tanslation_table[event];

	return (ioctl(th->fd, IFX_TAPI_EVENT_ENABLE, &ifx_event));
}

int
tapi_event_get(tapi_handle_t th, unsigned int device, int timeout,
							tapi_event_t *event)
{
	IFX_TAPI_EVENT_t ifx_event;
	struct pollfd fds;
	int r;

	fds.fd		= th->fd;
	fds.events	= POLLIN | POLLERR;
	fds.revents	= 0;

	r = poll(&fds, 1, timeout);
	if (r <= 0)
		return (r);

	memset(&ifx_event, 0, sizeof(ifx_event));
	ifx_event.dev	= device;
	ifx_event.ch	= IFX_TAPI_EVENT_ALL_CHANNELS;

	if (ioctl(th->fd, IFX_TAPI_EVENT_GET, &ifx_event) < 0)
		return (-1);

	event->device	= ifx_event.dev;
	event->channel	= ifx_event.ch;
	event->event	= tapi_translate_event_id(ifx_event.id);

	return (r);
}

int
tapi_pcm_if_config(tapi_handle_t th, unsigned int device,
		tapi_pcm_if_mode_t mode, tapi_pcm_if_freq_t freq,
		tapi_pcm_if_slope_t rxslope, tapi_pcm_if_slope_t txslope)
{
	IFX_TAPI_PCM_IF_CFG_t pcmifcfg;
	const IFX_TAPI_PCM_IF_MODE_t modemap[] = {
		[TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ] =
					IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ,
		[TAPI_PCM_IF_MODE_SLAVE]	= IFX_TAPI_PCM_IF_MODE_SLAVE,
		[TAPI_PCM_IF_MODE_MASTER]	= IFX_TAPI_PCM_IF_MODE_MASTER,
	};
	const IFX_TAPI_PCM_IF_DCLFREQ_t freqmap[] = {
		[TAPI_PCM_IF_FREQ_512KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_512,
		[TAPI_PCM_IF_FREQ_1024KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_1024,
		[TAPI_PCM_IF_FREQ_1536KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_1536,
		[TAPI_PCM_IF_FREQ_2048KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_2048,
		[TAPI_PCM_IF_FREQ_4096KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_4096,
		[TAPI_PCM_IF_FREQ_8192KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_8192,
		[TAPI_PCM_IF_FREQ_16384KHZ]	= IFX_TAPI_PCM_IF_DCLFREQ_16384,
	};
	const IFX_TAPI_PCM_IF_SLOPE_t slopemap[] = {
		[TAPI_PCM_IF_SLOPE_RISE]	= IFX_TAPI_PCM_IF_SLOPE_RISE,
		[TAPI_PCM_IF_SLOPE_FALL]	= IFX_TAPI_PCM_IF_SLOPE_FALL,
	};

	pcmifcfg.dev		= device;
	pcmifcfg.nHighway	= 0;
	pcmifcfg.nOpMode	= modemap[mode];
	pcmifcfg.nDCLFreq	= freqmap[freq];
	pcmifcfg.nDoubleClk	= IFX_DISABLE;
	pcmifcfg.nSlopeRX	= slopemap[rxslope];
	pcmifcfg.nSlopeTX	= slopemap[txslope];
	pcmifcfg.nOffsetRX	= IFX_TAPI_PCM_IF_OFFSET_NONE;
	pcmifcfg.nOffsetTX	= IFX_TAPI_PCM_IF_OFFSET_NONE;
	pcmifcfg.nDrive		= IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
	pcmifcfg.nShift		= IFX_DISABLE;
	pcmifcfg.nMCTS		= 0x00;

	return (ioctl(th->fd, IFX_TAPI_PCM_IF_CFG_SET, &pcmifcfg));
}

int
tapi_pcm_channel_config(tapi_handle_t th, unsigned int device,
		unsigned int channel, unsigned int slotrx, unsigned int slottx,
		tapi_pcm_coding_t coding)
{
	IFX_TAPI_PCM_CFG_t pcmcfg;
	const IFX_TAPI_PCM_RES_t codingmap[] = {
		[TAPI_PCM_CODING_NB_ALAW_8BIT]	= IFX_TAPI_PCM_RES_NB_ALAW_8BIT,
		[TAPI_PCM_CODING_NB_ULAW_8BIT]	= IFX_TAPI_PCM_RES_NB_ULAW_8BIT,
		[TAPI_PCM_CODING_NB_LINEAR_16BIT] =
			IFX_TAPI_PCM_RES_NB_LINEAR_16BIT,
		[TAPI_PCM_CODING_WB_ALAW_8BIT]	= IFX_TAPI_PCM_RES_WB_ALAW_8BIT,
		[TAPI_PCM_CODING_WB_ULAW_8BIT]	= IFX_TAPI_PCM_RES_WB_ULAW_8BIT,
		[TAPI_PCM_CODING_WB_LINEAR_16BIT] =
			IFX_TAPI_PCM_RES_WB_LINEAR_16BIT,
		[TAPI_PCM_CODING_WB_G722]	= IFX_TAPI_PCM_RES_WB_G722,
		[TAPI_PCM_CODING_NB_G726_16]	= IFX_TAPI_PCM_RES_NB_G726_16,
		[TAPI_PCM_CODING_NB_G726_24]	= IFX_TAPI_PCM_RES_NB_G726_24,
		[TAPI_PCM_CODING_NB_G726_32]	= IFX_TAPI_PCM_RES_NB_G726_32,
		[TAPI_PCM_CODING_NB_G726_40]	= IFX_TAPI_PCM_RES_NB_G726_40,
		[TAPI_PCM_CODING_WB_LINEAR_SPLIT_16BIT] =
			IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT,
	};

	pcmcfg.dev		= device;
	pcmcfg.ch		= channel;
	pcmcfg.nTimeslotRX	= slotrx;
	pcmcfg.nTimeslotTX	= slottx;
	pcmcfg.nHighway		= 0;
	pcmcfg.nResolution	= codingmap[coding];
	pcmcfg.nSampleSwap	= IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED;
	pcmcfg.nBitPacking	= IFX_TAPI_PCM_BITPACK_LSB;

	return (ioctl(th->fd, IFX_TAPI_PCM_CFG_SET, &pcmcfg));
}

int
tapi_pcm_channel_activate(tapi_handle_t th, unsigned int device,
							unsigned int channel)
{
	IFX_TAPI_PCM_ACTIVATION_t pcmact;

	pcmact.dev	= device;
	pcmact.ch	= channel;
	pcmact.mode	= IFX_ENABLE;

	return(ioctl(th->fd, IFX_TAPI_PCM_ACTIVATION_SET, &pcmact));
}

int
tapi_test_loop(tapi_handle_t th, unsigned int device,
				      unsigned int channel, unsigned int enable)
{
	IFX_TAPI_TEST_LOOP_t tloop;

	tloop.dev	= device;
	tloop.ch	= channel;
	tloop.bAnalog	= enable;

	return(ioctl(th->fd, IFX_TAPI_TEST_LOOP, &tloop));
}
