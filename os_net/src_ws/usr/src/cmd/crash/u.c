/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1991, 1996 by Sun Microsystems, Inc
 *	All rights reserved.
 *
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 *		RESTRICTED RIGHTS LEGEND:
 *   Use, duplication, or disclosure by the Government is subject
 *   to restrictions as set forth in subparagraph (c)(1)(ii) of
 *   the Rights in Technical Data and Computer Software clause at
 *   DFARS 52.227-7013 and in similar clauses in the FAR and NASA
 *   FAR Supplement.
 */

#pragma	ident	"@(#)u.c	1.31	96/04/18 SMI"	/* SVr4.0 1.20.12.1 */

/*
 * This file contains code for the crash functions:  user, pcb, stack, trace,
 * and kfp.
 */

#include <sys/param.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <time.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/psw.h>
#include <sys/pcb.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/lock.h>
#include <sys/cred.h>
#include <sys/user.h>
#include <sys/vmparam.h>

/* See notes below. */
#ifdef notdef
#ifndef i386
#include <sys/machpcb.h>
#endif
#endif

#if defined(i386)
#include <sys/fp.h>
#include <ieeefp.h>
#include <string.h>
#include <stdlib.h>
#endif /* i386 */

#if	defined(__ppc)
#include <sys/frame.h>
#endif	/* __ppc */

#include "crash.h"

#define	min(a, b) (a > b ? b : a)

#define	DATE_FMT	"%a %b %e %H:%M:%S %Y\n"

/*
 * %a	abbreviated weekday name %b	abbreviated month name %e	day
 * of month %H	hour %M	minute %S	second %Y	year
 */

struct proc procbuf;		/* proc entry buffer */
static char time_buf[50];	/* holds date and time string */

int *stk_bptr;			/* stack pointer */
void free();

static void pruser(int, int, long, int);
#if i386
static void prpcb(unsigned, int);
#else /* !i386 */
static void prpcb(kthread_id_t);
#endif /* i386 */
static void prstack(char *, unsigned, unsigned);
static void prktrace(unsigned);
static void prustk(unsigned, int);
static void prkstk(unsigned, int);
static void prkfp(unsigned);

static const char *limits[] = {
	"cpu time",
	"file size",
	"swap size",
	"stack size",
	"coredump size",
	"file descriptors",
	"address space"
};

/* read ublock into buffer */
int
getublock(int slot, long addr)
{
	struct proc *procp;

	if (addr == -1) {
		if (slot == -1)
			slot = getcurproc();
		if (slot >= vbuf.v_proc || slot < 0) {
			prerrmes("%d out of range\n", slot);
			return (-1);
		}
		procp = slot_to_proc(slot);
		if (procp == NULL)
			return (-1);
	} else
		procp = (struct proc *) addr;

	/* Virtmode must be 1 */
	readbuf((unsigned)procp, 0, 0, -1, (char *)&procbuf, sizeof (procbuf),
		"process table");

	if (procbuf.p_stat == SZOMB) {
		prerrmes("%d is a zombie process\n", slot);
		return (-1);
	}
	ubp = kvm_getu(kd, &procbuf);

	return ((ubp == NULL) ? -1 : 0);
}

int
getlwpblock(struct _kthread *tp, struct _klwp *lp)
{
	readmem((unsigned)tp->t_lwp, 1, -1, lp, sizeof (struct _klwp), "lwp");
	return (0);
}


/* get arguments for user function */
int
getuser(void)
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int lock = 0;
	long arg1 = -1;
	long arg2 = -1;
	long addr = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "eflw:")) != EOF) {
		switch (c) {
			case 'f':
				full = 1;
				break;
			case 'e':
				all = 1;
				break;
			case 'l':
				lock = 1;
				break;
			case 'w':
				redirect();
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			getargs(vbuf.v_proc, &arg1, &arg2, 0);
			if (arg1 == -1)
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					pruser(full, slot, addr, lock);
			else {
				if ((unsigned long) arg1 < vbuf.v_proc)
					slot = arg1;
				else
					addr = arg1;
				pruser(full, slot, addr, lock);
			}
			slot = arg1 = arg2 = -1;
		} while (args[++optind]);
	} else if (all) {
		readmem((long) V->st_value, 1, -1, &vbuf,
			sizeof (vbuf), "var structure");
		for (slot = 0; slot < vbuf.v_proc; slot++)
			pruser(full, slot, addr, lock);
	} else
		pruser(full, slot, addr, lock);
	return (0);
}

