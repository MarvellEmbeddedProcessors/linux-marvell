/*
* ***************************************************************************
* Copyright (C) 2016 Marvell International Ltd.
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

#ifndef _MVPP2_DEBUG_H_
#define _MVPP2_DEBUG_H_

#include <linux/kernel.h>
#include <linux/skbuff.h>

#define DBG_MSG(fmt, args...)	printk(fmt, ## args)

void mv_pp2x_skb_dump(struct sk_buff *skb, int size, int access);

int mv_pp2x_debug_param_set(u32 param);

int mv_pp2x_debug_param_get(void);

#endif /* _MVPP2_DEBUG_H_ */
