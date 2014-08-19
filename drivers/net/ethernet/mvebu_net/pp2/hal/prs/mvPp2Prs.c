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
#include "mvPp2Prs.h"
#include "gbe/mvPp2Gbe.h"
#include "mvPp2PrsHw.h"
#include "mvPp2Prs.h"

#define PRS_DBG(X...)


/*-------------------------------------------------------------------------------*/
/*		Static varialbes for internal use				 */
/*-------------------------------------------------------------------------------*/

int etherTypeDsa;

int mvPrsDblVlanAiShadow[DBL_VLAN_SHADOW_SIZE];

static int mvPrsDblVlanAiShadowSet(int index)
{
	RANGE_VALIDATE(index, 1, DBL_VLAN_SHADOW_SIZE - 1);
	mvPrsDblVlanAiShadow[index] = 1;
	return MV_OK;
}


static int mvPrsDblVlanAiShadowClear(int index)
{
	RANGE_VALIDATE(index, 1, DBL_VLAN_SHADOW_SIZE - 1);
	mvPrsDblVlanAiShadow[index] = 0;
	return MV_OK;
}

static int mvPrsDblVlanAiShadowClearAll(void)
{
	int i;

	for (i = 1; i < DBL_VLAN_SHADOW_SIZE; i++)
		mvPrsDblVlanAiShadowClear(i);

	return MV_OK;
}
static int mvPrsDblVlanAiFreeGet(void)
{
	int i;
	/* start from 1, 0 not in used */
	for (i = 1; i < DBL_VLAN_SHADOW_SIZE; i++)
		if (mvPrsDblVlanAiShadow[i] == 0)
			return i;

	return MV_PRS_OUT_OF_RAGE;
}


/******************************************************************************
* Common utilities
******************************************************************************/
static MV_BOOL mvPp2PrsEtypeEquals(MV_PP2_PRS_ENTRY *pe, int offset, unsigned short ethertype)
{
	unsigned char etype[MV_ETH_TYPE_LEN];

	PRS_DBG("%s\n", __func__);
	etype[0] =  (ethertype >> 8) & 0xFF;
	etype[1] =  ethertype & 0xFF;

	if (mvPp2PrsSwTcamBytesIgnorMaskCmp(pe, offset, MV_ETH_TYPE_LEN, etype) == NOT_EQUALS)
		return MV_FALSE;

	return MV_TRUE;
}

static void mvPp2PrsMatchEtype(MV_PP2_PRS_ENTRY *pe, int offset, unsigned short ethertype)
{
	PRS_DBG("%s\n", __func__);

	mvPp2PrsSwTcamByteSet(pe, offset + 0, ethertype >> 8, 0xff);
	mvPp2PrsSwTcamByteSet(pe, offset + 1, ethertype & 0xFF, 0xff);
}
#if 0
static void mvPp2PrsMatchMh(MV_PP2_PRS_ENTRY *pe, unsigned short mh)
{
	PRS_DBG("%s\n", __func__);

	mvPp2PrsSwTcamByteSet(pe, 0, mh >> 8, 0xff);
	mvPp2PrsSwTcamByteSet(pe, 1, mh & 0xFF, 0xff);
}
#endif

/******************************************************************************
 *
 * Marvell header Section
 *
 ******************************************************************************
 */
static MV_BOOL mvPrsMhRangeEquals(MV_PP2_PRS_ENTRY *pe, MV_U8 *mh, MV_U8 *mask)
{
	int index;
	unsigned char tcamByte, tcamMask;

	for (index = 0; index < MV_ETH_MH_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);
		if (tcamMask != mask[index])
			return MV_FALSE;

		if ((tcamMask & tcamByte) != (mh[index] & mask[index]))
			return MV_FALSE;
	}

	return MV_TRUE;
}

static MV_BOOL mvPrsMhRangeIntersec(MV_PP2_PRS_ENTRY *pe, MV_U8 *mh, MV_U8 *mask)
{
	int index;
	unsigned char tcamByte, tcamMask, commonMask;

	for (index = 0; index < MV_ETH_MH_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);

		commonMask = mask[index] & tcamMask;

		if ((commonMask & tcamByte) != (commonMask & mh[index]))
			return MV_FALSE;
	}
	return MV_TRUE;
}

static MV_BOOL mvPrsMhInRange(MV_PP2_PRS_ENTRY *pe, MV_U8 *mh, MV_U8 *mask)
{
	int index;
	unsigned char tcamByte, tcamMask;

	for (index = 0; index < MV_ETH_MH_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);
		if ((tcamByte & mask[index]) != (mh[index] & mask[index]))
			return MV_FALSE;
	}

	return MV_TRUE;
}

static MV_PP2_PRS_ENTRY *mvPrsMhRangeFind(int portMap, unsigned char *mh, unsigned char *mask)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	unsigned int entryPmap;

	pe = mvPp2PrsSwAlloc(PRS_LU_MH);

	/* Go through the all entires with PRS_LU_MAC */
	for (tid = PE_FIRST_FREE_TID; tid <= PE_LAST_FREE_TID; tid++) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_MH))
			continue;

		pe->index = tid;
		mvPp2PrsHwRead(pe);

		mvPp2PrsSwTcamPortMapGet(pe, &entryPmap);

		if (mvPrsMhRangeEquals(pe, mh, mask) && (entryPmap == (unsigned int)portMap))
			return pe;
	}
	mvPp2PrsSwFree(pe);
	return NULL;
}

static int mvPrsMhRangeAccept(int portMap, MV_U8 *mh, MV_U8 *mask, unsigned int ri, unsigned int riMask, MV_BOOL finish)
{
	int tid, len;
	MV_PP2_PRS_ENTRY *pe = NULL;

	/* Scan TCAM and see if entry with this <MH, port> already exist */
	pe = mvPrsMhRangeFind(portMap, mh, mask);

	if (pe == NULL) {
		/* entry not exist */
		/* Go through the all entires from first to last */
		tid = mvPp2PrsTcamFirstFree(PE_FIRST_FREE_TID, PE_LAST_FREE_TID);

		/* Can't add - No free TCAM entries */
		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			mvPp2PrsSwFree(pe);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_MH);
		pe->index = tid;
		mvPp2PrsSwTcamPortMapSet(pe, portMap);
		/* shift to MAC */
		mvPp2PrsSwSramShiftSet(pe, MV_ETH_MH_SIZE, SRAM_OP_SEL_SHIFT_ADD);

		mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
		mvPp2PrsSwSramFlowidGenSet(pe);

		/* set MH range */
		len = MV_ETH_MH_SIZE;

		while (len--)
			mvPp2PrsSwTcamByteSet(pe, len, mh[len], mask[len]);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe->index, PRS_LU_MH, "mh-range");

	}

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
	finish ? mvPp2PrsSwSramFlowidGenSet(pe) : mvPp2PrsSwSramFlowidGenClear(pe);

	/* Write entry to TCAM */
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);
	return MV_OK;
}

static int mvPrsMhRangeValid(unsigned int portMap, MV_U8 *mh, MV_U8 *mask)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int entryPmap;
	int tid;

	for (tid = PE_LAST_FREE_TID; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_MH))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		if ((mvPrsMhRangeIntersec(&pe, mh, mask)) & !mvPrsMhRangeEquals(&pe, mh, mask)) {
			if (entryPmap & portMap) {
				mvOsPrintf("%s: operation not supported, range intersection\n", __func__);
				mvOsPrintf("%s: user must delete portMap 0x%x from entry %d.\n",
					   __func__, entryPmap & portMap, tid);
				return MV_ERROR;
			}

		} else if (mvPrsMhRangeEquals(&pe, mh, mask) && (entryPmap != portMap) && (entryPmap & portMap)) {
			mvOsPrintf("%s: operation not supported, range intersection\n", __func__);
			mvOsPrintf("%s: user must delete portMap 0x%x from entry %d.\n",
				   __func__, entryPmap & portMap, tid);

			return MV_ERROR;
		}
	}
	return MV_OK;
}

int mvPrsMhSet(unsigned int portMap, unsigned short mh, unsigned short mh_mask, unsigned int ri, unsigned int riMask, MV_BOOL finish)
{
	MV_PP2_PRS_ENTRY pe;
	int tid;
	unsigned int entryPmap;
	MV_BOOL done = MV_FALSE;
	unsigned short n_mh;
	unsigned short n_mh_mask;

	/* step 1 - validation, ranges intersections are forbidden*/
	n_mh = htons(mh);
	n_mh_mask = htons(mh_mask);
	if (mvPrsMhRangeValid(portMap, (unsigned char *)&n_mh, (unsigned char *)&n_mh_mask))
		return MV_ERROR;

	/* step 2 - update TCAM */
	for (tid = PE_LAST_FREE_TID; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || !(mvPp2PrsShadowLu(tid) == PRS_LU_MH))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		if (mvPrsMhRangeEquals(&pe, (unsigned char *)&n_mh, (unsigned char *)&n_mh_mask) &&
				       (entryPmap == portMap)) {
			/* portMap and range are equals to TCAM entry*/
			done = MV_TRUE;
			mvPp2PrsSwSramRiUpdate(&pe, ri, riMask);
			finish ? mvPp2PrsSwSramFlowidGenSet(&pe) : mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsHwWrite(&pe);
			continue;
		}

		/* PRS_UDF_MAC_DEF */
		if (mvPrsMhInRange(&pe, (unsigned char *)&n_mh, (unsigned char *)&n_mh_mask) && (entryPmap & portMap)) {
			mvPp2PrsSwSramRiUpdate(&pe, ri, riMask);
			finish ? mvPp2PrsSwSramFlowidGenSet(&pe) : mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsHwWrite(&pe);
		}
	}
	/* step 3 - Add new range entry */
	if (!done)
		return mvPrsMhRangeAccept(portMap, (unsigned char *)&n_mh,
					 (unsigned char *)&n_mh_mask, ri, riMask, finish);

	return MV_OK;
}

int mvPrsMhDel(unsigned int portMap, unsigned short mh, unsigned short mh_mask)
{
	MV_PP2_PRS_ENTRY pe;
	int tid;
	unsigned int entryPmap;
	MV_BOOL found = MV_FALSE;
	unsigned short n_mh;
	unsigned short n_mh_mask;

	n_mh = htons(mh);
	n_mh_mask = htons(mh_mask);

	for (tid = PE_LAST_FREE_TID; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || !(mvPp2PrsShadowLu(tid) == PRS_LU_MH))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		/* differents ports */
		if (!(entryPmap & portMap))
			continue;

		if (mvPrsMhRangeEquals(&pe, (unsigned char *)&n_mh, (unsigned char *)&n_mh_mask)) {
			found = MV_TRUE;
			entryPmap &= ~portMap;

			if (!entryPmap) {
				/* delete entry */
				mvPp2PrsHwInv(pe.index);
				mvPp2PrsShadowClear(pe.index);
				continue;
			}

			/* update port map */
			mvPp2PrsSwTcamPortMapSet(&pe, entryPmap);
			mvPp2PrsHwWrite(&pe);
			continue;
		}

		/* PRS_UDF_MH_RANGE */
		if (!found) {
			/* range entry not exist */
			mvOsPrintf("%s: Error, entry not found\n", __func__);
			return MV_ERROR;
		}

		/* range entry allready found, now fix all relevant default entries*/
		if (mvPrsMhInRange(&pe, (unsigned char *)&n_mh, (unsigned char *)&n_mh_mask)) {
			mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsSwSramRiSet(&pe, mvPp2PrsShadowRi(tid), mvPp2PrsShadowRiMask(tid));
			mvPp2PrsHwWrite(&pe);
		}
	}
	return MV_OK;
}

/* Set default entry for Marvell header field */
static int mvPp2PrsMhInit(void)
{
	MV_PP2_PRS_ENTRY pe;

	mvPp2PrsSwClear(&pe);

	pe.index = PE_MH_DEFAULT;
	mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MH);
	mvPp2PrsSwSramShiftSet(&pe, MV_ETH_MH_SIZE, SRAM_OP_SEL_SHIFT_ADD);
	mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_MAC);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe.index, PRS_LU_MH, "mh-default");

	mvPp2PrsSwTcamPortMapSet(&pe, PORT_MASK);
	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}

/******************************************************************************
 *
 * MAC Address Section
 *
 ******************************************************************************
 */

char *mvPrsL2InfoStr(unsigned int l2_info)
{
	switch (l2_info << RI_L2_CAST_OFFS) {
	case RI_L2_UCAST:
		return "Ucast";
	case RI_L2_MCAST:
		return "Mcast";
	case RI_L2_BCAST:
		return "Bcast";
	default:
		return "Unknown";
	}
	return NULL;
}

