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

#ifndef TM_PLATFORM_IMPLEMENTATION_DEFINITIONS_H
#define TM_PLATFORM_IMPLEMENTATION_DEFINITIONS_H


#include "tm.h"
#include "tm_core_types.h"


/**
 * @brief   Declare a QMTM environment and check consitency
 */
#define QMTM_ENVIRONMENT(_name, _handle) \
		struct qmtm *_name = (struct tm *)(_handle); \
	if ((_name) == NULL) { \
		return -EINVAL; \
	} \
	if ((_name)->magic != TM_MAGIC) { \
		return -EBADF; \
	} \

/**
 * @brief   Check arguments and get pointers
 *
 * @param[in]   hndl    TM handle
 * @param[out]  ctl     TM core handle
 * @param[out]  env     TM struct pointer
 *
 * @retval  Zero on success
 * @retval  -EBADF On bad file descriptor
 * @retval  -EINVAL On bad arguments
 */
int tm_check_args(struct qmtm *hndl, struct tm_ctl **ctl, struct qmtm **env);

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
 * @brief   Wrapper macro to check for errors and return
 */
#define	TM_WRAPPER_END(_handle)	\
	if (rc) { \
		rc = tm_to_qmtm_errcode(rc); \
	} \
	return rc;


#define	TM_REGVAR(_type, _var)  struct _type _var
#define	TM_REGVAR_ADDR(_var)   (&(_var))

/* if the structure is defined it can be zero memored */
/* the Linux platform  generates warning for this action */

#endif  /* TM_PLATFORM_IMPLEMENTATION_DEFINITIONS_H */

