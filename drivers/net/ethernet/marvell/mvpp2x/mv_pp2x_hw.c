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

#ifdef ARMADA_390
#include <linux/phy.h>
#endif
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/inetdevice.h>
#include <uapi/linux/ppp_defs.h>

#include <net/ip.h>
#include <net/ipv6.h>

#include "mv_pp2x.h"
#include "mv_pp2x_hw.h"
#include "mv_pp2x_debug.h"

/* Utility/helper methods */

/*#define MVPP2_REG_BUF_SIZE (sizeof(last_used)/sizeof(last_used[0]))*/
#define MVPP2_REG_BUF_SIZE ARRAY_SIZE(last_used)

int mv_pp2x_ptr_validate(const void *ptr)
{
	if (ptr == NULL) {
		pr_err("%s: null pointer.\n", __func__);
		return MV_ERROR;
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_ptr_validate);

int mv_pp2x_range_validate(int value, int min, int max)
{
	if (((value) > (max)) || ((value) < (min))) {
		pr_err("%s: value 0x%X (%d) is out of range [0x%X , 0x%X].\n",
			__func__, (value), (value), (min), (max));
		return MV_ERROR;
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_range_validate);

void mv_pp2x_write(struct mv_pp2x_hw *hw, u32 offset, u32 data)
{
	void *reg_ptr = hw->cpu_base[smp_processor_id()] + offset;

#if defined(MVPP2_DEBUG) && !defined(CONFIG_MV_PP2_PALLADIUM)
	static void *last_used[20] = {0};
	static int next_write;
	int i;

	for (i = 0; i < MVPP2_REG_BUF_SIZE; i++) {
		if (last_used[i] == reg_ptr)
			break;
	}
	if (i == MVPP2_REG_BUF_SIZE) {
		pr_debug("NEW REG: mv_pp2x_write(%p)\n", reg_ptr);
		last_used[next_write] = reg_ptr;
		next_write++;
		next_write = next_write % MVPP2_REG_BUF_SIZE;
	} else {
		/*pr_info("mv_pp2x_write(%d)=%d , caller %pS\n",
		*	offset, data, __builtin_return_address(0));
		*/
	}
#endif
	if (debug_param)
		pr_info("mv_pp2x_write: 0x%p data=0x%x\n", reg_ptr, data);
	writel(data, reg_ptr);
}
EXPORT_SYMBOL(mv_pp2x_write);


void mv_pp2x_relaxed_write(struct mv_pp2x_hw *hw, u32 offset, u32 data)
{
	void *reg_ptr = hw->cpu_base[smp_processor_id()] + offset;

	writel_relaxed(data, reg_ptr);
}
EXPORT_SYMBOL(mv_pp2x_relaxed_write);


u32 mv_pp2x_read(struct mv_pp2x_hw *hw, u32 offset)
{
#if defined(MVPP2_DEBUG) && !defined(CONFIG_MV_PP2_PALLADIUM)
	static void *last_used[20] = {0};
	static int next_write;
	int i;
#endif
	void *reg_ptr = hw->cpu_base[smp_processor_id()] + offset;
	u32 val;

	val = readl(reg_ptr);
#if defined(MVPP2_DEBUG) && !defined(CONFIG_MV_PP2_PALLADIUM)
	for (i = 0; i < MVPP2_REG_BUF_SIZE; i++) {
		if (last_used[i] == reg_ptr)
			break;
	}
	if (i == MVPP2_REG_BUF_SIZE) {
		pr_debug("NEW REG: mv_pp2x_read(%p)\n", reg_ptr);
		last_used[next_write] = reg_ptr;
		next_write++;
		next_write = next_write % MVPP2_REG_BUF_SIZE;
	} else {
		/*pr_info("mv_pp2x_read(%d)=%d , caller %pS\n",
		 *	offset, val, __builtin_return_address(0));
		*/
	}

	if (debug_param)
		pr_info("mv_pp2x_read: 0x%p data=0x%x\n", reg_ptr, val);

#endif

	return val;
}
EXPORT_SYMBOL(mv_pp2x_read);


u32 mv_pp2x_relaxed_read(struct mv_pp2x_hw *hw, u32 offset)
{
	void *reg_ptr = hw->cpu_base[smp_processor_id()] + offset;
	u32 val;

	val = readl_relaxed(reg_ptr);
	return val;
}
EXPORT_SYMBOL(mv_pp2x_relaxed_read);


/* Parser configuration routines */

/* Flow ID definetion array */
static struct mv_pp2x_prs_flow_id
	mv_pp2x_prs_flow_id_array[MVPP2_PRS_FL_TCAM_NUM] = {
	/***********#Flow ID#**************#Result Info#************/
	{MVPP2_PRS_FL_IP4_TCP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_UDP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_TCP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_UDP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_NF_TAG,	{MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_TCP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_TCP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_UDP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_UDP_NF_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					 MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_VLAN_MASK |
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_TCP_NF_TAG,	{MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_TCP_NF_TAG,	{MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_UDP_NF_TAG,	{MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_UDP_NF_TAG,	{MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_FALSE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_TCP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4 |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_TCP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4_OPT |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_TCP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4_OTHER |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_TCP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_UDP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4 |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_UDP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4_OPT |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_UDP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP4_OTHER |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_UDP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_TCP_FRAG_TAG, {MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_FRAG_TAG, {MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TCP_FRAG_TAG, {MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_UDP_FRAG_TAG, {MVPP2_PRS_RI_L3_IP4 |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_FRAG_TAG,	{MVPP2_PRS_RI_L3_IP4_OPT |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UDP_FRAG_TAG,	{MVPP2_PRS_RI_L3_IP4_OTHER |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_TCP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP6 |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_TCP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_TCP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP6_EXT |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_TCP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_UDP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP6 |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_UDP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_UDP_FRAG_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
					   MVPP2_PRS_RI_L3_IP6_EXT |
					   MVPP2_PRS_RI_IP_FRAG_TRUE |
					   MVPP2_PRS_RI_L4_UDP,
					   MVPP2_PRS_RI_VLAN_MASK |
					   MVPP2_PRS_RI_L3_PROTO_MASK |
					   MVPP2_PRS_RI_IP_FRAG_MASK |
					   MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_TCP_FRAG_TAG,	{MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_TCP_FRAG_TAG,	{MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_TCP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_UDP_FRAG_TAG,	{MVPP2_PRS_RI_L3_IP6 |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_UDP_FRAG_TAG, {MVPP2_PRS_RI_L3_IP6_EXT |
					 MVPP2_PRS_RI_IP_FRAG_TRUE |
					 MVPP2_PRS_RI_L4_UDP,
					 MVPP2_PRS_RI_L3_PROTO_MASK |
					 MVPP2_PRS_RI_IP_FRAG_MASK |
					 MVPP2_PRS_RI_L4_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
				  MVPP2_PRS_RI_L3_IP4,
				  MVPP2_PRS_RI_VLAN_MASK |
				  MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
				  MVPP2_PRS_RI_L3_IP4_OPT,
				  MVPP2_PRS_RI_VLAN_MASK |
				  MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
				  MVPP2_PRS_RI_L3_IP4_OTHER,
				  MVPP2_PRS_RI_VLAN_MASK |
				  MVPP2_PRS_RI_L3_PROTO_MASK} },

	{MVPP2_PRS_FL_IP4_TAG, {MVPP2_PRS_RI_L3_IP4,
				MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TAG, {MVPP2_PRS_RI_L3_IP4_OPT,
				MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP4_TAG, {MVPP2_PRS_RI_L3_IP4_OTHER,
				MVPP2_PRS_RI_L3_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
				  MVPP2_PRS_RI_L3_IP6,
				  MVPP2_PRS_RI_VLAN_MASK |
				  MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_UNTAG, {MVPP2_PRS_RI_VLAN_NONE |
				  MVPP2_PRS_RI_L3_IP6_EXT,
				  MVPP2_PRS_RI_VLAN_MASK |
				  MVPP2_PRS_RI_L3_PROTO_MASK} },

	{MVPP2_PRS_FL_IP6_TAG, {MVPP2_PRS_RI_L3_IP6,
				MVPP2_PRS_RI_L3_PROTO_MASK} },
	{MVPP2_PRS_FL_IP6_TAG, {MVPP2_PRS_RI_L3_IP6_EXT,
				MVPP2_PRS_RI_L3_PROTO_MASK} },

	{MVPP2_PRS_FL_NON_IP_UNTAG, {MVPP2_PRS_RI_VLAN_NONE,
				     MVPP2_PRS_RI_VLAN_MASK} },

	{MVPP2_PRS_FL_NON_IP_TAG, {0, 0} },
};

/* Array of bitmask to indicate flow id attribute */
static int mv_pp2x_prs_flow_id_attr_tbl[MVPP2_PRS_FL_LAST];

/* Update parser tcam and sram hw entries */
int mv_pp2x_prs_hw_write(struct mv_pp2x_hw *hw, struct mv_pp2x_prs_entry *pe)
{
	int i;

	if (pe->index > MVPP2_PRS_TCAM_SRAM_SIZE - 1)
		return -EINVAL;

	/* Clear entry invalidation bit */
	pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] &= ~MVPP2_PRS_TCAM_INV_MASK;

	/* Write tcam index - indirect access */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
		mv_pp2x_write(hw, MVPP2_PRS_TCAM_DATA_REG(i), pe->tcam.word[i]);

	/* Write sram index - indirect access */
	mv_pp2x_write(hw, MVPP2_PRS_SRAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
		mv_pp2x_write(hw, MVPP2_PRS_SRAM_DATA_REG(i), pe->sram.word[i]);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_write);

/* Read tcam entry from hw */
int mv_pp2x_prs_hw_read(struct mv_pp2x_hw *hw, struct mv_pp2x_prs_entry *pe)
{
	int i;

	if (pe->index > MVPP2_PRS_TCAM_SRAM_SIZE - 1)
		return -EINVAL;

	/* Write tcam index - indirect access */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_IDX_REG, pe->index);

	pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] = mv_pp2x_read(hw,
			      MVPP2_PRS_TCAM_DATA_REG(MVPP2_PRS_TCAM_INV_WORD));
	if (pe->tcam.word[MVPP2_PRS_TCAM_INV_WORD] & MVPP2_PRS_TCAM_INV_MASK)
		return MVPP2_PRS_TCAM_ENTRY_INVALID;

	for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
		pe->tcam.word[i] = mv_pp2x_read(hw, MVPP2_PRS_TCAM_DATA_REG(i));

	/* Write sram index - indirect access */
	mv_pp2x_write(hw, MVPP2_PRS_SRAM_IDX_REG, pe->index);
	for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
		pe->sram.word[i] = mv_pp2x_read(hw, MVPP2_PRS_SRAM_DATA_REG(i));

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_read);

void mv_pp2x_prs_sw_clear(struct mv_pp2x_prs_entry *pe)
{
	memset(pe, 0, sizeof(struct mv_pp2x_prs_entry));
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_clear);

/* Invalidate tcam hw entry */
void mv_pp2x_prs_hw_inv(struct mv_pp2x_hw *hw, int index)
{
	/* Write index - indirect access */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_IDX_REG, index);
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_DATA_REG(MVPP2_PRS_TCAM_INV_WORD),
		      MVPP2_PRS_TCAM_INV_MASK);
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_inv);

/* Enable shadow table entry and set its lookup ID */
static void mv_pp2x_prs_shadow_set(struct mv_pp2x_hw *hw, int index, int lu)
{
	hw->prs_shadow[index].valid = true;
	hw->prs_shadow[index].lu = lu;
}

/* Update ri fields in shadow table entry */
static void mv_pp2x_prs_shadow_ri_set(struct mv_pp2x_hw *hw, int index,
				      unsigned int ri,
				      unsigned int ri_mask)
{
	hw->prs_shadow[index].ri_mask = ri_mask;
	hw->prs_shadow[index].ri = ri;
}

/* Update lookup field in tcam sw entry */
void mv_pp2x_prs_tcam_lu_set(struct mv_pp2x_prs_entry *pe, unsigned int lu)
{
	unsigned int offset = MVPP2_PRS_TCAM_LU_BYTE;
	unsigned int enable_off =
		MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_LU_BYTE);

	pe->tcam.byte[HW_BYTE_OFFS(offset)] = lu;
	pe->tcam.byte[HW_BYTE_OFFS(enable_off)] = MVPP2_PRS_LU_MASK;
}
EXPORT_SYMBOL(mv_pp2x_prs_tcam_lu_set);

/* Update mask for single port in tcam sw entry */
void mv_pp2x_prs_tcam_port_set(struct mv_pp2x_prs_entry *pe,
			       unsigned int port, bool add)
{
	int enable_off =
		HW_BYTE_OFFS(MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE));

	if (add)
		pe->tcam.byte[enable_off] &= ~(1 << port);
	else
		pe->tcam.byte[enable_off] |= 1 << port;
}
EXPORT_SYMBOL(mv_pp2x_prs_tcam_port_set);

/* Update port map in tcam sw entry */
void mv_pp2x_prs_tcam_port_map_set(struct mv_pp2x_prs_entry *pe,
				   unsigned int ports)
{
	unsigned char port_mask = MVPP2_PRS_PORT_MASK;
	int enable_off =
		HW_BYTE_OFFS(MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE));

	pe->tcam.byte[HW_BYTE_OFFS(MVPP2_PRS_TCAM_PORT_BYTE)] = 0;
	pe->tcam.byte[enable_off] &= ~port_mask;
	pe->tcam.byte[enable_off] |= ~ports & MVPP2_PRS_PORT_MASK;
}
EXPORT_SYMBOL(mv_pp2x_prs_tcam_port_map_set);

/* Obtain port map from tcam sw entry */
static unsigned int mv_pp2x_prs_tcam_port_map_get(struct mv_pp2x_prs_entry *pe)
{
	int enable_off =
		HW_BYTE_OFFS(MVPP2_PRS_TCAM_EN_OFFS(MVPP2_PRS_TCAM_PORT_BYTE));

	return ~(pe->tcam.byte[enable_off]) & MVPP2_PRS_PORT_MASK;
}

/* Set byte of data and its enable bits in tcam sw entry */
void mv_pp2x_prs_tcam_data_byte_set(struct mv_pp2x_prs_entry *pe,
				    unsigned int offs,
				    unsigned char byte,
				    unsigned char enable)
{
	pe->tcam.byte[TCAM_DATA_BYTE(offs)] = byte;
	pe->tcam.byte[TCAM_DATA_MASK(offs)] = enable;
}
EXPORT_SYMBOL(mv_pp2x_prs_tcam_data_byte_set);

/* Get byte of data and its enable bits from tcam sw entry */
static void mv_pp2x_prs_tcam_data_byte_get(struct mv_pp2x_prs_entry *pe,
					   unsigned int offs,
					   unsigned char *byte,
					   unsigned char *enable)
{
	*byte = pe->tcam.byte[TCAM_DATA_BYTE(offs)];
	*enable = pe->tcam.byte[TCAM_DATA_MASK(offs)];
}

/* Set dword of data and its enable bits in tcam sw entry */
static void mv_pp2x_prs_tcam_data_dword_set(struct mv_pp2x_prs_entry *pe,
					    unsigned int offs,
					    unsigned int word,
					    unsigned int enable)
{
	int index, offset;
	unsigned char byte, byte_mask;

	for (index = 0; index < 4; index++) {
		offset = (offs * 4) + index;
		byte = ((unsigned char *)&word)[HW_BYTE_OFFS(index)];
		byte_mask = ((unsigned char *)&enable)[HW_BYTE_OFFS(index)];
		mv_pp2x_prs_tcam_data_byte_set(pe, offset, byte, byte_mask);
	}
}

/* Get dword of data and its enable bits from tcam sw entry */
static void mv_pp2x_prs_tcam_data_dword_get(struct mv_pp2x_prs_entry *pe,
					    unsigned int offs,
					    unsigned int *word,
					    unsigned int *enable)
{
	int index, offset;
	unsigned char byte, mask;

	for (index = 0; index < 4; index++) {
		offset = (offs * 4) + index;
		mv_pp2x_prs_tcam_data_byte_get(pe, offset,  &byte, &mask);
		((unsigned char *)word)[HW_BYTE_OFFS(index)] = byte;
		((unsigned char *)enable)[HW_BYTE_OFFS(index)] = mask;
	}
}

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Compare tcam data bytes with a pattern */
static bool mv_pp2x_prs_tcam_data_cmp(struct mv_pp2x_prs_entry *pe, int offs,
				      u16 data)
{
	u16 tcam_data;

	tcam_data = (pe->tcam.byte[TCAM_DATA_BYTE(offs + 1)] << 8) | pe->tcam.byte[TCAM_DATA_BYTE(offs)];
	if (tcam_data != data)
		return false;
	return true;
}
#endif

/* Update ai bits in tcam sw entry */
void mv_pp2x_prs_tcam_ai_update(struct mv_pp2x_prs_entry *pe,
				unsigned int bits,
				unsigned int enable)
{
	int i, ai_idx = MVPP2_PRS_TCAM_AI_BYTE;

	for (i = 0; i < MVPP2_PRS_AI_BITS; i++) {

		if (!(enable & BIT(i)))
			continue;

		if (bits & BIT(i))
			pe->tcam.byte[HW_BYTE_OFFS(ai_idx)] |= 1 << i;
		else
			pe->tcam.byte[HW_BYTE_OFFS(ai_idx)] &= ~(1 << i);
	}

	pe->tcam.byte[HW_BYTE_OFFS(MVPP2_PRS_TCAM_EN_OFFS(ai_idx))] |= enable;
}
EXPORT_SYMBOL(mv_pp2x_prs_tcam_ai_update);


#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Get ai bits from tcam sw entry */
static int mv_pp2x_prs_tcam_ai_get(struct mv_pp2x_prs_entry *pe)
{
	return pe->tcam.byte[HW_BYTE_OFFS(MVPP2_PRS_TCAM_AI_BYTE)];
}
#endif

/* Set ethertype in tcam sw entry */
static void mv_pp2x_prs_match_etype(struct mv_pp2x_prs_entry *pe, int offset,
				    unsigned short ethertype)
{
	mv_pp2x_prs_tcam_data_byte_set(pe, offset + 0, ethertype >> 8, 0xff);
	mv_pp2x_prs_tcam_data_byte_set(pe, offset + 1, ethertype & 0xff, 0xff);
}

/* Set bits in sram sw entry */
static void mv_pp2x_prs_sram_bits_set(struct mv_pp2x_prs_entry *pe, int bit_num,
				      int val)
{
	pe->sram.byte[SRAM_BIT_TO_BYTE(bit_num)] |= (val << (bit_num % 8));
}

/* Clear bits in sram sw entry */
static void mv_pp2x_prs_sram_bits_clear(struct mv_pp2x_prs_entry *pe,
					int bit_num, int val)
{
	pe->sram.byte[SRAM_BIT_TO_BYTE(bit_num)] &= ~(val << (bit_num % 8));
}

/* Update ri bits in sram sw entry */
void mv_pp2x_prs_sram_ri_update(struct mv_pp2x_prs_entry *pe,
				unsigned int bits, unsigned int mask)
{
	unsigned int i;

	for (i = 0; i < MVPP2_PRS_SRAM_RI_CTRL_BITS; i++) {
		int ri_off = MVPP2_PRS_SRAM_RI_OFFS;

		if (!(mask & BIT(i)))
			continue;

		if (bits & BIT(i))
			mv_pp2x_prs_sram_bits_set(pe, ri_off + i, 1);
		else
			mv_pp2x_prs_sram_bits_clear(pe, ri_off + i, 1);

		mv_pp2x_prs_sram_bits_set(pe,
			MVPP2_PRS_SRAM_RI_CTRL_OFFS + i, 1);
	}
}
EXPORT_SYMBOL(mv_pp2x_prs_sram_ri_update);

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Obtain ri bits from sram sw entry */
static int mv_pp2x_prs_sram_ri_get(struct mv_pp2x_prs_entry *pe)
{
	return pe->sram.word[MVPP2_PRS_SRAM_RI_WORD];
}
#endif

/* Update ai bits in sram sw entry */
void mv_pp2x_prs_sram_ai_update(struct mv_pp2x_prs_entry *pe,
				unsigned int bits, unsigned int mask)
{
	unsigned int i;
	int ai_off = MVPP2_PRS_SRAM_AI_OFFS;

	for (i = 0; i < MVPP2_PRS_SRAM_AI_CTRL_BITS; i++) {

		if (!(mask & BIT(i)))
			continue;

		if (bits & BIT(i))
			mv_pp2x_prs_sram_bits_set(pe, ai_off + i, 1);
		else
			mv_pp2x_prs_sram_bits_clear(pe, ai_off + i, 1);

		mv_pp2x_prs_sram_bits_set(pe,
			MVPP2_PRS_SRAM_AI_CTRL_OFFS + i, 1);
	}
}
EXPORT_SYMBOL(mv_pp2x_prs_sram_ai_update);

/* Read ai bits from sram sw entry */
static int mv_pp2x_prs_sram_ai_get(struct mv_pp2x_prs_entry *pe)
{
	u8 bits;
	int ai_off = SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_AI_OFFS);
	int ai_en_off = ai_off + 1;
	int ai_shift = MVPP2_PRS_SRAM_AI_OFFS % 8;

	bits = (pe->sram.byte[ai_off] >> ai_shift) |
	       (pe->sram.byte[ai_en_off] << (8 - ai_shift));

	return bits;
}

/* In sram sw entry set lookup ID field of the tcam key to be used in the next
 * lookup interation
 */
void mv_pp2x_prs_sram_next_lu_set(struct mv_pp2x_prs_entry *pe,
				  unsigned int lu)
{
	int sram_next_off = MVPP2_PRS_SRAM_NEXT_LU_OFFS;

	mv_pp2x_prs_sram_bits_clear(pe, sram_next_off,
				    MVPP2_PRS_SRAM_NEXT_LU_MASK);
	mv_pp2x_prs_sram_bits_set(pe, sram_next_off, lu);
}
EXPORT_SYMBOL(mv_pp2x_prs_sram_next_lu_set);

/* In the sram sw entry set sign and value of the next lookup offset
 * and the offset value generated to the classifier
 */
static void mv_pp2x_prs_sram_shift_set(struct mv_pp2x_prs_entry *pe, int shift,
				       unsigned int op)
{
	/* Set sign */
	if (shift < 0) {
		mv_pp2x_prs_sram_bits_set(pe,
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT, 1);
		shift = 0 - shift;
	} else {
		mv_pp2x_prs_sram_bits_clear(pe,
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT, 1);
	}

	/* Set value */
	pe->sram.byte[SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_SHIFT_OFFS)] =
						   (unsigned char)shift;

	/* Reset and set operation */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS,
				    MVPP2_PRS_SRAM_OP_SEL_SHIFT_MASK);
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS, op);

	/* Set base offset as current */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS, 1);
}

/* In the sram sw entry set sign and value of the user defined offset
 * generated to the classifier
 */
static void mv_pp2x_prs_sram_offset_set(struct mv_pp2x_prs_entry *pe,
					unsigned int type, int offset,
					unsigned int op)
{
	/* Set sign */
	if (offset < 0) {
		mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_SIGN_BIT, 1);
		offset = 0 - offset;
	} else {
		mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_SIGN_BIT, 1);
	}

	/* Set value */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_OFFS,
				    MVPP2_PRS_SRAM_UDF_MASK);
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_OFFS, offset);
	pe->sram.byte[SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS +
					MVPP2_PRS_SRAM_UDF_BITS)] &=
					~(MVPP2_PRS_SRAM_UDF_MASK >>
					(8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8)));
	pe->sram.byte[SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS +
					MVPP2_PRS_SRAM_UDF_BITS)] |=
					(offset >> (8 -
					(MVPP2_PRS_SRAM_UDF_OFFS % 8)));

	/* Set offset type */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_UDF_TYPE_OFFS,
				    MVPP2_PRS_SRAM_UDF_TYPE_MASK);
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_UDF_TYPE_OFFS, type);

	/* Set offset operation */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_MASK);
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS, op);

	pe->sram.byte[SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
					MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] &=
					~(MVPP2_PRS_SRAM_OP_SEL_UDF_MASK >>
					(8 -
					(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	pe->sram.byte[SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
					MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] |=
					(op >> (8 -
					(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	/* Set base offset as current */
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS, 1);
}

/* Find parser flow entry */
static struct mv_pp2x_prs_entry *mv_pp2x_prs_flow_find(struct mv_pp2x_hw *hw,
						       int flow,
						       unsigned int ri,
						       unsigned int ri_mask)
{
	struct mv_pp2x_prs_entry *pe;
	int tid;
	unsigned int dword, enable;

	pe = kzalloc(sizeof(*pe), GFP_KERNEL);
	if (!pe)
		return NULL;
	mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_FLOWS);

	/* Go through the all entires with MVPP2_PRS_LU_FLOWS */
	for (tid = MVPP2_PRS_TCAM_SRAM_SIZE - 1; tid >= 0; tid--) {
		u8 bits;

		if (!hw->prs_shadow[tid].valid ||
		    hw->prs_shadow[tid].lu != MVPP2_PRS_LU_FLOWS)
			continue;

		pe->index = tid;
		mv_pp2x_prs_hw_read(hw, pe);

		/* Check result info, because there maybe several
		 * TCAM lines to generate the same flow
		 */
		mv_pp2x_prs_tcam_data_dword_get(pe, 0, &dword, &enable);
		if ((dword != ri) || (enable != ri_mask))
			continue;

		bits = mv_pp2x_prs_sram_ai_get(pe);

		/* Sram store classification lookup ID in AI bits [5:0] */
		if ((bits & MVPP2_PRS_FLOW_ID_MASK) == flow)
			return pe;
	}
	kfree(pe);

	return NULL;
}

/* Return first free tcam index, seeking from start to end */
static int mv_pp2x_prs_tcam_first_free(struct mv_pp2x_hw *hw,
				       unsigned char start,
				       unsigned char end)
{
	int tid;

	/*pr_crit("mv_pp2x_prs_tcam_first_free start=%d, end=%d, caller=%pS\n",
	 * start, end, __builtin_return_address(0));
	 */
	if (start > end)
		swap(start, end);

	if (end >= MVPP2_PRS_TCAM_SRAM_SIZE)
		end = MVPP2_PRS_TCAM_SRAM_SIZE - 1;

	for (tid = start; tid <= end; tid++) {
		if (!hw->prs_shadow[tid].valid)
			return tid;
	}
	pr_crit("Out of TCAM Entries !!: %s(%d)\n", __FILENAME__, __LINE__);
	return -EINVAL;
}

/* Enable/disable dropping all mac da's */
static void mv_pp2x_prs_mac_drop_all_set(struct mv_pp2x_hw *hw,
					 int port, bool add)
{
	struct mv_pp2x_prs_entry pe;

	if (hw->prs_shadow[MVPP2_PE_DROP_ALL].valid) {
		/* Entry exist - update port only */
		pe.index = MVPP2_PE_DROP_ALL;
		mv_pp2x_prs_hw_read(hw, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
		pe.index = MVPP2_PE_DROP_ALL;

		/* Non-promiscuous mode for all ports - DROP unknown packets */
		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DROP_MASK,
					 MVPP2_PRS_RI_DROP_MASK);

		mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
		mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MAC);

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(&pe, 0);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(&pe, port, add);

	mv_pp2x_prs_hw_write(hw, &pe);
}

/* Set port to promiscuous mode */
void mv_pp2x_prs_mac_promisc_set(struct mv_pp2x_hw *hw, int port, bool add)
{
	struct mv_pp2x_prs_entry pe;

	/* Promiscous mode - Accept unknown packets */

	if (hw->prs_shadow[MVPP2_PE_MAC_PROMISCUOUS].valid) {
		/* Entry exist - update port only */
		pe.index = MVPP2_PE_MAC_PROMISCUOUS;
		mv_pp2x_prs_hw_read(hw, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
		pe.index = MVPP2_PE_MAC_PROMISCUOUS;

		/* Continue - set next lookup */
		mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_DSA);

		/* Set result info bits */
		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L2_UCAST,
					 MVPP2_PRS_RI_L2_CAST_MASK);

		/* Shift to ethertype */
		mv_pp2x_prs_sram_shift_set(&pe, 2 * ETH_ALEN,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(&pe, 0);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MAC);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(&pe, port, add);

	mv_pp2x_prs_hw_write(hw, &pe);
}

/* Accept multicast */
void mv_pp2x_prs_mac_multi_set(struct mv_pp2x_hw *hw, int port, int index,
			       bool add)
{
	struct mv_pp2x_prs_entry pe;
	unsigned char da_mc;

	/* Ethernet multicast address first byte is
	 * 0x01 for IPv4 and 0x33 for IPv6
	 */
	da_mc = (index == MVPP2_PE_MAC_MC_ALL) ? 0x01 : 0x33;

	if (hw->prs_shadow[index].valid) {
		/* Entry exist - update port only */
		pe.index = index;
		mv_pp2x_prs_hw_read(hw, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);
		pe.index = index;

		/* Continue - set next lookup */
		mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_DSA);

		/* Set result info bits */
		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L2_MCAST,
					   MVPP2_PRS_RI_L2_CAST_MASK);

		/* Update tcam entry data first byte */
		mv_pp2x_prs_tcam_data_byte_set(&pe, 0, da_mc, 0xff);

		/* Shift to ethertype */
		mv_pp2x_prs_sram_shift_set(&pe, 2 * ETH_ALEN,
					 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(&pe, 0);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MAC);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(&pe, port, add);

	mv_pp2x_prs_hw_write(hw, &pe);
}

/* Set entry for dsa packets */
static void mv_pp2x_prs_dsa_tag_set(struct mv_pp2x_hw *hw, int port, bool add,
				    bool tagged, bool extend)
{
	struct mv_pp2x_prs_entry pe;
	int tid, shift;

	if (extend) {
		tid = tagged ? MVPP2_PE_EDSA_TAGGED : MVPP2_PE_EDSA_UNTAGGED;
		shift = 8;
	} else {
		tid = tagged ? MVPP2_PE_DSA_TAGGED : MVPP2_PE_DSA_UNTAGGED;
		shift = 4;
	}

	if (hw->prs_shadow[tid].valid) {
		/* Entry exist - update port only */
		pe.index = tid;
		mv_pp2x_prs_hw_read(hw, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
		pe.index = tid;

		/* Shift 4 bytes if DSA tag or 8 bytes in case of EDSA tag*/
		mv_pp2x_prs_sram_shift_set(&pe, shift,
					   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_DSA);

		if (tagged) {
			/* Set tagged bit in DSA tag */
			mv_pp2x_prs_tcam_data_byte_set(&pe, 0,
					MVPP2_PRS_TCAM_DSA_TAGGED_BIT,
					MVPP2_PRS_TCAM_DSA_TAGGED_BIT);
			/* Clear all ai bits for next iteration */
			mv_pp2x_prs_sram_ai_update(&pe, 0,
						   MVPP2_PRS_SRAM_AI_MASK);
			/* If packet is tagged continue check vlans */
			mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);
		} else {
			/* Set result info bits to 'no vlans' */
			mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
						   MVPP2_PRS_RI_VLAN_MASK);
			mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
		}

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(&pe, 0);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(&pe, port, add);

	mv_pp2x_prs_hw_write(hw, &pe);
}

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Set entry for dsa ethertype */
static void mv_pp2x_prs_dsa_tag_ethertype_set(struct mv_pp2x_hw *hw, int port,
					      bool add, bool tagged,
					      bool extend)
{
	struct mv_pp2x_prs_entry pe;
	int tid, shift, port_mask;

	if (extend) {
		tid = tagged ? MVPP2_PE_ETYPE_EDSA_TAGGED :
		      MVPP2_PE_ETYPE_EDSA_UNTAGGED;
		port_mask = 0;
		shift = 8;
	} else {
		tid = tagged ? MVPP2_PE_ETYPE_DSA_TAGGED :
		      MVPP2_PE_ETYPE_DSA_UNTAGGED;
		port_mask = MVPP2_PRS_PORT_MASK;
		shift = 4;
	}

	if (hw->prs_shadow[tid].valid) {
		/* Entry exist - update port only */
		pe.index = tid;
		mv_pp2x_prs_hw_read(hw, &pe);
	} else {
		/* Entry doesn't exist - create new */
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
		pe.index = tid;

		/* Set ethertype */
		mv_pp2x_prs_match_etype(&pe, 0, ETH_P_EDSA);
		mv_pp2x_prs_match_etype(&pe, 2, 0);

		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DSA_MASK,
					   MVPP2_PRS_RI_DSA_MASK);
		/* Shift ethertype + 2 byte reserved + tag*/
		mv_pp2x_prs_sram_shift_set(&pe, 2 + MVPP2_ETH_TYPE_LEN + shift,
					   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_DSA);

		if (tagged) {
			/* Set tagged bit in DSA tag */
			mv_pp2x_prs_tcam_data_byte_set(&pe,
						     MVPP2_ETH_TYPE_LEN + 2 + 3,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT,
						 MVPP2_PRS_TCAM_DSA_TAGGED_BIT);
			/* Clear all ai bits for next iteration */
			mv_pp2x_prs_sram_ai_update(&pe, 0,
						   MVPP2_PRS_SRAM_AI_MASK);
			/* If packet is tagged continue check vlans */
			mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);
		} else {
			/* Set result info bits to 'no vlans' */
			mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
						   MVPP2_PRS_RI_VLAN_MASK);
			mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
		}
		/* Mask/unmask all ports, depending on dsa type */
		mv_pp2x_prs_tcam_port_map_set(&pe, port_mask);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(&pe, port, add);

	mv_pp2x_prs_hw_write(hw, &pe);
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Search for existing single/triple vlan entry */
static struct mv_pp2x_prs_entry *mv_pp2x_prs_vlan_find(struct mv_pp2x_hw *hw,
						       unsigned short tpid,
						       int ai)
{
	struct mv_pp2x_prs_entry *pe;
	int tid;

	pe = kzalloc(sizeof(*pe), GFP_KERNEL);
	if (!pe)
		return NULL;
	mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);

	/* Go through the all entries with MVPP2_PRS_LU_VLAN */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		unsigned int ri_bits, ai_bits;
		bool match;

		if (!hw->prs_shadow[tid].valid ||
		    hw->prs_shadow[tid].lu != MVPP2_PRS_LU_VLAN)
			continue;

		pe->index = tid;

		mv_pp2x_prs_hw_read(hw, pe);
		match = mv_pp2x_prs_tcam_data_cmp(pe, 0, swab16(tpid));
		if (!match)
			continue;

		/* Get vlan type */
		ri_bits = mv_pp2x_prs_sram_ri_get(pe);
		ri_bits &= MVPP2_PRS_RI_VLAN_MASK;

		/* Get current ai value from tcam */
		ai_bits = mv_pp2x_prs_tcam_ai_get(pe);
		/* Clear double vlan bit */
		ai_bits &= ~MVPP2_PRS_DBL_VLAN_AI_BIT;

		if (ai != ai_bits)
			continue;

		if (ri_bits == MVPP2_PRS_RI_VLAN_SINGLE ||
		    ri_bits == MVPP2_PRS_RI_VLAN_TRIPLE)
			return pe;
	}
	kfree(pe);

	return NULL;
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Add/update single/triple vlan entry */
static int mv_pp2x_prs_vlan_add(struct mv_pp2x_hw *hw, unsigned short tpid,
				int ai, unsigned int port_map)
{
	struct mv_pp2x_prs_entry *pe;
	int tid_aux, tid;
	int ret = 0;

	pe = mv_pp2x_prs_vlan_find(hw, tpid, ai);

	if (!pe) {
		/* Create new tcam entry */
		tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_LAST_FREE_TID,
						  MVPP2_PE_FIRST_FREE_TID);
		if (tid < 0)
			return tid;

		pe = kzalloc(sizeof(*pe), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		/* Get last double vlan tid */
		for (tid_aux = MVPP2_PE_LAST_FREE_TID;
		     tid_aux >= MVPP2_PE_FIRST_FREE_TID; tid_aux--) {
			unsigned int ri_bits;

			if (!hw->prs_shadow[tid_aux].valid ||
			    hw->prs_shadow[tid_aux].lu != MVPP2_PRS_LU_VLAN)
				continue;

			pe->index = tid_aux;
			mv_pp2x_prs_hw_read(hw, pe);
			ri_bits = mv_pp2x_prs_sram_ri_get(pe);
			if ((ri_bits & MVPP2_PRS_RI_VLAN_MASK) ==
			    MVPP2_PRS_RI_VLAN_DOUBLE)
				break;
		}

		if (tid <= tid_aux) {
			ret = -EINVAL;
			goto error;
		}

		memset(pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);
		pe->index = tid;

		mv_pp2x_prs_match_etype(pe, 0, tpid);

		mv_pp2x_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_L2);
		/* Shift 4 bytes - skip 1 vlan tag */
		mv_pp2x_prs_sram_shift_set(pe, MVPP2_VLAN_TAG_LEN,
					   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
		/* Clear all ai bits for next iteration */
		mv_pp2x_prs_sram_ai_update(pe, 0, MVPP2_PRS_SRAM_AI_MASK);

		if (ai == MVPP2_PRS_SINGLE_VLAN_AI) {
			mv_pp2x_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_SINGLE,
						   MVPP2_PRS_RI_VLAN_MASK);
		} else {
			ai |= MVPP2_PRS_DBL_VLAN_AI_BIT;
			mv_pp2x_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_TRIPLE,
						   MVPP2_PRS_RI_VLAN_MASK);
		}
		mv_pp2x_prs_tcam_ai_update(pe, ai, MVPP2_PRS_SRAM_AI_MASK);

		mv_pp2x_prs_shadow_set(hw, pe->index, MVPP2_PRS_LU_VLAN);
	}
	/* Update ports' mask */
	mv_pp2x_prs_tcam_port_map_set(pe, port_map);

	mv_pp2x_prs_hw_write(hw, pe);

