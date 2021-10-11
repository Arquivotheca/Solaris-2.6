#ifndef lint
#pragma ident "@(#)disk_fdisk.c 1.59 95/04/07 SMI"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */
/*
 * MODULE PURPOSE:	This module contains all routines used to manipulate
 *			fdisk components
 */
#include "disk_lib.h"

/* Public Function Prototypes */

int		set_part_geom(Disk_t *, int, int, int);
int		set_part_attr(Disk_t *, int, int, int);
int		part_geom_same(Disk_t *, int, Label_t);
int		adjust_part_starts(Disk_t *);
int		fdisk_config(Disk_t *, char *, Label_t);
int		part_overlaps(Disk_t *, int, int, int, int **);
int		fdisk_space_avail(Disk_t *);
int		set_part_preserve(Disk_t *, int, int);
int		get_solaris_part(Disk_t *, Label_t);
int		max_size_part_hole(Disk_t *, int);

/* Library Function Prototypes */

int		_set_part_size(Disk_t *, int, int);
int		_set_part_start(Disk_t *, int, int);
int		_reset_fdisk(Disk_t *);

/* Local Function Prototypes */

static int	_find_part_id(Disk_t *, int);
static int	_fdisk_segment_size(Disk_t *, int);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * set_part_preserve()
 *	Set the preserve state for a partition (PRES_YES or PRES_NO).
 *	The partition geometry must be unchanged since the original drive
 *	configuration for the state to be set to PRES_YES. "0" sized
 *	partitions cannot be set to PRES_YES. Any partition can bet set
 *	to PRES_NO (given the disk constraints specified in the parameter
 *	list below).
 * Parameters:
 *	dp	- non-null disk structure pointer
 *		  (state:  okay, selected)
 *	part	- fdisk partition index number (1 - 4)
 *	val	- preservation value (PRES_NO or PRES_YES)
 * Return:
 *	D_OK		- update successful
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk not available for specified operation
 *	D_NOTSELECT	- disk state was not selected
 *	D_CHANGED	- the partition start and/or size has changed
 *			  since the original configuration
 *	D_ZERO		- preserve set to PRES_YES failed because partition
 *			  size is zero
 * Status:
 *	public
 */
int
set_part_preserve(Disk_t *dp, int part, int val)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (val == PRES_NO) {
		part_preserve_off(dp, part);
		return (D_OK);
	}

	if (part_size(dp, part) == 0)
		return (D_ZERO);

	if (part_startcyl(dp, part) != orig_part_startcyl(dp, part) &&
			part_size(dp, part) != orig_part_size(dp, part) &&
			disk_initialized(dp))
		return (D_CHANGED);

	part_preserve_on(dp, part);
	return (D_OK);
}

/*
 * part_overlaps()
 *	Determine if the fdisk partition specified (or the parameters
 *	specified) overlaps another partition already on the disk (not
 *	counting UNUSED partitions).
 *	WARNING:	'start' should be part_geom_rsect() if you are
 *			passing in dimensions for an existing partition
 *			Specified in sectors (not cylinders)
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	part	- index of partition being validated (1 - 4), or -1 if no
 *		  partition specified
 *	start	- proposed starting sector for partition being validated
 *	size	- proposed sector count size for partition being validated
 *	olpp	- address to an integer pointer initialized to the address
 *		  of an integer array containing a list of partitions which
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
part_overlaps(Disk_t *dp, int part, int start, int size, int **olpp)
{
	static int  _overlap_parts[FD_NUMPART];
	int	    f, pend, fstart;
	int	    count = 0;

	if (dp == NULL)
		return (0);

	if (disk_not_okay(dp))
		return (0);

	if (part != -1 && part_id(dp, part) == UNUSED)
		return (0);

	pend = start + size;	/* all calculations are done in sectors */

	WALK_PARTITIONS(f) {
		if (f == part || part_id(dp, f) == UNUSED)
			continue;

		fstart = part_geom_rsect(dp, f);

		if (fstart >= start && fstart < pend &&
				part_size(dp, f) > 0 &&
				part_id(dp, f) != UNUSED)
			_overlap_parts[count++] = f;
	}

	if (olpp != (int **)NULL)
		*olpp = _overlap_parts;

	return (count);
}