/* print ublock */
static void
pruser(int full, int slot, long addr, int lock)
{
	register int i, j;
	struct uf_entry  *uf;
	int sz;

	if (slot == -1 && addr == -1)
		slot = getcurproc();

	if (getublock(slot, addr) == -1)
		return;

	if (addr != -1)
		fprintf(fp, "PER PROCESS USER AREA FOR PROCESS AT ADDRESS %x\n",
			addr);
	else
		fprintf(fp, "PER PROCESS USER AREA FOR PROCESS %d\n", slot);

	fprintf(fp, "PROCESS MISC:\n");
	fprintf(fp, "\tcommand: %s,", ubp->u_comm);
	fprintf(fp, " psargs: %s\n", ubp->u_psargs);
	cftime(time_buf, DATE_FMT, &ubp->u_start);
	fprintf(fp, "\tstart: %s", time_buf);
	fprintf(fp, "\tmem: %x, type: %s%s\n",
		ubp->u_mem,
		ubp->u_acflag & AFORK ? "fork" : "exec",
		ubp->u_acflag & ASU ? " su-user" : "");
	if (ubp->u_cdir)
		fprintf(fp, "\tvnode of current directory: %8x", ubp->u_cdir);
	else
		fprintf(fp, " - ,");
	if (ubp->u_rdir)
		fprintf(fp, ", vnode of root directory: %8x,", ubp->u_rdir);
	fprintf(fp, "\nOPEN FILES, POFILE FLAGS, AND THREAD REFCNT:");
	sz = ubp->u_nofiles * sizeof (uf_entry_t);
	uf = (uf_entry_t *)malloc(sz);
	readmem((long) ubp->u_flist, 1, -1, uf, sz, "user file array");
	for (i = 0, j = 0; i < ubp->u_nofiles; i++) {
		if (uf[i].uf_ofile != 0) {
			if ((j++ % 2) == 0)
				fprintf(fp, "\n");
			fprintf(fp, "\t[%d]: F %#.8x, %x, %d", i,
				uf[i].uf_ofile,
				uf[i].uf_pofile,
				uf[i].uf_refcnt);
		}
	}
	free(uf);
	fprintf(fp, "\n");
	fprintf(fp, " cmask: %4.4o\n", ubp->u_cmask);
	fprintf(fp, "RESOURCE LIMITS:\n");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (limits[i] == 0)
			continue;
		fprintf(fp, "\t%s: ", limits[i]);
		if (ubp->u_rlimit[i].rlim_cur == RLIM_INFINITY)
			fprintf(fp, "unlimited/");
		else
			fprintf(fp, "%llu/", ubp->u_rlimit[i].rlim_cur);
		if (ubp->u_rlimit[i].rlim_max == RLIM_INFINITY)
			fprintf(fp, "unlimited\n");
		else
			fprintf(fp, "%llu\n", ubp->u_rlimit[i].rlim_max);
	}
	fprintf(fp, "SIGNAL DISPOSITION:");
	for (i = 0; i < MAXSIG; i++) {
		if (!(i & 3))
			fprintf(fp, "\n\t");
		fprintf(fp, "%4d: ", i + 1);
		if ((int) ubp->u_signal[i] == 0 || (int) ubp->u_signal[i] == 1)
			fprintf(fp, "%8s",
				(int) ubp->u_signal[i] ? "ignore" : "default");
		else
			fprintf(fp, "%-8x", (int) ubp->u_signal[i]);
	}
	fprintf(fp, "\n");
	if (full) {
		fprintf(fp, "\tnshmseg: %d\n",
			ubp->u_nshmseg);
		fprintf(fp, " tsize: %x, dsize: %x\n",
			ubp->u_tsize,
			ubp->u_dsize);
		fprintf(fp, "\tticks: %x\n",
			ubp->u_ticks);
		fprintf(fp, "\tsystrap: %d\n",
			ubp->u_systrap);
		fprintf(fp, "\tentrymask:");
		for (i = 0; i < sizeof (k_sysset_t) / sizeof (long); i++)
			fprintf(fp, " %08x", ubp->u_entrymask.word[i]);
		fprintf(fp, "\n");
		fprintf(fp, "\texitmask:");
		for (i = 0; i < sizeof (k_sysset_t) / sizeof (long); i++)
			fprintf(fp, " %08x", ubp->u_exitmask.word[i]);
		fprintf(fp, "\n");
		fprintf(fp, "\n\tEXDATA:\n");
		fprintf(fp, "\tvp: ");
		if (ubp->u_exdata.vp)
			fprintf(fp, " %8x,", ubp->u_exdata.vp);
		else
			fprintf(fp, " - , ");
		fprintf(fp, "tsize: %x, dsize: %x, bsize: %x, lsize: %x\n",
			ubp->u_exdata.ux_tsize,
			ubp->u_exdata.ux_dsize,
			ubp->u_exdata.ux_bsize,
			ubp->u_exdata.ux_lsize);
		fprintf(fp,
			"\tmagic#: %o, toffset: %x, doffset: %x, loffset: %x\n",
			ubp->u_exdata.ux_mag,
			ubp->u_exdata.ux_toffset,
			ubp->u_exdata.ux_doffset,
			ubp->u_exdata.ux_loffset);
		fprintf(fp,
			"\ttxtorg: %x, datorg: %x, entloc: %x, nshlibs: %d\n",
			ubp->u_exdata.ux_txtorg,
			ubp->u_exdata.ux_datorg,
			ubp->u_exdata.ux_entloc,
			ubp->u_exdata.ux_nshlibs);
		fprintf(fp, "\texecsz: %x\n", ubp->u_execsz);
		fprintf(fp, "\n\tSIGNAL MASK:");
		for (i = 0; i < MAXSIG; i++) {
			if (!(i & 3))
				fprintf(fp, "\n\t");
			fprintf(fp, "%4d: %-8x %-8x", i + 1,
				ubp->u_sigmask[i].__sigbits[0],
				ubp->u_sigmask[i].__sigbits[1]);
		}
	}
	fprintf(fp, "\n");
	if (lock) {
		prcondvar(&ubp->u_cv, "u_cv");
		fprintf(fp, "u_flock: ");
		prmutex(&(ubp->u_flock));
	}
}

