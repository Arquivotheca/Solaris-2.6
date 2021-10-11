#ifndef lint
#pragma ident "@(#)spmiapp_strings.h 1.36 96/10/02 SMI"
#endif


/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmiapp_strings.h
 * Group:	libspmiapp
 * Description:
 */

#ifndef	_SPMIAPP_STRINGS_H
#define	_SPMIAPP_STRINGS_H

#include <libintl.h>

#include "spmiapp_api.h"	/* make sure we get LIBAPPSTR definition */

/*
 * i18n:
 * Note: For Warning/Error strings.
 * Unlike in the past, for 2.6 you longer need to hard-code newlines
 * in the error/warning strings.
 */

/*
 * i18n: Common button strings
 * (other than those defined in any spmi_ui* modules)
 */
#define	LABEL_CONTINUE_BUTTON	LIBAPPSTR("Continue")
#define	LABEL_GOBACK_BUTTON	LIBAPPSTR("Go Back")
#define	LABEL_RESET_BUTTON	LIBAPPSTR("Reset")
#define	LABEL_CHANGE_BUTTON	LIBAPPSTR("Change")
#define	LABEL_CUSTOMIZE_BUTTON	LIBAPPSTR("Customize")
#define	LABEL_INITIAL_BUTTON	LIBAPPSTR("Initial")
#define	LABEL_UPGRADE_BUTTON	LIBAPPSTR("Upgrade")
#define	LABEL_AUTOLAYOUT	LIBAPPSTR("Auto-layout")
#define	LABEL_REPEAT_AUTOLAYOUT	LIBAPPSTR("Repeat Auto-layout")

/*
 * i18n: Common labels
 */
#define	LABEL_SLICE	LIBAPPSTR("Slice")
#define	LABEL_FILE_SYSTEM	LIBAPPSTR("File System")

/*
 * Common strings
 */

/*
 * i18n: Common message dialog window titles
 */
#define	TITLE_WARNING	LIBAPPSTR("Warning")
#define	TITLE_ERROR	LIBAPPSTR("Error")

/* printed if an app exits ungracefully via a signal */
#define	ABORTED_BY_SIGNAL_FMT	LIBAPPSTR(\
"Exiting (caught signal %d)"\
"\n\n"\
"Type suninstall to restart.\n")

/*
 * Window titles and their corresponding onscreen text...
 */

/*
 * i18n: "The Solaris Installation Program" screen
 */
#define	TITLE_INTRO	LIBAPPSTR("The Solaris Installation Program")

#define	MSG_INTRO	LIBAPPSTR(\
"The Solaris installation program "\
"is divided into a series of short sections "\
"where you'll be prompted to provide "\
"information for the installation. "\
"At the end of each section, you can change "\
"the selections you've made before continuing.")

#define	MSG_INTRO_CUI_NOTE	LIBAPPSTR(\
"\n\n" \
"About navigation..." \
"\n" \
"\t- The mouse cannot be used" \
"\n" \
"\t- If your keyboard does not have function keys, or they do not " \
"\n" \
"\t  respond, press ESC; the legend at the bottom of the screen " \
"\n" \
"\t  will change to show the ESC keys to use for navigation.")

/*
 * i18n: "Install Solaris Software - Initial" screen
 */
#define	TITLE_INTRO_INITIAL	LIBAPPSTR("Install Solaris Software - Initial")

#define	MSG_INTRO_INITIAL	LIBAPPSTR(\
"You'll be using the initial option to install Solaris " \
"software on the system. The initial option overwrites " \
"the system's disks when the new Solaris software is " \
"installed." \
"\n\n" \
"On the following screens, you can accept the defaults " \
"or you can customize how Solaris software will be " \
"installed by:" \
"\n\n" \
"\t- Allocating space for diskless clients or AutoClient systems" \
"\n" \
"\t- Selecting the type of Solaris software to install" \
"\n" \
"\t- Selecting disks to hold software you've selected" \
"\n" \
"\t- Specifying how file systems are laid out on the disks" \
"\n\n" \
"After completing these tasks, a summary of your "\
"selections (called a profile) will be displayed.")

/*
 * i18n: "Install Solaris Software" (old Upgrade System?) screen
 */
/* #define	TITLE_UPGRADE	LIBAPPSTR("Upgrade System?") */
#define	TITLE_UPGRADE	LIBAPPSTR("Install Solaris Software")

#define	MSG_UPGRADE	LIBAPPSTR(\
"This system is upgradable, so you have two options for " \
"installing Solaris software. " \
"\n\n" \
"The upgrade option updates the Solaris " \
"software on the system to the new release, "\
"saving as many modifications as possible that "\
"you've made to the previous version of Solaris software. "\
"You should back up the system before using the upgrade option. " \
"\n\n" \
"The initial option overwrites the system's disks with " \
"the new version of Solaris software. "\
"Backing up any modifications that you've "\
"made to the previous version of Solaris software is recommended "\
"before starting the initial option. "\
"This option also lets you preserve any existing file systems." \
"\n\n" \
"After selecting an option and completing the tasks "\
"that follow, a summary of your selections "\
"will be displayed.")

/*
 * i18n: Ask the user if they want to resume an upgrade.
 * 1st %s: root slice we're upgrading (e.g. c0t0d0s0)
 * 2nd %s: Solaris Version of root slice we're upgrading (e.g. Solaris 2.5.1)
 */
#define	UPG_RECOVER_QUERY	LIBAPPSTR(\
"The installation program will resume a "\
"previous upgrade that did not finish. "\
"The slice %s with the %s "\
"version was being upgraded."\
"\n\n" \
"If you don't want to resume the upgrade on this slice, " \
"select Cancel to upgrade a different slice or perform " \
"an initial installation.")

/*
 * i18n: "Allocate Client Services?" screens
 */
#define	TITLE_ALLOCATE_SVC_QUERY	LIBAPPSTR("Allocate Client Services?")

#define	MSG_ALLOCATE_SVC_QUERY	LIBAPPSTR(\
"Do you want to allocate space for diskless clients and AutoClient systems?")

#define	TITLE_CLIENTALLOC	LIBAPPSTR("Allocate Client Services")

