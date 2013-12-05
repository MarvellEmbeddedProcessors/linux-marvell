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

#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include "mvTypes.h"
#include "mvDebug.h"
#include "mvOs.h"

#include "gbe/mvNeta.h"

#include "mvPmt.h"

MV_NETA_PMT	**mvPmtBase = NULL;

/* #define PMT_DBG mvOsPrintf */
#define PMT_DBG(X...)

static char mvPmtCmdNames[MV_ETH_PMT_SIZE][PMT_TEXT] = {

	[MV_NETA_CMD_NONE]          = "NO_MOD",
	[MV_NETA_CMD_ADD_2B]        = "ADD_2B",
	[MV_NETA_CMD_CFG_VLAN]      = "CFG_VLAN",
	[MV_NETA_CMD_ADD_VLAN]      = "ADD_VLAN",
	[MV_NETA_CMD_CFG_DSA_1]     = "CFG_DSA_1",
	[MV_NETA_CMD_CFG_DSA_2]     = "CFG_DSA_2",
	[MV_NETA_CMD_ADD_DSA]       = "ADD_DSA",
	[MV_NETA_CMD_DEL_BYTES]     = "DEL_BYTES",
	[MV_NETA_CMD_REPLACE_2B]    = "REPLACE_2B",
	[MV_NETA_CMD_REPLACE_LSB]   = "REPLACE_LSB",
	[MV_NETA_CMD_REPLACE_MSB]   = "REPLACE_MSB",
	[MV_NETA_CMD_REPLACE_VLAN]  = "REPLACE_VLAN",
	[MV_NETA_CMD_DEC_LSB]       = "DEC_LSB",
	[MV_NETA_CMD_DEC_MSB]       = "DEC_MSB",
	[MV_NETA_CMD_ADD_CALC_LEN]  = "ADD_CALC_LEN",
	[MV_NETA_CMD_REPLACE_LEN]   = "REPLACE_LEN",
	[MV_NETA_CMD_IPV4_CSUM]     = "IPV4_CSUM",
	[MV_NETA_CMD_L4_CSUM]       = "L4_CSUM",
	[MV_NETA_CMD_SKIP]          = "SKIP",
	[MV_NETA_CMD_JUMP]          = "JUMP",
	[MV_NETA_CMD_JUMP_SKIP]     = "JUMP_SKIP",
	[MV_NETA_CMD_JUMP_SUB]      = "JUMP_SUB",
	[MV_NETA_CMD_PPPOE]         = "PPPOE",
	[MV_NETA_CMD_STORE]         = "STORE",
};

