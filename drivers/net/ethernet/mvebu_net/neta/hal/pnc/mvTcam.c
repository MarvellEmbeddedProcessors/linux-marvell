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

#include "mvOs.h"
#include "mvCommon.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#include "mvPnc.h"
#include "mvTcam.h"
#include "gbe/mvNetaRegs.h"

#define DWORD_LEN       32

#define TCAM_DBG(x...) if (tcam_ctl_flags & TCAM_F_DEBUG) mvOsPrintf(x)
/*#define TCAM_DBG(x...)*/


/*
 * SW control flags
 */
static int tcam_ctl_flags;
#define TCAM_F_DEBUG	0x1
#define TCAM_F_WRITE	0x2

/*
 * Keep short text per entry
 */
char tcam_text[CONFIG_MV_PNC_TCAM_LINES][TCAM_TEXT];

MV_U8 *mvPncVirtBase = NULL;

MV_STATUS mvPncInit(MV_U8 *pncVirtBase)
{
	mvPncVirtBase = pncVirtBase;

	mvOsPrintf("mvPncVirtBase = 0x%p\n", pncVirtBase);
	return MV_OK;
}

/*
 * Low-Level API: TCAM
 */
void tcam_sw_clear(struct tcam_entry *te)
{
	memset(te, 0, sizeof(struct tcam_entry));
}

/*
 *@ainfo : match to `1` on additional info bits 6..0
 */
void tcam_sw_set_ainfo(struct tcam_entry *te, unsigned int bits, unsigned int mask)
{
	int i;
	MV_U32 key = te->data.u.word[AI_WORD];
	MV_U32 val;

	WARN_ON_OOR(bits > AI_MASK);
	WARN_ON_OOR(mask > AI_MASK);

	for (i = 0; i < AI_BITS; i++) {
		if (mask & (1 << i)) {

			val = 1 << (i + AI_OFFS);

			if (bits & (1 << i))
				key |= val;
			else
				key &= ~val;
		}
	}
	te->data.u.word[AI_WORD] = key;
	te->mask.u.word[AI_WORD] |= mask << AI_OFFS;
}

static int tcam_sw_dump_ainfo(struct tcam_entry *te, char *buf)
{
	int i, data, mask;
	int off = 0;

	mask = ((te->mask.u.word[AI_WORD] >> AI_OFFS) & AI_MASK);
	if (mask == 0)
		return off;

	data = ((te->data.u.word[AI_WORD] >> AI_OFFS) & AI_MASK);
	off += mvOsSPrintf(buf + off, " AI=");
	for (i = 0; i < AI_BITS; i++)
		if (mask & (1 << i))
			off += mvOsSPrintf(buf + off, "%d",
							((data & (1 << i)) != 0));
		else
			off += mvOsSPrintf(buf + off, "x");

	return off;
}

/*
 *@port : port
 */
void tcam_sw_set_port(struct tcam_entry *te, unsigned int port, unsigned int mask)
{
	WARN_ON_OOR(port > PORT_MASK);
	WARN_ON_OOR(mask > PORT_MASK);

	te->data.u.word[PORT_WORD] &= ~(PORT_MASK << PORT_OFFS);
	te->mask.u.word[PORT_WORD] &= ~(PORT_MASK << PORT_OFFS);

	te->data.u.word[PORT_WORD] |= port << PORT_OFFS;
	te->mask.u.word[PORT_WORD] |= mask << PORT_OFFS;
}

void tcam_sw_get_port(struct tcam_entry *te, unsigned int *port, unsigned int *mask)
{
	*port = (te->data.u.word[PORT_WORD] >> PORT_OFFS) & PORT_MASK;
	*mask = (te->mask.u.word[PORT_WORD] >> PORT_OFFS) & PORT_MASK;
}

void tcam_sw_set_lookup(struct tcam_entry *te, unsigned int lookup)
{
	WARN_ON_OOR(lookup > LU_MASK);
	te->data.u.word[PORT_WORD] &= ~(LU_MASK << LU_OFFS);
	te->data.u.word[PORT_WORD] |= lookup << LU_OFFS;
	te->mask.u.word[PORT_WORD] |= LU_MASK << LU_OFFS;
}

