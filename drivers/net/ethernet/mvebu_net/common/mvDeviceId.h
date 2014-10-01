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

#ifndef __INCmvDeviceIdh
#define __INCmvDeviceIdh

#ifdef __cplusplus
extern "C" {
#endif	/* __cplusplus */

/* defines  */
#define MARVELL_VEN_ID			0x11ab
#define MV_INVALID_DEV_ID		0xffff

/* Disco-3 */
#define MV64460_DEV_ID          	0x6480
#define MV64460B_DEV_ID         	0x6485
#define MV64430_DEV_ID          	0x6420

/* Disco-5 */
#define MV64560_DEV_ID          	0x6450

/* Disco-6 */
#define MV64660_DEV_ID          	0x6460

/* Orion */
#define MV_1181_DEV_ID          	0x1181
#define MV_5181_DEV_ID          	0x5181
#define MV_5281_DEV_ID          	0x5281
#define MV_5182_DEV_ID          	0x5182
#define MV_8660_DEV_ID          	0x8660
#define MV_5180_DEV_ID          	0x5180
#define MV_5082_DEV_ID          	0x5082
#define MV_1281_DEV_ID          	0x1281
#define MV_6082_DEV_ID          	0x6082
#define MV_6183_DEV_ID          	0x6183
#define MV_6183L_DEV_ID          	0x6083

#define MV_5281_D0_REV          	0x4
#define MV_5281_D0_ID           	((MV_5281_DEV_ID << 16) | MV_5281_D0_REV)
#define MV_5281_D0_NAME         "88F5281 D0"

#define MV_5281_D1_REV          	0x5
#define MV_5281_D1_ID           	((MV_5281_DEV_ID << 16) | MV_5281_D1_REV)
#define MV_5281_D1_NAME         "88F5281 D1"

#define MV_5281_D2_REV          	0x6
#define MV_5281_D2_ID           	((MV_5281_DEV_ID << 16) | MV_5281_D2_REV)
#define MV_5281_D2_NAME         "88F5281 D2"

#define MV_5181L_A0_REV         	0x8	/* need for PCIE Er */
#define MV_5181_A1_REV          	0x1	/* for USB Er .. */
#define MV_5181_B0_REV          	0x2
#define MV_5181_B1_REV          	0x3
#define MV_5182_A1_REV          	0x1
#define MV_5180N_B1_REV         	0x3
#define MV_5181L_A0_ID          	((MV_5181_DEV_ID << 16) | MV_5181L_A0_REV)

/* kw */
#define MV_6281_DEV_ID          	0x6281
#define MV_6282_DEV_ID          	0x1155
#define MV_6192_DEV_ID          	0x6192
#define MV_6190_DEV_ID          	0x6190
#define MV_6180_DEV_ID          	0x6180
#define MV_6280_DEV_ID          	0x6280

#define MV_6281_A0_REV         		0x2
#define MV_6281_A0_ID          		((MV_6281_DEV_ID << 16) | MV_6281_A0_REV)
#define MV_6281_A0_NAME         	"88F6281 A0"

#define MV_6192_A0_REV         		0x2
#define MV_6192_A0_ID          		((MV_6192_DEV_ID << 16) | MV_6192_A0_REV)
#define MV_6192_A0_NAME         	"88F6192 A0"

#define MV_6190_A0_REV         		0x2
#define MV_6190_A0_ID          		((MV_6190_DEV_ID << 16) | MV_6190_A0_REV)
#define MV_6190_A0_NAME         	"88F6190 A0"

#define MV_6180_A0_REV         		0x2
#define MV_6180_A0_ID          		((MV_6180_DEV_ID << 16) | MV_6180_A0_REV)
#define MV_6180_A0_NAME         	"88F6180 A0"

#define MV_6281_A1_REV              0x3
#define MV_6281_A1_ID               ((MV_6281_DEV_ID << 16) | MV_6281_A1_REV)
#define MV_6281_A1_NAME             "88F6281 A1"

#define MV_6282_A1_REV              0x3
#define MV_6282_A1_ID               ((MV_6282_DEV_ID << 16) | MV_6282_A1_REV)
#define MV_6282_A1_NAME             "88F6282 A1"

#define MV_6280_A1_REV         		0x3
#define MV_6280_A1_ID          		((MV_6280_DEV_ID << 16) | MV_6280_A0_REV)
#define MV_6280_A1_NAME         	"88F6280 A1"

#define MV_6192_A1_REV              0x3
#define MV_6192_A1_ID               ((MV_6192_DEV_ID << 16) | MV_6192_A1_REV)
#define MV_6192_A1_NAME             "88F6192 A1"

#define MV_6190_A1_REV              0x3
#define MV_6190_A1_ID               ((MV_6190_DEV_ID << 16) | MV_6190_A1_REV)
#define MV_6190_A1_NAME             "88F6190 A1"

#define MV_6180_A1_REV              0x3
#define MV_6180_A1_ID               ((MV_6180_DEV_ID << 16) | MV_6180_A1_REV)
#define MV_6180_A1_NAME             "88F6180 A1"

#define MV_88F6XXX_A0_REV         	0x2
#define MV_88F6XXX_A1_REV         	0x3
/* Disco-Duo */
#define MV_78XX0_ZY_DEV_ID       0x6381
#define MV_78XX0_ZY_NAME         "MV78X00"

#define MV_78XX0_Z0_REV         0x1
#define MV_78XX0_Z0_ID          ((MV_78XX0_ZY_DEV_ID << 16) | MV_78XX0_Z0_REV)
#define MV_78XX0_Z0_NAME        "78X00 Z0"

#define MV_78XX0_Y0_REV         0x2
#define MV_78XX0_Y0_ID          ((MV_78XX0_ZY_DEV_ID << 16) | MV_78XX0_Y0_REV)
#define MV_78XX0_Y0_NAME        "78X00 Y0"

#define MV_78XX0_DEV_ID       	0x7800
#define MV_78XX0_NAME         	"MV78X00"

#define MV_76100_DEV_ID      	0x7610
#define MV_78200_DEV_ID      	0x7820
#define MV_78100_DEV_ID      	0x7810
#define MV_78XX0_A0_REV		0x1
#define MV_78XX0_A1_REV		0x2

#define MV_76100_NAME		"MV76100"
#define MV_78100_NAME		"MV78100"
#define MV_78200_NAME		"MV78200"

#define MV_76100_A0_ID		((MV_76100_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78100_A0_ID		((MV_78100_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78200_A0_ID		((MV_78200_DEV_ID << 16) | MV_78XX0_A0_REV)

#define MV_76100_A1_ID		((MV_76100_DEV_ID << 16) | MV_78XX0_A1_REV)
#define MV_78100_A1_ID		((MV_78100_DEV_ID << 16) | MV_78XX0_A1_REV)
#define MV_78200_A1_ID		((MV_78200_DEV_ID << 16) | MV_78XX0_A1_REV)

#define MV_76100_A0_NAME	"MV76100 A0"
#define MV_78100_A0_NAME	"MV78100 A0"
#define MV_78200_A0_NAME	"MV78200 A0"
#define MV_78XX0_A0_NAME	"MV78XX0 A0"

#define MV_76100_A1_NAME	"MV76100 A1"
#define MV_78100_A1_NAME	"MV78100 A1"
#define MV_78200_A1_NAME	"MV78200 A1"
#define MV_78XX0_A1_NAME	"MV78XX0 A1"

/*MV88F632X family*/
#define MV_6321_DEV_ID      	0x6321
#define MV_6322_DEV_ID      	0x6322
#define MV_6323_DEV_ID      	0x6323

#define MV_6321_NAME		"88F6321"
#define MV_6322_NAME		"88F6322"
#define MV_6323_NAME		"88F6323"

#define MV_632X_A1_REV		0x2

#define MV_6321_A1_ID		((MV_6321_DEV_ID << 16) | MV_632X_A1_REV)
#define MV_6322_A1_ID		((MV_6322_DEV_ID << 16) | MV_632X_A1_REV)
#define MV_6323_A1_ID		((MV_6323_DEV_ID << 16) | MV_632X_A1_REV)

#define MV_6321_A1_NAME		"88F6321 A1"
#define MV_6322_A1_NAME		"88F6322 A1"
#define MV_6323_A1_NAME		"88F6323 A1"

/*MV88F6500 family*/
#define MV_65XX_DEV_ID		0x6500
#define MV_6510_DEV_ID		0x6510
#define MV_6530_DEV_ID		0x6530
#define MV_6550_DEV_ID		0x6550
#define MV_6560_DEV_ID		0x6560

#define MV_6510_Z0_REV         		0x1
#define MV_6510_Z0_ID          		((MV_6510_DEV_ID << 16) | MV_6510_Z0_REV)
#define MV_6510_Z0_NAME         	"88F6510 Z0"

#define MV_6530_Z0_REV         		0x1
#define MV_6530_Z0_ID          		((MV_6530_DEV_ID << 16) | MV_6530_Z0_REV)
#define MV_6530_Z0_NAME         	"88F6530 Z0"

#define MV_6550_Z0_REV         		0x1
#define MV_6550_Z0_ID          		((MV_6550_DEV_ID << 16) | MV_6550_Z0_REV)
#define MV_6550_Z0_NAME         	"88F6550 Z0"

#define MV_6560_Z0_REV         		0x1
#define MV_6560_Z0_ID          		((MV_6560_DEV_ID << 16) | MV_6560_Z0_REV)
#define MV_6560_Z0_NAME         	"88F6560 Z0"


/* KW40 */
#define MV_67XX			0x6700
#define MV_6710_DEV_ID		0x6710

#define MV_6710_Z1_REV		0x0
#define MV_6710_Z1_ID		((MV_6710_DEV_ID << 16) | MV_6710_Z1_REV)
#define MV_6710_Z1_NAME		"MV6710 Z1"
#define MV_6710_A0_REV          0x0
#define MV_6710_A0_ID           ((MV_6710_DEV_ID << 16) | MV_6710_A0_REV)
#define MV_6710_A0_NAME         "MV6710 A0"

#define MV_6710_A1_REV          0x1
#define MV_6710_A1_ID           ((MV_6710_DEV_ID << 16) | MV_6710_A1_REV)
#define MV_6710_A1_NAME         "MV6710 A1"

#define MV_6W11_DEV_ID          0x6711
#define MV_6W11_A0_REV          0x0
#define MV_6W11_A0_ID           ((MV_6W11_DEV_ID << 16) | MV_6W11_A0_REV)
#define MV_6W11_A0_NAME         "MV6W11 A0"

#define MV_6W11_A1_REV          0x1
#define MV_6W11_A1_ID           ((MV_6W11_DEV_ID << 16) | MV_6W11_A1_REV)
#define MV_6W11_A1_NAME         "MV6W11 A1"

#define MV_6707_DEV_ID          0x6707
#define MV_6707_A0_REV          0x0
#define MV_6707_A0_ID           ((MV_6707_DEV_ID << 16) | MV_6707_A0_REV)
#define MV_6707_A0_NAME         "MV6707 A0"

#define MV_6707_A1_REV          0x1
#define MV_6707_A1_ID           ((MV_6707_DEV_ID << 16) | MV_6707_A1_REV)
#define MV_6707_A1_NAME         "MV6707 A1"


/* Armada XP Family */
#define MV_78XX0		0x78000
#define MV_78130_DEV_ID		0x7813
#define MV_78160_DEV_ID		0x7816
#define MV_78230_DEV_ID		0x7823
#define MV_78260_DEV_ID		0x7826
#define MV_78460_DEV_ID		0x7846
#define MV_78000_DEV_ID		0x7888

#define MV_FPGA_DEV_ID		0x2107

#define MV_78XX0_Z1_REV		0x0

#define MV_78130_Z1_ID		((MV_78130_DEV_ID << 16) | MV_78XX0_Z1_REV)
#define MV_78130_Z1_NAME	"MV78130 Z1"

#define MV_78160_Z1_ID		((MV_78160_DEV_ID << 16) | MV_78XX0_Z1_REV)
#define MV_78160_Z1_NAME	"MV78160 Z1"

#define MV_78230_Z1_ID		((MV_78230_DEV_ID << 16) | MV_78XX0_Z1_REV)
#define MV_78230_Z1_NAME	"MV78230 Z1"

#define MV_78260_Z1_ID		((MV_78260_DEV_ID << 16) | MV_78XX0_Z1_REV)
#define MV_78260_Z1_NAME	"MV78260 Z1"

#define MV_78460_Z1_ID		((MV_78460_DEV_ID << 16) | MV_78XX0_Z1_REV)
#define MV_78460_Z1_NAME	"MV78460 Z1"

#define MV_78XX0_A0_REV		0x1

#define MV_78130_A0_ID         ((MV_78130_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78130_A0_NAME       "MV78130 A0"

#define MV_78160_A0_ID         ((MV_78160_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78160_A0_NAME       "MV78160 A0"

#define MV_78230_A0_ID         ((MV_78230_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78230_A0_NAME       "MV78230 A0"

#define MV_78260_A0_ID         ((MV_78260_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78260_A0_NAME       "MV78260 A0"

#define MV_78460_A0_ID         ((MV_78460_DEV_ID << 16) | MV_78XX0_A0_REV)
#define MV_78460_A0_NAME       "MV78460 A0"

#define MV_78XX0_B0_REV		0x2

#define MV_78130_B0_ID         ((MV_78130_DEV_ID << 16) | MV_78XX0_B0_REV)
#define MV_78130_B0_NAME       "MV78130 B0"

#define MV_78160_B0_ID         ((MV_78160_DEV_ID << 16) | MV_78XX0_B0_REV)
#define MV_78160_B0_NAME       "MV78160 B0"

#define MV_78230_B0_ID         ((MV_78230_DEV_ID << 16) | MV_78XX0_B0_REV)
#define MV_78230_B0_NAME       "MV78230 B0"

#define MV_78260_B0_ID         ((MV_78260_DEV_ID << 16) | MV_78XX0_B0_REV)
#define MV_78260_B0_NAME       "MV78260 B0"

#define MV_78460_B0_ID         ((MV_78460_DEV_ID << 16) | MV_78XX0_B0_REV)
#define MV_78460_B0_NAME       "MV78460 B0"

/* Avanta LP Family */
#define MV_88F66X0		0x6600
#define MV_6610_DEV_ID		0x6610
#define MV_6610F_DEV_ID		0x610F
#define MV_6650_DEV_ID		0x6650
#define MV_6650F_DEV_ID		0x650F
#define MV_6658_DEV_ID		0x6658
#define MV_6660_DEV_ID		0x6660
#define MV_6665_DEV_ID		0x6665

#define MV_88F66X0_Z1_ID	0x0
#define MV_88F66X0_Z1_NAME      "Z1"
#define MV_88F66X0_Z2_ID	0x1
#define MV_88F66X0_Z2_NAME      "Z2"
#define MV_88F66X0_Z3_ID	0x2
#define MV_88F66X0_Z3_NAME      "Z3"
#define MV_88F66XX_A0_ID	0x3
#define MV_88F66XX_A0_NAME	"A0"

#define MV_88F66X0_ID_ARRAY { \
	MV_88F66X0_Z1_NAME,\
	MV_88F66X0_Z2_NAME,\
	MV_88F66X0_Z3_NAME,\
	MV_88F66XX_A0_NAME \
};

