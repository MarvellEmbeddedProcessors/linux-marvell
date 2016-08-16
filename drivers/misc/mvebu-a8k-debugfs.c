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
#include <linux/ioctl.h>

struct mvebu_a8k_state {
	struct dentry *debugfs_root;
	struct dentry *debugfs_ap;
	struct dentry *debugfs_sysregs;
	struct dentry *debugfs_debug;
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
DECLARE_RD_WR_FUNC(s3_1_c15_c2_1);
DECLARE_RD_WR_FUNC(s3_1_c15_c2_0);


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
	SYSREG_ENTRY(s3_1_c15_c2_1, "CPU Ext Ctrl"),
	SYSREG_ENTRY(s3_1_c15_c2_0, "CPU Aux Ctrl"),
};


static int mvebu_ap806_sysregs_debug_show(struct seq_file *seq, void *v)
{
	u64 val;
	int i;

	seq_puts(seq, "System Registers Dump:\n");
	for (i = 0; i < ARRAY_SIZE(sysregs_list); i++) {
		seq_printf(seq, "%s\t\t- ", sysregs_list[i].name);
		val = sysregs_list[i].read_func();
		seq_printf(seq, "0x%016llx", val);
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

		pr_info("New value %s - 0x%016llx.\n", sysregs_list[i].name,
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

#define MVEBU_DBG_VA2PA		_IOWR('q', 1, struct va2pa_args *)

#define VA2PA_TYPE_USER		0
#define VA2PA_TYPE_KERNEL	1

#define BMVAL(val, lsb, msb)	((val & GENMASK(msb, lsb)) >> lsb)
#define BMVAL_64(val, lsb, msb)	((val & GENMASK_ULL(msb, lsb)) >> lsb)

#define PAR_EL1_FAULT_BIT	0
#define PAR_EL1_FST_START	1
#define PAR_EL1_FST_END		6
#define PAR_EL1_SHA_START	7
#define PAR_EL1_SHA_END		8
#define PAR_EL1_NS_BIT		9
#define PAR_EL1_PA_START	12
#define PAR_EL1_PA_END		47
#define PAR_EL1_MAIR_START	56
#define PAR_EL1_MAIR_END	63

struct va2pa_args {
	void *va;
	void *pa;
	int type;
	u8  esr_fault;
	bool non_secure;
	u8   sharability;
	u8   mair;
};

static long _mvebu_debug_va2pa(struct va2pa_args *va2pa)
{
	u64 pa, par_el1;

	switch (va2pa->type) {
	case VA2PA_TYPE_USER:
		asm volatile("at s1e0r, %0" :: "r" (va2pa->va));
		goto translate;

	case VA2PA_TYPE_KERNEL:
		asm volatile("at s1e1r, %0" :: "r" (va2pa->va));
		goto translate;

	default:
		pr_err("bad translation type %d\n", va2pa->type);
		return -EINVAL;
	}

translate:
	asm volatile("mrs %0, par_el1" : "=r" (par_el1));

	/* Check if translation was sucessfull */
	if (BMVAL(par_el1, PAR_EL1_FAULT_BIT, PAR_EL1_FAULT_BIT)) {
		va2pa->pa = (void *)~0ULL;
		va2pa->esr_fault = BMVAL(par_el1, PAR_EL1_FST_START, PAR_EL1_FST_END);
		return -EFAULT;
	}

	/* lowest 12 bits are offset inside page so
	 * take them from the virtual address
	 */
	pa = BMVAL_64(par_el1, PAR_EL1_PA_START, PAR_EL1_PA_END) << 12;
	va2pa->pa = (void *)(pa | BMVAL((u64)va2pa->va, 0, 11));

	/* Figure out the attributes */
	va2pa->sharability = BMVAL(par_el1, PAR_EL1_SHA_START, PAR_EL1_SHA_END);
	va2pa->non_secure = BMVAL(par_el1, PAR_EL1_NS_BIT, PAR_EL1_NS_BIT);
	va2pa->mair = BMVAL(par_el1, PAR_EL1_MAIR_START, PAR_EL1_MAIR_END);

	return 0;
}

static long mvebu_debug_va2pa(unsigned long arg)
{
	struct va2pa_args va2pa;
	char __user *argp = (char __user *)arg;
	int ret;

	if (copy_from_user(&va2pa, argp, sizeof(struct va2pa_args))) {
		pr_err("Failed to copy from user\n");
		return -EFAULT;
	}

	ret = _mvebu_debug_va2pa(&va2pa);
	if (ret)
		return ret;

	if (copy_to_user(argp, &va2pa, sizeof(struct va2pa_args))) {
		pr_err("Failed to copy to user\n");
		return -EFAULT;
	}

	return 0;
}

static long mvebu_debug_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	switch (cmd) {

	case MVEBU_DBG_VA2PA:
		return mvebu_debug_va2pa(arg);

	default:
		pr_err("unknown debug ioctl\n");
		return -EINVAL;
	}

}
static const struct file_operations mvebu_debug_fops = {
	.unlocked_ioctl = mvebu_debug_ioctl,
};

static __init int mvebu_a8k_debugfs_init(void)
{
	struct mvebu_a8k_state *s = &a8k_state;

	s->debugfs_root = debugfs_create_dir("mvebu", NULL);
	s->debugfs_debug = debugfs_create_file("debug", 0660, s->debugfs_root, NULL, &mvebu_debug_fops);
	if (s->debugfs_root) {
		s->debugfs_ap = debugfs_create_dir("ap806", s->debugfs_root);
		s->debugfs_sysregs = debugfs_create_file("sysregs", 0660,
				s->debugfs_ap, NULL, &mvebu_ap806_sysregs_debug_fops);
	}

	return 0;
}
fs_initcall(mvebu_a8k_debugfs_init);

