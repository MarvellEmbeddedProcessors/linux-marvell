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

#ifndef __mvPp3Config_h__
#define __mvPp3Config_h__

#include <linux/kernel.h>
#include <linux/io.h>


#define MV_PP3_EMAC_NUM		4
#define MV_PP3_MSG_BUFF_SIZE	4096
#define MV_PP3_MSGR_BUF_NUM	8
#define MV_PP3_GP_POOL_MIN	8
#define MV_PP3_GP_POOL_MAX	35
#define MV_PP3_POOL_MAX		35

#define MV_PP3_NUM_MSG_IN_CHAN	10

/* get messenger bm queue parameters (frame, queue, size)
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_msg_bmq_params_get(int *frame, int *queue, int *size, int *group, int *irq_off)
{
	if ((frame == NULL) || (queue == NULL) || (size == NULL) || (group == NULL) || (irq_off == NULL))
		return -1;

	*frame = 2;
	*queue = 8;
	*size = MV_PP3_NUM_MSG_IN_CHAN;
	*group = 4;
	*irq_off = 20; /*148;*/

	return 0;
}

/* get messenger BM pool ID and number of buffer in the pool
Outputs:
	pool_id	- BM pool ID
	size	- max number of of buffers in pool
*/
static inline int mv_pp3_cfg_msg_bm_pool_params_get(int *pool_id, int *size)
{
	if ((pool_id != NULL) && (size != NULL)) {
		*pool_id = 19;
		*size = 64 * MV_PP3_MSGR_BUF_NUM;
		return 0;
	}
	return -1;
}

/* get channel HMAC SW parameters (free frame & queue & interrupt group)
Inputs:
	chan_num - channel ID
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_chan_sw_params_get(int chan_num, int *frame, int *queue, int *group, int *irq_off)
{
	if ((frame == NULL) || (queue == NULL) || (group == NULL) || (irq_off == NULL))
		return -1;

	*frame = 2;
	*queue = chan_num;
	*group = 2 + chan_num % 2;
	*irq_off = (chan_num % 2) ? 19 : 18;

	return 0;
}

/* get channel QM HW q number, messenger BM pool ID and buffer headroom
Inputs:
	chan_num - channel ID
Outputs:
	hwq_rx	- RX QM queue number
	hwq_tx	- TX QM queue number
	pool_id	- BM pool ID
	b_hr	- buffer header
*/
static inline int mv_pp3_cfg_chan_hw_params_get(int chan_num, unsigned short *hwq_rx, unsigned char *hwq_tx,
						unsigned char *pool_id, unsigned char *b_hr)
{
	if ((hwq_rx == NULL) || (hwq_tx == NULL) || (pool_id == NULL) || (b_hr == NULL))
		return -1;

	*hwq_rx = 368 + chan_num / 2;
	*hwq_tx = 16 + chan_num / 2;
	*pool_id = 19;
	*b_hr = 32;

	return 0;
}

/* get data path frames bitmap per cpu */
static inline int mv_pp3_cfg_dp_frames_bm(int cpu, int *frames_bmp)
{
	if (frames_bmp == NULL)
		return -1;

	*frames_bmp = (cpu == 0) ? 1 : 2;
	return 0;
}

/* get Linux buffers pool id per cpu */
static inline int mv_pp3_cfg_dp_linux_bpid(int cpu)
{
	return (cpu == 0) ? 14 : 20;
}

/* get long buffers pool id per EMAC */
static inline int mv_pp3_cfg_dp_long_bpid(int emac)
{
	switch (emac) {
	case 0:
		return 8;
	case 1:
		return 9;
	case 2:
		return 10;
	case 3:
		return 11;
	default:
		return -1;
	}
}

/* get short buffers pool id per EMAC */
static inline int mv_pp3_cfg_dp_short_bpid(int emac)
{
	return 12;
}

/* get LRO buffers pool id per EMAC */
static inline int mv_pp3_cfg_dp_lro_bpid(int emac)
{
	return 13;
}

/* get NSS mode buffer pools ids */
static inline int mv_pp3_cfg_nss_pools(int *long_p, int *short_p, int *lro_p, int *app_p)
{
	if ((long_p == NULL) || (short_p == NULL) || (lro_p == NULL) || (app_p == NULL))
		return -1;

	*long_p = 15;
	*short_p = 16;
	*lro_p = 17;
	*app_p = 20;

	return 0;
}

