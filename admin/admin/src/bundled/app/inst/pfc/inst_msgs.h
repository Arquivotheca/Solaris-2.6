#ifndef lint
#pragma ident "@(#)inst_msgs.h 1.57 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_msgs.h
 * Group:	ttinstall
 * Description:
 */

#ifndef	_INST_MSGS_H
#define	_INST_MSGS_H

/*
 * installtool app level usage strings
 */
#define	PFC_PARAMS_PUBLIC_USAGE NULL
#define	PFC_PARAMS_PRIVATE_USAGE	gettext(\
	"\t[-u] (enable upgrade and server selections)\n" \
	"\t[-v] (enable ui debugging)\n")
#define	PFC_PARAMS_TRAILING_USAGE NULL


#define	PF_YES	gettext("Yes")
#define	PF_NO	gettext("No")
#define	SELECT_STR	gettext("Select")

/*
 * This is a duplicate of one in the libspmisvc library...
 */
#define	CUI_MSG_SU_SYSTEM_UPGRADE_COMPLETE	gettext(\
	"Upgrade Complete")

/*
 * This file contains most of the `builtin' help and message strings for
 * the initial install program.
 *
 */
#define	PFC_RESTART	gettext(\
"To restart the Solaris installation program, \n"\
"type \"suninstall\".\n")

#define	INIT_CANT_LOAD_MEDIA  gettext(\
	"There were problems loading the media from %s.\n")

#define	INIT_CANT_RECOVER_ERROR	gettext(\
	"\n\nPlease reboot the system.\n\n"\
	"There are inconsistencies in the current state of the system \n"\
	"which only a system reboot can solve.\n\n")

/*
 * Upgrade messages
 */
#define	UPG_SUMMARY_ONSCREEN_HELP		gettext(\
"The information below is your profile which shows how Solaris "\
"software will be installed. It is a summary of the choices you've made "\
"on previous screens.")

#define	UPG_LOAD_INSTALLED_STATUS_MSG	gettext(\
	" Slice %s - Examining installed Solaris release... please wait... ")
/* currently unused */
#define	UPG_PARTIAL_UPGRADE_STATUS_MSG	gettext(\
	" Slice %s - Checking for previous upgrade attempt... please wait... ")

/*
 * Allocate Client Services Option Screen
 */
#define	ALLOC_SERV_OPT_TITLE	gettext(\
	"Allocate Client Services Options")
#define	ALLOC_SERV_OPT_ONSCREEN_HELP	gettext(\
	"On this screen you can select which services to allocate "\
	"for clients. Your selection will change the options displayed "\
	"on the Allocate Client Services screen.")

/*
 * Client Architecture Selection screen (inst_client_arch.c)
 */
#define	CLIENT_ARCH_REQUIRED	gettext(\
	" Platform %s is required and cannot be deselected... ")

/*
 * Server parameters, diskless clients & swap file size (inst_srv_param.c)
 */
#define	SERVER_PARAMETERS_TITLE		gettext("Clients")

#define	SERVER_PARAMETERS_ONSCREEN_HELP		gettext(\
"On this screen you must specify the number of diskless clients the "\
"server will support; you must also specify the megabytes of swap "\
"space to hold the /export/root and /export/swap file systems for each "\
"diskless client.\n\n"\
"NOTE: If the server will not be supporting any diskless clients "\
"change the defaults to 0 so disk space will not be allocated.")

/*
 * Software `base' Selection Screen (inst_sw_choice.c)
 */
#define	SW_BASE_CHOICE_OK_CHANGE_TITLE	gettext(\
	"Change Software Groups")

#define	SW_BASE_CHOICE_OK_CHANGE	gettext(\
	"Do you really want to change software groups? Changing software "\
	"groups deletes any customizations you've made to the "\
	"previously-selected software group.")

/*
 * Detailed Software `Editor' Screen (inst_sw_edit.c)
 * 'footer help' prompts
 */
#define	SW_MENU_REQUIRED_PROMPT		gettext(\
	" Module is required, it cannot be deselected ")
#define	SW_MENU_SELECTED_PROMPT		gettext(\
	" Module is selected, press Return to deselect ")
#define	SW_MENU_PARTIAL_PROMPT		gettext(\
	" Module is partially selected, press Return to deselect all "\
	"components ")
#define	SW_MENU_UNSELECTED_PROMPT	gettext(\
	" Module is deselected, press Return to select ")
