/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"@(#)lwp.c	1.90	96/09/23 SMI"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/thread.h>
#include <sys/tblock.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/vmparam.h>
#include <sys/stack.h>
#include <sys/procfs.h>
#include <sys/prsystm.h>
#include <sys/cpuvar.h>
#include <sys/kmem.h>
#include <sys/vtrace.h>
#include <sys/door.h>
#include <vm/seg_kp.h>
#include <sys/debug.h>
#include <sys/tnf.h>
#include <sys/schedctl.h>
#include <sys/poll.h>
#ifdef _VPIX
#include <sys/v86.h>
#endif

#define	NOCLASS	(-1)	/* pseudo-flag to lwp_create() */

void *segkp_lwp;		/* cookie for pool of segkp resources */
extern struct kmem_cache *lwp_cache;	/* cache of free lwps */

extern kmutex_t	reaplock;

/*
 * Create a thread that appears to be stopped at sys_rtt.
 */
klwp_id_t
lwp_create(void (*proc)(void), caddr_t arg, int len,
	proc_t *p, int state, int pri, k_sigset_t smask, int cid)
{
	klwp_id_t lwp = NULL;
	kthread_id_t t;
	int	stksize;
	caddr_t	lwpdata = NULL;
	extern struct seg *segkp;
	processorid_t	binding;
	int	err;
	extern id_t	syscid;

	/*
	 * Try to reclaim a <lwp,stack> from 'deathrow'
	 */
	if (lwp_reapcnt > 0) {
		mutex_enter(&reaplock);
		if ((t = lwp_deathrow) != NULL) {
			ASSERT(t->t_swap);
			lwp_deathrow = t->t_forw;
			lwp_reapcnt--;
			lwpdata = t->t_swap;
			lwp = t->t_lwp;
		}
		mutex_exit(&reaplock);
		if (t) {
			t->t_swap = NULL;
			t->t_lwp = NULL;
			t->t_forw = NULL;
			thread_free(t);
		}
	}
	if (lwpdata == NULL &&
	    (lwpdata = (caddr_t)segkp_cache_get(segkp_lwp)) == NULL)
		return (NULL);

	stksize = lwp_default_stksize;
#ifdef _VPIX
	stksize -= V86FRAME;
#endif

	/*
	 * Create a thread, initializing the stack pointer
	 */
	t = thread_create(lwpdata, stksize, NULL, NULL, 0, p, TS_STOPPED, pri);

	if (t == NULL) {
		segkp_release(segkp, lwpdata);
		if (lwp)
			kmem_cache_free(lwp_cache, lwp);
		return (NULL);
	}
	t->t_swap = lwpdata;	/* Start of page-able data */
	if (lwp == NULL)
		lwp = kmem_cache_alloc(lwp_cache, KM_SLEEP);
	bzero((caddr_t)lwp, sizeof (*lwp));
	t->t_lwp = lwp;

	if (cid != NOCLASS) {
		proc_t *pp = curproc;
		void	*bufp = NULL;
		size_t	bufsz;
		/*
		 * If curthread is sys class, we're called from main via
		 * newproc and shouldn't propagate class parameters.
		 * Otherwise, we're called from syslwp_create and should
		 * copy curthread parameters.
		 */
		bufsz = sclass[cid].cl_size;
		if (bufsz != 0)
			bufp = kmem_alloc(bufsz, KM_SLEEP);
		mutex_enter(&pp->p_lock);
		if (curthread->t_cid == syscid || curthread->t_cid != cid) {
			err = CL_ENTERCLASS(t, cid, NULL, NULL, bufp);
			t->t_pri = pri;	/* CL_ENTERCLASS may have changed it */
		} else {
			t->t_clfuncs = &(sclass[cid].cl_funcs->thread);
			err = CL_FORK(curthread, t, bufp);
			t->t_cid = cid;
		}
		mutex_exit(&pp->p_lock);
		if (err) {
			if (bufp)
				kmem_free(bufp, bufsz);
			/*
			 * The thread was created TS_STOPPED.
			 * We change it to TS_FREE to avoid an
			 * ASSERT() panic in thread_free().
			 */
			t->t_state = TS_FREE;
			thread_free(t);
			return (NULL);
		}
	}

	t->t_hold = smask;
	lwp->lwp_thread = t;
	lwp->lwp_procp = p;
	lwp->lwp_sigaltstack.ss_flags = SS_DISABLE;

	t->t_stk = lwp_stk_init(lwp, t->t_stk);
	(void) thread_load(t, proc, arg, len);

	mutex_enter(&p->p_lock);
	/*
	 * Block the process against /proc while we manipulate p->p_tlist.
	 */
	prbarrier(p);

	/*
	 * Allocate the SIGPROF buffer if ITIMER_REALPROF is in effect.
	 */
	if (timerisset(&p->p_rprof_timer.it_value))
		t->t_rprof = (struct rprof *)
		    kmem_zalloc(sizeof (struct rprof), KM_NOSLEEP);

	if (p->p_tlist == NULL) {
		t->t_dslot = 0;
		t->t_back = t;
		t->t_forw = t;
		p->p_tlist = t;
	} else {
		kthread_t *tx = p->p_tlist;

		/*
		 * Insert the new thread in the first available
		 * directory slot position (for the benefit of /proc).
		 * First find the thread that follows the empty slot.
		 */
		if (tx->t_dslot != 0) {
			/* slot 0 is available */
			t->t_dslot = 0;
		} else if (tx == tx->t_forw) {
			/* only one lwp; slot 1 is available */
			t->t_dslot = 1;
		} else {
			while ((tx = tx->t_forw) != p->p_tlist) {
				if (tx->t_back->t_dslot + 1 != tx->t_dslot)
					break;
			}
			t->t_dslot = tx->t_back->t_dslot + 1;
		}

		/*
		 * Insert the new thread before its following thread.
		 */
		t->t_forw = tx;
		t->t_back = tx->t_back;
		tx->t_back->t_forw = t;
		tx->t_back = t;

		/*
		 * If the new thread occupies directory slot zero,
		 * reset the process's p->p_tlist pointer.
		 */
		if (t->t_dslot == 0)
			p->p_tlist = t;
	}
	/*
	 * lwp/thread id 0 is never valid; reserved for special checks.
	 *
	 * XXX: Need code here to check for wraparound
	 * and to avoid duplicate thread id's.
	 */
	t->t_tid = ++p->p_lwptotal;
	p->p_lwpcnt++;

#ifdef TRACE
	/* relabel thread, now that it has a meaningful tid */
	trace_kthread_label(t, -1);
#endif	/* TRACE */

	/*
	 * Turn microstate accounting on for thread if on for process.
	 */
	if (p->p_flag & SMSACCT)
		t->t_proc_flag |= TP_MSACCT;

	/*
	 * If the process has watchpoints, mark the new thread as such.
	 */
	if (p->p_warea != NULL)
		t->t_proc_flag |= TP_WATCHPT;

	/* lwp was created in the stopped state */
	init_mstate(t, LMS_STOPPED);
	t->t_schedflag |= (TS_ALLSTART & ~TS_CSTART);
	t->t_whystop = PR_SUSPENDED;
	t->t_whatstop = SUSPEND_NORMAL;

	/*
	 * Set system call processing flags in case tracing or profiling
	 * is set.  The first system call will evaluate these and turn
	 * them off if they aren't needed.
	 */
	t->t_pre_sys = 1;
	t->t_post_sys = 1;

	binding = curthread->t_bind_cpu;	/* binding is inherited */
	t->t_bind_cpu = binding;
	if (binding != PBIND_NONE && t->t_affinitycnt == 0)
		t->t_bound_cpu = cpu[binding];

	t->t_bind_pset = curthread->t_bind_pset;
	t->t_cpupart = curthread->t_cpupart;

	if (state == TS_RUN)
		lwp_continue(t);

	mutex_exit(&p->p_lock);

	return (lwp);
}