/*
 * set_part_geom()
 *	Set the size of a specific disk fdisk partition. Size is
 *	specified in 512 byte blocks rounded up to the nearest cylinder
 *	boundary. Cylinder rounding is enforced on all new partitions,
 *	however, non-cylinder boundary alignments on existing partitions
 *	are honored (since other fdisk programs don't necessarily enforce
 *	rounding). Preserved partitions cannot be modified.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay,  selected)
 *	part	- partition index (1 - 4)
 *	start   - partition starting cylinder:
 *			GEOM_ORIG    - original setting
 *			GEOM_COMMIT  - last committed setting
 *			GEOM_IGNORE  - ignore this parameter
 *			#	     - explicit starting cylinder
 *	size	- number of sectors specifying the size of the partition
 *		  (this value will be rounded up to the nearest cylinder
 *		  boundary automatically by this routine). Other values
 *		  which can be used are:
 *			GEOM_ORIG    - original size
 *			GEOM_COMMIT  - last committed size
 *			GEOM_IGNORE  - ignore this parameter
 *			#	     - explicit size in sectors
 *			GEOM_REST    - whatever isn't already assigned
 *				       to another partition within the
 *				       partition's hole
 * Return:
 *	D_OK		- geometry set successfully
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	D_NOTSELECT	- disk not selected
 *	D_BADARG    	- 'part' was not 1 - 4, or start/size were not valid
 *	D_PRESERVED	- 'part' is preserved and cannot be modified
 *	D_NOSPACE	- there is insufficient space on the drive to
 *			  satisfy all size requests
 *	D_NOFIT		- at least one partition doesn't fit in the disk
 * Status:
 *	public
 */
int
set_part_geom(Disk_t *dp, int part, int start, int size)
{
	Fdisk_t		saved;
	int		status;
	int		startblk;
	int		modified = 0;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	if (invalid_fdisk_part(part))
		return (D_BADARG);

	switch (start) {

	case GEOM_ORIG:
		startblk = orig_part_startsect(dp, part);
		break;
	case GEOM_COMMIT:
		startblk = comm_part_startsect(dp, part);
		break;
	case GEOM_IGNORE:
		break;
	default:
		if (start < 0)
			return (D_BADARG);
		startblk = cyls_to_blocks(dp, start);
		break;
	}

	switch (size) {

	case GEOM_ORIG:
		size = orig_part_size(dp, part);
		break;
	case GEOM_COMMIT:
		size = comm_part_size(dp, part);
		break;
	case GEOM_REST:
	case GEOM_IGNORE:
		break;
	default:
		if (size < 0)
			return (D_BADARG);
		break;
	}
	/*
	 * save a copy of the fdisk in case you there is a failure part
	 * way through the update
	 */
	(void) memcpy(&saved, disk_fdisk_addr(dp), sizeof (Fdisk_t));

	/* set the starting cylinder (if necessary) */
	if (start != GEOM_IGNORE && startblk != part_startsect(dp, part)) {
		modified++;
		if ((status = _set_part_start(dp, part, startblk)) != D_OK) {
			(void) memcpy(disk_fdisk_addr(dp), &saved,
							sizeof (Fdisk_t));
			return (status);
		}
	}

	/* set the size (if necessary) */
	if (size != GEOM_IGNORE && size != part_size(dp, part)) {
		modified++;
		if ((status = _set_part_size(dp, part, size)) != D_OK) {
			(void) memcpy(disk_fdisk_addr(dp), &saved,
						sizeof (Fdisk_t));
			return (status);
		}
	}

	/*
	 * if the geometry of any partition changes, readjust partition
	 * starting cylinders (if necessary) and clear out the S-disk
	 * geometry pointer (must be reset with validate_fdisk())
	 */
	if (modified) {
		if ((status = adjust_part_starts(dp)) != D_OK) {
			(void) memcpy(disk_fdisk_addr(dp), &saved,
					sizeof (Fdisk_t));
			sdisk_geom_clear(dp);
			return (status);
		}

		sdisk_geom_clear(dp);
	}

	return (D_OK);
}

