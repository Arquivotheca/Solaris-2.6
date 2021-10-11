#ifndef lint
#pragma ident "@(#)pf_disk.c 1.10 96/06/27"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc. All Rights Reserved.
 */

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mntent.h>
#include <sys/vfstab.h>
#include "spmiapp_api.h"
#include "spmisvc_api.h"
#include "spmistore_api.h"
#include "spmisoft_api.h"
#include "spmicommon_api.h"
#include "profile.h"
#include "pf_strings.h"

/* prototypes */

int		configure_disks(Profile *);
int		configure_unused_disks(Profile *);

static int	configure_bootobj(Profile *);
static int	_fdisk_modified(Disk_t *);
static int	_pf_bootdevice(Disk_t *, char *);
static int	_pf_root_is_viable(Profile *, char *);
static int	_pf_count_viable_root(void);
static int	_pf_devices_conflict(char *, char *);
static int	_pf_disks_conflict(char *, char *);
static int	_pf_count_rootdisk(Profile *);
static Storage * _pf_find_filesys(Profile *, char *);
static Disk_t *	_pf_find_viable_root(Profile *, char *);

/*
 * configure_disks()
 *	All disks are already initially deselected. If the "use"
 *	list is defined, only those drives are selected. If the
 *	"dontuse" list is defined, only those drives which are not
 *	in the "dontuse" list are selected.
 * Parameters:
 *	prop	- pointer to profile structure
 * Return:
 *	D_OK	  - disk selection successful
 *	D_BADARG  - invalid argument
 *	D_FAILED  - disk selection failed
 */
int
configure_disks(Profile *prop)
{
	Disk_t		*dp;
	Namelist	*p = NULL;
	int		count;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	if (DISKUSE(prop) == NULL) {
		if (DISKDONTUSE(prop) == NULL) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG0_DISK_SELECT_ALL);

			/*
			 * select the drives that are usable on this host
			 */
			WALK_DISK_LIST(dp) {
				if (select_disk(dp, NULL) != D_OK) {
					write_notice(WARNMSG,
						MSG1_DISK_SELECT_FAILED,
						disk_name(dp));
					continue;
				}
			}
		} else {
			WALK_DISK_LIST(dp) {
				WALK_LIST(p, DISKDONTUSE(prop)) {
					if (streq(p->name,
							disk_name(dp)))
						break;
				}

				if (p == NULL) {
					if (select_disk(dp, NULL) !=
							D_OK) {
					    write_notice(WARNMSG,
						MSG1_DISK_SELECT_FAILED,
						disk_name(dp));
					}
				} else {
					write_status(LOGSCR,
						LEVEL1|LISTITEM,
						MSG1_DISK_DESELECT,
						disk_name(dp));
				}
			}
		}
	} else {
		/*
		 * select only drives that are explicitly listed in
		 * profile with "usedisk"
		 */
		WALK_LIST(p, DISKUSE(prop)) {
			/*
			 * disk has already been checked to be valid for
			 * the system by the parser
			 */
			if ((dp = find_disk(p->name)) == NULL ||
					disk_selected(dp))
				continue;

			if (select_disk(dp, NULL) != D_OK) {
				write_notice(ERRMSG,
					MSG1_DISK_SELECT_FAILED,
					disk_name(dp));
				return (D_FAILED);
			}
		}
	}

	/*
	 * make sure at least one disk is available for
	 * configuration or there is no need to go further
	 */
	count = 0;
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp))
			count++;
	}
	if (count == 0) {
		write_notice(ERRMSG,
			MSG0_NO_DISKS_SELECTED);
		return (D_FAILED);
	}

	/*
	 * setup the boot disk and if possible, the boot device
	 * NOTE: this must be called before configure_fdisk()
	 *	 as the "rootdisk" specifiers in the fdisk
	 *	 keywords must be resolved before they are
	 *	 executed
	 */
	if (configure_bootobj(prop) != D_OK)
		return (D_FAILED);

	/*
	 * configure the fdisk partitions
	 */
	if (configure_fdisk(DISKFDISK(prop)) != D_OK)
		return (D_FAILED);

	return (D_OK);
}