/*
 * xxx check about cleanup
 * for signals, timers, io, wait conditions
 */

void
lwp_exit()
{
	proc_t	*p = curproc;
	klwp_id_t lwp = ttolwp(curthread);
	kthread_t *t = curthread;
	int tmp_id;
	int waiting = 0;

	ASSERT(MUTEX_HELD(&p->p_lock));

	/*
	 * Call exit() if this is the last LWP in the process.
	 */
	if (p->p_lwpcnt == 1) {
		if (p->p_flag & EXITLWPS) {
			mutex_exit(&p->p_lock);
			exit(CLD_KILLED, SIGKILL);
		} else {
			mutex_exit(&p->p_lock);
			exit(CLD_EXITED, 0);
		}
		/* Never Returns */
	}
	/*
	 * If this is the aslwp dying...
	 * revert to "normal" signal model.
	 * Also transfer pending signals. Although the aslwp should never
	 * really die for a normal process which uses it, this is necessary
	 * for exece() to work correctly - it kills all lwps including the
	 * aslwp, in its call to exitlwps() - at this point, pending signals
	 * need to be transferred to maintain exece()'s semantics of preserving
	 * pending signals across the exec.
	 */
	if (t == p->p_aslwptp) {
		ASSERT(p->p_flag & ASLWP);
		p->p_flag &= ~ASLWP;
		p->p_aslwptp = NULL;
		p->p_sig = t->t_sig;
		p->p_sigqueue = t->t_sigqueue;
		t->t_sigqueue = NULL;
		sigorset(&p->p_sig, &p->p_notifsigs);
	}
	mutex_exit(&p->p_lock);
	tsd_exit();			/* free thread specific data */

	pollcleanup(t);

	/* untimeout any LWP-bound realtime timers */
	if (p->p_itimer != NULL)
		timer_lwpexit();

	if ((tmp_id = t->t_alarmid) > 0) {
		t->t_alarmid = 0;
		(void) untimeout(tmp_id);
	}
	if (t->t_door)
		door_slam();

	if (t->t_schedctl)
		schedctl_cleanup();

	mutex_enter(&p->p_lock);

	while ((tmp_id = t->t_itimerid) > 0) {
		t->t_itimerid = 0;
		mutex_exit(&p->p_lock);
		(void) untimeout(tmp_id);
		mutex_enter(&p->p_lock);
	}

	/*
	 * When this process is dumping core, its LWPs are held here
	 * until the core dump is finished. Then exitlwps() is called
	 * again to release these LWPs so that they can finish exiting.
	 */
	if (p->p_flag & COREDUMP)
		stop(PR_SUSPENDED, SUSPEND_NORMAL);

	/*
	 * Block the process against /proc now that we have really
	 * acquired p->p_lock (to manipulate p_tlist at least).
	 */
	prbarrier(p);

	if (p->p_flag & EXITLWPS)
		t->t_proc_flag &= ~TP_TWAIT;
	t->t_proc_flag |= TP_LWPEXIT;
	term_mstate(t);
#ifndef NPROBE
	/* Kernel probe */
	if (t->t_tnf_tpdp)
		tnf_thread_exit();
#endif /* NPROBE */
	prlwpexit(t);		/* notify /proc */

	p->p_lwpcnt--;
	t->t_forw->t_back = t->t_back;
	t->t_back->t_forw = t->t_forw;
	if (t == p->p_tlist)
		p->p_tlist = t->t_forw;

	/*
	 * Clean up the signal state.
	 */
	if (t->t_sigqueue != NULL)
		sigdelq(p, t, 0);
	if (lwp->lwp_curinfo) {
		siginfofree(lwp->lwp_curinfo);
		lwp->lwp_curinfo = NULL;
	}

	/*
	 * When this is the last running LWP in this process and
	 * some LWP is waiting for this condition to become true,
	 * or this thread was being suspended, then the waiting
	 * LWP is awakened.
	 */
	if (--p->p_lwprcnt == 0 || (t->t_proc_flag & TP_HOLDLWP))
		cv_broadcast(&p->p_holdlwps);

	/*
	 * If LWP was not created with the LWP_DETACHED flag set,
	 * then the LWP is a zombie LWP and is put onto a per process
	 * zombie list.  Avoid preemption after resetting t->t_procp.
	 */
	t->t_preempt++;
	if (t->t_proc_flag & TP_TWAIT) {
		p->p_zombcnt++;
		if (p->p_zomblist == NULL) {
			p->p_zomblist = t;
			t->t_forw = t;
			t->t_back = t;
		} else {
			p->p_zomblist->t_back->t_forw = t;
			t->t_back = p->p_zomblist->t_back;
			t->t_forw = p->p_zomblist;
			p->p_zomblist->t_back = t;
		}
		waiting = 1;	/* signal any waiting lwps later */
	} else {
		/*
		 * Detached LWPs are associated with process zero
		 * and are put onto death-row by resume().
		 */
		t->t_procp = &p0;
	}

	/*
	 * Need to drop p_lock so we can reacquire pidlock.
	 */
	mutex_exit(&p->p_lock);
	mutex_enter(&pidlock);
	/*
	 * Flag thread as exiting for lwp_wait().  We need to do this
	 * here because we dropped p_lock above.  If T_LWPZOMB was set
	 * then, lwp_wait() would think the thread had exited and get
	 * confused.  It's OK to drop p_lock below because we've turned
	 * off preemption and we're not grabbing any more locks.
	 * (Look at the lwp_wait() code if this is confusing.)
	 */
	if (waiting) {
		mutex_enter(&p->p_lock);
		t->t_flag |= T_LWPZOMB;
		if (p->p_lwprcnt > 0)
			cv_broadcast(&p->p_lwpexit);
		mutex_exit(&p->p_lock);
	}

	ASSERT(t != t->t_next);		/* t0 never exits */
	t->t_next->t_prev = t->t_prev;
	t->t_prev->t_next = t->t_next;
	mutex_exit(&pidlock);

	t->t_state = TS_ZOMB;
	swtch_from_zombie();
	/* never returns */
}