#define	MSG_CLIENTSETUP	LIBAPPSTR(\
"On this screen you can specify the size of root (/) and swap for clients.  " \
"The default number of clients is 5; the default root size is 25 Mbytes; " \
"the default swap size is 32 Mbytes.\n\n" \
"NOTE:  Specifying values on this screen only allocates space for clients. " \
"To complete client setup, you must use Solstice Host Manager after " \
"installing Solaris software.")

#define	TITLE_CLIENTS	LIBAPPSTR("Select Platforms")

#define	MSG_CLIENTS	LIBAPPSTR(\
"On this screen you must specify all platforms for clients that this server " \
"will need to support. The server's platform is selected by default " \
"and cannot be deselected.")

/*
 * i18n: Software selection screens
 */

#define	TITLE_SW	LIBAPPSTR("Select Software")

#define	MSG_SW	LIBAPPSTR(\
"Select the Solaris software to install on " \
"the system.\n\n" \
"NOTE: After selecting a software group, you can add or remove " \
"software by customizing it. However, this requires " \
"understanding of software dependencies and how Solaris "\
"software is packaged.")

#define	TITLE_CUSTOM	LIBAPPSTR("Customize Software")

/*
 * i18n: Select Language screen
 */

#define	TITLE_LOCALES	LIBAPPSTR("Select Languages")

#define	MSG_LOCALES	LIBAPPSTR(\
"Select the languages you want for displaying the user " \
"interface after Solaris software is installed.")

/*
 * i18n: Select Disks screens
 */
#define	TITLE_USEDISKS	LIBAPPSTR("Select Disks")

#define	MSG_USEDISKS	LIBAPPSTR(\
"Select the disks for installing Solaris " \
"software. Start by looking at the Required field; this " \
"value is the approximate space needed to install the " \
"software you've selected. Keep selecting disks until the Total " \
"Selected value exceeds the Required value.\n\n" \
"> To move a disk from the Available to the Selected window, " \
"click on the disk, then click on the > button.")

/*
 * i18n: Boot Device screens
 */
#define	TITLE_SELECT_BOOT_DISK	LIBAPPSTR("Select Boot Disk")
#define	TITLE_SELECT_BOOT_DEVICE	LIBAPPSTR("Select Root Location")

/* i18n: Window Title and Label */
#define	UPDATE_PROM_QUERY	LIBAPPSTR("Automatically Reboot System?")

/* i18n: List Selection */
#define	APP_NOPREF_CHOICE	LIBAPPSTR("No Preference")

/* i18n: These words get subbed into the messages according to conditions */
#define	APP_SLICE	LIBAPPSTR("slice")
#define	APP_DISK	LIBAPPSTR("disk")
#define	APP_PARTITION	LIBAPPSTR("partition")

/*
 * i18n: Preserve Data? screens
 */
#define	TITLE_PREQUERY	LIBAPPSTR("Preserve Data?")

#define	MSG_PREQUERY	LIBAPPSTR(\
"Do you want to preserve existing data? At least one of the disks you've " \
"selected for installing Solaris software has file systems or unnamed "\
"slices that you may want to save.")

/* i18n: Preserve Data screen */
#define	TITLE_PRESERVE	LIBAPPSTR("Preserve Data")

#define	MSG_PRESERVE	LIBAPPSTR(\
"On this screen you can preserve the data on some or all disk slices. " \
"Any slice you preserve will not be touched when Solaris software is " \
"installed. " \
"If you preserve data on / (root), /usr, or /var you must " \
"rename them because new versions of these file systems are created " \
"when Solaris software is installed.\n\n" \
"WARNING: Preserving an `overlap' slice will not preserve any data within it. "\
"To preserve this data, you must explicitly set the mount point name.")

/*
 * i18n: Auto Layout screens
 */

/* i18n: Warning Message Title */
#define	TITLE_AUTOLAYOUT_BOOT_WARNING	LIBAPPSTR(\
"Warning: Different Boot Device")

/* i18n: Warning Message Text */
#define	MSG_AUTOLAYOUT_BOOT_WARNING	LIBAPPSTR(\
"Because you have changed the boot device (where the " \
"root ('/') file system will be created) from %s, you must " \
"re-layout your disks to sync up file systems.")

#define	MSG_BOOT_PREVIOUS	LIBAPPSTR(\
"its previous location")

/*
 * i18n: File System and Disk Layout screen
 */
#define	TITLE_FILESYS	LIBAPPSTR("File System and Disk Layout")

#define	MSG_FILESYS	LIBAPPSTR(\
"The summary below is your current file system and disk layout, " \
"based on the information you've supplied.\n\n" \
"NOTE: If you choose to customize, you should understand file " \
"systems, their intended purpose on the disk, and how changing " \
"them may affect the operation of the system.")

/*
 * i18n: Customize Disks screen
 */
#define	TITLE_CUSTDISKS	LIBAPPSTR("Customize Disks")

/*
 * i18n: Customize Disks by Cylinder screen
 */
#define	TITLE_CYLINDERS	LIBAPPSTR("Customize Disks by Cylinders")

/*
 * i18n: Profile screen
 */
#define	TITLE_PROFILE	LIBAPPSTR("Profile")

#define	MSG_SUMMARY	LIBAPPSTR(\
"The information shown below is your profile for " \
"installing Solaris software. It reflects the choices " \
"you've made on previous screens.")

#define	MSG_SUMMARY_CLIENT_SERVICES	LIBAPPSTR(\
"\n\n" \
"NOTE: If you allocated space for client " \
"services, you must complete client setup by " \
"using Solstice Host Manager after Solaris " \
"software is installed.")

#define	BOOTOBJ_SUMMARY_NOTE	LIBAPPSTR(\
"\n\n" \
"NOTE:  You must change the BIOS because you " \
"have changed the default boot device.")

/*
 * i18n: the various install type labels on the Profile summary screen
 */
#define	INSTALL_TYPE_INITIAL_STR	LIBAPPSTR(\
"Initial")
#define	INSTALL_TYPE_UPGRADE_STR	LIBAPPSTR(\
"Upgrade")
#define	INSTALL_TYPE_UPGRADE_DSR_STR	LIBAPPSTR(\
"Upgrade")

/* i18n: upgrade slice label in profile screen */
#define	APP_SUMMARY_UPG_TARGET	LIBAPPSTR("\nUpgrade Target:")

/* i18n: backup media label in profile screen */
#define	APP_SUMMARY_DSR_BACKUP_MEDIA	LIBAPPSTR(\
"\nBackup media:")

