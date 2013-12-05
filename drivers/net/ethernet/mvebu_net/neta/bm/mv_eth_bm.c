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
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include "gbe/mvNeta.h"
#include "bm/mvBm.h"

#include "net_dev/mv_netdev.h"

typedef struct {
	int             pool_pkt_size[MV_BM_POOLS];
	MV_BM_CONFIG	port_config[CONFIG_MV_ETH_PORTS_NUM];
} MV_ETH_BM_CONFIG;

static MV_ETH_BM_CONFIG mv_eth_bm_config;

int mv_eth_bm_config_pkt_size_get(int pool)
{
	if (mvNetaMaxCheck(pool, MV_BM_POOLS, "bm_pool"))
		return -EINVAL;

	return mv_eth_bm_config.pool_pkt_size[pool];
}

int mv_eth_bm_config_pkt_size_set(int pool, int pkt_size)
{
	if (mvNetaMaxCheck(pool, MV_BM_POOLS, "bm_pool"))
		return -EINVAL;

	mv_eth_bm_config.pool_pkt_size[pool] = pkt_size;
	return 0;
}

int mv_eth_bm_config_long_pool_get(int port)
{
	if (mvNetaPortCheck(port))
		return -EINVAL;

	return mv_eth_bm_config.port_config[port].longPool;
}

int mv_eth_bm_config_long_buf_num_get(int port)
{
	if (mvNetaPortCheck(port))
		return -EINVAL;

	return mv_eth_bm_config.port_config[port].longBufNum;
}

int mv_eth_bm_config_short_pool_get(int port)
{
	if (mvNetaPortCheck(port))
		return -EINVAL;

	return mv_eth_bm_config.port_config[port].shortPool;
}

int mv_eth_bm_config_short_buf_num_get(int port)
{
	if (mvNetaPortCheck(port))
		return -EINVAL;

	return mv_eth_bm_config.port_config[port].shortBufNum;
}

/* Once time call: init configuration structure accordingly with compile time parameters */
MV_STATUS mv_eth_bm_config_get(void)
{
	MV_BM_CONFIG *bmConfig;
	int           port;

	mv_eth_bm_config.pool_pkt_size[0] = CONFIG_MV_ETH_BM_0_PKT_SIZE;
	mv_eth_bm_config.pool_pkt_size[1] = CONFIG_MV_ETH_BM_1_PKT_SIZE;
	mv_eth_bm_config.pool_pkt_size[2] = CONFIG_MV_ETH_BM_2_PKT_SIZE;
	mv_eth_bm_config.pool_pkt_size[3] = CONFIG_MV_ETH_BM_3_PKT_SIZE;

#ifdef CONFIG_MV_ETH_BM_PORT_0
	port = 0;
	bmConfig = &mv_eth_bm_config.port_config[port];
	memset(bmConfig, 0, sizeof(MV_BM_CONFIG));
	bmConfig->valid = 1;
	bmConfig->longPool = CONFIG_MV_ETH_BM_PORT_0_LONG_POOL;
	bmConfig->shortPool = CONFIG_MV_ETH_BM_PORT_0_SHORT_POOL;
	bmConfig->longBufNum = CONFIG_MV_ETH_BM_PORT_0_LONG_BUF_NUM;

#if (CONFIG_MV_ETH_BM_PORT_0_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_0_LONG_POOL)
	bmConfig->shortBufNum = CONFIG_MV_ETH_BM_PORT_0_SHORT_BUF_NUM;
#endif /* CONFIG_MV_ETH_BM_PORT_0_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_0_LONG_POOL */
#endif /* CONFIG_MV_ETH_BM_PORT_0 */

#ifdef CONFIG_MV_ETH_BM_PORT_1
	port = 1;
	bmConfig = &mv_eth_bm_config.port_config[port];
	memset(bmConfig, 0, sizeof(MV_BM_CONFIG));
	bmConfig->valid = 1;
	bmConfig->longPool = CONFIG_MV_ETH_BM_PORT_1_LONG_POOL;
	bmConfig->shortPool = CONFIG_MV_ETH_BM_PORT_1_SHORT_POOL;
	bmConfig->longBufNum = CONFIG_MV_ETH_BM_PORT_1_LONG_BUF_NUM;

#if (CONFIG_MV_ETH_BM_PORT_1_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_1_LONG_POOL)
	bmConfig->shortBufNum = CONFIG_MV_ETH_BM_PORT_1_SHORT_BUF_NUM;
#endif /* CONFIG_MV_ETH_BM_PORT_1_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_1_LONG_POOL */
#endif /* CONFIG_MV_ETH_BM_PORT_1 */

#ifdef CONFIG_MV_ETH_BM_PORT_2
	port = 2;
	bmConfig = &mv_eth_bm_config.port_config[port];
	memset(bmConfig, 0, sizeof(MV_BM_CONFIG));
	bmConfig->valid = 1;
	bmConfig->longPool = CONFIG_MV_ETH_BM_PORT_2_LONG_POOL;
	bmConfig->shortPool = CONFIG_MV_ETH_BM_PORT_2_SHORT_POOL;
	bmConfig->longBufNum = CONFIG_MV_ETH_BM_PORT_2_LONG_BUF_NUM;

#if (CONFIG_MV_ETH_BM_PORT_2_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_2_LONG_POOL)
	bmConfig->shortBufNum = CONFIG_MV_ETH_BM_PORT_2_SHORT_BUF_NUM;
#endif /* CONFIG_MV_ETH_BM_PORT_2_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_2_LONG_POOL */
#endif /* CONFIG_MV_ETH_BM_PORT_2 */

	#ifdef CONFIG_MV_ETH_BM_PORT_3
	port = 3;
	bmConfig = &mv_eth_bm_config.port_config[port];
	memset(bmConfig, 0, sizeof(MV_BM_CONFIG));
	bmConfig->valid = 1;
	bmConfig->longPool = CONFIG_MV_ETH_BM_PORT_3_LONG_POOL;
	bmConfig->shortPool = CONFIG_MV_ETH_BM_PORT_3_SHORT_POOL;
	bmConfig->longBufNum = CONFIG_MV_ETH_BM_PORT_3_LONG_BUF_NUM;

#if (CONFIG_MV_ETH_BM_PORT_3_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_3_LONG_POOL)
	bmConfig->shortBufNum = CONFIG_MV_ETH_BM_PORT_3_SHORT_BUF_NUM;
#endif /* CONFIG_MV_ETH_BM_PORT_3_SHORT_POOL != CONFIG_MV_ETH_BM_PORT_3_LONG_POOL */
#endif /* CONFIG_MV_ETH_BM_PORT_3 */

	return MV_OK;
}

void mv_eth_bm_config_print(void)
{
	int           i;
	MV_BM_CONFIG *bmConfig;

	mvOsPrintf("BM compile time configuration\n");
	for (i = 0; i < MV_BM_POOLS; i++)
		mvOsPrintf("pool %d: pkt_size = %d bytes\n", i, mv_eth_bm_config.pool_pkt_size[i]);

	mvOsPrintf("\n");
	mvOsPrintf("port:  longPool  shortPool  longBufNum  shortBufNum\n");
	for (i = 0; i < CONFIG_MV_ETH_PORTS_NUM; i++) {
		bmConfig = &mv_eth_bm_config.port_config[i];
		if (bmConfig->valid)
			mvOsPrintf("  %2d:   %4d       %4d        %4d         %4d\n",
				i, bmConfig->longPool, bmConfig->shortPool,
				bmConfig->longBufNum, bmConfig->shortBufNum);
	}
	mvOsPrintf("\n");
}

