#ifndef lint
#pragma ident "@(#)pf_strings.h 2.43 96/10/10"
#endif
/*
 * Copyright (c) 1994-1996 Sun Microsystems, Inc. All Rights Reserved.
 */

#ifndef _PF_STRINGS_H
#define	_PF_STRINGS_H

#include <libintl.h>

/* standard text domain for messages file */
#ifndef TEXT_DOMAIN
#define	TEXT_DOMAIN    "SUNW_INSTALL_PFINSTALL"
#endif

#ifdef lint
#define	gettext(x)	x
#endif

/*
 * Messages displayed during pfinstall
 */
#define	MSG1_USAGE			gettext(\
	"Usage: %s [-D] [-d <disk_configuration_file>]\n"\
	"          [-c <cdrom_base_directory>] profile")
#define	MSG1_EXIT_STATUS		gettext(\
	"Test run complete. Exit status %d.")
#define	MSG0_PROFILE_PARSE		gettext("Parsing profile")
#define	MSG0_PROFILE_PROCESS		gettext("Processing profile")
#define	MSG1_PROFILE_OPEN_FAILED	gettext(\
	"Could not open profile (%s)")
#define	MSG0_RESOURCE_INIT_FAILED	gettext(\
	"Could not initialize default resources")
#define	MSG1_DISKFILE_FAILED		gettext(\
	"The disk configuration file is invalid or non-existent (%s)")
#define	MSG0_DISKS_NONE			gettext(\
	"No disks found")
#define	MSG0_DISKS_CHECK_POWER		gettext(\
	"Check to make sure disks are cabled and powered up")
#define	MSG0_DISKS_NONE_USABLE		gettext(\
	"One or more disks are found, but one of "\
	"the following problems exists:")
#define	MSG0_DISKS_UNUSABLE_PROB1	gettext(\
	"Hardware failure")
#define	MSG0_DISKS_UNUSABLE_PROB2	gettext(\
	"Unformatted disk")
#define	MSG0_DISKS_UNUSABLE_PROB3	gettext(\
	"fdisk partitioning (PowerPC systems only)")
#define	MSG0_DISKS_UNUSABLE_POWERPC1	gettext(\
	"PowerPC systems require three unused fdisk partitions")
#define	MSG0_DISKS_UNUSABLE_POWERPC2	gettext(\
	"and at least 40 Mbytes of contiguous disk space")
#define	MSG0_INSTALL_FAILED		gettext(\
	"System installation failed")
#define	MSG0_MEDIA_NONE			gettext("No media")
#define	MSG1_MEDIA_INVALID		gettext(\
	"Media is invalid for system installation (%s)")
#define	MSG1_MEDIA_UNMOUNT		gettext("Media is not mounted (%s)")
#define	MSG1_MEDIA_NOPRODUCT		gettext(\
	"There is no valid Solaris product on the media (%s)")
#define	MSG1_MEDIA_LOAD_FAILED		gettext("Could not load the media (%s)")
#define	MSG1_SIGNAL_RECEIVED		gettext("Fatal signal received (%d)")
#define	MSG0_DISK_SELECT_ALL		gettext("Selecting all disks")
#define	MSG1_DISK_SELECT_FAILED		gettext("Could not select disk (%s)")
#define	MSG1_DISK_DESELECT		gettext("Deselecting disk (%s)")
#define	MSG1_DISK_DESELECT_FAILED	gettext("Could not deselect disk (%s)")
#define	MSG0_DISK_LAYOUT_SELECTED	gettext(\
	"Disk layout for selected disks")
#define	MSG0_DISK_LAYOUT_UNSELECTED	gettext(\
	"Disk layout for unselected disks")
#define	MSG1_DISK_DESELECT_UNMOD	gettext(\
	"Deselecting unmodified disk (%s)")
#define	MSG1_DISK_DESELECT_UNMOD_FAILED	gettext(\
	"Could not deselect unmodified disk to avoid VTOC overwrite (%s)")
#define	MSG1_DISK_FREE_SPACE_FAILED	gettext(\
	"Could not allocate residual unused disk space (%s)")
#define	MSG1_DISK_BOOT_SELECT_FAILED	gettext(\
	"Could not select boot disk (%s)")
#define	MSG0_DISK_BOOT_SPEC_FAILED	gettext(\
	"Could not specify the boot disk")