error:
	kfree(pe);

	return ret;
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Get first free double vlan ai number */
static int mv_pp2x_prs_double_vlan_ai_free_get(struct mv_pp2x_hw *hw)
{
	int i;

	for (i = 1; i < MVPP2_PRS_DBL_VLANS_MAX; i++) {
		if (!hw->prs_double_vlans[i])
			return i;
	}

	return -EINVAL;
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Search for existing double vlan entry */
static struct mv_pp2x_prs_entry *mv_pp2x_prs_double_vlan_find(
	struct mv_pp2x_hw *hw, unsigned short tpid1, unsigned short tpid2)
{
	struct mv_pp2x_prs_entry *pe;
	int tid;

	pe = kzalloc(sizeof(*pe), GFP_KERNEL);
	if (!pe)
		return NULL;
	mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);

	/* Go through the all entries with MVPP2_PRS_LU_VLAN */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		unsigned int ri_mask;
		bool match;

		if (!hw->prs_shadow[tid].valid ||
		    hw->prs_shadow[tid].lu != MVPP2_PRS_LU_VLAN)
			continue;

		pe->index = tid;
		mv_pp2x_prs_hw_read(hw, pe);

		match = mv_pp2x_prs_tcam_data_cmp(pe, 0, swab16(tpid1))
			&& mv_pp2x_prs_tcam_data_cmp(pe, 4, swab16(tpid2));

		if (!match)
			continue;

		ri_mask = mv_pp2x_prs_sram_ri_get(pe) & MVPP2_PRS_RI_VLAN_MASK;
		if (ri_mask == MVPP2_PRS_RI_VLAN_DOUBLE)
			return pe;
	}
	kfree(pe);

	return NULL;
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Add or update double vlan entry */
static int mv_pp2x_prs_double_vlan_add(struct mv_pp2x_hw *hw,
				       unsigned short tpid1,
				       unsigned short tpid2,
				       unsigned int port_map)
{
	struct mv_pp2x_prs_entry *pe;
	int tid_aux, tid, ai, ret = 0;

	pe = mv_pp2x_prs_double_vlan_find(hw, tpid1, tpid2);

	if (!pe) {
		/* Create new tcam entry */
		tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
						  MVPP2_PE_LAST_FREE_TID);
		if (tid < 0)
			return tid;

		pe = kzalloc(sizeof(*pe), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		/* Set ai value for new double vlan entry */
		ai = mv_pp2x_prs_double_vlan_ai_free_get(hw);
		if (ai < 0) {
			ret = ai;
			goto error;
		}

		/* Get first single/triple vlan tid */
		for (tid_aux = MVPP2_PE_FIRST_FREE_TID;
		     tid_aux <= MVPP2_PE_LAST_FREE_TID; tid_aux++) {
			unsigned int ri_bits;

			if (!hw->prs_shadow[tid_aux].valid ||
			    hw->prs_shadow[tid_aux].lu != MVPP2_PRS_LU_VLAN)
				continue;

			pe->index = tid_aux;
			mv_pp2x_prs_hw_read(hw, pe);
			ri_bits = mv_pp2x_prs_sram_ri_get(pe);
			ri_bits &= MVPP2_PRS_RI_VLAN_MASK;
			if (ri_bits == MVPP2_PRS_RI_VLAN_SINGLE ||
			    ri_bits == MVPP2_PRS_RI_VLAN_TRIPLE)
				break;
		}

		if (tid >= tid_aux) {
			ret = -ERANGE;
			goto error;
		}

		memset(pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_VLAN);
		pe->index = tid;

		hw->prs_double_vlans[ai] = true;

		mv_pp2x_prs_match_etype(pe, 0, tpid1);
		mv_pp2x_prs_match_etype(pe, 4, tpid2);

		mv_pp2x_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_VLAN);
		/* Shift 8 bytes - skip 2 vlan tags */
		mv_pp2x_prs_sram_shift_set(pe, 2 * MVPP2_VLAN_TAG_LEN,
					   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
		mv_pp2x_prs_sram_ri_update(pe, MVPP2_PRS_RI_VLAN_DOUBLE,
					   MVPP2_PRS_RI_VLAN_MASK);
		mv_pp2x_prs_sram_ai_update(pe, ai | MVPP2_PRS_DBL_VLAN_AI_BIT,
					   MVPP2_PRS_SRAM_AI_MASK);

		mv_pp2x_prs_shadow_set(hw, pe->index, MVPP2_PRS_LU_VLAN);
	}

	/* Update ports' mask */
	mv_pp2x_prs_tcam_port_map_set(pe, port_map);
	mv_pp2x_prs_hw_write(hw, pe);

error:
	kfree(pe);
	return ret;
}
#endif

/* IPv4 header parsing for fragmentation and L4 offset */
static int mv_pp2x_prs_ip4_proto(struct mv_pp2x_hw *hw, unsigned short proto,
				 unsigned int ri, unsigned int ri_mask)
{
	struct mv_pp2x_prs_entry pe;
	int tid;

	if ((proto != IPPROTO_TCP) && (proto != IPPROTO_UDP) &&
	    (proto != IPPROTO_IGMP))
		return -EINVAL;

	/* Not fragmented packet */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = tid;

	/* Set next lu to IPv4 */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_sram_shift_set(&pe, 12, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L4 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				    sizeof(struct iphdr) - 4,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);
	mv_pp2x_prs_sram_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				   MVPP2_PRS_IPV4_DIP_AI_BIT);
	mv_pp2x_prs_sram_ri_update(&pe, ri | MVPP2_PRS_RI_IP_FRAG_FALSE,
				   ri_mask | MVPP2_PRS_RI_IP_FRAG_MASK);

	mv_pp2x_prs_tcam_data_byte_set(&pe, 2, 0x00,
				       MVPP2_PRS_TCAM_PROTO_MASK_L);
	mv_pp2x_prs_tcam_data_byte_set(&pe, 3, 0x00,
				       MVPP2_PRS_TCAM_PROTO_MASK);
	mv_pp2x_prs_tcam_data_byte_set(&pe, 5, proto,
				       MVPP2_PRS_TCAM_PROTO_MASK);
	mv_pp2x_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Fragmented packet */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	pe.index = tid;
	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mv_pp2x_prs_sram_ri_update(&pe, ri, ri_mask);
	mv_pp2x_prs_sram_ri_update(&pe, ri | MVPP2_PRS_RI_IP_FRAG_TRUE,
				   ri_mask | MVPP2_PRS_RI_IP_FRAG_MASK);

	mv_pp2x_prs_tcam_data_byte_set(&pe, 2, 0x00, 0x0);
	mv_pp2x_prs_tcam_data_byte_set(&pe, 3, 0x00, 0x0);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}

/* IPv4 L3 multicast or broadcast */
static int mv_pp2x_prs_ip4_cast(struct mv_pp2x_hw *hw, unsigned short l3_cast)
{
	struct mv_pp2x_prs_entry pe;
	int mask, tid;

	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = tid;

	switch (l3_cast) {
	case MVPP2_PRS_L3_MULTI_CAST:
		mv_pp2x_prs_tcam_data_byte_set(&pe, 0, MVPP2_PRS_IPV4_MC,
					       MVPP2_PRS_IPV4_MC_MASK);
		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_MCAST,
					   MVPP2_PRS_RI_L3_ADDR_MASK);
		break;
	case  MVPP2_PRS_L3_BROAD_CAST:
		mask = MVPP2_PRS_IPV4_BC_MASK;
		mv_pp2x_prs_tcam_data_byte_set(&pe, 0, mask, mask);
		mv_pp2x_prs_tcam_data_byte_set(&pe, 1, mask, mask);
		mv_pp2x_prs_tcam_data_byte_set(&pe, 2, mask, mask);
		mv_pp2x_prs_tcam_data_byte_set(&pe, 3, mask, mask);
		mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_BCAST,
					   MVPP2_PRS_RI_L3_ADDR_MASK);
		break;
	default:
		return -EINVAL;
	}

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);

	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				   MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Set entries for protocols over IPv6  */
static int mv_pp2x_prs_ip6_proto(struct mv_pp2x_hw *hw, unsigned short proto,
				 unsigned int ri, unsigned int ri_mask)
{
	struct mv_pp2x_prs_entry pe;
	int tid;

	if ((proto != IPPROTO_TCP) && (proto != IPPROTO_UDP) &&
	    (proto != IPPROTO_ICMPV6) && (proto != IPPROTO_IPIP))
		return -EINVAL;

	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, ri, ri_mask);
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				    sizeof(struct ipv6hdr) - 6,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mv_pp2x_prs_tcam_data_byte_set(&pe, 0, proto,
			MVPP2_PRS_TCAM_PROTO_MASK);
	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				 MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Write HW */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}
#endif

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* IPv6 L3 multicast entry */
static int mv_pp2x_prs_ip6_cast(struct mv_pp2x_hw *hw, unsigned short l3_cast)
{
	struct mv_pp2x_prs_entry pe;
	int tid;

	if (l3_cast != MVPP2_PRS_L3_MULTI_CAST)
		return -EINVAL;

	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_MCAST,
				   MVPP2_PRS_RI_L3_ADDR_MASK);
	mv_pp2x_prs_sram_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				   MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Shift back to IPv6 NH */
	mv_pp2x_prs_sram_shift_set(&pe, -18, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	mv_pp2x_prs_tcam_data_byte_set(&pe, 0, MVPP2_PRS_IPV6_MC,
				       MVPP2_PRS_IPV6_MC_MASK);
	mv_pp2x_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}
#endif

/* Parser per-port initialization */
void mv_pp2x_prs_hw_port_init(struct mv_pp2x_hw *hw, int port, int lu_first,
			      int lu_max, int offset)
{
	u32 val;

	/* Set lookup ID */
	val = mv_pp2x_read(hw, MVPP2_PRS_INIT_LOOKUP_REG);
	val &= ~MVPP2_PRS_PORT_LU_MASK(port);
	val |=  MVPP2_PRS_PORT_LU_VAL(port, lu_first);
	mv_pp2x_write(hw, MVPP2_PRS_INIT_LOOKUP_REG, val);

	/* Set maximum number of loops for packet received from port */
	val = mv_pp2x_read(hw, MVPP2_PRS_MAX_LOOP_REG(port));
	val &= ~MVPP2_PRS_MAX_LOOP_MASK(port);
	val |= MVPP2_PRS_MAX_LOOP_VAL(port, lu_max);
	mv_pp2x_write(hw, MVPP2_PRS_MAX_LOOP_REG(port), val);

	/* Set initial offset for packet header extraction for the first
	 * searching loop
	 */
	val = mv_pp2x_read(hw, MVPP2_PRS_INIT_OFFS_REG(port));
	val &= ~MVPP2_PRS_INIT_OFF_MASK(port);
	val |= MVPP2_PRS_INIT_OFF_VAL(port, offset);
	mv_pp2x_write(hw, MVPP2_PRS_INIT_OFFS_REG(port), val);
}
EXPORT_SYMBOL(mv_pp2x_prs_hw_port_init);

/* Default flow entries initialization for all ports */
static void mv_pp2x_prs_def_flow_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
	int port;

	for (port = 0; port < MVPP2_MAX_PORTS; port++) {
		memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
		mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
		pe.index = MVPP2_PE_FIRST_DEFAULT_FLOW - port;

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(&pe, 0);

		/* Set flow ID*/
		mv_pp2x_prs_sram_ai_update(&pe, port, MVPP2_PRS_FLOW_ID_MASK);
		mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);

		/* Update shadow table and hw entry */
		mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_FLOWS);

		/*pr_crit("mv_pp2x_prs_def_flow_init: port(%d), index(%d)\n",
		 * port, pe.index);
		 */
		mv_pp2x_prs_hw_write(hw, &pe);
	}
}

/* Set default entry for Marvell Header field */
static void mv_pp2x_prs_mh_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));

	pe.index = MVPP2_PE_MH_DEFAULT;
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MH);
	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_MH_SIZE,
				 MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_MAC);

	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MH);
	mv_pp2x_prs_hw_write(hw, &pe);
}

/* Set default entires (place holder) for promiscuous, non-promiscuous and
 * multicast MAC addresses
 */
static void mv_pp2x_prs_mac_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));

	/* Non-promiscuous mode for all ports - DROP unknown packets */
	pe.index = MVPP2_PE_MAC_NON_PROMISCUOUS;
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_MAC);

	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_DROP_MASK,
				   MVPP2_PRS_RI_DROP_MASK);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);

	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MAC);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* place holders only - no ports */
	mv_pp2x_prs_mac_drop_all_set(hw, 0, false);
	mv_pp2x_prs_mac_promisc_set(hw, 0, false);

#if !defined(CONFIG_MV_PP2_PALLADIUM)
	mv_pp2x_prs_mac_multi_set(hw, MVPP2_PE_MAC_MC_ALL, 0, false);
	mv_pp2x_prs_mac_multi_set(hw, MVPP2_PE_MAC_MC_IP6, 0, false);
#endif
}

/* Set default entries for various types of dsa packets */
static void mv_pp2x_prs_dsa_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;

#if !defined(CONFIG_MV_PP2_PALLADIUM)
	/* None tagged EDSA entry - place holder */
	mv_pp2x_prs_dsa_tag_set(hw, 0, false, MVPP2_PRS_UNTAGGED,
				MVPP2_PRS_EDSA);

	/* Tagged EDSA entry - place holder */
	mv_pp2x_prs_dsa_tag_set(hw, 0, false, MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);

	/* None tagged DSA entry - place holder */
	mv_pp2x_prs_dsa_tag_set(hw, 0, false, MVPP2_PRS_UNTAGGED,
				MVPP2_PRS_DSA);

	/* Tagged DSA entry - place holder */
	mv_pp2x_prs_dsa_tag_set(hw, 0, false, MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);

	/* None tagged EDSA ethertype entry - place holder*/
	mv_pp2x_prs_dsa_tag_ethertype_set(hw, 0, false,
					  MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);

	/* Tagged EDSA ethertype entry - place holder*/
	mv_pp2x_prs_dsa_tag_ethertype_set(hw, 0, false,
					  MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);

	/* None tagged DSA ethertype entry */
	mv_pp2x_prs_dsa_tag_ethertype_set(hw, 0, true,
					  MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);

	/* Tagged DSA ethertype entry */
	mv_pp2x_prs_dsa_tag_ethertype_set(hw, 0, true,
					  MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
#endif

	/* Set default entry, in case DSA or EDSA tag not found */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_DSA);
	pe.index = MVPP2_PE_DSA_DEFAULT;
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_VLAN);

	/* Shift 0 bytes */
	mv_pp2x_prs_sram_shift_set(&pe, 0, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_MAC);

	/* Clear all sram ai bits for next iteration */
	mv_pp2x_prs_sram_ai_update(&pe, 0, MVPP2_PRS_SRAM_AI_MASK);

	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	mv_pp2x_prs_hw_write(hw, &pe);
}

/* Match basic ethertypes */
static int mv_pp2x_prs_etype_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
	int tid;

#if !defined(CONFIG_MV_PP2_PALLADIUM)
	/* Ethertype: PPPoE */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, ETH_P_PPP_SES);

	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_PPPOE_HDR_SIZE,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_PPPOE_MASK,
				   MVPP2_PRS_RI_PPPOE_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = false;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_PPPOE_MASK,
				  MVPP2_PRS_RI_PPPOE_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);

#endif
	/* Ethertype: ARP */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, ETH_P_ARP);

	/* Generate flow in the next iteration*/
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_ARP,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = true;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_L3_ARP,
				  MVPP2_PRS_RI_L3_PROTO_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);