static MV_BOOL mvPrsMacRangeEquals(MV_PP2_PRS_ENTRY *pe, MV_U8 *da, MV_U8 *mask)
{
	int index;
	unsigned char tcamByte, tcamMask;

	for (index = 0; index < MV_MAC_ADDR_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);
		if (tcamMask != mask[index])
			return MV_FALSE;

		if ((tcamMask & tcamByte) != (da[index] & mask[index]))
			return MV_FALSE;
	}

	return MV_TRUE;
}

static MV_BOOL mvPrsMacRangeIntersec(MV_PP2_PRS_ENTRY *pe, MV_U8 *da, MV_U8 *mask)
{
	int index;
	unsigned char tcamByte, tcamMask, commonMask;

	for (index = 0; index < MV_MAC_ADDR_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);

		commonMask = mask[index] & tcamMask;


		if ((commonMask & tcamByte) != (commonMask & da[index]))
			return MV_FALSE;
	}
	return MV_TRUE;
}

static MV_BOOL mvPrsMacInRange(MV_PP2_PRS_ENTRY *pe, MV_U8* da, MV_U8* mask)
{
	int index;
	unsigned char tcamByte, tcamMask;

	for (index = 0; index < MV_MAC_ADDR_SIZE; index++) {
		mvPp2PrsSwTcamByteGet(pe, index, &tcamByte, &tcamMask);
		if ((tcamByte & mask[index]) != (da[index] & mask[index]))
			return MV_FALSE;
	}

	return MV_TRUE;
}

static MV_PP2_PRS_ENTRY *mvPrsMacDaRangeFind(int portMap, unsigned char *da, unsigned char *mask, int udfType)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	unsigned int entryPmap;

	pe = mvPp2PrsSwAlloc(PRS_LU_MAC);

	/* Go through the all entires with PRS_LU_MAC */
	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_MAC))
			continue;

		if (mvPp2PrsShadowUdf(tid) != udfType)
			continue;

		pe->index = tid;
		mvPp2PrsHwRead(pe);

		mvPp2PrsSwTcamPortMapGet(pe, &entryPmap);

		if (mvPrsMacRangeEquals(pe, da, mask) && (entryPmap == portMap))
			return pe;
	}
	mvPp2PrsSwFree(pe);
	return NULL;

}

static MV_PP2_PRS_ENTRY *mvPrsMacDaFind(int port, unsigned char *da)
{
	unsigned char mask[MV_MAC_ADDR_SIZE];

	mask[0] = mask[1] = mask[2] = mask[3] = mask[4] = mask[5] = 0xff;

	/* Scan TCAM and see if entry with this <MAC DA, port> already exist */
	return mvPrsMacDaRangeFind((1 << port), da, mask, PRS_UDF_MAC_DEF);
}

static int mvPrsMacDaRangeAccept(int portMap, MV_U8 *da, MV_U8 *mask, unsigned int ri, unsigned int riMask, MV_BOOL finish)
{
	int tid, len;
	MV_PP2_PRS_ENTRY *pe = NULL;

	/* Scan TCAM and see if entry with this <MAC DA, port> already exist */
	pe = mvPrsMacDaRangeFind(portMap, da, mask, PRS_UDF_MAC_RANGE);

	if (pe == NULL) {
		/* entry not exist */
		/* find last simple mac entry*/
		for (tid = PE_LAST_FREE_TID ; tid >= PE_FIRST_FREE_TID; tid--)
			if (mvPp2PrsShadowIsValid(tid) && (mvPp2PrsShadowLu(tid) == PRS_LU_MAC) &&
			    (mvPp2PrsShadowUdf(tid) == PRS_UDF_MAC_DEF))
				break;

		/* Go through the all entires from first to last */
		tid = mvPp2PrsTcamFirstFree(tid + 1, PE_LAST_FREE_TID);

		/* Can't add - No free TCAM entries */
		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			mvPp2PrsSwFree(pe);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_MAC);
		pe->index = tid;
		mvPp2PrsSwTcamPortMapSet(pe, portMap);
		/* shift to ethertype */
		mvPp2PrsSwSramShiftSet(pe, 2 * MV_MAC_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

		/* set DA range */
		len = MV_MAC_ADDR_SIZE;

		while (len--)
			mvPp2PrsSwTcamByteSet(pe, len, da[len], mask[len]);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe->index, PRS_LU_MAC, "mac-range");
		mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_MAC_RANGE);

	}

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
	finish ? mvPp2PrsSwSramFlowidGenSet(pe) : mvPp2PrsSwSramFlowidGenClear(pe);

	/* Write entry to TCAM */
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);
	return MV_OK;
}


/* delete all port's simple (not range) multicast entries */
int mvPrsMcastDelAll(int port)
{
	MV_PP2_PRS_ENTRY pe;
	int tid, index;
	unsigned char da[MV_MAC_ADDR_SIZE], daMask[MV_MAC_ADDR_SIZE];

	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {

		if (!mvPp2PrsShadowIsValid(tid))
			continue;

		if (mvPp2PrsShadowLu(tid) != PRS_LU_MAC)
			continue;

		if (mvPp2PrsShadowUdf(tid) != PRS_UDF_MAC_DEF)
			continue;

		/* only simple mac entries */
		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		/* read mac addr from entry */
		for (index = 0; index < MV_MAC_ADDR_SIZE; index++)
			mvPp2PrsSwTcamByteGet(&pe, index, &da[index], &daMask[index]);

		if (MV_IS_BROADCAST_MAC(da))
			continue;

		if (MV_IS_MULTICAST_MAC(da))
			/* delete mcast entry */
			mvPrsMacDaAccept(port, da, 0);
	}

	return MV_OK;
}


/* TODO: use mvPrsMacDaRangeAccept */
int mvPrsMacDaAccept(int port, unsigned char *da, int add)
{
	MV_PP2_PRS_ENTRY *pe = NULL;
	unsigned int len, ports, ri;
	int tid;
	char name[PRS_TEXT_SIZE];

	/* Scan TCAM and see if entry with this <MAC DA, port> already exist */
	pe = mvPrsMacDaFind(port, da);

	if (pe == NULL) {
		/* No such entry */
		if (!add) {
			/* Can't remove - No such entry */
			return MV_ERROR;
		}
		 /* Create new TCAM entry */

		/* find last range mac entry*/
		for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++)
			if (mvPp2PrsShadowIsValid(tid) && (mvPp2PrsShadowLu(tid) == PRS_LU_MAC) &&
			    (mvPp2PrsShadowUdf(tid) == PRS_UDF_MAC_RANGE))
				break;

		/* Go through the all entires from first to last */
		tid = mvPp2PrsTcamFirstFree(0, tid - 1);

		/* Can't add - No free TCAM entries */
		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			mvPp2PrsSwFree(pe);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_MAC);
		pe->index = tid;

		mvPp2PrsSwTcamPortMapSet(pe, 0);

	}
	/* Update port mask */
	mvPp2PrsSwTcamPortSet(pe, port, add);
	mvPp2PrsSwTcamPortMapGet(pe, &ports);

	if (ports == 0) {
		if (add) {
			mvPp2PrsSwFree(pe);
			/* Internal error, port should be set in ports bitmap */
			return MV_ERROR;
		}
		/* No ports - invalidate the entry */
		mvPp2PrsHwInv(pe->index);
		mvPp2PrsShadowClear(pe->index);
		mvPp2PrsSwFree(pe);
		return MV_OK;

	}

	/* Continue - set next lookup */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_DSA);

	/* set match on DA */
	len = MV_MAC_ADDR_SIZE;
	while (len--)
		mvPp2PrsSwTcamByteSet(pe, len, da[len], 0xff);

	/* Set result info bits */
	if (MV_IS_BROADCAST_MAC(da)) {
		ri = RI_L2_BCAST;
		mvOsSPrintf(name, "bcast-port-%d", port);

	} else if (MV_IS_MULTICAST_MAC(da)) {
		ri = RI_L2_MCAST;
		mvOsSPrintf(name, "mcast-port-%d", port);
	} else {
		ri = RI_L2_UCAST | RI_MAC_ME_MASK;
		mvOsSPrintf(name, "ucast-port-%d", port);
	}

	/*mvPp2PrsSwSramRiSetBit(pe, RI_MAC_ME_BIT);*/
	mvPp2PrsSwSramRiUpdate(pe, ri, RI_L2_CAST_MASK | RI_MAC_ME_MASK);
	mvPp2PrsShadowRiSet(pe->index, ri, RI_L2_CAST_MASK | RI_MAC_ME_MASK);

	/* shift to ethertype */
	mvPp2PrsSwSramShiftSet(pe, 2 * MV_MAC_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

	/* Write entry to TCAM */
	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_MAC, name);
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_MAC_DEF);

	mvPp2PrsSwFree(pe);
	return MV_OK;
}


static int mvPrsMacDaRangeValid(unsigned int portMap, MV_U8 *da, MV_U8 *mask)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int entryPmap;
	int tid;

	for (tid = PE_LAST_FREE_TID ; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_MAC) ||
		    (mvPp2PrsShadowUdf(tid) != PRS_UDF_MAC_RANGE))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		if ((mvPrsMacRangeIntersec(&pe, da, mask)) && !mvPrsMacRangeEquals(&pe, da, mask)) {
			if (entryPmap & portMap) {
				mvOsPrintf("%s: operation not supported, range intersection\n", __func__);
				mvOsPrintf("%s: user must delete portMap 0x%x from entry %d.\n",
					   __func__, entryPmap & portMap, tid);
				return MV_ERROR;
			}

		} else if (mvPrsMacRangeEquals(&pe, da, mask) && (entryPmap != portMap) && (entryPmap & portMap)) {
			mvOsPrintf("%s: operation not supported, range intersection\n", __func__);
			mvOsPrintf("%s: user must delete portMap 0x%x from entry %d.\n",
				   __func__, entryPmap & portMap, tid);
			return MV_ERROR;
		}
	}
	return MV_OK;
}

int mvPrsMacDaRangeSet(unsigned int portMap, MV_U8 *da, MV_U8 *mask, unsigned int ri, unsigned int riMask, MV_BOOL finish)
{
	MV_PP2_PRS_ENTRY pe;
	int tid;
	unsigned int entryPmap;
	MV_BOOL done = MV_FALSE;

	/* step 1 - validation, ranges intersections are forbidden*/
	if (mvPrsMacDaRangeValid(portMap, da, mask))
		return MV_ERROR;

	/* step 2 - update TCAM */
	for (tid = PE_LAST_FREE_TID ; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || !(mvPp2PrsShadowLu(tid) == PRS_LU_MAC))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		if ((mvPp2PrsShadowUdf(tid) == PRS_UDF_MAC_RANGE) &&
		    mvPrsMacRangeEquals(&pe, da, mask) && (entryPmap == portMap)) {
			/* portMap and range are equals to TCAM entry*/
			done = MV_TRUE;
			mvPp2PrsSwSramRiUpdate(&pe, ri, riMask);
			finish ? mvPp2PrsSwSramFlowidGenSet(&pe) : mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsHwWrite(&pe);
			continue;
		}

		/* PRS_UDF_MAC_DEF */
		if (mvPrsMacInRange(&pe, da, mask) && (entryPmap & portMap)) {
			mvPp2PrsSwSramRiUpdate(&pe, ri, riMask);
			finish ? mvPp2PrsSwSramFlowidGenSet(&pe) : mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsHwWrite(&pe);
		}
	}
	/* step 3 - Add new range entry */
	if (!done)
		return mvPrsMacDaRangeAccept(portMap, da, mask, ri, riMask, finish);

	return MV_OK;

}

int mvPrsMacDaRangeDel(unsigned int portMap, MV_U8 *da, MV_U8 *mask)
{
	MV_PP2_PRS_ENTRY pe;
	int tid;
	unsigned int entryPmap;
	MV_BOOL found = MV_FALSE;

	for (tid = PE_LAST_FREE_TID ; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || !(mvPp2PrsShadowLu(tid) == PRS_LU_MAC))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		/* differents ports */
		if (!(entryPmap & portMap))
			continue;

		if ((mvPp2PrsShadowUdf(tid) == PRS_UDF_MAC_RANGE) && (mvPrsMacRangeEquals(&pe, da, mask))) {

			found = MV_TRUE;
			entryPmap &= ~portMap;

			if (!entryPmap) {
				/* delete entry */
				mvPp2PrsHwInv(pe.index);
				mvPp2PrsShadowClear(pe.index);
				continue;
			}

			/* update port map */
			mvPp2PrsSwTcamPortMapSet(&pe, entryPmap);
			mvPp2PrsHwWrite(&pe);
			continue;
		}

		/* PRS_UDF_MAC_RANGE */
		if (!found) {
			/* range entry not exist */
			mvOsPrintf("%s: Error, entry not found\n", __func__);
			return MV_ERROR;
		}

		/* range entry allready found, now fix all relevant default entries*/
		if (mvPrsMacInRange(&pe, da, mask)) {
			mvPp2PrsSwSramFlowidGenClear(&pe);
			mvPp2PrsSwSramRiSet(&pe, mvPp2PrsShadowRi(tid), mvPp2PrsShadowRiMask(tid));
			mvPp2PrsHwWrite(&pe);
		}
	}
	return MV_OK;
}

