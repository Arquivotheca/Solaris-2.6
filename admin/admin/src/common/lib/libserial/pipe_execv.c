#ifndef lint
#ident "@(#)pipe_execv.c 1.2 94/10/07 SMI"
#endif

/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include "serial_impl.h"

/*
 * pipe_execv mimics the 'read' side of popen() but accepts an arg-list
 * rather than having /bin/sh interpret a command-line.  In addition, the
 * third arg supplied is a pointer to a 'FILE *' which is returned filled in
 * with the stderr FILE* from the child.  If 'close_pipe_execl' returns a
 * non-zero status, the child's stderr output may be read.  In any case, the
 * 'errf' FILE* should be closed after 'close_pipe_execl' is called.
 */

static int pipe_execl_pid;

FILE *
pipe_execv(const char *path, char *const argv[], FILE **errf)
{
	int             p[2], e[2];
	int             parent_side, child_side, pid;
	int             parent_err, child_err;

	if(pipe(p) < 0)
		return(NULL);
	parent_side = p[STDIN_FILENO];	/* parent will be the reader */
	child_side = p[STDOUT_FILENO];	/* capture child's stdout */

	if(pipe(e) < 0)
		return(NULL);
	parent_err = e[STDIN_FILENO];	/* parent will be the reader */
	child_err = e[STDOUT_FILENO];	/* capture child's stderr */

	if ((pid = fork()) == 0) {
		/* child */
		/* trust the child - leave non-stdio file descriptors alone */
		(void) close(parent_side);
		(void) close(STDOUT_FILENO);
		if (fcntl(child_side, F_DUPFD, STDOUT_FILENO) != STDOUT_FILENO) {
			/* serious plumbing error, give up */
			serial_errno = SERIAL_ERR_PIPE_IO;
			return (NULL);
			/*NOTREACHED*/
		}
		(void) close(child_side); /* stdout is now the pipe */

		(void) close(parent_err);
		(void) close(STDERR_FILENO);
		if (fcntl(child_err, F_DUPFD, STDERR_FILENO) != STDERR_FILENO) {
			/* serious plumbing error, give up */
			serial_errno = SERIAL_ERR_PIPE_IO;
			return (NULL);
			/*NOTREACHED*/
		}
		(void) close(child_err); /* stderr is now the 2nd pipe */

		(void) execv(path, argv);
		/* serious plumbing error, give up */
		serial_errno = SERIAL_ERR_EXEC_CHILD;
		return (NULL);
		/*NOTREACHED*/
	}
	if(pid == -1)
		return(NULL);
	pipe_execl_pid = pid;
	(void) close(child_side);
	(void) close(child_err);
	*errf = fdopen(parent_err, "r");
	return(fdopen(parent_side, "r"));
}


/* close the pipe and return the child's exit status */
int
close_pipe_execl(FILE *fptr)
{
	int             status;

	(void) fclose(fptr);

	if (waitpid(pipe_execl_pid, &status, 0) < 0)
		status = -1;

	return (status);
}
