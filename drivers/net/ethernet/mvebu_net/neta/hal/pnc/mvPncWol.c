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
#include "mv802_3.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#include "gbe/mvNeta.h"

#include "mvPnc.h"
#include "mvTcam.h"

/* PNC debug */
/*#define PNC_DBG mvOsPrintf*/
#define PNC_DBG(X...)

/* PNC errors */
#define PNC_ERR mvOsPrintf
/*#define PNC_ERR(X...)*/

#define MV_PNC_MAX_RULES			128

/* up to 5 tids can be set for one rule */
#define MV_PNC_LOOKUP_MAX				(MV_PNC_TOTAL_DATA_SIZE / MV_PNC_LOOKUP_DATA_SIZE)

typedef struct {

	int		id;
	int		port_mask;
	char	data[MV_PNC_TOTAL_DATA_SIZE];
	char	mask[MV_PNC_TOTAL_DATA_SIZE];
	int		size;
	int		tids[MV_PNC_LOOKUP_MAX];
} MV_PNC_WOL_RULE;

/* Support up to 128 WoL rules */
static MV_PNC_WOL_RULE	*mv_pnc_wol_tbl[MV_PNC_MAX_RULES];

/* Return values:
 * 0..4 - number of exactly match TCAM enries (24 bytes)
 * 5    - totally the same rule,
 * -1   - partial match (not supported)
 */
int mv_pnc_rule_cmp(MV_PNC_WOL_RULE *pNew, MV_PNC_WOL_RULE *pExist)
{
	int		offset, i, lookup, non_equal_byte, non_equal_lookup;
	MV_U8	mask;

	offset = 0;
	lookup = 0;
	non_equal_byte = -1;
	non_equal_lookup = -1;
	while (offset < pNew->size) {

		for (i = 0; i < MV_PNC_LOOKUP_DATA_SIZE; i++) {

			mask = pNew->mask[offset + i] & pExist->mask[offset + i];
			if ((pNew->data[offset + i] & mask) != (pExist->data[offset + i] & mask)) {
				/* Different */
				mvOsPrintf("#%d different on lookup #%d byte #%d: new:%02x & %02x, exist:%02x & %02x\n",
							pExist->id, lookup, offset + i, pNew->data[offset + i], pNew->mask[offset + i],
							pExist->data[offset + i], pExist->mask[offset + i]);
				return lookup;
			}
			if (non_equal_byte == -1) {
				if ((pNew->mask[offset + i] != pExist->mask[offset + i]) ||
					(pNew->data[offset + i] != pExist->data[offset + i])) {
					/* Entries are different in this byte */

					/* Remember lookup where rules are different */
					if (non_equal_lookup == -1)
						non_equal_lookup = lookup;

					if ((pNew->mask[offset + i] != pExist->mask[offset + i]) && (pNew->mask[offset + i] != 0xFF)) {
						/* New rule is superset of the existing rule for this byte */
						non_equal_byte = offset + i;
					}
				}
			}
		}
		offset += MV_PNC_LOOKUP_DATA_SIZE;
		lookup++;
	}
	if (non_equal_byte != -1) {
		mvOsPrintf("#%d non equal on lookup=%d, byte=%d: new %02x & %02x, exist %02x & %02x\n",
				pExist->id, non_equal_lookup, non_equal_byte, pNew->data[non_equal_byte], pNew->mask[non_equal_byte],
				pExist->data[non_equal_byte], pExist->mask[non_equal_byte]);

		if (non_equal_lookup < (lookup - 1)) {
			/* Rejected */
			mvOsPrintf("rejected: non_equal_lookup #%d < last_lookup #%d\n", non_equal_lookup, lookup - 1);
			return -1;
		} else {
			mvOsPrintf("pass: non_equal_lookup #%d == last_lookup #%d\n", non_equal_lookup, lookup - 1);
			return non_equal_lookup;
		}
	}
	if (non_equal_lookup == -1) {
		/* rules are the equal */
		mvOsPrintf("#%d equal - number of lookups=%d\n", pExist->id, lookup);
		return MV_PNC_LOOKUP_MAX;
	} else {
		/* New rule is superset of existing rule */
		mvOsPrintf("#%d is superset on lookup #%d\n", pExist->id, non_equal_lookup);
		return lookup;
	}
}

