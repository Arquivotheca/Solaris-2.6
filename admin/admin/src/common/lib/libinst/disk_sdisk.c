#ifndef lint
#pragma ident   "@(#)disk_sdisk.c 1.115 95/09/01 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyrigh
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Governmen
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
/*
 * MODULE PURPOSE:	This module contains routines used to manipulate
 *			the S-disk structure
 */
#include "disk_lib.h"

/* Local Statics and Constants */

static int	_slice_autoadjust = 1;	/* slice autoadjust "on" by default */

/* Public Function Prototypes */

int		set_slice_mnt(Disk_t *, int, char *, char *);
int		set_slice_geom(Disk_t *, int, int, int);
int		set_slice_preserve(Disk_t *, int, int);
int		set_slice_ignore(Disk_t *, int, int);

int		slice_preserve_ok(Disk_t *, int);
int		slice_name_ok(char *);
int		slice_overlaps(Disk_t *, int, int, int, int **);

int		sdisk_space_avail(Disk_t *);
int		sdisk_max_hole_size(Disk_t *);
void		sdisk_use_free_space(Disk_t *);
int		sdisk_compare(Disk_t *, Label_t);
int		sdisk_geom_same(Disk_t *, Label_t);
int		sdisk_config(Disk_t *, char *, Label_t);

int 		adjust_slice_starts(Disk_t *);
int		filesys_preserve_ok(char *);
int		find_mnt_pnt(Disk_t *, char *, char *, Mntpnt_t *, Label_t);
int		get_slice_autoadjust(void);
int		set_slice_autoadjust(int);
int		get_mntpnt_size(Disk_t *, char *, char *, Label_t);
int		swap_size_allocated(Disk_t *, char *);

/* Library Function Prototypes */

int		_set_slice_start(Disk_t *, int, int);
int		_set_slice_size(Disk_t *, int, int);
int		_reset_sdisk(Disk_t *);
int		_null_sdisk(Disk_t *);
int		_reset_slice(Disk_t *, int);

/* Local Function Prototypes */

static int	_slice_cyl_block(Disk_t *, int);
static int	_prec_free_sectors(Disk_t *, int, int);
static int	_prec_loose_slices(Disk_t *, int, int, int *);
static int	_make_separate_fs(Disk_t *, int, int);
static int	_absorb_preferred_space(Disk_t *, int, int, int *);
static void	_absorb_general_space(Disk_t *, int, int, int *);
static void	_find_unused_fsname(char *);
static int	_find_unused_slice(Disk_t *, int);
static int 	_adjust_slice_start(Disk_t *, int);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * sdisk_geom_same()
 *	On fdisk systems, the Solaris disk geometry is dependent upon
 *	the Solaris fdisk partition geometry. Several conditions (such
 *	as the preservability of existing slices) are dependent upon
 *	whether or not the Solaris disk geometry has been altered. This
 *	function compares the current sdisk geometry against either
 *	the committed or existing sdisk geometry in search of changes.
 *	On non-fdisk systems, this routine always returns D_OK.
 *
 *	ALGORITHM:
 *	(1) Get the Solaris partition indexes for the current and
 *	    committed/existing configurations
 *	(2) If neither had a Solaris partition, then return D_OK
 *	(3) If one had a Solaris partition, but the other didn't,
 *	    then return D_GEOMCHNG
 *	(4) If both had a Solaris partition, then compare the structures,
 *	    and if they differ in any way, return D_GEOMCHNG, otherwise,
 *	    return D_OK
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 *	label	- specify against which geometry the current geometry
 *		  should be compared (CFG_COMMIT or CFG_EXIST)
 * Return:
 *	D_OK		- S-disk geometry unchanged
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk state not applicable to this operation
 *	D_BADARG	- 'label' wasn't CFG_EXIST or CFG_COMMIT
 *	D_GEOMCHNG	- S-disk geometry changed since disk last committed
 * Status:
 *	public
 */
int
sdisk_geom_same(Disk_t *dp, Label_t label)
{
	int	pid;
	int	cpid;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_no_fdisk_exists(dp))
		return (D_OK);

	cpid = get_solaris_part(dp, CFG_CURRENT);
	switch (label) {
	    case  CFG_EXIST:
		pid = get_solaris_part(dp, CFG_EXIST);
		break;
	    case CFG_COMMIT:
		pid = get_solaris_part(dp, CFG_COMMIT);
		break;
	    default:
		return (D_BADARG);
	}

	/* there wasn't a Solaris partition before and there isn't one now */
	if (cpid == 0 && pid == 0)
		return (D_OK);

	/*
	 * if a Solaris partition existed in one configuration but not the
	 * other, then there was a geometry change
	 */
	if ((cpid == 0 && pid > 0) || (cpid > 0 && pid == 0))
		return (D_GEOMCHNG);

	/*
	 * if both configurations had a Solaris partition, compare all fields
	 * of the two geometries
	 */
	switch (label) {
	    case CFG_EXIST:
		if (memcmp(part_geom_addr(dp, cpid),
				orig_part_geom_addr(dp, pid),
				sizeof (Geom_t)) != 0)
			return (D_GEOMCHNG);
		break;

	    case CFG_COMMIT:
		if (memcmp(part_geom_addr(dp, cpid),
				comm_part_geom_addr(dp, pid),
				sizeof (Geom_t)) != 0)
			return (D_GEOMCHNG);
	}

	return (D_OK);
}

/*
 * slice_overlaps()
 *	See if a slice starting at 'start' and of 'size' sectors in size
 *	overlaps another slice which has a mount point other than "overlap".
 *	Returns '0' if 'dp' is illegal or the disk state is not "okay".
 *	Repeated calls to this function will overwrite the current 'olpp'
 *	array values.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 *	slice	- slice being validated (-1 if no slice specified)
 *	start	- proposed starting cylinder for slice being validated
 *	size	- proposed sector count size for slice being validated
 *	olpp	- address to an integer pointer initialized to the address
 *		  of an integer array containing a list of slices which
 *		  were illegally overlapped (number of valid entries in
 *		  the array is the function return value). If NULL, no
 *		  array pointer is desired.
 * Return:
 *	#	- number of illegal overlaps found (also an index limit
 *		  for the 'overlaps' array)
 * Status:
 *	public
 */
int
slice_overlaps(Disk_t *dp, int slice, int start, int size, int **olpp)
{
	static int  _overlap_slices[16];
	int	f;
	int	send;
	int	fstart;
	int	fend;
	int	count = 0;

	if (dp == NULL || disk_not_okay(dp) ||
			sdisk_geom_null(dp) || sdisk_not_legal(dp))
		return (0);

	send = start + blocks_to_cyls(dp, size);

	WALK_SLICES(f) {
		if (f == slice || slice_size(dp, f) == 0 ||
				slice_is_overlap(dp, f))
			continue;

		fstart = slice_start(dp, f);
		fend = fstart + blocks_to_cyls(dp, slice_size(dp, f));

		if ((fstart >= start && fstart < send) ||
				(start >= fstart && start < fend))
			_overlap_slices[count++] = f;
	}

	if (olpp != NULL)
		*olpp = _overlap_slices;

	return (count);
}

/*
 * slice_preserve_ok()
 * 	Check if preserve allowed on this slice. Preserve is not permitted if
 *	the file system name is reserved (see filesys_preserve_ok()), the
 *	slice size is '0', the geometry of the slice has changed from the
 *	original configuration, or the geometry was aligned at load time.
 * Parameters:
 *	dp	- disk structure pointer
 *	slice	- slice index number
 * Return:
 *	D_OK	   - disk is permitted to be preserved
 *	D_NODISK   - 'dp' is NULL
 *	D_BADDISK  - disk state not valid for requested operation
 *	D_LOCKED   - cannot change attributes of a locked slice
 *	D_CHANGED  - the slice parameters (start/size) have been
 *		     changed already, so can't preserve)
 *	D_CANTPRES - disk is not permitted to be preserved
 *	D_ILLEGAL  - failed because original S-disk is in an 'illegal' state
 *	D_IGNORED  - slice is ignored and cannot be changed
 *	D_GEOMCHNG - disk geom change prohibits restore
 *	D_ALIGNED  - the starting cylinder / size were aligned at load
 * Status:
 *	public
 */
int
slice_preserve_ok(Disk_t *dp, int slice)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (slice_locked(dp, slice))
		return (D_LOCKED);

	if (orig_sdisk_not_legal(dp))
		return (D_ILLEGAL);

	if (slice_ignored(dp, slice))
		return (D_IGNORED);

	if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
		return (D_GEOMCHNG);

	if (disk_initialized(dp)) {
		if ((slice_start(dp, slice) !=
				orig_slice_start(dp, slice)) ||
				(slice_size(dp, slice) !=
				orig_slice_size(dp, slice)))
			return (D_CHANGED);
	}

	if (orig_slice_aligned(dp, slice))
		return (D_ALIGNED);

	return (filesys_preserve_ok(slice_mntpnt(dp, slice)));
}

