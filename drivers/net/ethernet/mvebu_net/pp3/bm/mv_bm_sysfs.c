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

#include "common/mv_sw_if.h"
#include "common/mv_hw_if.h"
#include "bm/mv_bm.h"
#include "bm/mv_bm_regs.h"

static struct  platform_device *neta_sysfs;

/*
#define PR_ERR_CODE(_rc, _func)	\
{								\
	pr_err("error code = 0x%08X on illegal operation at line %05d in function <%s> in file <%s>\n", rc, __LINE__, __func__, __FILE__);			\
	pr_err("%s: error code = 0x%08X on illegal operation at line %05d in function <%s> in file <%s>\n", _func, _rc, __LINE__, __func__, __FILE__);	\
}
*/
#define PR_ERR_CODE(_rc)	\
{							\
	pr_err("%s: error code = 0x%08X on illegal operation in function <%s>\n", __func__, _rc, attr->attr.name);	\
}
/*
	pr_err("%s: illegal operation in function <%s> at line %05d in file <%s>, error code = 0x%08X\n", __func__, attr->attr.name, __LINE__, __FILE__, rc);	\
	pr_err("%s: illegal operation in function <%s>, error code = 0x%08X\n", __func__, attr->attr.name, rc);		\
	pr_err("%s: error code = 0x%08X on illegal operation at line %05d in function <%s> in file <%s>\n", _func, _rc, __LINE__, __func__, __FILE__);	\
*/

#define PR_INFO_CALLED		\
{							\
	pr_info("%s is called\n", attr->attr.name);	\
}

#define BM_MALLOC_SIZE 0x00000100

#ifdef __linux__
#define BM_MALLOC(_size)	((u32)kmalloc(_size, GFP_KERNEL))
#else
#define BM_MALLOC(_size)	 ((u32)malloc(_size))
#endif


