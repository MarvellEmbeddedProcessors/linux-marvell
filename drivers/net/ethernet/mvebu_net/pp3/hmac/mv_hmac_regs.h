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

#ifndef __mvHmacRegs_h__
#define __mvHmacRegs_h__

/* includes */
/* Sets the field located at the specified offset & length in data.     */
#define U32_SET_FIELD(data, offset, mask, val)		((data) = (((data) & ~(mask)) | ((val) << (offset))))

/* unit offset */
#define MV_PP3_HMAC_GL_UNIT_OFFSET	0x30000
#define MV_PP3_HMAC_FR_UNIT_OFFSET	0x200000

/************************** HMAC GLOBAL regs *********************************************************/

/* Hmac_eco */
#define MV_HMAC_ECO_REG								0x0000
#define MV_HMAC_ECO_GENERAL_PURPOSE_ECO_OFFS		0

/* Hmac_bw_control */
#define MV_HMAC_BW_CTRL_REG								0x0004
#define MV_HMAC_BW_CTRL_NUMBER_OF_OUTSTANDING_REQUESTS_OFFS		0
#define MV_HMAC_BW_CTRL_NUMBER_OF_OUTSTANDING_REQUESTS_MASK    \
		(0x0000000f << MV_HMAC_BW_CTRL_NUMBER_OF_OUTSTANDING_REQUESTS_OFFS)

#define MV_HMAC_BW_CTRL_WRITE_DATA_BURST_SIZE_OFFS		4
#define MV_HMAC_BW_CTRL_WRITE_DATA_BURST_SIZE_MASK    \
		(0x0000000f << MV_HMAC_BW_CTRL_WRITE_DATA_BURST_SIZE_OFFS)

#define MV_HMAC_BW_CTRL_READ_DATA_BURST_SIZE_OFFS		8
#define MV_HMAC_BW_CTRL_READ_DATA_BURST_SIZE_MASK    \
		(0x0000000f << MV_HMAC_BW_CTRL_READ_DATA_BURST_SIZE_OFFS)

#define MV_HMAC_BW_CTRL_DESC_PREFETCH_NUMBER_OFFS		12
#define MV_HMAC_BW_CTRL_DESC_PREFETCH_NUMBER_MASK    \
		(0x0000000f << MV_HMAC_BW_CTRL_DESC_PREFETCH_NUMBER_OFFS)


/* Hmac_rec_qm_port_number */
#define MV_HMAC_REC_QM_PORT_NUMBER_REG								0x0008
#define MV_HMAC_REC_QM_PORT_NUMBER_RQ_QM_HMAC_PORT_NUM_OFFS		0
#define MV_HMAC_REC_QM_PORT_NUMBER_RQ_QM_HMAC_PORT_NUM_MASK    \
		(0x00000fff << MV_HMAC_REC_QM_PORT_NUMBER_RQ_QM_HMAC_PORT_NUM_OFFS)


/* Hmac_vmid_frame_%m */
#define MV_HMAC_VMID_FRAME_REG(m)							(0x0010 + 4*m)
#define MV_HMAC_VMID_FRAME_CONTEXT_ID_OFFS		0
#define MV_HMAC_VMID_FRAME_CONTEXT_ID_MASK    \
		(0x0000003f << MV_HMAC_VMID_FRAME_CONTEXT_ID_OFFS)

#define MV_HMAC_VMID_FRAME_AXI_PROT_PRIVILEGE_OFFS		16
#define MV_HMAC_VMID_FRAME_AXI_PROT_PRIVILEGE_MASK    \
		(0x00000001 << MV_HMAC_VMID_FRAME_AXI_PROT_PRIVILEGE_OFFS)

#define MV_HMAC_VMID_FRAME_AW_QOS_OFFS		20
#define MV_HMAC_VMID_FRAME_AW_QOS_MASK    \
		(0x00000003 << MV_HMAC_VMID_FRAME_AW_QOS_OFFS)

#define MV_HMAC_VMID_FRAME_AR_QOS_OFFS		24
#define MV_HMAC_VMID_FRAME_AR_QOS_MASK    \
		(0x00000003 << MV_HMAC_VMID_FRAME_AR_QOS_OFFS)