#define	MSG0_DISK_BOOT_NO_DISKS		gettext(\
	"No disks available for boot disk")
#define	MSG1_DISK_ROOTDEVICE		gettext(\
	"Using disk (%s) for \"rootdisk\"")
#define	MSG1_DISK_NOT_SELECTED		gettext(\
	"Disk is not selected (%s)")
#define	MSG0_NO_DISKS_SELECTED		gettext(\
	"There are no selected disks available for software configuration")
#define	MSG1_DISK_BOOT_NOT_SELECTED	gettext(\
	"The boot disk (%s) is not selected")
#define	MSG0_BOOT_CONFIGURE		gettext(\
	"Configuring boot device")
#define	MSG1_SPACE_CHECKING_PERCENT_COMPLETE		gettext(\
	"Checking file system space: %d%% completed")
#define	MSG1_UPGRADE_SCRIPT_PERCENT_COMPLETE		gettext(\
	"Upgrading Solaris: %d%% complete")
#define	MSG0_SOFTWARE_UPDATE_BEGIN		gettext(\
	"Starting software installation")
#define	MSG0_SOFTWARE_UPDATE_END		gettext(\
	"Completed software installation")
#define	MSG0_SOFTWARE_UPDATE_INTERACTIVE_PKGADD	gettext(\
	"Adding the %s package requires user interaction.  \
Installation cannot continue.")
#define	MSG0_SOFTWARE_UPDATE_INTERACTIVE_PKGRM	gettext(\
	"Removing the %s package requires user interaction.  \
Installation cannot continue.")

/*
 * message notifying user of possible size difference in
 * the '/' file system between live and disk configuration
 * runs
 */
#define MSG0_DEVSIZE_DIFFERENCE_0       	gettext(\
        "NOTE: The actual space required to hold the '/' file system may")
#define MSG0_DEVSIZE_DIFFERENCE_1       	gettext(\
        "be underestimated due to the use of a disk configuration file.")

/*
 * Messages specific to Upgrade
 */
#define	MSG0_UNABLE_TO_FORK_CHILD		gettext(\
	"Unable to start child process")

/*
 * Messages specific to DSR
 */

#define	MSG0_AUTO_LAYOUT_FAILED			gettext(\
	"Autolayout was unsuccessful at generating an acceptable \
file system layout")
#define	MSG2_INVALID_LAYOUT_CONSTRAINT		gettext(\
	"The %s layout constraint is not valid for %s")
#define	MSG2_INVALID_LAYOUT_CONSTRAINT_SIZE	gettext(\
	"Layout Constraint size for %s is less than minimum \
required size of %lu KB")
#define	MSG1_MUST_PROVIDE_LAYOUT_CONSTRAINT	gettext(\
	"No layout constraint provided for the failed file system %s")
#define	MSG0_NO_BACKUP_DEVICE			gettext(\
	"Backup media not specified.  A backup media \
(backup_media) keyword must be specified if an upgrade with disk \
space reallocation is required")
#define	MSG0_DSR_ARCHIVE_LIST_ERROR		gettext(\
	"Generation of backup list failed")
#define	MSG0_LIST_MANAGEMENT_ERROR		gettext(\
	"Linked list management failed")

#define	MSG0_TAPE_MEDIA				gettext(\
	"Tape")
#define	MSG0_DISKETTE_MEDIA			gettext(\
	"Diskette")
#define	MSG1_INSERT_FIRST_MEDIA			gettext(\
	"You must now load the first %s.  Please, ensure that \
it is not write protected and then load it and press return.")
#define	MSG2_REPLACE_MEDIA			gettext(\
	"Please insert %s number %d and press return")
#define	MSG0_VALIDATING_MEDIA			gettext(\
	"Validating the selected media")

#define	MSG0_GENERATE_BEGIN			gettext(\
	"")
#define	MSG1_GENERATE_PERCENT_COMPLETE 		gettext(\
	"Generating backup list: %u%% completed")
#define	MSG0_GENERATE_END			gettext(\
	"")

#define	MSG1_BACKUP_DISKETTE_BEGIN		gettext(\
	"Starting backup: Media = Local diskette, Path = %s")
#define	MSG1_BACKUP_TAPE_BEGIN			gettext(\
	"Starting backup: Media = Local tape, Path = %s")
