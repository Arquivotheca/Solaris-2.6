#ifndef lint
#pragma ident "@(#)ibe_strings.h 1.22 95/02/17"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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
#ifndef _IBE_STRINGS_H
#define _IBE_STRINGS_H
#endif

#define	MSG0_STATE_RESET_FAILED		ILIBSTR(\
	"Could not reinitialize system state")
#define	MSG0_DISK_LABEL_FAILED		ILIBSTR("Could not label disks")
#define	MSG0_DISK_NEWFS_FAILED		ILIBSTR(\
	"Could not check or create system critical file systems")
#define	MSG0_MNTPNT_LIST_FAILED		ILIBSTR(\
	"Could not create a list of mount points")
#define	MSG0_SYSTEM_PREP_BEGIN		ILIBSTR(\
	"Preparing system for Solaris")
#define	MSG0_SOLARIS_INSTALL_BEGIN	ILIBSTR(\
	"Beginning Solaris software installation")
#define	MSG0_SYSTEM_INSTALL_COMPLETE	ILIBSTR(\
	"System installation and configuration is completed")
#define	MSG0_PKG_INSTALL_TOTALFAIL	ILIBSTR(\
	"Could not install all packages. Product installation failed")
#define	MSG0_ADMIN_INSTALL_FAILED	ILIBSTR(\
	"Could not install administration information")
#define	MSG0_FILES_CUSTOMIZE		ILIBSTR("Customizing system files")
#define	MSG0_VFSTAB_CREATE_FAILED	ILIBSTR(\
	"Could not create the file system mount table (/etc/vfstab)")
#define	MSG0_HOST_CREATE_FAILED		ILIBSTR(\
	"Could not set up the remote host file (/etc/hosts)")
#define	MSG0_SERIAL_VALIDATE_FAILED	ILIBSTR(\
	"Could not validate the system serial number")
#define	MSG0_INSTALL_CONFIG_FAILED	ILIBSTR(\
	"Could not install system configuration files")
#define	MSG0_SYS_DEVICES_FAILED		ILIBSTR("Could not set up system devices")
#define	MSG0_BOOT_BLOCK_FAILED		ILIBSTR("Could not install boot blocks")
#define	MSG0_BOOT_BLOCK_NOTEXIST	ILIBSTR("No boot block found")
#define	MSG0_PBOOT_NOTEXIST		ILIBSTR("No pboot file found")
#define	MSG0_DRIVER_INSTALL		ILIBSTR(\
	"Installing unbundled device driver support")
#define	MSG0_INSTALL_LOG_LOCATION	ILIBSTR(\
	"Installation log 'install_log' location")
#define	MSG1_INSTALL_LOG_BEFORE		ILIBSTR("%s (before reboot)")
#define	MSG1_INSTALL_LOG_AFTER		ILIBSTR("%s (after reboot)")
#define	MSG0_PKG_PREP_FAILED		ILIBSTR(\
	"Package installation preparation failed")
#define	MSG0_PKG_CLEANUP_FAILED		ILIBSTR(\
	"Package installation cleanup failed")
#define	SYNC_WRITE_SET_FAILED		ILIBSTR(\
	"Could not access %s to set synchronous writes")
#define	MSG1_PKG_NONEXISTENT		ILIBSTR(\
	"Non-existent package in cluster (%s)")
#define	MSG0_PKG_INSTALL_INCOMPLETE	ILIBSTR(\
	"Package installation did not complete")
#define	MSG0_SOFTINFO_CREATE_FAILED	ILIBSTR(\
	"Could not create the product file")
#define	MSG0_RELEASE_CREATE_FAILED	ILIBSTR(\
	"Could not create the release file")
#define	MSG1_PKG_INSTALL_SUCCEEDED	ILIBSTR(\
	"%s software installation succeeded")
#define	MSG1_PKG_INSTALL_PARTFAIL	ILIBSTR(\
	"%s software installation partially failed")
#define	MSG1_DEV_INSTALL_FAILED		ILIBSTR(\
	"Could not install devices (%s)")

#define	MSG0_REBOOT_MESSAGE		ILIBSTR(\
	"The system will not automatically reconfigure devices upon reboot."\
	" You must use 'boot -r' when booting the system.")
#define	VFSTAB_COMMENT_LINE1		ILIBSTR( \
	"# This file contains vfstab entries for file systems on disks which\n")
#define	VFSTAB_COMMENT_LINE2		ILIBSTR( \
	"# were not selected during installation. The system administrator\n") 
#define	VFSTAB_COMMENT_LINE3		ILIBSTR( \
	"# should put the entries which are intended to be active in the\n")
#define	VFSTAB_COMMENT_LINE4		ILIBSTR( \
	"# /etc/vfstab file, and create corresponding mount points.\n")

#define	MSG0_VFSTAB_UNSELECTED		ILIBSTR(\
	"Unselected drive mount points (/var/sadm/system/data/vfstab.unselected)")
#define	MSG0_VFSTAB_UNSELECTED_FAILED	ILIBSTR(\
	"Could not create the unselected drive mount point file")