#if !defined(CONFIG_MV_PP2_PALLADIUM)
	/* Ethertype: LBTD */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, MVPP2_IP_LBDT_TYPE);

	/* Generate flow in the next iteration*/
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				   MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				   MVPP2_PRS_RI_CPU_CODE_MASK |
				   MVPP2_PRS_RI_UDF3_MASK);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = true;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				  MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				  MVPP2_PRS_RI_CPU_CODE_MASK |
				  MVPP2_PRS_RI_UDF3_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);
#endif

	/* Ethertype: IPv4 without options */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, ETH_P_IP);
	mv_pp2x_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				       MVPP2_PRS_IPV4_HEAD |
				       MVPP2_PRS_IPV4_IHL,
				       MVPP2_PRS_IPV4_HEAD_MASK |
				       MVPP2_PRS_IPV4_IHL_MASK);

	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IP header */
	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = false;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_L3_IP4,
				  MVPP2_PRS_RI_L3_PROTO_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Ethertype: IPv4 with options */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	pe.index = tid;

	/* Clear tcam data before updating */
	pe.tcam.byte[TCAM_DATA_BYTE(MVPP2_ETH_TYPE_LEN)] = 0x0;
	pe.tcam.byte[TCAM_DATA_MASK(MVPP2_ETH_TYPE_LEN)] = 0x0;

	mv_pp2x_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				       MVPP2_PRS_IPV4_HEAD,
				       MVPP2_PRS_IPV4_HEAD_MASK);

	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4_OPT,
				   MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = false;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_L3_IP4_OPT,
				  MVPP2_PRS_RI_L3_PROTO_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);
#if !defined(CONFIG_MV_PP2_PALLADIUM)

	/* Ethertype: IPv6 without options */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, ETH_P_IPV6);

	/* Skip DIP of IPV6 header */
	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 8 +
				   MVPP2_MAX_L3_ADDR_SIZE,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP6,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = false;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_L3_IP6,
				  MVPP2_PRS_RI_L3_PROTO_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Default entry for MVPP2_PRS_LU_L2 - Unknown ethtype */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_L2);
	pe.index = MVPP2_PE_ETH_TYPE_UN;

	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Generate flow in the next iteration*/
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Set L3 offset even it's unknown L3 */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_L2);
	hw->prs_shadow[pe.index].udf = MVPP2_PRS_UDF_L2_DEF;
	hw->prs_shadow[pe.index].finish = true;
	mv_pp2x_prs_shadow_ri_set(hw, pe.index, MVPP2_PRS_RI_L3_UN,
				  MVPP2_PRS_RI_L3_PROTO_MASK);
	mv_pp2x_prs_hw_write(hw, &pe);
#endif
	return 0;
}

/* Configure vlan entries and detect up to 2 successive VLAN tags.
 * Possible options:
 * 0x8100, 0x88A8
 * 0x8100, 0x8100
 * 0x8100
 * 0x88A8
 */
static int mv_pp2x_prs_vlan_init(struct platform_device *pdev,
				 struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
#if !defined(CONFIG_MV_PP2_PALLADIUM)
	int err;
#endif

	hw->prs_double_vlans = devm_kcalloc(&pdev->dev, sizeof(bool),
					    MVPP2_PRS_DBL_VLANS_MAX,
					    GFP_KERNEL);
	if (!hw->prs_double_vlans)
		return -ENOMEM;
#if !defined(CONFIG_MV_PP2_PALLADIUM)
	/* Double VLAN: 0x8100, 0x88A8 */
	err = mv_pp2x_prs_double_vlan_add(hw, ETH_P_8021Q, ETH_P_8021AD,
					  MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Double VLAN: 0x8100, 0x8100 */
	err = mv_pp2x_prs_double_vlan_add(hw, ETH_P_8021Q, ETH_P_8021Q,
					  MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Single VLAN: 0x88a8 */
	err = mv_pp2x_prs_vlan_add(hw, ETH_P_8021AD, MVPP2_PRS_SINGLE_VLAN_AI,
				   MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Single VLAN: 0x8100 */
	err = mv_pp2x_prs_vlan_add(hw, ETH_P_8021Q, MVPP2_PRS_SINGLE_VLAN_AI,
				   MVPP2_PRS_PORT_MASK);
	if (err)
		return err;

	/* Set default double vlan entry */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_VLAN);
	pe.index = MVPP2_PE_VLAN_DBL;

	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
	/* Clear ai for next iterations */
	mv_pp2x_prs_sram_ai_update(&pe, 0, MVPP2_PRS_SRAM_AI_MASK);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_DOUBLE,
				   MVPP2_PRS_RI_VLAN_MASK);

	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_DBL_VLAN_AI_BIT,
				   MVPP2_PRS_DBL_VLAN_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_VLAN);
	mv_pp2x_prs_hw_write(hw, &pe);
#endif
	/* Set default vlan none entry */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_VLAN);
	pe.index = MVPP2_PE_VLAN_NONE;

	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_L2);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_VLAN_NONE,
				   MVPP2_PRS_RI_VLAN_MASK);

	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_VLAN);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Set entries for PPPoE ethertype */
static int mv_pp2x_prs_pppoe_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
	int tid;

	/* IPv4 over PPPoE with options */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, PPP_IP);

	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4_OPT,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IP header */
	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_PPPOE);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* IPv4 over PPPoE without options */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	pe.index = tid;

	mv_pp2x_prs_tcam_data_byte_set(&pe, MVPP2_ETH_TYPE_LEN,
				       MVPP2_PRS_IPV4_HEAD | MVPP2_PRS_IPV4_IHL,
				       MVPP2_PRS_IPV4_HEAD_MASK |
				       MVPP2_PRS_IPV4_IHL_MASK);

	/* Clear ri before updating */
	pe.sram.word[MVPP2_PRS_SRAM_RI_WORD] = 0x0;
	pe.sram.word[MVPP2_PRS_SRAM_RI_CTRL_WORD] = 0x0;
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP4,
				   MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_PPPOE);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* IPv6 over PPPoE */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mv_pp2x_prs_match_etype(&pe, 0, PPP_IPV6);

	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_IP6,
				   MVPP2_PRS_RI_L3_PROTO_MASK);
	/* Skip eth_type + 4 bytes of IPv6 header */
	mv_pp2x_prs_sram_shift_set(&pe, MVPP2_ETH_TYPE_LEN + 4,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L3 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_PPPOE);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Non-IP over PPPoE */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_PPPOE);
	pe.index = tid;

	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN,
				   MVPP2_PRS_RI_L3_PROTO_MASK);

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	/* Set L3 offset even if it's unknown L3 */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L3,
				    MVPP2_ETH_TYPE_LEN,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_PPPOE);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}
#endif

/* Initialize entries for IPv4 */
static int mv_pp2x_prs_ip4_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
	int err;

	/* Set entries for TCP, UDP and IGMP over IPv4 */
	err = mv_pp2x_prs_ip4_proto(hw, IPPROTO_TCP, MVPP2_PRS_RI_L4_TCP,
				    MVPP2_PRS_RI_L4_PROTO_MASK);

	PALAD(MVPP2_PRINT_LINE());
	if (err)
		return err;

	err = mv_pp2x_prs_ip4_proto(hw, IPPROTO_UDP, MVPP2_PRS_RI_L4_UDP,
				    MVPP2_PRS_RI_L4_PROTO_MASK);

	PALAD(MVPP2_PRINT_LINE());
	if (err)
		return err;

	err = mv_pp2x_prs_ip4_proto(hw, IPPROTO_IGMP,
				    MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				    MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				    MVPP2_PRS_RI_CPU_CODE_MASK |
				    MVPP2_PRS_RI_UDF3_MASK);

	PALAD(MVPP2_PRINT_LINE());
	if (err)
		return err;

	/* IPv4 Broadcast */
	err = mv_pp2x_prs_ip4_cast(hw, MVPP2_PRS_L3_BROAD_CAST);

	PALAD(MVPP2_PRINT_LINE());
	if (err)
		return err;

	/* IPv4 Multicast */
	err = mv_pp2x_prs_ip4_cast(hw, MVPP2_PRS_L3_MULTI_CAST);

	PALAD(MVPP2_PRINT_LINE());
	if (err)
		return err;

	/* Default IPv4 entry for unknown protocols */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = MVPP2_PE_IP4_PROTO_UN;

	/* Set next lu to IPv4 */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_sram_shift_set(&pe, 12, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);
	/* Set L4 offset */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				    sizeof(struct iphdr) - 4,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);
	mv_pp2x_prs_sram_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				   MVPP2_PRS_IPV4_DIP_AI_BIT);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				   MVPP2_PRS_RI_L4_PROTO_MASK);

	mv_pp2x_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Default IPv4 entry for unicast address */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP4);
	pe.index = MVPP2_PE_IP4_ADDR_UN;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UCAST,
				   MVPP2_PRS_RI_L3_ADDR_MASK);

	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV4_DIP_AI_BIT,
				   MVPP2_PRS_IPV4_DIP_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	PALAD(MVPP2_PRINT_LINE());

	return 0;
}

#if !defined(CONFIG_MV_PP2_PALLADIUM)
/* Initialize entries for IPv6 */
static int mv_pp2x_prs_ip6_init(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_prs_entry pe;
	int tid, err;

	/* Set entries for TCP, UDP and ICMP over IPv6 */
	err = mv_pp2x_prs_ip6_proto(hw, IPPROTO_TCP,
				    MVPP2_PRS_RI_L4_TCP,
				    MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mv_pp2x_prs_ip6_proto(hw, IPPROTO_UDP,
				    MVPP2_PRS_RI_L4_UDP,
				    MVPP2_PRS_RI_L4_PROTO_MASK);
	if (err)
		return err;

	err = mv_pp2x_prs_ip6_proto(hw, IPPROTO_ICMPV6,
				    MVPP2_PRS_RI_CPU_CODE_RX_SPEC |
				    MVPP2_PRS_RI_UDF3_RX_SPECIAL,
				    MVPP2_PRS_RI_CPU_CODE_MASK |
				    MVPP2_PRS_RI_UDF3_MASK);
	if (err)
		return err;

	/* IPv4 is the last header. This is similar case as 6-TCP or 17-UDP */
	/* Result Info: UDF7=1, DS lite */
	err = mv_pp2x_prs_ip6_proto(hw, IPPROTO_IPIP,
				    MVPP2_PRS_RI_UDF7_IP6_LITE,
				    MVPP2_PRS_RI_UDF7_MASK);
	if (err)
		return err;

	/* IPv6 multicast */
	err = mv_pp2x_prs_ip6_cast(hw, MVPP2_PRS_L3_MULTI_CAST);
	if (err)
		return err;

	/* Entry for checking hop limit */
	tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
					  MVPP2_PE_LAST_FREE_TID);
	if (tid < 0)
		return tid;

	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = tid;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UN |
				   MVPP2_PRS_RI_DROP_MASK,
				   MVPP2_PRS_RI_L3_PROTO_MASK |
				   MVPP2_PRS_RI_DROP_MASK);

	mv_pp2x_prs_tcam_data_byte_set(&pe, 1, 0x00, MVPP2_PRS_IPV6_HOP_MASK);
	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				   MVPP2_PRS_IPV6_NO_EXT_AI_BIT);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Default IPv6 entry for unknown protocols */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_PROTO_UN;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				   MVPP2_PRS_RI_L4_PROTO_MASK);
	/* Set L4 offset relatively to our current place */
	mv_pp2x_prs_sram_offset_set(&pe, MVPP2_PRS_SRAM_UDF_TYPE_L4,
				    sizeof(struct ipv6hdr) - 4,
				    MVPP2_PRS_SRAM_OP_SEL_UDF_ADD);

	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				   MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Default IPv6 entry for unknown ext protocols */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_EXT_PROTO_UN;

	/* Finished: go to flowid generation */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_FLOWS);
	mv_pp2x_prs_sram_bits_set(&pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L4_OTHER,
				   MVPP2_PRS_RI_L4_PROTO_MASK);

	mv_pp2x_prs_tcam_ai_update(&pe, MVPP2_PRS_IPV6_EXT_AI_BIT,
				   MVPP2_PRS_IPV6_EXT_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP4);
	mv_pp2x_prs_hw_write(hw, &pe);

	/* Default IPv6 entry for unicast address */
	memset(&pe, 0, sizeof(struct mv_pp2x_prs_entry));
	mv_pp2x_prs_tcam_lu_set(&pe, MVPP2_PRS_LU_IP6);
	pe.index = MVPP2_PE_IP6_ADDR_UN;

	/* Finished: go to IPv6 again */
	mv_pp2x_prs_sram_next_lu_set(&pe, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_sram_ri_update(&pe, MVPP2_PRS_RI_L3_UCAST,
				   MVPP2_PRS_RI_L3_ADDR_MASK);
	mv_pp2x_prs_sram_ai_update(&pe, MVPP2_PRS_IPV6_NO_EXT_AI_BIT,
				   MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Shift back to IPV6 NH */
	mv_pp2x_prs_sram_shift_set(&pe, -18, MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	mv_pp2x_prs_tcam_ai_update(&pe, 0, MVPP2_PRS_IPV6_NO_EXT_AI_BIT);
	/* Unmask all ports */
	mv_pp2x_prs_tcam_port_map_set(&pe, MVPP2_PRS_PORT_MASK);

	/* Update shadow table and hw entry */
	mv_pp2x_prs_shadow_set(hw, pe.index, MVPP2_PRS_LU_IP6);
	mv_pp2x_prs_hw_write(hw, &pe);

	return 0;
}
#endif

/* Compare MAC DA with tcam entry data */
static bool mv_pp2x_prs_mac_range_equals(struct mv_pp2x_prs_entry *pe,
					 const u8 *da, unsigned char *mask)
{
	unsigned char tcam_byte, tcam_mask;
	int index;

	for (index = 0; index < ETH_ALEN; index++) {
		mv_pp2x_prs_tcam_data_byte_get(pe, index, &tcam_byte,
			&tcam_mask);
		if (tcam_mask != mask[index])
			return false;

		if ((tcam_mask & tcam_byte) != (da[index] & mask[index]))
			return false;
	}

	return true;
}

/* Find tcam entry with matched pair <MAC DA, port> */
static struct mv_pp2x_prs_entry *
mv_pp2x_prs_mac_da_range_find(struct mv_pp2x_hw *hw, int pmap, const u8 *da,
			      unsigned char *mask, int udf_type)
{
	struct mv_pp2x_prs_entry *pe;
	int tid;

	pe = kzalloc(sizeof(*pe), GFP_KERNEL);
	if (!pe)
		return NULL;
	mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_MAC);

	/* Go through the all entires with MVPP2_PRS_LU_MAC */
	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		unsigned int entry_pmap;

		if (!hw->prs_shadow[tid].valid ||
		    (hw->prs_shadow[tid].lu != MVPP2_PRS_LU_MAC) ||
		    (hw->prs_shadow[tid].udf != udf_type))
			continue;

		pe->index = tid;
		mv_pp2x_prs_hw_read(hw, pe);
		entry_pmap = mv_pp2x_prs_tcam_port_map_get(pe);

		if (mv_pp2x_prs_mac_range_equals(pe, da, mask))
			return pe;
	}
	kfree(pe);

	return NULL;
}

/* Update parser's mac da entry */
int mv_pp2x_prs_mac_da_accept(struct mv_pp2x_hw *hw, int port,
				   const u8 *da, bool add)
{
	struct mv_pp2x_prs_entry *pe;
	unsigned int pmap, len, ri;
	unsigned char mask[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	int tid;

	/* Scan TCAM and see if entry with this <MAC DA, port> already exist */
	pe = mv_pp2x_prs_mac_da_range_find(hw, (1 << port), da, mask,
					   MVPP2_PRS_UDF_MAC_DEF);

	/* No such entry */
	if (!pe) {
		if (!add)
			return 0;

		/* Create new TCAM entry */
		/* Find first range mac entry*/
		for (tid = MVPP2_PE_FIRST_FREE_TID;
		     tid <= MVPP2_PE_LAST_FREE_TID; tid++)
			if (hw->prs_shadow[tid].valid &&
			    (hw->prs_shadow[tid].lu == MVPP2_PRS_LU_MAC) &&
			    (hw->prs_shadow[tid].udf ==
						       MVPP2_PRS_UDF_MAC_RANGE))
				break;

		/* Go through the all entries from first to last */
		tid = mv_pp2x_prs_tcam_first_free(hw, MVPP2_PE_FIRST_FREE_TID,
						  tid - 1);
		if (tid < 0)
			return tid;

		pe = kzalloc(sizeof(*pe), GFP_KERNEL);
		if (!pe)
			return -1;
		mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_MAC);
		pe->index = tid;

		/* Mask all ports */
		mv_pp2x_prs_tcam_port_map_set(pe, 0);
	}

	/* Update port mask */
	mv_pp2x_prs_tcam_port_set(pe, port, add);

	/* Invalidate the entry if no ports are left enabled */
	pmap = mv_pp2x_prs_tcam_port_map_get(pe);
	if (pmap == 0) {
		if (add) {
			kfree(pe);
			return -1;
		}
		mv_pp2x_prs_hw_inv(hw, pe->index);
		hw->prs_shadow[pe->index].valid = false;
		kfree(pe);
		return 0;
	}

	/* Continue - set next lookup */
	mv_pp2x_prs_sram_next_lu_set(pe, MVPP2_PRS_LU_DSA);

	/* Set match on DA */
	len = ETH_ALEN;
	while (len--)
		mv_pp2x_prs_tcam_data_byte_set(pe, len, da[len], 0xff);

	/* Set result info bits */
	if (is_broadcast_ether_addr(da))
		ri = MVPP2_PRS_RI_L2_BCAST;
	else if (is_multicast_ether_addr(da))
		ri = MVPP2_PRS_RI_L2_MCAST;
	else
		ri = MVPP2_PRS_RI_L2_UCAST | MVPP2_PRS_RI_MAC_ME_MASK;

	mv_pp2x_prs_sram_ri_update(pe, ri, MVPP2_PRS_RI_L2_CAST_MASK |
				   MVPP2_PRS_RI_MAC_ME_MASK);
	mv_pp2x_prs_shadow_ri_set(hw, pe->index, ri, MVPP2_PRS_RI_L2_CAST_MASK |
				  MVPP2_PRS_RI_MAC_ME_MASK);

	/* Shift to ethertype */
	mv_pp2x_prs_sram_shift_set(pe, 2 * ETH_ALEN,
				   MVPP2_PRS_SRAM_OP_SEL_SHIFT_ADD);

	/* Update shadow table and hw entry */
	hw->prs_shadow[pe->index].udf = MVPP2_PRS_UDF_MAC_DEF;
	mv_pp2x_prs_shadow_set(hw, pe->index, MVPP2_PRS_LU_MAC);
	mv_pp2x_prs_hw_write(hw, pe);

	kfree(pe);

	return 0;
}

int mv_pp2x_prs_update_mac_da(struct net_device *dev, const u8 *da)
{
	struct mv_pp2x_port *port = netdev_priv(dev);
	int err;

	/* Remove old parser entry */
	err = mv_pp2x_prs_mac_da_accept(&port->priv->hw,
		port->id, dev->dev_addr, false);
	if (err)
		return err;

	/* Add new parser entry */
	err = mv_pp2x_prs_mac_da_accept(&port->priv->hw, port->id, da, true);
	if (err)
		return err;

	/* Set addr in the device */
	ether_addr_copy(dev->dev_addr, da);

	return 0;
}

/* Delete all port's multicast simple (not range) entries */
void mv_pp2x_prs_mcast_del_all(struct mv_pp2x_hw *hw, int port)
{
	struct mv_pp2x_prs_entry pe;
	int index, tid;

	for (tid = MVPP2_PE_FIRST_FREE_TID;
	     tid <= MVPP2_PE_LAST_FREE_TID; tid++) {
		unsigned char da[ETH_ALEN], da_mask[ETH_ALEN];

		if (!hw->prs_shadow[tid].valid ||
		    (hw->prs_shadow[tid].lu != MVPP2_PRS_LU_MAC) ||
		    (hw->prs_shadow[tid].udf != MVPP2_PRS_UDF_MAC_DEF))
			continue;

		/* Only simple mac entries */
		pe.index = tid;
		mv_pp2x_prs_hw_read(hw, &pe);

		/* Read mac addr from entry */
		for (index = 0; index < ETH_ALEN; index++)
			mv_pp2x_prs_tcam_data_byte_get(&pe, index, &da[index],
						       &da_mask[index]);

		if (is_multicast_ether_addr(da) && !is_broadcast_ether_addr(da))
			/* Delete this entry */
			mv_pp2x_prs_mac_da_accept(hw, port, da, false);
	}
}

int mv_pp2x_prs_tag_mode_set(struct mv_pp2x_hw *hw, int port, int type)
{
	switch (type) {
	case MVPP2_TAG_TYPE_EDSA:
		/* Add port to EDSA entries */
		mv_pp2x_prs_dsa_tag_set(hw, port, true,
					MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, true,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		/* Remove port from DSA entries */
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		break;

	case MVPP2_TAG_TYPE_DSA:
		/* Add port to DSA entries */
		mv_pp2x_prs_dsa_tag_set(hw, port, true,
					MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, true,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		/* Remove port from EDSA entries */
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		break;

	case MVPP2_TAG_TYPE_MH:
	case MVPP2_TAG_TYPE_NONE:
		/* Remove port form EDSA and DSA entries */
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_TAGGED, MVPP2_PRS_DSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_DSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_TAGGED, MVPP2_PRS_EDSA);
		mv_pp2x_prs_dsa_tag_set(hw, port, false,
					MVPP2_PRS_UNTAGGED, MVPP2_PRS_EDSA);
		break;

	default:
		if ((type < 0) || (type > MVPP2_TAG_TYPE_EDSA))
			return -EINVAL;
	}

	return 0;
}

/* Set prs flow for the port */
int mv_pp2x_prs_def_flow(struct mv_pp2x_port *port)
{
	struct mv_pp2x_prs_entry *pe;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int tid;

	pe = mv_pp2x_prs_flow_find(hw, port->id, 0, 0);

	/* Such entry not exist */
	if (!pe) {
		/* Go through the all entires from last to first */
		tid = mv_pp2x_prs_tcam_first_free(hw,
						  MVPP2_PE_LAST_FREE_TID,
						  MVPP2_PE_FIRST_FREE_TID);
		if (tid < 0)
			return tid;

		pe = kzalloc(sizeof(*pe), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_FLOWS);
		pe->index = tid;

		/* Set flow ID*/
		mv_pp2x_prs_sram_ai_update(pe, port->id,
					   MVPP2_PRS_FLOW_ID_MASK);
		mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe->index, MVPP2_PRS_LU_FLOWS);

		/*pr_crit("mv_pp2x_prs_def_flow: index(%d) port->id\n",
		 * pe->index, port->id);
		 */
	}

	mv_pp2x_prs_tcam_port_map_set(pe, (1 << port->id));
	mv_pp2x_prs_hw_write(hw, pe);
	kfree(pe);

	return 0;
}

/* Set prs dedicated flow for the port */
int mv_pp2x_prs_flow_id_gen(struct mv_pp2x_port *port, u32 flowId,
			    u32 res, u32 resMask)
{
	struct mv_pp2x_prs_entry *pe;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	int tid;
	unsigned int pmap = 0;

	pe = mv_pp2x_prs_flow_find(hw, flowId, res, resMask);

	/* Such entry not exist */
	if (!pe) {
		pe = kzalloc(sizeof(*pe), GFP_KERNEL);
		if (!pe)
			return -ENOMEM;

		/* Go through the all entires from last to first */
		tid = mv_pp2x_prs_tcam_first_free(hw,
			MVPP2_PE_LAST_FREE_TID,
			MVPP2_PE_FIRST_FREE_TID);
		if (tid < 0) {
			kfree(pe);
			return tid;
		}

		mv_pp2x_prs_tcam_lu_set(pe, MVPP2_PRS_LU_FLOWS);
		pe->index = tid;

		mv_pp2x_prs_sram_ai_update(pe, flowId, MVPP2_PRS_FLOW_ID_MASK);
		mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);

		/* Update shadow table */
		mv_pp2x_prs_shadow_set(hw, pe->index, MVPP2_PRS_LU_FLOWS);

		/*update result data and mask*/
		mv_pp2x_prs_tcam_data_dword_set(pe, 0, res, resMask);
	} else {
		pmap = mv_pp2x_prs_tcam_port_map_get(pe);
	}

	mv_pp2x_prs_tcam_port_map_set(pe, (1 << port->id) | pmap);
	mv_pp2x_prs_hw_write(hw, pe);
	kfree(pe);

	return 0;
}

int mv_pp2x_prs_flow_set(struct mv_pp2x_port *port)
{
	int index, ret;

#if !defined(CONFIG_MV_PP2_PALLADIUM)
	for (index = 0; index < MVPP2_PRS_FL_TCAM_NUM; index++) {
		ret = mv_pp2x_prs_flow_id_gen(port,
			mv_pp2x_prs_flow_id_array[index].flow_id,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri_mask);
		if (ret)
			return ret;
	}
#else

	index = MVPP2_PRS_FL_IP4_UNTAG_NO_OPV4_OPTIONS;
	ret = mv_pp2x_prs_flow_id_gen(port,
			mv_pp2x_prs_flow_id_array[index].flow_id,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri_mask);
	if (ret)
		return ret;

	MVPP2_PRINT_LINE();

	index = MVPP2_PRS_FL_NON_IP_UNTAG_INDEX;
	ret = mv_pp2x_prs_flow_id_gen(port,
			mv_pp2x_prs_flow_id_array[index].flow_id,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri,
			mv_pp2x_prs_flow_id_array[index].prs_result.ri_mask);
	if (ret)
		return ret;

	MVPP2_PRINT_LINE();
#endif
	return 0;
}

