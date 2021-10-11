#ifndef lint
#pragma ident "@(#)pf_fdisk.c 2.8 96/05/14"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc. All Rights Reserved.
 */
#include <string.h>
#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmisvc_api.h"
#include "spmiapp_api.h"
#include "profile.h"
#include "pf_strings.h"

/* Public Function Prototypes */

int		configure_fdisk(Fdisk *);
int		pf_convert_id(int);

/* Local Function Prototypes */

static int	pf_part_delete(Disk_t *, Fdisk *);
static int	pf_part_maxfree(Disk_t *, Fdisk *);
static int	pf_part_all(Disk_t *, Fdisk *);
static int	pf_part_explicit(Disk_t *, Fdisk *);
static int	_pf_part_delete(Disk_t *, Fdisk *);
static int	_pf_part_maxfree(Disk_t *, Fdisk *, int);
static int	_pf_part_all(Disk_t *, Fdisk *, int);
static int	_pf_part_explicit(Disk_t *, Fdisk *, int);
static char *	_pf_part_type_name(int);
static int	_pf_partid_match(int, int);

/*
 * Local structure definitions
 */
struct part_name {
	char	*name;
	int	id;
};

static struct part_name	  partnames[] = {
		{ "Solaris",		130	},
		{ "dosprimary",		 -1	},	/* DOSPRIMARY */
		{ "",			  0	}
		};

/* ******************************************************************** */
/* 			PUBLIC FUNCTIONS				*/
/* ******************************************************************** */
/*
 * configure_fdisk()
 *	Process the fdisk keywords entries specified in the profile.
 *	Each record is processed in the order it occurred in the
 *	profile. If no fdisk records are in the profile on an Intel
 *	install, the default record is:
 *
 *		fdisk all any solaris maxfree
 *
 *	If a record with a specific disk fails, pfinstall will exit.
 *	If a record applies to all disks, failed calls will be reported
 *	as warnings, but the processing will continue.
 *
 *	NOTE:	Disks which don't have a Solaris partition will not
 *		be processed by the backend code
 * Parameters:
 *	fdisk	  - pointer to fdisk command structure
 * Return:
 *	D_OK	  - fdisk configured successfully
 *	D_FAILED  - fdisk configuration failed
 * Status:
 *	public
 */
