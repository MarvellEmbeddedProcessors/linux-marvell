
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
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>

#include "mv_fw.h"




int mv_pp3_fw_read_img_file(char *path, struct mem_image *img)
{
	/* Format is: u32 address chars, : u32 data*/
	int fd;
	int tmp = 0;
	u32 addr = 0;
	u32 value = 0;
	char ch = 0;
	int size = 0;
	char buf[MV_FW_MAX_LINE_SIZE] = "\0";
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);

	fd = sys_open(path, O_RDONLY, 0);
	if (fd >= 0) {
		while ((tmp = sys_read(fd, &ch, 1)) > 0) {
			if (size >= MV_FW_MAX_LINE_SIZE || ch == '\n') {
				sscanf(buf, "%X:%X", &addr, &value);
				img->rows[img->size].address = addr;
				img->rows[img->size].data = value;
				img->size++;
				pr_info("0x%08X->0x%08X", addr, value);
				size = 0;
			} else {
				buf[size++] = ch;
			}
		}
		if (size > 0)
			sscanf(buf, "%X:%X", &(img->rows[size].address), &(img->rows[size].data));

		sys_close(fd);

	} else {
		pr_err("Failed to open file %s\n", path);
		return -EIO;
	}

	set_fs(old_fs);
	pr_info("Rows Read %d\n", img->size);

	return img->size;
}



int mv_pp3_fw_read_file(char *path, char *buf, int image_size)
{
	int fd;
	int tmp = 0;
	int size = 0;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);

	fd = sys_open(path, O_RDONLY, 0);
	if (fd >= 0) {
		while ((tmp =
			sys_read(fd, &buf[size], image_size - size)) > 0) {
			size += tmp;
		}
		sys_close(fd);
	} else {
		pr_err("Failed to open file %s\n", path);
		return -EIO;
	}

	set_fs(old_fs);

	pr_info("Read %d bytes\n", size);

	return size;
}


int mv_pp3_fw_write_file(char *path, char *buf, int buf_size)
{
	int fd;
	int tmp = 0;
	int size = 0;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);

	fd = sys_open(path, (O_CREAT | O_WRONLY), 0);
	if (fd >= 0) {
		while ((size < buf_size) && (tmp =
			sys_write(fd, &buf[size], buf_size - size)) > 0) {
			size += tmp;
		}
		sys_close(fd);
	} else {
		pr_err("Failed to open file %s\n", path);
		return -EIO;
	}

	set_fs(old_fs);

	pr_info("Written: %d bytes\n", size);

	return size;
}