static void mv_pp2x_prs_flow_id_attr_set(int flow_id, int ri, int ri_mask)
{
	int flow_attr = 0;

	flow_attr |= MVPP2_PRS_FL_ATTR_VLAN_BIT;
	if (ri_mask & MVPP2_PRS_RI_VLAN_MASK &&
	    (ri & MVPP2_PRS_RI_VLAN_MASK) == MVPP2_PRS_RI_VLAN_NONE)
		flow_attr &= ~MVPP2_PRS_FL_ATTR_VLAN_BIT;

	if ((ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_IP4 ||
	    (ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_IP4_OPT ||
	    (ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_IP4_OTHER)
		flow_attr |= MVPP2_PRS_FL_ATTR_IP4_BIT;

	if ((ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_IP6 ||
	    (ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_IP6_EXT)
		flow_attr |= MVPP2_PRS_FL_ATTR_IP6_BIT;

	if ((ri & MVPP2_PRS_RI_L3_PROTO_MASK) == MVPP2_PRS_RI_L3_ARP)
		flow_attr |= MVPP2_PRS_FL_ATTR_ARP_BIT;

	if (ri & MVPP2_PRS_RI_IP_FRAG_MASK)
		flow_attr |= MVPP2_PRS_FL_ATTR_FRAG_BIT;

	if ((ri & MVPP2_PRS_RI_L4_PROTO_MASK) == MVPP2_PRS_RI_L4_TCP)
		flow_attr |= MVPP2_PRS_FL_ATTR_TCP_BIT;

	if ((ri & MVPP2_PRS_RI_L4_PROTO_MASK) == MVPP2_PRS_RI_L4_UDP)
		flow_attr |= MVPP2_PRS_FL_ATTR_UDP_BIT;

	mv_pp2x_prs_flow_id_attr_tbl[flow_id] = flow_attr;
}

/* Init lookup id attribute array */
void mv_pp2x_prs_flow_id_attr_init(void)
{
	int index;
	u32 ri, ri_mask, flow_id;

	for (index = 0; index < MVPP2_PRS_FL_TCAM_NUM; index++) {
		ri = mv_pp2x_prs_flow_id_array[index].prs_result.ri;
		ri_mask = mv_pp2x_prs_flow_id_array[index].prs_result.ri_mask;
		flow_id = mv_pp2x_prs_flow_id_array[index].flow_id;

		mv_pp2x_prs_flow_id_attr_set(flow_id, ri, ri_mask);
	}
}

int mv_pp2x_prs_flow_id_attr_get(int flow_id)
{
	return mv_pp2x_prs_flow_id_attr_tbl[flow_id];
}

/* Classifier configuration routines */

/* Update classification flow table registers */
void mv_pp2x_cls_flow_write(struct mv_pp2x_hw *hw,
			    struct mv_pp2x_cls_flow_entry *fe)
{
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_INDEX_REG, fe->index);
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_TBL0_REG,  fe->data[0]);
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_TBL1_REG,  fe->data[1]);
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_TBL2_REG,  fe->data[2]);
}
EXPORT_SYMBOL(mv_pp2x_cls_flow_write);

static void mv_pp2x_cls_flow_read(struct mv_pp2x_hw *hw, int index,
				  struct mv_pp2x_cls_flow_entry *fe)
{
	fe->index = index;
	/*write index*/
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_INDEX_REG, index);

	fe->data[0] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL0_REG);
	fe->data[1] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL1_REG);
	fe->data[2] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL2_REG);
}

/* Update classification lookup table register */
static void mv_pp2x_cls_lookup_write(struct mv_pp2x_hw *hw,
				     struct mv_pp2x_cls_lookup_entry *le)
{
	u32 val;

	val = (le->way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | le->lkpid;
	mv_pp2x_write(hw, MVPP2_CLS_LKP_INDEX_REG, val);
	mv_pp2x_write(hw, MVPP2_CLS_LKP_TBL_REG, le->data);
}

void mv_pp2x_cls_lookup_read(struct mv_pp2x_hw *hw, int lkpid, int way,
			     struct mv_pp2x_cls_lookup_entry *le)
{
	unsigned int val = 0;

	/* write index reg */
	val = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | lkpid;
	mv_pp2x_write(hw, MVPP2_CLS_LKP_INDEX_REG, val);
	le->way = way;
	le->lkpid = lkpid;
	le->data = mv_pp2x_read(hw, MVPP2_CLS_LKP_TBL_REG);
}

/* Operations on flow entry */
int mv_pp2x_cls_sw_flow_hek_num_set(struct mv_pp2x_cls_flow_entry *fe,
				    int num_of_fields)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(num_of_fields, 0,
	    MVPP2_CLS_FLOWS_TBL_FIELDS_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[1] &= ~MVPP2_FLOW_FIELDS_NUM_MASK;
	fe->data[1] |= (num_of_fields << MVPP2_FLOW_FIELDS_NUM);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_hek_num_set);

int mv_pp2x_cls_sw_flow_hek_set(struct mv_pp2x_cls_flow_entry *fe,
				int field_index, int field_id)
{
	int num_of_fields;

	/* get current num_of_fields */
	num_of_fields = ((fe->data[1] &
		MVPP2_FLOW_FIELDS_NUM_MASK) >> MVPP2_FLOW_FIELDS_NUM);

	if (num_of_fields < (field_index+1)) {
		pr_debug("%s:num of heks=%d ,idx(%d) out of range\n",
			 __func__, num_of_fields, field_index);
		return -1;
	}

	fe->data[2] &= ~MVPP2_FLOW_FIELD_MASK(field_index);
	fe->data[2] |= (field_id <<  MVPP2_FLOW_FIELD_ID(field_index));

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_hek_set);

static void mv_pp2x_cls_sw_flow_eng_set(struct mv_pp2x_cls_flow_entry *fe,
					int engine, int is_last)
{
	fe->data[0] &= ~MVPP2_FLOW_LAST_MASK;
	fe->data[0] &= ~MVPP2_FLOW_ENGINE_MASK;

	fe->data[0] |= is_last;
	fe->data[0] |= (engine << MVPP2_FLOW_ENGINE);
	fe->data[0] |= MVPP2_FLOW_PORT_ID_SEL_MASK;
}

/* To init flow table waccording to different flow */
static inline void mv_pp2x_cls_flow_cos(struct mv_pp2x_hw *hw,
					struct mv_pp2x_cls_flow_entry *fe,
					int lkpid, int cos_type)
{
	int hek_num, field_id, lkp_type, is_last;
	int entry_idx = hw->cls_shadow->flow_free_start;

	switch (cos_type) {
	case MVPP2_COS_TYPE_VLAN:
		lkp_type = MVPP2_CLS_LKP_VLAN_PRI;
		break;
	case MVPP2_COS_TYPE_DSCP:
		lkp_type = MVPP2_CLS_LKP_DSCP_PRI;
		break;
	default:
		lkp_type = MVPP2_CLS_LKP_DEFAULT;
		break;
	}
	hek_num = 0;
	if ((lkpid == MVPP2_PRS_FL_NON_IP_UNTAG &&
		cos_type == MVPP2_COS_TYPE_DEF) ||
	    (lkpid == MVPP2_PRS_FL_NON_IP_TAG &&
		cos_type == MVPP2_COS_TYPE_VLAN))
		is_last = 1;
	else
		is_last = 0;

	/* Set SW */
	memset(fe, 0, sizeof(struct mv_pp2x_cls_flow_entry));
	mv_pp2x_cls_sw_flow_hek_num_set(fe, hek_num);
	if (hek_num)
		mv_pp2x_cls_sw_flow_hek_set(fe, 0, field_id);
	mv_pp2x_cls_sw_flow_eng_set(fe, MVPP2_CLS_ENGINE_C2, is_last);
	mv_pp2x_cls_sw_flow_extra_set(fe, lkp_type, MVPP2_CLS_FL_COS_PRI);
	fe->index = entry_idx;

	/* Write HW */
	mv_pp2x_cls_flow_write(hw, fe);

	/* Update Shadow */
	if (cos_type == MVPP2_COS_TYPE_DEF)
		hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_dflt = entry_idx;
	else if (cos_type == MVPP2_COS_TYPE_VLAN)
		hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_vlan = entry_idx;
	else
		hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_dscp = entry_idx;

	/* Update first available flow entry */
	hw->cls_shadow->flow_free_start++;
}

/* Init flow entry for RSS hash in PP22 */
static inline void mv_pp2x_cls_flow_rss_hash(struct mv_pp2x_hw *hw,
					     struct mv_pp2x_cls_flow_entry *fe,
					     int lkpid, int rss_mode)
{
	int field_id[4] = {0};
	int entry_idx = hw->cls_shadow->flow_free_start;
	int lkpid_attr = mv_pp2x_prs_flow_id_attr_get(lkpid);

	/* IP4 packet */
	if (lkpid_attr & MVPP2_PRS_FL_ATTR_IP4_BIT) {
		field_id[0] = MVPP2_CLS_FIELD_IP4SA;
		field_id[1] = MVPP2_CLS_FIELD_IP4DA;
	} else if (lkpid_attr & MVPP2_PRS_FL_ATTR_IP6_BIT) {
		field_id[0] = MVPP2_CLS_FIELD_IP6SA;
		field_id[1] = MVPP2_CLS_FIELD_IP6DA;
	}
	/* L4 port */
	field_id[2] = MVPP2_CLS_FIELD_L4SIP;
	field_id[3] = MVPP2_CLS_FIELD_L4DIP;

	/* Set SW */
	memset(fe, 0, sizeof(struct mv_pp2x_cls_flow_entry));
	if (rss_mode == MVPP2_RSS_HASH_2T) {
		mv_pp2x_cls_sw_flow_hek_num_set(fe, 2);
		mv_pp2x_cls_sw_flow_eng_set(fe, MVPP2_CLS_ENGINE_C3HA, 1);
		mv_pp2x_cls_sw_flow_hek_set(fe, 0, field_id[0]);
		mv_pp2x_cls_sw_flow_hek_set(fe, 1, field_id[1]);
	} else {
		mv_pp2x_cls_sw_flow_hek_num_set(fe, 4);
		mv_pp2x_cls_sw_flow_hek_set(fe, 0, field_id[0]);
		mv_pp2x_cls_sw_flow_hek_set(fe, 1, field_id[1]);
		mv_pp2x_cls_sw_flow_hek_set(fe, 2, field_id[2]);
		mv_pp2x_cls_sw_flow_hek_set(fe, 3, field_id[3]);
		mv_pp2x_cls_sw_flow_eng_set(fe, MVPP2_CLS_ENGINE_C3HB, 1);
	}
	mv_pp2x_cls_sw_flow_extra_set(fe,
		MVPP2_CLS_LKP_HASH, MVPP2_CLS_FL_RSS_PRI);
	fe->index = entry_idx;

	/* Update last for UDP NF flow */
	if ((lkpid_attr & MVPP2_PRS_FL_ATTR_UDP_BIT) &&
	    !(lkpid_attr & MVPP2_PRS_FL_ATTR_FRAG_BIT)) {
		if (!hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_rss1) {
			if (rss_mode == MVPP2_RSS_HASH_2T)
				mv_pp2x_cls_sw_flow_eng_set(fe,
						MVPP2_CLS_ENGINE_C3HA, 0);
			else
				mv_pp2x_cls_sw_flow_eng_set(fe,
						MVPP2_CLS_ENGINE_C3HB, 0);
		}
	}

	/* Write HW */
	mv_pp2x_cls_flow_write(hw, fe);

	/* Update Shadow */
	if (hw->cls_shadow->flow_info[lkpid -
		MVPP2_PRS_FL_START].flow_entry_rss1 == 0)
		hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_rss1 = entry_idx;
	else
		hw->cls_shadow->flow_info[lkpid -
			MVPP2_PRS_FL_START].flow_entry_rss2 = entry_idx;

	/* Update first available flow entry */
	hw->cls_shadow->flow_free_start++;
}

/* Init cls flow table according to different flow id */
void mv_pp2x_cls_flow_tbl_config(struct mv_pp2x_hw *hw)
{
	int lkpid, rss_mode, lkpid_attr;
	struct mv_pp2x_cls_flow_entry fe;

	for (lkpid = MVPP2_PRS_FL_START; lkpid < MVPP2_PRS_FL_LAST; lkpid++) {
		PALAD(MVPP2_PRINT_LINE());
		/* Get lookup id attribute */
		lkpid_attr = mv_pp2x_prs_flow_id_attr_get(lkpid);
		/* Default rss hash is based on 5T */
		rss_mode = MVPP2_RSS_HASH_5T;
		/* For frag packets or non-TCP&UDP, rss must be based on 2T */
		if ((lkpid_attr & MVPP2_PRS_FL_ATTR_FRAG_BIT) ||
		    !(lkpid_attr & (MVPP2_PRS_FL_ATTR_TCP_BIT |
		    MVPP2_PRS_FL_ATTR_UDP_BIT)))
			rss_mode = MVPP2_RSS_HASH_2T;

		/* For untagged IP packets, only need default
		 * rule and dscp rule
		 */
		if ((lkpid_attr & (MVPP2_PRS_FL_ATTR_IP4_BIT |
		     MVPP2_PRS_FL_ATTR_IP6_BIT)) &&
		    (!(lkpid_attr & MVPP2_PRS_FL_ATTR_VLAN_BIT))) {
			/* Default rule */
			mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
					     MVPP2_COS_TYPE_DEF);
			/* DSCP rule */
			mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
					     MVPP2_COS_TYPE_DSCP);
			/* RSS hash rule */
			if ((!(lkpid_attr & MVPP2_PRS_FL_ATTR_FRAG_BIT)) &&
			    (lkpid_attr & MVPP2_PRS_FL_ATTR_UDP_BIT)) {
				/* RSS hash rules for UDP rss mode update */
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  MVPP2_RSS_HASH_2T);
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  MVPP2_RSS_HASH_5T);
			} else {
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  rss_mode);
			}
		}

		/* For tagged IP packets, only need vlan rule and dscp rule */
		if ((lkpid_attr & (MVPP2_PRS_FL_ATTR_IP4_BIT |
		    MVPP2_PRS_FL_ATTR_IP6_BIT)) &&
		    (lkpid_attr & MVPP2_PRS_FL_ATTR_VLAN_BIT)) {
			/* VLAN rule */
			mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
					     MVPP2_COS_TYPE_VLAN);
			/* DSCP rule */
			mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
					     MVPP2_COS_TYPE_DSCP);
			/* RSS hash rule */
			if ((!(lkpid_attr & MVPP2_PRS_FL_ATTR_FRAG_BIT)) &&
			    (lkpid_attr & MVPP2_PRS_FL_ATTR_UDP_BIT)) {
				/* RSS hash rules for UDP rss mode update */
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  MVPP2_RSS_HASH_2T);
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  MVPP2_RSS_HASH_5T);
			} else {
				mv_pp2x_cls_flow_rss_hash(hw, &fe, lkpid,
							  rss_mode);
			}
		}

		/* For non-IP packets, only need default rule if untagged,
		 * vlan rule also needed if tagged
		 */
		if (!(lkpid_attr & (MVPP2_PRS_FL_ATTR_IP4_BIT |
		     MVPP2_PRS_FL_ATTR_IP6_BIT))) {
			/* Default rule */
			mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
					     MVPP2_COS_TYPE_DEF);
			/* VLAN rule if tagged */
			if (lkpid_attr & MVPP2_PRS_FL_ATTR_VLAN_BIT)
				mv_pp2x_cls_flow_cos(hw, &fe, lkpid,
						     MVPP2_COS_TYPE_VLAN);
		}
	}
}

/* Update the flow index for flow of lkpid */
void mv_pp2x_cls_lkp_flow_set(struct mv_pp2x_hw *hw, int lkpid, int way,
			      int flow_idx)
{
	struct mv_pp2x_cls_lookup_entry le;

	mv_pp2x_cls_lookup_read(hw, lkpid, way, &le);
	mv_pp2x_cls_sw_lkp_flow_set(&le, flow_idx);
	mv_pp2x_cls_lookup_write(hw, &le);
}

int mv_pp2x_cls_lkp_port_way_set(struct mv_pp2x_hw *hw, int port, int way)
{
	unsigned int val;

	if (mv_pp2x_range_validate(port, 0, MVPP2_MAX_PORTS - 1) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(way, 0, ONE_BIT_MAX) == MV_ERROR)
		return MV_ERROR;

	val = mv_pp2x_read(hw, MVPP2_CLS_PORT_WAY_REG);
	if (way == 1)
		val |= MVPP2_CLS_PORT_WAY_MASK(port);
	else
		val &= ~MVPP2_CLS_PORT_WAY_MASK(port);
	mv_pp2x_write(hw, MVPP2_CLS_PORT_WAY_REG, val);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_lkp_port_way_set);

int mv_pp2x_cls_hw_udf_set(struct mv_pp2x_hw *hw, int udf_no, int offs_id,
			   int offs_bits, int size_bits)
{
	unsigned int regVal;

	if (mv_pp2x_range_validate(offs_id, 0,
	    MVPP2_CLS_UDF_OFFSET_ID_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(offs_bits, 0,
	    MVPP2_CLS_UDF_REL_OFFSET_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(size_bits, 0,
	    MVPP2_CLS_UDF_SIZE_MASK) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(udf_no, 0,
	    MVPP2_CLS_UDF_REGS_NUM - 1) == MV_ERROR)
		return MV_ERROR;

	regVal = mv_pp2x_read(hw, MVPP2_CLS_UDF_REG(udf_no));
	regVal &= ~MVPP2_CLS_UDF_OFFSET_ID_MASK;
	regVal &= ~MVPP2_CLS_UDF_REL_OFFSET_MASK;
	regVal &= ~MVPP2_CLS_UDF_SIZE_MASK;

	regVal |= (offs_id << MVPP2_CLS_UDF_OFFSET_ID_OFFS);
	regVal |= (offs_bits << MVPP2_CLS_UDF_REL_OFFSET_OFFS);
	regVal |= (size_bits << MVPP2_CLS_UDF_SIZE_OFFS);

	mv_pp2x_write(hw, MVPP2_CLS_UDF_REG(udf_no), regVal);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_udf_set);

/* Init lookup decoding table with lookup id */
void mv_pp2x_cls_lookup_tbl_config(struct mv_pp2x_hw *hw)
{
	int index, flow_idx;
	int data[3];
	struct mv_pp2x_cls_lookup_entry le;
	struct mv_pp2x_cls_flow_info *flow_info;

	memset(&le, 0, sizeof(struct mv_pp2x_cls_lookup_entry));
	/* Enable classifier engine */
	mv_pp2x_cls_sw_lkp_en_set(&le, 1);

	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++) {
		flow_info = &(hw->cls_shadow->flow_info[index]);
		PALAD(MVPP2_PRINT_LINE());
		data[0] = MVPP2_FLOW_TBL_SIZE;
		data[1] = MVPP2_FLOW_TBL_SIZE;
		data[2] = MVPP2_FLOW_TBL_SIZE;
		le.lkpid = hw->cls_shadow->flow_info[index].lkpid;
		/* Find the min non-zero one in flow_entry_dflt,
		 * flow_entry_vlan, and flow_entry_dscp
		 */
		if (flow_info->flow_entry_dflt)
			data[0] = flow_info->flow_entry_dflt;
		if (flow_info->flow_entry_vlan)
			data[1] = flow_info->flow_entry_vlan;
		if (flow_info->flow_entry_dscp)
			data[2] = flow_info->flow_entry_dscp;
		flow_idx = min(data[0], min(data[1], data[2]));

		/* Set flow pointer index */
		mv_pp2x_cls_sw_lkp_flow_set(&le, flow_idx);

		/* Set initial rx queue */
		mv_pp2x_cls_sw_lkp_rxq_set(&le, 0x0);

		le.way = 0;

		/* Update lookup ID table entry */
		mv_pp2x_cls_lookup_write(hw, &le);

		le.way = 1;

		/* Update lookup ID table entry */
		mv_pp2x_cls_lookup_write(hw, &le);
	}
}

/* Classifier default initialization */
int mv_pp2x_cls_init(struct platform_device *pdev, struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_cls_lookup_entry le;
	struct mv_pp2x_cls_flow_entry fe;
	int index;

	/* Enable classifier */
	mv_pp2x_write(hw, MVPP2_CLS_MODE_REG, MVPP2_CLS_MODE_ACTIVE_MASK);
	PALAD(MVPP2_PRINT_LINE());

	/* Clear classifier flow table */
	memset(&fe.data, 0, MVPP2_CLS_FLOWS_TBL_DATA_WORDS);
	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE; index++) {
		PALAD(MVPP2_PRINT_LINE());
		fe.index = index;
		mv_pp2x_cls_flow_write(hw, &fe);
	}
	PALAD(MVPP2_PRINT_LINE());

	/* Clear classifier lookup table */
	le.data = 0;
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE; index++) {
		PALAD(MVPP2_PRINT_LINE());
		le.lkpid = index;
		le.way = 0;
		mv_pp2x_cls_lookup_write(hw, &le);

		le.way = 1;
		mv_pp2x_cls_lookup_write(hw, &le);
	}
	PALAD(MVPP2_PRINT_LINE());

	hw->cls_shadow = devm_kcalloc(&pdev->dev, 1,
				      sizeof(struct mv_pp2x_cls_shadow),
				      GFP_KERNEL);
	if (!hw->cls_shadow)
		return -ENOMEM;
	PALAD(MVPP2_PRINT_LINE());

	hw->cls_shadow->flow_info = devm_kcalloc(&pdev->dev,
				(MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START),
				sizeof(struct mv_pp2x_cls_flow_info),
				GFP_KERNEL);
	if (!hw->cls_shadow->flow_info)
		return -ENOMEM;
	MVPP2_PRINT_LINE();

	/* Start from entry 1 to allocate flow table */
	hw->cls_shadow->flow_free_start = 1;
	for (index = 0; index < (MVPP2_PRS_FL_LAST - MVPP2_PRS_FL_START);
		index++)
		hw->cls_shadow->flow_info[index].lkpid = index +
			MVPP2_PRS_FL_START;
	PALAD(MVPP2_PRINT_LINE());

	/* Init flow table */
	mv_pp2x_cls_flow_tbl_config(hw);

	PALAD(MVPP2_PRINT_LINE());

	/* Init lookup table */
	mv_pp2x_cls_lookup_tbl_config(hw);

	PALAD(MVPP2_PRINT_LINE());

	return 0;
}

void mv_pp2x_cls_port_config(struct mv_pp2x_port *port)
{
	struct mv_pp2x_cls_lookup_entry le;
	struct mv_pp2x_hw *hw = &port->priv->hw;
	u32 val;

	/* Set way for the port */
	val = mv_pp2x_read(hw, MVPP2_CLS_PORT_WAY_REG);
	val &= ~MVPP2_CLS_PORT_WAY_MASK(port->id);
	mv_pp2x_write(hw, MVPP2_CLS_PORT_WAY_REG, val);

	/* Pick the entry to be accessed in lookup ID decoding table
	 * according to the way and lkpid.
	 */
	le.lkpid = port->id;
	le.way = 0;
	le.data = 0;

	/* Set initial CPU queue for receiving packets */
	le.data &= ~MVPP2_CLS_LKP_TBL_RXQ_MASK;
	le.data |= port->first_rxq;

	/* Disable classification engines */
	le.data &= ~MVPP2_CLS_LKP_TBL_LOOKUP_EN_MASK;

	/* Update lookup ID table entry */
	mv_pp2x_cls_lookup_write(hw, &le);
}

/* Set CPU queue number for oversize packets */
void mv_pp2x_cls_oversize_rxq_set(struct mv_pp2x_port *port)
{
	struct mv_pp2x_hw *hw = &port->priv->hw;

	mv_pp2x_write(hw, MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(port->id),
		      port->first_rxq & MVPP2_CLS_OVERSIZE_RXQ_LOW_MASK);

}

void mv_pp21_get_mac_address(struct mv_pp2x_port *port, unsigned char *addr)
{
	u32 mac_addr_l, mac_addr_m, mac_addr_h;

	mac_addr_l = readl(port->base + MVPP2_GMAC_CTRL_1_REG);
	mac_addr_m = readl(port->priv->hw.lms_base + MVPP2_SRC_ADDR_MIDDLE);
	mac_addr_h = readl(port->priv->hw.lms_base + MVPP2_SRC_ADDR_HIGH);
	addr[0] = (mac_addr_h >> 24) & 0xFF;
	addr[1] = (mac_addr_h >> 16) & 0xFF;
	addr[2] = (mac_addr_h >> 8) & 0xFF;
	addr[3] = mac_addr_h & 0xFF;
	addr[4] = mac_addr_m & 0xFF;
	addr[5] = (mac_addr_l >> MVPP2_GMAC_SA_LOW_OFFS) & 0xFF;
}

void mv_pp2x_cause_error(struct net_device *dev, int cause)
{
	if (cause & MVPP2_CAUSE_FCS_ERR_MASK)
		netdev_err(dev, "FCS error\n");
	if (cause & MVPP2_CAUSE_RX_FIFO_OVERRUN_MASK)
		netdev_err(dev, "rx fifo overrun error\n");
	if (cause & MVPP2_CAUSE_TX_FIFO_UNDERRUN_MASK)
		netdev_err(dev, "tx fifo underrun error\n");
}

/* Display more error info */
void mv_pp2x_rx_error(struct mv_pp2x_port *port,
		      struct mv_pp2x_rx_desc *rx_desc)
{
	u32 status = rx_desc->status;

	switch (status & MVPP2_RXD_ERR_CODE_MASK) {
	case MVPP2_RXD_ERR_CRC:
		netdev_err(port->dev,
			   "bad rx status %08x (crc error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	case MVPP2_RXD_ERR_OVERRUN:
		netdev_err(port->dev,
			   "bad rx status %08x (overrun error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	case MVPP2_RXD_ERR_RESOURCE:
		netdev_err(port->dev,
			   "bad rx status %08x (resource error), size=%d\n",
			   status, rx_desc->data_size);
		break;
	}
}

/* Handle RX checksum offload */
void mv_pp2x_rx_csum(struct mv_pp2x_port *port, u32 status,
		     struct sk_buff *skb)
{
	if (((status & MVPP2_RXD_L3_IP4) &&
	     !(status & MVPP2_RXD_IP4_HEADER_ERR)) ||
	    (status & MVPP2_RXD_L3_IP6))
		if (((status & MVPP2_RXD_L4_UDP) ||
		     (status & MVPP2_RXD_L4_TCP)) &&
		     (status & MVPP2_RXD_L4_CSUM_OK)) {
			skb->ip_summed = CHECKSUM_UNNECESSARY;
			return;
		}

	skb->ip_summed = CHECKSUM_NONE;
}

/* Set the number of packets that will be received before Rx interrupt
 * will be generated by HW.
 */
void mv_pp2x_rx_pkts_coal_set(struct mv_pp2x_port *port,
			      struct mv_pp2x_rx_queue *rxq, u32 pkts)
{
	u32 val;

	val = (pkts & MVPP2_OCCUPIED_THRESH_MASK);
	mv_pp2x_write(&port->priv->hw, MVPP2_RXQ_NUM_REG, rxq->id);
	mv_pp2x_write(&port->priv->hw, MVPP2_RXQ_THRESH_REG, val);

	rxq->pkts_coal = pkts;
}

/* Set the time delay in usec before Rx interrupt */
void mv_pp2x_rx_time_coal_set(struct mv_pp2x_port *port,
			      struct mv_pp2x_rx_queue *rxq, u32 usec)
{
	u32 val;

	val = (port->priv->hw.tclk / USEC_PER_SEC) * usec;
	mv_pp2x_write(&port->priv->hw,
		      MVPP2_ISR_RX_THRESHOLD_REG(rxq->id), val);
}

/* Set threshold for TX_DONE pkts coalescing */
void mv_pp2x_tx_done_pkts_coal_set(void *arg)
{
	struct mv_pp2x_port *port = arg;
	int queue;
	u32 val;

	for (queue = 0; queue < port->num_tx_queues; queue++) {
		struct mv_pp2x_tx_queue *txq = port->txqs[queue];

		val = (txq->pkts_coal << MVPP2_TRANSMITTED_THRESH_OFFSET) &
		       MVPP2_TRANSMITTED_THRESH_MASK;
		mv_pp2x_write(&port->priv->hw, MVPP2_TXQ_NUM_REG, txq->id);
		mv_pp2x_write(&port->priv->hw, MVPP2_TXQ_THRESH_REG, val);
	}
}

/* Set the time delay in usec before Rx interrupt */
void mv_pp2x_tx_done_time_coal_set(struct mv_pp2x_port *port, u32 usec)
{
	u32 val;

	val = (port->priv->hw.tclk / USEC_PER_SEC) * usec;
	mv_pp2x_write(&port->priv->hw,
		      MVPP22_ISR_TX_THRESHOLD_REG(port->id), val);
}

/* Change maximum receive size of the port */
void mv_pp21_gmac_max_rx_size_set(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM
	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_0_REG);
	val &= ~MVPP2_GMAC_MAX_RX_SIZE_MASK;
	val |= (((port->pkt_size - MVPP2_MH_SIZE) / 2) <<
		MVPP2_GMAC_MAX_RX_SIZE_OFFS);
	writel(val, port->base + MVPP2_GMAC_CTRL_0_REG);
#endif
}

/* Set max sizes for Tx queues */
void mv_pp2x_txp_max_tx_size_set(struct mv_pp2x_port *port)
{
	u32	val, size, mtu;
	int	txq, tx_port_num;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	mtu = port->pkt_size * 8;
	if (mtu > MVPP2_TXP_MTU_MAX)
		mtu = MVPP2_TXP_MTU_MAX;

	/* WA for wrong Token bucket update: Set MTU value = 3*real MTU value */
	mtu = 3 * mtu;

	/* Indirect access to registers */
	tx_port_num = mv_pp2x_egress_port(port);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);