/* get arguments for pcb function */
int
getpcb(void)
{
#if i386
	int full = 0;
#endif /* i386 */
	long addr = -1;
	int c;

	optind = 1;
#if i386
	while ((c = getopt(argcnt, args, "fw:")) != EOF) {
#else /* !i386 */
	while ((c = getopt(argcnt, args, "w:")) != EOF) {
#endif /* !i386 */
		switch (c) {
#if i386
			case 'f':
				full = 1;
				break;
#endif /* i386 */
			case 'w':
				redirect();
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			if ((addr = strcon(args[optind], 'h')) == -1)
				continue;
#if i386
			prpcb(addr, full);
#else /* !i386 */
			prpcb((kthread_id_t)addr);
#endif /* i386 */
		} while (args[++optind]);
	} else {
#if i386
		prpcb((uint)Curthread, full);
#else /* !i386 */
		prpcb((kthread_id_t)Curthread);
#endif /* i386 */
	}
	return (0);
}

#if i386

/*
 * Are we emulating?  If not what kind of fpu h/w we have here?
 * The return value will be:
 *      0       no floating point support present
 *      1       80387 software emulator is present
 *      2       80287 chip is present
 *      3       80387 chip is present
 */
static int
fpu_avail(void)
{
	int fp_kind;
	Elf32_Sym *fp_kind_sym;

	if (!(fp_kind_sym = symsrch("fp_kind")))
		error("fp_kind not found in symbol table\n");
	readmem((long) fp_kind_sym->st_value, 1, -1, (char *) &fp_kind,
	    sizeof (fp_kind), "fp_kind");
	return (fp_kind);
}

/*
 * Print the FPU control word.
 */
static void
fpu_cw_print(const unsigned short cw, const int full)
{
	fprintf(fp, "\n\tcw      %#x", cw);
	if (cw && full)
		fprintf(fp, ": %s%s%s%s%s%s%s%s%s%s",
		    (cw & FPINV) ? "FPINV " : "",
		    (cw & FPDNO) ? "FPDNO " : "",
		    (cw & FPZDIV) ? "FPZDIV " : "",
		    (cw & FPOVR) ? "FPOVR " : "",
		    (cw & FPUNR) ? "FPUNR " : "",
		    (cw & FPPRE) ? "FPPRE " : "",
		    (cw & FPPC) ? "FPPC " : "",
		    (cw & FPRC) ? "FPRC " : "",
		    (cw & FPIC) ? "FPIC " : "",
		    (cw & WFPDE) ? "WFPDE " : "");
}

/*
 * Print the FPU status word.
 */
static void
fpu_sw_print(const char *name, const unsigned short sw,
    unsigned short *const top, const int full)
{
	fprintf(fp, "\n\t%s      %#x", name, sw);
	*top = (int) (sw & FPS_TOP) >> 11;
	if (sw && full)
		fprintf(fp, ": top=%d %s%s%s%s%s%s%s%s%s%s%s%s%s", *top,
		    (sw & FPS_IE) ? "FPS_IE " : "",
		    (sw & FPS_DE) ? "FPS_DE " : "",
		    (sw & FPS_ZE) ? "FPS_ZE " : "",
		    (sw & FPS_OE) ? "FPS_OE " : "",
		    (sw & FPS_UE) ? "FPS_UE " : "",
		    (sw & FPS_PE) ? "FPS_PE " : "",
		    (sw & FPS_SF) ? "FPS_SF " : "",
		    (sw & FPS_ES) ? "FPS_ES " : "",
		    (sw & FPS_C0) ? "FPS_C0 " : "",
		    (sw & FPS_C1) ? "FPS_C1 " : "",
		    (sw & FPS_C2) ? "FPS_C2 " : "",
		    (sw & FPS_C3) ? "FPS_C3 " : "",
		    (sw & FPS_B) ? "FPS_B " : "");
}

/*
 * Print the indexed FPU data register.
 */
static void
fpreg_print(const struct _fpreg *fpreg, const int precision, const int index,
	    const unsigned short top, const unsigned short tag)
{
	int decpt, sign, n;
	char buf[128], *bufp = buf;

	bufp = qecvt(*(long double *) fpreg, precision + 1, &decpt, &sign);

	n = fprintf(fp, " st[%d]\t%s%c", index, sign ? "-" : "+", *bufp++);
	if (!isdigit(*bufp))		/* in case of NaN or INF */
		n += fprintf(fp, "%s", bufp);
	else if (*(bufp - 1) == '0')	/* 0.0 */
		n += fprintf(fp, ".0");
	else {
		register int last = strlen(bufp) - 1;

		/*
		 * Getting rid of the unnecessary trailing 0's.
		 */
		while (last && *(bufp + last) == '0') {
			*(bufp + last) = '\0';
			last--;
		}
		n += fprintf(fp, ".%s", bufp);
		if (decpt -1)
			n += fprintf(fp, " e%d", decpt - 1);
	}
	/*
	 * Print the tags in the same (50th) column.
	 */
	for (sign = n; sign < 50; sign++)
		fprintf(fp, " ");
	switch (tag) {
	case 0:
		fprintf(fp, "\tVALID\n");
		break;
	case 1:
		fprintf(fp, "\tZERO\n");
		break;
	case 2:
		fprintf(fp, "\tSPECIAL\n");
		break;
	case 3:
		fprintf(fp, "\tEMPTY\n");
		break;
	default:
		error("fpreg_print: impossible tag value!\n");
		break;
	}
}


/*
 * Print the 8 FPU data registers, each of which is 10 bytes.
 */
static void
fpregs_print(const struct _fpreg *fpreg, const int precision,
    const unsigned short top, const unsigned short tag)
{
	register int i;
	static void fpreg_print(const struct _fpreg *, const int, const int,
	    const unsigned short, const unsigned short);

	fprintf(fp, "fpu regs:\n");
	for (i = 0; i < 8; i++)
		fpreg_print(fpreg + i, precision, i, top,
			    ((int) tag >> ((i + top) % 8) * 2) & 3);
}

/*
 * Print the FPU state.
 */
void
prfpu_state(fpregset_t *fpregset, const int full)
{
#define	PRECISION 25

	unsigned short top;		/* top of FPU data register stack */
	int precision = PRECISION;
	register int i;
	struct _fpstate fpstate;
	static int fpu_avail(void);
	static void fpu_cw_print(const unsigned short, const int);
	static void fpu_sw_print(const char *, const unsigned short,
	    unsigned short *const, const int);
	static void fpregs_print(const struct _fpreg *, const int,
	    const unsigned short, const unsigned short);

	fprintf(fp, "\nfpu state:\n");

	switch (fpu_avail()) {
	case FP_NO:
		fprintf(fp, "\tNo floating point support is present.\n");
		return;
	case FP_SW:
		fprintf(fp, "\t80387 software emulator is present.\n"
			    "\tfp_emul[]:\n");
		for (i = 0; i < 62; i += 6) {
			register int j;

			fprintf(fp, "\t  [%3d]:", i * 4);
			for (j = 0; j < 6; j++) {
	fprintf(fp, " %8x", fpregset->fp_reg_set.fp_emul_space.fp_emul[i]);
				if ((i * 4) + j == 240) {
	fprintf(fp, " %-8hx", fpregset->fp_reg_set.fp_emul_space.fp_emul[i]);
					break;
				}
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "\tfp_epad[]: %x\n",
			(short int) fpregset->fp_reg_set.fp_emul_space.fp_epad);
		return;
	case FP_287:
		fprintf(fp, "\t80287 chip is present.");
		break;
	case FP_387:
		fprintf(fp, "\t80387 chip is present.");
		break;
	case FP_486:
		fprintf(fp, "\t80486 chip is present.");
		break;
	default:
		fprintf(fp, "\tUnidentified floating point support.\n");
		return;
	}
	memcpy(&fpstate, &fpregset->fp_reg_set.fpchip_state, sizeof (fpstate));
	if (full) {
		unsigned short top;  /* just to be different from other 'top' */

		(void) printf("  (Re: <ieeefp.h> and <sys/fp.h>)");
		fpu_sw_print("status word at exception",
		    (unsigned short) fpstate.status, &top, full);
		fprintf(fp, "\n");
	}
	fpu_cw_print((unsigned short) fpstate.cw, full);
	fpu_sw_print("sw", (unsigned short) fpstate.sw, &top, full);
	(void) printf("\n\tcssel   %#x  ipoff %#x\n\t"
	    "datasel %#x  dataoff %#x\n\n",
	    fpstate.cssel, fpstate.ipoff, fpstate.datasel, fpstate.dataoff);
	fpregs_print(fpstate._st, precision, top, (unsigned short) fpstate.tag);
}

#endif /* i386 */

/* print user, kernel, or active pcb */
#if i386
void
prpcb(uint addr, int full)
#else /* !i386 */
static
void
prpcb(addr)
	kthread_id_t addr;
#endif /* !i386 */
{
	int i;
	struct pcb *pcbp;
#ifdef notdef
#ifndef i386
	struct machpcb *mpcbp;
#endif /* i386 */
#endif
	struct _klwp lwpb;
	struct _kthread threadb;
#if i386
	void prfpu_state(fpregset_t *, const int);
#endif /* i386 */

	fprintf(fp, "PCB for thread %x\n", addr);

	readmem((unsigned)addr, 1, -1, &threadb, sizeof (threadb), "thread");

	if (getlwpblock(&threadb, &lwpb) == -1)
		return;

	pcbp = &lwpb.lwp_pcb;

#if	defined(i386)
	fprintf(fp, "pc:   %08x\tsp:    %08x\tflags: %08d\n",
		threadb.t_pc, threadb.t_sp, pcbp->pcb_flags);

	prfpu_state(&pcbp->pcb_fpu.fpu_regs, full);

	fprintf(fp, "\ncpu:    %08x\tfpu flags:    %08d\n",
		pcbp->pcb_fpu.fpu_cpu, pcbp->pcb_fpu.fpu_flags);

	fprintf(fp, "\ndebug regs:\n\t");
	for (i = 0; i < NDEBUGREG; i++) {
		fprintf(fp, "%8x ", pcbp->pcb_dregs.debugreg[i]);
		if ((i % 8) == 7)
			fprintf(fp, "\n\t");
	}

	/*
	 * XXX - disassemble pcbp->pcb_instr
	 */
	fprintf(fp, "\ninstruction at stop:    %08x\n",
		pcbp->pcb_instr);
#elif	defined(__ppc)
	fprintf(fp, "pc:  0x%08x\tsp:  0x%08x\tflags:  0x%08x\n",
		threadb.t_pc, threadb.t_sp, pcbp->pcb_flags);

	for (i = 0; i < 31; i += 3) {
		if (i == 30) {
			fprintf(fp, "F%d:\t%-16.9f\t%-16.9f\n", i,
			pcbp->pcb_fpu.fpu_regs[i],
			pcbp->pcb_fpu.fpu_regs[i+1]);
		}
		fprintf(fp, "F%d:\t%-16.9f\t%-16.9f\t%-16.9f\n", i,
		pcbp->pcb_fpu.fpu_regs[i], pcbp->pcb_fpu.fpu_regs[i+1],
		pcbp->pcb_fpu.fpu_regs[i+2]);
	}
	fprintf(fp, "fpscr:\t0x%08x\n", pcbp->pcb_fpu.fpu_fpscr);
	fprintf(fp, "fpvalid:\t0x%08x\n", pcbp->pcb_fpu.fpu_valid);

	fprintf(fp, "\ninstruction at stop:    %08x\n",
		pcbp->pcb_instr);
#else /* !i386 && !__ppc */
#ifdef notdef

	/*
	* We no longer print this anymore. Save in case one day we
	* bring it back
	*/

	mpcbp = lwptompcb(&lwpb);

	fprintf(fp, "pc:   %08x\tsp:    %08x\tuwm:  %08x\n",
		threadb.t_pcb.val[0],
		threadb.t_pcb.val[1],
		mpcbp->mpcb_uwm);
	fprintf(fp, "wbuf: %08x\twbcnt: %08d\tpcb_fpu_q: %08x\n",
		mpcbp->mpcb_wbuf,
		mpcbp->mpcb_wbcnt,
		mpcbp->mpcb_fpu_q);
	fprintf(fp, "spbuf:\n");
	for (i = 0; i < MAXWIN; i++) {
		fprintf(fp, "%8x ", mpcbp->mpcb_spbuf[i]);
		if ((i % 8) == 7)
			fprintf(fp, "\n");
	}
	fprintf(fp, "\nflags: %08d\twocnt: %08d\twucnt: %08d\n",
		pcbp->pcb_flags,
		mpcbp->mpcb_wocnt,
		mpcbp->mpcb_wucnt);
	fprintf(fp, "fsr %x\tqcnt  %d\n",
		mpcbp->mpcb_fpu.fpu_fsr,
		mpcbp->mpcb_fpu.fpu_qcnt);
	fprintf(fp, "fpu regs:\n");
	for (i = 0; i < 32; i++) {
		fprintf(fp, "%8x ", mpcbp->mpcb_fpu.fpu_fr.fpu_regs[i]);
		if ((i % 8) == 7)
			fprintf(fp, "\n");
	}
#endif
#endif /* !i386 */
}

/* get arguments for stack function */
int
getstack(void)
{
	int phys = 0;
	char type = 'k';
	unsigned addr = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "ukpw:")) != EOF) {
		switch (c) {
			case 'w':
				redirect();
				break;
			case 'p':
				phys = 1;
				break;
			case 'u':
				type = 'u';
				break;
			case 'k':
				type = 'k';
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			if ((addr = strcon(args[optind], 'h')) == -1)
				continue;
			if (type == 'u')
				prustk(addr, phys);
			else
				prkstk(addr, phys);
		} while (args[++optind]);
	} else {
		if (type == 'u')
			prustk((unsigned)Curthread, phys);
		else
			prkstk((unsigned)Curthread, phys);
	}
	return (0);
}