void mv_pnc_wol_init(void)
{
	struct tcam_entry *te;

	memset(mv_pnc_wol_tbl, 0, sizeof(mv_pnc_wol_tbl));

	/* Set default entires for each one of LU used for WoL */
	te = tcam_sw_alloc(TCAM_LU_WOL);
	tcam_sw_set_lookup_all(te);
	sram_sw_set_rinfo(te, RI_DROP, RI_DROP);
	sram_sw_set_lookup_done(te, 1);
	tcam_sw_text(te, "wol_eof");

	tcam_hw_write(te, TE_WOL_EOF);
	tcam_sw_free(te);
}

/* Add WoL rule to TCAM */
int mv_pnc_wol_rule_set(int port, char *data, char *mask, int size)
{
	int               tid, i, free, lookup, match_lu, offset;
	MV_PNC_WOL_RULE   *pWolRule, *pNewRule, *pMatchRule;

	/* Check parameters validity */
	if (mvNetaPortCheck(port))
		return -1;

	if (mvNetaMaxCheck(size, (MV_PNC_TOTAL_DATA_SIZE + 1), "data_size"))
		return -1;

	/* Save WoL rule in mv_pnc_wol_tbl */
	pNewRule = mvOsMalloc(sizeof(MV_PNC_WOL_RULE));
	if (pNewRule == NULL) {
		mvOsPrintf("%s: port=%d, size=%d - Can't allocate %d bytes\n",
				__func__, port, size, sizeof(sizeof(MV_PNC_WOL_RULE)));
		return -2;
	}
	memset(pNewRule, 0, sizeof(MV_PNC_WOL_RULE));
	pNewRule->port_mask = (1 << port);
	memcpy(pNewRule->data, data, size);
	memcpy(pNewRule->mask, mask, size);

	/* complete with don't care */
	memset(&pNewRule->mask[size], 0, MV_PNC_TOTAL_DATA_SIZE - size);

	/* remember last byte that mask != 0 */
	pNewRule->size = 0;
	for (i = 0; i < MV_PNC_TOTAL_DATA_SIZE; i++) {
		if (pNewRule->mask[i] != 0)
			pNewRule->size = i + 1;
	}

	/* Check if such rule already exist */
	free = -1;
	pMatchRule = NULL;
	match_lu = 0;
	for (i = 0; i < MV_PNC_MAX_RULES; i++) {

		pWolRule = mv_pnc_wol_tbl[i];
		if (pWolRule == NULL) {
			/* Rememeber first free place */
			if (free == -1)
				free = i;

			continue;
		}
		lookup = mv_pnc_rule_cmp(pNewRule, pWolRule);
		if (lookup < 0) {
			/* Rules are partilly different - not supported */
			mvOsPrintf("%s: port=%d, size=%d - WoL rule partial match other rule\n",
						__func__, port, size);
			mvOsFree(pNewRule);
			return -3;
		}

		if (lookup == MV_PNC_LOOKUP_MAX) {
			/* The same rule exist - update port mask for all TCAM entries of the rule */
			pWolRule->port_mask |= (1 << port);
			for (lookup = 0; lookup < MV_PNC_LOOKUP_MAX; lookup++) {
				if (pWolRule->tids[lookup] != 0)
					pnc_tcam_port_update(pWolRule->tids[lookup], port, 1);
			}
			mvOsPrintf("%s: port=%d, size=%d - WoL rule already exist\n", __func__, port, size);
			mvOsFree(pNewRule);
			return i;
		}
		/* remember maximum match lookup and matched rule */
		if (lookup > match_lu) {
			match_lu = lookup;
			pMatchRule = pWolRule;
		}
	}
	if (free == -1) {
		mvOsPrintf("%s: port=%d, size=%d - No free place\n", __func__, port, size);
		mvOsFree(pNewRule);
		return -MV_FULL;
	}

	/* Set WoL rule to TCAM */
	pNewRule->id = free;
	tid = TE_WOL;

	offset = 0;
	for (lookup = 0; lookup < MV_PNC_LOOKUP_MAX; lookup++) {
		char              name[TCAM_TEXT];
		struct tcam_entry *te;
		unsigned int mask;

		if (lookup < match_lu) {
			pNewRule->tids[lookup] = pMatchRule->tids[lookup];
			offset += MV_PNC_LOOKUP_DATA_SIZE;

			/* Update port mask */
			pnc_tcam_port_update(pNewRule->tids[lookup], port, 1);
			continue;
		}

		if (offset >= pNewRule->size)
			break;

		/* Set free TCAM entry */
		for (; tid < TE_WOL_EOF; tid++) {

			te = pnc_tcam_entry_get(tid);
			if (te != NULL) {
				tcam_sw_free(te);
				continue;
			}

			te = tcam_sw_alloc(TCAM_LU_WOL + lookup);

			for (i = 0; i < MV_PNC_LOOKUP_DATA_SIZE; i++) {
				tcam_sw_set_byte(te, i, pNewRule->data[offset + i]);
				tcam_sw_set_mask(te, i, pNewRule->mask[offset + i]);
			}

			/* Set AI */
			if (lookup == 0)
				sram_sw_set_ainfo(te, pNewRule->id, AI_MASK);
			else if (lookup > match_lu)
				tcam_sw_set_ainfo(te, pNewRule->id, AI_MASK);
			else {
				tcam_sw_set_ainfo(te, pMatchRule->id, AI_MASK);
				sram_sw_set_ainfo(te, pNewRule->id, AI_MASK);
			}
			/* set port mask */
			mask = pnc_port_mask(port);
			tcam_sw_set_port(te, 0, mask);

			sprintf(name, "wol_%d", pNewRule->id);
			tcam_sw_text(te, name);

			if ((offset + i) >= pNewRule->size) {
				/* Last TCAM entry */
				sram_sw_set_lookup_done(te, 1);
			} else {
				sram_sw_set_shift_update(te, 0, MV_PNC_LOOKUP_DATA_SIZE);
				sram_sw_set_next_lookup(te, TCAM_LU_WOL + lookup + 1);
			}
			offset += MV_PNC_LOOKUP_DATA_SIZE;

			pNewRule->tids[lookup] = tid;
			tcam_hw_write(te, tid);
			tcam_sw_free(te);
			break;
		}
	}

	mv_pnc_wol_tbl[pNewRule->id] = pNewRule;
	mvOsPrintf("%s: port=%d, size=%d - New rule added [%d] = %p, \n",
				__func__, port, size, pNewRule->id, pNewRule);
	return pNewRule->id;
}