	/* Set MTU */
	val = mv_pp2x_read(hw, MVPP2_TXP_SCHED_MTU_REG);
	val &= ~MVPP2_TXP_MTU_MAX;
	val |= mtu;
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_MTU_REG, val);

	/* TXP token size and all TXQs token size must be larger that MTU */
	val = mv_pp2x_read(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG);
	size = val & MVPP2_TXP_TOKEN_SIZE_MAX;
	if (size < mtu) {
		size = mtu;
		val &= ~MVPP2_TXP_TOKEN_SIZE_MAX;
		val |= size;
		mv_pp2x_write(hw, MVPP2_TXP_SCHED_TOKEN_SIZE_REG, val);
	}

	for (txq = 0; txq < port->num_tx_queues; txq++) {
		val = mv_pp2x_read(hw,
				   MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq));
		size = val & MVPP2_TXQ_TOKEN_SIZE_MAX;

		if (size < mtu) {
			size = mtu;
			val &= ~MVPP2_TXQ_TOKEN_SIZE_MAX;
			val |= size;
			mv_pp2x_write(hw,
				      MVPP2_TXQ_SCHED_TOKEN_SIZE_REG(txq),
				      val);
		}
	}
}

/* Set Tx descriptors fields relevant for CSUM calculation */
u32 mv_pp2x_txq_desc_csum(int l3_offs, int l3_proto,
			  int ip_hdr_len, int l4_proto)
{
	u32 command;

	/* fields: L3_offset, IP_hdrlen, L3_type, G_IPv4_chk,
	 * G_L4_chk, L4_type required only for checksum calculation
	 */
	command = (l3_offs << MVPP2_TXD_L3_OFF_SHIFT);
	command |= (ip_hdr_len << MVPP2_TXD_IP_HLEN_SHIFT);
	command |= MVPP2_TXD_IP_CSUM_DISABLE;

	if (l3_proto == ETH_P_IP) {
		command &= ~MVPP2_TXD_IP_CSUM_DISABLE;	/* enable IPv4 csum */
		command &= ~MVPP2_TXD_L3_IP6;		/* enable IPv4 */
	} else {
		command |= MVPP2_TXD_L3_IP6;		/* enable IPv6 */
	}

	if (l4_proto == IPPROTO_TCP) {
		command &= ~MVPP2_TXD_L4_UDP;		/* enable TCP */
		command &= ~MVPP2_TXD_L4_CSUM_FRAG;	/* generate L4 csum */
	} else if (l4_proto == IPPROTO_UDP) {
		command |= MVPP2_TXD_L4_UDP;		/* enable UDP */
		command &= ~MVPP2_TXD_L4_CSUM_FRAG;	/* generate L4 csum */
	} else {
		command |= MVPP2_TXD_L4_CSUM_NOT;
	}

	return command;
}

/* Get number of sent descriptors and decrement counter.
 * The number of sent descriptors is returned.
 * Per-CPU access
 */

 /* Tx descriptors helper methods */

/* Get number of Tx descriptors waiting to be transmitted by HW */
int mv_pp2x_txq_pend_desc_num_get(struct mv_pp2x_port *port,
				  struct mv_pp2x_tx_queue *txq)
{
	u32 val;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	mv_pp2x_write(hw, MVPP2_TXQ_NUM_REG, txq->id);
	val = mv_pp2x_read(hw, MVPP2_TXQ_PENDING_REG);

	return val & MVPP2_TXQ_PENDING_MASK;
}

/* Get pointer to next Tx descriptor to be processed (send) by HW */
struct mv_pp2x_tx_desc *mv_pp2x_txq_next_desc_get(
		struct mv_pp2x_aggr_tx_queue *aggr_txq)
{
	int tx_desc = aggr_txq->next_desc_to_proc;

	aggr_txq->next_desc_to_proc = MVPP2_QUEUE_NEXT_DESC(aggr_txq, tx_desc);
	return aggr_txq->first_desc + tx_desc;
}

/* Update HW with number of aggregated Tx descriptors to be sent */
void mv_pp2x_aggr_txq_pend_desc_add(struct mv_pp2x_port *port, int pending)
{
	/* aggregated access - relevant TXQ number is written in TX desc */
	mv_pp2x_write(&port->priv->hw, MVPP2_AGGR_TXQ_UPDATE_REG, pending);
}

int mv_pp2x_aggr_desc_num_read(struct mv_pp2x *priv, int cpu)
{
	u32 val = mv_pp2x_read(&priv->hw, MVPP2_AGGR_TXQ_STATUS_REG(cpu));

	return(val & MVPP2_AGGR_TXQ_PENDING_MASK);
}

/* Check if there are enough free descriptors in aggregated txq.
 * If not, update the number of occupied descriptors and repeat the check.
 */
int mv_pp2x_aggr_desc_num_check(struct mv_pp2x *priv,
				struct mv_pp2x_aggr_tx_queue *aggr_txq,
				int num)
{
	if ((aggr_txq->count + num) > aggr_txq->size) {
		/* Update number of occupied aggregated Tx descriptors */
		int cpu = smp_processor_id();
		u32 val = mv_pp2x_relaxed_read(&priv->hw,
				MVPP2_AGGR_TXQ_STATUS_REG(cpu));

		aggr_txq->count = val & MVPP2_AGGR_TXQ_PENDING_MASK;

		if ((aggr_txq->count + num) > aggr_txq->size)
			return -ENOMEM;
	}

	return 0;
}

/* Reserved Tx descriptors allocation request */
int mv_pp2x_txq_alloc_reserved_desc(struct mv_pp2x *priv,
				    struct mv_pp2x_tx_queue *txq, int num)
{
	u32 val;

	val = (txq->id << MVPP2_TXQ_RSVD_REQ_Q_OFFSET) | num;
	mv_pp2x_relaxed_write(&priv->hw, MVPP2_TXQ_RSVD_REQ_REG, val);

	val = mv_pp2x_relaxed_read(&priv->hw, MVPP2_TXQ_RSVD_RSLT_REG);

	return val & MVPP2_TXQ_RSVD_RSLT_MASK;
}

/* Set rx queue offset */
void mv_pp2x_rxq_offset_set(struct mv_pp2x_port *port,
			    int prxq, int offset)
{
	u32 val;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	/* Convert offset from bytes to units of 32 bytes */
	offset = offset >> 5;

	val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(prxq));
	val &= ~MVPP2_RXQ_PACKET_OFFSET_MASK;

	/* Offset is in */
	val |= ((offset << MVPP2_RXQ_PACKET_OFFSET_OFFS) &
		MVPP2_RXQ_PACKET_OFFSET_MASK);

	mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(prxq), val);
}

/* Port configuration routines */

void mv_pp21_port_mii_set(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM
	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_2_REG);

	switch (port->mac_data.phy_mode) {
	case PHY_INTERFACE_MODE_SGMII:
		val |= MVPP2_GMAC_INBAND_AN_MASK;
		break;
	case PHY_INTERFACE_MODE_RGMII:
		val |= MVPP2_GMAC_PORT_RGMII_MASK;
	default:
		val &= ~MVPP2_GMAC_PCS_ENABLE_MASK;
	}

	writel(val, port->base + MVPP2_GMAC_CTRL_2_REG);
#endif
}

void mv_pp21_port_fc_adv_enable(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM
	u32 val;

	val = readl(port->base + MVPP2_GMAC_AUTONEG_CONFIG);
	val |= MVPP2_GMAC_FC_ADV_EN;
	writel(val, port->base + MVPP2_GMAC_AUTONEG_CONFIG);
#endif
}

void mv_pp21_port_enable(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM
	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_0_REG);
	val |= MVPP2_GMAC_PORT_EN_MASK;
	val |= MVPP2_GMAC_MIB_CNTR_EN_MASK;
	writel(val, port->base + MVPP2_GMAC_CTRL_0_REG);
#endif
}

void mv_pp21_port_disable(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM

	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_0_REG);
	val &= ~(MVPP2_GMAC_PORT_EN_MASK);
	writel(val, port->base + MVPP2_GMAC_CTRL_0_REG);
#endif
}

/* Set IEEE 802.3x Flow Control Xon Packet Transmission Mode */
void mv_pp21_port_periodic_xon_disable(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM

	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_1_REG) &
		    ~MVPP2_GMAC_PERIODIC_XON_EN_MASK;
	writel(val, port->base + MVPP2_GMAC_CTRL_1_REG);
#endif
}

/* Configure loopback port */
void mv_pp21_port_loopback_set(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM

	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_1_REG);

	if (port->mac_data.speed == 1000)
		val |= MVPP2_GMAC_GMII_LB_EN_MASK;
	else
		val &= ~MVPP2_GMAC_GMII_LB_EN_MASK;

	if (port->mac_data.phy_mode == PHY_INTERFACE_MODE_SGMII)
		val |= MVPP2_GMAC_PCS_LB_EN_MASK;
	else
		val &= ~MVPP2_GMAC_PCS_LB_EN_MASK;

	writel(val, port->base + MVPP2_GMAC_CTRL_1_REG);
#endif
}

void mv_pp21_port_reset(struct mv_pp2x_port *port)
{
#ifndef CONFIG_MV_PP2_PALLADIUM

	u32 val;

	val = readl(port->base + MVPP2_GMAC_CTRL_2_REG) &
		    ~MVPP2_GMAC_PORT_RESET_MASK;
	writel(val, port->base + MVPP2_GMAC_CTRL_2_REG);

	while (readl(port->base + MVPP2_GMAC_CTRL_2_REG) &
	       MVPP2_GMAC_PORT_RESET_MASK)
		continue;
#endif
}

/* Refill BM pool */
void mv_pp2x_pool_refill(struct mv_pp2x *priv, u32 pool,
			 dma_addr_t phys_addr, struct sk_buff *cookie)
{
	STAT_DBG(struct mv_pp2x_bm_pool *bm_pool = &priv->bm_pools[pool]);

	mv_pp2x_bm_pool_put(&priv->hw, pool, phys_addr, cookie);
	STAT_DBG(bm_pool->stats.bm_put++);
}

/* Set pool buffer size */
void mv_pp2x_bm_pool_bufsize_set(struct mv_pp2x_hw *hw,
				 struct mv_pp2x_bm_pool *bm_pool, int buf_size)
{
	u32 val;

	bm_pool->buf_size = buf_size;

	val = ALIGN(buf_size, 1 << MVPP2_POOL_BUF_SIZE_OFFSET);
	mv_pp2x_write(hw, MVPP2_POOL_BUF_SIZE_REG(bm_pool->id), val);
}

/* Attach long pool to rxq */
void mv_pp21_rxq_long_pool_set(struct mv_pp2x_hw *hw,
		int prxq, int long_pool)
{
	u32 val;

	val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(prxq));
	val &= ~MVPP21_RXQ_POOL_LONG_MASK;
	val |= ((long_pool << MVPP21_RXQ_POOL_LONG_OFFS) &
		    MVPP21_RXQ_POOL_LONG_MASK);

	mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(prxq), val);
}

/* Attach short pool to rxq */
void mv_pp21_rxq_short_pool_set(struct mv_pp2x_hw *hw,
				int prxq, int short_pool)
{
	u32 val;

	val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(prxq));
	val &= ~MVPP21_RXQ_POOL_SHORT_MASK;
	val |= ((short_pool << MVPP21_RXQ_POOL_SHORT_OFFS) &
		    MVPP21_RXQ_POOL_SHORT_MASK);

	mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(prxq), val);
}

/* Attach long pool to rxq */
void mv_pp22_rxq_long_pool_set(struct mv_pp2x_hw *hw,
			       int prxq, int long_pool)
{
	u32 val;

	val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(prxq));
	val &= ~MVPP22_RXQ_POOL_LONG_MASK;
	val |= ((long_pool << MVPP22_RXQ_POOL_LONG_OFFS) &
		    MVPP22_RXQ_POOL_LONG_MASK);

	mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(prxq), val);
}

/* Attach short pool to rxq */
void mv_pp22_rxq_short_pool_set(struct mv_pp2x_hw *hw,
				int prxq, int short_pool)
{
	u32 val;

	val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(prxq));
	val &= ~MVPP22_RXQ_POOL_SHORT_MASK;
	val |= ((short_pool << MVPP22_RXQ_POOL_SHORT_OFFS) &
		    MVPP22_RXQ_POOL_SHORT_MASK);

	mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(prxq), val);
}

/* Enable/disable receiving packets */
void mv_pp2x_ingress_enable(struct mv_pp2x_port *port)
{
	u32 val;
	int lrxq, queue;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	for (lrxq = 0; lrxq < port->num_rx_queues; lrxq++) {
		queue = port->rxqs[lrxq]->id;
		val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(queue));
		val &= ~MVPP2_RXQ_DISABLE_MASK;
		mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(queue), val);
	}
}

void mv_pp2x_ingress_disable(struct mv_pp2x_port *port)
{
	u32 val;
	int lrxq, queue;
	struct mv_pp2x_hw *hw = &port->priv->hw;

	for (lrxq = 0; lrxq < port->num_rx_queues; lrxq++) {
		queue = port->rxqs[lrxq]->id;
		val = mv_pp2x_read(hw, MVPP2_RXQ_CONFIG_REG(queue));
		val |= MVPP2_RXQ_DISABLE_MASK;
		mv_pp2x_write(hw, MVPP2_RXQ_CONFIG_REG(queue), val);
	}
}

void mv_pp2x_egress_enable(struct mv_pp2x_port *port)
{
	u32 qmap;
	int queue;
	int tx_port_num = mv_pp2x_egress_port(port);
	struct mv_pp2x_hw *hw = &port->priv->hw;

	/* Enable all initialized TXs. */
	qmap = 0;
	for (queue = 0; queue < port->num_tx_queues; queue++) {
		struct mv_pp2x_tx_queue *txq = port->txqs[queue];

		if (txq->first_desc != NULL)
			qmap |= (1 << queue);
	}

	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_Q_CMD_REG, qmap);

	pr_debug("tx_port_num=%d qmap=0x%x\n", tx_port_num, qmap);
}

/* Disable transmit via physical egress queue
 * - HW doesn't take descriptors from DRAM
 */
void mv_pp2x_egress_disable(struct mv_pp2x_port *port)
{
	u32 reg_data;
	int delay;
	int tx_port_num = mv_pp2x_egress_port(port);
	struct mv_pp2x_hw *hw = &port->priv->hw;

	/* Issue stop command for active channels only */
	mv_pp2x_write(hw, MVPP2_TXP_SCHED_PORT_INDEX_REG, tx_port_num);
	reg_data = (mv_pp2x_read(hw, MVPP2_TXP_SCHED_Q_CMD_REG)) &
		    MVPP2_TXP_SCHED_ENQ_MASK;
	if (reg_data != 0)
		mv_pp2x_write(hw, MVPP2_TXP_SCHED_Q_CMD_REG,
			      (reg_data << MVPP2_TXP_SCHED_DISQ_OFFSET));

	/* Wait for all Tx activity to terminate. */
	delay = 0;
	do {
		if (delay >= MVPP2_TX_DISABLE_TIMEOUT_MSEC) {
			netdev_warn(port->dev,
				    "Tx stop timed out, status=0x%08x\n",
				    reg_data);
			break;
		}
		mdelay(1);
		delay++;

		/* Check port TX Command register that all
		 * Tx queues are stopped
		 */
		reg_data = mv_pp2x_read(hw, MVPP2_TXP_SCHED_Q_CMD_REG);
	} while (reg_data & MVPP2_TXP_SCHED_ENQ_MASK);
}

/* Parser default initialization */
int mv_pp2x_prs_default_init(struct platform_device *pdev,
		struct mv_pp2x_hw *hw)
{
	int err, index, i;

	PALAD(MVPP2_PRINT_LINE());

	/* Enable tcam table */
	mv_pp2x_write(hw, MVPP2_PRS_TCAM_CTRL_REG, MVPP2_PRS_TCAM_EN_MASK);

	/* Clear all tcam and sram entries */
	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		PALAD(MVPP2_PRINT_LINE());
		mv_pp2x_write(hw, MVPP2_PRS_TCAM_IDX_REG, index);
		for (i = 0; i < MVPP2_PRS_TCAM_WORDS; i++)
			mv_pp2x_write(hw, MVPP2_PRS_TCAM_DATA_REG(i), 0);

		mv_pp2x_write(hw, MVPP2_PRS_SRAM_IDX_REG, index);
		for (i = 0; i < MVPP2_PRS_SRAM_WORDS; i++)
			mv_pp2x_write(hw, MVPP2_PRS_SRAM_DATA_REG(i), 0);
	}
	PALAD(MVPP2_PRINT_LINE());

	/* Invalidate all tcam entries */
	for (index = 0; index < MVPP2_PRS_TCAM_SRAM_SIZE; index++) {
		PALAD(MVPP2_PRINT_LINE());
		mv_pp2x_prs_hw_inv(hw, index);
	}
	PALAD(MVPP2_PRINT_LINE());

	hw->prs_shadow = devm_kcalloc(&pdev->dev, MVPP2_PRS_TCAM_SRAM_SIZE,
				      sizeof(struct mv_pp2x_prs_shadow),
				      GFP_KERNEL);
	PALAD(MVPP2_PRINT_LINE());

	if (!hw->prs_shadow)
		return -ENOMEM;
	PALAD(MVPP2_PRINT_LINE());

	/* Always start from lookup = 0 */
	for (index = 0; index < MVPP2_MAX_PORTS; index++)
		mv_pp2x_prs_hw_port_init(hw, index, MVPP2_PRS_LU_MH,
					 MVPP2_PRS_PORT_LU_MAX, 0);
	PALAD(MVPP2_PRINT_LINE());

	mv_pp2x_prs_def_flow_init(hw);
	PALAD(MVPP2_PRINT_LINE());

	mv_pp2x_prs_mh_init(hw);
	PALAD(MVPP2_PRINT_LINE());

	mv_pp2x_prs_mac_init(hw);
	PALAD(MVPP2_PRINT_LINE());

	mv_pp2x_prs_dsa_init(hw);
	PALAD(MVPP2_PRINT_LINE());

	err = mv_pp2x_prs_etype_init(hw);
	if (err)
		return err;
	PALAD(MVPP2_PRINT_LINE());

	err = mv_pp2x_prs_vlan_init(pdev, hw);
	if (err)
		return err;
	PALAD(MVPP2_PRINT_LINE());
#if !defined(CONFIG_MV_PP2_PALLADIUM)
	err = mv_pp2x_prs_pppoe_init(hw);
	if (err)
		return err;

	err = mv_pp2x_prs_ip6_init(hw);
	if (err)
		return err;
#endif

	err = mv_pp2x_prs_ip4_init(hw);
	if (err)
		return err;
	PALAD(MVPP2_PRINT_LINE());
	return 0;
}

/* shift to (current offset + shift) */
int mv_pp2x_prs_sw_sram_shift_set(struct mv_pp2x_prs_entry *pe,
				  int shift, unsigned int op)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(shift, 0 - MVPP2_PRS_SRAM_SHIFT_MASK,
	    MVPP2_PRS_SRAM_SHIFT_MASK) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(op, 0,
	    MVPP2_PRS_SRAM_OP_SEL_SHIFT_MASK) == MV_ERROR)
		return MV_ERROR;

	/* Set sign */
	if (shift < 0) {
		pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] |=
			(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8));
		shift = 0 - shift;
	} else
		pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] &=
			(~(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8)));

	/* Set offset */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_SHIFT_OFFS)] = (unsigned char)shift;

	/* Reset and Set operation */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS)] &=
		~(MVPP2_PRS_SRAM_OP_SEL_SHIFT_MASK <<
		(MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS)] |=
		(op << (MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8));

	/* Set base offset as current */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS)] &=
		(~(1 << (MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS % 8)));

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_shift_set);

int mv_pp2x_prs_sw_sram_shift_get(struct mv_pp2x_prs_entry *pe, int *shift)
{
	int sign;

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;
	if (mv_pp2x_ptr_validate(shift) == MV_ERROR)
		return MV_ERROR;

	sign = pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] &
		(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8));
	*shift = ((int)(pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_SHIFT_OFFS)])) &
		MVPP2_PRS_SRAM_SHIFT_MASK;

	if (sign == 1)
		*shift *= -1;
	return MV_OK;
}

int mv_pp2x_prs_sw_sram_offset_set(struct mv_pp2x_prs_entry *pe,
				   unsigned int type, int offset,
				   unsigned int op)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(offset, 0 - MVPP2_PRS_SRAM_UDF_MASK,
	    MVPP2_PRS_SRAM_UDF_MASK) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(type, 0,
	    MVPP2_PRS_SRAM_UDF_TYPE_MASK) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(op, 0,
		MVPP2_PRS_SRAM_OP_SEL_UDF_MASK) == MV_ERROR)
		return MV_ERROR;

	/* Set offset sign */
	if (offset < 0) {
		offset = 0 - offset;
		/* set sram offset sign bit */
		pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] |=
			(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8));
	} else
		pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_SHIFT_SIGN_BIT)] &=
			(~(1 << (MVPP2_PRS_SRAM_SHIFT_SIGN_BIT % 8)));

	/* set offset value */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS)] &=
		(~(MVPP2_PRS_SRAM_UDF_MASK <<
		(MVPP2_PRS_SRAM_UDF_OFFS % 8)));
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS)] |=
		(offset << (MVPP2_PRS_SRAM_UDF_OFFS % 8));
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS + MVPP2_PRS_SRAM_UDF_BITS)] &=
		~(MVPP2_PRS_SRAM_UDF_MASK >>
		(8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8)));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS + MVPP2_PRS_SRAM_UDF_BITS)] |=
		(offset >> (8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8)));

	/* set offset type */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_TYPE_OFFS)] &=
		~(MVPP2_PRS_SRAM_UDF_TYPE_MASK <<
		(MVPP2_PRS_SRAM_UDF_TYPE_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_TYPE_OFFS)] |=
		(type << (MVPP2_PRS_SRAM_UDF_TYPE_OFFS % 8));

	/* Set offset operation */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS)] &=
		~(MVPP2_PRS_SRAM_OP_SEL_UDF_MASK <<
		(MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS)] |=
		(op << (MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
		MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] &=
		 ~(MVPP2_PRS_SRAM_OP_SEL_UDF_MASK >>
		 (8 - (MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS +
		MVPP2_PRS_SRAM_OP_SEL_UDF_BITS)] |=
		(op >> (8 - (MVPP2_PRS_SRAM_OP_SEL_UDF_OFFS % 8)));

	/* Set base offset as current */
	pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS)] &=
		(~(1 << (MVPP2_PRS_SRAM_OP_SEL_BASE_OFFS % 8)));

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_offset_set);

int mv_pp2x_prs_sw_sram_offset_get(struct mv_pp2x_prs_entry *pe,
				   unsigned int *type, int *offset,
				   unsigned int *op)
{
	int sign;

	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(offset) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(type) == MV_ERROR)
		return MV_ERROR;

	*type = pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_TYPE_OFFS)] >>
		(MVPP2_PRS_SRAM_UDF_TYPE_OFFS % 8);
	*type &= MVPP2_PRS_SRAM_UDF_TYPE_MASK;

	*offset = (pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_OFFS)] >>
		(MVPP2_PRS_SRAM_UDF_OFFS % 8)) & 0x7f;
	*offset |= (pe->sram.byte[
		    SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS) +
		    SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_UDF_OFFS)] <<
		    (8 - (MVPP2_PRS_SRAM_UDF_OFFS % 8))) & 0x80;

	*op = (pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS)] >>
		(MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8)) & 0x7;
	*op |= (pe->sram.byte[HW_BYTE_OFFS(
		SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS) +
		SRAM_BIT_TO_BYTE(MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS))] <<
		(8 - (MVPP2_PRS_SRAM_OP_SEL_SHIFT_OFFS % 8))) & 0x18;

	/* if signed bit is tes */
	sign = pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_UDF_SIGN_BIT)] &
		(1 << (MVPP2_PRS_SRAM_UDF_SIGN_BIT % 8));
	if (sign != 0)
		*offset = 1 - (*offset);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_offset_get);

int mv_pp2x_prs_sw_sram_next_lu_get(struct mv_pp2x_prs_entry *pe,
				    unsigned int *lu)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(lu) == MV_ERROR)
		return MV_ERROR;

	*lu = pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_NEXT_LU_OFFS)];
	*lu = ((*lu) >> MVPP2_PRS_SRAM_NEXT_LU_OFFS % 8);
	*lu &= MVPP2_PRS_SRAM_NEXT_LU_MASK;
	return MV_OK;
}

int mv_pp2x_prs_sram_bit_get(struct mv_pp2x_prs_entry *pe, int bitNum,
			     unsigned int *bit)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	*bit = pe->sram.byte[SRAM_BIT_TO_BYTE(bitNum)]  &
		(1 << (bitNum % 8));
	*bit = (*bit) >> (bitNum % 8);
	return MV_OK;
}