int
lwp_suspend(kthread_id_t t)
{
	int tid;
	proc_t *p = ttoproc(t);

	ASSERT(MUTEX_HELD(&p->p_lock));

	/*
	 * Set the thread's TP_HOLDLWP flag so it will stop in holdlwp().
	 * If an lwp is stopping itself, there is no need to wait.
	 */
	t->t_proc_flag |= TP_HOLDLWP;
	if (t == curthread) {
		aston(t);
	} else {
		/*
		 * Make sure the lwp stops promptly.
		 */
		thread_lock(t);
		aston(t);
		/*
		 * XXX Should use virtual stop like /proc does instead of
		 * XXX waking the thread to get it to stop.
		 */
		if (t->t_state == TS_SLEEP && (t->t_flag & T_WAKEABLE))
			setrun_locked(t);
		else if (t->t_state == TS_ONPROC && t->t_cpu != CPU)
			poke_cpu(t->t_cpu->cpu_id);
		tid = t->t_tid;	 /* remember thread ID */
		/*
		 * Wait for lwp to stop
		 */
		while (!SUSPENDED(t)) {
			/*
			 * Drop the thread lock before waiting and reacquire it
			 * afterwards, so the thread can change its t_state
			 * field.
			 */
			thread_unlock(t);

			/*
			 * Check if aborted by exitlwps().
			 */
			if (p->p_flag & EXITLWPS)
				lwp_exit();

			/*
			 * Cooperate with jobcontrol signals and /proc stopping
			 * by calling cv_wait_sig() to wait for the target
			 * lwp to stop.  Just using cv_wait() can lead to
			 * deadlock because, if some other lwp has stopped
			 * by either of these mechanisms, then p_lwprcnt will
			 * never become zero if we do a cv_wait().
			 */
			if (!cv_wait_sig(&p->p_holdlwps, &p->p_lock))
				return (EINTR);

			/*
			 * Check to see if thread died while we were
			 * waiting for it to suspend.
			 */
			if (idtot(p->p_tlist, tid) == (kthread_t *)NULL)
				return (ESRCH);

			thread_lock(t);
			/*
			 * If TP_HOLDLWP flag goes away, lwp_continue() must
			 * have been called while we were waiting, so cancel
			 * the suspend.
			 */
			if ((t->t_proc_flag & TP_HOLDLWP) == 0) {
				thread_unlock(t);
				return (0);
			}
		}
		thread_unlock(t);
	}
	return (0);
}

