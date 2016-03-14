/*-
 * Copyright (c) 2007 Bruce M. Simpson.
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
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/imgact.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/cons.h>
#include <sys/exec.h>
#include <sys/ucontext.h>
#include <sys/proc.h>
#include <sys/kdb.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/user.h>

#include <vm/vm.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>

#include <machine/cache.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#include <machine/cpuinfo.h>
#include <machine/cpufunc.h>
#include <machine/cpuregs.h>
#include <machine/cpuregs_74k.h>
#include <machine/hwfunc.h>
#include <machine/intr_machdep.h>
#include <machine/locore.h>
#include <machine/md_var.h>
#include <machine/pte.h>
#include <machine/sigframe.h>
#include <machine/trap.h>
#include <machine/vmparam.h>

#include <mips/sentry5/s5reg.h>

#ifdef CFE
#include <dev/cfe/cfe_api.h>
#endif

#define S5_TRACE 1

extern int *edata;
extern int *end;

void
platform_cpu_init()
{
	/* Nothing special */
}

#define	BCM4710_REG_CHIPC		0xb8000000
#define	  BCM4710_REG_CHIPC_PMUWD_OFFS	 0x634
#define	BCM4710_REG_MIPS		0xb8003000	/* MIPS core registers */

typedef volatile struct {
        uint32_t        corecontrol;
        uint32_t        exceptionbase;
        uint32_t        PAD1[1];
        uint32_t        biststatus;
        uint32_t        intstatus;
        uint32_t        intmask[6];
        uint32_t        nmimask;
        uint32_t        PAD2[4];
        uint32_t        gpioselect;
        uint32_t        gpiooutput;
        uint32_t        gpioenable;
        uint32_t        PA3[101];
        uint32_t        clkcontrolstatus;
} bcm4710_m74kregs_t;

/*
 * Fix timer interrupt on BCM471x, unless you do this after boot the timer
 * interrupt won't deassept after write into COP0[compare].
 */
static void
bcm4710_ti_fix(void)
{
	bcm4710_m74kregs_t *regs;
	uint32_t volatile *intmask;

	regs = (bcm4710_m74kregs_t *)BCM4710_REG_MIPS;
	/* Use intmask5 register to route the timer interrupt */
	intmask = (uint32_t volatile *)&regs->intmask[5];
	writel(intmask, 1 << 31);
}

static void
mips_init(void)
{
	int i, j;
#if 0
	unsigned int c7;
#endif

	printf("entry: mips_init()\n");

#ifdef S5_TRACE
	boothowto |= RB_VERBOSE;
#endif

#ifdef CFE
	/*
	 * Query DRAM memory map from CFE.
	 */
	physmem = 0;
	for (i = 0; i < 10; i += 2) {
		int result;
		uint64_t addr, len, type;

		result = cfe_enummem(i / 2, 0, &addr, &len, &type);
		if (result < 0) {
#ifdef S5_TRACE
			printf("There is no phys memory for: %d\n", i);
#endif
			phys_avail[i] = phys_avail[i + 1] = 0;
			break;
		}
		if (type != CFE_MI_AVAILABLE){
#ifdef S5_TRACE
			printf("phys memory is not available: %d\n", i);
#endif
			continue;
		}

		phys_avail[i] = addr;
		if (i == 0 && addr == 0) {
			/*
			 * If this is the first physical memory segment probed
			 * from CFE, omit the region at the start of physical
			 * memory where the kernel has been loaded.
			 */
			phys_avail[i] += MIPS_KSEG0_TO_PHYS(kernel_kseg0_end);
		}
#ifdef S5_TRACE
		printf("phys memory is available for: %d\n", i);
		printf(" => addr =  %jx\n", addr);
		printf(" => len =  %jd\n", len);
#endif
		phys_avail[i + 1] = addr + len;
		physmem += len;
	}

#if 0
	c7 = mips_rd_config7();
	if (c7 & (1 << 29)) {
		mips_wr_config7(c7 & ~(1 << 29));
		printf("config7 = %u\n", mips_rd_config7());
	}

	printf("perfcnt = %u, %u, %u, %u\n", mips_rd_perfcnt_ctrl0(),
	    mips_rd_perfcnt_ctrl1(), mips_rd_perfcnt_ctrl2(),
	    mips_rd_perfcnt_ctrl3());

	mips_wr_perfcnt_ctrl0(mips_rd_perfcnt_ctrl0() & ~(1 << 4));
	mips_wr_perfcnt_ctrl1(mips_rd_perfcnt_ctrl1() & ~(1 << 4));
	mips_wr_perfcnt_ctrl2(mips_rd_perfcnt_ctrl2() & ~(1 << 4));
	mips_wr_perfcnt_ctrl3(mips_rd_perfcnt_ctrl3() & ~(1 << 4));
#endif

#ifdef S5_TRACE
	printf("Total phys memory is : %ld\n", physmem);
#endif

	realmem = btoc(physmem);
#endif

	for (j = 0; j < i; j++)
		dump_avail[j] = phys_avail[j];

	physmem = realmem;

	init_param1();
	init_param2(physmem);
	mips_cpu_init();
	pmap_bootstrap();
	mips_proc0_init();
	mutex_init();
	kdb_init();
#ifdef KDB
	if (boothowto & RB_KDB)
		kdb_enter(KDB_WHY_BOOTFLAGS, "Boot flags requested debugger");
#endif
}

