#ifndef lint
#pragma ident "@(#)ibe_util.c 1.49 95/01/27 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
#include "disk_lib.h"
#include "ibe_lib.h"

#include <sys/systeminfo.h>
#include <sys/stat.h>
#include <sys/filio.h>
#include <sys/mman.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <dirent.h>
#include <fcntl.h>

/* Local Constants */

#define	N_PROG_LOCKS	10

/* Local Statics */

static int	_install_debug = 0;

/* Globals and Externals */

extern char	**environ;		/* for exec's */

/* Public Function Prototypes */

int		set_install_debug(int);
int		get_install_debug(void);
int		reset_system_state(void);

/* Library Function Prototypes */

int		_create_dir(char *);
int		_lock_prog(char *);
int		_arch_cmp(char *, char *, char *);
int		_copy_file(char *, char *);
int		_build_admin(Admin_file *);

/* Local Function Prototypes */

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * set_install_debug()
 *	Set the _install_debug variable to "on" (!0) or "off" (0).
 *	Arguments which are not '0' are assumed to mean '1'. The
 *	old value is returned.
 * Parameters:
 *	state	- '0' (clear) or '1' (set)
 * Return:
 *	0,1	- old state
 * Status:
 *	public
 */
int
set_install_debug(int state)
{
	int	old;

	old = _install_debug;
	if (state == 0)
		_install_debug = 0;
	else
		_install_debug = 1;

	return (old);
}

/*
 * get_install_debug()
 *	Return the current _install_debug state.
 * Parameters:
 *	none
 * Return:
 *	0	- _install_debug is currently deactivated
 *	1	- _install_debug is currently active
 * Status:
 *	public
 */
int
get_install_debug(void)
{
	return (_install_debug);
}

/*
 * reset_system_state()
 *	Routine used to reset the state of the system. Resetting
 *	includes:
 *
 *	(1) halt all running newfs and fsck processes which
 *	    may still be active
 *	(2) unregister all currently active swap devices
 *	(3) unmount all filesystems registered in /etc/mnttab
 *	    to be mounted under /a (NOTE: this must be done after
 *	    killing fscks or a "busy" may be recorded)
 *
 * Parameters:
 *	none
 * Return:
 *	 0	- reset successful
 *	-1	- reset failed
 * Status:
 *	public
 */
