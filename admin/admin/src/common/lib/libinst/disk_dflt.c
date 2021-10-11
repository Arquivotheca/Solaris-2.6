#ifndef lint
#pragma ident "@(#)disk_dflt.c 1.102 95/12/06"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc. All Rights Reserved. Sun
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
/*
 * This module contains routines which are use in configuration default
 * layouts on disks
 */
#include "disk_lib.h"
#include "ibe_lib.h"

#include <sys/mntent.h>
#include <memory.h>
#include <nlist.h>

/* Constants */

#define	SWAP_MIN	(16 * 2048)	/* swap floor sector count (16 MB) */
#define	SWAP_MAX	(256 * 2048)	/* swap ceiling sector count */
#define	NUMALTSECTCYL 	2		/* alt sector dflt size in cylinders */

/* Public Function Prototypes */

int		get_default_fs_size(char *, Disk_t *, int);
int		get_minimum_fs_size(char *, Disk_t *, int);
int		sdisk_default_all(void);

/* Library Function Prototypes */

int 		_setup_sdisk_default(Disk_t *);
int 		_setup_fdisk_default(Disk_t *);

/* Local Function Prototypes */

static int	_config_fs_dflt(Disk_t *, char *, int, int);
static int	_config_alts_dflt(Disk_t *);
static int	_sdisk_default_pass(void);
static int	_fs_immed_ancestor(char *, char *);
static void	_fs_absorb_children(char *, int);
static int	_min_fs_space(char *, int);
static int	_min_altsector_space(Disk_t *);
static int	_min_swap_space(void);
static int	_min_cache_space(void);
static int	_dflt_cache_space(Disk_t *);
static int	_dflt_swap_space(Disk_t *);
static int	_dflt_fs_space(char *, int);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * get_default_fs_size()
 * 	Calculate the default slice size for the default resource
 *	specified. The value returned is in 512 byte blocks.
 *
 *	NOTE:	this routine uses, but does not update, the default mount
 *		status mask
 * Parameters:
 *	fs	- non-NULL name of default file system requiring tabulation
 *	dp	- disk structure pointer for root drive (used for SWAP
 *		  constraint calculations - NULL otherwise)
 *		  (state:  okay)
 *	roll	- DONTROLLUP	- don't include DONTCARE children of 'fs'
 *		  ROLLUP	- include DONTCARE children of 'fs'
 * Return:
 *	0	- no space required for the file system, or bad argument
 *	#	- default partition size in 512 byte blocks
 * Status:
 *	public
 */
int
get_default_fs_size(char *fs, Disk_t *dp, int roll)
{
	int	size;

	if (streq(fs, SWAP)) {
		size = _dflt_swap_space(dp);
	} else if (streq(fs, ALTSECTOR)) {
		size = _min_altsector_space(dp);
	} else if (streq(fs, CACHE)) {
		size = _dflt_cache_space(dp);
	} else {
		size = _dflt_fs_space(fs, roll);
	}

	return (size);
}

/*
 * get_minimum_fs_size()
 * 	Calculate the minimum slice size for the default resource
 *	specified. The value returned is in 512 byte blocks.
 *
 *	NOTE:	this routine uses, but does not update, the default mount
 *		status mask
 * Parameters:
 *	fs	- non-NULL name of default mount point
 *	dp	- disk structure pointer (NULL if no SWAP constraint used)
 *		  (state:  okay)
 *	roll	- DONTROLLUP	- don't include DONTCARE children of 'mntpnt'
 *		  ROLLUP	- include DONTCARE children of 'mntpnt'
 * Return:
 *	0	- no space required for the file system, or bad arguement
 *	#	- default partition size in sectors
 * Status:
 *	private
 */
int
get_minimum_fs_size(char *fs, Disk_t *dp, int roll)
{
	int	  size;

	if (streq(fs, SWAP))
		size = _min_swap_space();
	else if (streq(fs, ALTSECTOR))
		size = _min_altsector_space(dp);
	else if (streq(fs, CACHE))
		size = _min_cache_space();
	else
		size = _min_fs_space(fs, roll);

	return (size);
}

/*
 * sdisk_default_all()
 *	Auto layout across all selected disks that have a valid
 *	S-disk geometry pointer. Keep retrying different strategies
 *	until filesys_ok() returns cleanly, or there are no more
 *	strategies to try. If the routine fails to fit everything,
 *	the dfltmnt mask is left as it stood at the end of the
 *	fit attempt. If the bootdisk is not selected, this routine
 *	automatically selects one so that root and swap get configured.
 *
 *	NOTE:	All selected drives must be committed before this
 *		routine is called because the committed state is
 *		used in the recovery attempts.
 *
 * Parameters:
 *	none
 * Return:
 *	 0	- successful layout
 *	-1	- layout failed
 * Status:
 *	public
 */