#define	SW_MENU_MODINFO_PROMPT		gettext(\
	" Press Return for module information ")

#define	SW_MENU_EXPAND_PROMPT		gettext(\
	" Press Return to show components ")

#define	SW_MENU_CONTRACT_PROMPT		gettext(\
	" Press Return to hide components ")

#define	SW_MENU_NAVIGATION_HINT		gettext(\
	" Move left, right, up, down using the arrow keys ")

/*
 * File system size and SW dependency validation warning screens (inst_check.c)
 */

#define	SW_DEPEND_ONSCREEN_HELP		gettext(\
"When customizing a software group, you added or removed packages " \
"that other software depends on to function, or you added packages that "\
"now require other software. Select OK to ignore this problem if you plan to "\
"mount the required software later, or if you're sure you do not want "\
"the functionality of the dependent software.")

#define	SMALL_PARTITION_ONSCREEN_HELP	gettext(\
"The file systems listed below are too small to hold the software you've "\
"selected. To resolve the problem, you can make the undersized slices "\
"larger, or remove software. " \
"You can ignore the space problem, "\
"but installing Solaris software may not be successful.")

/*
 * Disk Editor error messages (inst_disk_edit.c)
 */
#define	CUSTOMIZE_DISK_FILESYS_TITLE	gettext(\
	"Select Disk to Customize")

#define	CUSTOMIZE_DISK_FILESYS_NODISKS		gettext(\
	"File systems cannot be customized without first selecting disks.\n\n"\
	"Go back to the Disk screen to select disks.")

#define	CUSTOMIZE_DISK_FILESYS_ONSCREEN	gettext(\
	"Select a disk to customize.")

#define	DISK_EDIT_INVALID_DISK	gettext("There is no current disk")

#define	DISK_EDIT_INVALID_MOUNTPT	gettext(\
	"The mount point name you entered (%s) is invalid. "\
	"A mount point must:\n\n"\
	"- begin with a `/'.\n"\
	"- consist only of alpha-numeric characters and \n"\
	"  the separators `/', `.', `,', `-', and `_'\n\n"\
	"Additionally, there are two special mount point names you can \n"\
	"give to slices:\n\n"\
	"- `swap' indicates the slice is reserved for Solaris virtual \n"\
	"  memory backing store.\n\n"\
	"- `overlap' indicates the slice has been intentionally \n"\
	"  overlapped with another slice.\n\n"\
	"  Overlapped slices are not included in space calculations.\n\n")


#define	DISK_EDIT_INVALID_SIZE		gettext(\
	"The value you entered (%s) is not a valid size for this slice.  " \
	"The size you specify must be greater than or equal to zero and less " \
	"than or equal to the space available on the disk.")

#define	DISK_EDIT_INVALID_START_CYL	gettext(\
	"The value you entered (%s) is not a valid starting cylinder number.  "\
	"The starting cylinder must be a positive integer less than the " \
	"number of cylinders contained in the disk.")

#define	DISK_EDIT_NO_SPACE_LEFT		gettext(\
	"There is not enough free space on this disk to set this slice's "\
	"size to %s %s.  Specify a smaller size for this slice or reduce "\
	"the size of another slice.")

#define	DISK_EDIT_DUP_MOUNTPT		gettext(\
	"The value you entered (%s) for a file system mount point already "\
	"exists on another slice; two file systems cannot have the same "\
	"mount point.")

#define	DISK_EDIT_CHANGED		gettext(\
	"This slice cannot be preserved because its size has been modified.")

#define	DISK_EDIT_CANT_PRESERVE		gettext(\
	"This mount point cannot be preserved.")

#define	DISK_EDIT_CANT_CHANGE		gettext(\
	"This file system is marked as preserved.  To change its size, "\
	"unpreserve the slice by marking it [ ].")

#define	DISK_EDIT_LONG_MOUNTPT		gettext(\
	"The mount point you have entered, '%s', exceeds the maximum " \
	"allowable length for mount points (%d).  Use a shorter path "\
	"for the mount point.")

#define	NO_BOOT_WARNING		gettext(\
	"You have not selected the boot device %s for installing and " \
	"automatically rebooting Solaris software.\n\n" \
	"> To let the Solaris installation program configure a " \
	"boot device for you choose OK.\n\n" \
	"> To select a different boot device choose Select.")
