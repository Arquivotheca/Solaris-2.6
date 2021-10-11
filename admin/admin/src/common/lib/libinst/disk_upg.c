#ifndef lint
#pragma ident   "@(#)disk_upg.c 1.39 95/07/20"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#include <sys/fs/ufs_fs.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <fcntl.h>

/*
 * Constants defining the location of the INST_RELEASE file
 * in the old and new /var/sadm locations (PSARC/1994/331)
 */
#define	OLD_RELEASE	"/a/var/sadm/softinfo/INST_RELEASE"
#define	OLD_VAR_RELEASE	"/a/sadm/softinfo/INST_RELEASE"

#define	NEW_RELEASE	"/a/var/sadm/system/admin/INST_RELEASE"
#define	NEW_VAR_RELEASE	"/a/sadm/system/admin/INST_RELEASE"

/* Public Function Prototypes */

Disk_t *	upgradeable_disks(void);
int		do_mounts_and_swap(Disk_t *);

/* Library Function Prototypes */

/* Local Function Prototypes */

static int 	_disk_has_release(Disk_t *);
static int 	_check_release_file(char *);
static void 	_restore_fs_name(char *, char *);
static char *	_check_var(void);

/* ******************************************************************** */
/*			PUBLIC API FUNCTIONS				*/
/* ******************************************************************** */

/*
 * upgradeable_disks()
 *	Search through the disk list (as assembled by either load_disk_list())
 *	or build_disk_list() and copy those disks which have a valid
 *	./sadm/softinfo/INST_RELEASE or ./var/sadm/softinfo/INST_RELEASE file.
 *	Disks which qualify have a VERSION:
 *
 *	(1) later then 2.0 on sparc
 *	(2) later than 2.3 on i386, except for those with release 2.4 or 2.5
 *		and REV=100 (this is the Solaris 2.4 Core release, which is not
 *		upgradeable to 2.5.  If it is decided that Solaris 2.4 Core
 *		will be upgradeable to 2.6, however, this code will need to
 *		be changed for 2.6).  
 *	(3) all releases are upgradeable for ppc
 *
 *	Duplicate the disk to create a separate disk list, the head of which
 *	is returned.
 *
 *	This routine searched the existing slice mount points of all drives
 *	in the current disk chain which are in an "okay" state and which
 *	have a "legal" sdisk configuration.
 *
 *	NOTE:	the list returned by this function may have the
 *		same disk names as those found in the main disk
 *		chain as created by build_disk_list() or
 *		load_disk_list().
 * Parameters:
 *	none
 * Return:
 *	Disk_t *  - pointer to list of upgradeable disks
 *		    (NULL if no upgradeable disks found)
 * Status:
 *	public
 */
Disk_t *
upgradeable_disks(void)
{
	Disk_t	*head = NULL;
	Disk_t	*dp;
	Disk_t	*tmp;
	Disk_t	*tp;

	if (umount_slash_a() < 0)
		return (NULL);

	WALK_DISK_LIST(dp) {
		if (disk_not_okay(dp) ||
				sdisk_geom_null(dp) ||
				sdisk_not_legal(dp))
			continue;

		if (_disk_has_release(dp) == 0)
			continue;

		if ((tmp = _alloc_disk(disk_name(dp))) == NULL)
			continue;

		(void) duplicate_disk(tmp, dp);

		/* add new disk to the upgrade list */
		if (head == NULL)
			head = tmp;
		else {
			for (tp = head; next_disk(tp); tp = next_disk(tp));
			_set_next_disk(tp, tmp);
		}

		/* null terminate the list */
		tmp->next = NULL;
	}

	return (head);
}

/*
 * do_mounts_and_swap()
 *	Takes a pointer to a disk structure, which is the disk to be upgraded.
 *	Nothing is mounted when this funciton is called.  First, mount the
 *	root. Assume it's slice 0. Then find the /etc/vfstab. Mount everything
 *	in the vfstab.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	Return values of mount_and_add_swap()
 * Status:
 *	public
 */
