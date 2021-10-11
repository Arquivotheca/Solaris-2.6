/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sysconfig.c	1.6	96/10/17 SMI"	/* from SVr4.0 1.78 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/systm.h>
#include <sys/tuneable.h>
#include <sys/errno.h>
#include <sys/var.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/sysconfig.h>
#include <sys/resource.h>
#include <sys/ulimit.h>
#include <sys/unistd.h>
#include <sys/debug.h>
#include <sys/cpuvar.h>

int
sysconfig(int which)
{
	switch (which) {

	/*
	 * if it is not handled in mach_sysconfig either
	 * it must be EINVAL.
	 */
	default:
		return (mach_sysconfig(which)); /* `uname -i`/os */

	case _CONFIG_CLK_TCK:
		return (hz);	/* clock frequency per second */

	case _CONFIG_PROF_TCK:
		return (hz);	/* profiling clock freq per sec */

	case _CONFIG_NGROUPS:
		/*
		 * Maximum number of supplementary groups.
		 */
		return (ngroups_max);

	case _CONFIG_OPEN_FILES:
		/*
		 * Maxiumum number of open files (soft limit).
		 */
		return ((u_int)U_CURLIMIT(&u, RLIMIT_NOFILE));

	case _CONFIG_CHILD_MAX:
		/*
		 * Maxiumum number of processes.
		 */
		return (v.v_maxup);

	case _CONFIG_POSIX_VER:
		return (_POSIX_VERSION); /* current POSIX version */

	case _CONFIG_PAGESIZE:
		return (PAGESIZE);

	case _CONFIG_XOPEN_VER:
		return (_XOPEN_VERSION); /* current XOPEN version */

	case _CONFIG_NPROC_CONF:
		return (ncpus);

	case _CONFIG_NPROC_ONLN:
		return (ncpus_online);

	case _CONFIG_AIO_LISTIO_MAX:
		return (_AIO_LISTIO_MAX);

	case _CONFIG_AIO_MAX:
		return (_AIO_MAX);

	case _CONFIG_AIO_PRIO_DELTA_MAX:
		return (_AIO_PRIO_DELTA_MAX);

	case _CONFIG_DELAYTIMER_MAX:
		return (LONG_MAX);

	case _CONFIG_MQ_OPEN_MAX:
		return (_MQ_OPEN_MAX);

	case _CONFIG_MQ_PRIO_MAX:
		return (_MQ_PRIO_MAX);

	case _CONFIG_RTSIG_MAX:
		return (_SIGRTMAX - _SIGRTMIN + 1);

	case _CONFIG_SEM_NSEMS_MAX:
		return (_SEM_NSEMS_MAX);

	case _CONFIG_SEM_VALUE_MAX:
		return (_SEM_VALUE_MAX);

	case _CONFIG_SIGQUEUE_MAX:
		return (_SIGQUEUE_MAX);

	case _CONFIG_SIGRT_MIN:
		return (_SIGRTMIN);

	case _CONFIG_SIGRT_MAX:
		return (_SIGRTMAX);

	case _CONFIG_TIMER_MAX:
		return (_TIMER_MAX);

	case _CONFIG_PHYS_PAGES:
		return (physinstalled);

	case _CONFIG_AVPHYS_PAGES:
		return (freemem);
	}
}
