#pragma ident	"@(#)ssignal.c	1.3	96/05/03 SMI"  /* from S5R2 1.2 */

/*LINTLIBRARY*/
/*
 *	ssignal, gsignal: software signals
 */
#include <signal.h>

/* Highest allowable user signal number */
#define	MAXSIG NSIG

/* Lowest allowable signal number (lowest user number is always 1) */
#define	MINSIG (-4)

/* Table of signal values */
typedef int (*sigfunc)();
sigfunc *ssigp;


sigfunc *
_ssig()
{
	if (ssigp == 0)
		ssigp = (sigfunc *)calloc(MAXSIG-MINSIG+1, sizeof (sigfunc));
	return (ssigp);
}

int
(*ssignal(sig, fn))()
register int sig, (*fn)();
{
	register int (*savefn)();
	register sigfunc *sp = _ssig();

	if (sp == 0)
		return ((int (*)())SIG_DFL);
	if (sig >= MINSIG && sig <= MAXSIG) {
		savefn = sp[sig-MINSIG];
		sp[sig-MINSIG] = fn;
	} else
		savefn = (int (*)())SIG_DFL;

	return (savefn);
}

int
gsignal(sig)
register int sig;
{
	register int (*sigfn)();
	register sigfunc *sp = _ssig();

	if (sp == 0)
		return (0);
	if (sig < MINSIG || sig > MAXSIG ||
				(sigfn = sp[sig-MINSIG]) == (int (*)())SIG_DFL)
		return (0);
	else if (sigfn == (int (*)())SIG_IGN)
		return (1);
	else {
		sp[sig-MINSIG] = (int (*)())SIG_DFL;
		return ((*sigfn)(sig));
	}
}