/* Drop special MAC DA - 6 bytes */
int mvPrsMacDaDrop(int port, unsigned char *da, int add)
{
	return MV_OK;
}

int mvPrsMacDropAllSet(int port, int add)
{
	MV_PP2_PRS_ENTRY pe;

	if (mvPp2PrsShadowIsValid(PE_DROP_ALL)) {
		/* Entry exist - update port only */
		pe.index = PE_DROP_ALL;
		mvPp2PrsHwRead(&pe);
	} else {
		/* Entry doesn't exist - create new */
		mvPp2PrsSwClear(&pe);
		mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MAC);
		pe.index = PE_DROP_ALL;

		/* Non-promiscous mode for all ports - DROP unknown packets */
		mvPp2PrsSwSramRiSetBit(&pe, RI_DROP_BIT);
		/*	mvPp2PrsSwSramLuDoneSet(&pe);*/

		mvPp2PrsSwSramFlowidGenSet(&pe);
		mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_FLOWS);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, "drop-all");

		mvPp2PrsSwTcamPortMapSet(&pe, 0);
	}

	mvPp2PrsSwTcamPortSet(&pe, port, add);

	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}

/* Set port to promiscous mode */
int mvPrsMacPromiscousSet(int port, int add)
{
	MV_PP2_PRS_ENTRY pe;

	/* Promiscous mode - Accept unknown packets */

	if (mvPp2PrsShadowIsValid(PE_MAC_PROMISCOUS)) {
		/* Entry exist - update port only */
		pe.index = PE_MAC_PROMISCOUS;
		mvPp2PrsHwRead(&pe);
	} else {
		/* Entry doesn't exist - create new */
		mvPp2PrsSwClear(&pe);
		mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MAC);
		pe.index = PE_MAC_PROMISCOUS;

		/* Continue - set next lookup */
		mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_DSA);

		/* Set result info bits */
		mvPp2PrsSwSramRiUpdate(&pe, RI_L2_UCAST, RI_L2_CAST_MASK);

		/* shift to ethertype */
		mvPp2PrsSwSramShiftSet(&pe, 2 * MV_MAC_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

		/* mask all ports */
		mvPp2PrsSwTcamPortMapSet(&pe, 0);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, "promisc");
	}

	mvPp2PrsSwTcamPortSet(&pe, port, add);

	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}

/* 0 - reject, 1 - accept */
int mvPrsMacAllMultiSet(int port, int add)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int i, idx = 0;
	char *rule_str[MAX_MAC_MC] = { "mcast-mac-ip4", "mcast-mac-ip6" };

	/* Ethernet multicast address first byte is with 0x01 */
	unsigned char da_mc[MAX_MAC_MC] = { 0x01, 0x33 };

	for (i = IP4_MAC_MC; i < MAX_MAC_MC; i++) {
		if (i == IP4_MAC_MC)
			idx = PE_MAC_MC_ALL;
		else
			idx = PE_MAC_MC_IP6;
		/* all multicast */

		if (mvPp2PrsShadowIsValid(idx)) {
			/* Entry exist - update port only */
			pe.index = idx;
			mvPp2PrsHwRead(&pe);
		} else {
			/* Entry doesn't exist - create new */
			mvPp2PrsSwClear(&pe);

			pe.index = idx;

			mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MAC);

			/* Continue - set next lookup */
			mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_DSA);

			/* Set result info bits */
			mvPp2PrsSwSramRiUpdate(&pe, RI_L2_MCAST, RI_L2_CAST_MASK);

			mvPp2PrsSwTcamByteSet(&pe, 0, da_mc[i], 0xff);

			/* shift to ethertype */
		mvPp2PrsSwSramShiftSet(&pe, 2 * MV_MAC_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

			/* no ports */
			mvPp2PrsSwTcamPortMapSet(&pe, 0);

			/* Update mvPrsShadowTbl */
			mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, rule_str[i]);
		}

		mvPp2PrsSwTcamPortSet(&pe, port, add);

		mvPp2PrsHwWrite(&pe);
	}

	return MV_OK;
}

int mvPrsMhRxSpecialSet(int port, unsigned short mh, int add)
{
#if 0 /* this function to be removed in future */
	MV_PP2_PRS_ENTRY pe;

	if (mvPp2PrsShadowIsValid(PE_RX_SPECIAL)) {
		/* Entry exist - update port only */
		pe.index = PE_RX_SPECIAL;
		mvPp2PrsHwRead(&pe);
	} else {
		/* Entry doesn't exist - create new */
		mvPp2PrsSwClear(&pe);
		mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MAC);
		pe.index = PE_RX_SPECIAL;

		mvPp2PrsSwSramRiUpdate(&pe, RI_CPU_CODE_RX_SPEC, RI_CPU_CODE_MASK);
		mvPp2PrsSwSramFlowidGenSet(&pe);
		mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_FLOWS);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, "RX special");

		mvPp2PrsSwTcamPortMapSet(&pe, 0);
	}

	mvPp2PrsMatchMh(&pe, mh);
	mvPp2PrsSwTcamPortSet(&pe, port, add);

	mvPp2PrsHwWrite(&pe);
#endif
	return MV_OK;
}

/* Set default entires (place holder) for promiscous, non-promiscous and all-milticast MAC addresses */
static int mvPp2PrsMacInit(void)
{
	MV_PP2_PRS_ENTRY pe;

	mvPp2PrsSwClear(&pe);

	/* Non-promiscous mode for all ports - DROP unknown packets */
	pe.index = PE_MAC_NON_PROMISCOUS;
	mvPp2PrsSwTcamLuSet(&pe, PRS_LU_MAC);
	mvPp2PrsSwSramRiSetBit(&pe, RI_DROP_BIT);
	/* mvPp2PrsSwSramLuDoneSet(&pe);*/

	mvPp2PrsSwSramFlowidGenSet(&pe);
	mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_FLOWS);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, "non-promisc");

	mvPp2PrsSwTcamPortMapSet(&pe, PORT_MASK);
	mvPp2PrsHwWrite(&pe);

	/* place holders only - no ports */
	mvPrsMacDropAllSet(0, 0);
	mvPrsMacPromiscousSet(0, 0);
	mvPrsMacAllMultiSet(0, 0);

	return MV_OK;
}

/******************************************************************************
 *
 * DSA Section
 *
 ******************************************************************************
 */

static int mvPp2PrsDsaTagEtherTypeSet(int port, int add, int tagged, int extend)
{
	MV_PP2_PRS_ENTRY pe;
	char name[PRS_TEXT_SIZE];
	int tid, shift, portMask;

	/* if packet is tagged continue check vlans */

	if (extend) {
		if (tagged) {
			tid = PE_ETYPE_EDSA_TAGGED;
			mvOsSPrintf(name, "Etype-EDSA-tagged");
		} else {
			tid = PE_ETYPE_EDSA_UNTAGGED;
			mvOsSPrintf(name, "Etype-EDSA-untagged");
		}
		portMask = 0;
		shift = 8;
	} else {

		if (tagged) {
			tid = PE_ETYPE_DSA_TAGGED;
			mvOsSPrintf(name, "Etype-DSA-tagged");
		} else {
			tid = PE_ETYPE_DSA_UNTAGGED;
			mvOsSPrintf(name, "Etype-DSA-untagged");
		}
		portMask = PORT_MASK;
		shift = 4;
	}

	if (mvPp2PrsShadowIsValid(tid)) {
		/* Entry exist - update port only */
		pe.index = tid;
		mvPp2PrsHwRead(&pe);
	} else {
		/* Entry doesn't exist - create new */
		mvPp2PrsSwClear(&pe);
		mvPp2PrsSwTcamLuSet(&pe, PRS_LU_DSA);
		pe.index = tid;

		/* set etherType*/
		mvPp2PrsMatchEtype(&pe, 0, etherTypeDsa);
		mvPp2PrsMatchEtype(&pe, 2, 0);

		mvPp2PrsSwSramRiSetBit(&pe, RI_DSA_BIT);

		/* shift etherType + 2 byte reserved + tag*/
		mvPp2PrsSwSramShiftSet(&pe, 2 + MV_ETH_TYPE_LEN + shift, SRAM_OP_SEL_SHIFT_ADD);

		mvPp2PrsShadowSet(pe.index, PRS_LU_DSA, name);

		/* set tagged bit in DSA tag */
		/* TODO use define */
		if (tagged) {
			/* set bit 29 in dsa tag */
			mvPp2PrsSwTcamByteSet(&pe, MV_ETH_TYPE_LEN + 2 + 3, 0x20, 0x20);

			/* Clear all AI bits for next iteration */
			mvPp2PrsSwSramAiUpdate(&pe, 0, SRAM_AI_MASK);

			mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_VLAN);
		} else {
			/* Set result info bits - No valns ! */
			mvPp2PrsSwSramRiUpdate(&pe, RI_VLAN_NONE, RI_VLAN_MASK);
			mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_L2);
		}
		/* all ports enabled */
		mvPp2PrsSwTcamPortMapSet(&pe, portMask);
	}

	mvPp2PrsSwTcamPortSet(&pe, port, add);

	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}


static int mvPp2PrsDsaTagSet(int port, int add, int tagged, int extend)
{
	MV_PP2_PRS_ENTRY pe;
	char name[PRS_TEXT_SIZE];
	int tid, shift;

	/* if packet is tagged continue check vlans */

	if (extend) {
		if (tagged) {
			tid = PE_EDSA_TAGGED;
			mvOsSPrintf(name, "EDSA-tagged");
		} else {
			tid = PE_EDSA_UNTAGGED;
			mvOsSPrintf(name, "EDSA-untagged");
		}

		shift = 8;
	} else {

		if (tagged) {
			tid = PE_DSA_TAGGED;
			mvOsSPrintf(name, "DSA-tagged");
		} else {
			tid = PE_DSA_UNTAGGED;
			mvOsSPrintf(name, "DSA-untagged");
		}

		shift = 4;
	}

	if (mvPp2PrsShadowIsValid(tid)) {
		/* Entry exist - update port only */
		pe.index = tid;
		mvPp2PrsHwRead(&pe);
	} else {
		/* Entry doesn't exist - create new */
		mvPp2PrsSwClear(&pe);
		mvPp2PrsSwTcamLuSet(&pe, PRS_LU_DSA);
		pe.index = tid;


		/* shift 4 bytes if DSA tag , Skip 8 bytes if extand DSA tag*/
		mvPp2PrsSwSramShiftSet(&pe, shift, SRAM_OP_SEL_SHIFT_ADD);

		mvPp2PrsShadowSet(pe.index, PRS_LU_DSA, name);

		/* set tagged bit in DSA tag */
		/* TODO use define */
		if (tagged) {
			mvPp2PrsSwTcamByteSet(&pe, 0, 0x20, 0x20);

			/* Clear all AI bits for next iteration */
			mvPp2PrsSwSramAiUpdate(&pe, 0, SRAM_AI_MASK);

			mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_VLAN);
		} else {
			/* Set result info bits - No valns ! */
			mvPp2PrsSwSramRiUpdate(&pe, RI_VLAN_NONE, RI_VLAN_MASK);
			mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_L2);
		}

		mvPp2PrsSwTcamPortMapSet(&pe, 0);
	}

	mvPp2PrsSwTcamPortSet(&pe, port, add);

	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}



static int mvPp2PrsDsaInit(void)
{
	MV_PP2_PRS_ENTRY pe;

	etherTypeDsa = DSA_ETHER_TYPE;

	/* none tagged EDSA entry -place holder */
	mvPp2PrsDsaTagSet(0, 0, UNTAGGED, EDSA);

	/* tagged EDSA entry -place holder */
	mvPp2PrsDsaTagSet(0, 0, TAGGED, EDSA);

	/* none tagged DSA entry -place holder */
	mvPp2PrsDsaTagSet(0, 0, UNTAGGED, DSA);

	/* tagged DSA entry -place holder */
	mvPp2PrsDsaTagSet(0, 0, TAGGED, DSA);

	/* none tagged EDSA EtherType entry - place holder*/
	mvPp2PrsDsaTagEtherTypeSet(0, 0, UNTAGGED, EDSA);

	/* tagged EDSA EtherType entry - place holder*/
	mvPp2PrsDsaTagEtherTypeSet(0, 0, TAGGED, EDSA);

	/* none tagged DSA EtherType entry */
	mvPp2PrsDsaTagEtherTypeSet(0, 1, UNTAGGED, DSA);

	/* tagged DSA EtherType entry */
	mvPp2PrsDsaTagEtherTypeSet(0, 1, TAGGED, DSA);

	/* default entry , if DSA or EDSA tag not found */
	mvPp2PrsSwClear(&pe);
	mvPp2PrsSwTcamLuSet(&pe, PRS_LU_DSA);
	pe.index = PE_DSA_DEFAULT;
	mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_VLAN);

	/* shift 0 bytes */
	mvPp2PrsSwSramShiftSet(&pe, 0, SRAM_OP_SEL_SHIFT_ADD);
	mvPp2PrsShadowSet(pe.index, PRS_LU_MAC, "default-DSA-tag");

	/* Clear all AI bits for next iteration */
	mvPp2PrsSwSramAiUpdate(&pe, 0, SRAM_AI_MASK);

	/* match for all ports*/
	mvPp2PrsSwTcamPortMapSet(&pe, PORT_MASK);
	mvPp2PrsHwWrite(&pe);

	return MV_OK;
}

