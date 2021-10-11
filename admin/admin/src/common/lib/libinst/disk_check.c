#ifndef lint
#pragma ident "@(#)disk_check.c 1.84 95/12/15 SMI"
#endif	/* lint */
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
 * This module contains disk configuration validation routines
 */
#include "disk_lib.h"

#include <sys/mntent.h>

/* Public Function Prototypes */

int		validate_disks(void);
int		validate_disk(Disk_t *);
int		validate_sdisk(Disk_t *);
int		validate_fdisk(Disk_t *);
int		check_disks(void);
int		check_disk(Disk_t *);
int		check_sdisk(Disk_t *);
int		check_fdisk(Disk_t *);
Errmsg_t	*get_error_list(void);
void		free_error_list(void);

/* Library Function Prototypes */

/* Local Function Prototypes */

static int		_dup_slice_mnts(Disk_t *);
static int		_check_slice_overlap(Disk_t *);
static int		_size_slice_zero(Disk_t *);
static int		_extends_off_sdisk(Disk_t *);
static int		_extends_off_fdisk(Disk_t *);
static int		_disk_crosscheck(void);
static int		_check_part_overlap(Disk_t *);
static int		_check_part_order(Disk_t *);
static void		_free_error_ent(Errmsg_t *);
static int		_i386_part_bios(Disk_t *);
static int		_i386_slice_bios(Disk_t *);
static void		_add_error_msg(int, char *, ...);
static Errmsg_t 	*_worst_error_msg(void);
static void		_set_sdisk_geom(Disk_t *);
static int		_check_sdisk_legal(Disk_t *);
static int		_check_sdisk(Disk_t *);
static int		_check_fdisk(Disk_t *);
static int		_check_unused_space(Disk_t *);

/* Globals and Externals */

char		err_text[256];	/* OBSOLETE */

/* Local Statics and Constants */

static Errmsg_t	*_error_list = NULL;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * check_disks()
 *	Check the consistency of the disk layout for all selected disks in an
 *	"okay" state in the disk list. This includes a per-disk fdisk and
 *	sdisk configuration check, as well as a cross disk consistency check.
 * Parameters:
 *	none
 * Return:
 *	# >= 0		- number of errors/warnings
 * Status:
 *	public
 */
int
check_disks(void)
{
	Disk_t	*dp;
	int	count = 0;

	free_error_list();
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp) && disk_selected(dp))
			count += _check_fdisk(dp) + _check_sdisk(dp);
	}

	count += _disk_crosscheck();
	return (count);
}

/*
 * validate_disks() - OBSOLETE (use check_disks())
 *	Check the consistency of the disk layout for all selected disks
 *	in an "okay" state in the disk list. This includes a per-disk
 *	fdisk and sdisk configuration check, as well as a cross
 *	disk consistency check.
 * Parameters:
 *	none
 * Return:
 *	D_OK 	- all disks are "okay"
 *	#<0	- not okay (ERROR)
 *	#>0	- not okay (WARNING)
 * Status:
 *	public
 */
int
validate_disks(void)
{
	int		count = 0;
	Disk_t		*dp;
	Errmsg_t	*emp;

	free_error_list();
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp) && disk_selected(dp))
			count += _check_fdisk(dp) + _check_sdisk(dp);
	}

	/* cross-disk checks */
	count += _disk_crosscheck();
	if ((emp = _worst_error_msg()) != NULL) {
		(void) strcpy(err_text, emp->msg);
		return (emp->code);
	}

	return (D_OK);
}

/*
 * check_disk()
 *	Check the layout consistency of a disk. This includes fdisk and
 *	sdisk checks.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 * Return:
 *	D_NODISK	- 'dp' argument was NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	# >= 0		- number of errors/warnings
 * Status:
 *	public
 */
int
check_disk(Disk_t *dp)
{
	int	count = 0;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	free_error_list();
	count += _check_fdisk(dp) + _check_sdisk(dp);
	return (count);
}