void mv_pp2x_prs_sw_sram_lu_done_set(struct mv_pp2x_prs_entry *pe)
{
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_lu_done_set);

void mv_pp2x_prs_sw_sram_lu_done_clear(struct mv_pp2x_prs_entry *pe)
{
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, 1);
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_lu_done_clear);

int mv_pp2x_prs_sw_sram_lu_done_get(struct mv_pp2x_prs_entry *pe,
				    unsigned int *bit)
{
	return mv_pp2x_prs_sram_bit_get(pe, MVPP2_PRS_SRAM_LU_DONE_BIT, bit);
}

void mv_pp2x_prs_sw_sram_flowid_set(struct mv_pp2x_prs_entry *pe)
{
	mv_pp2x_prs_sram_bits_set(pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_flowid_set);

void mv_pp2x_prs_sw_sram_flowid_clear(struct mv_pp2x_prs_entry *pe)
{
	mv_pp2x_prs_sram_bits_clear(pe, MVPP2_PRS_SRAM_LU_GEN_BIT, 1);
}
EXPORT_SYMBOL(mv_pp2x_prs_sw_sram_flowid_clear);

int mv_pp2x_prs_sw_sram_flowid_gen_get(struct mv_pp2x_prs_entry *pe,
				       unsigned int *bit)
{
	return mv_pp2x_prs_sram_bit_get(pe, MVPP2_PRS_SRAM_LU_GEN_BIT, bit);
}

/* return RI and RI_UPDATE */
int mv_pp2x_prs_sw_sram_ri_get(struct mv_pp2x_prs_entry *pe,
			       unsigned int *bits, unsigned int *enable)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(bits) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(enable) == MV_ERROR)
		return MV_ERROR;

	*bits = pe->sram.word[MVPP2_PRS_SRAM_RI_OFFS/32];
	*enable = pe->sram.word[MVPP2_PRS_SRAM_RI_CTRL_OFFS/32];
	return MV_OK;
}

int mv_pp2x_prs_sw_sram_ai_get(struct mv_pp2x_prs_entry *pe,
			       unsigned int *bits, unsigned int *enable)
{
	if (mv_pp2x_ptr_validate(pe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(bits) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(enable) == MV_ERROR)
		return MV_ERROR;

	*bits = (pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_AI_OFFS)] >> (MVPP2_PRS_SRAM_AI_OFFS % 8)) |
		(pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_AI_OFFS +
			MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
			(8 - (MVPP2_PRS_SRAM_AI_OFFS % 8)));

	*enable = (pe->sram.byte[SRAM_BIT_TO_BYTE(
		MVPP2_PRS_SRAM_AI_CTRL_OFFS)] >>
		(MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)) |
		(pe->sram.byte[SRAM_BIT_TO_BYTE(
			MVPP2_PRS_SRAM_AI_CTRL_OFFS +
			MVPP2_PRS_SRAM_AI_CTRL_BITS)] <<
			(8 - (MVPP2_PRS_SRAM_AI_CTRL_OFFS % 8)));

	*bits &= MVPP2_PRS_SRAM_AI_MASK;
	*enable &= MVPP2_PRS_SRAM_AI_MASK;

	return MV_OK;
}

/*#include "mvPp2ClsHw.h" */

/********************************************************************/
/***************** Classifier Top Public lkpid table APIs ********************/
/********************************************************************/

/*------------------------------------------------------------------*/

int mv_pp2x_cls_hw_lkp_read(struct mv_pp2x_hw *hw, int lkpid, int way,
			    struct mv_pp2x_cls_lookup_entry *fe)
{
	unsigned int regVal = 0;

	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(way, 0, WAY_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(lkpid, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	/* write index reg */
	regVal = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) |
		(lkpid << MVPP2_CLS_LKP_INDEX_LKP_OFFS);
	mv_pp2x_write(hw, MVPP2_CLS_LKP_INDEX_REG, regVal);

	fe->way = way;
	fe->lkpid = lkpid;

	fe->data = mv_pp2x_read(hw, MVPP2_CLS_LKP_TBL_REG);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_read);

int mv_pp2x_cls_hw_lkp_write(struct mv_pp2x_hw *hw, int lkpid,
			     int way, struct mv_pp2x_cls_lookup_entry *fe)
{
	unsigned int regVal = 0;

	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(way, 0, 1) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(lkpid, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	/* write index reg */
	regVal = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) |
		(lkpid << MVPP2_CLS_LKP_INDEX_LKP_OFFS);
	mv_pp2x_write(hw, MVPP2_CLS_LKP_INDEX_REG, regVal);

	/* write flowId reg */
	mv_pp2x_write(hw, MVPP2_CLS_LKP_TBL_REG, fe->data);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_write);

int mv_pp2x_cls_hw_lkp_print(struct mv_pp2x_hw *hw, int lkpid, int way)
{
	unsigned int uint32bit;
	int int32bit;
	struct mv_pp2x_cls_lookup_entry lkp;

	if (mv_pp2x_range_validate(way, 0, WAY_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(lkpid, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	mv_pp2x_cls_hw_lkp_read(hw, lkpid, way, &lkp);

	DBG_MSG(" 0x%2.2x  %1.1d\t", lkp.lkpid, lkp.way);
	mv_pp2x_cls_sw_lkp_rxq_get(&lkp, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);
	mv_pp2x_cls_sw_lkp_en_get(&lkp, &int32bit);
	DBG_MSG("%1.1d\t", int32bit);
	mv_pp2x_cls_sw_lkp_flow_get(&lkp, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);
	mv_pp2x_cls_sw_lkp_mod_get(&lkp, &int32bit);
	DBG_MSG(" 0x%2.2x\t", int32bit);
	mv_pp2x_cls_hw_lkp_hit_get(hw, lkp.lkpid, way, &uint32bit);
	DBG_MSG(" 0x%8.8x\n", uint32bit);
	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_print);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_lkp_rxq_get(struct mv_pp2x_cls_lookup_entry *lkp, int *rxq)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(rxq) == MV_ERROR)
		return MV_ERROR;

	*rxq =  (lkp->data & MVPP2_FLOWID_RXQ_MASK) >> MVPP2_FLOWID_RXQ;
	return MV_OK;
}

int mv_pp2x_cls_sw_lkp_rxq_set(struct mv_pp2x_cls_lookup_entry *lkp, int rxq)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(rxq, 0,
	    (1 << MVPP2_FLOWID_RXQ_BITS) - 1) == MV_ERROR)
		return MV_ERROR;

	lkp->data &= ~MVPP2_FLOWID_RXQ_MASK;
	lkp->data |= (rxq << MVPP2_FLOWID_RXQ);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_rxq_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_lkp_en_get(struct mv_pp2x_cls_lookup_entry *lkp, int *en)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(en) == MV_ERROR)
		return MV_ERROR;

	*en = (lkp->data & MVPP2_FLOWID_EN_MASK) >> MVPP2_FLOWID_EN;
	return MV_OK;
}

int mv_pp2x_cls_sw_lkp_en_set(struct mv_pp2x_cls_lookup_entry *lkp, int en)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(en, 0, 1) == MV_ERROR)
		return MV_ERROR;

	lkp->data &= ~MVPP2_FLOWID_EN_MASK;
	lkp->data |= (en << MVPP2_FLOWID_EN);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_en_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_lkp_flow_get(struct mv_pp2x_cls_lookup_entry *lkp,
				int *flow_idx)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(flow_idx) == MV_ERROR)
		return MV_ERROR;

	*flow_idx = (lkp->data & MVPP2_FLOWID_FLOW_MASK) >> MVPP2_FLOWID_FLOW;
	return MV_OK;
}

int mv_pp2x_cls_sw_lkp_flow_set(struct mv_pp2x_cls_lookup_entry *lkp,
		int flow_idx)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(flow_idx, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	lkp->data &= ~MVPP2_FLOWID_FLOW_MASK;
	lkp->data |= (flow_idx << MVPP2_FLOWID_FLOW);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_flow_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_lkp_mod_get(struct mv_pp2x_cls_lookup_entry *lkp,
			       int *mod_base)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(mod_base) == MV_ERROR)
		return MV_ERROR;

	*mod_base = (lkp->data & MVPP2_FLOWID_MODE_MASK) >> MVPP2_FLOWID_MODE;
	return MV_OK;
}

int mv_pp2x_cls_sw_lkp_mod_set(struct mv_pp2x_cls_lookup_entry *lkp,
			       int mod_base)
{
	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	/* TODO: what is the max value of mode base */
	if (mv_pp2x_range_validate(mod_base, 0,
	    (1 << MVPP2_FLOWID_MODE_BITS) - 1) == MV_ERROR)
		return MV_ERROR;

	lkp->data &= ~MVPP2_FLOWID_MODE_MASK;
	lkp->data |= (mod_base << MVPP2_FLOWID_MODE);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_mod_set);

/*********************************************************************/
/***************** Classifier Top Public flows table APIs  ********************/
/********************************************************************/

int mv_pp2x_cls_hw_flow_read(struct mv_pp2x_hw *hw, int index,
			     struct mv_pp2x_cls_flow_entry *fe)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(index, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	fe->index = index;

	/*write index*/
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_INDEX_REG, index);

	fe->data[0] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL0_REG);
	fe->data[1] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL1_REG);
	fe->data[2] = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL2_REG);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_flow_read);

/*----------------------------------------------------------------------*/
/*PPv2.1 new feature MAS 3.18*/

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_flow_hek_get(struct mv_pp2x_cls_flow_entry *fe,
				int *num_of_fields, int field_ids[])
{
	int index;

	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(num_of_fields) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(field_ids) == MV_ERROR)
		return MV_ERROR;

	*num_of_fields = (fe->data[1] & MVPP2_FLOW_FIELDS_NUM_MASK) >>
			MVPP2_FLOW_FIELDS_NUM;

	for (index = 0; index < (*num_of_fields); index++)
		field_ids[index] = ((fe->data[2] &
			MVPP2_FLOW_FIELD_MASK(index)) >>
			MVPP2_FLOW_FIELD_ID(index));

	return MV_OK;
}

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_port_get(struct mv_pp2x_cls_flow_entry *fe,
				 int *type, int *portid)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(type) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(portid) == MV_ERROR)
		return MV_ERROR;

	*type = (fe->data[0] & MVPP2_FLOW_PORT_TYPE_MASK) >>
			MVPP2_FLOW_PORT_TYPE;
	*portid = (fe->data[0] & MVPP2_FLOW_PORT_ID_MASK) >>
			MVPP2_FLOW_PORT_ID;

	return MV_OK;
}

int mv_pp2x_cls_sw_flow_port_set(struct mv_pp2x_cls_flow_entry *fe,
				 int type, int portid)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(type, 0,
	    ((1 << MVPP2_FLOW_PORT_TYPE_BITS) - 1)) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(portid, 0,
	    ((1 << MVPP2_FLOW_PORT_ID_BITS) - 1)) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_PORT_ID_MASK;
	fe->data[0] &= ~MVPP2_FLOW_PORT_TYPE_MASK;

	fe->data[0] |= (portid << MVPP2_FLOW_PORT_ID);
	fe->data[0] |= (type << MVPP2_FLOW_PORT_TYPE);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_port_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_portid_select(struct mv_pp2x_cls_flow_entry *fe,
				      int from)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(from, 0, 1) == MV_ERROR)
		return MV_ERROR;

	if (from)
		fe->data[0] |= MVPP2_FLOW_PORT_ID_SEL_MASK;
	else
		fe->data[0] &= ~MVPP2_FLOW_PORT_ID_SEL_MASK;

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_portid_select);

int mv_pp2x_cls_sw_flow_pppoe_set(struct mv_pp2x_cls_flow_entry *fe, int mode)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mode, 0, MVPP2_FLOW_PPPOE_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_PPPOE_MASK;
	fe->data[0] |= (mode << MVPP2_FLOW_PPPOE);
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_pppoe_set);

int mv_pp2x_cls_sw_flow_vlan_set(struct mv_pp2x_cls_flow_entry *fe, int mode)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mode, 0, MVPP2_FLOW_VLAN_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_VLAN_MASK;
	fe->data[0] |= (mode << MVPP2_FLOW_VLAN);
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_vlan_set);

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_flow_macme_set(struct mv_pp2x_cls_flow_entry *fe, int mode)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mode, 0, MVPP2_FLOW_MACME_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_MACME_MASK;
	fe->data[0] |= (mode << MVPP2_FLOW_MACME);
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_macme_set);

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_flow_udf7_set(struct mv_pp2x_cls_flow_entry *fe, int mode)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mode, 0, MVPP2_FLOW_UDF7_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_UDF7_MASK;
	fe->data[0] |= (mode << MVPP2_FLOW_UDF7);
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_udf7_set);

int mv_pp2x_cls_sw_flow_seq_ctrl_set(struct mv_pp2x_cls_flow_entry *fe,
				     int mode)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(mode, 0, MVPP2_FLOW_ENGINE_MAX) == MV_ERROR)
		return MV_ERROR;

	fe->data[1] &= ~MVPP2_FLOW_SEQ_CTRL_MASK;
	fe->data[1] |= (mode << MVPP2_FLOW_SEQ_CTRL);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_seq_ctrl_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_engine_get(struct mv_pp2x_cls_flow_entry *fe,
				   int *engine, int *is_last)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(engine) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(is_last) == MV_ERROR)
		return MV_ERROR;

	*engine = (fe->data[0] & MVPP2_FLOW_ENGINE_MASK) >> MVPP2_FLOW_ENGINE;
	*is_last = fe->data[0] & MVPP2_FLOW_LAST_MASK;

	return MV_OK;
}

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_flow_engine_set(struct mv_pp2x_cls_flow_entry *fe,
				   int engine, int is_last)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(is_last, 0, 1) == MV_ERROR)
		return MV_ERROR;

	fe->data[0] &= ~MVPP2_FLOW_LAST_MASK;
	fe->data[0] &= ~MVPP2_FLOW_ENGINE_MASK;

	fe->data[0] |= is_last;
	fe->data[0] |= (engine << MVPP2_FLOW_ENGINE);

	return MV_OK;

}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_engine_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_extra_get(struct mv_pp2x_cls_flow_entry *fe,
				  int *type, int *prio)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(type) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(prio) == MV_ERROR)
		return MV_ERROR;

	*type = (fe->data[1] & MVPP2_FLOW_LKP_TYPE_MASK) >>
		MVPP2_FLOW_LKP_TYPE;
	*prio = (fe->data[1] & MVPP2_FLOW_FIELD_PRIO_MASK) >>
		MVPP2_FLOW_FIELD_PRIO;

	return MV_OK;
}

int mv_pp2x_cls_sw_flow_extra_set(struct mv_pp2x_cls_flow_entry *fe,
				  int type, int prio)
{
	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(type, 0,
	    MVPP2_FLOW_PORT_ID_MAX) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(prio, 0,
	    ((1 << MVPP2_FLOW_FIELD_ID_BITS) - 1)) == MV_ERROR)
		return MV_ERROR;

	fe->data[1] &= ~MVPP2_FLOW_LKP_TYPE_MASK;
	fe->data[1] |= (type << MVPP2_FLOW_LKP_TYPE);

	fe->data[1] &= ~MVPP2_FLOW_FIELD_PRIO_MASK;
	fe->data[1] |= (prio << MVPP2_FLOW_FIELD_PRIO);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_extra_set);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_sw_flow_dump(struct mv_pp2x_cls_flow_entry *fe)
{
	int	int32bit_1, int32bit_2, i;
	int	fieldsArr[MVPP2_CLS_FLOWS_TBL_FIELDS_MAX];
	int	status = MV_OK;

	if (mv_pp2x_ptr_validate(fe) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG(
	"INDEX: F[0] F[1] F[2] F[3] PRT[T  ID] ENG LAST LKP_TYP  PRIO\n");

	/*index*/
	DBG_MSG("0x%3.3x  ", fe->index);

	/*filed[0] filed[1] filed[2] filed[3]*/
	status |= mv_pp2x_cls_sw_flow_hek_get(fe, &int32bit_1, fieldsArr);

	for (i = 0 ; i < MVPP2_CLS_FLOWS_TBL_FIELDS_MAX; i++)
		if (i < int32bit_1)
			DBG_MSG("0x%2.2x ", fieldsArr[i]);
		else
			DBG_MSG(" NA  ");

	/*port_type port_id*/
	status |= mv_pp2x_cls_sw_flow_port_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("[%1d  0x%3.3x]  ", int32bit_1, int32bit_2);

	/* engine_num last_bit*/
	status |= mv_pp2x_cls_sw_flow_engine_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("%1d   %1d    ", int32bit_1, int32bit_2);

	/* lookup_type priority*/
	status |= mv_pp2x_cls_sw_flow_extra_get(fe, &int32bit_1, &int32bit_2);
	DBG_MSG("0x%2.2x    0x%2.2x", int32bit_1, int32bit_2);

	DBG_MSG("\n");
	DBG_MSG("\n");
	DBG_MSG("       PPPEO   VLAN   MACME   UDF7   SELECT SEQ_CTRL\n");
	DBG_MSG("         %1d      %1d      %1d       %1d      %1d      %1d\n",
			(fe->data[0] & MVPP2_FLOW_PPPOE_MASK) >>
					MVPP2_FLOW_PPPOE,
			(fe->data[0] & MVPP2_FLOW_VLAN_MASK) >>
					MVPP2_FLOW_VLAN,
			(fe->data[0] & MVPP2_FLOW_MACME_MASK) >>
					MVPP2_FLOW_MACME,
			(fe->data[0] & MVPP2_FLOW_UDF7_MASK) >>
					MVPP2_FLOW_UDF7,
			(fe->data[0] & MVPP2_FLOW_PORT_ID_SEL_MASK) >>
					MVPP2_FLOW_PORT_ID_SEL,
			(fe->data[1] & MVPP2_FLOW_SEQ_CTRL_MASK) >>
					MVPP2_FLOW_SEQ_CTRL);
	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_flow_dump);

/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*	Classifier Top Public length change table APIs			*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*	additional cls debug APIs					*/
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_hw_regs_dump(struct mv_pp2x_hw *hw)
{
	int i = 0;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_CLS_MODE_REG,
			  "MVPP2_CLS_MODE_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_PORT_WAY_REG,
			  "MVPP2_CLS_PORT_WAY_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_LKP_INDEX_REG,
			  "MVPP2_CLS_LKP_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_LKP_TBL_REG,
			  "MVPP2_CLS_LKP_TBL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_INDEX_REG,
			  "MVPP2_CLS_FLOW_INDEX_REG");

	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL0_REG,
			  "MVPP2_CLS_FLOW_TBL0_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL1_REG,
			  "MVPP2_CLS_FLOW_TBL1_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_FLOW_TBL2_REG,
			  "MVPP2_CLS_FLOW_TBL2_REG");

	mv_pp2x_print_reg(hw, MVPP2_CLS_PORT_SPID_REG,
			  "MVPP2_CLS_PORT_SPID_REG");

	for (i = 0; i < MVPP2_CLS_SPID_UNI_REGS; i++) {
		sprintf(reg_name, "MVPP2_CLS_SPID_UNI_%d_REG", i);
		mv_pp2x_print_reg(hw, (MVPP2_CLS_SPID_UNI_BASE_REG + (4 * i)),
				  reg_name);
	}
	for (i = 0; i < MVPP2_CLS_GEM_VIRT_REGS_NUM; i++) {
		/* indirect access */
		mv_pp2x_write(hw, MVPP2_CLS_GEM_VIRT_INDEX_REG, i);
		sprintf(reg_name, "MVPP2_CLS_GEM_VIRT_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_GEM_VIRT_REG, reg_name);
	}
	for (i = 0; i < MVPP2_CLS_UDF_BASE_REGS; i++)	{
		sprintf(reg_name, "MVPP2_CLS_UDF_REG_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_UDF_REG(i), reg_name);
	}
	for (i = 0; i < 16; i++) {
		sprintf(reg_name, "MVPP2_CLS_MTU_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_MTU_REG(i), reg_name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_OVER_RXQ_LOW_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_OVERSIZE_RXQ_LOW_REG(i),
				reg_name);
	}
	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_SWFWD_P2HQ_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS_SWFWD_P2HQ_REG(i), reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_CLS_SWFWD_PCTRL_REG,
			  "MVPP2_CLS_SWFWD_PCTRL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS_SEQ_SIZE_REG,
			  "MVPP2_CLS_SEQ_SIZE_REG");

	for (i = 0; i < MVPP2_MAX_PORTS; i++) {
		sprintf(reg_name, "MVPP2_CLS_PCTRL_%d_REG", i);
		mv_pp2x_print_reg(hw, MV_PP2_CLS_PCTRL_REG(i), reg_name);
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_regs_dump);
/*----------------------------------------------------------------------*/
static int mv_pp2x_cls_hw_flow_hit_get(struct mv_pp2x_hw *hw,
				       int index,  unsigned int *cnt)
{
	if (mv_pp2x_range_validate(index, 0,
	    MVPP2_CLS_FLOWS_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	/*set index */
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_FLOW(index));

	if (cnt)
		*cnt = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL_HIT_REG);
	else
		DBG_MSG("HITS = %d\n", mv_pp2x_read(hw,
			MVPP2_CLS_FLOW_TBL_HIT_REG));

	return MV_OK;
}
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_hw_lkp_hit_get(struct mv_pp2x_hw *hw, int lkpid, int way,
			       unsigned int *cnt)
{
	if (mv_pp2x_range_validate(way, 0, 1) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(lkpid, 0,
	    MVPP2_CLS_LKP_TBL_SIZE) == MV_ERROR)
		return MV_ERROR;

	/*set index */
	mv_pp2x_write(hw, MVPP2_CNT_IDX_REG, MVPP2_CNT_IDX_LKP(lkpid, way));

	if (cnt)
		*cnt = mv_pp2x_read(hw, MVPP2_CLS_LKP_TBL_HIT_REG);
	else
		DBG_MSG("HITS: %d\n", mv_pp2x_read(hw,
			MVPP2_CLS_LKP_TBL_HIT_REG));

	return MV_OK;
}
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_hw_flow_dump(struct mv_pp2x_hw *hw)
{
	int index;

	struct mv_pp2x_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mv_pp2x_cls_hw_flow_read(hw, index, &fe);
		mv_pp2x_cls_sw_flow_dump(&fe);
		mv_pp2x_cls_hw_flow_hit_get(hw, index, NULL);
		DBG_MSG("-------------------------------------------------\n");
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_flow_dump);

/*----------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mv_pp2x_cls_hw_flow_hits_dump(struct mv_pp2x_hw *hw)
{
	int index;
	unsigned int cnt;
	struct mv_pp2x_cls_flow_entry fe;

	for (index = 0; index < MVPP2_CLS_FLOWS_TBL_SIZE ; index++) {
		mv_pp2x_cls_hw_flow_hit_get(hw, index, &cnt);
		if (cnt != 0) {
			mv_pp2x_cls_hw_flow_read(hw, index, &fe);
			mv_pp2x_cls_sw_flow_dump(&fe);
			DBG_MSG("HITS = %d\n", cnt);
			DBG_MSG("\n");
		}
	}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_flow_hits_dump);

/*----------------------------------------------------------------------*/
/*PPv2.1 new counters MAS 3.20*/
int mv_pp2x_cls_hw_lkp_hits_dump(struct mv_pp2x_hw *hw)
{
	int index, way, entryInd;
	unsigned int cnt;

	DBG_MSG("< ID  WAY >:	HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			entryInd = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) |
				index;
			mv_pp2x_cls_hw_lkp_hit_get(hw, index, way,  &cnt);
			if (cnt != 0)
				DBG_MSG(" 0x%2.2x  %1.1d\t0x%8.8x\n",
					index, way, cnt);
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_hits_dump);

/*----------------------------------------------------------------------*/
int mv_pp2x_cls_sw_lkp_dump(struct mv_pp2x_cls_lookup_entry *lkp)
{
	int int32bit;
	int status = 0;

	if (mv_pp2x_ptr_validate(lkp) == MV_ERROR)
		return MV_ERROR;

	DBG_MSG("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE\n");

	/* id */
	DBG_MSG(" 0x%2.2x  %1.1d\t", lkp->lkpid, lkp->way);

	/*rxq*/
	status |= mv_pp2x_cls_sw_lkp_rxq_get(lkp, &int32bit);
	DBG_MSG("0x%2.2x\t", int32bit);

	/*enabe bit*/
	status |= mv_pp2x_cls_sw_lkp_en_get(lkp, &int32bit);
	DBG_MSG("%1.1d\t", int32bit);

	/*flow*/
	status |= mv_pp2x_cls_sw_lkp_flow_get(lkp, &int32bit);
	DBG_MSG("0x%3.3x\t", int32bit);

	/*mode*/
	status |= mv_pp2x_cls_sw_lkp_mod_get(lkp, &int32bit);
	DBG_MSG(" 0x%2.2x\t", int32bit);

	DBG_MSG("\n");

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_sw_lkp_dump);

int mv_pp2x_cls_hw_lkp_dump(struct mv_pp2x_hw *hw)
{
	int index, way, int32bit, ind;
	unsigned int uint32bit;

	struct mv_pp2x_cls_lookup_entry lkp;

	DBG_MSG("< ID  WAY >:	RXQ	EN	FLOW	MODE_BASE  HITS\n");
	for (index = 0; index < MVPP2_CLS_LKP_TBL_SIZE ; index++)
		for (way = 0; way < 2 ; way++)	{
			ind = (way << MVPP2_CLS_LKP_INDEX_WAY_OFFS) | index;
			mv_pp2x_cls_hw_lkp_read(hw, index, way, &lkp);
			DBG_MSG(" 0x%2.2x  %1.1d\t", lkp.lkpid, lkp.way);
			mv_pp2x_cls_sw_lkp_rxq_get(&lkp, &int32bit);
			DBG_MSG("0x%2.2x\t", int32bit);
			mv_pp2x_cls_sw_lkp_en_get(&lkp, &int32bit);
			DBG_MSG("%1.1d\t", int32bit);
			mv_pp2x_cls_sw_lkp_flow_get(&lkp, &int32bit);
			DBG_MSG("0x%3.3x\t", int32bit);
			mv_pp2x_cls_sw_lkp_mod_get(&lkp, &int32bit);
			DBG_MSG(" 0x%2.2x\t", int32bit);
			mv_pp2x_cls_hw_lkp_hit_get(hw, index, way, &uint32bit);
			DBG_MSG(" 0x%8.8x\n", uint32bit);
			DBG_MSG("\n");

		}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_hw_lkp_dump);

/*----------------------------------------------------------------------*/
/*	Classifier C2 engine QoS table Public APIs			*/
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_qos_hw_read(struct mv_pp2x_hw *hw, int tbl_id,
			       int tbl_sel, int tbl_line,
			       struct mv_pp2x_cls_c2_qos_entry *qos)
{
	unsigned int regVal = 0;

	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(tbl_sel, 0, 1) == MV_ERROR) /* one bit */
		return MV_ERROR;

	if (tbl_sel == 1) {
		/*dscp*/
		/* TODO define 8=DSCP_TBL_NUM  64=DSCP_TBL_LINES */
		if (mv_pp2x_range_validate(tbl_id, 0,
		    MVPP2_QOS_TBL_NUM_DSCP) == MV_ERROR)
			return MV_ERROR;
		if (mv_pp2x_range_validate(tbl_line, 0,
		    MVPP2_QOS_TBL_LINE_NUM_DSCP) == MV_ERROR)
			return MV_ERROR;
	} else {
		/*pri*/
		/* TODO define 64=PRI_TBL_NUM  8=PRI_TBL_LINES */
		if (mv_pp2x_range_validate(tbl_id, 0,
		    MVPP2_QOS_TBL_NUM_PRI) == MV_ERROR)
			return MV_ERROR;
		if (mv_pp2x_range_validate(tbl_line, 0,
		    MVPP2_QOS_TBL_LINE_NUM_PRI) == MV_ERROR)
			return MV_ERROR;
	}

	qos->tbl_id = tbl_id;
	qos->tbl_sel = tbl_sel;
	qos->tbl_line = tbl_line;

	/* write index reg */
	regVal |= (tbl_line << MVPP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |= (tbl_sel << MVPP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (tbl_id << MVPP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);

	mv_pp2x_write(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG, regVal);

	/* read data reg*/
	qos->data = mv_pp2x_read(hw, MVPP2_CLS2_QOS_TBL_REG);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_hw_read);

/*----------------------------------------------------------------------*/

int mvPp2ClsC2QosPrioGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *prio)
{
	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(prio) == MV_ERROR)
		return MV_ERROR;

	*prio = (qos->data & MVPP2_CLS2_QOS_TBL_PRI_MASK) >>
		MVPP2_CLS2_QOS_TBL_PRI_OFF;
	return MV_OK;
}
EXPORT_SYMBOL(mvPp2ClsC2QosPrioGet);

/*----------------------------------------------------------------------*/

int mvPp2ClsC2QosDscpGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *dscp)
{
	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(dscp) == MV_ERROR)
		return MV_ERROR;

	*dscp = (qos->data & MVPP2_CLS2_QOS_TBL_DSCP_MASK) >>
		MVPP2_CLS2_QOS_TBL_DSCP_OFF;
	return MV_OK;
}
EXPORT_SYMBOL(mvPp2ClsC2QosDscpGet);