/*
 * filesys_preserve_ok()
 * 	Check if preserve is allowed on a specific file system name.
 *	The following mount point specifiers cannot be preserved:
 *		/
 *		/usr
 *		/var
 * Parameters:
 *	fs	   - file system name
 * Return:
 *	D_OK	   - slice name is permitted to be preserved
 *	D_CANTPRES - slice name is not permitted to be preserved
 * Status:
 *	public
 */
int
filesys_preserve_ok(char * fs)
{
	static char *	_fs_cant_pres[] = { ROOT, "/usr", "/var", NULL };
	int		i;

	for (i = 0; _fs_cant_pres[i]; i++) {
		if (strcmp(fs, _fs_cant_pres[i]) == 0)
			return (D_CANTPRES);
	}

	return (D_OK);
}

/*
 * slice_name_ok()
 * 	Check if the name specified is a legal slice name. Legal slice names
 *	begin with '/', or are "swap", "", "alts", or "overlap".
 * Parameters:
 *	fs	 - file system name
 * Return:
 *	D_OK	 - 'fs' is an acceptable mountpoint name
 *	D_BADARG - 'fs' is not an acceptable mountpoint name
 * Status:
 *	public
 */
int
slice_name_ok(char *fs)
{
	char 	*cp;

	if (*fs == '\0' || strcmp(fs, OVERLAP) == 0 ||
			strcmp(fs, SWAP) == 0 || strcmp(fs, ALTSECTOR) == 0)
		return (D_OK);

	if (fs[0] != '/')
		return (D_BADARG);

	for (cp = fs; *cp; cp++) {
		if (!isalnum(*cp) && strchr("/.,-_", *cp) == NULL)
			return (D_BADARG);
	}

	return (D_OK);
}

/*
 * sdisk_config()
 *	Set the sdisk structure state according to 'label'. This routine returns
 *	in error if requested to restore the existing or committed states and
 *	there was an sdisk geometry change (e.g. SOLARIS partition changed)
 *	between the current and original (or committed) configuration.
 * Parameter:
 *	disk	- disk structure pointer (NULL if specifying drive by
 *		  'drive'). 'dp' has precedence in the drive order
 *		  (state:  okay, selected)
 *	drive	- name of drive (e.g. c0t0d0)(NULL if specifying drive
 *		  by 'dp')
 *		  (state:  okay, selected)
 *	label	- configuration strategy specifier:
 *			CFG_NONE    - reset S-disk to a min config
 *			CFG_DEFAULT - default config
 *			CFG_EXIST   - original S-disk config
 *			CFG_COMMIT  - last committed S-Disk config
 * Return:
 *	D_OK	    - disk state set successfully
 *	D_NODISK    - neither argument was specified
 *	D_BADDISK   - disk state not valid for requested operation
 *	D_BADARG    - illegal 'label' value
 *	D_NOTSELECT - disk state was not selected
 *	D_GEOMCHNG  - disk geom change prohibits restore
 * Status:
 *	public
 */
int
sdisk_config(Disk_t *disk, char *drive, Label_t label)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	/* only selected drives can be configured */
	if (disk_not_selected(dp))
		return (D_NOTSELECT);
	/*
	 * just ignore disks which don't have an sdisk geometry (i.e. no
	 * Solaris partition or not fdisk configuration not validated)
	 */
	if (sdisk_geom_null(dp))
		return (D_OK);

	switch (label) {
	    case CFG_NONE:
		return (_reset_sdisk(dp));

	    case CFG_DEFAULT:
		return (_setup_sdisk_default(dp));

	    case CFG_EXIST:
		return (_restore_sdisk_orig(dp));

	    case CFG_COMMIT:
		return (_restore_sdisk_commit(dp));
	}

	return (D_BADARG);
}

/*
 * set_slice_geom()
 *	Set the starting cylinder and/or size for 'slice'. 'size' parameters
 *	are automatically rounded to cylinder boundaries. Once the geometry
 *	has been set, starting cylinders are automatically adjusted across the
 *	drive. Disks specified must not be in a "bad" state, and must be
 *	selected.
 * Parameters:
 *	dp	- valid disk structure pointer for selected disk
 *		  (state: okay, selected)
 *	slice	- slice index
 *		  (state: not locked)
 *	start	- starting cylinder number, or:
 *			GEOM_ORIG     - original start cylinder
 *			GEOM_COMMIT   - last committed start cylinder
 *			GEOM_IGNORE   - don't do anything with star
 *	size	- number of sectors, or:
 *			GEOM_ORIG     - original slice size
 *			GEOM_COMMIT   - last committed slice size
 *			GEOM_REST     -	whatever isn't already assigned
 *					to another slice
 *			GEOM_IGNORE   - don't do anything with size
 * Return:
 *	D_OK	    - slice geometry set successfully
 *	D_NODISK    - 'dp' is NULL
 *	D_BADDISK   - disk state not valid for requested operation
 *	D_LOCKED    - slice is locked and geometry cannot be changed
 *	D_NOTSELECT - disk state was not selected
 *	D_BADARG    - invalid 'slice'
 *	D_GEOMCHNG  - disk geom change prohibits restore
 *	other	    - return from adjust_slice_start(), _set_slice_geom()
 * Status:
 *	public
 */
int
set_slice_geom(Disk_t *dp, int slice, int start, int size)
{
	Sdisk_t	saved;
	int	status;
	int	modified = 0;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (slice_locked(dp, slice) && disk_initialized(dp))
		return (D_LOCKED);

	if (invalid_sdisk_slice(slice))
		return (D_BADARG);

	switch (start) {
	    case GEOM_ORIG:
		if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
			return (D_GEOMCHNG);

		start = orig_slice_start(dp, slice);
		break;

	    case GEOM_COMMIT:
		if (sdisk_geom_same(dp, CFG_COMMIT) != D_OK)
			return (D_GEOMCHNG);

		start = comm_slice_start(dp, slice);
		break;

	    case GEOM_IGNORE:
		break;
	}

	switch (size) {
	    case GEOM_ORIG:
		if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
			return (D_GEOMCHNG);

		size = orig_slice_size(dp, slice);
		break;

	    case GEOM_COMMIT:
		if (sdisk_geom_same(dp, CFG_COMMIT) != D_OK)
			return (D_GEOMCHNG);

		size = comm_slice_size(dp, slice);
		break;

	    case GEOM_REST:
	    case GEOM_IGNORE:
		break;
	}

	(void) memcpy(&saved, disk_sdisk_addr(dp), sizeof (Sdisk_t));

	/*
	 * the size must be set before the start, since if the size is '0',
	 * then _set_slice_size() automatically resets the start to '0', and
	 * _set_slice_start() will fail if start is 0 and slice_size is not 0
	 */
	if (size != GEOM_IGNORE && size != slice_size(dp, slice)) {
		modified++;
		if ((status = _set_slice_size(dp, slice, size)) != D_OK) {
			(void) memcpy(disk_sdisk_addr(dp),
					&saved, sizeof (Sdisk_t));
			return (status);
		}
	}

	if (start != GEOM_IGNORE && start != slice_start(dp, slice)) {
		modified++;
		if ((status = _set_slice_start(dp, slice, start)) != D_OK) {
			(void) memcpy(disk_sdisk_addr(dp),
					&saved, sizeof (Sdisk_t));
			return (status);
		}
	}

	if (modified && get_slice_autoadjust()) {
		if ((status = adjust_slice_starts(dp)) != D_OK) {
			(void) memcpy(disk_sdisk_addr(dp),
					&saved, sizeof (Sdisk_t));
			return (status);
		}
	}

	return (D_OK);
}

/*
 * get_slice_autoadjust()
 *	Get the current "autoadjust" flag to '1' (enabled) or '0' (disabled).
 * Parameters:
 *	none
 * Return:
 *	0/1	- old value of autoadjust variable
 * Status:
 *	public
 */
int
get_slice_autoadjust(void)
{
	return (_slice_autoadjust);
}

/*
 * set_slice_autoadjust()
 *	Change the "autoadjust" flag to '1' (enabled) or '0' (disabled).
 * Parameters:
 *	val	- 0 (don't autoadjust when any slice geom changes), or
 *		  1 (adjust automatically when any slice geom changes)
 * Return:
 *	0/1	- old value of autoadjust variable
 * Status:
 *	public
 */
