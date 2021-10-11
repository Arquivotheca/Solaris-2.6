
/*
 *  Copyright (c) 1990 Sun Microsystems, Inc.  All Rights Reserved.
 */

/*
 * FILE:  adm_lock_impl.c
 *
 *	Admin Framework routines for handling locking of framework
 *	system files, such as log file, recovery file, etc.
 */

#pragma	ident	"@(#)adm_lock_impl.c	1.7	95/06/29 SMI"

#include <errno.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"

#define	MAX_INTERRUPTS	3	/* Maximum number of interrupts */

/*
 * -------------------------------------------------------------------
 *  adm_lock - Lock a file for shared, exclusive, or append use.
 *	Accepts the file descriptor of the file to be locked, the type
 *	of lock to be taken (shared, exclusive, append), and the number
 *	of seconds to wait for the lock; where 0 = no waiting and -1 =
 *	wait forever.
 *	This routine uses the locking facilities of the fcntl system
 *	call.  Since SNM uses SIGALRM and setitimer, we are forced to
 *	use a sleep and retry algorithm (yuk!).
 *	Returns 0 if lock obtained; Errno otherwise.
 * -------------------------------------------------------------------
 */

int
adm_lock(
	int  fd,		/* File descriptor of file to lock */
	int  type,		/* Type of lock to obtain */
	int  waittime)		/* Number of seconds to wait for a lock */
{
	struct flock flck;	/* fcntl lock control structure */
	int  lock_cmd;		/* fcntl lock command */
	int  wtime;		/* Wait time */
	int  int_count;		/* Number of interrupt retries */
	int  wait_count;	/* Number of timeout retries */
	int  stat;		/* Lock status */

	stat = 0;				/* Assume success */
	lock_cmd = F_SETLK;			/* Assume non-blocking */
	int_count = 0;
	wait_count = 0;

	/* Validate wait time argument and set lock command & count */
	wtime = waittime;
	if ((waittime < -1) || (waittime > ADM_LOCK_MAXWAIT))
		wtime = ADM_LOCK_MAXWAIT;
	if (wtime == -1)
		lock_cmd = F_SETLKW;
	if (wtime > 0)
		wait_count = (wtime + ADM_LOCK_SLEEP - 1) / ADM_LOCK_SLEEP;

	/* Setup lock control structure for locking entire file */
	flck.l_whence = 0;
	flck.l_start = 0;
	flck.l_len = 0;

	/* Setup lock control structure for type of lock */
	switch (type) {
		case ADM_LOCK_SH:		/* Shared lock on file */
			flck.l_type = F_RDLCK;
			break;
		case ADM_LOCK_EX:		/* Exclusive lock on file */
		case ADM_LOCK_AP:		/* Append lock on file */
			flck.l_type = F_WRLCK;
			break;
		default:			/* Invalid lock type */
			stat = -1;
			break;
	}					/* End of switch */

	/* Try to get lock while handling various wait time cases */
	while (stat == 0) {

		/* Try to get the lock */
		if ((fcntl(fd, lock_cmd, &flck)) != -1)
			break;			/* !!! Got it !!! */

		/* If interrupt, check if too many already */
		if (errno == EINTR)
			if (int_count < MAX_INTERRUPTS) {
				int_count++;
				continue;
			} else {
				stat = -1;
				break;
			}

		/* Check for a fatal error from fcntl */
		if ((errno != EACCES) && (errno != EAGAIN)) {
			stat = errno;
			break;
		}

		/* Lock already held; check time cases for what to do */
		switch (wtime) {
			case -1:		/* Wait forever; loop */
				break;

			case 0:			/* Do not wait at all */
				stat = -1;
				break;

			default:		/* Wait for given period */
				if (wait_count > 0) {
					wait_count--;
					sleep(ADM_LOCK_SLEEP);
				}
				else
					stat = -1;
				break;
		}				/* End of switch */

		/* See if we give up */
		if (stat == -1)
			break;
	}					/* End of while */

	/* Return the lock status */
	return (stat);
}

/*
 * -------------------------------------------------------------------
 *  adm_unlock - Unlock a previously locked file.
 *	Accepts the file descriptor of the file to be unlocked.
 *	This routine uses the locking facilities of the fcntl system
 *	call.
 *	Returns 0 if unlock successful; Errno otherwise.
 * -------------------------------------------------------------------
 */

int
adm_unlock(
	int  fd)		/* File descriptor of file to lock */
{
	struct flock flck;	/* fnctl lock control structure */
	int  stat;		/* Status return code */

	/* Setup lock control structure for unlocking entire file */
	flck.l_whence = 0;
	flck.l_start = 0;
	flck.l_len = 0;

	/* Setup lock control structure for type of lock */
	flck.l_type = F_UNLCK;

	/* Try to unlock the file */
	if ((stat = fcntl(fd, (int)F_SETLK, &flck)) == -1)
		stat = errno;
	else
		stat = 0;

	/* Return success */
	return (stat);
}
