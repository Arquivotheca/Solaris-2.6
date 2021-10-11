#ifndef lint
#pragma ident "@(#)svc_updatedisk.c 1.10 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_updatedisk.c
 * Group:	libspmisvc
 * Description: Routines to update or validate the configuration of
 *		live disks.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "svc_strings.h"
#include "spmicommon_api.h"

/* internal prototype */

int		_setup_disks(Disk_t *, Vfsent *);
void 		_swap_add(Disk_t *);

/* private prototype */

static int 	_check_ufs(Disk_t *, int, int);
static int 	_create_ufs(Disk_t *, int, int);
static int	_format_disk(Disk_t *);
static int	_label_disks(Disk_t *);
static int	_label_fdisk(Disk_t *);
static int	_label_sdisk(Disk_t *);
static int	_load_alt_slice(Disk_t *);
static int	_newfs_disks(Disk_t *dlist, Vfsent *);

/* local static identifiers */

static char	cmd[MAXNAMELEN];

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	_setup_disks
 * Description: Update the fdisk and solaris labels on all selected drives,
 *		start swapping to all swap devices (not swap files), create
 *		or check UFS file systems on all devices which are specified.
 * Scope:	internal
 * Parameters:	dlist	- pointer to disk list
 *		vlist	- pointer to mount list
 * Return:	NOERR	- successful updating of disk state
 *		ERROR	- update failed
 */
int
_setup_disks(Disk_t *dlist, Vfsent *vlist)
{
	/*
	 * update the fdisk and Solaris VTOC labels on all selected drives
	 */
	if (_label_disks(dlist) == 0) {
		write_notice(ERRMSG, MSG0_DISK_LABEL_FAILED);
		return (ERROR);
	}

	/*
	 * start swapping to all swap slices as soon as possible
	 * to relieve virtual memory constraints
	 */
	_swap_add(dlist);

	/*
	 * create and check file systems on selected disks
	 * according to the disk list specifications
	 */
	if (_newfs_disks(dlist, vlist) != 0) {
		write_notice(ERRMSG, MSG0_DISK_NEWFS_FAILED);
		return (ERROR);
	}

	return (NOERR);
}

/*
 * Function:	_swap_add
 * Description:	Scan the disk list and start swapping to all slices of
 *		type 'swap'. This does not activate swapping to swap
 *		files, only swap devices.
 *
 *		NOTE:	This is not done for CacheOS clients (MT_CCLIENT)
 *
 * Scope:	public
 * Parameters:	dlist	- pointer to head of disk list
 * Return:	none
 */
void
_swap_add(Disk_t *dlist)
{
	Disk_t *dp;
	int	i;
	char    cmd[MAXNAMELEN];

	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK))
		return;

	if (get_machinetype() == MT_CCLIENT)
		return;

	/* if there are no disks, there is no work to do */
	if (dlist == NULL)
		return;

	for (dp = dlist; dp; dp = next_disk(dp)) {
		if (!disk_selected(dp))
			continue;

		WALK_SLICES(i) {
			if (!slice_is_swap(dp, i) || slice_ignored(dp, i))
				continue;

			(void) sprintf(cmd,
				"/usr/sbin/swap -a %s > /dev/null 2>&1",
				make_block_device(disk_name(dp), i));
			(void) system(cmd);
		}
	}
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	_check_ufs
 * Description:	Run fsck on the file system for a specific disk slice. Return
 *		in error only if the mount point name is a default mount list
 *		file system. Dflt mount file systems are checked sequentially.
 *		All other filesystems are checked in parallel in the background.
 *		FSCK failures are only reported for default mount point file
 *		systems.
 *
 *		NOTE: 	this routine does not actually execute an FSCK if the
 *			disk debug flag is set.
 *
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 *		slice	- slice index
 *		wait	- 0/1 whether the process should wait or be backgrounded
 * Return:	0	- successful, or 'dp' is NULL, of 'slice' is invalid
 *		# != 0	- exit code returned from fsck command
 */