#define	APP_SUMMARY_FSLAYOUT	LIBAPPSTR(\
"\nFile System and Disk Layout:\n")

/*
 * i18n: Reboot After Installation? screen
 */
#define	TITLE_REBOOT	LIBAPPSTR("Reboot After Installation?")

#define	MSG_REBOOT	LIBAPPSTR(\
"After Solaris software is installed, the system must be rebooted. "\
"You can choose to have the system automatically rebooted, or choose " \
"No Reboot so you can run scripts and do other customizations.")

/*
 * i18n: "Installing Solaris Software - Progress" screen
 */
#define	TITLE_PROGRESS	LIBAPPSTR(\
"Installing Solaris Software - Progress")

#define	MSG_PROGRESS	LIBAPPSTR(\
"The Solaris software is now being installed on the system " \
"using the profile you created. Installing Solaris software can take " \
"up to 2 hours depending on the software you've " \
"selected and the speed of the network or local CD-ROM. " \
"\n\n" \
"When Solaris software is completely installed, the message " \
"`Installation complete' will be displayed.\n")

#define	LABEL_PROGRESS_PARTITIONING	LIBAPPSTR(\
"Partitioning disks...")
#define	LABEL_PROGRESS_INSTALL	LIBAPPSTR("Installing: ")
#define	LABEL_PROGRESS_COMPLETE	LIBAPPSTR("Installation complete")

/*
 * i18n: "Upgrading Solaris Software - Progress" screen
 */
#define	TITLE_UPG_PROGRESS	LIBAPPSTR(\
"Upgrading Solaris Software - Progress")

#define	MSG_UPG_PROGRESS	LIBAPPSTR(\
"The Solaris software is now being upgraded on the system "\
"using the profile you created. Upgrading Solaris software can take "\
"up to 2 hours (may be longer on servers) depending on the software you've "\
"selected, the reallocation of any space if needed, and the speed of "\
"the network or local CD-ROM. "\
"\n\n"\
"When Solaris software is completely upgraded, the message "\
"`Upgrade complete' will be displayed.")

#define	LABEL_UPG_PROGRESS	LIBAPPSTR(\
"Upgrading")

/*
 * i18n: Mount Remote File Systems? screen
 */
#define	TITLE_MOUNTQUERY	LIBAPPSTR("Mount Remote File Systems?")

#define	MSG_MOUNTQUERY	LIBAPPSTR(\
"Do you want to mount software from a remote file server? This may be " \
"necessary if you had to remove software because of disk space problems.")

/*
 * i18n: Mount Remote File Systems screen
 */
#define	TITLE_MOUNTREMOTE	LIBAPPSTR("Mount Remote File Systems")

#define	TITLE_REMOTEMOUNT_STATUS	LIBAPPSTR("Mount Remote File Systems")

/*
 * i18n: "Automatically Layout File Systems" screen
 */
#define	TITLE_AUTOLAYOUT	LIBAPPSTR(\
"Automatically Layout File Systems")

#define	MSG_AUTOLAYOUT	LIBAPPSTR(\
"On this screen you must select all the file systems you want auto-layout " \
"to create, or accept the default file systems shown.\n\n" \
"NOTE: For small disks, it may be necessary " \
"for auto-layout to break up some of the file systems you request " \
"into smaller file systems to fit the available disk space. So, after " \
"auto-layout completes, you may find file systems in the layout " \
"that you did not select from the list below.")

/*
 * i18n: "Automatically Layout File Systems?" screen
 */
#define	TITLE_AUTOLAYOUTQRY	LIBAPPSTR(\
"Automatically Layout File Systems?")

#define	MSG_AUTOLAYOUTQRY	LIBAPPSTR(\
"Do you want to use auto-layout to automatically layout " \
"file systems? Manually laying out file systems requires advanced system " \
"administration skills.")

/*
 * i18n: "Repeat Auto-layout?" screen
 */
#define	TITLE_REDO_AUTOLAYOUT	LIBAPPSTR("Repeat Auto-layout?")

#define	MSG_REDO_AUTOLAYOUT	LIBAPPSTR(\
"Do you want to repeat the auto-layout of your file systems? " \
"Repeating the auto-layout will destroy the current layout of file " \
"systems, except those marked as preserved.")

/*
 * i18n: "Create Solaris fdisk Partition" screen
 */
#define	TITLE_CREATESOLARIS	LIBAPPSTR("Create Solaris fdisk Partition")

#define	MSG_CREATESOLARIS	LIBAPPSTR(\
"There is no Solaris fdisk partition on this disk. " \
"You must create a Solaris fdisk partition if you want to use this " \
"disk to install Solaris software.\n\n" \
"Two or three of the following methods are available: " \
"have the software auto-layout the Solaris partition to fill the " \
"entire fdisk (which will overwrite any existing fdisk partitions), " \
"auto-layout the Solaris partition to fill the remainder of the disk " \
"(which will lay out the Solaris partition around existing fdisk " \
"partitions), or manually lay out the Solaris fdisk partition.")

/*
 * i18n: "Customize fdisk Partitions" screen
 */
#define	TITLE_CUSTOMSOLARIS	LIBAPPSTR("Customize fdisk Partitions")

#define	MSG_CUSTOMSOLARIS	LIBAPPSTR(\
"On this screen you can create, delete, and customize fdisk partitions. " \
"The Free field is updated as you assign sizes to fdisk partitions " \
"1 through 4.")

/*
 * i18n: Exit screen
 */
#define	TITLE_EXIT	LIBAPPSTR("Exit")

#define	MSG_EXIT	LIBAPPSTR(\
"If you exit the Solaris installation program, " \
"your profile is deleted. " \
"However, you can restart the Solaris installation program " \
"from the console window.")

/*
 * i18n: "Customize Existing Software?" (upgrade) screen
 */
#define	TITLE_UPG_CUSTOM_SWQUERY	LIBAPPSTR("Customize Software?")

#define	MSG_UPG_CUSTOM_SWQUERY	LIBAPPSTR(\
"Do you want to customize (add or delete) software "\
"for the upgrade? By default, the existing software on "\
"the system will be upgraded.")

/*
 * i18n: "Solaris Version to Upgrade"  (upgrade) screen
 */