#define	    ALT_BOOTDRIVE_WARNING	gettext(\
	"Moving / (root) to this disk automatically made it "\
	"the boot disk.\n\n"\
	"For x86 systems, this means whenever you boot the Solaris "\
	"software, you must use the Solaris boot diskette.\n\n"\
	"For SPARC and PowerPC systems, this means that you must "\
	"manually change the boot device using the eeprom command "\
	"before rebooting.\n\n"\
	"Remember, the new boot disk for this system is:\n\n"\
	"\t\t%s\n\n")

#define	DISK_EDIT_LOAD_ORIG_WARNING	gettext(\
	"Loading the existing slices from disk will overwrite any "\
	"changes you have made to this disk's layout.  Any slices "\
	"marked as preserved will become unpreserved.")\

#define	DISK_EDIT_ORIG_SLICES_TITLE	gettext(\
	"Existing Slices No Longer Valid")

#define	DISK_EDIT_ORIG_SLICES_ERR	gettext(\
"The original, existing slice configuration for this drive is no "\
"longer valid.  Typically, this is because you have moved the Solaris "\
"partition or re-created it within the fdisk table.\n\n"\
"To use the existing slicing for this disk, you must use the "\
"existing Solaris partition without modification.  \n\n"\
"To recover the original Solaris fdisk partition, return to the Disk "\
"screen and deselect this disk to undo all changes; then reselect the "\
"disk.")

#define	DISK_EDIT_TITLE	gettext("Customize Disk:")

/*
 * File System Preserve Error Messages (inst_disk_preserve.c)
 */

#define	PRESERVE_ERR_CANT_PRESERVE_UNKNOWN	gettext(\
	"This file system mount point (%s) cannot be preserved.")

#define	PRESERVE_ERR_CANT_PRESERVE	gettext(\
	"This file system mount point (%s) cannot be preserved with this "\
	"name.\n\nTo preserve the data in this file system, change the "\
	"mount point name; then reselect it to be preserved.")

#define	PRESERVE_ERR_MISALIGNED		gettext(\
	"The slice containing this file system does not start or end on a "\
	"cylinder boundary.\n\n"\
	"Mis-aligned slices cannot be preserved.  To preserve the data"\
	"in this file system, perform a back up and then restore it "\
	"after installing Solaris software.")

#define	PRESERVE_ERR_SHOULDNT_PRESERVE	gettext(\
	"WARNING: preserving %s system means that you will not be able to "\
	"use any of the system packaging commands on software packages "\
	"installed here.\n\n"\
	"This is because the software package database is kept in the /var "\
	"file system, and /var cannot be preserved during an initial "\
	"installation.\n\n"\
	"To preserve this file system without losing packaging "\
	"information, upgrade the system.  If you cannot "\
	"upgrade, you must re-install the software to recreate "\
	"the package database information.")

#define	PRESERVE_ERR_CONFLICTS_1	gettext(\
	"The slice for this file system mount point (%s) has been resized or "\
	"moved, so it cannot be preserved.\n\nTo preserve the data in this "\
	"file system, cancel this installation and restart the Solaris "\
	"installation program.")

#define	PRESERVE_ERR_CONFLICTS_2	gettext(\
	"Preserving this file system mount point conflicts with changes "\
	"you've made in the disk and file system editor.  \n\n"\
	"The following slices and file system mount points "\
	"will be deleted if you decide to preserve %s:\n\n")

/*
 * Disk Selection Error Messages (inst_disk_use.c)
 */
#define	USE_DISK_BOOTDRIVE_UNSELECTED_TITLE	gettext(\
	"Required Boot Drive Not Selected")

#define	USE_DISK_BOOTDRIVE_UNSELECTED	gettext(\
	"You have not selected the default boot drive (%s).")

#define	USE_DISK_INSUFFICIENT_SPACE_TITLE	gettext(\
	"Not Enough Disk Space")

#define	USE_DISK_INSUFFICIENT_SPACE	gettext(\
	"Auto-layout was not successful because of disk space problems. "\
	"To resolve this problem, you can reduce the number of file systems "\
	"for auto-layout, or you can manually layout file systems.")

#define	USE_DISK_INSUFFICIENT_SPACE_1	gettext(\
	"You must select at least one disk to install Solaris software.\n\n")

#define	USE_DISK_INSUFFICIENT_SPACE_2	gettext(\
"There is not enough disk space to successfully install Solaris software on "\
"this system. To increase disk space, remove software or "\
"deallocate space for client services.")

