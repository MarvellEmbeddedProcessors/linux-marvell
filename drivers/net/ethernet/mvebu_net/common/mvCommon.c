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

int mvCharToHex(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return (ch - '0');

	if ((ch >= 'a') && (ch <= 'f'))
		return (ch - 'a') + 10;

	if ((ch >= 'A') && (ch <= 'F'))
		return (ch - 'A') + 10;

	return -1;
}

int mvCharToDigit(char ch)
{
	if ((ch >= '0') && (ch <= '9'))
		return (ch - '0');

	return -1;
}

/*******************************************************************************
* mvMacStrToHex - Convert MAC format string to hex.
*
* DESCRIPTION:
*		This function convert MAC format string to hex.
*
* INPUT:
*       macStr - MAC address string. Fornat of address string is
*                uu:vv:ww:xx:yy:zz, where ":" can be any delimiter.
*
* OUTPUT:
*       macHex - MAC in hex format.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvMacStrToHex(const char *macStr, MV_U8 *macHex)
{
	int i;
	char tmp[3];

	for (i = 0; i < MV_MAC_ADDR_SIZE; i++) {
		tmp[0] = macStr[(i * 3) + 0];
		tmp[1] = macStr[(i * 3) + 1];
		tmp[2] = '\0';
		macHex[i] = (MV_U8) (strtol(tmp, NULL, 16));
	}
	return MV_OK;
}

/*******************************************************************************
* mvMacHexToStr - Convert MAC in hex format to string format.
*
* DESCRIPTION:
*		This function convert MAC in hex format to string format.
*
* INPUT:
*       macHex - MAC in hex format.
*
* OUTPUT:
*       macStr - MAC address string. String format is uu:vv:ww:xx:yy:zz.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_STATUS mvMacHexToStr(MV_U8 *macHex, char *macStr)
{
	int i;

	for (i = 0; i < MV_MAC_ADDR_SIZE; i++)
		mvOsSPrintf(&macStr[i * 3], "%02x:", macHex[i]);

	macStr[(i * 3) - 1] = '\0';

	return MV_OK;
}

/*******************************************************************************
* mvSizePrint - Print the given size with size unit description.
*
* DESCRIPTION:
*		This function print the given size with size unit description.
*       FOr example when size paramter is 0x180000, the function prints:
*       "size 1MB+500KB"
*
* INPUT:
*       size - Size in bytes.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvSizePrint(MV_U64 size)
{
	mvOsOutput("size ");

	if (size >= _1G) {
		mvOsOutput("%3lldGB ", (MV_U64)(size >> 30));
		size &= (MV_U64)(_1G - 1);
		if (size)
			mvOsOutput("+");
	}
	if (size >= _1M) {
		mvOsOutput("%3lldMB ", size / _1M);
		size %= _1M;
		if (size)
			mvOsOutput("+");
	}
	if (size >= _1K) {
		mvOsOutput("%3lldKB ", size / _1K);
		size %= _1K;
		if (size)
			mvOsOutput("+");
	}
	if (size > 0)
		mvOsOutput("%3lldB ", size);

}

/*******************************************************************************
* mvHexToBin - Convert hex to binary
*
* DESCRIPTION:
*		This function Convert hex to binary.
*
* INPUT:
*       pHexStr - hex buffer pointer.
*       size    - Size to convert.
*
* OUTPUT:
*       pBin - Binary buffer pointer.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvHexToBin(const char *pHexStr, MV_U8 *pBin, int size)
{
	int j, i;
	char tmp[3];
	MV_U8 byte;

	for (j = 0, i = 0; j < size; j++, i += 2) {
		tmp[0] = pHexStr[i];
		tmp[1] = pHexStr[i + 1];
		tmp[2] = '\0';
		byte = (MV_U8) (strtol(tmp, NULL, 16) & 0xFF);
		pBin[j] = byte;
	}
}

void mvAsciiToHex(const char *asciiStr, char *hexStr)
{
	int i = 0;

	while (asciiStr[i] != 0) {
		mvOsSPrintf(&hexStr[i * 2], "%02x", asciiStr[i]);
		i++;
	}
	hexStr[i * 2] = 0;
}

void mvBinToHex(const MV_U8 *bin, char *hexStr, int size)
{
	int i;

	for (i = 0; i < size; i++)
		mvOsSPrintf(&hexStr[i * 2], "%02x", bin[i]);

	hexStr[i * 2] = '\0';
}

void mvBinToAscii(const MV_U8 *bin, char *asciiStr, int size)
{
	int i;

	for (i = 0; i < size; i++)
		mvOsSPrintf(&asciiStr[i * 2], "%c", bin[i]);

	asciiStr[i * 2] = '\0';
}

/*******************************************************************************
* mvLog2 -
*
* DESCRIPTION:
*	Calculate the Log2 of a given number.
*
* INPUT:
*       num - A number to calculate the Log2 for.
*
* OUTPUT:
*       None.
*
* RETURN:
*       Log 2 of the input number, or 0xFFFFFFFF if input is 0.
*
*******************************************************************************/
MV_U32 mvLog2(MV_U32 num)
{
	MV_U32 result = 0;
	if (num == 0)
		return 0xFFFFFFFF;
	while (num != 1) {
		num = num >> 1;
		result++;
	}
	return result;
}

