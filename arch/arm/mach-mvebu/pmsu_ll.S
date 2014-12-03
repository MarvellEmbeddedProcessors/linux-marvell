/*
 * Copyright (C) 2014 Marvell
 *
 * Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 * Gregory Clement <gregory.clement@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>


/*
 * This is the entry point through which CPUs exiting cpuidle deep
 * idle state are going.
 */
ENTRY(armada_370_xp_cpu_resume)
ARM_BE8(setend	be )			@ go BE8 if entered LE
	/*
	 * Disable the MMU that might have been enabled in BootROM if
	 * this code is used in the resume path of a suspend/resume
	 * cycle.
	 */
	mrc	p15, 0, r1, c1, c0, 0
	bic	r1, #1
	mcr	p15, 0, r1, c1, c0, 0
	bl	ll_add_cpu_to_smp_group
	bl	ll_enable_coherency
	b	cpu_resume
ENDPROC(armada_370_xp_cpu_resume)

ENTRY(armada_38x_cpu_resume)
	/* do we need it for Armada 38x*/
ARM_BE8(setend	be )			@ go BE8 if entered LE
	bl	v7_invalidate_l1
	mrc     p15, 4, r1, c15, c0	@ get SCU base address
	orr	r1, r1, #0x8		@ SCU CPU Power Status Register
	mrc	15, 0, r0, cr0, cr0, 5	@ get the CPU ID
	and	r0, r0, #15
	add	r1, r1, r0
	mov	r2, #0x0
	strb	r2, [r1]		@ switch SCU power state to Normal mode

	/*
	 * PCIe deadlock WA for Armada 38x cpuidle:
	 * The saved data is copied back to the DRAM according to the saved pointer
	 */
	mov	r2, #0x10000
	mul	r1, r0, r2
	ldr	r0, =0xF1100000
	add	r0, r0, r1		@ add CPU offset
	ldr	r1, [r0], #4		@ get address
	ldr	r2, [r0], #4		@ get pointer address
	str	r2, [r1]		@ update pointer
	ldr	r3, [r0],#4		@ ptr size
	lsr	r3, r3 , #0x2
1:
	ldr	r1, [r0], #4
	str	r1, [r2], #4
	subs	r3, r3, #0x1
	bne	1b
	b	cpu_resume
ENDPROC(armada_38x_cpu_resume)

ENTRY(armada_38x_cpu_mem_resume)
	ARM_BE8(setend	be)

	/* Disable MMU that was enabled in bootROM */
	mrc  p15, 0, r1, c1, c0, 0
	bic  r1, #0x1
	mcr  p15, 0, r1, c1, c0, 0

	/* Restore internal registers Base address @ 0xF1000000 */
	ldr  r1, =0xf1000000
ARM_BE8(rev	r1, r1)
	ldr  r0, =0xD0020080
	str  r1, [r0]

	/* Update SCU offset CP15 register */
	ldr  r2, = 0xC000		/* SCU offset = 0xC000 */
	add  r1, r1, r2			/* r1 = INTER_REG_BASE + SCU_OFFSET */
	mcr  p15, 4, r1, c15, c0, 0 	/* Write SCU base register */

	bl	v7_invalidate_l1
	mrc     p15, 4, r1, c15, c0	@ get SCU base address
	orr	r1, r1, #0x8		@ SCU CPU Power Status Register
	mrc	15, 0, r0, cr0, cr0, 5	@ get the CPU ID
	and	r0, r0, #15
	add	r1, r1, r0
	mov	r2, #0x0
	strb	r2, [r1]		@ switch SCU power state to Normal mode

	b	cpu_resume

ENDPROC(armada_38x_cpu_mem_resume)

#define GPIO_32_47_DATA_OUT_EN_CTRL_REG_ADDR	(0x144)	/* 0x18144 */
#define GPIO_32_47_DATA_OUT_REG_ADDR		(0x140) /* 0x18140 */
#define GPIO_CMD_VALUE				(0x2)
#define GPIO_ACK_VALUE				(0xE)
#define GPIO_PIN_MASK				(0xFFFFFFFE)
#define SDRAM_DLB_EVICT_OFFS_REG		(0x30C)	/* 0x170C */
#define SDRAM_OPERATION_REG			(0x18)	/* 0x1418 */
#define SDRAM_CONFIG_REG			(0x00)	/* 0x1400 */