int
set_slice_autoadjust(int  val)
{
	int	i = _slice_autoadjust;

	_slice_autoadjust = (val == 0 ? 0 : 1);
	return (i);
}

/*
 * adjust_slice_starts()
 *	Adjust the starting cylinders for all slices on the disk 'dp' which
 *	are not locked, preserved, stuck, or overlap. Consume any gaps which
 *	may exist between a given slice and the slice which precedes it. If
 *	the routine fails, the pre-call slice configuration is restored before
 *	returning, so there are no side-effects of a failure.
 * Parameters:
 *	dp	- valid disk structure pointer for selected disk no
 *		  in "bad" state
 *		  (state:  okay, selected)
 * Return:
 *	D_OK		- adjusts successful
 *	D_NODISK	- 'dp' is NULL
 *	D_BADARG	- slice 'i' on disk 'dp' can't be adjusted
 *	D_BADDISK	- disk not available for specified operation
 *	D_NOTSELECT	- disk state was not selected
 *	D_NOSPACE	- couldn't adjust because of insufficient space
 *	D_NOFIT		- slice sizes were too big to fit in their S-disk
 *			  segments
 * Status:
 *	public
 */
int
adjust_slice_starts(Disk_t *dp)
{
	Sdisk_t	save;
	int	status;
	int	sstart;
	int	fstart;
	int	send;
	int	i;
	int	f;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	(void) memcpy(&save, disk_sdisk_addr(dp), sizeof (Sdisk_t));

	WALK_SLICES(i) {
		if ((status = _adjust_slice_start(dp, i)) != D_OK) {
			(void) memcpy(disk_sdisk_addr(dp),
					&save, sizeof (Sdisk_t));
			return (status);
		}
	}

	/*
	 * check to see if one of the preserved/lock/stuck FS got overrun by
	 * another slice
	 */
	WALK_SLICES(i) {
		if (slice_size(dp, i) == 0 || slice_is_overlap(dp, i))
			continue;

		sstart = slice_start(dp, i);
		send = sstart + blocks_to_cyls(dp, slice_size(dp, i));
		WALK_SLICES(f) {
			if (i == f)
				continue;

			if (slice_size(dp, f) == 0 || slice_is_overlap(dp, f))
				continue;

			fstart = slice_start(dp, f);
			if (send > fstart && sstart <= fstart)
				return (D_NOFIT);
		}
	}

	return (D_OK);
}

/*
 * find_mnt_pnt()
 *	Determines if 'fs' is found on a disk. If 'disk' (or 'drive') are
 *	specified, the search is restricted to that disk (in this case the
 *	disk does not have to be in a "selected" state). Otherwise, all disks
 *	in entire disk which are marked "selected" and with an "okay" state are
 *	searched. If a match is found, 'info.dp' is set to the disk structure
 *	address, and 'info.slice' is set to the slice number (unless 'info' is
 *	a NULL pointer).
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive') - 'disk' has precedence over 'drive'
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 *	mnt_pnt	- mount point name used in the search
 *	info	- pointer to mnt_pnt structure used to retrieve
 *		  disk/slice data (NULL if no info retrieval is desired)
 *	label	- specifies which configuration set to search. Allowable
 *		  values are: CFG_CURRENT, CFG_COMMIT, CFG_EXIST
 * Returns:
 *	1 	- 'fs' was found on a disk
 *	0 	- 'fs' was NOT found
 * Status:
 *	public
 */
int
find_mnt_pnt(Disk_t *disk, char *drive, char *fs,
			Mntpnt_t *info, Label_t label)
{
	Disk_t	*dp = NULL;
	int	i;

	if (fs == NULL)
		return (0);

	if (disk != NULL)
		dp = disk;
	else if (drive != NULL) {
		if ((dp = find_disk(drive)) == NULL)
			return (0);
	}

	if (dp == NULL) {
		for (dp = first_disk(); dp; dp = next_disk(dp)) {
			if (disk_not_okay(dp) || disk_not_selected(dp))
				continue;

			switch (label) {
			    case CFG_CURRENT:
				if (sdisk_geom_null(dp) ||
						sdisk_not_legal(dp))
					continue;

				WALK_SLICES(i) {
					if (slice_ignored(dp, i))
						continue;

					if (strcmp(fs, slice_mntpnt(dp,
							i)) == 0) {
						if (info != NULL) {
							info->dp = dp;
							info->slice = i;
						}

						return (1);
					}
				}

				break;

			    case   CFG_EXIST:
				if (orig_sdisk_geom_null(dp) ||
						orig_sdisk_not_legal(dp))
					continue;

				WALK_SLICES(i) {
					if (slice_ignored(dp, i))
						continue;

					if (strcmp(fs, orig_slice_mntpnt(dp,
							i)) == 0) {
						if (info != NULL) {
							info->dp = dp;
							info->slice = i;
						}

						return (1);
					}
				}
				break;

			    case  CFG_COMMIT:
				if (comm_sdisk_geom_null(dp) ||
						comm_sdisk_not_legal(dp))
					continue;

				WALK_SLICES(i) {
					if (comm_slice_ignored(dp, i))
						continue;

					if (strcmp(fs, comm_slice_mntpnt(dp,
							i)) == 0) {
						if (info != NULL) {
							info->dp = dp;
							info->slice = i;
						}

						return (1);
					}
				}
				break;

			    default:
				return (D_BADARG);
			}
		}
	} else {
		if (disk_not_okay(dp))
			return (0);

		WALK_SLICES(i) {
			switch (label) {
			    case CFG_CURRENT:
				if (slice_ignored(dp, i))
					continue;

				if (strcmp(fs, slice_mntpnt(dp, i)) == 0) {
					if (info != NULL) {
						info->dp = dp;
						info->slice = i;
					}

					return (1);
				}

				break;

			    case   CFG_EXIST:
				if (orig_slice_ignored(dp, i))
					continue;

				if (strcmp(fs, orig_slice_mntpnt(dp, i)) == 0) {
					if (info != NULL) {
						info->dp = dp;
						info->slice = i;
					}

					return (1);
				}

				break;

			    case  CFG_COMMIT:
				if (comm_slice_ignored(dp, i))
					continue;

				if (strcmp(fs, comm_slice_mntpnt(dp, i)) == 0) {
					if (info != NULL) {
						info->dp = dp;
						info->slice = i;
					}

					return (1);
				}

				break;

			    default:
				return (D_BADARG);
			}
		}
	}

	return (0);
}

/*
 * set_slice_preserve()
 *	Set the preserve state for 'slice' to 'val' (PRES_YES or PRES_NO). The
 *	geometry of 'slice' must not have changed since the original drive
 *	configuration. '0' sized slices cannot be preserved.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay, selected)
 *	slice	- slice index number on disk
 *	val	- preservation value to which the slice is to be se
 *		  (valid values: PRES_NO, PRES_YES)
 * Return:
 *	D_OK		- slice updated as specified
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk not available for specified operation
 *	D_NOTSELECT	- disk state was not selected
 *	D_LOCKED	- cannot change attributes of a locked slice
 *	D_CHANGED	- the slice start and/or size has changed since
 *			  the original configuration
 *	D_CANTPRES	- attempt to reserve a reserved slice
 *	D_OVER		- can't preserve this slice because it currently
 *			  overlaps with another slice illegally
 *	D_ZERO		- slice size is zero
 *	D_GEOMCHNG	- S-disk geometry changed since disk last committed
 * Status:
 *	public
 */
int
set_slice_preserve(Disk_t *dp, int slice, int val)
{
	int	status;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (slice_locked(dp, slice))
		return (D_LOCKED);

	if (val == PRES_NO) {
		slice_preserve_off(dp, slice);
		return (D_OK);
	}

	if (slice_size(dp, slice) == 0)
		return (D_ZERO);

	if ((status = slice_preserve_ok(dp, slice)) != D_OK)
		return (status);

	if (slice_start(dp, slice) != orig_slice_start(dp, slice) &&
			slice_size(dp, slice) != orig_slice_size(dp, slice) &&
			disk_initialized(dp))
		return (D_CHANGED);

	if (slice_overlaps(dp, slice, slice_start(dp, slice),
			slice_size(dp, slice), (int **)0) != 0 &&
			strcmp(slice_mntpnt(dp, slice), OVERLAP) != 0)
		return (D_OVER);

	slice_preserve_on(dp, slice);
	return (D_OK);
}