#define	MSG1_BACKUP_DISK_BEGIN			gettext(\
	"Starting backup: Media = Local file system, Path = %s")
#define	MSG1_BACKUP_RSH_BEGIN			gettext(\
	"Starting backup: Media = Remote system (rsh), Path = %s")
#define	MSG1_BACKUP_NFS_BEGIN			gettext(\
	"Starting backup: Media = Remote file system (NFS), Path = %s")
#define	MSG1_BACKUP_PERCENT_COMPLETE 		gettext(\
	"Backing up file systems: %u%% completed")
#define	MSG0_BACKUP_END				gettext(\
	"Completed Backup")

#define	MSG1_RESTORE_DISKETTE_BEGIN		gettext(\
	"Starting retstore: Media = Local diskette, Path = %s")
#define	MSG1_RESTORE_TAPE_BEGIN			gettext(\
	"Starting retstore: Media = Local tape, Path = %s")
#define	MSG1_RESTORE_DISK_BEGIN			gettext(\
	"Starting retstore: Media = Local file system, Path = %s")
#define	MSG1_RESTORE_RSH_BEGIN			gettext(\
	"Starting retstore: Media = Remote system (rsh), Path = %s")
#define	MSG1_RESTORE_NFS_BEGIN			gettext(\
	"Starting retstore: Media = Remote file system (NFS), Path = %s")
#define	MSG1_RESTORE_PERCENT_COMPLETE 		gettext(\
	"Restoring file systems: %u%% completed")
#define	MSG0_RESTORE_END			gettext(\
	"Completed restore")


/*
 * general parsing messages
 */
#define	MSG0_SYSTYPE_DUPLICATE_IGNORED	gettext(\
	"Extra system type entry ignored")
#define	MSG0_SYNTAX_TOOMANY_FIELDS	gettext("Entry has too many fields")
#define	MSG0_NO_OPTYPE			gettext(\
	"No install_type specified in profile")
#define	MSG1_SYNTAX_ERROR		 gettext(\
	"Syntax error: unknown value (%s)")
#define	MSG1_MISSING_PARAMS		gettext(\
	"Required parameters missing (%s)")

/*
 * field 1 parsing messages
 */
#define	MSG0_FIELD1			gettext("Field 1")
#define	MSG1_FIELD1			MSG0_FIELD1
#define	MSG1_FIELD2			gettext("Field 2")
#define	MSG0_KEYWORD_NONE		gettext("No keyword")
#define	MSG1_KEYWORD_DUPLICATED		gettext(\
	"Illegal duplicate profile keyword (%s)")
#define	MSG1_KEYWORD_INVALID		gettext("Keyword \"%s\" is invalid")
#define	MSG1_KEYWORD_INVALID_OPTYPE	gettext(\
	"Keyword \"%s\" is not supported for the specified install_type")

#define	MSG0_ONLY_SLICE_ZERO		gettext(\
	"The device can only be specified for slice \"0\" on this system")
#define	MSG0_USE_DONTUSE_EXCLUSIVE	gettext(\
	"The \"usedisk\" and \"dontuse\" keywords are mutually exclusive")
#define	MSG0_INSTALL_TYPE_FIRST		gettext(\
	"First profile keyword must be \"install_type\"")
#define	MSG0_KEYWORD_FDISK_INVALID	gettext(\
	"The \"fdisk\" keyword is not supported on this system")

/*
 * field 2 parsing messages
 */
#define	MSG0_FIELD2			gettext("Field 2")
#define	MSG0_VALUE_NONE			gettext("No keyword value")
#define	MSG1_SYSTEMTYPE_INVALID		gettext("Invalid system type (%s)")
#define	MSG1_INSTALLTYPE_INVALID	gettext("Invalid install type (%s)")
#define	MSG1_PARTITIONING_INVALID	gettext(\
	"Invalid partitioning type (%s)")
#define	MSG1_ROOTDEVICE_SLICE_INVALID	gettext("Invalid slice (%s)")
#define	MSG0_ROOTDEVICE_SLICE_NONE	gettext(\
	"Must specify slice with \"rootdisk\" keyword")
#define	MSG1_SLICE_FIXED		gettext(\
	"Slice cannot be modified (%s)")
#define	MSG1_SLICE_DUPLICATE		gettext(\
	"Slice is already specified (%s)")
#define	MSG1_DISK_INVALID		gettext(\
	"Disk is not valid on this system (%s)")
