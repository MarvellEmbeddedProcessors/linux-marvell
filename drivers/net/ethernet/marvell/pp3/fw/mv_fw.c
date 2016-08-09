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
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>
#include <linux/io.h>
#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "mv_fw.h"
#include "mv_fw_regs.h"
#include "mv_fw_shared.h"
#include "fw/mv_pp3_fw_msg.h"
#include "fw/mv_pp3_fw_msg_structs.h"
#include "fw/mv_fw.h"

static struct mv_pp3 *pp3_fw_priv;
static u32 mv_pp3_fw_buffers[MV_PP3_PPN_MEM_BUFS];

static unsigned int active_ppc_num;
static void __iomem *apb_base_addr;
static unsigned char *mv_pp3_fw_path;
static bool mv_pp3_fw_is_available_bool;

/* active PPNs mask per cluster */
static u32 ppc_ppn_mask[MV_PP3_PPC_MAX_NUM];

/* array to store virtual DRAM buffers addresses allocated by host for FW */
static unsigned int host_to_nss_dram_addr[MV_PP3_PPC_MAX_NUM][LAST_DRAM_BUFFER];
/* array to store physical DRAM buffers addresses allocated by host for FW */
static unsigned int fw_nss_dram_addr[MV_PP3_PPC_MAX_NUM][LAST_DRAM_BUFFER];

static bool fw_ec_eng2apb_response_status_check(void);

/* Send request for memory buffer size needed by FW */
int mv_pp3_fw_memory_alloc(struct mv_pp3 *priv)
{
	int i, fw_mem, buf_num;
	u32 fw_buffs[MV_PP3_PPN_MEM_BUFS];

	/* Send request for memory buffer size needed by FW */
	fw_mem = pp3_fw_mem_bufs_alloc_size_get();

	buf_num = mv_memory_buffer_alloc(fw_mem, MV_PP3_PPN_MEM_BUFS, mv_pp3_fw_buffers);
	if (!buf_num) {
		pr_err("%s: failed to allocate %d kbytes requested by FW.\n", __func__, fw_mem);
		return -1;
	}
	for (i = 0; i < buf_num; i++) {
		dma_addr_t phys_addr = dma_map_single(mv_pp3_dev_get(priv),
				(void *)(mv_pp3_fw_buffers[i]), fw_mem/buf_num, DMA_TO_DEVICE);

		fw_buffs[i] = cpu_to_be32((u32)phys_addr);
	}
	if (pp3_fw_mem_bufs_alloc_set(fw_mem, buf_num, fw_buffs) < 0) {
		pr_err("%s: pp3 cannot allocate buffers for FW", __func__);
		return -1;
	}
	pr_info("Allocate %d kbytes by FW request\n", fw_mem);

	return 0;
}

void mv_pp3_fw_memory_free(struct mv_pp3 *priv)
{
	int j;

	for (j = 0; j < MV_PP3_PPN_MEM_BUFS; j++)
		kfree((void *)mv_pp3_fw_buffers[j]);

}


int mv_pp3_fw_ppc_num_set(int ppc_num)
{
	if (ppc_num > MV_PP3_PPC_MAX_NUM) {
		pr_err("Invalid value %d, system suport up to %d PPCs\n",
				ppc_num, MV_PP3_PPC_MAX_NUM);
		return -EINVAL;
	}
	active_ppc_num = ppc_num;
	return 0;
}


int mv_pp3_fw_ppc_num_get(void)
{
	return active_ppc_num;
}


int mv_pp3_fw_parse_img_file(const char *image, int image_size, struct mem_image *img)
{
	int i, size;
	char ch;
	char buf[MV_FW_MAX_LINE_SIZE] = "\0";

	size = 0;
	for (i = 0; i < image_size; i++) {
		ch = image[i];
		if ((size >= MV_FW_MAX_LINE_SIZE) || (ch == '\n')) {
			sscanf(buf, "%X:%X", &img->rows[img->size].address,
					&img->rows[img->size].data);
			img->size++;
			size = 0;
			if (img->size >= img->allocated) {
				pr_warn("FW image is too large: %d bytes, %d lines\n",
					image_size, img->size);
				break;
			}
			continue;
		}
		buf[size++] = ch;
	}
	if (size > 0)
		sscanf(buf, "%X:%X", &(img->rows[img->size].address), &(img->rows[img->size].data));

	return img->size;
}