ENTRY(enter_mem_suspend)
	/* Save ARM registers */
	stmfd	sp!, {r4-r12, lr}		@ save registers on stack
	/*
	* Issue a Data Synchronization Barrier instruction to ensure
	* that all state saving has been completed.
	*/
	dsb

	/* Flush the DLB and wait ~7 usec */
	/* Clear bits 7:0 in 'DLB Eviction Control Register', 0x170C */
	/* Flush the DLB and wait ~7 usec*/
	ldr r2, =SDRAM_DLB_EVICT_OFFS_REG
	orr	r2, r2, r0
	ldr	r3, [r2]
ARM_BE8(rev	r3, r3)
	bic     r3, #0x000000FF
ARM_BE8(rev	r3, r3)
	str     r3, [r2]

	/* Wait ~7 us */
	ldr r2, = 6000  /* WC freq =  1.6 Ghz, 2 cycles per loop */
1:	subs r2,r2,#1
	bne 1b

	/* Set DRAM in battery backup mode */
	/* Clear bit 24 in 'SDRAM Configuration Register', 0x1400 */
	ldr 	r2, =SDRAM_CONFIG_REG
	orr	r2, r2, r0
	ldr	r3, [r2]
ARM_BE8(rev	r3, r3)
	bic     r3, #0x01000000
ARM_BE8(rev	r3, r3)
	str     r3, [r2]

	/* Prepare to go to self-refresh */
	/* Involves writing 0x7 to 'SDRAM Operation Register', 0x1418 */
	ldr	r2, =(SDRAM_OPERATION_REG)
	orr	r2, r2, r0
	ldr	r3, [r2]
ARM_BE8(rev	r3, r3)
	ldr	r4, =0x00000007
	orr	r3, r3, r4
ARM_BE8(rev	r3, r3)

	/* Configure GPIOs 33-35 for communicating with PIC */
	/* Prepare command value for GPIOs 33-35 */
	ldr r4, =(GPIO_32_47_DATA_OUT_REG_ADDR)
	orr r4, r4, r1
	ldr r5, =(GPIO_PIN_MASK)
	ldr r6, =(GPIO_CMD_VALUE)
	ldr r7, [r4]
ARM_BE8(rev r7, r7)
	and r7, r7, r5
	orr r7, r7, r6
ARM_BE8(rev r7, r7)

	/* Set GPIO 33-35 as out */
	ldr r0, =(GPIO_32_47_DATA_OUT_EN_CTRL_REG_ADDR)
	orr r0, r0, r1
	ldr r6, [r0]
ARM_BE8(rev r6, r6)
	and r6, r6, r5
ARM_BE8(rev r6, r6)
	str r6, [r0]

	/* Issue the cmd */
	str r7, [r4]

	/* Prepare the cmd ack */
	ldr r6, =(GPIO_ACK_VALUE)
ARM_BE8(rev r7, r7)
	orr r7, r7, r6
ARM_BE8(rev r7, r7)

	/*
	 * Wait between cmd (0x1) and cmd ack (0x7)
	 */
	ldr r1, =1000000000
1:	subs r1,r1,#1
	bne 1b

	/*
	 * Put Dram into self refresh. From here on we can perform
	 * 8 instructions to ensure execution from I-Cache
	 */
	.align 5
	/* Enter self-refresh */
	str	r3, [r2]

	/* Wait 100 cycles for DDR to enter self refresh */
	ldr r1, = 50
1:	subs r1,r1,#1
	bne 1b

	/* Issue the cmd ack. This will turn of the board */
	str r7, [r4]

	/* trap the processor */
	b .
	/* Wait a while */
ENDPROC(enter_mem_suspend)

.global mvebu_boot_wa_start
.global mvebu_boot_wa_end

/* The following code will be executed from SRAM */
ENTRY(mvebu_boot_wa_start)
mvebu_boot_wa_start:
ARM_BE8(setend	be)
	adr	r0, 1f
	ldr	r0, [r0]		@ load the address of the
					@ resume register
	ldr	r0, [r0]		@ load the value in the
					@ resume register
ARM_BE8(rev	r0, r0)			@ the value is stored LE
	mov	pc, r0			@ jump to this value
/*
 * the last word of this piece of code will be filled by the physical
 * address of the boot address register just after being copied in SRAM
 */
1:
	.long   .
mvebu_boot_wa_end:
ENDPROC(mvebu_boot_wa_end)