/*
 * set_slice_ignore()
 *	Set the ignore state for 'slice' to 'val' (IGNORE_YES or
 *	IGNORE_NO). The geometry of 'slice' must not have changed
 *	since the original drive configuration, and the slice must
 *	not be aligned. '0' sized slices cannot be ignored.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay, selected, legal)
 *	slice	- valid slice index
 *	val	- ignore value to which the slice is to be set
 *		  (valid values: IGNORE_NO, IGNORE_YES)
 * Return:
 *	D_OK		- slice updated as specified
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk not available for specified operation
 *	D_NOTSELECT	- disk state was not selected
 *	D_LOCKED	- cannot change attributes of a locked slice
 *	D_CHANGED	- the slice start and/or size has changed since
 *			  the original configuration
 *	D_ALIGNED	- attempt to ignore an aligned slice
 *	D_ZERO		- attempt to ignore a zero sized slice
 * Status:
 *	public
 */
int
set_slice_ignore(Disk_t *dp, int slice, int val)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (slice_locked(dp, slice))
		return (D_LOCKED);

	if (val == IGNORE_NO) {
		slice_ignore_off(dp, slice);
		return (D_OK);
	}

	if (slice_size(dp, slice) == 0)
		return (D_ZERO);

	if (slice_start(dp, slice) != orig_slice_start(dp, slice) &&
			slice_size(dp, slice) != orig_slice_size(dp, slice) &&
			disk_initialized(dp))
		return (D_CHANGED);

	if (slice_overlaps(dp, slice, slice_start(dp, slice),
			slice_size(dp, slice), (int **)0) != 0 &&
			strcmp(slice_mntpnt(dp, slice), OVERLAP) != 0)
		return (D_OVER);

	if (orig_slice_aligned(dp, slice))
		return (D_ALIGNED);

	slice_ignore_on(dp, slice);
	return (D_OK);
}

/*
 * sdisk_max_hole_size()
 *	Find the largest section of contiguously unused data area on the disk.
 * Parameters:
 *	dp	- non-NULL disk structure pointer for disk
 * Returns:
 *	0	- no space available
 *	#	- number of unallocated usable blocks
 * Status:
 *	public
 */
int
sdisk_max_hole_size(Disk_t *dp)
{
	int	s;
	int	sstart = 0;
	int	send;
	int	f;
	int	hole = 0;

	if (dp == NULL || disk_not_okay(dp) ||
			sdisk_geom_null(dp) || sdisk_not_legal(dp))
		return (0);

	while (sstart < sdisk_geom_lcyl(dp)) {
		/*
		 * look for a slice whose starting cylinder lines up
		 * with the currently potential start of a hole
		 */
		WALK_SLICES(s) {
			if (slice_size(dp, s) > 0 &&
					slice_isnt_overlap(dp, s) &&
					slice_start(dp, s) == sstart) {
				sstart = slice_start(dp, s) +
					blocks_to_cyls(dp,
						slice_size(dp, s));
				break;
			}
		}

		/*
		 * we made it through the slices an none lined up with the
		 * starting sector, so we've found a hole
		 */
		if (invalid_sdisk_slice(s)) {
			send = sdisk_geom_lcyl(dp);
			WALK_SLICES(f) {
				if (slice_size(dp, f) > 0 &&
						slice_isnt_overlap(dp, f) &&
						slice_start(dp, f) < send &&
						slice_start(dp, f) > sstart)
					send = slice_start(dp, f);
			}

			if ((send - sstart) > hole)
				hole = send - sstart;

			sstart = send;
		}
	}

	return (cyls_to_blocks(dp, hole));
}

/*
 * sdisk_space_avail()
 * 	Calculate the number of sectors on 'dp' not allocated to any
 *	slice. All overlap slices are ignored.
 *
 *	WARNING:	start cylinders must be adjusted so there
 *			are no illegal overlaps before calling this
 *			routine, or bogus values will be returned
 * Parameters:
 *	dp	- non-NULL disk structure pointer for disk
 *		  (state: okay, valid geometry pointer)
 * Returns:
 *	0	- no space available
 *	#	- number of unallocated usable blocks
 * Status:
 *	public
 */
int
sdisk_space_avail(Disk_t *dp)
{
	int	slice;
	int	fstart;
	int	fend;
	int	sstart;
	int	send;
	int	size = 0;
	int	f;

	if (dp == NULL || disk_not_okay(dp) ||
			sdisk_geom_null(dp) || sdisk_not_legal(dp))
		return (0);

	WALK_SLICES(slice) {
		/*
		 * ignore slices which don't impact the total or are already
		 * subtracted from the usable_sdisk_blks() calculation
		 */
		if (slice_size(dp, slice) == 0 ||
				slice == BOOT_SLICE ||
				slice == ALT_SLICE ||
				slice_is_overlap(dp, slice))
			continue;

		sstart = slice_start(dp, slice);
		send = sstart + blocks_to_cyls(dp, slice_size(dp, slice));

		WALK_SLICES(f) {
			if (slice_size(dp, f) == 0 ||
					slice == f ||
					slice == BOOT_SLICE ||
					slice == ALT_SLICE ||
					slice_is_overlap(dp, f))
				continue;

			fstart = slice_start(dp, f);
			fend = fstart + blocks_to_cyls(dp, slice_size(dp, f));

			/* 'f' and 'slice' are disjoint */
			if (send <= fstart && sstart >= fend)
				continue;

			/* 'f' subsumes 'slice' so ignore 'slice' */
			if (sstart >= fstart && send <= fend) {
				send = sstart;
				break;
			}

			/* the backend of 'slice' overlapped by 'f' */
			if (send > fstart && send <= fend)
				send = fstart;
		}

		size += cyls_to_blocks(dp, send - sstart);
	}

	/* put a floor on the size */
	if (size >= usable_sdisk_blks(dp))
		return (0);

	return (usable_sdisk_blks(dp) - size);
}

/*
 * set_slice_mnt()
 *	Make the following checks on 'mnt':
 *		- One swap partition per disk
 *		- Unique mount point
 *		- Mount point
 *
 *	Pathname is absolute (except for swap). Reset the preserve flag not
 *	PRES_NO if the file system can't be preserved. If a slice was labelled
 *	"overlap" and is changed to anything else, its size will be
 *	automatically zeroed out if it currently overlap another named (not
 *	"overlap") slice. If 'mnt' is "overlap", 'opts' is ignored and the
 *	mount options field is automatically cleared. Setting the mountpoint
 *	to "/" results in a call to select_bootdrive().
 *
 *	Setting slice 2 to "overlap" (SPARC only) will automatically clear
 *	the "touch2" flag, and the start and size parameters will be set to be
 *	the whole drive.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay)
 *	slice	- slice index number
 *	mnt	- valid slice name
 *	opts	- mount options:
 *			char *	- mount option string
 *			NULL *	- don't modify at all
 *			""	- clear the options field
 * Return:
 *	D_OK	  - operation successful
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - disk not available for specified operation
 *	D_LOCKED  - cannot modify a locked slice
 *	D_ILLEGAL - failed because S-disk is in an 'illegal' state
 *	D_BADARG  - mount point was not 'swap', 'overlap', or 'alts'
 *		    and did not begin with a '/'
 *	D_DUPMNT  - mount point already exists
 *	D_IGNORED - slice is ignored and cannot be changed
 *	D_BOOTFIXED - trying to set "/" and the drive cannot
 *		    be the boot disk
 * Status:
 *	public
 */