/*
 * continue a lwp that's been stopped by lwp_suspend().
 */
void
lwp_continue(kthread_id_t t)
{
	proc_t *p = ttoproc(t);
	int was_suspended = t->t_proc_flag & TP_HOLDLWP;

	ASSERT(MUTEX_HELD(&p->p_lock));

	t->t_proc_flag &= ~TP_HOLDLWP;
	thread_lock(t);
	if (SUSPENDED(t) && !(p->p_flag & (HOLDFORK|HOLDFORK1|HOLDWATCH))) {
		p->p_lwprcnt++;
		t->t_schedflag |= TS_CSTART;
		setrun_locked(t);
	}
	thread_unlock(t);
	/*
	 * Wakeup anyone waiting for this thread to be suspended
	 */
	if (was_suspended)
		cv_broadcast(&p->p_holdlwps);
}

/*
 * ********************************
 *  Miscellaneous lwp routines	  *
 * ********************************
 */
/*
 * When a process is undergoing a fork, its p_flag is set to HOLDFORK.
 * This will cause the process's lwps to stop at a hold point.  A hold
 * point is where a kernel thread has a flat stack.  This is at the
 * return from a system call and at the return from a user level trap.
 *
 * When a process is undergoing a fork1 or vfork, its p_flag is set to
 * HOLDFORK1.  This will cause the process's lwps to stop at a modified
 * hold point.  The lwps in the process are not being cloned, so they
 * are held at the usual hold points and also within issig_forreal().
 * This has the side-effect that their system calls do not return
 * showing EINTR.
 *
 * An lwp can also be held.  This is identified by the TP_HOLDLWP flag on
 * the thread.  The TP_HOLDLWP flag is set in lwp_suspend(), where the active
 * lwp is waiting for the target lwp to be stopped.
 */
