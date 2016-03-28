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

#ifndef MV_TM__H
#define MV_TM__H

#include "common/mv_sw_if.h"
#include "platform/mv_pp3.h"
#include "tm_to_qmtm_enums.h"

/* Ports Numbers */
#define TM_A0_PORT_PPC0_0     0
#define TM_A0_PORT_PPC0_1     1
#define TM_A0_PORT_PPC1_MNT0  2
#define TM_A0_PORT_PPC1_MNT1  3
#define TM_A0_PORT_EMAC0      4
#define TM_A0_PORT_EMAC1      5
#define TM_A0_PORT_EMAC2      6
#define TM_A0_PORT_EMAC3      7
#define TM_A0_PORT_EMAC4      8
#define TM_A0_PORT_EMAC_LPB   9
#define TM_A0_PORT_CMAC_IN    10
#define TM_A0_PORT_CMAC_LA    11
#define TM_A0_PORT_HMAC       12
#define TM_A0_PORT_UNUSED0    13
#define TM_A0_PORT_DROP0      14
#define TM_A0_PORT_DROP1      15

#define MV_TM_MAX_PORTS			16

/* Predefined Shaping rate */
#define MV_TM_1G_BW			1000
#define MV_TM_2HG_BW		2500
#define MV_TM_6G_BW			6000

extern uint8_t tm_debug_on;
extern struct qmtm *qmtm_hndl;
extern void __iomem *tm_regs_base;
extern const char *tm_prod_str;

enum mv_tm_level {
	TM_Q_LEVEL = 0,
	TM_A_LEVEL,
	TM_B_LEVEL,
	TM_C_LEVEL,
	TM_P_LEVEL
};


enum mv_tm_config {
	TM_INVALID_CONFIG = 0,
	TM_DEFAULT_CONFIG,
	TM_CFG1_CONFIG,
	TM_2xPPC_CONFIG,
	TM_CFG3_CONFIG,
	TM_LAST_CONFIG
};


/**
 * @brief   Data structure for qmtm adaptation
 */
struct qmtm {
	u32 magic;                 /**< magic number of tm struct */
	struct tm_ctl *tmctl;      /**< Internal TM handle (into TM core) */
};

/**
 * @brief   Declare a TM environment and check consitency
 */
#define TM_ENVIRONMENT(_name, _handle) \
		struct qmtm *_name = (	struct qmtm *)(_handle); \
	if ((_name) == NULL) { \
		return -EINVAL; \
	} \
	if ((_name)->magic != TM_MAGIC) { \
		return -EBADF; \
	} \

/**
 * @brief   Wrapper macro to initialize variables from handle pointer
 */
#define	TM_WRAPPER_BEGIN(_handle, _ctl, _henv)	\
	struct tm_ctl *_ctl; \
	struct qmtm *_henv; \
	int rc; \
	rc = tm_check_args(_handle, &(_ctl), &(_henv)); \
	if (rc != 0) { \
		return rc; \
	}

/**
 * @brief   Check arguments and get pointers
 *
 */
int tm_check_args(struct qmtm *hndl, struct tm_ctl **ctl, struct qmtm **env);

/**
 * @brief   Wrapper macro to check for errors and return
 */
#define	TM_WRAPPER_END(_handle)	\
	if (rc) { \
		rc = tm_to_qmtm_errcode(rc); \
	} \
	return rc;

/* TM unit once time global initialization */
int tm_global_init(void __iomem *base, const char *prod_str);

/**
 * @brief   create  & initialize TM configuration library  database.
  */
int tm_open(void);

/**
 * @brief   Close TM configuration library.
  */
int tm_close(void);

int tm_defzero(void);
int tm_defcon(void);
int tm_cfg1(void);
int tm_2xppc(void);


int tm_cfg3_tree(void);


/**
 * @brief   Init SysFS.
 *
 * @return an integer return code.
 */
int mv_pp3_tm_sysfs_init(struct kobject *pp3_kobj);


/**
 * @brief   Exit SysFS.
 *
 * @return an integer return code.
 */
int mv_pp3_tm_sysfs_exit(struct kobject *hmac_kobj);


#endif /* MV_TM__H */