/*
 * validate_disk() - OBSOLETE (use check_disk())
 *	Check the layout consistency of a disk. This includes fdisk and
 *	sdisk checks.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay)
 * Return:
 *	D_OK		- no errors or warnings encountered
 *	D_NODISK	- 'dp' argument was NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	# < 0		- error status
 *	# > 0		- warning status
 * Status:
 *	public
 */
int
validate_disk(Disk_t *dp)
{
	int		count = 0;
	Errmsg_t	*emp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	free_error_list();
	count += _check_fdisk(dp) + _check_sdisk(dp);

	if ((emp = _worst_error_msg()) != NULL) {
		(void) strcpy(err_text, emp->msg);
		return (emp->code);
	}

	return (D_OK);
}

/*
 * check_fdisk()
 *	Validate that the fdisk configuration in the CURRENT state is sane.
 *	The sdisk pointer is established if the fdisk is legal and there is
 *	a valid Solaris partition.
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 *		    (state: selected, okay)
 * Return:
 *	# >= 0	- number of errors/warning encountered
 *	# < 0	- error with call
 *		D_NODISK	- 'dp' argument was NULL
 *		D_BADDISK	- disk state is not applic to this operation
 *		D_NOTSELECT	- disk not selected
 * Status:
 *	public
 */
int
check_fdisk(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	return (_check_fdisk(dp));
}

/*
 * validate_fdisk() - OBSOLETE (use check_fdisk())
 *	Validate that the fdisk configuration in the CURRENT state is sane.
 *	The sdisk pointer is established if the fdisk is legal and there is
 *	a valid Solaris partition.
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 *		    (state: selected, okay)
 * Return:
 *	D_OK		- no fdisk errors or warnings
 *	D_NODISK	- 'dp' argument was NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	D_NOTSELECT	- disk not selected
 *	# < 0		- error
 *	# > 0		- warning
 * Status:
 *	public
 */
int
validate_fdisk(Disk_t *dp)
{
	int		count = 0;
	Errmsg_t	*emp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	count += _check_fdisk(dp);

	if ((emp = _worst_error_msg()) != NULL) {
		(void) strcpy(err_text, emp->msg);
		return (emp->code);
	}

	return (D_OK);
}

/*
 * check_sdisk()
 *	Check the sdisk consistency of a disk looking for error or warning
 *	conditions. Only look at slices that actually have mount points defined.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: okay, selected)
 * Return:
 *	# >= 0	- number of errors/warning encountered
 *	# < 0	- error with call
 *		D_NODISK	- 'dp' argument was NULL
 *		D_BADDISK	- disk state is not applic to this operation
 *		D_NOTSELECT	- disk not selected
 * Status:
 *	public
 */
int
check_sdisk(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	return (_check_sdisk(dp));
}

/*
 * validate_sdisk() - OBSOLETE (use check_sdisk())
 *	Check the sdisk consistency of a disk looking for error or warning
 *	conditions. Only look at slices that actually have mount points defined.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state:  okay, selected)
 * Return:
 *	D_OK		- no errors or warnings
 *	D_NODISK	- 'dp' argument was NULL
 *	D_BADDISK	- disk state is not applicable to this operation
 *	D_NOTSELECT	- disk not selected
 *	# < 0		- error
 *	# > 0		- warning
 * Status:
 *	public
 */