/* Hmac_event_addr_low_%m */
#define MV_HMAC_EVENT_ADDR_LOW_REG(m)							(0x0050 + 8*m)
#define MV_HMAC_EVENT_ADDR_LOW_EVENT_ADDRESS_LOW_OFFS		8
#define MV_HMAC_EVENT_ADDR_LOW_EVENT_ADDRESS_LOW_MASK    \
		(0x00ffffff << MV_HMAC_EVENT_ADDR_LOW_EVENT_ADDRESS_LOW_OFFS)


/* Hmac_event_addr_high_%m */
#define MV_HMAC_EVENT_ADDR_HIGH_REG(m)							(0x0054 + 8*m)
#define MV_HMAC_EVENT_ADDR_HIGH_EVENT_ADDRESS_HIGH_OFFS		0
#define MV_HMAC_EVENT_ADDR_HIGH_EVENT_ADDRESS_HIGH_MASK    \
		(0x000000ff << MV_HMAC_EVENT_ADDR_HIGH_EVENT_ADDRESS_HIGH_OFFS)


/* Hmac_dram_update_time_out_%m */
#define MV_HMAC_DRAM_UPDATE_TIME_OUT_REG(m)							(0x00d0 + 4*m)
#define MV_HMAC_DRAM_UPDATE_TIME_OUT_DRAM_UPDAT_TIME_OUT_OFFS		0
#define MV_HMAC_DRAM_UPDATE_TIME_OUT_DRAM_UPDAT_TIME_OUT_MASK    \
		(0x0000ffff << MV_HMAC_DRAM_UPDATE_TIME_OUT_DRAM_UPDAT_TIME_OUT_OFFS)


/* Hmac_dram_update_threshold_%m */
#define MV_HMAC_DRAM_UPDATE_THRESHOLD_REG(m)							(0x0110 + 4*m)
#define MV_HMAC_DRAM_UPDATE_THRESHOLD_DRAM_UPDAT_THRESHOLD_OFFS		0
#define MV_HMAC_DRAM_UPDATE_THRESHOLD_DRAM_UPDAT_THRESHOLD_MASK    \
		(0x0000ffff << MV_HMAC_DRAM_UPDATE_THRESHOLD_DRAM_UPDAT_THRESHOLD_OFFS)


/* Hmac_axi_prot_secure_%m */
#define MV_HMAC_AXI_PROT_SECURE_REG(m)							(0x0300 + 4*m)
#define MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_OFFS		0
#define MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_MASK    \
		(0x00000001 << MV_HMAC_AXI_PROT_SECURE_AXI_PROT_SECURE_OFFS)

/************************** HMAC FRAME regs *********************************************************/

/* Hmac_%m_rec_q_%n_control */
#define MV_HMAC_REC_Q_CTRL_REG(n)							(0x8000 + 0x100*n)
#define MV_HMAC_REC_Q_CTRL_RCV_Q_EN_OFFS		0
#define MV_HMAC_REC_Q_CTRL_RCV_Q_EN_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_CTRL_RCV_Q_EN_OFFS)

#define MV_HMAC_REC_Q_CTRL_RCV_Q_FLUSH_OFFS		1
#define MV_HMAC_REC_Q_CTRL_RCV_Q_FLUSH_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_CTRL_RCV_Q_FLUSH_OFFS)

#define MV_HMAC_REC_Q_CTRL_Q_DLB_EN_OFFS		8
#define MV_HMAC_REC_Q_CTRL_Q_DLB_EN_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_CTRL_Q_DLB_EN_OFFS)

#define MV_HMAC_REC_Q_CTRL_RCV_Q_TIMER_SEL_OFFS		12
#define MV_HMAC_REC_Q_CTRL_RCV_Q_TIMER_SEL_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_CTRL_RCV_Q_TIMER_SEL_OFFS)


/* Hmac_%m_rec_q_%n_occ_status_update */
#define MV_HMAC_REC_Q_OCC_STATUS_UPDATE_REG(n)							(0x8004 + 0x100*n)
#define MV_HMAC_REC_Q_OCC_STATUS_UPDATE_RCV_ACK_COUNT_OFFS		0
#define MV_HMAC_REC_Q_OCC_STATUS_UPDATE_RCV_ACK_COUNT_MASK    \
		(0x0000ffff << MV_HMAC_REC_Q_OCC_STATUS_UPDATE_RCV_ACK_COUNT_OFFS)