int
sdisk_default_all(void)
{
	static Defmnt_t	**mpp = NULL;
	int		retcode;
	Disk_t		*dp;
	Disk_t		*tmpdp;
	Disk_t		*obdp;		/* original boot disk */
	Disk_t		*nextbd;	/* next boot disk */
	char		*cp;

	if (first_disk() == NULL)
		return (-1);

	/*
	 * make sure there is an insallable boot disk before running
	 * the layout algorithms
	 */
	dp = obdp = find_bootdisk();

	if (obdp == NULL || sdisk_not_usable(obdp)) {
		if ((cp = spec_dflt_bootdisk()) == NULL ||
				((dp = find_disk(cp)) == NULL ||
				sdisk_not_usable(dp))) {
			WALK_DISK_LIST(dp) {
				if (sdisk_is_usable(dp) &&
					    select_bootdisk(dp, NULL)
					    == D_OK)
					break;
			}
		}

		if (dp == NULL)
			return (-1);
	}

	mpp = get_dfltmnt_list(mpp);
	/*
	 * default return code is failure and must be explicitly reset
	 * by a successful _sdisk_default_pass()
	 */
	retcode = -1;
	nextbd = dp->next;

	while ((retcode = _sdisk_default_pass()) < 0) {
		/*
		 * try to change the root disk if it wasn't explicitly
		 * set in the committed state
		 */
		if (find_mnt_pnt(NULL, NULL,
					ROOT, NULL, CFG_COMMIT) != 0)
			break;

		while (nextbd != NULL && sdisk_not_usable(nextbd))
			nextbd = nextbd->next;
		if (nextbd == NULL)
			break;

		tmpdp = nextbd->next;
		if (select_bootdisk(nextbd, "") != D_OK)
			break;

		nextbd = tmpdp;

		/* reset default mount list */
		if (set_dfltmnt_list(mpp) != D_OK)
			break;

		/* restore all selected disks to their committed states */
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp)) {
				if (sdisk_config(dp, NULL,
						CFG_COMMIT) != D_OK)
					break;
			}
		}
	}

	if (retcode == -1 &&
			find_mnt_pnt(NULL, NULL, ROOT,
					NULL, CFG_CURRENT) == 0)
		(void) select_bootdisk(obdp, "");

	return (retcode);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * _setup_fdisk_default()
 *	Set the F-disk to have a single partition, which is of
 *	type "SUNIXOS", is "ACTIVE", and uses all usable disk
 *	blocks on the drive. Validate the F-disk configuration in
 *	order to establish the S-disk geometry pointer (if there
 *	is a Solaris partition defined)
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 * Return:
 *	D_OK	  - disk setup successfully, or no setup required
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - S-disk geometry pointer NULL or state not "okay"
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_fdisk_default(Disk_t *dp)
{
	int	status;

	if (dp == NULL)
		return (D_NODISK);

	if ((status = _reset_fdisk(dp)) != D_OK)
		return (status);

	/* use partition '1' for the SUNIXOS default partition */
	if ((status = set_part_geom(dp, 1, disk_geom_firstcyl(dp),
			usable_disk_blks(dp))) != D_OK)
		return (status);

	if ((status = set_part_attr(dp, 1, SUNIXOS, ACTIVE)) !=  D_OK)
		return (status);

	if ((status = validate_fdisk(dp)) != D_OK)
		return (status);

	return (D_OK);
}

/*
 * _setup_sdisk_default()
 * 	Set-up as many default slices (mount points) onto the disk provided
 *	as can fit. Use the defmnts[] utilities to determine how default
 *	file systems can be split up (if necessary). '/' will only be
 *	configured on the boot disk. SWAP will attempt to be fit on the
 *	boot disk, but if it can't be, will be fit on a supplemental disk.
 *	The default algorithm for fitting is:
 *
 *	(1) for each "SELECTED" file system in the default mount list,
 *	    try and fit it in its "default" slice rolling up all its
 *	    children. If this fails, then:
 *	(2) try and fit it with roll-up on any slice. If this fails, then:
 *	(3) try and fit it without roll-up on its "default" slice. If
 *	    this fails, then:
 *	(4) try and fit it without roll-up on any slice.
 *
 *	If a non-roll-up attempt succeeds, then any "orphaned" child default
 *	file systems have their dfltmnt mask status set to DFLT_SELECT.
 *
 *	If there is < one cylinder of space left over when you are done,
 *	absorb it in the first modifiable slice which will accept it. If
 *	none will accept the extra space, just go on.
 *
 *	This routine uses and updates the defmnt[] masks.
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 * Return:
 *	D_OK		- disk setup successful
 *	D_NODISK	- 'dp' is NULL
 *	D_BOOTCONFIG	- attempt to set ROOT or SWAP on the boot drive
 *			  failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_sdisk_default(Disk_t *dp)
{
	static Defmnt_t	**mpp = NULL;
	static Defmnt_t	**app = NULL;
	int		i, status, extra;

	if (dp == NULL)
		return (D_NODISK);

	/*
	 * set the default size for the F-disk Solaris altsector slice
	 * (if required)
	 */
	if (disk_fdisk_exists(dp))
		(void) _config_alts_dflt(dp);

	mpp = get_dfltmnt_list(mpp);

	/* run through the entire default mount list */
	for (status = D_OK, i = 0; mpp[i]; i++) {
		if (sdisk_space_avail(dp) < MINFSSIZE)
			break;

		if (disk_not_bootdrive(dp) && strcmp(mpp[i]->name, ROOT) == 0)
			continue;

		/*
		 * try to fit FS in explicit slice rolled up. If that
		 * fails, try to fit it anywhere rolled up. If that
		 * fails, try to fit it in explicit slice not rolled
		 * up. If that fails, try anywhere not rolled up.
		 * Obviously, if the specific slice is WILD_SLICE, two
		 * of these steps will be skipped due to redundancy
		 */
		status = _config_fs_dflt(dp, mpp[i]->name, ROLLUP, SPEC_SLICE);

		if (status == D_DUPMNT)
			status = _config_fs_dflt(dp, mpp[i]->name,
							ROLLUP, WILD_SLICE);

		if (status == D_NOSPACE) {
			status = _config_fs_dflt(dp, mpp[i]->name,
							DONTROLLUP, SPEC_SLICE);

			if (status == D_DUPMNT)
				status = _config_fs_dflt(dp, mpp[i]->name,
							DONTROLLUP, WILD_SLICE);
		}

		if (strcmp(mpp[i]->name, ROOT) == 0 && status != D_OK)
			return (D_BOOTCONFIG);
	}
	/*
	 * this will pick up any individual child filesystems which
	 * were orphaned in the last loop which might fit without siblings
	 * as long as there is room for at least a minimum file system
	 */
	if (mpp[i] == NULL) {
		app = get_dfltmnt_list(app);

		for (i = 0; app[i]; i++) {
			if (!disk_bootdrive(dp) &&
					strcmp(app[i]->name, ROOT) == 0)
				continue;

			if (mpp[i]->status != DFLT_SELECT &&
					app[i]->status == DFLT_SELECT) {
				(void) _config_fs_dflt(dp, app[i]->name,
					DONTROLLUP, SPEC_SLICE);
			}
		}
	}

	extra = sdisk_space_avail(dp);

	if (extra <= one_cyl(dp) && extra > 0) {
		for (i = LAST_STDSLICE; i >= 0; i--) {
			if (slice_mntpnt_not_exists(dp, i) ||
					slice_is_overlap(dp, i) ||
					slice_locked(dp, i) ||
					slice_ignored(dp, i) ||
					slice_preserved(dp, i))
				continue;

			if (set_slice_geom(dp, i, GEOM_IGNORE,
					slice_size(dp, i) + extra) == D_OK)
				break;
		}
	}
	return (D_OK);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _sdisk_default_pass()
 *	Try running a default layout across the drives given the
 *	current bootdrive configuration.
 *
 *	ALGORITHM:
 *	(1) get the current rollup mask
 *	(2) while filesys_ok() returns in error and there are
 *	    still DFLT_DONTCAREs in the rollup mask, keep
 *	    calling sdisk_config() on selected disks in the disk
 *	    list until there are no more disks, or filesys_ok()
 *	    is satisfied. The first DONTCARE file system we will
 *	    try to spin off is /usr/openwin due to its known size.
 *	(3) if filesys_ok() is not satisfied and there are no more
 *	    disks, run through the mask and set the next DONTCARE
 *	    entry to SELECTED, reset all the drives to their
 *	    committed state, and try the run again.
 *
 * Parameters:
 *	none
 * Return:
 *	 0	- configuration successfully completed
 *	-1	- configuration incomplete
 * Status:
 *	private
 */