#define	MSG1_DEVICE_INVALID		gettext(\
	"Device is not valid on this system (%s)")
#define	MSG0_DISKNAME_NONE		gettext("No disk name")
#define	MSG1_DISKNAME_INVALID		gettext("Invalid disk name (%s)")
#define	MSG1_MOUNTPNT_REMOTE_INVALID	gettext(\
	"Invalid remote mount point (%s)")
#define	MSG0_HOST_NAME_NONE		gettext("No host name")
#define	MSG1_CLIENTNUM_INVALID		gettext("Invalid client number (%s)")
#define	MSG1_CLIENTROOT_INVALID		gettext("Invalid client root size (%s)")
#define	MSG1_CLIENTSWAP_INVALID		gettext("Invalid client swap size (%s)")
#define	MSG1_CLIENTARCH_INVALID		gettext(\
	"Invalid client platform for Solaris CD image (%s)")
#define	MSG0_DEFAULT_LOCALES		gettext("Processing default locales")
#define	MSG1_LOCALE_DEFAULT		gettext(\
	"Specifying default locale (%s)")
#define	MSG1_LOCALE_DEFAULT_INVALID	gettext("Invalid default locale (%s)")
#define	MSG1_LOCALE_INVALID		gettext(\
	"Invalid locale for Solaris CD image (%s)")
#define	MSG1_LOCALE_DUPLICATE		gettext(\
	"Locale is already selected (%s)")
#define	MSG1_INVALID_DEVICE_NAME	gettext(\
	"Invalid device name (%s)")
#define	MSG1_INVALID_BAKDEV_TYPE	gettext(\
	"Invalid backup_device type (%s)")
#define	MSG1_INVALID_RESERVE_SPACE	gettext(\
	"Invalid reserve space size (%s)")

/*
 * field 3 parsing messages
 */
#define	MSG0_FIELD3			gettext("Field 3")
#define	MSG1_FIELD3			MSG0_FIELD3
#define	MSG0_SIZE_NONE			gettext("No size")
#define	MSG1_SIZE_INVALID		gettext("Invalid size (%s)")
#define	MSG1_SIZE_INVALID_ANY		gettext(\
	"Invalid size for device type \"any\" (%s)")
#define	MSG0_PARTTYPE_NONE		gettext("No fdisk partition value")
#define	MSG1_PARTTYPE_INVALID		gettext(\
	"Invalid fdisk partition value (%s)")
#define	MSG0_IPADDR_NONE		gettext("No IP address")
#define	MSG1_IPADDR_INVALID		gettext("Invalid IP address (%s)")
#define	MSG1_IPADDR_UNKNOWN		gettext(\
	"Unknown IP address for host (%s)")
#define	MSG1_INVALID_SLICE_MODE		gettext(\
	"Invalid slice attribute (%s)")

/*
 * field 4 parsing messages
 */
#define	MSG0_FIELD4			gettext("Field 4")
#define	MSG0_MOUNTPNT_NONE		gettext("No local mount point")
#define	MSG1_MOUNTPNT_INVALID		gettext("Invalid mount point (%s)")
#define	MSG1_MOUNTPNT_INVALID_ANY	gettext(\
	"Invalid mount point for device type \"any\" (%s)")
#define	MSG1_MOUNTPNT_INVALID_FREE	gettext(\
	"Invalid mount point for size \"free\" (%s)")
#define	MSG1_MOUNTPNT_INVALID_AUTO	gettext(\
	"Invalid mount point name for size \"auto\" (%s)")
#define	MSG0_PARTITIONING_EXISTING_SIZE	gettext(\
	"Existing partitioning requires a size of \"existing\"")
#define	MSG0_MOUNTPNT_INVALID_IGNORE	gettext(\
	"\"ignore\" mount point requires size of \"existing\"")
#define	MSG0_SLICE_TOOSMALL		gettext(\
	"Slices must be at least 10 Mbytes")
#define	MSG0_SIZE_ALL_INVALID		gettext(\
	"Size \"all\" only allowed with Solaris fdisk partitions")
#define	MSG0_SIZE_SOLARIS_TOOSMALL	gettext(\
	"Solaris fdisk partitions must be at least 30 Mbytes")
#define	MSG1_PART_CREATE_PROHIBITED	gettext(\
	"Cannot create fdisk partition type (%s)")