#define	TITLE_OS_MULTIPLE	LIBAPPSTR(\
"Select Version to Upgrade")

#define	MSG_OS	LIBAPPSTR(\
"More than one version of Solaris has been found on the system. "\
"Select the version of Solaris to upgrade from.")

#define	OS_SOLARIS_PREFIX	LIBAPPSTR("Solaris")
#define	OS_VERSION_LABEL	LIBAPPSTR("Solaris Version")

/*
 * i18n: "Analyzing System" (upgrade) screen
 */
#define	TITLE_SW_ANALYZE	LIBAPPSTR("Analyzing System")

#define	MSG_SW_ANALYZE	LIBAPPSTR(\
"The Solaris software on the system is being analyzed for "\
"the upgrade.")

#define	LABEL_SW_ANALYZE	LIBAPPSTR("Analyzing System...")
#define	LABEL_SW_ANALYZE_COMPLETE	LIBAPPSTR(\
"Analyze Complete")

/*
 * i18n: the labels for the various phases of software space checking
 * Where appropriate, the software will add on a ": pkgname" tag to the
 * end.
 * In the GUI overall length (including the ": pkgname" add on
 * must fit within the
 * Installtool*dsr_analyze_dialog*panelhelpText*columns: value.
 * In the CUI it must be < 60 chars.
 */
#define	LABEL_UPG_VAL_FIND_MODIFIED	LIBAPPSTR(\
"Checking modified files")
#define	LABEL_UPG_VAL_CURPKG_SPACE	LIBAPPSTR(\
"Calculating database size for packages on system")
#define	LABEL_UPG_VAL_CURPATCH_SPACE	LIBAPPSTR(\
"Calculating database size for patches on system")
#define	LABEL_UPG_VAL_SPOOLPKG_SPACE	LIBAPPSTR(\
"Calculating database size for spooled packages on system")
#define	LABEL_UPG_VAL_CONTENTS_SPACE	LIBAPPSTR(\
"Calculating size of packages on system")
#define	LABEL_UPG_VAL_NEWPKG_SPACE	LIBAPPSTR(\
"Calculating size of new packages")
#define	LABEL_UPG_VAL_EXEC_PKGADD	LIBAPPSTR(\
"Adding package")
#define	LABEL_UPG_VAL_EXEC_PKGRM	LIBAPPSTR(\
"Removing package")
#define	LABEL_UPG_VAL_EXEC_REMOVEF	LIBAPPSTR(\
"Removing obsolete files in package")
#define	LABEL_UPG_VAL_EXEC_SPOOL	LIBAPPSTR(\
"Adding spooled package")
#define	LABEL_UPG_VAL_EXEC_RMTEMPLATE	LIBAPPSTR(\
"Removing spooled package")
#define	LABEL_UPG_VAL_EXEC_RMDIR	LIBAPPSTR(\
"Removing directory")
#define	LABEL_UPG_VAL_EXEC_RMSVC	LIBAPPSTR(\
"Removing service")
#define	LABEL_UPG_VAL_EXEC_RMPATCH	LIBAPPSTR(\
"Removing patch")
#define	LABEL_UPG_VAL_EXEC_RMTEMPLATEDIR	LIBAPPSTR(\
"Removing template directory")

/*
 * i18n: "Change Auto-layout Constraints" (upgrade) screen
 *
 * This screen has column headings in English that look like:
 *
 *      File System      Slice      Free    Space    Constraints   Minimum
 *                                  Space   Needed                 Size
 * ----------------------------------------------------------------------
 * [X]  /               c0t0d0s0      86        0    Changeable  115
 * etc...
 *
 * The total width across the screen for all these
 * headings must be <= 67 chars in the CUI
 * (which includes 3 spaces between columns)
 */

#define	TITLE_DSR_FSREDIST	LIBAPPSTR(\
"Change Auto-layout Constraints")

/* %s is the additional CUI note (found in the CUI message file) */
#define	MSG_DSR_FSREDIST	LIBAPPSTR(\
"On this screen you can change the "\
"constraints on the file systems and repeat auto-layout "\
"until it can successfully reallocate space. "\
"%s " \
"All size and space values are in Mbytes."\
"\n\n"\
"TIP: To help auto-layout reallocate space, change "\
"more file systems from the Constraints menus to be "\
"changeable or movable, especially those that reside on the same "\
"disks as the file systems that need more space.")

/*
 * i18n:
 * Used throughout the DSR upgrade screens.
 * 12 chars max
 * File system name tag when the last mounted on field for a file system
 * cannot be found or the slice has no file system name.
 */
#define	APP_FS_UNNAMED	"<       >"

/* i18n: "Change Auto-layout Constraints" error/warning messages */
#define	TITLE_APP_ER_DSR_MSG_FINAL_TOO_SMALL	LIBAPPSTR(\
"Invalid Minimun Size")

#define	APP_ER_DSR_MSG_FINAL_TOO_SMALL	LIBAPPSTR(\
"The minimum sizes for the following slices are invalid. "\
"The minimum size must be "\
"equal to or greater than the required size for that slice:")

/*
 * i18n:
 * 1st %s: prepended message consisting of above message +
 *	any number of this current message.
 * 2nd %s: file system name (e.g. /usr)
 * 3rd %s: slice specifier (e.g. c0t0d0s0)
 * 4th %s: required slice size in string format
 * 5th %s: minimum slice size as entered by user.
 */
#define	APP_ER_DSR_ITEM_FINAL_TOO_SMALL	LIBAPPSTR(\
"%s\n\n"\
"File system: %s\n"\
"    Slice: %s\n"\
"    Required Size: %s MB\n"\
"    Minimum Size Entered: %s MB")

#define	APP_ER_DSR_AVAILABLE_LOSE_DATA		LIBAPPSTR(\
"Auto-layout will use all the space on the following "\
"file systems to reallocate space (these are the file systems "\
"that you marked as Available). All the data in the "\
"file systems will be lost. "\
"\n\n")

/*
 * i18n:
 * 1st %s: prepended message consisting of above message +
 *	any number of this current message.
 * 2nd %s: file system name (e.g. /usr)
 * 3rd %s: slice specifier (e.g. c0t0d0s0)
 */
#define	APP_ER_DSR_AVAILABLE_LOSE_DATA_ITEM		LIBAPPSTR(\
"%sFile system: %*s    Slice: %s")