static int
_sdisk_default_pass(void)
{
	static Defmnt_t	**mpp = NULL;
	Disk_t		*dp;
	Space		**status;
	int		retcode;
	int		retry;
	int		i;

	if ((mpp = get_dfltmnt_list(mpp)) == NULL)
		return (-1);

	/*
	 * default return code is failure and must be explicitly reset
	 * by a successful filesys_ok()
	 */
	retcode = -1;

	/*
	 * continue this loop as long as there are retry options available
	 * and the current run was not successful
	 */
	do {
		/*
		 * if we don't have everything we need, keep trying to
		 * configure on other drives
		 */
		for (dp = first_disk(); (status = filesys_ok()) != NULL && dp;
					dp = next_disk(dp)) {

			if (disk_not_selected(dp))
				continue;

			if (sdisk_config(dp, NULL, CFG_DEFAULT) != D_OK)
				(void) sdisk_config(dp, NULL, CFG_COMMIT);
		}

		if (status == NULL) {
			/*
			 * if we had a successful run, it's time to return;
			 * assign the free space (if there is any) only on
			 * drives which were modified by the autolayout
			 * routine (since last committed)
			 */
			WALK_DISK_LIST(dp) {
				if (disk_selected(dp) &&
					    sdisk_compare(dp, CFG_COMMIT) != 0)
					(void) sdisk_use_free_space(dp);
			}
			retcode = 0;
		} else {
			/* Entering retry code */
			retry = 0;

			/* look first for /usr/openwin... */
			for (i = 0; mpp[i] != NULL; i++) {
				if (strcmp(mpp[i]->name, USROWN) == 0 &&
					    mpp[i]->status == DFLT_DONTCARE) {
					mpp[i]->status = DFLT_SELECT;
					++retry;
					break;
				}
			}

			/* look for the next DONTCARE */
			if (retry == 0) {
				for (i = 0; mpp[i] != NULL; i++) {
					if (mpp[i]->status == DFLT_DONTCARE) {
						mpp[i]->status = DFLT_SELECT;
						++retry;
						break;
					}
				}
			}

			/* config mods were made, so retry the run */
			if (retry > 0) {
				/* set the rollup mask for restart */
				if (set_dfltmnt_list(mpp) != D_OK)
					break;
				/*
				 * restore the drives and start over with the
				 * modified mask
				 */
				WALK_DISK_LIST(dp) {
					if (sdisk_not_usable(dp))
						continue;

					(void) sdisk_config(dp, NULL,
						CFG_COMMIT);
				}
			}
		}
	} while (retry > 0 && retcode < 0);

	/*
	 * return 0 if a run was successful, otherwise, return -1 because none
	 * of the retry option runs were successful and there's nothing left to
	 * try
	 */
	return (retcode);
}

