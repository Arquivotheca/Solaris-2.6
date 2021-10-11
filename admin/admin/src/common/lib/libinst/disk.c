#ifndef lint
#pragma ident   "@(#)disk.c 1.88 95/03/17 SMI"
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
/*
 * This module contains routines which manipulate the disk structure or one
 * of its unit subcomponents as a complete entity
 */
#include "disk_lib.h"
#include "ibe_lib.h"

#include <signal.h>

/* Globals */

int		numparts = 8;

/* Public Function Prototypes */

int		commit_disk_config(Disk_t *);
int		commit_disks_config(void);
int		deselect_disk(Disk_t *, char *);
int		select_disk(Disk_t *, char *);
int		select_bootdisk(Disk_t *, char *);
int		restore_disk(Disk_t *, Label_t);
char *		spec_dflt_bootdisk(void);
int		duplicate_disk(Disk_t *, Disk_t *);

/* Library Function Prototypes */

int		_restore_sdisk_orig(Disk_t *);
int		_restore_fdisk_orig(Disk_t *);
int		_restore_sdisk_commit(Disk_t *);
int		_restore_fdisk_commit(Disk_t *);
Disk_t *	_init_bootdisk(void);
void 		_init_commit_orig(Disk_t *);

/* Local Function Prototypes */

static void	_copy_sdisk_config(Sdisk_t *, Sdisk_t *);
static void	_orig_sdisk_restore(Disk_t *);
static void	_orig_fdisk_restore(Disk_t *);
static void	_orig_sdisk_save(Disk_t *);
static void	_orig_fdisk_save(Disk_t *);
static void	_commit_sdisk_save(Disk_t *);
static void	_commit_fdisk_save(Disk_t *);
static void	_commit_fdisk_restore(Disk_t *);
static void	_commit_sdisk_restore(Disk_t *);
static int	_i386_is_mca(Disk_t *);
static char *	_i386_default(void);
static char *	_existing_root_default(void);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * select_bootdisk()
 *	Specify the disk to be marked as "boot disk". Only one disk can
 *	have the boot flag set at one time.
 *
 *	ALGORITHM:
 *	(1) Search for the disk specified by the caller, and return
 *	    in error if not found.
 *	(2) Check to see if the disk specified is already marked as
 *	    the boot disk. If not, unmark the current boot disk and
 *	    then set the boot flag for the user specified disk.
 *	(3) Move the new boot disk to the front of the disk list.
 *
 *	NOTE:	this routine should always be used to set or change the boot
 *		disk
 *
 *	NOTE:	the default disk configuration algorithms will only
 *		configure '/' and 'swap' on the selected drive which
 *		has the boot flag set. This routine will not do the
 *		selection for you.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive
 *		  by 'drive') - 'disk' has precedence over 'drive'
 *		  (state:  okay)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying
 *		  drive by 'disk')
 * Return:
 *	D_OK	    - bootdisk selected successfully
 *	D_NODISK    - neither argument was specified
 *	D_BADDISK   - disk state is not applicable to this operation
 *		      altered after initialization
 * Status:
 *	public
 */
int
select_bootdisk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;
	Disk_t	*obdp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_bootdrive(dp))
		return (D_OK);

	if (disk_not_bootdrive(dp)) {
		if ((obdp = find_bootdisk()) != NULL)
			disk_bootdrive_off(obdp);

		disk_bootdrive_on(dp);
	}

	if (disk_not_bootdrive(first_disk()))
		_sort_disks();

	return (D_OK);
}

/*
 * select_disk()
 *	Mark a specific disk as "selected". Selected disks are included
 *	in disk searches, default configuration routines, and can be
 *	committed and restored. The calling routine is responsible for
 *	ensuring that the selection of a disk will not result in
 *	duplicate filesystem ("/xxx") names in the mountpoint namespace.
 *
 *	WARNING:  If the S-disk was in an "illegal" state (e.g. it is an
 *		  i386 system and there is a FS on slice 2) then the S-disk
 *		  slices are reset to a legal configuration automatically,
 *		  trashing all user data that was on the slice.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive by
 *		  'drive') - 'disk' has precedence over 'drive'
 *		  (state:  okay)
 *	drive	- name of drive (e.g. c0t0d0)(NULL if specifying drive by
 *		  'dp'
 * Return:
 *	D_OK	  - selection completed successfully
 *	D_NODISK  - neither drive specifier argument is valid
 *	D_BADDISK - disk state not "okay" or S-disk geometry pointer
 *		    is NULL and the S-disk is in an illegal state
 * Status:
 *	public
 */