int mv_pp3_fw_request_file(char *path, struct mem_image *img)
{
	int lines;
	const struct firmware *fw = NULL;

	if (request_firmware(&fw, path, mv_pp3_dev_get(pp3_fw_priv))) {
		pr_err("Failed to load firmware file: %s\n", path);
		return -ENOENT;
	}
	lines = mv_pp3_fw_parse_img_file(fw->data, fw->size, img);
	pr_info("Load firmware file %s: %d bytes, %d lines\n", path, fw->size, lines);
	release_firmware(fw);
	return lines;
}

/* imem and profile table FW load to ppc unit*/
static int mv_pp3_ppc_fw_image_load(struct mem_image *ptr_to_image, int ppc)
{
	unsigned int data, size, curr_val, mask;
	void __iomem *curr_apb_address;
	u32 *addr = NULL;
	enum ppc_mem_type type;

	size = ptr_to_image->size;
	type = ptr_to_image->type;

	if ((type != PPC_MEM_IMEM) && (type != PPC_MEM_PROF)) {
		pr_err("%s: wronge image type\n", __func__);
		return -1;
	}

	curr_apb_address = apb_base_addr + ptr_to_image->offset +  MV_DP_PPC_BASE(ppc);

	mask = ptr_to_image->mask;

	for (curr_val = 0; curr_val < size; curr_val++)	{
		data = ptr_to_image->rows[curr_val].data;
		addr = (u32 *)(curr_apb_address + (ptr_to_image->rows[curr_val].address & mask));
		mv_pp3_hw_reg_write(addr, data);
	}

	/* WA - must add read after sequence of memory writes */
	if (addr)
		mv_pp3_hw_reg_read(addr);

	return 0;
}


/* search engine FW load */
static int mv_pp3_se_cfg_fw_load(struct mem_image *ptr_to_image)
{
	unsigned int data, size, curr_val, mask;
	void __iomem *addr, *curr_apb_address;

	size = ptr_to_image->size;

	curr_apb_address = apb_base_addr + ptr_to_image->offset;

	mask = ptr_to_image->mask;

	for (curr_val = 0; curr_val < size; curr_val++)	{

		data = ptr_to_image->rows[curr_val].data;
		addr =  curr_apb_address + (ptr_to_image->rows[curr_val].address & mask);
		mv_pp3_hw_reg_write(addr, data);

		if (addr == (MV_EC_APB2ENG_REQ_ADDR_CNTRL_REG + apb_base_addr))
			/* - read status and wait for done to be set */
			if (!fw_ec_eng2apb_response_status_check())
				return -1;
	}
	return 0;
}

int mv_pp3_get_path(char *path, int len, char *dir, char *file_name)
{
    /* in case the dir initialized load FW from file system */
	if (dir) {
		/* check that len does not exceed max len */
		if (strlen(dir) >= len) {
			pr_err("Failed to initialize file %s path <%s>\n", file_name, __func__);
			return -ENOMEM;
		}
		strcpy(path, dir);
	} else
		/* in case no path initialize point to kernel dir of the FW */
		strcpy(path, "pp3_gnss/");

	/* check path len */
	if (strlen(path) + strlen(file_name) >= len) {
			pr_err("Failed to initialize file %s path <%s>\n", file_name, __func__);
			return -ENOMEM;
	}
	strcat(path, file_name);
	return 0;
}