void
holdlwp()
{
	proc_t *p = curproc;
	kthread_t *t = curthread;

	mutex_enter(&p->p_lock);
	/*
	 * Don't terminate immediately if process is dumping core.
	 * Once process has core dump, all LWPs are then terminated.
	 */
	if (!(p->p_flag & COREDUMP)) {
		if ((p->p_flag & EXITLWPS) || (t->t_proc_flag & TP_EXITLWP))
			lwp_exit();
	}
	if (!(ISHOLD(p)) && !(p->p_flag & (HOLDFORK1|HOLDWATCH))) {
		mutex_exit(&p->p_lock);
		return;
	}
	/*
	 * stop() decrements p->p_lwprcnt and cv_signal()s &p->p_holdlwps
	 * when p->p_lwprcnt becomes zero.
	 */
	stop(PR_SUSPENDED, SUSPEND_NORMAL);
	if (p->p_flag & EXITLWPS)
		lwp_exit();
	mutex_exit(&p->p_lock);
}

/*
 * Have all lwps within the process hold at a point where they are
 * cloneable (HOLDFORK) or just safe w.r.t fork1 (HOLDFORK1).
 */
int
holdlwps(int holdflag)
{
	proc_t *p = curproc;

	ASSERT(holdflag == HOLDFORK || holdflag == HOLDFORK1);
	mutex_enter(&p->p_lock);
again:
	while (p->p_flag & (EXITLWPS|HOLDFORK|HOLDFORK1|HOLDWATCH)) {
		if (p->p_flag & EXITLWPS)
			lwp_exit();
		/*
		 * If another lwp is doing a fork(), we have to
		 * fail here and return to userland with EINTR.
		 */
		if (p->p_flag & HOLDFORK) {
			mutex_exit(&p->p_lock);
			return (0);
		}
		/*
		 * Another lwp is doing a fork1() or is undergoing
		 * watchpoint activity.  We hold here for it to complete.
		 */
		stop(PR_SUSPENDED, SUSPEND_NORMAL);
	}
	p->p_flag |= holdflag;
	pokelwps(p);
	--p->p_lwprcnt;
	/*
	 * Wait for the process to become quiescent (p->p_lwprcnt == 0).
	 */
	while (p->p_lwprcnt > 0) {
		/*
		 * Check if aborted by exitlwps().
		 * Also check if HOLDWATCH is set; it takes precedence.
		 */
		if (p->p_flag & (EXITLWPS|HOLDWATCH)) {
			p->p_lwprcnt++;
			p->p_flag &= ~holdflag;
			cv_broadcast(&p->p_holdlwps);
			goto again;
		}
		/*
		 * Cooperate with jobcontrol signals and /proc stopping.
		 * If some other lwp has stopped by either of these
		 * mechanisms, then p_lwprcnt will never become zero
		 * and the process will appear deadlocked unless we
		 * stop here in sympathy with the other lwp before
		 * doing the cv_wait() below.
		 *
		 * If the other lwp stops after we do the cv_wait(), it
		 * will wake us up to loop around and do the sympathy stop.
		 *
		 * Since stop() drops p->p_lock, we must start from
		 * the top again on returning from stop().
		 */
		if (p->p_stopsig |
		    (curthread->t_proc_flag & TP_PRSTOP)) {
			int whystop = p->p_stopsig? PR_JOBCONTROL :
					PR_REQUESTED;
			p->p_lwprcnt++;
			p->p_flag &= ~holdflag;
			stop(whystop, p->p_stopsig);
			goto again;
		}
		cv_wait(&p->p_holdlwps, &p->p_lock);
	}
	p->p_lwprcnt++;
	p->p_flag &= ~holdflag;
	mutex_exit(&p->p_lock);
	return (1);
}

/*
 * Have all other lwps within the process hold themselves in the kernel
 * while the active lwp undergoes watchpoint activity (remapping a page).
 * If the active lwp stops in this function, return zero to indicate
 * to the caller that the list of watched pages may have changed.
 * A non-zero return indicates that the list has not been changed.
 */
