/*
 * Copyright (c) 1990-1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)hardclk.c	1.38	96/10/21 SMI"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/clock.h>
#include <sys/intreg.h>
#include <sys/cpuvar.h>
#include <sys/pte.h>
#include <sys/debug.h>
#include <sys/machlock.h>
#include <sys/mmu.h>
#include <sys/cmn_err.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <sys/sunddi.h>

extern void splx();

#ifndef KADB

/*
 * Machine-dependent clock routines.
 */

#define	CLOCK_RES	1000		/* 1 microsec in nanosecs */

int clock_res = CLOCK_RES;

#define	POST_BUG	/* workaround bug in OBP 2.[0123] POST code */

/*
 * Start the real-time clock.
 */
void
clkstart()
{
#ifdef POST_BUG
	/*
	 * Some early SS-2 PROMs had POST routines that scribbled
	 * on the calibration register of the tod clock.  We repair
	 * that here.  The bits that are set are the SIGN bit and
	 * five bits of calibration value (the maximum) => 0x3f.
	 *
	 * Without this fix, the tod clock runs fast by 3 minutes a month.
	 * See 1068412.  PROMs of 2.4 or later vintage are fixed.
	 */
	if (CLOCK->clk_ctrl == 0x3f) {
		struct pte pte;
		u_int saveprot;
#ifdef DEBUG
		cmn_err(CE_CONT, "?Zeroing bad TOD register contents\n");
#endif
		mmu_getpte((caddr_t)CLOCK, &pte);
		saveprot = pte.pg_prot;
		pte.pg_prot = KW;
		mmu_setpte((caddr_t)CLOCK, pte);
		CLOCK->clk_ctrl |= CLK_CTRL_WRITE;
		CLOCK->clk_ctrl = 0;	/* clear it all */
		mmu_getpte((caddr_t)CLOCK, &pte);
		pte.pg_prot = saveprot;
		mmu_setpte((caddr_t)CLOCK, pte);
	}
#endif /* POST_BUG */
	/*
	 * Start counter in a loop to interrupt hz times/second.
	 */
	COUNTER->limit10 = (((1000000 / hz) + 1) << CTR_USEC_SHIFT)
		& CTR_USEC_MASK;
	/*
	 * Turn on level 10 clock intr.
	 */
	set_clk_mode(IR_ENA_CLK10, 0);
}

/*
 * enable profiling timer
 */
/* ARGSUSED */
int
enable_profiling(int cpuid)
{
	/*
	 * set interval timer to go off ever 10 ms
	 */
	COUNTER->limit14 = (10000 << CTR_USEC_SHIFT) & CTR_USEC_MASK;
	/*
	 * Turn on level 14 clock for profiling
	 */
	set_clk_mode(IR_ENA_CLK14, 0);

	return (0);
}

/*
 * disable profiling timer
 */
/* ARGSUSED */
void
disable_profiling(int cpuid)
{
	/*
	 * turn off level 14 clock
	 */
	set_clk_mode(0, IR_ENA_CLK14);
}

/*
 *  acknowledge the occurence of a profiling interrupt
 */
/* ARGSUSED */
void
clear_profiling_intr(int cpuid)
{
	/*
	 * Sun C 2.0 compiler bug,
	 *
	 * COUNTER->limit14;
	 *
	 * is optimized away, despite COUNTER pointing to a volatile.
	 */

	volatile struct counterregs *cr = COUNTER;

	(void) cr->limit14;
}

int
enable_profiling_interrupt(dev_info_t *devi, u_int (*intr_handler)())
{
	if (have_fast_profile_intr) {
		if (ddi_add_fastintr(devi, 0, NULL, NULL,
			(u_int (*)())fast_profile_intr) != DDI_SUCCESS) {
			cmn_err(CE_NOTE, "profile_attach: fast attach failed");
			return (DDI_FAILURE);
		}
	} else {
		if (ddi_add_intr(devi, 0, NULL, NULL,
			intr_handler, (caddr_t) 0) != DDI_SUCCESS) {
				return (DDI_FAILURE);
		}
	}
	return (DDI_SUCCESS);
}


int
disable_profiling_interrupt(dev_info_t *devi)
{
	/*
	 * Remove the interrupt vector.
	 */
	ddi_remove_intr(devi, (u_int) 0, NULL);

	return (DDI_SUCCESS);
}
#endif KADB

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register. Because the counter interrupts are level sensitive, not
 * edge sensitive, we no longer have to be careful about wedging time.
 * We clear outstanding clock interrupts since they will surely be
 * piled up. However, our first interval is still of random length, since
 * we do not reset the counters.
 */