void tcam_sw_set_lookup_all(struct tcam_entry *te)
{
	te->data.u.word[PORT_WORD] &= ~(LU_MASK << LU_OFFS);
	te->mask.u.word[PORT_WORD] &= ~(LU_MASK << LU_OFFS);
}

void tcam_sw_get_lookup(struct tcam_entry *te, unsigned int *lookup, unsigned int *mask)
{
	*lookup = (te->data.u.word[PORT_WORD] >> LU_OFFS) & LU_MASK;
	*mask = (te->mask.u.word[PORT_WORD] >> LU_OFFS) & LU_MASK;
}

/* offset:23..0 */
void tcam_sw_set_byte(struct tcam_entry *te, unsigned int offset, unsigned char data)
{
	WARN_ON_OOR(offset >= ((TCAM_LEN - 1) * 4));

	te->data.u.byte[offset] = data;
	te->mask.u.byte[offset] = 0xFF;
}

void tcam_sw_set_mask(struct tcam_entry *te, unsigned int offset, unsigned char mask)
{
	WARN_ON_OOR(offset >= ((TCAM_LEN - 1) * 4));

	te->mask.u.byte[offset] = mask;
}

int tcam_sw_cmp_byte(struct tcam_entry *te, unsigned int offset, unsigned char data)
{
	unsigned char mask;

	ERR_ON_OOR(offset >= ((TCAM_LEN - 1) * 4));

	mask = te->mask.u.byte[offset];

	if ((te->data.u.byte[offset] & mask) == (data & mask))
		return 0;

	return 1;
}

int tcam_sw_cmp_bytes(struct tcam_entry *te, unsigned int offset, unsigned int size, unsigned char *data)
{
	int i;

	ERR_ON_OOR((offset + size) >= ((TCAM_LEN - 1) * 4));

	for (i = 0; i < size; i++) {
		if (tcam_sw_cmp_byte(te, offset + i, data[i]))
			return 1;
	}
	return 0;
}

/*
 * Low-Level API: SRAM
 */
void sram_sw_set_flowid(struct tcam_entry *te, unsigned int flowid,
			unsigned int mask)
{
	unsigned int i;

	WARN_ON_OOR(mask > FLOW_CTRL_MASK);

	for (i = 0; i < FLOW_CTRL_BITS; i++) {
		if (mask & (1 << i)) {
			te->sram.word[0] &= ~(FLOW_PART_MASK << (i * FLOW_PART_BITS));
			te->sram.word[0] |= flowid & (FLOW_PART_MASK << (i * FLOW_PART_BITS));
			te->sram.word[1] |= 1 << i;
		}
	}
}

void sram_sw_set_flowid_partial(struct tcam_entry *te, unsigned int flowid,
				unsigned int idx)
{
	WARN_ON_OOR(idx >= FLOW_CTRL_BITS);
	WARN_ON_OOR(flowid > FLOW_PART_MASK);

	te->sram.word[0] &= ~(FLOW_PART_MASK << (idx * FLOW_PART_BITS));
	te->sram.word[0] |= (flowid << (idx * FLOW_PART_BITS));
	te->sram.word[1] |= 1 << idx;
}

void sram_sw_set_rinfo(struct tcam_entry *te, unsigned int rinfo, unsigned int mask)
{
	unsigned int word;
	unsigned int i;

	WARN_ON_OOR(rinfo > RI_MASK);

	for (i = 0; i < RI_BITS; i++) {
		if (mask & (1 << i)) {

			word = (RI_VALUE_OFFS + i) / DWORD_LEN;
			if (rinfo & (1 << i))
				te->sram.word[word] |= (1 << ((i + RI_VALUE_OFFS) % DWORD_LEN));
			else
				te->sram.word[word] &= ~(1 << ((i + RI_VALUE_OFFS) % DWORD_LEN));

			word = (RI_MASK_OFFS + i) / DWORD_LEN;
			te->sram.word[word] |= (1 << ((i + RI_MASK_OFFS) % DWORD_LEN));
		}
	}
}

#ifdef MV_ETH_PNC_NEW

