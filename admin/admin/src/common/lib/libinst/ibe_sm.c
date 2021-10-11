#ifndef lint
#pragma ident "@(#)ibe_sm.c 1.94 95/02/17"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include <sys/mntent.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>

/* Globals and Externals */

extern int	progress_cleanup(void);

/* Public Function Prototypes */

int		ibe_sm(Module *, Disk_t *, Dfs *);

/* Library Function Prototypes */

/* Local Function Prototypes */

static void		_ibe_cleanup(void);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * ibe_sm()
 * 	This is the 'back end' state machine.  It does the actual work
 * 	behind installing a system.  By the time this function is entered
 *	several checks have been made to validate the system. If there
 *	are any errors, the process exits to the shell prompt.
 * Parameters:
 *	prod	- pointer to product module
 *	dlist	- pointer to head of disk linked list
 *	cfs	- pointer to head of remote file system linked list
 * Return:
 *	NOERR	- successful
 *	ERROR	- installation failure
 * Status:
 *	public
 */
int
ibe_sm(Module *prod, Disk_t *dlist, Dfs *cfs)
{
	Vfsent		*vlist = NULL;
	Vfsent		*vp;
	MachineType	mt = get_machinetype();
	TransList	*trans;
	char		buf[MAXPATHLEN];
	struct vfstab	*vfsp;

	/*
	 * Initialize the base directory from which targetted
	 * file systems are accessed. For installs which occur
	 * directly on the system being installed, this is "".
	 * For systems which are installed from a net boot image,
	 * this is "/a".
	 */
	if (mt == MT_CCLIENT)
		set_rootdir("");
	else
		set_rootdir("/a");

	/* print message indicating system preparation beginning */
	write_status(SCR, LEVEL0, MSG0_SYSTEM_PREP_BEGIN);

	/*
	 * cleanup from possible previous install attempts for
	 * all install processes which could be restarted
	 */
	if (INDIRECT_INSTALL) {
		if (reset_system_state() < 0) {
			write_notice(ERRMSG, MSG0_STATE_RESET_FAILED);
			return (ERROR);
		}
	}

	/*
	 * create a list of local and remote mount points
	 */
	if (_create_mount_list(dlist, cfs, &vlist) == ERROR) {
		write_notice(ERRMSG, MSG0_MNTPNT_LIST_FAILED);
		return (ERROR);
	}

	/*
	 * update the F-disk and VTOC on all selected disks
	 * according to the disk list configuration; start
	 * swapping to defined disk swap slices
	 */
	if (_setup_disks(dlist, vlist) == ERROR)
		return (ERROR);

	/*
	 * create mount points on the target system according
	 * to the mount list; note that the target file systems
	 * may be offset of a base directory if the installation
	 * is indirect.
	 */
	if (_mount_filesys_all(vlist) != NOERR)
		return (ERROR);

	/*
	 * lock critical applications in memory for performance
	 */
	(void) _lock_prog("/usr/sbin/pkgadd");
	(void) _lock_prog("/usr/sadm/install/bin/pkginstall");
	(void) _lock_prog("/usr/bin/cpio");

	/*
	 * create the packaging admin file to be used for all
	 * package adds (software related task). Also create the transfer
	 * file list, used to move necessary files from /tmp/root to /a
	 */
	if (_setup_software(prod, &trans) == ERROR) {
		write_notice(ERRMSG, MSG0_PKG_INSTALL_TOTALFAIL);
		return (ERROR);
	}

	write_status(LOGSCR, LEVEL0, MSG0_FILES_CUSTOMIZE);

	/*
	 * write out the 'etc/vfstab' file to the appropriate location
	 * This is a writeable system file (affects location wrt indirect
	 * installs)
	 */
	if (_setup_vfstab(vlist) == ERROR) {
		write_notice(ERRMSG, MSG0_VFSTAB_CREATE_FAILED);
		return (ERROR);
	}

	/*
	 * write out the vfstab.unselected file to the appropriate
	 * location (if unselected disks with file systems exist)
	 */
	if (_setup_vfstab_unselect() == ERROR) {
		write_notice(ERRMSG, MSG0_VFSTAB_UNSELECTED_FAILED);
		return (ERROR);
	}

	/*
	 * set up the etc/hosts file. This is a writeable system
	 * file (affects location wrt indirect installs)
	 */
	if (_setup_etc_hosts(cfs) == ERROR) {
		write_notice(ERRMSG, MSG0_HOST_CREATE_FAILED);
		return (ERROR);
	}

	/*
	 * initialize serial number if there isn't one supported on the
	 * target architecture
	 */
	if (_setup_hostid() == ERROR) {
		write_notice(ERRMSG, MSG0_SERIAL_VALIDATE_FAILED);
		return (ERROR);
	}

	/*
	 * Copy information from tmp/root to real root, using the transfer
	 * file list. From this point on, all modifications to the system
	 * will have to write directly to the /a/<*> directories. (if
	 * applicable)
	 */
	if (_setup_tmp_root(&trans) == ERROR) {
		write_notice(ERRMSG, MSG0_INSTALL_CONFIG_FAILED);
		return (ERROR);
	}

	/*
	 * setup /dev, /devices, and /reconfigure
	 */
	if (_setup_devices() == ERROR) {
		write_notice(ERRMSG, MSG0_SYS_DEVICES_FAILED);
		return (ERROR);
	}

	/*
	 * set up boot block
	 */
	if (_setup_bootblock() != NOERR) {
		write_notice(ERRMSG, MSG0_BOOT_BLOCK_FAILED);
		return (ERROR);
	}

	/*
	 * 3rd party driver installation
	 *
	 * NOTE:	this needs to take 'bdir' as an argument
	 *		if it intends to write directly into the
	 *		installed image.
	 */
	if (access("/tmp/diskette_rc.d/inst9.sh", X_OK) == 0) {
		write_status(LOGSCR, LEVEL0, MSG0_DRIVER_INSTALL);
		(void) system("/sbin/sh /tmp/diskette_rc.d/inst9.sh");
	}

	/*
	 * copy status file to /var/sadm or /tmp
	 */
	(void) _setup_install_log();

	/*
	 * wait for newfs's and fsck's to complete
	 */
	if (get_install_debug() == 0) {
		for (; system("ps -e | egrep newfs > /dev/null") == 0;
				(void) sleep(30));
		for (; system("ps -e | egrep fsck > /dev/null") == 0;
				(void) sleep(30));
	}

	/*
	 * on non-AutoClient systems, finish mounting all file systems
	 * which were not previously mounted during install; this
	 * leaves the system correctly configured for a finish script
	 */
	if (mt != MT_CCLIENT) {
		if (get_install_debug() > 0 || get_trace_level() > 1)
			write_status(SCR, LEVEL0, MSG0_MOUNTING_TARGET);

		WALK_LIST(vp, vlist) {
			vfsp = vp->entry;
			/* we are only mounting local file systems */
			if (strcmp(vfsp->vfs_fstype, MNTTYPE_UFS) != 0)
				continue;

			if (vfsp->vfs_mountp == NULL)
				continue;

			/*
			 * mount all unmounted file systems which were
			 * not previously mounted
			 */
			if (_mount_synchronous_fs(vfsp, vlist) == 1)
				continue;

			(void) sprintf(buf, "%s%s",
				get_rootdir(),
				strcmp(vfsp->vfs_mountp, ROOT) == 0 ? "" :
					vfsp->vfs_mountp);

			if (_mount_filesys_specific(buf, vfsp) != NOERR) {
				write_notice(WARNMSG,
					MSG2_FILESYS_MOUNT_FAILED,
					vfsp->vfs_mountp,
					vfsp->vfs_special);
			}
		}
	}

	/*
	 * cleanup
	 */
	_free_mount_list(&vlist);

	_ibe_cleanup();

	/* sync out the disks (precautionary) */
	sync();

	/* print message indicating system installation complete */
	write_status(SCR, LEVEL0, MSG0_SYSTEM_INSTALL_COMPLETE);

	return (NOERR);
}

static void
_ibe_cleanup(void)
{
	if (get_machinetype() != MT_CCLIENT) {
		/* NOTE: do not unmount /a due to autoinstall dependencies */
		/* cleanup the progess display */
		(void) progress_cleanup();
	}

}