int
validate_sdisk(Disk_t *dp)
{
	int		count = 0;
	Errmsg_t	*emp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	if (disk_not_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	count += _check_sdisk(dp);

	if ((emp = _worst_error_msg()) != NULL) {
		(void) strcpy(err_text, emp->msg);
		return (emp->code);
	}

	return (D_OK);
}

/*
 * free_error_list()
 *	Free space used by the error list.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
void
free_error_list(void)
{
	_free_error_ent(_error_list);
	_error_list = (Errmsg_t *)NULL;
}

/*
 * get_error_list()
 *	Retrieve a pointer to the head of the current error list.
 * Parameters:
 *	none
 * Return:
 *	pointer to head of error/warning message list (NULL if
 *	list is empty)
 * Status:
 *	public
 */
Errmsg_t *
get_error_list(void)
{
	return (_error_list);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _check_sdisk()
 *	Check the sdisk configuration for error and warning conditions.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings encountered
 * Status:
 *	private
 */
static int
_check_sdisk(Disk_t *dp)
{
	int	count = 0;

	if (sdisk_geom_not_null(dp)) {
		count = _check_sdisk_legal(dp) +
			_size_slice_zero(dp) +
			_dup_slice_mnts(dp) +
			_check_slice_overlap(dp) +
			_extends_off_sdisk(dp) +
			_i386_slice_bios(dp) +
			_check_unused_space(dp);
	}

	return (count);
}

/*
 * _dup_slice_mnts()
 * 	Check for duplicate mount points on all slices on all selected
 *	disks.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *		  (state: selected)
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_dup_slice_mnts(Disk_t *dp)
{
	int	count = 0;
	int	i;
	int	j;
	Disk_t	*p;

	WALK_SLICES(i) {
		if (slice_mntpnt_isnt_fs(dp, i) || slice_ignored(dp, i))
			continue;

		for (j = i + 1; j < numparts; j++) {
			if (slice_ignored(dp, j))
				continue;

			if (strcmp(slice_mntpnt(dp, i),
					slice_mntpnt(dp, j)) == 0) {
				_add_error_msg(D_DUPMNT,
					MSG3_SLICE_DUPLICATE,
					i, j, disk_name(dp));
				count++;
			}

			WALK_DISK_LIST(p) {
				if (p == dp || disk_not_selected(p))
					continue;

				WALK_SLICES(j) {
					if (slice_ignored(dp, j) ||
							slice_locked(dp, j))
						continue;

					if (strcmp(slice_mntpnt(dp, i),
						    slice_mntpnt(p, j)) == 0) {
						_add_error_msg(D_DUPMNT,
						    MSG4_SLICE_DUPLICATE_DIFF,
						    i, disk_name(dp),
						    j, disk_name(p));
						count++;
					}
				}
			}
		}
	}

	return (count);
}

/*
 * _check_slice_overlap()
 * 	Check for illegally overlapping slices
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_slice_overlap(Disk_t *dp)
{
	int	count = 0;
	int	i;
	int	e;
	int	*np;
	int	cnt;

	WALK_SLICES(i) {
		if (slice_is_overlap(dp, i) ||
				slice_size(dp, i) == 0)
			continue;

		cnt = slice_overlaps(dp, i, slice_start(dp, i),
				slice_size(dp, i), &np);

		/* only report overlaps which are farther down the drive */
		if (cnt != 0) {
			for (e = 0; e < cnt; e++) {
				if (np[e] > i) {
					_add_error_msg(D_OVER,
						MSG3_SLICE_OVERLAP,
						i, np[e], disk_name(dp));
					count++;
				}
			}
		}
	}

	return (count);
}