/* print kernel stack */
static void
prkstk(unsigned addr, int phys)
{
	long stkfp;
	unsigned stklo, stkhi;
	struct _kthread threadb;

	readmem(addr, !phys, -1, &threadb, sizeof (threadb), "thread");
	stkhi = (int) threadb.t_stk;
	stklo = (int) threadb.t_swap;
	stkfp = threadb.t_sp;
	if ((stkfp > stklo) && (stkfp < stkhi))
		prstack("kernel stack", stkhi, stkfp);
	else
		fprintf(fp, "kernel stack out of range\n");
}

static void
prstack(char *type, unsigned stkhi, unsigned stkfp)
{
	int stksize, i;
	char *stk;
	int *stkp;

	fprintf(fp, "\n%s:\nFP: %8x ", type, stkfp);
	fprintf(fp, "end of stack: %8x\n", stkhi);

	stksize = stkhi - stkfp;
	if ((stk = (char *) malloc(stksize)) == NULL) {
		fprintf(fp, "crash: prstack: Could not allocate memory\n");
		return;
	}
	readmem(stkfp, 1, -1, stk, stksize, type);
	for (i = 0, stkp = (int *) stk; stkfp < stkhi; i++, stkfp += 4,
		stkp++) {
		if ((i % 4) == 0)
			fprintf(fp, "\n%8.8x: ", stkfp);
		fprintf(fp, "  %8.8x  ", *stkp);
	}
	fprintf(fp, "\n");
	free(stk);
}


