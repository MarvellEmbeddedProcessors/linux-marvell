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

#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "voiceband/commUnit/mvCommUnitRegs.h"

static MV_TARGET commUnitAddrDecPrioTable[] = {
	SDRAM_CS0,
	SDRAM_CS1,
	SDRAM_CS2,
	SDRAM_CS3,
	PEX0_MEM,
	PEX0_IO,
	TBL_TERM
};

static MV_STATUS mvCommUnitWinRead(MV_U32 winNum, MV_UNIT_WIN_INFO *pDecWin);
static MV_STATUS mvCommUnitWinWrite(MV_U32 winNum, MV_UNIT_WIN_INFO *pDecWin);
static INLINE MV_VOID mvCommUnitWinEnable(MV_U32 winNum, MV_BOOL enable);
static MV_STATUS commUnitWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin);

/*******************************************************************************
* mvCommUnitWinInit - Initialize CommUnit address decode windows
*
* DESCRIPTION:
*               This function initialize CommUnit window decode unit. It set the
*               default address decode
*               windows of the unit.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if setting fail.
*******************************************************************************/
MV_STATUS mvCommUnitWinInit(MV_UNIT_WIN_INFO *pAddrWinMap)
{
	MV_U32 winNum;
	MV_U32 winPrioIndex = 0;
	MV_UNIT_WIN_INFO *addrDecWin;

	/* Clear access for all windows */
	MV_REG_WRITE(COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG, 0);

	/* Disable all windows */
	for (winNum = 0; winNum < COMM_UNIT_MBUS_MAX_WIN; winNum++)
		mvCommUnitWinEnable(winNum, MV_FALSE);

	for (winNum = 0; ((commUnitAddrDecPrioTable[winPrioIndex] != TBL_TERM) && (winNum < COMM_UNIT_MBUS_MAX_WIN));) {
		addrDecWin = &pAddrWinMap[commUnitAddrDecPrioTable[winPrioIndex]];

		if (addrDecWin->enable == MV_TRUE) {
			if (MV_OK != mvCommUnitWinWrite(winNum, addrDecWin)) {
				mvOsPrintf("mvCommUnitWinInit: failed setting window(%d)\n", winNum);
				return MV_ERROR;
			}
			winNum++;
		}
		winPrioIndex++;
	}

	return MV_OK;
}

/*******************************************************************************
* mvCommUnitWinWrite - Set CommUnit target address window
*
* DESCRIPTION:
*       This function sets a peripheral target (e.g. SDRAM bank0, PCI_MEM0)
*       address window, also known as address decode window.
*       After setting this target window, the CommUnit will be able to access the
*       target within the address window.
*
* INPUT:
*       winNum      - CommUnit to target address decode window number.
*       pDecWin     - CommUnit target window data structure.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if address window overlaps with other address decode windows.
*       MV_BAD_PARAM if base address is invalid parameter or target is
*       unknown.
*
*******************************************************************************/
static MV_STATUS mvCommUnitWinWrite(MV_U32 winNum, MV_UNIT_WIN_INFO *pDecWin)
{
	MV_U32 ctrlReg = 0, sizeReg = 0;
	MV_U32 size;

	/* Parameter checking   */
	if (winNum >= COMM_UNIT_MBUS_MAX_WIN) {
		mvOsPrintf("mvCommUnitWinWrite: ERR. Invalid win num %d\n", winNum);
		return MV_BAD_PARAM;
	}

	/* Check if the requested window overlaps with current windows         */
	if (MV_TRUE == commUnitWinOverlapDetect(winNum, &pDecWin->addrWin)) {
		mvOsPrintf("mvCommUnitWinWrite: ERR. Window %d overlap\n", winNum);
		return MV_ERROR;
	}

	/* check if address is aligned to the size */
	if (MV_IS_NOT_ALIGN(pDecWin->addrWin.baseLow, pDecWin->addrWin.size)) {
		mvOsPrintf("mvCommUnitWinWrite: Error setting CommUnit window %d"
			   "\nAddress 0x%08x is unaligned to size 0x%x.\n",
			   winNum, pDecWin->addrWin.baseLow, pDecWin->addrWin.size);
		return MV_ERROR;
	}

	size = (pDecWin->addrWin.size / MV_COMM_UNIT_WIN_SIZE_ALIGN) - 1;

	/* for the safe side we disable the window before writing the new
	   values */
	mvCommUnitWinEnable(winNum, MV_FALSE);

	ctrlReg |= ((pDecWin->attrib << COMM_UNIT_WIN_ATTRIB_OFFS) & COMM_UNIT_WIN_ATTRIB_MASK);
	ctrlReg |= ((pDecWin->targetId << COMM_UNIT_WIN_TARGET_OFFS) & COMM_UNIT_WIN_TARGET_MASK);
	ctrlReg |= (pDecWin->addrWin.baseLow & COMM_UNIT_WIN_BASE_MASK);

	/* Update Size value  */
	sizeReg |= ((size << COMM_UNIT_WIN_SIZE_OFFS) & COMM_UNIT_WIN_SIZE_MASK);

	/* Write to address size and control registers  */
	MV_REG_WRITE(COMM_UNIT_WIN_SIZE_REG(winNum), sizeReg);
	MV_REG_WRITE(COMM_UNIT_WIN_CTRL_REG(winNum), ctrlReg);

	/* Enable address decode target window  */
	if (pDecWin->enable == MV_TRUE)
		mvCommUnitWinEnable(winNum, MV_TRUE);

	return MV_OK;
}

