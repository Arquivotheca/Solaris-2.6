#ifndef lint
#ident   "@(#)mount.c 1.10 95/02/10 SMI"
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
#include "sw_lib.h"

#include <sys/stat.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <sys/wait.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>

/* Local Statics and Constants */

static int	started_nfsd;	/* so we can kill any nfsd's we start */
static int	started_mountd;	/* so we can kill any mountd's we start */
static int	shared;		/* so we can unshare spool dir when done */
static int	lockfile;	/* fd of mnttab lock file */
static int	mnttab_locked;	/* lock status */

#define	MNTTAB_NEW	"/etc/mnttab.temp"
#define	LOCKFILE	"/etc/.mnt.lock"
#define	SHARECMD	"/usr/sbin/share"
#define	UNSHARECMD	"/usr/sbin/unshare"

/* Public Function Prototypes */

extern int      swi_mount_fs(char *, char *, char *);
extern int      swi_umount_fs(char *);
extern int      swi_share_fs(char *);
extern int      swi_unshare_fs(char *);

/* Local Function Prototypes */

static int addtomnttab(struct mnttab *);
static int rmfrommnttab(char *);
static int lockmnttab(void);
static void unlockmnttab(void);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * mount_fs()
 *	Build a mnttab entry and mount the filesystem.  If file system type is
 *	unspecified, we go through all the possible filesystem types to be
 *	mounted until we find one that works or exhaust the possibilities.
 *	Note that this mode does not allow you to check for specific error
 *	conditions, making it nearly useless.
 * Parameters:
 *	special - block special node or directory to be mounted
 *	mountp	- mount point
 *	fstype	- [optional] file system type
 * Return:
 *	 0	- success
 *	-1	- failure
 * Status:
 *	public
 */
int
swi_mount_fs(char * special, char * mountp, char * fstype)
{
	struct mnttab mnt;		/* monttab entry struct */
	char	tbuf[20];		/* buffer for time string */
	char	buf[FSTYPSZ];		/* a buffer for the filesystem type */
	int	flags;			/* flags for the mount */
	int	fstypes;		/* number of filesystem types */
	int	i, m;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mount_fs");
#endif

	/*
	 * set flags for both mount(2) call
	 * and putting in the mount table
	 */
	flags = MS_FSS | MS_RDONLY;
	mnt.mnt_mntopts = MNTOPT_RO;

	/*
	 * set device, mountpoint and time for mnttab
	 */
	mnt.mnt_special = special;
	mnt.mnt_mountp = mountp;
	(void) sprintf(tbuf, "%ld", time(0L));
	mnt.mnt_time = tbuf;

	if (fstype == (char *)0) {
		/*
		 * get the number of filesystem types
		 * configured into this system
		 */
		fstypes = sysfs(GETNFSTYP);

		/*
		 * run through all possible file system types
		 * until the mount works or we're out of types
		 */
		for (i = 0; i < fstypes; i++) {
			m = mount(special, mountp, flags, i);
			if (m == 0)
				break;
		}
		/*
		 * get the string for the filesystem
		 * type and put it in mnttab
		 */
		(void) sysfs(GETFSTYP, i, buf);
		fstype = buf;
	} else {
		/*
		 * Use argument file system type
		 */
		i = sysfs(GETFSIND, fstype);
		if (i < 0) {
			return(ERR_FSTYPE);
		}
		m = mount(special, mountp, flags, i);
	}

	if (m < 0)
		return (-1);

	mnt.mnt_fstype = fstype;

	/*
	 * add the new entry to the mount table
	 */
	if(addtomnttab(&mnt) != SUCCESS)
		return(ERR_LOCKFILE);
	return (SUCCESS);
}

/*
 * umount_fs()
 * Parameters:
 *	mountp	-
 * Return:
 *	0		- success
 *	ERR_LOCKFILE
 * Status:
 *	public
 */
int
swi_umount_fs(char * mountp)
{
	int	status;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("umount_fs");
#endif

	status = umount(mountp);
	if (status == 0 || errno == EINVAL)
		if(rmfrommnttab(mountp) != SUCCESS)    /* remove from mnttab */
			return(ERR_LOCKFILE);
	return (status);
}

/*
 * share_fs()
 *	Export a file system ro to the world TODO: export to only a set 
 *	of clients
 *		1) Find out if file system is already shared;
 *		   if it is, we're done. (TODO)
 *		2) Share the file system (system(share)).
 *		   Keep track of the fact we started initiated the sharing.
 *		3) Determine if nfsd is running.  If not, start some.
 *		   Keep track of the fact we started nfsd.
 *		4) Determine if mountd is running.  If not, start it.
 *		   Keep track of the fact we started mountd.
 * Parameters:
 *	fsname	-
 * Return:
 *	-1 	couldn't share file system
 *	 0 	file system already shared (caller must generate next event)
 *	 1 	file system now shared, next event generated when share
 *			command exits
 * Status:
 *	public
 */