/*
 * i18n: "Change Auto-layout Constraints"
 * labels on top for currently 'active" slice
 */
#define	LABEL_DSR_FSREDIST_SLICE	LIBAPPSTR("Slice:")
#define	LABEL_DSR_FSREDIST_REQSIZE	LIBAPPSTR("Required Size:")
#define	LABEL_DSR_FSREDIST_CURRSIZE	LIBAPPSTR("Existing Size:")

/* i18n: "Change Auto-layout Constraints" column label headings */
#define	LABEL_DSR_FSREDIST_CURRFREESIZE	LIBAPPSTR("Free\nSpace")
#define	LABEL_DSR_FSREDIST_SPACE_NEEDED	LIBAPPSTR("Space\nNeeded")
#define	LABEL_DSR_FSREDIST_OPTIONS	LIBAPPSTR("Constraints")
#define	LABEL_DSR_FSREDIST_FINALSIZE	LIBAPPSTR("Minimum\nSize")

#define	LABEL_DSR_FSREDIST_ADDITIONAL_SPACE	LIBAPPSTR(\
"Total Space Needed:")
#define	LABEL_DSR_FSREDIST_ALLOCATED_SPACE	LIBAPPSTR(\
"Total Space Allocated:")

#define	LABEL_DSR_FSREDIST_LEGENDTAG_FAILED	LIBAPPSTR("*")
#define	LABEL_DSR_FSREDIST_LEGEND_FAILED	LIBAPPSTR("Failed File System")

#define	LABEL_DSR_FSREDIST_COLLAPSE	LIBAPPSTR("Collapse...")
#define	LABEL_DSR_FSREDIST_FILTER	LIBAPPSTR("Filter...")

/*
 * i18n: "Change Auto-layout Constraints"
 * Unlike most message boxes, the buttons in this message box are
 * not all the same size because the Repeat Auto-layout button is so
 * long that it forces all buttons to be to big and forces the overall
 * width of the whole screen to be huge.
 * So, put space around the buttons to make them a reasonable size.
 */
#define	LABEL_DSR_FSREDIST_GOBACK_BUTTON	LIBAPPSTR(" Go Back ")
#define	LABEL_DSR_FSREDIST_RESET_BUTTON		LIBAPPSTR(" Defaults ")
#define	LABEL_DSR_FSREDIST_EXIT_BUTTON		LIBAPPSTR("   Exit   ")
#define	LABEL_DSR_FSREDIST_HELP_BUTTON		LIBAPPSTR("   Help   ")

/*
 * i18n: "Change Auto-layout Constraints"
 * labels in Contraints option menu
 */
#define	LABEL_DSR_FSREDIST_FIXED	LIBAPPSTR("Fixed")
#define	LABEL_DSR_FSREDIST_MOVE		LIBAPPSTR("Movable")
#define	LABEL_DSR_FSREDIST_CHANGE	LIBAPPSTR("Changeable")
#define	LABEL_DSR_FSREDIST_AVAILABLE	LIBAPPSTR("Available")
#define	LABEL_DSR_FSREDIST_COLLAPSED	LIBAPPSTR("Collapsed")

#define	MSG_FSREDIST_GOBACK_LOSE_EDITS	LIBAPPSTR(\
"If you go back, all the selections you've made "\
"in the Auto-layout Constraints screen will be lost.")

/* i18n: "Change Auto-layout Constraints": Filter File Systems screen */
#define	TITLE_DSR_FILTER	LIBAPPSTR("Filter File Systems")

/* i18n: GUI Filter text */
#define	MSG_GUI_DSR_FILTER		LIBAPPSTR(\
"Select which file systems to display on the Auto-layout "\
"Constraints screen. If you select \"%s\" or \"%s,\" "\
"you must also specify a search string. You can "\
"use wildcards (* and ?) in the search string to display "\
"groups of slices or file systems.")

/* i18n: CUI Filter text */
#define	MSG_CUI_DSR_FILTER		LIBAPPSTR(\
"Select which file systems to display on the Auto-layout "\
"Constraints screen.")

/* i18n: filter radio button choices */
#define	LABEL_DSR_FSREDIST_FILTER_RADIO	LIBAPPSTR(\
"Filter:")
#define	LABEL_DSR_FSREDIST_FILTER_ALL	LIBAPPSTR(\
"All")
#define	LABEL_DSR_FSREDIST_FILTER_FAILED	LIBAPPSTR(\
"Failed File Systems")
#define	LABEL_DSR_FSREDIST_FILTER_VFSTAB	LIBAPPSTR(\
"Mounted by vfstab")
#define	LABEL_DSR_FSREDIST_FILTER_NONVFSTAB	LIBAPPSTR(\
"Not mounted by vfstab")
#define	LABEL_DSR_FSREDIST_FILTER_SLICE	LIBAPPSTR(\
"By slice name")
#define	LABEL_DSR_FSREDIST_FILTER_MNTPNT	LIBAPPSTR(\
"By file system name")

#define	LABEL_DSR_FSREDIST_FILTER_RETEXT	LIBAPPSTR(\
"Search %s:")
#define	LABEL_DSR_FSREDIST_FILTER_RE_EG	LIBAPPSTR(\
"(For example, c0t3 or /export.*)")

/* i18n: "Change Auto-layout Constraints": Collapse File Systems screen */
#define	TITLE_DSR_FS_COLLAPSE	LIBAPPSTR(\
"Collapse File Systems")

#define	MSG_DSR_FS_COLLAPSE	LIBAPPSTR(\
"The file systems selected below will remain on the system "\
"after the upgrade. To reduce the number of file systems, "\
"deselect one or more file systems from the list. The data in a "\
"deselected file system will be moved (collapsed) into its "\
"parent file system.")

#define	LABEL_DSR_FS_COLLAPSE_FS	LIBAPPSTR(\
"File System")

#define	LABEL_DSR_FS_COLLAPSE_PARENT	LIBAPPSTR(\
"Parent File System")

#define	LABEL_DSR_FS_COLLAPSE_CHANGED	LIBAPPSTR(\
"If you change the number of file systems, "\
"the system will be reanalyzed and the "\
"changes you've made in the Auto-layout "\
"Constraints screen will be lost.")