int mvPp2PrsEtypeDsaSet(unsigned int eType)
{
	int tid;

	MV_PP2_PRS_ENTRY pe;

	etherTypeDsa = eType;

	for (tid = PE_ETYPE_EDSA_TAGGED; tid <= PE_ETYPE_DSA_UNTAGGED; tid++) {

		pe.index = tid;

		mvPp2PrsHwRead(&pe);

		/* overrwite old etherType*/
		mvPp2PrsMatchEtype(&pe, 0, etherTypeDsa);

		mvPp2PrsHwWrite(&pe);
	}

	return MV_OK;
}

int mvPp2PrsEtypeDsaModeSet(int port, int extand)
{
	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS - 1);
	POS_RANGE_VALIDATE(extand, 1);

	if (extand) {
		mvPp2PrsDsaTagEtherTypeSet(port, 1, UNTAGGED, EDSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 1, TAGGED, EDSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 0, UNTAGGED, DSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 0, TAGGED, DSA);
	} else {
		mvPp2PrsDsaTagEtherTypeSet(port, 0, UNTAGGED, EDSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 0, TAGGED, EDSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 1, UNTAGGED, DSA);
		mvPp2PrsDsaTagEtherTypeSet(port, 1, TAGGED, DSA);
	}
	return MV_OK;
}

int mvPp2PrsTagModeSet(int port, int type)
{

	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS - 1);

	switch (type) {

	case MV_TAG_TYPE_EDSA:
		/* Add port to EDSA entries */
		mvPp2PrsDsaTagSet(port, 1, TAGGED, EDSA);
		mvPp2PrsDsaTagSet(port, 1, UNTAGGED, EDSA);
		/* remove port from DSA entries */
		mvPp2PrsDsaTagSet(port, 0, TAGGED, DSA);
		mvPp2PrsDsaTagSet(port, 0, UNTAGGED, DSA);

		break;

	case MV_TAG_TYPE_DSA:
		/* Add port to DSA entries */
		mvPp2PrsDsaTagSet(port, 1, TAGGED, DSA);
		mvPp2PrsDsaTagSet(port, 1, UNTAGGED, DSA);

		/* remove port from EDSA entries */
		mvPp2PrsDsaTagSet(port, 0, TAGGED, EDSA);
		mvPp2PrsDsaTagSet(port, 0, UNTAGGED, EDSA);

		break;

	case MV_TAG_TYPE_MH:
	case MV_TAG_TYPE_NONE:

		/* remove port form EDSA and DSA entries */
		mvPp2PrsDsaTagSet(port, 0, TAGGED, DSA);
		mvPp2PrsDsaTagSet(port, 0, UNTAGGED, DSA);
		mvPp2PrsDsaTagSet(port, 0, TAGGED, EDSA);
		mvPp2PrsDsaTagSet(port, 0, UNTAGGED, EDSA);

		break;

	default:
		POS_RANGE_VALIDATE(type, MV_TAG_TYPE_EDSA);
	}

	return MV_OK;

}


/******************************************************************************
 *
 * VLAN Section
 *
 ******************************************************************************
 */

char *mvPrsVlanInfoStr(unsigned int vlan_info)
{
	switch (vlan_info << RI_VLAN_OFFS) {
	case RI_VLAN_NONE:
		return "None";
	case RI_VLAN_SINGLE:
		return "Single";
	case RI_VLAN_DOUBLE:
		return "Double";
	case RI_VLAN_TRIPLE:
		return "Triple";
	default:
		return "Unknown";
	}
	return NULL;
}

static MV_PP2_PRS_ENTRY *mvPrsVlanFind(unsigned short tpid, int ai)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	unsigned int riBits, aiBits, enable;
	unsigned char tpidArr[2];

	pe = mvPp2PrsSwAlloc(PRS_LU_VLAN);

#ifdef MV_CPU_LE
	tpidArr[0] = ((unsigned char *)&tpid)[1];
	tpidArr[1] = ((unsigned char *)&tpid)[0];
#else
	tpidArr[0] = ((unsigned char *)&tpid)[0];
	tpidArr[1] = ((unsigned char *)&tpid)[1];
#endif
	/* Go through the all entires with PRS_LU_MAC */
	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_VLAN))
			continue;

		pe->index = tid;

		mvPp2PrsHwRead(pe);
		if (mvPp2PrsSwTcamBytesIgnorMaskCmp(pe, 0, 2, tpidArr) == EQUALS) {
			mvPp2PrsSwSramRiGet(pe, &riBits, &enable);

			/* get Vlan type */
			riBits = (riBits & RI_VLAN_MASK);

			/* get current AI value Tcam */
			mvPp2PrsSwTcamAiGet(pe, &aiBits, &enable);

			/* clear double Vlan Bit */
			aiBits &= ~(1 << DBL_VLAN_AI_BIT);

			if (ai != aiBits)
				continue;

			if ((riBits == RI_VLAN_SINGLE) || (riBits == RI_VLAN_TRIPLE))
				return pe;
		}
	}
	mvPp2PrsSwFree(pe);
	return NULL;
}

int mvPrsVlanExist(unsigned short tpid, int ai)
{
	if (NULL == mvPrsVlanFind(tpid, ai))
		return 0;
	else
		return 1;
}

static MV_PP2_PRS_ENTRY *mvPrsDoubleVlanFind(unsigned short tpid1, unsigned short tpid2)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	unsigned int bits, enable;
	unsigned char tpidArr1[2];
	unsigned char tpidArr2[2];

#ifdef MV_CPU_LE
	tpidArr1[0] = ((unsigned char *)&tpid1)[1];
	tpidArr1[1] = ((unsigned char *)&tpid1)[0];

	tpidArr2[0] = ((unsigned char *)&tpid2)[1];
	tpidArr2[1] = ((unsigned char *)&tpid2)[0];
#else /* MV_CPU_LE */
	tpidArr1[0] = ((unsigned char *)&tpid1)[0];
	tpidArr1[1] = ((unsigned char *)&tpid1)[1];

	tpidArr2[0] = ((unsigned char *)&tpid2)[0];
	tpidArr2[1] = ((unsigned char *)&tpid2)[1];
#endif

	pe = mvPp2PrsSwAlloc(PRS_LU_VLAN);

	/* Go through the all entires with PRS_LU_MAC */
	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_VLAN))
			continue;

		pe->index = tid;
		mvPp2PrsHwRead(pe);
		if ((mvPp2PrsSwTcamBytesIgnorMaskCmp(pe, 0, 2, tpidArr1) == EQUALS) &&
		    (mvPp2PrsSwTcamBytesIgnorMaskCmp(pe, 4, 2, tpidArr2) == EQUALS)) {

			mvPp2PrsSwSramRiGet(pe, &bits, &enable);

			if ((bits & RI_VLAN_MASK) == RI_VLAN_DOUBLE)
				return pe;
		}
	}
	mvPp2PrsSwFree(pe);
	return NULL;
}

int mvPrsDoubleVlanExist(unsigned short tpid1, unsigned short tpid2)
{
	if (NULL == mvPrsDoubleVlanFind(tpid1, tpid2))
		return 0;
	else
		return 1;
}

/* return last double vlan entry */
static int mvPpPrsDoubleVlanLast(void)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int bits, enable;
	int tid;

	for (tid = PE_LAST_FREE_TID; tid >= PE_FIRST_FREE_TID; tid--) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_VLAN))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwSramRiGet(&pe, &bits, &enable);

		if ((bits & RI_VLAN_MASK) == RI_VLAN_DOUBLE)
			return tid;
	}
	return tid;
}

/* return first Single or Triple vlan entry */
static int mvPpPrsVlanFirst(void)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int bits, enable;
	int tid;

	for (tid = PE_FIRST_FREE_TID; tid <= PE_LAST_FREE_TID; tid++) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_VLAN))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);
		mvPp2PrsSwSramRiGet(&pe, &bits, &enable);

		bits &= RI_VLAN_MASK;

		if ((bits == RI_VLAN_SINGLE) || (bits == RI_VLAN_TRIPLE))
			return tid;
	}
	return tid;
}

static int mvPp2PrsVlanAdd(unsigned short tpid, int ai, unsigned int portBmp)
{
	int lastDouble, tid, status = 0;
	MV_PP2_PRS_ENTRY *pe = NULL;
	char name[PRS_TEXT_SIZE];

	pe = mvPrsVlanFind(tpid, ai);

	if (pe == NULL) {

		/* Create new TCAM entry */
		tid = mvPp2PrsTcamFirstFree(PE_LAST_FREE_TID, PE_FIRST_FREE_TID);

		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			return MV_ERROR;
		}

		/* get last double vlan tid */
		lastDouble = mvPpPrsDoubleVlanLast();

		if (tid <= lastDouble) {
			/* double vlan entries overlapping*/
			mvOsPrintf("%s:Can't add entry, please remove unnecessary triple or single vlans entries.\n",
				   __func__);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_VLAN);
		pe->index = tid;

		mvPp2PrsMatchEtype(pe, 0, tpid);

		/* Clear all AI bits for next iteration */
		status |= mvPp2PrsSwSramAiUpdate(pe, 0, SRAM_AI_MASK);

		/* Continue - set next lookup */
		status |= mvPp2PrsSwSramNextLuSet(pe, PRS_LU_L2);

		/* shift 4 bytes - skip 1 VLAN tags */
		status |= mvPp2PrsSwSramShiftSet(pe, MV_VLAN_HLEN, SRAM_OP_SEL_SHIFT_ADD);

		if (ai == SINGLE_VLAN_AI) {
			/* single vlan*/
			mvOsSPrintf(name, "single-VLAN");
			mvPp2PrsSwSramRiUpdate(pe, RI_VLAN_SINGLE, RI_VLAN_MASK);
		} else {
			/* triple vlan*/
			mvOsSPrintf(name, "triple-VLAN-%d", ai);
			ai |= (1 << DBL_VLAN_AI_BIT);
			mvPp2PrsSwSramRiUpdate(pe, RI_VLAN_TRIPLE, RI_VLAN_MASK);
		}

		status |= mvPp2PrsSwTcamAiUpdate(pe, ai, SRAM_AI_MASK);

		mvPp2PrsShadowSet(pe->index, PRS_LU_VLAN, name);
	}

	status |= mvPp2PrsSwTcamPortMapSet(pe, portBmp);

	if (status == 0)
		mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPp2PrsVlanDel(unsigned short tpid, int ai)
{
	MV_PP2_PRS_ENTRY *pe = NULL;

	pe = mvPrsVlanFind(tpid, ai);

	if (pe == NULL) {
		/* No such entry */
		mvOsPrintf("Can't remove - No such entry\n");
		return MV_ERROR;
	}

	/* remove entry */
	mvPp2PrsHwInv(pe->index);
	mvPp2PrsShadowClear(pe->index);
	mvPp2PrsSwFree(pe);
	return MV_OK;
}