int mv_pp3_se_fw_image_download(char *dir)
{
	char path[MV_MAX_FW_FILE_PATH] = "\0";
	struct mem_image mem;
	int rc;

	memset(&mem, 0, sizeof(mem));

	rc = mv_pp3_get_path(path, MV_MAX_FW_FILE_PATH, dir, "se_image.txt");
	if (rc != 0)
		return -1;

	mem.offset = MV_NSS_SE_OFFS;
	mem.mask = MV_NSS_SE_MASK;

	pr_info("Firmware file to load: %s\n", path);
	mem.rows = kzalloc(MV_PP3_FW_MAX_ROWS * sizeof(struct mem_rec), GFP_KERNEL);
	mem.allocated = MV_PP3_FW_MAX_ROWS;

	if (!mem.rows) {
		pr_err("FW Image Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	/* request firmware from the FS or internal files */
	rc = mv_pp3_fw_request_file(path, &mem);
	/* check read size is legal */
	if ((rc < 0) || (mem.size == 0)) {
		kfree(mem.rows);
		return -ENOENT;
	}

	if (mv_pp3_se_cfg_fw_load(&mem) < 0) {
		pr_err("search engine fw load Failed\n");
		kfree(mem.rows);
		return -1;
	}

	kfree(mem.rows);

	return 0;
}


int mv_pp3_ppc_fw_image_download(int ppc, char *dir, unsigned int mem_type)
{
	char path[MV_MAX_FW_FILE_PATH] = "\0";
	char file[MV_MAX_FW_FILE_PATH] = "\0";
	struct mem_image mem;
	int rc;

	memset(&mem, 0, sizeof(mem));

	switch (mem_type) {
	case PPC_MEM_IMEM:
		strcat(file, "imem_addr_data.txt");
		mem.offset = MV_PPC_IMEM_OFFS;
		mem.mask = MV_PPC_IMEM_MASK;
	break;
	case PPC_MEM_PROF:
		strcat(file, "pt_addr_data.txt");
		mem.offset = MV_PPC_PROF_MEM_OFFS;
		mem.mask = MV_PPC_PROF_MEM_MASK;
	break;

	default:
		pr_err("Unexpected firmware file type%d\n", mem_type);
		return -ENOENT;
	}

	rc = mv_pp3_get_path(path, MV_MAX_FW_FILE_PATH, dir, file);
	if (rc != 0)
		return -ENOENT;

	pr_info("Firmware file to load: %s\n", path);
	mem.type = mem_type;
	mem.rows = kzalloc(MV_PP3_FW_MAX_ROWS * sizeof(struct mem_rec), GFP_KERNEL);
	mem.allocated = MV_PP3_FW_MAX_ROWS;

	if (!mem.rows) {
		pr_err("FW Image Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	/* request firmware from the FS or internal files */
	rc = mv_pp3_fw_request_file(path, &mem);
	/* check read size is legal */
	if ((rc < 0) || (mem.size == 0)) {
		kfree(mem.rows);
		return -ENOENT;
	}

	if (mv_pp3_ppc_fw_image_load(&mem, ppc) < 0) {
		pr_err("PPC %d fw load Failed\n", ppc);
		kfree(mem.rows);
		return -1;
	}

	kfree(mem.rows);

	return 0;
}

/* size - number of words to print */
static void mv_fw_dump_print(u32 dram_offset, u32 size)
{
	u32 num0, num1, num2, num3;
	u32 numb_of_strings, i;
	u32 *ptr_to_sp;

	numb_of_strings = size/4; /* 4 words in output line */
	if (size % 4)
		numb_of_strings++;
	ptr_to_sp = (u32 *)dram_offset;
	for (i = 0; i < numb_of_strings*4; i = i+4) {
		num0 = cpu_to_be32(*(ptr_to_sp + i));
		num1 = cpu_to_be32(*(ptr_to_sp + i + 1));
		num2 = cpu_to_be32(*(ptr_to_sp + i + 2));
		num3 = cpu_to_be32(*(ptr_to_sp + i + 3));
		pr_info("0x%8p:  0x%08x  0x%08x  0x%08x  0x%08x\n", (ptr_to_sp + i),  num0, num1, num2, num3);
	}
}

/* size - number of words to print */
int mv_fw_pkts_rec_dump(int ppc, u32 offset, int size)
{
	u32 dram_offset;

	if (offset > PACKET_RECORDING_DRAM_BUFFER_SIZE) {
		pr_err("Wrong  entry offset %d\n", offset);
		return -1;
	}
	if (offset + size * 4 > PACKET_RECORDING_DRAM_BUFFER_SIZE) {
		pr_err("Wrong size %d\n", size);
		return -1;
	}
	pr_info("PPC%d:: FW messages/packets buffer log start_entry: %d numb of words: %d\n",
		ppc, offset, size);
	/*calculate adr of logger start entry */
	dram_offset = host_to_nss_dram_addr[ppc][PACKET_RECORDING_BUFFER] + offset;
	mv_fw_dump_print(dram_offset, size);

	return 0;
}

/* size - nuber of words to print */
int mv_fw_sp_dump(int ppc, u32 ppn_numb, u32 buf_index, u32 start_sp_adr, u32 size)
{
	u32 dram_offset;

	if (ppn_numb > 15) {
		pr_err("Wrong PPN numb %d\n", ppn_numb);
		return -1;
	}
	if (buf_index > MAX_SP_MIRROR_BUF_NUMB) {
		pr_err("Wrong  SP address %x\n", start_sp_adr);
		return -1;
	}
	if (start_sp_adr > SP_SIZE) {
		pr_err("Wrong  SP address %x\n", start_sp_adr);
		return -1;
	}
	if (start_sp_adr + size * 4 > SP_SIZE) {
		pr_err("Wrong size %d\n", size);
		return -1;
	}

	pr_info("PPC%d:: PPN #%d SP start_address: %x size: %d\n", ppc, ppn_numb, start_sp_adr, size);
	/*calculate adr of SP image in DRAM */
	dram_offset = host_to_nss_dram_addr[ppc][SP_MIRRORING_BUFFER] +
		ppn_numb * SP_MIRRORING_PER_PPN_DRAM_BUFFER_SIZE + buf_index * SP_SIZE + start_sp_adr;
	mv_fw_dump_print(dram_offset, size);

	return 0;

}

/* size - number of entries to print */
int mv_fw_inf_logger_dump(int ppc, u32 ppn_numb, u32 start_lg_entry, int size)
{
	u32 dram_offset;

	if (ppn_numb > 15) {
		pr_err("Wrong PPN numb %d\n", ppn_numb);
		return -1;
	}
	if (start_lg_entry > MAX_INF_LOG_ENTRY_NUMB) {
		pr_err("Wrong  entry numb %d\n", start_lg_entry);
		return -1;
	}
	if (start_lg_entry + size > MAX_INF_LOG_ENTRY_NUMB) {
		pr_err("Wrong size %d\n", size);
		return -1;
	}

	pr_info("PPC%d:: PPN #%d info logger start_entry: %d numb of logs: %d\n",
		ppc, ppn_numb, start_lg_entry, size);
	/*calculate adr of logger start entry */
	dram_offset = host_to_nss_dram_addr[ppc][LOGGER_BUFFER] +
		ppn_numb * INFORMATION_LOGGER_PER_PPN_DRAM_BUFFER_SIZE + start_lg_entry * LOGGER_ENTRY_SIZE;
	mv_fw_dump_print(dram_offset, (size*LOGGER_ENTRY_SIZE) / 4);

	return 0;
}

int mv_fw_critical_logger_dump(int ppc, u32 ppn_numb, u32 start_lg_entry, int size)
{
	u32 dram_offset;

	if (ppn_numb > 15) {
		pr_err("Wrong PPN numb %d\n", ppn_numb);
		return -1;
	}
	if (start_lg_entry > MAX_CRITICAL_LOG_ENTRY_NUMB) {
		pr_err("Wrong  entry numb %d\n", start_lg_entry);
		return -1;
	}
	if (start_lg_entry + size > MAX_CRITICAL_LOG_ENTRY_NUMB) {
		pr_err("Wrong size %d\n", size);
		return -1;
	}

	pr_info("PPC%d:: PPN #%d critical logger start_entry: %d numb of entries: %d\n",
		ppc, ppn_numb, start_lg_entry, size);
	/*calculate adr of logger start entry */
	dram_offset = host_to_nss_dram_addr[ppc][LOGGER_BUFFER] +  LOGGER_CRITICAL_DRAM_OFFSET +
		ppn_numb * CRITICAL_LOGGER_PER_PPN_DRAM_BUFFER_SIZE + start_lg_entry * LOGGER_ENTRY_SIZE;
	mv_fw_dump_print(dram_offset, (size*LOGGER_ENTRY_SIZE) / 4);

	return 0;
}

void mv_fw_keep_alive_dump(int ppc)
{
	int i;
	u32 dram_offset, num;
	u32 *ptr_to_keep_alive_array;

	pr_info("PPC%d Keep Alive array contents:\n", ppc);
	/*calculate start adr of keep alive array */
	dram_offset = host_to_nss_dram_addr[ppc][KEEP_ALIVE_BUFFER];

	ptr_to_keep_alive_array = (u32 *)dram_offset;
	for (i = 0; i < MV_NUM_OF_PPN; i++) {
		num = cpu_to_be32(*(ptr_to_keep_alive_array + i));
		pr_info("PPN #%2d:   0x%08x\n", i, num);
	}
}

static bool fw_ec_eng2apb_response_status_check(void)
{
	int count_down = 100;
	u32 rq_status;

	/* - read status and wait for done to be set */
	do {
		rq_status = mv_pp3_hw_reg_read(apb_base_addr + MV_EC_ENG2APB_RESPONSE_STATUS_REG);
	} while (((rq_status & 0x4) != 4) && (count_down--));

	if ((count_down == 0) && ((rq_status & 0x4) != 4)) {
		pr_err("%s: no response from EC engine", __func__);
		return false;
	}
	return true;
}


static int mv_pp3_ppc_dram_allocation(int ppc)
{
	dma_addr_t nss_dram_addr;
	u32 host_addr;

	/* allocate SP_MIRRORING_DRAM_BUFFER */

	host_addr = (u32)dma_alloc_coherent(mv_pp3_dev_get(pp3_fw_priv),
				SP_MIRRORING_DRAM_BUFFER_SIZE, &nss_dram_addr, GFP_KERNEL);
	if (!host_addr) {
		pr_err("Can't allocate %d bytes of coherent memory for SP_MIRRORING_BUFFER on PPC #%d\n",
			SP_MIRRORING_DRAM_BUFFER_SIZE, ppc);
		return -ENOMEM;
	}
	pr_info("0x%x bytes of coherent memory allocated for PPC%d::SP_MIRRORING_BUFFER     : vaddr=0x%x, paddr=0x%x\n",
		SP_MIRRORING_DRAM_BUFFER_SIZE, ppc, host_addr, (unsigned int)nss_dram_addr);
	host_to_nss_dram_addr[ppc][SP_MIRRORING_BUFFER] = host_addr;
	fw_nss_dram_addr[ppc][SP_MIRRORING_BUFFER] = cpu_to_be32(nss_dram_addr);

	/* allocate PACKET_RECORDING_BUFFER */
	host_addr = (u32)dma_alloc_coherent(mv_pp3_dev_get(pp3_fw_priv), PACKET_RECORDING_DRAM_BUFFER_SIZE,
						&nss_dram_addr, GFP_KERNEL);
	if (!host_addr) {
		pr_err("Can't allocate %d bytes of coherent memory for PACKET_RECORDING_BUFFER on PPC #%d\n",
			PACKET_RECORDING_DRAM_BUFFER_SIZE, ppc);
		return -ENOMEM;
	}
	pr_info("0x%x bytes of coherent memory allocated for PPC%d::PACKET_RECORDING_BUFFER : vaddr=0x%x, paddr=0x%x\n",
		PACKET_RECORDING_DRAM_BUFFER_SIZE, ppc, host_addr, (unsigned int)nss_dram_addr);
	host_to_nss_dram_addr[ppc][PACKET_RECORDING_BUFFER] = host_addr;
	fw_nss_dram_addr[ppc][PACKET_RECORDING_BUFFER] = cpu_to_be32(nss_dram_addr);

	/* allocate LOGGER_BUFFER */
	host_addr = (u32)dma_alloc_coherent(mv_pp3_dev_get(pp3_fw_priv), LOGGER_BUFFER_SIZE,
						&nss_dram_addr, GFP_KERNEL);
	if (!host_addr) {
		pr_err("Can't allocate %d bytes of coherent memory for LOGGER_BUFFER on PPC #%d\n",
			LOGGER_BUFFER_SIZE, ppc);
		return -ENOMEM;
	}
	pr_info("0x%x bytes of coherent memory allocated for PPC%d::LOGGER_BUFFER           : vaddr=0x%x, paddr=0x%x\n",
		LOGGER_BUFFER_SIZE, ppc, host_addr, (unsigned int)nss_dram_addr);
	host_to_nss_dram_addr[ppc][LOGGER_BUFFER] = host_addr;
	fw_nss_dram_addr[ppc][LOGGER_BUFFER] = cpu_to_be32(nss_dram_addr);
	pr_info("  LOGGER_INFORMATION_DRAM_OFFSET  0x%x\n", host_addr);
	pr_info("  LOGGER_CRITICAL_DRAM_OFFSET     0x%x\n", host_addr + LOGGER_CRITICAL_DRAM_OFFSET);

	/* allocate KEEP_ALIVE_BUFFER */
	host_addr = (u32)dma_alloc_coherent(mv_pp3_dev_get(pp3_fw_priv), KEEP_ALIVE_BUFFER_SIZE,
						&nss_dram_addr, GFP_KERNEL);
	if (!host_addr) {
		pr_err("Can't allocate %d bytes of coherent memory for KEEP_ALIVE_BUFFER on PPC #%d\n",
			KEEP_ALIVE_BUFFER_SIZE, ppc);
		return -ENOMEM;
	}
	pr_info("0x%x bytes of coherent memory allocated for PPC%d::KEEP_ALIVE_BUFFER       : vaddr=0x%x, paddr=0x%x\n",
		KEEP_ALIVE_BUFFER_SIZE, ppc, host_addr, (unsigned int)nss_dram_addr);
	host_to_nss_dram_addr[ppc][KEEP_ALIVE_BUFFER] = host_addr;
	fw_nss_dram_addr[ppc][KEEP_ALIVE_BUFFER] = cpu_to_be32(nss_dram_addr);

	return 0;
}

static void mv_pp3_ppc_dram_buffers_set(int ppc)
{
	void __iomem *apb_shared_sram_adr;

	/* write physical address to Shared SRAM */
	apb_shared_sram_adr = mv_pp3_nss_regs_vaddr_get() + MV_DP_PPC_BASE(ppc) +
				MV_PPC_SHARED_MEM_OFFS + DRAM_FOR_FW_SHARED_SRAM_OFFSET;
	pr_info(" shared_sram_adr %p\n", apb_shared_sram_adr);

	mv_pp3_hw_write(apb_shared_sram_adr, LAST_DRAM_BUFFER, fw_nss_dram_addr[ppc]);
}

/* search engine FW init */
int mv_pp3_se_fw_init(void)
{
	int err = -ENOENT;
	if (mv_pp3_fw_path)
		err = mv_pp3_se_fw_image_download(mv_pp3_fw_path);
	if (err) {
		err = mv_pp3_se_fw_image_download(NULL);
		if (err)
			return err;
	}

	/* WA for SE bug */
	err = mv_pp3_fw_half_range_addr_cfg();

	return err;

}

/* ppc FW load - imem and profile table */
int mv_pp3_ppc_fw_load(int ppc)
{
	int err = -ENOENT;

	if (mv_pp3_fw_path)
		err = mv_pp3_ppc_fw_image_download(ppc, mv_pp3_fw_path, PPC_MEM_IMEM);
	if (err) {
		err = mv_pp3_ppc_fw_image_download(ppc, NULL, PPC_MEM_IMEM);
		if (err)
			return err;
	}
	err = -ENOENT;
	if (mv_pp3_fw_path)
		err = mv_pp3_ppc_fw_image_download(ppc, mv_pp3_fw_path, PPC_MEM_PROF);
	if (err) {
		err = mv_pp3_ppc_fw_image_download(ppc, NULL, PPC_MEM_PROF);
		if (err)
			return err;
	}

	mv_pp3_ppc_dram_buffers_set(ppc);

	return 0;
}

int mv_pp3_fw_load(void)
{
	int  err, ppc = 0;

	/* init base address */
	apb_base_addr = mv_pp3_nss_regs_vaddr_get();

	/*load FW to all PPCs */
	for (ppc = 0; ppc < active_ppc_num; ppc++) {
		err = mv_pp3_ppc_fw_load(ppc);
		if (err) {
			pr_err("%s: FW load to ppc %d failed\n", __func__, ppc);
			return -1;
		}
	}

	/* load FW to search engine */
	err = mv_pp3_se_fw_init();
	if (err) {
		pr_err("%s: FW load to search engine failed\n", __func__);
		return -1;
	}
	mv_pp3_fw_is_available_bool = true;
	return 0;
}

bool mv_pp3_fw_is_available(void)
{
	return mv_pp3_fw_is_available_bool;
}

/* set path for FW files download */
/* allocate buffer with additional place for file name concatination */

void mv_pp3_fw_path_set(u8 *path)
{
	int s_size = strlen(path) + sizeof("imem_addr_data.txt");

	if (!mv_pp3_fw_path)
		kfree(mv_pp3_fw_path);

	mv_pp3_fw_path = kmalloc(s_size, GFP_KERNEL);
	if (!mv_pp3_fw_path)
		return;

	strcpy(mv_pp3_fw_path, path);
}

int mv_pp3_fw_half_range_addr_cfg(void)
{
	unsigned int resp_data[MV_EC_ENG2APB_DATA_REGS_NUM];
	void __iomem *curr_apb_address;
	int i;

	/* clear the status register */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_ENG2APB_RESPONSE_STATUS_REG, 7);
	/* request opcode */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_APB2ENG_REQ_OPCODE_REG, 1);
	/* address 0x50, trigger, SE_dual */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_APB2ENG_REQ_ADDR_CNTRL_REG, 0x01500050);
	/* read status and wait for done to be set */
	if (!fw_ec_eng2apb_response_status_check())
		return -1;

	/* read response data */
	/* resp_data[0] = word 3 with address 0xd7080, resp_data[3] = word 0 with address 0xd708c */
	for (i = 0; i < MV_EC_ENG2APB_DATA_REGS_NUM; i++)
		resp_data[i] = mv_pp3_hw_reg_read(apb_base_addr + MV_EC_ENG2APB_RESPONSE_DATA3_REG + (i * 4));

	/* clear the status register */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_ENG2APB_RESPONSE_STATUS_REG, 7);
	/* request opcode */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_APB2ENG_REQ_OPCODE_REG, 2);

	/* resp_data[0] = word 3 with address 0xd7080 */
	/* clear bit 5 in byte 13 */
	resp_data[0] = resp_data[0] & ~(1 << 13);
	/* write new configuration */
	for (i = 0; i < MV_EC_ENG2APB_DATA_REGS_NUM; i++) {
		curr_apb_address = apb_base_addr + MV_EC_APB2ENG_REQUEST_DATA3_REG + (i * 4);
		mv_pp3_hw_reg_write(curr_apb_address, resp_data[i]);
	}
	/* address 0x50, trigger, SE_dual */
	mv_pp3_hw_reg_write(apb_base_addr + MV_EC_APB2ENG_REQ_ADDR_CNTRL_REG, 0x01500050);
	/* read status and wait for done to be set */
	if (!fw_ec_eng2apb_response_status_check())
		return -1;

	return 0;
}