/*
 * set_part_attr()
 *	Set a partition attirbute ("id" or "active") for a
 *	specific fdisk partition. At most one partition can be
 *	active at a time. Setting a partition to UNUSED clears
 *	out its geometry fields. Setting a partition to SUNIXOS
 *	adjusts its data cylinder/sector and first/last cylinder
 *	fields. Changing a partition from UNUSED to anything else
 *	results in a call to adjust_part_starts(), since UNUSED
 *	partitions are permitted to overlap, and all other
 *	partitions are not. Note that preserved partitions are
 *	permitted to have active status modifications, just not
 *	type modifications.
 *
 *	ALGORITHM:
 *	(1) Validate the disk and arguments
 *	(2) If the 'id' is not GEOM_IGNORE and is different from
 *	    what is currently out there, then:
 *	    - if the new id is SUNIXOS, then make sure there isn't
 *	      another SUNIXOS partition on the drive (multiple SUNIXOS
 *	      is not allowed), and set up the dcyl/dsect and firstcyl/
 *	      lcyl values appropriately
 *	(3) If the 'id' is UNUSED, then:
 *	    - clear all the geometry fields except nsect and onecyl,
 *	      and turn off the preserve and stuck flags in the partition
 *	(4) If the existing id is SUNIXOS, then readjust the dcyl/desct
 *	    and firstcyl/lcyl
 *	(5) If the existing id is UNUSED, then adjust_part_starts()
 *	    since UNUSED are permitted to overlap, and other partition
 *	    types are not
 *	(6) If the 'active' value is not GEOM_IGNORE and is different
 *	    from the current value, then, if 'active' is ACTIVE,
 *	    deactivate the current active partition (if there is one).
 *	    Set the active state as specified.
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	part	- partition number (1 - 4)
 *	id	- fdisk parititon id specifier:
 *			0 - 0xFF	- hex value for partition id
 *			GEOM_IGNORE	- attribute is not to be changed
 *	active	- Reboot active flag:
 *			ACTIVE		- make the partition active
 *			NOTACTIVE	- make the partition inactive
 *			GEOM_IGNORE	- attribute is not to be changed
 * Return:
 *	D_OK	    - attribute set successfully
 *	D_NODISK    - 'dp' is NULL
 *	D_BADDISK   - disk state is not applicable to this operation
 *	D_NOTSELECT - disk is not "selected"
 *	D_BADARG    - 'id', 'active', or 'part' are invalid
 *	D_PRESERVED - 'part' is preserved and cannot be modified
 * Status:
 *	public
 */
int
set_part_attr(Disk_t *dp, int part, int id, int active)
{
	int	i;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp))
		return (D_NOTSELECT);

	if (invalid_fdisk_part(part))
		return (D_BADARG);

	if (id != GEOM_IGNORE && id != part_id(dp, part)) {

		if (part_preserved(dp, part) && disk_initialized(dp))
			return (D_PRESERVED);

		switch (id) {

		case   SUNIXOS:
			if (_find_part_id(dp, id))
				return (D_DUPMNT);

			if (part_geom_tcyl(dp, part) == 0)
				break;

			part_geom_firstcyl(dp, part) = 1;
			part_geom_lcyl(dp, part) =
				part_geom_tcyl(dp, part) - NUMALTCYL;
			part_geom_lsect(dp, part) =
				part_geom_lcyl(dp, part) *
					disk_geom_onecyl(dp);
			part_geom_dcyl(dp, part) =
				part_geom_lcyl(dp, part) -
					part_geom_firstcyl(dp, part);
			part_geom_dsect(dp, part) =
				part_geom_dcyl(dp, part) *
					disk_geom_onecyl(dp);
			break;

		case   UNUSED:
			part_geom_firstcyl(dp, part) = 0;
			part_geom_lcyl(dp, part) = 0;
			part_geom_lsect(dp, part) = 0;
			part_geom_dcyl(dp, part) = 0;
			part_geom_dsect(dp, part) = 0;
			part_geom_tcyl(dp, part) = 0;
			part_geom_tsect(dp, part) = 0;
			part_geom_rsect(dp, part) = 0;
			/* don't touch nsect, nhead, onecyl, or hbacyl */

			part_active_set(dp, part, NOTACTIVE);
			part_stuck_off(dp, part);
			part_preserve_off(dp, part);

			/* adjust the remaining partition starts */
			(void) adjust_part_starts(dp);
			break;

		default:
			if (id < 0 || id > 0xFF)
				return (D_BADARG);
			if (part_id(dp, part) == SUNIXOS) {
				part_geom_firstcyl(dp, part) = 0;
				part_geom_lcyl(dp, part) =
						part_geom_tcyl(dp, part);
				part_geom_lsect(dp, part) =
						part_geom_tsect(dp, part);
				part_geom_dcyl(dp, part) = blocks_to_cyls(dp,
						part_geom_tsect(dp, part));
				part_geom_dsect(dp, part) =
						part_geom_tsect(dp, part);
			}
			break;
		}

		if (part_id(dp, part) == SUNIXOS)
			sdisk_geom_clear(dp);

		part_id_set(dp, part, id);
	}

	if (active != GEOM_IGNORE && active != part_active(dp, part)) {
		if (active == ACTIVE) {
			WALK_PARTITIONS(i)
				part_active_set(dp, i, NOTACTIVE);
		} else if (active != NOTACTIVE)
			return (D_BADARG);
		part_active_set(dp, part, active);
	}

	return (D_OK);
}