/*******************************************************************************
* mvNetaPmtWrite - Add entry to Packet Modification Table
*
* INPUT:
*       int			port    - NETA port number
*       int			idx     - PMT entry index to write to
*       MV_NETA_PMT	pEntry  - PMT entry
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvNetaPmtWrite(int port, int idx, MV_NETA_PMT *pEntry)
{
	MV_NETA_PMT	*pBase;

	if ((port < 0) || (port >= mvNetaHalData.maxPort)) {
		mvOsPrintf("%s: port %d is out of range\n", __func__, port);
		return MV_OUT_OF_RANGE;
	}

	if ((idx < 0) || (idx >= MV_ETH_PMT_SIZE)) {
		mvOsPrintf("%s: entry %d is out of range\n", __func__, idx);
		return MV_OUT_OF_RANGE;
	}
	if ((mvPmtBase == NULL) || (mvPmtBase[port] == NULL)) {
		mvOsPrintf("%s: PMT for port #%d is not initialized\n", __func__, port);
		return MV_INIT_ERROR;
	}
	pBase = mvPmtBase[port];
	pBase[idx].word = pEntry->word;

	return MV_OK;
}

/*******************************************************************************
* mvNetaPmtRead - Read entry from Packet Modification Table
*
* INPUT:
*       int			port - NETA port number
*       int			idx  - PMT entry index to read from
* OUTPUT:
*       MV_NETA_PMT	pEntry - PMT entry
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS mvNetaPmtRead(int port, int idx, MV_NETA_PMT *pEntry)
{
	MV_NETA_PMT	*pBase;

	if ((port < 0) || (port >= mvNetaHalData.maxPort)) {
		mvOsPrintf("%s: port %d is out of range\n", __func__, port);
		return MV_OUT_OF_RANGE;
	}

	if ((idx < 0) || (idx >= MV_ETH_PMT_SIZE)) {
		mvOsPrintf("%s: entry %d is out of range\n", __func__, idx);
		return MV_OUT_OF_RANGE;
	}
	if ((mvPmtBase == NULL) || (mvPmtBase[port] == NULL)) {
		mvOsPrintf("%s: PMT for port #%d is not initialized\n", __func__, port);
		return MV_INIT_ERROR;
	}
	pBase = mvPmtBase[port];
	pEntry->word = pBase[idx].word;

	return MV_OK;
}

/*******************************************************************************
* mvNetaPmtClear - Clear Packet Modification Table
*
* INPUT:
*       int			port - NETA port number
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvNetaPmtClear(int port)
{
	int         idx;
	MV_NETA_PMT entry;

	if ((port < 0) || (port >= mvNetaHalData.maxPort)) {
		mvOsPrintf("%s: port %d is out of range\n", __func__, port);
		return MV_OUT_OF_RANGE;
	}

	MV_NETA_PMT_INVALID_SET(&entry);
	for (idx = 0; idx < MV_ETH_PMT_SIZE; idx++)
		mvNetaPmtWrite(port, idx, &entry);

	return MV_OK;
}

/*******************************************************************************
* mvNetaPmtInit - Init Packet Modification Table driver
*
* INPUT:
*       int			port - NETA port number
*
* RETURN:   MV_STATUS
*               MV_OK - Success, Others - Failure
*******************************************************************************/
MV_STATUS   mvNetaPmtInit(int port, MV_NETA_PMT *pBase)
{
	if ((port < 0) || (port >= mvNetaHalData.maxPort)) {
		mvOsPrintf("%s: port %d is out of range\n", __func__, port);
		return MV_OUT_OF_RANGE;
	}

	if (mvPmtBase == NULL) {
		mvPmtBase = mvOsMalloc(mvNetaHalData.maxPort * sizeof(MV_NETA_PMT *));
		if (mvPmtBase == NULL) {
			mvOsPrintf("%s: Allocation failed\n", __func__);
			return MV_OUT_OF_CPU_MEM;
		}
		memset(mvPmtBase, 0, mvNetaHalData.maxPort * sizeof(MV_NETA_PMT *));
	}
	mvPmtBase[port] = pBase;

	mvNetaPmtClear(port);

	return MV_OK;
}

/*******************************************************************************
* mvNetaPmtDestroy - Free PMT Base memory
*
* INPUT:
*
* RETURN:   void
*******************************************************************************/
MV_VOID   mvNetaPmtDestroy(MV_VOID)
{
	if (mvPmtBase)
		mvOsFree(mvPmtBase);
}

/*******************************************************************************
* mvNetaPmtEntryPrint - Print PMT entry
*
* INPUT:
*       MV_NETA_PMT*    pEntry - PMT entry to be printed
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtEntryPrint(MV_NETA_PMT *pEntry)
{
	mvOsPrintf("%04x %04x: %s",
		MV_NETA_PMT_CTRL_GET(pEntry), MV_NETA_PMT_DATA_GET(pEntry),
		mvPmtCmdNames[MV_NETA_PMT_CMD_GET(pEntry)]);

	if (pEntry->word & MV_NETA_PMT_IP4_CSUM_MASK)
		mvOsPrintf(", IPv4 csum");

	if (pEntry->word & MV_NETA_PMT_L4_CSUM_MASK)
		mvOsPrintf(", L4 csum");

	if (pEntry->word & MV_NETA_PMT_LAST_MASK)
		mvOsPrintf(", Last");

	mvOsPrintf("\n");
}

/*******************************************************************************
* mvNetaPmtDump - Dump Packet Modification Table
*
* INPUT:
*       int			port    - NETA port number
*       int         flags   -
*
* RETURN:   void
*******************************************************************************/
void   mvNetaPmtDump(int port, int flags)
{
	int             idx, count = 0;
	MV_NETA_PMT 	entry;
	MV_STATUS       status;

	if ((port < 0) || (port >= mvNetaHalData.maxPort)) {
		mvOsPrintf("%s: port %d is out of range\n", __func__, port);
		return;
	}

	for (idx = 0; idx < MV_ETH_PMT_SIZE; idx++) {
		status = mvNetaPmtRead(port, idx, &entry);
		if (status != MV_OK) {
			mvOsPrintf("%s failed: port=%d, idx=%d, status=%d\n",
					__func__, port, idx, status);
			return;
		}
		if ((flags & PMT_PRINT_VALID_FLAG) && !MV_NETA_PMT_IS_VALID(&entry))
			continue;

		count++;
		mvOsPrintf("[%3d]: ", idx);
		mvNetaPmtEntryPrint(&entry);
	}

	if (!count)
		mvOsPrintf("PMT is empty, %d entries\n", MV_ETH_PMT_SIZE);
}