#define	USE_DISK_INSUFFICIENT_SPACE_2A	gettext(\
	"  Because this is an OS server, you can also reduce the number of "\
	"clients, and/or their root (/) and swap space.")

#define	USE_DISK_INSUFFICIENT_SPACE_3	gettext(\
	"You have not selected the minimum disk space for installing the "\
	"Solaris software you've selected. To resolve this problem, you can add more "\
	"disks, add different disks, or remove software.")

#define	USE_DISK_INSUFFICIENT_SPACE_4	gettext(\
	"You have not specified the minimum disk space for installing the "\
	"software you've selected. To resolve this problem, add more disks "\
	"or different disks, or remove software. " \
	"You can ignore the space\n" \
	"problem, but installing Solaris software may not be successful.")

#define	USE_DISK_INSUFFICIENT_SPACE_5	gettext(\
"You have not specified the minimum disk space for installing the "\
"software you've selected. You can resolve this by adding more disks "\
"or removing software. "\
"You can ignore the space\n" \
"problem, but installing Solaris software may not be successful.")

#define	USE_DISK_INSUFFICIENT_SPACE_6	gettext(\
	"There is not enough space to prepare a default disk and file system "\
	"layout. To resolve this problem, you can create your own file system "\
	"disk layout, or select more or different disks." \
	"You can ignore the space\n" \
	"problem, but installing Solaris software may not be successful.")

#define	USE_DISK_INSUFFICIENT_SPACE_7	gettext(\
"There is not enough space to successfully install Solaris software on this "\
"system.  To increase disk space, remove some software.")

#define	USE_DISK_INSUFFICIENT_SPACE_7A	gettext(\
	"  Because this is a server, you can increase disk space by reducing "\
	"the number of clients and/or the root (/) and swap space.")

#define	 USE_DISK_CANT_AUTO_TITLE	gettext(\
	"Auto-layout Not Possible")

#define	 USE_DISK_CANT_AUTO	gettext(\
	"Auto-layout cannot lay out file systems with the software and disks "\
	"you've selected. You can remove software, add disks, or manually "\
	"create the file system layout.")

#define	USE_DISK_UNSELECT_DISK_TITLE	gettext(\
	"Really Unselect Disk?")

#define	USE_DISK_UNSELECT_DISK_WARNING	gettext(\
	"Deselecting disk %s will delete all edits you've made to its "\
	"configuration.")

#define	AL_FAILED_TITLE	gettext("Auto-layout Unsuccessful")

/*
 * Autoconfig Screens (inst_fs_auto.c)
 */
#define	    AUTO_ALT_BOOTDRIVE_WARNING	gettext(\
"Because the default boot disk (%s) was not selected, or was not "\
"available, the following disk has been assigned as the boot disk:\n\n"\
"\t\t%s\n\n"\
"For x86 systems, this means that when you're finished installing "\
"Solaris software , you must use the Solaris boot diskette whenever you boot "\
"the system.\n\n"\
"For SPARC and PowerPC systems, this means that after installing "\
"Solaris software, and before rebooting, you must manually change "\
"the boot device using the eeprom command.")

/*
 * File System Summary Screen (inst_filesys.c)
 */
#define	FILESYS_EDIT_FILE_SYSTEM_TITLE	gettext(\
	"Note")

#define	FILESYS_EDIT_FILE_SYSTEM	gettext(\
"The File System and Disk Layout screen is not modifiable. "\
"However, you may proceed to the disk customization screen from here.")

/*
 * Space warning (inst_space.c)
 */
#define	SPACE_NOTICE_ONSCREEN_HELP	gettext(\
"Review your file systems. The list below shows the recommended "\
"Solaris file systems with their Minimum and Suggested space. If your "\
"file systems are below the Minimum, there might not be enough "\
"file system space to install Solaris software.\n\n"\
"NOTE: Some file systems may include other file systems. For example, "\
"(/) root might include /var.")

/*
 * Disk Selection Screen (inst_disk_use.c)
 */
#define	USE_DISK_CHOOSE_DISK_ONSCREEN_HELP	gettext(\
"On this screen you must select the disks for installing Solaris "\
"software. Start by looking at the Suggested Minimum field; "\
"this value is the approximate space needed to install the "\
"software you've selected. Keep selecting disks until the Total Selected "\
"value exceeds the Suggested Minimum value.")