/*
 * configure_unused_disks()
 *	Deselect all drives which were not modified for default
 *	partitioning only.
 *
 *	ALGORITHM:
 *	For default partitioning:
 *	(1)  look for obvious modifications due to autolayout
 *		(the current and original are flat out different)
 *	(2)  look for subtle modifications where a filesys line
 *		set the mount point / size to exactly what it was
 *		(this is still considered a "modification")
 *	(3)  deselect drives which don't meet these criteria
 *
 *	For explicit partitioning:
 *	(1)  deselect all drive which were not referenced in
 *		a filesys or fdisk record.
 *
 *	For existing partitioning:
 *	(1)  do nothing
 *
 * Parameters:
 *	prop	  - pointer to profile structure
 * Return:
 *	D_OK	  - successfully configured unused disks
 *	D_BADARG  - invalid argument
 *	D_FAILED  - unused disk configuration failed
 * Status:
 *	private
 */
int
configure_unused_disks(Profile *prop)
{
	Disk_t	*dp;
	int	s;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	/*
	 * when using existing partitioning, all disks which were
	 * selected initially remain selected
	 */
	if (DISKPARTITIONING(prop) == LAYOUT_EXIST)
		return (D_OK);

	/*
	 * do not deselect drives if the configuration is not
	 * complete
	 */
	if (ResobjIsComplete(RESSIZE_MINIMUM) != NULL)
		return (D_OK);

	/*
	 * for default and explicit partitioning, for each selected
	 * disk, check to see if there were any modifications between
	 * the original disk and the current configuration
	 */
	WALK_DISK_LIST(dp) {
		if (!disk_selected(dp))
			continue;

		/* check for explicit fdisk modifications */
		if (_fdisk_modified(dp) == 1) {
			if (DISKPARTITIONING(prop) == LAYOUT_DEFAULT)
				SdiskobjAllocateUnused(dp);

			continue;
		}

		/* look for obvious slice mods due to autolayout */
		if (DISKPARTITIONING(prop) == LAYOUT_DEFAULT &&
					sdisk_compare(dp, CFG_COMMIT) != 0) {
			SdiskobjAllocateUnused(dp);
			continue;
		}

		/*
		 * look for slice mods due to filesys lines in the
		 * profile
		 */
		WALK_SLICES_STD(s) {
			if (slice_access(make_slice_name(
					disk_name(dp), s), 0) == 1)
				break;
		}

		/* check for explicit slice modifications */
		if (s <= LAST_STDSLICE) {
			if (DISKPARTITIONING(prop) == LAYOUT_DEFAULT)
				SdiskobjAllocateUnused(dp);

			continue;
		}

		/*
		 * there have been no explicit or implicit mods to
		 * the disk, so go ahead and deselect
		 */
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_DISK_DESELECT_UNMOD, disk_name(dp));

		if (deselect_disk(dp, NULL) != D_OK) {
			write_notice(ERRMSG,
				MSG1_DISK_DESELECT_UNMOD_FAILED,
				disk_name(dp));
			return (D_FAILED);
		}
	}

	return (D_OK);
}

/*
 * _fdisk_modified()
 *	Boolean function to determine if the fdisk partitions
 *	have been modified by the profile.
 * Parameters:
 *	dp	- non-NULL pointer to selected disk
 * Return
 *	1	- the fdisk configuration has been modified
 *	0	- the fdisk configuration has not been modified
 * Status:
 *	private
 */
static int
_fdisk_modified(Disk_t *dp)
{
	int	p;

	if (disk_no_fdisk_req(dp))
		return (0);

	WALK_PARTITIONS(p) {
		if (part_id(dp, p) != orig_part_id(dp, p))
			break;

		if (part_active(dp, p) != orig_part_active(dp, p))
			break;

		if (part_geom_same(dp, p, CFG_EXIST) != D_OK)
			break;
	}

	if (p <= FD_NUMPART)
		return (1);

	return (0);
}

/*
 * Function:	configure_bootobj
 * Description:	The purpose of this function is to set the boot object
 *		if explicit user directives were provided which affect
 *		it. Also, all "rootdisk" values in the profile should
 *		be expanded based on the boot object values.
 *
 *		NOTE:	fdisk keywords have not yet been processed, so
 *			we cannot count on the validity of the current
 *			sdisk state during this function unless we are
 *			researching "existing" values.
 * Parameters:	prop	  - pointer to profile structure
 * Return:	D_OK	  - root disk configuration successful
 *		D_BADARG  - invalid argument
 *		D_FAILED  - configuration failed
 */