static void mv_pp3_ppc_run(int ppc)
{
	void __iomem *base_addr;

	base_addr = apb_base_addr + MV_DP_PPC_BASE(ppc);
	mv_pp3_hw_reg_write(base_addr + MV_PPC_IMEM_HOLD_OFF_REG, 0);

}


void mv_pp3_ppc_run_all(void)
{
	int  ppc;

	/* calculate EAP start address */
	for (ppc = 0; ppc < active_ppc_num; ppc++)
		mv_pp3_ppc_run(ppc);

	mdelay(10);
	for (ppc = 0; ppc < active_ppc_num; ppc++) {
		ppc_ppn_mask[ppc] = mv_pp3_hw_reg_read(apb_base_addr + MV_DP_PPC_BASE(ppc) + MV_PPC_WAIT_FOR_DEQ_REG);
		pr_info("Start PPC #%d with 0x%x active PPNs mask\n", ppc, ppc_ppn_mask[ppc]);
	}
}
int mv_pp3_fw_init(struct mv_pp3 *priv)
{
	int ppc, err;

	pp3_fw_priv = priv;

	/* alloc DRAM for each active PPC */
	for (ppc = 0; ppc < active_ppc_num; ppc++) {
		err = mv_pp3_ppc_dram_allocation(ppc);
		if (err)
			return err;
	}
	return 0;

}