#ifdef MV_ETH_PNC_LB
void sram_sw_set_load_balance(struct tcam_entry *te, unsigned int value)
{
	unsigned int word;

	WARN_ON_OOR(value > LB_QUEUE_MASK);

	word = LB_QUEUE_OFFS / DWORD_LEN;
	te->sram.word[word] &= ~(LB_QUEUE_MASK << (LB_QUEUE_OFFS % DWORD_LEN));
	te->sram.word[word] |= value << (LB_QUEUE_OFFS % DWORD_LEN);
}

static int sram_sw_dump_load_balance(struct tcam_entry *te, char *buf)
{
	unsigned int word, value;

	word = LB_QUEUE_OFFS / DWORD_LEN;
	value = te->sram.word[word] >> (LB_QUEUE_OFFS % DWORD_LEN);
	value &= LB_QUEUE_MASK;

	if (value)
		return mvOsSPrintf(buf, " LB=%d", value);

	return 0;
}
#endif /* MV_ETH_PNC_LB */

void sram_sw_set_rinfo_extra(struct tcam_entry *te, unsigned int ri_extra)
{
	unsigned int word, value;
	unsigned int i, c;

	WARN_ON_OOR(ri_extra > RI_EXTRA_MASK);

	for (c = i = 0; i < RI_EXTRA_BITS; i += 2, c++)	{
		value = ((ri_extra >> i) & 3);
		if (value) {
			word = (RI_EXTRA_VALUE_OFFS + i) / DWORD_LEN;
			te->sram.word[word] &= ~(3 << ((i + RI_EXTRA_VALUE_OFFS) % DWORD_LEN));
			te->sram.word[word] |= value << ((i + RI_EXTRA_VALUE_OFFS) % DWORD_LEN);

			word = (RI_EXTRA_CTRL_OFFS + c) / DWORD_LEN;
			te->sram.word[word] |= 1 << ((c + RI_EXTRA_CTRL_OFFS) % DWORD_LEN);
		}
	}
}
#endif /* MV_ETH_PNC_NEW */

static int sram_sw_dump_rinfo(struct tcam_entry *te, char *buf)
{
	unsigned int word, shift, rinfo;
	int i, off = 0;

	word = RI_VALUE_OFFS / DWORD_LEN;
	shift = RI_VALUE_OFFS % DWORD_LEN;
	rinfo = (te->sram.word[word] >> shift) & ((1 << RI_BITS) - 1);

	for (i = 0; i < RI_BITS; i++)
		if (rinfo & (1 << i))
			off += mvOsSPrintf(buf + off, " R%d", i);

	return off;
}

void sram_sw_set_shift_update(struct tcam_entry *te, unsigned int index, unsigned int value)
{
	unsigned int word;

	WARN_ON_OOR(index > SHIFT_IDX_MASK);	/* 0x7  */
	WARN_ON_OOR(value > SHIFT_VAL_MASK);	/* 0x7F */

	/* Reset value prior to set new one */
	word = SHIFT_IDX_OFFS / DWORD_LEN;
	te->sram.word[word] &= ~(SHIFT_IDX_MASK << (SHIFT_IDX_OFFS % DWORD_LEN));
	te->sram.word[word] |= index << (SHIFT_IDX_OFFS % DWORD_LEN);

	word = SHIFT_VAL_OFFS / DWORD_LEN;
	te->sram.word[word] &= ~(SHIFT_VAL_MASK << (SHIFT_VAL_OFFS % DWORD_LEN));
	te->sram.word[word] |= value << (SHIFT_VAL_OFFS % DWORD_LEN);

	TCAM_DBG("%s: w=%x i=0x%x v=0x%x\n", __func__, word, index, value);
}

static int sram_sw_dump_shift_update(struct tcam_entry *te, char *buf)
{
	unsigned int word;
	unsigned int index;
	unsigned int value;

	word = SHIFT_VAL_OFFS / DWORD_LEN;
	value = te->sram.word[word] >> (SHIFT_VAL_OFFS % DWORD_LEN);
	value &= SHIFT_VAL_MASK;

	word = SHIFT_IDX_OFFS / DWORD_LEN;
	index = te->sram.word[word] >> (SHIFT_IDX_OFFS % DWORD_LEN);
	index &= SHIFT_IDX_MASK;

	if (value)
		return mvOsSPrintf(buf, " [%d]=%d", index, value);

	return 0;
}

