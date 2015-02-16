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
* mv_prestera_irq.c
*
* DESCRIPTION:
*       functions in kernel mode special for prestera IRQ.
*
* DEPENDENCIES:
*
*******************************************************************************/
#include "mvOs.h"
#include "mv_prestera_irq.h"
#include "mv_prestera.h"
#include "mv_prestera_pci.h"

#ifdef MV_PP_DBG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif

#define PRESTERA_MAX_INTERRUPTS 4
#define SEMA_DEF_VAL		0

#define IRQ_AURORA_SW_CORES	(IRQ_AURORA_SW_CORE0 | IRQ_AURORA_SW_CORE1 | IRQ_AURORA_SW_CORE2)
#define IRQ_SWITCH_MASK		(0x7 << (IRQ_AURORA_SW_CORE0 - (CPU_INT_SOURCE_CONTROL_IRQ_OFFS + 1)))

#define SW_CORES		0x3

static struct pp_dev	*assigned_irq[PRESTERA_MAX_INTERRUPTS];
static int		assinged_irq_nr;
static int		short_bh_count;

int prestera_int_bh_cnt_get(void)
{
	return short_bh_count;
}

/*******************************************************************************
*	mv_ac3_bc2_enable_switch_irq
*
*
*******************************************************************************/
static void mv_ac3_bc2_enable_switch_irq(uintptr_t i_regs, bool enable)
{
	int reg_val, i;
	unsigned int addr;

	/* For all Switching Core Int */
	for (i = 0; i < SW_CORES; i++) {

		/* Set as pci endpoint and enable */
		addr = CPU_INT_SOURCE_CONTROL_REG((IRQ_AURORA_SW_CORE0 + i));
		reg_val = readl(i_regs + addr);
		dprintk("irq status and ctrl(0x%x) = 0x%x\n", addr, reg_val);

		if (enable)
			reg_val |= (PEX_IRQ_EN) | (PEX_IRQ_EP);
		else
			reg_val &= ~(PEX_IRQ_EN) & ~(PEX_IRQ_EP);

		writel(reg_val, i_regs + addr);

		dprintk("irq status and ctrl(0x%x) = 0x%x\n",
			addr, readl(i_regs + addr));

		/* Clear irq */
		writel(IRQ_AURORA_SW_CORE0 + i,
			i_regs + CPU_INT_CLEAR_MASK_LOCAL_REG);
		dprintk("clr reg(0x%x)\n", CPU_INT_CLEAR_MASK_LOCAL_REG);
	}
}

/*******************************************************************************
* prestera_tl_isr
*
* DESCRIPTION:
*       This is the Prestera ISR reponsible for only scheduling the BH (Tasklet).
*
* INPUTS:
*       irq     - the Interrupt ReQuest number
*       dev_id  - the client data used as argument to the handler
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       IRQ_HANDLED allways
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static irqreturn_t prestera_tl_isr(int		irq,
					void		*dev_id)
{
	struct pp_dev *ppdev = dev_id;

	/* disable the interrupt vector */
	disable_irq_nosync(irq);

	/* enqueue the PP task BH in the tasklet */
	tasklet_hi_schedule((struct tasklet_struct *)ppdev->irq_data.tasklet);

	short_bh_count++;

	return IRQ_HANDLED;
}

static irqreturn_t prestera_tl_isr_pci(int irq, void *dev_id)
{
	struct pp_dev *ppdev = dev_id;
	int reg;

	reg = readl(ppdev->config.base + MSYS_CAUSE_VEC1_REG_OFFS);
	dprintk("msys cause reg 0x%x, switch_mask 0x%x\n",
							 reg, IRQ_SWITCH_MASK);

	if ((reg & IRQ_SWITCH_MASK) == 0)
		return IRQ_NONE;

	/* Disable the interrupt vector */
	disable_irq_nosync(irq);

	/* Enqueue the PP task BH in the tasklet */
	tasklet_hi_schedule((struct tasklet_struct *)ppdev->irq_data.tasklet);

	short_bh_count++;

	return IRQ_HANDLED;
}