static void
pruserstack(char *type, unsigned stkhi, unsigned stkfp)
{
	int stksize, i;
	char *stk;
	int *stkp;

	fprintf(fp, "\n%s:\nFP: %8x ", type, stkfp);
	fprintf(fp, "end of stack: %8x\n", stkhi);

	stksize = stkhi - stkfp;
	if ((stk = (char *) malloc(stksize)) == NULL) {
		fprintf(fp, "crash: pruserstack: Could not allocate memory\n");
		return;
	}

	if (kvm_uread(kd, stkfp, stk, stksize) != stksize) {
		fprintf(fp, "crash: pruserstack: Could not read user stack\n");
		perror("kvm_uread: ");
		return;
	}

	for (i = 0, stkp = (int *) stk; stkfp < stkhi; i++, stkfp += 4,
		stkp++) {
		if ((i % 4) == 0)
			fprintf(fp, "\n%8.8x: ", stkfp);
		fprintf(fp, "  %8.8x  ", *stkp);
	}
	fprintf(fp, "\n");
	free(stk);
}


#if i386
char *regnames[] = {
	/* IU general regs */
	"gs", "fs", "es", "ds", "edi", "esi", "ebp", "esp",
	"ebx", "edx", "ecx", "eax",

	/* Miscellaneous */
	"trapno", "err", "eip", "cs", "efl", "uesp", "ss",

	/* FPU regs */
	"fctrl", "fstat", "ftag", "fip", "fcs", "fopoff", "fopsel",
	"st0", "st0a", "st0b", "st1", "st1a", "st1b",
	"st2", "st2a", "st2b", "st3", "st3a", "st3b",
	"st4", "st4a", "st4b", "st5", "st5a", "st5b",
	"st6", "st6a", "st6b", "st7", "st7a", "st7b",
};

