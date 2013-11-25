/*
 * arch/arm/mm/proc-v7-2level.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define TTB_S		(1 << 1)
#define TTB_RGN_NC	(0 << 3)
#define TTB_RGN_OC_WBWA	(1 << 3)
#define TTB_RGN_OC_WT	(2 << 3)
#define TTB_RGN_OC_WB	(3 << 3)
#define TTB_NOS		(1 << 5)
#define TTB_IRGN_NC	((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA	((0 << 0) | (1 << 6))
#define TTB_IRGN_WT	((1 << 0) | (0 << 6))
#define TTB_IRGN_WB	((1 << 0) | (1 << 6))

/* PTWs cacheable, inner WB not shareable, outer WB not shareable */
#define TTB_FLAGS_UP	TTB_IRGN_WB|TTB_RGN_OC_WB
#define PMD_FLAGS_UP	PMD_SECT_WB

/* PTWs cacheable, inner WBWA shareable, outer WBWA not shareable */
#define TTB_FLAGS_SMP	TTB_IRGN_WBWA|TTB_S|TTB_NOS|TTB_RGN_OC_WBWA
#define PMD_FLAGS_SMP	PMD_SECT_WBWA|PMD_SECT_S

/*
 *	cpu_v7_switch_mm(pgd_phys, tsk)
 *
 *	Set the translation table base pointer to be pgd_phys
 *
 *	- pgd_phys - physical address of new TTB
 *
 *	It is assumed that:
 *	- we are not using split page tables
 */
