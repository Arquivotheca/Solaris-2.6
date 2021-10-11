#ifndef lint
#pragma ident "@(#)common_mount.c 1.3 96/01/03 SMI"
#endif
/*
 * Copyright (c) 1996 Sun Microsystems, Inc.
 * All Rights Reserved.
 */
/*
 * Module:	common_mount.c
 * Group:	libspmicommon
 * Description: Contains all functions used to mount, unmount, and
 *		otherwise deal with special devices containing file
 *		systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/mount.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fs/ufs_fs.h>
#include "spmicommon_lib.h"

/* public prototypes */

int		FsMount(char *, char *, char *, char *);
int		UfsRestoreName(char *, char *);
int		UfsMount(char *, char *, char *);
int		UfsUmount(char *, char *, char *);
int		DirUmountAll(char *);
int		DirUmount(char *);

/* local prototypes */

static int	DirUmountRecurse(FILE *, char *);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	FsMount
 * Description:	Mount a block special device containing a file system. If
 *		the file system type is unspecified, all possible file
 *		system types are tried until one successfully mounts or
 *		all possibilities are exhausted. If the special device name
 *		specified is a simple slice device name (e.g. c0t3d0s3), the
 *		block special device is assume to exist in /dev/dsk. Otherwise,
 *		the user may supply a fully qualified path name to the block
 *		special device.
 *
 *		NOTE: This function explicitly uses the "mount" command in
 *			order to ensure that the mnttab file is kept up-to-date.
 * Scope:	public
 * Parameters:	device	[RO, *RO]
 *			Slice device name for which the block device
 *			will be used for the mount. The device may
 *			either be specified in relative (e.g. c0t3d0s4)
 *			or absolute (e.g. /dev/dsk/c0t3d0s4) form.
 *		mntpnt	[RO, *RO]
 *			UNIX path name for the mount point directory. This
 *			will be prepended with get_rootdir() by the function
 *			before mounting.
 *		mntopt	[RO, *RO] (optional)
 *			Mount options. If NULL specified, then "mount" command
 *			defaults are used. Options should appear as they would
 *			on the mount line (e.g. "-r").
 *		fstype	[RO, *RO] (optional)
 *			File system type specifier (e.g. "ufs"). If none
 *			is specified, a mount attempt is made for each type
 *			which is defined on the system.
 * Return:	 0	- mount was successful
 *		-1	- mount failed
 */
int
FsMount(char *device, char *mntpnt, char *mntopts, char *fstype)
{
	struct stat	sbuf;
	char	cmd[MAXPATHLEN];
	char	fsname[MAXNAMELEN];
	char	disk[MAXNAMELEN];
	int	n;
	int	i;

	/* validate parameters */
	if (!is_slice_name(device) && !is_pathname(device))
		return (-1);

	if (!is_pathname(mntpnt) || (stat(mntpnt, &sbuf) < 0) ||
			((sbuf.st_mode & S_IFDIR) == 0))
		return (-1);

	/* create the block special disk device name */
	if (is_slice_name(device))
		(void) sprintf(disk, "/dev/dsk/%s", device);
	else
		(void) strcpy(disk, device);

	/*
	 * if no file system type is specified, run through all possible
	 * types until the mount works or there are no more types
	 */
	if (fstype == NULL) {
		n = sysfs(GETNFSTYP);
		for (i = 0; i < n; i++) {
			if (sysfs(GETFSTYP, i, fsname) == 0) {
				(void) sprintf(cmd,
					"mount -F %s %s %s %s >/dev/null 2>&1",
					fsname,
					mntopts == NULL ? "" : mntopts,
					disk, mntpnt);
				if (WEXITSTATUS(system(cmd)) == 0)
					return (0);
			}
		}
	} else {
		(void) sprintf(cmd, "mount -F %s %s %s %s >/dev/null 2>&1",
			fstype,
			mntopts == NULL ? "" : mntopts,
			disk, mntpnt);
		if (WEXITSTATUS(system(cmd)) == 0)
			return (0);
	}

	return (-1);
}

/*
 * Function:	UfsRestoreName
 * Description: Restore the "last-mounted-on" field in the superblock of the
 *		device referenced to the file system name specified. If the
 *		special device name specified is a simple slice device name
 *		(e.g. c0t3d0s3), the character special device is assumed to
 *		exist in /dev/rdsk. Otherwise, the user may supply a fully
 *		qualified path name to the character special device.
 * Scope:	public
 * Parameters:	device	[RO, *RO]
 *			Slice device name for which the block device
 *			will be used for the mount. The device may
 *			either be specified in relative (e.g. c0t3d0s4)
 *			or absolute (e.g. /dev/rdsk/c0t3d0s4) form.
 *		name	[RO, *RO]
 *			The non-NULL name to set the last-mounted-on value.
 * Return:	 0	- successfully restored the file system name
 *		-1	- failed to restore the file system name
 */