static int
configure_bootobj(Profile *prop)
{
	Mntpnt_t	info;
	Disk_t *	bootdisk;
	int		bootdev;
	int		expldisk;
	int		expldev;
	int		nroot;
	char		devname[128];
	char *		cp;
	int		count = 0;
	Storage	*	fsp;
	Fdisk *		fdp;

	/* validate parameters */
	if (prop == NULL)
		return (D_BADARG);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG0_BOOT_CONFIGURE);

	/* initialize all boot object values to undefined */
	bootdisk = NULL;
	expldisk = 0;
	bootdev = -1;
	expldev = 0;

	/*
	 * if a "root_device" keyword was specified, this value
	 * takes precedence for resolving the rootdisk
	 */
	if (ROOTDEVICE(prop) != NULL) {
		/*
		 * check for conflict between root_device and
		 * boot_device keywords
		 */
		if (BOOTDEVICE(prop) != NULL &&
				!ci_streq(BOOTDEVICE(prop), "any")) {
			if (_pf_disks_conflict(BOOTDEVICE(prop),
						ROOTDEVICE(prop)) ||
					(IsIsa("sparc") &&
					    _pf_devices_conflict(
						BOOTDEVICE(prop),
						ROOTDEVICE(prop)))) {
				write_notice(ERRMSG,
					MSG2_ROOTDEV_BOOTDEV_CONFLICT,
					ROOTDEVICE(prop),
					BOOTDEVICE(prop));
				return (D_FAILED);
			}
		}
		/*
		 * check for conflict between root_device and filesys
		 * keyword for "/" file system
		 */
		if ((fsp = _pf_find_filesys(prop, ROOT)) != NULL) {
			if (_pf_disks_conflict(ROOTDEVICE(prop), fsp->dev) ||
					_pf_devices_conflict(ROOTDEVICE(prop),
						fsp->dev)) {
				write_notice(ERRMSG,
					MSG2_ROOTDEV_FILESYS_CONFLICT,
					ROOTDEVICE(prop),
					fsp->dev);
				return (D_FAILED);
			}
		}

		/*
		 * check for conflict between root_device and existing
		 * partitioning
		 */
		if (DISKPARTITIONING(prop) == LAYOUT_EXIST) {
			if (_pf_root_is_viable(prop, ROOTDEVICE(prop)) == 0) {
				write_notice(ERRMSG,
					MSG1_ROOTDEV_PARTITIONING_CONFLICT,
					ROOTDEVICE(prop));
				return (D_FAILED);
			}
		}

		/* no conflicts; set the value from the root_device keyword */
		bootdisk = find_disk(ROOTDEVICE(prop));
		bootdev = _pf_bootdevice(NULL, ROOTDEVICE(prop));
		expldisk = 1;
		expldev = (bootdev == -1 ? 0 : 1);
	}

	/*
	 * if a "boot_device" was specified, determine the value
	 * of the rootdisk and from this value.
	 */
	if (bootdisk == NULL && BOOTDEVICE(prop) != NULL &&
			!ci_streq(BOOTDEVICE(prop), "any")) {
		/*
		 * check for conflict between the boot_device and
		 * filesys keyword for "/" file system
		 */
		if ((fsp = _pf_find_filesys(prop, ROOT)) != NULL) {
			if (_pf_disks_conflict(BOOTDEVICE(prop), fsp->dev) ||
					(IsIsa("sparc") &&
					    _pf_devices_conflict(
						BOOTDEVICE(prop), fsp->dev))) {
				write_notice(ERRMSG,
					MSG2_BOOTDEV_FILESYS_CONFLICT,
					BOOTDEVICE(prop),
					fsp->dev);
				return (D_FAILED);
			}
		}

		/*
		 * check for conflict between the boot_device and
		 * existing partitioning configuration
		 */
		if (DISKPARTITIONING(prop) == LAYOUT_EXIST) {
			if (_pf_root_is_viable(prop, BOOTDEVICE(prop)) == 0) {
				write_notice(ERRMSG,
					MSG1_BOOTDEV_PARTITIONING_CONFLICT,
					BOOTDEVICE(prop));
				return (D_FAILED);
			}
		}

		/* no conflicts; set the value from the boot_device keyword */
		bootdisk = find_disk(BOOTDEVICE(prop));
		bootdev = _pf_bootdevice(NULL, BOOTDEVICE(prop));
		expldisk = 1;
		expldev = (bootdev == -1 ? 0 : 1);
	}

	/*
	 * if a "filesys" keyword was specified for "/", determine
	 * the value of rootdisk from this value
	 */
	if (bootdisk == NULL && (fsp = _pf_find_filesys(prop, ROOT)) != NULL) {
		/*
		 * check for conflict between filesys keyword for "/" and
		 * existing partitioning configuration
		 */
		if (is_slice_name(fsp->dev)) {
			if (DISKPARTITIONING(prop) == LAYOUT_EXIST) {
				if (_pf_root_is_viable(prop, fsp->dev) == 0) {
					write_notice(ERRMSG,
					    MSG1_FILESYS_PARTITIONING_CONFLICT,
					    fsp->dev);
					return (D_FAILED);
				}
			}

			/* set the value from the boot_device keyword */
			bootdisk = find_disk(fsp->dev);
			bootdev = _pf_bootdevice(NULL, fsp->dev);
			expldisk = 1;
			expldev = (bootdev == -1 ? 0 : 1);
		} else if (strstr(fsp->dev, "rootdisk")) {
			if (DISKPARTITIONING(prop) == LAYOUT_EXIST) {
				if ((bootdisk = _pf_find_viable_root(prop,
						fsp->dev)) == NULL) {
					write_notice(ERRMSG,
					    MSG1_FILESYS_PARTITIONING_CONFLICT,
					    fsp->dev);
					return (D_FAILED);
				}

				/* set the value from the boot_device keyword */
				bootdev = _pf_bootdevice(NULL, fsp->dev);
				expldisk = 1;
				expldev = (bootdev == -1 ? 0 : 1);
			}
		}
	}

	/*
	 * if "partitioning" existing, find the existing "/" slice
	 */
	if (bootdisk == NULL && DISKPARTITIONING(prop) == LAYOUT_EXIST) {
		nroot = _pf_count_viable_root();
		if (nroot == 0) {
			write_notice(ERRMSG, MSG0_EXISTING_NO_ROOT);
			return (D_FAILED);
		} else if (nroot == 1) {
			/* set the value from the existing partitioning */
			(void) find_mnt_pnt(NULL, NULL, ROOT, &info, CFG_EXIST);
			bootdisk = info.dp;
			bootdev = _pf_bootdevice(info.dp, NULL);
			expldisk = 1;
			expldev = (bootdev == -1 ? 0 : 1);
		} else if (nroot > 1) {
			write_notice(ERRMSG, MSG0_EXISTING_MULTIPLE_ROOT);
			return (D_FAILED);
		}
	}

	/*
	 * if we still haven't resolved the disk for rootdisk, pick initial
	 * value for the boot disk (only the disk) only if there is at least
	 * one fdisk or filesys keyword requiring "rootdisk" keyword
	 * substitution
	 */
	if (bootdisk == NULL && _pf_count_rootdisk(prop) > 0) {
		/*
		 * you can only check disk selection at this point because we
		 * haven't processed the Solaris partition keywords for "fdisk"
		 */
		if (DiskobjFindBoot(CFG_EXIST, &bootdisk) != D_OK ||
				bootdisk == NULL ||
				!disk_selected(bootdisk)) {
			WALK_DISK_LIST(bootdisk) {
				if (disk_selected(bootdisk))
					break;
			}

			if (bootdisk == NULL) {
				write_notice(ERRMSG,
					MSG0_DISK_BOOT_NO_DISKS);
				return (D_FAILED);
			}
		}
	}

	/*
	 * if the boot disk was explicitly specified, make sure it
	 * is available for configuration
	 */
	if (bootdisk != NULL && expldisk == 1) {
		if (!disk_selected(bootdisk)) {
			write_notice(ERRMSG,
				MSG1_DISK_BOOT_NOT_SELECTED,
				disk_name(bootdisk));
			return (D_FAILED);
		}
	}
	/*
	 * update the boot object using the specified attributes
	 */
	if (get_trace_level() > 1) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			"Setting boot disk %s (%d) and device %d (%d)",
			bootdisk != NULL ? disk_name(bootdisk) : "NULL",
			expldisk, bootdev, expldev);
	}

	if (BootobjSetAttribute(CFG_CURRENT,
		    BOOTOBJ_DISK,
			bootdisk != NULL ? disk_name(bootdisk) : NULL,
		    BOOTOBJ_DISK_EXPLICIT, expldisk,
		    BOOTOBJ_DEVICE, bootdev,
		    BOOTOBJ_DEVICE_EXPLICIT, expldev,
		    NULL) != D_OK) {
		write_notice(ERRMSG, MSG0_DISK_BOOT_SPEC_FAILED);
		return (D_FAILED);
	}

	/*
	 * update all filesys records with "rootdisk" to the boot device
	 */
	WALK_LIST(fsp, DISKFILESYS(prop)) {
		if ((cp = strrchr(fsp->dev, '.')) != NULL) {
			*cp++ = '\0';
			(void) sprintf(devname,
				"%s%s", disk_name(bootdisk), cp);
			free(fsp->dev);
			fsp->dev = xstrdup(devname);
			count++;
		}
	}
	/*
	 * update all fdisk records with "rootdisk" to the boot device
	 */
	WALK_LIST(fdp, DISKFDISK(prop)) {
		if (streq(fdp->disk, "rootdisk")) {
			free(fdp->disk);
			fdp->disk = xstrdup(disk_name(bootdisk));
			count++;
		}
	}
	/*
	 * post a message if at least one profile record had a "rootdisk"
	 * keyword substitution
	 */
	if (count > 0) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_DISK_ROOTDEVICE,
			disk_name(bootdisk));
	}

	return (D_OK);
}