/* Delete specific WoL rule (maybe more than one TCAM entry) */
int mv_pnc_wol_rule_del(int idx)
{
#if 0
	int lookup, tid;
	MV_PNC_WOL_RULE *pWolRule;

	pWolRule = mv_pnc_wol_tbl[idx];
	if (pWolRule == NULL)
		return 1;

	/* Invalidate TCAM entries */
	for (lookup = 0; lookup < pWolRule->maxLookup; lookup++) {
		tid = pNewRule->tids[lookup];

		/* FIXME: Decrement reference count of TID, if last invalidate - TCAM entry */
		pnc_te_del(tid);
	}
#endif
	mvOsPrintf("Not supported\n");
	return 0;
}

int mv_pnc_wol_rule_del_all(int port)
{
	int i;
	MV_PNC_WOL_RULE *pWolRule;

	if (mvNetaPortCheck(port))
		return -1;

	for (i = 0; i < MV_PNC_MAX_RULES; i++) {
		pWolRule = mv_pnc_wol_tbl[i];
		if (pWolRule != NULL) {
			mvOsFree(pWolRule);
			mv_pnc_wol_tbl[i] = NULL;
		}
	}
	/* Set free TCAM entry */
	for (i = TE_WOL; i < TE_WOL_EOF; i++)
		pnc_te_del(i);

	return 0;
}

