/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)adm_amsl_pipes.c	1.8	92/03/03 SMI"

/*
 * FILE:  pipes.c
 *
 *	Admin Framework class agent routines for handling pipes between
 *	the AMSL dispatch routine and the class method executable runfile.
 */

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include "adm_fw.h"
#include "adm_fw_impl.h"
#include "adm_amsl.h"
#include "adm_amsl_impl.h"

/*
 * -------------------------------------------------------------------
 *  init_pipes - Initialize pipes and poll structure for executing a
 *	class method executable runfile.
 *	Accepts pointer to the request's amsl_req structure, the poll
 *	file descripter structure.
 *	This routine creates pipes for STDIN, STDOUT, STDERR, STDARG and
 *	initializes the Pfd structure for polling events from these
 *	pipes.  Both ends of the pipes are opened.
 *	Returns zero if successful or Admin error status code if an
 *	error occurs, with error message on the top of the error stack
 *	pointed to by the amsl_req structure.
 * -------------------------------------------------------------------
 */

int
init_pipes(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd Pfd[])	/* Polling file descriptor structure */
{
	int tfd[2];		/* Temporary file descriptor array */
	int i;			/* Temporary integer */

	/* Clear the pollfd structure so failures will cleanup properly */
	for (i=0; i < (AMSL_NUM_PIPES * 2); i++) {
		Pfd[i].fd = -1;
		Pfd[i].events = 0;
		Pfd[i].revents = 0;
	}

	/* Allocate the pipes for STDIN, STDOUT, STDERR, STDFMT */
	for (i=0; i < (AMSL_NUM_PIPES * 2); i++) {
		if (pipe(tfd) != 0) {
			(void) free_pipes(Pfd);
			return (amsl_err(reqp, ADM_ERR_NOPIPE, errno, i));
		}
		Pfd[i].fd = tfd[0];
		Pfd[i].events = POLLIN;
		i++;
		Pfd[i].fd = tfd[1];
		Pfd[i].events = POLLOUT;

		/* NOTE:  Set any streamio options here! */
	}

	/* Return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  check_pipes - Check pipes to see how many are still open.
 *	Accepts pointer to the array of poll structures for the pipes.
 *	This routine returns the number of open pipes in the poll file
 *	descriptor structure.  Zero means all pipes are closed.
 * -------------------------------------------------------------------
 */

int
check_pipes(
	struct pollfd Pfd[])	/* Polling file descriptor structures */
{
	int count;		/* Number of open pipes */
	int i;			/* Temporary integer */

	count = 0;

	/* Check each pipe.  If open, increment count. */
	for (i = 0; i < (AMSL_NUM_PIPES * 2); i++) {
		if (Pfd[i].fd != -1)
			count++;
	}

	/* Return count */
	return (count);
}

/*
 * -------------------------------------------------------------------
 *  free_pipes - Cleanup pipes and poll structure for executing a
 *	class method executable runfile.
 *	Accepts pointer to the array of poll file descriptor structures.
 *	This routine closes down any open pipes for STDIN, STDOUT, STDERR,
 *	and STDARG.  It clears the polling file descripter structure.
 *	There is no return status.
 * -------------------------------------------------------------------
 */

void
free_pipes(
	struct pollfd Pfd[])	/* Polling file descriptor structure */
{
	int ofd;		/* Temporary file descriptor */
	int i;

	/* Close any open pipe file descriptors and clear poll structure */
	for (i = 0; i < (AMSL_NUM_PIPES * 2); i++) {
		if ((ofd = Pfd[i].fd) != -1) {
			(void) close(ofd);
			Pfd[i].fd = -1;
		}
	}

	/* Return without status */
	return;
}

/*
 * -------------------------------------------------------------------
 *  set_pipes - Sets up one end of the pipes for executing a
 *	class method executable runfile.
 *	Accepts pointer to the request's amsl_req structure, the poll
 *	file descripter structure, and a flag indicating whether to
 *	setup the parent end of the pipes or the child end of the pipes..
 *	This routine closes one end of the  pipes for STDIN, STDOUT, STDERR,
 *	STDARG (the end used by the other process).
 *	There is no return status.
 * -------------------------------------------------------------------
 */

int
set_pipes(
	struct amsl_req *reqp,	/* Pointer to amsl request structure */
	struct pollfd Pfd[],	/* Polling file descriptor structure */
	u_int  flag)		/* Flag: AMSL_PARENT or AMSL_CHILD */
{
	int ofd;		/* Old file descriptor for dup */
	int nfd;		/* New file descriptor from dup */

	/* Process ends of the pipes based on whether parent or child */
	switch (flag) {

	case AMSL_PARENT:		/* Parent process... */

		/* Close off end of pipes used by child process */
		(void) close(Pfd[AMSL_STDIN_READ].fd);
		Pfd[AMSL_STDIN_READ].fd = -1;
		(void) close(Pfd[AMSL_STDOUT_WRITE].fd);
		Pfd[AMSL_STDOUT_WRITE].fd = -1;
		(void) close(Pfd[AMSL_STDERR_WRITE].fd);
		Pfd[AMSL_STDERR_WRITE].fd = -1;
		(void) close(Pfd[AMSL_STDFMT_WRITE].fd);
		Pfd[AMSL_STDFMT_WRITE].fd = -1;

		/* Do NOT block on reads to STDOUT, STDERR, and STDFMT */
		(void) fcntl(Pfd[AMSL_STDOUT_READ].fd, (int)F_SETFL,
		    (int)O_NONBLOCK);
		(void) fcntl(Pfd[AMSL_STDERR_READ].fd, (int)F_SETFL,
		    (int)O_NONBLOCK);
		(void) fcntl(Pfd[AMSL_STDFMT_READ].fd, (int)F_SETFL,
		    (int)O_NONBLOCK);
		break;

	case AMSL_CHILD:		/* Child process... */

		/* Close off end of pipes used by parent process */
		(void) close(Pfd[AMSL_STDIN_WRITE].fd);
		Pfd[AMSL_STDIN_WRITE].fd = -1;
		(void) close(Pfd[AMSL_STDOUT_READ].fd);
		Pfd[AMSL_STDOUT_READ].fd = -1;
		(void) close(Pfd[AMSL_STDERR_READ].fd);
		Pfd[AMSL_STDERR_READ].fd = -1;
		(void) close(Pfd[AMSL_STDFMT_READ].fd);
		Pfd[AMSL_STDFMT_READ].fd = -1;

		/* Connect child's STDIN for reading from pipe */
		ofd = Pfd[AMSL_STDIN_READ].fd;
		if ((nfd = dup2(ofd, AMSL_STDIN_FD)) == -1) {
			(void) free_pipes(Pfd);
			return (amsl_err(reqp, ADM_ERR_CHLDDUPFD, errno, 0));
		}
		(void) close(ofd);
		Pfd[AMSL_STDIN_READ].fd = nfd;

		/* Connect child's STDOUT for writing to pipe */
		ofd = Pfd[AMSL_STDOUT_WRITE].fd;
		if ((nfd = dup2(ofd, AMSL_STDOUT_FD)) == -1) {
			(void) free_pipes(Pfd);
			return (amsl_err(reqp, ADM_ERR_CHLDDUPFD, errno, 1));
		}
		(void) close(ofd);
		Pfd[AMSL_STDOUT_WRITE].fd = nfd;

		/* Connect child's STDERR for writing to pipe */
		ofd = Pfd[AMSL_STDERR_WRITE].fd;
		if ((nfd = dup2(ofd, AMSL_STDERR_FD)) == -1) {
			(void) free_pipes(Pfd);
			return (amsl_err(reqp, ADM_ERR_CHLDDUPFD, errno, 2));
		}
		(void) close(ofd);
		Pfd[AMSL_STDERR_WRITE].fd = nfd;
		break;

	default:			/* Invalid flag value */
		(void) free_pipes(Pfd);
		return (amsl_err(reqp, ADM_ERR_BADPIPEFLAG, flag));
		/*NOTREACHED*/
		break;
	}

	/* Return success */
	return (0);
}

/*
 * -------------------------------------------------------------------
 *  close_pipe - Close one end of a pipe and cleanup Pfd entry for it.
 *	Accepts pointer to a poll file descriptor structure representing
 *	one end of a pipe.
 *	There is no return status.
 * -------------------------------------------------------------------
 */

void
close_pipe(
	struct pollfd *pfd)	/* Ptr to pipe file descriptor structure */
{
	if ((pfd->fd) != -1) {
		ADM_DBG("i", ("Invoke: Pipe closed for fd=%d", pfd->fd));
		(void) close(pfd->fd);
		pfd->fd = -1;
	}

	/* Return without status */
	return;
}