/*
 * field 5 and 6 parsing messages
 */
#define	MSG0_FIELD5			gettext("Field 5")
#define	MSG0_FIELD6			gettext("Field 6")
#define	MSG0_PRES_EXISTING		gettext(\
	"Size must be \"existing\" to preserve")
#define	MSG0_PRES_IGNORE		gettext(\
	"Cannot preserve \"ignore\" mount point")
#define	MSG0_MOUNTOPT_IGNORE		gettext(\
	"Cannot have mount options with \"ignore\" mount point")
#define	MSG1_MOUNTOPT_INVALID		gettext(\
	"Invalid optional parameter (%s)")

/*
 * verification strings
 */
#define	MSG0_VERIFY_CACHEOS_SW		gettext(\
	"This system type does not support software keywords")
#define	MSG0_VERIFY_CACHEOS_PARTITIONING	gettext(\
	"This system type does not support existing partitioning")
#define	MSG0_VERIFY_NOTSERVER		gettext(\
	"This system type does not support client keywords")
#define	MSG0_VERIFY_DISK_CONFIG		gettext("Verifying disk configuration")
#define	MSG0_VERIFY_SPACE_CONFIG	gettext("Verifying space allocation")
#define	MSG0_AUTOCLIENT_UPGRADE_INVALID	gettext(\
	"Cannot upgrade an AutoClient system")

/*
 * software messages
 */
#define	MSG1_CLUSTER_SELECT		gettext("Selecting cluster (%s)")
#define	MSG1_CLUSTER_UNKNOWN		gettext("Unknown cluster ignored (%s)")
#define	MSG1_CLUSTER_DESELECT		gettext("Deselecting cluster (%s)")
#define	MSG1_PACKAGE_UNKNOWN		gettext("Unknown package ignored (%s)")
#define	MSG1_PACKAGE_SELECT		gettext("Selecting package (%s)")
#define	MSG1_PACKAGE_SELECT_FAILED	gettext("Could not select package (%s)")
#define	MSG1_PACKAGE_SELECT_SELECT	gettext(\
	"Selected package is already selected (%s)")
#define	MSG1_PACKAGE_DESELECT		gettext("Deselecting package (%s)")
#define	MSG1_PACKAGE_DESELECT_FAILED	gettext(\
	"Could not deselect package (%s)")
#define	MSG1_PACKAGE_DESELECT_REQD	gettext(\
	"Cannot deselect required package (%s)")
#define	MSG1_PACKAGE_DESELECT_DESELECT	gettext(\
	"Deselected package is already deselected (%s)")
#define	MSG2_PACKAGE_DEPEND		gettext(\
	"%s depends on %s, which is not selected")
#define	MSG1_LOCALE_SELECT		gettext("Selecting locale (%s)")
#define	MSG1_LOCALE_SELECT_FAILED	gettext("Could not select locale (%s)")
#define	MSG1_SOFTWARE_END_PKGADD	gettext(\
	"done.  %6.2f Mbytes remaining.")
#define	MSG1_SOFTWARE_META_DUPLICATE	gettext("Duplicate software group (%s)")
#define	MSG1_SOFTWARE_TOTAL		gettext(\
	"Total software size:  %6.2f Mbytes")
#define	MSG1_PLATFORM_SELECT_FAILED	gettext(\
	"Could not select client platform (%s)")

/*
 * fdisk and partition related messages
 */
#define	MSG1_FDISK_INVALID		gettext(\
	"fdisk partition table invalid (%s)")
#define	MSG2_PART_PRESERVE_FAILED	gettext(\
	"Could not preserve %s fdisk partition (%s)")
#define	MSG1_PART_SOLARIS_MIN		gettext(\
	"At least one 30 Mbyte %s fdisk partition "\
	"is required on a selected drive")
#define	MSG2_PART_ACTIVE		gettext(\
	"Setting %s fdisk partition to \"active\" (%s)")
#define	MSG2_PART_ACTIVE_FAILED		gettext(\
	"Could not set %s fdisk partition to \"active\" (%s)")
#define	MSG2_PART_CREATE		gettext(\
	"Creating %s fdisk partition (%s)")
#define	MSG2_PART_CREATE_FAILED		gettext(\
	"Could not create %s fdisk partition (%s)")
