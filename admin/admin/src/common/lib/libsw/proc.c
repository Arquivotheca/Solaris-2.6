#ifndef lint
#pragma ident   "@(#)proc.c 1.3 93/12/16 SMI"
#endif lint
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */
#include "sw_lib.h"

#include <sys/signal.h>
#include <sys/procfs.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

/* Public Support Functions */

/* Library Support Functions */

int      	proc_walk(int (*)(int, char *), char *);
int      	proc_running(int, char *);
int      	proc_kill(int, char *);

/* Local Support Functions */

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * proc_walk()
 *	Walk the process list by opening each file in "/proc".
 *	Call func once per process with a file descriptor open
 *	to the /proc entry and the [command] name of interest.
 *
 *	func should return as follows:
 *
 *		-1:	error (NB:  this does not stop traversal)
 *		 0:	function completed successfully
 *		 1:	function completed successfully and
 *			process table traversal should stop
 *
 *	proc_walk returns 1 on success, 0 on benign error
 *	 (such as no processes with given name running) and
 *	 -1 on system error.
 * Parameters:
 *	name
 * Return:
 * Status:
 *	semi-private (internal library use only)
 */
int
proc_walk(int (*func)(int, char *), char * name)
{
	struct dirent *dirp;
	DIR	*proc;
	char	cwd[MAXPATHLEN];
	int	status, p;

	status = 0;

	if (getcwd(cwd, sizeof (cwd)) == (char *)0) {
		perror("chdir");
		return (-1);
	}

	proc = opendir("/proc");
	if (proc == (DIR *)0) {
		perror("opendir");
		return (-1);
	}
	if (chdir("/proc") < 0) {
		perror("chdir");
		return (-1);
	}
	while (dirp = readdir(proc)) {
		if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0)
			continue;
		p = open(dirp->d_name, O_RDWR);
		if (p < 0)
			continue;
		if (func(p, name) > 0) {
			status = 1;
			(void) close(p);
			break;
		}
		(void) close(p);
	}

	(void) closedir(proc);

	if (chdir(cwd) < 0) {
		perror("chdir");
		return (-1);
	}
	return (status);
}

/*
 * proc_running()
 *	Signal proc_walk to stop traversal when we find
 *	a process that has the given name.
 * Parameters:
 *	fd	-
 *	name	-
 * Return:
 *	 0	-
 *	 1	-
 *	-1
 * Status:
 *	semi-private (internal library use only)
 */
int
proc_running(int fd, char * name)
{
	prpsinfo_t psinfo;

	if (ioctl(fd, PIOCPSINFO, &psinfo) < 0) {
		perror("ioctl (PIOCPSINFO)");
		return (-1);
	}

	if (strcmp(psinfo.pr_fname, name) == 0)
		return (1);
	return (0);
}

/*
 * proc_kill()
 *	Kill process if it has the given name.
 * Parameters:
 *	fd	-
 *	name	-
 * Return:
 *	 0	-
 *	-1	-
 *	 1	-
 * Status:
 *	semi-private (internal library use only)
 */
int
proc_kill(int fd, char * name)
{
	prpsinfo_t psinfo;
	int	term = SIGTERM;

	if (ioctl(fd, PIOCPSINFO, &psinfo) < 0) {
		perror("ioctl (PIOCPSINFO)");
		return (-1);
	}

	if (strcmp(psinfo.pr_fname, name) == 0) {
		if (ioctl(fd, PIOCKILL, &term) < 0) {
			perror("ioctl (PIOCKILL)");
			return (-1);
		}
	}
	return (0);
}

#ifdef STANDALONE
main(argc, argv)
	int	argc;
	char	**argv;
{
	if (argc < 2) {
		(void) fprintf(stderr, 
			"usage:  proc procname1 [procname2...]\n");
		exit(-1);
	}
	while (--argc > 0) {
		(void) proc_walk(proc_kill, *++argv);
	}
}
#endif
