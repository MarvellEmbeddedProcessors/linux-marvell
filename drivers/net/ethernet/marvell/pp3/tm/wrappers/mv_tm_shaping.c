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

#include "mv_tm_shaping.h"
#include "tm_shaping.h"
#include "tm_errcodes.h"
#include "tm_elig_prio_func.h"

/* Public functions to be called from external modules */
int mv_tm_set_shaping_ex(enum mv_tm_level level,
						uint32_t index,
						uint32_t cbw,
						uint32_t ebw,
						uint32_t *pcbs,
						uint32_t *pebs)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_set_shaping_ex(ctl, TM_LEVEL(level), index, cbw, ebw, pcbs, pebs);
	if (rc != 0) {
		pr_info("tm_set_shaping_ex error: %d\n", rc);
		if (rc == TM_CONF_MIN_TOKEN_TOO_LARGE) {
			pr_info("cbs or ebs are too small , should be :");
			if (*pcbs)
				pr_info(" cbs = %u", *pcbs);
			if (*pebs)
				pr_info(" ebs = %u", *pebs);
			pr_info("\n");
		}
	}
	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_set_shaping_ex);

int mv_tm_set_shaping(enum mv_tm_level level,
						uint32_t index,
						uint32_t cbw,
						uint32_t ebw)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_set_shaping(ctl, TM_LEVEL(level), index, cbw, ebw);
	if (rc != 0)
		pr_info("tm_set_shaping error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_set_shaping);

int mv_tm_set_min_shaping(enum mv_tm_level level,
						uint32_t index,
						uint32_t cbw)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_set_min_shaping(ctl, TM_LEVEL(level), index, cbw);
	if (rc != 0)
		pr_info("tm_set_min_shaping error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_set_min_shaping);

int mv_tm_set_no_shaping(enum mv_tm_level level,
						uint32_t index)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_set_no_shaping(ctl, TM_LEVEL(level), index);
	if (rc != 0)
		pr_info("tm_set_no_shaping error: %d\n", rc);

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_set_no_shaping);

int mv_tm_get_shaping(enum mv_tm_level level, uint32_t index, uint32_t *cir, uint32_t *eir)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_read_shaping(ctl, TM_LEVEL(level), index, cir, eir, NULL, NULL);
	if (rc != 0) {
		pr_info("mv_tm_get_shaping error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	TM_WRAPPER_END(qmtm_hndl);
}

EXPORT_SYMBOL(mv_tm_get_shaping);

int mv_tm_get_shaping_ex(enum mv_tm_level level, uint32_t index,
						uint32_t *cir, uint32_t *eir, uint32_t *cbs, uint32_t *ebs)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_read_shaping(ctl, TM_LEVEL(level), index, cir, eir, cbs, ebs);
	if (rc != 0) {
		pr_info("mv_tm_get_shaping error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_get_shaping_ex);

int mv_tm_get_shaping_full_info(enum mv_tm_level level, uint32_t index,
								uint8_t *elig_fun, uint8_t *cir_eir_mask,
								uint32_t *cir, uint32_t *eir,
								uint32_t *cbs, uint32_t *ebs)
{
	TM_WRAPPER_BEGIN(qmtm_hndl, ctl, henv);

	rc = tm_get_node_elig_prio_fun_info(ctl, TM_LEVEL(level), index, elig_fun, cir_eir_mask);
	if (rc != 0) {
		pr_info("mv_tm_get_shaping error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}
	rc = tm_read_shaping(ctl, TM_LEVEL(level), index, cir, eir, cbs, ebs);
	if (rc != 0) {
		pr_info("mv_tm_get_shaping error: %d\n", rc);
		TM_WRAPPER_END(qmtm_hndl);
	}

	TM_WRAPPER_END(qmtm_hndl);
}
EXPORT_SYMBOL(mv_tm_get_shaping_full_info);
