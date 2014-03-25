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
********************************************************************************
* mv_cph_dev.c
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) char device definition
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 22Jan2013
*
* FILE REVISION NUMBER:
*               Revision: 1.0
*
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/poll.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#include "mv_cph_header.h"

/* Used to prevent multiple access to device */
static int               g_cph_device_open;
static struct miscdevice g_cph_misc_dev;

/******************************************************************************
* cph_dev_open()
* _____________________________________________________________________________
*
* DESCRIPTION: The function executes device open actions
*
* INPUTS:
*       inode - Device inode pointer.
*       file  - File handler.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
static int cph_dev_open(struct inode *inode, struct file *file)
{
	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

#if 0
	if (g_cph_device_open > 0)
		return -EBUSY;
#endif

	g_cph_device_open++;

	return MV_OK;
}

/******************************************************************************
* cph_dev_release()
* _____________________________________________________________________________
*
* DESCRIPTION: The function executes device release actions
*
* INPUTS:
*       inode - Device inode pointer.
*       file  - File handler.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
static int cph_dev_release(struct inode *inode, struct file *file)
{
	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

#if 0
	if (cph_device_open > 0)
		cph_device_open--;
#endif
	return MV_OK;
}

/******************************************************************************
* cph_dev_ioctl()
* _____________________________________________________________________________
*
* DESCRIPTION: The function executes ioctl commands
*
* INPUTS:
*       inode - Device inode pointer.
*       file  - File handler.
*       cmd   - Command.
*       arg   - Ponter to arg.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
long cph_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct CPH_IOCTL_APP_RULE_T    cph_app_rule;
	struct CPH_IOCTL_FLOW_MAP_T    cph_flow_map;
	struct CPH_IOCTL_DSCP_MAP_T    cph_dscp_map;
	struct CPH_IOCTL_MISC_T        cph_misc;
	struct CPH_IOCTL_TCONT_STATE_T cph_tcont;
	int                   rc  = -EINVAL;

	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

	switch (cmd) {
	/* CPH application packet handling IOCTL
	-------------------------------------------*/
	case MV_CPH_IOCTL_SET_COMPLEX_PROFILE:
		if (copy_from_user(&cph_misc, (struct CPH_IOCTL_MISC_T *)arg, sizeof(struct CPH_IOCTL_MISC_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_set_complex_profile(cph_misc.profile_id, cph_misc.active_port);
		break;

	case MV_CPH_IOCTL_SET_FEATURE_FLAG:
		if (copy_from_user(&cph_misc, (struct CPH_IOCTL_MISC_T *)arg, sizeof(struct CPH_IOCTL_MISC_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_set_feature_flag(cph_misc.feature_type, cph_misc.feature_flag);
		break;

	case MV_CPH_IOCTL_APP_ADD_RULE:
		if (copy_from_user(&cph_app_rule, (struct CPH_IOCTL_APP_RULE_T *)arg,
			sizeof(struct CPH_IOCTL_APP_RULE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_add_app_rule(cph_app_rule.parse_bm, &cph_app_rule.parse_key, cph_app_rule.mod_bm,
					&cph_app_rule.mod_value, cph_app_rule.frwd_bm, &cph_app_rule.frwd_value);
		break;

	case MV_CPH_IOCTL_APP_DEL_RULE:
		if (copy_from_user(&cph_app_rule, (struct CPH_IOCTL_APP_RULE_T *)arg,
			sizeof(struct CPH_IOCTL_APP_RULE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_del_app_rule(cph_app_rule.parse_bm, &cph_app_rule.parse_key);
		break;

	case MV_CPH_IOCTL_APP_UPDATE_RULE:
		if (copy_from_user(&cph_app_rule, (struct CPH_IOCTL_APP_RULE_T *)arg,
			sizeof(struct CPH_IOCTL_APP_RULE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_update_app_rule(cph_app_rule.parse_bm, &cph_app_rule.parse_key, cph_app_rule.mod_bm,
					&cph_app_rule.mod_value, cph_app_rule.frwd_bm, &cph_app_rule.frwd_value);
		break;

	case MV_CPH_IOCTL_APP_GET_RULE:
		if (copy_from_user(&cph_app_rule, (struct CPH_IOCTL_APP_RULE_T *)arg,
			sizeof(struct CPH_IOCTL_APP_RULE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_get_app_rule(cph_app_rule.parse_bm, &cph_app_rule.parse_key, &cph_app_rule.mod_bm,
					&cph_app_rule.mod_value, &cph_app_rule.frwd_bm, &cph_app_rule.frwd_value);

		if (rc != MV_OK)
			goto ioctl_err;

		if (copy_to_user((struct CPH_IOCTL_APP_RULE_T *)arg, &cph_app_rule,
			sizeof(struct CPH_IOCTL_APP_RULE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_to_user failed\n");
			goto ioctl_err;
		}
		break;

	/* CPH flow mapping IOCTL
	-------------------------------------------*/
	case MV_CPH_IOCTL_FLOW_ADD_RULE:
		if (copy_from_user(&cph_flow_map, (struct CPH_IOCTL_FLOW_MAP_T *)arg,
			sizeof(struct CPH_IOCTL_FLOW_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_add_flow_rule(&cph_flow_map.flow_map);
		break;

	case MV_CPH_IOCTL_FLOW_DEL_RULE:
		if (copy_from_user(&cph_flow_map, (struct CPH_IOCTL_FLOW_MAP_T *)arg,
			sizeof(struct CPH_IOCTL_FLOW_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_del_flow_rule(&cph_flow_map.flow_map);
		break;

	case MV_CPH_IOCTL_FLOW_GET_RULE:
		if (copy_from_user(&cph_flow_map, (struct CPH_IOCTL_FLOW_MAP_T *)arg,
			sizeof(struct CPH_IOCTL_FLOW_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_get_flow_rule(&cph_flow_map.flow_map);

		if (rc != MV_OK)
			goto ioctl_err;

		if (copy_to_user((struct CPH_IOCTL_FLOW_MAP_T *)arg, &cph_flow_map,
			sizeof(struct CPH_IOCTL_FLOW_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_to_user failed\n");
			goto ioctl_err;
		}
		break;

	case MV_CPH_IOCTL_FLOW_CLEAR_RULE:
		rc = cph_clear_flow_rule();
		break;

	case MV_CPH_IOCTL_FLOW_CLEAR_RULE_BY_MH:
		if (copy_from_user(&cph_flow_map, (struct CPH_IOCTL_FLOW_MAP_T *)arg,
			sizeof(struct CPH_IOCTL_FLOW_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_clear_flow_rule_by_mh(cph_flow_map.flow_map.mh);
		break;

	case MV_CPH_IOCTL_FLOW_SET_DSCP_MAP:
		if (copy_from_user(&cph_dscp_map, (struct CPH_IOCTL_DSCP_MAP_T *)arg,
			sizeof(struct CPH_IOCTL_DSCP_MAP_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_set_flow_dscp_map(&cph_dscp_map.dscp_map);
		break;

	case MV_CPH_IOCTL_FLOW_DEL_DSCP_MAP:
		rc = cph_del_flow_dscp_map();
		break;

	case MV_CPH_IOCTL_SET_TCONT_LLID_STATE:
		if (copy_from_user(&cph_tcont, (unsigned int *)arg, sizeof(struct CPH_IOCTL_TCONT_STATE_T))) {
			MV_CPH_PRINT(CPH_ERR_LEVEL, "copy_from_user failed\n");
			goto ioctl_err;
		}

		rc = cph_set_tcont_state(cph_tcont.tcont, cph_tcont.state);
		break;

	case MV_CPH_IOCTL_SETUP:
		rc = cph_dev_setup();
		break;

	default:
		rc = -EINVAL;
	}

ioctl_err:
	return rc;
}


static const struct file_operations g_cph_dev_fops = {
	.open			= cph_dev_open,
	.release		= cph_dev_release,
	.unlocked_ioctl	= cph_dev_ioctl,
};

/******************************************************************************
* cph_dev_setup()
* _____________________________________________________________________________
*
* DESCRIPTION: Setup device
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_dev_setup(void)
{
	MV_STATUS rc  = MV_OK;

	/* Get parameter from XML file */
	rc = cph_db_get_xml_param();
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_db_get_xml_param");

	return rc;
}

/******************************************************************************
* cph_dev_init()
* _____________________________________________________________________________
*
* DESCRIPTION: Initialize CPH device
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_dev_init(void)
{
	MV_STATUS rc = MV_OK;
	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

	g_cph_misc_dev.minor = MISC_DYNAMIC_MINOR;
	g_cph_misc_dev.name  = MV_CPH_DEVICE_NAME;
	g_cph_misc_dev.fops  = &g_cph_dev_fops;

	rc = misc_register(&g_cph_misc_dev);
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call misc_register");

	rc = cph_netdev_init();
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_netdev_init");

	rc = cph_sysfs_init();
	CHECK_API_RETURN_AND_LOG_ERROR(rc, "fail to call cph_sysfs_init");

	pr_info("CPH: misc device %s registered with minor: %d\n", MV_CPH_DEVICE_NAME, g_cph_misc_dev.minor);
	return rc;
}

/******************************************************************************
* cph_dev_shutdown()
* _____________________________________________________________________________
*
* DESCRIPTION: Shutdown CPH device
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*******************************************************************************/
void cph_dev_shutdown(void)
{
	MV_CPH_PRINT(CPH_DEBUG_LEVEL, "Enter\n");

	cph_sysfs_exit();

	misc_deregister(&g_cph_misc_dev);
}