int
holdwatch()
{
	proc_t *p = curproc;

	mutex_enter(&p->p_lock);
	while (p->p_flag & (EXITLWPS|HOLDFORK|HOLDFORK1|HOLDWATCH)) {
		if (p->p_flag & EXITLWPS)
			lwp_exit();
		/*
		 * If another lwp is doing a fork1() or is undergoing
		 * watchpoint activity, we hold here for it to complete.
		 */
		if (p->p_flag & (HOLDFORK1|HOLDWATCH)) {
			stop(PR_SUSPENDED, SUSPEND_NORMAL);
			if (p->p_flag & EXITLWPS)
				lwp_exit();
			mutex_exit(&p->p_lock);
			return (0);
		}
		/*
		 * Give precedence to HOLDWATCH if a HOLDFORK is in progress.
		 * See holdlwps() above to see details of this cooperation.
		 */
		while (p->p_flag & HOLDFORK) {
			p->p_flag |= HOLDWATCH;
			cv_broadcast(&p->p_holdlwps);
			cv_wait(&p->p_holdlwps, &p->p_lock);
			p->p_flag &= ~HOLDWATCH;
		}
	}
	p->p_flag |= HOLDWATCH;
	pokelwps(p);
	--p->p_lwprcnt;
	/*
	 * Wait for the process to become quiescent (p->p_lwprcnt == 0).
	 */
	while (p->p_lwprcnt > 0) {
		/*
		 * See comments in holdlwps(), above.
		 */
		if (p->p_flag & EXITLWPS) {
			p->p_lwprcnt++;
			p->p_flag &= ~HOLDWATCH;
			cv_broadcast(&p->p_holdlwps);
			lwp_exit();
		}
		if (p->p_stopsig ||
		    (curthread->t_proc_flag & TP_PRSTOP)) {
			int whystop = p->p_stopsig? PR_JOBCONTROL :
					PR_REQUESTED;
			p->p_lwprcnt++;
			p->p_flag &= ~HOLDWATCH;
			stop(whystop, p->p_stopsig);
			if (p->p_flag & EXITLWPS)
				lwp_exit();
			mutex_exit(&p->p_lock);
			return (0);
		}
		cv_wait(&p->p_holdlwps, &p->p_lock);
	}
	p->p_lwprcnt++;
	p->p_flag &= ~HOLDWATCH;
	mutex_exit(&p->p_lock);
	return (1);
}

/*
 * force all interruptible lwps to trap into the kernel.
 */
void
pokelwps(proc_t *p)
{
	kthread_id_t t;

	ASSERT(MUTEX_HELD(&p->p_lock));

	t = p->p_tlist;
	do {
		if (t == curthread)
			continue;
		thread_lock(t);
		aston(t);	/* make thread trap or do post_syscall */
		if (t->t_state == TS_SLEEP) {
			if (t->t_flag & T_WAKEABLE)
				setrun_locked(t);
		} else if (t->t_state == TS_STOPPED) {
			/*
			 * Ensure that exit() is not blocked by lwps
			 * that were stopped via jobcontrol or /proc.
			 */
			if (p->p_flag & EXITLWPS) {
				p->p_stopsig = 0;
				t->t_schedflag |= (TS_XSTART | TS_PSTART);
				setrun_locked(t);
			}
			/*
			 * If we are holding lwps for a fork(),
			 * force lwps that have been suspended via
			 * lwp_suspend() and are suspended inside
			 * of a system call to proceed to their
			 * holdlwp() points where they are clonable.
			 */
			if ((p->p_flag & HOLDFORK) && SUSPENDED(t)) {
				if (ttolwp(t)->lwp_asleep &&
				    (t->t_schedflag & TS_CSTART) == 0) {
					p->p_lwprcnt++;
					t->t_schedflag |= TS_CSTART;
					setrun_locked(t);
				}
			}
		} else if (t->t_state == TS_ONPROC) {
			if (t->t_cpu != CPU)
				poke_cpu(t->t_cpu->cpu_id);
		}
		thread_unlock(t);
	} while ((t = t->t_forw) != p->p_tlist);
}

/*
 * undo the effects of holdlwps() or holdwatch().
 */