/*
 * _config_fs_dflt()
 *	Try to configure a default file system on the specified disk. The
 *	file system size requirements are always "default" and are based
 *	on the 'roll' parameter. Slice selection is parameterized tomay be
 *	either explicit (SPEC_SLICE) or "best-fit" (WILD_SLICE). This
 *	routine does use and update the mount status fields.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	fs	- default mount point name to be added to disk
 *	roll	- ROLLUP or DONTROLLUP space for 'fs' when trying to fit
 *	slicestate  - WILD_SLICE or SPEC_SLICE
 * Return:
 *	D_OK	  - succeeded
 *	D_BADARG  - file system specified is not a default mount point
 *	D_NOSPACE - to add 'fs' to 'dp'
 * Status:
 *	private
 */
static int
_config_fs_dflt(Disk_t *dp, char *fs, int roll, int slicestate)
{
	Defmnt_t	def;
	Mntpnt_t	p;
	char		name[26];
	char		*cp;
	int 		status;
	int		size;
	int		slice;
	int		root;
	int		found;

	/* non-default mount entries cannot be default configured */
	if (get_dfltmnt_ent(&def, fs) != D_OK)
		return (D_BADARG);

	/*
	 * ignored file systems are the responsibility of another file
	 * system (no work to do)
	 */
	if (def.status == DFLT_IGNORE)
		return (D_OK);

	/*
	 * if there is already a slice configured (e.g. explicit layout) 
	 * make sure the default sizing fits, and the dfltmnt mask 
	 * file system status is correct
	 */
	if (find_mnt_pnt(NULL, NULL, def.name, &p, CFG_CURRENT) == 1) {
		if (def.status != DFLT_SELECT) {
			def.status = DFLT_SELECT;
			(void) set_dfltmnt_ent(&def, def.name);
		}

		/*
		 * use default size here because this is default layout;
		 * if JumpStart got here it's using partitioning default,
		 * which means it must meet the default criteria
		 */
		size = get_default_fs_size(def.name, dp, roll);
		if (size > slice_size(p.dp, p.slice))
			return (D_NOSPACE);
	
		if (get_trace_level() > 2) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Default file system already configured (%s)",
				def.name);
		}

		/*
		 * update the status of all the DFLT_DONTCARE children of the
		 * configured file system according to the rollup specifier
		 */
		_fs_absorb_children(def.name, roll);
		return (D_OK);
	}

	/* see if the file system was orphaned by an explicit layout */
	if (def.status == DFLT_DONTCARE) {
		(void) strcpy(name, def.name);
		found = 0;
		root = 0;
		/*
		 * starting with the file system, look for the parent
		 * directory, and its parent, and so forth, until you
		 * find the default mount point which is the immediate
		 * ancestor of the file system; if the ancestor has
		 * space already allocated on a disk, then set the
		 * "found" flag
		 */
		for (cp = strrchr(name, '/');
				found == 0 && root == 0 && cp != NULL;
				cp = strrchr(name, '/')) {
			if (cp == name) {
				root++;
				cp++;
			}

			*cp = '\0';

			if (_fs_immed_ancestor(name, def.name) == 1) {
				if (find_mnt_pnt(NULL, NULL, name,
					    &p, CFG_CURRENT) == 1)
					found++;
				break;
			}
		}

		if (found == 1) {
			/*
			 * we have found the immediate ancestor and it has
			 * already been configured; see if it is big enough to
			 * hold this DFLT_DONTCARE child using default sizing
			 * (assuming the slice can assume extra data)
			 */
			size = get_default_fs_size(name, dp, ROLLUP);
			if (size <= slice_size(p.dp, p.slice)) {
				if (slice_not_preserved(dp, p.slice) &&
					    slice_not_locked(dp, p.slice) &&
					    slice_not_ignored(dp, p.slice)) {
					def.status = DFLT_IGNORE;
					(void) set_dfltmnt_ent(&def, def.name);
					if (get_trace_level() > 2) {
						write_status(SCR,
						LEVEL1|LISTITEM,
			"Orphaned file system absorbed by ancestor (%s)",
							def.name);
					}

					return (D_OK);
				}
			}
		}

		/*
		 * we assume if you have an ancestor that it better be
		 * layed out on disk because of the ordering of the
		 * default mount list; for this reason we don't bother
		 * to check "found == 0" and we just assume this file
		 * system has been orphaned by an existing parent
		 */
		def.status = DFLT_SELECT;
		(void) set_dfltmnt_ent(&def, fs);
		if (get_trace_level() > 2) {
			write_status(SCR, LEVEL1|LISTITEM,
				"Orphaned file system being selected (%s)",
				def.name);
		}
	}

	/*
	 * we know at this point we must allocate disk space to this file
	 * system; make sure the disk has enough contiguous space to fit it
	 */
	size = get_default_fs_size(def.name, dp, roll);
	if (size > sdisk_max_hole_size(dp))
		return (D_NOSPACE);

	/*
	 * if the default slice for is WILD_SLICE or the parameter indicates
	 * that any slice can be used, find an available slice (don't use slice
	 * 2 unless its size has been explicitly set)
	 */
	slice = def.slice;
	if (slice == WILD_SLICE || slicestate == WILD_SLICE) {
		WALK_SLICES(slice) {
			if ((slice != ALL_SLICE ||
					sdisk_touch2(dp)) &&
					slice_mntpnt_not_exists(dp, slice) &&
					slice_not_locked(dp, slice) &&
					slice_not_preserved(dp, slice) &&
					slice_not_ignored(dp, slice) &&
					slice_size(dp, slice) == 0)
				break;
		}
	}

	/*
	 * 'slice' is either an unused free slice with a WILD_SLICE search, or
	 * an explicit default slice; if the slice is not valid, there is no
	 * free slice open for the task
	 */
	if (invalid_sdisk_slice(slice))
		return (D_NOSPACE);

	/*
	 * test to see if the slice found is already in use ('0' size and
	 * no mount point name); save the mount point name (for restore)
	 * and set the mount point name and size
	 */
	if (slice_mntpnt_exists(dp, slice) || slice_size(dp, slice) > 0)
		return (D_DUPMNT);

	(void) strcpy(name, slice_mntpnt(dp, slice));
	if ((status = set_slice_mnt(dp, slice, def.name, NULL)) != D_OK)
		return (status);

	if ((status = set_slice_geom(dp, slice, GEOM_IGNORE, size)) != D_OK) {
		/* restore the original slice mount point name */
		(void) set_slice_mnt(dp, slice, name, NULL);
		return (status);
	}

	if (get_trace_level() > 2) {
		write_status(SCR, LEVEL1|LISTITEM,
			"Default file system configured (%s)",
			def.name);
	}

	/*
	 * update the status of all the DFLT_DONTCARE children of the
	 * configured file system according to the rollup specifier
	 */
	_fs_absorb_children(def.name, roll);
	return (D_OK);
}