#define	USE_DISK_SELECTED_PROMPT	gettext(\
	" Disk is selected, press Return to deselect")

#define	USE_DISK_BADDISK_PROMPT	gettext(\
	" Disk is not usable: unknown drive type")

#define	USE_DISK_NOT_SELECTED_PROMPT	gettext(\
	" Disk is not selected, press Return to select")

/*
 * Remote File System screens (inst_rfs.c)
 */
#define	REMOTE_TITLE	gettext("Remote File Systems")

#define	REMOTE_EDIT_REMOTE_FILE_SYSTEM_TITLE	gettext(\
	"Edit Remote File System")

#define	REMOTE_ADD_NEW_REMOTE_FILE_SYSTEM_TITLE	gettext(\
	"Mount Remote File System")

#define	REMOTE_ADD_NEW_REMOTE_FILE_SYSTEM	gettext(\
	"On this screen you can specify a remote file system to mount "\
	"from a server. You can explicity specify the file system or "\
	"select one from a list of the server's exportable "\
	"file systems.\n\n"\
	"NOTE: After entering information in the fields below, you can "\
	"test the mount. You should perform a test mount "\
	"on any system that requires software from a remote server.\n\n")

#define	REMOTE_EXPORTS_TITLE	gettext(\
	"Server's Exportable File Systems")

#define	REMOTE_EXPORTS_ONSCREEN_HELP	gettext(\
	"On this screen you can select one of the server's exportable file "\
	"systems; it will be copied into the `File system path' field on the "\
	"Mount Remote File System screen.")

#define	REMOTE_NEED_IPADDR	gettext(\
	" Provide an IP address for this server.")

#define	REMOTE_TEST_MOUNT_FAILED_TITLE	gettext("Test Mount Unsuccessful")

/*
 * Disk prep (inst_disk_prep.c)
 */
#define	DISK_PREP_PART_EDITOR_ONSCREEN	gettext(\
	"On this screen you can create and delete fdisk partitions for the "\
	"selected disk. To change the size, type, or location of an existing "\
	"fdisk partition, you must first delete it, and then create it "\
	"from scratch.\n\n"\
	"NOTE: You must create a Solaris fdisk partition on any disk you "\
	"want to use to install Solaris software.\n\n")

#define	FDISK_DISMISS_TO_EDIT	gettext(\
	"You must correct this error before the disk configuration can be "\
	"saved.")

#define	FDISK_CONTINUE_TO_EDIT	gettext(\
"If you do not create a Solaris partition, this disk cannot be used to "\
"install Solaris software.")

#define	CHOOSE_SOLARIS_PART_TYPE_CHOICE1		gettext(\
	"Auto-layout Solaris partition: entire disk")
#define	CHOOSE_SOLARIS_PART_TYPE_CHOICE2		gettext(\
	"Auto-layout Solaris partition: remainder of disk")
#define	CHOOSE_SOLARIS_PART_TYPE_CHOICE3		gettext(\
	"Manually create Solaris fdisk partition")

#define	CHANGE_PART_TYPE_TITLE		gettext(\
	"fdisk Partition Type")

#define	CHANGE_PART_TYPE_ONSCREEN_HELP		gettext(\
	"Select an fdisk parition type.")

#define	DISK_PREP_NEWPART_TITLE		gettext(\
	"fdisk Partition Size")

#define	DISK_PREP_NEWPART_ONSCREEN_HELP		gettext(\
"On this screen you can specify the size of the new fdisk partition.\n\n"\
"The initial size shown in Partition size (MB): is the "\
"maximum size for the partition. If you are creating a Solaris "\
"partition, the minimum recommended size is 200 MB.")

#define	DISK_PREP_PART_SIZE_TOO_BIG	gettext(\
	"The size you've entered is too big.  The maximum size for this "\
	"partition is %d MB (%d Cylinders).")

#define	DISK_PREP_DELETE_PART_TITLE	gettext(\
	"Delete fdisk Partition?")

#define	DISK_PREP_DELETE_PART	gettext(\
	"Do you really want to delete fdisk partition %d? "\
	"This partition is %d MB and may contain data.")

#define	DISK_PREP_CONFIG_ERR_TITLE	gettext("fdisk Configuration Problem")
#define	DISK_PREP_DISK_NOTUSABLE_ERR_TITLE	gettext("Unusable Disk")