/*
 * Function:	_pf_bootdevice
 * Description:	Determine what the boot device index value is for the
 *		given disk.
 *		  (1) SPARC, set to '0'
 *		  (2) Intel, set to the Solaris partition index
 *		  (3) PowerPC, set to the DOS partition index
 * Parameters:	dp	[RO, *RO] (Disk_t *)
 *			Pointer to disk object to use to search
 *			for valid boot device. NULL if specified
 *			with 'name'.
 *		name	[RO, *RO] (char *)
 *			Name of device to use for searching via parsing
 *			NULL if specified with 'dp'.
 * Return:	-1	unspecified boot device
 *		 0	boot device index
 */
static int
_pf_bootdevice(Disk_t *dp, char *name)
{
	int	bootdev;
	char *	cp;

	if (dp != NULL) {
		if (IsIsa("sparc")) {
			bootdev = 0;
		} else if (IsIsa("i386")) {
			bootdev = get_solaris_part(dp, CFG_CURRENT);
			if (invalid_fdisk_part(bootdev))
				bootdev = -1;
		} else if (IsIsa("ppc")) {
			WALK_PARTITIONS(bootdev) {
				if (part_id(dp, bootdev) == DOSOS12 ||
						part_id(dp, bootdev) == DOSOS16)
					break;
			}
			if (invalid_fdisk_part(bootdev))
				bootdev = -1;
		} else {
			bootdev = -1;
		}
	} else if (name != NULL) {
		if (IsIsa("sparc") && (cp = strrchr(name, 's')) != NULL)
			bootdev = atoi(++cp);
		else
			bootdev = -1;
	} else
		return (-1);

	return (bootdev);
}

