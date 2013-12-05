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

#include "mvNeta.h"

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
#if defined(CONFIG_MV_ETH_BM)
	PNC_BM,
#endif
	TBL_TERM
};

static MV_STATUS ethWinOverlapDetect(MV_U32 port, MV_U32 winNum, MV_ADDR_WIN *pAddrWin);

/*******************************************************************************
* mvNetaWinInit
*
* DESCRIPTION:
*	This function initialize ETH window decode unit. It set the default
*	address decode windows of the unit.
*
* INPUT:
*	port	 : The port number to initialize the address decoding for.
*	addWinMap: An array holding the address decoding information for the
*		    system.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR if setting fail.
*******************************************************************************/
MV_STATUS mvNetaWinInit(MV_U32 port, MV_UNIT_WIN_INFO *addrWinMap)
{
	MV_U32 winNum, winPrioIndex = 0, i, regVal = 0;
	MV_UNIT_WIN_INFO *addrDecWin;
	MV_U32 accessProtReg = 0;

	/* Initiate Ethernet address decode */
	/* First disable all address decode windows */
	for (winNum = 0; winNum < ETH_MAX_DECODE_WIN; winNum++)
		regVal |= MV_BIT_MASK(winNum);

	MV_REG_WRITE(ETH_BASE_ADDR_ENABLE_REG(port), regVal);

	/* Go through all windows in user table until table terminator      */
	for (winNum = 0; ((ethAddrDecPrioTab[winPrioIndex] != TBL_TERM) && (winNum < ETH_MAX_DECODE_WIN));) {
		addrDecWin = &addrWinMap[ethAddrDecPrioTab[winPrioIndex]];

		if (addrDecWin->enable == MV_TRUE) {
			if (MV_OK != mvNetaWinWrite(port, winNum, addrDecWin)) {
				mvOsPrintf("mvNetaWinInit failed: winNum=%d (%d, %d)\n",
					   winNum, winPrioIndex, ethAddrDecPrioTab[winPrioIndex]);
				return MV_ERROR;
			}
			winNum++;
		}
		winPrioIndex++;
	}

	/* set full access to all windows. */
	for (i = 0; i < winNum; i++)
		accessProtReg |= (FULL_ACCESS << (i * 2));

	MV_REG_WRITE(ETH_ACCESS_PROTECT_REG(port), accessProtReg);

	return MV_OK;
}

/*******************************************************************************
* mvNetaWinWrite
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
MV_STATUS mvNetaWinWrite(MV_U32 port, MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin)
{
	MV_U64 size;
	MV_U32 alignment;
	MV_U32 baseReg, sizeReg;

	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvNetaWinSet: ERR. Invalid win num %d\n", winNum);
		return MV_BAD_PARAM;
	}

	size = pAddrDecWin->addrWin.size;
	if (size == 0) {
		mvOsPrintf("%s: ERR. Invalid window size %lld\n",	__func__, size);
		return MV_BAD_PARAM;
	}
	if (!MV_IS_POWER_OF_2(size)) {
		/* try to get a good size */
		pAddrDecWin->addrWin.size = 1 << (mvLog2(size) + 1);
		mvOsPrintf("%s: WARN. Wrong window size %lld, rounding to %lld\n",
			__func__, size, pAddrDecWin->addrWin.size);
		size = pAddrDecWin->addrWin.size;
	}

	/* Check if the requested window overlapps with current windows     */
	if (MV_TRUE == ethWinOverlapDetect(port, winNum, &pAddrDecWin->addrWin)) {
		mvOsPrintf("%s: ERR. Window %d overlap\n", __func__, winNum);
		return MV_ERROR;
	}

	/* check if address is aligned to the size */
	if (MV_IS_NOT_ALIGN(pAddrDecWin->addrWin.baseLow, pAddrDecWin->addrWin.size)) {
		mvOsPrintf("mvNetaWinSet: Error setting Ethernet window %d.\n"
			   "Address 0x%08x is unaligned to size 0x%llx.\n",
			   winNum, pAddrDecWin->addrWin.baseLow, pAddrDecWin->addrWin.size);
		return MV_ERROR;
	}

	baseReg = (pAddrDecWin->addrWin.baseLow & ETH_WIN_BASE_MASK);
	sizeReg = MV_REG_READ(ETH_WIN_SIZE_REG(port, winNum));

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
	mvNetaWinEnable(port, winNum, MV_FALSE);
	MV_REG_WRITE(ETH_WIN_BASE_REG(port, winNum), baseReg);

	/* Write to address decode Size Register                            */
	MV_REG_WRITE(ETH_WIN_SIZE_REG(port, winNum), sizeReg);

	/* Enable address decode target window                              */
	if (pAddrDecWin->enable == MV_TRUE)
		mvNetaWinEnable(port, winNum, MV_TRUE);

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
static MV_STATUS ethWinOverlapDetect(MV_U32 port, MV_U32 winNum, MV_ADDR_WIN *pAddrWin)
{
	MV_U32 baseAddrEnableReg;
	MV_U32 winNumIndex;
	MV_UNIT_WIN_INFO addrDecWin;

	/* Read base address enable register. Do not check disabled windows     */
	baseAddrEnableReg = MV_REG_READ(ETH_BASE_ADDR_ENABLE_REG(port));

	for (winNumIndex = 0; winNumIndex < ETH_MAX_DECODE_WIN; winNumIndex++) {
		/* Do not check window itself           */
		if (winNumIndex == winNum)
			continue;

		/* Do not check disabled windows        */
		if (baseAddrEnableReg & (1 << winNumIndex))
			continue;

		/* Get window parameters        */
		if (MV_OK != mvNetaWinRead(port, winNumIndex, &addrDecWin)) {
			mvOsPrintf("ethWinOverlapDetect: ERR. TargetWinGet failed\n");
			return MV_ERROR;
		}

		if (MV_TRUE == mvWinOverlapTest(pAddrWin, &(addrDecWin.addrWin)))
			return MV_TRUE;
	}
	return MV_FALSE;
}