int
select_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_selected(dp))
		return (D_OK);

	if (sdisk_not_legal(dp)) {
		if (_reset_sdisk(dp) != D_OK)
			return (D_BADDISK);
	}

	disk_select_on(dp);
	return (D_OK);
}

/*
 * deselect_disk()
 *	Mark a disk as "deselected". This will remove the disk from future
 *	disk search calls and commit routines.
 * Parameters:
 *	disk	- disk structure pointer (NULL if specifying drive by
 *		  'drive') - 'disk' has precedence over 'drive'
 *		  (state:  okay)
 *	drive	- name of drive - e.g. c0t0d0 (NULL if specifying drive
 *		  by 'disk')
 * Return:
 *	D_OK	  - deselection completed successfully
 *	D_NODISK  - neither drive specifier argument is valid
 *	D_BADDISK - disk state is not applicable to this operation
 * Status:
 *	public
 */
int
deselect_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	disk_select_off(dp);
	return (D_OK);
}

/*
 * commit_disk_config()
 * 	Copy the current configurations for the fdisk and sdisk from the
 *	current working set to the committed set.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 * Return:
 *	D_OK		- commit completed successfully
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	D_NOTSELECT	- disk not selected
 * Status:
 *	public
 */
int
commit_disk_config(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp))
		return (D_NOTSELECT);

	/*
	 * due to updates in the sdisk geometry pointer on fdisk systems, the
	 * sdisk commit MUST occur AFTER the fdisk commit
	 */
	_commit_fdisk_save(dp);
	_commit_sdisk_save(dp);
	return (D_OK);
}

/*
 * restore_disk()
 *	Restore both the fdisk and sdisk configurations from committed or
 *	original configurations (parameter specified) to the current state for
 *	a specific disk.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 *	label	- source of disk configuration (CFG_COMMIT or CFG_EXIST)
 * Return:
 *	D_OK		- restore completed successfully
 *	D_NODISK	- 'dp' is NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	D_BADARG	- 'label' was neither CFG_EXIST or CFG_COMMIT
 *	D_NOTSELECT 	- disk not selected
 * Status:
 *	public
 */
int
restore_disk(Disk_t *dp, Label_t label)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp))
		return (D_NOTSELECT);

	switch (label) {
	    case CFG_EXIST:
		/*
		 * due to updates in the sdisk geometry pointer on fdisk
		 * systems, the sdisk commit MUST occur AFTER the fdisk commit
		 */
		_orig_fdisk_restore(dp);
		_orig_sdisk_restore(dp);
		break;

	    case CFG_COMMIT:
		/*
		 * due to updates in the sdisk geometry pointer on fdisk
		 * systems, the sdisk commit MUST occur AFTER the fdisk commit
		 */
		_commit_fdisk_restore(dp);
		_commit_sdisk_restore(dp);
		break;

	    default:
		return (D_BADARG);
	}

	return (D_OK);
}

/*
 * spec_dflt_bootdisk()
 *	Determine which disk (if any) is the default boot disk. On i386, the
 *	boot disk must be on controller '0'.
 *
 * 	ALGORITHM:
 *	(1) i386 systems:
 *		If there is no controller 0 on the system, then return "". Otherwise,
 *		find the first "c0" disk drive and use it to determine the "c0" bus
 *		type (mcis?). If the bus is mcis, then the default drive is c0t6d0. If
 *		not, then look for either c0t0d0 or c0d0 drives, and if either exists
 *		(they are mutually exclusive) then that is the default drive. If they
 *		don't, then look at the drive you originally got to determine the
 *		controller type (assume disks of a feather flock together) and if it
 *		is IDE (DKC_DIRECT) then the default drive is c0d0, otherwise the
 *		default drive is c0t0d0.
 *	(2) SPARC and Power PC systems:
 *		Consult the EEPROM for the default boot device.
 *
 *	If the default boot device cannot be determined at this point, or the
 *	disk stat is bad, select the first disk with an existing "/" file system
 *	which is in an "okay" state. If none exist, pick the first disk which is
 *	in an "okay" state.
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to local string containing the name of the disk most likely
 *		  to be the default boot drive for this system ( "" if a default cannot
 *		  be determined)
 * Status:
 *	public
 */
