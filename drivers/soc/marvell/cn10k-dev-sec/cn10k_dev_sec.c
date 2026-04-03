// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 Marvell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/arm-smccc.h>
#include <linux/mutex.h>
#include <soc/marvell/octeontx/octeontx_smc.h>
#include "cn10k_dev_sec_provision.h"

static DEFINE_MUTEX(cn10k_dev_sec_mutex);

static long cn10k_dev_sec_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int cn10k_dev_sec_provision_key_status(__u32 *status);

static const struct file_operations cn10k_dev_sec_file_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cn10k_dev_sec_ioctl,
};

static struct miscdevice cn10k_dev_sec_drv = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "cn10k_dev_sec",
	.fops = &cn10k_dev_sec_file_ops,
	.mode = 0600,
};

#define DMA_BUF_SIZE  (2 * sizeof(provision_drv_msg_t))

static void *cn10k_dev_sec_dma_virt;
static dma_addr_t cn10k_dev_sec_dma_phys;


static long cn10k_dev_sec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct csr_request bsr = { .reg_offset = BOOTROM_STATUS };
	struct tim0_key_info key_info = {0};
	struct tim_prov_request *tim_req = NULL;
	provision_drv_msg_t *msg = NULL;
	__u32 status;
	long ret;
	int i;

	mutex_lock(&cn10k_dev_sec_mutex);

	switch (cmd) {
	case PROVISION_FROM_FILE:
		msg = kzalloc(sizeof(provision_drv_msg_t), GFP_KERNEL);
		if (!msg) {
			ret = -ENOMEM;
			goto out;
		}

		if (copy_from_user(&msg->provision_cfg, (void __user *)arg,
				   sizeof(provision_config_t))) {
			ret = -EFAULT;
			kfree(msg);
			break;
		}

		msg->is_tim_prov = 0;
		for (i = 0; i < MAX_NUM_KEY_CONTAINER; i++) {
			if (memchr_inv(&msg->provision_cfg.sec_boot.keys[i], 0,
				       sizeof(struct dsa_key_token_pair)))
				msg->key_bitmap |= (1U << i);
		}

		ret = cn10k_dev_sec_provision_send_smc(msg);
		kfree(msg);
		break;
	case LCS_DEPLOY:
		ret = cn10k_dev_sec_deploy_lcs();
		break;
	case PROVISION_TIM0_KEYS:
		tim_req = kzalloc(sizeof(struct tim_prov_request), GFP_KERNEL);
		if (!tim_req) {
			ret = -ENOMEM;
			goto out;
		}

		if (copy_from_user(tim_req, (void __user *)arg,
				   sizeof(*tim_req))) {
			ret = -EFAULT;
			kfree(tim_req);
			goto out;
		}

		msg = kzalloc(sizeof(provision_drv_msg_t), GFP_KERNEL);
		if (!msg) {
			ret = -ENOMEM;
			kfree(tim_req);
			goto out;
		}

		msg->is_tim_prov = 1;
		msg->key_bitmap = tim_req->bitmap;
		cn10k_dev_sec_provision_rkek(tim_req->has_rkek, tim_req->rkek, &msg->provision_cfg);
		ret = cn10k_dev_sec_provision_send_smc(msg);
		kfree(msg);
		kfree(tim_req);
		break;
	case GET_TIM0_KEY_INFO:
		ret = cn10k_dev_sec_get_tim0_key_info(&key_info);
		if (ret)
			goto out;
		if (copy_to_user((void __user *)arg, &key_info, sizeof(key_info))) {
			ret = -EFAULT;
			goto out;
		}

		break;
	case GET_OTP_PROVISION_STATUS:
		ret = cn10k_dev_sec_provision_key_status(&status);
		if (ret)
			goto out;
		if (put_user(status, (__u32 __user *)arg)) {
			ret = -EFAULT;
			goto out;
		}

		break;
	case GET_BOOTROM_STATUS:
		ret = cn10k_dev_sec_read_csr(&bsr);
		if (ret)
			goto out;
		if (put_user(bsr.value, (__u32 __user *)arg)) {
			ret = -EFAULT;
			goto out;
		}

		break;
	
	default:
		ret = -EINVAL;
		break;
	}

out:
	mutex_unlock(&cn10k_dev_sec_mutex);
	return ret;
}

int cn10k_dev_sec_deploy_lcs(void)
{
	csr_req_t request_csr = { .reg_offset = LCS_DEBUG_STATUS };
	struct arm_smccc_res res;
	__u32 curr_lcs;
	int ret;

	ret = cn10k_dev_sec_read_csr(&request_csr);
	if (ret)
		return ret;

	curr_lcs = request_csr.value & DEBUG_STATUS_REG_LCS_MASK;
	if (curr_lcs != LIFE_STATE_PROVISION) {
		pr_err("Cannot deploy: current LCS is not PROVISION\n");
		return -EPERM;
	}

	arm_smccc_smc(PLAT_OCTEONTX_EHSM_ADVANCE_LIFECYCLE_STATE,
		      LIFE_STATE_DEPLOY, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_err("LCS advancement to deploy state failed\n");
		return -EFAULT;
	}

	return 0;
}