/*******************************************************************************
* mvNetaWinRead
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
MV_STATUS mvNetaWinRead(MV_U32 port, MV_U32 winNum, MV_UNIT_WIN_INFO *pAddrDecWin)
{
	MV_U32 baseReg, sizeReg;
	MV_U32 alignment, size;

	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvNetaWinGet: ERR. Invalid winNum %d\n", winNum);
		return MV_NOT_SUPPORTED;
	}

	baseReg = MV_REG_READ(ETH_WIN_BASE_REG(port, winNum));
	sizeReg = MV_REG_READ(ETH_WIN_SIZE_REG(port, winNum));

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
	if (~(MV_REG_READ(ETH_BASE_ADDR_ENABLE_REG(port))) & (1 << winNum))
		pAddrDecWin->enable = MV_TRUE;
	else
		pAddrDecWin->enable = MV_FALSE;
	return MV_OK;
}

/*******************************************************************************
* mvNetaWinEnable - Enable/disable a ETH to target address window
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
MV_STATUS mvNetaWinEnable(MV_U32 port, MV_U32 winNum, MV_BOOL enable)
{
	/* Parameter checking   */
	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvNetaTargetWinEnable:ERR. Invalid winNum%d\n", winNum);
		return MV_ERROR;
	}

	if (enable == MV_TRUE)
		MV_REG_BIT_RESET(ETH_BASE_ADDR_ENABLE_REG(port), (1 << winNum));
	else
		/* Disable address decode target window                             */
		MV_REG_BIT_SET(ETH_BASE_ADDR_ENABLE_REG(port), (1 << winNum));

	return MV_OK;
}

/*******************************************************************************
* mvNetaProtWinSet - Set access protection of Ethernet to target window.
*
* DESCRIPTION:
*       Each Ethernet port can be configured with access attributes for each
*       of the Ethenret to target windows (address decode windows). This
*       function sets access attributes to a given window for the given channel.
*
* INPUTS:
*       ethPort   - ETH channel number. See MV_ETH_CHANNEL enumerator.
*       winNum - IETH to target address decode window number.
*       access - IETH access rights. See MV_ACCESS_RIGHTS enumerator.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_ERROR in case window number is invalid or access right reserved.
*
*******************************************************************************/
MV_STATUS mvNetaProtWinSet(MV_U32 portNo, MV_U32 winNum, MV_ACCESS_RIGHTS access)
{
	MV_U32 protReg;

	/* Parameter checking   */
	if (portNo >= MV_ETH_MAX_PORTS) {
		mvOsPrintf("mvNetaProtWinSet:ERR. Invalid port number %d\n", portNo);
		return MV_ERROR;
	}

	if (winNum >= ETH_MAX_DECODE_WIN) {
		mvOsPrintf("mvNetaProtWinSet:ERR. Invalid winNum%d\n", winNum);
		return MV_ERROR;
	}

	if ((access == ACC_RESERVED) || (access >= MAX_ACC_RIGHTS)) {
		mvOsPrintf("mvNetaProtWinSet:ERR. Inv access param %d\n", access);
		return MV_ERROR;
	}
	/* Read current protection register */
	protReg = MV_REG_READ(ETH_ACCESS_PROTECT_REG(portNo));

	/* Clear protection window field */
	protReg &= ~(ETH_PROT_WIN_MASK(winNum));

	/* Set new protection field value */
	protReg |= (access << (ETH_PROT_WIN_OFFS(winNum)));

	/* Write protection register back   */
	MV_REG_WRITE(ETH_ACCESS_PROTECT_REG(portNo), protReg);

	return MV_OK;
}