/*
 * part_geom_same()
 *	Test to see if the partition geometry has changed between
 *	the current configuration and the committed/existin configuration.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 *	label	- specify which configuration the current configuration
 *		  is to be compared against (CFG_COMMIT or CFG_EXIST)
 * Return:
 *	D_OK		- partition geometry unchanged
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk state not applicable to this operation
 *	D_BADARG	- invalid 'part', or 'label' value
 *	D_GEOMCHNG	- S-disk geometry changed since disk last committed
 * Status:
 *	public
 */
int
part_geom_same(Disk_t *dp, int  part, Label_t label)
{
	Geom_t	*gp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	switch (label) {

	case  CFG_EXIST:
		gp = orig_part_geom_addr(dp, part);
		break;

	case CFG_COMMIT:
		gp = comm_part_geom_addr(dp, part);
		break;

	default:
		return (D_BADARG);
	}

	if (memcmp(&part_geom(dp, part), gp, sizeof (Geom_t)) != 0)
		return (D_GEOMCHNG);

	return (D_OK);
}

/*
 * fdisk_space_avail()
 * 	Calculate the overall number of unallocated 512 byte blocks
 *	on a specified disk. UNUSED partitions are ignored. The
 *	size is rounded down to the nearest cylinder (since partial
 *	cylinders are not available for use).
 *
 *	NOTE:	partial cylinders should only result when an existing
 *		fdisk label was created by a non-Solaris fdisk
 *		program and starts before cylinder 1 (not on a cylinder
 *		boundary)
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 * Returns:
 *	0	- no space available
 *	#	- number of unallocated usable blocks
 * Status:
 *	public
 */
int
fdisk_space_avail(Disk_t *dp)
{
	int	size = 0;
	int	p;

	if (dp == NULL || disk_not_okay(dp))
		return (0);

	WALK_PARTITIONS(p) {
		if (part_id(dp, p) != UNUSED)
			size += part_size(dp, p);
	}
	/*
	 * round used size up to nearest cylinder (which results
	 * in the available size being rounded down)
	 */
	size = blocks_to_blocks(dp, size);

	if (size > usable_disk_blks(dp) ||
			(size < one_cyl(dp) && size > 0))
		return (0);

	return (usable_disk_blks(dp) - size);
}

/*
 * fdisk_config()
 *	Routine to configure the entire current fdisk structure
 *	The action taken is determed by the parameter 'label' (see
 *	below).
 *
 *	NOTE:	The S-disk geometry pointer is cleared by called
 *		routines whenever appropriate. Need to call
 *		validate_fdisk() to reinstate that pointer before
 *		accessing the S-disk in subsequent calls.
 * Parameter:
 *	disk	- disk structure pointer - NULL if specifying drive
 *		  by 'drive'. 'dp' has precedence in the drive order
 *		  (state:  okay, selected)
 *	drive	- name of drive (e.g. c0t0d0) - NULL if specifying
 *		  drive by 'dp'
 *	label	- configuration strategy specifier:
 *
 *		  CFG_NONE    - clear all flags and fields
 *		  CFG_DEFAULT - clear all flags and fields, and install
 *				a single Solaris partition which consumes
 *				the entire drive
 *		  CFG_EXIST   - make identical to the original fdisk
 *				configuration
 *		  CFG_COMMIT  - make identical to the last commited fdisk
 *				configuration
 * Return:
 *	D_OK	    - disk state set successfully
 *	D_NODISK    - neither argument was specified
 *	D_BADDISK   - disk state not valid for requested operation
 *	D_BADARG    - illegal 'label' value
 *	D_NOTSELECT - disk state was not selected
 * Status:
 *	public
 */