/*
 * Function:	_pf_find_filesys
 * Description:	Find a filesys keyword record for the file system name
 *		specified which contains either an explicit slice device,
 *		or a "rootdisk" value.
 * Parameters:	prop	[RO, *RO] (Profile *)
 *			Profile structure pointer.
 *		name	[RO, *RO] (char *)
 *			File system name being used in search.
 * Return:	NULL	Invalid parameter, or no matching filesys record.
 *		!NULL	Pointer to the filesys record.
 */
static Storage *
_pf_find_filesys(Profile *prop, char *name)
{
	Storage *	fsp;

	if (prop == NULL || name == NULL || *name == '0')
		return (NULL);

	WALK_LIST(fsp, DISKFILESYS(prop)) {
		if (streq(fsp->name, name) &&
				(is_slice_name(fsp->dev) ||
					strstr(fsp->dev, "rootdisk")))
			break;
	}

	return (fsp);
}

/*
 * Function:	_pf_disks_conflict
 * Description: Determine if two disk device profile keyword specifiers
 *		directly conflict. Only check the disk portion of the
 *		device specifier (i.e. no guarantees on slice mismatch).
 * Parmaters:	src1	[RO, *RO] (char *)
 *			Name of first disk (can be a full slice or fdisk
 *			partition device).
 *		src2	[RO, *RO] (char *)
 *			Name of second disk (can be a full slice or fdisk
 *			partition device)
 * Return:	0	No conflict.
 *		1	A conflict exists.
 */
static int
_pf_disks_conflict(char *src1, char *src2)
{
	char	disk1[32];
	char	disk2[32];

	if (src1 == NULL || src2 == NULL)
		return (0);

	if (strstr(src1, "rootdisk") || strstr(src2, "rootdisk"))
		return (0);

	if (simplify_disk_name(disk1, src1) == -1 ||
			simplify_disk_name(disk2, src2) == -1)
		return (1);

	if (streq(disk1, disk2))
		return (0);

	return (1);
}