static ssize_t mv_bm_help(char *buf)
{
	int off = 0;

	off += sprintf(buf+off, "cat  status                            - show BM status\n");
	off += sprintf(buf+off, "echo > bm_open                         - Init BM registers\n");
	off += sprintf(buf+off, "echo > bm_attr_all_pools_def_set             - configures BM read/write default attributes\n");
	off += sprintf(buf+off, "echo rD wD rC wC rQ wQ > bm_attr_qm_pool_set - configures BM read/write attributes for qm pools\n");
/*      : o += sprintf(b+o, "echo rD wD rC wC rQ wQ > bm_attr_gp_pool_set - configures BM read/write attributes for gp pools\n"); */
	off += sprintf(buf+off, "echo rD wD rC wC rQ wQ > bm_attr_gp_pool_set - configures BM read/write attributes for gp pools\n");
	off += sprintf(buf+off, "echo > bm_enable_status_get                  - Get BM enable status\n");
	off += sprintf(buf+off, "echo nb > bm_qm_gpm_pools_def_quick_init     - Initiates of GPM pools with default values\n");
	off += sprintf(buf+off, "echo nb > bm_qm_dram_pools_def_quick_init    - Initiates of DRAM pools with default values\n");
	off += sprintf(buf+off, "echo nb et ft id ca cot cit, cnb > bm_qm_gpm_pools_quick_init  - Initiates QM GPM pools\n");
	off += sprintf(buf+off, "echo nb et ft id ca cot cit, cnb > bm_qm_dram_pools_quick_init - Initiates QM DRAM pools\n");
	off += sprintf(buf+off, "echo p > bm_pool_quick_init_status_get       - Get pool quick init status\n");
	off += sprintf(buf+off, "echo p nb pm > bm_gp_pool_def_basic_init     - Basic init of gp pools with default values\n");
	off += sprintf(buf+off, "echo p nb ps pp et ft id ca cot cit cnb > bm_gp_pool_basic_init - Basic init of gp pools\n");
	off += sprintf(buf+off, "echo > bm_enable                            - Global enable BM\n");
	off += sprintf(buf+off, "echo > bm_disable                           - Global disable BM\n");
	off += sprintf(buf+off, "echo p > bm_pool_fill_level_get             - gives fill level of pool in DRAM\n");
	off += sprintf(buf+off, "echo id > bm_vmid_set                       - Set BM VMID\n");
	off += sprintf(buf+off, "echo p nb fl pm > bm_gp_pool_def_quick_init - Configure BM registers and allocate memory for pools with default values\n");
	off += sprintf(buf+off, "echo p nb fl ps pp et at cid ca cot cit cnb > bm_gp_pool_quick_init - Configure BM registers and allocate memory for pools\n");
	off += sprintf(buf+off, "echo > bm_global_registers_dump        - Print all global registers\n");
	off += sprintf(buf+off, "echo > bm_pool_registers_dump pool     - Print values of all BM pool registers\n");
	off += sprintf(buf+off, "echo > bm_bank_registers_dump bank     - Print values of all BM bank registers\n");
	off += sprintf(buf+off, "echo > bm_cache_memory_dump bank       - Print all 512 lines of cache per input bank sram_b0...b4_cache_mem\n");
	off += sprintf(buf+off, "echo > bm_idle_status_get              - Read BM idle status\n");
	off += sprintf(buf+off, "echo p pn dpe dpf > bm_pool_status_get - Get status per pool: pool_nempty, dpool_ae, dpool_af\n");
	off += sprintf(buf+off, "echo > bm_idle_debug                   - Print several registers status that gives indication why BM is busy\n");
	off += sprintf(buf+off, "echo > bm_error_dump                   - Print several registers status that gives indication why BM is busy\n");
	off += sprintf(buf+off, "echo p nb > bm_pool_memory_fill         - Fill memory of pool with PE index\n");
	off += sprintf(buf+off, "echo p nb ps et ft > bm_pool_dram_set   - Configure BM with Fill level of pool in DRAM\n");
	off += sprintf(buf+off, "echo p nb ps qi > bm_pool_fill_level_set      - Configure BM with Fill level of pool in DRAM\n");
	off += sprintf(buf+off, "echo p qi > bm_pool_enable                    - Enables BM pool\n");
	off += sprintf(buf+off, "echo p ps > bm_gp_pool_pe_size_set            - Set PE pointer size in general purpose pool\n");
	off += sprintf(buf+off, "echo p pp > bm_gp_pool_pair_set               - Configure if pool is defined to work in pairs\n");
	off += sprintf(buf+off, "echo p cid ca cot cit cnb > bm_pool_cache_set - Configure pool cache parameters\n");
	off += sprintf(buf+off, "echo pool > bm_pool_disable                   - Set Pool to disable\n");
/*
	off += sprintf(buf+off, "echo > qm_gpm_init  b qah qal pah pal  - init QM QM GPM pools (0,1)\n");
	off += sprintf(buf+off, "echo > qm_dram_init b qah qal pah pal  - init QM DRAM pools (2,3)\n");
*/
	off += sprintf(buf+off, "echo ba, ofs, wN, dP > bm_register_read  - Read register from BM units\n");
	off += sprintf(buf+off, "echo ba, ofs, wN, dP > bm_register_write - Write register in BM units\n");
	off += sprintf(buf+off, "\n");

	return off;
}

static ssize_t mv_bm_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	const char      *name = attr->attr.name;
	int             off = 0;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	pr_info("mv_bm_show is called\n");
	if (!strcmp(name, "status")) {
		u32 status = 0;
		pr_info("bm_enable_status_get: ");
		/*bm_enable_status_get(&status);*/
		pr_info("status is %d\n", status);
	} else if (!strcmp(name, "help")) {
		off = mv_bm_help(buf);
	} else if (!strcmp(name, "debug")) {
		pr_info("debug\n");
	} else {
		off = 1;
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
	}

	return off;
}

