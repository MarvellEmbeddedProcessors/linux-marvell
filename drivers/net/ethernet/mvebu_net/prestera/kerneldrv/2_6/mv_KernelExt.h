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
* mvKernelExt.h
*
* DESCRIPTION:
*       functions in kernel mode special for mainOs.
*       External definitions
*
* DEPENDENCIES:
*
*       $Revision:8$
*******************************************************************************/
#ifndef __mv_KernelExt_h__
#define __mv_KernelExt_h__

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#define V306PLUS
#endif

#if defined(CONFIG_ARCH_KIRKWOOD) && defined(V306PLUS)
#define XCAT340
#define MVKERNELEXT_MAJOR 244
#endif

#if defined(V306PLUS)
#define MVKERNELEXT_MAJOR 244
#endif

#if defined(CONFIG_X86_64) && defined(CONFIG_X86)
#define INTEL64
#define INTEL64_CPU
#define MVKERNELEXT_MAJOR 244
#endif

#ifndef MVKERNELEXT_MAJOR
#  define MVKERNELEXT_MAJOR 254
#endif
#ifndef MVKERNELEXT_MINOR
#  define MVKERNELEXT_MINOR 1
#endif

#ifdef __KERNEL__
/*
 * Typedef: mv_waitqueue_t
 *
 * Description:
 *          wait queue
 *          This type defined here because it is close to kernel internals
 *
 * Fields:
 *          first           - pointer to first item in queue
 *          last            - pointer to last item in queue
 *
 */
typedef struct {
    struct mv_task      *first;
    struct mv_task      *last;
} mv_waitqueue_t;

/*******************************************************************************
* translate_priority
*
* DESCRIPTION:
*       Translates a v2pthread priority into kernel priority
*
* INPUTS:
*       policy    - scheduler policy
*       priority  - vxWorks task priority
*
* OUTPUTS:
*       None
*
* RETURNS:
*       kernel priority
*
* COMMENTS:
*
*******************************************************************************/
static int translate_priority(
        int policy,
        int priority
);

#ifndef CONFIG_SMP
#  define MV_GLOBAL_LOCK()      local_irq_disable()
#  define MV_GLOBAL_UNLOCK()    local_irq_enable()
#else /* CONFIG_SMP */
#  include <linux/spinlock.h>
#  define MV_GLOBAL_LOCK()      spin_lock_irq(&mv_giantlock)
#  define MV_GLOBAL_UNLOCK()    spin_unlock_irq(&mv_giantlock)
#endif /* CONFIG_SMP */

#endif /* __KERNEL__ */

#include "../common/mv_KernelExt.h"

#endif /* __mv_KernelExt_h__ */
