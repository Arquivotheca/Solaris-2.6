/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#ifndef lint
#ident	"@(#)mount.c 1.6 93/04/12"
#endif

#include "defs.h"
#include "ui.h"
#include "host.h"
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>

/*
 * Remotely mount a file system.  Uses
 * /tmp_mnt/nfs/<host>.swm_pkg as the
 * remote mount point.
 *
 * 1) See if it's already mounted, if so, return
 * 2) Create the mount point
 * 3) Mount the file system
 */
rmount_fs(fsname)
	char	*fsname;
{
	Hostlist *hlp;
	static char	dircmd[MAXHOSTNAMELEN+32];
	static char	mntcmd[BUFSIZ];
	static char	umntcmd[BUFSIZ];
	static char	rmntdir[MAXPATHLEN];
	static int doinit = 1;

	if (doinit) {
		(void) sprintf(rmntdir,
			"%s/%s.swm_pkg", get_rmntdir(), thishost);
		(void) sprintf(umntcmd,
			"%s/%s.swm_pkg 2>&1", get_rmntdir(), thishost);
		(void) sprintf(dircmd,
		    "/bin/rmdir %s >/dev/null 2>&1; /bin/mkdir -p %s",
			rmntdir, rmntdir);
		(void) sprintf(mntcmd, "/sbin/mount -F nfs -o ro %s:%s %s",
			thishost, fsname, rmntdir);
		(void) sprintf(umntcmd, "/sbin/umount %s >/dev/null 2>&1",
			rmntdir);
		doinit = 0;
	}

	for (hlp = hostlist.h_next; hlp != &hostlist; hlp = hlp->h_next) {
		/*
		 * Mount spool file system if host is selected,
		 * has its own file system(s), and hasn't
		 * mounted the spool f/s yet.
		 */
		if ((hlp->h_status & HOST_SELECTED) &&
		    (hlp->h_status & HOST_LOCAL) == 0 &&
		    (hlp->h_status & HOST_MOUNTED) == 0) {
			(void) fprintf(stderr, gettext(
			    "Mounting package spool directory on host `%s'\n"),
				hlp->h_name);
#ifndef DEMO
			(void) host_run_cmd(hlp, umntcmd);

			if (host_run_cmd(hlp, dircmd)) {
				(void) fprintf(stderr, gettext(
	"Cannot install software on host `%s' because the package spool\n\
directory mount point `%s' cannot be created.\n"),
					hlp->h_name, rmntdir);
				continue;
			}

			if (host_run_cmd(hlp, mntcmd)) {
				(void) fprintf(stderr, gettext(
		"Cannot install software on host `%s' because the\n\
package spool directory `%s' cannot be mounted.\n"),
					hlp->h_name, rmntdir);
				continue;
			}
#endif
			hlp->h_status |= HOST_MOUNTED;
		}
	}
	return (SUCCESS);
}

/*
 * Remotely unmount a file system.
 *
 * 1) Unmount the file system
 * 2) Remove the remote mount point
 */
rumount_fs(fsname)
	char	*fsname;
{
	Hostlist *hlp;
	char	dircmd[MAXHOSTNAMELEN+32];
	char	mntcmd[BUFSIZ];
	char	rmntdir[MAXPATHLEN];

	if (fsname == (char *)0)
		return (SUCCESS);

	(void) sprintf(rmntdir, "%s/%s.swm_pkg", get_rmntdir(), thishost);
	(void) sprintf(dircmd, "/bin/rmdir %s 2>&1", rmntdir);
	(void) sprintf(mntcmd, "/sbin/umount %s 2>&1", rmntdir);

	for (hlp = hostlist.h_next; hlp != &hostlist; hlp = hlp->h_next) {
		/*
		 * Unmount spool file system if host has it
		 * mounted and has its own file system(s).
		 */
		if ((hlp->h_status & HOST_MOUNTED) &&
		    (hlp->h_status & HOST_LOCAL) == 0) {
			(void) fprintf(stderr, gettext(
		    "Unmounting package spool directory on host `%s'\n"),
				hlp->h_name);
#ifndef DEMO
			if (host_run_cmd(hlp, mntcmd)) {
				(void) fprintf(stderr, gettext(
		    "Warning:  cannot unmount package spool directory\n\
`%s' on host `%s'.\n"),
					rmntdir, hlp->h_name);
				continue;
			}
			if (host_run_cmd(hlp, dircmd)) {
				(void) fprintf(stderr, gettext(
		"Warning:  cannot remove package spool directory mount point\n\
`%s' on host `%s'.\n"),
					rmntdir, hlp->h_name);
				continue;
			}
#endif
			hlp->h_status &= ~HOST_MOUNTED;
		}
	}
	return (SUCCESS);
}