#define	MSG2_PART_CREATE_DUPLICATE	gettext(\
	"Cannot create duplicate %s fdisk partitions (%s)")
#define	MSG2_PART_NO_FREE_SPACE		gettext(\
	"Not enough free space to create %s fdisk partition (%s)")
#define	MSG2_PART_NO_FREE_PART		gettext(\
	"No free fdisk partitions to create \"maxfree\" %s partition (%s)")
#define	MSG2_PART_DELETE		gettext(\
	"Deleting %s fdisk partition (%s)")
#define	MSG2_PART_DELETE_FAILED		gettext(\
	"Could not delete %s fdisk partition (%s)")
#define	MSG2_PART_DELETE_NONE		gettext(\
	"No existing %s fdisk partition(s) to delete (%s)")
#define	MSG2_PART_ALL			gettext(\
	"Creating \"all\" %s fdisk partition (%s)")
#define	MSG2_PART_ALL_FAILED		gettext(\
	"Could not create \"all\" %s fdisk partition (%s)")
#define	MSG2_PART_MAXFREE		gettext(\
	"Creating \"maxfree\" %s fdisk partition (%s)")
#define	MSG2_PART_MAXFREE_EXISTS	gettext(\
	"Using existing %s fdisk partition (%s)")
#define	MSG2_PART_MAXFREE_FAILED	gettext(\
	"Could not create \"maxfree\" %s fdisk partition (%s)")
#define	MSG2_DISK_DESELECT_NO_PART	gettext(\
	"Deselecting disk with no %s fdisk partition (%s)")

/*
 * rootdisk related messages
 */
#define	MSG1_ROOTDEVICE_INVALID		gettext(\
	"\"root_device\" (%s) is not valid on this system")
#define	MSG2_ROOTDEV_BOOTDEV_CONFLICT	gettext(\
	"\"root_device\" (%s) and \"boot_device\" (%s) conflict")
#define	MSG2_ROOTDEV_FILESYS_CONFLICT	gettext(\
	"\"root_device\" (%s) and \"filesys\" (%s) conflict")
#define	MSG1_ROOTDEV_PARTITIONING_CONFLICT	gettext(\
	"\"root_device\" (%s) conflicts with existing partitioning")

#define	MSG1_BOOTDEVICE_INVALID		gettext(\
	"\"boot_device\" (%s) is not valid on this system")
#define	MSG2_BOOTDEV_FILESYS_CONFLICT	gettext(\
	"\"boot_device\" (%s) and \"filesys\" (%s) conflict")
#define	MSG1_BOOTDEV_PARTITIONING_CONFLICT	gettext(\
	"\"boot_device\" (%s) conflicts with existing partitioning")
#define	MSG1_FILESYS_PARTITIONING_CONFLICT	gettext(\
	"\"filesys\" (%s) conflicts with existing partitioning")
#define	MSG0_EXISTING_NO_ROOT		gettext(\
	"There are no \"/\" file systems in the existing configuration")
#define	MSG0_EXISTING_MULTIPLE_ROOT	gettext(\
	"There are multiple \"/\" file systems in the existing configuration")

/*
 * instructional messages
 */
#define	MSG2_RECONFIG_PROM_INTRO	gettext(\
	"WARNING: The system firmware is currently not configured to\n" \
	"         automatically reboot using the newly installed Solaris\n"\
	"         release. You must manually reconfigure the %s to\n" \
	"         boot from %s automatically.")
#define	MSG0_PROM_BIOS			gettext("BIOS")
#define	MSG0_PROM_EEPROM		gettext("EEPROM")
#define	MSG0_PROM_FIRMWARE		gettext("firmware")

#define	MSG0_NOT_UPDATABLE		gettext(\
	"The firmware on this machine cannot be set for automatic rebooting")

/*
 * standard messages
 */
#define	MSG_STD_YES			gettext("yes")
#define	MSG_STD_NO			gettext("no")
#define	MSG_STD_OUTOFMEM		gettext("Out of virtual memory")
#define	MSG_STD_ERROR			gettext("ERROR: ")
#define	MSG_STD_WARNING			gettext("WARNING: ")

/*
 * trace messages
 */
#define	MSG1_TRACE_CACHEOS_SWAP		gettext(\
	"/.cache/swap swap file %d MB")

/*
 * table messages
 */
