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

static MV_BM_POOL	hwfBmPool[MV_BM_POOLS];


static int mv_eth_hwf_pool_add(MV_BM_POOL *pBmPool, int bufNum)
{
	int      i;
	void     *pVirt;
	MV_ULONG physAddr;

	/* Check total number of buffers doesn't exceed capacity */
	if ((bufNum < 0) ||
		((bufNum + pBmPool->bufNum) > (pBmPool->capacity))) {

		mvOsPrintf("%s: to many %d buffers for BM pool #%d: capacity=%d, buf_num=%d\n",
			   __func__, bufNum, pBmPool->pool, pBmPool->capacity, pBmPool->bufNum);
		return 0;
	}
	/* Allocate buffers for the pool */
	for (i = 0; i < bufNum; i++) {

		pVirt = mvOsIoCachedMalloc(NULL, pBmPool->bufSize, &physAddr, NULL);
		if (pVirt == NULL) {
			mvOsPrintf("%s: Warning! Not all buffers of %d bytes allocated\n",
						__func__, pBmPool->bufSize);
			break;
		}
		mvBmPoolPut(pBmPool->pool, (MV_ULONG) physAddr);
	}
	pBmPool->bufNum += i;

	mvOsPrintf("BM pool #%d for HWF: bufSize=%4d - %4d of %4d buffers added\n",
	       pBmPool->pool, pBmPool->bufSize, i, bufNum);

	return i;
}

/*******************************************************************************
 * mv_eth_hwf_bm_create - create BM pools used by the port for HWF only
 *
 * INPUT:
 *       int        port	- port number
 *
 * RETURN:   MV_STATUS
 *               MV_OK - Success, Others - Failure
 *
 *******************************************************************************/
MV_STATUS mv_eth_hwf_bm_create(int port, int mtuPktSize)
{
	static bool		isFirst = true;
	MV_BM_POOL		*pBmPool;
	int				long_pool, short_pool;
	int				long_buf_size, short_buf_size;

	/* Check validity of the parameters */
	if (mvNetaPortCheck(port))
		return MV_FAIL;

	/* For the first time - clean hwfBmPool array */
	if (isFirst == true) {
		memset(&hwfBmPool, 0, sizeof(hwfBmPool));
		isFirst = false;
	}

	long_pool = mv_eth_bm_config_long_pool_get(port);
	/* Check validity of the parameters */
	if (mvNetaMaxCheck(long_pool,  MV_BM_POOLS, "bm pool"))
		return MV_FAIL;

	/* For HWF Packet offset in the packet is 8 bytes */
	long_buf_size = mv_eth_bm_config_pkt_size_get(long_pool);
	if (long_buf_size == 0)
		long_buf_size = mtuPktSize + 8;

	/* Check validity of the parameters */
	if (long_buf_size < (mtuPktSize + 8))
		return MV_FAIL;

	/* Create long pool */
	pBmPool = &hwfBmPool[long_pool];
	if (pBmPool->pVirt == NULL) {
		/* Allocate new pool */
		pBmPool->pVirt = mv_eth_bm_pool_create(long_pool, MV_BM_POOL_CAP_MAX, &pBmPool->physAddr);
		if (pBmPool->pVirt == NULL) {
			mvOsPrintf("%s: Can't allocate %d bytes for Long pool #%d of port #%d\n",
					__func__, MV_BM_POOL_CAP_MAX * sizeof(MV_U32), long_pool, port);
			return MV_OUT_OF_CPU_MEM;
		}
		pBmPool->pool = long_pool;
		pBmPool->capacity = MV_BM_POOL_CAP_MAX;
		pBmPool->bufSize = long_buf_size;
		mvNetaBmPoolBufSizeSet(port, long_pool, long_buf_size);
	} else {
		/* Share pool with other port - check buffer size */
		if (long_buf_size > pBmPool->bufSize) {
			/* The BM pool doesn't match the mtuPktSize */
			mvOsPrintf("%s: longBufSize=%d is too match for the pool #%d (%d bytes)\n",
						__func__, long_buf_size, pBmPool->pool, pBmPool->bufSize);
			return MV_FAIL;
		}
	}
	mv_eth_hwf_pool_add(pBmPool, mv_eth_bm_config_long_buf_num_get(port));

	/* Create short pool */
	short_pool = mv_eth_bm_config_short_pool_get(port);
	short_buf_size = mv_eth_bm_config_pkt_size_get(short_pool);
	if (short_pool != long_pool) {
		pBmPool = &hwfBmPool[short_pool];
		if (pBmPool->pVirt == NULL) {
			/* Allocate new pool */
			pBmPool->pVirt = mv_eth_bm_pool_create(short_pool, MV_BM_POOL_CAP_MAX, &pBmPool->physAddr);
			if (pBmPool->pVirt == NULL) {
				mvOsPrintf("%s: Can't allocate %d bytes for Short pool #%d of port #%d\n",
						__func__, MV_BM_POOL_CAP_MAX * sizeof(MV_U32), short_pool, port);
				return MV_OUT_OF_CPU_MEM;
			}
			pBmPool->pool = short_pool;
			pBmPool->capacity = MV_BM_POOL_CAP_MAX;
			pBmPool->bufSize = short_buf_size;
			mvNetaBmPoolBufSizeSet(port, short_pool, short_buf_size);
		} else {
			/* Share pool with other port - check buffer size */
			if (short_buf_size > pBmPool->bufSize) {
				/* The BM pool doesn't match the mtuPktSize */
				mvOsPrintf("%s: shortBufSize=%d is too match for the pool #%d (%d bytes)\n",
							__func__, short_buf_size, pBmPool->pool, pBmPool->bufSize);
				return MV_FAIL;
			}
		}
		/* Add buffers to short pool */
		mv_eth_hwf_pool_add(pBmPool, mv_eth_bm_config_short_buf_num_get(port));
	}
	mvNetaHwfBmPoolsSet(port, short_pool, long_pool);
	return MV_OK;
}

void mv_hwf_bm_dump(void)
{
	int          i;
	MV_BM_POOL   *bmPool;

	mvOsPrintf("HWF BM Pools configuration\n");
	mvOsPrintf("pool:    capacity    bufSize    bufNum      virtPtr       physAddr\n");
	for (i = 0; i < MV_BM_POOLS; i++) {
		bmPool = &hwfBmPool[i];
		if (bmPool->pVirt)
			mvOsPrintf("  %2d:     %4d       %4d       %4d      %p      0x%08x\n",
						bmPool->pool, bmPool->capacity, bmPool->bufSize, bmPool->bufNum,
						bmPool->pVirt, (unsigned)bmPool->physAddr);
	}
}