int mv_pp3_profile_download(char *path)
{
	struct mem_image mem;

	memset(&mem, 0, sizeof(mem));

	pr_info("DOWNLOAD PATH: %s\n", path);

	mem.rows = kzalloc(MV_PP3_PROFILE_MEM_ROWS * sizeof(struct mem_rec), GFP_KERNEL);
	mem.allocated = MV_PP3_PROFILE_MEM_ROWS;

	if (!mem.rows) {
		pr_err("FW Image Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d Rows\n", mem.allocated);

	mv_pp3_fw_read_img_file(path, &mem);
	if (mem.size > 0)
		mv_fw_mem_img_write(&mem,  PP3_PROFILE_MEM);

	kfree(mem.rows);

	return 0;
}

int mv_pp3_profile_dump(char *path)
{
	char *buf = NULL;
	int size = 0;

	pr_info("DUMP PATH: %s\n", path);

	buf = kzalloc(MV_PP3_PROFILE_MEM_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("FW Image Buffer Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d\n", MV_PP3_PROFILE_MEM_SIZE);

	size = mv_fw_mem_read(buf, MV_PP3_PROFILE_MEM_SIZE, PP3_PROFILE_MEM);
	if (size > 0)
		size = mv_pp3_fw_write_file(path, buf, MV_PP3_PROFILE_MEM_SIZE);

	kfree(buf);

	return 0;
}

int mv_pp3_cfg_dump(char *path)
{
	char *buf = NULL;
	int size = 0;

	pr_info("DUMP PATH: %s\n", path);

	buf = kzalloc(MV_PP3_CFG_MEM_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("FW Image Buffer Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d\n", MV_PP3_CFG_MEM_SIZE);

	size = mv_fw_mem_read(buf, MV_PP3_CFG_MEM_SIZE, PP3_CFG_MEM);
	if (size > 0)
		size = mv_pp3_fw_write_file(path, buf, MV_PP3_CFG_MEM_SIZE);

	kfree(buf);

	return 0;
}

int mv_pp3_cfg_download(char *path)
{
	char *buf = NULL;
	int size = 0;

	pr_info("DOWNLOAD PATH: %s\n", path);

	buf = kzalloc(MV_PP3_CFG_MEM_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("FW Image Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d\n", MV_PP3_CFG_MEM_SIZE);

	size = mv_pp3_fw_read_file(path, buf, MV_PP3_CFG_MEM_SIZE);
	if (size > 0)
		mv_fw_mem_write(buf, size, PP3_CFG_MEM);

	kfree(buf);

	return 0;
}


int mv_pp3_ppn_run(char *path)
{

	return 0;
}



int mv_pp3_imem_download(char *path)
{
	struct mem_image mem;

	memset(&mem, 0, sizeof(mem));

	pr_info("DOWNLOAD PATH: %s\n", path);

	mem.rows = kzalloc(MV_PP3_FW_MEM_ROWS * sizeof(struct mem_rec), GFP_KERNEL);
	mem.allocated = MV_PP3_FW_MEM_ROWS;

	if (!mem.rows) {
		pr_err("FW Image Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d Rows\n", mem.allocated);

	mv_pp3_fw_read_img_file(path, &mem);
	if (mem.size > 0)
		mv_fw_mem_img_write(&mem,  PP3_IMEM);

	kfree(mem.rows);

	return 0;
}


int mv_pp3_imem_dump(char *path)
{
	char *buf = NULL;
	int size = 0;

	pr_info("DUMP PATH: %s\n", path);

	buf = kzalloc(MV_PP3_FW_MEM_SIZE, GFP_KERNEL);
	if (!buf) {
		pr_err("FW Image Buffer Allocation Failed in <%s>\n", __func__);
		return -ENOMEM;
	}
	pr_info("ALLOCATED: %d\n", MV_PP3_FW_MEM_SIZE);

	size = mv_fw_mem_read(buf, MV_PP3_FW_MEM_SIZE, PP3_IMEM);
	if (size > 0)
		size = mv_pp3_fw_write_file(path, buf, MV_PP3_FW_MEM_SIZE);

	kfree(buf);

	return 0;
}



int mv_fw_mem_write(char *data, int size, enum pp3_mem_type mem_type)
{
	int i;

	/*TODO: probably verify checksum, version etc. */
	pr_info("Writing .... Type: %d size: %d [showing 0..63]\n",
		mem_type, size);

	for (i = 0; i < size; i++)
		pr_cont("%02X ", data[i]);

	pr_info("Download SUCCESSFULL: Type: %d size: %d [showing 0..63]\n",
		mem_type, size);
	return 0;
}


int mv_fw_mem_img_write(struct mem_image *img, enum pp3_mem_type mem_type)
{
	int i;

	/*TODO: probably verify checksum, version etc. */
	pr_info("Writing .... Type: %d size: %d [showing 0..63]\n",
		mem_type, img->size);

	for (i = 0; i < img->size; i++)
		pr_info("%08X:%08X", img->rows[i].address, img->rows[i].data);

	pr_info("Download SUCCESSFULL: Type: %d size: %d [showing 0..63]\n",
		mem_type, img->size);
	return 0;
}



int mv_fw_mem_read(char *data, int size, enum pp3_mem_type mem_type)
{
	int i;

	/*TODO: probably verify checksum, version etc. */
	pr_info("Reading .... Type: %d size: %d [generating ...]\n",
		mem_type, size);

	for (i = 0; i < size; i++)
		data[i] = (char)i;

	pr_info("Read SUCCESSFULL: Type: %d size: %d [showing 0..63]\n",
		mem_type, size);
	return size;
}