int
fdisk_config(Disk_t * disk, char * drive, Label_t label)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	/*
	 * excluding fdisk_config() from system which do not expose
	 * the fdisk interface (not just based on existence)
	 */
	if (disk_no_fdisk_req(dp))
		return (D_OK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp))
		return (D_NOTSELECT);

	switch (label) {
	    case CFG_NONE:
		return (_reset_fdisk(dp));

	    case CFG_DEFAULT:
		return (_setup_fdisk_default(dp));

	    case CFG_EXIST:
		return (_restore_fdisk_orig(dp));

	    case CFG_COMMIT:
		return (_restore_fdisk_commit(dp));
	}

	return (D_BADARG);
}

/*
 * adjust_part_starts()
 *	Adjust the current fdisk partition starting cylinders so that
 *	partitions are contiguous. UNUSED partitions and partitions of
 *	size 0 are not adjusted, and are permitted to overlap. Preserved
 *	and stuck partitions may result in gaps. This is not considered
 *	an error.
 *
 *	NOTE:	This routine is called automatically whenever set_part_geom()
 *		changes a partition geometry, or set_part_attr() changes a
 *		partition ID. This should not need to be called manually.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay, selected)
 * Return:
 *	D_OK	    - adjustment successful
 *	D_NODISK    - 'dp' is NULL
 *	D_BADDISK   - disk state is not valid for requested operation
 *	D_NOTSELECT - disk state is not "selected"
 *	D_NOSPACE   - there is insufficient space on the drive to
 *			satisfy all size requests
 *	D_NOFIT	    - at least one partition doesn't fit in the disk
 *			segment in which it is specified (bumps up
 *			against a preserved or stuck partition)
 * Status:
 *	public
 */
int
adjust_part_starts(Disk_t *dp)
{
	int	start, i;
	Fdisk_t	saved;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	(void) memcpy(&saved, disk_fdisk_addr(dp), sizeof (Fdisk_t));

	/* set the starting sector comparitor to at invalid value */
	start = -1;

	WALK_PARTITIONS(i) {
		/* skip unused partitions */
		if (part_id(dp, i) == UNUSED)
			continue;

		if (part_preserved(dp, i) || part_stuck(dp, i)) {
			/*
			 * we've encountered an unmovable partition; make
			 * sure the current configuration doesn't have
			 * overlapping fixed partitions, and set the starting
			 * sector comparitor to the end of that partition
			 */
			if (part_startsect(dp, i) < start) {
				(void) memcpy(disk_fdisk_addr(dp),
						&saved, sizeof (Fdisk_t));
				return (D_NOFIT);
			}

			start = part_startsect(dp, i) + part_size(dp, i);
		} else {
			/*
			 * we've encountered a movable partition, so figure
			 * out where the starting sector should be and reset
			 * the start (only Solaris partitions are forced to
			 * be physical cylinder aligned)
			 */

			/*
			 * if the current starting sector doesn't have a valid
			 * value, give it the default value of one physical
			 * cylinder
			 */
			if (start < 0) {
				start = disk_geom_firstcyl(dp) *
						disk_geom_onecyl(dp);
			}

			if (part_id(dp, i) == SUNIXOS)
				start = blocks_to_blocks(dp, start);

			part_start_set(dp, i, start);
			start += part_size(dp, i);
		}

		/*
		 * if the starting offset sector has been pushed off the end
		 * of the disk, then apparently there isn't enough room to
		 * hold everything
		 */
		if (start > disk_geom_tsect(dp)) {
			(void) memcpy(disk_fdisk_addr(dp), &saved,
					sizeof (Fdisk_t));
			return (D_NOSPACE);
		}
	}

	return (D_OK);
}

/*
 * get_solaris_part()
 *	Retrieve the partition index number for the Solaris (SUNIXOS)
 *	partition in the current, committed, or existing fdisk.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	label	- specify which fdisk configuration should be
 *		  searched (CFG_CURRENT, CFG_COMMIT, or CFG_EXIST)
 * Return:
 *	0	 - no Solaris partition found, or 'label' invalid
 *	1<=#<=4  - partition index for Solaris partition (1 - 4)
 * Status:
 *	public
 */