/*
 * _size_slice_zero()
 * 	Check that a mounted slice has a size >= at least 1 cylinder.
 * Parameters:
 *	dp	- disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_size_slice_zero(Disk_t *dp)
{
	int	count = 0;
	int	i;

	WALK_SLICES(i) {
		if (slice_mntpnt_is_fs(dp, i) &&
				slice_size(dp, i) < one_cyl(dp)) {
			_add_error_msg(D_ZERO,
				MSG1_SLICE_SIZE_TOOSMALL,
				make_slice_name(disk_name(dp), i));
			count++;
		}
	}

	return (count);
}

/*
 * _extends_off_sdisk()
 * 	Check that all slices are within the cylinder bounds of the disk. This
 *	includes unnamed slices, as well as slices of size '0'.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_extends_off_sdisk(Disk_t *dp)
{
	int	count = 0;
	int	s;
	int	ncyl;

	WALK_SLICES(s) {
		ncyl = slice_start(dp, s) +
			blocks_to_cyls(dp, slice_size(dp, s));

		if (ncyl > sdisk_geom_lcyl(dp)) {
			_add_error_msg(D_OFF,
				MSG1_SLICE_BEYOND_END,
				make_slice_name(disk_name(dp), s));
			count++;
		}
	}

	return (count);
}

/*
 * _extends_off_fdisk()
 *	Check that all fdisk partitions have a legal geometry (i.e.
 *	don't run off the end of the disk)
 * Parameters:
 *	dp	- non-NULL disk structure pointer with loaded fdisk
 *		  partition table
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_extends_off_fdisk(Disk_t *dp)
{
	int	count = 0;
	int	p;
	int	nsect;

	WALK_PARTITIONS(p) {
		if (part_id(dp, p) == UNUSED)
			continue;

		nsect = part_startsect(dp, p) + part_size(dp, p);
		if (nsect > disk_geom_lsect(dp)) {
			_add_error_msg(D_OFF,
				MSG2_PART_BEYOND_END,
				p, disk_name(dp));
			count++;
		}
	}

	return (count);
}

/*
 * _disk_crosscheck()
 *	Run validation checks across drives.
 * Parameters:
 *	void
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_disk_crosscheck(void)
{
	int		count = 0;
	Defmnt_t	ent;
	Disk_t		*dp;
	char		*dname;

	/*
	 * if "/" is a SELECTED file system, make sure it is defined, resides
	 * on the default boot disk, and has a valid sdisk geometry
	 */
	if (get_dfltmnt_ent(&ent, ROOT) == D_OK &&
				ent.status == DFLT_SELECT) {
		/* determine the default boot disk name */
		if ((dname = spec_dflt_bootdisk()) == NULL) {
			_add_error_msg(D_BOOTCONFIG,
				MSG1_DISK_BOOT_NO_ROOT,
				"*");
			count++;
		} else if ((dp = find_disk(dname)) == NULL ||
				disk_not_selected(dp) ||
				find_mnt_pnt(dp, NULL, "/", NULL,
					CFG_CURRENT) == 0 ||
				sdisk_not_legal(dp) ||
				sdisk_geom_null(dp)) {
			_add_error_msg(D_BOOTCONFIG,
				MSG1_DISK_BOOT_NO_ROOT,
				dname);
			count++;
		}
	}

	return (count);
}

/*
 * _i386_slice_bios()
 *	On i386 systems, check to see that slices 0, 8, and 9 all end before
 *	physical drive cylinder 1023 (due to BIOS limitations in the boot code).
 * Parameters:
 *	dp	- valid disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_i386_slice_bios(Disk_t *dp)
{
	int		count = 0;
	int		size = 0;
	Mntpnt_t	i;
	int		p;

	if (dp == NULL)
		return (0);

	if (strneq(get_default_inst(), "i386") ||
			(p = get_solaris_part(dp, CFG_CURRENT)) == 0)
		return (0);

	/* check slice 8 (bootslice) */
	if (BOOT_SLICE < numparts) {
		size = part_startsect(dp, p) +
			(slice_start(dp, BOOT_SLICE) * sdisk_geom_onecyl(dp)) +
			slice_size(dp, BOOT_SLICE);
		/* round up to the next hba cylinder boundary */
		size = (size + disk_geom_hbacyl(dp) - 1) / disk_geom_hbacyl(dp); 
		if (size > 1023) {
			_add_error_msg(D_OUTOFREACH,
				MSG0_SLICE_BOOT_BEYOND_BIOS);
			count++;
		}
	}

	/* check slice 9 (altslice) */
	if (ALT_SLICE < numparts) {
		size = part_startsect(dp, p) +
			(slice_start(dp, ALT_SLICE) * sdisk_geom_onecyl(dp)) +
			slice_size(dp, ALT_SLICE);
		/* round up to the next hba cylinder boundary */
		size = (size + disk_geom_hbacyl(dp) - 1) / disk_geom_hbacyl(dp); 
		if (size > 1023) {
			_add_error_msg(D_OUTOFREACH,
				MSG0_SLICE_ALTSECT_BEYOND_BIOS);
			count++;
		}
	}

	/* check the "/" slice (if necessary) */
	if (find_mnt_pnt(dp, NULL, ROOT, &i, CFG_CURRENT) != 0) {
		size = part_startsect(dp, p) +
			(slice_start(dp, i.slice) * sdisk_geom_onecyl(dp)) +
			slice_size(dp, i.slice);
		/* round up to the next hba cylinder boundary */
		size = (size + disk_geom_hbacyl(dp) - 1) / disk_geom_hbacyl(dp); 
		if (size > 1023) {
			_add_error_msg(D_OUTOFREACH,
				MSG0_SLICE_ROOT_BEYOND_BIOS);
			count++;
		}
	}

	return (count);
}

