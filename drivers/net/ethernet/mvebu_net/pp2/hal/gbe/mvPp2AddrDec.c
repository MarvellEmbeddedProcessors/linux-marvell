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
#include "mvSysEthConfig.h"

#include "mvPp2Gbe.h"

MV_TARGET ethAddrDecPrioTab[] = {
#if defined(MV_INCLUDE_SDRAM_CS0)
	SDRAM_CS0,
#endif
#if defined(MV_INCLUDE_SDRAM_CS1)
	SDRAM_CS1,
#endif
#if defined(MV_INCLUDE_SDRAM_CS2)
	SDRAM_CS2,
#endif
#if defined(MV_INCLUDE_SDRAM_CS3)
	SDRAM_CS3,
#endif
	TBL_TERM
};

static MV_STATUS ethWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin);

/*******************************************************************************
* mvPp2WinInit
*
* DESCRIPTION:
*	This function initialize ETH window decode unit. It set the default
*	address decode windows of the unit.
*
* INPUT:
*	addWinMap: An array holding the address decoding information for the
*		    system.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if setting fail.
*******************************************************************************/
MV_STATUS mvPp2WinInit(MV_U32 dummy/*backward compability*/, MV_UNIT_WIN_INFO *addrWinMap)
{
	MV_U32 winNum, winPrioIndex = 0;
	MV_UNIT_WIN_INFO *addrDecWin;

	/* Initiate Ethernet address decode */
	/* First disable all address decode windows */
	mvPp2WrReg(ETH_BASE_ADDR_ENABLE_REG, 0);

	/* Go through all windows in user table until table terminator      */
	for (winNum = 0; ((ethAddrDecPrioTab[winPrioIndex] != TBL_TERM) && (winNum < ETH_MAX_DECODE_WIN));) {
		addrDecWin = &addrWinMap[ethAddrDecPrioTab[winPrioIndex]];

		if (addrDecWin->enable == MV_TRUE) {
			if (MV_OK != mvPp2WinWrite(0, winNum, addrDecWin)) {
				mvOsPrintf("mvPp2WinInit failed: winNum=%d (%d, %d)\n",
					   winNum, winPrioIndex, ethAddrDecPrioTab[winPrioIndex]);
				return MV_ERROR;
			}
			winNum++;
		}
		winPrioIndex++;
	}

	return MV_OK;
}

/*******************************************************************************
* mvPp2WinWrite
*
* DESCRIPTION:
*	This function writes the address decoding registers according to the
*	given window configuration.
*
* INPUT:
*	unit	    - The Ethernet unit number to configure.
*       winNum	    - ETH target address decode window number.
*       pAddrDecWin - ETH target window data structure.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_OK on success,
*	MV_BAD_PARAM if winNum is invalid.
*	MV_ERROR otherwise.
*
*******************************************************************************/
MV_STATUS mvPp2WinWrite(MV_U32 dummy/*backward compability*/, MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin)
{
	MV_U32 size, alignment;
	MV_U32 baseReg, sizeReg;

	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvPp2WinSet: ERR. Invalid win num %d\n", winNum);
		return MV_BAD_PARAM;
	}

	/* Check if the requested window overlapps with current windows     */
	if (MV_TRUE == ethWinOverlapDetect(winNum, &pAddrDecWin->addrWin)) {
		mvOsPrintf("mvPp2WinWrite: ERR. Window %d overlap\n", winNum);
		return MV_ERROR;
	}

	/* check if address is aligned to the size */
	if (MV_IS_NOT_ALIGN(pAddrDecWin->addrWin.baseLow, pAddrDecWin->addrWin.size)) {
		mvOsPrintf("mvPp2WinSet: Error setting Ethernet window %d.\n"
			   "Address 0x%08x is unaligned to size 0x%x.\n",
			   winNum, pAddrDecWin->addrWin.baseLow, (MV_U32)pAddrDecWin->addrWin.size);
		return MV_ERROR;
	}

	size = pAddrDecWin->addrWin.size;
	if (!MV_IS_POWER_OF_2(size)) {
		mvOsPrintf("mvPp2WinWrite: Error setting AUDIO window %d. "
			   "Window size is not a power to 2.", winNum);
		return MV_BAD_PARAM;
	}

	baseReg = (pAddrDecWin->addrWin.baseLow & ETH_WIN_BASE_MASK);
	sizeReg = mvPp2RdReg(ETH_WIN_SIZE_REG(winNum));

	/* set size */
	alignment = 1 << ETH_WIN_SIZE_OFFS;
	sizeReg &= ~ETH_WIN_SIZE_MASK;
	sizeReg |= (((size / alignment) - 1) << ETH_WIN_SIZE_OFFS);

	/* set attributes */
	baseReg &= ~ETH_WIN_ATTR_MASK;
	baseReg |= pAddrDecWin->attrib << ETH_WIN_ATTR_OFFS;

	/* set target ID */
	baseReg &= ~ETH_WIN_TARGET_MASK;
	baseReg |= pAddrDecWin->targetId << ETH_WIN_TARGET_OFFS;

	/* for the safe side we disable the window before writing the new
	   values */
	mvPp2WinEnable(0, winNum, MV_FALSE);
	mvPp2WrReg(ETH_WIN_BASE_REG(winNum), baseReg);

	/* Write to address decode Size Register                            */
	mvPp2WrReg(ETH_WIN_SIZE_REG(winNum), sizeReg);

	/* Enable address decode target window                              */
	if (pAddrDecWin->enable == MV_TRUE)
		mvPp2WinEnable(0, winNum, MV_TRUE);

	return MV_OK;
}

