#ifndef lint
#pragma ident "@(#)svc_strings.h 1.12 96/08/30 SMI"
#endif

/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmisvc_strings.h
 * Group:	libspmisvc
 * Description:	This header contains strings used in libspmisvc
 *		library modules.
 */

#include <libintl.h>

/* constants */

#ifndef	TEXT_DOMAIN
#define	TEXT_DOMAIN	"SUNW_INSTALL_LIBSVC"
#endif

#ifndef ILIBSTR
#define	ILIBSTR(x)	dgettext(TEXT_DOMAIN, x)
#endif

/* message strings */

#define	MSG0_TRACE_MOUNT_LIST		ILIBSTR(\
	"Mount List")
#define	MSG2_FILESYS_MOUNT_FAILED	ILIBSTR(\
	"Could not mount %s (%s)")

/*
 * svc_updateconfig.c messages
 */

#define	MSG_OPEN_FAILED			ILIBSTR(\
	"Could not open file (%s)")
#define	MSG0_HOST_ADDRESS		ILIBSTR(\
	"Network host addresses (/etc/hosts)")
#define	MSG0_REBOOT_MESSAGE		ILIBSTR(\
	"The system will not automatically reconfigure devices upon reboot."\
	" You must use 'boot -r' when booting the system.")
#define	MSG1_DIR_ACCESS_FAILED		ILIBSTR(\
	"Could not access directory (%s)")
#define	MSG1_FILE_ACCESS_FAILED		ILIBSTR(\
	"Could not access file (%s)")
#define	MSG0_BOOTRC_INSTALL		ILIBSTR(\
	"Installing boot startup script (/etc/bootrc)")
#define	MSG0_BOOTENV_INSTALL		ILIBSTR(\
	"Updating boot environment configuration file")
#define	VFSTAB_COMMENT_LINE1		ILIBSTR(\
	"# This file contains vfstab entries for file systems on disks which\n")
#define	VFSTAB_COMMENT_LINE2		ILIBSTR(\
	"# were not selected during installation. The system administrator\n")
#define	VFSTAB_COMMENT_LINE3		ILIBSTR(\
	"# should put the entries which are intended to be active in the\n")
#define	VFSTAB_COMMENT_LINE4		ILIBSTR(\
	"# /etc/vfstab file, and create corresponding mount points.\n")
#define	MSG0_BOOT_BLOCK_NOTEXIST	ILIBSTR(\
	"No boot block found")
#define	MSG0_PBOOT_NOTEXIST		ILIBSTR(\
	"No pboot file found")
#define	MSG0_INSTALLBOOT_FAILED		ILIBSTR(\
	"installboot(1M) failed")
#define	MSG0_DEVICES_CUSTOMIZE		ILIBSTR(\
	"Customizing system devices")
#define	MSG0_DEVICES_LOGICAL		ILIBSTR(\
	"Logical devices (/dev)")
#define	MSG0_DEVICES_PHYSICAL		ILIBSTR(\
	"Physical devices (/devices)")
#define	MSG0_VFSTAB_UNSELECTED		ILIBSTR(\
	"Unselected disk mount points \
(/var/sadm/system/data/vfstab.unselected)")
#define	MSG0_VFSTAB_INSTALL_FAILED	ILIBSTR(\
	"Could not install new vfstab data")
#define	MSG1_FILE_ACCESS_FAILED		ILIBSTR(\
	"Could not access file (%s)")
#define	MSG1_TRANS_ATTRIB_FAILED	ILIBSTR(\
	"Could not set file attributes (%s)")
#define	MSG0_TRANS_SETUP_FAILED		ILIBSTR(\
	"Could not initialize transfer list")
#define	MSG0_TRANS_CORRUPT		ILIBSTR(\
	"Transfer list corrupted")
#define	MSG0_BOOT_INFO_INSTALL		ILIBSTR(\
	"Installing boot information")
#define	MSG0_BOOT_FIRMWARE_UPDATE	ILIBSTR(\
	"Updating system firmware for automatic rebooting")
#define	MSG1_BOOT_BLOCKS_INSTALL	ILIBSTR(\
	"Installing boot blocks (%s)")
#define	MSG0_ROOT_UNSELECTED		ILIBSTR(\
	"The / mount point is not on a selected disk")
#define	MSG1_DEV_INSTALL_FAILED		ILIBSTR(\
	"Could not install devices (%s)")
#define	MSG1_READLINK_FAILED		ILIBSTR(\
	"readlink() call failed (%s)")
#define	MSG0_INSTALL_LOG_LOCATION	ILIBSTR(\
	"Installation log 'install_log' location")
#define	MSG1_INSTALL_LOG_BEFORE		ILIBSTR(\
	"%s (before reboot)")
