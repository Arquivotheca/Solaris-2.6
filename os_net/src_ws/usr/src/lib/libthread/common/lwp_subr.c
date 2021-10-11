/*	Copyright (c) 1993 SMI	*/
/*	 All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SMI	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)lwp_subr.c	1.22	96/05/20	SMI"

#include "libthread.h"

/*
 * the thread library uses this routine when it needs another LWP.
 */
int
_new_lwp(struct thread *t, void (*func)(), int door)
{
	uthread_t *aging = NULL;
	long flags;
	int ret;

	if (t == NULL) {
		/*
		 * increase the pool of LWPs on which threads execute.
		 */
		if ((aging = _idle_thread_create()) == NULL)
			return (EAGAIN);
		if (door)
			aging->t_flag |= T_DOORSERVER;
		else {
			_sched_lock();
			_nlwps++;
			_sched_unlock();
		}

		aging->t_startpc = (long)func;
		ret = _lwp_exec(aging, (int)_thr_exit, (caddr_t)aging->t_sp,
		    _lwp_start, (LWP_SUSPENDED | LWP_DETACHED),
		    &(aging->t_lwpid));
		if (ret) {
			/* if failed */
			_thread_free(aging);
			if (!door) {
				_sched_lock();
				_nlwps--;
				_sched_unlock();
			}
			return (ret);
		}
		ASSERT(aging->t_lwpid != 0);
		_lwp_continue(aging->t_lwpid);
		return (0);
	}
	flags = 0;
	ASSERT(t->t_usropts & THR_BOUND);
	flags |= LWP_DETACHED;
	if (t->t_usropts & THR_SUSPENDED)
		flags |= LWP_SUSPENDED;
	else
		t->t_state = TS_ONPROC;

	/* _thread_start will call _sc_setup */
	ret = _lwp_exec(t, (int)_thr_exit, (caddr_t)t->t_sp, func,
					flags, &(t->t_lwpid));
	ASSERT(ret || t->t_lwpid != 0);
	return (ret);
}

void
_lwp_start(void)
{
	uthread_t *t = curthread;

	_sc_setup();
	(*(void (*)())(t->t_startpc))();
	_thr_exit(NULL);
}