/* rxq:95..93 info:92 */
void sram_sw_set_rxq(struct tcam_entry *te, unsigned int rxq, unsigned int force)
{
	unsigned int word;

	WARN_ON_OOR(rxq > RXQ_MASK);

	if (force) {
		word = RXQ_INFO_OFFS / DWORD_LEN;
		te->sram.word[word] |= 1 << (RXQ_INFO_OFFS % DWORD_LEN);
	}

	word = RXQ_QUEUE_OFFS / DWORD_LEN;
	te->sram.word[word] &= ~(RXQ_MASK << (RXQ_QUEUE_OFFS % DWORD_LEN));
	te->sram.word[word] |= rxq << (RXQ_QUEUE_OFFS % DWORD_LEN);
}

unsigned int sram_sw_get_rxq(struct tcam_entry *te, unsigned int *force)
{
	unsigned int word;
	unsigned int rxq;

	word = RXQ_INFO_OFFS / DWORD_LEN;
	if (force)
		*force = te->sram.word[word] & (1 << (RXQ_INFO_OFFS % DWORD_LEN));

	word = RXQ_QUEUE_OFFS / DWORD_LEN;
	rxq = te->sram.word[word] >> (RXQ_QUEUE_OFFS % DWORD_LEN);
	rxq &= RXQ_MASK;

	return rxq;
}

static int sram_sw_dump_rxq(struct tcam_entry *te, char *buf)
{
	unsigned int rxq, force;

	rxq = sram_sw_get_rxq(te, &force);
	if (rxq)
		return mvOsSPrintf(buf, " %sQ%d", force ? "f" : "", rxq);

	return 0;
}

/* index */
void sram_sw_set_next_lookup_shift(struct tcam_entry *te, unsigned int index)
{
	unsigned int word;

	WARN_ON_OOR(index > SHIFT_IDX_MASK);

	word = NEXT_LU_SHIFT_OFFS / DWORD_LEN;
	te->sram.word[word] |= index << (NEXT_LU_SHIFT_OFFS % DWORD_LEN);
}

static int sram_sw_dump_next_lookup_shift(struct tcam_entry *te, char *buf)
{
	unsigned int word, value;

	word = NEXT_LU_SHIFT_OFFS / DWORD_LEN;
	value = te->sram.word[word] >> (NEXT_LU_SHIFT_OFFS % DWORD_LEN);
	value &= SHIFT_IDX_MASK;

	if (value)
		return mvOsSPrintf(buf, " SH=%d", value);

	return 0;
}

/* done */
void sram_sw_set_lookup_done(struct tcam_entry *te, unsigned int value)
{
	unsigned int word;

	word = LU_DONE_OFFS / DWORD_LEN;
	if (value)
		te->sram.word[word] |= 1 << (LU_DONE_OFFS % DWORD_LEN);
	else
		te->sram.word[word] &= ~(1 << (LU_DONE_OFFS % DWORD_LEN));
}

/* index:91..89 val:88..82 */
void sram_sw_set_ainfo(struct tcam_entry *te, unsigned int bits, unsigned int mask)
{
	unsigned int word;
	unsigned int i;

	WARN_ON_OOR(bits > AI_MASK);
	WARN_ON_OOR(mask > AI_MASK);

	for (i = 0; i < AI_BITS; i++)
		if (mask & (1 << i)) {
			word = (AI_VALUE_OFFS + i) / DWORD_LEN;
			if (bits & (1 << i))
				te->sram.word[word] |= (1 << ((i + AI_VALUE_OFFS) % DWORD_LEN));
			else
				te->sram.word[word] &= ~(1 << ((i + AI_VALUE_OFFS) % DWORD_LEN));

			word = (AI_MASK_OFFS + i) / DWORD_LEN;
			te->sram.word[word] |= 1 << ((i + AI_MASK_OFFS) % DWORD_LEN);
		}
}