/* get frame and queue number in order to manage bm pools per cpu
Inputs:
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	queue	- HMAC queue number
	size	- max number of CFH messages in HMAC queue
	group	- queue interrupt group
	irq_off	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_dp_bmq_params_get(int cpu, int *frame, int *queue, int *size, int *group, int *irq_off)
{
	if ((frame == NULL) || (queue == NULL) || (size == NULL) || (group == NULL) || (irq_off == NULL))
		return -1;

	switch (cpu) {
	case 0:
		*frame = 2;
		*queue = 8;
		*size = MV_PP3_NUM_MSG_IN_CHAN;
		*group = 4;
		*irq_off = 20;
	break;
	case 1:
		*frame = 2;
		*queue = 9;
		*size = MV_PP3_NUM_MSG_IN_CHAN;
		*group = 5;
		*irq_off = 21;
	break;
	default:
		return -1;
	}

	return 0;
}

/* get data path queue SW parameters
Inputs:
	emac	- emac number
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	qbm	- HMAC queues bitmap; each emac use 2 queues for high/low priority traffic
	group	- queue interrupt group
	irq	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_dp_sw_params_get(int emac, int cpu, int *frame, int *qbm, int *group, int *irq)
{
	if ((frame == NULL) || (qbm == NULL) || (group == NULL) || (irq == NULL) || (emac > 3))
		return -1;

	*group = 0;
	*frame = (cpu == 0) ? 0 : 1;
	*irq = (cpu == 0) ? 128 : 136;
	*qbm = (3 << (emac * 2));

	return 0;
}

/* get data path queue HW parameters
Inputs:
	emac	- emac number
Outputs:
	hwq_rx	- RX QM first queue number
	hwq_tx	- TX QM first queue number
	emacs	- QM first queue number used by EMAC
*/
static inline int mv_pp3_cfg_dp_hw_params_get(int emac, int *hwq_rx, int *hwq_tx, int *emacq)
{
	if ((hwq_rx == NULL) || (hwq_tx == NULL) || (emacq == NULL) || (emac > 3))
		return -1;

	*hwq_rx = 304 + (emac * 8);
	*hwq_tx = (3 << (emac * 2));
	*emacq = 192 + (emac * 16);

	return 0;
}

/* get data path queue SW parameters for NSS mode
Inputs:
	cpu	- cpu number
Outputs:
	frame	- HMAC frame number
	first_q	- first HMAC queue number in queue groups
	group	- queue interrupt group
	irq	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_dp_nss_sw_params_get(int cpu, int *frame, int *first_q, int *group, int *irq)
{
	if ((frame == NULL) || (first_q == NULL) || (group == NULL) || (irq == NULL))
		return -1;

	*group = 1;
	*frame = (cpu == 0) ? 0 : 1;
	*irq = (cpu == 0) ? 1 : 9;
	*first_q = 8;

	return 0;
}

/* get data path queue HW parameters for NSS mode
Inputs:
	cpu	- cpu number
Outputs:
	first_rxq	- RX QM first queue number
	first_txq	- TX QM first queue number
*/
static inline int mv_pp3_cfg_dp_nss_hw_params_get(int cpu, int *first_rxq, int *first_txq)
{
	if ((first_rxq == NULL) || (first_txq == NULL))
		return -1;

	*first_rxq = 336;
	*first_txq = 8;

	return 0;
}

/* get data path RX queue parameters for NIC mode
Inputs:
	emac	- emac number
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	sw_rxq	- HMAC RX queue number
	hw_rxq	- QM RX queue number
*/
static inline int mv_pp3_cfg_dp_nic_rxq_params_get(int emac, int cpu, int *frame, int *sw_rxq, int *hw_rxq)
{
	if ((frame == NULL) || (sw_rxq == NULL) || (hw_rxq == NULL) || (emac > 3))
		return -1;

	*frame = (cpu == 0) ? 0 : 1;
	*sw_rxq = 0 + (emac * 2);
	*hw_rxq = 304 + (emac * 8);

	return 0;
}

/* get data path TX queue parameters for NIC mode
Inputs:
	emac	- emac number
	cpu	- CPU number
Outputs:
	frame	- HMAC frame number
	sw_txq	- HMAC TX queue number
	hw_txq	- QM TX queue number
*/
static inline int mv_pp3_cfg_dp_nic_txq_params_get(int emac, int cpu, int *frame, int *sw_txq, int *hw_txq)
{
	if ((frame == NULL) || (sw_txq == NULL) || (hw_txq == NULL) || (emac > 3))
		return -1;

	*frame = (cpu == 0) ? 0 : 1;
	*sw_txq = 0 + (emac * 2);
	*hw_txq = 0 + (emac * 2);

	return 0;
}

/* get data path queue interrupts parameters for NIC mode
Inputs:
	cpu	- cpu number
Outputs:
	group	- queue interrupt group
	irq	- IRQ offset to connect queue ISR
*/
static inline int mv_pp3_cfg_dp_nic_inter_params_get(int cpu, int *group, int *irq)
{
	if ((group == NULL) || (irq == NULL))
		return -1;

	*group = 1;
	*irq = (cpu == 0) ? 0 : 8;

	return 0;
}

#endif /* __mvPp3Config_h__ */