/*
 * _check_part_overlap()
 *	Check for overlapping F-disk partitions.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_part_overlap(Disk_t *dp)
{
	int	count = 0;
	int	i;
	int	*np;

	WALK_PARTITIONS(i) {
		if (part_overlaps(dp, i, part_geom_rsect(dp, i),
				part_size(dp, i), &np) != 0) {
			_add_error_msg(D_OVER,
				MSG3_PART_OVERLAP,
				i, np[0], disk_name(dp));
			count++;
		}
	}

	return (count);
}

/*
 * _check_part_order()
 *	Check partition ordering. Partitions should occur in the same
 *	physical order on the disk as they do ordinally. This should
 *	not be a problem with S494 FCS, since the fdisk partition table
 *	is not automatically sorted in physical order.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_part_order(Disk_t *dp)
{
	int	count = 0;
	int	last = 0;
	int	i;

	/* check for partition ordering */
	WALK_PARTITIONS(i) {
		if (part_id(dp, i) == UNUSED)
			continue;

		if (part_startsect(dp, i) < last) {
			_add_error_msg(D_BADORDER,
				MSG3_PART_ORDER_INVALID,
				last, i, disk_name(dp));
			count++;
		}

		last += blocks_to_cyls(dp, part_size(dp, i));
	}

	return (count);
}

/*
 * _free_error_ent()
 *	Recursive function to deallocate memory for all entries in the error
 *	message linked list.
 * Parameters:
 *	emp	- pointer to next entry to free
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_free_error_ent(Errmsg_t *emp)
{
	if (emp == NULL)
		return;

	_free_error_ent(emp->next);

	if (emp->msg != NULL)
		free(emp->msg);

	free(emp);
}

/*
 * _add_error_msg()
 *	Add a warning or error message to the end of the error linked list.
 * Parameters:
 *	code	- error code
 *	string	- format string for printf()
 *	...	- optional string arguments
 * Return:
 *	none
 * Status
 *	private
 */
static void
_add_error_msg(int code, char *string, ...)
{
	static char	_error_buf[256];
	va_list		ap;
	Errmsg_t	*emp, **tmp;

	_error_buf[0] = '\0';

	if (string != NULL) {
		/* assemble the message */
		va_start(ap, string);
		(void) vsprintf(_error_buf, string, ap);
		va_end(ap);
	}

	emp = (Errmsg_t *)xcalloc(sizeof (Errmsg_t));
	if (emp == NULL)
		return;

	emp->code = code;
	emp->msg = xstrdup(_error_buf);
	emp->next = NULL;

	/* add the entry to the end of the linked list */
	for (tmp = &_error_list; *tmp != NULL; tmp = &(*tmp)->next);
	*tmp = emp;
}

/*
 * _worst_error_msg() -	TRANSITION ROUTINE:
 *	Return a pointer to the "worst" error message entry in the error
 * 	list. This is used for compatibility with S494 behavior until the
 *	applications can handle the new message linked list format.
 * Parameters:
 *	none
 * Return:
 *	NULL		- no error messages
 *	Errmsg_t *	- pointer to either the first error message,
 *			  or the first warning message
 * Status:
 *	private
 */