/*******************************************************************************
* mvCommUnitWinRead - Read peripheral target address window.
*
* DESCRIPTION:
*               Read CommUnit peripheral target address window.
*
* INPUT:
*       winNum - CommUnit to target address decode window number.
*
* OUTPUT:
*       pDecWin - CommUnit target window data structure.
*
* RETURN:
*       MV_ERROR if register parameters are invalid.
*
*******************************************************************************/
MV_STATUS mvCommUnitWinRead(MV_U32 winNum, MV_UNIT_WIN_INFO *pDecWin)
{

	MV_U32 ctrlReg, sizeReg, enReg;
	MV_U32 size;

	/* Parameter checking   */
	if (winNum >= COMM_UNIT_MBUS_MAX_WIN) {
		mvOsPrintf("mvCommUnitWinRead: ERR. Invalid winNum %d\n", winNum);
		return MV_NOT_SUPPORTED;
	}

	sizeReg = MV_REG_READ(COMM_UNIT_WIN_SIZE_REG(winNum));
	ctrlReg = MV_REG_READ(COMM_UNIT_WIN_CTRL_REG(winNum));
	enReg = MV_REG_READ(COMM_UNIT_WIN_ENABLE_REG(winNum));

	/* Check if window is enabled   */
	if (enReg & COMM_UNIT_WIN_ENABLE_MASK) {
		pDecWin->enable = MV_TRUE;

		/* Extract window parameters from registers */
		pDecWin->targetId = (ctrlReg & COMM_UNIT_WIN_TARGET_MASK) >> COMM_UNIT_WIN_TARGET_OFFS;
		pDecWin->attrib = (ctrlReg & COMM_UNIT_WIN_ATTRIB_MASK) >> COMM_UNIT_WIN_ATTRIB_OFFS;
		pDecWin->addrWin.baseLow = (ctrlReg & COMM_UNIT_WIN_BASE_MASK);
		pDecWin->addrWin.baseHigh = 0;

		size = (sizeReg & COMM_UNIT_WIN_SIZE_MASK) >> COMM_UNIT_WIN_SIZE_OFFS;
		pDecWin->addrWin.size = (size + 1) * MV_COMM_UNIT_WIN_SIZE_ALIGN;
	} else {
		pDecWin->enable = MV_FALSE;
	}

	return MV_OK;
}

/*******************************************************************************
* mvCommUnitWinEnable - Enable/disable a CommUnit to target address window
*
* DESCRIPTION:
*       This function enable/disable a CommUnit to target address window.
*       According to parameter 'enable' the routine will enable the
*       window, thus enabling CommUnit accesses.
*       Otherwise, the window will be disabled.
*
* INPUT:
*       winNum - CommUnit to target address decode window number.
*       enable - Enable/disable parameter.
*
* OUTPUT:
*       N/A
*
* RETURN:
*       MV_ERROR if decode window number was wrong or enabled window overlaps.
*
*******************************************************************************/
static INLINE MV_VOID mvCommUnitWinEnable(MV_U32 winNum, MV_BOOL enable)
{
	MV_U32 enReg = 0, accProtWin = 0;

	enReg = MV_REG_READ(COMM_UNIT_WIN_ENABLE_REG(winNum));
	accProtWin = MV_REG_READ(COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG);

	if (enable == MV_TRUE) {
		MV_REG_WRITE(COMM_UNIT_WIN_ENABLE_REG(winNum), (enReg | COMM_UNIT_WIN_ENABLE_MASK));
		MV_REG_WRITE(COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG, (accProtWin | COMM_UNIT_WIN_PROTECT_MASK(winNum)));

	} else {
		MV_REG_WRITE(COMM_UNIT_WIN_ENABLE_REG(winNum), 0 /*(enReg & ((MV_U32)~COMM_UNIT_WIN_ENABLE_MASK)) */);
		MV_REG_WRITE(COMM_UNIT_WINDOWS_ACCESS_PROTECT_REG,
			     (accProtWin & ((MV_U32) ~COMM_UNIT_WIN_PROTECT_MASK(winNum))));
	}
}

/*******************************************************************************
* commUnitWinOverlapDetect - Detect CommUnit address windows overlapping
*
* DESCRIPTION:
*       An unpredicted behaviour is expected in case CommUnit address decode
*       windows overlaps.
*       This function detects CommUnit address decode windows overlapping of a
*       specified window. The function does not check the window itself for
*       overlapping. The function also skipps disabled address decode windows.
*
* INPUT:
*       winNum      - address decode window number.
*       pDecWin     - An address decode window struct.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if the given address window overlap current address
*       decode map, MV_FALSE otherwise, MV_ERROR if reading invalid data
*       from registers.
*
*******************************************************************************/
static MV_STATUS commUnitWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin)
{
	MV_U32 winNumIndex;
	MV_UNIT_WIN_INFO addrDecWin;

	for (winNumIndex = 0; winNumIndex < COMM_UNIT_MBUS_MAX_WIN; winNumIndex++) {
		/* Do not check window itself       */
		if (winNumIndex == winNum)
			continue;

		/* Get window parameters    */
		if (MV_OK != mvCommUnitWinRead(winNumIndex, &addrDecWin)) {
			mvOsPrintf("%s: ERR. TargetWinGet failed\n", __func__);
			return MV_ERROR;
		}

		/* Do not check disabled windows    */
		if (addrDecWin.enable == MV_FALSE)
			continue;

		if (MV_TRUE == mvWinOverlapTest(pAddrWin, &(addrDecWin.addrWin)))
			return MV_TRUE;
	}

	return MV_FALSE;
}