int
reset_system_state(void)
{
	if (get_install_debug() == 0) {

		if (system("ps -e | egrep newfs >/dev/null 2>&1") == 0) {
			(void) system("kill -9 `ps -e | egrep newfs | \
				awk '{print $1}'` \
				`ps -e | egrep mkfs | awk '{print $1}'` \
				`ps -e | egrep fsirand | awk '{print $1}'` \
				> /dev/null 2>&1");
		}

		if (system("ps -e | egrep fsck >/dev/null 2>&1") == 0) {
			(void) system("kill -9 `ps -e | egrep fsck | \
				awk '{print $1}'` \
				> /dev/null 2>&1");
		}

		while (system("swap -l 2>&1 | egrep configured > \
				/dev/null 2>&1") != 0) {
			if (system("swap -d `swap -l | egrep -v swapfile | \
					head -1 | awk '{print $1}'` \
					>/dev/null 2>&1") != 0)
				return (-1);
		}

		if (umount_slash_a() < 0) {
			return (-1);
		}
	}

	return (0);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _build_admin()
 * 	Create the admin file for initial install only.
 * Parameters:
 *	admin	- non-NULL pointer to an Admin_file structure
 * Return:
 *	NOERR	- success
 *	ERROR	- setup attempt failed
 * Status:
 *	private
 */
int
_build_admin(Admin_file *admin)
{
	static char	_lbase[MAXPATHLEN] = "";

	/* verify admin is valid */
	if (admin == NULL)
		return (ERROR);

	/* if the basedir hasn't changed, return success */
	if (admin->basedir != NULL &&
			strcmp(admin->basedir, _lbase) == 0)
		return (NOERR);

	/* create and save admin file */
	if (admin_write(admin_file((char *)NULL), admin))
		return (ERROR);

	if (admin->basedir != NULL)
		(void) strcpy(_lbase, admin->basedir);

	return (NOERR);
}

/*
 * _create_dir()
 * 	Create all the directories in the path, setting the modes,
 *	owners and groups along the way.  This is a recursive function.
 * Parameters:
 *	path	- directory pathname
 * Returns:
 *	NOERR	- directory created successfully
 *	errno or FAIL (-1) on error
 * Status:
 *	semi-private (internal library use only)
 */
int
_create_dir(char *path)
{
	int	status;
	char	*slash;

	if (*path == '\0' || access(path, X_OK) == 0)
		return (NOERR);

	if (get_install_debug() == 1) {
		write_status(SCR,
			LEVEL1|LISTITEM, CREATING_MNTPNT, path);
		return (NOERR);
	}

	slash = strrchr(path, '/');
	if (slash != NULL) {
		*slash = '\0';
		status = _create_dir(path);
		*slash = '/';
		if (status != NOERR) {
			write_notice(ERRMSG, CREATE_MNTPNT_FAILED, path);
			return (status);
		}
	}

	if (mkdir(path, 0775) == NOERR)
		return (NOERR);

	if (errno != EEXIST)
		return (errno);

	return (NOERR);
}

/*
 * _lock_prog()
 * 	Lock the program specified by program into memory.  This function
 * 	can be called up to 10 times to lock N_PROG_LOCKS (ie. 10) programs
 * 	into memory.
 * Parameters:
 *	program	- pathname of program to be locked
 * Return:
 *	 0	- lock successful
 *	-1	- lock failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_lock_prog(char * program)
{
	static	caddr_t	*pa = NULL;
	static	int	locked_programs = 0;
	struct	stat	sb;
	int			program_fd = -1;
	int			i;

	if (get_install_debug() == 1)
		return (0);

	if (pa == NULL) {
		if ((pa = (char **) malloc(N_PROG_LOCKS * sizeof (caddr_t)))
				== NULL) {
			return (-1);
		}
		for (i = 0; i < N_PROG_LOCKS; i++)
				pa[i] = NULL;
	}

	if (locked_programs < N_PROG_LOCKS) {
		/* Open file descriptor to file program. */
		if ((program_fd = open(program, O_RDONLY, 0)) == -1)
			return (-1);

		if (fstat(program_fd, &sb) < 0)
			return (-1);

		/* Map it in and lock the pages into memory */
		if ((pa[locked_programs] = mmap((caddr_t) 0, sb.st_size,
				PROT_READ, MAP_SHARED, program_fd,
				(off_t) 0)) == (caddr_t) -1)
			return (-1);

		(void) close(program_fd);
		if (mlock(pa[locked_programs], sb.st_size) == -1)
			return (-1);

		locked_programs++;

	}

	return (0);
}

/*
 * _arch_cmp()
 *	Compare the architecture of the current machine to that
 *	listed in the package to determine if the specified
 *	package applies to this machine
 * Parameters:
 *	arch	- m_arch field listed in the package (e.g. sparc)
 *	impl	- machine hardware implementation
 *	inst	- instruction set architecture
 * Return:
 *	TRUE	- the package does apply to this system
 *	FALSE	- the package does not apply to this system
 * Status:
 *	semi-private (internal library use only)
 */
int
_arch_cmp(char *arch, char *impl, char *inst)
{
	char   inst_impl[MAXNAMELEN];
	char   inst_all[MAXNAMELEN];
	char   *cp;

	(void) sprintf(inst_impl, "%s.%s", inst, impl);
	(void) sprintf(inst_all, "%s.all", inst);

	/* truncate arch to contain a single architecture */
	if (cp = strchr(arch, ','))
		*cp = '\0';

	/* exact match of architectures */
	if (strcmp(arch, inst_impl) == 0) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* all.all or just all */
	if ((strcmp(arch, "all.all") == 0) ||
			(strcmp(arch, "all") == 0)) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* arch.all or just arch */
	if ((strcmp(arch, inst_all) == 0) ||
			(strcmp(arch, inst) == 0)) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* no architecture specified, all assumed */
	if (strcmp(arch, " ") == 0) {
		if (cp)
			*cp = ',';
		return (TRUE);
	}

	/* Put architecture string back to original value */
	if (cp)
		*cp++ = ',';

	return (FALSE);
}

/*
 * _copy_file()
 *	Copy a file from one location in the file system to another
 *	location using the 'cp' command.
 * Parameters:
 *	dst	- copy destination file
 *	src	- copy source file
 * Return:
 *	NOERR	- copy succeeded
 *	ERROR	- copy failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_copy_file(char *dst, char *src)
{
	char	cmd[MAXPATHLEN];

	if (get_install_debug() == 0) {
		(void) sprintf(cmd,
			"/usr/bin/cp %s %s >/dev/null 2>&1", src, dst);
		if (system(cmd) != 0) {
			write_notice(ERRMSG, MSG_COPY_FAILED, src, dst);
			return (ERROR);
		}
	}
	return (NOERR);
}