/*----------------------------------------------------------------------*/

int mvPp2ClsC2QosColorGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *color)
{
	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(color) == MV_ERROR)
		return MV_ERROR;

	*color = (qos->data & MVPP2_CLS2_QOS_TBL_COLOR_MASK) >>
		MVPP2_CLS2_QOS_TBL_COLOR_OFF;
	return MV_OK;
}
EXPORT_SYMBOL(mvPp2ClsC2QosColorGet);

/*----------------------------------------------------------------------*/

int mvPp2ClsC2QosGpidGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *gpid)
{
	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(gpid) == MV_ERROR)
		return MV_ERROR;

	*gpid = (qos->data & MVPP2_CLS2_QOS_TBL_GEMPORT_MASK) >>
		MVPP2_CLS2_QOS_TBL_GEMPORT_OFF;
	return MV_OK;
}
EXPORT_SYMBOL(mvPp2ClsC2QosGpidGet);

/*----------------------------------------------------------------------*/

int mvPp2ClsC2QosQueueGet(struct mv_pp2x_cls_c2_qos_entry *qos, int *queue)
{
	if (mv_pp2x_ptr_validate(qos) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(queue) == MV_ERROR)
		return MV_ERROR;

	*queue = (qos->data & MVPP2_CLS2_QOS_TBL_QUEUENUM_MASK) >>
		MVPP2_CLS2_QOS_TBL_QUEUENUM_OFF;
	return MV_OK;
}
EXPORT_SYMBOL(mvPp2ClsC2QosQueueGet);

/*----------------------------------------------------------------------*/
/*	Classifier C2 engine TCAM table Public APIs			*/
/*----------------------------------------------------------------------*/

/* note: error is not returned if entry is invalid
 * user should check c2->valid afer returned from this func
 */
int mv_pp2x_cls_c2_hw_read(struct mv_pp2x_hw *hw, int index,
			   struct mv_pp2x_cls_c2_entry *c2)
{
	unsigned int regVal;
	int	TcmIdx;

	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	c2->index = index;

	/* write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, index);

	/* read inValid bit*/
	regVal = mv_pp2x_read(hw, MVPP2_CLS2_TCAM_INV_REG);
	c2->inv = (regVal & MVPP2_CLS2_TCAM_INV_INVALID_MASK) >>
		MVPP2_CLS2_TCAM_INV_INVALID_OFF;

	if (c2->inv)
		return MV_OK;

	for (TcmIdx = 0; TcmIdx < MVPP2_CLS_C2_TCAM_WORDS; TcmIdx++)
		c2->tcam.words[TcmIdx] = mv_pp2x_read(hw,
					MVPP2_CLS2_TCAM_DATA_REG(TcmIdx));

	/* read action_tbl 0x1B30 */
	c2->sram.regs.action_tbl = mv_pp2x_read(hw, MVPP2_CLS2_ACT_DATA_REG);

	/* read actions 0x1B60 */
	c2->sram.regs.actions = mv_pp2x_read(hw, MVPP2_CLS2_ACT_REG);

	/* read qos_attr 0x1B64 */
	c2->sram.regs.qos_attr = mv_pp2x_read(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG);

	/* read hwf_attr 0x1B68 */
	c2->sram.regs.hwf_attr = mv_pp2x_read(hw, MVPP2_CLS2_ACT_HWF_ATTR_REG);

	/* read hwf_attr 0x1B6C */
	c2->sram.regs.rss_attr = mv_pp2x_read(hw, MVPP2_CLS2_ACT_DUP_ATTR_REG);

	/* read seq_attr 0x1B70 */
	c2->sram.regs.seq_attr = mv_pp2x_read(hw, MVPP22_CLS2_ACT_SEQ_ATTR_REG);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_read);
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_sw_words_dump(struct mv_pp2x_cls_c2_entry *c2)
{
	int i;

	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	/* TODO check size */
	/* hw entry id */
	DBG_MSG("[0x%3.3x] ", c2->index);

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		DBG_MSG("%4.4x ", (c2->tcam.words[i--]) & 0xFFFF);

	DBG_MSG("| ");

	DBG_MSG("%8.8x %8.8x %8.8x %8.8x %8.8x", c2->sram.words[4],
		c2->sram.words[3], c2->sram.words[2], c2->sram.words[1],
		c2->sram.words[0]);

	/*tcam inValid bit*/
	DBG_MSG(" %s", (c2->inv == 1) ? "[inv]" : "[valid]");

	DBG_MSG("\n        ");

	i = MVPP2_CLS_C2_TCAM_WORDS - 1;

	while (i >= 0)
		DBG_MSG("%4.4x ", ((c2->tcam.words[i--] >> 16)  & 0xFFFF));

	DBG_MSG("\n");

	return MV_OK;
}

/*----------------------------------------------------------------------*/

int mvPp2ClsC2TcamByteGet(struct mv_pp2x_cls_c2_entry *c2,
			  unsigned int offs, unsigned char *byte,
			  unsigned char *enable)
{
	if (mv_pp2x_ptr_validate(c2) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(byte) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_ptr_validate(enable) == MV_ERROR)
		return MV_ERROR;

	if (mv_pp2x_range_validate(offs, 0, 8) == MV_ERROR)
		return MV_ERROR;

	*byte = c2->tcam.bytes[TCAM_DATA_BYTE(offs)];
	*enable = c2->tcam.bytes[TCAM_DATA_MASK(offs)];
	return MV_OK;
}

/*----------------------------------------------------------------------*/
/*	return EQUALS if tcam_data[off]&tcam_mask[off] = byte		*/
/*----------------------------------------------------------------------*/
/*	Classifier C2 engine Hit counters Public APIs			*/
/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_hit_cntr_is_busy(struct mv_pp2x_hw *hw)
{
	unsigned int regVal;

	regVal = mv_pp2x_read(hw, MVPP2_CLS2_HIT_CTR_REG);
	regVal &= MVPP2_CLS2_HIT_CTR_CLR_DONE_MASK;
	regVal >>= MVPP2_CLS2_HIT_CTR_CLR_DONE_OFF;

	return (1 - (int)regVal);
}

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_hit_cntr_clear_all(struct mv_pp2x_hw *hw)
{
	int iter = 0;

	/* wrirte clear bit*/
	mv_pp2x_write(hw, MVPP2_CLS2_HIT_CTR_CLR_REG,
		(1 << MVPP2_CLS2_HIT_CTR_CLR_CLR_OFF));

	while (mv_pp2x_cls_c2_hit_cntr_is_busy(hw))
		if (iter++ >= RETRIES_EXCEEDED) {
			DBG_MSG("%s:Error - retries exceeded.\n", __func__);
			return MV_ERROR;
		}

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hit_cntr_clear_all);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_hit_cntr_read(struct mv_pp2x_hw *hw, int index, u32 *cntr)
{
	unsigned int value = 0;

	/* write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, index);

	value = mv_pp2x_read(hw, MVPP2_CLS2_HIT_CTR_REG);

	if (cntr)
		*cntr = value;
	else
		DBG_MSG("INDEX: 0x%8.8X	VAL: 0x%8.8X\n", index, value);

	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hit_cntr_read);


/*----------------------------------------------------------------------*/
int mv_pp2x_cls_c2_hit_cntr_dump(struct mv_pp2x_hw *hw)
{
	int i;
	unsigned int cnt;

	for (i = 0; i < MVPP2_CLS_C2_TCAM_SIZE; i++) {
		mv_pp2x_cls_c2_hit_cntr_read(hw, i, &cnt);
		if (cnt != 0)
			DBG_MSG("INDEX: 0x%8.8X	VAL: 0x%8.8X\n", i, cnt);
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hit_cntr_dump);

/*----------------------------------------------------------------------*/

int mv_pp2x_cls_c2_regs_dump(struct mv_pp2x_hw *hw)
{
	int i;
	char reg_name[100];

	mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_IDX_REG,
			  "MVPP2_CLS2_TCAM_IDX_REG");

	for (i = 0; i < MVPP2_CLS_C2_TCAM_WORDS; i++) {
		printk(reg_name, "MVPP2_CLS2_TCAM_DATA_%d_REG", i);
		mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_DATA_REG(i), reg_name);
	}

	mv_pp2x_print_reg(hw, MVPP2_CLS2_TCAM_INV_REG,
			  "MVPP2_CLS2_TCAM_INV_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_DATA_REG,
			  "MVPP2_CLS2_ACT_DATA_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG,
			  "MVPP2_CLS2_DSCP_PRI_INDEX_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_QOS_TBL_REG,
			  "MVPP2_CLS2_QOS_TBL_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_REG,
			  "MVPP2_CLS2_ACT_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG,
			  "MVPP2_CLS2_ACT_QOS_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_HWF_ATTR_REG,
			  "MVPP2_CLS2_ACT_HWF_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP2_CLS2_ACT_DUP_ATTR_REG,
			  "MVPP2_CLS2_ACT_DUP_ATTR_REG");
	mv_pp2x_print_reg(hw, MVPP22_CLS2_ACT_SEQ_ATTR_REG,
			  "MVPP22_CLS2_ACT_SEQ_ATTR_REG");
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_regs_dump);

void mv_pp2x_cls_flow_port_add(struct mv_pp2x_hw *hw, int index, int port_id)
{
	u32 data;

	/* Write flow index */
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_INDEX_REG, index);
	/* Read first data with port info */
	data = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL0_REG);
	/* Add the port */
	data |= ((1 << port_id) << MVPP2_FLOW_PORT_ID);
	/* Update the register */
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_TBL0_REG, data);
}

void mv_pp2x_cls_flow_port_del(struct mv_pp2x_hw *hw, int index, int port_id)
{
	u32 data;

	/* Write flow index */
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_INDEX_REG, index);
	/* Read first data with port info */
	data = mv_pp2x_read(hw, MVPP2_CLS_FLOW_TBL0_REG);
	/* Delete the port */
	data &= ~(((1 << port_id) << MVPP2_FLOW_PORT_ID));
	/* Update the register */
	mv_pp2x_write(hw, MVPP2_CLS_FLOW_TBL0_REG, data);
}

/* The function prepare a temporary flow table for lkpid flow,
 * in order to change the original one
 */
void mv_pp2x_cls_flow_tbl_temp_copy(struct mv_pp2x_hw *hw, int lkpid,
				    int *temp_flow_idx)
{
	struct mv_pp2x_cls_flow_entry fe;
	int index = lkpid - MVPP2_PRS_FL_START;
	int flow_start = hw->cls_shadow->flow_free_start;
	struct mv_pp2x_cls_flow_info *flow_info;

	flow_info = &(hw->cls_shadow->flow_info[index]);

	if (flow_info->flow_entry_dflt) {
		mv_pp2x_cls_flow_read(hw, flow_info->flow_entry_dflt, &fe);
		fe.index = flow_start++;
		mv_pp2x_cls_flow_write(hw, &fe);
	}
	if (flow_info->flow_entry_vlan) {
		mv_pp2x_cls_flow_read(hw, flow_info->flow_entry_vlan, &fe);
		fe.index = flow_start++;
		mv_pp2x_cls_flow_write(hw, &fe);
	}
	if (flow_info->flow_entry_dscp) {
		mv_pp2x_cls_flow_read(hw, flow_info->flow_entry_dscp, &fe);
		fe.index = flow_start++;
		mv_pp2x_cls_flow_write(hw, &fe);
	}
	if (flow_info->flow_entry_rss1) {
		mv_pp2x_cls_flow_read(hw, flow_info->flow_entry_rss1, &fe);
		fe.index = flow_start++;
		mv_pp2x_cls_flow_write(hw, &fe);
	}
	if (flow_info->flow_entry_rss2) {
		mv_pp2x_cls_flow_read(hw, flow_info->flow_entry_rss2, &fe);
		fe.index = flow_start++;
		mv_pp2x_cls_flow_write(hw, &fe);
	}

	*temp_flow_idx = hw->cls_shadow->flow_free_start;
}

/* C2 rule and Qos table */
int mv_pp2x_cls_c2_hw_write(struct mv_pp2x_hw *hw, int index,
			    struct mv_pp2x_cls_c2_entry *c2)
{
	int TcmIdx;

	if (!c2 || index >= MVPP2_CLS_C2_TCAM_SIZE)
		return -EINVAL;

	c2->index = index;

	/* write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, index);

	/* write valid bit*/
	c2->inv = 0;
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_INV_REG,
		((c2->inv) << MVPP2_CLS2_TCAM_INV_INVALID_OFF));

	for (TcmIdx = 0; TcmIdx < MVPP2_CLS_C2_TCAM_WORDS; TcmIdx++)
		mv_pp2x_write(hw, MVPP2_CLS2_TCAM_DATA_REG(TcmIdx),
			c2->tcam.words[TcmIdx]);

	/* write action_tbl CLSC2_ACT_DATA */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_DATA_REG, c2->sram.regs.action_tbl);

	/* write actions CLSC2_ACT */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_REG, c2->sram.regs.actions);

	/* write qos_attr CLSC2_ATTR0 */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG, c2->sram.regs.qos_attr);

	/* write hwf_attr CLSC2_ATTR1 */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_HWF_ATTR_REG, c2->sram.regs.hwf_attr);

	/* write rss_attr CLSC2_ATTR2 */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_DUP_ATTR_REG, c2->sram.regs.rss_attr);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_write);

int mv_pp2x_cls_c2_qos_hw_write(struct mv_pp2x_hw *hw,
				struct mv_pp2x_cls_c2_qos_entry *qos)
{
	unsigned int regVal = 0;

	if (!qos || qos->tbl_sel > MVPP2_QOS_TBL_SEL_DSCP)
		return -EINVAL;

	if (qos->tbl_sel == MVPP2_QOS_TBL_SEL_DSCP) {
		/*dscp*/
		if (qos->tbl_id >=  MVPP2_QOS_TBL_NUM_DSCP ||
			qos->tbl_line >= MVPP2_QOS_TBL_LINE_NUM_DSCP)
			return -EINVAL;
	} else {
		/*pri*/
		if (qos->tbl_id >=  MVPP2_QOS_TBL_NUM_PRI ||
			qos->tbl_line >= MVPP2_QOS_TBL_LINE_NUM_PRI)
			return -EINVAL;
	}
	/* write index reg */
	regVal |= (qos->tbl_line << MVPP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |= (qos->tbl_sel << MVPP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (qos->tbl_id << MVPP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);
	mv_pp2x_write(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG, regVal);

	/* write data reg*/
	mv_pp2x_write(hw, MVPP2_CLS2_QOS_TBL_REG, qos->data);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_hw_write);

int mv_pp2x_cls_c2_hw_inv(struct mv_pp2x_hw *hw, int index)
{
	if (!hw || index >= MVPP2_CLS_C2_TCAM_SIZE)
		return -EINVAL;

	/* write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, index);

	/* set invalid bit*/
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_INV_REG, (1 <<
		MVPP2_CLS2_TCAM_INV_INVALID_OFF));

	/* trigger */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_DATA_REG(4), 0);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_inv);

void mv_pp2x_cls_c2_hw_inv_all(struct mv_pp2x_hw *hw)
{
	int index;

	for (index = 0; index < MVPP2_CLS_C2_TCAM_SIZE; index++) {
		PALAD(MVPP2_PRINT_LINE());
		mv_pp2x_cls_c2_hw_inv(hw, index);
	}
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_hw_inv_all);

static void mv_pp2x_cls_c2_qos_hw_clear_all(struct mv_pp2x_hw *hw)
{
	struct mv_pp2x_cls_c2_qos_entry qos;

	memset(&qos, 0, sizeof(struct mv_pp2x_cls_c2_qos_entry));

	/* clear DSCP tables */
	qos.tbl_sel = MVPP2_QOS_TBL_SEL_DSCP;
	for (qos.tbl_id = 0; qos.tbl_id < MVPP2_QOS_TBL_NUM_DSCP;
		qos.tbl_id++) {
		for (qos.tbl_line = 0; qos.tbl_line <
			MVPP2_QOS_TBL_LINE_NUM_DSCP; qos.tbl_line++) {
			mv_pp2x_cls_c2_qos_hw_write(hw, &qos);
			PALAD(MVPP2_PRINT_LINE());
		}
	}

	/* clear PRIO tables */
	qos.tbl_sel = MVPP2_QOS_TBL_SEL_PRI;
	for (qos.tbl_id = 0; qos.tbl_id <
		MVPP2_QOS_TBL_NUM_PRI; qos.tbl_id++)
		for (qos.tbl_line = 0; qos.tbl_line <
			MVPP2_QOS_TBL_LINE_NUM_PRI; qos.tbl_line++) {
			PALAD(MVPP2_PRINT_LINE());
			mv_pp2x_cls_c2_qos_hw_write(hw, &qos);
		}
}

int mv_pp2x_cls_c2_qos_tbl_set(struct mv_pp2x_cls_c2_entry *c2,
			       int tbl_id, int tbl_sel)
{
	if (!c2 || tbl_sel > 1)
		return -EINVAL;

	if (tbl_sel == 1) {
		/*dscp*/
		if (tbl_id >= MVPP2_QOS_TBL_NUM_DSCP)
			return -EINVAL;
	} else {
		/*pri*/
		if (tbl_id >= MVPP2_QOS_TBL_NUM_PRI)
			return -EINVAL;
	}
	c2->sram.regs.action_tbl = (tbl_id <<
			MVPP2_CLS2_ACT_DATA_TBL_ID_OFF) |
			(tbl_sel << MVPP2_CLS2_ACT_DATA_TBL_SEL_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_tbl_set);

int mv_pp2x_cls_c2_color_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
			     int from)
{
	if (!c2 || cmd > MVPP2_COLOR_ACTION_TYPE_RED_LOCK)
		return -EINVAL;

	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_COLOR_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_COLOR_OFF);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 <<
			MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF);
	else
		c2->sram.regs.action_tbl &= ~(1 <<
			MVPP2_CLS2_ACT_DATA_TBL_COLOR_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_color_set);

int mv_pp2x_cls_c2_prio_set(struct mv_pp2x_cls_c2_entry *c2, int cmd,
			    int prio, int from)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK ||
			prio >= MVPP2_QOS_TBL_LINE_NUM_PRI)
		return -EINVAL;

	/*set command*/
	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_PRI_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_PRI_OFF);

	/*set modify priority value*/
	c2->sram.regs.qos_attr &= ~MVPP2_CLS2_ACT_QOS_ATTR_PRI_MASK;
	c2->sram.regs.qos_attr |= ((prio << MVPP2_CLS2_ACT_QOS_ATTR_PRI_OFF) &
		MVPP2_CLS2_ACT_QOS_ATTR_PRI_MASK);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 <<
			MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);
	else
		c2->sram.regs.action_tbl &= ~(1 <<
			MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_prio_set);

int mv_pp2x_cls_c2_dscp_set(struct mv_pp2x_cls_c2_entry *c2,
			    int cmd, int dscp, int from)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK ||
			dscp >= MVPP2_QOS_TBL_LINE_NUM_DSCP)
		return -EINVAL;

	/*set command*/
	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_DSCP_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_DSCP_OFF);

	/*set modify DSCP value*/
	c2->sram.regs.qos_attr &= ~MVPP2_CLS2_ACT_QOS_ATTR_DSCP_MASK;
	c2->sram.regs.qos_attr |= ((dscp <<
		MVPP2_CLS2_ACT_QOS_ATTR_DSCP_OFF) &
		MVPP2_CLS2_ACT_QOS_ATTR_DSCP_MASK);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 <<
			MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);
	else
		c2->sram.regs.action_tbl &= ~(1 <<
			MVPP2_CLS2_ACT_DATA_TBL_PRI_DSCP_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_dscp_set);

int mv_pp2x_cls_c2_queue_low_set(struct mv_pp2x_cls_c2_entry *c2,
				 int cmd, int queue, int from)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK ||
			queue >= (1 << MVPP2_CLS2_ACT_QOS_ATTR_QL_BITS))
		return -EINVAL;

	/*set command*/
	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_QL_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_QL_OFF);

	/*set modify Low queue value*/
	c2->sram.regs.qos_attr &= ~MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK;
	c2->sram.regs.qos_attr |= ((queue <<
			MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF) &
			MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 <<
			MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF);
	else
		c2->sram.regs.action_tbl &= ~(1 <<
			MVPP2_CLS2_ACT_DATA_TBL_LOW_Q_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_queue_low_set);

int mv_pp2x_cls_c2_queue_high_set(struct mv_pp2x_cls_c2_entry *c2,
				  int cmd, int queue, int from)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK ||
			queue >= (1 << MVPP2_CLS2_ACT_QOS_ATTR_QH_BITS))
		return -EINVAL;

	/*set command*/
	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_QH_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_QH_OFF);

	/*set modify High queue value*/
	c2->sram.regs.qos_attr &= ~MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK;
	c2->sram.regs.qos_attr |= ((queue <<
			MVPP2_CLS2_ACT_QOS_ATTR_QH_OFF) &
			MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK);

	if (from == 1)
		c2->sram.regs.action_tbl |= (1 <<
			MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF);
	else
		c2->sram.regs.action_tbl &= ~(1 <<
			MVPP2_CLS2_ACT_DATA_TBL_HIGH_Q_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_queue_high_set);

int mv_pp2x_cls_c2_forward_set(struct mv_pp2x_cls_c2_entry *c2, int cmd)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK)
		return -EINVAL;

	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_FRWD_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_FRWD_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_forward_set);

