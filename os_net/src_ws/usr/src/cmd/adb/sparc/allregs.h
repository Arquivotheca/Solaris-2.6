/*
 * Copyright (c) 1986,1995,1996, by Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#ident	"@(#)allregs.h	1.13	96/02/13 SMI"

/*
 * adb keeps its own idea of the current value of most of the
 * processor registers, in an "adb_regs" structure.  This is used
 * in different ways for kadb, adb -k, and normal adb.
 *
 * For kadb, this is really the machine state -- so kadb must
 * get the current window pointer (CWP) field of the psr and
 * use it to index into the windows to find the currently
 * relevant register set.
 *
 * For a normal adb, the TBR and WIM registers aren't present in the
 * struct regs that we get (either within a core file, or from
 * PTRACE_GETREGS); so I might use those two fields for something.
 * In this case, we always ignore the "locals" half of register
 * window zero.  Its "ins" half is used to hold the current "outs",
 * and window one has the current locals and "ins".
 *
 * For adb -k (post-mortem examination of kernel crash dumps), there
 * is no way to find the current globals or outs, but the stack frame
 * that sp points to will tell us the current locals and ins.  Because
 * we have no current outs, I suppose that we could use window zero for
 * the locals and ins, but I'd prefer to make it the same as normal adb.
 * Also, if the kernel crash-dumper is changed to make these available
 * somehow, I'd have to change things again.
 */

#ifndef _ALLREGS_H
#define	_ALLREGS_H

#ifndef KADB
#include <sys/reg.h>
#endif

#define	MAXKADBWIN	32

#ifndef _ASM

#include <sys/pcb.h>

struct allregs {
	int		r_psr;
	int		r_pc;
	int		r_npc;
	int		r_tbr;
	int		r_wim;
	int		r_y;
	int		r_globals[7];
#ifdef KADB
	struct rwindow	r_window[MAXKADBWIN];	/* locals, then ins */
#else
	int		r_outs[8];
	int		r_locals[8];
	int		r_ins[8];
#endif
};

struct allregs_v9 {
	u_longlong_t	r_tstate;
	int		r_pc;
	int		r_npc;
	int		r_tba;
	int		r_y;
	u_longlong_t	r_globals[7];
#ifdef KADB
	int		r_tt;
	int		r_pil;
	int		r_cwp;
	int		r_otherwin;
	int		r_cleanwin;
	int		r_cansave;
	int		r_canrestore;
	int		r_wstate;
	struct rwindow	r_window[MAXKADBWIN];	/* locals, then ins */
	u_longlong_t	r_outs[8];
#else
	int		r_outs[8];
	int		r_locals[8];
	int		r_ins[8];
#endif
};

#endif	/* _ASM */

/*
 * XXX - some v9 definitions from v9/sys/privregs.h. Need to define here
 * because we've already included v7/sys/privregs.h via other headers.
 * We could/should fix this by splitting up architecture version dependent
 * code.
 */

/*
 * Trap State Register (TSTATE)
 *
 *	|-------------------------------------|
 *	| CCR | ASI | --- | PSTATE | -- | CWP |
 *	|-----|-----|-----|--------|----|-----|
 *	 39 32 31 24 23 20 19	  8 7  5 4   0
 */
#define	TSTATE_CWP_MASK		0x01F
#define	TSTATE_CWP_SHIFT	0
#define	TSTATE_PSTATE_MASK	0xFFF
#define	TSTATE_PSTATE_SHIFT	8
#define	TSTATE_ASI_MASK		0x0FF
#define	TSTATE_ASI_SHIFT	24
#define	TSTATE_CCR_MASK		0x0FF
#define	TSTATE_CCR_SHIFT	32

/*
 * Some handy tstate macros
 */
#define	TSTATE_AG	(PSTATE_AG << TSTATE_PSTATE_SHIFT)
#define	TSTATE_IE	(PSTATE_IE << TSTATE_PSTATE_SHIFT)
#define	TSTATE_PRIV	(PSTATE_PRIV << TSTATE_PSTATE_SHIFT)
#define	TSTATE_AM	(PSTATE_AM << TSTATE_PSTATE_SHIFT)
#define	TSTATE_PEF	(PSTATE_PEF << TSTATE_PSTATE_SHIFT)
#define	TSTATE_MG	(PSTATE_MG << TSTATE_PSTATE_SHIFT)
#define	TSTATE_IG	(PSTATE_IG << TSTATE_PSTATE_SHIFT)
#define	TSTATE_CWP	TSTATE_CWP_MASK

/*
 * Processor State Register (PSTATE)
 *
 *   |-------------------------------------------------------------|
 *   |  IG | MG | CLE | TLE | MM | RED | PEF | AM | PRIV | IE | AG |
 *   |-----|----|-----|-----|----|-----|-----|----|------|----|----|
 *	11   10	   9     8   7  6   5	  4     3     2	    1    0
 */
#define	PSTATE_AG	0x001		/* alternate globals */
#define	PSTATE_IE	0x002		/* interrupt enable */
#define	PSTATE_PRIV	0x004		/* privileged mode */
#define	PSTATE_AM	0x008		/* use 32b address mask */
#define	PSTATE_PEF	0x010		/* fp enable */
#define	PSTATE_RED	0x020		/* red mode */
#define	PSTATE_MM	0x0C0		/* memory model */
#define	PSTATE_TLE	0x100		/* trap little endian */
#define	PSTATE_CLE	0x200		/* current little endian */
#define	PSTATE_MG	0x400		/* MMU globals */
#define	PSTATE_IG	0x800		/* interrupt globals */

#endif	/* !_ALLREGS_H */