#define	MSG0_MOUNT_POINTS		ILIBSTR("Mount points table (/etc/vfstab)")
#define	MSG0_VFSTAB_INSTALL_FAILED	ILIBSTR("Could not install new vfstab data")
#define	MSG0_BOOT_DISK_UNSELECT		ILIBSTR("The boot disk is not selected")
#define	MSG0_BOOT_INFO_INSTALL		ILIBSTR("Installing boot information")
#define	MSG0_ROOT_UNSELECTED		ILIBSTR(\
	"The / mount point is not on a selected disk")
#define	MSG1_BOOT_BLOCKS_INSTALL	ILIBSTR("Installing boot blocks (%s)")
#define	MSG0_INSTALLBOOT_FAILED		ILIBSTR("installboot(1M) failed")
#define	MSG1_READLINK_FAILED		ILIBSTR("readlink() call failed (%s)")
#define	MSG0_BOOTRC_INSTALL		ILIBSTR("Installing boot startup script (/etc/bootrc)")
#define	PKGS_PART_INSTALLED		ILIBSTR("%s packages partially installed")
#define	PKGS_FULLY_INSTALLED		ILIBSTR("%s packages fully installed")
#define	MSG1_VFSTAB_ORIG_OPEN		ILIBSTR("Opening original vfstab file (%s)")

#define	MSG0_DEVICES_CUSTOMIZE		ILIBSTR("Customizing system devices")
#define	MSG0_DEVICES_LOGICAL		ILIBSTR("Logical devices (/dev)")
#define	MSG0_DEVICES_PHYSICAL		ILIBSTR("Physical devices (/devices)")
#define	MSG1_DIR_ACCESS_FAILED		ILIBSTR("Could not access directory (%s)")
#define	MSG1_FILE_ACCESS_FAILED		ILIBSTR("Could not access file (%s)")
#define	MOUNTING_TARGET			ILIBSTR("Mounting target file systems")
#define	MSG0_MOUNTING_TARGET		ILIBSTR("Mounting remaining file systems")
#define	MSG0_HOST_ADDRESS		ILIBSTR("Network host addresses (/etc/hosts)")
#define	MSG_LEADER_ERROR		ILIBSTR("ERROR: ")
#define	MSG_LEADER_WARNING		ILIBSTR("WARNING: ")
#define	NONE_STRING			ILIBSTR("none")
#define	CREATING_MNTPNT			ILIBSTR("Creating mount point (%s)")
#define	CREATE_MNTPNT_FAILED		ILIBSTR("Could not create mount point (%s)")
#define	MSG2_FILESYS_MOUNT		ILIBSTR("Mounting %s (%s)")
#define	MSG2_FILESYS_MOUNT_FAILED	ILIBSTR("Could not mount %s (%s)")
#define	MSG_OPEN_FAILED			ILIBSTR("Could not open file (%s)")
#define	MSG_COPY_FAILED			ILIBSTR("Could not copy file (%s) to (%s)")
#define	MSG_READ_FAILED			ILIBSTR("Could not read file (%s)")
#define	MSG2_LINK_FAILED		ILIBSTR("Could not link file (%s) to (%s)")
#define	MSG1_TRANS_ATTRIB_FAILED	ILIBSTR("Could not set file attributes (%s)")
#define	MSG0_TRANS_SETUP_FAILED		ILIBSTR("Could not initialize transfer list")
#define	MSG0_TRANS_CORRUPT		ILIBSTR("Transfer list corrupted")
#define MSG3_SLICE_CREATE               ILIBSTR("Creating %s (%ss%d)")
#define MSG3_SLICE_CREATE_FAILED        ILIBSTR(\
	"File system creation failed for %s (%ss%d)")
#define MSG3_SLICE_CHECK                ILIBSTR("Checking %s (%ss%d)")
#define MSG3_SLICE_CHECK_FAILED         ILIBSTR(\
	"File system check failed for %s (%ss%d)")
#define MSG0_ALT_SECTOR_SLICE           ILIBSTR(\
	"Processing the alternate sector slice")
#define MSG0_ALT_SECTOR_SLICE_FAILED    ILIBSTR(\
	"Could not process the alternate sector slice")
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
#define	MSG0_SLICE2_ACCESS_FAILED	ILIBSTR(\
	"Could not access slice 2 to create Solaris disk label (VTOC)")
#define	MSG0_VTOC_CREATE		ILIBSTR(\
	"Creating Solaris disk label (VTOC)")
#define	MSG0_VTOC_CREATE_FAILED		ILIBSTR(\
	"Could not create Solaris disk label (VTOC)")
#define	MSG0_DISK_FORMAT		ILIBSTR("Formatting disk")
#define	MSG1_DISK_FORMAT_FAILED		ILIBSTR("format(1M) failed (%s)")
#define	MSG1_DISK_SETUP			ILIBSTR("Configuring disk (%s)")
#define	MSG4_SLICE_VTOC_ENTRY		ILIBSTR(\
	"slice: %2d (%15s)  tag: 0x%-2x  flag: 0x%-2x")
#define	MSG0_PROCESS_FOREGROUND		ILIBSTR(\
	"Process running in foreground")
#define	MSG0_PROCESS_BACKGROUND		ILIBSTR(\
	"Process running in background")