#define	MSG1_INSTALL_LOG_AFTER		ILIBSTR(\
	"%s (after reboot)")
#define	MSG0_MOUNT_POINTS		ILIBSTR(\
	"Mount points table (/etc/vfstab)")

/*
 * svc_updastedisk.c strings
 */

#define	MSG0_DISK_LABEL_FAILED		ILIBSTR(\
	"Could not label disks")
#define	MSG0_DISK_NEWFS_FAILED		ILIBSTR(\
	"Could not check or create system critical file systems")
#define	MSG3_SLICE_CREATE		ILIBSTR(\
	"Creating %s (%ss%d)")
#define	MSG3_SLICE_CREATE_FAILED	ILIBSTR(\
	"File system creation failed for %s (%ss%d)")
#define	MSG3_SLICE_CHECK		ILIBSTR(\
	"Checking %s (%ss%d)")
#define	MSG3_SLICE_CHECK_FAILED		ILIBSTR(\
	"File system check failed for %s (%ss%d)")
#define	MSG0_PROCESS_FOREGROUND		ILIBSTR(\
	"Process running in foreground")
#define	MSG0_PROCESS_BACKGROUND		ILIBSTR(\
	"Process running in background")
#define	MSG0_SLICE2_ACCESS_FAILED	ILIBSTR(\
	"Could not access slice 2 to create Solaris disk label (VTOC)")
#define	MSG0_ALT_SECTOR_SLICE		ILIBSTR(\
	"Processing the alternate sector slice")
#define	MSG0_ALT_SECTOR_SLICE_FAILED	ILIBSTR(\
	"Could not process the alternate sector slice")
#define	MSG0_VTOC_CREATE		ILIBSTR(\
	"Creating Solaris disk label (VTOC)")
#define	MSG0_VTOC_CREATE_FAILED		ILIBSTR(\
	"Could not create Solaris disk label (VTOC)")
#define	MSG4_FDISK_ENTRY		ILIBSTR(\
	"type: %-3d  active:  %-3d  offset: %-6d  size: %-7d")
#define	MSG0_FDISK_OPEN_FAILED		ILIBSTR(\
	"Could not open Fdisk partition table input file")
#define	MSG1_FDISK_TABLE		ILIBSTR(\
	"Fdisk partition table for disk %s (input file for fdisk(1M))")
#define	MSG0_FDISK_CREATE		ILIBSTR(\
	"Creating Fdisk partition table")
#define	MSG0_FDISK_CREATE_FAILED	ILIBSTR(\
	"Could not create Fdisk partition table")
#define	MSG0_FDISK_INPUT_FAILED		ILIBSTR(\
	"Could not create Fdisk partition table input file")
#define	MSG0_CREATE_CHECK_UFS		ILIBSTR(\
	"Creating and checking UFS file systems")
#define	MSG0_VTOC_CREATE		ILIBSTR(\
	"Creating Solaris disk label (VTOC)")
#define	MSG0_DISK_FORMAT		ILIBSTR(\
	"Formatting disk")
#define	MSG1_DISK_FORMAT_FAILED		ILIBSTR(\
	"format(1M) failed (%s)")
#define	MSG1_DISK_SETUP			ILIBSTR(\
	"Configuring disk (%s)")
#define	MSG4_SLICE_VTOC_ENTRY		ILIBSTR(\
	"slice: %2d (%15s)  tag: 0x%-2x  flag: 0x%-2x")

/*
 * svc_updatesoft.c strings
 */

#define	MSG0_TRANS_SETUP_FAILED		ILIBSTR(\
	"Could not initialize transfer list")
#define	MSG0_TRANS_CORRUPT		ILIBSTR(\
	"Transfer list corrupted")
#define	MSG0_SOLARIS_INSTALL_BEGIN	ILIBSTR(\
	"Beginning Solaris software installation")
#define	MSG0_ADMIN_INSTALL_FAILED	ILIBSTR(\
	"Could not install administration information")
#define	MSG0_PKG_PREP_FAILED		ILIBSTR(\
	"Package installation preparation failed")
#define	MSG1_PKG_NONEXISTENT		ILIBSTR(\
	"Non-existent package in cluster (%s)")
#define	MSG0_PKG_INSTALL_INCOMPLETE	ILIBSTR(\
	"Package installation did not complete")
#define	MSG1_PKG_INSTALL_SUCCEEDED	ILIBSTR(\
	"%s software installation succeeded")
#define	MSG0_SOFTINFO_CREATE_FAILED	ILIBSTR(\
	"Could not create the product file")
#define	MSG0_RELEASE_CREATE_FAILED	ILIBSTR(\
	"Could not create the release file")