int
get_solaris_part(Disk_t *dp, Label_t label)
{
	Fdisk_t	*fp;
	int	p;

	if (dp == NULL || disk_no_fdisk_exists(dp))
		return (0);

	switch (label) {
	    case CFG_CURRENT:
		fp = disk_fdisk_addr(dp);
		break;

	    case CFG_COMMIT:
		fp = disk_cfdisk_addr(dp);
		break;

	    case CFG_EXIST:
		fp = disk_ofdisk_addr(dp);
		break;

	    default:
		return (0);
	}

	WALK_PARTITIONS_REAL(p) {
		if (fp->part[p].id == SUNIXOS)
			return (p + 1);
	}

	return (0);
}

/*
 * max_size_part_hole()
 *	Determine what the maximum size in 512 byte blocks for a specific
 *	partition given current free space. fdisk partitions are assumed to
 *	be in physically ascending order, and UNUSED partitions may be
 *	interspersed.
 *
 *	Available space includes:
 *	(1) space currently assigned to the partitions
 *	(2) space in subsequent UNUSED partitions
 *	(3) contiguous subsequent space unassigned to
 *	    any other partition
 *	(4) UNUSED and unassigned space following any
 *	    subsequent unstuck and unpreserved partitions
 *	    up to the first stuck or preserved partition
 *	    or the end of the disk (which ever comes first)
 *
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	part	- partition index number
 * Return:
 *	0	- 'dp' is NULL, disk state is not 'okay', disk
 *		  is not 'selected, 'part' is invalid, or there
 *		  are no space free
 *	# > 0	- partition size in sectors
 * Status:
 *	public
 */
