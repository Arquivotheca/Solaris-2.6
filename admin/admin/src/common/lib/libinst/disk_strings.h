/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Disassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#ifndef _DISK_STRINGS_H
#define	_DISK_STRINGS_H
#endif

#pragma ident "@(#)disk_strings.h 1.14 95/04/07"

/*
 * disk_find.c strings
 */
#define	INSTALL_DEFAULT_VTOC		ILIBSTR(\
	"Installing a default Solaris label (%s)")

/*
 * disk_profile.c strings
 */
#define	MSG0_DEFAULT_CONFIGURE_ALL	ILIBSTR(\
	"Automatically configuring remaining system file systems")
#define	MSG0_INTERNAL_GET_DFLTMNT	ILIBSTR(\
	"Could not get default mount list")
#define	MSG0_INTERNAL_SET_DFLTMNT	ILIBSTR(\
	"Could not set default mount list")
#define	MSG0_INTERNAL_FREE_DFLTMNT	ILIBSTR(\
	"Could not free default mount list")
#define	MSG1_INTERNAL_DISK_RESET	ILIBSTR(\
	"Could not initialize state (%s)")
#define	MSG1_INTERNAL_DISK_COMMIT	ILIBSTR("Could not commit state (%s)")
#define	MSG2_SLICE_REALIGNED		ILIBSTR(\
	"Slice %s has been realigned to cylinder boundaries (%s)")
#define	MSG2_SLICE_ALIGN_REQUIRED	ILIBSTR(\
	"%s must be cylinder aligned (%s)")
#define	MSG2_SLICE_PRESERVE		ILIBSTR("Preserving slice %s (%s)")
#define	MSG1_SLICE_PRESERVE_FAILED	ILIBSTR("Could not preserve slice (%s)")
#define	MSG2_SLICE_EXISTING		ILIBSTR(\
	"Preserving existing geometry for %s (%s)")
#define	MSG0_STD_UNNAMED		ILIBSTR("unnamed")
#define	MSG0_EXISTING_FS_SIZE_INVALID	ILIBSTR(\
	"Existing partitioning requires a size of \"existing\"")
#define	MSG2_SLICE_CONFIGURE		ILIBSTR("Configuring %s (%s)")
#define	MSG1_DEFAULT_SIZE_INVALID	ILIBSTR("No default size (%s)")
#define	MSG1_NO_ALL_FREE_DISK		ILIBSTR(\
	"No completely free disk available (%s)")
#define	MSG1_DISK_INVALID		ILIBSTR(\
	"Disk is not valid on this machine (%s)")
#define	MSG0_DISK_INVALID		ILIBSTR(\
	"Disk is not valid on this machine")
#define	MSG1_DISK_NOT_FREE		ILIBSTR("All slices are in use (%s)")
#define	MSG0_DISKS_NOT_FREE		ILIBSTR("No disk is completely free")
#define	MSG2_SLICE_SIZE_NOT_AVAIL	ILIBSTR(\
	"No %d MB slice available (%s)")
#define	MSG1_SLICE_NOT_AVAIL		ILIBSTR(\
	"No unused slice available (%s)")
#define	MSG2_SLICE_ANY_BECOMES		ILIBSTR(\
	"\"any\" for %s becomes \"%s\"")
#define	MSG1_SLICE_IGNORE		ILIBSTR("Ignoring slice (%s)")
#define	MSG1_SLICE_IGNORE_FAILED	ILIBSTR("Could not ignore slice (%s)")
#define	MSG1_SLICE_PRESERVE_OFF_FAILED	ILIBSTR(\
	"Could not disable preserve on slice (%s)")
#define MSG2_START_CYL_INVALID          gettext(\
	"Starting cylinder (%d) precedes first available cylinder (%d)")
#define MSG1_START_CYL_EXCEEDS_DISK     ILIBSTR(\
	"Starting cylinder exceeds disk capacity (%d)")
#define	MSG1_SLICE_GEOM_SET_FAILED	ILIBSTR(\
	"Could not fit slice on disk (%s)")

/*
 * tracing messages for disk_profile.c
 */