int mvPp2PrsDoubleVlanAdd(unsigned short tpid1, unsigned short tpid2, unsigned int portBmp)
{
	int tid, ai, status = 0;
	int firstVlan;
	MV_PP2_PRS_ENTRY *pe = NULL;
	char name[PRS_TEXT_SIZE];

	pe = mvPrsDoubleVlanFind(tpid1, tpid2);

	if (pe == NULL) {

		/* Create new TCAM entry */

		tid = mvPp2PrsTcamFirstFree(PE_FIRST_FREE_TID, PE_LAST_FREE_TID);

		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_VLAN);
		pe->index = tid;

		/* set AI value for new double vlan entry */
		ai = mvPrsDblVlanAiFreeGet();

		if (ai == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: Can't add - number of Double vlan rules reached to maximum.\n", __func__);
			return MV_ERROR;
		}

		/* get first single/triple vlan tid */
		firstVlan = mvPpPrsVlanFirst();

		if (tid >= firstVlan) {
			/* double vlan entries overlapping*/
			mvOsPrintf("%s:Can't add entry, please remove unnecessary double vlans entries.\n", __func__);
			return MV_ERROR;
		}

		mvPrsDblVlanAiShadowSet(ai);

		mvPp2PrsMatchEtype(pe, 0, tpid1);
		mvPp2PrsMatchEtype(pe, 4, tpid2);

		/* Set AI value in SRAM for double vlan */
		status |= mvPp2PrsSwSramAiUpdate(pe, (ai | (1 << DBL_VLAN_AI_BIT)), SRAM_AI_MASK);

		/* Continue - set next lookup, */
		status |= mvPp2PrsSwSramNextLuSet(pe, PRS_LU_VLAN);

		/* Set result info bits */
		status |= mvPp2PrsSwSramRiUpdate(pe, RI_VLAN_DOUBLE, RI_VLAN_MASK);

		/* shift 8 bytes - skip 2 VLAN tags */
		status |= mvPp2PrsSwSramShiftSet(pe, 2 * MV_VLAN_HLEN, SRAM_OP_SEL_SHIFT_ADD);

		/* Update mvPrsShadowTbl */
		mvOsSPrintf(name, "double-VLAN-%d", ai);
		mvPp2PrsShadowSet(pe->index, PRS_LU_VLAN, name);

	}
	/* set ports bitmap*/
	status |= mvPp2PrsSwTcamPortMapSet(pe, portBmp);

	if (status == 0)
		mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return status;
}

int mvPp2PrsDoubleVlanDel(unsigned short tpid1, unsigned short tpid2)
{
	MV_PP2_PRS_ENTRY *pe = NULL;
	unsigned int ai, enable;


	pe = mvPrsDoubleVlanFind(tpid1, tpid2);

	if (pe == NULL) {
		/* No such entry */
		mvOsPrintf("Can't remove - No such entry\n");
		return MV_ERROR;
	}

	/* TODO - remove all corresponding vlan triples  */

	mvPp2PrsSwSramAiGet(pe, &ai, &enable);
	/* remove double vlan AI sign*/
	ai &= ~(1 << DBL_VLAN_AI_BIT);
	mvPrsDblVlanAiShadowClear(ai);

	/* remove entry */
	mvPp2PrsHwInv(pe->index);
	mvPp2PrsShadowClear(pe->index);
	mvPp2PrsSwFree(pe);
	return MV_OK;

}


int mvPp2PrsSingleVlan(unsigned short tpid, unsigned int portBmp, int add)
{
	if (add)
		return mvPp2PrsVlanAdd(tpid, SINGLE_VLAN_AI, portBmp);
	else
		return mvPp2PrsVlanDel(tpid, SINGLE_VLAN_AI);
}


int mvPp2PrsDoubleVlan(unsigned short tpid1, unsigned short tpid2, unsigned int portBmp, int add)
{
	if (add)
		return mvPp2PrsDoubleVlanAdd(tpid1, tpid2, portBmp);
	else
		return mvPp2PrsDoubleVlanDel(tpid1, tpid2);
}

int mvPp2PrsTripleVlan(unsigned short tpid1, unsigned short tpid2, unsigned short tpid3, unsigned int portBmp, int add)
{
	MV_PP2_PRS_ENTRY *pe;
	unsigned int ai, aiEnable;
	int status;

	pe = mvPrsDoubleVlanFind(tpid1, tpid2);

	if (!pe) {
		if (add)
			mvOsPrintf("User must enter first double vlan <0x%x,0x%x> before triple\n", tpid1, tpid2);
		else
			mvOsPrintf("Can't remove - No such entry\n");

		return MV_ERROR;
	}

	/* get AI value form double VLAN entry */
	mvPp2PrsSwSramAiGet(pe, &ai, &aiEnable);

	ai &= ~(1 << DBL_VLAN_AI_BIT);

	if (add)
		status = mvPp2PrsVlanAdd(tpid3, ai, portBmp);
	else
		status = mvPp2PrsVlanDel(tpid3, ai);

	mvPp2PrsSwFree(pe);

	return status;
}

/* Detect up to 2 successive VLAN tags:
 * Possible options:
 * 0x8100, 0x88A8
 * 0x8100, 0x8100
 * 0x8100
 * 0x88A8
 */
static int mvPp2PrsVlanInit(void)
{
	MV_PP2_PRS_ENTRY pe;

	mvPrsDblVlanAiShadowClearAll();

	/* double VLAN: 0x8100, 0x88A8 */
	if (mvPp2PrsDoubleVlan(MV_VLAN_TYPE, MV_VLAN_1_TYPE, PORT_MASK, 1))
		return MV_ERROR;

	/* double VLAN: 0x8100, 0x8100 */
	if (mvPp2PrsDoubleVlan(MV_VLAN_TYPE, MV_VLAN_TYPE, PORT_MASK, 1))
		return MV_ERROR;

	/* single VLAN: 0x88a8 */
	if (mvPp2PrsSingleVlan(MV_VLAN_1_TYPE, PORT_MASK, 1))
		return MV_ERROR;

	/* single VLAN: 0x8100 */
	if (mvPp2PrsSingleVlan(MV_VLAN_TYPE, PORT_MASK, 1))
		return MV_ERROR;

	/*---------------------------------*/
	/*  Set default double vlan entry  */
	/*---------------------------------*/
	mvPp2PrsSwClear(&pe);
	mvPp2PrsSwTcamLuSet(&pe, PRS_LU_VLAN);
	pe.index = PE_VLAN_DBL;
	/* Continue - set next lookup */
	mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_L2);

	/* double vlan AI bit */
	mvPp2PrsSwTcamAiUpdate(&pe, (1 << DBL_VLAN_AI_BIT), (1 << DBL_VLAN_AI_BIT));

	/* clear AI for next iterations */
	mvPp2PrsSwSramAiUpdate(&pe, 0, SRAM_AI_MASK);

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(&pe, RI_VLAN_DOUBLE, RI_VLAN_MASK);

	mvPp2PrsSwTcamPortMapSet(&pe, PORT_MASK);
	mvPp2PrsHwWrite(&pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe.index, PRS_LU_VLAN, "double-VLAN-accept");

	/*---------------------------------*/
	/*   Set default vlan none entry   */
	/*---------------------------------*/

	mvPp2PrsSwClear(&pe);
	mvPp2PrsSwTcamLuSet(&pe, PRS_LU_VLAN);
	pe.index = PE_VLAN_NONE;
	/* Continue - set next lookup */
	mvPp2PrsSwSramNextLuSet(&pe, PRS_LU_L2);

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(&pe, RI_VLAN_NONE, RI_VLAN_MASK);

	mvPp2PrsSwTcamPortMapSet(&pe, PORT_MASK);
	mvPp2PrsHwWrite(&pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe.index, PRS_LU_VLAN, "no-VLAN");

	return MV_OK;
}

/* remove all vlan entries */
int mvPp2PrsVlanAllDel(void)
{
	int tid;

	/* clear doublr Vlan shadow */
	mvPrsDblVlanAiShadowClearAll();

	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {
		if (mvPp2PrsShadowIsValid(tid) && (mvPp2PrsShadowLu(tid) == PRS_LU_VLAN)) {
			mvPp2PrsHwInv(tid);
			mvPp2PrsShadowClear(tid);
		}
	}

	return MV_OK;
}
/******************************************************************************
 *
 * Ethertype Section
 *
 ******************************************************************************
 */
/*TODO USE this function for all def etypres creation */
static int mvPrsEthTypeCreate(int portMap, unsigned short eth_type, unsigned int ri, unsigned int riMask)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;
	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(PE_FIRST_FREE_TID, PE_LAST_FREE_TID);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, eth_type);

	mvPp2PrsSwSramRiSet(pe, ri, riMask);
	mvPp2PrsSwTcamPortMapSet(pe, portMap);
	/* Continue - set next lookup */

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	mvPp2PrsSwSramFlowidGenSet(pe);

	mvPp2PrsHwWrite(pe);

	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-user-define");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_USER);
	mvPp2PrsShadowRiSet(pe->index, ri, riMask);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPrsEthTypeValid(unsigned int portMap, unsigned short ethertype)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int entryPmap;
	int tid;

	for (tid = PE_LAST_FREE_TID ; tid >= PE_FIRST_FREE_TID; tid--) {
		if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_L2))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		if (!mvPp2PrsEtypeEquals(&pe, 0, ethertype))
			continue;

		/* in default entries portmask must be 0xff */
		if ((mvPp2PrsShadowUdf(tid) == PRS_UDF_L2_DEF) && (portMap != PORT_MASK)) {
			mvOsPrintf("%s: operation not supported.\n", __func__);
			mvOsPrintf("%s: ports map must be 0xFF for default ether type\n", __func__);
			return MV_ERROR;

		} else {

			/* port maps cannot intersection in User entries*/
			/* PRS_UDF_L2_USER */
			mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);
			if ((portMap & entryPmap) && (portMap != entryPmap)) {
				mvOsPrintf("%s: operation not supported\n", __func__);
				mvOsPrintf("%s: user must delete portMap 0x%x from entry %d.\n",
					__func__, entryPmap & portMap, tid);
				return MV_ERROR;
			}
		}
	}
	return MV_OK;
}

int mvPrsEthTypeSet(int portMap, unsigned short ethertype, unsigned int ri, unsigned int riMask, MV_BOOL finish)
{
	MV_PP2_PRS_ENTRY pe;
	int tid;
	unsigned int entryPmap;
	MV_BOOL done = MV_FALSE;

	/* step 1 - validation */
	if (mvPrsEthTypeValid(portMap, ethertype))
		return MV_ERROR;


	/* step 2 - update TCAM */
	for (tid = PE_FIRST_FREE_TID ; tid <= PE_LAST_FREE_TID; tid++) {
		if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_L2))
			continue;

		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		if (!mvPp2PrsEtypeEquals(&pe, 0, ethertype))
			continue;

		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);

		if (entryPmap != portMap)
			continue;

		done = MV_TRUE;
		mvPp2PrsSwSramRiUpdate(&pe, ri, riMask);

		if ((mvPp2PrsShadowUdf(tid) == PRS_UDF_L2_USER) || finish)
			mvPp2PrsSwSramFlowidGenSet(&pe);

		mvPp2PrsHwWrite(&pe);
	}
	/* step 3 - Add new ethertype entry */
	if (!done)
		return mvPrsEthTypeCreate(portMap, ethertype, ri, riMask);

	return MV_OK;
}

int mvPrsEthTypeDel(int portMap, unsigned short ethertype)
{
	MV_PP2_PRS_ENTRY pe;
	unsigned int entryPmap;
	int tid;

	for (tid = PE_FIRST_FREE_TID; tid <= PE_LAST_FREE_TID; tid++) {

		if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_L2))
			continue;

		/* EtherType entry */
		pe.index = tid;
		mvPp2PrsHwRead(&pe);

		if (!mvPp2PrsEtypeEquals(&pe, 0, ethertype))
			continue;

		if (mvPp2PrsShadowUdf(tid) == PRS_UDF_L2_DEF) {
			if (portMap != PORT_MASK) {
				mvOsPrintf("%s: ports map must be 0xFF for default ether type\n", __func__);
				return MV_ERROR;
			}

			mvPp2PrsSwSramRiSet(&pe, mvPp2PrsShadowRi(tid), mvPp2PrsShadowRiMask(tid));

			if (!mvPp2PrsShadowFin(tid))
				mvPp2PrsSwSramFlowidGenClear(&pe);

			mvPp2PrsHwWrite(&pe);

			continue;
		}

		/*PRS_UDF_L2_USER */

		mvPp2PrsSwTcamPortMapGet(&pe, &entryPmap);
		/*mask ports in user entry */
		entryPmap &= ~portMap;

		if (entryPmap == 0) {
			mvPp2PrsHwInv(tid);
			mvPp2PrsShadowClear(tid);
			continue;
		}

		mvPp2PrsSwTcamPortMapSet(&pe, entryPmap);
		mvPp2PrsHwWrite(&pe);
	}
	return MV_OK;
}

static int mvPp2PrsEtypePppoe(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_PPPOE_TYPE);
	mvPp2PrsSwSramShiftSet(pe, MV_PPPOE_HDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_PPPOE);
	mvPp2PrsSwSramRiSetBit(pe, RI_PPPOE_BIT);
	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-PPPoE");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowRiSet(pe->index, RI_PPPOE_MASK, RI_PPPOE_MASK);
	mvPp2PrsShadowFinSet(pe->index, MV_FALSE);
	mvPp2PrsSwFree(pe);

	return MV_OK;
}