/*******************************************************************************
* ethWinOverlapDetect - Detect ETH address windows overlapping
*
* DESCRIPTION:
*       An unpredicted behaviur is expected in case ETH address decode
*       windows overlapps.
*       This function detects ETH address decode windows overlapping of a
*       specified window. The function does not check the window itself for
*       overlapping. The function also skipps disabled address decode windows.
*
* INPUT:
*       winNum      - address decode window number.
*       pAddrDecWin - An address decode window struct.
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
static MV_STATUS ethWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin)
{
	MV_U32 baseAddrEnableReg;
	MV_U32 winNumIndex;
	MV_UNIT_WIN_INFO addrDecWin;

	/* Read base address enable register. Do not check disabled windows     */
	baseAddrEnableReg = mvPp2RdReg(ETH_BASE_ADDR_ENABLE_REG);

	for (winNumIndex = 0; winNumIndex < ETH_MAX_DECODE_WIN; winNumIndex++) {
		/* Do not check window itself           */
		if (winNumIndex == winNum)
			continue;

		/* Do not check disabled windows        */
		if (baseAddrEnableReg & (1 << winNumIndex))
			continue;

		/* Get window parameters        */
		if (MV_OK != mvPp2WinRead(0, winNumIndex, &addrDecWin)) {
			mvOsPrintf("ethWinOverlapDetect: ERR. TargetWinGet failed\n");
			return MV_ERROR;
		}

		if (MV_TRUE == mvWinOverlapTest(pAddrWin, &(addrDecWin.addrWin)))
			return MV_TRUE;
	}
	return MV_FALSE;
}

/*******************************************************************************
* mvPp2WinRead
*
* DESCRIPTION:
*       Read Ethernet peripheral target address window.
*
* INPUT:
*       winNum - ETH to target address decode window number.
*
* OUTPUT:
*       pAddrDecWin - ETH target window data structure.
*
* RETURN:
*	MV_BAD_PARAM if winNum is invalid.
*	MV_ERROR otherwise.
*
*******************************************************************************/
MV_STATUS mvPp2WinRead(MV_U32 dummy/*backward compability*/, MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin)
{
	MV_U32 baseReg, sizeReg;
	MV_U32 alignment, size;

	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvPp2WinGet: ERR. Invalid winNum %d\n", winNum);
		return MV_NOT_SUPPORTED;
	}

	baseReg = mvPp2RdReg(ETH_WIN_BASE_REG(winNum));
	sizeReg = mvPp2RdReg(ETH_WIN_SIZE_REG(winNum));

	alignment = 1 << ETH_WIN_SIZE_OFFS;
	size = (sizeReg & ETH_WIN_SIZE_MASK) >> ETH_WIN_SIZE_OFFS;
	pAddrDecWin->addrWin.size = (size + 1) * alignment;

	/* Extract base address                                     */
	pAddrDecWin->addrWin.baseLow = baseReg & ETH_WIN_BASE_MASK;
	pAddrDecWin->addrWin.baseHigh = 0;

	/* attrib and targetId */
	pAddrDecWin->attrib = (baseReg & ETH_WIN_ATTR_MASK) >> ETH_WIN_ATTR_OFFS;
	pAddrDecWin->targetId = (baseReg & ETH_WIN_TARGET_MASK) >> ETH_WIN_TARGET_OFFS;

	/* Check if window is enabled   */
	if ((mvPp2RdReg(ETH_BASE_ADDR_ENABLE_REG)) & (1 << winNum))
		pAddrDecWin->enable = MV_TRUE;
	else
		pAddrDecWin->enable = MV_FALSE;
	return MV_OK;
}

/*******************************************************************************
* mvPp2WinEnable - Enable/disable a ETH to target address window
*
* DESCRIPTION:
*       This function enable/disable a ETH to target address window.
*       According to parameter 'enable' the routine will enable the
*       window, thus enabling ETH accesses (before enabling the window it is
*       tested for overlapping). Otherwise, the window will be disabled.
*
* INPUT:
*       winNum - ETH to target address decode window number.
*       enable - Enable/disable parameter.
*
* OUTPUT:
*       N/A
*
* RETURN:
*       MV_ERROR if decode window number was wrong or enabled window overlapps.
*
*******************************************************************************/
MV_STATUS mvPp2WinEnable(MV_U32 dummy/*backward compability*/, MV_U32 winNum, MV_BOOL enable)
{
	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvPp2TargetWinEnable:ERR. Invalid winNum%d\n", winNum);
		return MV_ERROR;
	}

	if (enable)
		MV_REG_BIT_SET(ETH_BASE_ADDR_ENABLE_REG, (1 << winNum));
	else
		/* Disable address decode target window                             */
		MV_REG_BIT_RESET(ETH_BASE_ADDR_ENABLE_REG, (1 << winNum));

	return MV_OK;
}