int
max_size_part_hole(Disk_t *dp, int part)
{
	int	freecyl;
	int	size;
	int	use;
	int	p;

	if (dp == NULL ||
			disk_not_okay(dp) ||
			disk_not_selected(dp) ||
			invalid_fdisk_part(part))
		return (0);

	/* initialize with the specified partition */
	size = _fdisk_segment_size(dp, part);
	use = 0;

	/*
	 * look for holes farther up the drive on the other side
	 * of used, unstuck, unpreserved partitions
	 */
	for (p = part + 1; valid_fdisk_part(p); p++) {
		if (part_stuck(dp, p) ||
				part_preserved(dp, p))
			break;

		if (part_id(dp, p) == UNUSED) {
			if (use == 1) {
				size += _fdisk_segment_size(dp, p);
				use = 0;
			}
		} else
			use = 1;
	}

	/*
	 * check to make sure you account for space which may be at
	 * the end of the drive beyond partition 4. This is only
	 * a problem if partition 4 is in use, the partition being sized
	 * is not partition 4, and there were no intervening "stuck"
	 * partitions
	 */
	if (invalid_fdisk_part(p) &&
			part != FD_NUMPART &&
			part_id(dp, FD_NUMPART) != UNUSED) {
		freecyl = disk_geom_lcyl(dp) -
			blocks_to_cyls(dp, part_startsect(dp, FD_NUMPART) +
				part_size(dp, FD_NUMPART));

		if (freecyl > 0)
			size += cyls_to_blocks(dp, freecyl);
	}

	/*
	 * make adjustments for drives that had an initial
	 * sector < one_cyl (DOS created fdisk label) so that
	 * there is consistency between fdisk_space_avail() and
	 * this routine under this condition
	 */
	WALK_PARTITIONS(p) {
		if (part_id(dp, p) != UNUSED && part_size(dp, p) > 0)
			break;
	}

	if (valid_fdisk_part(p) && part_startsect(dp, p) <
			(one_cyl(dp) * disk_geom_firstcyl(dp))) {
		size -= (disk_geom_firstcyl(dp) * one_cyl(dp));

		if (size < 0)
			size = 0;
	}

	return (size);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _reset_fdisk()
 *	Reset the current fdisk structure to a NULL state:
 *
 *	ALGORITHM:
 *	(1) clear all fdisk state flags
 *	(2) for each partition:
 *		- clear all state flags
 *		- set 'id' to UNUSED
 *		- set 'active' to NOTACTIVE
 *		- set all geometry fields to 0, except onecyl
 *		  and nsect
 *	(3) clear the S-disk geometry pointer
 *
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 * Return:
 *	D_OK	  - reset successful
 *	D_NODISK  - 'dp' is NULL
 * Status:
 *	semi-private (internal library use only)
 */
int
_reset_fdisk(Disk_t *dp)
{
	int	p;

	if (dp == NULL)
		return (D_NODISK);

	fdisk_state_clear(dp);
	WALK_PARTITIONS(p) {
		part_state_clear(dp, p);
		part_active_set(dp, p, NOTACTIVE);
		part_id_set(dp, p, UNUSED);
		part_geom_dsect(dp, p) = 0;
		part_geom_tsect(dp, p) = 0;
		part_geom_rsect(dp, p) = 0;
		part_geom_firstcyl(dp, p) = 0;
		part_geom_lcyl(dp, p) = 0;
		part_geom_dcyl(dp, p) = 0;
		part_geom_tcyl(dp, p) = 0;
		/* nsect, onecyl, and hbacyl are not touched */
	}

	return (D_OK);
}

/*
 * _set_part_size()
 *	Set the size of a specific fdisk partition.
 *
 *	ALGORITHM:
 *	(1) validate disk and parameters
 *	(2) determine the size (in sectors)
 *	(3) if the size is '0', set the starting cylinder
 *	    to '0' also, otherwise, round it up to the
 *	    nearest cylinder boundary
 *      (4) if the current size is '0' but the new size is
 *	    not, then adjust the starting cylinder to the
 *	    first legal data cylinder
 *      (5) if the partition ID is SUNIXOS, adjust the first
 *	    and last cylinder values. Clear the S-disk geometry
 *	    pointer
 *
 *	WARNING:	all partitions must be adjusted before this
 *			routine is called or incorrect partition
 *			sizes could result
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	part	- partition index number (1 - 4)
 *	newsize	- size of partition in 512 byte blocks - will be
 *		  cylinder boundary rounded:
 *
 *		  GEOM_REST	- max allowable size for partition
 *		  # >= 0	- explicit size; must be no larger
 *				  than the hole where the partition
 *				  resides (max_size_part_hole())
 * Return:
 *	D_OK		- partition size set successfully
 *	D_NODISK  	- 'dp' is NULL
 *	D_BADARG  	- 'part' or 'size' is an invalid value
 *	D_NOSPACE 	- there is no space available
 *	D_PRESERVED	- 'part' is preserved and cannot be modified
 * Status:
 *	semi-private (internal library use only)
 */
int
_set_part_size(Disk_t *dp, int part, int newsize)
{
	int	blks;

	if (dp == NULL)
		return (D_NODISK);

	if (invalid_fdisk_part(part))
		return (D_BADARG);

	if (part_preserved(dp, part) && disk_initialized(dp))
		return (D_PRESERVED);

	if (newsize == part_size(dp, part))
		return (D_OK);

	if (newsize == GEOM_REST) {
		if ((blks = max_size_part_hole(dp, part)) == 0)
			return (D_NOSPACE);
	} else {
		/* if the size is already set, we're done */
		if (newsize == part_size(dp, part))
			return (D_OK);

		if (newsize < 0)
			return (D_BADARG);

		/* convert newsize into sectors rounded up to cyl boundary */
		blks = blocks_to_blocks(dp, newsize);
		if (blks > usable_disk_blks(dp))
			return (D_NOSPACE);
	}

	if (blks == 0) {
		part_start_set(dp, part, 0);
		part_stuck_off(dp, part);
	} else {
		/*
		 * set the starting cylinder for '0' sized partitions with
		 * starting cyl of '0' to the first legal starting cyl
		 */
		if (part_size(dp, part) == 0 &&
					part_startsect(dp, part) == 0)
			part_start_set(dp, part,
				disk_geom_firstcyl(dp) * one_cyl(dp));
	}

	part_geom_tcyl(dp, part) = blocks_to_cyls(dp, blks);
	part_geom_tsect(dp, part) = blks;
	/*
	 * make adjustments to the geometry parameters depending on
	 * the type of partition
	 */
	if (part_id(dp, part) == SUNIXOS) {
		part_geom_firstcyl(dp, part) = 1;

		/* allow for alternate cylinders (if necessary) */
		part_geom_lcyl(dp, part) =
				part_geom_tcyl(dp, part) - NUMALTCYL;

		/* clear the S-disk geometry pointer */
		sdisk_geom_clear(dp);
	} else {
		part_geom_firstcyl(dp, part) = 0;
		part_geom_lcyl(dp, part) = part_geom_tcyl(dp, part);
	}

	part_geom_dcyl(dp, part) = part_geom_lcyl(dp, part) -
			part_geom_firstcyl(dp, part);
	part_geom_dsect(dp, part) =
			cyls_to_blocks(dp, part_geom_dcyl(dp, part));
	part_geom_lsect(dp, part) =
			cyls_to_blocks(dp, part_geom_lcyl(dp, part));

	/*
	 * check and correct possible invalid cylinder and size values
	 */
	if (part_geom_dsect(dp, part) < 0)
		part_geom_dsect(dp, part) = 0;

	if (part_geom_dcyl(dp, part) < 0)
		part_geom_dcyl(dp, part) = 0;

	if (part_geom_lcyl(dp, part) < 0)
		part_geom_lcyl(dp, part) = 0;

	if (part_geom_firstcyl(dp, part) > part_geom_tcyl(dp, part))
		part_geom_firstcyl(dp, part) = 0;

	return (D_OK);
}

/*
 * _set_part_start()
 * 	Set the starting sector for a specific partition. The start is
 *	specified in 512 byte blocks. Cannot modify perserved partitions.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	part	- partition index number (1 - 4)
 *	start	- relative offset sector for partition in sectors
 *		  (must be < the sector equivalent to the lcyl)
 * Return:
 *	D_OK		- start sector set successfully
 *	D_NODISK	- 'dp is NULL
 *	D_BADARG	- invalid partition index or offset sector argument
 *	D_PRESERVED	- partition is preserved and cannot be modified
 * Status:
 *	semi-private (internal library use only)
 */
int
_set_part_start(Disk_t *dp, int part, int start)
{
	if (dp == NULL)
		return (D_NODISK);

	if (invalid_fdisk_part(part) ||
			blocks_to_cyls(dp, start) > disk_geom_lcyl(dp))
		return (D_BADARG);

	if (part_preserved(dp, part) && disk_initialized(dp))
		return (D_PRESERVED);

	part_start_set(dp, part, start);
	return (D_OK);
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */

/*
 * _find_part_id()
 *	Determine if the current fdisk configuration has a partition
 *	with a specific partition id.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 * 	0	- no fdisk partition has the specified id
 *	1	- at least one fdisk partition has the specified id
 * Status:
 *	private
 */
static int
_find_part_id(Disk_t *dp, int id)
{
	int	p;

	if (dp == NULL)
		return (0);

	WALK_PARTITIONS(p) {
		if (part_id(dp, p) == id)
			return (1);
	}

	return (0);
}

/*
 * _fdisk_segment_size()
 *	Determine the potential size of a given partition. This
 *	includes the blocks currently assigned to the partition
 *	itself, as well as any UNUSED (or unassigned) space
 *	immediately contiguous to it.
 *
 *	ALGORITHM:
 *	(1) Search backwards from 'part' for the end of a preceding
 *	    partition which is not UNUSED (or for the beginning of
 *	    the drive).
 *	(2) Round that start value up to the nearest cylinder boundary
 *	(3) search forwards from 'part' for the start of a subsequent
 *	    partition which is not UNUSED (or for the end of the drive)
 *	(4) Round the end value down to the nearest cylinder boundary
 *	(5) Make adjustment for fdisk labels which have their first
 *	    partitions beginning before cylinder 1 so that there is
 *	    consistency with fdisk_space_avail()
 *
 * Parameters:
 *	dp	- non-null disk structure pointer
 *	part	- partition index
 * Return:
 *	# >= 0	- size in blocks
 * Status:
 *	private
 */
static int
_fdisk_segment_size(Disk_t *dp, int part)
{
	int	p, start, end, size;

	/* find the start of the hole */
	for (p = part - 1; p > 0; p--)
		if (part_id(dp, p) != UNUSED)
			break;

	if (p > 0)
		start = blocks_to_cyls(dp, part_startsect(dp, p) +
				part_size(dp, p));
	else
		start = disk_geom_firstcyl(dp);

	/* find the end of the hole */
	for (p = part + 1; valid_fdisk_part(p) &&
			part_id(dp, p) == UNUSED; p++);

	if (valid_fdisk_part(p))
		end = part_startcyl(dp, p);
	else
		end = disk_geom_lcyl(dp);

	if (end > start)
		size = (end - start) * one_cyl(dp);
	else
		size = 0;

	return (size);
}