#define	APP_DSR_COLLAPSE_SPACE_OK	LIBAPPSTR(\
"After changing the number of file systems, the "\
"system now has enough space for the upgrade. Choose "\
"Repeat Auto-layout in the Auto-layout Constraints screen "\
"to continue with the upgrade.")

/*
 * i18n: "Change Auto-layout Constraints"
 * for tagging fields that are not applicable in
 *
 */
#define	LABEL_DSR_FSREDIST_NA	LIBAPPSTR("-----")

/*
 * i18n: "File System Modification Summary" screen
 *
 * This screen has column headings in English that look like:
 *
 * File System      Slice     Size    Modification   Existing   Existing
 *                            (MB)                   Slice      Size (MB)
 *
 * The total width across the screen for all these
 * headings must be <= 76 chars in the CUI
 * (which includes 3 spaces between columns)
 */
#define	TITLE_DSR_FSSUMMARY	LIBAPPSTR("File System Modification Summary")

#define	MSG_DSR_FSSUMMARY	LIBAPPSTR(\
"Auto-layout has determined how to reallocate space on "\
"the file systems. The list below shows what modifications "\
"will be made to the file systems and what the final "\
"file system layout will be after the upgrade. "\
"\n\n"\
"To change the constraints on the file system that auto-layout "\
"uses to reallocate space, choose Change.")

#define	LABEL_DSR_FSSUMM_NEWSLICE	LIBAPPSTR("Slice")
#define	LABEL_DSR_FSSUMM_NEWSIZE	LIBAPPSTR("Size\n(MB)")
#define	LABEL_DSR_FSSUMM_ORIGSLICE	LIBAPPSTR("Existing\nSlice")
#define	LABEL_DSR_FSSUMM_ORIGSIZE	LIBAPPSTR("Existing\nSize (MB)")
#define	LABEL_DSR_FSSUMM_WHAT_HAPPENED	LIBAPPSTR("Modification")

/* i18n: possible values for the "Modification" column */
#define	LABEL_DSR_FSSUMM_NOCHANGE	LIBAPPSTR(\
"None")
#define	LABEL_DSR_FSSUMM_CHANGED	LIBAPPSTR(\
"Changed")
#define	LABEL_DSR_FSSUMM_DELETED	LIBAPPSTR(\
"Deleted")
#define	LABEL_DSR_FSSUMM_CREATED	LIBAPPSTR(\
"Created")
#define	LABEL_DSR_FSSUMM_UNUSED	LIBAPPSTR(\
"Unused")
#define	LABEL_DSR_FSSUMM_COLLAPSED	LIBAPPSTR(\
"Collapsed")

/*
 * i18n: "Select Media for Backup" (upgrade) screen
 */
#define	TITLE_DSR_MEDIA	LIBAPPSTR("Select Media for Backup")

#define	MSG_DSR_MEDIA	LIBAPPSTR(\
"Select the media that will be used to "\
"temporarily back up the file systems "\
"that auto-layout will modify."\
"\n\n"\
"Space required for backup: %*d MB")

/* i18n: %s is one of "diskettes" or "tapes" below */
#define	MSG_DSR_MEDIA_MULTIPLE	LIBAPPSTR(\
"NOTE: If multiple %s are required for the backup, "\
"you'll be prompted to insert %s during the upgrade.")

#define	LABEL_DSR_MEDIA_MFLOPPY	LIBAPPSTR("diskettes")
#define	LABEL_DSR_MEDIA_MTAPES	LIBAPPSTR("tapes")

#define	LABEL_DSR_MEDIA_FLOPPY	LIBAPPSTR("diskette")
#define	LABEL_DSR_MEDIA_TAPE	LIBAPPSTR("tape")

#define	LABEL_DSR_MEDIA_MEDIA	LIBAPPSTR("Media:")
#define	LABEL_DSR_MEDIA_PATH	LIBAPPSTR("Path:")

#define	TEXT_DSR_MEDIA_ORIG_FLOPPY	LIBAPPSTR("/dev/rdiskette")
#define	TEXT_DSR_MEDIA_ORIG_TAPE	LIBAPPSTR("/dev/rmt/0")

/*
 * i18n: only the "For example" and perhaps the /export/temp
 * should need translating here...
 */
#define	LABEL_DSR_MEDIA_DEV_LFLOPPY	LIBAPPSTR(\
"(For example, /dev/rdiskette)")
#define	LABEL_DSR_MEDIA_DEV_LTAPE	LIBAPPSTR(\
"(For example, /dev/rmt/0)")
#define	LABEL_DSR_MEDIA_DEV_LDISK	LIBAPPSTR(\
"(For example, /export/temp or /dev/dsk/c0t0d0s1)")
#define	LABEL_DSR_MEDIA_DEV_NFS	LIBAPPSTR(\
"(For example, host:/export/temp)")
#define	LABEL_DSR_MEDIA_DEV_RSH	LIBAPPSTR(\
"(For example, user@host:/export/temp)")

/* i18n: the possible media types */
#define	LABEL_DSR_MEDIA_OPT_LFLOPPY	LIBAPPSTR("Local diskette")
#define	LABEL_DSR_MEDIA_OPT_LTAPE	LIBAPPSTR("Local tape")
#define	LABEL_DSR_MEDIA_OPT_LDISK	LIBAPPSTR("Local file system")
#define	LABEL_DSR_MEDIA_OPT_NFS	LIBAPPSTR("Remote file system (NFS)")
#define	LABEL_DSR_MEDIA_OPT_RSH	LIBAPPSTR("Remote system (rsh)")

/* i18n: dialog title */
#define	TITLE_DSR_MEDIA_INSERT	LIBAPPSTR("Insert Media")

#define	MSG_DSR_MEDIA_INSERT_FIRST	LIBAPPSTR(\
"Please insert the first %s "\
"so the installation program can validate it."\
"%s")

#define	MSG_DSR_MEDIA_INSERT_TAPE_NOTE	LIBAPPSTR(\
"\n\n" \
"NOTE: Make sure the %s is not write protected.")

#define	MSG_DSR_MEDIA_INSERT_FLOPPY_NOTE	LIBAPPSTR(\
"\n\n" \
"NOTE: Make sure the %s is formatted and that it is not write protected. "\
"You can use the fdformat command to format diskettes.")

/*
 * i18n: 2nd %s is in order to tack on the "make sure it's not write
 * protected note, if necessary.
 */
