/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#pragma ident "@(#)signal.c 1.3	94/09/09 SMI"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 * 4.3BSD signal compatibility functions
 *
 * the implementation interprets signal masks equal to -1 as "all of the
 * signals in the signal set", thereby allowing signals with numbers
 * above 32 to be blocked when referenced in code such as:
 *
 *	for (i = 0; i < NSIG; i++)
 *		mask |= sigmask(i)
 */

#include <sys/types.h>
#include <sys/siginfo.h>
#include <sys/ucontext.h>
#include <signal.h>
#include "signal.h"
#include <errno.h>
#include <stdio.h>
#include <sys/reg.h>

#define set2mask(setp) ((setp)->__sigbits[0])
#define mask2set(mask, setp) \
	((mask) == -1 ? sigfillset(setp) : sigemptyset(setp), (((setp)->__sigbits[0]) = (mask)))

extern void (*_siguhandler[])();

/*
 * sigstack is emulated with sigaltstack by guessing an appropriate
 * value for the stack size - on machines that have stacks that grow 
 * upwards, the ss_sp arguments for both functions mean the same thing, 
 * (the initial stack pointer sigstack() is also the stack base 
 * sigaltstack()), so a "very large" value should be chosen for the 
 * stack size - on machines that have stacks that grow downwards, the
 * ss_sp arguments mean opposite things, so 0 should be used (hopefully
 * these machines don't have hardware stack bounds registers that pay
 * attention to sigaltstack()'s size argument.
 */

#ifdef sun
#define SIGSTACKSIZE	0
#endif


/*
 * sigvechandler is the real signal handler installed for all
 * signals handled in the 4.3BSD compatibility interface - it translates
 * SVR4 signal hander arguments into 4.3BSD signal handler arguments
 * and then calls the real handler
 */

static void
sigvechandler(sig, sip, ucp) 
	int sig;
	siginfo_t *sip;
	ucontext_t *ucp;
{
	struct sigcontext sc;
	int code;
	char *addr;
	register int i, j;
	int gwinswitch = 0;
	
	sc.sc_onstack = ((ucp->uc_stack.ss_flags & SS_ONSTACK) != 0);
	sc.sc_mask = set2mask(&ucp->uc_sigmask);

	/* 
	 * Machine dependent code begins
	 */
	sc.sc_sp = ucp->uc_mcontext.gregs[R_R1];
	sc.sc_pc = ucp->uc_mcontext.gregs[R_PC];
	sc.sc_msr = ucp->uc_mcontext.gregs[R_MSR];
	sc.sc_R3 = ucp->uc_mcontext.gregs[R_R3];
	/*
	 * Machine dependent code ends
	 */

	if (sip != NULL)
		if ((code = sip->si_code) == BUS_OBJERR)
			code = SEGV_MAKE_ERR(sip->si_errno);

	if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV || sig == SIGBUS)
		if (sip != NULL)
			addr = (char *)sip->si_addr;
	else
		addr = SIG_NOADDR;
	
	(*_siguhandler[sig])(sig, code, &sc, addr);

	if (sc.sc_onstack)
		ucp->uc_stack.ss_flags |= SS_ONSTACK;
	else
		ucp->uc_stack.ss_flags &= ~SS_ONSTACK;
	mask2set(sc.sc_mask, &ucp->uc_sigmask);

	/* 
	 * Machine dependent code begins
	 */
	ucp->uc_mcontext.gregs[R_R1] = sc.sc_sp;
	ucp->uc_mcontext.gregs[R_PC] = sc.sc_pc;
	ucp->uc_mcontext.gregs[R_MSR] = sc.sc_msr;
	ucp->uc_mcontext.gregs[R_R3] = sc.sc_R3;
	/*
	 * Machine dependent code ends
	 */

	setcontext (ucp);
}

sigsetmask(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_SETMASK, &nset, &oset);
	return set2mask(&oset);
}

sigblock(mask)
	int mask;
{
	sigset_t oset;
	sigset_t nset;

	(void) sigprocmask(0, (sigset_t *)0, &nset);
	mask2set(mask, &nset);
	(void) sigprocmask(SIG_BLOCK, &nset, &oset);
	return set2mask(&oset);
}

sigpause(mask)
	int mask;
{
	sigset_t set;

	(void) sigprocmask(0, (sigset_t *)0, &set);
	mask2set(mask, &set);
	return (sigsuspend(&set));
}

sigvec(sig, nvec, ovec)
        int sig;
        struct sigvec *nvec;
	struct sigvec *ovec;
{
        struct sigaction nact;
        struct sigaction oact;
        struct sigaction *nactp;
        void (*ohandler)(), (*nhandler)();

        if (sig <= 0 || sig >= NSIG) {
                errno = EINVAL;
                return -1;
        }

        ohandler = _siguhandler[sig];

        if (nvec) {
		__sigaction(sig, (struct sigaction *)0, &nact);
                nhandler = nvec->sv_handler; 
                _siguhandler[sig] = nhandler;
                if (nhandler != SIG_DFL && nhandler != SIG_IGN)
                        nact.sa_handler = (void (*)())sigvechandler;
		else
			nact.sa_handler = nhandler;
		mask2set(nvec->sv_mask, &nact.sa_mask);
		/*
		if ( sig == SIGTSTP || sig == SIGSTOP )
			nact.sa_handler = SIG_DFL; 	*/
		nact.sa_flags = SA_SIGINFO;
		if (!(nvec->sv_flags & SV_INTERRUPT))
			nact.sa_flags |= SA_RESTART;
		if (nvec->sv_flags & SV_RESETHAND)
			nact.sa_flags |= SA_RESETHAND;
		if (nvec->sv_flags & SV_ONSTACK)
			nact.sa_flags |= SA_ONSTACK;
		nactp = &nact;
        } else
		nactp = (struct sigaction *)0;

        if (__sigaction(sig, nactp, &oact) < 0) {
                _siguhandler[sig] = ohandler;
                return -1;
        }

        if (ovec) {
		if (oact.sa_handler == SIG_DFL || oact.sa_handler == SIG_IGN)
			ovec->sv_handler = oact.sa_handler;
		else
			ovec->sv_handler = ohandler;
		ovec->sv_mask = set2mask(&oact.sa_mask);
		ovec->sv_flags = 0;
		if (oact.sa_flags & SA_ONSTACK)
			ovec->sv_flags |= SV_ONSTACK;
		if (oact.sa_flags & SA_RESETHAND)
			ovec->sv_flags |= SV_RESETHAND;
		if (!(oact.sa_flags & SA_RESTART))
			ovec->sv_flags |= SV_INTERRUPT;
	}
			
        return 0;
}


void (*
signal(s, a))()
        int s;
        void (*a)();
{
        struct sigvec osv;
	struct sigvec nsv;
        static int mask[NSIG];
        static int flags[NSIG];

	nsv.sv_handler = a;
	nsv.sv_mask = mask[s];
	nsv.sv_flags = flags[s];
        if (sigvec(s, &nsv, &osv) < 0)
                return (SIG_ERR);
        if (nsv.sv_mask != osv.sv_mask || nsv.sv_flags != osv.sv_flags) {
                mask[s] = nsv.sv_mask = osv.sv_mask;
                flags[s] = nsv.sv_flags = osv.sv_flags & ~SV_RESETHAND;
                if (sigvec(s, &nsv, (struct sigvec *)0) < 0)
                        return (SIG_ERR);
        }
        return (osv.sv_handler);
}