#define	TITLE_DISK_PREP_PART_SIZE_TOO_SMALL	gettext(\
	"Invalid Partition Size")

#define	DISK_PREP_PART_SIZE_TOO_SMALL	gettext(\
"The size you entered for this partition is too small.\n\n"\
"An fdisk partition must be at least 1 MB.")


/*
 * Install Summary Screen (inst_summary.c)
 */
#define	INSTALL_TYPE	gettext("Installation Option")
#define	INSTALL_UPG_TARGET	gettext("Upgrade Target")
#define	INSTALL_UPG_DSR_BACKUP_MEDIA	gettext("Backup Media")
#define	INSTALL_SUMMARY_CLIENT_ARCH_TITLE	gettext(\
	"Client Platforms")

#define	INSTALL_CLIENT_SERVICES	gettext("Client Services")
#define	INSTALL_NONE	gettext("None")
#define	INSTALL_NUMCLIENTS	gettext("Number of clients")
#define	INSTALL_SWAP_PER_CLIENT	gettext("Swap per client")
#define	INSTALL_ROOT_PER_CLIENT	gettext("Root per client")
#define	INSTALL_BOOT_DEVICE	gettext("Boot Device")

#define	INSTALL_SUMMARY_LOCALE_TITLE	gettext("Languages")

#define	ERR_CHECK_DISKS	gettext(\
"You have an invalid disk configuration because of the condition(s) " \
"displayed in the window below.  Errors should be fixed to ensure a " \
"successful installation. Warnings can be ignored without causing the " \
"installation to fail.")

/*
 * inst_check.c
 */
#define	BAD_HOSTNAME_TITLE	gettext("Invalid Host Name")
#define	BAD_HOSTNAME	gettext(\
	"The host name you entered is invalid.  A host name must:\n"\
	"\t- be between %d and %d characters long.\n"\
	"\t- contain letters, digits, and minus signs (-).\n"\
	"\t- may not begin or end with a minus sign (-).")

#define	BAD_IPADDR_TITLE	gettext("Invalid IP Address")
#define	BAD_IPADDR	gettext(\
	"The IP address you entered is invalid. Check the following "\
	"requirements for IP addresses: \n\n"\
	"- An IP address must contain four sets of numbers separated\n"\
	"  by periods (example 129.200.9.1).\n\n"\
	"- Each component of an IP address must be between 0 and 254.\n\n"\
	"- IP addresses in the range 224.x.x.0 to 254.x.x.255 are \n"\
	"  reserved.\n\n")

#define	BAD_MOUNTPT_NAME_TITLE	gettext("Invalid Mount Point Name")
#define	BAD_MOUNTPT_NAME	gettext(\
	"The mount point name you entered is invalid. Check the following "\
	"requirements for mount points: \n\n"\
	"- A mount point name must begin with a `/'.\n"\
	"- A mount point name may consist only of alpha-numeric \n"\
	"  characters and the separators `/', `.', `,', `-', and `_'\n\n")

#define	DISK_CONFIG_ERROR_TITLE	gettext("Disk Configuration Error")

#define	SDISK_ERR_OFFEND	gettext(\
	"This disk layout is invalid and cannot be saved in its "\
	"current form.  You have configured a slice that goes beyond "\
	"the end of the disk.  To resolve the problem, make "\
	"this slice smaller.")

#define	SDISK_ERR_ZERO	gettext(\
"This disk layout is invalid and cannot be saved in its "\
"current form.  You have created a slice too small to hold a file "\
"system.  To resolve this problem, make this slice larger, or delete it.")

#define	SDISK_ERR_OVERLAP	gettext(\
	"This disk layout is invalid and cannot be saved in its "\
	"current form.  You have created two slices that overlap. "\
	"Slices with file systems cannot occupy the same physical "\
	"cylinders. To resolve this problem, edit the starting/"\
	"ending cylinders of one of these slices, or remove the  "\
	"file system mount point name from one of the slices.")

#define	SDISK_ERR_DUPMNT	gettext(\
	"This disk layout is invalid and cannot be saved in its current "\
	"form. You have created two slices with the same file system "\
	"or mount point names. To resolve the problem, change the file "\
	"system or mount point name on one of these slices so it is unique.")

#define	DISK_ERR_BOOTCONFIG	gettext(\
	"The current disk layout is invalid. You have not specified a root "\
	"file system; you must have a root file system on one of the disks.")