static int sram_sw_dump_ainfo(struct tcam_entry *te, char *buf)
{
	unsigned int word, shift, data, mask;
	int i, off = 0;

	word = AI_VALUE_OFFS / DWORD_LEN;
	shift = AI_VALUE_OFFS % DWORD_LEN;
	data = ((te->sram.word[word] >> shift) & AI_MASK);
	shift = AI_MASK_OFFS % DWORD_LEN;
	mask = ((te->sram.word[word] >> shift) & AI_MASK);

	if (mask) {
		off += mvOsSPrintf(buf + off, " AI=");
		for (i = 0; i < AI_BITS; i++) {
			if (mask & (1 << i))
				off += mvOsSPrintf(buf + off, "%d", ((data & (1 << i)) != 0));
			else
				off += mvOsSPrintf(buf + off, "x");
		}
	}
	return off;
}

/* 121..118 */
void sram_sw_set_next_lookup(struct tcam_entry *te, unsigned int lookup)
{
	unsigned int word;

	WARN_ON_OOR(lookup > LU_MASK);

	word = LU_ID_OFFS / DWORD_LEN;
	te->sram.word[word] |= lookup << (LU_ID_OFFS % DWORD_LEN);
}
static int sram_sw_dump_next_lookup(struct tcam_entry *te, char *buf)
{
	unsigned int word;
	unsigned int lookup;

	word = LU_DONE_OFFS / DWORD_LEN;
	lookup = te->sram.word[word] >> (LU_DONE_OFFS % DWORD_LEN);
	lookup &= 0x1;

	if (lookup)
		return mvOsSPrintf(buf, " LU=D");

	word = LU_ID_OFFS / DWORD_LEN;
	lookup = te->sram.word[word] >> (LU_ID_OFFS % DWORD_LEN);
	lookup &= LU_MASK;

	if (lookup)
		return mvOsSPrintf(buf, " LU=%d", lookup);

	return 0;
}

/*
 * tcam_sw_alloc - allocate new TCAM entry
 * @lookup: lookup section
 */
struct tcam_entry *tcam_sw_alloc(unsigned int lookup)
{
	struct tcam_entry *te = mvOsMalloc(sizeof(struct tcam_entry));

	WARN_ON_OOM(!te);

	tcam_sw_clear(te);
	tcam_sw_set_lookup(te, lookup);
	sram_sw_set_shift_update(te, 7, 0);

	return te;
}

void tcam_sw_free(struct tcam_entry *te)
{
	mvOsFree(te);
}

void tcam_sw_text(struct tcam_entry *te, char *text)
{
	strncpy(te->ctrl.text, text, TCAM_TEXT);
	te->ctrl.text[TCAM_TEXT - 1] = 0;
}

int tcam_sw_dump(struct tcam_entry *te, char *buf)
{
	unsigned int *word;
	unsigned int off = 0;
	MV_U32       w32;
	int			 i;

	/* hw entry id */
	off += mvOsSPrintf(buf + off, "[%4d] ", te->ctrl.index);

	word = (unsigned int *)&te->data;
	i = TCAM_LEN - 1;
	off += mvOsSPrintf(buf+off, "%4.4x ", word[i--] & 0xFFFF);

	while (i >= 0) {
		w32 = word[i--];
		off += mvOsSPrintf(buf+off, "%8.8x ", MV_32BIT_LE_FAST(w32));
	}
	off += mvOsSPrintf(buf+off, "| ");

	word = (unsigned int *)&te->sram;
	off += mvOsSPrintf(buf+off, SRAM_FMT, SRAM_VAL(word));

	off += sram_sw_dump_next_lookup(te, buf + off);
	off += sram_sw_dump_next_lookup_shift(te, buf + off);
	off += sram_sw_dump_rinfo(te, buf + off);
	off += sram_sw_dump_ainfo(te, buf + off);
	off += sram_sw_dump_shift_update(te, buf + off);
	off += sram_sw_dump_rxq(te, buf + off);

#ifdef MV_ETH_PNC_LB
	off += sram_sw_dump_load_balance(te, buf + off);
#endif /* MV_ETH_PNC_LB */

	off += (te->ctrl.flags & TCAM_F_INV) ? mvOsSPrintf(buf + off, " [inv]") : 0;
	off += mvOsSPrintf(buf + off, "\n       ");

	word = (unsigned int *)&te->mask;
	i = TCAM_LEN - 1;
	off += mvOsSPrintf(buf+off, "%4.4x ", word[i--] & 0xFFFF);

	while (i >= 0) {
		w32 = word[i--];
		off += mvOsSPrintf(buf+off, "%8.8x ", MV_32BIT_LE_FAST(w32));
	}

	off += mvOsSPrintf(buf + off, "   (%s)", te->ctrl.text);
	off += tcam_sw_dump_ainfo(te, buf + off);
	off += mvOsSPrintf(buf + off, "\n");

	return off;
}

