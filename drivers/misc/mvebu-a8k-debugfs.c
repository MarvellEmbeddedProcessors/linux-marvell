/*
 * debugfs entry for Marvell's Armada-8K CPU system registers access.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>

struct mvebu_a8k_state {
	struct dentry *debugfs_root;
	struct dentry *debugfs_ap;
	struct dentry *debugfs_sysregs;
};

struct sysreg_entry {
	char name[32];
	char desc[128];
	u64 (*read_func)(void);
	void (*write_func)(u64);
};

/* Control structure */
static struct mvebu_a8k_state a8k_state;

/* The below macros provide a generic way for creating system registers
** read / write function, without the need to explicitly write them.
*/
#define SYSREG_READ_ASM(v, code)		\
		asm volatile("mrs %0, " __stringify(code) : "=r" (v))

#define SYSREG_WRITE_ASM(v, code)		\
		asm volatile("msr " __stringify(code) ", %0" : : "r" (v))

#define DECLARE_RD_WR_FUNC(code)		\
	static u64 read_##code(void)		\
	{					\
		u64 v;				\
		SYSREG_READ_ASM(v, code);	\
		return v;			\
	}					\
	static void write_##code(u64 v)		\
	{					\
		SYSREG_WRITE_ASM(v, code);	\
	}

/* This will result in definining a read / write function for
** each of the system registers below.
*/
DECLARE_RD_WR_FUNC(midr_el1);
DECLARE_RD_WR_FUNC(mpidr_el1);
DECLARE_RD_WR_FUNC(sctlr_el1);
DECLARE_RD_WR_FUNC(s3_1_c15_c0_0);
DECLARE_RD_WR_FUNC(s3_1_c11_c0_2);
DECLARE_RD_WR_FUNC(s3_1_c11_c0_3);


#define SYSREG_ENTRY(code, _desc)		\
	{					\
		.name = __stringify(code),	\
		.desc = _desc,			\
		.read_func = &read_##code,	\
		.write_func = &write_##code	\
	}

static struct sysreg_entry sysregs_list[] = {
	SYSREG_ENTRY(midr_el1, "Main ID"),
	SYSREG_ENTRY(mpidr_el1, "Multiprocessor Affinity"),
	SYSREG_ENTRY(sctlr_el1, "System Ctrl"),
	SYSREG_ENTRY(s3_1_c15_c0_0, "L2 Aux Ctrl"),
	SYSREG_ENTRY(s3_1_c11_c0_2, "L2 Ctrl"),
	SYSREG_ENTRY(s3_1_c11_c0_3, "L2 Ext Ctrl"),
};


static int mvebu_ap806_sysregs_debug_show(struct seq_file *seq, void *v)
{
	u64 val;
	int i;

	seq_puts(seq, "System Registers Dump:\n");
	for (i = 0; i < ARRAY_SIZE(sysregs_list); i++) {
		seq_printf(seq, "%s\t\t- ", sysregs_list[i].name);
		val = sysregs_list[i].read_func();
		seq_printf(seq, "0x%08llx", val);
		seq_printf(seq, " (%s).\n", sysregs_list[i].desc);
	}

	return 0;
}

/*
** Write to a specific system register:
** e.g.
** # echo s3_1_c15_c0_0 0x42 > ap806/sysregs
*/
static ssize_t mvebu_ap806_sysregs_debug_write(struct file *f, const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	char buf[64];
	u32 val32;
	u64 val64;
	char *start, *end;
	int len, found, i;
	size_t cnt = min(count, sizeof(buf)-1);

	if (copy_from_user(buf, user_buf, cnt))
		return -EFAULT;
	buf[cnt] = '\0';

	start = buf;
	/* Get sysreg name */
	end = memscan(start, ' ', cnt);
	len = end - start;
	if (len > cnt) {
		pr_err("Bad command.\n");
		return count;
	}

	found = 0;
	for (i = 0; i < ARRAY_SIZE(sysregs_list); i++) {
		if (strncmp(sysregs_list[i].name, start, len))
			continue;
		found = 1;
		break;
	}

	start = end;
	/* Skip all spaces between sysreg name and value. */
	while (start[0] == ' ')
		start++;

	if (found) {
		if (kstrtou32(start, 16, &val32)) {
			pr_err("Bad value.\n");
			return count;
		}

		pr_info("Writing 0x%08x to %s.\n", val32, sysregs_list[i].name);
		val64 = (u64)val32;

		sysregs_list[i].write_func(val64);

		pr_info("New value %s - 0x%08llx.\n", sysregs_list[i].name,
				sysregs_list[i].read_func());
	} else {
		pr_err("Bad system register name.\n");
	}

	return count;
}


static int mvebu_ap806_sysregs_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mvebu_ap806_sysregs_debug_show, inode->i_private);
}

static const struct file_operations mvebu_ap806_sysregs_debug_fops = {
	.open = mvebu_ap806_sysregs_debug_open,
	.read = seq_read,
	.write = mvebu_ap806_sysregs_debug_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static __init int mvebu_a8k_debugfs_init(void)
{
	struct mvebu_a8k_state *s = &a8k_state;

	s->debugfs_root = debugfs_create_dir("mvebu-a8k", NULL);
	if (s->debugfs_root) {
		s->debugfs_ap = debugfs_create_dir("ap806", s->debugfs_root);
		s->debugfs_sysregs = debugfs_create_file("sysregs", 0660,
				s->debugfs_ap, NULL, &mvebu_ap806_sysregs_debug_fops);
	}

	return 0;
}
fs_initcall(mvebu_a8k_debugfs_init);