#define	MSG1_PKG_INSTALL_PARTFAIL	ILIBSTR(\
	"%s software installation partially failed")
#define	PKGS_FULLY_INSTALLED		ILIBSTR(\
	"%s packages fully installed")
#define	PKGS_PART_INSTALLED		ILIBSTR(\
	"%s packages partially installed")
#define	MSG2_LINK_FAILED		ILIBSTR(\
	"Could not link file (%s) to (%s)")
#define	MSG_OPEN_FAILED			ILIBSTR(\
	"Could not open file (%s)")
#define	MSG_READ_FAILED			ILIBSTR(\
	"Could not read file (%s)")
#define	NONE_STRING			ILIBSTR(\
	"none")
#define	MSG0_PKGADD_EXEC_FAILED		ILIBSTR(\
	"pkgadd exec failed")

/*
 * svc_updatesys.c strings
 */
#define	MSG0_SU_SUCCESS				ILIBSTR(\
	"SystemUpdate completed successfully")
#define	MSG0_SU_INVALID_OPERATION		ILIBSTR(\
	"Invalid requested operation supplied to SystemUpdate")
#define	MSG0_SU_MNTPNT_LIST_FAILED		ILIBSTR(\
	"Could not create a list of mount points")
#define	MSG0_SU_SETUP_DISKS_FAILED		ILIBSTR(\
	"Could not update disks with new configuration")
#define	MSG0_SU_STATE_RESET_FAILED		ILIBSTR(\
	"Could not reinitialize system state")
#define	MSG0_SU_MOUNT_FILESYS_FAILED		ILIBSTR(\
	"Could not mount the configured file system(s)")
#define	MSG0_SU_PKG_INSTALL_TOTALFAIL		ILIBSTR(\
	"Could not install all packages. Product installation failed")
#define	MSG0_SU_VFSTAB_CREATE_FAILED		ILIBSTR(\
	"Could not create the file system mount table (/etc/vfstab)")
#define	MSG0_SU_VFSTAB_UNSELECTED_FAILED	ILIBSTR(\
	"Could not create the unselected drive mount point file")
#define	MSG0_SU_HOST_CREATE_FAILED		ILIBSTR(\
	"Could not set up the remote host file (/etc/hosts)")
#define	MSG0_SU_SERIAL_VALIDATE_FAILED		ILIBSTR(\
	"Could not validate the system serial number")
#define	MSG0_SU_SYS_DEVICES_FAILED		ILIBSTR(\
	"Could not set up system devices")
#define	MSG0_SU_CREATE_DIR_FAILED		ILIBSTR(\
	"Could not create a target directory")
#define	MSG0_SU_BOOT_BLOCK_FAILED		ILIBSTR(\
	"Could not install boot blocks")
#define	MSG0_SU_PROM_UPDATE_FAILED		ILIBSTR(\
	"Could not update system for automatic rebooting")
#define	MSG0_SU_UPGRADE_SCRIPT_FAILED		ILIBSTR(\
	"The upgrade script terminated abnormally")
#define	MSG0_SU_DISKLIST_READ_FAILED		ILIBSTR(\
	"Unable to read the disk list from file")
#define	MSG0_SU_DSRAL_CREATE_FAILED		ILIBSTR(\
	"Unable to create an instance of the backup list object.")
#define	MSG0_SU_DSRAL_ARCHIVE_BACKUP_FAILED	ILIBSTR(\
	"Unable to save the backup.")
#define	MSG0_SU_DSRAL_ARCHIVE_RESTORE_FAILED	ILIBSTR(\
	"Unable to restore the backup.")
#define	MSG0_SU_DSRAL_DESTROY_FAILED		ILIBSTR(\
	"Unable to destroy the instance of the backup list object.")
#define	MSG0_SU_UNMOUNT_FAILED			ILIBSTR(\
	"Unable to unmount mounted file systems")
#define	MSG0_SU_FATAL_ERROR			ILIBSTR(\
	"An unrecoverable internal error has occurred.")
#define	MSG0_SU_FILE_COPY_FAILED		ILIBSTR(\
	"Unable to copy a temporary file to it's final location")
#define	MSG0_SU_UNKNOWN_ERROR_CODE		ILIBSTR(\
	"The error code provided is invalid")
#define	MSG0_SU_INITIAL_INSTALL			ILIBSTR(\
	"Preparing system for Solaris install")
#define	MSG0_SU_UPGRADE				ILIBSTR(\
	"Preparing system for Solaris upgrade")
#define	MSG0_SU_FILES_CUSTOMIZE			ILIBSTR(\
	"Customizing system files")
#define	MSG0_SU_INSTALL_CONFIG_FAILED		ILIBSTR(\
	"Could not install system configuration files")