/*
 * Function:	_pf_device_conflict
 * Description:	Determine if a given slice device conflicts with a profile
 *		keyword device.
 * Parameters:	anydev	[RO, *RO] (char *)
 *			Device against which a specified slice is
 *			being compared.
 *		slicedev [RO, *RO] (char *)
 *			A specified slice device (e.g. c0t3d0s3)
 * Return:	0	There is no conflict
 *		1	A conflict exists.
 */
static int
_pf_devices_conflict(char *anydev, char *slicedev)
{
	int	sindex = -1;
	int	aindex = -1;
	char *	cp;

	if (anydev == NULL || slicedev == NULL)
		return (0);

	if ((cp = strrchr(slicedev, 's')) != NULL)
		sindex = atoi(++cp);

	if (IsIsa("sparc") && (cp = strrchr(anydev, 's')) != NULL)
		aindex = atoi(++cp);

	if (aindex < 0 || sindex < 0 || sindex == aindex)
		return (0);

	return (1);
}

/*
 * Function:	_pf_find_viable_root
 * Description:	Retrieve a disk pointer for the disk containing a "/"
 *		file system on the slice device specified.
 * Parameters:	prop	[RO, *RO] (Profile *)
 *			Profile structure pointer.
 *		device	[RO, *RO] (char *)
 *			c#[t#]d#s# slice specifier for a slice proposed to
 *			contain a "/" file system on the existing system
 * Return:	NULL	device is not a slice or there are no disks with
 *			a root file system on a usable disk
 */
static Disk_t *
_pf_find_viable_root(Profile *prop, char *device)
{
	int	  slice;
	char *	  cp;
	Disk_t *  dp;

	if (prop == NULL || device == NULL || !is_slice_name(device))
		return (NULL);

	cp = strrchr(device, 's');
	slice = atoi(++cp);
	WALK_DISK_LIST(dp) {
		if (sdisk_is_usable(dp))
			if (streq(slice_mntpnt(dp, slice), ROOT))
				break;
	}

	return (dp);
}

/*
 * Function:	_pf_count_viable_root
 * Description:	Count the number of instance of a "/" file system in the
 *		existing configuration.
 * Parameters:	none
 * Return:	# >= 0	Number of slices in the existing configuration
 *		which contain a "/" file system
 */
static int
_pf_count_viable_root(void)
{
	int	  count = 0;
	int	  s;
	Disk_t *  dp;

	WALK_DISK_LIST(dp) {
		if (sdisk_is_usable(dp)) {
			WALK_SLICES(s) {
				if (streq(slice_mntpnt(dp, s), ROOT))
					count++;
			}
		}
	}

	return (count);
}

/*
 * Function:	_pf_root_is_viable
 * Description:	For initial installations, find out if the device specified
 *		designates a slice which actually contains a "/" file system
 *		in the existing disk configuration.
 * Parameters:	prop	[RO, *RO] (Profile *)
 *			Profile structure pointer.
 *		device	[RO, *RO] (char *)
 *			c#[t#]d#s# slice specifier for a slice proposed to
 *			contain a "/" file system on the existing system
 * Return:	0	The specified device does not contain a "/"
 *		1	A "/" file system was verified on the slice
 */
static int
_pf_root_is_viable(Profile *prop, char *device)
{
	Disk_t *	dp;
	int		s;
	char *		cp;

	if (ISOPTYPE(prop, SI_INITIAL_INSTALL)) {
		if (is_slice_name(device) &&
				(dp = find_disk(device)) != NULL) {
			if ((cp = strrchr(device, 's')) != NULL) {
				s = atoi(++cp);
				if (streq(orig_slice_mntpnt(dp, s), ROOT))
					return (1);
			}
		}
	}

	return (0);
}

/*
 * Function:	_pf_count_rootdisk
 * Description:	Count the number of filesys and fdisk records which contain
 *		the device specifier "rootdisk".
 * Parameters:	prop	[RO, *RO] (Profile *)
 *			Profile structure pointer.
 * Return:	0	No records contain this device
 *		# > 0	1 or more records contain the "rootdisk" specifier
 */
static int
_pf_count_rootdisk(Profile *prop)
{
	Fdisk *	  fdp;
	Storage * fsp;
	int	  count = 0;

	WALK_LIST(fsp, DISKFILESYS(prop))
		if (strstr(fsp->dev, "rootdisk"))
			count++;

	WALK_LIST(fdp, DISKFDISK(prop))
		if (strstr(fdp->disk, "rootdisk"))
			count++;

	return (count);
}
