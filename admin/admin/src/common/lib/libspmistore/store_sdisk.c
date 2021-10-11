#ifndef lint
#pragma ident "@(#)store_sdisk.c 1.14 96/08/22 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */

/*
 * Module:	store_sdisk.c
 * Group:	libspmistore
 * Description: Sdisk and slice maninpulation functions.
 */

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include "spmicommon_api.h"
#include "spmistore_lib.h"

/* constants */

#define	NUMALTSECTCYL 	2	/* alt sector dflt size in cylinders */

/* private prototypes */

static int	coreSliceobjGetAltsectorSize(const Disk_t *);
static int	coreSliceobjReset(const Label_t, Disk_t *, const int);
static int 	coreSliceobjAdjustStart(const Label_t, Disk_t *, const int);
static int	coreSliceobjSetStart(Label_t, Disk_t *, int, int);
static int	coreSliceobjSetSize(Label_t, Disk_t *, int, int);
static int	coreSliceobjSetAttribute(const Label_t, Disk_t *, const int,
				const int, va_list);
static int	coreSliceobjGetAttribute(const Label_t, const Disk_t *,
				const int, const int, va_list);
static int	coreSliceobjSetKey(Label_t, Disk_t *, int, int, char *, int);
static int	SdiskobjRootUnsetBoot(Disk_t *, int);
static int      SliceobjFindUsePerDisk(Label_t, Disk_t *, char *, int, int);

/* globals */

static int	_SliceAutoadjust = 1;

/* ---------------------- public functions ----------------------- */

/*
 * Function:	SliceobjSetAttribute
 * Description:	Set the attributes associate with a given slice.
 *		Available attributes are:
 *
 *		 Attribute	     Value     Description
 *		-----------	    -------   -------------
 *		SLICEOBJ_USE	    char[]    Character array containing string
 *					      with name for slice. Valid values
 *					      are: SWAP, OVERLAP, "", ALTSECTOR,
 *					      and <absolute path>
 *		SLICEOBJ_INSTANCE   int	      Instance index associated with 
 *					      'use'. VAL_UNSPECIFIED means no
 *					      specific instance
 *		SLICEOBJ_START	    int	      Starting cylinder. Valid values
 *					      are: # >= 0
 *		SLICEOBJ_SIZE	    int	      Size of slice in sectors. Valid
 *					      values are: # >= 0, GEOM_REST
 *		SLICEOBJ_MOUNTOPTS  char[]    Array containing mount options 
 *					      for slices used for file systems
 *		SLICEOBJ_EXPLICIT   int	      Flag specifying explicit size.
 *					      Valid values are: TRUE FALSE
 *		SLICEOBJ_STUCK	    int	      Flag specifying explicit start.
 *					      Valid values are: TRUE, FALSE
 *		SLICEOBJ_PRESERVED  int	      Flag specifying preserved.
 *					      Valid values are: TRUE, FALSE
 *		SLICEOBJ_IGNORED    int       Flag specifying slice is ignored.
 *					      Valid values are: TRUE, FALSE
 *
 * Scope:	public
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk handle.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		...	keyword/attribute value pairs for modification
 * Return:	D_OK		modification successful
 *		D_BADARG 	modification failed
 *		D_BADDISK	disk state no condusive to modification
 *		D_NOTSELECT	disk not selected
 *		D_LOCKED	slice modifications restricted
 */