void
continuelwps(proc_t *p)
{
	kthread_t *t;

	ASSERT(MUTEX_HELD(&p->p_lock));
	ASSERT((p->p_flag & (HOLDFORK|HOLDFORK1|HOLDWATCH)) == 0);

	t = p->p_tlist;
	do {
		thread_lock(t);		/* SUSPENDED looks at t_schedflag */
		if (SUSPENDED(t) && !(t->t_proc_flag & TP_HOLDLWP)) {
			p->p_lwprcnt++;
			t->t_schedflag |= TS_CSTART;
			setrun_locked(t);
		}
		thread_unlock(t);
	} while ((t = t->t_forw) != p->p_tlist);
}

/*
 * Make sure we don't get a SIGWAITING after terminating all
 * the other lwps and becoming a single-threaded process again.
 */
static void
turn_off_sigwait(register proc_t *p)
{
	register klwp_t *lwp = ttolwp(curthread);

	ASSERT(MUTEX_HELD(&p->p_lock));
	if (p->p_flag & SWAITSIG) {
		p->p_flag &= ~SWAITSIG;
		p->p_lwpblocked--;
	}
	sigdelset(&curthread->t_sig, SIGWAITING);
	sigdelq(p, curthread, SIGWAITING);
	sigdelset(&p->p_sig, SIGWAITING);
	sigdelq(p, NULL, SIGWAITING);
	if (lwp->lwp_cursig == SIGWAITING) {
		lwp->lwp_cursig = 0;
		if (lwp->lwp_curinfo) {
			siginfofree(lwp->lwp_curinfo);
			lwp->lwp_curinfo = NULL;
		}
	}
}

/*
 * every lwp within the process except for the current one must stop
 * before the process can be turned into a zombie. when the lwps have
 * stopped they can be rendered.
 * The 'coredump' flag means stop all the lwps so a core dump can be taken.
 * exitlwps() will be called again without 'coredump' set to exit for real.
 */
void
exitlwps(int coredump)
{
	proc_t *p = curproc;
	kthread_t *t;
	int heldcnt;

	if (curthread->t_door)
		door_slam();
	if (curthread->t_schedctl)
		schedctl_cleanup();
	mutex_enter(&p->p_lock);
	if (p->p_lwpcnt == 1 && !p->p_zombcnt) {
		turn_off_sigwait(p);
		mutex_exit(&p->p_lock);
		return;
	}
	if (p->p_lwpcnt > 1) {
		if (p->p_flag & EXITLWPS)
			lwp_exit();	/* never returns */

		p->p_flag |= EXITLWPS;
		if (coredump)	/* tell other lwps to stop, not exit */
			p->p_flag |= COREDUMP;

		/*
		 * Give precedence to exitlwps() if a holdlwps() is
		 * in progress. The LWP doing the holdlwps() operation
		 * is aborted when it is awakened.
		 */
		while (p->p_flag & (HOLDFORK|HOLDFORK1|HOLDWATCH)) {
			cv_broadcast(&p->p_holdlwps);
			cv_wait(&p->p_holdlwps, &p->p_lock);
		}
		p->p_flag |= HOLDFORK;
		pokelwps(p);
		--p->p_lwprcnt;
		/*
		 * Wait for process to become quiescent.
		 * (p->p_lwprcnt == 0)
		 */
		while (p->p_lwprcnt > 0)
			cv_wait(&p->p_holdlwps, &p->p_lock);
		p->p_lwprcnt++;
		ASSERT(p->p_lwprcnt == 1);
		/*
		 * The COREDUMP flag puts the process into a quiescent
		 * state. The process's LWPs remain attached to this
		 * process until exitlwps() is called again without
		 * the 'coredump' flag set, then the LWPs are terminated
		 * and the process can exit.
		 */
		if (coredump) {
			p->p_flag &= ~(COREDUMP|HOLDFORK|EXITLWPS);
			turn_off_sigwait(p);
			mutex_exit(&p->p_lock);
			return;
		}
		/*
		 * Determine if there are any LWPs left dangling in
		 * the stopped state. This happens when exitlwps()
		 * aborts a holdlwps() operation.
		 */
		p->p_flag &= ~HOLDFORK;
		if ((heldcnt = p->p_lwpcnt) > 1) {
			t = curthread->t_forw;
			while (--heldcnt > 0) {
				t->t_proc_flag &= ~TP_TWAIT;
				lwp_continue(t);
				t = t->t_forw;
			}
		}
	} else if (coredump) {
		p->p_flag &= ~(HOLDFORK|EXITLWPS);
		turn_off_sigwait(p);
		mutex_exit(&p->p_lock);
		return;
	}
	/*
	 * Wait for non detached LWPs to become zombies so
	 * that they can be freed up below.
	 */
	--p->p_lwprcnt;
	while (p->p_lwprcnt > 0)
		cv_wait(&p->p_holdlwps, &p->p_lock);
	++p->p_lwprcnt;
	ASSERT(p->p_lwpcnt == 1 && p->p_lwprcnt == 1);
	p->p_flag &= ~EXITLWPS;
	curthread->t_proc_flag &= ~TP_TWAIT;
	turn_off_sigwait(p);
	mutex_exit(&p->p_lock);
	/*
	 * If there are zombies left in this process then detach
	 * them and put them onto death-row.
	 */
	if ((t = p->p_zomblist) != NULL) {
		mutex_enter(&reaplock);
		lwp_reapcnt += p->p_zombcnt;
		p->p_zombcnt = 0;
		t->t_back->t_forw = lwp_deathrow;
		lwp_deathrow = t;
		mutex_exit(&reaplock);
	}
}

