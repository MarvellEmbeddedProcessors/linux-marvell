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

******************************************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/avantalp.h>
#include <plat/sdd_if.h>

#include "mvSysHwConfig.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "cpu/mvCpu.h"

#ifdef CONFIG_MV_AMP_ENABLE

/* Use SoC Per-CPU Timer as all Global Timers are busy. */
#define	TIMER_CTRL		(MV_CPUIF_REGS_OFFSET(whoAmI()) + 0x40)
#define  TIMER_EN(x)		(1 << (2 * x))
#define  TIMER_RELOAD_EN(x)	(2 << (2 * x))
#define  TIMER_25MHZ_EN(x)	(1 << (x + 11))
#define TIMER_CAUSE		(MV_CPUIF_REGS_OFFSET(whoAmI()) + 0x68)
#define  TIMER_INT_CLR(x)	(~(1 << (8 * x)))
#define TIMER_RELOAD(x)		(MV_CPUIF_REGS_OFFSET(whoAmI()) + 0x50 + (8 * x))
#define TIMER_VAL(x)		(MV_CPUIF_REGS_OFFSET(whoAmI()) + 0x54 + (8 * x))

#define DFEV_TIMER	0
#define DFEV_TIMER_IRQ	IRQ_PRIV_SOC_PRIV_TIMER0

#else /* !CONFIG_MV_AMP_ENABLE */

/* Use SoC Global Timer */
#define TIMER_CTRL		(MV_CNTMR_REGS_OFFSET + 0x0000)
#define  TIMER_EN(x)		(1 << (2 * x))
#define  TIMER_RELOAD_EN(x)	(2 << (2 * x))
#define  TIMER_25MHZ_EN(x)	(1 << (x + 11))
#define TIMER_CAUSE		(MV_CNTMR_REGS_OFFSET + 0x0004)
#define  TIMER_INT_CLR(x)	(~(1 << (8 * x)))
#define TIMER_RELOAD(x)		(MV_CNTMR_REGS_OFFSET + 0x0010 + (8 * x))
#define TIMER_VAL(x)		(MV_CNTMR_REGS_OFFSET + 0x0014 + (8 * x))

#define DFEV_TIMER	3
#define DFEV_TIMER_IRQ	IRQ_GLOBAL_TIMER(DFEV_TIMER)

#endif /* !CONFIG_MV_AMP_ENABLE */

/* SSI Reset GPIO */
#define SSI_RESET	19

static irq_handler_t sdd_if_tick;

static irqreturn_t sdd_if_tick_wrapper(int irq, void *data)
{
	irqreturn_t r;

	if (sdd_if_tick)
		r = sdd_if_tick(irq, data);
	else
		r = IRQ_HANDLED;

	MV_REG_WRITE(TIMER_CAUSE, TIMER_INT_CLR(DFEV_TIMER));
	return r;
}
void sdd_if_hw_init(void)
{
	gpio_request(SSI_RESET, "SSI RESET");
	gpio_direction_output(SSI_RESET, 1);
}
EXPORT_SYMBOL(sdd_if_hw_init);

void sdd_if_hw_exit(void)
{
	gpio_free(SSI_RESET);
}
EXPORT_SYMBOL(sdd_if_hw_exit);

void sdd_if_peripheral_reset(void)
{
	gpio_set_value(SSI_RESET, 0);
	msleep(25);
	gpio_set_value(SSI_RESET, 1);
}
EXPORT_SYMBOL(sdd_if_peripheral_reset);

unsigned long sdd_if_read(unsigned long addr)
{
	return readl(DFEV_VIRT_BASE + addr);
}
EXPORT_SYMBOL(sdd_if_read);

void sdd_if_write(unsigned long data, unsigned long addr)
{
	writel(data, DFEV_VIRT_BASE + addr);
}
EXPORT_SYMBOL(sdd_if_write);

int sdd_if_timer_request(irq_handler_t tickfcn, unsigned int tickrate)
{
	unsigned long tv;
	int r;

	if (!tickfcn)
		return -EINVAL;

	sdd_if_tick = tickfcn;

	tv = alp_soc_timer_rate_get() / tickrate;
	MV_REG_WRITE(TIMER_VAL(DFEV_TIMER), tv);
	MV_REG_WRITE(TIMER_RELOAD(DFEV_TIMER), tv);
	MV_REG_WRITE(TIMER_CAUSE, TIMER_INT_CLR(DFEV_TIMER));

	r = request_irq(DFEV_TIMER_IRQ,
		sdd_if_tick_wrapper, 0, "DFEV Tick", NULL);
	if (r)
		return r;

	return 0;
}
EXPORT_SYMBOL(sdd_if_timer_request);

void sdd_if_timer_release(void)
{
	sdd_if_timer_stop();
	free_irq(DFEV_TIMER_IRQ, NULL);
}
EXPORT_SYMBOL(sdd_if_timer_release);

void sdd_if_timer_start(void)
{
	unsigned long reg;

	reg = MV_REG_READ(TIMER_CTRL);
	reg |= TIMER_EN(DFEV_TIMER) | TIMER_RELOAD_EN(DFEV_TIMER) | TIMER_25MHZ_EN(DFEV_TIMER);
	MV_REG_WRITE(TIMER_CTRL, reg);
}
EXPORT_SYMBOL(sdd_if_timer_start);

void sdd_if_timer_stop(void)
{
	unsigned long reg;

	reg = MV_REG_READ(TIMER_CTRL);
	reg &= ~TIMER_EN(DFEV_TIMER);
	MV_REG_WRITE(TIMER_CTRL, reg);
}
EXPORT_SYMBOL(sdd_if_timer_stop);