char *
spec_dflt_bootdisk(void)
{
	static char	drive[16];
	Disk_t		*dp;
	char		*cp;

	drive[0] = '\0';

	if (first_disk() == NULL)
		return (drive);

	if (streq(get_default_inst(), "i386"))
		(void) strcpy(drive, _i386_default());
	else {
		if ((cp = _eeprom_default()) == NULL)
			(void) strcpy(drive, _existing_root_default());
		else
			(void) strcpy(drive, cp);
	}

	/*
	 * if the default disk state is not "okay" or the disk is "", just pick
	 * the first "okay" disk
	 */
	if (drive[0] == '\0' || find_disk(drive) == NULL ||
			disk_not_okay(find_disk(drive))) {
		WALK_DISK_LIST(dp) {
			if (disk_okay(dp)) {
				(void) strcpy(drive, disk_name(dp));
				break;
			}
		}
	}

	return (drive);
}

/*
 * duplicate_disk()
 *	Copy one disk structure to another disk structure.
 * Parameters:
 *	to 	- non-NULL disk structure pointer referencing
 *		  the destination disk structure
 *	from 	- non-NULL disk structure pointer referencing
 *		  the source disk structure
 * Return:
 *	0	- successful
 *	1	- duplication failed
 * Status:
 *	public
 */
int
duplicate_disk(Disk_t *to, Disk_t *from)
{
	int	s;

	if (to == NULL || from == NULL)
		return (1);

	(void) memcpy(to, from, sizeof (Disk_t));

	WALK_SLICES(s) {
		if (orig_slice_mntopts(from, s) != NULL) {
			orig_slice_mntopts_set(to, s,
				xstrdup(orig_slice_mntopts(from, s)));
		}

		if (comm_slice_mntopts(from, s) != NULL) {
			comm_slice_mntopts_set(to, s,
				xstrdup(comm_slice_mntopts(from, s)));
		}

		if (slice_mntopts(from, s) != NULL) {
			slice_mntopts_set(to, s,
				xstrdup(slice_mntopts(from, s)));
		}
	}

	return (0);
}

/*
 * commit_disks_config()
 *	Commit the disk state on all selected disks.
 * Parameters:
 *	none
 * Return:
 *	D_OK	 - all disks committed successfully
 *	D_FAILED - disk commit failed for a disk
 * Status:
 *	public
 */