/*
 * _config_alts_dflt()
 *	Try to configure slice '9' as the default alternate sector slice
 *	on drives which require one. Alternate sector (ALTS) slices are
 *	only configured on non-SCSI drives, and are allocated out of the
 *	data area of the drive.
 *
 *	ALGORITHM:
 *	(1)  Find out the recommended ALTS slice size
 *	(2)  If an alt slice is not required, but currently has a
 *	     non-zero size, crush the size to '0' and free the space
 *	(3)  If an alt slice is required and currently exists (has a
 *	     non-zero size), just keep the existing one regardless of
 *	     size differences.
 *	(4)  If an alt slice is required an one does not currently
 *	     exist with a non-zero size, create a new one beginning
 *	     at the first usable data cylinder, setting it to the
 *	     suggested size (based on drive type) and label the slice
 *	     appropriately for the VTOC.
 *
 *	NOTE:	slice 9 is a reserved slice which is only used for the
 *		alternate sector slice on fdisk systems.
 *
 * Parameters:
 *	dp	  - non-NULL disk structure pointer with a valid S-disk
 *		    geometry pointer
 * Return:
 *	D_OK	  - alternate slice configured (if necessary)
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - the S-disk geometry pointer is NULL (checked to
 *		    prevent core dumps when F-disk changes are made
 *		    without a subsequent validate_fdisk() call)
 *	D_BADARG  - _min_altsector_space() returned a failure status
 * Status:
 *	private
 */
static int
_config_alts_dflt(Disk_t * dp)
{
	int	asect;

	if (dp == NULL)
		return (D_NODISK);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (disk_no_fdisk_exists(dp))
		return (D_OK);

	if ((asect = _min_altsector_space(dp)) == 0) {
		/* crush the alternate sector slice - not needed */
		if (slice_size(dp, ALT_SLICE) > 0) {
			slice_size_set(dp, ALT_SLICE, 0);
			slice_start_set(dp, ALT_SLICE, 0);
		}
		return (D_OK);
	}

	if (asect < 0)
		return (D_BADARG);
	/*
	 * if the alternate sectors are required on this drive and
	 * the current alternate sector slice is sized at '0',
	 * resize it
	 */
	if ((slice_size(dp, ALT_SLICE) == 0) && (asect > 0)) {
		slice_mntpnt_set(dp, ALT_SLICE, ALTSECTOR);
		slice_start_set(dp, ALT_SLICE, sdisk_geom_firstcyl(dp));
		slice_size_set(dp, ALT_SLICE, asect);
	}
	return (D_OK);
}

/*
 * _fs_immed_ancestor()
 *	Boolean function which determines if 'ancestor' is the closest
 *	(most immediate) DFLT_SELECT default file system name to 'child'
 *	given the current machinetype status mask. If 'ancenstor' is not
 *	DFLT_SELECT, this routine will return '0'. If 'ancestor' == 'child'
 *	DFLT_SELECT is set, then will always return '1'.
 * Parameters:
 *	ancestor - parent default file system used in search
 *	child    - child default file system used in search
 * Return:
 *	0  - 'ancestor' is not the immediate ancestor of 'child'
 *	1  - 'ancestor' is the immediate ancestor of 'child'
 * Status:
 *	private
 */
static int
_fs_immed_ancestor(char *ancestor, char *child)
{
	Defmnt_t def;
	char	*name;
	char	*cp;

	/* both child and ancestor must be filesystems */
	if (child == NULL || child[0] != '/' ||
				ancestor == NULL || ancestor[0] != '/')
		return (0);

	/* the child must be farther down in the FS tree */
	if ((int) strlen(child) < (int) strlen(ancestor))
		return (0);

	/* make sure this malloc is freed before returning */
	name = xstrdup(child);

	do {
		if (get_dfltmnt_ent(&def, name) == D_OK &&
					def.status == DFLT_SELECT) {
			if (strcmp(name, ancestor) == 0) {
				free(name);	/* free malloc */
				return (1);
			}
			break;
		}
		if ((cp = strrchr(name, '/')) != NULL) {
			if (strcmp(name, ROOT) == 0)
				cp = NULL;
			else {
				if (cp == name)
					*++cp = '\0';
				else
					*cp = '\0';
			}
		}
	} while (cp && ((int) strlen(ancestor) <= (int) strlen(name)));

	free(name);				/* free malloc */
	return (0);
}

