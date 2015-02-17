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

******************************************************************************/

#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include "spi_dev.h"
#ifndef CONFIG_OF
#include "boardEnv/mvBoardEnvLib.h"
#include "spi/mvSpi.h"
#endif
#include "voiceband/mvSysTdmSpi.h"

/* Defines */
#define SPI_MOD_NAME				"spi"

static ssize_t spi_read(struct file *file, char __user *buf, size_t size, loff_t *ppos);
static ssize_t spi_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos);
static unsigned int spi_poll(struct file *pFile, poll_table *pPollTable);
static long spi_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg);
static int spi_open(struct inode *pInode, struct file *pFile);
static int spi_release(struct inode *pInode, struct file *pFile);

/* SPI-API Dispatchers */
static int spi_read_reg(unsigned long arg);
static int spi_write_reg(unsigned long arg);

/* Structs */
static const struct file_operations spi_fops = {
	.owner		= THIS_MODULE,
	.llseek		= NULL,
	.read		= spi_read,
	.write		= spi_write,
	.poll		= spi_poll,
	.unlocked_ioctl	= spi_ioctl,
	.open		= spi_open,
	.release	= spi_release,
	.fasync		= NULL
};

/* Globals */
static struct miscdevice spi_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SPI_MOD_NAME,
	.fops = &spi_fops,
};

static ssize_t spi_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static ssize_t spi_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	return 0;
}

static unsigned int spi_poll(struct file *pFile, poll_table *pPollTable)
{
	return 0;
}

static long spi_ioctl(struct file *pFile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	/* Argument checking */
	if (_IOC_TYPE(cmd) != SPI_MOD_IOCTL_MAGIC) {
		printk(KERN_ERR "%s: invalid SPI MOD Magic Num %i %i\n", __func__, _IOC_TYPE(cmd), SPI_MOD_IOCTL_MAGIC);
		return -ENOTTY;
	}

	if ((_IOC_NR(cmd) > SPI_MOD_IOCTL_MAX) || (_IOC_NR(cmd) < SPI_MOD_IOCTL_MIN)) {
		printk(KERN_ERR "%s: invalid SPI MOD IOCTL request\n", __func__);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));

	if (ret) {
		printk(KERN_ERR "%s: invalid SPI MOD access type %i from cmd %i\n", __func__, _IOC_DIR(cmd), cmd);
		return -EFAULT;
	}

	switch (cmd) {
	case SPI_MOD_IOX_REG_READ:
		/*printk("ioctl: SPI_MOD_IOX_REG_READ\n");*/
		ret = spi_read_reg(arg);
		break;

	case SPI_MOD_IOX_REG_WRITE:
		/*printk("ioctl: SPI_MOD_IOX_REG_WRITE\n");*/
		ret = spi_write_reg(arg);
		break;

	default:
		printk(KERN_ERR "%s: error, ioctl command(0x%x) not supported !!!\n", __func__, cmd);
		ret = -EFAULT;
		break;
	}

	return ret;
}

static int spi_read_reg(unsigned long arg)
{
	SpiModRWObjType data;

	/* Get user data */
	if (copy_from_user(&data, (void *)arg, sizeof(SpiModRWObjType))) {
		printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	mvSysTdmIntDisable(data.lineId);

	mvSysTdmSpiRead(data.lineId, data.pCmdBuff, data.cmdSize, data.pDataBuff, data.dataSize, data.spiType);

	mvSysTdmIntEnable(data.lineId);

	/* Copy status back to user */
	if (copy_to_user((void *)arg, &data, sizeof(SpiModRWObjType))) {
		printk(KERN_ERR "%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int spi_write_reg(unsigned long arg)
{
	SpiModRWObjType data;

	/* Get user data */
	if (copy_from_user(&data, (void *)arg, sizeof(SpiModRWObjType))) {
		printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	mvSysTdmIntDisable(data.lineId);

	mvSysTdmSpiWrite(data.lineId, data.pCmdBuff, data.cmdSize, data.pDataBuff, data.dataSize, data.spiType);

	mvSysTdmIntEnable(data.lineId);

	/* Copy status back to user */
	if (copy_to_user((void *)arg, &data, sizeof(SpiModRWObjType))) {
		printk(KERN_ERR "%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int spi_open(struct inode *pInode, struct file *pFile)
{
	try_module_get(THIS_MODULE);
	return 0;
}

static int spi_release(struct inode *pInode, struct file *pFile)
{
	module_put(THIS_MODULE);
	return 0;
}

int __init spi_module_init(void)
{
	int status = 0;

	printk(KERN_INFO "Loading Marvell %s device\n", SPI_MOD_NAME);

	status = misc_register(&spi_misc_dev);

	/* Register SPI device module */
	if (status < 0) {
		printk(KERN_ERR "Error, failed to load %s module(%d)\n", SPI_MOD_NAME, status);
		return status;
	}
	return 0;
}

void __exit spi_module_exit(void)
{
	printk(KERN_INFO "Unloading %s device module\n", SPI_MOD_NAME);

	/* Unregister SPI misc device */
	misc_deregister(&spi_misc_dev);

	return;
}

/* Module stuff */
module_init(spi_module_init);
module_exit(spi_module_exit);
MODULE_DESCRIPTION("SPI Access Device");
MODULE_AUTHOR("Nadav Haklai <nadavh@marvell.com>");
MODULE_LICENSE("GPL");