int
commit_disks_config(void)
{
	Disk_t	*disk;

	WALK_DISK_LIST(disk) {
		if (disk_not_selected(disk))
			continue;

		if (commit_disk_config(disk) != D_OK) {
			write_notice(ERRMSG,
				MSG1_INTERNAL_DISK_COMMIT,
				disk_name(disk));
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _init_commit_orig()
 * 	Copy the current sdisk and fdisk configurations to their committed and
 *	original archive structures. This routine should only be called at disk
 *	initialization time. Disk states are ignored. The fdisk structures must
 *	always be saved before the corresponding sdisk structure in order to
 *	maintain correctness in the sdisk geometry pointer on fdisk supporting
 *	systems.
 * Parameters:
 *	dp	- valid disk structure pointer
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_init_commit_orig(Disk_t *dp)
{
	if (dp != NULL) {
		/* always save the fdisk before the sdisk */
		_orig_fdisk_save(dp);
		_orig_sdisk_save(dp);
		_commit_fdisk_save(dp);
		_commit_sdisk_save(dp);
	}
}

/*
 * _restore_sdisk_orig()
 *	Restore the sdisk configuration from original, preserving the current
 *	geometry pointer. Intended for use without corresponding fdisk restore.
 *	The restore is only permitted if the critical parameters of the disk
 *	geometry (fdisk SUNIXOS partition) has not changed between the original
 *	and the current sdisk configurations.
 * Parameters:
 *	dp	   - non-NULL disk structure pointer
 * Return:
 *	D_OK	   - sdisk restored
 *	D_BADDISK  - disk state not applicable to this operation
 *	D_NODISK   - 'dp' is NULL
 *	D_GEOMCHNG - disk geometry change since orig config prohibits restore
 *	D_ILLEGAL  - failed because sdisk is in an 'illegal' state
 * Status:
 *	semi-private (internal library use only)
 */
int
_restore_sdisk_orig(Disk_t *dp)
{
	int	status;

	if (dp == NULL)
		return (D_NODISK);

	if ((status = sdisk_geom_same(dp, CFG_EXIST)) != D_OK)
		return (status);

	_orig_sdisk_restore(dp);
	return (D_OK);
}

/*
 * _restore_sdisk_commit()
 *	Restore the sdisk configuration from committed state, preserving the current
 *	geometry pointer. Intended for use without corresponding fdisk restore. The
 *	restore is only permitted if the critical parameters of the disk geometry
 *	(fdisk SUNIXOS partition) has not changed between the original and the current
 *	sdisk configurations.
 * Parameters:
 *	dp	   - non-NULL disk structure pointer
 * Return:
 *	D_OK	   - sdisk restored
 *	D_NODISK   - 'dp' is NULL
 *	D_BADDISK  - disk state not applicable to this operation
 *	D_GEOMCHNG - disk geom change since committed config prohibits restore
 *	D_ILLEGAL  - failed because sdisk is in an 'illegal' state
 * Status:
 *	semi-private (internal library use only)
 */
int
_restore_sdisk_commit(Disk_t *dp)
{
	int	status;

	if (dp == NULL)
		return (D_NODISK);

	if ((status = sdisk_geom_same(dp, CFG_COMMIT)) != D_OK)
		return (status);

	_commit_sdisk_restore(dp);
	return (D_OK);
}

/*
 * _restore_fdisk_orig()
 *	Restore the original fdisk configuration. NULL out the sdisk geometry pointer
 *	for systems requiring an F-disk (it must be reestablished).
 * Parameters:
 *	dp	   - non-NULL disk structure pointer
 * Return:
 *	D_OK	   - fdisk restored
 *	D_NODISK   - 'dp' is NULL
 *	D_BADDISK  - disk state not applicable to this operation
 * Status:
 *	semi-private (internal library use only)
 */
int
_restore_fdisk_orig(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	_orig_fdisk_restore(dp);

	/* break the sdisk geometry reference on fdisk systems */
	if (disk_fdisk_exists(dp))
		sdisk_geom_clear(dp);

	return (D_OK);
}

/*
 * _restore_fdisk_commit()
 *	Restore the committed fdisk configuration. NULL out the sdisk geometry pointer
 *	for systems requiring an fdisk (it must be reestablished).
 * Parameters:
 *	dp         - non-NULL disk structure pointer
 * Return:
 *	D_OK	   - fdisk restored
 *	D_NODISK   - 'dp' is NULL
 *	D_BADDISK  - disk state not applicable to this operation
 * Status:
 *	semi-private (internal library use only)
 */
int
_restore_fdisk_commit(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	_commit_fdisk_restore(dp);

	/* break the sdisk geometry reference on fdisk systems */
	if (disk_fdisk_exists(dp))
		sdisk_geom_clear(dp);

	return (D_OK);
}

/*
 * _init_bootdisk()
 *	Scan the disk list and try to determine which disk is the most reasonable
 *	boot disk candidate and set the boot flag on the corresponding disk structure.
 *	If the default boot disk is not in the disk list, then just mark first disk on
 *	the list.
 *
 *	NOTE:	This routine should only be called once, at the time the disk list is
 *		being initialized. Alterations made by this routine to the current
 *		configuration may need to be propogated to the commit and orig
 *		structures by the calling function.
 * Parameters:
 *	none
 * Return:
 *	Disk_t * - boot drive was identified and marked
 *	NULL	 - there are no disks in the list
 * Status:
 *	semi-private (internal library use only)
 */
Disk_t *
_init_bootdisk(void)
{
	Disk_t	*dp;
	char	*drive;

	if (first_disk() == NULL)
		return (NULL);

	drive = spec_dflt_bootdisk();
	if ((dp = find_disk(drive)) == NULL)
		dp = first_disk();

	(void) select_bootdisk(dp, NULL);
	return (dp);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _commit_sdisk_save()
 *	Store the current sdisk configuration into the committed configuration
 *	structure. This routine should be called after a corresponding
 *	_commit_fdisk_save() call so that the sdisk geometry pointer is correct
 *	on fdisk systems.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_commit_sdisk_save(Disk_t *dp)
{
	int	p;

	if (dp == NULL)
		return;

	_copy_sdisk_config(&dp->c_sdisk, &dp->sdisk);

	/* reset the S-disk geometry pointer on fdisk systems */
	if ((p = get_solaris_part(dp, CFG_COMMIT)) > 0)
		comm_sdisk_geom_set(dp, comm_part_geom_addr(dp, p));
}

/*
 * _commit_sdisk_restore()
 *	Restore the current sdisk configuration from the committed configuration
 *	structure. This routine should be called after a corresponding
 *	_commit_fdisk_restore() call so that the sdisk geometry pointer
 *	reference is correct on fdisk systems. If the restoration would result
 *	in the drive being put into an illegal state, the drive is
 *	automatically deselected.
 * Parameters:
 *	dp 	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_commit_sdisk_restore(Disk_t *dp)
{
	int	i;
	int	p;

	if (dp == NULL)
		return;

	WALK_SLICES(i)
		free(slice_mntopts(dp, i));

	(void) memcpy((void *) &dp->sdisk, (void *) &dp->c_sdisk,
			sizeof (Sdisk_t));

	WALK_SLICES(i)
		slice_mntopts(dp, i) = xstrdup(comm_slice_mntopts(dp, i));

	if (sdisk_not_legal(dp))
		(void) deselect_disk(dp, NULL);

	/* reset the S-disk geometry pointer on fdisk systems */
	if ((p = get_solaris_part(dp, CFG_CURRENT)) > 0)
		sdisk_geom_set(dp, part_geom_addr(dp, p));
}

/*
 * _orig_sdisk_save()
 *	Store the current slice configuration into the original configuration
 *	structure. This routine should be called after a corresponding
 *	_orig_fdisk_save() call so that the S-disk geometry pointer is correct on
 *	fdisk systems.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_orig_sdisk_save(Disk_t *dp)
{
	int	p;

	if (dp == NULL)
		return;

	_copy_sdisk_config(&dp->o_sdisk, &dp->sdisk);

	/* reset the S-disk geometry pointer on fdisk systems */
	if ((p = get_solaris_part(dp, CFG_EXIST)) > 0)
		orig_sdisk_geom_set(dp, orig_part_geom_addr(dp, p));
}

/*
 * _orig_sdisk_restore()
 *	Restore the current sdisk configuration from the original configuration
 *	structure. This routine should be called after a corresponding
 *	_orig_fdisk_restore() call so that the sdisk geometry pointer reference
 *	is correct on fdisk systems. If the restoration would result in the disk
 *	being put into an illegal state, the disk is automatically deselected.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_orig_sdisk_restore(Disk_t *dp)
{
	int	i;
	int	p;

	if (dp == NULL)
		return;

	WALK_SLICES(i)
		free(slice_mntopts(dp, i));

	(void) memcpy((void *) &dp->sdisk, (void *) &dp->o_sdisk,
			sizeof (Sdisk_t));

	WALK_SLICES(i)
		slice_mntopts(dp, i) = xstrdup(orig_slice_mntopts(dp, i));

	if (sdisk_not_legal(dp))
		(void) deselect_disk(dp, NULL);

	if ((p = get_solaris_part(dp, CFG_CURRENT)) > 0)
		sdisk_geom_set(dp, part_geom_addr(dp, p));
}

/*
 * _copy_sdisk_config()
 *	Copy data from one sdisk structure to another. This is a low level
 *	utility and does not do state checking, or disk consistency checking.
 * Parameters:
 *	to	- non-NULL pointer to destination sdisk structure
 *	from	- non-NULL pointer to source sdisk structure
 * Return:
 *	none
 * Status:
 *	private
 * Note:
 *	malloc, free
 */
static void
_copy_sdisk_config(Sdisk_t *to, Sdisk_t *from)
{
	int	i;

	if (to == NULL || from == NULL)
		return;

	WALK_SLICES(i)
		free(to->slice[i].mntopts);

	(void) memcpy((void *)to, (void *)from, sizeof (Sdisk_t));

	WALK_SLICES(i)
		to->slice[i].mntopts = xstrdup(from->slice[i].mntopts);
}

/*
 * _commit_fdisk_save()
 *	Store the current slice configuration into the committed configuration
 *	structure. This routine should be called before a corresponding
 *	_commit_sdisk_save() call so that the sdisk geometry pointer is correct
 *	on fdisk systems.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_commit_fdisk_save(Disk_t *dp)
{
	if (dp != NULL)
		(void) memcpy((void *) &dp->c_fdisk, (void *) &dp->fdisk,
			sizeof (Fdisk_t));
}

/*
 * _orig_fdisk_save()
 *	Store the current slice configuration into the original configuration
 *	structure. This routine should be called before a corresponding
 *	_orig_sdisk_save() call so that the sdisk geometry pointer is correct on
 *	fdisk systems.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_orig_fdisk_save(Disk_t *dp)
{
	if (dp != NULL)
		(void) memcpy((void *) &dp->o_fdisk, (void *) &dp->fdisk,
			sizeof (Fdisk_t));
}

/*
 * _commit_fdisk_restore()
 *	Restore the fdisk partition and state information from the original
 *	to the current configuration. This routine should be called before a
 *	corresponding _commit_sdisk_restore() call so that the sdisk geometry
 *	pointer is correct.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_commit_fdisk_restore(Disk_t *dp)
{
	if (dp != NULL)
		(void) memcpy((void *) &dp->fdisk, (void *) &dp->c_fdisk,
			sizeof (Fdisk_t));
}

/*
 * _orig_fdisk_restore()
 *	Copy the original state of the fdisk configuration into the current
 *	configuration. This routine should be called before a corresponding
 *	_orig_sdisk_restore() call so that the sdisk geometry pointer is correct.
 * Parameters:
 *	dp	- valid disk structure pointer for specified drive
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_orig_fdisk_restore(Disk_t *dp)
{
	if (dp != NULL)
		(void) memcpy((void *)&dp->fdisk, (void *)&dp->o_fdisk,
			sizeof (Fdisk_t));
}

/*
 * _i386_is_mca()
 *	Test to see if the hardware bus adapter for a specific disk is an MCA
 *	(meaning the boot disk must be c0t6*).
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 * Return:
 *	1	- 'dp' does reside on an mcis bus
 *	0	- 'dp' does not reside on an mcis bus or disk data was loaded
 *		  from a data file
 * Status:
 *	private
 */
static int
_i386_is_mca(Disk_t *dp)
{
	int	len;
	char	devices[MAXNAMELEN];
	char	dev[MAXNAMELEN];

	if (dp == NULL ||
			disk_not_okay(dp) ||
			strneq(get_default_inst(), "i386") ||
			_diskfile_load)
		return (0);

	(void) sprintf(dev, "/dev/rdsk/%sp0", disk_name(dp));
	if ((len = readlink(dev, devices, MAXNAMELEN)) < 0)
		return (0);

	devices[len] = '\0';
	if (strstr(devices, "/mcis@"))
		return (1);

	return (0);
}

/*
 * _i386_default()
 *	i386 system boot drive logic. Find the first disk in the disk chain. In most
 *	cases this will be on the default BIOS controller (0), otherwise, it will be
 *	in alphanumeric order of what appears in the /dev/rdsk directory.
 *
 *	NOTE:	We are assuming that the lowest numbered controller on the system is
 *		a single digit
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to local static string containing default drive name
 * Status:
 *	private
 */
static char *
_i386_default(void)
{
	static char	drive[16];
	Disk_t *	dp;

	drive[0] = '\0';

	if (strneq(get_default_inst(), "i386"))
		return (drive);

	if ((dp = first_disk()) != NULL) {
		(void) strncpy(drive, disk_name(dp), 2);
		drive[2] = '\0';

		if (_i386_is_mca(dp)) {
			(void) strcat(drive, "t6d0");
		} else if (disk_ctype(dp) == DKC_DIRECT) {
			(void) strcat(drive, "d0");
		} else {
			(void) strcat(drive, "t0d0");
		}
	}

	return (drive);
}

/*
 * _existing_root_default()
 *	Determine which disk would be the default based on where "/" was previously
 *	installed. If there are multiple instances of "/", then use the first.
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to local char array containing drive
 *		  name (or "" if none can be determined)
 * Status:
 *	private
 */
static char *
_existing_root_default(void)
{
	static char	drive[16];
	Disk_t		*rdp = NULL;
	Disk_t		*dp;
	int		bootcnt = 0;

	drive[0] = '\0';

	/* count all instances of "/" being defined */
	WALK_DISK_LIST(dp) {
		if (find_mnt_pnt(dp, NULL, ROOT, NULL, CFG_EXIST) == 1) {
			if (rdp == NULL)
				rdp = dp;

			bootcnt++;
			break;
		}
	}

	if (bootcnt > 0)
		(void) strcpy(drive, disk_name(rdp));

	return (drive);
}