/* Print the registers */
static void
prregs(gregset_t *rg)
{
	register int i;

	for (i = 0; i < NGREG; i++) {
		fprintf(fp, "\t%s: %x", regnames[i], *rg[i]);
		if ((i % 4) == 3 || i == NGREG - 1)
			fprintf(fp, "\n");
	}
}
#endif /* i386 */


/* print user stack */
static void
prustk(unsigned addr, int phys)
{
	long stklo, stkhi, tmp;
	long stkfp;
	struct _kthread threadb;
	struct proc procb;
	struct _klwp lwpb;
#if i386
	gregset_t rg;
	static void prregs(gregset_t *);
#else /* !i386 */
	struct regs rg;
#endif /* !i386 */

	fprintf(fp, "prustk: addr %x\n", addr);
	readmem(addr, !phys, -1, &threadb, sizeof (threadb), "thread entry");
	readmem((unsigned)threadb.t_procp, 1, -1, &procb,
		sizeof (procb), "proc");
	ubp = kvm_getu(kd, &procb);
	readmem((unsigned)threadb.t_lwp, 1, -1, &lwpb, sizeof (lwpb),
		"lwp entry");

	if (lwpb.lwp_regs == 0)
		fprintf(fp, "No access to user registers\n");
	else {
		readmem((unsigned)lwpb.lwp_regs, 1, -1, &rg,
					sizeof (rg), "ar0 registers");
#if	defined(i386)
	prregs(&rg);
#elif	defined(__ppc)
		fprintf(fp, "msr %x  pc %x  cr %x  lr %x\n",
			rg.r_msr, rg.r_pc, rg.r_cr, rg.r_lr);
		fprintf(fp, "r0 %x  r1 %x  r2 %x  r3 %x\n",
			rg.r_r0, rg.r_r1, rg.r_r2, rg.r_r3);
		fprintf(fp, "r4 %x  r5 %x  r6 %x  r7 %x\n",
			rg.r_r4, rg.r_r5, rg.r_r6, rg.r_r7);
		fprintf(fp, "r8 %x  r9 %x  r10 %x  r11 %x\n",
			rg.r_r8, rg.r_r9, rg.r_r10, rg.r_r11);
		fprintf(fp, "r12 %x  r13 %x  r14 %x  r15 %x\n",
			rg.r_r12, rg.r_r13, rg.r_r14, rg.r_r15);
		fprintf(fp, "r16 %x  r17 %x  r18 %x  r19 %x\n",
			rg.r_r16, rg.r_r17, rg.r_r18, rg.r_r19);
		fprintf(fp, "r20 %x  r21 %x  r22 %x  r23 %x\n",
			rg.r_r20, rg.r_r21, rg.r_r22, rg.r_r23);
		fprintf(fp, "r24 %x  r25 %x  r26 %x  r27 %x\n",
			rg.r_r24, rg.r_r25, rg.r_r26, rg.r_r27);
		fprintf(fp, "r28 %x  r29 %x  r30 %x  r31 %x\n",
			rg.r_r28, rg.r_r29, rg.r_r30, rg.r_r31);
#else /* !i386 */
		fprintf(fp, "psr %x  pc %x  npc %x\n",
			rg.r_psr, rg.r_pc, rg.r_npc);
		fprintf(fp, "psr %x  pc %x  npc %x\n", rg.r_psr, rg.r_pc,
				rg.r_npc);
		fprintf(fp, "y %x  g1 %x  g2 %x\n", rg.r_y, rg.r_g1, rg.r_g2);
		fprintf(fp, "g3 %x  g4 %x  g5 %x\n", rg.r_g3, rg.r_g4, rg.r_g5);
		fprintf(fp, "g6 %x  g7 %x  o0 %x\n", rg.r_g6, rg.r_g7, rg.r_o0);
		fprintf(fp, "o1 %x  o2 %x  o3 %x\n", rg.r_o1, rg.r_o2, rg.r_o3);
		fprintf(fp, "o4 %x  o5 %x  o6 %x\n", rg.r_o4, rg.r_o5, rg.r_o6);
		fprintf(fp, "o7 %x\n", rg.r_o7);
#endif /* !i386 */
		stkhi = (int) USRSTACK;
		stklo = stkhi - procb.p_stksize;
#if i386
		stkfp = rg[EBP];
#else /* !i386 */
		stkfp = rg.r_sp;
#endif /* !i386 */
		if ((stkfp > stklo) && (stkfp < stkhi))
			pruserstack("user stack", stkhi, stkfp);
		else
			fprintf(fp, "user stack out of range\n");
	}
}