static int
_check_ufs(Disk_t *dp, int slice, int wait)
{
	int	status = 0;

	if (dp == NULL || !valid_sdisk_slice(slice))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG3_SLICE_CHECK,
		slice_mntpnt(dp, slice),
		disk_name(dp),
		slice);

	if (get_trace_level() > 0) {
		write_status(SCR, LEVEL2|LISTITEM,
			wait == 1 ?
			MSG0_PROCESS_FOREGROUND :
			MSG0_PROCESS_BACKGROUND);
	}

	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK))
		return (0);

	(void) sprintf(cmd,
		"%s /usr/sbin/fsck -y -o p -o w /dev/rdsk/%ss%d \
			</dev/null >> %s 2>&1 %s",
		(wait == 1 ? "nice -19" : ""),
		disk_name(dp),
		slice,
		(wait == 1 ? "/tmp/install_log" : "/dev/null"),
		(wait == 1 ? "" : "&"));

	status = system(cmd);
	status >>= 8;

	if (wait == 1 && status != 0) {
		write_notice(ERRMSG,
			MSG3_SLICE_CHECK_FAILED,
			slice_mntpnt(dp, slice),
			disk_name(dp),
			slice);
		return (status);
	}

	return (0);
}

/*
 * Function:	_create_ufs
 * Description:	Create a UFS file system on a specific disk slice. Return
 *		in error only if the mount point name is a default mount list
 *		file system and the newfs fails. Default mount file systems are
 *		created sequentially; all other file systems are created in
 *		parallel in the background. NEWFS failures are only reported for
 *		default mount point file systems.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 *		slice	- valid slice index number
 *		wait	- 0/1 whether the process should wait or be backgrounded
 * Return:	0	- successful, or 'dp' is NULL, of 'slice' is invalid
 *		# != 0	- error code returned from newfs command
 */
static int
_create_ufs(Disk_t *dp, int slice, int wait)
{
	int	status = 0;

	if (dp == NULL || !valid_sdisk_slice(slice))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG3_SLICE_CREATE,
		slice_mntpnt(dp, slice),
		disk_name(dp),
		slice);

	if (get_trace_level() > 0) {
		write_status(SCR, LEVEL2|LISTITEM,
			wait == 1 ?
			MSG0_PROCESS_FOREGROUND :
			MSG0_PROCESS_BACKGROUND);
	}

	(void) sprintf(cmd,
		"%s /usr/sbin/newfs /dev/rdsk/%ss%d </dev/null >>%s 2>&1 %s",
		(wait == 1 ? "nice -19" : ""),
		disk_name(dp),
		slice,
		(wait == 1 ? "/tmp/install_log" : "/dev/null"),
		(wait == 1 ? "" : "&"));

	if (!GetSimulation(SIM_EXECUTE) && !GetSimulation(SIM_SYSDISK)) {
		status = system(cmd);
		status >>= 8;
		if (wait == 1 && status != 0) {
			write_notice(ERRMSG,
				MSG3_SLICE_CREATE_FAILED,
				slice_mntpnt(dp, slice),
				disk_name(dp),
				slice);
		}
	}

	return (status);
}

/*
 * Function:	_format_disk
 * Description:	Low level format an entire physical disk.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	D_OK	  - disk analysis completed successfully
 *		D_NODISK  - invalid disk structure pointer
 *		D_BADDISK - disk not in a state to be analyzed
 *		D_BADARG  - could not create tmpfile for format data input
 */