int
SliceobjSetAttribute(Disk_t *dp, int slice, ...)
{
	va_list	  ap;
	int	  status;
	int	  state = CFG_CURRENT;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (!disk_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (SliceobjIsLocked(state, dp, slice) && disk_initialized(dp))
		return (D_LOCKED);

	va_start(ap, res);
	status = coreSliceobjSetAttribute(state, dp, slice, NOPRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	SliceobjGetAttribute
 * Description:	Get the attributes associate with a given slice.
 *		Available attributes are:
 *
 *		 Attribute	     Value     Description
 *		-----------	    -------   -------------
 *		SLICEOBJ_USE	    char[]    MAXNAMELEN array to retreive
 *					      name associated with slice.
 *		SLICEOBJ_INSTANCE   &int      Instance index associated with 
 *					      'use'. VAL_UNSPECIFIED means no
 *					      specific instance.
 *		SLICEOBJ_START	    &int      Starting cylinder.
 *		SLICEOBJ_SIZE	    &int      Size of slice in sectors.
 *		SLICEOBJ_MOUNTOPTS  char[]    MAXNAMELEN array to retrieve
 *					      mount options for slices used
 *					      for file systems
 *		SLICEOBJ_EXPLICIT   &int      Flag specifying explicit size.
 *					      Possible values are: TRUE FALSE
 *		SLICEOBJ_STUCK	    &int      Flag specifying explicit start.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_PRESERVED  &int      Flag specifying preserved.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_LOCKED     &int      Flag specifying slice restricted.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_REALIGNED  &int      Flag specifying slice starting
 *					      cylinder aligned at load.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_IGNORED    &int      Flag specifying slice is ignored.
 *					      Possible values are: TRUE, FALSE
 *
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk handle.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		...	NULL termianted list of keyword/attribute value pairs.
 * Return:	D_OK	    retrieval successful
 *		D_BADARG    invalid argument specified
 */
int
SliceobjGetAttribute(Label_t state, Disk_t *dp, int slice, ...)
{
	va_list	  ap;
	int	  status;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	va_start(ap, res);
	status = coreSliceobjGetAttribute(state, dp, slice, NOPRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	adjust_slice_starts
 * Description:	Adjust the starting cylinders for all slices on the disk 'dp'
 *		which are not locked, preserved, stuck, or overlap. Consume any
 *		gaps which may exist between a given slice and the slice which
 *		precedes it. If the routine fails, the pre-call slice
 *		configuration is restored before returning, so there are no
 *		side-effects of a failure.
 * Scope:	public
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk object handle (state: okay, selected)
 * Return:	D_OK		adjusts successful
 *		D_BADARG	invalid argument
 *		D_BADDISK	disk not available for specified operation
 *		D_NOTSELECT	disk not selected
 *		D_NOSPACE	insufficient space on disk
 *		D_NOFIT		insufficient space in slice segment
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
	int	state = CFG_CURRENT;

	/* validate parameters */
	if (dp == NULL)
		return (D_BADARG);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (!disk_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	(void) memcpy(&save, Sdiskobj_Addr(CFG_CURRENT, dp), sizeof (Sdisk_t));

	WALK_SLICES(i) {
		if ((status = coreSliceobjAdjustStart(state, dp, i)) != D_OK) {
			(void) memcpy(Sdiskobj_Addr(CFG_CURRENT, dp),
					&save, sizeof (Sdisk_t));
			return (status);
		}
	}

	/*
	 * check to see if one of the preserved/lock/stuck FS got overrun by
	 * another slice
	 */
	WALK_SLICES(i) {
		if (Sliceobj_Size(state, dp, i) == 0 || slice_is_overlap(dp, i))
			continue;

		sstart = Sliceobj_Start(state, dp, i);
		send = sstart + blocks_to_cyls(dp, Sliceobj_Size(state, dp, i));
		WALK_SLICES(f) {
			if (i == f)
				continue;

			if (Sliceobj_Size(state, dp, f) == 0 || slice_is_overlap(dp, f))
				continue;

			fstart = Sliceobj_Start(state, dp, f);
			if (send > fstart && sstart <= fstart)
				return (D_NOFIT);
		}
	}

	return (D_OK);
}

/*
 * Function:	filesys_preserve_ok
 * Description:	Check if preserve is allowed on a specific file system name.
 *		The following mount point specifiers cannot be preserved:
 *			/
 *			/usr
 *			/var
 * Scope:	global
 * Parameters:	fs	   - file system name
 * Return:	D_OK	   - slice name is permitted to be preserved
 *		D_CANTPRES - slice name is not permitted to be preserved
 */
int
filesys_preserve_ok(char *fs)
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
 * Function:	find_mnt_pnt
 * Description: Determines if 'fs' is found on a disk. If 'disk' (or 'drive')
 *		are specified, the search is restricted to that disk (in this
 *		case the disk does not have to be in a "selected" state).
 *		Otherwise, all disks in entire disk which are marked "selected"
 *		and with an "okay" state are searched. If a match is found,
 *		'info.dp' is set to the disk structure address, and 'info.slice'
 *		is set to the slice number (unless 'info' is a NULL pointer).
 * Scope:	public
 * Parameters:	disk	- disk structure pointer (NULL if specifying drive
 *			  by 'drive') - 'disk' has precedence over 'drive'
 *		drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *			  drive by 'disk')
 *		name	- mount point name used in the search
 *		info	- pointer to mnt_pnt structure used to retrieve
 *			  disk/slice data (NULL if no info retrieval is desired)
 *		state	- specifies which configuration set to search. Allowable
 *			  values are: CFG_CURRENT, CFG_COMMIT, CFG_EXIST
 * Returns:	1 	- 'name' was found on a disk
 *		0 	- 'name' was NOT found
 */
int
find_mnt_pnt(Disk_t *disk, char *drive, char *name,
			Mntpnt_t *info, Label_t state)
{
	Disk_t	*dp = NULL;
	int	i;

	/* validate parameters */
	if (name == NULL)
		return (0);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (0);

	if (disk != NULL)
		dp = disk;
	else if (drive != NULL) {
		if ((dp = find_disk(drive)) == NULL)
			return (0);
	}

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (!disk_okay(dp) || !disk_selected(dp))
				continue;

			if (Sdiskobj_Geom(state, dp) == NULL ||
					SdiskobjIsIllegal(CFG_EXIST, dp))
				continue;

			WALK_SLICES(i) {
				if (SliceobjIsIgnored(state, dp, i))
					continue;

				if (streq(name, Sliceobj_Use(state, dp, i))) {
					if (info != NULL) {
						info->dp = dp;
						info->slice = i;
					}

					return (1);
				}
			}
		}
	} else {
		if (!disk_okay(dp))
			return (0);

		WALK_SLICES(i) {
			if (SliceobjIsIgnored(state, dp, i))
				continue;

			if (streq(name, Sliceobj_Use(state, dp, i))) {
				if (info != NULL) {
					info->dp = dp;
					info->slice = i;
				}

				return (1);
			}
		}
	}

	return (0);
}

/*
 * Function:	get_slice_autoadjust
 * Description:	Get the current "autoadjust" flag to '1' (enabled) or '0'
 *		(disabled).
 * Scope:	public
 * Parameters:	none
 * Return:	0/1	- old value of autoadjust variable
 */
int
get_slice_autoadjust(void)
{
	return (_SliceAutoadjust);
}

/*
 * Function:	sdisk_compare
 * Description:	Compare the current state of the Sdisk to either the committed
 *		or the existing state. Return in error if there are differences.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *		state	- specify against which geometry the current geometry
 *			  should be compared (CFG_COMMIT or CFG_EXIST)
 * Return:	0	- the structures are the same
 *		1	- the structures differ
 */
int
sdisk_compare(Disk_t *dp, Label_t state)
{
	int	s;

	/* validate parameters */
	if (dp == NULL)
		return (1);

	if (state != CFG_COMMIT && state != CFG_EXIST)
		return (1);

	if (sdisk_geom_same(dp, state) != D_OK)
		return (1);

	if (Sdiskobj_State(CFG_CURRENT, dp) != Sdiskobj_State(state, dp))
		return (1);

	for (s = 0; s <= LAST_STDSLICE; s++) {
		if (memcmp(Sliceobj_Addr(CFG_CURRENT, dp, s),
					Sliceobj_Addr(state, dp, s),
					sizeof (Slice_t)) != 0)
				return (1);
	}

	return (0);
}

/*
 * Function:	sdisk_geom_same
 * Description:	On fdisk systems, the Solaris disk geometry is dependent upon
 *		the Solaris fdisk partition geometry. Several conditions (such
 *		as the preservability of existing slices) are dependent upon
 *		whether or not the Solaris disk geometry has been altered. This
 *		function compares the current sdisk geometry against either
 *		the committed or existing sdisk geometry in search of changes.
 *		On non-fdisk systems, this routine always returns D_OK.
 *
 *		ALGORITHM:
 *		(1) Get the Solaris partition indexes for the current and
 *		    committed/existing configurations
 *		(2) If neither had a Solaris partition, then return D_OK
 *		(3) If one had a Solaris partition, but the other didn't,
 *		    then return D_GEOMCHNG
 *		(4) If both had a Solaris partition, then compare the structures,
 *		    and if they differ in any way, return D_GEOMCHNG, otherwise,
 *		    return D_OK
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state:  okay)
 *		state	- specify against which geometry the current geometry
 *			  should be compared (CFG_COMMIT or CFG_EXIST)
 * Return:	D_OK		sdisk geometry unchanged
 *		D_BADARG	invalid argument
 *		D_BADDISK	disk state not applicable to this operation
 *		D_GEOMCHNG	S-disk geom changed since disk last committed
 */
int
sdisk_geom_same(Disk_t *dp, Label_t state)
{
	int	pid;
	int	cpid;

	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return (D_BADARG);

	if (!disk_okay(dp))
		return (D_BADDISK);

	if (!disk_fdisk_exists(dp))
		return (D_OK);

	cpid = get_solaris_part(dp, CFG_CURRENT);
	pid = get_solaris_part(dp, state);

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
	if (memcmp(Partobj_GeomAddr(CFG_CURRENT, dp, cpid),
			Partobj_GeomAddr(state, dp, pid), sizeof (Geom_t)) != 0)
		return (D_GEOMCHNG);

	return (D_OK);
}

/*
 * Function:	sdisk_max_hole_size
 * Description: Find the largest section of contiguously unused data area on
 *		the disk.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer for disk
 * Returns:	0	- no space available
 *		#	- number of unallocated usable blocks
 */
int
sdisk_max_hole_size(Disk_t *dp)
{
	int	s;
	int	sstart = 0;
	int	send;
	int	f;
	int	hole = 0;

	if (dp == NULL || !disk_okay(dp) ||
			sdisk_geom_null(dp) || SdiskobjIsIllegal(CFG_CURRENT, dp))
		return (0);

	while (sstart < sdisk_geom_lcyl(dp)) {
		/*
		 * look for a slice whose starting cylinder lines up
		 * with the currently potential start of a hole
		 */
		WALK_SLICES(s) {
			if (Sliceobj_Size(CFG_CURRENT, dp, s) > 0 &&
					!slice_is_overlap(dp, s) &&
					Sliceobj_Start(CFG_CURRENT, dp, s) == sstart) {
				sstart = Sliceobj_Start(CFG_CURRENT, dp, s) +
					blocks_to_cyls(dp,
						Sliceobj_Size(CFG_CURRENT, dp, s));
				break;
			}
		}

		/*
		 * we made it through the slices an none lined up with the
		 * starting sector, so we've found a hole
		 */
		if (!valid_sdisk_slice(s)) {
			send = sdisk_geom_lcyl(dp);
			WALK_SLICES(f) {
				if (Sliceobj_Size(CFG_CURRENT, dp, f) > 0 &&
						!slice_is_overlap(dp, f) &&
						Sliceobj_Start(CFG_CURRENT, dp, f) < send &&
						Sliceobj_Start(CFG_CURRENT, dp, f) > sstart)
					send = Sliceobj_Start(CFG_CURRENT, dp, f);
			}

			if ((send - sstart) > hole)
				hole = send - sstart;

			sstart = send;
		}
	}

	return (cyls_to_blocks(dp, hole));
}

/*
 * Function:	sdisk_space_avail
 * Description:	Calculate the number of sectors on 'dp' not allocated to any
 *		slice. All overlap slices are ignored.
 *
 *		WARNING:start cylinders must be adjusted so there
 *			are no illegal overlaps before calling this
 *			routine, or bogus values will be returned
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer for disk
 *			  (state: okay, valid geometry pointer)
 * Returns:	0	- no space available
 *		#	- number of unallocated usable blocks
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

	if (dp == NULL || !disk_okay(dp) ||
			sdisk_geom_null(dp) || SdiskobjIsIllegal(CFG_CURRENT, dp))
		return (0);

	WALK_SLICES(slice) {
		/*
		 * ignore slices which don't impact the total or are already
		 * subtracted from the usable_sdisk_blks() calculation
		 */
		if (Sliceobj_Size(CFG_CURRENT, dp, slice) == 0 ||
				slice == BOOT_SLICE ||
				slice == ALT_SLICE ||
				slice_is_overlap(dp, slice))
			continue;

		sstart = Sliceobj_Start(CFG_CURRENT, dp, slice);
		send = sstart + blocks_to_cyls(dp, Sliceobj_Size(CFG_CURRENT, dp, slice));

		WALK_SLICES(f) {
			if (Sliceobj_Size(CFG_CURRENT, dp, f) == 0 ||
					slice == f ||
					slice == BOOT_SLICE ||
					slice == ALT_SLICE ||
					slice_is_overlap(dp, f))
				continue;

			fstart = Sliceobj_Start(CFG_CURRENT, dp, f);
			fend = fstart + blocks_to_cyls(dp,
				Sliceobj_Size(CFG_CURRENT, dp, f));

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
 * Function:	set_slice_autoadjust
 * Description:	Change the "autoadjust" flag to '1' (enabled) or '0' (disabled).
 * Scope:	public
 * Parameters:	val	- 0 (don't autoadjust when any slice geom changes), or
 *			  1 (adjust automatically when any slice geom changes)
 * Return:	0/1	- old value of autoadjust variable
 */
int
set_slice_autoadjust(int val)
{
	int	i = _SliceAutoadjust;

	_SliceAutoadjust = (val == 0 ? 0 : 1);
	return (i);
}

/*
 * Function:	set_slice_geom
 * Description:	Set the starting cylinder and/or size for 'slice'. 'size'
 *		parameters are automatically rounded to cylinder boundaries.
 *		Once the geometry has been set, starting cylinders are
 *		automatically adjusted across the drive. Disks specified must
 *		not be in a "bad" state, and must be selected.
 * Scope:	public
 * Parameters:	dp	- valid disk structure pointer for selected disk
 *			  (state: okay, selected)
 *		slice	- slice index
 *			  (state: not locked)
 *		start	- starting cylinder number, or:
 *				GEOM_ORIG     - original start cylinder
 *				GEOM_COMMIT   - last committed start cylinder
 *				GEOM_IGNORE   - don't do anything with star
 *		size	- number of sectors, or:
 *				GEOM_ORIG     - original slice size
 *				GEOM_COMMIT   - last committed slice size
 *				GEOM_REST     -	whatever isn't already assigned
 *						to another slice
 *				GEOM_IGNORE   - don't do anything with size
 * Return:	D_OK	    slice geometry set successfully
 *		D_BADARG    invalid argument
 *		D_BADDISK   disk state not valid for requested operation
 *		D_LOCKED    slice is locked and geometry cannot be changed
 *		D_NOTSELECT disk state was not selected
 *		D_GEOMCHNG  disk geom change prohibits restore
 *		other	    return from adjust_Sliceobj_Start(CFG_CURRENT, ),
 *			    SliceobjSetAttribute()
 */
int
set_slice_geom(Disk_t *dp, int slice, int start, int size)
{
	int	status;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (start == GEOM_IGNORE && size == GEOM_IGNORE)
		return (D_OK);

	/* get the real starting cylinder */
	switch (start) {
	    case GEOM_ORIG:
		if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
			return (D_GEOMCHNG);
		start = Sliceobj_Start(CFG_EXIST, dp, slice);
		break;
	    case GEOM_COMMIT:
		if (sdisk_geom_same(dp, CFG_COMMIT) != D_OK)
			return (D_GEOMCHNG);
		start = Sliceobj_Start(CFG_COMMIT, dp, slice);
		break;
	}

	/* get the real size */
	switch (size) {
	    case GEOM_ORIG:
		if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
			return (D_GEOMCHNG);
		size = Sliceobj_Size(CFG_EXIST, dp, slice);
		break;
	    case GEOM_COMMIT:
		if (sdisk_geom_same(dp, CFG_COMMIT) != D_OK)
			return (D_GEOMCHNG);
		size = Sliceobj_Size(CFG_COMMIT, dp, slice);
		break;
	}

	if (start == GEOM_IGNORE) {
		status = SliceobjSetAttribute(dp, slice,
				SLICEOBJ_SIZE,	size,
				NULL);
	} else if (size == GEOM_IGNORE) {
		status = SliceobjSetAttribute(dp, slice,
				SLICEOBJ_START,	start,
				NULL);
	} else {
		status = SliceobjSetAttribute(dp, slice,
				SLICEOBJ_START,	start,
				SLICEOBJ_SIZE,	size,
				NULL);
	}

	return (status);
}

/*
 * Function:	set_slice_ignore
 * Description:	Set the ignore state for 'slice' to 'val' (IGNORE_YES or
 *		IGNORE_NO). The geometry of 'slice' must not have changed
 *		since the original drive configuration, and the slice must
 *		not be aligned. '0' sized slices cannot be ignored.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state: okay, selected, legal)
 *		slice	- valid slice index
 *		val	- ignore value to which the slice is to be set
 *			  (valid values: IGNORE_NO, IGNORE_YES)
 * Return:	D_OK		slice updated as specified
 *		D_BADARG	invalid argument
 *		D_BADDISK	disk not available for specified operation
 *		D_NOTSELECT	disk state was not selected
 *		D_LOCKED	cannot change attributes of a locked slice
 *		D_CHANGED	the slice start and/or size has changed since
 *				the original configuration
 *		D_ALIGNED	attempt to ignore an aligned slice
 *		D_ZERO		attempt to ignore a zero sized slice
 */
int
set_slice_ignore(Disk_t *dp, int slice, int val)
{
	int	status;

	status = SliceobjSetAttribute(dp, slice,
			SLICEOBJ_IGNORED,	val,
			NULL);

	return (status);
}

/*
 * Function:	set_slice_mnt
 * Description:	Make the following checks on 'mnt':
 *		- One swap partition per disk
 *		- Unique mount point
 *		- Mount point
 *
 *		Pathname is absolute (except for swap). Reset the preserve flag
 *		not PRES_NO if the file system can't be preserved. If a slice
 *		was labelled "overlap" and is changed to anything else, its size
 *		will be automatically zeroed out if it currently overlap another
 *		named (not "overlap") slice. If 'mnt' is "overlap", 'opts' is
 *		ignored and the mount options field is automatically cleared.
 *		Setting the mountpoint to "/" resets the boot object.
 *
 *		Setting slice 2 to "overlap" (SPARC only) will automatically
 *		clear the "touch2" flag, and the start and size parameters will
 *		be set to be the whole drive.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state: okay)
 *		slice	- slice index number
 *		use	- valid slice name
 *		opts	- mount options:
 *				char *	- mount option string
 *				NULL *	- don't modify at all
 *				""	- clear the options field
 * Return:	D_OK	  operation successful
 *		D_BADARG  invalid argument
 *		D_BADDISK disk not available for specified operation
 *		D_LOCKED  cannot modify a locked slice
 *		D_ILLEGAL failed because S-disk is in an 'illegal' state
 *		D_DUPMNT  mount point already exists
 *		D_IGNORED slice is ignored and cannot be changed
 *		D_BOOTFIXED trying to set "/" on a disk/slice that cannot be
 *			    the boot object
 */
int
set_slice_mnt(Disk_t *dp, int slice, char *use, char *opts)
{
	int	status = D_OK;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (opts != NULL) {
		status = SliceobjSetAttribute(dp, slice,
				SLICEOBJ_USE,		use,
				SLICEOBJ_INSTANCE,
					is_pathname(use) ? 0 : VAL_UNSPECIFIED,
				SLICEOBJ_MOUNTOPTS,	opts,
				NULL);
	} else {
		status = SliceobjSetAttribute(dp, slice,
				SLICEOBJ_USE,		use,
				SLICEOBJ_INSTANCE,
					is_pathname(use) ? 0 : VAL_UNSPECIFIED,
				NULL);
	}

	return (status);
}

/*
 * Function:	set_slice_preserve()
 * Description:	Set the preserve state for 'slice' to 'val' (PRES_YES or
 *		PRES_NO). The geometry of 'slice' must not have changed since
 *		the original drive configuration. '0' sized slices cannot be
 *		preserved.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state: okay, selected)
 *		slice	- slice index number on disk
 *		val	- preservation value to which the slice is to be se
 *			  (valid values: PRES_NO, PRES_YES)
 * Return:	D_OK		slice updated as specified
 *		D_BADARG	invalid argument
 *		D_BADDISK	disk not available for specified operation
 *		D_NOTSELECT	disk state was not selected
 *		D_LOCKED	cannot change attributes of a locked slice
 *		D_CHANGED	the slice start and/or size has changed since
 *				the original configuration
 *		D_CANTPRES	attempt to reserve a reserved slice
 *		D_OVER		can't preserve this slice because it currently
 *				overlaps with another slice illegally
 *		D_ZERO		slice size is zero
 *		D_GEOMCHNG	sdisk geom changed since disk last committed
 */
int
set_slice_preserve(Disk_t *dp, int slice, int val)
{
	int	status;

	status = SliceobjSetAttribute(dp, slice,
			SLICEOBJ_PRESERVED,    val,
			NULL);

	return (status);
}

/*
 * Function:	slice_name_ok
 * Description: Check if the name specified is a legal slice name. Legal slice
 *		names begin with '/', or are "swap", "", "alts", or "overlap".
 * Scope:	public
 * Parameters:	name	[RO, *RO] (char *)
 *			Slice name.
 * Return:	D_OK	   acceptable slice name
 *		D_BADARG   unacceptable slice name
 */
int
slice_name_ok(char *name)
{
	if (name == NULL)
		return (D_BADARG);

	if (!is_pathname(name) &&
			!streq(name, OVERLAP) &&
			!streq(name, SWAP) &&
			!streq(name, ALTSECTOR) &&
			!streq(name, ""))
		return (D_BADARG);

	return (D_OK);
}

/*
 * Function:	slice_overlaps
 * Description:	See if a slice starting at 'start' and of 'size' sectors in size
 * 		overlaps another slice which has a mount point other than
 *		"overlap". Returns '0' if 'dp' is illegal or the disk state is
 *		not "okay". Repeated calls to this function will overwrite the
 *		current 'olpp' array values.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state:  okay)
 *		slice	- slice being validated (-1 if no slice specified)
 *		start	- proposed starting cylinder for slice being validated
 *		size	- proposed sector count size for slice being validated
 *		olpp	- address to an integer pntr initialized to the address
 *			  of an integer array containing a list of slices which
 *			  were illegally overlapped (number of valid entries in
 *			  the array is the function return value). If NULL, no
 *			  array pointer is desired.
 * Return:	#	- number of illegal overlaps found (also an index limit
 *			  for the 'overlaps' array)
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

	if (dp == NULL || !disk_okay(dp) ||
			sdisk_geom_null(dp) || SdiskobjIsIllegal(CFG_CURRENT, dp))
		return (0);

	send = start + blocks_to_cyls(dp, size);

	WALK_SLICES(f) {
		if (f == slice || Sliceobj_Size(CFG_CURRENT, dp, f) == 0 ||
				streq(Sliceobj_Use(CFG_CURRENT, dp, f), OVERLAP))
			continue;

		fstart = Sliceobj_Start(CFG_CURRENT, dp, f);
		fend = fstart + blocks_to_cyls(dp,
				Sliceobj_Size(CFG_CURRENT, dp, f));

		if ((fstart >= start && fstart < send) ||
				(start >= fstart && start < fend))
			_overlap_slices[count++] = f;
	}

	if (olpp != NULL)
		*olpp = _overlap_slices;

	return (count);
}

/*
 * Function:	slice_preserve_ok
 * Description: Check if preserve allowed on this slice. Preserve is not
 *		permitted if the file system name is reserved (see
 *		filesys_preserve_ok()), the slice size is '0', the geometry
 *		of the slice has changed from the original configuration, or
 *		the geometry was aligned at load time.
 * Scope:	public
 * Parameters:	dp	- disk structure pointer
 *		slice	- slice index number
 * Return:	D_OK	   - disk is permitted to be preserved
 *		D_BADARG   invalid argument
 *		D_BADDISK  - disk state not valid for requested operation
 *		D_LOCKED   - cannot change attributes of a locked slice
 *		D_CHANGED  - the slice parameters (start/size) have been
 *			     changed already, so can't preserve)
 *		D_CANTPRES - disk is not permitted to be preserved
 *		D_ILLEGAL  - original S-disk is in an 'illegal' state
 *		D_IGNORED  - slice is ignored and cannot be changed
 *		D_GEOMCHNG - disk geom change prohibits restore
 *		D_ALIGNED  - the starting cylinder / size were aligned at load
 */
int
slice_preserve_ok(Disk_t *dp, int slice)
{
	int	status;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (!disk_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	if (SliceobjIsLocked(CFG_CURRENT, dp, slice))
		return (D_LOCKED);

	if (SliceobjIsIgnored(CFG_CURRENT, dp, slice))
		return (D_IGNORED);

	if (SdiskobjIsIllegal(CFG_EXIST, dp))
		return (D_ILLEGAL);

	if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
		return (D_GEOMCHNG);

	if (disk_initialized(dp)) {
		if ((Sliceobj_Start(CFG_CURRENT, dp, slice) !=
				Sliceobj_Start(CFG_EXIST, dp, slice)) ||
				(Sliceobj_Size(CFG_CURRENT, dp, slice) !=
				Sliceobj_Size(CFG_EXIST, dp, slice)))
			return (D_CHANGED);
	}

	if (SliceobjIsRealigned(CFG_EXIST, dp, slice))
		return (D_ALIGNED);

	status = filesys_preserve_ok(Sliceobj_Use(CFG_CURRENT, dp, slice));
	return (status);
}

/*
 * Function:	SliceobjFindUse
 * Description:	Given the name and instance of a resource, find a slice in the
 *		sdisk configuration of the disk list which matches the
 *		name and instance specifications. Only RESSTAT_INDEPENDENT
 *		disks are searched. The caller may indicate if an exact
 *		name/instance match is required, or if slices with
 *		VAL_UNSPECIFIED instances may be first-come-first-matched.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		disk	[RO, *RO] (Disk_t *) [OPTIONAL]
 *			Disk object handle used to constrain search to a single
 *			disk. NULL if no constraint required.
 *		name	[RO, *RO] (Resobj *)
 *			Non-null resource name pointer.
 *		instance [RO] (int)
 *			Non-negative resource instance number.
 *		match	[RO] (int)
 *			Match flag indicating the degree of exactness for
 *			which the instance must match. Valid values are:
 *			    0 - exact or unspecified slice instances match
 *			    1 - only exact slice instance match
 * Return:	NULL		No match
 *		SliceKey *	Pointer to local static structure containing
 *				disk and slice specifications for match.
 */
SliceKey *
SliceobjFindUse(Label_t state, Disk_t *disk,
		char *name, int instance, int match)
{
	static SliceKey	   key;
	Disk_t *	   dp;
	int		   slice;

	if (instance < 0 && instance != VAL_UNSPECIFIED)
		return (NULL);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (NULL);

	if (disk != NULL) {
		if (!disk_selected(disk))
			return (NULL);

		/*
		 * first try to do an exact name/instance match; if this
		 * comes up dry and the user has not requested an explcit
		 * match, see if there is a slice with a VAL_UNSPECIFIED
		 * instance that matches on the name
		 */
		slice = SliceobjFindUsePerDisk(state, disk, name, instance, 1);
		if (valid_sdisk_slice(slice)) {
			key.dp = disk;
			key.slice = slice;
			return (&key);
		}

		if (!match) {
			slice = SliceobjFindUsePerDisk(state, disk, name, instance, 0);
			if (valid_sdisk_slice(slice)) {
				key.dp = disk;
				key.slice = slice;
				return (&key);
			}
		}
	} else {
		/*
		 * first try to do an exact name/instance match; if this
		 * comes up dry and the user has not requested an explcit
		 * match, see if there is a slice with a VAL_UNSPECIFIED
		 * instance that matches on the name
		 */
		WALK_DISK_LIST(dp) {
			if (!disk_selected(dp))
				continue;

			slice = SliceobjFindUsePerDisk(state, dp, name, instance, 1);
			if (valid_sdisk_slice(slice)) {
				key.dp = dp;
				key.slice = slice;
				return (&key);
			}
		}

		if (!match) {
			WALK_DISK_LIST(dp) {
				if (!disk_selected(dp))
					continue;

				slice = SliceobjFindUsePerDisk(state, dp, name,
						instance, 0);
				if (valid_sdisk_slice(slice)) {
					key.dp = dp;
					key.slice = slice;
					return (&key);
				}
			}
		}
	}

	return (NULL);
}

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	SliceobjCountuse
 * Description:	Count the number of times the specified slice "use" name
 *		appears on a slice with > 0 size. The user can constrain the
 *		search to a specific disk by providing a disk pointer, or
 *		can leave the search to be unlimited by providing a NULL
 *		disk pointer. Instance fields are skipped during this search.
 *		Ignored and locked slices are also skipped.
 * Parameters:	state	[RO] (Label_t)
 *			State of the sdisk configuration to search.  Valid
 *			values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle with which to constrain the search,
 *			or NULL to specify all selected disks with usable
 *			sdisk configurations.
 *		name	[RO, *RO] (char[])
 *			Name to use in search.
 * Return:	0	No matches found, or invalid parameters
 *		# > 0	Number of matches found
 */
int
SliceobjCountUse(const Label_t state, const Disk_t *dp, const char *name)
{
	int	  slice;
	int	  count;
	Disk_t *  tdp;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (0);

	if (name == NULL)
		return (0);

	count = 0;
	if (dp != NULL) {
		if (!sdisk_is_usable(dp))
			return (0);

		WALK_SLICES(slice) {
			if (SliceobjIsIgnored(state, dp, slice) ||
				    SliceobjIsLocked(state, dp, slice))
				continue;

			if (Sliceobj_Size(state, dp, slice) > 0 &&
				    streq(Sliceobj_Use(state, dp, slice), name))
				count++;
		}
	} else {
		WALK_DISK_LIST(tdp) {
			if (!sdisk_is_usable(tdp))
				continue;

			WALK_SLICES(slice) {
				if (SliceobjIsIgnored(state, tdp, slice) ||
					    SliceobjIsLocked(state, tdp, slice))
					continue;

				if (Sliceobj_Size(state, tdp, slice) > 0 &&
						streq(Sliceobj_Use(state,
							tdp, slice), name))
					count++;
			}
		}
	}

	return (count);
}

/*
 * Function:	SliceobjSetAttributePriv
 * Description:	Privileged call to set the attributes associate with a given
 *		slice. Available attributes are:
 *
 *		 Attribute	     Value     Description
 *		-----------	    -------   -------------
 *		SLICEOBJ_USE	    char[]    Character array containing string
 *					      with name for slice. Valid values
 *					      are: SWAP, OVERLAP, "", ALTSECTOR,
 *					      and <absolute path>
 *		SLICEOBJ_INSTANCE   int	      Instance index associated with 
 *					      'use'. VAL_UNSPECIFIED means no
 *					      specific instance
 *		SLICEOBJ_START	    int	      Starting cylinder. Valid values
 *					      are: # >= 0
 *		SLICEOBJ_SIZE	    int	      Size of slice in sectors. Valid
 *					      values are: # >= 0, GEOM_REST
 *		SLICEOBJ_MOUNTOPTS  char[]    Array containing mount options 
 *					      for slices used for file systems
 *		SLICEOBJ_EXPLICIT   int	      Flag specifying explicit size.
 *					      Valid values are: TRUE FALSE
 *		SLICEOBJ_STUCK	    int	      Flag specifying explicit start.
 *					      Valid values are: TRUE, FALSE
 *		SLICEOBJ_PRESERVED  int	      Flag specifying preserved.
 *					      Valid values are: TRUE, FALSE
 *		SLICEOBJ_LOCKED	    int	      Flag specifying restricted.
 *					      Valid values are: TRUE, FALSE
 *		SLICEOBJ_REALIGNED  int	      Flag specifying starting cylinder
 *					      was realigned at load. Valid
 *					      values are: TRUE, FALSE
 *		SLICEOBJ_IGNORED    int       Flag specifying slice is ignored.
 *					      Valid values are: TRUE, FALSE
 *
 * Scope:	internal
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk handle.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		...	keyword/attribute value pairs for modification
 * Return:	D_OK		modification successful
 *		D_BADARG 	modification failed
 */
int
SliceobjSetAttributePriv(Disk_t *dp, const int slice, ...)
{
	va_list	ap;
	int	status;
	int	state = CFG_CURRENT;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	va_start(ap, res);
	status = coreSliceobjSetAttribute(state, dp, slice, PRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	SliceobjGetAttributePriv
 * Description:	Privileged call to get the attributes associate with a given
 *		slice. Available attributes are:
 *
 *		Attribute		  Value	    Description
 *		-----------		  -------   -------------
 *		SLICEOBJ_USE	    char[]    MAXNAMELEN array to retreive
 *					      name associated with slice.
 *		SLICEOBJ_INSTANCE   &int      Instance index associated with 
 *					      'use'. VAL_UNSPECIFIED means no
 *					      specific instance.
 *		SLICEOBJ_START	    &int      Starting cylinder.
 *		SLICEOBJ_SIZE	    &int      Size of slice in sectors.
 *		SLICEOBJ_MOUNTOPTS  char[]    MAXNAMELEN array to retrieve
 *					      mount options for slices used
 *					      for file systems
 *		SLICEOBJ_EXPLICIT   &int      Flag specifying explicit size.
 *					      Possible values are: TRUE FALSE
 *		SLICEOBJ_STUCK	    &int      Flag specifying explicit start.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_PRESERVED  &int      Flag specifying preserved.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_LOCKED     &int      Flag specifying slice restricted.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_REALIGNED  &int      Flag specifying slice starting
 *					      cylinder aligned at load.
 *					      Possible values are: TRUE, FALSE
 *		SLICEOBJ_IGNORED    &int      Flag specifying slice is ignored.
 *					      Possible values are: TRUE, FALSE
 *
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk handle.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		...	keyword/attribute value pairs for retrieval
 * Return:	D_OK	    retrieval successful
 *		D_BADARG    retrieval failed
 */
int
SliceobjGetAttributePriv(const Label_t state, const Disk_t *dp,
					const int slice, ...)
{
	va_list	  ap;
	int	  status;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	va_start(ap, res);
	status = coreSliceobjGetAttribute(state, dp, slice, PRIVILEGE, ap);
	va_end(ap);

	return (status);
}

/*
 * Function:	_reset_sdisk
 * Description:	Reset the S-disk state, and for each slice in the S-disk
 *		structure, set all the data fields associated with the specified
 *		disk to a default value. For non-system slices, this means
 *		clearing the slice flag, the mount point name and options field,
 *		and setting the start/size to '0'. For non-system slices, this
 *		means setting the mount point name and start/size appropriately
 *		for the slice. A final run is made over all slices to insure the
 *		lock flag is set on all locked slices.
 * Scope:	internal
 * Parameters:	dp	- non-NULL disk structure pointer with a valid S-disk
 *			  geometry pointer
 * Return:	D_OK	  	reset successful
 *		D_BADARG	invalid argument
 *		D_BADDISK	disk state not valid for requested operation
 */
int
_reset_sdisk(Disk_t *dp)
{
	int	i;
	int	state = CFG_CURRENT;

	/* validate argument */
	if (dp == NULL)
		return (D_BADARG);

	if (!disk_okay(dp) || sdisk_geom_null(dp))
		return (D_BADDISK);

	WALK_SLICES(i)
		(void) coreSliceobjReset(state, dp, i);

	Sdiskobj_State(state, dp) = 0;
	return (D_OK);
}

/*
 * Function:	SliceobjIsAllocated
 * Description:	Determine if the specified slice in the given state
 *		has already had attributes set.
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 * Return:	0
 *		1
 */
int
SliceobjIsAllocated(const Label_t state, const Disk_t *dp, const int slice)
{
	/* validate parameters */
	if (!sdisk_is_usable(dp) || !valid_sdisk_slice(slice))
		return (0);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (0);

	if (Sliceobj_Size(state, dp, slice) > 0 ||
			Sliceobj_Use(state, dp, slice)[0] != '\0' ||
			Sliceobj_Instance(state, dp, slice) !=
				VAL_UNSPECIFIED ||
			SliceobjIsLocked(state, dp, slice) ||
			SliceobjIsIgnored(state, dp, slice) ||
			SliceobjIsPreserved(state, dp, slice) ||
			SliceobjIsStuck(state, dp, slice) ||
			SliceobjIsExplicit(state, dp, slice))
		return (1);

	return (0);
}

/*
 * Function:	SdiskobjRootSetBoot
 * Description:	Update the boot object disk and device fields as
 *		appropriate.
 * Scope:	public
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *		   	Pointer to valid disk object.
 *		slice	[RO] (int)
 *			Slice index for "/" file system.
 * Return:	D_OK		update successful
 *		D_BADARG	invalid arguments
 *		D_BOOTFIXED	Attempting to alter boot attributes.
 *		D_FAILED	Internal failure to update boot object
 */
int
SdiskobjRootSetBoot(Disk_t *dp, int slice)
{
	char	    disk[32];
	int	    device;
	int	    p;
	Disk_t *    bdp;

	/* validate parameters */
	if ((disk_initialized(dp) && !sdisk_is_usable(dp)) ||
			!valid_sdisk_slice(slice))
		return (D_BADARG);

	if (BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK,   disk,
			BOOTOBJ_DEVICE, &device,
			NULL) != D_OK)
		return (D_FAILED);
	/*
	 * if the boot disk is explictly specified and does not match
	 * the one being set, this is an error on all architectures
	 */
	if (BootobjIsExplicit(CFG_CURRENT, BOOTOBJ_DISK_EXPLICIT)) {
		(void) DiskobjFindBoot(CFG_CURRENT, &bdp);
		if (dp != bdp)
			return (D_BOOTFIXED);
	}

	/* check for explicit device conflicts on SPARC systems */
	if (IsIsa("sparc") && BootobjIsExplicit(CFG_CURRENT,
			BOOTOBJ_DEVICE_EXPLICIT)) {
		if (slice != device)
			return (D_BOOTFIXED);
	}

	/* update the boot disk on all architectures */
	if (BootobjSetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, disk_name(dp),
			NULL) != D_OK) {
		return (D_FAILED);
	}

	/* update the boot device based on architecture */
	if (IsIsa("sparc")) {
		/* on SPARC, the boot device is the slice */
		if (BootobjSetAttribute(CFG_CURRENT,
				BOOTOBJ_DEVICE, slice,
				NULL) != D_OK)
			return (D_FAILED);
	} else if (IsIsa("i386")) {
		/*
		 * on Intel, the boot device is the index of the Solaris
		 * partition containing the slice
		 */
		if (BootobjSetAttribute(CFG_CURRENT,
				BOOTOBJ_DEVICE,
					get_solaris_part(dp, CFG_CURRENT),
				NULL) != D_OK)
			return (D_FAILED);
	} else if (IsIsa("ppc")) {
		/*
		 * on PowerPC, the boot device is the index of the DOS
		 * partition on disk
		 */
		WALK_PARTITIONS(p) {
			if (part_id(dp, p) == DOSOS12 ||
					part_id(dp, p) == DOSOS16)
				break;
		}

		if (valid_fdisk_part(p) &&
				BootobjSetAttribute(CFG_CURRENT,
					BOOTOBJ_DEVICE, p,
					NULL) != D_OK)
			return (D_FAILED);
	}

	return (D_OK);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	coreSliceobjSetSize
 * Description:	Set the size of a specified slice to a user supplied sector
 *		count.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Non-NULL disk structure pointer with valid sdisk geometry pointer
 *			(state: legal)
 *		slice	slice index being set
 *		newsize	size to set the slice to, in 512 byte blocks
 * Returns:	D_OK		size set successfully
 *		D_BADARG  	inavalid argument
 *		D_BADDISK	S-disk geometry pointer is NULL
 *		D_LOCKED	slice is locked and cannot be modified
 *		D_PRESERVED  	slice is preserved and cannot be modified
 *		D_IGNORED	slice is ignored and cannot be changed
 *		D_NOSPACE 	size exceeds physical disk limitations
 *		D_ILLEGAL       failed because S-disk is in an 'illegal' state
 */
static int
coreSliceobjSetSize(Label_t state, Disk_t *dp, int slice, int newsize)
{
	int	blks;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	if (Sdiskobj_Geom(state, dp) == NULL)
		return (D_BADDISK);

	if (SdiskobjIsIllegal(CFG_CURRENT, dp))
		return (D_ILLEGAL);

	if (disk_initialized(dp)) {
		if (SliceobjIsLocked(state, dp, slice))
			return (D_LOCKED);
		if (SliceobjIsPreserved(state, dp, slice))
			return (D_PRESERVED);
		if (SliceobjIsIgnored(state, dp, slice))
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
		if (newsize == Sliceobj_Size(state, dp, slice))
			return (D_OK);

		if (newsize < 0)
			return (D_BADARG);

		/* convert newsize into sectors rounding up to cyl boundry */
		blks = blocks_to_blocks(dp, newsize);
		if (blks > usable_sdisk_blks(dp))
			return (D_NOSPACE);
	}

	if (blks == 0) {
		/* set_slice_geom() relies on this behavior */
		if (!SliceobjIsStuck(state, dp, slice))
			Sliceobj_Start(state, dp, slice) = 0;
	} else {
		if (!SliceobjIsLocked(state, dp, slice) &&
				Sliceobj_Start(state, dp, slice) <
				sdisk_geom_firstcyl(dp))
			Sliceobj_Start(state, dp, slice) = sdisk_geom_firstcyl(dp);
	}

	Sliceobj_Size(state, dp, slice) = blks;
	return (D_OK);
}

/*
 * Function:	coreSliceobjAdjustStart
 * Description:	Adjust the starting cylinder of the specified slice such that
 *		the slice immediately follows whatever non-overlap slice
 *		precedes it. If the size of the slice is 0, the starting cyl
 *		is always set to 0. This routine assumes that the calling
 *		routine is adjusting slices for an entire drive from lowest
 *		slice to highest slice. This means that all slices less than
 *		the specified slice should be adjusted by the time the current
 *		slice is processed.
 *
 *		ALGORITHM:
 *		(1)  if the slice is preserved, overlap, ignored, stuck,
 *		     or locked, return without making any changes.
 *		(2)  if the slice's size is '0', adjust its starting cylinder
 *		     to 0
 *		(3)  move the slice to the start of the drive, and start
 *		     checking to see if its new position overlaps with another
 *		     preserved, locked, stuck, ignored, or preceding slices.
 *		     If so, the current slice is backed up the drive until it
 *		     fits behind one of the slices w/o illegally overlapping.
 *		(4)  if the slice can't fit on the drive, then return D_NOSPACE,
 *		     otherwise, set the starting cylinder of the current slice
 *		     accordingly and return the return value of
 *		     coreSliceobjSetStart().
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	- valid disk structure pointer with a non-NULL sdisk
 *		  	  geometry pointer
 *		slice	  valid slice index number
 * Return:	D_OK	  adjustment successful
 *		D_BADDISK sdisk geometry pointer NULL
 *		D_BADARG  invalid slice number
 *		D_NOSPACE preceding slices fill the disk
 */
static int
coreSliceobjAdjustStart(const Label_t state, Disk_t *dp, const int slice)
{
	int	i;
	int	last;
	int	olast;
	int	count = 0;
	int	*np;
	int	status;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	if (streq(Sliceobj_Use(state, dp, slice), OVERLAP) ||
			SliceobjIsPreserved(state, dp, slice) ||
			SliceobjIsIgnored(state, dp, slice) ||
			SliceobjIsStuck(state, dp, slice) ||
			SliceobjIsLocked(state, dp, slice)) {
		return (D_OK);
	}

	if (Sliceobj_Size(state, dp, slice) == 0) {
		Sliceobj_Start(state, dp, slice) = 0;
		return (D_OK);
	}

	last = sdisk_geom_firstcyl(dp);

	/*
	 * back the slice up the drive until it no longer overlaps an
	 * immovable slice or another slice of a lower slice number
	 */
	while ((count = slice_overlaps(dp, slice, last,
			Sliceobj_Size(state, dp, slice), &np)) > 0) {
		olast = last;
		for (i = 0; i < count; i++) {
			if (SliceobjIsPreserved(state, dp, np[i]) ||
					SliceobjIsIgnored(state, dp, np[i]) ||
					SliceobjIsLocked(state, dp, np[i]) ||
					SliceobjIsStuck(state, dp, np[i]) ||
					np[i] < slice) {
				last = Sliceobj_Start(state, dp, np[i]) +
					blocks_to_cyls(dp,
						Sliceobj_Size(state, dp,
							np[i]));
			}
		}

		if (last == olast)
			break;

		if (last + blocks_to_cyls(dp,
				Sliceobj_Size(state, dp,
					slice)) > sdisk_geom_lcyl(dp))
			return (D_NOSPACE);
	}

	if (last + blocks_to_cyls(dp, Sliceobj_Size(state, dp, slice)) >
			sdisk_geom_lcyl(dp))
		return (D_NOSPACE);

	status = coreSliceobjSetStart(state, dp, slice, last);
	return (status);
}

/*
 * Function:	coreSliceobjSetStart
 * Description: Set the starting cylinder for 'slice'. This is a low-level routine
 *		which does not verify the "okay" or "selected" status of the disk.
 *		This routine should only be called after disk initialization.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	- valid disk structure pointer
 *		slice	- slice index number
 *		start	- starting cylinder index number
 * Return:	D_OK		start set successfully
 *		D_BADARG	invalid argument
 *		D_LOCKED	slice is locked and cannot be changed
 *		D_PRESERVED	slice is preserved and cannot be changed
 *		D_IGNORED	slice is ignored and cannot be changed
 *		D_ILLEGAL       failed because S-disk is in an 'illegal' state
 */
static int
coreSliceobjSetStart(Label_t state, Disk_t *dp, int slice, int start)
{
	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice))
		return (D_BADARG);

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	if (SdiskobjIsIllegal(CFG_CURRENT, dp))
		return (D_ILLEGAL);

	if (disk_initialized(dp)) {
		if (SliceobjIsLocked(state, dp, slice))
			return (D_LOCKED);
		if (SliceobjIsPreserved(state, dp, slice))
			return (D_PRESERVED);
		if (SliceobjIsIgnored(state, dp, slice))
			return (D_IGNORED);

		if (start < 0 || start >= sdisk_geom_lcyl(dp))
			return (D_BADARG);

		if (start < sdisk_geom_firstcyl(dp) &&
				Sliceobj_Size(state, dp, slice) > 0)
			return (D_BADARG);
	}

	Sliceobj_Start(state, dp, slice) = start;
	return (D_OK);
}

/*
 * Function:	SdiskobjRootUnsetBoot
 * Description:	Update the boot object disk and device fields as
 *		appropriate.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *		   	Pointer to valid disk object.
 *		slice	[RO] (int)
 *			Slice index for "/" file system.
 * Return:	D_OK		update successful
 *		D_BOOTFIXED	Attempting to alter boot attributes.
 *		D_FAILED	Internal failure to update boot object
 */
static int
SdiskobjRootUnsetBoot(Disk_t *dp, int slice)
{
	int	device;
	Disk_t * bdp;

	if (IsIsa("sparc") && BootobjIsExplicit(CFG_CURRENT,
			BOOTOBJ_DEVICE_EXPLICIT)) {
		(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DEVICE, &device,
			NULL);
		(void) DiskobjFindBoot(CFG_CURRENT, &bdp);
		if (dp == bdp && slice == device)
			return (D_BOOTFIXED);
	}

	return (D_OK);
}

/*
 * Function:	coreSliceobjSetAttribute
 * Description: Low level routine to modify the attributes associated with
 *		a slice. Fields which are restricted for privileged
 *		modification only are only flagged as an error if their
 *		requested attribute differs from the existing attribute and
 *		the caller does not have privilege.
 * Scope:	private
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Disk pointer.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		privilege	[RO] (int)
 *			Privilege to modify restricted attributes. Valid
 *			values are: 
 *			    PRIVILEGE
 *			    NOPRIVILEGE
 *		ap   	Keyword/attribute varargs list.
 * Return:	D_OK	   modification successful
 *		D_BADARG   invalid argument
 *		D_FAILED   modification failed
 */
/*VARARGS*/
static int
coreSliceobjSetAttribute(const Label_t state, Disk_t *dp, const int slice,
		const int privilege, va_list ap)
{
	SliceobjAttr_t	keyword;
	Sdisk_t		saved;
	int		status = D_OK;
	char *		cp;
	int		ival;
	int		explicit = VAL_UNSPECIFIED;
	int		locked = VAL_UNSPECIFIED;
	int		stuck = VAL_UNSPECIFIED;
	int		ignored = VAL_UNSPECIFIED;
	int		realigned = VAL_UNSPECIFIED;
	int		preserved = VAL_UNSPECIFIED;
	int		size = VAL_UNSPECIFIED;
	int		start = VAL_UNSPECIFIED;
	int		instance = -2;
	char 		usebuf[MAXNAMELEN] = "";
	char * 		use = (char *)VAL_UNSPECIFIED;
	char 		optsbuf[MAXNAMELEN] = "";
	char * 		mountopts = (char *)VAL_UNSPECIFIED;
	int		geomchanged = 0;

	/* validate parameters */
	if (privilege != PRIVILEGE && !sdisk_is_usable(dp))
		return (D_BADARG);

	if (!valid_sdisk_slice(slice))
		return (D_BADARG);

	if (privilege != PRIVILEGE && privilege != NOPRIVILEGE)
		return (D_BADARG);

	while ((keyword = va_arg(ap, SliceobjAttr_t)) != NULL &&
				status == D_OK) {
		switch (keyword) {
		    case SLICEOBJ_USE:
			cp = va_arg(ap, char *);
			if (cp == NULL || cp < (char *)0) {
				status = D_BADARG;
			} else if (slice_name_ok(cp) != D_OK) {
				status = D_BADARG;
			} else if (!streq(Sliceobj_Use(state, dp, slice), cp)) {
				(void) strcpy(usebuf, cp);
				use = &usebuf[0];
			}
			break;

		    case SLICEOBJ_INSTANCE:
			ival = va_arg(ap, int);
			if (ival < 0 && ival != VAL_UNSPECIFIED)
				status = D_BADARG;
			else if (instance != Sliceobj_Instance(state, dp, slice))
				instance = ival;
			break;

		    case SLICEOBJ_START:
			ival = va_arg(ap, int);
			if (ival < 0)
				status = D_BADARG;
			else if (start != Sliceobj_Start(state, dp, slice))
				start = ival;
			break;

		    case SLICEOBJ_SIZE:
			ival = va_arg(ap, int);
			if (ival < 0 && ival != GEOM_REST)
				status = D_BADARG;
			else if (size != Sliceobj_Size(state, dp, slice))
				size = ival;
			break;

		    case SLICEOBJ_MOUNTOPTS:
			cp = va_arg(ap, char *);
			if (cp == NULL)
				status = D_BADARG;
			else if (!streq(Sliceobj_Mountopts(state, dp, slice), cp)) {
				(void) strcpy(optsbuf, cp);
				mountopts = &optsbuf[0];
			}
			break;

		    case SLICEOBJ_EXPLICIT:
			ival = va_arg(ap, int);
			if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				explicit = ival;
			break;

		    case SLICEOBJ_STUCK:
			ival = va_arg(ap, int);
			if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				stuck = ival;
			break;

		    case SLICEOBJ_IGNORED:
			ival = va_arg(ap, int);
			if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				ignored = ival;
			break;

		    case SLICEOBJ_PRESERVED:
			ival = va_arg(ap, int);
			if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				preserved = ival;
			break;

		    case SLICEOBJ_LOCKED:		/* RESTRICTED */
			ival = va_arg(ap, int);
			if (privilege != PRIVILEGE)
				status = D_BADARG;
			else if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				locked = ival;
			break;
			
		    case SLICEOBJ_REALIGNED:		/* RESTRICTED */
			ival = va_arg(ap, int);
			if (privilege != PRIVILEGE)
				status = D_BADARG;
			else if (ival != TRUE && ival != FALSE)
				status = D_BADARG;
			else
				realigned = ival;
			break;

		    default:
			status = D_BADARG;
			break;
		}
	}

	/* save a copy of the unmodified sdisk configuration */
	(void) memcpy(&saved, Sdiskobj_Addr(state, dp), sizeof (Sdisk_t));

	/* first turn off all flags which were deactivated */
	if (status == D_OK && explicit == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_EXPLICIT);

	if (status == D_OK && stuck == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_STUCK);

	if (status == D_OK && ignored == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_IGNORED);

	if (status == D_OK && locked == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_LOCKED);

	if (status == D_OK && realigned == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_REALIGNED);

	if (status == D_OK && preserved == FALSE)
		SliceobjClearBit(state, dp, slice, SLF_PRESERVED);

	/*
	 * set the value components
	 */
	if (status == D_OK && (use != (char *) VAL_UNSPECIFIED ||
				instance != -2)) {
		status = coreSliceobjSetKey(state, dp, slice, privilege,
				use == (char *)VAL_UNSPECIFIED ?
					Sliceobj_Use(state, dp, slice) : use, 
				instance == -2 ?
					Sliceobj_Instance(state,
						dp, slice) : instance);
	}

	/* size setting should always precede start setting */
	if (status == D_OK && size != VAL_UNSPECIFIED) {
		if ((status = coreSliceobjSetSize(state, dp,
				slice, size)) == D_OK)
			geomchanged++;
	}

	if (status == D_OK && start != VAL_UNSPECIFIED) {
		if ((status = coreSliceobjSetStart(state, dp,
				slice, start)) == D_OK)
			geomchanged++;
	}

	if (status == D_OK && mountopts != (char *)VAL_UNSPECIFIED) {
		/* Save this for a time when you can be more strict
		if (!streq(mountopts, "") &&
				!is_pathname(Sliceobj_Use(state, dp, slice)))
			status = D_BADARG;
		else
		*/
		(void) strcpy(Sliceobj_Mountopts(state, dp, slice), mountopts);
	}

	/* enable all flags which were set by the user */
	if (status == D_OK && explicit == TRUE)
		SliceobjSetBit(state, dp, slice, SLF_EXPLICIT);

	if (status == D_OK && stuck == TRUE)
		SliceobjSetBit(state, dp, slice, SLF_STUCK);

	if (status == D_OK && locked == TRUE)
		SliceobjSetBit(state, dp, slice, SLF_LOCKED);

	if (status == D_OK && realigned == TRUE)
		SliceobjSetBit(state, dp, slice, SLF_REALIGNED);

	if (status == D_OK && ignored == TRUE) {
		if (status == D_OK && Sliceobj_Size(state, dp, slice) == 0)
			status = D_ZERO;

		if (status == D_OK && SliceobjIsRealigned(CFG_EXIST, dp, slice))
			status = D_ALIGNED;

		if (status == D_OK && sdisk_geom_same(dp, CFG_EXIST) != D_OK)
			status = D_CHANGED;;

		if (status == D_OK && (Sliceobj_Start(state, dp, slice)
				    != Sliceobj_Start(CFG_EXIST, dp, slice) ||
				Sliceobj_Size(state, dp, slice) !=
				    Sliceobj_Size(CFG_EXIST, dp, slice)) &&
				disk_initialized(dp))
			status = D_CHANGED;

		if (status == D_OK) {
			if (slice_overlaps(dp, slice,
					Sliceobj_Start(state, dp, slice),
					Sliceobj_Size(state, dp, slice),
					NULL) != 0 &&
				    !streq(Sliceobj_Use(state,
					dp, slice), OVERLAP))
				status = D_OVER;
		}

		if (status == D_OK)
			SliceobjSetBit(state, dp, slice, SLF_IGNORED);
	}

	if (status == D_OK && preserved == TRUE) {
		if (status == D_OK && Sliceobj_Size(state, dp, slice) == 0)
			status = D_ZERO;

		if (status == D_OK)
			status = slice_preserve_ok(dp, slice);

		if (status == D_OK && disk_initialized(dp)) {
			if (Sliceobj_Start(state, dp, slice) !=
					Sliceobj_Start(CFG_EXIST, dp, slice) ||
				    Sliceobj_Size(state, dp, slice) !=
					Sliceobj_Size(CFG_EXIST, dp, slice))
				status = D_CHANGED;
		}

		if (status == D_OK) {
			if (slice_overlaps(dp, slice,
					Sliceobj_Start(state, dp, slice),
					Sliceobj_Size(state, dp, slice),
					NULL) != 0 &&
				    !streq(Sliceobj_Use(state, dp, slice), OVERLAP))
				status = D_OVER;
		}

		if (status == D_OK)
			SliceobjSetBit(state, dp, slice, SLF_PRESERVED);
	}

	/*
	 * if the start or size of the slice has changed and autoadjust
	 * is enabled, then adjust the slice starts
	 */
	if (status == D_OK && disk_initialized(dp) && \
			geomchanged && get_slice_autoadjust())
		status = adjust_slice_starts(dp);

	/* restore the original configuration if the update failed */
	if (status != D_OK)
		(void) memcpy(Sdiskobj_Addr(state, dp), &saved, sizeof (Sdisk_t));

	return (status);
}

/*
 * Function:	coreSliceobjGetAttribute
 * Description:	Low level routine to retrieve slice attributes.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk pointer.
 *		slice	[RO] (int)
 *			Valid slice index.
 *		privilege	[RO] (int)
 *			Privilege to modify restricted attributes. Valid
 *			values are: 
 *			    PRIVILEGE
 *			    NOPRIVILEGE
 *		ap   	Keyword/attribute varargs list.
 * Return:	D_OK	   modification successful
 *		D_BADARG   invalid argument
 *		D_FAILED   modification failed
 */
/*ARGSUSED2*/
static int
coreSliceobjGetAttribute(const Label_t state, const Disk_t *dp, const int slice,
			const int privilege, va_list ap)
{
	SliceobjAttr_t	keyword;
	int		status = D_OK;
	int *		ip;
	char *		cp;

	while ((keyword = va_arg(ap, SliceobjAttr_t)) != NULL &&
				status == D_OK) {
		switch (keyword) {
		    case SLICEOBJ_USE:
			cp = va_arg(ap, char *);
			if (cp == NULL)
				status = D_BADARG;
			else
				(void) strcpy(cp, Sliceobj_Use(state, dp, slice));
			break;

		    case SLICEOBJ_INSTANCE:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = Sliceobj_Instance(state, dp, slice);
			break;

		    case SLICEOBJ_MOUNTOPTS:
			cp = va_arg(ap, char *);
			if (cp == NULL)
				status = D_BADARG;
			else
				(void) strcpy(cp, Sliceobj_Mountopts(state,
						dp, slice) ?
					    Sliceobj_Mountopts(state,
						dp, slice) : "");
			break;

		    case SLICEOBJ_EXPLICIT:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsExplicit(state, dp, slice);
			break;

		    case SLICEOBJ_STUCK:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsStuck(state, dp, slice);
			break;

		    case SLICEOBJ_IGNORED:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsIgnored(state, dp, slice);
			break;

		    case SLICEOBJ_LOCKED:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsLocked(state, dp, slice);
			break;

		    case SLICEOBJ_PRESERVED:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsPreserved(state, dp, slice);
			break;

		    case SLICEOBJ_REALIGNED:
			ip = va_arg(ap, int *);
			if (ip == NULL)
				status = D_BADARG;
			else
				*ip = SliceobjIsRealigned(state, dp, slice);
			break;

		    default:
			status = D_BADARG;
			break;
		}
	}

	return (status);
}

/*
 * Function:    SliceobjFindUsePerDisk
 * Description: Search all slices on a specific disk for the designated
 *		resource. The slice "use" field must always match the
 *		resource name exactly. The slice instance field matching
 *		is controlled by the 'match' parameter. If 'match' is set to
 *		 '1', the instance must also exactly match between
 *		the slice and the resource. If 'match' is '0', a match
 *		is considered to be found if there is either an exact
 *		match, or the slice instance is '-1' (unspecified).
 * Scope:       private
 * Parameters:	state   [RO] (Label_t)
 *			Make the search in the user specified slice state. Valid
 *			values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Pointer to a valid disk object that has a usable
 *			sdisk slice configuration.
 *		name	[RO, *RO] (Resobj *)
 *			Non-null resource name pointer.
 *		instance [RO] (int)
 *			Non-negative resource instance number.
 *		match	[RO] (int)
 *			Match flag indicating the degree of exactness for
 *			which the instance must match. Valid values are:
 *			  0 - exact or unspecified slice instances match
 *			  1 - only exact slice instance match
 * Return:	D_FAILED  No match found.
 *		D_BADARG  Invalid argument specified.
 *		# >= 0    Slice index for matching slice.
 */
static int
SliceobjFindUsePerDisk(Label_t state, Disk_t *dp, char *name,
		int instance, int match)
{
	int     slice;

	/* validate parameters */
	if (dp == NULL || !sdisk_is_usable(dp) || name == NULL || instance < 0)

	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	WALK_SLICES(slice) {
		if (!SliceobjIsIgnored(state, dp, slice) &&
				streq(Sliceobj_Use(state, dp, slice), name)) {
			if (match && Sliceobj_Instance(state,
					dp, slice) == instance)
				return (slice);

			if (!match && Sliceobj_Instance(state, dp,
					slice) == VAL_UNSPECIFIED)
				return (slice);
		}
	}

	return (D_FAILED);
}

/*
 * Function:	coreSliceobjSetKey
 * Description:	Low level routine which actually updates the use and instance
 *		components of the slice key.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *			    CFG_COMMIT
 *			    CFG_EXIST
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk handle.
 *		slice	[RO] (int)
 *			Valid slice index
 *		privilege [RO] (int)
 *		use	[RO, *RO] (char[])
 *		instance  [RO] (int)
 * Return:
 */
/*ARGSUSED2*/
static int
coreSliceobjSetKey(Label_t state, Disk_t *dp, int slice, int privilege,
		char *use, int instance)
{
	int	status;

	/* 
	 * make sure the key isn't already known in the current name space
	 */
	if (instance >= 0) {
		if (SliceobjFindUse(state, dp, use, instance, 1) != NULL)
			return (D_DUPMNT);
	}

	if (streq(Sliceobj_Use(state, dp, slice), ROOT) &&
			Sliceobj_Instance(state, dp, slice) == 0 &&
			disk_initialized(dp)) {
		if ((status = SdiskobjRootUnsetBoot(dp, slice)) != D_OK)
			return (status);
	}

	if (streq(use, ROOT) && instance == 0 && disk_initialized(dp)) {
		/* PowerPC can only put "/" on slice '0' */
		if (IsIsa("ppc") && slice != 0)
			return (D_BOOTFIXED);

		/*
		 * setting "/" implies a boot object selection if
		 * the disk is initialized
		 */
		if ((status = SdiskobjRootSetBoot(dp, slice)) != D_OK)
			return (status);
	}
	
	/*
	 * changing slice 2 to "overlap" (any instance) automatically
	 * resets the geometry and clears
	 */
	if (slice == ALL_SLICE &&
			!streq(Sliceobj_Use(state, dp, slice), OVERLAP) &&
			streq(use, OVERLAP))
		(void) coreSliceobjReset(state, dp, slice);

	/*
	 * if changing the use from a file system mount point to a
	 * non-file system mount point, clear the mount options
	 */
	if (is_pathname(Sliceobj_Use(state, dp, slice)) && !is_pathname(use))
		(void) strcpy(Sliceobj_Mountopts(state, dp, slice), "");

	/* set the use and instance */
	(void) strcpy(Sliceobj_Use(state, dp, slice), use);
	Sliceobj_Instance(state, dp, slice) = instance;

	/*
	 * if the slice was preserved and the new use is not longer
	 * preservable, clear the preserve bit
	 */
	if (SliceobjIsPreserved(state, dp, slice)) {
		if (slice_preserve_ok(dp, slice) == D_CANTPRES)
			SliceobjClearBit(state, dp, slice, SLF_PRESERVED);
	}

	return (D_OK);
}

/*
 * Function:	coreSliceobjReset
 * Description:	Reset the state and data fields for a specific slice to a NULL
 *		(or for system slices, default) state.
 *
 *		ALGORITHM:
 *		(1)  clear all the slice specific state flags (except 'locked')
 *		(2)  for non-system slices, set the size and start cyl to '0'
 *		     and clear the mount point and mount options fields.
 *		(3)  for system slices, set the slice configuration as follows:
 *		     (a)  alternate sector slice (FDISK SYSTEMS ONLY)
 *			  If the suggested alternate sector size is not '0', and
 *			  the disk geometry hasn't changed since load time, and
 *			  there was a non-zero alternate sector slice already
 *			  defined on the drive, then restore the original config.
 *			  If the suggested alt sector size is '0', then set the
 *			  slice size and start cylinder to '0' regardless of the
 *		  	original configuration
 *		     (b)  all slice (slice 2)
 *			  set the mount point name to "overlap", the starting cyl
 *			  to '0', and the size to the total # of blocks on the
 *			  disk
 *			  NOTE: this may have to change if the definition of
 *				slice 2 is altered
 *		     (c)  boot slice (FDISK SYSTEMS ONLY)
 *			  Set the starting cyl to '0', and the size to '1' cyl
 *
 *		NOTE:	This must be called after the S-disk geom has been set
 *			up and the data values are valid, otherwise the
 *			firstcyl etc. macros will be erroneously NULL.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			Direct the search to the current, committed, or existing
 *			state of the slice object. Valid values are:
 *			    CFG_CURRENT
 *		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle with a valid sdisk geometry.
 *		slice	[RO] (int)
 *			Index of slice to be reset.
 * Return:	D_OK		reset successful
 *		D_BADARG	invalid argument 
 *		D_BADDISK	sdisk geometry pointer is NULL
 */
static int
coreSliceobjReset(const Label_t state, Disk_t *dp, const int slice)
{
	int	asect;
	char *	use = "";
	int	start = 0;
	int	size = 0;

	/* validate parameters */
	if (dp == NULL || !valid_sdisk_slice(slice) || state != CFG_CURRENT)
		return (D_BADARG);

	if (sdisk_geom_null(dp))
		return (D_BADDISK);

	switch (slice) {
	    case ALT_SLICE:
		use = ALTSECTOR;
		if ((asect = coreSliceobjGetAltsectorSize(dp)) > 0) {
			/*
			 * if alternate sectors are required and the CURRENT
			 * alternate sector slice has no size, set the size to
			 * the default value, otherwise keep the current size;
			 * you must check current because this routine is run
			 * during initialization and existing isn't set at that
			 * time
			 */
			if (Sliceobj_Size(CFG_CURRENT, dp, ALT_SLICE) == 0) {
				start = sdisk_geom_firstcyl(dp);
				size = asect;
			} else {
				start = Sliceobj_Start(CFG_CURRENT, dp, ALT_SLICE);
				size = Sliceobj_Size(CFG_CURRENT, dp, ALT_SLICE);
			}
		} else {
			start = 0;
			size = 0;
		}
		break;
	    case ALL_SLICE:
		use = OVERLAP;
		start = 0;
		size = sdisk_geom_lcyl(dp) *
			sdisk_geom_onecyl(dp);
		break;
	    case BOOT_SLICE:
		start = 0;
		size = one_cyl(dp);
		break;
	}

	/*
	 * DO NOT USE SliceobjSet* routines or infinite loop will result
	 */
	(void) strcpy(Sliceobj_Use(state, dp, slice), use);
	(void) strcpy(Sliceobj_Mountopts(state, dp, slice), "");
	Sliceobj_Instance(state, dp, slice) = VAL_UNSPECIFIED;
	Sliceobj_Start(state, dp, slice) = start;
	Sliceobj_Size(state, dp, slice) = size;
	SliceobjClearBit(state, dp, slice, SLF_PRESERVED);
	SliceobjClearBit(state, dp, slice, SLF_EXPLICIT);
	SliceobjClearBit(state, dp, slice, SLF_STUCK);
	SliceobjClearBit(state, dp, slice, SLF_IGNORED);

	return (D_OK);
}

/*
 * Function:	coreSliceobjGetAltsectorSize
 * Description:	Return the suggested alternate sector slice size for the
 *		given drive. Alternate sectors are only required on fdisk
 *		systems with non-SCSI disks.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of sectors required for the atlernate sector
 *			  slice
 */
static int
coreSliceobjGetAltsectorSize(const Disk_t *dp)
{
	if (dp == NULL || !disk_fdisk_exists(dp))
		return (0);

	if (_disk_is_scsi(dp))
		return (0);

	return (NUMALTSECTCYL * one_cyl(dp));
}
