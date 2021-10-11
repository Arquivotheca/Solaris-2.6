#ifndef lint
#pragma ident "@(#)store_strings.h 1.8 96/06/17 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	spmistore_strings.h
 * Group:	libspmistore
 * Description:
 */

#ifndef _SPMISTORE_STRINGS_H
#define	_SPMISTORE_STRINGS_H

#include <libintl.h>

/* constants */

#ifndef	TEXT_DOMAIN
#define	TEXT_DOMAIN	"SUNW_INSTALL_LIBSTORE"
#endif

#ifndef ILIBSTR
#define	ILIBSTR(x)	dgettext(TEXT_DOMAIN, x)
#endif

/* message strings */

#define	INSTALL_DEFAULT_VTOC		ILIBSTR(\
	"Installing a default Solaris label (%s)")

/*
 * store_check.c strings
 */
#define	MSG3_PART_ORDER_INVALID		ILIBSTR(\
	"Partitions %d and %d are out of order (%s)")
#define	MSG3_PART_OVERLAP		ILIBSTR(\
	"Partitions %d and %d overlap (%s)")
#define	MSG0_SLICE_BOOT_BEYOND_BIOS	ILIBSTR(\
	"The boot slice extends beyond disk cylinder 1023")
#define	MSG0_SLICE_ALTSECT_BEYOND_BIOS	ILIBSTR(\
	"The alternate sector slice extends beyond disk cylinder 1023")
#define	MSG0_SLICE_ROOT_BEYOND_BIOS	ILIBSTR(\
	"The '/' slice extends beyond HBA cylinder 1023")
#define	MSG0_SOLARIS_BEYOND_BIOS	ILIBSTR(\
	"The Solaris partition starts beyond HBA cylinder 1023")
#define	MSG1_BOOT_DISK_NO_ROOT		ILIBSTR(\
	"There is no \'/\' file system on the boot disk (%s)")
#define	MSG0_BOOT_DEVICE_NO_ROOT	ILIBSTR(\
	"There is no \'/\' file system on the boot device")
#define	MSG1_BOOT_DISK_NO_SOLARIS	ILIBSTR(\
	"There is no Solaris partition on the boot disk (%s)")
#define	MSG0_BOOT_DEVICE_NO_SOLARIS	ILIBSTR(\
	"There is no Solaris partition on the boot device")
#define	MSG1_BOOT_DISK_NO_DOS		ILIBSTR(\
	"There is no DOS partition on the boot disk (%s)")
#define	MSG0_BOOT_DEVICE_NO_DOS		ILIBSTR(\
	"There is no DOS partition on the boot device")
#define	MSG0_BOOT_PROM_CHANGING		ILIBSTR(\
	"The system firmware will be modified for automatic rebooting")
#define	MSG0_BOOT_PROM_CHANGE_REQUIRED	ILIBSTR(\
	"You must modify the system firmware for automatic rebooting")
#define	MSG2_PART_BEYOND_END		ILIBSTR(\
	"Partition %d extends beyond the end of the disk (%s)")
#define	MSG1_SLICE_BEYOND_END		ILIBSTR(\
	"Slice extends beyond the end of the disk (%s)")
#define	MSG1_SLICE_SIZE_TOOSMALL	ILIBSTR(\
	"Slice is less than one cylinder (%s)")
#define	MSG3_SLICE_OVERLAP		ILIBSTR(\
	"Slices %d and %d overlap (%s)")
#define	MSG3_SLICE_DUPLICATE		ILIBSTR(\
	"Slices %d and %d have duplicate mount points (%s)")
#define	MSG4_SLICE_DUPLICATE_DIFF	ILIBSTR(\
	"Slices %d on disk %s and %d on disk %s have duplicate mount points")
#define	UNUSED_SLICE_SPACE		ILIBSTR(\
	"Unused disk space (%s)")
#define	ILLEGAL_SLICE_CONFIG		ILIBSTR(\
	"Slice configuration is not legal (%s)")

/*
 * store_common.c strings
 */
#define	MSG0_ALIGNED			ILIBSTR(\
	"The starting sector for the slice has been cylinder aligned")
#define	MSG0_ALTSLICE			ILIBSTR(\
	"Cannot modify the alternate sector slice")
#define	MSG0_BADARGS			ILIBSTR(\
	"Internal error: incorrect argument to function")
#define	MSG0_BADDISK			ILIBSTR(\
	"Cannot update this disk in its current state")
#define	MSG0_BADORDER			ILIBSTR(\
	"Physical ordering of slice or partition is not valid")
#define	MSG0_BOOTCONFIG			ILIBSTR(\
	"Boot disk does not contain the \"/\" file system")
#define	MSG0_BOOTFIXED			ILIBSTR(\
	"Cannot change boot device designation")
#define	MSG0_CANTPRES			ILIBSTR(\
	"Cannot preserve slice or partition")
#define	MSG0_CHANGED			ILIBSTR(\
	"Slice or partition has been modified")
#define	MSG0_DUPMNT			ILIBSTR(\
	"Duplicate mount point")
#define	MSG0_GEOMCHNG			ILIBSTR(\
	"Disk geometry changed")
#define	MSG0_IGNORED			ILIBSTR(\
	"Action failed because slice marked \"ignored\"")
#define	MSG0_ILLEGAL			ILIBSTR(\
	"Disk layout is not valid")
#define	MSG0_LOCKED			ILIBSTR(\
	"Slice state is fixed and can not be changed")
#define	MSG0_NOFIT			ILIBSTR(\
	"Not enough contiguous space available")
#define	MSG0_NOGEOM			ILIBSTR(\
	"No disk geometry defined")
#define	MSG0_NOSOLARIS			ILIBSTR(\
	"No Solaris FDISK partition configured on the disk")
#define	MSG0_NOSPACE			ILIBSTR(\
	"Not enough free space available")
#define	MSG0_NOTSELECT			ILIBSTR(\
	"Disk is not selected")
#define	MSG0_OFF			ILIBSTR(\
	"Slice or partition extends beyond the end of the disk")
#define	MSG0_OK				ILIBSTR(\
	"Completion successful")
#define	MSG0_OUTOFREACH			ILIBSTR(\
	"Critical data cannot be reached by the BIOS code")
#define	MSG0_OVER			ILIBSTR(\
	"Slices or partitions overlap")
#define	MSG0_PRESERVED			ILIBSTR(\
	"Cannot modify a preserved slice or partition")
#define	MSG0_ZERO			ILIBSTR(\
	"Slice or partition size is '0'")
#define	MSG0_FAILED			ILIBSTR(\
	"Requested operation failed")
#define	MSG1_STD_UNKNOWN_ERROR		ILIBSTR(\
	"Unknown return code (%d)")
#define	MSG0_DISK_INVALID		ILIBSTR(\
	"Disk is not valid on this machine")

/*
 * store_disk.c
 */
#define	MSG1_INTERNAL_DISK_COMMIT	ILIBSTR(\
	"Could not commit state (%s)")

#endif	/* _SPMISTORE_STRINGS_H */
