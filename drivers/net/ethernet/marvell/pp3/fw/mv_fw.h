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

#ifndef __mv_fw_h__
#define __mv_fw_h__

#include "platform/mv_pp3.h"

/* IMEM size 196KB */
#define MV_PP3_FW_MAX_SIZE (196 * 1024)
#define MV_PP3_FW_MAX_ROWS (MV_PP3_FW_MAX_SIZE / 4)
#define MV_FW_MAX_LINE_SIZE (128)

#define MV_MAX_FW_FILE_PATH	(128)

/* datapath ppc */
#define MV_DP_PPC_BASE(ppc)		MV_PPC_BASE(ppc+1)

enum ppc_mem_type {
	PPC_MEM_IMEM,
	PPC_MEM_CFG,
	PPC_MEM_PROF,
	PPC_MEM_SE_CFG,
	PPC_MEM_SRAM,
	PPC_MEM_SPAD,
	PPC_MEM_INVALID
};


struct mem_rec {
	u32 address;
	u32 data;
};

struct mem_image {
	struct mem_rec *rows;
	u32 size;
	u32 allocated;
	enum ppc_mem_type type;
	u32 offset;
	u32 mask;
};

int mv_pp3_fw_memory_alloc(struct mv_pp3 *priv);
void mv_pp3_fw_memory_free(struct mv_pp3 *priv);

void mv_pp3_fw_path_set(u8 *path);
void mv_pp3_ppc_run_all(void);
int mv_pp3_ppc_idle_wait_all(void);

int mv_pp3_se_fw_image_download(char *dir);
int mv_pp3_ppc_fw_image_download(int ppc, char *dir, unsigned int mem_type);
int mv_pp3_cfg_download(char *path);
int mv_pp3_fw_image_load(struct mem_image *ptr_to_image);
int mv_pp3_fw_dram_allocation(void);
int mv_pp3_fw_init(struct mv_pp3 *priv);
int mv_pp3_fw_load(void);
int mv_pp3_imem_dump(char *path);
int mv_pp3_fw_half_range_addr_cfg(void);

int mv_pp3_profile_dump(char *path);

int mv_pp3_cfg_dump(char *path);

int mv_pp3_fw_read_file(char *path, char *buf, int size);
int mv_pp3_fw_write_file(char *path, char *buf, int size);
int mv_pp3_fw_read_img_file(char *path, struct mem_image *img);

int mv_fw_mem_write(char *data, int size, unsigned int target_mem);
int mv_fw_mem_read(char *data, int size, unsigned int source_mem);

int mv_fw_mem_img_write(struct mem_image *img, unsigned int mem_type);


int mv_fw_pkts_rec_dump(int ppc, u32 start_entry, int size);
int mv_fw_sp_dump(int ppc, u32 ppn_numb, u32 buf_index, u32 start_sp_adr, u32 size);
int mv_fw_inf_logger_dump(int ppc, u32 ppn_numb, u32 start_lg_entry, int size);
int mv_fw_critical_logger_dump(int ppc, u32 ppn_numb, u32 start_lg_entry, int size);
void mv_fw_keep_alive_dump(int ppc);
bool mv_fw_keep_alive_get(int ppc);

int mv_pp3_fw_ppc_num_set(int ppc_num);
int mv_pp3_fw_ppc_num_get(void);
/* SYSFS*/
int mv_pp3_fw_sysfs_init(struct kobject *fw_kobj);
int mv_pp3_fw_sysfs_exit(struct kobject *fw_kobj);

#endif /* __mv_fw_h__ */
