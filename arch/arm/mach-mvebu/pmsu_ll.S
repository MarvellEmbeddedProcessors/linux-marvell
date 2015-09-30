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


ENTRY(armada_38x_scu_power_up)
	mrc     p15, 4, r1, c15, c0	@ get SCU base address
	orr	r1, r1, #0x8		@ SCU CPU Power Status Register
	mrc	15, 0, r0, cr0, cr0, 5	@ get the CPU ID
	and	r0, r0, #15
	add	r1, r1, r0
	mov	r0, #0x0
	strb	r0, [r1]		@ switch SCU power state to Normal mode
	ret	lr
ENDPROC(armada_38x_scu_power_up)

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
	bl	armada_38x_scu_power_up
	b	cpu_resume
ENDPROC(armada_38x_cpu_resume)

.global armada_38x_mem_resume_data

#define MBUS_INTERNAL_REG_ADDRESS 0xd0020080

ENTRY(armada_38x_mem_resume)
ARM_BE8(setend	be )			@ go BE8 if entered LE
	/* MMU disable, left enabled by the bootloader */
	mrc	p15, 0, r1, c1, c0, 0
	bic	r1, #1
	mcr	p15, 0, r1, c1, c0, 0

	bl      v7_invalidate_l1

	/*
	 * Load the internal register base address, we keep the value
	 * unmodified in r1 throughout this function.
	 */
	adr	r1, armada_38x_mem_resume_data
	ldr	r1, [r1]

	/* Restore internal register address */
	mov	r2, r1
ARM_BE8(rev	r2, r2)
	ldr	r3, =MBUS_INTERNAL_REG_ADDRESS
	str	r2, [r3]

	/* Update SCU offset CP15 register */
	add	r2, r1, #0xC000
	mcr  	p15, 4, r2, c15, c0, 0

	/*
	 * Disable L2 cache, left enabled by the bootloader,
	 * it will be re-enabled later by the resume logic
	 */
	add	r2, r1, #0x8100
	ldr  	r3, =0x0
	str  	r3, [r2]

	b 	cpu_resume
armada_38x_mem_resume_data:
	.long	.
ENDPROC(armada_38x_mem_resume)

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