#define	MSG0_DISK_TABLE_DISK		gettext("Disk")
#define	MSG0_DISK_TABLE_HEADER		gettext("Solaris Slice Table")
#define	MSG3_DISK_TABLE_STATISTICS	gettext(\
	"  Usable space: %d MB (%d cylinders)   Free space: %d MB")
#define	MSG0_DISK_TABLE_SLICE		gettext("Slice")
#define	MSG0_DISK_TABLE_START		gettext("Start")
#define	MSG0_DISK_TABLE_CYLS		gettext("Cylinder")
#define	MSG0_DISK_TABLE_MB		gettext("MB")
#define	MSG0_DISK_TABLE_MOUNT		gettext("Directory")
#define	MSG0_DISK_TABLE_PRESERVED	gettext("Preserve")

#define	MSG0_FDISK_TABLE_HEADER		gettext("fdisk Partition Table")
#define	MSG0_FDISK_TABLE_PARTITION	gettext("Partition")
#define	MSG0_FDISK_TABLE_OFFSET		gettext("Offset")
#define	MSG0_FDISK_TABLE_MB		gettext("MB")
#define	MSG0_FDISK_TABLE_TYPE		gettext("Type")

/*
 * space listing strings
 */
#define	MSG1_SPACE_NOT_FIT		gettext(\
	"\"%s\" does not fit on any disk")
#define	MSG1_SPACE_TOO_SMALL		gettext(\
	"\"%s\" has insufficient space allocated")

#define	MSG0_SPACE_REQUIREMENTS		gettext("Space Requirements (Mbytes)")
#define	MSG0_SPACE_DIRECTORY		gettext("Directory")
#define	MSG0_SPACE_ACTUAL		gettext("Actual")
#define	MSG0_SPACE_REQUIRED		gettext("Required")
#define	MSG0_SPACE_DEFAULT		gettext("Default")
#define	MSG0_SPACE_TOTAL		gettext("TOTAL")
#define	MSG0_SPACE_NOTE			gettext("NOTE:")
#define	MSG0_SPACE_NOTE_LINE1		gettext(\
	"an '*' following actual sizes indicates an")
#define	MSG0_SPACE_NOTE_LINE2		gettext(\
	"existing or explicit size was specified in")
#define	MSG0_SPACE_NOTE_LINE3		gettext(\
	"the profile")

/*
 * swap file messages
 */
#define	MSG0_SWAPFILE_CREATE		gettext(\
	"Creating swap file (/.cache/swap)")
#define	MSG0_SWAPFILE_CREATE_FAILED	gettext(\
	"Could not create swap file (/.cache/swap)")
#define	MSG0_SWAPFILE_CREATE_ENTRY	gettext(\
	"Creating /etc/vfstab entry (/.cache/swap)")


/*
 * upgrade messages
 */

#define	MSG0_RESUME_UPGRADE		gettext(\
	"Resuming partially completed upgrade")
#define	MSG1_RESUME_UPGRADE_STATUS	gettext(\
	"Status from resumed upgrade = %d")
#define	MSG0_UPGRADE_WOULD_RESUME	gettext(\
	"Partially completed upgrade would have resumed if not in debug mode")
#define	MSG0_LOADING_LOCAL_ENV		gettext(\
	"Loading local environment and services")
#define	MSG0_LOADING_LOCAL_ENV_FAILURE	gettext(\
	"Failure loading local environment")
#define	MSG0_GEN_UPGRADE_ACTIONS	gettext(\
	"Generating upgrade actions")
#define	MSG0_UPGRADE_FAILURE		gettext(\
	"Upgrade actions failed")
#define	MSG0_MOUNT_FAILURE		gettext(\
	"Mount failed for either root, swap, or other file system")
#define	MSG0_GENERATING_SCRIPT		gettext(\
	"Generating upgrade script")
#define	MSG0_DO_UPGRADE			gettext(\
	"Executing upgrade script")
#define	MSG0_UPGRADE_NO_ELIGIBLE_DISKS	gettext(\
	"No upgradeable root file systems were found")
#define	MSG0_ROOTDEV_MULTIUPGRADE		gettext(\
	"Use a \"root_device\" keyword to identify which \"/\" \
system to upgrade")
#define	MSG1_UPGRADE_ROOTDEVICE_MISMATCH	gettext(\
	"The specified root (%s) was not found or was not upgradeable")

#endif _PF_STRINGS_H