static int mv_pp3_ppc_idle_wait(int ppc)
{
	void __iomem *base_addr;
	u32 busy, dq;
	u32 count;

	base_addr = apb_base_addr + MV_DP_PPC_BASE(ppc);

	count = 0;
	do {
		busy = mv_pp3_hw_reg_read(base_addr + MV_PPC_BUSY_REG);
		if (busy)
			udelay(1);
		else {
			dq = mv_pp3_hw_reg_read(base_addr + MV_PPC_WAIT_FOR_DEQ_REG);
			if (dq == ppc_ppn_mask[ppc])
				return 0;
			else
				udelay(1);
		}
	} while (count < 100);
	pr_err("%s: Cannot stop PPC #%d (busy = 0x%x, dq = 0x%x)\n", __func__, ppc, busy, dq);

	return 1;
}

int mv_pp3_ppc_idle_wait_all(void)
{
	int ppc;
	int err = 0;

	for (ppc = 0; ppc < active_ppc_num; ppc++)
		err += mv_pp3_ppc_idle_wait(ppc);

	return err;
}

static u32 mv_pp3_ppn_deq_status_get(int ppc)
{
	return	mv_pp3_hw_reg_read(mv_pp3_nss_regs_vaddr_get() +
		MV_DP_PPC_BASE(ppc) +
		MV_PPC_WAIT_FOR_DEQ_REG);
}

