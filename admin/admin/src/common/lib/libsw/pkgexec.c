#ifndef lint
#pragma ident "@(#)pkgexec.c 1.24 95/02/10 SMI"
#endif
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
#include "disk_lib.h"
#include "ibe_lib.h"

#include <sys/wait.h>
#include <ulimit.h>
#include <signal.h>
#include <errno.h>


/* external references */

extern int	start_pkgadd(char *);
extern int	end_pkgadd(char *);
extern int	interactive_pkgadd(u_int *);
extern int	interactive_pkgrm(u_int *);

/* Public Function Prototypes */

int      	swi_add_pkg (char *, PkgFlags *, char *);
int      	swi_remove_pkg (char *, PkgFlags *);

/* Local Function Prototypes */


/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * add_pkg()
 *	Adds the package specified by "pkgdir", using the command line
 *	arguments specified by "pkg_params".  "prod_dir" specifies the
 *	location of the package to be installed. Has both an interactive
 *	and non-interactive mode.
 * Parameters:
 *	pkg_dir	   - directory containing package
 *	pkg_params - packaging command line arguments
 *	prod_dir   - pathname for package to be installed
 * Returns
 *	SUCCESS		- successful
 *	ERR_PIPECREATE	- if input/output pipes cannot be created.
 *	ERR_ULIMIT	- if ulimit returns error
 *	ERR_FORKFAIL	- if the fork of the child process fails
 *	other		- Exit Status of pkgadd
 * Status:
 *	public
 */
