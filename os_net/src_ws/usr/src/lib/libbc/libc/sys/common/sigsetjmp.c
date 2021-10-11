/*	Copyright (c) 1988 AT&T */
/*	All Rights Reserved   */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sigsetjmp.c	1.1	90/11/01 SMI"	/* SVr4.0 1.2	*/


#include <sys/setjmp.h>
#include "ucontext.h"

int _getsp();

int sigsetjmp(env, savemask)
	sigjmp_buf env;
	int savemask;
{
	register o_setjmp_struct_t *bp = (o_setjmp_struct_t *)env;
	register int sp = _getsp();
	ucontext_t uc;

	/*
	 * Get the current machine context.
	 */
	uc.uc_flags = UC_STACK | UC_SIGMASK;
	__getcontext(&uc);

	/*
	 * Note that the pc and former sp (fp) from the stack are valid
	 * because the call to __getcontext must flush the user windows
	 * to the stack.
	 */
	bp->sjs_flags = 0;
	bp->sjs_sp    = *((int *)sp+14);
	bp->sjs_pc    = *((int *)sp+15) + 0x8;
	bp->sjs_stack = uc.uc_stack;

	if (savemask) {
		/* save the mask */
		bp->sjs_flags |= JB_SAVEMASK;
		memcpy(bp->sjs_sigmask, &(uc.uc_sigmask), 3*sizeof(int));
	}
	else {
		memset(bp->sjs_sigmask, 0, 3*sizeof(int));
	}

	return 0;
}


void siglongjmp(env, val)
	sigjmp_buf env;
	int val;
{
	o_setjmp_struct_t *bp = (o_setjmp_struct_t *)env;
	setjmp_struct_t sjmp, *sp;

	sp = &sjmp;
	sp->sjs_flags = bp->sjs_flags;
	sp->sjs_sp = bp->sjs_sp;
	sp->sjs_pc = bp->sjs_pc;
	sp->sjs_sigmask[0] = bp->sjs_sigmask[0];
	sp->sjs_sigmask[1] = bp->sjs_sigmask[1];
	sp->sjs_sigmask[2] = bp->sjs_sigmask[2];
	sp->sjs_stack = bp->sjs_stack;

	_siglongjmp(sjmp, val);
}

int 
_setjmp(env)
	jmp_buf env;
{
	register o_setjmp_struct_t *bp = (o_setjmp_struct_t *)env;
	register int sp = _getsp();
	ucontext_t uc;

	/*
	 * Get the current machine context.
	 */
	uc.uc_flags = UC_STACK | UC_SIGMASK;
	__getcontext(&uc);

	/*
	 * Note that the pc and former sp (fp) from the stack are valid
	 * because the call to __getcontext must flush the user windows
	 * to the stack.
	 */
	bp->sjs_flags = 0;
	bp->sjs_sp    = *((int *)sp+14);
	bp->sjs_pc    = *((int *)sp+15) + 0x8;
	bp->sjs_stack = uc.uc_stack;

	return 0;
}


void
_longjmp(env, val)
	jmp_buf env;
	int val;
{
	o_setjmp_struct_t *bp = (o_setjmp_struct_t *)env;
	setjmp_struct_t sjmp, *sp;

	sp = &sjmp;
	sp->sjs_flags = bp->sjs_flags;
	sp->sjs_sp = bp->sjs_sp;
	sp->sjs_pc = bp->sjs_pc;
	sp->sjs_stack = bp->sjs_stack;

	_siglongjmp(sjmp, val);
}