static int
_format_disk(Disk_t *dp)
{
	static char	tmpfile[64] = "";
	FILE		*fp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_format_disk(dp))
		return (D_OK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG0_DISK_FORMAT);

	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK))
		return (D_OK);

	if (tmpfile[0] == '\0') {
		(void) tmpnam(tmpfile);
		if ((fp = fopen(tmpfile, "w")) == NULL) {
			tmpfile[0] = '\0';
			return (D_BADARG);
		}

		(void) fprintf(fp, "f\n");
		(void) fprintf(fp, "quit\n");
		(void) fclose(fp);
	}

	if (disk_fdisk_exists(dp)) {
		(void) sprintf(cmd,
		"/usr/sbin/format -s -d /dev/rdsk/%sp0 -f %s >/dev/null 2>&1",
			disk_name(dp), tmpfile);
	} else {
		(void) sprintf(cmd,
		"/usr/sbin/format -s -d /dev/rdsk/%ss2 -f %s >/dev/null 2>&1",
			disk_name(dp), tmpfile);
	}

	if (system(cmd) != 0) {
		write_notice(ERRMSG,
			MSG1_DISK_FORMAT_FAILED, disk_name(dp));
		return (D_BADDISK);
	}

	return (D_OK);
}

/*
 * Function:	_label_disks
 * Description:	Write necessary labels out to all "selected" disks in the disk
 *		list which are in an "okay" state. This includes F-disk and
 *		S-disk labels. Low level format any drives flagged for
 *		processing.
 * Scope:	private
 * Parameters:	dp	- pointer to the head of the disk list (NULL If
 *			  the standard disk chain is to be used)
 * Return:	0	- one of the drives failed in writing the label
 *		1	- success writing the label on all drives
 */
static int
_label_disks(Disk_t *dp)
{
	if (dp == NULL)
		dp = first_disk();

	for (; dp; dp = next_disk(dp)) {
		if (!disk_selected(dp) || !disk_okay(dp))
			continue;

		write_status(LOGSCR, LEVEL0,
			MSG1_DISK_SETUP,
			disk_name(dp));

		/* format disk (if necessary) */
		if (_format_disk(dp) < 0)
			return (0);

		/* update F-disk label (if necessary) */
		if (_label_fdisk(dp) < 0)
			return (0);

		/* update S-disk label (if necessary) */
		if (_label_sdisk(dp) < 0)
			return (0);
	}

	return (1);
}

/*
 * Function:	_label_fdisk
 * Description: Write out an F-disk partition table on a specified drive based
 *		on the current F-disk configuration. This will also install the
 *		mboot file since the partition table is embedded within mboot.
 *
 *		ALGORITHM:
 *		(1) Assemble an	fdisk(1M) input file based on the current fdisk
 *		    configuration in the specified disk structure
 *		(2) Call the fdisk command to install the partition table and
 *		    the mboot file
 *
 *		NOTE: 	this routine does not actually write the partition
 *			table to the disk if the disk debug flag is set.
 *		NOTE:	partitions are written out in the order in which they
 *			were originally loaded, which is the same order in which
 *			they should be displayed to users. This is to prevent
 *			the install library from scrambling partition tables
 *			with the original sort
 * Scope:	private
 * Parameters:	dp	- non-NULL pointer to disk structure
 * Return:	 0	- attempt to write F-disk table succeeded, or no
 *			  partition table required on drive
 *		-1	- attempt to write F-disk label failed
 */