static void mv_pp3_sram_ppn_status_set(int ppc, u8 val)
{
	u32 words[MV_NUM_OF_PPN / sizeof(u32)];
	u8 *buffer = (u8 *)words;
	u32 *shared_sram_addr;
	int i;

	for (i = 0; i < MV_NUM_OF_PPN; i++)
		*buffer++ = val;

	shared_sram_addr = (u32 *)(apb_base_addr +
				MV_DP_PPC_BASE(ppc) +
				MV_PPC_SHARED_MEM_OFFS +
				PPN_KEEPALIVE_SRAM_OFFSET);

	mv_pp3_hw_write(shared_sram_addr, (MV_NUM_OF_PPN / 4), words);
}

static void mv_pp3_sram_ppn_status_get(int ppc, u32 *buffer)
{
	u32 *shared_sram_addr;

	shared_sram_addr = (u32 *)(apb_base_addr +
				MV_DP_PPC_BASE(ppc) +
				MV_PPC_SHARED_MEM_OFFS +
				PPN_KEEPALIVE_SRAM_OFFSET);

	mv_pp3_hw_read(shared_sram_addr, (MV_NUM_OF_PPN / 4), buffer);
}

bool mv_fw_keep_alive_get(int ppc)
{
	int i;
	u32 deq_status;
	u8  sram_status[MV_NUM_OF_PPN];

	if ((ppc < 0) || (ppc >= active_ppc_num)) {
		pr_err("%s: Unexpected ppc number %d\n", __func__, ppc);
		return false;
	}

	if (ppc_ppn_mask[ppc] <= 2) /* don't check fw debug versions with 1-2 PPNs only */
		return true;

	deq_status = mv_pp3_ppn_deq_status_get(ppc); /* all 16 */

	mv_pp3_sram_ppn_status_get(ppc, (u32 *)sram_status);   /* all 16 - store in sram_status */

	for (i = 0; i < MV_NUM_OF_PPN; i++) {
		if (((deq_status & (1 << i)) == 0)  && (sram_status[i] != 0)) {
			/* second check */
			udelay(30);

			deq_status = mv_pp3_ppn_deq_status_get(ppc); /* all 16 */

			mv_pp3_sram_ppn_status_get(ppc, (u32 *)sram_status);

			if (((deq_status & (1 << i)) == 0)  && (sram_status[i] != 0)) {
				pr_info("keep alive: ppc=%d ppn=%d  failed\n", ppc, i);
				return false; /* BAD */
			}
		}
	}

	mv_pp3_sram_ppn_status_set(ppc, 0x1); /* all 16 */

	return true; /* GOOD */
}