int mv_pp2x_cls_c2_rss_set(struct mv_pp2x_cls_c2_entry *c2, int cmd, int rss_en)
{
	if (!c2 || cmd > MVPP2_ACTION_TYPE_UPDT_LOCK || rss_en >=
			(1 << MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_BITS))
		return -EINVAL;

	c2->sram.regs.actions &= ~MVPP2_CLS2_ACT_RSS_MASK;
	c2->sram.regs.actions |= (cmd << MVPP2_CLS2_ACT_RSS_OFF);

	c2->sram.regs.rss_attr &= ~MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK;
	c2->sram.regs.rss_attr |= (rss_en <<
			MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_rss_set);

int mv_pp2x_cls_c2_flow_id_en(struct mv_pp2x_cls_c2_entry *c2, int flowid_en)
{
	if (!c2)
		return -EINVAL;

	/*set Flow ID enable or disable*/
	if (flowid_en)
		c2->sram.regs.actions |= (1 << MVPP2_CLS2_ACT_FLD_EN_OFF);
	else
		c2->sram.regs.actions &= ~(1 << MVPP2_CLS2_ACT_FLD_EN_OFF);

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_flow_id_en);

int mv_pp2x_cls_c2_tcam_byte_set(struct mv_pp2x_cls_c2_entry *c2,
				 unsigned int offs, unsigned char byte,
				 unsigned char enable)
{
	if (!c2 || offs >= MVPP2_CLS_C2_TCAM_DATA_BYTES)
		return -EINVAL;

	c2->tcam.bytes[TCAM_DATA_BYTE(offs)] = byte;
	c2->tcam.bytes[TCAM_DATA_MASK(offs)] = enable;

	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_tcam_byte_set);

int mv_pp2x_cls_c2_qos_queue_set(struct mv_pp2x_cls_c2_qos_entry *qos,
				 u8 queue)
{
	if (!qos || queue >= (1 << MVPP2_CLS2_QOS_TBL_QUEUENUM_BITS))
		return -EINVAL;

	qos->data &= ~MVPP2_CLS2_QOS_TBL_QUEUENUM_MASK;
	qos->data |= (((u32)queue) << MVPP2_CLS2_QOS_TBL_QUEUENUM_OFF);
	return 0;
}
EXPORT_SYMBOL(mv_pp2x_cls_c2_qos_queue_set);

static int mv_pp2x_c2_tcam_set(struct mv_pp2x_hw *hw,
			       struct mv_pp2x_c2_add_entry *c2_add_entry,
			       unsigned int c2_hw_idx)
{
	int ret_code;
	struct mv_pp2x_cls_c2_entry c2_entry;
	int hek_offs;
	unsigned char hek_byte[MVPP2_CLS_C2_HEK_OFF_MAX],
		      hek_byte_mask[MVPP2_CLS_C2_HEK_OFF_MAX];

	if (!c2_add_entry || !hw || c2_hw_idx >= MVPP2_CLS_C2_TCAM_SIZE)
		return -EINVAL;

	/* Clear C2 sw data */
	memset(&c2_entry, 0, sizeof(struct mv_pp2x_cls_c2_entry));

	/* Set QOS table, selection and ID */
	ret_code = mv_pp2x_cls_c2_qos_tbl_set(&c2_entry,
					c2_add_entry->qos_info.qos_tbl_index,
					c2_add_entry->qos_info.qos_tbl_type);
	if (ret_code)
		return ret_code;

	/* Set color, cmd and source */
	ret_code = mv_pp2x_cls_c2_color_set(&c2_entry,
					    c2_add_entry->action.color_act,
					    c2_add_entry->qos_info.color_src);
	if (ret_code)
		return ret_code;

	/* Set priority(pbit), cmd, value(not from qos table) and source */
	ret_code = mv_pp2x_cls_c2_prio_set(&c2_entry,
					   c2_add_entry->action.pri_act,
					   c2_add_entry->qos_value.pri,
					   c2_add_entry->qos_info.pri_dscp_src);
	if (ret_code)
		return ret_code;

	/* Set DSCP, cmd, value(not from qos table) and source */
	ret_code = mv_pp2x_cls_c2_dscp_set(&c2_entry,
					   c2_add_entry->action.dscp_act,
					   c2_add_entry->qos_value.dscp,
					   c2_add_entry->qos_info.pri_dscp_src);
	if (ret_code)
		return ret_code;

	/* Set queue low, cmd, value, and source */
	ret_code = mv_pp2x_cls_c2_queue_low_set(&c2_entry,
						c2_add_entry->action.q_low_act,
						c2_add_entry->qos_value.q_low,
					     c2_add_entry->qos_info.q_low_src);
	if (ret_code)
		return ret_code;

	/* Set queue high, cmd, value and source */
	ret_code = mv_pp2x_cls_c2_queue_high_set(&c2_entry,
					c2_add_entry->action.q_high_act,
					c2_add_entry->qos_value.q_high,
					c2_add_entry->qos_info.q_high_src);
	if (ret_code)
		return ret_code;

	/* Set forward */
	ret_code = mv_pp2x_cls_c2_forward_set(&c2_entry,
					      c2_add_entry->action.frwd_act);
	if (ret_code)
		return ret_code;

	/* Set RSS */
	ret_code = mv_pp2x_cls_c2_rss_set(&c2_entry,
					  c2_add_entry->action.rss_act,
					  c2_add_entry->rss_en);
	if (ret_code)
		return ret_code;

	/* Set flowID(not for multicast) */
	ret_code = mv_pp2x_cls_c2_flow_id_en(&c2_entry,
					     c2_add_entry->action.flowid_act);
	if (ret_code)
		return ret_code;

	/* Set C2 HEK */
	memset(hek_byte, 0, MVPP2_CLS_C2_HEK_OFF_MAX);
	memset(hek_byte_mask, 0, MVPP2_CLS_C2_HEK_OFF_MAX);

	/* HEK offs 8, lookup type, port type */
	hek_byte[MVPP2_CLS_C2_HEK_OFF_LKP_PORT_TYPE] =
		(c2_add_entry->port.port_type <<
			MVPP2_CLS_C2_HEK_PORT_TYPE_OFFS) |
		(c2_add_entry->lkp_type <<
			MVPP2_CLS_C2_HEK_LKP_TYPE_OFFS);
	hek_byte_mask[MVPP2_CLS_C2_HEK_OFF_LKP_PORT_TYPE] =
			MVPP2_CLS_C2_HEK_PORT_TYPE_MASK |
			((c2_add_entry->lkp_type_mask <<
				MVPP2_CLS_C2_HEK_LKP_TYPE_OFFS) &
				MVPP2_CLS_C2_HEK_LKP_TYPE_MASK);
	/* HEK offs 9, port ID */
	hek_byte[MVPP2_CLS_C2_HEK_OFF_PORT_ID] =
		c2_add_entry->port.port_value;
	hek_byte_mask[MVPP2_CLS_C2_HEK_OFF_PORT_ID] =
		c2_add_entry->port.port_mask;

	for (hek_offs = MVPP2_CLS_C2_HEK_OFF_PORT_ID; hek_offs >=
			MVPP2_CLS_C2_HEK_OFF_BYTE0; hek_offs--) {
		ret_code = mv_pp2x_cls_c2_tcam_byte_set(&c2_entry,
							hek_offs,
							hek_byte[hek_offs],
						hek_byte_mask[hek_offs]);
		if (ret_code)
			return ret_code;
	}

	/* Write C2 entry data to HW */
	ret_code = mv_pp2x_cls_c2_hw_write(hw, c2_hw_idx, &c2_entry);
	if (ret_code)
		return ret_code;

	return 0;
}

/* C2 TCAM init */
int mv_pp2x_c2_init(struct platform_device *pdev, struct mv_pp2x_hw *hw)
{
	int i;

	/* Invalid all C2 and QoS entries */
	mv_pp2x_cls_c2_hw_inv_all(hw);
	PALAD(MVPP2_PRINT_LINE());

	mv_pp2x_cls_c2_qos_hw_clear_all(hw);
	PALAD(MVPP2_PRINT_LINE());

	/* Set CLSC2_TCAM_CTRL to enable C2, or C2 does not work */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_CTRL_REG,
		      MVPP2_CLS2_TCAM_CTRL_EN_MASK);
	PALAD(MVPP2_PRINT_LINE());

	/* Allocate mem for c2 shadow */
	hw->c2_shadow = devm_kcalloc(&pdev->dev, 1,
				      sizeof(struct mv_pp2x_c2_shadow),
				      GFP_KERNEL);
	if (!hw->c2_shadow)
		return -ENOMEM;
	PALAD(MVPP2_PRINT_LINE());

	/* Init the rule idx to invalid value */
	for (i = 0; i < 8; i++) {
		hw->c2_shadow->rule_idx_info[i].vlan_pri_idx =
			MVPP2_CLS_C2_TCAM_SIZE;
		hw->c2_shadow->rule_idx_info[i].dscp_pri_idx =
			MVPP2_CLS_C2_TCAM_SIZE;
		hw->c2_shadow->rule_idx_info[i].default_rule_idx =
			MVPP2_CLS_C2_TCAM_SIZE;
	}
	hw->c2_shadow->c2_tcam_free_start = 0;
	PALAD(MVPP2_PRINT_LINE());

	return 0;
}

static int mv_pp2x_c2_rule_add(struct mv_pp2x_port *port,
			       struct mv_pp2x_c2_add_entry *c2_add_entry)
{
	int ret, lkp_type, c2_index = 0;
	bool first_free_update = false;
	struct mv_pp2x_c2_rule_idx *rule_idx;

	rule_idx = &(port->priv->hw.c2_shadow->rule_idx_info[port->id]);

	if (!port || !c2_add_entry)
		return -EINVAL;

	lkp_type = c2_add_entry->lkp_type;
	/* Write rule in C2 TCAM */
	if (lkp_type == MVPP2_CLS_LKP_VLAN_PRI) {
		if (rule_idx->vlan_pri_idx == MVPP2_CLS_C2_TCAM_SIZE) {
			/* If the C2 rule is new, apply a free c2 rule index */
			c2_index =
				port->priv->hw.c2_shadow->c2_tcam_free_start;
			first_free_update = true;
		} else {
			/* If the C2 rule is exist one,
			* take the C2 index from shadow
			*/
			c2_index = rule_idx->vlan_pri_idx;
			first_free_update = false;
		}
	} else if (lkp_type == MVPP2_CLS_LKP_DSCP_PRI) {
		if (rule_idx->dscp_pri_idx == MVPP2_CLS_C2_TCAM_SIZE) {
			c2_index =
				port->priv->hw.c2_shadow->c2_tcam_free_start;
			first_free_update = true;
		} else {
			c2_index = rule_idx->dscp_pri_idx;
			first_free_update = false;
		}
	} else if (lkp_type == MVPP2_CLS_LKP_DEFAULT) {
		if (rule_idx->default_rule_idx == MVPP2_CLS_C2_TCAM_SIZE) {
			c2_index =
				port->priv->hw.c2_shadow->c2_tcam_free_start;
			first_free_update = true;
		} else {
			c2_index = rule_idx->default_rule_idx;
			first_free_update = false;
		}
	} else {
		return -EINVAL;
	}

	/* Write C2 TCAM HW */
	ret = mv_pp2x_c2_tcam_set(&port->priv->hw, c2_add_entry, c2_index);
	if (ret)
		return ret;

	/* Update first free rule */
	if (first_free_update)
		port->priv->hw.c2_shadow->c2_tcam_free_start++;

	/* Update shadow */
	if (lkp_type == MVPP2_CLS_LKP_VLAN_PRI)
		rule_idx->vlan_pri_idx = c2_index;
	else if (lkp_type == MVPP2_CLS_LKP_DSCP_PRI)
		rule_idx->dscp_pri_idx = c2_index;
	else if (lkp_type == MVPP2_CLS_LKP_DEFAULT)
		rule_idx->default_rule_idx = c2_index;

	return 0;
}

/* Fill the qos table with queue */
static void mv_pp2x_cls_c2_qos_tbl_fill(struct mv_pp2x_port *port,
					u8 tbl_sel, u8 start_queue)
{
	struct mv_pp2x_cls_c2_qos_entry qos_entry;
	u32 pri, line_num;
	u8 cos_value, cos_queue, queue;


	if (tbl_sel == MVPP2_QOS_TBL_SEL_PRI)
		line_num = MVPP2_QOS_TBL_LINE_NUM_PRI;
	else
		line_num = MVPP2_QOS_TBL_LINE_NUM_DSCP;

	memset(&qos_entry, 0, sizeof(struct mv_pp2x_cls_c2_qos_entry));
	qos_entry.tbl_id = port->id;
	qos_entry.tbl_sel = tbl_sel;

	/* Fill the QoS dscp/pbit table */
	for (pri = 0; pri < line_num; pri++) {
		/* cos_value equal to dscp/8 or pbit value */
		cos_value = ((tbl_sel == MVPP2_QOS_TBL_SEL_PRI) ?
			pri : (pri / 8));
		/* each nibble of pri_map stands for a cos-value,
		 * nibble value is the queue
		 */
		cos_queue = mv_pp2x_cosval_queue_map(port, cos_value);
		qos_entry.tbl_line = pri;
		/* map cos queue to physical queue */
		/* Physical queue contains 2 parts: port ID and CPU ID,
		 * CPU ID will be used in RSS
		 */
		queue = start_queue + cos_queue;
		mv_pp2x_cls_c2_qos_queue_set(&qos_entry, queue);
		PALAD(MVPP2_PRINT_LINE());
		mv_pp2x_cls_c2_qos_hw_write(&port->priv->hw, &qos_entry);
	}
}

/* C2 rule set */
int mv_pp2x_cls_c2_rule_set(struct mv_pp2x_port *port, u8 start_queue)
{
	struct mv_pp2x_c2_add_entry c2_init_entry;
	int ret;
	u8 cos_value, cos_queue, queue, lkp_type;

	/* QoS of pbit rule */
	for (lkp_type = MVPP2_CLS_LKP_VLAN_PRI; lkp_type <=
			MVPP2_CLS_LKP_DEFAULT; lkp_type++) {
		memset(&c2_init_entry, 0, sizeof(struct mv_pp2x_c2_add_entry));

		/* Port info */
		c2_init_entry.port.port_type = MVPP2_SRC_PORT_TYPE_PHY;
		c2_init_entry.port.port_value = (1 << port->id);
		c2_init_entry.port.port_mask = 0xff;
		/* Lookup type */
		c2_init_entry.lkp_type = lkp_type;
		c2_init_entry.lkp_type_mask = 0x3F;
		/* Action info */
		c2_init_entry.action.color_act =
				MVPP2_COLOR_ACTION_TYPE_NO_UPDT_LOCK;
		c2_init_entry.action.pri_act =
				MVPP2_ACTION_TYPE_NO_UPDT_LOCK;
		c2_init_entry.action.dscp_act =
				MVPP2_ACTION_TYPE_NO_UPDT_LOCK;
		c2_init_entry.action.q_low_act =
				MVPP2_ACTION_TYPE_UPDT_LOCK;
		c2_init_entry.action.q_high_act =
				MVPP2_ACTION_TYPE_UPDT_LOCK;
		if (port->priv->pp2_version == PPV22)
			c2_init_entry.action.rss_act =
				MVPP2_ACTION_TYPE_UPDT_LOCK;
		/* To CPU */
		c2_init_entry.action.frwd_act =
				MVPP2_FRWD_ACTION_TYPE_SWF_LOCK;

		/* QoS info */
		if (lkp_type != MVPP2_CLS_LKP_DEFAULT) {
			/* QoS info from C2 QoS table */
			/* Set the QoS table index equal to port ID */
			c2_init_entry.qos_info.qos_tbl_index = port->id;
			c2_init_entry.qos_info.q_low_src =
					MVPP2_QOS_SRC_DSCP_PBIT_TBL;
			c2_init_entry.qos_info.q_high_src =
					MVPP2_QOS_SRC_DSCP_PBIT_TBL;
			if (lkp_type == MVPP2_CLS_LKP_VLAN_PRI) {
				c2_init_entry.qos_info.qos_tbl_type =
					MVPP2_QOS_TBL_SEL_PRI;
				mv_pp2x_cls_c2_qos_tbl_fill(port,
							MVPP2_QOS_TBL_SEL_PRI,
							start_queue);
			} else if (lkp_type == MVPP2_CLS_LKP_DSCP_PRI) {
				c2_init_entry.qos_info.qos_tbl_type =
					MVPP2_QOS_TBL_SEL_DSCP;
				mv_pp2x_cls_c2_qos_tbl_fill(port,
							MVPP2_QOS_TBL_SEL_DSCP,
							start_queue);
			}
		} else {
			/* QoS info from C2 action table */
			c2_init_entry.qos_info.q_low_src =
					MVPP2_QOS_SRC_ACTION_TBL;
			c2_init_entry.qos_info.q_high_src =
					MVPP2_QOS_SRC_ACTION_TBL;
			cos_value = port->priv->pp2_cfg.cos_cfg.default_cos;
			cos_queue = mv_pp2x_cosval_queue_map(port, cos_value);
			/* map to physical queue */
			/* Physical queue contains 2 parts: port ID and CPU ID,
			 * CPU ID will be used in RSS
			 */
			queue = start_queue + cos_queue;
			c2_init_entry.qos_value.q_low = ((u16)queue) &
				((1 << MVPP2_CLS2_ACT_QOS_ATTR_QL_BITS) - 1);
			c2_init_entry.qos_value.q_high = ((u16)queue) >>
					MVPP2_CLS2_ACT_QOS_ATTR_QL_BITS;
		}
		/* RSS En in PP22 */
		c2_init_entry.rss_en = port->priv->pp2_cfg.rss_cfg.rss_en;

		/* Add rule to C2 TCAM */
		PALAD(MVPP2_PRINT_LINE());
		ret = mv_pp2x_c2_rule_add(port, &c2_init_entry);
		if (ret)
			return ret;
	}

	return 0;
}

/* The function get the queue in the C2 rule with input index */
u8 mv_pp2x_cls_c2_rule_queue_get(struct mv_pp2x_hw *hw, u32 rule_idx)
{
	u32 regVal;
	u8 queue;

	/* Write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, rule_idx);

	/* Read Reg CLSC2_ATTR0 */
	regVal = mv_pp2x_read(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG);
	queue = (regVal & (MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK |
			MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK)) >>
			MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF;
	return queue;
}

/* The function set the qos queue in one C2 rule */
void mv_pp2x_cls_c2_rule_queue_set(struct mv_pp2x_hw *hw, u32 rule_idx,
				   u8 queue)
{
	u32 regVal;

	/* Write index reg */
	mv_pp2x_write(hw, MVPP2_CLS2_TCAM_IDX_REG, rule_idx);

	/* Read Reg CLSC2_ATTR0 */
	regVal = mv_pp2x_read(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG);
	/* Update Value */
	regVal &= (~(MVPP2_CLS2_ACT_QOS_ATTR_QL_MASK |
			MVPP2_CLS2_ACT_QOS_ATTR_QH_MASK));
	regVal |= (((u32)queue) << MVPP2_CLS2_ACT_QOS_ATTR_QL_OFF);

	/* Write Reg CLSC2_ATTR0 */
	mv_pp2x_write(hw, MVPP2_CLS2_ACT_QOS_ATTR_REG, regVal);
}

/* The function get the queue in the pbit table entry */
u8 mv_pp2x_cls_c2_pbit_tbl_queue_get(struct mv_pp2x_hw *hw, u8 tbl_id,
				     u8 tbl_line)
{
	u8 queue;
	u32 regVal = 0;

	/* write index reg */
	regVal |= (tbl_line << MVPP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |= (MVPP2_QOS_TBL_SEL_PRI <<
			MVPP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (tbl_id << MVPP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);
	mv_pp2x_write(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG, regVal);
	/* Read Reg CLSC2_DSCP_PRI */
	regVal = mv_pp2x_read(hw, MVPP2_CLS2_QOS_TBL_REG);
	queue = (regVal &  MVPP2_CLS2_QOS_TBL_QUEUENUM_MASK) >>
			MVPP2_CLS2_QOS_TBL_QUEUENUM_OFF;

	return queue;
}

/* The function set the queue in the pbit table entry */
void mv_pp2x_cls_c2_pbit_tbl_queue_set(struct mv_pp2x_hw *hw,
				       u8 tbl_id, u8 tbl_line, u8 queue)
{
	u32 regVal = 0;

	/* write index reg */
	regVal |= (tbl_line << MVPP2_CLS2_DSCP_PRI_INDEX_LINE_OFF);
	regVal |=
		(MVPP2_QOS_TBL_SEL_PRI << MVPP2_CLS2_DSCP_PRI_INDEX_SEL_OFF);
	regVal |= (tbl_id << MVPP2_CLS2_DSCP_PRI_INDEX_TBL_ID_OFF);
	mv_pp2x_write(hw, MVPP2_CLS2_DSCP_PRI_INDEX_REG, regVal);

	/* Read Reg CLSC2_DSCP_PRI */
	regVal = mv_pp2x_read(hw, MVPP2_CLS2_QOS_TBL_REG);
	regVal &= (~MVPP2_CLS2_QOS_TBL_QUEUENUM_MASK);
	regVal |= (((u32)queue) << MVPP2_CLS2_QOS_TBL_QUEUENUM_OFF);

	/* Write Reg CLSC2_DSCP_PRI */
	mv_pp2x_write(hw, MVPP2_CLS2_QOS_TBL_REG, regVal);
}

/* RSS */
/* The function will set rss table entry */
int mv_pp22_rss_tbl_entry_set(struct mv_pp2x_hw *hw,
			      struct mv_pp22_rss_entry *rss)
{
	unsigned int regVal = 0;

	if (!rss || rss->sel > MVPP22_RSS_ACCESS_TBL)
		return -EINVAL;

	if (rss->sel == MVPP22_RSS_ACCESS_POINTER) {
		if (rss->u.pointer.rss_tbl_ptr >= MVPP22_RSS_TBL_NUM)
			return -EINVAL;
		/* Write index */
		regVal |= rss->u.pointer.rxq_idx <<
				MVPP22_RSS_IDX_RXQ_NUM_OFF;
		mv_pp2x_write(hw, MVPP22_RSS_IDX_REG, regVal);
		/* Write entry */
		regVal &= (~MVPP22_RSS_RXQ2RSS_TBL_POINT_MASK);
		regVal |= rss->u.pointer.rss_tbl_ptr <<
				MVPP22_RSS_RXQ2RSS_TBL_POINT_OFF;
		mv_pp2x_write(hw, MVPP22_RSS_RXQ2RSS_TBL_REG, regVal);
	} else if (rss->sel == MVPP22_RSS_ACCESS_TBL) {
		if (rss->u.entry.tbl_id >= MVPP22_RSS_TBL_NUM ||
		    rss->u.entry.tbl_line >= MVPP22_RSS_TBL_LINE_NUM ||
		    rss->u.entry.width >= MVPP22_RSS_WIDTH_MAX)
			return -EINVAL;
		/* Write index */
		regVal |= (rss->u.entry.tbl_line <<
				MVPP22_RSS_IDX_ENTRY_NUM_OFF |
			   rss->u.entry.tbl_id << MVPP22_RSS_IDX_TBL_NUM_OFF);
		mv_pp2x_write(hw, MVPP22_RSS_IDX_REG, regVal);
		/* Write entry */
		regVal &= (~MVPP22_RSS_TBL_ENTRY_MASK);
		regVal |= (rss->u.entry.rxq << MVPP22_RSS_TBL_ENTRY_OFF);
		mv_pp2x_write(hw, MVPP22_RSS_TBL_ENTRY_REG, regVal);
		regVal &= (~MVPP22_RSS_WIDTH_MASK);
		regVal |= (rss->u.entry.width << MVPP22_RSS_WIDTH_OFF);
		mv_pp2x_write(hw, MVPP22_RSS_WIDTH_REG, regVal);
	}
	return 0;
}

/* The function will get rss table entry */
int mv_pp22_rss_tbl_entry_get(struct mv_pp2x_hw *hw,
			      struct mv_pp22_rss_entry *rss)
{
	unsigned int regVal = 0;

	if (!rss || rss->sel > MVPP22_RSS_ACCESS_TBL)
		return -EINVAL;

	if (rss->sel == MVPP22_RSS_ACCESS_POINTER) {
		/* Read entry */
		rss->u.pointer.rss_tbl_ptr =
			mv_pp2x_read(hw, MVPP22_RSS_RXQ2RSS_TBL_REG) &
				     MVPP22_RSS_RXQ2RSS_TBL_POINT_MASK;
	} else if (rss->sel == MVPP22_RSS_ACCESS_TBL) {
		if (rss->u.entry.tbl_id >= MVPP22_RSS_TBL_NUM ||
		    rss->u.entry.tbl_line >= MVPP22_RSS_TBL_LINE_NUM)
			return -EINVAL;
		/* Read index */
		regVal |= (rss->u.entry.tbl_line <<
				MVPP22_RSS_IDX_ENTRY_NUM_OFF |
			   rss->u.entry.tbl_id <<
				MVPP22_RSS_IDX_TBL_NUM_OFF);
		mv_pp2x_write(hw, MVPP22_RSS_IDX_REG, regVal);
		/* Read entry */
		rss->u.entry.rxq = mv_pp2x_read(hw,
						MVPP22_RSS_TBL_ENTRY_REG) &
						MVPP22_RSS_TBL_ENTRY_MASK;
		rss->u.entry.width = mv_pp2x_read(hw,
						  MVPP22_RSS_WIDTH_REG) &
						  MVPP22_RSS_WIDTH_MASK;
	}
	return 0;
}

int mv_pp22_rss_hw_dump(struct mv_pp2x_hw *hw)
{
	int tbl_id, tbl_line;

	struct mv_pp22_rss_entry rss_entry;

	memset(&rss_entry, 0, sizeof(struct mv_pp22_rss_entry));

	rss_entry.sel = MVPP22_RSS_ACCESS_TBL;

	for (tbl_id = 0; tbl_id < MVPP22_RSS_TBL_NUM; tbl_id++) {
		DBG_MSG("\n-------- RSS TABLE %d-----------\n", tbl_id);
		DBG_MSG("HASH	QUEUE	WIDTH\n");

		for (tbl_line = 0; tbl_line < MVPP22_RSS_TBL_LINE_NUM;
			tbl_line++) {
			rss_entry.u.entry.tbl_id = tbl_id;
			rss_entry.u.entry.tbl_line = tbl_line;
			mv_pp22_rss_tbl_entry_get(hw, &rss_entry);
			DBG_MSG("0x%2.2x\t", rss_entry.u.entry.tbl_line);
			DBG_MSG("0x%2.2x\t", rss_entry.u.entry.rxq);
			DBG_MSG("0x%2.2x", rss_entry.u.entry.width);
			DBG_MSG("\n");
		}
	}
	return MV_OK;
}
EXPORT_SYMBOL(mv_pp22_rss_hw_dump);

/* The function allocate a rss table for each phisical rxq,
 * they have same cos priority
 */
int mv_pp22_rss_rxq_set(struct mv_pp2x_port *port, u32 cos_width)
{
	int rxq;
	struct mv_pp22_rss_entry rss_entry;
	int cos_mask = ((1 << cos_width) - 1);

	if (port->priv->pp2_version != PPV22)
		return 0;

	memset(&rss_entry, 0, sizeof(struct mv_pp22_rss_entry));

	rss_entry.sel = MVPP22_RSS_ACCESS_POINTER;

	for (rxq = 0; rxq < port->num_rx_queues; rxq++) {
		PALAD(MVPP2_PRINT_LINE());
		rss_entry.u.pointer.rxq_idx = port->rxqs[rxq]->id;
		rss_entry.u.pointer.rss_tbl_ptr =
				port->rxqs[rxq]->id & cos_mask;
		if (mv_pp22_rss_tbl_entry_set(&port->priv->hw, &rss_entry))
			return -1;
	}

	return 0;
}

void mv_pp22_rss_c2_enable(struct mv_pp2x_port *port, bool en)
{
	int lkp_type, regVal;
	int c2_index[MVPP2_CLS_LKP_MAX];
	struct mv_pp2x_c2_rule_idx *rule_idx;

	rule_idx = &(port->priv->hw.c2_shadow->rule_idx_info[port->id]);

	/* Get the C2 index from shadow */
	c2_index[MVPP2_CLS_LKP_VLAN_PRI] = rule_idx->vlan_pri_idx;
	c2_index[MVPP2_CLS_LKP_DSCP_PRI] = rule_idx->dscp_pri_idx;
	c2_index[MVPP2_CLS_LKP_DEFAULT] = rule_idx->default_rule_idx;

	for (lkp_type = 0; lkp_type < MVPP2_CLS_LKP_MAX; lkp_type++) {
		PALAD(MVPP2_PRINT_LINE());
		/* For lookup type of MVPP2_CLS_LKP_HASH,
		 * there is no corresponding C2 rule, so skip it
		 */
		if (lkp_type == MVPP2_CLS_LKP_HASH)
			continue;
		/* write index reg */
		mv_pp2x_write(&port->priv->hw, MVPP2_CLS2_TCAM_IDX_REG,
			      c2_index[lkp_type]);
		/* Update rss_attr in reg CLSC2_ATTR2 */
		regVal = mv_pp2x_read(&port->priv->hw,
				      MVPP2_CLS2_ACT_DUP_ATTR_REG);
		if (en)
			regVal |= MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK;
		else
			regVal &= (~MVPP2_CLS2_ACT_DUP_ATTR_RSSEN_MASK);

		mv_pp2x_write(&port->priv->hw,
			      MVPP2_CLS2_ACT_DUP_ATTR_REG, regVal);
	}
}
