/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/

#ifndef __mv_a2m_h__
#define __mv_a2m_h__


#define MV_PP3_A2M_MAX_MASTER		2
#define MV_PP3_A2M_MAX_DECODE_WIN	8

#define MV_PP3_A2M_WIN_CTRL_REG(win)	(0x00 + 8 * (win))

/* Fields of MV_NSS_A2M_WIN_CTRL_REG regsiter */
#define PP3_A2M_WIN_ENABLE_BIT		0
#define PP3_A2M_WIN_ENABLE_MASK		(1 << PP3_A2M_WIN_ENABLE_BIT)

#define PP3_A2M_WIN_USR_ATTR_BIT	1
#define PP3_A2M_WIN_USR_ATTR_MASK	(1 << PP3_A2M_WIN_USR_ATTR_BIT)

#define PP3_A2M_WIN_TARGET_OFFS		4
#define PP3_A2M_WIN_TARGET_MASK		(0xf << PP3_A2M_WIN_TARGET_OFFS)

/* The target associated with this window*/
#define PP3_A2M_WIN_TARGET_OFFS		4
#define PP3_A2M_WIN_TARGET_MASK		(0xf << PP3_A2M_WIN_TARGET_OFFS)

/* The target attributes associated with window */
#define PP3_A2M_WIN_ATTR_OFFS		8
#define PP3_A2M_WIN_ATTR_MASK		(0xff << PP3_A2M_WIN_ATTR_OFFS)

#define PP3_A2M_WIN_SIZE_OFFS		16
#define PP3_A2M_WIN_SIZE_MASK		(0xFFFF << PP3_A2M_WIN_SIZE_OFFS)
/*-------------------------------------------------------------------------*/

#define MV_PP3_A2M_WIN_BASE_REG(win)	(0x04 + 8 * (win))

/* The Base address associated with window */
#define PP3_A2M_WIN_BASE_OFFS		16
#define PP3_A2M_WIN_BASE_MASK		(0xFFFF << PP3_A2M_WIN_BASE_OFFS)
/*-------------------------------------------------------------------------*/

#define PP3_AMB_CTRL0_REG(n)		(0xC0 + (n) * 0x20)
#define PP3_AMB_CTRL1_REG(n)		(0xC4 + (n) * 0x20)
#define PP3_AMB_MASK0_REG(n)		(0xD0 + (n) * 0x20)
#define PP3_AMB_MASK1_REG(n)		(0xD4 + (n) * 0x20)
/*-------------------------------------------------------------------------*/

#endif /* __mv_a2m_h__ */