#define	MSG_DSR_MEDIA_ANOTHER	LIBAPPSTR(\
"Please insert %s number %d."\
"%s")

/*
 *  i18n: "Generating Backup List" (upgrade) screen
 */
#define	TITLE_DSR_ALGEN	LIBAPPSTR("Generating Backup List")

#define	MSG_DSR_ALGEN	LIBAPPSTR(\
"A list is being generated of all the file systems that "\
"auto-layout needs to modify. The file systems in the list "\
"will be temporarily backed up during the upgrade.")

#define	LABEL_DSR_ALGEN_COMPLETE	LIBAPPSTR(\
"Generate complete")

#define	LABEL_DSR_ALGEN_FAIL	LIBAPPSTR(\
"(failure)")

#define	LABEL_DSR_ALGEN_FS	LIBAPPSTR(\
"Searching file system:")

/*
 * i18n: Upgrade Progress - Archive backup/restore/newfs/upgrade
 */
#define	LABEL_DSR_ALBACKUP_PROGRESS	LIBAPPSTR(\
"Backing up:")
#define	LABEL_DSR_ALBACKUP_COMPLETE	LIBAPPSTR(\
"Backup complete")

#define	LABEL_DSR_ALRESTORE_PROGRESS	LIBAPPSTR(\
"Restoring:")
#define	LABEL_DSR_ALRESTORE_COMPLETE	LIBAPPSTR(\
"Restore complete")

#define	LABEL_UPGRADE_PROGRESS_COMPLETE	LIBAPPSTR(\
"Upgrade complete")

/*
 * i18n: "More Space Needed" (upgrade) screen
 */
#define	TITLE_DSR_SPACE_REQ	LIBAPPSTR("More Space Needed")

#define	MSG_DSR_SPACE_REQ	LIBAPPSTR(\
"The system's file systems do not have enough space for the upgrade. "\
"The file systems that need more space are listed "\
"below. "\
"You can either go back and delete software that "\
"installs into the file systems listed, "\
"or you can let auto-layout reallocate space on the file "\
"systems. "\
"\n\n" \
"If you choose auto-layout, it will reallocate space on the "\
"file systems by:\n" \
"\t- Backing up file systems that it needs to change\n"\
"\t- Repartitioning the disks based on the file system changes\n"\
"\t- Restoring the file systems that were backed up\n\n"\
"You'll be able to confirm any file system changes "\
"before auto-layout reallocates space.")

#define	LABEL_DSR_SPACE_REQ_CURRSIZE	LIBAPPSTR("Existing\nSize (MB)")
#define	LABEL_DSR_SPACE_REQ_REQSIZE	LIBAPPSTR("Required\nSize (MB)")

/*
 * i18n: "More Space Needed" (upgrade) screen
 * (2nd dialog - not on main aprade path, but off of collapse file
 * systems screen.
 */
#define	MSG_DSR_SPACE_REQ_FS_COLLAPSE	LIBAPPSTR(\
"Because of the changes you've made on the Collapse File "\
"Systems screen, the following file systems do not have "\
"enough space for the upgrade.")

/*
 * Error/Warning messages
 */

/*
 * i18n: "Select Versionto Upgrade" errors
 */
#define	APP_ER_NOUPDSK	LIBAPPSTR("You must select a Solaris OS to upgrade.")

/*
 * i18n:
 * 1st %s is solaris release string (i.e. Solaris 2.5.1)
 * 2nd %s is slice name (i.e. c0t0d0s0)
 * 3rd %s is an additional error message from those below...
 * The intent is to end up with a message that looks like:
 * The Solaris Version (Solaris 2.5.1) on slice c0t0d0s0
 * cannot be upgraded.
 *
 * A file system listed in the file system table (vfstab)
 * could not be mounted.
 */
#define	APP_ER_SLICE_CANT_UPGRADE	LIBAPPSTR(\
"The Solaris Version (%s) on slice %s cannot be upgraded." \
"\n\n" \
"%s")

#define	APP_MSG_VFSTAB_OPEN_FAILED	LIBAPPSTR(\
"The file system table (vfstab) could not be opened.")

#define	APP_MSG_MOUNT_FAILED	LIBAPPSTR(\
"A file system listed in the file system table (vfstab) " \
"could not be mounted.")

#define	APP_MSG_UMOUNT_FAILED	LIBAPPSTR(\
"A file system listed in the file system table (vfstab) "\
"could not be unmounted.")

#define	APP_MSG_FSCK_FAILED	LIBAPPSTR(\
"A file system listed in the file system table (vfstab) " \
"could not be checked by fsck.")

#define	APP_MSG_ADD_SWAP_FAILED	LIBAPPSTR(\
"Swap could not be added to the system.")

#define	APP_MSG_DELETE_SWAP_FAILED	LIBAPPSTR(\
"Swap could not be removed from the system.")

#define	APP_MSG_LOAD_INSTALLED	LIBAPPSTR(\
"There is an unknown problem with the software " \
"configuration installed on this disk.")

#define	APP_ER_FORCE_INSTALL	LIBAPPSTR(\
"There are no other upgradeable versions "\
"of Solaris on this system. You can choose "\
"to do an initial installation, or you can "\
"exit and fix any errors that are preventing "\
"you from upgrading. "\
"\n\n"\
"WARNING: If you choose Initial, you'll be "\
"presented with screens to do an initial "\
"installation, which will overwrite your file "\
"systems with the new version of Solaris. "\
"Backing up any modifications that you've "\
"made to the previous version of Solaris "\
"is recommended before starting the initial "\
"option. The initial option also lets you "\
"preserve existing file systems.")

/*
 * i18n: generic error message:
 * this one should have hard-coded newlines since it may just be printf'd
 */
#define	APP_ER_UNMOUNT	LIBAPPSTR(\
"Please reboot the system.\n" \
"There are inconsistencies in the current state of \n" \
"the system which only a system reboot can solve.")

/*
 * Disk checking errors
 */

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	APP_ER_NOKNOWNDISKS	LIBAPPSTR(\
"No disks found.\n\n"\
" > Check to make sure disks are cabled and\n"\
"   powered up.")

/*
 * i18n: disk error
 */
#define	DISK_ERROR_NO_INFO	LIBAPPSTR(\
	"There is no detailed information available about the disk drive or "\
	"its current state.")