static int
_label_fdisk(Disk_t *dp)
{
	u_char  logval;
	FILE    *fp;
	char    *fname;
	int	p;
	int	c;
	int	active;
	Disk_t	*bdp;

	if (dp == NULL)
		return (-1);

	if (disk_no_fdisk_exists(dp))
		return (0);

	/*
	 * if this is the Solaris partition on the current boot disk, make it
	 * active
	 */
	(void) DiskobjFindBoot(CFG_CURRENT, &bdp);

	/*
	 * only report fdisk status information on systems which expose the
	 * fdisk interface, or if tracing is enabled
	 */
	if (disk_fdisk_req(dp) || get_trace_level() > 0) {
		write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_FDISK_CREATE);
		write_status(LOG, LEVEL0, MSG1_FDISK_TABLE, disk_name(dp));
	}

	/* if this is a live run, create a temporary input file for fdisk */
	if (!GetSimulation(SIM_EXECUTE)) {
		/*
		 * create a temporary file for fdisk(1M) input preserving the
		 * original partition ordering
		 */
		if ((fname = tmpnam(NULL)) == NULL) {
			if (disk_fdisk_req(dp) || get_trace_level() > 0)
				write_notice(ERRMSG, MSG0_FDISK_INPUT_FAILED);
			return (-1);
		} else if ((fp = fopen(fname, "w")) == NULL) {
			if (disk_fdisk_req(dp) || get_trace_level() > 0)
				write_notice(ERRMSG, MSG0_FDISK_OPEN_FAILED);
			return (-1);
		}
	}

	/*
	 * store a copy of the fdisk input file in the install log or
	 * display to screen depending on debug status
	 */
	logval = (GetSimulation(SIM_EXECUTE) ? SCR : LOG);
	WALK_PARTITIONS(c) {
		WALK_PARTITIONS(p) {
			if (part_orig_partnum(dp, p) != c)
				continue;

			if (bdp == dp) {
				if (IsIsa("i386")) {
					if (part_id(dp, p) == SUNIXOS)
						active = ACTIVE;
					else
						active = NOTACTIVE;
				} if (IsIsa("ppc")) {
					if (part_id(dp, p) == DOSOS12 ||
						    part_id(dp, p) == DOSOS16)
						active = ACTIVE;
					else
						active = NOTACTIVE;
				}
			} else
				active = part_active(dp, p);

			/*
			 * write status information only on systems are fdisk
			 * cognizant
			 */
			if (disk_fdisk_req(dp) || get_trace_level() > 0) {
				write_status(logval,
					LEVEL1|LISTITEM|CONTINUE,
					MSG4_FDISK_ENTRY,
					part_id(dp, p),
					active,
					part_geom_rsect(dp, p),
					part_geom_tsect(dp, p));
			}

			if (!GetSimulation(SIM_EXECUTE)) {
				(void) fprintf(fp,
					"%d %d 0 0 0 0 0 0 %d %d\n",
					part_id(dp, p),
					active,
					part_geom_rsect(dp, p),
					part_geom_tsect(dp, p));
			}

			break;
		}
	}

	/* ignore the rest of this code if we are running in dry-run */
	if (GetSimulation(SIM_EXECUTE)) {
		if (disk_fdisk_req(dp) || get_trace_level() > 0)
			write_status(logval, LEVEL0, "");
		return (0);
	}

	(void) fclose(fp);

	/* call fdisk(1M) to put down the fdisk label */
	(void) sprintf(cmd,
		"/sbin/fdisk -n -F %s /dev/rdsk/%sp0 >/dev/null 2>&1",
		fname, disk_name(dp));

	if (system(cmd) != 0) {
		if (disk_fdisk_req(dp) || get_trace_level() > 0)
			write_notice(ERRMSG, MSG0_FDISK_CREATE_FAILED);
		(void) unlink(fname);
		return (-1);
	}

	(void) unlink(fname);
	return (0);
}

/*
 * Function:	_label_sdisk
 * Description: If the disk requires a Solaris label, create one using the
 *		current F-disk configuration in the disk structure. Load
 *		the alternate sector slice if one is required.
 *
 *		ALGORITHM:
 *		(1) If the S-disk geometry pointer is NULL, there is no work
 *		    to do (the disk was never validated)
 *		(2) If an F-disk is required, but there is no Solaris partition
 *		    there is no work to do
 *		(3) Load the VTOC from the disk (or setup a dummy if there isn't
 *		    one or if install debug is turned on).
 *		(4) Update that VTOC structure from the current S-disk structure
 *		(5) Write out the VTOC structure (slam the controller on Intel)
 *		(6) If there is an alternate sector slice required, load it
 *		    with the addbadsec(1M)
 *
 *		NOTE:	The VTOC is only read and written, and the alternate
 *			sector slice loaded when the install debug flag is
 *			turned off
 *
 *		NOTE: In 494 there is rumored to be another mechanism for
 *			loading the alternate sector slice, but there is no
 *			information on this at this time
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	0	- success, 'dp' is NULL, or the S-disk geometry pointer
 *			  is NULL
 *		-1	- failure
 */