void
platform_reset(void)
{
	void *pmuwatchdog;

	printf("bcm471x::platform_reset()\n");
	pmuwatchdog = (void *)(BCM4710_REG_CHIPC + BCM4710_REG_CHIPC_PMUWD_OFFS);
	intr_disable();
	writel(pmuwatchdog, 2);
	for (;;);
}

void
platform_start(__register_t a0, __register_t a1, __register_t a2,
	       __register_t a3)
{
	vm_offset_t kernend;
	uint64_t platform_counter_freq;
	unsigned int intctl, ts_hwX;

	/* clear the BSS and SBSS segments */
	kernend = (vm_offset_t)&end;
	memset(&edata, 0, kernend - (vm_offset_t)(&edata));

	mips_postboot_fixup();

	/* Initialize pcpu stuff */
	mips_pcpu0_init();

	platform_counter_freq = 480 * 1000 * 1000;
	mips_timer_early_init(platform_counter_freq);

#ifdef CFE
	/*
	 * Initialize CFE firmware trampolines before
	 * we initialize the low-level console.
	 *
	 * CFE passes the following values in registers:
	 * a0: firmware handle
	 * a2: firmware entry point
	 * a3: entry point seal
	 */
	if (a3 == CFE_EPTSEAL)
		cfe_init(a0, a2);
#endif
	cninit();

	mips_init();

	bcm4710_ti_fix();

	/*
	 * The section below does not really belong here, it should
	 * probably go into CPU-specific nexus or maybe into timer probe
	 * routine. It's 74k prescribed way of finding out what IPx the TI is
	 * routed to. The irony that the BCM471x is broken in that regard, it
	 * lists TI as merged into HW3 line, while in fact it's HW5 just
	 * like pretty much any other mips out there, so if we move this
	 * code it needs to be replaced with the override.
	 */
	intctl = mips_rd_intctl();
	printf("intctl = 0x%08x\n", intctl);
	ts_hwX = (intctl & MIPS_INTCTL_IPTI_MASK) >> MIPS_INTCTL_IPTI_SHIFT;
	if (MIPS_INTCTL_IRQ_VAL(ts_hwX)) {
        	ts_hwX -= MIPS_INTCTL_IRQ_HW0;
        	if (ts_hwX != counter_irq) {
			const char *ps;
#if 0
			ps = "";
			counter_irq = ts_hwX;
#else
			ps = "NOT ";
#endif
			printf("%soverriding TI irq from default %u to %u\n",
			    ps, counter_irq, ts_hwX);
		}
	}

# if 0
	/*
	 * Probe the Broadcom Sentry5's on-chip PLL clock registers
	 * and discover the CPU pipeline clock and bus clock
	 * multipliers from this.
	 * XXX: Wrong place. You have to ask the ChipCommon
	 * or External Interface cores on the SiBa.
	 */
	uint32_t busmult, cpumult, refclock, clkcfg1;
#define S5_CLKCFG1_REFCLOCK_MASK	0x0000001F
#define S5_CLKCFG1_BUSMULT_MASK		0x000003E0
#define S5_CLKCFG1_BUSMULT_SHIFT	5
#define S5_CLKCFG1_CPUMULT_MASK		0xFFFFFC00
#define S5_CLKCFG1_CPUMULT_SHIFT	10

	counter_freq = 100000000;	/* XXX */

	clkcfg1 = s5_rd_clkcfg1();
	printf("clkcfg1 = 0x%08x\n", clkcfg1);

	refclock = clkcfg1 & 0x1F;
	busmult = ((clkcfg1 & 0x000003E0) >> 5) + 1;
	cpumult = ((clkcfg1 & 0xFFFFFC00) >> 10) + 1;

	printf("refclock = %u\n", refclock);
	printf("busmult = %u\n", busmult);
	printf("cpumult = %u\n", cpumult);

	counter_freq = cpumult * refclock;
# else
	platform_counter_freq = 500 * 1000 * 1000; /* BCM4718 is 500MHz */
# endif

	/* BCM471x timer is 1/2 of Clk */
	mips_timer_init_params(platform_counter_freq, 1);
}
