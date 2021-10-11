#ifndef lint
#pragma ident "@(#)store_check.c 1.8 96/06/22 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_check.c
 * Group:	libspmistore
 * Description: 
 */
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include "spmistore_lib.h"
#include "spmicommon_api.h"
#include "store_strings.h"

/* public prototypes */

int		check_disk(Disk_t *);
int		check_disks(void);
int		check_fdisk(Disk_t *);
int		check_sdisk(Disk_t *);
void		free_error_list(void);
Errmsg_t *	get_error_list(void);
int		validate_fdisk(Disk_t *);
Errmsg_t *	worst_error(void);

/* private prototypes */

static void	_add_error_msg(int, char *, ...);
static int	_check_bootability(void);
static int	_check_fdisk(Disk_t *);
static int	_check_sdisk(Disk_t *);
static int	_check_sdisk_legal(Disk_t *);
static int	_check_part_order(Disk_t *);
static int	_check_part_overlap(Disk_t *);
static int	_check_slice_overlap(Disk_t *);
static int	_check_unused_space(Disk_t *);
static int	_dup_slice_keys(Disk_t *);
static int	_extends_off_fdisk(Disk_t *);
static int	_extends_off_sdisk(Disk_t *);
static void	_free_error_ent(Errmsg_t *);
static int	_i386_part_bios(Disk_t *);
static int	_i386_slice_bios(Disk_t *);
static void	_set_sdisk_geom(Disk_t *);
static int	_size_slice_zero(Disk_t *);

/* globals */

char		err_text[256];	/* OBSOLETE */

/* globals and constants */

static Errmsg_t	*_error_list = NULL;

/*---------------------- public functions -----------------------*/

/*
 * Function:	check_disk
 * Description: Check the layout consistency of a disk. This includes fdisk and
 *		sdisk checks.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *		  	  (state:  okay)
 * Return:	D_NODISK	- 'dp' argument was NULL
 *		D_BADDISK	- disk state is not applicable to this operation
 *		# >= 0		- number of errors/warnings
 */