int
configure_fdisk(Fdisk *fdisk)
{
	Fdisk		*fdp;
	Disk_t		*dp;
	Fdisk		fddflt = \
		{ "all", 0, 0, SUNIXOS, FD_SIZE_MAXFREE, NULL };
	int		p;
	int		count;

	/*
	 * only process fdisk records if this is an Intel system
	 */
	if (disk_no_fdisk_req(first_disk()))
		return (D_OK);

	/*
	 * mark all existing defined partitions as preserved
	 */
	WALK_DISK_LIST(dp) {
		if (disk_not_selected(dp))
			continue;

		WALK_PARTITIONS(p) {
			if (part_id(dp, p) == UNUSED)
				continue;

			if (set_part_preserve(dp, p, PRES_YES) != D_OK) {
				write_notice(ERRMSG,
					MSG2_PART_PRESERVE_FAILED,
					_pf_part_type_name(part_id(dp, p)),
					disk_name(dp));
				return (D_FAILED);
			}
		}
	}

	/*
	 * default logic is "fdisk all any solaris maxfree"
	 */
	if (fdisk == NULL) {
		if (pf_part_maxfree(NULL, &fddflt) != D_OK)
			return (D_FAILED);
	} else {
		/*
		 * process explicit fdisk requests in order
		 */
		WALK_LIST(fdp, fdisk) {
			if (ci_streq(fdp->disk, "all"))
				dp = NULL;
			else if ((dp = find_disk(fdp->disk)) == NULL)
				continue;	/* parser already checked */

			switch (fdp->size) {
			    case FD_SIZE_ALL:
				if (pf_part_all(dp, fdp) != D_OK)
					return (D_FAILED);
				break;

			    case FD_SIZE_DELETE:
				if (pf_part_delete(dp, fdp) != D_OK)
					return (D_FAILED);
				break;

			    case FD_SIZE_MAXFREE:
				if (pf_part_maxfree(dp, fdp) != D_OK)
					return (D_FAILED);
				break;

			    default:
				if (pf_part_explicit(dp, fdp) != D_OK)
					return (D_FAILED);
				break;
			}
		}
	}

	count = 0;

	/*
	 * deselect drives which don't have a solaris partition
	 * so the layout routines don't get confused
	 */
	WALK_DISK_LIST(dp) {
		if (disk_not_selected(dp))
			continue;

		if ((p = get_solaris_part(dp, CFG_CURRENT)) == 0) {
			if (deselect_disk(dp, NULL) != D_OK) {
				write_notice(ERRMSG,
					MSG1_DISK_DESELECT_FAILED,
					disk_name(dp));
				return (D_FAILED);
			} else {
				write_status(LOGSCR, LEVEL1|LISTITEM,
					MSG2_DISK_DESELECT_NO_PART,
					_pf_part_type_name(SUNIXOS),
					disk_name(dp));
			}
		} else {
			if (validate_fdisk(dp) != D_OK) {
				write_notice(ERRMSG,
					MSG1_FDISK_INVALID,
					disk_name(dp));
				return (D_FAILED);
			} else {
				if (part_size(dp, p) >= mb_to_sectors(30))
					count++;
			}
		}
	}
	/*
	 * check to see that there is at least one usable Solaris
	 * partition of at least 30MB in size
	 */
	if (count == 0) {
		write_notice(ERRMSG,
			MSG1_PART_SOLARIS_MIN,
			_pf_part_type_name(SUNIXOS));
		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * pf_convert_id()
 *	Convert a partition type identifier from a possible alias to
 *	an explicit type identifier for the purposes of partition
 *	creation. If an real partition type is specified, that type
 *	is returned without conversion.
 * Parameters:
 *	id	- partition id (either specific, or an alias)
 * Return:
 *	# > 0	- translated partition id (type)
 */
int
pf_convert_id(int id)
{
	if (id == DOSPRIMARY)
		return (DOSHUGE);

	return (id);
}

/* ******************************************************************** */
/* 			LOCAL FUNCTIONS					*/
/* ******************************************************************** */
/*
 * pf_part_explicit()
 *	Create a partition of the type specified in th e'fdp' structure
 *	across the entire 'dp' drive. This function requires a specific
 *	disk. Any failure on an explicit layout is considered fatal, since
 *	the user was expecting an explicit behavior.
 * Parameters:
 *	dp	- disk structure pointer. If NULL, the command should
 *		  be run across all selected drives, printing warning
 *		  messages in the event of a failure. If 'dp' is not
 *		  NULL, the command should be executed on the explicit
 *		  drive, and halt if it fails.
 *	fdp	- pointer to fdisk entry structure
 * Return:
 *	D_OK	 - explicit partition created successfully
 *	D_BADARG - invalid argument
 *	D_FAILED - explicit partition creation failed
 * Status:
 *	private
 */
static int
pf_part_explicit(Disk_t *dp, Fdisk *fdp)
{
	/* validate parameters */
	if (fdp == NULL)
		return (D_BADARG);

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp))
				(void) _pf_part_explicit(dp, fdp, 0);
		}
	} else {
		if (disk_selected(dp)) {
			if (_pf_part_explicit(dp, fdp, 1) != D_OK)
				return (D_FAILED);
		} else {
			write_notice(ERRMSG,
				MSG1_DISK_NOT_SELECTED,
				fdp->disk);
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * pf_part_all()
 *	Create a partition of the type specified in the 'fdp' structure
 *	across an entire drive. All other existing partitions are removed.
 *	If 'dp' is NULL, the command applies to all drives.  If a layout
 *	failure occurs on a non-Solaris partition, a warning is printed.
 *	If a failure occurs on a Solaris partition on any drive, pfinstall
 *	exits.
 * Parameters:
 *	dp	- disk structure pointer. If NULL, the command should
 *		  be run across all selected drives, printing warning
 *		  messages in the event of a failure. If 'dp' is not
 *		  NULL, the command should be executed on the explicit
 *		  drive, and halt if it fails.
 *	fdp	- pointer to fdisk entry structure
 * Return:
 *	D_OK	 - configuration successful
 *	D_FAILED - configuration failed
 * Status:
 *	private
 */
static int
pf_part_all(Disk_t *dp, Fdisk *fdp)
{
	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp))
				(void) _pf_part_all(dp, fdp, 0);
		}
	} else {
		if (disk_selected(dp)) {
			if (_pf_part_all(dp, fdp, 1) != D_OK)
				return (D_FAILED);
		} else {
			write_notice(ERRMSG,
				MSG1_DISK_NOT_SELECTED, fdp->disk);
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * pf_part_maxfree()
 *	Create a partition of the type specified in the 'fdp' structure
 *	in the largest segment of unused space currently found on the drive.
 *	If 'dp' is NULL, the command applies to all drives.  If a layout
 *	failure occurs on a non-Solaris partition, a warning is printed.
 *	If a failure occurs on a Solaris partition on any drive, pfinstall
 *	exits.
 * Parameters:
 *	dp	- disk structure pointer. If NULL, the command should
 *		  be run across all selected drives, printing warning
 *		  messages in the event of a failure. If 'dp' is not
 *		  NULL, the command should be executed on the explicit
 *		  drive, and halt if it fails.
 *	fdp	- pointer to fdisk entry structure
 * Return:
 *	D_OK	 -
 *	D_BADARG -
 *	D_FAILED -
 * Status:
 *	private
 */
static int
pf_part_maxfree(Disk_t *dp, Fdisk *fdp)
{
	/* validate parameters */
	if (fdp == NULL)
		return (D_BADARG);

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp))
				(void) _pf_part_maxfree(dp, fdp, 0);
		}
	} else {
		if (disk_selected(dp)) {
			if (_pf_part_maxfree(dp, fdp, 1) != D_OK) {
				write_notice(ERRMSG,
					MSG2_PART_CREATE_FAILED,
					_pf_part_type_name(fdp->id),
					disk_name(dp));
				return (D_FAILED);
			}
		} else {
			write_notice(ERRMSG,
				MSG1_DISK_NOT_SELECTED, fdp->disk);
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * pf_part_delete()
 *	Delete all partitions of the type specified in the 'fdp' structure.
 *	If 'dp' is NULL, the command applies to all drives.  If a layout
 *	failure occurs on any partition, pfinstall exits.
 * Parameters:
 *	dp	- disk structure pointer. If NULL, the command should
 *		  be run across all selected drives, printing warning
 *		  messages in the event of a failure. If 'dp' is not
 *		  NULL, the command should be executed on the explicit
 *		  drive, and halt if it fails.
 *	fdp	- pointer to fdisk entry structure
 * Return:
 *	D_OK	 - delete successful
 *	D_BADARG - invalid argument
 *	D_FAILED - delete failed
 * Status:
 *	private
 */
static int
pf_part_delete(Disk_t *dp, Fdisk *fdp)
{
	/* validate parameters */
	if (fdp == NULL)
		return (D_BADARG);

	if (dp == NULL) {
		WALK_DISK_LIST(dp) {
			if (disk_selected(dp)) {
				if (_pf_part_delete(dp, fdp) != D_OK)
					return (D_FAILED);
			}
		}
	} else {
		if (disk_selected(dp)) {
			if (_pf_part_delete(dp, fdp) != D_OK)
				return (D_FAILED);
		} else {
			write_notice(ERRMSG,
				MSG1_DISK_NOT_SELECTED,
				fdp->disk);
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * _pf_part_delete()
 *	Delete all partitions of the type specified by 'fdp' on disk 'dp'.
 *	If a layout failure occurs on any partition because of an absence
 *	of the specified partition type, a warning is printed and the function
 *	returns in error. Any other failure, and pfinstall exits, because
 *	this should never fail unless there is an underlying software problem.
 * Parameters:
 *	dp	  - non-NULL disk structure pointer
 *	fdp	  - pointer to fdisk entry structure
 * Return:
 *	D_OK	  - delete successful
 *	D_BADARG  - invalid argument
 *	D_FAILED  - delete failed
 * Status:
 *	private
 */
static int
_pf_part_delete(Disk_t *dp, Fdisk *fdp)
{
	int	p;
	int	count = 0;

	/* validate parameters */
	if (dp == NULL || fdp == NULL)
		return (D_BADARG);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG2_PART_DELETE,
		_pf_part_type_name(fdp->id), disk_name(dp));

	WALK_PARTITIONS(p) {
		if (_pf_partid_match(part_id(dp, p), fdp->id) == D_OK) {
			count++;
			/* turn off preservation, first */
			(void) set_part_preserve(dp, p, PRES_NO);
			if (set_part_attr(dp, p, UNUSED,
					GEOM_IGNORE) != D_OK) {
				write_notice(ERRMSG,
					MSG2_PART_DELETE_FAILED,
					_pf_part_type_name(fdp->id),
					disk_name(dp));
				return (D_FAILED);
			}
		}
	}

	/*
	 * print a warning message if there were no partitions of the specified
	 * type to be deleted, but this is not considered to be a fatal error
	 */
	if (count == 0) {
		write_notice(WARNMSG,
			MSG2_PART_DELETE_NONE,
			_pf_part_type_name(fdp->id),
			disk_name(dp));
	}

	return (D_OK);
}

/*
 * _pf_part_maxfree()
 *	Create a partition of the type specified by 'fdp' on disk 'dp'
 *	in the largest segment of unused space currently found on the drive.
 *	If there is an existing partition of the type specified, then there
 *	is no work to be done.
 *
 *	ALGORITHM:
 *	(1) look for an existing partition of the same type; if there
 *	    is one, you're done
 *	(2) if no existing parition, look for a free partition
 *	(3) if there isn't one, or isn't one with sufficient space
 *	    print a warning and return in error
 *	(4) return in success
 *
 *	NOTE:	Solaris partitions must be at least 30 MB.
 *
 * Parameters:
 *	dp	 - non-NULL disk structure pointer
 *	fdp	 - pointer to fdisk entry structure
 *	required - indicate if the action is required to succeed (0|1)
 * Return:
 *	D_OK	 - successful creation
 *	D_BADARG - invalid argument
 *	D_FAILED - creation failed
 * Status:
 *	private
 */
static int
_pf_part_maxfree(Disk_t *dp, Fdisk *fdp, int required)
{
	int	part = 0;
	int	size = 0;
	int	p, m;
	int	unused = 0;
	int	mtype;

	/* validate parameters */
	if (dp == NULL || fdp == NULL)
		return (D_OK);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG2_PART_MAXFREE,
		_pf_part_type_name(fdp->id), disk_name(dp));

	/*
	 * if there is an existing partition of the same type,
	 * you're done
	 */
	WALK_PARTITIONS(p) {
		if (_pf_partid_match(part_id(dp, p),
				fdp->id) == D_OK) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG2_PART_MAXFREE_EXISTS,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_OK);
		}
	}

	/* no existing partition, so look for a free partition */
	WALK_PARTITIONS(p) {
		if (part_id(dp, p) == UNUSED) {
			unused++;
			m = max_size_part_hole(dp, p);
			if (m > size) {
				size = m;
				part = p;
			}
		}
	}

	if (part > 0) {
		/* Solaris partitions must be at least 30MB */
		if (sectors_to_mb(size) < 30 && fdp->id == SUNIXOS) {
			mtype = (required ? ERRMSG : WARNMSG);
			write_notice(mtype,
				MSG2_PART_NO_FREE_SPACE,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}

		if (set_part_geom(dp, part, GEOM_IGNORE, size) != D_OK ||
				set_part_attr(dp, part,
					pf_convert_id(fdp->id),
					GEOM_IGNORE) != D_OK ||
				adjust_part_starts(dp) != D_OK) {
			write_notice(ERRMSG,
				MSG2_PART_MAXFREE_FAILED,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}
	} else {
		mtype = (required ? ERRMSG : WARNMSG);
		if (unused > 0) {
			write_notice(mtype,
				MSG2_PART_NO_FREE_SPACE,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		} else {
			write_notice(mtype,
				MSG2_PART_NO_FREE_PART,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * _pf_part_all()
 *	Create a partition of the type specified by 'fdp'  on disk 'dp'
 *	which spans the entire drive. If the layout fails, pfinstall exits,
 *	because this should never fail unless there is something seriously
 *	wrong with the underlying software.
 * Parameters:
 *	dp	 - non-NULL disk structure pointer
 *	fdp	 - fdisk entry structure pointer
 *	required - required to succeed (0|1)
 * Return:
 *	D_OK	 - successful creation
 *	D_BADARG - invalid argument
 *	D_FAILED - creation failed
 * Status:
 *	private
 */
static int
_pf_part_all(Disk_t *dp, Fdisk *fdp, int required)
{
	int	pid;
	int	mtype;

	/* validate arguments */
	if (dp == NULL || fdp == NULL)
		return (D_BADARG);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG2_PART_ALL,
		_pf_part_type_name(fdp->id), disk_name(dp));

	/*
	 * translate a general alias to a specific partition type if
	 * necessary
	 */
	if (FdiskobjConfig(LAYOUT_RESET, dp, NULL) != D_OK ||
			FdiskobjConfig(LAYOUT_DEFAULT, dp, NULL) != D_OK ||
			(pid = get_solaris_part(dp, CFG_CURRENT)) <= 0 ||
			set_part_attr(dp, pid, pf_convert_id(fdp->id),
				GEOM_IGNORE) != D_OK) {
		mtype = (required ? ERRMSG : WARNMSG);
		write_notice(mtype,
			MSG2_PART_ALL_FAILED,
			_pf_part_type_name(fdp->id),
			disk_name(dp));
		return (D_FAILED);
	}

	return (D_OK);
}

/*
 * _pf_part_explicit()
 *	Create a partition of an explicit size and type specified by
 *	'fdp' on drive 'dp'. If a layout failure occurs, a warning is
 *	printed and the function returns in error.
 *
 *	ALGORITHM:
 *	(1) look for existing entries of the same type; duplicates are
 *		not allowed
 *	(2) look for the best fit hole to hole the size specified
 *	(3) for Solaris partitions, make sure it is at least 30MB
 *	(4) allocate a partition of the exact size specified
 *	(5) if any failure occurs, print a warning message and return
 *		a failure
 *
 *	NOTE:	Solaris partitions must be at least 30 MB.
 *
 * Parameters:
 *	dp	 - non-NULL disk structure pointer
 *	fdp	 - pointer to fdisk entry structure
 *	required - required to succeed
 * Return:
 *	D_OK	  - explicit partition configured successfully
 *	D_BADARG  - invalid arguments
 *	D_FAILED  - creation failed
 */
static int
_pf_part_explicit(Disk_t *dp, Fdisk *fdp, int required)
{
	int	unused = 0;
	int	part = 0;
	int	smallest;
	int	p, m;
	int	size;
	int	mtype;

	/* validate parameters */
	if (dp == NULL || fdp == NULL)
		return (D_BADARG);

	if (disk_not_selected(dp)) {
		write_notice(ERRMSG,
			MSG1_DISK_NOT_SELECTED, fdp->disk);
		return (D_FAILED);
	}

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG2_PART_CREATE,
		_pf_part_type_name(fdp->id), disk_name(dp));

	/*
	 * look for an existing entry of the specified type. Duplicates
	 * are not allowed
	 */
	WALK_PARTITIONS(p) {
		if (_pf_partid_match(part_id(dp, p), fdp->id) == D_OK) {
			mtype = (required ? ERRMSG : WARNMSG);
			write_notice(mtype,
				MSG2_PART_CREATE_DUPLICATE,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}
	}

	size = mb_to_sectors(fdp->size);
	smallest = usable_disk_blks(dp) + 1;	/* init to whole drive + 1 */

	/*
	 * no existing partition, so look for a free partition that's
	 * big enough to hold the software and is the best fit
	 */
	WALK_PARTITIONS(p) {
		if (part_id(dp, p) == UNUSED) {
			unused++;
			m = max_size_part_hole(dp, p);
			if (m > size && m < smallest) {
				smallest = m;
				part = p;
			}
		}
	}

	if (part > 0) {
		/* Solaris partitions must be at least 30 MB */
		if (sectors_to_mb(size) < 30 && fdp->id == SUNIXOS) {
			mtype = (required ? ERRMSG : WARNMSG);
			write_notice(mtype,
				MSG2_PART_NO_FREE_SPACE,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}

		/*
		 * create the partition exactly the size the user specified,
		 * even if the hole it fits in is bigger
		 */
		if (set_part_geom(dp, part, GEOM_IGNORE, size) != D_OK ||
				set_part_attr(dp, part, pf_convert_id(fdp->id),
					GEOM_IGNORE) != D_OK ||
				adjust_part_starts(dp) != D_OK) {
			mtype = (required ? ERRMSG : WARNMSG);
			write_notice(mtype,
				MSG2_PART_CREATE_FAILED,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}
	} else {
		mtype = (required ? ERRMSG : WARNMSG);
		if (unused > 0) {
			write_notice(mtype,
				MSG2_PART_NO_FREE_SPACE,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		} else {
			write_notice(mtype,
				MSG2_PART_NO_FREE_PART,
				_pf_part_type_name(fdp->id),
				disk_name(dp));
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * _pf_part_type_name()
 *	Convert a partition id to a string representing the name.
 *	If the type is not commonly known, return the string "type #"
 *	where "#" is the partition number.
 * Parameters:
 *	id	- numeric partition type identifier
 * Return:
 *	char *	- pointer to local string containing name
 * Status:
 *	private
 */
static char *
_pf_part_type_name(int id)
{
	static char	buf[32];
	int		n;

	for (n = 0; partnames[n].name[0] != '\0'; n++) {
		if (partnames[n].id == id)
			return (partnames[n].name);
	}

	(void) sprintf(buf, "type %d", id);

	return (buf);
}

/*
 * _pf_partid_match()
 *	Determine if the profile partition id (type) specified (pfpartid)
 *	matches or is an alias for a given partition id (partid).
 * Parameters:
 *	partid	 - comparitor partition id (type). This could be an
 *		   alias for multiple partition types
 *	pfpartid - partition id (type) specified in profile
 * Return:
 *	D_OK	 - the profile partition id does specify 'partid'
 *	D_FAILED - the profile partition id does not specify 'partid'
 * Status:
 *	private
 */
static int
_pf_partid_match(int partid, int pfpartid)
{
	switch (pfpartid) {
	    case DOSPRIMARY:
		if (partid == DOSOS12 ||
				partid == DOSOS16 ||
				partid == DOSHUGE)
			return (D_OK);
		break;

	    default:
		if (partid == pfpartid)
			return (D_OK);
		break;
	}

	return (D_FAILED);
}