/*******************************************************************************
* mvNetaPmtAdd2Bytes - Set PMT entry with "add 2 bytes" command
*
* INPUT:
*       MV_U16 data         - 2 bytes of data to be added
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtAdd2Bytes(MV_NETA_PMT *pEntry, MV_U16 data)
{
	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_ADD_2B);
	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtReplace2Bytes - Set PMT entry with "Replace 2 bytes" command
*
* INPUT:
*       MV_U16 data         - 2 bytes of data to be replaced
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtReplace2Bytes(MV_NETA_PMT *pEntry, MV_U16 data)
{
	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_REPLACE_2B);
	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtDelShorts - Set PMT entry with "Delete" command
*
* INPUT:
*       MV_U8   toDelete    - number of shorts to be deleted
*       MV_U8   skipBefore  - number of shorts to be skipped before delete
*       MV_U8   skipAfter   - number of shorts to be skipped after delete
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtDelShorts(MV_NETA_PMT *pEntry, MV_U8 toDelete,
				MV_U8 skipBefore, MV_U8 skipAfter)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_DEL_BYTES);

	data = MV_NETA_PMT_DEL_SHORTS(toDelete) |
		MV_NETA_PMT_DEL_SKIP_B(skipBefore) |
		MV_NETA_PMT_DEL_SKIP_A(skipAfter);

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/* Set update checksum flags to PMT entry */
void    mvNetaPmtFlags(MV_NETA_PMT *pEntry, int last, int ipv4, int l4)
{
	if (last)
		pEntry->word |= MV_NETA_PMT_LAST_MASK;

	if (ipv4)
		pEntry->word |= MV_NETA_PMT_IP4_CSUM_MASK;

	if (l4)
		pEntry->word |= MV_NETA_PMT_L4_CSUM_MASK;
}

/* Set Last flag to PMT entry */
void    mvNetaPmtLastFlag(MV_NETA_PMT *pEntry, int last)
{
	if (last)
		pEntry->word |= MV_NETA_PMT_LAST_MASK;
	else
		pEntry->word &= ~MV_NETA_PMT_LAST_MASK;
}