/*
 * _min_altsector_space()
 *	Return the suggested alternate sector slice size for the given drive.
 *	Alternate sectors are only required on fdisk systems with non-SCSI
 *	disks.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of sectors required for the atlernate sector
 *		  slice
 * Status:
 *	private
 */
static int
_min_altsector_space(Disk_t *dp)
{
	if (dp == NULL || disk_no_fdisk_exists(dp))
		return (0);

	if (_disk_is_scsi(dp))
		return (0);

	return (NUMALTSECTCYL * one_cyl(dp));
}

/*
 * _min_fs_space()
 *	Calculate the minimum size required for a given slice based on software
 *	and expansion space requirements. 'roll' specifies if the space for
 *	file systems marked "DONTCARE" which are children of 'fs' should be
 *	included during the calculation. This routine does NOT look at the
 *	current state of the disk chain, and is driven entirely by the current
 *	defmnt*[] status mask. Entries marked DFLT_DONTCARE return their size
 *	only if 'roll' is DONT_ROLLUP, otherwise, they return '0'. Entries
 *	marked DFLT_IGNORE always return '0'.
 *
 *	NOTE:	this routine does not modify the current defmnt status mask
 * Parameters:
 *	fs	- non-NULL default mount point name for which data is being
 *		  retrieved
 *	roll	- Valid values:
 *		  DONTROLLUP	don't include DONTCARE children of 'fs'
 *		  ROLLUP	include DONTCARE children of 'fs'
 * Returns:
 *	0	- no space table entry for file system specified, entry
 *		  marked "DFLT_IGNORE", or bad arguments passed
 *	# > 0	- size of file system in sectors
 * Status:
 *	private
 */
static int
_min_fs_space(char *fs, int roll)
{
	static Defmnt_t	**mpp = NULL;
	Defmnt_t	def;
	int		size;
	int		exp;
	int		i;

	/* validate parameters */
	if (fs == NULL || (roll != ROLLUP && roll != DONTROLLUP))
		return (0);

	/* load the current space data */
	update_dfltmnt_list();

	/* get the default mount entry for this file system */
	if (get_dfltmnt_ent(&def, fs) != D_OK || def.status == DFLT_IGNORE)
		return (0);

	/* initialize the size and expansion variables */
	size = (def.size > 0 ? def.size : 0);
	exp = (def.expansion > 0 ? def.expansion : 0);

	/* process DFLT_DONTCARE entries directory */
	if (def.status == DFLT_DONTCARE) {
		if (roll == ROLLUP)
			size = 0;
		else
			size += exp;

		return (size);
	}

	/*
	 * we know def.status == DFLT_SELECT at this point; include the
	 * space for all default mount points which 'fs' is an immediate
	 * ancestor, and which are not themselves DFLT_SELECT; take the
	 * rollup parameter into account when making the determination
	 */
	mpp = get_dfltmnt_list(mpp);
	for (i = 0; mpp[i]; i++) {
		/* dataless systems only include "/" and "/var" */
		if (get_machinetype() == MT_DATALESS &&
				strneq(mpp[i]->name, ROOT) &&
				strneq(mpp[i]->name, VAR))
			continue;

		/*
		 * space for itself accounted for at size variable
		 * initialization
		 */
		if (streq(fs, mpp[i]->name))
			continue;

		/* only count DONTCARE children if we are rolling up */
		if (roll == DONTROLLUP && mpp[i]->status == DFLT_DONTCARE)
			continue;

		/*
		 * see if this is a dependent child, and if so, add its
		 * space
		 */
		if (_fs_immed_ancestor(fs, mpp[i]->name)) {
			if (mpp[i]->size > 0)
				size += mpp[i]->size;

			if (mpp[i]->expansion > 0)
				exp += mpp[i]->expansion;
		}
	}

	size += exp;
	return (size);
}

/*
 * _min_cache_space()
 *	Minimum cache space required. Minimum cache space is
 *	calculated to be 24 MB plus the amount of required swap
 *	space not already acounted for by swap slices (i.e. the
 *	size /.cache/swap must be):
 *
 *		(total swap space required for the system) -
 *		    (total space allocated to swap slices)
 *
 * Parameters:
 *	none
 * Return:
 *	# >= 0	- number of sectors
 * Status:
 *	private
 */
static int
_min_cache_space(void)
{
	int		size;
	char		*swapenv;
	MachineType	mt = get_machinetype();

	/* /.cache is only supported on CacheOS systems */
	if (mt != MT_CCLIENT)
		return (0);

	/*
	 * find out how much swap is required
	 */
	if ((swapenv = getenv("SYS_SWAPSIZE")) != NULL && \
			is_allnums(swapenv) == 1)
		size = mb_to_sectors(atoi(swapenv));
	else
		size = mb_to_sectors(32);

	/*
	 * subtrace swap space allocated to swap slices on
	 * selected disks
	 */
	size -= swap_size_allocated(NULL, NULL);

	/*
	 * if more swap space is allocated to slices than is required,
	 * reset the size to '0'
	 */
	if (size < 0)
		size = 0;

	/*
	 * add the minimum 24 MB floor to the total size
	 */
	size += mb_to_sectors(24);

	return (size);
}

