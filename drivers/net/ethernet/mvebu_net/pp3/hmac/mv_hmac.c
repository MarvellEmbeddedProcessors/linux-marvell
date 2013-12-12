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

/* includes */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "common/mv_hw_if.h"
#include "hmac/mv_hmac.h"
#include "hmac/mv_hmac_regs.h"

/* local functions declaration */
static int mv_pp3_hmac_queue_create(struct mv_pp3_queue_ctrl *q_ctrl, int desc_num);

/* per frame bitmap to store queues state (allocated/free) */
static unsigned int mv_pp3_hmac_queue_act[MV_PP3_HMAC_MAX_FRAME] = {0};
/* */
struct pp3_unit_info pp3_hmac_gl;
struct pp3_unit_info pp3_hmac_fr;

/* store unit base address = silicon base address + unit offset */
void mv_pp3_hmac_gl_unit_base(unsigned int unit_offset)
{
	pp3_hmac_gl.base_addr = unit_offset;
	pp3_hmac_gl.ins_offs = 0;
}

/* store unit base address = silicon base address + unit offset */
/* store unit instance offset                                   */
void mv_pp3_hmac_frame_unit_base(unsigned int unit_offset, unsigned int ins_offset)
{
	pp3_hmac_fr.base_addr = unit_offset;
	pp3_hmac_fr.ins_offs = ins_offset;
}

/* Allocate memory and init RX queue HW facility
 * size is a queue size in datagrams (16 bytes each) */
u32 mv_pp3_hmac_rxq_init(int frame, int queue, int size, struct mv_pp3_queue_ctrl *qctrl)
{
	/* check if already created */
	if ((mv_pp3_hmac_queue_act[frame] >> queue) & 1)
		return 1;

	qctrl->size = size;
	mv_pp3_hmac_queue_create(qctrl, size * MV_PP3_HMAC_DG_SIZE);
/*	Write pointer to allocated memory to
a.	rq_address_high table address bits [39:32]
b.	rq_address_low table address bits [31:8], aligned to 256B
3.	Store queue size in rq_size table, number of 16B units.
4.	Configure Receive Threshold TBD.
5.	Disable queue, hmac_%m_rec_q_%n_control set to 0.
*/
	return 0;
}

/* Return pointer to first free CFH:
 * size is CFH size in datagrams (16 bytes each)     */
void mv_pp3_hmac_txq_next_cfh(int frame, int queue, struct mv_pp3_queue_ctrl *qctrl, int size, u8 **cfh_ptr)
{
	if ((qctrl->next_proc + size * MV_PP3_HMAC_DG_SIZE) > qctrl->last)
		/* do FIFO wraparound */;

	*cfh_ptr = qctrl->next_proc;
	qctrl->next_proc += size;
}

void mv_pp3_hmac_rxq_reset(int frame, int queue)
{
	unsigned int data;

	/* reset queue */
	data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_REC_Q_CTRL_REG(queue));
	data &= ~(MV_HMAC_REC_Q_CTRL_RCV_Q_FLUSH_MASK);
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_REC_Q_CTRL_REG(queue), data);
}

void mv_pp3_hmac_txq_qm_mode_cfg(int frame, int queue, int qm_num)
{
	unsigned int data;

	data = mv_pp3_hmac_frame_reg_read(frame, MV_HMAC_SEND_Q_CTRL_REG(queue));
	data &= ~(MV_HMAC_SEND_Q_CTRL_Q_MODE_MASK); /* set mode to QM */
	mv_pp3_hmac_frame_reg_write(frame, MV_HMAC_SEND_Q_CTRL_REG(queue), data);
}

/* allocate descriptors */
static u8 *mv_pp3_queue_mem_alloc(int size)
{
	u8 *p_virt;

	p_virt = kmalloc(size, GFP_ATOMIC);
	/*if (pVirt)
		mvOsMemset(pVirt, 0, descSize);*/

	return p_virt;
}

static int mv_pp3_hmac_queue_create(struct mv_pp3_queue_ctrl *q_ctrl, int desc_num)
{
	int size;

	/* Allocate memory for queue */
	size = ((desc_num * MV_PP3_CFH_MIN_SIZE) + MV_PP3_HMAC_Q_ALIGN);
	q_ctrl->first = mv_pp3_queue_mem_alloc(size);

	if (q_ctrl->first == NULL) {
		printk(KERN_ERR "%s: Can't allocate %d bytes for %d descr\n", __func__, size, desc_num);
		return 1;
	}

	/* Make sure descriptor address is aligned */
	/*q_ctrl->first = (char *)MV_ALIGN_UP((MV_ULONG) qCtrl->descBuf.bufVirtPtr, MV_PP2_DESC_Q_ALIGN);*/

	q_ctrl->last = q_ctrl->first + size;
	return 0;
}