/* get arguments for trace function */
int
gettrace(void)
{
	long addr = -1;
	int c;
	struct syment *sp;

	optind = 1;
	while ((c = getopt(argcnt, args, "prw:")) != EOF) {
		switch (c) {
			case 'w':
				redirect();
				break;
			case 'p':
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			if ((addr = strcon(args[optind], 'h')) == -1)
				continue;
			prktrace(addr);
		} while (args[++optind]);
	} else
		prktrace((unsigned)Curthread);
	return (0);
}

#define	LDONE 4
#define	RDONE 5

/* Trace command left print label */
static char    *llabel[6][2] = {
	{"outs |", "|"},
	{"locals |", "|"},
	{"ins |", "|"},
	{"", ""},
	{"ins |", "|"},
	{"", ""},
};

/* Trace command right print label */
static char    *rlabel[6][2] = {
	{"| ins", "|"},
	{"", ""},
	{"| outs", "|"},
	{"| locals", "|"},
	{"", ""},
	{"| ins", "|"},
};

/* Trace command blank line print label */
static char    *blabel[6][2] = {
	{"|", "|"},
	{"|", "|"},
	{"", "|"},
	{"", "|"},
	{"", ""},
	{"", ""},
};

/* print kernel trace */
static void
prktrace(unsigned addr)
{
	long stkfp, cur_pc, next_sp;
	struct _kthread threadb;
	struct _klwp lwpb;
#if defined(i386)
	gregset_t rg;
	static void prregs(gregset_t *);
#elif defined(__ppc)
	struct regs rg;
#elif defined(sparc)
	struct rwindow rwins;
	int label_index = 0;
#endif

	fprintf(fp, "STACK TRACE FOR THREAD %x:\n", addr);

	readmem(addr, 1, -1, &threadb, sizeof (threadb), "thread");
#if defined(i386)
	stkfp = rg[EBP];
	cur_pc = rg[EIP];
	prregs(&rg);
#elif defined(__ppc)
	stkfp = rg.r_sp;
	cur_pc = rg.r_pc;

	/* Loop until we have a NULL stack pointer */
	while ((stkfp != 0) && (stkfp != 0xffffffff)) {
		fprintf(fp, "0x%08x\n", cur_pc);
		cur_pc  = ((struct minframe *)stkfp)->fr_savpc;
		next_sp = (long)((struct minframe *)stkfp)->fr_savfp;
		stkfp = next_sp;
	}
#elif defined(sparc)
	stkfp = threadb.t_sp;
	cur_pc = threadb.t_pc;

	label_index = 1;
	/*
	 * XXX - Any work needed here for i386?
	 */

	/* Loop until we have a NULL stack pointer */
	while (stkfp != 0) {
		readmem((unsigned)stkfp, 1, -1, &rwins, sizeof (struct rwindow),
							"register set");
		/* Print the set of local registers from the stack */
		fprintf(fp, "%12s0 0x%08x 0x%08x 0x%08x 0x%08x %-10s\n",
			llabel[label_index][0],
			rwins.rw_local[0],
			rwins.rw_local[1],
			rwins.rw_local[2],
			rwins.rw_local[3],
			rlabel[label_index][0]);
		fprintf(fp, "%12s4 0x%08x 0x%08x 0x%08x 0x%08x %-10s\n",
			llabel[label_index][1],
			rwins.rw_local[4],
			rwins.rw_local[5],
			rwins.rw_local[6],
			rwins.rw_local[7],
			rlabel[label_index][0]);

		label_index = (label_index + 1) % 4;


		/* Set the address to next */
		cur_pc = (long) rwins.rw_in[7];
		/* Set the next stack pointer */
		next_sp = (long) rwins.rw_in[6];

		/* Determine if this is the last print */
		if (next_sp == 0) {
			if (label_index == 0) {
				label_index = RDONE;
				fprintf(fp, "\t\t\t\t\t\t\t  |\n");
			} else {
				label_index = LDONE;
				fprintf(fp, "\t   |\t\t\t\t\t\t\n", "|");
			}
		} else {
			if (label_index == 0) {
				fprintf(fp, "0x%08x\t\t\t\t\t\t  |\n",
					cur_pc);
			} else {
				fprintf(fp, "\t   |\t\t\t\t\t\t  0x%08x\n",
					cur_pc);
			}
		}

		/* Print the set of local registers from the stack */
		fprintf(fp, "%12s0 0x%08x 0x%08x 0x%08x 0x%08x %-10s\n",
			llabel[label_index][0],
			rwins.rw_in[0],
			rwins.rw_in[1],
			rwins.rw_in[2],
			rwins.rw_in[3],
			rlabel[label_index][0]);
		fprintf(fp, "%12s4 0x%08x 0x%08x 0x%08x 0x%08x %-10s\n",
			llabel[label_index][1],
			rwins.rw_in[4],
			rwins.rw_in[5],
			rwins.rw_in[6],
			rwins.rw_in[7],
			rlabel[label_index][1]);

		if (next_sp != 0) {
			fprintf(fp, "%12s\t\t\t\t\t\t  %-10s\n",
				blabel[label_index][0],
				blabel[label_index][1]);

			label_index = (label_index + 1) % 4;
		}
		/* Set the stack pointer to next */
		stkfp = next_sp;
	}
#endif
}

/* get arguments for kfp function */
int
getkfp(void)
{
	int c;
	long addr;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) != EOF) {
		switch (c) {
			case 'w':
				redirect();
				break;
			default:
				longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		if ((addr = strcon(args[optind], 'h')) == -1)
			error("\n");
		prkfp(addr);
	} else
		prkfp((unsigned)Curthread);
	return (0);
}

/* print kfp */
static void
prkfp(unsigned addr)
{
	struct _kthread   tb;

	readmem(addr, 1, -1, &tb, sizeof (tb), "thread structure");

	fprintf(fp, "kfp: %8.8x\n", tb.t_sp);
}
