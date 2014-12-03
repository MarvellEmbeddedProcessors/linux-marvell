/*
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <asm/hardware/cache-l2x0.h>

static int proc_dump_cp15_read(struct seq_file *m, void *p)
{
	unsigned int value;

	asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (value));
	seq_printf(m, "Main ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (value));
	seq_printf(m, "Cache Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 2" : "=r" (value));
	seq_printf(m, "TCM Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 3" : "=r" (value));
	seq_printf(m, "TLB Type: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (value));
	seq_printf(m, "Microprocessor Affinity: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 0" : "=r" (value));
	seq_printf(m, "Processor Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 1" : "=r" (value));
	seq_printf(m, "Processor Feature 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 2" : "=r" (value));
	seq_printf(m, "Debug Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 3" : "=r" (value));
	seq_printf(m, "Auxiliary Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 4" : "=r" (value));
	seq_printf(m, "Memory Model Feature 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 5" : "=r" (value));
	seq_printf(m, "Memory Model Feature 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 6" : "=r" (value));
	seq_printf(m, "Memory Model Feature 2: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c1, 7" : "=r" (value));
	seq_printf(m, "Memory Model Feature 3: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 0" : "=r" (value));
	seq_printf(m, "Set Attribute 0: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 1" : "=r" (value));
	seq_printf(m, "Set Attribute 1: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 2" : "=r" (value));
	seq_printf(m, "Set Attribute 2: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 3" : "=r" (value));
	seq_printf(m, "Set Attribute 3: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c0, c2, 4" : "=r" (value));
	seq_printf(m, "Set Attribute 4: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (value));
	seq_printf(m, "Current Cache Size ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 1" : "=r" (value));
	seq_printf(m, "Current Cache Level ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 1, %0, c0, c0, 7" : "=r" (value));
	seq_printf(m, "Auxiliary ID: 0x%08x\n", value);

	asm volatile ("mrc p15, 2, %0, c0, c0, 0" : "=r" (value));
	seq_printf(m, "Cache Size Selection: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (value));
	seq_printf(m, "Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r" (value));
	seq_printf(m, "Auxiliary Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c0, 2" : "=r" (value));
	seq_printf(m, "Coprocessor Access Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c1, c1, 0" : "=r" (value));
	seq_printf(m, "Secure Configuration : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (value));
	seq_printf(m, "Translation Table Base 0 : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 1" : "=r" (value));
	seq_printf(m, "Translation Table Base 1 : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c2, c0, 2" : "=r" (value));
	seq_printf(m, "Translation Table Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (value));
	seq_printf(m, "Domain Access Control : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r" (value));
	seq_printf(m, "Data Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c0, 1" : "=r" (value));
	seq_printf(m, "Instruction Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c1, 0" : "=r" (value));
	seq_printf(m, "Auxiliary Data Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c5, c1, 1" : "=r" (value));
	seq_printf(m, "Auxiliary Instruction Fault Status : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r" (value));
	seq_printf(m, "Data Fault Address : 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c6, c0, 2" : "=r" (value));
	seq_printf(m, "Instruction Fault Address : 0x%08x\n", value);

	asm volatile ("mrc p15, 4, %0, c15, c0, 0" : "=r" (value));
	seq_printf(m, "Configuration Base Address: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r" (value));
	seq_printf(m, "Memory Attribute PRRR: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c10, c2, 1" : "=r" (value));
	seq_printf(m, "Memory Attribute NMRR: 0x%08x\n", value);

	asm volatile ("mrc p15, 0, %0, c15, c0, 0" : "=r" (value));
	seq_printf(m, "Power Control Register: 0x%08x\n", value);

	return 0;
}

static int proc_dump_cp15_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_dump_cp15_read, NULL);
}

static const struct file_operations proc_dump_cp15_fops = {
	.owner		= THIS_MODULE,
	.open		= proc_dump_cp15_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static struct proc_dir_entry *proc_mv_dump_dir;

int dump_init_module(void)
{
	proc_mv_dump_dir = proc_mkdir("mv_dump", NULL);
	proc_create("cp15", 0444, proc_mv_dump_dir, &proc_dump_cp15_fops);

	return 0;
}

void dump_cleanup_module(void)
{
	proc_remove(proc_mv_dump_dir);
}

module_init(dump_init_module);
module_exit(dump_cleanup_module);