static Errmsg_t *
_worst_error_msg(void)
{
	Errmsg_t	*emp;

	/* return first error if there is one */
	for (emp = _error_list; emp != NULL; emp = emp->next) {
		if (emp->code < 0)
			return (emp);
	}

	/* return either the first warning, or a NULL if the list is empty */
	return (_error_list);
}

/*
 * _i386_part_bios()
 *	On i386 systems, check to see that the Solaris partition begins
 *	before the BIOS 1023 cylinder.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_i386_part_bios(Disk_t *dp)
{
	int	count = 0;
	int	size;
	int	p;

	if (dp == NULL)
		return (0);

	if (strneq(get_default_inst(), "i386") ||
			(p = get_solaris_part(dp, CFG_CURRENT)) == 0)
		return (0);

	/* round up to HBA cylinder boundary */
	size = (part_startsect(dp, p) +
		disk_geom_hbacyl(dp) - 1) / disk_geom_hbacyl(dp);
	if (size > 1023) {
		_add_error_msg(D_OUTOFREACH,
			MSG0_SOLARIS_BEYOND_BIOS);
		count++;
	}

	return (count);
}

/*
 * _set_sdisk_geom()
 *	Reset the sdisk_geometry pointer if it is NULL.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_set_sdisk_geom(Disk_t *dp)
{
	int	reset = 0;
	int	p;
	int	i;

	if (sdisk_geom_not_null(dp) ||
			(p = get_solaris_part(dp, CFG_CURRENT)) == 0)
		return;

	/*
	 * check to see if the disk size of geometry has changed
	 * in a way that is incompatible with the current S-disk
	 * configuration. If so, reset the S-disk.
	 */
	sdisk_geom_set(dp, part_geom_addr(dp, p));
	if (slice_size(dp, ALL_SLICE) != accessible_sdisk_blks(dp))
		reset++;

	WALK_SLICES(i) {
		if (slice_start(dp, i) >= sdisk_geom_lcyl(dp) ||
				slice_size(dp, i) % one_cyl(dp))
			reset++;
	}

	if (reset) {
		(void) _reset_sdisk(dp);
	} else {
		if (part_startsect(dp, i) != orig_part_startsect(dp, i)) {
			WALK_SLICES(i)
				slice_preserve_off(dp, i);
		}
	}
}

/*
 * _check_sdisk_legal()
 *	Check to see if slice 2 is in an illegal file system configuration
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_sdisk_legal(Disk_t *dp)
{
	int	count = 0;

	if (sdisk_not_legal(dp)) {
		_add_error_msg(D_ILLEGAL,
			ILLEGAL_SLICE_CONFIG,
			disk_name(dp));
		count++;
	}

	return (count);
}

/*
 * _check_unused_space()
 *	Determine if the disk has more than 1 MB of unused disk
 *	space.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_unused_space(Disk_t *dp)
{
	int	count = 0;

	if (sdisk_space_avail(dp) >= mb_to_sectors(1)) {
		_add_error_msg(D_UNUSED,
			UNUSED_SLICE_SPACE,
			disk_name(dp));
		count++;
	}

	return (count);
}

/*
 * _check_fdisk()
 *	Check the fdisk configuration of the drive for various fdisk errors.
 *	Reset the sdisk geometry pointer if there is a Solaris partition.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	# >= 0	- number of errors/warnings which occurred
 * Status:
 *	private
 */
static int
_check_fdisk(Disk_t *dp)
{
	int	count = 0;

	if (dp == NULL)
		return (0);

	if (disk_fdisk_exists(dp)) {
		count = _check_part_overlap(dp) +
			_check_part_order(dp) +
			_extends_off_fdisk(dp) +
			_i386_part_bios(dp);
		_set_sdisk_geom(dp);
	}

	return (count);
}