/*
 * duplicate a lwp.
 */
klwp_id_t
forklwp(klwp_id_t lwp, proc_t *cp)
{
	klwp_id_t clwp;
	void *tregs, *tfpu;
	kthread_id_t t = lwptot(lwp);
	kthread_id_t ct;
	extern void lwp_rtt(void);
	proc_t *p = lwptoproc(lwp);
	int	cid;
	void	*bufp;
	size_t	bufsz;

#ifdef	sparc
	if (t == curthread)
		(void) flush_user_windows_to_stack(NULL);
#endif

	save_syscall_args();	/* copy args out of registers first */
	clwp = lwp_create(lwp_rtt, 0, 0, cp, TS_STOPPED,
	    t->t_pri, t->t_hold, NOCLASS);
	if (clwp == NULL)
		return (NULL);
	/*
	 * most of the parent's lwp can be copied to its duplicate,
	 * except for the fields that are unique to each lwp, like
	 * lwp_thread, lwp_procp, lwp_regs, and lwp_ap.
	 */
	ct = clwp->lwp_thread;
	tregs = clwp->lwp_regs;
	tfpu = clwp->lwp_fpu;

	/* copy parent lwp to child lwp */
	*clwp = *lwp;

	/*
	 * if we are cloning the parent's "aslwp" daemon lwp, make sure the
	 * child's p_aslwptp pointer points to this cloned lwp.
	 */
	if (t == curproc->p_aslwptp)
		cp->p_aslwptp = ct;

	/* fix up child's lwp */
	clwp->lwp_cursig = 0;
	clwp->lwp_curinfo = (struct sigqueue *)0;
	clwp->lwp_thread = ct;
	ct->t_sysnum = t->t_sysnum;
	clwp->lwp_regs = tregs;
	clwp->lwp_fpu = tfpu;
	clwp->lwp_ap = clwp->lwp_arg;
	clwp->lwp_procp = cp;
	bzero((caddr_t)clwp->lwp_timer, sizeof (clwp->lwp_timer));
	init_mstate(ct, LMS_STOPPED);
	bzero((caddr_t)&clwp->lwp_ru, sizeof (clwp->lwp_ru));
	clwp->lwp_lastfault = 0;
	clwp->lwp_lastfaddr = 0;
	clwp->lwp_stime = 0;
	clwp->lwp_utime = 0;

	/* copy parent's struct regs to child. */
	lwp_forkregs(lwp, clwp);

	/* fork device context, if any */
	if (t->t_ctx)
		forkctx(t, ct);

retry:
	cid = t->t_cid;
	bufsz = sclass[cid].cl_size;
	if (bufsz != 0)
		bufp = kmem_alloc(bufsz, KM_SLEEP);
	mutex_enter(&p->p_lock);
	if (cid != t->t_cid) {
		/*
		 * Someone just changed this thread's scheduling class,
		 * so try pre-allocating the buffer again.  Hopefully we
		 * don't hit this often.
		 */
		mutex_exit(&p->p_lock);
		kmem_free(bufp, bufsz);
		goto retry;
	}
	if (t->t_proc_flag & TP_HOLDLWP)
		ct->t_proc_flag |= TP_HOLDLWP;
	ct->t_clfuncs = t->t_clfuncs;
	CL_FORK(t, ct, bufp);
	ct->t_cid = t->t_cid;	/* after data allocated so prgetpsinfo works */
	mutex_exit(&p->p_lock);

	return (clwp);
}
