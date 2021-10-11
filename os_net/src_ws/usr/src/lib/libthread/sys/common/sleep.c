/*	Copyright (c) 1993 SMI	*/
/*	 All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SMI	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)sleep.c	1.25	96/02/22	SMI"


#ifndef ABI
#ifdef __STDC__
#pragma weak sleep = _sleep

#pragma	weak _ti_sleep = _sleep
#endif
#endif
#include "libthread.h"
#include "sys/time.h"


/*
 * Suspend the thread for `sleep_tm' seconds - (mis)using cond_timedwait().
 * If cond_timedwait() returns prematurely, due to a signal, return to the
 * caller the (unsigned) quantity of (requested) seconds unslept.
 * No interaction with alarm().
 */
unsigned
_sleep(unsigned sleep_tm)
{
	cond_t sleep_cv = DEFAULTCV; /* dummy cv and mutex */
	mutex_t   sleep_mx = DEFAULTMUTEX;
	struct timeval wake_tv;
	timestruc_t alarm_ts;
	timestruc_t balarm_ts;
	int ret = 0;

	if (sleep_tm == 0)
		return (0);
	alarm_ts.tv_sec = time(NULL) + sleep_tm;
	alarm_ts.tv_nsec = 0;
	/* absolute time */
	if (ISBOUND(curthread)) {
		balarm_ts.tv_sec = sleep_tm;
		balarm_ts.tv_nsec = 0;
		_cancelon();
		ret = ___lwp_cond_wait(&sleep_cv, &sleep_mx, &balarm_ts);
		_canceloff();
		if (ret) {
			if (ret == EINVAL)
				_panic("sleep: ___lwp_cond_timedwait fails");
			else if (ret == ETIME)
				return (0); /* valid sleep */
		}
	} else {
		/*
		 * Call cond_timedwait() here  with absolute time.
		 */
		if (ret = _cond_timedwait_cancel(&sleep_cv, &sleep_mx,
							&alarm_ts)) {
			if (ret == EINVAL)
				_panic("sleep: cond_timedwait fails");
			else if (ret == ETIME)
				return (0); /* valid sleep */
		}
	}
	_gettimeofday(&wake_tv, NULL);
	if (wake_tv.tv_sec >= alarm_ts.tv_sec)
		return(0);
	else
		/* if premature wakeup */
		return (alarm_ts.tv_sec - wake_tv.tv_sec);
}