/*
 * _min_swap_space()
 *	Calculate the minimum swap space which must be configured on
 *	the system. All systems must have a minimum of 32 MB of
 *	virtual memory, where VM = physical_mem + swap space.
 *	This calculation can be overridden in this routine, by
 *	explicit value sets. The size of swap is calculated, in
 *	order of precedence, as:
 *
 *	(1) the explicit SYS_SWAPSIZE environment variable
 *	(2) the explicit dfltmnt() table value
 *	(3) the value based on physical memory size
 *
 * Parameters:
 *	none
 * Return:
 *	# >= 0	- # of sectors
 * Status:
 *	private
 */
static int
_min_swap_space(void)
{
	Defmnt_t	def;
	char		*swapenv;
	int		size;

	/* non-default mount entries cannot be default configured */
	if (get_dfltmnt_ent(&def, SWAP) != D_OK)
		return (0);

	if ((swapenv = getenv("SYS_SWAPSIZE")) != NULL && \
			is_allnums(swapenv) == 1) {
		/* environment variable lookup */
		size = mb_to_sectors(atoi(swapenv));
	} else if (def.expansion >= 0) {
		/* explicit default */
		size = def.expansion;
	} else {
		/* default heuristic */
		size = mb_to_sectors(32) - _calc_memsize();
	}

	/* sanity check on size return value */
	if (size < 0)
		size = 0;

	return (size);
}

/*
 * _dflt_swap_space()
 *	Calculate the default swap space required. The default swap space
 *	cannot be less that 16 MB, or more than 32 MB. The default value
 *	is calculated based on physical memory size, where:
 *
 *		  physical		swap
 *		  --------		----
 *		  0 -  64  MB		 32 MB
 *		 64 - 128  MB		 64 MB
 *		128 - 512  MB		128 MB
 *		  > 512    MB		256 MB
 *
 *	The value is then truncated if it exceeds 20% of the disk
 *	capacity on which the configuration is being made (if the
 *	calculation is being done in the context of a specific drive).
 *	This is for historical reasons (104 MB disks) and should
 *	probably be dropped entirely in future releases. This calculation
 *	can be overridden in this routine, by explicit value sets. The size
 *	of swap is calculated, in order of precedence, as:
 *
 *	(1) the explicit SYS_SWAPSIZE environment variable
 *	(2) the explicit dfltmnt() table value
 *	(3) the value based on above calculation
 *
 * Parameters:
 *	dp	- disk structure pointer used to calculate
 *		  disk constraints on swap size relative to
 *		  a particular disk
 * Return:
 *	# >= 0	- number of sectors
 * Status:
 *	 private
 */
static int
_dflt_swap_space(Disk_t *dp)
{
	Defmnt_t	def;
	int		size;
	int		mem = sectors_to_mb(_calc_memsize());
	int		dsize;
	char		*swapenv;

	/* non-default mount entries cannot be default configured */
	if (get_dfltmnt_ent(&def, SWAP) != D_OK)
		return (0);

	if ((swapenv = getenv("SYS_SWAPSIZE")) != NULL) {
		/* environment explicit setting */
		size = mb_to_sectors(atoi(swapenv));
	} else if (def.expansion >= 0) {
		/* configured explicit setting */
		size = def.expansion;
	} else {
		/* default heuristic */
		if (mem < 64)
			size = mb_to_sectors(32);
		else if (mem >= 64 && mem < 128)
			size = mb_to_sectors(64);
		else if (mem >= 128 && mem < 512)
			size = mb_to_sectors(128);
		else
			size = mb_to_sectors(256);

		/* apply disk percentage constraint if applicable */
		if (dp != NULL && disk_okay(dp) &&
				sdisk_geom_not_null(dp)) {
			dsize = 0.2 * usable_sdisk_blks(dp);

			if (size > dsize)
				size = dsize;
		}

		/* apply floor/ceiling constraints */
		if (size > SWAP_MAX)
			size = SWAP_MAX;
		else if (size < SWAP_MIN)
			size = SWAP_MIN;
	}

	return (size);
}

/*
 * _dflt_fs_space()
 *	Determine the default file system size required. Default expansion
 *	overhead should only be added to the non-explicit component of the
 *	size.
 * Parameters:
 *	fs	- non-NULL name of default mount point
 *	roll	- DONTROLLUP	- don't include DONTCARE children of 'mntpnt'
 *		  ROLLUP	- include DONTCARE children of 'mntpnt'
 * Return:
 *	# >= 0	- number of sectors
 * Status:
 *	private
 */