void
set_clk_mode(u_char on, u_char off)
{
	register u_char intreg, dummy;
	register int s;

	/*
	 * make sure that we are only playing w/
	 * clock interrupt register bits
	 */
	on &= (IR_ENA_CLK14 | IR_ENA_CLK10);
	off &= (IR_ENA_CLK14 | IR_ENA_CLK10);

	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
#ifndef KADB
	s = spl7();
#endif
	intreg = *INTREG & ~(off | IR_ENA_INT);

	/*
	 * Next we turns off the CLK10 and CLK14 bits to avoid any
	 * triggers, and clear any outstanding clock interrupts.
	 */
	*INTREG &= ~(IR_ENA_CLK14 | IR_ENA_CLK10);
	/* SAS simulates the counters, so okay to clear any interrupt */
	dummy = COUNTER->limit10;
	dummy = COUNTER->limit14;
#if defined(lint) || defined(__lint)
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register.
	 */
	*INTREG |= (intreg | on);		/* enable interrupts */
#ifndef KADB
	(void) splx(s);
#endif
}


#ifndef KADB			/* to EOF */

/*
 * Write the specified time into the clock chip.
 * Must be called with tod_lock held.
 */
void
tod_set(timestruc_t ts)
{
	todinfo_t tod = utc_to_tod(ts.tv_sec);
	struct pte pte;
	unsigned int saveprot;

	ASSERT(MUTEX_HELD(&tod_lock));

#if !defined(SAS) && !defined(MPSAS)
	/*
	 * The eeprom (which also contains the tod clock) is normally
	 * marked read only, change it temporarily it update tod.
	 */
	mmu_getpte((caddr_t)CLOCK, &pte);
	saveprot = pte.pg_prot;
	pte.pg_prot = KW;
	mmu_setpte((caddr_t)CLOCK, pte);

	CLOCK->clk_ctrl |= CLK_CTRL_WRITE;	/* allow writes */
	CLOCK->clk_year		= BYTE_TO_BCD(tod.tod_year - YRBASE);
	CLOCK->clk_month	= BYTE_TO_BCD(tod.tod_month);
	CLOCK->clk_day		= BYTE_TO_BCD(tod.tod_day);
	CLOCK->clk_weekday	= BYTE_TO_BCD(tod.tod_dow);
	CLOCK->clk_hour		= BYTE_TO_BCD(tod.tod_hour);
	CLOCK->clk_min		= BYTE_TO_BCD(tod.tod_min);
	CLOCK->clk_sec		= BYTE_TO_BCD(tod.tod_sec);
	CLOCK->clk_ctrl &= ~CLK_CTRL_WRITE;	/* load values */

	/*
	 * Now write protect it, preserving the new modify/ref bits
	 */
	mmu_getpte((caddr_t)CLOCK, &pte);
	pte.pg_prot = saveprot;
	mmu_setpte((caddr_t)CLOCK, pte);
#endif
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the clock chip is valid.
 * Must be called with tod_lock held.
 */
timestruc_t
tod_get(void)
{
	timestruc_t ts;
	todinfo_t tod;
	struct pte pte;
	unsigned int saveprot;

	ASSERT(MUTEX_HELD(&tod_lock));

#if	!defined(SAS) && !defined(MPSAS)
	/*
	 * Turn off updates so we can read the clock cleanly.
	 */
	mmu_getpte((caddr_t)CLOCK, &pte);
	saveprot = pte.pg_prot;
	pte.pg_prot = KW;
	mmu_setpte((caddr_t)CLOCK, pte);

	CLOCK->clk_ctrl |= CLK_CTRL_READ;
	tod.tod_year	= BCD_TO_BYTE(CLOCK->clk_year) + YRBASE;
	tod.tod_month	= BCD_TO_BYTE(CLOCK->clk_month & 0x1f);
	tod.tod_day	= BCD_TO_BYTE(CLOCK->clk_day & 0x3f);
	tod.tod_dow	= BCD_TO_BYTE(CLOCK->clk_weekday & 0x7);
	tod.tod_hour	= BCD_TO_BYTE(CLOCK->clk_hour & 0x3f);
	tod.tod_min	= BCD_TO_BYTE(CLOCK->clk_min & 0x7f);
	tod.tod_sec	= BCD_TO_BYTE(CLOCK->clk_sec & 0x7f);
	CLOCK->clk_ctrl &= ~CLK_CTRL_READ;

	/*
	 * Now write protect it, preserving the new modify/ref bits
	 */
	mmu_getpte((caddr_t)CLOCK, &pte);
	pte.pg_prot = saveprot;
	mmu_setpte((caddr_t)CLOCK, pte);
#endif !SAS

	ts.tv_sec = tod_to_utc(tod);
	ts.tv_nsec = 0;

	return (ts);
}

/*
 * The following wrappers have been added so that locking
 * can be exported to platform-independent clock routines
 * (ie adjtime(), clock_setttime()), via a functional interface.
 */
int
hr_clock_lock()
{
	return (CLOCK_LOCK());
}

void
hr_clock_unlock(int s)
{
	CLOCK_UNLOCK(s);
}
#endif KADB