int
swi_share_fs(char * fsname)
{
	char	cmd[BUFSIZ];
	int	status;
	pid_t	pid;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("share_fs");
#endif

	if (shared)
		return (0);

	(void) sprintf(cmd,
	    "%s -F nfs -o ro -d \"SWM[%lu]:  pkg spool directory\" %s %s",
		SHARECMD, (u_long)getpid(), fsname, fsname);
	status = system(cmd);
	if (status < 0) 
		return (status);
	shared = 1;

	if (proc_walk(proc_running, "nfsd") == 0) {
		pid = fork();
		if (pid > 0)
			started_nfsd = 1;
		else if (pid < 0)
			return(ESRCH);
		else {
			(void) execl("/usr/lib/nfs/nfsd", "nfsd", "-a", "8", 0);
			exit(-1);
		}
	}

	if (proc_walk(proc_running, "mountd") == 0) {
		pid = fork();
		if (pid > 0)
			started_mountd = 1;
		else if (pid < 0)
			return(ESRCH);
		else {
			(void) execl("/usr/lib/nfs/mountd", "mountd", 0);
			exit(-1);
		}
	}

	return (shared);
}

/*
 * unshare_fs()
 *	Make a shared file system private to our machine again.
 *
 * 	1) If we didn't share the file system, we're done.
 * 	2) Unshare the file system.
 * 	3) If we started nfsd, kill all nfsd's.
 * 	4) If we started mount, kill mountd.
 * Parameters:
 *	fsname	-
 * Return:
 * Status:
 *	public
 */
int
swi_unshare_fs(char * fsname)
{
	char	cmd[BUFSIZ];
	int	status;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("unshare_fs");
#endif

	if (!shared || fsname == (char *)0)
		return (0);

	(void) sprintf(cmd, "%s -F nfs %s", UNSHARECMD, fsname);
	status = system(cmd);
	if (status < 0) {
		return (ERR_SHARE);
	} else if (WEXITSTATUS(status) != 0) {
		return (ERR_SHARE);
	}
	shared = 0;

	if (started_nfsd) {
		(void) proc_walk(proc_kill, "nfsd");
		started_nfsd = 0;
	}

	if (started_mountd) {
		(void) proc_walk(proc_kill, "mountd");
		started_mountd = 0;
	}
	return (SUCCESS);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * rmfrommnttab()
 *
 * Parameters:
 *	mountp	-
 * Return:
 *	SUCCESS		-
 *	ERR_LOCKFILE	-
 * Status:
 *	private
 */
static int
rmfrommnttab(char * mountp)
{
	FILE *mounted, *newmnt;		/* old and new mnttab files */
	struct mnttab mnt;		/* struct containing mntab entry */
	int	errs = 0;

	if(lockmnttab() != SUCCESS)	/* lock old mnttab */
		return(ERR_LOCKFILE);
	/*
	 * Open old mnttab for reading
	 */
	mounted = fopen(MNTTAB, "r");
	if (mounted != (FILE *)0) {
		/*
		 * Open new mnttab for writing
		 */
		newmnt = fopen(MNTTAB_NEW, "w");
		if (newmnt != (FILE *)0) {
			while (getmntent(mounted, &mnt) != -1) {
				/*
				 * If not the entry we're deleting,
				 * append the entry to the new mnttab.
				 */
				if (strcmp(mountp, mnt.mnt_mountp) != 0)
					if (putmntent(newmnt, &mnt) == EOF)
						errs = 1;
			}
			if (fclose(newmnt) == EOF)
				errs = 1;
		}
		(void) fclose(mounted);
		if (errs == 0)
			(void) rename(MNTTAB_NEW, MNTTAB);
	}
	unlockmnttab();			/* unlock mnttab */
	return(SUCCESS);
}

/*
 * addtomnttab()
 *	Add the specified mnttab entry to the system mnttab.
 * Parameters:
 *	mnt	- mounttab entry
 * Return:
 *	SUCCESS		-
 *	ERR_LOCKFILE	-
 * Status:
 *	private
 */
static int
addtomnttab(struct mnttab * mnt)
{
	FILE *mnted;

	if(lockmnttab() != SUCCESS)		/* lock old mnttab */
		return(ERR_LOCKFILE);

	/* Open mount tab. Failure isn't nice, but we can still work */
	mnted = fopen(MNTTAB, "a+");
	if (mnted != (FILE *)0) {
		/* append the new entry to mnttab */
		(void) putmntent(mnted, mnt);
		(void) fclose(mnted);
	}
	unlockmnttab();
	return(SUCCESS);
}

/*
 * lockmnttab()
 * Parameters:
 *	void
 * Return:
 *	ERR_LOCKFILE	-
 *	SUCCESS		-
 * Status:
 *	private
 */
static int
lockmnttab(void)
{
	if (mnttab_locked)
		return(ERR_LOCKFILE);

	lockfile = creat(LOCKFILE, 0600);
	if ((lockfile < 0) || (lockf(lockfile, F_LOCK, 0L) < 0))
		return(ERR_LOCKFILE);
	else
		mnttab_locked = 1;	/* mark locked */
	return(SUCCESS);
}

/*
 * unlockmnttab()
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
static void
unlockmnttab(void)
{
	if (mnttab_locked) {
		(void) lockf(lockfile, F_ULOCK, 0L);
		mnttab_locked = 0;	/* mark unlocked */
	}
	return;
}