static ssize_t mv_bm_config(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	int rc = -BM_INPUT_NOT_IN_RANGE;
	int             err = 0;
/*
	u32 flags;
*/
	unsigned long flags;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	local_irq_save(flags);

	if (!strcmp(name, "bm_open")) {
		pr_info("bm_open is called\n");
		rc = bm_open();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_attr_all_pools_def_set")) {
		PR_INFO_CALLED
		rc = bm_attr_all_pools_def_set();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_attr_qm_pool_set")) {
		u32 arDomain, awDomain, arCache, awCache, arQOS, awQOS;

		/* Read input values */
		PR_INFO_CALLED
		arDomain = awDomain = arCache = awCache = arQOS = awQOS = 0xFFFFFFFF;
		sscanf(buf, "%d %d %d %d %d %d", &arDomain, &awDomain, &arCache, &awCache, &arQOS, &awQOS);
		rc = bm_attr_qm_pool_set(arDomain, awDomain, arCache, awCache, arQOS, awQOS);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_attr_gp_pool_set")) {
		u32 arDomain, awDomain, arCache, awCache, arQOS, awQOS;

		/* Read input values */
		PR_INFO_CALLED
		arDomain = awDomain = arCache = awCache = arQOS = awQOS = 0xFFFFFFFF;
		sscanf(buf, "%d %d %d %d %d %d", &arDomain, &awDomain, &arCache, &awCache, &arQOS, &awQOS);
		rc = bm_attr_gp_pool_set(arDomain, awDomain, arCache, awCache, arQOS, awQOS);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_enable_status_get"))	{
		u32 bm_req_rcv_en;

		/* Read input values */
		PR_INFO_CALLED
		bm_req_rcv_en = 0xFFFFFFFF;
		rc = bm_enable_status_get(&bm_req_rcv_en);
		if (rc != OK)
			PR_ERR_CODE(rc)
		pr_info("\t bm_req_rcv_en = %d\n", bm_req_rcv_en);
	} else if (!strcmp(name, "bm_qm_gpm_pools_def_quick_init")) {
		u32 num_of_buffers;
		struct mv_word40 qece_base_address, pl_base_address;

		/* Read input values */
		PR_INFO_CALLED
		num_of_buffers = qece_base_address.hi = qece_base_address.lo = pl_base_address.hi = pl_base_address.lo = 0xFFFFFFFF;
		qece_base_address.hi = 0;
		qece_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		pl_base_address.hi = 0;
		pl_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d", &num_of_buffers);
		rc = bm_qm_gpm_pools_def_quick_init(num_of_buffers, (u32 *)&qece_base_address, (u32 *)&pl_base_address);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_qm_dram_pools_def_quick_init")) {
		u32 num_of_buffers;
		struct mv_word40 qece_base_address, pl_base_address;

		/* Read input values */
		PR_INFO_CALLED
		num_of_buffers = qece_base_address.hi = qece_base_address.lo = pl_base_address.hi = pl_base_address.lo = 0xFFFFFFFF;
		qece_base_address.hi = 0;
		qece_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		pl_base_address.hi = 0;
		pl_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d", &num_of_buffers);
		rc = bm_qm_dram_pools_def_quick_init(num_of_buffers, (u32 *)&qece_base_address, (u32 *)&pl_base_address);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_qm_gpm_pools_quick_init")) {
		u32 num_of_buffers, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;
		struct mv_word40 qece_base_address, pl_base_address;

		/* Read input values */
		PR_INFO_CALLED
		num_of_buffers = qece_base_address.hi = qece_base_address.lo = pl_base_address.hi = pl_base_address.lo = ae_thr = af_thr = cache_vmid = cache_attr = cache_so_thr = cache_si_thr = cache_num_of_buffers = 0xFFFFFFFF;
		qece_base_address.hi = 0;
		qece_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		pl_base_address.hi = 0;
		pl_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d %d %d %d %d", &num_of_buffers, &ae_thr, &af_thr, &cache_vmid, &cache_attr, &cache_so_thr, &cache_si_thr, &cache_num_of_buffers);
		rc = bm_qm_gpm_pools_quick_init(num_of_buffers, (u32 *)&qece_base_address, (u32 *)&pl_base_address, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_qm_dram_pools_quick_init")) {
		u32 num_of_buffers, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;
		struct mv_word40 qece_base_address, pl_base_address;

		/* Read input values */
		PR_INFO_CALLED
		num_of_buffers = qece_base_address.hi = qece_base_address.lo = pl_base_address.hi = pl_base_address.lo = ae_thr = af_thr = cache_vmid = cache_attr = cache_so_thr = cache_si_thr = cache_num_of_buffers = 0xFFFFFFFF;
		qece_base_address.hi = 0;
		qece_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		pl_base_address.hi = 0;
		pl_base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d %d %d %d %d", &num_of_buffers, &ae_thr, &af_thr, &cache_vmid, &cache_attr, &cache_so_thr, &cache_si_thr, &cache_num_of_buffers);
		rc = bm_qm_dram_pools_quick_init(num_of_buffers, (u32 *)&qece_base_address, (u32 *)&pl_base_address, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_quick_init_status_get")) {
		u32 pool, completed;

		/* Read input values */
		PR_INFO_CALLED
		pool = completed = 0xFFFFFFFF;
		sscanf(buf, "%d", &pool);
		rc = bm_pool_quick_init_status_get(pool, &completed);
		if (rc != OK)
			PR_ERR_CODE(rc)
		pr_info("\t completed = %d\n", completed);
	} else if (!strcmp(name, "bm_gp_pool_def_basic_init")) {
		u32 pool, num_of_buffers, partition_model;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = base_address.hi = base_address.lo = partition_model = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d", &pool, &num_of_buffers, &partition_model);
		rc = bm_gp_pool_def_basic_init(pool, num_of_buffers, (u32 *)&base_address, partition_model);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_gp_pool_basic_init")) {
		u32 pool, num_of_buffers, pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = base_address.hi = base_address.lo = pe_size = pool_pair = ae_thr = af_thr = cache_vmid = cache_attr = cache_so_thr = cache_si_thr = cache_num_of_buffers = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d", &pool, &num_of_buffers, &pe_size, &pool_pair, &ae_thr, &af_thr, &cache_vmid, &cache_attr, &cache_so_thr, &cache_si_thr, &cache_num_of_buffers);
		rc = bm_gp_pool_basic_init(pool, num_of_buffers, (u32 *)&base_address, pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_enable")) {
		PR_INFO_CALLED
		rc = bm_enable();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_disable")) {
		PR_INFO_CALLED
		rc = bm_disable();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_fill_level_get")) {
		u32 pool, fill_level;

		/* Read input values */
		PR_INFO_CALLED
		pool = fill_level = 0xFFFFFFFF;
		sscanf(buf, "%d", &pool);
		rc = bm_pool_fill_level_get(pool, &fill_level);
		if (rc != OK)
			PR_ERR_CODE(rc)
		pr_info("\t fill_level = %d\n", fill_level);
	} else if (!strcmp(name, "bm_vmid_set")) {
		u32 bm_vmid;

		/* Read input values */
		PR_INFO_CALLED
		bm_vmid = 0xFFFFFFFF;
		sscanf(buf, "%d", &bm_vmid);
		rc = bm_vmid_set(bm_vmid);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_gp_pool_def_quick_init")) {
		u32 pool, num_of_buffers, fill_level, partition_model;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = fill_level = base_address.hi = base_address.lo = partition_model = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d", &pool, &num_of_buffers, &fill_level, &partition_model);
		rc = bm_gp_pool_def_quick_init(pool, num_of_buffers, fill_level, (u32 *)&base_address, partition_model);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_gp_pool_quick_init")) {
		u32 pool, num_of_buffers, fill_level, pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = fill_level = base_address.hi = base_address.lo = pe_size = pool_pair = ae_thr = af_thr = cache_vmid = cache_attr = cache_so_thr = cache_si_thr = cache_num_of_buffers = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d", &pool, &num_of_buffers, &fill_level, &pe_size, &pool_pair, &ae_thr, &af_thr, &cache_vmid, &cache_attr, &cache_so_thr, &cache_si_thr, &cache_num_of_buffers);
		rc = bm_gp_pool_quick_init(pool, num_of_buffers, fill_level, (u32 *)&base_address, pe_size, pool_pair, ae_thr, af_thr, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_global_registers_dump")) {
		PR_INFO_CALLED
		rc = bm_global_registers_dump();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_registers_dump")) {
		u32 pool;

		/* Read input values */
		PR_INFO_CALLED
		pool = 0xFFFFFFFF;
		sscanf(buf, "%d", &pool);
		rc = bm_pool_registers_dump(pool);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_bank_registers_dump")) {
		u32 bank;

		/* Read input values */
		PR_INFO_CALLED
		bank = 0xFFFFFFFF;
		sscanf(buf, "%d", &bank);
		rc = bm_bank_registers_dump(bank);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_cache_memory_dump")) {
		u32 bank;

		/* Read input values */
		PR_INFO_CALLED
		bank = 0xFFFFFFFF;
		sscanf(buf, "%d", &bank);
		rc = bm_cache_memory_dump(bank);
		if (rc != OK)
			pr_err("%s: illegal operation in function <%s>, error code = 0x%08X\n", __func__, attr->attr.name, rc);
	} else if (!strcmp(name, "bm_idle_status_get")) {
		u32 status;

		/* Read input values */
		PR_INFO_CALLED
		status = 0xFFFFFFFF;
		rc = bm_idle_status_get(&status);
		if (rc != OK)
			PR_ERR_CODE(rc)
		pr_info("\t status = %d\n", status);
	} else if (!strcmp(name, "bm_pool_status_get")) {
		u32 pool, pool_nempty, dpool_ae, dpool_af;

		PR_INFO_CALLED
		pool = pool_nempty = dpool_ae = dpool_af = 0xFFFFFFFF;
		/* Read input values */
		sscanf(buf, "%d %d %d %d", &pool, &pool_nempty, &dpool_ae, &dpool_af);
		rc = bm_pool_status_get(pool, &pool_nempty, &dpool_ae, &dpool_af);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_idle_debug")) {
		PR_INFO_CALLED
		rc = bm_idle_debug();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_error_dump")) {
		PR_INFO_CALLED
		rc = bm_error_dump();
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_memory_fill")) {
		u32 pool,  num_of_buffers;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = base_address.hi = base_address.lo = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d", &pool, &num_of_buffers);
		rc = bm_pool_memory_fill(pool, num_of_buffers, (u32 *)&base_address);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_dram_set"))	{
		u32 pool,  num_of_buffers, pe_size, ae_thr, af_thr;
		struct mv_word40 base_address;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = pe_size = base_address.hi = base_address.lo = ae_thr = af_thr = 0xFFFFFFFF;
		base_address.hi = 0;
		base_address.lo = BM_MALLOC(BM_MALLOC_SIZE);
		sscanf(buf, "%d %d %d %d %d", &pool, &num_of_buffers, &pe_size, &ae_thr, &af_thr);
		rc = bm_pool_dram_set(pool,  num_of_buffers, pe_size,  (u32 *)&base_address, ae_thr, af_thr);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_fill_level_set")) {
		u32 pool,  num_of_buffers, pe_size,  quick_init;

		/* Read input values */
		PR_INFO_CALLED
		pool = num_of_buffers = pe_size = quick_init = 0xFFFFFFFF;
		sscanf(buf, "%d %d %d %d", &pool, &num_of_buffers, &pe_size, &quick_init);
		rc = bm_pool_fill_level_set(pool, num_of_buffers, pe_size, quick_init);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_enable")) {
		u32 pool, quick_init;

		/* Read input values */
		PR_INFO_CALLED
		pool = quick_init = 0xFFFFFFFF;
		sscanf(buf, "%d %d", &pool, &quick_init);
		rc = bm_pool_enable(pool, quick_init);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_gp_pool_pe_size_set")) {
		u32 pool, pe_size;

		/* Read input values */
		PR_INFO_CALLED
		pool = pe_size = 0xFFFFFFFF;
		sscanf(buf, "%d %d", &pool, &pe_size);
		rc = bm_gp_pool_pe_size_set(pool, pe_size);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_gp_pool_pair_set")) {
		u32 pool, pool_pair;

		/* Read input values */
		PR_INFO_CALLED
		pool = pool_pair = 0xFFFFFFFF;
		sscanf(buf, "%d %d", &pool, &pool_pair);
		rc = bm_gp_pool_pair_set(pool, pool_pair);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_cache_set")) {
		u32 pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers;

		/* Read input values */
		PR_INFO_CALLED
		pool = cache_vmid = cache_attr = cache_so_thr = cache_si_thr = cache_num_of_buffers = 0xFFFFFFFF;
		sscanf(buf, "%d %d %d %d %d %d", &pool, &cache_vmid, &cache_attr, &cache_so_thr, &cache_si_thr, &cache_num_of_buffers);
		rc = bm_pool_cache_set(pool, cache_vmid, cache_attr, cache_so_thr, cache_si_thr, cache_num_of_buffers);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_pool_disable")) {
		u32 pool;

		/* Read input values */
		PR_INFO_CALLED
		pool = 0xFFFFFFFF;
		sscanf(buf, "%d", &pool);
		rc = bm_pool_disable(pool);
		if (rc != OK)
			PR_ERR_CODE(rc)
/*not used*/
	} else if (!strcmp(name, "bm_register_read")) {
		u32 base_address, offset, wordsNumber, dataPtr;

		PR_INFO_CALLED
		base_address = offset = wordsNumber = dataPtr = 0xFFFFFFFF;
		/* Read input values */
		sscanf(buf, "%x %x %x %x", &base_address, &offset, &wordsNumber, &dataPtr);
		rc = bm_register_read(base_address, offset, wordsNumber, (u32 *)&dataPtr);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else if (!strcmp(name, "bm_register_write")) {
		u32 base_address, offset, wordsNumber, dataPtr;

		PR_INFO_CALLED
		base_address = offset = wordsNumber = dataPtr = 0xFFFFFFFF;
		/* Read input values */
		sscanf(buf, "%x %x %x %x", &base_address, &offset, &wordsNumber, &dataPtr);
		rc = bm_register_write(base_address, offset, wordsNumber, (u32 *)&dataPtr);
		if (rc != OK)
			PR_ERR_CODE(rc)
	} else {
		err = 1;
/*		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);*/
		pr_err("%s: wrong name of BM function <%s>\n", __func__, attr->attr.name);
	}

	local_irq_restore(flags);

	return err ? -EINVAL : len;
}

static DEVICE_ATTR(help,                            S_IRUSR, mv_bm_show, NULL);
static DEVICE_ATTR(status,                          S_IRUSR, mv_bm_show, NULL);
static DEVICE_ATTR(bm_open,                         S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_attr_all_pools_def_set,       S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_attr_qm_pool_set,             S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_attr_gp_pool_set,             S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_enable_status_get,            S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_qm_gpm_pools_def_quick_init,  S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_qm_dram_pools_def_quick_init, S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_qm_gpm_pools_quick_init,      S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_qm_dram_pools_quick_init,     S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_quick_init_status_get,   S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_def_basic_init,       S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_basic_init,           S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_enable,                       S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_disable,                      S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_fill_level_get,          S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_vmid_set,                     S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_def_quick_init,       S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_quick_init,           S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_global_registers_dump,        S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_registers_dump,          S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_bank_registers_dump,          S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_cache_memory_dump,            S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_idle_status_get,              S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_status_get,              S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_idle_debug,                   S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_error_dump,                   S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_memory_fill,             S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_dram_set,                S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_fill_level_set,          S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_enable,                  S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_pe_size_set,          S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_gp_pool_pair_set,             S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_cache_set,               S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_pool_disable,                 S_IWUSR, NULL,       mv_bm_config);
/*
static DEVICE_ATTR(qm_gpm_init,                     S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(qm_dram_init,                    S_IWUSR, NULL,       mv_bm_config);
*/
static DEVICE_ATTR(bm_register_read,                S_IWUSR, NULL,       mv_bm_config);
static DEVICE_ATTR(bm_register_write,               S_IWUSR, NULL,       mv_bm_config);


static struct attribute *mv_bm_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_status.attr,
	&dev_attr_bm_open.attr,
	&dev_attr_bm_attr_all_pools_def_set.attr,
	&dev_attr_bm_attr_qm_pool_set.attr,
	&dev_attr_bm_attr_gp_pool_set.attr,
	&dev_attr_bm_enable_status_get.attr,
	&dev_attr_bm_qm_gpm_pools_def_quick_init.attr,
	&dev_attr_bm_qm_dram_pools_def_quick_init.attr,
	&dev_attr_bm_qm_gpm_pools_quick_init.attr,
	&dev_attr_bm_qm_dram_pools_quick_init.attr,
	&dev_attr_bm_pool_quick_init_status_get.attr,
	&dev_attr_bm_gp_pool_def_basic_init.attr,
	&dev_attr_bm_gp_pool_basic_init.attr,
	&dev_attr_bm_enable.attr,
	&dev_attr_bm_disable.attr,
	&dev_attr_bm_pool_fill_level_get.attr,
	&dev_attr_bm_vmid_set.attr,
	&dev_attr_bm_gp_pool_def_quick_init.attr,
	&dev_attr_bm_gp_pool_quick_init.attr,
	&dev_attr_bm_global_registers_dump.attr,
	&dev_attr_bm_pool_registers_dump.attr,
	&dev_attr_bm_bank_registers_dump.attr,
	&dev_attr_bm_cache_memory_dump.attr,
	&dev_attr_bm_idle_status_get.attr,
	&dev_attr_bm_pool_status_get.attr,
	&dev_attr_bm_idle_debug.attr,
	&dev_attr_bm_error_dump.attr,
	&dev_attr_bm_pool_memory_fill.attr,
	&dev_attr_bm_pool_dram_set.attr,
	&dev_attr_bm_pool_fill_level_set.attr,
	&dev_attr_bm_pool_enable.attr,
	&dev_attr_bm_gp_pool_pe_size_set.attr,
	&dev_attr_bm_gp_pool_pair_set.attr,
	&dev_attr_bm_pool_cache_set.attr,
	&dev_attr_bm_pool_disable.attr,
/*
	&dev_attr_qm_gpm_init.attr,
	&dev_attr_qm_dram_init.attr,
*/
	&dev_attr_bm_register_read.attr,
	&dev_attr_bm_register_write.attr,
	NULL
};


static struct attribute_group mv_bm_group = {
	.name = "bm",
	.attrs = mv_bm_attrs,
};

int mv_pp3_bm_sysfs_init(struct kobject *neta_kobj)
{
	int err;

	err = sysfs_create_group(neta_kobj, &mv_bm_group);
	if (err) {
		pr_err(KERN_INFO "sysfs group failed for bm%d\n", err);
		return err;
	}

	return err;
}

int mv_sysfs_exit(struct kobject *neta_kobj)
{
	sysfs_remove_group(neta_kobj, &mv_bm_group);

	return 0;
}


int mv_pp3_bm_sysfs_init_main(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	if (!pd) {
		neta_sysfs = platform_device_register_simple("neta", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	}
	if (!pd) {
		pr_err(KERN_ERR"%s: cannot find neta device\n", __func__);
		return -1;
	}

	mv_pp3_bm_sysfs_init(&pd->kobj);

	return 0;
}
void mv_eth_sysfs_exit_main(void)
{
	struct device *pd;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "neta");
	if (!pd) {
		pr_err(KERN_ERR"%s: cannot find pp2 device\n", __func__);
		return;
	}

	platform_device_unregister(neta_sysfs);

	return;
}


/*
Examples for running the functions above.

1.	Set BM attributes
		bm_attr_all_pools_def_set();
2.	Init QM GPM pools (function ýbm_qm_gpm_pools_def_quick_init)
		bm_qm_gpm_pools_def_quick_init( num_of_buffers,
				qece_base_address_hi, qece_base_address_lo,
					pl_base_address_hi, pl_base_address_lo);
3.	Optional: Init QM DRAM pools (function )
		bm_qm_dram_pools_def_quick_init(num_of_buffers,
				qece_base_address_hi, qece_base_address_lo,
					pl_base_address_hi, pl_base_address_lo);
4.	Init GP purpose pool  - at least one (function bm_gp_pool_def_basic_init  or ýbm_gp_pool_def_quick_init )
		bm_gp_pool_def_basic_init(pool, num_of_buffers, base_address_hi, base_address_lo, partition_model);
		bm_gp_pool_def_quick_init(pool, num_of_buffers, fill_level, base_address_hi, base_address_lo, partition_model);
5.	Enable BM (function bm_enable)
		bm_enable();

++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
echo > bm_open
echo > bm_attr_all_pools_def_set
echo > bm_attr_qm_pool_set 2 1 11 14 2 1
echo > bm_attr_gp_pool_set 2 1 11 14 2 1
echo > bm_enable_status_get
echo > bm_qm_gpm_pools_def_quick_init  266
echo > bm_qm_dram_pools_def_quick_init  416
echo > bm_qm_gpm_pools_quick_init 416 16 32 30 59 16 15 16
echo > bm_qm_dram_pools_quick_init 416 16 32 30 59 16 15 16
echo > bm_pool_quick_init_status_get 0
echo > bm_gp_pool_def_basic_init 10 416 1
echo > bm_gp_pool_basic_init 10 16 1 1 0 16
echo > bm_enable
echo > bm_disable
echo > bm_pool_fill_level_get 3
echo > bm_vmid_set 11
echo > bm_gp_pool_def_quick_init 10 416 256 1
echo > bm_gp_pool_quick_init 10 416 256 1 1
echo > bm_global_registers_dump
echo > bm_pool_registers_dump 10
echo > bm_bank_registers_dump 3
echo > bm_cache_memory_dump 3
echo > bm_idle_status_get
echo > bm_pool_status_get 0 1 2 3
echo > bm_idle_debug
echo > bm_error_dump
echo > bm_pool_memory_fill 0 16
echo > bm_pool_dram_set 0 16 0 16
echo > bm_pool_fill_level_set 0 16 1 1
echo > bm_pool_enable 0 1
echo > bm_gp_pool_pe_size_set 20 1
echo > bm_gp_pool_pair_set 30 1
echo > bm_pool_cache_set 0 10 16777216 16777316 50 16
echo > bm_pool_disable 0



*/