int
UfsRestoreName(char *device, char *name)
{
	struct stat	sbuf;
	char	  disk[MAXNAMELEN];
	int	  fd;
	int	  sblock[SBSIZE/sizeof (int)];
	struct fs *fsp = (struct fs *) sblock;

	/* validate parameters */
	if (!is_slice_name(device) && !is_pathname(device))
		return (-1);

	if (!is_pathname(name))
		return (-1);

	if (is_slice_name(device))
		(void) sprintf(disk, "/dev/rdsk/%s", device);
	else
		(void) strcpy(disk, device);

	/* make sure the device is a character special device */
	if ((stat(disk, &sbuf) < 0) ||
			((sbuf.st_mode & S_IFCHR) == 0) ||
			((fd = open(disk, O_RDWR)) < 0))
		return (-1);

	if (lseek(fd, SBOFF, SEEK_SET) < 0 ||
			read(fd, fsp, sizeof (sblock)) < 0) {
		(void) close(fd);
		return (-1);
	}

	(void) strcpy(fsp->fs_fsmnt, name);
	if (lseek(fd, SBOFF, SEEK_SET) < 0) {
		(void) close(fd);
		return (-1);
	}

	(void) write(fd, fsp, sizeof (sblock));
	(void) close(fd);
	return (0);
}

/*
 * Function:	UfsMount
 * Description:	Mount a block special device containing a UFS file system. If
 *		the special device name specified is a simple slice device
 *		name (e.g. c0t3d0s3), the block special device is assume
 *		to exist in /dev/dsk. Otherwise, the user may supply a
 *		fully qualified path name to the block special device.
 *
 *		NOTE: This function explicitly uses the "mount" command in
 *			order to ensure that the mnttab file is kept up-to-date.
 * Scope:	public
 * Parameters:	device	[RO, *RO]
 *			Slice device name for which the block device
 *			will be used for the mount. The device may
 *			either be specified in relative (e.g. c0t3d0s4)
 *			or absolute (e.g. /dev/dsk/c0t3d0s4) form.
 *		mntpnt	[RO, *RO]
 *			UNIX path name for the mount point directory. This
 *			will be prepended with get_rootdir() by the function
 *			before mounting.
 *		mntopt	[RO, *RO] (optional)
 *			Mount options. If NULL specified, then "mount" command
 *			defaults are used. Options should appear as they would
 *			on the mount line (e.g. "-r").
 * Return:	 0	- the mount completed successfully
 *		-1	- the mount failed
 */
int
UfsMount(char *device, char *mntpnt, char *mntopt)
{
	if (FsMount(device, mntpnt, mntopt, "ufs") < 0)
		return (-1);
	else
		return (0);
}

/*
 * Function:	UfsUmount
 * Description:	Unmount a block special device containing a UFS file system,
 *		with the option to set the "last-mounted-on" field in the
 *		super-block to an explicit value. If the special device name
 *		specified is a simple slice device name (e.g. c0t3d0s3), the
 *		block special device is assume to exist in /dev/dsk. Otherwise,
 *		the user may supply a fully qualified path name to the block
 *		special device. If the user provides a fully qualified
 *		block special device, and a mount point name for restoration,
 *		the user must also provide a fully qualified character
 *		special device. This is the only time a non-NULL value
 *		should be specified for the character special device.
 *
 *		NOTE: This function explicitly uses the "unmount" command in
 *			order to ensure that the mnttab file is kept up-to-date.
 * Scope:	public
 * Parameters:	bdevice	[RO, *RO]
 *			Slice device name for which the block device
 *			will be used for the mount. The device may
 *			either be specified in relative (e.g. c0t3d0s4)
 *			or absolute (e.g. /dev/dsk/c0t3d0s4) form.
 *		mntpnt	[RO, *RO] (optional)
 *			UNIX path name for the mount point directory.
 *			NULL if no last mounted file system name
 *			restoration if required.
 *		cdevice	[RO, *RO] (optional)
 *			Absolute path for the character device associated
 *			with the block device being unmounted. This field
 *			must only be specified when the block device is
 *			specified as an absolute path, and a non-NULL mount
 *			point restoration value is provided. NULL, otherwise.
 * Return:	 0	- the mount completed successfully
 *		-1	- the mount failed
 */