#define	MSG0_SU_DRIVER_INSTALL			ILIBSTR(\
	"Installing unbundled device driver support")
#define	MSG0_SU_MOUNTING_TARGET			ILIBSTR(\
	"Mounting remaining file systems")
#define	MSG0_SU_INITIAL_INSTALL_COMPLETE	ILIBSTR(\
	"Install complete")
#define	MSG0_SU_UPGRADE_COMPLETE		ILIBSTR(\
	"Upgrade complete")

/*
 * svc_vfstab.c strings
 */
#define	MOUNTING_TARGET			ILIBSTR(\
	"Mounting target file systems")
#define	MSG2_FILESYS_MOUNT		ILIBSTR(\
	"Mounting %s (%s)")
#define	MSG1_VFSTAB_ORIG_OPEN		ILIBSTR(\
	"Opening original vfstab file (%s)")

/*
 * svc_dsr_archive_list.c messages
 */

#define	MSG0_DSRAL_SUCCESS			ILIBSTR(\
	"The backup list has been generated for the upgrade.")
#define	MSG0_DSRAL_RECOVERY			ILIBSTR(\
	"A previously interrupted upgrade can be resumed.")
#define	MSG0_DSRAL_CALLBACK_FAILURE		ILIBSTR(\
	"The calling application's callback returned with an error.")
#define	MSG0_DSRAL_PROCESS_FILE_FAILURE		ILIBSTR(\
	"Unable to modify the upgrade's process control file.")
#define	MSG0_DSRAL_MEMORY_ALLOCATION_FAILURE	ILIBSTR(\
	"Unable to allocate dynamic memory.")
#define	MSG0_DSRAL_INVALID_HANDLE		ILIBSTR(\
	"Provided instance handle is invalid.")
#define	MSG0_DSRAL_UPGRADE_CHECK_FAILURE	ILIBSTR(\
	"Unable to determine if a file will be replaced during the upgrade.")
#define	MSG0_DSRAL_INVALID_MEDIA		ILIBSTR(\
	"Invalid media.")
#define	MSG0_DSRAL_NOT_CHAR_DEVICE		ILIBSTR(\
	"Invalid character (raw) device.")
#define	MSG0_DSRAL_UNABLE_TO_WRITE_MEDIA	ILIBSTR(\
	"Unable to write to specified media. Make sure the "\
	"media is loaded and not write protected.")
#define	MSG0_DSRAL_UNABLE_TO_STAT_PATH		ILIBSTR(\
	"Unable to stat media. Make sure the media path is valid.")
#define	MSG0_DSRAL_CANNOT_RSH			ILIBSTR(\
	"Unable to open a remote shell on the system specified "\
	"in the media path. Make sure the system being upgraded "\
	"has .rhosts permissions on the specified system.")
#define	MSG0_DSRAL_UNABLE_TO_OPEN_DIRECTORY	ILIBSTR(\
	"Unable to open a directory that is being backed up.")
#define	MSG0_DSRAL_INVALID_PERMISSIONS		ILIBSTR(\
	"Invalid file permissions. The media must have "\
	"read/write access for other.")
#define	MSG0_DSRAL_INVALID_DISK_PATH		ILIBSTR(\
	"Invalid directory or block device.")
#define	MSG0_DSRAL_DISK_NOT_FIXED		ILIBSTR(\
	"The media cannot be used for the "\
	"backup because it is being changed or moved during "\
	"the upgrade.")
#define	MSG0_DSRAL_UNABLE_TO_MOUNT		ILIBSTR(\
	"Unable to mount the media.")
#define	MSG0_DSRAL_NO_MACHINE_NAME		ILIBSTR(\
	"The media path requires a system name.")
#define	MSG0_DSRAL_ITEM_NOT_FOUND		ILIBSTR(\
	"The requested item was not found in the list of installed services.")
#define	MSG0_DSRAL_CHILD_PROCESS_FAILURE	ILIBSTR(\
	"An error occurred managing the archiving process.")
#define	MSG0_DSRAL_LIST_MANAGEMENT_ERROR	ILIBSTR(\
	"An internal error occurred in the list management functions.")
#define	MSG0_DSRAL_INSUFFICIENT_MEDIA_SPACE	ILIBSTR(\
	"The media has insufficient space for the backup.")
#define	MSG0_DSRAL_SYSTEM_CALL_FAILURE		ILIBSTR(\
	"An internal system call returned a failure.")
#define	MSG0_DSRAL_INVALID_FILE_TYPE		ILIBSTR(\
	"An unrecognized file type has been encountered on the system.")
#define	MSG0_DSRAL_INVALID_ERROR_CODE		ILIBSTR(\
	"The provided error code is invalid for the upgrade object.")