/*
 * tcam_hw_inv - invalidate TCAM entry on HW
 * @tid: entry index
 */
void tcam_hw_inv(int tid)
{
	MV_U32 va;

	WARN_ON_OOR(tid >= CONFIG_MV_PNC_TCAM_LINES);
	va = (MV_U32) mvPncVirtBase;
	va |= PNC_TCAM_ACCESS_MASK;
	va |= (tid << TCAM_LINE_INDEX_OFFS);
	va |= (0xd << TCAM_WORD_ENTRY_OFFS);

	MV_MEMIO_LE32_WRITE(va, 1);
	TCAM_DBG("%s: (inv) 0x%8x <-- 0x%x [%2x]\n", __func__, va, 1, tid);
}

void tcam_hw_inv_all(void)
{
	MV_U32 va;
	int tid = CONFIG_MV_PNC_TCAM_LINES;

	while (tid--) {
		va = (MV_U32) mvPncVirtBase;
		va |= PNC_TCAM_ACCESS_MASK;
		va |= (tid << TCAM_LINE_INDEX_OFFS);
		va |= (0xd << TCAM_WORD_ENTRY_OFFS);

		MV_MEMIO_LE32_WRITE(va, 1);
		TCAM_DBG("%s: (inv) 0x%8x <-- 0x%x [%2x]\n", __func__, va, 1, tid);
	}
}

/*
 * tcam_hw_write - install TCAM entry on HW
 * @tid: entry index
 */
int tcam_hw_write(struct tcam_entry *te, int tid)
{
	MV_U32 i, va, w32;

	TCAM_DBG("%s: tid=0x%x\n", __func__, tid);
	ERR_ON_OOR(tid >= CONFIG_MV_PNC_TCAM_LINES);

	/* sram */
	for (i = 0; i < SRAM_LEN; i++) {
		w32 = te->sram.word[i];
		/* last word triggers hardware */
		if (tcam_ctl_flags & TCAM_F_WRITE || w32 || (i == (SRAM_LEN - 1))) {
			va = (MV_U32) mvPncVirtBase;
			va |= PNC_SRAM_ACCESS_MASK;
			va |= (tid << TCAM_LINE_INDEX_OFFS);
			va |= (i << TCAM_WORD_ENTRY_OFFS);
			MV_MEMIO_LE32_WRITE(va, w32);
			TCAM_DBG("%s: (sram) 0x%8x <-- 0x%x\n", __func__, va, w32);
		}
	}

	/* tcam */
	for (i = 0; i < (TCAM_LEN - 1); i++) {
		w32 = te->data.u.word[i];

		if (tcam_ctl_flags & TCAM_F_WRITE || w32) {
			va = (MV_U32) mvPncVirtBase;
			va |= PNC_TCAM_ACCESS_MASK;
			va |= (tid << TCAM_LINE_INDEX_OFFS);
			va |= ((2 * i) << TCAM_WORD_ENTRY_OFFS);

			MV_MEMIO32_WRITE(va, w32);
			TCAM_DBG("%s: (tcam data) 0x%08x <-- 0x%08x\n", __func__, va, w32);
		}
	}

	/* mask */
	for (i = 0; i < (TCAM_LEN - 1); i++) {
		w32 = te->mask.u.word[i];

		if (tcam_ctl_flags & TCAM_F_WRITE || w32) {
			va = (MV_U32) mvPncVirtBase;
			va |= PNC_TCAM_ACCESS_MASK;
			va |= (tid << TCAM_LINE_INDEX_OFFS);
			va |= ((2 * i + 1) << TCAM_WORD_ENTRY_OFFS);

			MV_MEMIO32_WRITE(va, w32);
			TCAM_DBG("%s: (tcam mask) 0x%08x <-- 0x%08x\n", __func__, va, w32);
		}
	}

	va = (MV_U32) mvPncVirtBase;
	va |= PNC_TCAM_ACCESS_MASK;
	va |= (tid << TCAM_LINE_INDEX_OFFS);
	va |= (0xc << TCAM_WORD_ENTRY_OFFS);

	w32 = te->data.u.word[TCAM_LEN - 1] & 0xFFFF;
	w32 |= (te->mask.u.word[TCAM_LEN - 1] << 16);

	MV_MEMIO_LE32_WRITE(va, w32);
	TCAM_DBG("%s: (last) 0x%8x <-- 0x%x\n", __func__, va, w32);

	/* FIXME: perf hit */
	if (te->ctrl.text[0]) {
		TCAM_DBG("%s: (text) <-- %s\n", __func__, te->ctrl.text);
		strncpy(tcam_text[tid], te->ctrl.text, TCAM_TEXT);
		tcam_text[tid][TCAM_TEXT - 1] = 0;
	}

	return 0;
}