/* Hmac_%m_rec_q_%n_status */
#define MV_HMAC_REC_Q_STATUS_REG(n)							(0x800c + 0x100*n)
#define MV_HMAC_REC_Q_STATUS_RQ_BUSY_OFFS		0
#define MV_HMAC_REC_Q_STATUS_RQ_BUSY_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_STATUS_RQ_BUSY_OFFS)

#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_READ_OFFS		1
#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_READ_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_READ_OFFS)

#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_WRITE_OFFS		2
#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_WRITE_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_STATUS_RQ_PENDING_AXI_WRITE_OFFS)

#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_EVENT_OFFS		3
#define MV_HMAC_REC_Q_STATUS_RQ_PENDING_EVENT_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_STATUS_RQ_PENDING_EVENT_OFFS)

#define MV_HMAC_REC_Q_STATUS_RQ_REMAINDER_NEMPTY_OFFS		4
#define MV_HMAC_REC_Q_STATUS_RQ_REMAINDER_NEMPTY_MASK    \
		(0x00000001 << MV_HMAC_REC_Q_STATUS_RQ_REMAINDER_NEMPTY_OFFS)


/* Hmac_%m_rec_q_%n_timeout */
#define MV_HMAC_REC_Q_TIMEOUT_REG(n)							(0x8010 + n)
#define MV_HMAC_REC_Q_TIMEOUT_RQ_TIME_OUT_0_OFFS		5
#define MV_HMAC_REC_Q_TIMEOUT_RQ_TIME_OUT_0_MASK    \
		(0x000007ff << MV_HMAC_REC_Q_TIMEOUT_RQ_TIME_OUT_0_OFFS)


/* Hmac_%m_send_q_%n_control */
#define MV_HMAC_SEND_Q_CTRL_REG(n)							(0x8040 + 0x100*n)
#define MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_OFFS		0
#define MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_CTRL_SEND_Q_EN_OFFS)

#define MV_HMAC_SEND_Q_CTRL_SEND_Q_FLUSH_OFFS		1
#define MV_HMAC_SEND_Q_CTRL_SEND_Q_FLUSH_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_CTRL_SEND_Q_FLUSH_OFFS)

#define MV_HMAC_SEND_Q_CTRL_Q_MODE_OFFS		4
#define MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_CTRL_Q_MODE_OFFS)

#define MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_OFFS		8
#define MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_CTRL_BM_PE_FORMAT_OFFS)

#define MV_HMAC_SEND_Q_CTRL_SEND_Q_TIMER_SEL_OFFS		12
#define MV_HMAC_SEND_Q_CTRL_SEND_Q_TIMER_SEL_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_CTRL_SEND_Q_TIMER_SEL_OFFS)


/* Hmac_%m_send_q_%n_occ_status_update */
#define MV_HMAC_SEND_Q_OCC_STATUS_UPDATE_REG(n)							(0x8044 + 0x100*n)
#define MV_HMAC_SEND_Q_OCC_STATUS_UPDATE_SND_ADD_COUNT_OFFS		0
#define MV_HMAC_SEND_Q_OCC_STATUS_UPDATE_SND_ADD_COUNT_MASK    \
		(0x0000ffff << MV_HMAC_SEND_Q_OCC_STATUS_UPDATE_SND_ADD_COUNT_OFFS)


/* Hmac_%m_send_q_%n_q_num_bpid */
#define MV_HMAC_SEND_Q_NUM_BPID_REG(n)							(0x8048 + 0x100*n)
#define MV_HMAC_SEND_Q_NUM_BPID_QNUM_OFFS		0
#define MV_HMAC_SEND_Q_NUM_BPID_QNUM_MASK    \
		(0x00000fff << MV_HMAC_SEND_Q_NUM_BPID_QNUM_OFFS)

#define MV_HMAC_SEND_Q_NUM_BPID_BPID_OFFS		0
#define MV_HMAC_SEND_Q_NUM_BPID_BPID_MASK    \
		(0x000000ff << MV_HMAC_SEND_Q_NUM_BPID_BPID_OFFS)