int
UfsUmount(char *bdevice, char *mntpnt, char *cdevice)
{
	char	buf[MAXPATHLEN];

	/* validate parameters */
	if (is_slice_name(bdevice)) {
		if (cdevice != NULL)
			return (-1);
	} else if (is_pathname(bdevice)) {
		if (mntpnt != NULL && !is_pathname(cdevice))
			return (-1);
	} else {
		return (-1);
	}

	if (is_slice_name(bdevice))
		(void) sprintf(buf, "umount /dev/dsk/%s >/dev/null 2>&1",
			bdevice);
	else
		(void) sprintf(buf, "umount %s >/dev/null 2>&1", bdevice);

	if (WEXITSTATUS(system(buf)) != 0)
		return (-1);

	/* only restore the mount point name if one is specified */
	if (mntpnt != NULL) {
		if (is_slice_name(bdevice))
			(void) UfsRestoreName(bdevice, mntpnt);
		else
			(void) UfsRestoreName(cdevice, mntpnt);
	}

	return (0);
}

/*
 * Function:	DirUmountAll
 * Description: Unmount all file systems mounted under a specified directory.
 *		This routine assumes that all mounted file systems are logged
 *		in /etc/mnttab and are unmounted in the reverse order in which
 *		they appear in /etc/mnttab.
 *
 *		NOTE: This function explicitly uses the "unmount" command in
 *			order to ensure that the mnttab file is kept up-to-date.
 * Scope:	public
 * Parameters:	mntpnt	[RO, *RO]
 *			Non-NULL pointer to name of directory to be unmounted.
 * Return:	 0	- successfull
 *		-1	- unmount failed; see errno for reason
 */
int
DirUmountAll(char *mntpnt)
{
	struct stat	sbuf;
	FILE *		fp;
	int		retval = 0;

	/* validate parameters */
	if (!is_pathname(mntpnt) || (stat(mntpnt, &sbuf) < 0) ||
			((sbuf.st_mode & S_IFDIR) == 0))
		return (-1);

	/*
	 * open the mnttab and recursively begin unmounting file systems
	 * which are ultimately mounted on the specified directory
	 */
	if ((fp = fopen(MNTTAB, "r")) == NULL) {
		retval = -1;
	} else {
		retval = DirUmountRecurse(fp, mntpnt);
		(void) fclose(fp);
	}

	return (retval);
}

/*
 * Function:	DirUmount
 * Description:	Unmount the file system mounted on the specified directory.
 *
 *		NOTE: This function explicitly uses the "unmount" command in
 *			order to ensure that the mnttab file is kept up-to-date.
 * Scope:	public
 * Parameters:	mntpnt	[RO, *RO]
 *			Non-NULL name of directory to unmount.
 * Return:	 0	- the directory was successfully unmounted
 *		-1	- the directory unmount attempt failed
 */
int
DirUmount(char *mntpnt)
{
	struct stat	sbuf;
	char		buf[MAXPATHLEN];

	/* validate parameter */
	if (!is_pathname(mntpnt) || (stat(mntpnt, &sbuf) < 0) ||
			((sbuf.st_mode & S_IFDIR) == 0))
		return (-1);

	(void) sprintf(buf, "umount %s >/dev/null 2>&1", mntpnt);
	if (WEXITSTATUS(system(buf)) != 0)
		return (-1);

	return (0);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	DirUmountRecurse
 * Description:	Recursively process the mnttab unmounting file systems
 *		which are children of the specified file system name.
 * Scope:	private
 * Parameters:	fp	[RO, *RO]
 *			FILE pointer to /etc/mnttab file.
 *		name	[RO, *RO]
 *			Non-NULL base name of directory being unmounted.
 * Return:	 0	- processing successful
 *		-1	- processing failed
 */
static int
DirUmountRecurse(FILE *fp, char *name)
{
	struct mnttab	ment;
	char		buf[MAXPATHLEN];

	/* validate parameters */
	if (fp == NULL || name == NULL)
		return (-1);

	(void) sprintf(buf, "%s/", name);
	while (getmntent(fp, &ment) == 0) {
		if (strneq(ment.mnt_mountp, buf, strlen(name)) ||
				streq(ment.mnt_mountp, name)) {
			if (DirUmountRecurse(fp, name) < 0)
				return (-1);

			if (DirUmount(ment.mnt_mountp) < 0)
				return (-1);

			break;
		}
	}

	return (0);
}