/*
 * i18n: disk error
 */
#define	DISK_PREP_BAD_CONTROLLER	LIBAPPSTR(\
	"It appears that this disk drive is not responding to requests or "\
	"commands from the disk controller it is attached to.  As a result, "\
	"no information about this drive is available to the controller and "\
	"it cannot be probed, formatted or used.")

/*
 * i18n: disk error
 */
#define	DISK_PREP_UNKNOWN_CONTROLLER	LIBAPPSTR(\
	"It appears that this disk drive is attached to a disk controller "\
	"which is not recognized by the device driver software.  As a result, "\
	"no information about this controller is available to the driver and "\
	"none of the attached devices may be probed, formatted or used.")

/*
 * i18n: disk error
 */
#define	DISK_PREP_CANT_FORMAT	LIBAPPSTR(\
	"This drive has no label.  suninstall tried to provide a default "\
	"label using the `format' program but failed.  Since the drive cannot "\
	"be labelled, any changes to the partitioning cannot be saved.  As a "\
	"result, this drive may not be used by Solaris software.")

/*
 * i18n: disk error
 */
#define	DISK_PREP_NOPGEOM	LIBAPPSTR(\
	"This disk drive does not have a valid label.  If you want to use "\
	"this disk for the install, exit the Solaris installation program, "\
	"use the format(1M) command from the command line to label the disk, "\
	"and type 'suninstall' to restart the installation program.")

/*
 * i18n: disk error
 */
#define	DISK_PREP_CREATE_PART_ERR_TITLE1	LIBAPPSTR(\
	"fdisk Partition In Use")

/*
 * i18n: disk error
 */
#define	DISK_PREP_CREATE_PART_ERR1	LIBAPPSTR(\
	"This fdisk partition is currently being used.\n\n"\
	"You must delete the existing partition before you can create "\
	"a new one.")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_CREATE_PART_ERR_TITLE	LIBAPPSTR(\
	"No Space Available")

/*
 * i18n: disk error
 */
#define	DISK_PREP_CREATE_PART_ERR	LIBAPPSTR(\
	"All space on this disk is currently assigned to existing fdisk "\
	"partitions. You cannot create a new partition until you delete "\
	"an existing one.")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_DISK_HOSED	LIBAPPSTR(\
	"This disk (%s) cannot be used to install Solaris software.\n\n%s\n\n")

/*
 * i18n: disk error
 */
#define	DISK_PREP_NO_FDISK_LABEL_TITLE	LIBAPPSTR(\
	"Disk Not Formatted")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_NO_FDISK_LABEL	LIBAPPSTR(\
	"This disk drive is not formatted.  Unformatted disks cannot be used "\
	"to install Solaris software.\n\n"\
	"CAUTION: The Solaris installation program will format this disk now, "\
	"but existing data will be overwritten. If this disk has data "\
	"on it that you want to preserve, exit the Solaris installation "\
	"program and back up the data.")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_NO_SOLARIS_PART_TITLE	LIBAPPSTR(\
	"No Solaris fdisk Partition")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_NO_SOLARIS_PART	LIBAPPSTR(\
	"There is no Solaris fdisk partition on this disk. "\
	"You must create a Solaris fdisk partition if you want to use it to "\
	"install Solaris software.")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_NO_FREE_FDISK_PART_TITLE	LIBAPPSTR(\
	"No Free Partition")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	DISK_PREP_NO_FREE_FDISK_PART	LIBAPPSTR(\
	"All available fdisk partitions on this disk are in use.  "\
	"Therefore, an fdisk partition cannot be created for "\
	"Solaris software.\n\n"\
	"You must manually create a Solaris fdisk partition, or not use "\
	"this disk.")

/*
 * i18n: resource error - requires newline formatting since it's just printf'd
 */
#define	APP_ER_NOKNOWNRESOURCES	LIBAPPSTR(\
"No default resources found.\n\n")

/*
 * i18n: disk error - requires newline formatting since it's just printf'd
 */
#define	APP_ER_NOUSABLEDISKS	LIBAPPSTR(\
"One or more disks are found, but one of the\n"\
"following problems exists:\n\n"\
" > Hardware failure\n\n"\
" > Unformatted disk\n\n"\
" > fdisk partitioning (PowerPC systems only)\n\n"\
"   PowerPC systems require three unused\n"\
"   fdisk partitions and at least 40 Mbytes of\n"\
"   contiguous disk space.")

/*
 * i18n: DSR upgrade warnings
 */
#define	TITLE_APP_ER_CANT_AUTO_LAYOUT	LIBAPPSTR(\
"Auto-layout Unsuccessful")

#define	APP_ER_DSR_CANT_AUTO	LIBAPPSTR(\
"Auto-layout could not determine how to reallocate space on "\
"the file systems. On the next screen, change the "\
"constraints on the file systems to help auto-layout reallocate space.")

#define	APP_ER_DSR_AUTOLAYOUT_FAILED	LIBAPPSTR(\
"Auto-layout could not determine how to reallocate "\
"space on the file systems "\
"with the constraints you specified. "\
"Try other constraints.")

#define	APP_ER_DSR_RE_COMPFAIL	LIBAPPSTR(\
"Invalid regular expression `%s`.")

#define	APP_ER_DSR_RE_MISSING	LIBAPPSTR(\
"Please enter a regular expression to filter by.")

#define	APP_ER_DSR_FILTER_NOMATCH	LIBAPPSTR(\
"There are no file systems that match this filter criteria.")

#define	APP_ER_DSR_MEDIA_NODEVICE	LIBAPPSTR(\
"Please enter a media device path.")

/*
 * i18n: DSR Archive list media validation error strings
 */
#define	APP_ER_DSR_MEDIA_SUMM	LIBAPPSTR(\
"%s\n\n" \
"Current Media Selection:\n"\
"\tMedia: %s\n" \
"\tPath: %s\n")

#define	APP_ER_DSR_NOT_ENOUGH_SWAP	LIBAPPSTR(\
"The total amount of swap that you have allocated does not "\
"meet the minimum system requirements." \
"\n\n"\
"    Total Required Swap Size: %*lu MB\n"\
"    Total Swap Size Entered: %*lu MB")

#endif	/* _SPMIAPP_STRINGS_H */