ENTRY(cpu_v7_switch_mm)
#ifdef CONFIG_MMU
	mov	r2, #0
	mmid	r1, r1				@ get mm->context.id
	ALT_SMP(orr	r0, r0, #TTB_FLAGS_SMP)
	ALT_UP(orr	r0, r0, #TTB_FLAGS_UP)
#ifdef CONFIG_ARM_ERRATA_430973
	mcr	p15, 0, r2, c7, c5, 6		@ flush BTAC/BTB
#endif
#ifdef CONFIG_PID_IN_CONTEXTIDR
	mrc	p15, 0, r2, c13, c0, 1		@ read current context ID
	lsr	r2, r2, #8			@ extract the PID
	bfi	r1, r2, #8, #24			@ insert into new context ID
#endif
#ifdef CONFIG_ARM_ERRATA_754322
	dsb
#endif
	mcr	p15, 0, r1, c13, c0, 1		@ set context ID
	isb
	mcr	p15, 0, r0, c2, c0, 0		@ set TTB 0
	isb
#endif
	mov	pc, lr
ENDPROC(cpu_v7_switch_mm)

.macro flush_pte adr
	mcr	p15, 0, \adr, c7, c10, 1	@ flush_pte
.endm

/*
 *	cpu_v7_set_pte_ext(ptep, pte)
 *
 *	Set a level 2 translation table entry.
 *
 *	- ptep  - pointer to level 2 translation table entry
 *		  (hardware version is stored at +2048 bytes)
 *	- pte   - PTE value to store
 *	- ext	- value for extended PTE bits
 */
ENTRY(cpu_v7_set_pte_ext)
#ifdef CONFIG_MMU
	str	r1, [r0]			@ linux version

	/* Calc HW PTE Entry Offset */
#ifdef CONFIG_MV_LARGE_PAGE_SUPPORT
#ifdef CONFIG_MV_8KB_SW_PAGE_SIZE_SUPPORT
	mov	r3, #0x3FC
	and	r3, r3, r0
	mov	r3, r3, lsl#1
#elif defined(CONFIG_MV_16KB_SW_PAGE_SIZE_SUPPORT)
	mov	r3, #0x1FC
	and	r3, r3, r0
	mov	r3, r3, lsl#2
#elif defined(CONFIG_MV_32KB_SW_PAGE_SIZE_SUPPORT)
	mov	r3, #0xFC
	and	r3, r3, r0
	mov	r3, r3, lsl#3
#elif defined(CONFIG_MV_64KB_SW_PAGE_SIZE_SUPPORT)
	mov	r3, #0x7C
	and	r3, r3, r0
	mov	r3, r3, lsl#4
#elif defined(CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT)
	mov	r3, #0x7C
	and	r3, r3, r0
	mov	r3, r3, lsl#4
#endif /* CONFIG_MV_8KB_SW_PAGE_SIZE_SUPPORT */
	bic	r0, r0, #0x3FC
	bic	r0, r0, #0x400
	orr	r0, r0, r3
#endif /* CONFIG_MV_LARGE_PAGE_SUPPORT */
	bic	r3, r1, #0x000003f0
#ifdef CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT
	bic	r3, r3, #0x00000F000
#endif
	bic	r3, r3, #PTE_TYPE_MASK
	orr	r3, r3, r2
#ifdef CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT
	orr	r3, r3, #PTE_EXT_AP0 | 1
#else
	orr	r3, r3, #PTE_EXT_AP0 | 2
#endif

	tst	r1, #1 << 4
	orrne	r3, r3, #PTE_EXT_TEX(1)

	eor	r1, r1, #L_PTE_DIRTY
	tst	r1, #L_PTE_RDONLY | L_PTE_DIRTY
	orrne	r3, r3, #PTE_EXT_APX

	tst	r1, #L_PTE_USER
	orrne	r3, r3, #PTE_EXT_AP1

	tst	r1, #L_PTE_XN
	orrne	r3, r3, #PTE_EXT_XN

	tst	r1, #L_PTE_YOUNG
	tstne	r1, #L_PTE_VALID
	eorne	r1, r1, #L_PTE_NONE
	tstne	r1, #L_PTE_NONE
	moveq	r3, #0

 ARM(	str	r3, [r0, #2048]! )
 THUMB(	add	r0, r0, #2048 )
 THUMB(	str	r3, [r0] )
#ifdef CONFIG_MV_LARGE_PAGE_SUPPORT
#ifdef CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT
	@ Need to duplicate the entry 16 times because of overlapping in PTE index bits.
	str	r3, [r0, #4]
	str	r3, [r0, #8]
	str	r3, [r0, #12]
	str	r3, [r0, #16]
	str	r3, [r0, #20]
	str	r3, [r0, #24]
	str	r3, [r0, #28]
	flush_pte r0
	add	r0, r0, #32
	str	r3, [r0]
	str	r3, [r0, #4]
	str	r3, [r0, #8]
	str	r3, [r0, #12]
	str	r3, [r0, #16]
	str	r3, [r0, #20]
	str	r3, [r0, #24]
	str	r3, [r0, #28]
	flush_pte r0
#else
#if PAGE_SHIFT > 12 		// >= 8KB
	add	r3, r3, #0x1000
	str	r3, [r0, #4]
#endif /* PAGE_SHIFT > 12 */
#if PAGE_SHIFT > 13		// >= 16KB
	add	r3, r3, #0x1000
	str	r3, [r0, #8]
	add	r3, r3, #0x1000
	str	r3, [r0, #12]
#endif /* PAGE_SHIFT > 13 */
#if PAGE_SHIFT > 14		// >= 32KB
	add	r3, r3, #0x1000
	str	r3, [r0, #16]
	add	r3, r3, #0x1000
	str	r3, [r0, #20]
	add	r3, r3, #0x1000
	str	r3, [r0, #24]
	add	r3, r3, #0x1000
	str	r3, [r0, #28]
#endif /* PAGE_SHIFT > 14 */
	flush_pte r0
#if PAGE_SHIFT > 15		// >= 64KB
	add	r0, r0, #32
	add	r3, r3, #0x1000
	str	r3, [r0]
	add	r3, r3, #0x1000
	str	r3, [r0, #4]
	add	r3, r3, #0x1000
	str	r3, [r0, #8]
	add	r3, r3, #0x1000
	str	r3, [r0, #12]
	add	r3, r3, #0x1000
	str	r3, [r0, #16]
	add	r3, r3, #0x1000
	str	r3, [r0, #20]
	add	r3, r3, #0x1000
	str	r3, [r0, #24]
	add	r3, r3, #0x1000
	str	r3, [r0, #28]
	flush_pte r0
#endif /* PAGE_SHIFT > 15 */
#endif /* CONFIG_MV_64KB_MMU_PAGE_SIZE_SUPPORT */
#else /* CONFIG_MV_LARGE_PAGE_SUPPORT */
	flush_pte	r0
#endif /* CONFIG_MV_LARGE_PAGE_SUPPORT */
#endif /* CONFIG_MMU */
	mov	pc, lr
ENDPROC(cpu_v7_set_pte_ext)

	/*
	 * Memory region attributes with SCTLR.TRE=1
	 *
	 *   n = TEX[0],C,B
	 *   TR = PRRR[2n+1:2n]		- memory type
	 *   IR = NMRR[2n+1:2n]		- inner cacheable property
	 *   OR = NMRR[2n+17:2n+16]	- outer cacheable property
	 *
	 *			n	TR	IR	OR
	 *   UNCACHED		000	00
	 *   BUFFERABLE		001	10	00	00
	 *   WRITETHROUGH	010	10	10	10
	 *   WRITEBACK		011	10	11	11
	 *   reserved		110
	 *   WRITEALLOC		111	10	01	01
	 *   DEV_SHARED		100	01
	 *   DEV_NONSHARED	100	01
	 *   DEV_WC		001	10
	 *   DEV_CACHED		011	10
	 *
	 * Other attributes:
	 *
	 *   DS0 = PRRR[16] = 0		- device shareable property
	 *   DS1 = PRRR[17] = 1		- device shareable property
	 *   NS0 = PRRR[18] = 0		- normal shareable property
	 *   NS1 = PRRR[19] = 1		- normal shareable property
	 *   NOS = PRRR[24+n] = 1	- not outer shareable
	 */
.equ	PRRR,	0xff0a81a8
.equ	NMRR,	0x40e040e0

	/*
	 * Macro for setting up the TTBRx and TTBCR registers.
	 * - \ttb0 and \ttb1 updated with the corresponding flags.
	 */
	.macro	v7_ttb_setup, zero, ttbr0, ttbr1, tmp
	mcr	p15, 0, \zero, c2, c0, 2	@ TTB control register
	ALT_SMP(orr	\ttbr0, \ttbr0, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr0, \ttbr0, #TTB_FLAGS_UP)
	ALT_SMP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_SMP)
	ALT_UP(orr	\ttbr1, \ttbr1, #TTB_FLAGS_UP)
	mcr	p15, 0, \ttbr1, c2, c0, 1	@ load TTB1
	.endm

	__CPUINIT

	/*   AT
	 *  TFR   EV X F   I D LR    S
	 * .EEE ..EE PUI. .T.T 4RVI ZWRS BLDP WCAM
	 * rxxx rrxx xxx0 0101 xxxx xxxx x111 xxxx < forced
	 *   01    0 110       0011 1100 .111 1101 < we want
	 */
	.align	2
	.type	v7_crval, #object
v7_crval:
	crval	clear=0x2120c302, mmuset=0x10c03c7d, ucset=0x00c01c7c

	.previous