/* match ip4 and ihl == 5 */
static int mvPp2PrsEtypeIp4(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* IPv4 without options */
	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);
	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP_TYPE);
	mvPp2PrsSwTcamByteSet(pe, MV_ETH_TYPE_LEN + 0, 0x45, 0xff);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP4, RI_L3_PROTO_MASK);

	/* Skip eth_type + 4 bytes of IP header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 4, SRAM_OP_SEL_SHIFT_ADD);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-ipv4");
	mvPp2PrsShadowFinSet(pe->index, MV_FALSE);
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowRiSet(pe->index, RI_L3_IP4, RI_L3_PROTO_MASK);
	mvPp2PrsSwFree(pe);

	/* IPv4 with options */
	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;
	mvPp2PrsMatchEtype(pe, 0, MV_IP_TYPE);
	mvPp2PrsSwTcamByteSet(pe, MV_ETH_TYPE_LEN + 0, 0x40, 0xf0);

	/* Skip eth_type + 4 bytes of IP header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 4, SRAM_OP_SEL_SHIFT_ADD);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP4_OPT, RI_L3_PROTO_MASK);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-ipv4-opt");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowFinSet(pe->index, MV_FALSE);
	mvPp2PrsShadowRiSet(pe->index, RI_L3_IP4_OPT, RI_L3_PROTO_MASK);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPp2PrsEtypeArp(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}


	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP_ARP_TYPE);

	/* generate flow in the next iteration*/
	/*mvPp2PrsSwSramAiSetBit(pe, AI_DONE_BIT);*/
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_ARP, RI_L3_PROTO_MASK);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-arp");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowFinSet(pe->index, MV_TRUE);
	mvPp2PrsShadowRiSet(pe->index, RI_L3_ARP, RI_L3_PROTO_MASK);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPp2PrsEtypeLbdt(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP_LBDT_TYPE);

	/* generate flow in the next iteration*/
	/*mvPp2PrsSwSramAiSetBit(pe, AI_DONE_BIT);*/
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramRiUpdate(pe, RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-lbdt");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowFinSet(pe->index, MV_TRUE);
	mvPp2PrsShadowRiSet(pe->index, RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* match ip6 */
static int mvPp2PrsEtypeIp6(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_L2);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP6_TYPE);

	/* Skip DIP of IPV6 header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 8 + MV_MAX_L3_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);
/*
	there is no support in extension yet
 */
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP6, RI_L3_PROTO_MASK);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-ipv6");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowFinSet(pe->index, MV_FALSE);
	mvPp2PrsShadowRiSet(pe->index, RI_L3_IP6, RI_L3_PROTO_MASK);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}


/* unknown ethertype */
static int mvPp2PrsEtypeUn(void)
{
	MV_PP2_PRS_ENTRY *pe;

	/* Default entry for PRS_LU_L2 - Unknown ethtype */
	pe = mvPp2PrsSwAlloc(PRS_LU_L2);
	pe->index = PE_ETH_TYPE_UN;

	/* generate flow in the next iteration*/
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_UN, RI_L3_PROTO_MASK);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* set L3 offset even it's unknown L3 */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_L2, "etype-unknown");
	mvPp2PrsShadowUdfSet(pe->index, PRS_UDF_L2_DEF);
	mvPp2PrsShadowFinSet(pe->index, MV_TRUE);
	mvPp2PrsShadowRiSet(pe->index, RI_L3_UN, RI_L3_PROTO_MASK);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/*
 * pnc_etype_init - match basic ethertypes
 */
static int mvPp2PrsEtypeInit(void)
{
	int rc;

	PRS_DBG("%s\n", __func__);

	rc = mvPp2PrsEtypePppoe();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeArp();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeLbdt();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeIp4();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeIp6();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeUn();
	if (rc)
		return rc;

	return MV_OK;
}

/******************************************************************************
 *
 * PPPoE Section
 *
 ******************************************************************************
 */

static int mvPp2PrsIpv6Pppoe(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_PPPOE);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP6_PPP);

	/* there is no support in extension yet */
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP6, RI_L3_PROTO_MASK);

	/* Skip DIP of IPV6 header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 8 + MV_MAX_L3_ADDR_SIZE, SRAM_OP_SEL_SHIFT_ADD);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);
	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_PPPOE, "Ipv6-over-PPPoE");

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPp2PrsIpv4Pppoe(void)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/** ipV4 over PPPoE without options **/

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_PPPOE);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP_PPP);
	mvPp2PrsSwTcamByteSet(pe, MV_ETH_TYPE_LEN + 0, 0x45, 0xff);

	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP4, RI_L3_PROTO_MASK);

	/* Skip eth_type + 4 bytes of IP header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 4, SRAM_OP_SEL_SHIFT_ADD);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_PPPOE, "Ipv4-over-PPPoE");

	mvPp2PrsSwFree(pe);


	/** ipV4 over PPPoE with options **/

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_PPPOE);

	pe->index = tid;

	mvPp2PrsMatchEtype(pe, 0, MV_IP_PPP);

	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP4_OPT, RI_L3_PROTO_MASK);

	/* Skip eth_type + 4 bytes of IP header */
	mvPp2PrsSwSramShiftSet(pe, MV_ETH_TYPE_LEN + 4, SRAM_OP_SEL_SHIFT_ADD);

	/* set L3 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_PPPOE, "Ipv4-over-PPPoE-opt");
	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* Create entry for non-ip over PPPoE (for PPP LCP, IPCPv4, IPCPv6, etc) */
static int mvPp2PrsNonipPppoe(void)
{
	int ret;
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/** non-Ip over PPPoE **/

	/* Go through the all entires from first to last */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Can't add - No free TCAM entries */
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_PPPOE);
	PTR_VALIDATE(pe);

	pe->index = tid;

	ret = mvPp2PrsSwSramRiUpdate(pe, RI_L3_UN, RI_L3_PROTO_MASK);
	RET_VALIDATE(ret);

	ret = mvPp2PrsSwSramFlowidGenSet(pe);
	RET_VALIDATE(ret);

	/* set L3 offset even it's unknown L3 */
	ret = mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L3, MV_ETH_TYPE_LEN, SRAM_OP_SEL_OFFSET_ADD);
	RET_VALIDATE(ret);

	ret = mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	RET_VALIDATE(ret);

	ret = mvPp2PrsHwWrite(pe);
	RET_VALIDATE(ret);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_PPPOE, "NonIP-over-PPPoE");
	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* match etype = PPPOE */
static int mvPp2PrsPppeInit(void)
{
	int rc;

	rc = mvPp2PrsIpv4Pppoe();
	if (rc)
		return rc;

	rc = mvPp2PrsIpv6Pppoe();
	if (rc)
		return rc;

	rc = mvPp2PrsNonipPppoe();
	if (rc)
		return rc;

	return MV_OK;
}



/******************************************************************************
 *
 * IPv4 Section
 *
 ******************************************************************************
 */

/* IPv4/TCP header parsing for fragmentation and L4 offset.  */
static int mvPp2PrsIp4Proto(unsigned short proto, unsigned int ri, unsigned int riMask)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* TCP, Not Fragmented */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP4);
	pe->index = tid;

	mvPp2PrsSwTcamByteSet(pe, 2, 0x00, 0x3f);
	mvPp2PrsSwTcamByteSet(pe, 3, 0x00, 0xff);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	mvPp2PrsSwTcamByteSet(pe, 5, proto, 0xff);

	if (proto == MV_IP_PROTO_TCP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-tcp");
	} else if (proto == MV_IP_PROTO_UDP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-udp");
	} else if (proto == MV_IP_PROTO_IGMP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-igmp");
	} else {
		mvOsPrintf("%s: IPv4 unsupported protocol %d\n", __func__, proto);
		mvPp2PrsSwFree(pe);
		return MV_ERROR;
	}

	/* set L4 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, sizeof(MV_IP_HEADER) - 4, SRAM_OP_SEL_OFFSET_ADD);
	mvPp2PrsSwSramShiftSet(pe, 12, SRAM_OP_SEL_SHIFT_ADD);

	/* Next: go to IPV4 */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	/* Set sram AIbits */
	mvPp2PrsSwSramAiUpdate(pe, (1 << IPV4_DIP_AI_BIT), (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* AI bits check */
	mvPp2PrsSwTcamAiUpdate(pe, 0, (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	/* TCP, Fragmented */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP4);
	pe->index = tid;

	mvPp2PrsSwTcamByteSet(pe, 5, proto, 0xff);
	mvPp2PrsSwSramRiSetBit(pe, RI_IP_FRAG_BIT);

	if (proto == MV_IP_PROTO_TCP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-tcp-frag");
	} else if (proto == MV_IP_PROTO_UDP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-udp-frag");
	} else if (proto == MV_IP_PROTO_IGMP) {
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-igmp-frag");
	} else {
		mvOsPrintf("%s: IPv4 unsupported protocol %d\n", __func__, proto);
		mvPp2PrsSwFree(pe);
		return MV_ERROR;
	}

	/* set L4 offset */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, sizeof(MV_IP_HEADER) - 4, SRAM_OP_SEL_OFFSET_ADD);
	mvPp2PrsSwSramShiftSet(pe, 12, SRAM_OP_SEL_SHIFT_ADD);

	/* Next: go to IPV4 */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	/* Set sram AIbits */
	mvPp2PrsSwSramAiUpdate(pe, (1 << IPV4_DIP_AI_BIT), (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	/* AI bits check */
	mvPp2PrsSwTcamAiUpdate(pe, 0, (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* IPv4 L3 Multicast or borad cast.  1-MC, 2-BC */
static int mvPp2PrsIp4Cast(unsigned short l3_cast)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	if (l3_cast != L3_MULTI_CAST &&
	    l3_cast != L3_BROAD_CAST) {
		mvOsPrintf("%s: Invalid Input\n", __func__);
		return MV_ERROR;
	}

	/* Get free entry */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP4);
	pe->index = tid;

	if (l3_cast == L3_MULTI_CAST) {
		mvPp2PrsSwTcamByteSet(pe, 0, 0xE0, 0xE0);
		mvPp2PrsSwSramRiUpdate(pe, RI_L3_MCAST, RI_L3_ADDR_MASK);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-mc");
	} else if (l3_cast == L3_BROAD_CAST) {
		mvPp2PrsSwTcamByteSet(pe, 0, 0xFF, 0xFF);
		mvPp2PrsSwTcamByteSet(pe, 1, 0xFF, 0xFF);
		mvPp2PrsSwTcamByteSet(pe, 2, 0xFF, 0xFF);
		mvPp2PrsSwTcamByteSet(pe, 3, 0xFF, 0xFF);
		mvPp2PrsSwSramRiUpdate(pe, RI_L3_BCAST, RI_L3_ADDR_MASK);
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-bc");
	}

	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV4_DIP_AI_BIT), (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* Finished: go to flowid generation */
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}


static int mvPp2PrsIp4Init(void)
{
	int rc;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* Set entries for TCP, UDP and IGMP over IPv4 */
	rc = mvPp2PrsIp4Proto(MV_IP_PROTO_TCP, RI_L4_TCP, RI_L4_PROTO_MASK);
	if (rc)
		return rc;

	rc = mvPp2PrsIp4Proto(MV_IP_PROTO_UDP, RI_L4_UDP, RI_L4_PROTO_MASK);
	if (rc)
		return rc;

	/* IPv4 Broadcast */
	rc = mvPp2PrsIp4Cast(L3_BROAD_CAST);
	if (rc)
		return rc;

	/* IPv4 Multicast */
	rc = mvPp2PrsIp4Cast(L3_MULTI_CAST);
	if (rc)
		return rc;

	rc = mvPp2PrsIp4Proto(MV_IP_PROTO_IGMP, RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK);
	if (rc)
		return rc;

	/* Default IPv4 entry for unknown protocols */
	pe = mvPp2PrsSwAlloc(PRS_LU_IP4);
	pe->index = PE_IP4_PROTO_UN;

	/* Next: go to IPV4 */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP4);
	/* Set sram AIbits */
	mvPp2PrsSwSramAiUpdate(pe, (1 << IPV4_DIP_AI_BIT), (1 << IPV4_DIP_AI_BIT));

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, RI_L4_OTHER, RI_L4_PROTO_MASK);

	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, sizeof(MV_IP_HEADER) - 4, SRAM_OP_SEL_OFFSET_ADD);
	mvPp2PrsSwSramShiftSet(pe, 12, SRAM_OP_SEL_SHIFT_ADD);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* AI bits check */
	mvPp2PrsSwTcamAiUpdate(pe, 0, (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-l4-unknown");

	mvPp2PrsSwFree(pe);

	/* Default IPv4 entry for unit cast address */
	pe = mvPp2PrsSwAlloc(PRS_LU_IP4);
	pe->index = PE_IP4_ADDR_UN;

	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV4_DIP_AI_BIT), (1 << IPV4_DIP_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	mvPp2PrsSwSramRiUpdate(pe, RI_L3_UCAST, RI_L3_ADDR_MASK);
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv4-uc");

	/* Finished: go to flowid generation */
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/******************************************************************************
 *
 * IPv6 Section
 *
 *******************************************************************************/
/* TODO continue from here */
/* IPv6 - detect TCP */

static int mvPp2PrsIp6Proto(unsigned short proto, unsigned int ri, unsigned int riMask, MV_BOOL ip6_ext)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* TCP, Not Fragmented */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = tid;

	/* Match Protocol */
	mvPp2PrsSwTcamByteSet(pe, 0, proto, 0xff);

	/* Set Rule Shadow */
	switch (proto) {
	/* TCP */
	case MV_IP_PROTO_TCP:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-tcp" : "ipv6-ext-tcp");
		break;

	/* UDP */
	case MV_IP_PROTO_UDP:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-udp" : "ipv6-ext-udp");
		break;

	/* ICMP */
	case MV_IP_PROTO_ICMPV6:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-icmp" : "ipv6-ext-icmp");
		break;

	/* IPv4, for IPv6 DS Lite */
	case MV_IP_PROTO_IPIP:
		if (ip6_ext != MV_FALSE) {
			mvOsPrintf("%s: IPv4 header not a IP6 extension header\n", __func__);
			mvPp2PrsSwFree(pe);
			return MV_ERROR;
		}
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-lite-ip4");
		break;

	/* IPV6 Extension Header */

	/* Hop-by-Hop Options Header, Dummy protocol for TCP */
	case MV_IP_PROTO_NULL:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-hh" : "ipv6-ext-nh-hh");
		break;

	/* Encapsulated IPv6 Header, IPv6-in-IPv4 tunnelling */
	case MV_IP_PROTO_IPV6:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-eh" : "ipv6-ext-nh-eh");
		break;

	/* Route header */
	case MV_IP_PROTO_RH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-rh" : "ipv6-ext-nh-rh");
		break;

	/* Fragment Header */
	case MV_IP_PROTO_FH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-fh" : "ipv6-ext-nh-fh");
		break;
#if 0
	/* Encapsulation Security Payload protocol */
	case MV_IP_PROTO_ESP:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-esp" : "ipv6-ext-nh-esp");
		break;
#endif
	/* Authentication Header */
	case MV_IP_PROTO_AH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-ah" : "ipv6-ext-nh-ah");
		break;

	/* Destination Options Header */
	case MV_IP_PROTO_DH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-dh" : "ipv6-ext-nh-dh");
		break;

	/* Mobility Header */
	case MV_IP_PROTO_MH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, (MV_FALSE == ip6_ext) ? "ipv6-nh-mh" : "ipv6-ext-nh-mh");
		break;

	default:
		mvOsPrintf("%s: IPv6 unsupported protocol %d\n", __func__, proto);
		mvPp2PrsSwFree(pe);
		return MV_ERROR;
	}

	/* Set TCAM and SRAM for TCP, UDP and IGMP */
	if (proto == MV_IP_PROTO_TCP ||
	    proto == MV_IP_PROTO_UDP ||
	    proto == MV_IP_PROTO_ICMPV6 ||
	    proto == MV_IP_PROTO_IPIP) {
		/* Set TCAM AI */
		if (MV_FALSE == ip6_ext)
			mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));
		else
			mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AI_BIT), (1 << IPV6_EXT_AI_BIT));

		/* Set result info */
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);

		/* set L4 offset relatively to our current place */
		if (MV_FALSE == ip6_ext)
			mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, sizeof(MV_IP6_HEADER) - 6, SRAM_OP_SEL_OFFSET_ADD);
		else
			mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, 0, SRAM_OP_SEL_OFFSET_IP6_ADD);

		/* Finished: go to LU Generation */
		mvPp2PrsSwSramFlowidGenSet(pe);
		mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	} else { /* Set TCAM and SRAM for IPV6 Extension Header */
		if (MV_FALSE == ip6_ext) { /* Case 1: xx is first NH of IPv6 */
			/* Skip to NH */
			mvPp2PrsSwSramShiftSet(pe, 34, SRAM_OP_SEL_SHIFT_ADD);

			/* Set AI bit */
			mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));
			/* update UDF2 */
			mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_IPV6_PROTO, 0, SRAM_OP_SEL_SHIFT_ADD);
		} else { /* Case 2: xx is not first NH of IPv6 */
			/* Skip to NH */
			mvPp2PrsSwSramShiftSet(pe, 0, SRAM_OP_SEL_SHIFT_IP6_ADD);

			/* Set AI bit */
			mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AI_BIT), (1 << IPV6_EXT_AI_BIT));
			/* update UDF2 */
			mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_IPV6_PROTO, 0, SRAM_OP_SEL_SHIFT_IP6_ADD);
		}

		/* Next LU */
		mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);

		/* Set sram AIbits */
		if (proto == MV_IP_PROTO_AH) {
			mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_EXT_AH_AI_BIT), (1 << IPV6_EXT_AH_AI_BIT));
			mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AI_BIT));
			mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));
		} else {
			mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_AI_BIT));
			mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_EXT_AI_BIT), (1 << IPV6_EXT_AI_BIT));
			mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));
		}

		/* Set RI, IPv6 Ext */
		mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP6_EXT, RI_L3_PROTO_MASK);
	}

	/* All ports */
	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* Write HW */
	mvPp2PrsHwWrite(pe);

	/* Release SW entry */
	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* Parse the extension header on AH */