int cn10k_dev_sec_provision_send_smc(provision_drv_msg_t *msg)
{
	struct arm_smccc_res res;

	memcpy(cn10k_dev_sec_dma_virt, msg, sizeof(provision_drv_msg_t));

	arm_smccc_smc(PLAT_OCTEONTX_EHSM_KAK_PROVISION, cn10k_dev_sec_dma_phys,
		      sizeof(provision_drv_msg_t), 0, 0, 0, 0, 0, &res);

	if (res.a0) {
		pr_err("Provisioning failed!\n");
		return -EIO;
	}

	return 0;
}

int cn10k_dev_sec_get_tim0_key_info(struct tim0_key_info *info)
{
	struct arm_smccc_res res;

	memset(cn10k_dev_sec_dma_virt, 0, sizeof(struct tim0_key_info));

	arm_smccc_smc(PLAT_OCTEONTX_EHSM_GET_TIM0_KAK_INFO, cn10k_dev_sec_dma_phys,
		      sizeof(struct tim0_key_info), 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		pr_err("Failed to read TIM0 key info\n");
		return -EIO;
	}

	memcpy(info, cn10k_dev_sec_dma_virt, sizeof(struct tim0_key_info));
	return 0;
}

int cn10k_dev_sec_read_csr(struct csr_request *csr_req)
{
	struct arm_smccc_res res;

	arm_smccc_smc(PLAT_OCTEONTX_EHSM_READ_CSR, csr_req->reg_offset,
		      0, 0, 0, 0, 0, 0, &res);

	if (res.a0) {
		pr_err("Failed to read eHSM CSR 0x%x\n", csr_req->reg_offset);
		return -EFAULT;
	}

	csr_req->value = res.a1;
	return 0;
}

void cn10k_dev_sec_provision_rkek(__u32 has_rkek, __u32 *user_rkek, provision_config_t *cfg)
{
	if (has_rkek) {
		cfg->rkek_option = OEM_INPUT;
		memcpy(cfg->rkek, user_rkek, RKEK_SIZE_WORDS * sizeof(__u32));
	} else {
		cfg->rkek_option = SP800_90_DRBG;
	}
	cfg->rkek_encryption = RKEK_PROV_PLAINTEXT;
}

static int cn10k_dev_sec_provision_key_status(__u32 *status)
{
	struct csr_request rot = { .reg_offset = ROOT_TRUST_STATUS };
	int ret;

	ret = cn10k_dev_sec_read_csr(&rot);
	if (ret) {
		pr_err("Could not get OTP Provision status\n");
		return ret;
	}

	*status = rot.value;
	return 0;
}

static int __init cn10k_dev_sec_drv_init(void)
{
	struct device *dev;
	int error;

	if (octeontx_soc_check_smc() != 2) {
		pr_err("Not supported\n");
		return -EPERM;
	}

	error = misc_register(&cn10k_dev_sec_drv);
	if (error) {
		pr_err("Failed to register misc device\n");
		return error;
	}

	dev = cn10k_dev_sec_drv.this_device;

	/*
	 * Misc devices don't have DMA mask configured by default.
	 * Set up the dma_mask pointer to point to coherent_dma_mask,
	 * then set the actual mask value for coherent allocations
	 * used in SMC calls.
	 */
	dev->dma_mask = &dev->coherent_dma_mask;
	error = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64));
	if (error) {
		pr_err("Failed to set DMA mask: %d\n", error);
		misc_deregister(&cn10k_dev_sec_drv);
		return error;
	}

	cn10k_dev_sec_dma_virt = dma_alloc_coherent(dev, DMA_BUF_SIZE,
					    &cn10k_dev_sec_dma_phys, GFP_KERNEL);
	if (!cn10k_dev_sec_dma_virt) {
		pr_err("Failed to allocate DMA buffer\n");
		misc_deregister(&cn10k_dev_sec_drv);
		return -ENOMEM;
	}

	pr_info("eHSM interface driver loaded\n");
	return 0;
}

static void __exit cn10k_dev_sec_drv_exit(void)
{
	if (cn10k_dev_sec_dma_virt)
		dma_free_coherent(cn10k_dev_sec_drv.this_device, DMA_BUF_SIZE,
				  cn10k_dev_sec_dma_virt, cn10k_dev_sec_dma_phys);
	misc_deregister(&cn10k_dev_sec_drv);
}

module_init(cn10k_dev_sec_drv_init);
module_exit(cn10k_dev_sec_drv_exit);

MODULE_DESCRIPTION("Driver to interact with Marvell's Boot security module");
MODULE_AUTHOR("Shaunak Deshpande <sdeshpande4@marvell.com>");
MODULE_LICENSE("GPL");