/*******************************************************************************
* mvWinOverlapTest
*
* DESCRIPTION:
*       This function checks the given two address windows for overlaping.
*
* INPUT:
*       pAddrWin1 - Address window 1.
*       pAddrWin2 - Address window 2.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if address window overlaps, MV_FALSE otherwise.
*
*******************************************************************************/
MV_STATUS mvWinOverlapTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2)
{
	/* Need to cancel overlap testing, because we use the
	** MBUS Bridge Windows to access IO windows, and thus there will be
	** always an overlap between the IO & DRAM windows.
	*/
	return MV_FALSE;
#if 0
	/* this code can only be enabled if physical DRAM size is smaller
	** or equal to 3GB for debug purposes.\
	*/
	MV_U32 winBase1, winBase2;
	MV_U32 winTop1, winTop2;

	/* check if we have overflow than 4G */
	if (((0xffffffff - pAddrWin1->baseLow) < pAddrWin1->size - 1) ||
	    ((0xffffffff - pAddrWin2->baseLow) < pAddrWin2->size - 1)) {
		return MV_TRUE;
	}

	winBase1 = pAddrWin1->baseLow;
	winBase2 = pAddrWin2->baseLow;
	winTop1 = winBase1 + pAddrWin1->size - 1;
	winTop2 = winBase2 + pAddrWin2->size - 1;

	if (((winBase1 <= winTop2) && (winTop2 <= winTop1)) || ((winBase1 <= winBase2) && (winBase2 <= winTop1)))
		return MV_TRUE;
	else
		return MV_FALSE;
#endif
}

/*******************************************************************************
* mvWinWithinWinTest
*
* DESCRIPTION:
*       This function checks the given win1 boundries is within win2 boundries.
*
* INPUT:
*       pAddrWin1 - Address window 1.
*       pAddrWin2 - Address window 2.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if found win1 inside win2, MV_FALSE otherwise.
*
*******************************************************************************/
MV_STATUS mvWinWithinWinTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2)
{
	MV_U64 winBase1, winBase2;
	MV_U64 winTop1, winTop2;

	winBase1 = ((MV_U64)pAddrWin1->baseHigh << 32) + (MV_U32)pAddrWin1->baseLow;
	winBase2 = ((MV_U64)pAddrWin2->baseHigh << 32) + (MV_U32)pAddrWin2->baseLow;
	winTop1 = winBase1 + (MV_U64)pAddrWin1->size - 1;
	winTop2 = winBase2 + (MV_U64)pAddrWin2->size - 1;

	if (((winBase1 >= winBase2) && (winBase1 <= winTop2)) || ((winTop1 >= winBase2) && (winTop1 <= winTop2)))
		return MV_TRUE;
	else
		return MV_FALSE;
}

/*******************************************************************************
* mvReverseBits
*
* DESCRIPTION:
*       This function Reverts the direction of the bits (LSB to MSB and vice versa)
*
* INPUT:
*	num - MV_U8 number to revert
*
* OUTPUT:
*       Reverted number
*
* RETURN:
*	None
*
*******************************************************************************/
MV_U8 mvReverseBits(MV_U8 num)
{
	num = (num & 0xF0) >> 4 | (num & 0x0F) << 4;
	num = (num & 0xCC) >> 2 | (num & 0x33) << 2;
	num = (num & 0xAA) >> 1 | (num & 0x55) << 1;
	return num;
}
/*******************************************************************************
* mvCountMaskBits
*
* DESCRIPTION:
*       This function count 1 in mask bit
*
* INPUT:
*	num - MV_U8 number to count
*
* OUTPUT:
*       None
*
* RETURN:
*	number of 1 in mask
*
*******************************************************************************/
MV_U32 mvCountMaskBits(MV_U8 mask)
{
	int i;
	MV_U32 c = 0;

	for (i = 0; i < 8; i++) {
		if (mask & 1)
			c++;
		mask = mask >> 1;
	}
	return c;
}