/*******************************************************************************
* mvNetaPmtReplaceLSB - Set PMT entry with "Replace LSB" command
*
* INPUT:
*       MV_U8 value    - value to be placed
*       MV_U8 mask     - mask defines which bits to be replaced
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtReplaceLSB(MV_NETA_PMT *pEntry, MV_U8 value, MV_U8 mask)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_REPLACE_LSB);

	data = (value << 0) | (mask << 8);

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtReplaceMSB - Set PMT entry with "Replace MSB" command
*
* INPUT:
*       MV_U8 value    - value to be placed
*       MV_U8 mask     - mask defines which bits to be replaced
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtReplaceMSB(MV_NETA_PMT *pEntry, MV_U8 value, MV_U8 mask)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_REPLACE_MSB);

	data = (value << 0) | (mask << 8);

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtSkip - Set PMT entry with "Skip" command
*
* INPUT:
*       MV_U16 shorts   - number of shorts to be skipped
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtSkip(MV_NETA_PMT *pEntry, MV_U16 shorts)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_SKIP);

	data = MV_NETA_PMT_CALC_LEN_DATA(shorts * 2);
	data |= MV_NETA_PMT_CALC_LEN_0_ZERO;
	data |= MV_NETA_PMT_CALC_LEN_1(MV_NETA_PMT_ZERO_ADD);
	data |= MV_NETA_PMT_CALC_LEN_2(MV_NETA_PMT_ZERO_ADD);
	data |= MV_NETA_PMT_CALC_LEN_3_ADD_MASK;

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtJump - Set PMT entry with "Jump" command
*
* INPUT:
*       MV_U16 target   - PMT entry to jump to
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void    mvNetaPmtJump(MV_NETA_PMT *pEntry, MV_U16 target, int type, int cond)
{
	MV_U16  data;

	if (type == 0) {
		MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_JUMP);
	} else if (type == 1) {
		MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_JUMP_SKIP);
	} else if (type == 2) {
		MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_JUMP_SUB);
	} else {
		mvOsPrintf("%s - Unexpected type = %d\n", __func__, type);
		return;
	}

	data = target;
	if (cond == 1)
		data |= MV_NETA_PMT_IP4_CSUM_MASK;
	else if (cond == 2)
		data |= MV_NETA_PMT_L4_CSUM_MASK;

	MV_NETA_PMT_DATA_SET(pEntry, data);
}


/*******************************************************************************
* mvNetaPmtDecLSB - Set PMT entry with "Decrement LSB" command
*
* INPUT:
*       MV_U8   skipBefore  - number of shorts to be skipped before delete
*       MV_U8   skipAfter   - number of shorts to be skipped after delete
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void        mvNetaPmtDecLSB(MV_NETA_PMT *pEntry, MV_U8 skipBefore, MV_U8 skipAfter)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_DEC_LSB);

	data =  MV_NETA_PMT_DEL_SKIP_B(skipBefore) |
		MV_NETA_PMT_DEL_SKIP_A(skipAfter);

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtDecMSB - Set PMT entry with "Decrement MSB" command
*
* INPUT:
*       MV_U8   skipBefore  - number of shorts to be skipped before delete
*       MV_U8   skipAfter   - number of shorts to be skipped after delete
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void        mvNetaPmtDecMSB(MV_NETA_PMT *pEntry, MV_U8 skipBefore, MV_U8 skipAfter)
{
	MV_U16  data;

	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_DEC_MSB);

	data =  MV_NETA_PMT_DEL_SKIP_B(skipBefore) |
		MV_NETA_PMT_DEL_SKIP_A(skipAfter);

	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtReplaceIPv4csum - Set PMT entry with "Replace IP checksum" command
*
* INPUT:
*       MV_U16   data
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void        mvNetaPmtReplaceIPv4csum(MV_NETA_PMT *pEntry, MV_U16 data)
{
	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_IPV4_CSUM);
	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/*******************************************************************************
* mvNetaPmtReplaceL4csum - Set PMT entry with "Replace TCP/UDP checksum" command
*
* INPUT:
*       MV_U16   data
*
* OUTPUT:
*       MV_NETA_PMT* pEntry - PMT entry to be set
*
* RETURN:   void
*******************************************************************************/
void        mvNetaPmtReplaceL4csum(MV_NETA_PMT *pEntry, MV_U16 data)
{
	MV_NETA_PMT_CMD_SET(pEntry, MV_NETA_CMD_L4_CSUM);
	MV_NETA_PMT_DATA_SET(pEntry, data);
}

/**************************************************************/
/* High level PMT configuration functions - multiple commands */
/**************************************************************/

/* Configure PMT to decrement TTL in IPv4 header - 2 entries */
int     mvNetaPmtTtlDec(int port, int idx, int ip_offs, int isLast)
{
	MV_NETA_PMT     pmtEntry;

	/* Skip to TTL and Decrement - Set flag for IP csum */
	MV_NETA_PMT_CLEAR(&pmtEntry);
	mvNetaPmtDecMSB(&pmtEntry, (ip_offs + 8)/2, 0);
	mvNetaPmtFlags(&pmtEntry, 0, 1, 0);
	mvNetaPmtWrite(port, idx, &pmtEntry);
	idx++;

	/* Update IP checksum */
	MV_NETA_PMT_CLEAR(&pmtEntry);
	mvNetaPmtReplaceIPv4csum(&pmtEntry, 0);
	if (isLast)
		mvNetaPmtLastFlag(&pmtEntry, 1);

	mvNetaPmtWrite(port, idx, &pmtEntry);

	return idx;
}

/* Configure PMT to replace bytes in the packet: minimum 2 bytes - 1 entry for each 2 bytes */
int     mvNetaPmtDataReplace(int port, int idx, int offset,
				 MV_U8 *data, int bytes, int isLast)
{
	int             i;
	MV_U16          u16;
	MV_NETA_PMT     pmtEntry;

	if (offset > 0) {
		/* Skip command first */
		MV_NETA_PMT_CLEAR(&pmtEntry);
		mvNetaPmtSkip(&pmtEntry, offset/2);
		mvNetaPmtWrite(port, idx, &pmtEntry);
		idx++;
	}
	for (i = 0; i < bytes; i += 2) {
		/* Replace */
		MV_NETA_PMT_CLEAR(&pmtEntry);
		u16 = ((data[i] << 8) | data[i+1]);
		mvNetaPmtReplace2Bytes(&pmtEntry, u16);
		if (isLast && (i == bytes))
			mvNetaPmtLastFlag(&pmtEntry, 1);

		mvNetaPmtWrite(port, idx, &pmtEntry);
		idx++;
	}

	return idx;
}