#define	DISK_ERR_NODISK_SELECTED	gettext(\
	"You have not selected any disks.\n\nYou must select at least "\
	"one disk with the root file system (/) before trying to install "\
	"Solaris software.")

#define	DISK_ERR_NOROOT_FS	gettext(\
	"You have not configured a root file system (/).\n\n"\
	"You must configure one disk with the root file system (/) before "\
	"trying to install Solaris software.")

#define	DISK_ERR_ROOTBOOT	gettext (\
	"The disk containing the root file system (/) could not be "\
	"set as the boot disk.")

#define	SDISK_ERR_BADORDER	gettext(\
	"This disk configuration is invalid and cannot be used in its "\
	"current form.  Two slices are out of order, that is, \n\n "\
	"a higher numbered slice (such as 2) "\
	"has beginning and ending cylinders that come before a lower "\
	"numbered slice (such as 1).")

/*
 * Reboot Screen (inst_parade.c)
 */
#define	INSTALL_REBOOT_YES	gettext("Auto Reboot")
#define	INSTALL_REBOOT_NO	gettext("No reboot")

#define	PLEASE_WAIT_STR		gettext(\
	" Please wait ... ")


/*
 * i18n: DSR CUI specific screens
 */

/* i18n: DSR media device for backup screen */
#define	TITLE_CUI_DSR_MEDIA_PATH	gettext(\
"Specify Path to Media")

#define	MSG_CUI_DSR_MEDIA_PATH	gettext(\
"Specify a path to the media that you selected "\
"for the backup."\
"\n\n"\
"Current media selection: %s")

/* i18n: %s is an example string */
#define	LABEL_CUI_DSR_MEDIA_PATH	gettext(\
"Path %s:")

/*
 * i18n: DSR autolayout constraints screens
 */
#define	MSG_CUI_ADD_FSREDIST	gettext(\
"You can also select Edit to filter the list of file systems, "\
"collapse file systems, or reset the constraints.")

#define	TITLE_CUI_FSREDIST_EDIT	gettext("Select Edit Option")
#define	MSG_CUI_FSREDIST_EDIT	gettext(\
"Select an edit option for the Auto-layout Constraints screen.")

/*
 * i18n:
 *	1st %s is a slice specifier (e.g. c0t0d0s0)
 *	2nd %s is a file system name (e.g. /usr or (unnamed))
 */
#define	LABEL_CUI_FSREDIST_EDIT_SLICE	gettext(\
"Change constraint for %s (%s) ...")
#define	LABEL_CUI_FSREDIST_FILTER	gettext(\
"Filter ...")
#define	LABEL_CUI_FSREDIST_COLLAPSE	gettext(\
"Collapse ...")
#define	LABEL_CUI_FSREDIST_RESET	gettext(\
"Reset constraints")

#define	TITLE_CUI_FSREDIST_EDIT_SLICE	gettext("Change Constraint")

#define	MSG_CUI_FSREDIST_EDIT_SLICE	gettext(\
"Change the constraint for the file system by selecting a "\
"different one from the list.")

#define	TITLE_CUI_FSREDIST_EDIT_SLICE_SIZE	gettext(\
"Specify Minimum Size")

#define	MSG_CUI_FSREDIST_EDIT_SLICE_SIZE	gettext(\
"Specify the minimum size that the file system "\
"will be after the upgrade. "\
"This enables you to increase or decrease the size "\
"of the file system.")

#define	LABEL_CUI_DSR_EDIT_SIZE	gettext("Minimum Size:")

#define	ER_CUI_FSREDIST_CANT_EDIT_COLLAPSED	gettext(\
"You cannot edit any auto-layout constraints on a "\
"collapsed file system.\n\n"\
"To uncollapse the file system, choose the Collapse "\
"edit option instead")

/* i18n: filter off of DSR autolayout constraints screens */
#define	TITLE_CUI_FSREDIST_FILTER	gettext(\
"Specify Filter Search String")

#define	MSG_CUI_FSREDIST_FILTER	gettext(\
"Specify a search string for the filter "\
"you selected. You can use wildcards (* and ?) "\
"in the search string to display groups of "\
"slices or file systems." \
"\n\n" \
"Current filter selection: %s")

/* i18n: %s is an example string */
#define	LABEL_CUI_DSR_FILTER_PATTERN	gettext("Search string %s:")

#endif	/* _INST_MSGS_H */