int
set_slice_mnt(Disk_t *dp, int slice, char * mnt, char *opts)
{
	Mntpnt_t	tmp;
	int		status;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	/* if this mnt pnt is the same as whats already there, do nothing */
	if (strcmp(slice_mntpnt(dp, slice), mnt) == 0) {
		if (opts == NULL ||
				(slice_mntopts(dp, slice) == NULL &&
					opts == '\0') ||
				(slice_mntopts(dp, slice) != NULL &&
					strcmp(slice_mntopts(dp, slice),
						opts)) == 0)
			return (D_OK);
	}

	if (slice_locked(dp, slice))
		return (D_LOCKED);

	if (sdisk_not_legal(dp))
		return (D_ILLEGAL);

	if (slice_ignored(dp, slice))
		return (D_IGNORED);

	if (slice_name_ok(mnt) != D_OK)
		return (D_BADARG);

	if (strcmp(slice_mntpnt(dp, slice), mnt) != 0) {
		if (*mnt == '/') {
			/* error if pathname and mount point already exist */
			if (find_mnt_pnt(NULL, NULL, mnt,
						&tmp, CFG_CURRENT))
				return (D_DUPMNT);

			/*
			 * setting "/" at this point implies a boot disk
			 * selection after the system is initialized
			 */
			if (strcmp(mnt, ROOT) == 0 &&
						disk_not_bootdrive(dp) &&
						disk_initialized(dp)) {
				if ((status = select_bootdisk(dp, NULL)) !=
						D_OK)
					return (status);
			}
		} else {
			/*
			 * changing slice 2 to "overlap" causes the start and
			 * size geom parameters to be set to the whole drive
			 * automatically
			 */
			if (slice == ALL_SLICE) {
				if (strcmp(mnt, OVERLAP) == 0) {
					if (_reset_slice(dp, slice) == D_OK)
						sdisk_unset_touch2(dp);
				} else
					sdisk_set_touch2(dp);
			}

			slice_mntpnt_clear(dp, slice);
			slice_mntopts_clear(dp, slice);
		}

		/*
		 * if changing it from an overlap slice makes the slice illegal,
		 * zero out the size
		 */
		if (slice_is_overlap(dp, slice)) {
			if (slice_overlaps(dp, slice, slice_start(dp, slice),
						slice_size(dp, slice), NULL)) {
				(void) set_slice_geom(dp, slice,
						GEOM_IGNORE, 0);
			}
		}

		slice_mntpnt_set(dp, slice, mnt);

		/*
		 * if we've changed the slice mountpoint to be a file system
		 * which can't be preserved, and it is currently marked as
		 * preserved, set to PRES_NO.
		 */
		if (slice_preserved(dp, slice)) {
			if (slice_preserve_ok(dp, slice) == D_CANTPRES)
				slice_preserve_off(dp, slice);
		}
	}

	if (opts != NULL) {
		slice_mntopts_clear(dp, slice);
		if (*opts != '\0' && slice_mntpnt_is_fs(dp, slice))
			slice_mntopts_set(dp, slice, xstrdup(opts));
	}

	return (D_OK);
}

/*
 * sdisk_use_free_space()
 *	Try and allocate the unsed free space on disk 'dp' to a file system.
 *	Try to squeeze small pieces into other non-preserved file systems
 *	within its disk section. For larger sections, allocate a separate
 *	/export/home* file system.
 *
 *	NOTE:		if a disk has preserved file systems, this algorithm
 *			will deal with the space in each isolated section of
 *			the disk separately
 *
 *	WARNING:	This routine uses adjust_slice_starts().
 *
 * Parameters:
 *	dp	- disk structure pointer for a selected drive
 * Return:
 *	none
 * Status:
 *	public
 */
void
sdisk_use_free_space(Disk_t *dp)
{
	int	np[16];
	int	ocyl;
	int	ncyl;
	int	fsect;
	int	prec;
	int	exp;
	int	used;
	int	i;
	int	n;
	int	percent;

	if (dp == NULL ||
			disk_not_okay(dp) ||
			sdisk_geom_null(dp) ||
			sdisk_not_legal(dp) ||
			(disk_not_selected(dp) && disk_initialized(dp)))
		return;

	(void) adjust_slice_starts(dp);
	ocyl = ncyl = sdisk_geom_firstcyl(dp);

	while ((ncyl = _slice_cyl_block(dp, ocyl)) >= 0) {
		fsect = _prec_free_sectors(dp, ocyl, ncyl);
		prec = _prec_loose_slices(dp, ocyl, ncyl, np);
		if (prec == 0) {
			if (fsect >= MINFSSIZE)
				(void) _make_separate_fs(dp, ncyl, fsect);
		} else {
			exp = _absorb_preferred_space(dp, prec, fsect, np);
			if (exp > 0) {
				for (used = 0, i = 0; i < prec; i++)
					used += slice_size(dp, np[i]);

				percent = (exp * 100) / used;
				n = -1;
				if (percent > percent_free_space() &&
						exp >= MINFSSIZE) {
					n = _make_separate_fs(dp,
							ncyl, exp);
				}

				if (n < 0) {
					_absorb_general_space(dp,
						prec, exp, np);
				}
			}
		}

		ocyl = ncyl;
	}
}

/*
 * get_mntpnt_size()
 *	Search the current/committed/existing (see 'label) slices
 *	of all selected drives in the disk list for the first occurrence
 *	of a slice with the mount point 'fs'. The size is returned in
 *	512 byte blocks.
 * Parameters:
 *	dp	- disk structure pointer (NULL if specifying drive
 *		  by 'drive') - 'disk' has precedence over 'drive'
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 *	fs	- name of file system being queried
 *	label	- specify which layer of the S-disk to search.
 *		  (valid values: CFG_CURRENT, CFG_COMMIT, CFG_EXIST)
 * Return:
 *	0	- unknown mount point, slice size is 0, or find_mnt_pnt()
 *		  returned in error
 *	# >= 0	- size of 'fs' in 512 byte blocks
 * Status:
 *	public
 */
int
get_mntpnt_size(Disk_t *dp, char *drive, char *fs, Label_t label)
{
	Mntpnt_t	d;

	if (find_mnt_pnt(dp, drive, fs, &d, label))
		return (slice_size(d.dp, d.slice));

	return (0);
}

/*
 * swap_size_allocated()
 *	Add up the number of sectors allocated to slices with the mount point
 *	of "swap". Summations can be done either on a specific drive, or across
 *	all selected drives.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive by
 *		  'drive' or if the whole chain is to be searched).
 *		  'dp' has precedence in the drive order
 *	drive	- name of drive (e.g. c0t0d0) (NULL if specifying drive
 *		  by 'dp' or if the whole chain is to be searched)
 * Return:
 *	0	- no swap configured on specified drives
 *	# > 0	- number of sectors allocated to swap on specfied drives
 * Status:
 *	public
 */
int
swap_size_allocated(Disk_t *disk, char *drive)
{
	Disk_t	*dp = NULL;
	int	i;
	int	sum = 0;

	if (disk != NULL)
		dp = disk;
	else if (drive != NULL)
		if ((dp = find_disk(drive)) == NULL)
			return (0);

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (sdisk_is_usable(dp)) {
				WALK_SLICES_STD(i) {
					if (slice_is_swap(dp, i))
						sum += slice_size(dp, i);
				}
			}
		}
	} else {
		if (sdisk_is_usable(dp)) {
			WALK_SLICES_STD(i) {
				if (slice_is_swap(dp, i))
					sum += slice_size(dp, i);
			}
		}
	}

	return (sum);
}

/*
 * sdisk_compare()
 *	Compare the current state of the Sdisk to either the committed or the
 *	existing state. Return in error if there are differences.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	label	- specify against which geometry the current geometry
 *                should be compared (CFG_COMMIT or CFG_EXIST)
 * Return:
 *	0	- the structures are the same
 *	1	- the structures differ
 * Status:
 *	public
 */