static int
_dflt_fs_space(char *fs, int roll)
{
	static Defmnt_t	**mpp = NULL;
	Defmnt_t	def;
	float		pf;
	int		size;
	int		exp;
	int		i;
	MachineType	type;
	

	/* validate parameters */
	if (fs == NULL || (roll != ROLLUP && roll != DONTROLLUP))
		return (0);

	/* load the current space data */
	update_dfltmnt_list();

	/* get the default mount entry for this file system */
	if (get_dfltmnt_ent(&def, fs) != D_OK || def.status == DFLT_IGNORE)
		return (0);

	/* initialize the size, expansion, and overhead variables */
	size = (def.size > 0 ? def.size : 0);
	exp = (def.expansion > 0 ? def.expansion : 0);
	pf = percent_free_space();

	/* process DFLT_DONTCARE entries directory */
	if (def.status == DFLT_DONTCARE) {
		if (roll == ROLLUP)
			size = 0;
		else {
			/*
			 * The over head is a percentage of total file
			 * system size. So it is going to be some fraction
			 * of the overal software's size.  It is exactly
			 * 1/(1-(percent_free/100))
			 */
			size = ((float)size / (1 - (float) (pf/100)));
			size += exp;
		}

		return (size);
	}

	/*
	 * we know def.status == DFLT_SELECT at this point; include the
	 * space for all default mount points which 'fs' is an immediate
	 * ancestor, and which are not themselves DFLT_SELECT; take the
	 * rollup parameter into account when making the determination
	 */
	mpp = get_dfltmnt_list(mpp);
	type = get_machinetype();
	
	for (i = 0; mpp[i]; i++) {
		/* dataless systems only include "/" and "/var" */
		if (type == MT_DATALESS &&
				strneq(mpp[i]->name, ROOT) &&
				strneq(mpp[i]->name, VAR))
			continue;

		/*
		 * space for itself accounted for at size variable
		 * initialization
		 */
		if (streq(fs, mpp[i]->name))
			continue;

		/* only count DONTCARE children if we are rolling up */
		if (roll == DONTROLLUP && mpp[i]->status == DFLT_DONTCARE)
			continue;

		/*
		 * see if this is a dependent child, and if so, add its
		 * space
		 */
		if (_fs_immed_ancestor(fs, mpp[i]->name)) {
			if (mpp[i]->size > 0)
				size += mpp[i]->size;

			if (mpp[i]->expansion > 0)
				exp += mpp[i]->expansion;
		}
	}

	/*
	 * The over head is a percentage of total file system size. So it
	 * is going to be some fraction of the overal software's size.  It
	 * is exactly 1/(1-(percent_free/100))
	 */
	size = ((float)size / (1 - (float) (pf/100)));
	size += exp;

	/* default file systems have a minimum size */
	if (size < MINFSSIZE)
		size = MINFSSIZE;

	return (size);
}

/*
 * _dflt_cache_space()
 *	Calculate the default cache file system space.
 *	(1)  Look on the boot disk to see if there is sufficient usable
 *	     space to fit a minimum sized /.cache
 *	(2)  If '(1)' fails, look across all disks to find the largest
 *	     available segment of disk
 * Parameters:
 *	disk	- pointer to specific disk against which the default should
 *		  be calculated (NULL to invoke "biggest hole" algorithm)
 * Return:
 *	# >= 0 	- size in sectors
 * Status:
 *	private
 */
static int
_dflt_cache_space(Disk_t *disk)
{
	Disk_t		*dp;
	int		max = -1;
	int		min;
	int		avail;
	MachineType	mt = get_machinetype();

	/* /.cache is only supported on CacheOS systems */
	if (mt != MT_CCLIENT)
		return (0);

	min = _min_cache_space();

	/*
	 * if the user specified and explicit disk, use it;
	 * otherwise, use the default boot disk (if there is
	 * one
	 */
	if (disk == NULL)
		dp = find_bootdisk();
	else
		dp = disk;

	/*
	 * if there is a disk at this point, check to see if
	 * it has enough contiguous space to meet minimum
	 * requirements; this is the "disk of preference"
	 */
	if (dp != NULL) {
		if (sdisk_is_usable(dp)) {
			avail = sdisk_max_hole_size(dp);
			if (avail >= min)
				return (avail);
		}
	}

	/*
	 * if the user did not explicitly specify a disk to use
	 * to determine the default /.cache size, and the boot
	 * disk either didn't exist or didn't have enough space
	 * to meet minimum requirements, go find the disk that
	 * has the most contiguous space avaialble
	 */
	if (disk == NULL) {
		WALK_DISK_LIST(dp) {
			if (sdisk_is_usable(dp)) {
				avail = sdisk_max_hole_size(dp);
				if (avail > max)
					max = avail;
			}
		}
	}

	/*
	 * make sure you return at least the minimum required space
	 * as a default value, even if there isn't a disk big enough
	 * to hold it
	 */
	if (max < min)
		max = min;

	return (max);
}

/*
 * _fs_absorb_children()
 *	The file system was allocated space; update the dfltmnt mask to mark
 *	it as DFLT_SELECT, and either absorb or spin off any DFLT_DONTCARE
 *	children.
 * Parameters:
 *	fs	- file system absorbing its children
 *	roll	- DONTROLLUP	- don't include DONTCARE children of 'fs'
 *		  ROLLUP	- include DONTCARE children of 'fs'
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_fs_absorb_children(char *fs, int roll)
{
	static Defmnt_t	**mpp = NULL;
	int	i;

	mpp = get_dfltmnt_list(mpp);
	for (i = 0; mpp[i]; i++) {
		if (_fs_immed_ancestor(fs, mpp[i]->name) == 0)
			continue;

		if (mpp[i]->status == DFLT_DONTCARE) {
			if (roll == DONTROLLUP)
				mpp[i]->status = DFLT_SELECT;
			else
				mpp[i]->status = DFLT_IGNORE;

			if (get_trace_level() > 2) {
				write_status(SCR, LEVEL1|LISTITEM,
				"Child file system status changed (%s) (%d)",
					mpp[i]->name,
					mpp[i]->status);
			}

			(void) set_dfltmnt_ent(mpp[i], mpp[i]->name);
		}
	}
}