/*
 * tcam_hw_read - load TCAM entry from HW
 * @tid: entry index
 */
int tcam_hw_read(struct tcam_entry *te, int tid)
{
	MV_U32 i, va, w32;

	TCAM_DBG("%s: tid=0x%x\n", __func__, tid);
	ERR_ON_OOR(tid >= CONFIG_MV_PNC_TCAM_LINES);

	te->ctrl.index = tid;

	/* sram */
	for (i = 0; i < SRAM_LEN; i++) {
		va = (MV_U32) mvPncVirtBase;
		va |= PNC_SRAM_ACCESS_MASK;
		va |= (tid << TCAM_LINE_INDEX_OFFS);
		va |= (i << TCAM_WORD_ENTRY_OFFS);

		te->sram.word[i] = w32 = MV_MEMIO_LE32_READ(va);
		TCAM_DBG("%s: (sram) 0x%8x --> 0x%x\n", __func__, va, w32);
	}

	/* tcam */
	for (i = 0; i < (TCAM_LEN - 1); i++) {
		va = (MV_U32) mvPncVirtBase;
		va |= PNC_TCAM_ACCESS_MASK;
		va |= (tid << TCAM_LINE_INDEX_OFFS);
		va |= ((2 * i) << TCAM_WORD_ENTRY_OFFS);

		te->data.u.word[i] = w32 = MV_MEMIO32_READ(va);
		TCAM_DBG("%s: (tcam data) 0x%8x --> 0x%x\n", __func__, va, w32);
	}

	/* mask */
	for (i = 0; i < (TCAM_LEN - 1); i++) {
		va = (MV_U32) mvPncVirtBase;
		va |= PNC_TCAM_ACCESS_MASK;
		va |= (tid << TCAM_LINE_INDEX_OFFS);
		va |= ((2 * i + 1) << TCAM_WORD_ENTRY_OFFS);

		te->mask.u.word[i] = w32 = MV_MEMIO32_READ(va);
		TCAM_DBG("%s: (tcam mask) 0x%8x --> 0x%x\n", __func__, va, w32);
	}

	va = (MV_U32) mvPncVirtBase;
	va |= PNC_TCAM_ACCESS_MASK;
	va |= (tid << TCAM_LINE_INDEX_OFFS);
	va |= (0xc << TCAM_WORD_ENTRY_OFFS);

	w32 = MV_MEMIO_LE32_READ(va);
	te->data.u.word[TCAM_LEN - 1] = w32 & 0xFFFF;
	te->mask.u.word[TCAM_LEN - 1] = w32 >> 16;
	TCAM_DBG("%s: (last) 0x%8x --> 0x%x\n", __func__, va, w32);

	va = (MV_U32) mvPncVirtBase;
	va |= PNC_TCAM_ACCESS_MASK;
	va |= (tid << TCAM_LINE_INDEX_OFFS);
	va |= (0xd << TCAM_WORD_ENTRY_OFFS);

	w32 = MV_MEMIO_LE32_READ(va);
	te->ctrl.flags = w32 & TCAM_F_INV;
	TCAM_DBG("%s: (inv) 0x%8x --> 0x%x\n", __func__, va, w32);

	/* text */
	TCAM_DBG("%s: (text) --> %s\n", __func__, tcam_text[tid]);
	strncpy(te->ctrl.text, tcam_text[tid], TCAM_TEXT);
	te->ctrl.text[TCAM_TEXT - 1] = 0;

	return 0;
}