static int mvPp2PrsIp6ProtoAh(unsigned short proto, unsigned int ri, unsigned int riMask)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	/* TCP, Not Fragmented */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = tid;

	/* Match Protocol */
	mvPp2PrsSwTcamByteSet(pe, 0, proto, 0xff);

	/* Set Rule Shadow */
	switch (proto) {
	/* TCP */
	case MV_IP_PROTO_TCP:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-tcp");
		break;

	/* UDP */
	case MV_IP_PROTO_UDP:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-udp");
		break;

	/* ICMP */
	case MV_IP_PROTO_ICMPV6:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-icmp");
		break;

	/* IPV6 Extension Header */

	/* Hop-by-Hop Options Header, Dummy protocol for TCP */
	case MV_IP_PROTO_NULL:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-hh");
		break;

	/* Encapsulated IPv6 Header, IPv6-in-IPv4 tunnelling */
	case MV_IP_PROTO_IPV6:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-eh");
		break;

	/* Route header */
	case MV_IP_PROTO_RH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-rh");
		break;

	/* Fragment Header */
	case MV_IP_PROTO_FH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-fh");
		break;

	/* Authentication Header */
	case MV_IP_PROTO_AH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-ah");
		break;

	/* Destination Options Header */
	case MV_IP_PROTO_DH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-dh");
		break;

	/* Mobility Header */
	case MV_IP_PROTO_MH:
		mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-ah-nh-mh");
		break;

	default:
		mvOsPrintf("%s: IPv6 unsupported extension header %d\n", __func__, proto);
		mvPp2PrsSwFree(pe);
		return MV_ERROR;
	}

	/* Set TCAM and SRAM for TCP, UDP and IGMP */
	if (proto == MV_IP_PROTO_TCP ||
	    proto == MV_IP_PROTO_UDP ||
	    proto == MV_IP_PROTO_ICMPV6) {
		/* Set result info */
		mvPp2PrsSwSramRiUpdate(pe, ri, riMask);
		/* Set sram AIbits */
		mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_EXT_AH_L4_AI_BIT), (1 << IPV6_EXT_AH_L4_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_LEN_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));
	} else {
		/* Set sram AIbits */
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_L4_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_EXT_AH_LEN_AI_BIT), (1 << IPV6_EXT_AH_LEN_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));
	}

	/* Set AI bit */
	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AH_AI_BIT), (1 << IPV6_EXT_AH_AI_BIT));

	/* Next LU */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);

	/* Set RI, IPv6 Ext */
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_IP6_EXT, RI_L3_PROTO_MASK);

	/* All ports */
	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* Write HW */
	mvPp2PrsHwWrite(pe);

	/* Release SW entry */
	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* Parse AH length field */
static int mvPp2PrsIp6AhLen(unsigned char ah_len, MV_BOOL l4_off_set)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;
	char tmp_buf[15];

	PRS_DBG("%s\n", __func__);

	/* TCP, Not Fragmented */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = tid;

	/* Match AH Len */
	mvPp2PrsSwTcamByteSet(pe, 1, ah_len, 0xff);

	/* Set Rule Shadow */
	sprintf(tmp_buf, "ipv6-ah-len%d", ah_len);
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, tmp_buf);

	/* Set AI bit */
	if (l4_off_set) {
		mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AH_L4_AI_BIT), (1 << IPV6_EXT_AH_L4_AI_BIT));
		/* Set L4 offset */
		mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4,
					(IPV6_EXT_EXCLUDE_BYTES + ah_len * IPV6_EXT_AH_UNIT_BYTES),
					SRAM_OP_SEL_OFFSET_LKP_ADD);
		/* Finished: go to LU Generation */
		mvPp2PrsSwSramFlowidGenSet(pe);
		mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	} else {
		mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AH_LEN_AI_BIT), (1 << IPV6_EXT_AH_LEN_AI_BIT));
		/* Set sram AIbits */
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_L4_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_LEN_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_EXT_AH_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_EXT_AI_BIT), (1 << IPV6_EXT_AI_BIT));
		mvPp2PrsSwSramAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));

		/* Skip to NH */
		mvPp2PrsSwSramShiftSet(pe, (IPV6_EXT_EXCLUDE_BYTES + ah_len * IPV6_EXT_AH_UNIT_BYTES), SRAM_OP_SEL_SHIFT_ADD);

		/* Next LU */
		mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);
	}

	/* All ports */
	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* Write HW */
	mvPp2PrsHwWrite(pe);

	/* Release SW entry */
	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* IPv6 L3 Multicast or borad cast.  1-MC */
static int mvPp2PrsIp6Cast(unsigned short l3_cast)
{
	int tid;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	if (l3_cast != L3_MULTI_CAST) {
		mvOsPrintf("%s: Invalid Input\n", __func__);
		return MV_ERROR;
	}

	/* Get free entry */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		return MV_ERROR;
	}
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = tid;

	mvPp2PrsSwTcamByteSet(pe, 0, 0xFF, 0xFF);
	mvPp2PrsSwSramRiUpdate(pe, RI_L3_MCAST, RI_L3_ADDR_MASK);
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-mc");

	mvPp2PrsSwTcamAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* Set sram AIbits */
	mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));

	/* Shift back to IPV6 NH */
	mvPp2PrsSwSramShiftSet(pe, -18, SRAM_OP_SEL_SHIFT_ADD);

	/* Finished: go to flowid generation */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

static int mvPp2PrsIp6Init(void)
{
	int tid, rc;
	MV_PP2_PRS_ENTRY *pe;

	PRS_DBG("%s\n", __func__);

	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);

	/* Check hop limit */
	tid = mvPp2PrsTcamFirstFree(0, MV_PP2_PRS_TCAM_SIZE - 1);
	if (tid == MV_PRS_OUT_OF_RAGE) {
		mvOsPrintf("%s: No free TCAM entiry\n", __func__);
		mvPp2PrsSwFree(pe);
		return MV_ERROR;
	}
	pe->index = tid;

	mvPp2PrsSwTcamByteSet(pe, 1, 0x00, 0xff);
	mvPp2PrsSwSramRiUpdate(pe, (RI_L3_UN | RI_DROP_BIT), (RI_L3_PROTO_MASK | RI_DROP_MASK));

	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);
	/* Update TCAM AI */
	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv6-hop-zero");

	mvPp2PrsSwFree(pe);

	/* Set entries for TCP and UDP over IPv6 */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_TCP,
			      RI_L4_TCP,
			      RI_L4_PROTO_MASK,
			      MV_FALSE);
	if (rc)
		return rc;

	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_UDP,
			      RI_L4_UDP,
			      RI_L4_PROTO_MASK,
			      MV_FALSE);
	if (rc)
		return rc;

	/* IPv6 Multicast */
	rc = mvPp2PrsIp6Cast(L3_MULTI_CAST);
	if (rc)
		return rc;

	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_ICMPV6,
			      RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK,
			      MV_FALSE);
	if (rc)
		return rc;

	/* IPv4 is the last header. This is similar case as 6-TCP or 17-UDP */
	/* Result Info: UDF7=1, DS lite */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_IPIP, RI_UDF7_IP6_LITE, RI_UDF7_MASK, MV_FALSE);
	if (rc)
		return rc;

	/* Default IPv6 entry for unknown protocols */
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = PE_IP6_PROTO_UN;

	/* generate flow in the next iteration*/
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, RI_L4_OTHER, RI_L4_PROTO_MASK);

	/* set L4 offset relatively to our current place */
	mvPp2PrsSwSramOffsetSet(pe, SRAM_OFFSET_TYPE_L4, sizeof(MV_IP6_HEADER) - 4, SRAM_OP_SEL_OFFSET_ADD);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* AI bits check */
	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv6-l4-unknown");

	mvPp2PrsSwFree(pe);

	/* Default IPv6 entry for unknown Ext protocols */
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = PE_IP6_EXT_PROTO_UN;

	/* Finished: go to LU Generation */
	mvPp2PrsSwSramFlowidGenSet(pe);
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_FLOWS);

	/* Set result info bits */
	mvPp2PrsSwSramRiUpdate(pe, RI_L4_OTHER, RI_L4_PROTO_MASK);

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	/* AI bits check */
	mvPp2PrsSwTcamAiUpdate(pe, (1 << IPV6_EXT_AI_BIT), (1 << IPV6_EXT_AI_BIT));

	mvPp2PrsHwWrite(pe);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP4, "ipv6-ext-l4-unknown");

	mvPp2PrsSwFree(pe);

	/* Default IPv6 entry for unit cast address */
	pe = mvPp2PrsSwAlloc(PRS_LU_IP6);
	pe->index = PE_IP6_ADDR_UN;

	mvPp2PrsSwTcamAiUpdate(pe, 0, (1 << IPV6_NO_EXT_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);

	mvPp2PrsSwSramRiUpdate(pe, RI_L3_UCAST, RI_L3_ADDR_MASK);
	mvPp2PrsShadowSet(pe->index, PRS_LU_IP6, "ipv6-uc");

	/* Finished: go to IPv6 again */
	mvPp2PrsSwSramNextLuSet(pe, PRS_LU_IP6);

	/* Shift back to IPV6 NH */
	mvPp2PrsSwSramShiftSet(pe, -18, SRAM_OP_SEL_SHIFT_ADD);

	/* Set sram AIbits */
	mvPp2PrsSwSramAiUpdate(pe, (1 << IPV6_NO_EXT_AI_BIT), (1 << IPV6_NO_EXT_AI_BIT));

	mvPp2PrsSwTcamPortMapSet(pe, PORT_MASK);
	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;
}