#define MV_HMAC_SEND_Q_NUM_BPID_BM_ALLOC_COUNT_OFFS		8
#define MV_HMAC_SEND_Q_NUM_BPID_BM_ALLOC_COUNT_MASK    \
		(0x00000007 << MV_HMAC_SEND_Q_NUM_BPID_BM_ALLOC_COUNT_OFFS)


/* Hmac_%m_send_q_%n_status */
#define MV_HMAC_SEND_Q_STATUS_REG(n)							(0x804c + 0x100*n)
#define MV_HMAC_SEND_Q_STATUS_SQ_BUSY_OFFS		0
#define MV_HMAC_SEND_Q_STATUS_SQ_BUSY_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_STATUS_SQ_BUSY_OFFS)

#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_READ_OFFS		1
#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_READ_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_READ_OFFS)

#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_WRITE_OFFS		2
#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_WRITE_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_STATUS_SQ_PENDING_AXI_WRITE_OFFS)

#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_EVENT_OFFS		3
#define MV_HMAC_SEND_Q_STATUS_SQ_PENDING_EVENT_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_STATUS_SQ_PENDING_EVENT_OFFS)

#define MV_HMAC_SEND_Q_STATUS_SQ_REMAINDER_NEMPTY_OFFS		4
#define MV_HMAC_SEND_Q_STATUS_SQ_REMAINDER_NEMPTY_MASK    \
		(0x00000001 << MV_HMAC_SEND_Q_STATUS_SQ_REMAINDER_NEMPTY_OFFS)


/* Hmac_%m_send_q_%n_timeout */
#define MV_HMAC_SEND_Q_TIMEOUT_REG(n)							(0x8050 + 0x100*n)
#define MV_HMAC_SEND_Q_TIMEOUT_SQ_TIME_OUT_0_OFFS		5
#define MV_HMAC_SEND_Q_TIMEOUT_SQ_TIME_OUT_0_MASK    \
		(0x000007ff << MV_HMAC_SEND_Q_TIMEOUT_SQ_TIME_OUT_0_OFFS)

/* HMAC Frame unit tables offsets */
#define MV_PP3_HMAC_RQ_ADDR_LOW(n)		(0x0000 + 0x100*n)
#define MV_PP3_HMAC_RQ_ADDR_HIGH(n)		(0x0004 + 0x100*n)
#define MV_PP3_HMAC_RQ_SIZE(n)			(0x0008 + 0x100*n)
#define MV_PP3_HMAC_RQ_OCC_STATUS(n)	(0x000C + 0x100*n)
#define MV_PP3_HMAC_RQ_AXI_ATTR(n)		(0x0010 + 0x100*n)
#define MV_PP3_HMAC_RQ_EVENT_GROUP(n)	(0x0014 + 0x100*n)
#define MV_PP3_HMAC_RQ_INT_THRESH(n)	(0x0018 + 0x100*n)
#define MV_PP3_HMAC_RQ_BACK_PRES0(n)	(0x001c + 0x100*n)
#define MV_PP3_HMAC_RQ_BACK_PRES1(n)	(0x0020 + 0x100*n)

#define MV_PP3_HMAC_SQ_ADDR_LOW(n)		(0x0040 + 0x100*n)
#define MV_PP3_HMAC_SQ_ADDR_HIGH(n)		(0x0044 + 0x100*n)
#define MV_PP3_HMAC_SQ_SIZE(n)			(0x0048 + 0x100*n)
#define MV_PP3_HMAC_SQ_OCC_STATUS(n)	(0x004C + 0x100*n)
#define MV_PP3_HMAC_SQ_AXI_ATTR(n)		(0x0050 + 0x100*n)
#define MV_PP3_HMAC_SQ_EVENT_GROUP(n)	(0x0054 + 0x100*n)
#define MV_PP3_HMAC_SQ_SW_QNUM_TDEST(n)	(0x0058 + 0x100*n)

#define MV_PP3_HMAC_OCC_COUNTER_MASK	0xFFFF
/**/

#endif /* __mvHmacRegs_h__ */