/*
 * tcam_hw_record - record enable
 */
void tcam_hw_record(int port)
{
	TCAM_DBG("%s: port %d 0x%x <-- 1\n", __func__, port, MV_PNC_HIT_SEQ0_REG);
	MV_REG_WRITE(MV_PNC_HIT_SEQ0_REG, (port << 1) | 1);
}

/*
 * tcam_hw_hits - dump hit sequence
 */
int tcam_hw_hits(char *buf)
{
	MV_U32 i, off = 0;

	off += mvOsSPrintf(buf + off, "seq hit\n");
	off += mvOsSPrintf(buf + off, "--- ---\n");

	i = MV_REG_READ(MV_PNC_HIT_SEQ0_REG);
	off += mvOsSPrintf(buf + off, "0 - %d\n", (i >> 10) & 0x3FF);
	off += mvOsSPrintf(buf + off, "1 - %d\n", (i >> 20) & 0x3FF);

	i = MV_REG_READ(MV_PNC_HIT_SEQ1_REG);
	off += mvOsSPrintf(buf + off, "2 - %d\n", (i >> 0) & 0x3FF);
	off += mvOsSPrintf(buf + off, "3 - %d\n", (i >> 10) & 0x3FF);
	off += mvOsSPrintf(buf + off, "4 - %d\n", (i >> 20) & 0x3FF);

	i = MV_REG_READ(MV_PNC_HIT_SEQ2_REG);
	off += mvOsSPrintf(buf + off, "5 - %d\n", (i >> 0) & 0x3FF);
	off += mvOsSPrintf(buf + off, "6 - %d\n", (i >> 10) & 0x3FF);
	off += mvOsSPrintf(buf + off, "7 - %d\n", (i >> 20) & 0x3FF);

	return off;
}

void tcam_hw_debug(int en)
{
	tcam_ctl_flags = en;
}

/*
 * tcam_hw_dump - print out TCAM registers
 * @all - whether to dump all entries or valid only
 */
int tcam_hw_dump(int all)
{
	int i;
	struct tcam_entry te;
	char buff[1024];

	for (i = 0; i < CONFIG_MV_PNC_TCAM_LINES; i++) {
		tcam_sw_clear(&te);
		tcam_hw_read(&te, i);
		if (!all && (te.ctrl.flags & TCAM_F_INV))
			continue;
		tcam_sw_dump(&te, buff);
		mvOsPrintf(buff);
	}

	return 0;
}

/******************************************************************************
 *
 * HW Init
 *
 ******************************************************************************
 */
int tcam_hw_init(void)
{
	int i;
	MV_U32	regVal;
	struct tcam_entry te;

#if (CONFIG_MV_PNC_TCAM_LINES > MV_PNC_TCAM_LINES)
#error "CONFIG_MV_PNC_TCAM_LINES must be less or equal than MV_PNC_TCAM_LINES"
#endif

	/* Power on TCAM arrays accordingly with CONFIG_MV_PNC_TCAM_LINES */
	regVal = MV_REG_READ(MV_PNC_TCAM_CTRL_REG);
	for (i = 0; i < (MV_PNC_TCAM_LINES / MV_PNC_TCAM_ARRAY_SIZE); i++) {
		if ((i * MV_PNC_TCAM_ARRAY_SIZE) < CONFIG_MV_PNC_TCAM_LINES)
			regVal |= MV_PNC_TCAM_POWER_UP(i); /* Power ON */
		else
			regVal &= ~MV_PNC_TCAM_POWER_UP(i); /* Power OFF */
	}
	MV_REG_WRITE(MV_PNC_TCAM_CTRL_REG, regVal);

	tcam_sw_clear(&te);
	sram_sw_set_lookup_done(&te, 1);

	/* Perform full write */
	tcam_ctl_flags = TCAM_F_WRITE;

	for (i = 0; i < CONFIG_MV_PNC_TCAM_LINES; i++) {
		sram_sw_set_flowid(&te, i, FLOW_CTRL_MASK);
		tcam_sw_text(&te, "empty");
		tcam_hw_write(&te, i);
		tcam_hw_inv(i);
	}

	/* Back to partial write */
	tcam_ctl_flags = 0;

	return 0;
}