int
swi_add_pkg (char * pkg_dir, PkgFlags * pkg_params, char * prod_dir)
{
	int	pid;
	u_int	status_loc = 0;
	int	options = WNOHANG;
	int	n;
	pid_t	waitstat;
	int	fdout[2];
	int	fderr[2];
	int	fdin[2];
	char	buffer[256];
	char	buf[MAXPATHLEN];
	int	size, nfds;
	struct timeval timeout;
	fd_set	readfds, writefds, execptfds;
	long	fds_limit;
	char	* cmdline[20];
	int	spool = FALSE;

	start_pkgadd(pkg_dir);

	/* if we're debugging, don't do any real work */
	if (get_sw_debug()) {
#ifdef DEBUG
		(void) printf("pkgadd");
		if (pkg_params != NULL) {
			if (pkg_params->spool != NULL) {
				spool = TRUE;
				(void) printf("pkgtrans -o");
				if (prod_dir != NULL)
					(void) printf("%s", prod_dir);
				else
					(void) printf("/var/spool/pkg");

				if (pkg_params->basedir != NULL) {
					(void) printf("%s/%s", pkg_params->basedir,
							pkg_params->spool);
				} else
					(void) printf("%s", pkg_params->spool);
			else {
				if (pkg_params->accelerated == 1)
					(void) printf(" -I");
				if (pkg_params->silent == 1)
					(void) printf(" -S");
				if (pkg_params->checksum == 1)
					(void) printf(" -C");
				if (pkg_params->basedir != NULL)
					(void) printf(" -R %s", pkg_params->basedir);
				if (admin_file(NULL) != NULL)
					(void) printf(" -a %s",
						(char*) admin_file(NULL));
				if (pkg_params->notinteractive == 1)
					(void) printf(" -n");
			}
		} else if (admin_file(NULL) != NULL)
			(void) printf(" -a %s", (char*) admin_file(NULL));

		if (prod_dir != NULL && spool == FALSE)
			(void) printf(" -d %s", prod_dir);

		(void) printf(" %s\n", pkg_dir);
#else DEBUG
		end_pkgadd(pkg_dir);
#endif DEBUG
		return (SUCCESS);
	}

	/* set up pipes to collect output from pkgadd */

	if ((pipe(fdout) == -1) || (pipe(fderr) == -1))
		return (ERR_PIPECREATE);

	if ((pkg_params->notinteractive == 0) && (pipe(fdin) == -1))
		return (ERR_PIPECREATE);

	if ((pid = fork()) == 0) {
		/* set stderr and stdout to pipes; set stdin if interactive */
		if (pkg_params->notinteractive == 0)
			(void) dup2(fdin[0], 0);

		(void) dup2(fdout[1], 1);
		(void) dup2(fderr[1], 2);
	   	(void) close(fdout[1]);
	   	(void) close(fdout[0]);
	   	(void) close(fderr[1]);
	   	(void) close(fderr[0]);

		if ((fds_limit = ulimit(UL_GDESLIM)) <= 0)
			return (ERR_ULIMIT);

		/* close all file descriptors in child */
		for (n = 3; n < fds_limit; n++)
			(void) close(n);

		/* build args for pkgadd command line */
		n = 0;
		cmdline[n++] = "/usr/sbin/pkgadd";

		/* use pkg_params to set command line */
		if (pkg_params != NULL) {
			if (pkg_params->spool != NULL) {
				spool = TRUE;
				n = 0;
				cmdline[n++] = "/usr/bin/pkgtrans";
				cmdline[n++] = "-o";
				if (prod_dir != NULL)
					cmdline[n++] = prod_dir;
				else
					cmdline[n++] = "/var/spool/pkg";

				if (pkg_params->basedir != NULL) {
					sprintf(buf,"%s/%s", pkg_params->basedir,
							pkg_params->spool);
					cmdline[n++] = buf;
				} else
					cmdline[n++] = pkg_params->spool;
			} else {
				if (pkg_params->accelerated == 1)
					cmdline[n++] = "-I";
				if (pkg_params->silent == 1)
					cmdline[n++] = "-S";
				if (pkg_params->checksum == 1)
					cmdline[n++] = "-C";
				if (pkg_params->basedir != NULL) {
					cmdline[n++] = "-R";
					cmdline[n++] = pkg_params->basedir;
				}
				if (admin_file(NULL) != NULL) {
					cmdline[n++] = "-a";
					cmdline[n++] = (char*)admin_file(NULL);
				}
				if (pkg_params->notinteractive == 1)
					cmdline[n++] = "-n";
			}

		} else {
			if (admin_file(NULL) != NULL) {
				cmdline[n++] = "-a";
				cmdline[n++] = (char*) admin_file(NULL);
			}
		}
		if (prod_dir != NULL && spool == FALSE) {
			cmdline[n++] = "-d";
			cmdline[n++] = prod_dir;
		}

		cmdline[n++] = pkg_dir;
		cmdline[n++] = (char*) 0;

		(void) sigignore(SIGALRM);
		(void) execv(cmdline[0], cmdline);

		/* i shouldn't be here, but ... */
		write_notice(ERROR, dgettext("SUNW_INSTALL_SWLIB",
				"Fail on exec of pkgadd\n"));
		_exit (-1);

	} else if (pid == -1) {
		if (pkg_params->notinteractive == 0)
			(void) close(fdin[0]);

		(void) close(fdout[1]);
		(void) close(fdout[0]);
		(void) close(fderr[1]);
		(void) close(fderr[0]);

		return (ERR_FORKFAIL);
	} else {
		if (pkg_params->notinteractive == 0) {
			interactive_pkgadd(&status_loc);
			(void) close(fdin[0]);
			(void) close(fdin[1]);
		} else {
			nfds = (fdout[0] > fderr[0]) ? fdout[0] : fderr[0];
			nfds++;
	   		timeout.tv_sec = 1;
	   		timeout.tv_usec = 0;

	   		do {
	   			FD_ZERO(&execptfds);
	   			FD_ZERO(&writefds);
	   			FD_ZERO(&readfds);
			   	FD_SET(fdout[0], &readfds);
			   	FD_SET(fderr[0], &readfds);

			   	if (select (nfds, &readfds, &writefds,
						&execptfds, &timeout) != -1) {
			   		if (FD_ISSET(fdout[0], &readfds)) {

						if ((size = read(fdout[0], buffer,
								sizeof (buffer))) != -1 ) {
							buffer[size] = '\0';
							write_status(LOG, LEVEL0|CONTINUE|PARTIAL, buffer);
						}
					}
					if (FD_ISSET(fderr[0], &readfds)) {

						if ((size = read(fderr[0], buffer,
								sizeof (buffer))) != -1 ) {
							buffer[size] = '\0';
							write_status(LOG, LEVEL0|CONTINUE|PARTIAL, buffer);
						}
					}
				}

		 		waitstat = waitpid(pid, (int*)&status_loc, options);

			} while ((!WIFEXITED(status_loc) && !WIFSIGNALED(status_loc)) ||
						(waitstat == 0));
		}
		(void) close(fdout[0]);
		(void) close(fdout[1]);
		(void) close(fderr[0]);
		(void) close(fderr[1]);

		end_pkgadd(pkg_dir);

		return (WEXITSTATUS(status_loc));
	}
	return (SUCCESS);
}


/*
 * remove_pkg()
 *
 * Parameters:
 *	pkg_dir	   -
 *	pkg_params -
 * Return:
 *
 * Status:
 *	public
 */
int
swi_remove_pkg (char * pkg_dir, PkgFlags * pkg_params)
{
	int	pid;
	u_int	status_loc = 0;
	int	options = WNOHANG;
	int	n;
	pid_t	waitstat;
	int	fdout[2];
	int	fderr[2];
	int	fdin[2];
	char	buffer[256];
	int	size, nfds;
	struct timeval timeout;
	fd_set	readfds, writefds, execptfds;
	long	fds_limit;
	char	*cmdline[20];

	/* if we're debugging, don't do any real work */
	if (get_sw_debug()) {
#ifdef DEBUG
		(void) printf("pkgadd");
		if (pkg_params != NULL) {
			if (pkg_params->accelerated == 1)
				(void) printf(" -I");
			if (pkg_params->silent == 1)
				(void) printf(" -S");
			if (pkg_params->checksum == 1)
				(void) printf(" -C");
			if (pkg_params->basedir != NULL)
				(void) printf(" -R %s", pkg_params->basedir);
			if (pkg_params->spool != NULL)
				(void) printf(" -s %s", pkg_params->spool);
			else {
				if (admin_file(NULL) != NULL)
					(void) printf(" -a %s",
						(char*) admin_file(NULL));
				if (pkg_params->notinteractive == 1)
					(void) printf(" -n");
			}
		} else if (admin_file(NULL) != NULL)
			(void) printf(" -a %s", (char*) admin_file(NULL));

		(void) printf(" %s\n", pkg_dir);
#endif DEBUG
		return (SUCCESS);
	}

	/* set up pipes to collect output from pkgadd */

	if ((pipe(fdout) == -1) || (pipe(fderr) == -1))
		return (ERR_PIPECREATE);

	if ((pkg_params->notinteractive == 0) && (pipe(fdin) == -1))
		return (ERR_PIPECREATE);

	if ((pid = fork()) == 0) {
		/* set stderr and stdout to pipes */
		/* set stdin if interactive */
		if (pkg_params->notinteractive == 0)
			(void) dup2(fdin[0], 0);

		(void) dup2(fdout[1], 1);
		(void) dup2(fderr[1], 2);
		(void) close(fdout[1]);
		(void) close(fdout[0]);
		(void) close(fderr[1]);
		(void) close(fderr[0]);

		if ((fds_limit = ulimit(UL_GDESLIM)) <= 0) {
			return (ERR_ULIMIT);
		}

		/* close all file descriptors in child */
		for (n = 3; n < fds_limit; n++)
			(void) close(n);

		/* build args for pkgadd command line */
		n = 0;
		cmdline[n++] = "/usr/sbin/pkgadd";

		/* use pkg_params to set command line */
		if (pkg_params != NULL) {
			if (pkg_params->accelerated == 1)
				cmdline[n++] = "-I";
			if (pkg_params->silent == 1)
				cmdline[n++] = "-S";
			if (pkg_params->checksum == 1)
				cmdline[n++] = "-C";

			if (pkg_params->basedir != NULL) {
				cmdline[n++] = "-R";
				cmdline[n++] = pkg_params->basedir;
			}
			if (pkg_params->spool != NULL) {
				cmdline[n++] = "-s";
				cmdline[n++] = pkg_params->spool;

			} else {
				if (admin_file(NULL) != NULL) {
					cmdline[n++] = "-a";
					cmdline[n++] = (char*)admin_file(NULL);
				}
				if (pkg_params->notinteractive == 1)
					cmdline[n++] = "-n";
			}
		} else {
			if (admin_file(NULL) != NULL) {
				cmdline[n++] = "-a";
				cmdline[n++] = (char*)admin_file(NULL);
			}
		}
		cmdline[n++] = pkg_dir;
		cmdline[n++] = (char*) 0;

		(void) execv("/usr/sbin/pkgadd", cmdline);

		/* i shouldn't be here, but ... */
		write_notice(ERROR, dgettext("SUNW_INSTALL_SWLIB",
				"Fail on exec of pkgrm"));
		_exit (-1);

	} else if (pid == -1) {
		if (pkg_params->notinteractive == 0) {
			(void) close(fdin[0]);
		}
		(void) close(fdout[1]);
		(void) close(fdout[0]);
		(void) close(fderr[1]);
		(void) close(fderr[0]);
		return (ERR_FORKFAIL);
	} else {
		if (pkg_params->notinteractive == 0) {
			interactive_pkgrm(&status_loc);
			(void) close(fdin[0]);
			(void) close(fdin[1]);
		} else {
			nfds = (fdout[0] > fderr[0]) ? fdout[0] : fderr[0];
			nfds++;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			do {
				FD_ZERO(&execptfds);
				FD_ZERO(&writefds);
				FD_ZERO(&readfds);
				FD_SET(fdout[0], &readfds);
				FD_SET(fderr[0], &readfds);
				(void) select (nfds, &readfds, &writefds,
						&execptfds, &timeout);

				if (FD_ISSET(fdout[0], &readfds)) {
					size = read(fdout[0], buffer,
							sizeof (buffer));
					buffer[size] = '\0';
					write_status(LOG, LEVEL0|CONTINUE, buffer);
				}
				if (FD_ISSET(fderr[0], &readfds)) {
					size = read(fderr[0], buffer,
								sizeof (buffer));
					buffer[size] = '\0';
					write_status(LOG, LEVEL0|CONTINUE, buffer);
				}
			 	waitstat = waitpid(pid, (int*)&status_loc,
						options);
			} while ((!WIFEXITED(status_loc) &&
					!WIFSIGNALED(status_loc)) || (waitstat == 0));
		}
		(void) close(fdout[0]);
		(void) close(fdout[1]);
		(void) close(fderr[0]);
		(void) close(fderr[1]);
		return (WEXITSTATUS(status_loc));
	}
	return (SUCCESS);
}