/* Add IPv6 Next Header parse rule set */
int mvPrsIp6NhSet(void)
{
	int rc;
	unsigned char ah_len = 0;

	/* Hop-by-Hop Options Header, Dummy protocol for TCP */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_NULL, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_NULL, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* Encapsulated IPv6 Header, IPv6-in-IPv4 tunnelling */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_IPV6, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_IPV6, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* Route header */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_RH, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_RH, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* Fragment Header */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_FH, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_FH, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* Authentication Header */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_AH, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_AH, 0, 0, MV_TRUE);
	if (rc)
		return rc;
	/* Check NH on AH header */
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_NULL, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_IPV6, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_RH, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_FH, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_DH, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_MH, 0, 0);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_TCP, RI_L4_TCP, RI_L4_PROTO_MASK);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_UDP, RI_L4_UDP, RI_L4_PROTO_MASK);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6ProtoAh(MV_IP_PROTO_ICMPV6, RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK);
	if (rc)
		return rc;
	/* Check AH length */
	for (ah_len = IP6_AH_LEN_16B; ah_len < IP6_AH_LEN_MAX; ah_len++) {
		rc = mvPp2PrsIp6AhLen(ah_len, MV_FALSE);
		if (rc)
			return rc;
		/* Set L4 offset */
		rc = mvPp2PrsIp6AhLen(ah_len, MV_TRUE);
		if (rc)
			return rc;
	}

	/* Destination Options Header */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_DH, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_DH, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* Mobility Header */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_MH, 0, 0, MV_FALSE);
	if (rc)
		return rc;
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_MH, 0, 0, MV_TRUE);
	if (rc)
		return rc;

	/* L4 parse */
	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_TCP,
			      RI_L4_TCP,
			      RI_L4_PROTO_MASK,
			      MV_TRUE);
	if (rc)
		return rc;

	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_UDP,
			      RI_L4_UDP,
			      RI_L4_PROTO_MASK,
			      MV_TRUE);
	if (rc)
		return rc;

	rc = mvPp2PrsIp6Proto(MV_IP_PROTO_ICMPV6,
			      RI_CPU_CODE_RX_SPEC | RI_UDF3_RX_SPECIAL, RI_CPU_CODE_MASK | RI_UDF3_MASK,
			      MV_TRUE);
	if (rc)
		return rc;

	return MV_OK;
}

/*
 ******************************************************************************
 *
 * flows
 *
 ******************************************************************************
 */

static MV_PP2_PRS_ENTRY *mvPrsFlowFind(int flow)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	unsigned int bits, enable;

	pe = mvPp2PrsSwAlloc(PRS_LU_FLOWS);

	/* Go through the all entires with PRS_LU_MAC */
	for (tid = MV_PP2_PRS_TCAM_SIZE - 1; tid >= 0; tid--) {
		if ((!mvPp2PrsShadowIsValid(tid)) || (mvPp2PrsShadowLu(tid) != PRS_LU_FLOWS))
			continue;

		pe->index = tid;
		mvPp2PrsHwRead(pe);
		mvPp2PrsSwSramAiGet(pe, &bits, &enable);

		/* sram store classification lookup id in AI bits [5:0] */
		if ((bits & FLOWID_MASK) == flow)
			return pe;
	}
	mvPp2PrsSwFree(pe);
	return NULL;
}

int mvPrsFlowIdGen(int tid, int flowId, unsigned int res, unsigned int resMask, int portBmp)
{
	MV_PP2_PRS_ENTRY *pe;
	char name[PRS_TEXT_SIZE];

	PRS_DBG("%s\n", __func__);

	POS_RANGE_VALIDATE(flowId, FLOWID_MASK);
	POS_RANGE_VALIDATE(tid, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Default configuration entry - overrwite is forbidden */
	if (mvPp2PrsShadowIsValid(tid) && (mvPp2PrsShadowLu(tid) != PRS_LU_FLOWS)) {
		mvOsPrintf("%s: Error, Tcam entry is in use\n", __func__);
		return MV_ERROR;
	}

	pe = mvPp2PrsSwAlloc(PRS_LU_FLOWS);
	pe->index = tid;

	mvPp2PrsSwSramAiUpdate(pe, flowId, FLOWID_MASK);
	mvPp2PrsSwSramLuDoneSet(pe);

	mvOsSPrintf(name, "flowId-%d", flowId);

	/* Update mvPrsShadowTbl */
	mvPp2PrsShadowSet(pe->index, PRS_LU_FLOWS, name);

	mvPp2PrsSwTcamPortMapSet(pe, portBmp);

	/*update result data and mask*/
	mvPp2PrsSwTcamWordSet(pe, TCAM_DATA_OFFS, res, resMask);

	mvPp2PrsHwWrite(pe);

	mvPp2PrsSwFree(pe);

	return MV_OK;

}

int mvPrsFlowIdDel(int tid)
{
	PRS_DBG("%s\n", __func__);

	POS_RANGE_VALIDATE(tid, MV_PP2_PRS_TCAM_SIZE - 1);

	/* Only handle valid flow type rule */
	if (!mvPp2PrsShadowIsValid(tid) || (mvPp2PrsShadowLu(tid) != PRS_LU_FLOWS)) {
		mvOsPrintf("%s: Error, Tcam entry is not use or not flow type\n", __func__);
		return MV_ERROR;
	}

	mvPp2PrsHwInv(tid);
	mvPp2PrsShadowClear(tid);

	return MV_OK;

}

int mvPrsFlowIdFirstFreeGet(void)
{
	int fid;

	for (fid = MV_PP2_PRS_FIRST_FLOW_ID; fid <= MV_PP2_PRS_LAST_FLOW_ID; fid++)
		if (mvPrsFlowIdGet(fid) == MV_FALSE)
			break;

	if (fid <= MV_PP2_PRS_LAST_FLOW_ID)
		return fid;

	return MV_PP2_PRS_INVALID_FLOW_ID;
}

int mvPrsFlowIdLastFreeGet(void)
{
	int fid;

	for (fid = MV_PP2_PRS_LAST_FLOW_ID; fid >= MV_PP2_PRS_FIRST_FLOW_ID; fid--)
		if (mvPrsFlowIdGet(fid) == MV_FALSE)
			break;

	if (fid >=  MV_PP2_PRS_FIRST_FLOW_ID)
		return fid;

	return MV_PP2_PRS_INVALID_FLOW_ID;
}

int mvPrsFlowIdRelease(int flowId)
{
	POS_RANGE_VALIDATE(flowId, FLOWID_MASK);

	mvPrsFlowIdClear(flowId);

	return MV_OK;
}

int mvPrsDefFlow(int port)
{

	MV_PP2_PRS_ENTRY *pe;
	int tid, mallocFlag = 0;
	char name[PRS_TEXT_SIZE];

	PRS_DBG("%s\n", __func__);

	POS_RANGE_VALIDATE(port, MV_PP2_MAX_PORTS - 1);

	pe = mvPrsFlowFind(FLOWID_DEF(port));

	/* Such entry not exist */
	if (!pe) {
		/* Go through the all entires from last to fires */
		tid = mvPp2PrsTcamFirstFree(MV_PP2_PRS_TCAM_SIZE - 1, 0);

		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			return MV_ERROR;
		}

		mallocFlag = 1;
		pe = mvPp2PrsSwAlloc(PRS_LU_FLOWS);
		pe->index = tid;

		/* set flowID*/
		mvPp2PrsSwSramAiUpdate(pe, FLOWID_DEF(port), FLOWID_MASK);
		mvPrsFlowIdSet(FLOWID_DEF(port));
		mvPp2PrsSwSramLuDoneSet(pe);

		mvOsSPrintf(name, "def-flowId-port-%d", port);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe->index, PRS_LU_FLOWS, name);

	}

	mvPp2PrsSwTcamPortMapSet(pe, (1 << port));

	mvPp2PrsHwWrite(pe);

	if (mallocFlag)
		mvPp2PrsSwFree(pe);

	return MV_OK;
}

int mvPrsDefFlowInit(void)
{
	MV_PP2_PRS_ENTRY *pe;
	int tid;
	int port;
	char name[PRS_TEXT_SIZE];

	PRS_DBG("%s\n", __func__);

	for (port = 0; port < MV_PP2_MAX_PORTS; port++) {
		/* Go through the all entires from last to fires */
		tid = PE_FIRST_DEFAULT_FLOW - port;

		if (tid == MV_PRS_OUT_OF_RAGE) {
			mvOsPrintf("%s: No free TCAM entiry\n", __func__);
			return MV_ERROR;
		}

		pe = mvPp2PrsSwAlloc(PRS_LU_FLOWS);
		pe->index = tid;

		mvPp2PrsSwTcamPortMapSet(pe, 0);

		/* set flowID*/
		mvPp2PrsSwSramAiUpdate(pe, FLOWID_DEF(port), FLOWID_MASK);
		mvPrsFlowIdSet(FLOWID_DEF(port));
		mvPp2PrsSwSramLuDoneSet(pe);

		mvOsSPrintf(name, "def-flowId-port-%d", port);

		/* Update mvPrsShadowTbl */
		mvPp2PrsShadowSet(pe->index, PRS_LU_FLOWS, name);

		mvPp2PrsHwWrite(pe);
		mvPp2PrsSwFree(pe);

	}
	return MV_OK;
}

/******************************************************************************
 *
 * Paeser Init
 *
 ******************************************************************************
 */

int mvPrsDefaultInit(void)
{
	int port, rc;

	/*enable tcam table*/
	mvPp2PrsSwTcam(1);

	/*write zero to all the lines*/
	mvPp2PrsHwClearAll();

	mvPp2PrsHwInvAll();
	mvPp2PrsShadowClearAll();
	mvPrsFlowIdClearAll();

	/* TODO: Mask & clear all interrupts */

	/* Always start from lookup = 0 */
	for (port = 0; port < MV_PP2_MAX_PORTS; port++)
		mvPp2PrsHwPortInit(port, PRS_LU_MH, MV_PP2_PRS_PORT_LU_MAX, 0);

	rc = mvPrsDefFlowInit();
	if (rc)
		return rc;

	rc = mvPp2PrsMhInit();
	if (rc)
		return rc;

	rc = mvPp2PrsMacInit();
	if (rc)
		return rc;

	rc = mvPp2PrsDsaInit();
	if (rc)
		return rc;

	rc = mvPp2PrsEtypeInit();
	if (rc)
		return rc;

	rc = mvPp2PrsVlanInit();
	if (rc)
		return rc;

	rc = mvPp2PrsPppeInit();
	if (rc)
		return rc;

	rc = mvPp2PrsIp4Init();
	if (rc)
		return rc;

	rc = mvPp2PrsIp6Init();
	if (rc)
		return rc;

	return MV_OK;
}