/*******************************************************************************
* prestera_bh
*
* DESCRIPTION:
*       This is the Prestera DSR, reponsible for only signaling of the occurence
*       of an event, any procecing will be done in the intTask (user space thread)
*       it self.
*
* INPUTS:
*       data    - the interrupt control data
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       None.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
static void prestera_bh(unsigned long data)
{
	/* awake any reading process */
	up(&((struct intData *)data)->sem);
}

/*******************************************************************************
 * prestera_int_connect
 *
 * DESCRIPTION:
 *       connect and interrupt via register it at the kernel.
 *
 * INPUTS:
 * ppdev - pointer to prestera device structure associated with intVect
 *       routine - the bound routine for this interrupt vector
 *
 * OUTPUTS:
 *       cookie  - the interrupt control data
 *
 * RETURNS:
 *       0 on success, -1 otherwise.
 *
 * COMMENTS:
 *       None.
 *
 ******************************************************************************/
int prestera_int_connect(struct pp_dev		*ppdev,
			 void			*routine,
			 struct intData		**cookie)
{
	unsigned int		status, intVec = ppdev->irq_data.intVec;
	struct intData		*irq_data = &ppdev->irq_data;
	struct tasklet_struct	*tasklet;

	tasklet = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
	if (NULL == tasklet) {
		printk(KERN_ERR "kmalloc failed\n");
		return -ENOMEM;
	}

	*cookie = irq_data;

	/* The user process will wait on it */
	sema_init(&(irq_data->sem), SEMA_DEF_VAL);

	/* For cleanup we will need the tasklet */
	irq_data->tasklet = tasklet;

	tasklet_init(tasklet, prestera_bh, (unsigned long)irq_data);

	if ((ppdev->devId == MV_BOBCAT2_DEV_ID || ppdev->devId == MV_ALLEYCAT3_DEV_ID) &&
								   ppdev->on_pci_bus == 1)
		status = request_irq(intVec, prestera_tl_isr_pci, IRQF_SHARED,
						      "mvPP", (void *)ppdev);
	else
		status = request_irq(intVec, prestera_tl_isr, IRQF_DISABLED,
						      "mvPP", (void *)ppdev);

	if (status) {
		panic("Can not assign IRQ %d to PresteraDev\n", intVec);
		return -1;
	}


	printk(KERN_DEBUG "%s: connected Prestera IRQ - %d\n", __func__, intVec);
	disable_irq_nosync(intVec);
	local_irq_disable();
	if (assinged_irq_nr < PRESTERA_MAX_INTERRUPTS) {
		assigned_irq[assinged_irq_nr++] = ppdev;
		local_irq_enable();
	} else {
		local_irq_enable();
		printk(KERN_DEBUG "%s: too many irqs assigned\n", __func__);
	}

	/* Enable interrupt after registering handler */
	if (ppdev->devId == MV_BOBCAT2_DEV_ID || ppdev->devId == MV_ALLEYCAT3_DEV_ID)
		mv_ac3_bc2_enable_switch_irq(ppdev->config.base, true);

	return 0;
}


void prestera_int_init(void)
{
	assinged_irq_nr = 0;
	short_bh_count = 0;
}


/*******************************************************************************
* prestera_int_cleanup
*
* DESCRIPTION:
*       unbind all interrupts
*
* INPUTS:
*       None
*
* OUTPUTS:
*       None
*
* RETURNS:
*       0 on success, -1 otherwise.
*
* COMMENTS:
*       None.
*
*******************************************************************************/
int prestera_int_cleanup(void)
{
	struct intData *irq_data;
	struct pp_dev *ppdev;

	while (assinged_irq_nr > 0) {

		ppdev = assigned_irq[--assinged_irq_nr];
		irq_data = &ppdev->irq_data;

		/* Do not disable pcie shared irqs - they can be used by
		 * other devices connected via the same pcie such as dragonite)
		 */
		if ((ppdev->devId == MV_BOBCAT2_DEV_ID || ppdev->devId == MV_ALLEYCAT3_DEV_ID) &&
									  ppdev->on_pci_bus == 1)
			mv_ac3_bc2_enable_switch_irq(ppdev->config.base, false);
		else
			disable_irq_nosync(irq_data->intVec);

		free_irq(irq_data->intVec, (void *)ppdev);
		tasklet_kill(irq_data->tasklet);
		kfree(irq_data->tasklet);
	}
	return 0;
}
