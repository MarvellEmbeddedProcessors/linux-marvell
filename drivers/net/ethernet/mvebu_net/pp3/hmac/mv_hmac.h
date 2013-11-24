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

#ifndef __mvHmac_h__
#define __mvHmac_h__


#define MV_PP3_HMAC_MAX_FRAME			(16)
#define MV_PP3_HMAC_DATAGRAM_SIZE		(16)
#define MV_PP3_CFH_MIN_SIZE				(32)
#define MV_PP3_HMAC_Q_ALIGN				(256)

extern struct pp3_uint_info pp3_hmac_gl;
extern struct pp3_uint_info pp3_hmac_fr;

struct mv_pp3_queue_ctrl {
	u8 *first;
	u8 *next_proc;
	u8 *last;
	int   size;  /* number of 16 bytes units (datagram) in queue */
};

static inline u32 mv_pp3_hmac_gl_reg_read(u32 reg)
{
	u32 reg_data;

	mv_pp3_hw_read(reg + pp3_hmac_gl.base_addr, 1, &reg_data);
	/* add debug print */

	return reg_data;
}

static inline u32 mv_pp3_hmac_frame_reg_read(int frameId, u32 reg)
{
	u32 reg_data;
	u32 reg_addr;

	reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frameId + reg;
	/* add debug print */
	mv_pp3_hw_read(reg_addr, 1, &reg_data);

	return reg_data;
}

static inline void mv_pp3_hmac_gl_reg_write(u32 reg, u32 data)
{
	mv_pp3_hw_write(reg + pp3_hmac_gl.base_addr, 1, &data);
}

static inline void mv_pp3_hmac_frame_reg_write(int frame_id, u32 reg, u32 data)
{
	u32 reg_addr = pp3_hmac_fr.base_addr + pp3_hmac_fr.ins_offs * frame_id + reg;

	mv_pp3_hw_write(reg_addr, 1, &data);
}

/* Init HMAC global unit base address
 * unit_offset = silicon base address + unit offset  */
void mv_pp3_hmac_gl_unit_base(u32 unit_offset);
/* Init HMAC Frame first unit base address
 * unit_offset = silicon base address + unit offset
 * frame_offset - is an next frame unit offset       */
void mv_pp3_hmac_frame_unit_base(u32 unit_offset, u32 frame_offset);

/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each) */
u32 mv_pp3_hmac_rxq_init(int frame, int queue, int size, struct mv_pp3_queue_ctrl *qctrl);
void mv_pp3_hmac_rxq_flush(int frame, int queue);
void mv_pp3_hmac_rxq_enable(int frame, int queue);
void mv_pp3_hmac_rxq_disable(int frame, int queue);
void mv_pp3_hmac_rxq_occ_get(int frame, int queue, int *size);
void mv_pp3_hmac_rxq_next_cfh(int frame, int queue, int *size, u8 *cfh_ptr);

/* Allocate memory and init TX queue HW facility:
 * size is a queue size in datagrams (16 bytes each) */
u32 mv_pp3_hmac_txq_init(int frame, int queue, int size, struct mv_pp3_queue_ctrl *qctrl);
void mv_pp3_hmac_txq_flush(int frame, int queue);
void mv_pp3_hmac_txq_enable(int frame, int queue);
void mv_pp3_hmac_txq_disable(int frame, int queue);
/* Return pointer to first free CFH:
 * size is CFH size in datagrams (16 bytes each)     */
void mv_pp3_hmac_txq_next_cfh(int frame, int queue, struct mv_pp3_queue_ctrl *qctrl, int size, u8 **cfh_ptr);
/* size - is number of datagrams to transmit         */
void mv_pp3_hmac_txq_send(int frame, int queue, int size);

/* configure queue parameters used by BM queue
 * bpid - is a buffer pool ID for allocation request
 * bm_alloc_count - is a size of the allocation
 * request in units of 16B                           */
void mv_pp3_hmac_txq_bm_mode_cfg(int frame, int queue, int bpid, int bm_alloc_count);
/* configure queue parameters used by QM queue
 * q_num - is a number of QM queue                   */
void mv_pp3_hmac_txq_qm_mode_cfg(int frame, int queue, int q_num);

#endif /* __mvHmac_h__ */