#define	MSG1_DFLTMNT_FORCE_IGNORE	ILIBSTR("Force DFLT_IGNORE (%s)")
#define	MSG1_DFLTMNT_FORCE_SELECT	ILIBSTR("Force DFLT_SELECT (%s)")
#define	MSG0_DFLTMNT_CLEAR		ILIBSTR(\
	"Force DFLT_IGNORE on all mount points")

/*
 * disk_util.c strings
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
	"Cannot alter boot disk designation")
#define	MSG0_CANTPRES			ILIBSTR(\
	"Cannot preserve slice or partition")
#define	MSG0_CHANGED			ILIBSTR(\
	"Slice or partition has been modified")
#define	MSG0_DUPMNT			ILIBSTR(\
	"Duplicate mount point")
#define	MSG0_GEOMCHNG			ILIBSTR("Disk geometry changed")
#define	MSG0_IGNORED			ILIBSTR(\
	"Action failed because slice marked \"ignored\"")
#define	MSG0_ILLEGAL			ILIBSTR("Disk layout is not valid")
#define	MSG0_LOCKED			ILIBSTR(\
	"Slice state is fixed and can not be changed")
#define	MSG0_NOFIT			ILIBSTR(\
	"Not enough contiguous space available")
#define	MSG0_NOGEOM			ILIBSTR("No disk geometry defined")
#define	MSG0_NOSOLARIS			ILIBSTR(\
	"No Solaris FDISK partition configured on the disk")
#define	MSG0_NOSPACE			ILIBSTR(\
	"Not enough free space available")
#define	MSG0_NOTSELECT			ILIBSTR("Disk is not selected")
#define	MSG0_OFF			ILIBSTR(\
	"Slice or partition extends beyond the end of the disk")
#define	MSG0_OK				ILIBSTR("Completion successful")
#define	MSG0_OUTOFREACH			ILIBSTR(\
	"Critical data cannot be reached by the BIOS code")
#define	MSG0_OVER			ILIBSTR("Slices or partitions overlap")
#define	MSG0_PRESERVED			ILIBSTR(\
	"Cannot modify a preserved slice or partition")
#define	MSG0_ZERO			ILIBSTR(\
	"Slice or partition size is '0'")
#define	MSG0_FAILED			ILIBSTR("Requested operation failed")
#define	MSG1_STD_UNKNOWN_ERROR		ILIBSTR("Unknown return code (%d)")

/*
 * disk_check.c strings
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
	"The '/' slice extends beyond disk cylinder 1023")
#define	MSG0_SOLARIS_BEYOND_BIOS	ILIBSTR(\
	"The Solaris partition starts beyond disk cylinder 1023")
#define	MSG1_DISK_BOOT_NO_ROOT		ILIBSTR(\
	"The boot disk is not selected or does not have a \"/\" mount point (%s)")
#define	MSG2_PART_BEYOND_END		ILIBSTR(\
	"Partition %d extends beyond the end of the disk (%s)")
#define	MSG1_SLICE_BEYOND_END		ILIBSTR(\
	"Slice extends beyond the end of the disk (%s)")
#define	MSG1_SLICE_SIZE_TOOSMALL	ILIBSTR(\
	"Slice is less than one cylinder (%s)")
#define	MSG3_SLICE_OVERLAP		ILIBSTR("Slices %d and %d overlap (%s)")
#define	MSG3_SLICE_DUPLICATE		ILIBSTR(\
	"Slices %d and %d have duplicate mount points (%s)")
#define	MSG4_SLICE_DUPLICATE_DIFF	ILIBSTR(\
	"Slices %d on disk %s and %d on disk %s have duplicate mount points")
#define	UNUSED_SLICE_SPACE		ILIBSTR("Unused disk space (%s)")
#define	ILLEGAL_SLICE_CONFIG		ILIBSTR(\
	"Slice configuration is not legal (%s)")

/* Table entries */

#define	MSG11_SLICE_TABLE_ENTRY		ILIBSTR(\
	"SLICE %2d  start: %4d  size: %7d  mount: %-16s  options: %-8s  [%s%s%s%s%s%s ]\r\n")

/* Debug strings */

#define	MSG2_TRACE_AUTO_SIZE		ILIBSTR("Auto size is %d sectors (~%d MB)")
#define	MSG0_TRACE_MOUNT_LIST		ILIBSTR("Mount List")