void mv_pnc_wol_sleep(int port)
{
	MV_U32 regVal;
	int    pnc_port = pnc_eth_port_map(port);

	regVal = MV_REG_READ(MV_PNC_INIT_LOOKUP_REG);

	regVal &= ~MV_PNC_PORT_LU_INIT_MASK(pnc_port);
	regVal |= MV_PNC_PORT_LU_INIT_VAL(pnc_port, TCAM_LU_WOL);

	MV_REG_WRITE(MV_PNC_INIT_LOOKUP_REG, regVal);
}

void mv_pnc_wol_wakeup(int port)
{
	MV_U32 regVal;
	int    pnc_port = pnc_eth_port_map(port);

	regVal = MV_REG_READ(MV_PNC_INIT_LOOKUP_REG);

	regVal &= ~MV_PNC_PORT_LU_INIT_MASK(pnc_port);
	regVal |= MV_PNC_PORT_LU_INIT_VAL(pnc_port, TCAM_LU_MAC);

	MV_REG_WRITE(MV_PNC_INIT_LOOKUP_REG, regVal);
}

int mv_pnc_wol_rule_dump(int idx)
{
	int	i;
	MV_PNC_WOL_RULE *pWolRule;

	if (mvNetaMaxCheck(idx, MV_PNC_MAX_RULES, "pnc_rules"))
		return -1;

	pWolRule = mv_pnc_wol_tbl[idx];
	if (pWolRule == NULL)
		return 1;

	mvOsPrintf("[%3d]: id=%d, port_mask=0x%x, size=%d, tids=[",
				idx, pWolRule->id, pWolRule->port_mask, pWolRule->size);
	for (i = 0; i < MV_PNC_LOOKUP_MAX; i++) {
		if (pWolRule->tids[i] == 0)
			break;
		mvOsPrintf(" %d", pWolRule->tids[i]);
	}
	mvOsPrintf("]\n");

	mvOsPrintf(" offs: ");
	for (i = 0;  i < MV_PNC_LOOKUP_DATA_SIZE; i++)
		mvOsPrintf("%02d", i);
	mvOsPrintf("\n");

	mvOsPrintf(" data: ");
	i = 0;
	while (i < pWolRule->size) {
		mvOsPrintf("%02x", pWolRule->data[i++]);
		if ((i % MV_PNC_LOOKUP_DATA_SIZE) == 0)
			mvOsPrintf("\n       ");
	}
	mvOsPrintf("\n");

	mvOsPrintf(" mask: ");
	i = 0;
	while (i < pWolRule->size) {
		mvOsPrintf("%02x", pWolRule->mask[i++]);
		if ((i % MV_PNC_LOOKUP_DATA_SIZE) == 0)
			mvOsPrintf("\n       ");
	}
	mvOsPrintf("\n\n");

	return 0;
}

void mv_pnc_wol_dump(void)
{
	int				i;

	mvOsPrintf("WoL rules table\n");

	for (i = 0; i < MV_PNC_MAX_RULES; i++)
		mv_pnc_wol_rule_dump(i);
}


int  mv_pnc_wol_pkt_match(int port, char *data, int size, int *ruleId)
{
	int               i, j;
	MV_PNC_WOL_RULE   *pWolRule;

	/* Check if data match one of existing rules */
	for (i = 0; i < MV_PNC_MAX_RULES; i++) {

		pWolRule = mv_pnc_wol_tbl[i];
		if (pWolRule == NULL)
			continue;

		/* packet size must be more or equal than rule size */
		if (size < pWolRule->size)
			continue;

		for (j = 0; j < pWolRule->size; j++) {
			if ((data[j] & pWolRule->mask[j]) != (pWolRule->data[j] & pWolRule->mask[j]))
				break;
		}

		if (j == pWolRule->size) {
			/* rule matched */
			if (ruleId != NULL)
				*ruleId = i;
			return 1;
		}
	}
	return 0;
}