int
sdisk_compare(Disk_t *dp, Label_t label)
{
	int	s;

	if (dp == NULL)
		return (0);

	if (sdisk_geom_same(dp, label) != D_OK)
		return (1);

	switch (label) {
	    case CFG_EXIST:
		if (sdisk_state(dp) != orig_sdisk_state(dp))
			return (1);

		for (s = 0; s <= LAST_STDSLICE; s++) {
			if (memcmp(slice_addr(dp, s),
					orig_slice_addr(dp, s),
					sizeof (Slice_t)) != 0) {
				return (1);
			}
		}
		break;

	    case CFG_COMMIT:
		if (sdisk_state(dp) != comm_sdisk_state(dp))
			return (1);

		for (s = 0; s <= LAST_STDSLICE; s++) {
			if (memcmp(slice_addr(dp, s),
					comm_slice_addr(dp, s),
					sizeof (Slice_t)) != 0) {
				return (1);
			}
		}
		break;
	}
	return (0);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _set_slice_start()
 * 	Set the starting cylinder for 'slice'. This is a low-level routine
 *	which does not verify the "okay" or "selected" status of the disk.
 *	This routine should only be called after disk initialization.
 * Parameters:
 *	dp	- valid disk structure pointer
 *	slice	- slice index number
 *	start	- starting cylinder index number
 * Return:
 *	D_OK		- start set successfully
 *	D_NODISK	- 'dp is NULL
 *	D_BADARG	- invalid cylinder number for 'slice'
 *	D_LOCKED	- slice is locked and cannot be changed
 *	D_PRESERVED	- slice is preserved and cannot be changed
 *	D_IGNORED	- slice is ignored and cannot be changed
 *	D_ILLEGAL       - failed because S-disk is in an 'illegal' state
 * Status:
 *	semi-private (internal library use only)
 */
int
_set_slice_start(Disk_t *dp, int slice, int start)
{
	if (dp == NULL)
		return (D_NODISK);

	if (sdisk_not_legal(dp))
		return (D_ILLEGAL);

	if (disk_initialized(dp)) {
		if (slice_locked(dp, slice))
			return (D_LOCKED);

		if (slice_preserved(dp, slice))
			return (D_PRESERVED);

		if (slice_ignored(dp, slice))
			return (D_IGNORED);

		if (start < 0 || start >= sdisk_geom_lcyl(dp))
			return (D_BADARG);

		if (start < sdisk_geom_firstcyl(dp) &&
				slice != ALL_SLICE &&
				slice_size(dp, slice) > 0)
			return (D_BADARG);
	}

	if (slice == ALL_SLICE && start != 0)
		sdisk_set_touch2(dp);

	slice_start_set(dp, slice, start);
	return (D_OK);
}

/*
 * _set_slice_size()
 *	Set the size of a specified slice to a user supplied sector count.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with valid S-disk geometry
 *		  pointer
 *		  (state: legal)
 *	slice	- slice index being set
 *	newsize	- size to set the slice to, in 512 byte blocks
 * Returns:
 *	D_OK		- size set successfully
 *	D_NODISK  	- 'dp' is NULL
 *	D_BADDISK	- S-disk geometry pointer is NULL
 *	D_BADARG  	- inavalid size specified for 'slice'
 *	D_LOCKED	- slice is locked and cannot be modified
 *	D_PRESERVED  	- slice is preserved and cannot be modified
 *	D_IGNORED	- slice is ignored and cannot be changed
 *	D_NOSPACE 	- size exceeds physical disk limitations
 *	D_ILLEGAL       - failed because S-disk is in an 'illegal' state
 * Status:
 *	semi-private (internal library use only)
 */
int
_set_slice_size(Disk_t *dp, int slice, int newsize)
{
	int	blks;

	if (dp == NULL)
		return (D_NODISK);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (sdisk_not_legal(dp))
		return (D_ILLEGAL);

	if (disk_initialized(dp)) {
		if (slice_locked(dp, slice))
			return (D_LOCKED);
		if (slice_preserved(dp, slice))
			return (D_PRESERVED);
		if (slice_ignored(dp, slice))
			return (D_IGNORED);
	}

	/*
	 * if the size specified is "free", set the block size
	 * to the largest contiguous segment on the disk
	 */
	if (newsize == GEOM_REST) {
		blks = sdisk_max_hole_size(dp);
		if (blks == 0)
			return (D_NOSPACE);
	} else {
		if (newsize == slice_size(dp, slice))
			return (D_OK);

		if (newsize < 0)
			return (D_BADARG);

		/* convert newsize into sectors rounding up to cyl boundry */
		blks = blocks_to_blocks(dp, newsize);
		if (blks > usable_sdisk_blks(dp) ||
				((slice == ALL_SLICE) &&
				(blks > accessible_sdisk_blks(dp))))
			return (D_NOSPACE);
	}

	if (blks == 0) {
		/* set_slice_geom() relies on this behavior */
		if (slice_not_stuck(dp, slice))
			slice_start_set(dp, slice, 0);
	} else {
		if (slice_not_locked(dp, slice) &&
				slice_start(dp, slice) <
				sdisk_geom_firstcyl(dp))
			slice_start_set(dp, slice, sdisk_geom_firstcyl(dp));
	}

	slice_size_set(dp, slice, blks);
	if ((slice == ALL_SLICE) && (blks != accessible_sdisk_blks(dp)))
		sdisk_set_touch2(dp);

	return (D_OK);
}

/*
 * _null_sdisk()
 *	Null out the entire current S-disk structure, restore the geometry
 *	pointer to it's pre-cleared value, and set the lock flag on all
 *	locked system slices.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid S-disk
 *		  geometry pointer
 *		  (state:  okay)
 * Return:
 *	D_OK	  - reset successful
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - disk state not valid for requested operation, or S-disk
 *		    geometry pointer is NULL
 * Status:
 *	semi-private (internal library use only)
 */
int
_null_sdisk(Disk_t *dp)
{
	int	i;
	Geom_t	*gp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	WALK_SLICES(i)
		slice_mntopts_clear(dp, i);

	gp = sdisk_geom(dp);
	(void) memset(disk_sdisk_addr(dp), 0, sizeof (Sdisk_t));
	sdisk_geom_set(dp, gp);
	_lock_unusable_slices(dp);
	return (D_OK);
}

/*
 * _reset_sdisk()
 *	Reset the S-disk state, and for each slice in the S-disk structure,
 *	set all the data fields associated with the specified disk to a default
 *	value. For non-system slices, this means clearing the slice flag, the
 *	mount point name and options field, and setting the start/size to '0'.
 *	For non-system slices, this means setting the mount point name and
 *	start/size appropriately for the slice. A final run is made over all
 *	slices to insure the lock flag is set on all locked slices.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid S-disk geometry
 *		  pointer
 * Return:
 *	D_OK	  - reset successful
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - disk state not valid for requested operation
 * Status:
 *	semi-private (internal library use only)
 */
int
_reset_sdisk(Disk_t *dp)
{
	int	i;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	WALK_SLICES(i)
		(void) _reset_slice(dp, i);

	sdisk_state_clear(dp);
	_lock_unusable_slices(dp);
	return (D_OK);
}

/*
 * _reset_slice()
 *	Reset the state and data fields for a specific slice to a NULL (or
 *	for system slices, default) state.
 *
 *	ALGORITHM:
 *	(1)  clear all the slice specific state flags (except 'locked')
 *	(2)  for non-system slices, set the size and start cylinder to '0'
 *	     and clear the mount point and mount options fields.
 *	(3)  for system slices, set the slice configuration as follows:
 *	     (a)  alternate sector slice (FDISK SYSTEMS ONLY)
 *		  If the suggested alternate sector size is not '0', and
 *		  the disk geometry hasn't changed since load time, and
 *		  there was a non-zero alternate sector slice already
 *		  defined on the drive, then restore the original configuration.
 *		  If the suggested alternate sector size is '0', then set the
 *		  slice size and start cylinder to '0' regardless of the
 *		  original configuration
 *	     (b)  all slice (slice 2)
 *		  set the mount point name to "overlap", the starting cylinder
 *		  to '0', and the size to the total number of blocks on the disk
 *		  (NOTE: this may have to change if the definition of slice 2 is
 *		         altered)
 *	     (c)  boot slice (FDISK SYSTEMS ONLY)
 *		  Set the starting cylinder to '0', and the size to '1' cylinder
 *
 *	NOTE:	This must be called after the S-disk geometry has been set
 *		up and the data values are valid, otherwise the firstcyl etc.
 *		macros will be erroneously NULL.
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid sdisk
 *		  geometry pointer
 *	slice	- index of slice to be reset
 * Return:
 *	D_OK	  - reset successful
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - the S-disk geometry pointer is NULL
 * Status:
 *	semi-private (internal library use only)
 */
int
_reset_slice(Disk_t *dp, int slice)
{
	int	asect;

	if (dp == NULL)
		return (D_NODISK);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	/* clear out user-modifiable slice specific state flags */
	slice_preserve_off(dp, slice);
	slice_stuck_off(dp, slice);
	slice_ignore_off(dp, slice);

	/* clear the mount point name and options (will be reset if needed) */
	slice_mntpnt_clear(dp, slice);
	slice_mntopts_clear(dp, slice);

	switch (slice) {
	    case ALT_SLICE:
		slice_mntpnt_set(dp, slice, ALTSECTOR);
		if ((asect = get_minimum_fs_size(ALTSECTOR, dp,
				DONTROLLUP)) > 0) {
			if (orig_slice_size(dp, slice) > 0 &&
				    sdisk_geom_same(dp, CFG_EXIST) == D_OK) {
				slice_start_set(dp, slice,
					orig_slice_start(dp, slice));
				slice_size_set(dp, slice,
					orig_slice_size(dp, slice));
			} else {
				slice_start_set(dp, slice,
					sdisk_geom_firstcyl(dp));
				slice_size_set(dp, slice, asect);
			}
		}
		break;

	    case ALL_SLICE:
		slice_mntpnt_set(dp, slice, OVERLAP);
		slice_start_set(dp, slice, 0);
		slice_size_set(dp, slice, accessible_sdisk_blks(dp));
		sdisk_unset_touch2(dp);
		break;

	    case BOOT_SLICE:
		slice_mntpnt_clear(dp, slice);
		slice_start_set(dp, slice, 0);
		slice_size_set(dp, slice, one_cyl(dp));
		break;

	    default:
		slice_start_set(dp, slice, 0);
		slice_size_set(dp, slice, 0);
		break;
	}

	return (D_OK);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _slice_cyl_block()
 *	Find the next cylinder following 'cyl' which is the starting cylinder
 *	of a fixed slice (i.e. locked/preserved/stuck/ignored) which would
 *	potentially block preceding slices from sliding forward down the
 *	disk. Slices of size '0' and of type 'overlap' should be ignored,
 *	since they don't block anything.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with valid S-disk
 *		  geometry pointer
 *	cyl	- cylinder which is the lower end of search
 * Return:
 *	# >= 0	- starting cylinder number of nearest (lowest start) fixed
 *		  slice which starts after 'cyl'
 *	-1	- cyl == last cylinder on disk
 * Status:
 *	private
 */
static int
_slice_cyl_block(Disk_t *dp, int cyl)
{
	int	i;
	int	last;

	if (dp == NULL || sdisk_geom_null(dp))
		return (0);

	last = sdisk_geom_lcyl(dp);
	if (cyl == last)
		return (-1);

	WALK_SLICES(i) {
		if (slice_is_overlap(dp, i) || slice_size(dp, i) == 0)
			continue;

		if (slice_start(dp, i) <= cyl ||
				slice_start(dp, i) >= last)
			continue;

		if (slice_preserved(dp, i) ||
				slice_locked(dp, i) ||
				slice_ignored(dp, i) ||
				slice_stuck(dp, i))
			last = slice_start(dp, i);
	}

	return (last);
}

/*
 * _prec_free_sectors()
 *	Determine the number of sectors which are unallocated within
 *	a specified region of the disk. Sectors contained within overlap
 *	slices are not considered to be allocated.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid S-disk
 *		  geometry pointer
 *	lowcyl	- lower cylinder boundary of search area on disk
 *	highcyl - upper cylinder boundary of search area on disk
 * Return:
 *	# >= 0	- number of sectors which are unallocate within the
 *		  specfied region of the disk
 * Status:
 *	private
 */
static int
_prec_free_sectors(Disk_t *dp, int lowcyl, int highcyl)
{
	int	count, i, begin, end, send;

	if (dp == NULL || sdisk_geom_null(dp))
		return (0);

	count = 0;
	begin = lowcyl;

	while (begin < highcyl) {
		WALK_SLICES(i) {
			if (slice_is_overlap(dp, i) ||
					slice_size(dp, i) == 0)
				continue;

			send = slice_start(dp, i) +
					blocks_to_cyls(dp, slice_size(dp, i));

			if (slice_start(dp, i) == begin)
				begin = (send > highcyl ? highcyl : send);
		}

		end = highcyl;
		WALK_SLICES(i) {
			if (slice_is_overlap(dp, i) ||
					slice_size(dp, i) == 0)
				continue;

			if (slice_start(dp, i) < end &&
					slice_start(dp, i) >= begin)
				end = slice_start(dp, i);
		}

		count += cyls_to_blocks(dp, end - begin);
		begin = end;
	}

	return (count);
}

/*
 * _prec_loose_slices()
 *	Create an array of slice indexes which represent those slices
 *	which are not fixed (i.e. stuck, preserved, locked, explicit,
 *	or ignored), which are not overlaps, and which have a starting cylinder
 *	greater than 'lowcyl' and less than 'highcyl'. This array comprises
 *	a list of slices which are viable recipients of a block of unallocated
 *	sectors located on the disk between 'lowcyl' and 'highcyl'.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid S-disk
 *		  geometry pointer
 *	lowcyl	- cylinder specifying the lower boundary of the area
 *	highcyl	- cylinder specifying the upper boundary of the area
 *	sp	- pointer to a caller defined array of integers; used to
 *		  retrieve slice index list. The array should have NUMPARTS
 *		  elements.
 * Return:
 *	# >= 0	- number of slice indexes placed in the 'sp' array
 * Status:
 *	private
 */
static int
_prec_loose_slices(Disk_t *dp, int lowcyl, int highcyl, int *sp)
{
	int	i;
	int	count = 0;

	if (dp == NULL || sdisk_geom_null(dp))
		return (0);

	WALK_SLICES(i) {
		if (slice_size(dp, i) == 0 ||
				slice_is_overlap(dp, i) ||
				slice_start(dp, i) < lowcyl ||
				slice_start(dp, i) > highcyl ||
				slice_stuck(dp, i) ||
				slice_explicit(dp, i) ||
				slice_preserved(dp, i) ||
				slice_ignored(dp, i) ||
				slice_locked(dp, i)) {
			continue;
		}

		sp[count++] = i;
	}

	return (count);
}

/*
 * _absorb_preferred_space()
 *	Distribute a user specified number of unallocated (free) sectors
 *	amongst a set of existing, non-static, slices. The list of possible
 *	recipient slices is passed in as a parameter. The algorithm for
 *	determining which slices get home much of the unallocated space is
 *	as follows:
 *
 *	ALGORITHM:
 *	(1)  if any of the candidate recipient slices is "/", "/var", or
 *	     "/opt", then divide up to 100MB between them ('/' only if
 *	     there is no separate '/var' and it is a server), otherwise,
 *	(2)  if there is a separate '/export', dump the rest of the
 *	     free space there.
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a non-NULL S-disk
 *		  geometry pointer
 *	cnt	- size of 'np' array
 *	free	- number of free sectors
 *	np	- array of indexes for viable slices which may legally be
 *		  allocated the specified free sectors
 * Return:
 *	# >= 0	- number of free sectors remaining
 * Status:
 *	private
 */
static int
_absorb_preferred_space(Disk_t *dp, int cnt, int free, int * np)
{
	int	 remain, add;
	int	 i, j;
	int	 ep[16 + 1];
	int	 shortfall;
	int	 def;

	if (dp == NULL || sdisk_geom_null(dp))
		return (free);

	remain = free;

	/*
	 * if there is still unused space, make sure SWAP is its full
	 * default size (without the disk constraint, but rounded to
	 * the drive's cylinder boundaries). Make sure swap is a
	 * viable candidate to receive free space
	 */
	if (remain > 0) {
		for (i = 0; i < cnt; i++) {
			if (slice_is_swap(dp, np[i]))
				break;
		}
		if (i < cnt) {
			/*
			 * get unconstrained size and then cylinder align it
			 */
			def = get_default_fs_size(SWAP, NULL, DONTROLLUP);
			def = blocks_to_blocks(dp, def);
			shortfall = def - slice_size(dp, np[i]);
			if (shortfall > 0) {
				if (shortfall > remain)
					shortfall = remain;

				if (set_slice_geom(dp, np[i], GEOM_IGNORE,
						slice_size(dp, np[i]) +
							shortfall) == D_OK)
					remain -= shortfall;
			}
		}
	}

	/*
	 * if there is still unused space, add it to '/var', '/opt',
	 * of '/' (if either '/opt' or '/var' are not separate file
	 * systems)
	 */
	if (remain > 0) {
		for (j = i = 0; i < cnt; i++) {
			if ((streq(slice_mntpnt(dp, np[i]), ROOT) &&
					(find_mnt_pnt(NULL, NULL, VAR,
					    NULL, CFG_CURRENT) == 0 ||
					find_mnt_pnt(NULL, NULL, OPT,
					    NULL, CFG_CURRENT) == 0)) ||
				streq(slice_mntpnt(dp, np[i]), VAR) ||
				streq(slice_mntpnt(dp, np[i]), OPT))
			ep[j++] = np[i];
		}

		if (j > 0) {
			add = blocks_to_blocks(dp, free / j);
			/* cap at 100 MB addition (cylinder aligned) */
			if (add > 204800)
				add = blocks_to_blocks(dp, 204800);

			for (i = 0; i < j && remain > 0; i++) {
				if (set_slice_geom(dp, ep[i], GEOM_IGNORE,
						slice_size(dp, ep[i]) +
						add) == D_OK) {
					remain -= add;
					if (add > remain)
						add = remain;
				}
			}
		}
	}
	/*
	 * if there is still unused space, add it to '/export' if there
	 * is one
	 */
	for (j = i = 0; i < cnt; i++) {
		if (strcmp(slice_mntpnt(dp, np[i]), EXPORT) == 0) {
			if (set_slice_geom(dp, np[i], GEOM_IGNORE,
					slice_size(dp, np[i]) + remain) == D_OK)
				remain = 0;
		}
	}

	/* return whatever sectors I couldn't find a home for */
	return (remain);
}

/*
 * _absorb_general_space()
 *	Distribute a user specified number of unallocated (free) sectors
 *	evenly amongst all potential recipient slices.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a non-NULL S-disk
 *		  geometry pointer
 *	cnt	- size of 'np' array
 *	free	- number of free sectors
 *	np	- array of indexes for viable slices which may legally be
 *		  allocated the specified free sectors
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_absorb_general_space(Disk_t *dp, int cnt, int free, int * np)
{
	int	i;
	int	add;
	int	remain;

	if (dp == NULL || sdisk_geom_null(dp))
		return;

	/* allocate evenly amongst all candidates */
	remain = free;
	add = blocks_to_blocks(dp, free / cnt);

	for (i = 0; i < cnt && remain > 0; i++) {
		if (set_slice_geom(dp, np[i], GEOM_IGNORE,
				slice_size(dp, np[i]) + add) == D_OK) {
			remain -= add;
			if (add > remain)
				add = remain;
		}
	}
}

/*
 * _make_separate_fs()
 *	Find an unused slice (preferably located on a slice following the
 *	user specified cylinder). Find an unused mount point name (looking
 *	across all selected drives) and assign it the mount point name and
 *	the user supplied size. The starting cylinder is set to '1' less than
 *	the user specified cylinder in order to insure that future calls to
 *	adjust_slice_starts() moves the slice into the correct location on
 *	the drive. This routine is used as part of the procedure of placing
 *	unused disk space into named file systems.  The cylinder locating is
 *	necessary to ensure that disks with fixed slices (e.g. preserved
 *	slices) have file systems made in the correct location on the drive.
 *	The 'cyl' parameter is actually the number of the starting cylinder
 *	of the slice which terminates (blocks) the free space area.
 *
 *	NOTE:	this routine does not leave the S-disk in a valid state.
 *		An adjust_slice_start() call must be made in order to
 *		moved the slice into an acceptable position.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with a valid S-disk
 *		  geometry pointer
 *	cyl	- cylinder before which the slice should be placed
 *	size	- size of partition in sectors
 * Return:
 *	 0	- space allocated successfully
 *	-1	- failure to make a separate file system
 * Status:
 *	private
 */
static int
_make_separate_fs(Disk_t *dp, int cyl, int size)
{
	int	slice;
	char	name[MAXNAMELEN];
	Slice_t	saved;

	if (dp == NULL || sdisk_geom_null(dp))
		return (-1);

	if ((slice = _find_unused_slice(dp, cyl)) >= 0) {
		(void) memcpy(&saved, &dp->sdisk.slice[slice],
				sizeof (Slice_t));
		_find_unused_fsname(name);
		if (set_slice_geom(dp, slice, cyl - 1, size) != D_OK ||
				set_slice_mnt(dp, slice, name, "-") != D_OK) {
			(void) memcpy(&dp->sdisk.slice[slice], &saved,
					sizeof (Slice_t));
			return (-1);
		}
	} else
		return (-1);
	return (0);
}

/*
 * _find_unused_fsname()
 *	Generate a file system name of the form:
 *
 *		/export/home    or
 *		/export/home<#>
 *
 *	which is unique across all slices on all selected drives within
 *	the current slice configuration. The number appended onto the
 *	filesystem name is iteratively incremented by '1' (starting at 0)
 *	until a unique name is found.
 * Parameters:
 *	name	- pointer to character array defined in calling process;
 *		  used to retrieve the file system name. Should be a
 *		  minimum of 16 characters long.
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_find_unused_fsname(char *name)
{
	int	i;

	(void) strcpy(name, "/export/home");
	for (i = 0; ; i++) {
		if (find_mnt_pnt(NULL, NULL, name,
				NULL, CFG_CURRENT) == 0)
			break;
		(void) sprintf(name, "/export/home%d", i);
	}
}

/*
 * _find_unused_slice()
 *	Search all the slices on a disk looking for an zero sized, unlocked,
 *	unstuck, unpreserved, unignored, slice with no defined mount point.
 *	The slice should preferably come after other slices which start
 *	at a parameter specified cylinder.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	cyl	- user specific cylinder after which
 *		  (0 <= # < total usable cylinders on the drive)
 * Return:
 *	#	- number of unused slice
 *	-1	- no unused slices found (or 'dp' is NULL or the
 *		  s-disk geometry pointer is null)
 * Status:
 *	private
 */
static int
_find_unused_slice(Disk_t *dp, int cyl)
{
	int	mark = 16;
	int	slice = -1;
	int	i;

	if (dp == NULL || sdisk_geom_null(dp))
		return (-1);

	WALK_SLICES(i) {
		if (i >= mark)
			break;

		if (slice_size(dp, i) > 0 ||
				slice_preserved(dp, i) ||
				slice_ignored(dp, i) ||
				slice_stuck(dp, i) ||
				slice_locked(dp, i)) {
			if (slice_start(dp, i) == cyl)
				mark = i;
		} else {
			if (slice_size(dp, i) == 0 &&
				    slice_mntpnt_not_exists(dp, i) &&
				    slice < mark)
				slice = i;
		}
	}
	if (slice > 0)
		return (slice);

	/*
	 * if there was no luck in the preferred location on the disk,
	 * look anywhere (first come, first served)
	 */
	WALK_SLICES(i) {
		if (slice_size(dp, i) == 0 &&
				slice_mntpnt_not_exists(dp, i) &&
				slice_not_preserved(dp, i) &&
				slice_not_ignored(dp, i) &&
				slice_not_stuck(dp, i) &&
				slice_not_locked(dp, i)) {
			return (i);
		}
	}
	/* no luck finding anything */
	return (-1);
}

/*
 * _adjust_slice_start()
 *	Adjust the starting cylinder of the specified slice such that
 *	the slice immediately follows whatever non-overlap slice precedes
 *	it. If the size of the slice is 0, the starting cylinder is always
 *	set to 0. This routine assumes that the calling routine is
 *	adjusting slices for an entire drive from lowest slice to highest
 *	slice. This means that all slices less than the specified slice
 *	should be adjusted by the time the current slice is processed.
 *
 *	ALGORITHM:
 *
 *	(1)  if the slice is preserved, an overlap slice, ignored, stuck,
 *	     or locked, return without making any changes.
 *	(2)  if the slice's size is '0', adjust its starting cylinder
 *	     to 0
 *	(3)  move the slice to the start of the drive, and start
 *	     checking to see if its new position overlaps with another
 *	     preserved, locked, stuck, ignored, or preceding slices. If so,
 *	     the current slice is backed up the drive until it fits
 *	     behind one of the slices without illegally overlapping.
 *	(4)  if the slice cannot be fit on the drive, then return D_NOSPACE,
 *	     otherwise, set the starting cylinder of the current slice
 *	     accordingly and return the return value of _set_slice_start().
 * Parameters:
 *	dp	- non-null disk structure pointer with a non-NULL sdisk
 *		  geometry pointer
 *	slice	- valid slice index number
 * Return:
 *	D_OK	  - adjustment successful
 *	D_NODISK  - 'dp' is NULL
 *	D_BADDISK - S-disk geometry pointer NULL
 *	D_BADARG  - invalid slice number
 *	D_NOSPACE - the preceding slice fills up the disk: can't _adjust
 * Status:
 *	private
 */
static int
_adjust_slice_start(Disk_t *dp, int slice)
{
	int	i, last, olast;
	int	count = 0;
	int	*np;

	if (dp == NULL)
		return (D_NODISK);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (invalid_sdisk_slice(slice))
		return (D_BADARG);

	if (slice_is_overlap(dp, slice) ||
			slice_preserved(dp, slice) ||
			slice_ignored(dp, slice) ||
			slice_stuck(dp, slice) ||
			slice_locked(dp, slice)) {
		return (D_OK);
	}

	if (slice_size(dp, slice) == 0) {
		slice_start(dp, slice) = 0;
		return (D_OK);
	}

	last = sdisk_geom_firstcyl(dp);

	/*
	 * back the slice up the drive until it no longer overlaps an
	 * immovable slice or another slice of a lower slice number
	 */
	while ((count = slice_overlaps(dp, slice, last,
				slice_size(dp, slice), &np)) > 0) {
		olast = last;
		for (i = 0; i < count; i++) {
			if (slice_preserved(dp, np[i]) ||
					slice_ignored(dp, np[i]) ||
					slice_locked(dp, np[i]) ||
					slice_stuck(dp, np[i]) ||
					np[i] < slice) {
				last = slice_start(dp, np[i]) +
					blocks_to_cyls(dp,
						slice_size(dp, np[i]));
			}
		}
		if (last == olast)
			break;

		if (last + blocks_to_cyls(dp,
				slice_size(dp, slice)) > sdisk_geom_lcyl(dp))
			return (D_NOSPACE);
	}

	if (last + blocks_to_cyls(dp, slice_size(dp, slice)) >
			sdisk_geom_lcyl(dp))
		return (D_NOSPACE);

	return (_set_slice_start(dp, slice, last));
}