/* Armada 375 Family */
#define MV_88F67X0			0x6700
#define MV_6720_DEV_ID		0x6720
#define MV_88F6720_Z1_ID	0x0
#define MV_88F6720_Z1_NAME	"Z1"
#define MV_88F6720_Z2_ID	0x1
#define MV_88F6720_Z2_NAME      "Z2"
#define MV_88F6720_Z3_ID	0x2
#define MV_88F6720_Z3_NAME      "Z3"
#define MV_88F672X_A0_ID	0x3
#define MV_88F672X_A0_NAME	"A0"

#define MV_88F672X_ID_ARRAY { \
	MV_88F6720_Z1_NAME,\
	MV_88F6720_Z2_NAME,\
	MV_88F6720_Z3_NAME,\
	MV_88F672X_A0_NAME \
};

/* Armada 38x Family */
#define MV_88F68XX		0x6800
#define MV_6810_DEV_ID		0x6810
#define MV_6811_DEV_ID		0x6811
#define MV_6820_DEV_ID		0x6820
#define MV_6828_DEV_ID		0x6828

#define MV_88F68XX_69XX_Z1_ID		0x0
#define MV_88F68XX_69XX_Z1_NAME		"Z1"
#define MV_88F68XX_69XX_A0_ID		0x1
#define MV_88F68XX_69XX_A0_NAME		"A0"

#define MV_88F68XX_69XX_ID_ARRAY { \
	MV_88F68XX_69XX_Z1_NAME,\
	MV_88F68XX_69XX_A0_NAME,\
};

/* Armada 39x Family */
#define MV_88F69XX		0x6900
#define MV_6910_DEV_ID		0x6910
#define MV_6920_DEV_ID		0x6920
#define MV_6928_DEV_ID		0x6928

/* BobCat2  Family */
#define MV_BOBCAT2_DEV_ID		0xFC00

 /* Lion2  Family */
#define MV_LION2_DEV_ID		0x8000

/* AlleyCat3  Family */
#define MV_ALLEYCAT3_DEV_ID		0xF400

 /* IDT Swicth  */
#define PCI_VENDOR_ID_IDT_SWITCH	0x111D
#define MV_IDT_SWITCH_DEV_ID_808E	0x808E
#define MV_IDT_SWITCH_DEV_ID_802B	0x802B

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif				/* __INCmvDeviceIdh */
