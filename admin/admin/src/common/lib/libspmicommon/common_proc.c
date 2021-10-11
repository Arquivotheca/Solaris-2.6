#ifndef lint
#pragma ident "@(#)common_proc.c 1.2 96/01/03 SMI"
#endif
/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	common_proc.c
 * Group:	libspmicommon
 * Description:	This module contains functions used to handle UNIX processes.
 */

#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/procfs.h>
#include "spmicommon_lib.h"

/* public prototypes */

int      	ProcWalk(int (*)(int, char *), char *);
int      	ProcIsRunning(int, char *);
int      	ProcKill(int, char *);

/* --------------------------- test function ------------------------ */
#ifdef MODULE_TEST
int
main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		(void) fprintf(stderr, 
			"usage:  proc procname1 [procname2...]\n");
		exit(-1);
	}

	while (--argc > 0)
		(void) ProcWalk(ProcKill, *++argv);
}
#else

/* -------------------- public prototypes --------------------------- */

/*
 * Function:	ProcWalk
 * Description:	Walk the process list by opening each file in "/proc".
 *		Call func once per process with a file descriptor open
 *		to the /proc entry and the [command] name of interest.
 *
 *		The registered function should return:
 *
 *		-1:	error in processing (do not terminate walk)
 *		 0:	function completed successfully
 *		 1:	function completed successfully and
 *			process table traversal should stop
 * Scope:	internal
 * Parameters:	none
 * Return:	func	[RO, *RO]
 *			Pointer to a function to be executed for each
 *			process in the process list.
 *		value	[RO, *RO]
 *			Value to pass into the processing function.
 * Return:	 0	successfull execution  on success,
 *		 1	premature termination (e.g. benign error)
 *		-1	fatal error during processing
 */
int
ProcWalk(int (*func)(int, char *), char *value)
{
	struct dirent *	dirp;
	DIR *		proc;
	char		cwd[MAXPATHLEN];
	int		status = 0;
	int		p;

	if ((getcwd(cwd, sizeof (cwd)) == NULL) ||
			((proc = opendir("/proc")) == NULL) ||
			(chdir("/proc") < 0))
		return (-1);

	while (((dirp = readdir(proc)) != NULL) && (status <= 0)) {
		if (streq(dirp->d_name, ".") || streq(dirp->d_name, "..") ||
				((p = open(dirp->d_name, O_RDWR)) < 0))
			continue;

		status = func(p, value);
		(void) close(p);
	}

	(void) closedir(proc);
	if (chdir(cwd) < 0)
		return (-1);

	return (status);
}

/*
 * Function:	ProcIsRunning
 * Description: Boolean function indicating whether or not a process of
 *		the name specified is currently executing. This function
 *		requires the proc file system be mounted on /proc.
 * Scope:	public
 * Parameters:	fd	[RO]
 *			Open file descriptor to a process file in /proc.
 *		name	[RO, *RO]
 *			Name of process for which a comparison is to be made.
 * Return:	 0	the process is not running
 *	 	 1	the process is running
 */
int
ProcIsRunning(int fd, char *name)
{
	prpsinfo_t	psinfo;

	/* validate parameters */
	if (name == NULL || ioctl(fd, PIOCPSINFO, &psinfo) < 0 ||
			!streq(psinfo.pr_fname, name))
		return (0);

	return (1);
}

/*
 * Function:	ProcKill
 * Description:	Send the running process a SIGTERM signal if it has a specific
 *		process name.
 * Scope:	public
 * Parameters:	fd	[RO]
 *			Open file descriptor to a process file in /proc.
 *		name	[RO, *RO]
 *			Name of process which is to be termianted.
 * Return:	 0	the specified process was successfully terminated
 *		-1	no process was terminated
 */
int
ProcKill(int fd, char *name)
{
	prpsinfo_t	psinfo;
	int		term = SIGTERM;

	if (name != NULL && ioctl(fd, PIOCPSINFO, &psinfo) == 0 &&
			streq(psinfo.pr_fname, name) &&
			(ioctl(fd, PIOCKILL, &term) == 0))
		return (0);;

	return (-1);
}
#endif /* MODULE_TEST */