static int
_label_sdisk(Disk_t *dp)
{
	struct vtoc	vtoc;
	char		device[126];
	int		write_alts = 0;
	int		fd = -1;
	int		i;

	if (dp == NULL)
		return (0);

	if (sdisk_geom_null(dp))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_VTOC_CREATE);

	(void) sprintf(device, "/dev/rdsk/%ss2", disk_name(dp));

	if (!GetSimulation(SIM_EXECUTE) && !GetSimulation(SIM_SYSDISK)) {
		if ((fd = open(device, O_RDWR | O_NDELAY)) < 0) {
			write_notice(ERRMSG, MSG0_SLICE2_ACCESS_FAILED);
			return (-1);
		}
	}

	if (fd < 0 || read_vtoc(fd, &vtoc) < 0) {
		(void) memset(&vtoc, 0, sizeof (struct vtoc));
		vtoc.v_sectorsz = 512;
		vtoc.v_nparts = (ushort) numparts;
		vtoc.v_sanity = VTOC_SANE;
		vtoc.v_version = V_VERSION;	/* required for Intel */
	}

	WALK_SLICES(i) {
		if (slice_ignored(dp, i))
			continue;

		vtoc.v_part[i].p_start = cyls_to_blocks(dp, slice_start(dp, i));
		vtoc.v_part[i].p_size = slice_size(dp, i);
		vtoc.v_part[i].p_flag = 0;

		if (i == BOOT_SLICE) {
			vtoc.v_part[i].p_tag = V_BOOT;
			vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (strcmp(slice_mntpnt(dp, i), ROOT) == 0) {
			vtoc.v_part[i].p_tag = V_ROOT;
		} else if (strcmp(slice_mntpnt(dp, i), SWAP) == 0) {
			vtoc.v_part[i].p_tag = V_SWAP;
			vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (i == ALT_SLICE && slice_size(dp, i) > 0) {
			vtoc.v_part[i].p_tag = V_ALTSCTR;
			vtoc.v_part[i].p_flag = V_UNMNT;
			write_alts = 1;
		} else if (strncmp(slice_mntpnt(dp, i), USR, 4) == 0) {
			vtoc.v_part[i].p_tag = V_USR;
		} else if (strcmp(slice_mntpnt(dp, i), VAR) == 0) {
			vtoc.v_part[i].p_tag = V_VAR;
		} else if (strncmp(slice_mntpnt(dp, i), HOME, 5) == 0) {
			vtoc.v_part[i].p_tag = V_HOME;
		} else if (strncmp(slice_mntpnt(dp, i), EXPORTHOME, 12) == 0) {
			vtoc.v_part[i].p_tag = V_HOME;
		} else if (strncmp(slice_mntpnt(dp, i), CACHE, 12) == 0) {
			vtoc.v_part[i].p_tag = V_CACHE;
		} else if (slice_is_overlap(dp, i)) {
			vtoc.v_part[i].p_tag = V_BACKUP;
			if (slice_locked(dp, i))
				vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (slice_mntpnt_exists(dp, i)) {
			/* assigned, but not a system FS */
			if (!slice_preserved(dp, i))
				vtoc.v_part[i].p_tag = V_UNASSIGNED;
		} else {
			/* no mount point and not whole disk */
			if (!slice_preserved(dp, i)) {
				vtoc.v_part[i].p_tag = V_UNASSIGNED;
				vtoc.v_part[i].p_flag = V_UNASSIGNED;
			}
		}

		if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK)) {
			write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
				MSG4_SLICE_VTOC_ENTRY,
				i,
				slice_mntpnt(dp, i),
				vtoc.v_part[i].p_tag,
				vtoc.v_part[i].p_flag);
		}
	}

	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK))
		return (0);

	/* write out the VTOC (and label) */
	if (write_vtoc(fd, &vtoc) < 0) {
		write_notice(ERRMSG, MSG0_VTOC_CREATE_FAILED);
		(void) close(fd);
		return (-1);
	}

	(void) close(fd);

	/* load alternate sector slice if one exists */

	if (write_alts == 1) {
		if (_load_alt_slice(dp) < 0)
			return (-1);
	}

	return (0);
}