int
check_disk(Disk_t *dp)
{
	int	count = 0;

	if (dp == NULL)
		return (D_NODISK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	free_error_list();
	count += _check_fdisk(dp) + _check_sdisk(dp);
	return (count);
}

/*
 * Function:	check_disks
 * Description: Check the consistency of the disk layout for all selected disks.
 * Scope:	public
 * Parameters:	none
 * Return:	# >= 0		- number of errors/warnings
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

	count += _check_bootability();
	return (count);
}

/*
 * Function:	check_fdisk
 * Description: Validate that the fdisk configuration in the CURRENT state is
 *		sane. The sdisk pointer is established if the fdisk is legal
 *		and there is a valid Solaris partition.
 * Scope:	public
 * Parameters:	dp	  - non-NULL disk structure pointer
 *			    (state: selected, okay)
 * Return:	# >= 0	- number of errors/warning encountered
 *		# < 0	- error with call
 *			D_NODIS     - 'dp' argument was NULL
 *			D_BADDISK   - disk state is not applic to this operation
 *			D_NOTSELECT - disk not selected
 */
int
check_fdisk(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	if (!disk_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	return (_check_fdisk(dp));
}

/*
 * Function:	check_sdisk
 * Description: Check the sdisk consistency of a disk looking for error or
 *		warning conditions. Only look at slices that actually have
 *		mount points defined.
 * Scope:	public
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state: okay, selected)
 * Return:	# >= 0	- number of errors/warning encountered
 *		# < 0	- error with call
 *			D_NODISK     - 'dp' argument was NULL
 *			D_BADDISK    - disk state isn't applic to this operation
 *			D_NOTSELECT  - disk not selected
 */
int
check_sdisk(Disk_t *dp)
{
	if (dp == NULL)
		return (D_NODISK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	if (!disk_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	return (_check_sdisk(dp));
}

/*
 * Function:	free_error_list
 * Description: Free space used by the error list.
 * Scope:	public
 * Parameters:	none
 * Return:	none
 */
void
free_error_list(void)
{
	_free_error_ent(_error_list);
	_error_list = (Errmsg_t *)NULL;
}

/*
 * Function:	get_error_list
 * Description: Retrieve a pointer to the head of the current error list.
 * Scope:	public
 * Parameters:	none
 * Return:	pointer to head of error/warning message list (NULL if
 *		list is empty)
 */
Errmsg_t *
get_error_list(void)
{
	return (_error_list);
}

/*
 * Function:	validate_fdisk - OBSOLETE (use check_fdisk())
 * Description:	Validate that the fdisk configuration in the CURRENT state is
 *		sane. The sdisk pointer is established if the fdisk is legal and
 *		there is a valid Solaris partition.
 * Scope:	public
 * Parameters:	dp	  - non-NULL disk structure pointer
 *			    (state: selected, okay)
 * Return:	D_OK		- no fdisk errors or warnings
 *		D_NODISK	- 'dp' argument was NULL
 *		D_BADDISK	- disk state is not applicable to this operation
 *		D_NOTSELECT	- disk not selected
 *		# < 0		- error
 *		# > 0		- warning
 */
int
validate_fdisk(Disk_t *dp)
{
	int		count = 0;
	Errmsg_t	*emp;

	if (dp == NULL)
		return (D_NODISK);

	if (!disk_okay(dp))
		return (D_BADDISK);

	if (!disk_selected(dp) && disk_initialized(dp))
		return (D_NOTSELECT);

	free_error_list();
	count += _check_fdisk(dp);

	if ((emp = worst_error()) != NULL) {
		(void) strcpy(err_text, emp->msg);
		return (emp->code);
	}

	return (D_OK);
}

/*
 * Function:	worst_error
 * Description: Return a pointer to the "worst" error message entry in the error
 *	 	list. This is used for compatibility with S494 behavior until
 *		the applications can handle the new message linked list format.
 * Scope:	public
 * Parameters:	none
 * Return:	NULL		- no error messages
 *		Errmsg_t *	- pointer to either the first error message,
 *				  or the first warning message
 */
Errmsg_t *
worst_error(void)
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

/*---------------------- internal functions -----------------------*/

/*
 * Function:	_add_error_msg
 * Description: Add a warning or error message to the end of the error linked
 *		list.
 * Scope:	private
 * Parameters:	code	- error code
 *		string	- format string for printf()
 *		...	- optional string arguments
 * Return:	none
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
 * Function:	_check_bootability
 * Description: Check to see that the system will reboot using the
 *		system default booting sequence. This is not needed
 *		for AutoClients.
 * Scope:	private
 * Parameters:	none
 * Return:	# >= 0	- number of errors/warnings which occurred
 */
static int
_check_bootability(void)
{
	Mntpnt_t    info;
	char	    cdisk[32];
	int	    cdiskexp;
	int	    cdev;
	int	    cdevexp;
	int	    count = 0;
	int	    prom_changeable;
	int	    prom_authorized;
	Disk_t *    dp;
	int	    pid;

	/* AutoClients do not need a bootability test */
	if (get_machinetype() == MT_CCLIENT)
		return (0);

	if (BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, cdisk,
			BOOTOBJ_DISK_EXPLICIT, &cdiskexp,
			BOOTOBJ_DEVICE, &cdev,
			BOOTOBJ_DEVICE_EXPLICIT, &cdevexp,
			BOOTOBJ_PROM_UPDATEABLE, &prom_changeable,
			BOOTOBJ_PROM_UPDATE, &prom_authorized,
			NULL) != D_OK) {
		return (0);
	}

	/*
	 * check for "/" file system on the current boot disk as long as the
	 * current boot disk is defined
	 */
	if (DiskobjFindBoot(CFG_CURRENT, &dp) != D_OK || dp == NULL ||
			find_mnt_pnt(dp, NULL, ROOT, &info,
				CFG_CURRENT) == 0) {
		_add_error_msg(D_BOOTCONFIG,
			MSG1_BOOT_DISK_NO_ROOT,
			cdisk);
		count++;
	} else {
		/* validate devices */
		/*
		 * SPARC systems:
		 *	If the boot device was explicitly specified, check to
		 *	see that this is the slice that contains "/".
		 */
		if (IsIsa("sparc")) {
			if (info.slice != cdev) {
				_add_error_msg(D_BOOTCONFIG,
					MSG0_BOOT_DEVICE_NO_ROOT);
					count++;
			}
		}

		/*
		 * Intel systems:
		 *	Make sure, if the current boot disk and device are
		 *	defined, that the device contains a Solaris partition.
		 */
		if (IsIsa("i386")) {
			/* look for a Solaris partition */
			if ((pid = get_solaris_part(dp, CFG_CURRENT)) == 0) {
				_add_error_msg(D_BOOTCONFIG,
					MSG1_BOOT_DISK_NO_SOLARIS,
					disk_name(dp));
				count++;
			}

			if (BootobjIsExplicit(CFG_CURRENT,
				BOOTOBJ_DEVICE_EXPLICIT) &&
					cdev != -1 && pid != cdev) {
				_add_error_msg(D_BOOTCONFIG,
					MSG0_BOOT_DEVICE_NO_SOLARIS);
				count++;
			}
		}

		/*
		 * PPC systems:
		 *	Make sure, if the current boot disk and device are
		 *	defined, that the device contains a DOS partition.
		 *	Also make sure the disk contains a Solaris partition.
		 */
		if (IsIsa("ppc")) {
			/* look for any Solaris partition */
			if (get_solaris_part(dp, CFG_CURRENT) == 0) {
				_add_error_msg(D_BOOTCONFIG,
					MSG1_BOOT_DISK_NO_SOLARIS,
					disk_name(dp));
					count++;
			}

			/* find the DOS partition */
			WALK_PARTITIONS(pid) {
				if (part_id(dp, pid) == DOSOS12 ||
						part_id(dp, pid) == DOSOS16)
				break;
			}

			if (!valid_fdisk_part(pid)) {
				/* if there is no DOS partition, error */
				_add_error_msg(D_BOOTCONFIG,
					MSG1_BOOT_DISK_NO_DOS,
					disk_name(dp));
				count++;
			} else if (BootobjIsExplicit(CFG_CURRENT,
					     BOOTOBJ_DEVICE_EXPLICIT) &&
					cdev != -1 && pid != cdev) {
				/*
				 * if the boot device was explicitly spec'd,
				 * and the current is a valid partition index,
				 * but not the one specified, error
				 */
				_add_error_msg(D_BOOTCONFIG,
					MSG0_BOOT_DEVICE_NO_DOS);
				count++;
			}
		}
	}

	/*
	 * check if there has been any fundamental change in the prom
	 * configuration
	 */
	if (BootobjCompare(CFG_CURRENT, CFG_EXIST, 1) != 0) {
		if (prom_changeable && prom_authorized) {
			_add_error_msg(D_PROMRECONFIG,
				MSG0_BOOT_PROM_CHANGING);
			count++;
		} else {
			_add_error_msg(D_PROMMISCONFIG,
				MSG0_BOOT_PROM_CHANGE_REQUIRED);
			count++;
		}
	}

	return (count);
}

/*
 * Function:	_check_fdisk
 * Description:	Check the fdisk configuration of the drive for various fdisk
 *		errors. Reset the sdisk geometry pointer if there is a Solaris
 *		partition.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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

/*
 * Function:	_check_part_order
 * Description:	Check partition ordering. Partitions should occur in the same
 *		physical order on the disk as they do ordinally. This should
 *		not be a problem with S494 FCS, since the fdisk partition table
 *		is not automatically sorted in physical order.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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
 * Function:	_check_part_overlap
 * Description:	Check for overlapping F-disk partitions.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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
 * Function:	_check_sdisk
 * Description: Check the sdisk configuration for error and warning conditions.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings encountered
 */
static int
_check_sdisk(Disk_t *dp)
{
	int	count = 0;

	if (!sdisk_geom_null(dp)) {
		count = _check_sdisk_legal(dp) +
			_size_slice_zero(dp) +
			_dup_slice_keys(dp) +
			_check_slice_overlap(dp) +
			_extends_off_sdisk(dp) +
			_i386_slice_bios(dp) +
			_check_unused_space(dp);
	}

	return (count);
}

/*
 * Function:	_check_sdisk_legal
 * Description: Check to see if slice 2 is in an illegal file system
 *		configuration
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
 */
static int
_check_sdisk_legal(Disk_t *dp)
{
	int	count = 0;

	if (SdiskobjIsIllegal(CFG_CURRENT, dp)) {
		_add_error_msg(D_ILLEGAL,
			ILLEGAL_SLICE_CONFIG,
			disk_name(dp));
		count++;
	}

	return (count);
}

/*
 * Function:	_check_slice_overlap
 * Description: Check for illegally overlapping slices
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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

		cnt = slice_overlaps(dp, i, Sliceobj_Start(CFG_CURRENT,
		dp, i),
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
 * Function:	_check_unused_space
 * Description:
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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
 * Functin:	_dup_slice_keys
 * Description: Check for duplicate slice keys (use/instance pairs) amongst
 *		all viable slices.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 *			  (state: selected)
 * Return:	# >= 0	- number of errors/warnings which occurred
 */
static int
_dup_slice_keys(Disk_t *dp)
{
	int	count = 0;
	int	i;
	int	j;
	Disk_t	*p;

	WALK_SLICES(i) {
		/* keys on ignored slices are no considered */
		if (slice_ignored(dp, i))
			continue;

		/*
		 * check the key of the current slice against all other
		 * viable slice keys on this disk starting with
		 * the slice immediately following the current
		 * slice
		 */
		for (j = i + 1; j < numparts; j++) {
			/*
			 * don't compare with "invisible" slices
			 */
			if (slice_ignored(dp, j) || slice_locked(dp, j))
				continue;

			if (streq(slice_use(dp, i), slice_use(dp, j)) &&
						is_pathname(slice_use(dp, i)) &&
					slice_instance(dp, i) ==
						slice_instance(dp, j)) {
				_add_error_msg(D_DUPMNT,
					MSG3_SLICE_DUPLICATE,
					i, j, disk_name(dp));
				count++;
			}
		}

		/*
		 * check the key of the current slice against all
		 * other viable slices on all other disks
		 */
		WALK_DISK_LIST(p) {
			/*
			 * don't compare with the current disk, or other
			 * disks which aren't selected
			 */
			if (p == dp || !disk_selected(p))
				continue;

			WALK_SLICES(j) {
				/*
				 * don't compare with "invisible" slices
				 */
				if (slice_ignored(dp, j) ||
						slice_locked(dp, j))
					continue;

				if (streq(slice_use(dp, i),
						    slice_use(p, j)) &&
						is_pathname(slice_use(dp, i)) &&
						slice_instance(dp, i) ==
						    slice_instance(dp, j)) {
					_add_error_msg(D_DUPMNT,
						MSG4_SLICE_DUPLICATE_DIFF,
						i, disk_name(dp),
						j, disk_name(p));
					count++;
				}
			}
		}
	}

	return (count);
}

/*
 * Function:	_extends_off_fdisk
 * Description: Check that all fdisk partitions have a legal geometry (i.e.
 *		don't run off the end of the disk)
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer with loaded fdisk
 *			  partition table
 * Return:	# >= 0	- number of errors/warnings which occurred
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
 * Function:	_extends_off_sdisk
 * Description: Check that all slices are within the cylinder bounds of the
 *		disk. This includes unnamed slices, as well as slices of size
 *		'0'.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
 */
static int
_extends_off_sdisk(Disk_t *dp)
{
	int	count = 0;
	int	s;
	int	ncyl;

	WALK_SLICES(s) {
		ncyl = Sliceobj_Start(CFG_CURRENT, dp, s) +
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
 * Function:	_free_error_ent
 * Description:	Recursive function to deallocate memory for all entries in the
 *		error message linked list.
 * Scope:	private
 * Parameters:	emp	- pointer to next entry to free
 * Return:	none
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
 * Function:	_i386_part_bios
 * Description: On i386 systems, check to see that the Solaris partition begins
 *		before the BIOS 1023 cylinder.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
 */
static int
_i386_part_bios(Disk_t *dp)
{
	int	count = 0;
	int	size;
	int	p;

	if (dp == NULL)
		return (0);

	if (!IsIsa("i386") || (p = get_solaris_part(dp, CFG_CURRENT)) == 0)
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
 * Function:	_i386_slice_bios
 * Description: On i386 systems, check to see that slices 0, 8, and 9 all end
 *		before physical drive cylinder 1023 (due to BIOS limitations in
 *		the boot code).
 * Scope:	private
 * Parameters:	dp	- valid disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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

	if (!IsIsa("i386") || (p = get_solaris_part(dp, CFG_CURRENT)) == 0)
		return (0);

	/* check slice 8 (bootslice) */
	if (BOOT_SLICE < numparts) {
		size = part_startsect(dp, p) +
			(Sliceobj_Start(CFG_CURRENT, dp, BOOT_SLICE) *
				sdisk_geom_onecyl(dp)) +
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
 * Function:	_set_sdisk_geom
 * Description: Reset the sdisk_geometry pointer if it is NULL.
 * Scope:	private
 * Parameters:	dp	- non-NULL disk structure pointer
 * Return:	none
 */
static void
_set_sdisk_geom(Disk_t *dp)
{
	int	reset = 0;
	int	p;
	int	i;

	if (!sdisk_geom_null(dp) ||
			(p = get_solaris_part(dp, CFG_CURRENT)) == 0)
		return;

	/*
	 * check to see if the disk size of geometry has changed
	 * in a way that is incompatible with the current S-disk
	 * configuration. If so, reset the S-disk.
	 */
	Sdiskobj_Geom(CFG_CURRENT, dp) = Partobj_GeomAddr(CFG_CURRENT, dp, p);
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
		if (Partobj_Startsect(CFG_CURRENT, dp, i) !=
				Partobj_Startsect(CFG_EXIST, dp, i)) {
			WALK_SLICES(i) {
				SliceobjClearBit(CFG_CURRENT,
					dp, i, SLF_PRESERVED);
			}
		}
	}
}

/*
 * Function:	_size_slice_zero
 * Description: Check that a mounted slice has a size >= at least 1 cylinder.
 * Scope:	private
 * Parameters:	dp	- disk structure pointer
 * Return:	# >= 0	- number of errors/warnings which occurred
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

