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

#include <linux/kernel.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of.h>
#include "common.h"

/*
 * Clock Gating Control
 */
static const struct clk_gating_soc_desc armada_3700_north_bridge_gating_desc[] __initconst = {
	{ "sata-host", NULL, 3, 0, CLK_GATE_SET_TO_DISABLE },
	{ }
};

static void __init
armada_3700_north_bridge_clk_gating_init(struct device_node *np)
{
	mvebu_clk_gating_setup(np, armada_3700_north_bridge_gating_desc);
}
CLK_OF_DECLARE(armada_3700_north_bridge_clk_gating,
	       "marvell,armada-3700-north-bridge-gating-clock",
	       armada_3700_north_bridge_clk_gating_init);

static const struct clk_gating_soc_desc armada_3700_south_bridge_gating_desc[] __initconst = {
	{ "gbe1-core", NULL, 4, 0, CLK_GATE_SET_TO_DISABLE },
	{ "gbe0-core", NULL, 5, 0, CLK_GATE_SET_TO_DISABLE },
	{ "usb32-ss-sys", NULL, 17, 0, CLK_GATE_SET_TO_DISABLE },
	{ }
};

static void __init
armada_3700_south_bridge_clk_gating_init(struct device_node *np)
{
	mvebu_clk_gating_setup(np, armada_3700_south_bridge_gating_desc);
}
CLK_OF_DECLARE(armada_3700_south_bridge_clk_gating,
	       "marvell,armada-3700-south-bridge-gating-clock",
	       armada_3700_south_bridge_clk_gating_init);
