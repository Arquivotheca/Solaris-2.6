#ifndef lint
#pragma ident "@(#)store_disk.c 1.11 96/07/17 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_disk.c
 * Group:	libspmistore
 * Description:
 */

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/dkio.h>
#include "spmistore_lib.h"
#include "spmicommon_lib.h"

/* module globals  */
static Disk_t	*_Disks = NULL;

/* private prototypes */

static int	DiskobjCompareName(char *, char *);
static void	coreFdiskobjSave(Label_t, Disk_t *);
static void	coreSdiskobjSave(Label_t, Disk_t *);
static void	coreFdiskobjRestore(Label_t, Disk_t *);
static void	coreSdiskobjRestore(Label_t, Disk_t *);

/* Globals */

int		numparts = 8;

/* ---------------------- public functions ----------------------- */

/*
 * Function:	commit_disk_config
 * Description: Copy the current configurations for the fdisk and sdisk from
 *		the current working set to the committed set.
 * Scope:	public
 * Parameters: dp	- non-NULL disk structure pointer
 *			  (state:  okay, selected)
 * Return:	D_OK		commit completed successfully
 *		D_BADARG	invalid argument
 *		D_NOTSELECT	disk not selected
 */
int
commit_disk_config(Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return (D_BADARG);

	if (!disk_selected(dp))
		return (D_NOTSELECT);

	/*
	 * due to updates in the sdisk geometry pointer on fdisk systems, the
	 * sdisk commit MUST occur AFTER the fdisk commit
	 */
	coreFdiskobjSave(CFG_COMMIT, dp);
	coreSdiskobjSave(CFG_COMMIT, dp);

	return (D_OK);
}

/*
 * Function:	commit_disks_config
 * Description: Commit the disk state on all selected disks.
 * Scope:	public
 * Parameters:	none
 * Return:	D_OK	 - all disks committed successfully
 *		D_FAILED - disk commit failed for a disk
 */