int
do_mounts_and_swap(Disk_t *dp)
{
	char	diskname[16];

	(void) sprintf(diskname, "%ss0", disk_name(dp));
	return (mount_and_add_swap(diskname));
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * _disk_has_release()
 *	Search through the slice mount point names in the existing
 *	slices and check to see if there is a valid INST_RELEASE file.
 *	Only '/' mountpoints are searched. vfstab entries for '/var'
 *	are also checked (even if they are on other drives). The
 *	purpose is to find root disks which booted up into an OS
 *	which is upgradeable.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a non-NULL S-disk
 *		  geometry pointer
 *		  (state:  okay, legal)
 * Return:
 *	1	- the disk does have a valid INST_RELEASE file
 *	0	- there is no valid INST_RELEASE file on this disk
 * Status:
 *	private
 */
static int
_disk_has_release(Disk_t * dp)
{
	char	root[32], var[32];
	char	cmd[MAXNAMELEN];
	char	*pdev;
	int	found = 0;
	int	s;
	char	*cp;

	/*
	 * by default all disks which have '/' are considered
	 * upgradeable
	 */
	if (get_install_debug() == 1)
		return (1);

	WALK_SLICES(s) {
		if (found != 0)
			break;

		/* we're only interest in "/" */
		if (strcmp(orig_slice_mntpnt(dp, s), ROOT) != 0)
			continue;

		(void) sprintf(root, "%ss%d", disk_name(dp), s);
		(void) sprintf(cmd, "/sbin/mount -r /dev/dsk/%s /a", root);

		if (system(cmd) == 0) {
			if (_check_release_file(NEW_RELEASE) == 1)
				found = 1;
			else if (_check_release_file(OLD_RELEASE) == 1)
				found = 1;
			else if ((pdev = _check_var()) != NULL) {
				(void) system("/sbin/umount /a");
				_restore_fs_name(root, ROOT);
				(void) sprintf(cmd, "/sbin/mount -r %s /a",
					pdev);
				if (system(cmd) == 0) {
					if (_check_release_file(
							NEW_VAR_RELEASE) == 1 ||
						    _check_release_file(
							OLD_VAR_RELEASE) == 1) {
						found = 1;
					}

					(void) system("/sbin/umount /a");
					cp = strrchr(pdev, '/');
					(void) strcpy(var, ++cp);
					_restore_fs_name(var, VAR);
				}
			} else {
				(void) system("/sbin/umount /a");
				_restore_fs_name(root, ROOT);
			}
		}

		(void) umount_slash_a();
	}

	return (found);
}

/*
 * _check_release_file()
 * 	Look for the lines "OS=Solaris", VERSION=", and > "2.0" for SPARC
 *	and > 2.3 on Intel on the VERSION line in the INST_RELEASE file.
 *	2.0 is the only non-upgradeable 2.x version.
 * Parameters:
 *	file	- name of file containing the pathname of interest
 * Return:
 *	1	- this is a usable (valid) release file for upgrade
 *	0	- cannot use this file for upgrade (or file does not
 *		  exist)
 * Status:
 *	private
 */
static int
_check_release_file(char * file)
{
	FILE	*fp;
	char	line[32];
	char	*inst;
	int	valid = 0;
	int	n;

	if ((fp = fopen(file, "r")) != NULL) {
		if (fgets(line, 32, fp) != NULL &&
				strncmp(line, "OS=Solaris", 10) == 0 &&
				fgets(line, 32, fp) != NULL &&
				strncmp(line, "VERSION=", 8) == 0 &&
				isdigit(line[10])) {
			n = atoi(&line[10]);
			inst = get_default_inst();
			if (streq(inst, "sparc") && n > 0) {
				valid = 1;
			} else if (streq(inst, "i386") && n >= 3) {
				if (n > 3 || (n == 3 &&
						atoi(&line[12]) >= 2) ||
						(n <= 3) && !isdigit(line[11]))
					valid = 1;
				/*
				 *  Check for a 2.4 or 2.5 Solaris Core system,
				 *  which is identified by a REV=100 line in the
				 *  INST_RELEASE file.  It is not upgradeable
				 *  to 2.5.  The following block of code is
				 *  what should be removed in 2.6 if these
				 *  releases are upgradable to 2.6.
				 */
				if ((n == 4 || n == 5) &&
						fgets(line, 32, fp) != NULL &&
						strncmp(line, "REV=", 4) == 0 &&
						isdigit(line[5])) {
					n = atoi(&line[4]);
					if (n == 100)
						valid = 0;
				}
			} else if (streq(inst, "ppc")) {
				valid = 1;
			}
		}

		(void) fclose(fp);
	}

	return (valid);
}

/*
 * _restore_fs_name()
 *	Go to the superblock of the file system referenced by 'device'
 *	and remove the leading '/a' from the "last mounted" field of
 *	the superblock.
 * Parameters:
 *	device	- disk name (e.g. c0t0d0s0)
 *	fs	- file system name
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_restore_fs_name(char * device, char * fs)
{
	char	  disk[MAXNAMELEN];
	int	  fd;
	int	  sblock[SBSIZE/sizeof (int)];
	struct fs *fsp = (struct fs *) sblock;

	(void) sprintf(disk, "/dev/rdsk/%s", device);
	if ((fd = open(disk, O_RDWR)) < 0)
		return;

	if (lseek(fd, SBOFF, SEEK_SET) == -1 ||
			read(fd, fsp, sizeof (sblock)) == -1) {
		(void) close(fd);
		return;
	}

	(void) strcpy(fsp->fs_fsmnt, fs);
	if (lseek(fd, SBOFF, SEEK_SET) == -1) {
		(void) close(fd);
		return;
	}

	(void) write(fd, fsp, sizeof (sblock));
	(void) close(fd);
}

/*
 * _check_var()
 *	Check the /etc/vfstab file for the local file system and find out if
 *	there is a separate entry for /var.
 * Parameters:
 *	none
 * Return:
 *	NULL	- no /var entry found in the mnttab
 *	char *	- pointer to string containing /var entry
 * Status:
 *	private
 */
static char *
_check_var(void)
{
	static char	emnt[64];
	char	line[256];
	FILE	*fp;
	char	*pdev;
	char	*pfs;

	if ((fp = fopen("/a/etc/vfstab", "r")) != NULL) {
		emnt[0] = '\0';
		while (fgets(line, 255, fp) != NULL) {
			if ((pdev = strtok(line, " \t")) == NULL)
				continue;

			if (*pdev != '/')
				continue;

			if (strtok(NULL, " \t") == NULL)
				continue;

			if ((pfs = strtok(NULL, " \t")) == NULL)
				continue;

			if (strcmp(pfs, "/var") == 0) {
				(void) fclose(fp);

				if (_map_to_effective_dev(pdev, emnt) != 0)
					return (NULL);

				return (emnt);
			}
		}

		(void) fclose(fp);
	}

	return (NULL);
}