/*
 * Function:	_load_alt_slice
 * Description: If there is a non-zero alternate sector slice, make sure
 *		initialize the alternate sector table. Do nothing if the
 *		slice is already loaded (initialized) or is zero sized.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	 0	- load successful
 *		-1	- load failed
 */
static int
_load_alt_slice(Disk_t *dp)
{
	int	status;

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_ALT_SECTOR_SLICE);

	/*
	 * initialize the alternate sector slice with default
	 * data, or do nothing if it's already initialized
	 */
	(void) sprintf(cmd,
		"/usr/bin/addbadsec -f /dev/null /dev/rdsk/%sp0 \
			>/dev/null 2>&1",
		disk_name(dp));

	status = system(cmd);

	if (status != 0) {
		write_notice(ERRMSG, MSG0_ALT_SECTOR_SLICE_FAILED);
		return (-1);
	}

	return (0);
}

/*
 * Function:	_newfs_disks
 * Descritpion: Process the entire disk list. Newfs all UFS file systems
 *		with valid UFS mount points (and non-zero size) on all
 *		selected disks which are not "preserved", "locked", or
 *		"ignored". Run fsck for all UFS file systems which are marked
 *		"preserved".
 * Scope:	private
 * Parameters:	dlist	- pointer to list of disks
 *		vlist	- pointer to list of mount points
 * Return:	 0	- labelling completed successfully
 *		-1	- NEWFS of a default file system failed
 *		 1	- FSCK of default file system failed
 */
static int
_newfs_disks(Disk_t *dlist, Vfsent *vlist)
{
	Disk_t	*dp;
	int	i;
	int	wait;
	Vfsent	*vp;

	write_status(LOGSCR, LEVEL0, MSG0_CREATE_CHECK_UFS);

	WALK_LIST(dp, dlist) {
		if (SdiskobjIsIllegal(CFG_CURRENT, dp) ||
				!disk_okay(dp) ||
				!disk_selected(dp))
			continue;

		if (disk_fdisk_exists(dp) && sdisk_geom_null(dp))
			continue;

		WALK_SLICES(i) {
			if (slice_locked(dp, i) ||
					slice_ignored(dp, i) ||
					slice_mntpnt_isnt_fs(dp, i) ||
					slice_size(dp, i) == 0)
				continue;

			WALK_LIST(vp, vlist) {
				if (vp->entry != NULL &&
					    vp->entry->vfs_mountp != NULL &&
					    strcmp(vp->entry->vfs_mountp,
						slice_mntpnt(dp, i)) == 0) {
					wait = _mount_synchronous_fs(
						vp->entry, vlist);
					break;
				}
			}

			/* make sure the wait variable is set */
			if (vp == NULL)
				continue;

			if (slice_preserved(dp, i)) {
				if (_check_ufs(dp, i, wait) == 0)
					continue;

				/*
				 * bail out on FSCK only if it was a
				 * default mount filesystem
				 */
				if (get_dfltmnt_ent((Defmnt_t *)0,
						slice_mntpnt(dp, i)) == D_OK)
					return (1);
			} else if (_create_ufs(dp, i, wait) != 0)
				return (-1);
		}
	}

	return (0);
}