int
commit_disks_config(void)
{
	Disk_t	*dp;

	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {
			if (commit_disk_config(dp) != D_OK)
				return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * Function:	deselect_disk
 * Description:	Mark a disk as "deselected". This will remove the disk from
 *		future disk search calls and commit routines.
 * Scope:	public
 * Parameters:	disk	- disk structure pointer (NULL if specifying drive by
 *			  'drive') - 'disk' has precedence over 'drive'
 *			  (state:  okay)
 *		drive	- name of drive - e.g. c0t0d0 (NULL if specifying drive
 *			  by 'disk')
 * Return:	D_OK	  - deselection completed successfully
 *		D_NODISK  - neither drive specifier argument is valid
 *		D_BADDISK - disk state is not applicable to this operation
 */
int
deselect_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	disk_select_off(dp);
	return (D_OK);
}

/*
 * Function:	find_disk
 * Description:	Search the disk list for a disk which has the same base name
 *		as 'dev' (e.g. c0t0d0 retrieved fro c0t0d0s2 or
 *		/dev/rdsk/c0t0d0s0). Return a pointer to the disk structure,
 *		or NULL of none exists.
 * Scope:	public
 * Parameters:	dev	- special file name for a device (e.g. c0t0d0s3 of
 *			  c0t0d0p0)
 * Return:	NULL	- no such device in path; 'dev' is a NULL pointer,
 *			  or 'dev' is a NULL string
 *		!NULL	- pointer to disk structure in array
 * Note:	This routine assumes that drive names are unique to devices.
 */
Disk_t *
find_disk(char *dev)
{
	char	name[16];
	Disk_t	*dp;

	if (dev == NULL || *dev == '\0')
		return (NULL);

	if (simplify_disk_name(name, dev) == 0) {
		WALK_DISK_LIST(dp) {
			if (streq(disk_name(dp), name))
				return (dp);
		}
	}

	return (NULL);
}

/*
 * Function:	first_disk
 * Description:	Return a pointer to the first disk in the disk list.
 * Scope:	public
 * Parameters:	none
 * Return:	NULL  - no disks defined on system
 *		Disk_t *  - pointer to head of (physical) disk chain
 */
Disk_t *
first_disk(void)
{
	return (_Disks);
}

/*
 * Function:	next_disk
 * Description:	Return a pointer to the next disk in the disk list.
 * Scope:	public
 * Parameters:	Disk_t * - pointer to current disk structure
 * Return:	NULL	 - 'dp' is NULL or dp->next is NULL
 *		Disk_t * - pointer to next disk in chain
 */
Disk_t *
next_disk(Disk_t *dp)
{
	if (dp == NULL)
		return (dp);

	return (dp->next);
}

/*
 * Function:	restore_disk
 * Description:	Restore both the fdisk and sdisk configurations from committed
 *		or original configurations (parameter specified) to the current
 *		state for a specific disk.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state:  okay, selected)
 *		state	- source of disk configuration (CFG_COMMIT or CFG_EXIST)
 * Return:	D_OK		restore completed successfully
 *		D_BADARG	invalid argument
 *		D_NOTSELECT 	disk not selected
 */
int
restore_disk(Disk_t *dp, Label_t state)
{
	/* validate parameters */
	if (dp == NULL)
		return (D_BADARG);

	if (state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	if (!disk_selected(dp))
		return (D_NOTSELECT);

	/* the sdisk restore MUST occur AFTER the fdisk restore */
	coreFdiskobjRestore(state, dp);
	coreSdiskobjRestore(state, dp);

	return (D_OK);
}

/*
 * Function:	select_disk
 * Description: Mark a specific disk as "selected". Selected disks are included
 *		in disk searches, default configuration routines, and can be
 *		committed and restored. The calling routine is responsible for
 *		ensuring that the selection of a disk will not result in
 *		duplicate filesystem ("/xxx") names in the mountpoint namespace.
 *
 *		NOTE:	If the S-disk was in an "illegal" state (e.g. it is an
 *			i386 system and there is a FS on slice 2) then the sdisk
 *			slices are reset to a legal configuration automatically,
 *			trashing all user data that was on the slice.
 * Scope:	public
 * Parameters:	disk	- disk structure pointer (NULL if specifying drive by
 *			  'drive') - 'disk' has precedence over 'drive'
 *			  (state:  okay)
 *		drive	- name of drive (e.g. c0t0d0)(NULL if specifying drive
 *			  by 'dp'
 * Return:	D_OK	   selection completed successfully
 *		D_NODISK   neither drive specifier argument is valid
 *		D_BADDISK  could not reset sdisk configuration
 */
int
select_disk(Disk_t *disk, char *drive)
{
	Disk_t	*dp;

	if (disk != NULL)
		dp = disk;
	else if ((dp = find_disk(drive)) == NULL)
		return (D_NODISK);

	if (disk_selected(dp))
		return (D_OK);

	if (SdiskobjIsIllegal(CFG_CURRENT, dp)) {
		if (_reset_sdisk(dp) != D_OK)
			return (D_BADDISK);
	}

	disk_select_on(dp);
	return (D_OK);
}

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	DiskobjDestroy
 * Description: Free the dynamic memory used by a disk object.
 * Scope:	internal
 * Parameters:	dp	[RO, *RO]
 *			Non-NULL pointer to a valid disk object.
 * Return:	none
 */
void
DiskobjDestroy(Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL)
		return;

	free(dp);
}

/*
 * Function:	DiskobjAddToList
 * Description: Add a disk object ot the current list of objects, placing the
 *		disks in order of their disk names ordered numerically by
 *		controller/target/disk respectively.
 * Scope:	internal
 * Parameters:	new	[RO, *RO]
 *			Non-NULL pointer to the name of the disk to be
 *			added to the primary disk object list
 * Return:	none
 */
void
DiskobjAddToList(Disk_t *new)
{
	Disk_t **	dpp;

	/* validate parameter */
	if (new == NULL)
		return;

	/*
	 * break out of the loop either because there are no more
	 * disks to compare, or because you've encountered the first
	 * disk with a name that follows the name of the disk
	 * being added
	 */
	WALK_LIST_INDIRECT(dpp, _Disks) {
		if (DiskobjCompareName(disk_name(new), disk_name(*dpp)) < 0)
			break;
	}

	new->next = *dpp;
	*dpp = new;
}

/*
 * Function:	_disk_is_scsi
 * Description:	Determine if a disk controller type is or is not SCSI. The
 *		controller type should have been set at disk initialization
 *		time. Controller types identified as SCSI as of S494 are:
 *			DKC_SCSI_CCS
 *			DKC_CDROM
 *			DKC_MD21
 * Scope:	internal
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	0	- disk is not a known SCSI type
 * 		1	- disk is a known SCSI type
 */
int
_disk_is_scsi(const Disk_t *dp)
{
	if (dp == NULL)
		return (0);

	switch (dp->ctype) {

	case  DKC_SCSI_CCS:
	case  DKC_CDROM:
	case  DKC_MD21:
		return (1);

	default:
		return (0);
	}
}

/*
 * Function:	DiskobjSave
 * Description:	Save the specified disk state.
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			State of disk to save. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	D_OK	   successful save
 *		D_BADARG   invalid argument
 */
int
DiskobjSave(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return (D_BADARG);

	coreFdiskobjSave(state, dp);
	coreSdiskobjSave(state, dp);
	return (D_OK);
}

/*
 * Function:	FdiskobjRestore
 * Description:	Restore the specified fdisk state. NULL out the sdisk
 *		geometry pointer for systems requiring an fdisk (it must be
 *		reestablished).
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			State of fdisk to restore. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	D_OK	   fdisk restored
 *		D_BADARG   invalid argument
 */
int
FdiskobjRestore(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return (D_BADARG);

	coreFdiskobjRestore(state, dp);

	/* break the sdisk geometry reference on fdisk systems */
	if (disk_fdisk_exists(dp))
		Sdiskobj_Geom(CFG_CURRENT, dp) = NULL;

	return (D_OK);
}

/*
 * Function:	SdiskobjRestore
 * Description:	Restore the specified sdisk configuration state, preserving the
 *		current geometry pointer. Intended for use without
 *		corresponding fdisk restore. The restore is only permitted if
 *		the critical parameters of the disk geometry (fdisk SUNIXOS
 *		partition) has not changed between the original and the current
 *		sdisk configurations.
 * Scope:	internal
 * Parameters:	state	[RO] (Label_t)
 *			State of sdisk to restore. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	D_OK	   sdisk restored
 *		D_BADARG   invalid argument
 *		D_BADDISK  disk state not applicable to this operation
 *		D_GEOMCHNG disk geometry change since orig config prohibits
 *			   restore
 *		D_ILLEGAL  failed because sdisk is in an 'illegal' state
 */
int
SdiskobjRestore(Label_t state, Disk_t *dp)
{
	int	status;

	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return (D_BADARG);

	/* make sure the geometry hasn't changed */
	if ((status = sdisk_geom_same(dp, state)) != D_OK)
		return (status);

	coreSdiskobjRestore(state, dp);
	return (D_OK);
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	coreFdiskobjSave
 * Description:	Store the current slice configuration into the committed
 *		configuration structure. This routine should be called before
 *		a corresponding _commit_sdisk_save() call so that the sdisk
 *		geometry pointer is correct on fdisk systems.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of fdisk to save. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
coreFdiskobjSave(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return;

	(void) memcpy((void *) Fdiskobj_Addr(state, dp),
		(void *) Fdiskobj_Addr(CFG_CURRENT, dp), sizeof (Fdisk_t));
}

/*
 * Function:	coreSdiskobjSave
 * Description:	Store the current sdisk configuration into the committed
 *		configuration structure. This routine should be called after
 *		a corresponding _commit_fdisk_save() call so that the sdisk
 *		geometry pointer is correct on fdisk systems.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of fdisk to save. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
coreSdiskobjSave(Label_t state, Disk_t *dp)
{
	int	p;

	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return;

	(void) memcpy((void *) Sdiskobj_Addr(state, dp),
			(void *) Sdiskobj_Addr(CFG_CURRENT, dp),
			sizeof (Sdisk_t));

	/* reset the sdisk geometry pointer on fdisk systems */
	if (disk_fdisk_exists(dp) && (p = get_solaris_part(dp, state)) > 0)
		Sdiskobj_Geom(state, dp) = Partobj_GeomAddr(state, dp, p);
}

/*
 * Function:	coreFdiskobjRestore
 * Description:	Copy the specified state of the fdisk configuration into the
 *		current configuration. This routine should be called before
 *		a corresponding coreSdiskobjRestore() call so that the sdisk
 *		geometry pointer is correct.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of fdisk to restore. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle.
 * Return:	none
 */
static void
coreFdiskobjRestore(Label_t state, Disk_t *dp)
{
	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return;

	(void) memcpy((void *) Fdiskobj_Addr(CFG_CURRENT, dp),
		(void *) Fdiskobj_Addr(state, dp), sizeof (Fdisk_t));
}

/*
 * Function:	coreSdiskobjRestore
 * Description: Restore the specified sdisk configuration from the original
 *		configuration structure. This routine should be called after
 *		a corresponding coreFdiskobjRestore() call so that the sdisk
 *		geometry pointer reference is correct on fdisk systems. If the
 *		restoration would result in the disk being put into an illegal
 *		state, the disk is automatically deselected.
 * Scope:	private
 * Parameters:	state	[RO] (Label_t)
 *			State of sdisk to restore. Valid values are:
 *			    CFG_COMMIT
 *			    CFG_EXIST
 * 		dp	[RO, *RO] (Disk_t *)
 *			Disk object handle for specified drive
 * Return:	none
 */
static void
coreSdiskobjRestore(Label_t state, Disk_t *dp)
{
	int	p;

	/* validate parameters */
	if (dp == NULL || (state != CFG_COMMIT && state != CFG_EXIST))
		return;

	(void) memcpy((void *) Sdiskobj_Addr(CFG_CURRENT, dp),
			(void *) Sdiskobj_Addr(state, dp), sizeof (Sdisk_t));

	if (SdiskobjIsIllegal(CFG_CURRENT, dp))
		(void) deselect_disk(dp, NULL);

	if ((p = get_solaris_part(dp, CFG_CURRENT)) > 0)
		Sdiskobj_Geom(CFG_CURRENT, dp) =
			Partobj_GeomAddr(CFG_CURRENT, dp, p);
}

/*
 * Function:	DiskobjCompareName
 * Description:	Compare the names of two disks (e.g. c0t3d0s2) and return
 *		a `0', if they are the same, `-1' if `name1' precedes `name2',
 *		and `1' if `name2' precedes `name1'. Sorting orders the names
 *		based on their controller, target (optional), and disk index.
 * Scope:	private
 * Parameters:	name1	[RO, *RO]
 *			Non-NULL pointer to cannonical disk name.
 *		name2	[RO, *RO]
 *			Non-NULL pointer to cannonical disk name.
 * Return:	0	- the two names are identical
 *		-1	- `name1' precedes `name2'
 *		1	- `name2' precedes `name1'
 */
static int
DiskobjCompareName(char *name1, char *name2)
{
	int	value1;
	int	value2;

	/* validate parameters */
	if (name1 == NULL || name2 == NULL)
		return (0);

	/* get the controller index */
	must_be(name1, 'c');
	must_be(name2, 'c');
	value1 = atoi(name1);
	value2 = atoi(name2);
	skip_digits(name1);
	skip_digits(name2);
	if (value1 < value2)
		return (-1);
	else if (value2 < value1)
		return (1);

	/* get the target index (if it exists) */
	value1 = value2 = -1;
	if (*name1 == 't') {
		value1 = atoi(++name1);
		skip_digits(name1);
	}
	if (*name2 == 't') {
		value2 = atoi(++name2);
		skip_digits(name2);
	}
	if (value1 < value2)
		return (-1);
	else if (value2 < value1)
		return (1);

	/* get the disk index */
	must_be(name1, 'd');
	must_be(name2, 'd');
	value1 = atoi(name1);
	value2 = atoi(name2);
	if (value1 < value2)
		return (-1);
	else if (value2 < value1)
		return (1);

	return (0);
}
