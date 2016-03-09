/*
 * Copyright (c) 2016 Maxim Sobolev <sobomax@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _MACHINE_CPUREGS_74K_H_
#define _MACHINE_CPUREGS_74K_H_

#if !defined(_MACHINE_CPUFUNC_H_)
#error machine/cpufunc.h needs to be included
#endif

/*
 * The IntCtl register controls the interrupt capabilities of the 74K core,
 * including vectored interrupts and support for an external interrupt
 * controller.
 */
#define	MIPS_COP_0_INTCTL	12
#define	MIPS_COP_0_INTCTL_SEL	1

/*
 * bits 29..31: For Interrupt Compatibility and Vectored Interrupt modes, this
 * field specifies the IP number to which the Timer Interrupt request is
 * merged, and allows software to determine whether to consider CauseTI for a
 * potential interrupt.
 */
#define	MIPS_INTCTL_IPTI_SHIFT		(29)
#define	MIPS_INTCTL_IPTI_MASK		(7 << MIPS_INTCTL_IPTI_SHIFT)

/*
 * bits 26..28: For Interrupt Compatibility and Vectored Interrupt modes,
 * this field specifies the IP number to which the Performance Counter
 * Interrupt request is merged, and allows software to determine whether to
 * consider Cause[PCI] for a potential interrupt.
 */
#define	MIPS_INTCTL_IPPCI_SHIFT		(26)
#define	MIPS_INTCTL_IPPCI_MASK		(7 << MIPS_INTCTL_IPPCI_SHIFT)
/*
 * bits 23:25: For Interrupt Compatibility and Vectored Interrupt modes, this
 * field specifies the IP number to which the Fast Debug Channel Interrupt
 * request is merged, and allows software to determine whether to consider
 * Cause[FDCI] for a potential interrupt.
 */
#define	MIPS_INTCTL_IPFDCI_SHIFT	(23)
#define	MIPS_INTCTL_IPFDCI_MASK		(7 << MIPS_INTCTL_IPFDCI_SHIFT)
/* 
 * bits 5:9: If vectored interrupts are implemented (as denoted by
 * Config3[VInt] or Config3[VEIC]), this field specifies the spacing between
 * vectored interrupts.
 */
#define	MIPS_INTCTL_VS_SHIFT		(5)
#define	MIPS_INTCTL_VS_MASK		(7 << MIPS_INTCTL_VS_SHIFT)

/* Hardware Interrupt Sources (shared by IPTI, IPPCI, IPFDCI */
#define	MIPS_INTCTL_IRQ_HW0		(2)
#define	MIPS_INTCTL_IRQ_HW1		(3)
#define	MIPS_INTCTL_IRQ_HW2		(4)
#define	MIPS_INTCTL_IRQ_HW3		(5)
#define	MIPS_INTCTL_IRQ_HW4		(6)
#define	MIPS_INTCTL_IRQ_HW5		(7)
#define	MIPS_INTCTL_IRQ_VAL(i)		((i) >= MIPS_INTCTL_IRQ_HW0)

typedef volatile struct {
	uint32_t	corecontrol;
	uint32_t	exceptionbase;
	uint32_t	PAD1[1];
	uint32_t	biststatus;
	uint32_t	intstatus;
	uint32_t	intmask[6];
	uint32_t	nmimask;
	uint32_t	PAD2[4];
	uint32_t	gpioselect;
	uint32_t	gpiooutput;
	uint32_t	gpioenable;
	uint32_t	PA3[101];
	uint32_t	clkcontrolstatus;
} mips74kregs_t;

MIPS_RW32_COP0_SEL(intctl, MIPS_COP_0_INTCTL, MIPS_COP_0_INTCTL_SEL);

#endif /* _MACHINE_CPUREGS_74K_H_ */
