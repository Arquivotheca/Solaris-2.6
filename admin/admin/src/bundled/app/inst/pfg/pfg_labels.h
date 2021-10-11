#ifndef lint
#pragma ident "@(#)pfg_labels.h 1.83 96/10/09 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfg_labels.h
 * Group:	installtool
 * Description:
 */

#ifndef	_PFG_LABELS_H
#define	_PFG_LABELS_H

/*
 * Definitions of all the strings are made available for I18N.
 */

/*
 * Common strings.
 */
#define	PFG_WARNING	PFGSTR("WARNING: %s")
#define	PFG_ERROR	PFGSTR("ERROR: %s")
#define	PFG_CANCEL	PFGSTR("Cancel")
#define	PFG_CHANGE	PFGSTR("Change")
#define	PFG_CONTINUE	PFGSTR("Continue")
#define	PFG_CUSTOMIZE	PFGSTR("Customize...")
#define	PFG_ALLOCATE	PFGSTR("Allocate...")
#define	PFG_EXIT	PFGSTR("  Exit  ")
#define	PFG_GOBACK	PFGSTR("Go Back")
#define	PFG_HELP	PFGSTR("  Help  ")
#define	PFG_MBYTES	PFGSTR("MB")
#define	PFG_OKAY	PFGSTR("   OK   ")
#define	PFG_TOTAL	PFGSTR("Total")
#define	PFG_PRESERVED	PFGSTR("PRESERVED")
#define	PFG_YES		PFGSTR("Yes")
#define	PFG_NO		PFGSTR("No")

/* pfdata.c */

#define	PFG_DT_ERROR	PFGSTR("error")
#define	PFG_DT_WARN	PFGSTR("warning")
#define	PFG_DT_PROFILE	PFGSTR("# profile: %s")
#define	PFG_DT_CREATE	PFGSTR("# created: %s")
#define	PFG_DT_FNDATION	PFGSTR("# Foundation")
#define	PFG_DT_LOCALE	PFGSTR("# Languages")
#define	PFG_DT_NONE	PFGSTR("# none")
#define	PFG_DT_CUSTOM	PFGSTR("# Software")
#define	PFG_DT_NONE	PFGSTR("# none")
#define	PFG_DT_DISKS	PFGSTR("# File System and Disk Layout")
#define	PFG_DT_RFS	PFGSTR("# Remote File Systems")


/* pferror.c */

#define	PFG_ER_MODIFY	PFGSTR("This slice has been preserved;\n" \
			"it cannot be modified")
#define	PFG_ER_CUT	PFGSTR("Cannot cut preserved slice")
#define	PFG_ER_SUNW	PFGSTR("Name (argument 1) must start with SUNW")
#define	PFG_ER_ARGLONG	PFGSTR("Name (argument 1) is longer than " \
			"nine characters")
#define	PFG_ER_ARG2	PFGSTR("Argument 2 must be 'add', 'delete' or ''")
#define	PFG_ER_NOFS	PFGSTR("No file systems on disks that can " \
			"be preserved")
#define	PFG_ER_ZERO	PFGSTR("Cannot preserve slice of size zero")
#define	PFG_ER_PARSER	PFGSTR("Internal parser error")
#define	PFG_ER_CANTOPEN	PFGSTR("Cannot open specified file")
#define	PFG_ER_NOTOKEN	PFGSTR("No Token/Keyword")
#define	PFG_ER_INVALID	PFGSTR("Invalid system type")
#define	PFG_ER_SLICE	PFGSTR("One or more slices too small for file system")
#define	PFG_ER_DUPMOUNT	PFGSTR("You have already preserved a mount point " \
			"with this name")
#define	PFG_ER_OVERLAP	PFGSTR("Slices overlap")
#define	PFG_ER_EXTENDS	PFGSTR("Slice extends beyond end of disk")
#define	PFG_ER_CYLZERO	PFGSTR("Cylinder 0 cannot be used")
#define	PFG_ER_BEGIN	PFGSTR("Unallocated space at beginning of disk")
#define	PFG_ER_END	PFGSTR("Unallocated space at end of disk")
#define	PFG_ER_SPACE	PFGSTR("Unallocated space on disk")
#define	PFG_ER_NODISKS	PFGSTR("You must select at least one disk to " \
			"install Solaris software.")
#define	PFG_ER_VTOC	PFGSTR(\
	"Unable to load disk configuration file. " \
	"Exiting the installation.")
#define	PFG_ER_FINDDSKS	PFGSTR(\
	"No disks found on system. " \
	"Exiting the installation.")
#define	PFG_ER_NOMEDIA	PFGSTR("No media")
#define	PFG_ER_BADMEDIA	PFGSTR("Invalid media type")
#define	PFG_ER_NOTMOUNT	PFGSTR("Media not mounted")
#define	PFG_ER_NOPROD	PFGSTR("No product on media")
#define	PFG_ER_LOADMED	PFGSTR("Load media")
#define	PFG_ER_NOVALUE	PFGSTR("Missing value")
#define	PFG_ER_INSTALL	PFGSTR("Invalid install type")
#define	PFG_ER_DUPSYS	PFGSTR("Duplicate system type specification")
#define	PFG_ER_INVPROD	PFGSTR("Invalid product specification")
#define	PFG_ER_INVPART	PFGSTR("Invalid partitioning value")
#define	PFG_ER_NOMEM	PFGSTR("Out of memory")
#define	PFG_ER_NOSWAP	PFGSTR("No swap file size specified")
#define	PFG_ER_DUPCONF	PFGSTR("Duplicate cluster specified")
#define	PFG_ER_INVALDSK	PFGSTR("Invalid disk name specified")
#define	PFG_ER_UNKCOMM	PFGSTR("Unknown command (first argument)")
#define	PFG_ER_NOTSRVR	PFGSTR("System type not server")
#define	PFG_ER_MISSPARM	PFGSTR("Missing parameters")
#define	PFG_ER_IPADDR	PFGSTR("Invalid IP address")
#define	PFG_ER_HOSTNAME	PFGSTR("Invalid host name")
#define	PFG_ER_ROOTDISK	PFGSTR("Root disk undefined")
#define	PFG_ER_MISSIP	PFGSTR("Missing IP address for /usr")
#define	PFG_ER_NFS	PFGSTR("Ignoring preserve for NFS filesystem")
#define	PFG_ER_EXISTING	PFGSTR("Size must be 'existing' to preserve")
#define	PFG_ER_CANTPRE	PFGSTR("To preserve /, /usr,  or /var file " \
			"systems, you must rename them.")
#define	PFG_ER_NOSIZE	PFGSTR("No size specified")
#define	PFG_ER_NOSPACE	PFGSTR(\
"Size changed unsuccessful.  No more space available")
#define	PFG_ER_NOFIT	PFGSTR(\
"Size changed unsuccessful.  Slice cannot expand " \
"to fit specified size")
#define	PFG_ER_MODLOCK	PFGSTR("Cannot modify locked slice")
#define	PFG_ER_NONBOOT	PFGSTR("Cannot put / (root) on non-boot device")
#define	PFG_ER_IGNORE	PFGSTR("Cannot modify slice in IGNORE state")
#define	PFG_ER_EXPLICIT	PFGSTR("Invalid device for explicit partitioning")
#define	PFG_ER_INVALMNT	PFGSTR("Invalid mount point name specified\n" \
			"Mount point must begin with a '/'")
#define	PFG_ER_MNTPNT	PFGSTR("Mount point names must start with '/'")
#define	PFG_ER_MNTSPACE	PFGSTR("Mount point names cannot contain spaces")
#define	PFG_ER_MNTLONG	PFGSTR("Mount point name is too long")
#define	PFG_ER_SIZE	PFGSTR("Size must be a positive integer value")
#define	PFG_ER_RANGE	PFGSTR("Number must be between 0 and the size " \
			"of the disk")
#define	PFG_ER_NOTSEL	PFGSTR("Disk must be selected before it can be " \
			"configured")
#define	PFG_ER_BADDISK	PFGSTR("Disk not selectable. State not valid for " \
			"installing Solaris software.")
#define	PFG_ER_INVALCYL	PFGSTR("Invalid starting cylinder")
#define	PFG_ER_UNALLOC	PFGSTR("Unallocated Space on drive")
#define	PFG_ER_CHANGED	PFGSTR("Slice start and/or size has changed from " \
			"original value specified")
#define	PFG_ER_GEOMCHG	PFGSTR("disk geometry has changed since disk last " \
			"committed")
#define	PFG_ER_RQDFS	PFGSTR("Required file system, can not deselect")
#define	PFG_ER_UNSUPFS	PFGSTR("File System not supported for current " \
			"system type")
#define	PFG_ER_BOOTDISK PFGSTR("The boot disk is not selected " \
			"or does not have a / mount point")
#define	PFG_ER_UNKNOWN	PFGSTR("Unknown Error")
#define	PFG_ER_BADMNT	PFGSTR("invalid mount list specified, using " \
			"system defaults")
#define	PFG_ER_DISKOUTREACH PFGSTR("The root (/) slice of the Solaris " \
	"partition must end before the first 1023 cylinders of the disk.\n\n" \
	"> You must reduce the size of root for a valid\n" \
	"  disk configuration.")
#define	PFG_ER_OUTREACH PFGSTR("The root (/) slice of the Solaris partition " \
	"must end before the first 1023 cylinders of the disk.\n\n" \
	"> If there is an EXT DOS and/or Other partition listed above \n " \
	"   the Solaris partition on the screen, delete one or both \n " \
	"   partitions.\n\n" \
	"> If there is a PRI DOS partition listed above the Solaris \n " \
	"   partition on the screen, reduce the size of PRI DOS by %d MB \n " \
	"   and continue.")
#define	PFG_ER_ORDER	PFGSTR("Partitions are out of legal order")
#define	PFG_ER_LANG	PFGSTR("Language is not supported")
#define	PFG_ER_ATTR	PFGSTR("Cannot set attribute, No space available")

/* pfglayout.c */
#define	PFG_AUTO_BASIC	PFGSTR("Basic")
#define	PFG_AUTO_OPTIONS PFGSTR("Options")
#define	PFG_SLICENAME PFGSTR("Partition")
#define	PFG_RECSIZE PFGSTR("Recommended")
#define	PFG_CREATESIZE PFGSTR("Create")
#define	PFG_DISKLABEL PFGSTR("Disk")
#define	PFG_SLICELABEL PFGSTR("Slice")
#define	PFG_ANYSTRING	PFGSTR("Any")

/* pfgautolayout.c */

/* pfgautoquery.c */

#define	PFG_AQ_AUTOLAY	PFGSTR("Auto Layout")
#define	PFG_AQ_MANLAY	PFGSTR("Manual Layout")


/* pfgclients.c */

#define	PFG_CL_CLIENTS	PFGSTR("Number of clients:")
#define	PFG_CL_SWAP	PFGSTR("Megabytes of swap per client:")
#define	PFG_CL_ARCH	PFGSTR("Supported Platforms")
#define	PFG_CL_POSCLNT	PFGSTR("Number of clients must be a positive integer")
#define	PFG_CL_POSSWAP	PFGSTR("Swap size must be a positive integer")

/* pfgservice_select.c */

#define	PFG_SVC_ARCH	PFGSTR("Platform")
#define	PFG_SVC_OS	PFGSTR("OS")
#define	PFG_SVC_LOCALE	PFGSTR("Language")
#define	PFG_SVC_SIZE	PFGSTR("Size")
#define	PFG_SVC_MNTPT	PFGSTR("Mount Point")

/* pfgclient_setup.c */

#define	PFG_ROOT	PFGSTR("Root")
#define	PFG_SWAP	PFGSTR("Swap")
#define	PFG_SWAPANDROOT	PFGSTR("Both")
#define	PFG_SERVICE	PFGSTR("Service")
#define	PFG_CL_ROOTSVC	PFGSTR("Root Svc")
#define	PFG_CL_SWAPSVC	PFGSTR("Swap Svc")
#define	PFG_CL_SVCTYPE	PFGSTR("Type")
#define	PFG_CL_NUMCL	PFGSTR("# Clients")
#define	PFG_CL_MULTIPLY	PFGSTR("X")
#define	PFG_CL_SIZEPER	PFGSTR("Size Per")
#define	PFG_CL_EQUALS	PFGSTR("=")
#define	PFG_CL_TOTAL	PFGSTR("Total Size")
#define	PFG_CL_MNTPT	PFGSTR("Mount Point")
#define	PFG_NONE_CHOICE	PFGSTR("None")
#define	PFG_SEPSWAP_CHOICE	PFGSTR("Separate Swap")
#define	PFG_SWAPONROOT_CHOICE	PFGSTR("Swap On Root")
#define	PFG_SWAP_LABEL	PFGSTR("Swap")
#define	PFG_ROOT_LABEL	PFGSTR("Root")
#define	PFG_ROOTSVC_NONE	PFGSTR("None");
#define	PFG_NO_PLATFORM	PFGSTR("No Platform");
#define	PFG_PLATFORM1	PFGSTR("Platform1");
#define	PFG_PLATFORM2	PFGSTR("Platform2");
#define	PFG_PLATFORM3	PFGSTR("Platform3");


/* pfgcyl.c */

#define	PFG_CY_RECCOM	PFGSTR("Recommended")
#define	PFG_CY_MINIMUM	PFGSTR("Minimum")
#define	PFG_CY_LOAD	PFGSTR("Load...")
#define	PFG_CY_DISK	PFGSTR("Disk: %s  %4ld MB")
#define	PFG_CY_CYLS	PFGSTR("Disk: %s  %4ld CYLS")
#define	PFG_CY_OVERLAP	PFGSTR("Allow Overlapping Slices")
#define	PFG_CY_EXIST	PFGSTR("Load existing disk layout")
#define	PFG_CY_HEADINGS	PFGSTR("Size          Start          End    ")


/* pfgdisks.c */

#define	PFG_DK_RECCOM	PFGSTR("Recommended")
#define	PFG_DK_MINIMUM	PFGSTR("Minimum")
#define	PFG_DK_DSKMSG	PFGSTR("Disk: %s  %4ld MB")
#define	PFG_DK_CYLS	PFGSTR("CYLS")
#define	PFG_DK_ALLOC	PFGSTR("Allocated:")
#define	PFG_DK_FREE	PFGSTR("Free:")
#define	PFG_DK_CAPACITY	PFGSTR("Capacity:")
#define	PFG_DK_RNDERROR	PFGSTR("Rounding Error:")
#define	PFG_DK_BOOTSLICE	PFGSTR("Boot Slice")
#define	PFG_OVERHEAD	PFGSTR("OS Overhead:")


/* pfgfilesys.c */

#define	PFG_FS_FILESYS	PFGSTR("File System")
#define	PFG_FS_DISK	PFGSTR("Disk")
#define	PFG_FS_SIZE	PFGSTR("Size")
#define	PFG_FS_OPTIONS	PFGSTR("Options")

#define	PFG_FS_MINSIZE	PFGSTR("Minimum Size")


/* pfglocale.c */

#define	PFG_LC_DONT	PFGSTR("Available Languages")
#define	PFG_LC_SUPPORT	PFGSTR("Selected Languages")
#define	PFG_UD_DONOTUSE	PFGSTR("Available Disks")
#define	PFG_UD_USE	PFGSTR("Selected Disks")
#define	PFG_UD_ADD	PFGSTR(">")
#define	PFG_UD_REMOVE	PFGSTR("<")
#define	PFG_UD_REMOVE_ALL	PFGSTR("<<")
#define	PFG_UD_AUTO_ADD	PFGSTR(">>")
#define	PFG_LC_ADD	PFGSTR("Add > ")
#define	PFG_LC_REMOVE	PFGSTR(" < Remove")
#define	PFG_UD_EDIT	PFGSTR("Edit fdisk")
#define	PFG_UD_RECCOM	PFGSTR("Recommended:  ")
#define	PFG_UD_MINIMUM	PFGSTR("Required:  ")
#define	PFG_UD_TOTAVA	PFGSTR("Total Available:  ")
#define	PFG_UD_TOTSEL	PFGSTR("Total Selected:  ")

/* pfgmain.c */

#define	PFG_MN_ERROR	PFGSTR("error")
#define	PFG_MN_ROOT	PFGSTR(\
	"Must be root (uid 0) or use '-d disk_configuration_file' option")
#define	PFG_MN_WARN	PFGSTR("warning")
#define	PFG_MN_RESET	PFGSTR("Reset")
#define	PFG_MN_REMOVE	PFGSTR("Remove")
#define	PFG_MN_REBOOT	PFGSTR("Auto Reboot")
#define	PFG_MN_NO_REBOOT	PFGSTR("No Reboot")
#define	PFG_MN_WIPEOUT	PFGSTR("Creating this partition will wipe out " \
			"all existing partitions.")
#define	PFG_MN_BELOW	PFGSTR(\
"You have not selected the minimum disk space for installing the Solaris software " \
"you've selected. To resolve this problem, you can add more disks, " \
"add different disks, or remove software.")

#define	PFG_MN_LOADVTOC	PFGSTR(\
"Loading existing disk (vtoc) information will " \
"overwrite the layout of this disk.")

#define	PFG_MN_CHANGES	PFGSTR("Changes have been made to this disk slice. " \
			"If you preserve the slice, changes will be lost.")
#define	PFG_MN_PRESERVE	PFGSTR("Preserve")
#define	PFG_MN_SUCCESS	PFGSTR("\n\nInstallation successful... \n\n")
#define	PFG_MN_PROFILE	PFGSTR("Dump Profile")
#define	PFG_MN_STRUCTS	PFGSTR("Dump Structures")
#define	PFG_MN_RESTART	PFGSTR(\
"To restart the Solaris installation program, select \n"\
"\"Restart Install\" from the Install Workspace menu.\n")

/* pfgmeta.c */

#define	PFG_MT_BASE	PFGSTR("Software Group")
#define	PFG_MT_SIZE	PFGSTR("Recommended Size")

/* pfgprequery.c */

#define	PFG_PQ_PRESERVE	PFGSTR("Preserve...")

/* pfgpreserve.c */

#define	PFG_PS_DSKSLICE	PFGSTR("Disk Slice")
#define	PFG_PS_FILESYS	PFGSTR("File System")
#define	PFG_PS_SIZE	PFGSTR("Size")
#define	PFG_PS_PRESERVE	PFGSTR("Preserve")

/* pfgremote.c */

#define	PFG_RM_SERVER	PFGSTR("Server:")
#define	PFG_RM_ADDR	PFGSTR("IP address:")
#define	PFG_RM_REMOTE	PFGSTR("Remote file system:")
#define	PFG_RM_LOCAL	PFGSTR("Local mount point:")
#define	PFG_RM_RFS	PFGSTR("Remote File Systems")
#define	PFG_RM_ADD	PFGSTR("Add >")
#define	PFG_RM_REMOVE	PFGSTR("< Remove")
#define	PFG_RM_TEST	PFGSTR("Test")
#define	PFG_RM_BLANK	PFGSTR("Blank field found")
#define	PFG_RM_NOTHING	PFGSTR("Nothing selected")
#define	PFG_RM_FAILED	PFGSTR("Test mount of '%s' unsuccessful")
#define	PFG_RM_SUCCESS	PFGSTR("Test mount of '%s' successful")
#define	PFG_RM_NOSERVER	PFGSTR("You must specify the server")
#define	PFG_RM_NOREM	PFGSTR("You must specify the remote file system")
#define	PFG_RM_NOLOCAL	PFGSTR("You must specify the local mount point")

/* pfgremquery.c */

#define	PFG_RQ_EDIT	PFGSTR("Remote Mounts...")


/* pfgsoftware.c */

#define	PFG_SW_PACKAGE	PFGSTR("Software Clusters and Packages")
#define	PFG_SW_SIZE	PFGSTR("Size (MB)")
#define	PFG_SW_UNRESOLV	PFGSTR("Unresolved Software Dependencies")
#define	PFG_SW_PKGINFO	PFGSTR("Software Description:")
#define	PFG_SW_PRODUCT	PFGSTR("Product:")
#define	PFG_SW_ABBREV	PFGSTR("Abbreviation:")
#define	PFG_SW_VENDOR	PFGSTR("Vendor:")
#define	PFG_SW_ESTIMATE	PFGSTR("Estimated Size:")
#define	PFG_SW_DESC	PFGSTR("Description:")
#define	PFG_SW_EST	PFGSTR("Estimated size:")
#define	PFG_SW_TOTAL	PFGSTR("Total")
#define	PFG_SW_BUTTON	PFGSTR("Button Legend")
#define	PFG_SW_EXPAND	PFGSTR("Expanded cluster")
#define	PFG_SW_CONTRACT	PFGSTR("Collapsed cluster")
#define	PFG_SW_REQD	PFGSTR("Required")
#define	PFG_SW_PARTIAL	PFGSTR("Partial")
#define	PFG_SW_SELECT	PFGSTR("Selected")
#define	PFG_SW_UNSELECT	PFGSTR("Unselected")


/* pfgsolarcust.c */

#define	PFG_SC_UNUSED	PFGSTR("<unused>")
#define	PFG_SC_SOLARIS	PFGSTR("Solaris")
#define	PFG_SC_OTHER	PFGSTR("Other")
#define	PFG_SC_DOS	PFGSTR("PRI DOS")
#define	PFG_SC_EDOS	PFGSTR("EXT DOS")
#define	PFG_SC_CYLS	PFGSTR("CYLS")
#define	PFG_SC_ALLOC	PFGSTR("Allocated:")
#define	PFG_SC_FREE	PFGSTR("Free:")
#define	PFG_SC_CAPACITY	PFGSTR("Capacity:")
#define	PFG_SC_RNDERROR	PFGSTR("Rounding Error:")
#define	PFG_SC_PARTNAME	PFGSTR("Partition")
#define	PFG_SC_PARTSIZE	PFGSTR("Size")
#define	PFG_SC_PARTCYL	PFGSTR("Start Cylinder")

#define	PFG_FDISKPRES	PFGSTR("Changes to this partition will " \
			"destroy existing data on the partition.")

/* pfgsolarispart.c */

#define	PFG_SP_ENTIRE	PFGSTR("Auto-layout Solaris partition to " \
			"fill entire disk")
#define	PFG_SP_REMAIN	PFGSTR("Auto-layout Solaris partition to fill " \
			"remainder of disk")
#define	PFG_SP_CREATE	PFGSTR("Manually create Solaris fdisk partition")
#define	PFG_SP_MBFMT	PFGSTR("%s (%s MB)")


/* pfgsummary.c */

#define	PFG_SM_SUMMARY	PFGSTR("Profile")
#define	PFG_SM_BEGIN	PFGSTR("Begin Installation")
#define	PFG_SM_BEGIN_UP	PFGSTR("Begin Upgrade")
#define	PFG_SM_BASE	PFGSTR("\n")
#define	PFG_SM_ITYPE	PFGSTR("Installation Option:\n  %s\n")
#define	PFG_SM_CLIENT_SERVICES	PFGSTR("\nClient Services:\n")
#define	PFG_SM_NO_CLIENT_SERVICES	PFGSTR("\nClient Services:\n  None\n")
#define	PFG_SM_NUMCLIENTS	PFGSTR("   Number of clients: %d\n")
#define	PFG_SM_SWAP_PER_CLIENT	PFGSTR("   Swap per client: %d\n")
#define	PFG_SM_ROOT_PER_CLIENT	PFGSTR("   Root per client: %d\n")
#define	PFG_SM_BOOTDEVICE	PFGSTR("\nBoot Device:\n  %s%c%d\n")
#define	PFG_SM_BOOTDEVICE_PPC	PFGSTR("\nBoot Device:\n  %s\n")
#define	PFG_SM_ARCH	PFGSTR("   Client platforms:\n")
#define	PFG_SM_SWBASE	PFGSTR("\nSoftware:\n  %s %s,\n  %s\n")
#define	PFG_SM_LANGS	PFGSTR("\nLanguages:\n")
#define	PFG_SM_CUSTOM	PFGSTR("Software:")
#define	PFG_SM_INCL	PFGSTR("      -Including\n")
#define	PFG_SM_EXCL	PFGSTR("      -Excluding\n")
#define	PFG_SM_RFS	PFGSTR("\nRemote File Systems:\n")
#define	PFG_ER_DISKERROR	PFGSTR(\
"You have an invalid disk configuration " \
"because of the condition(s) displayed in the window below.  " \
"Errors should be fixed to ensure a successful installation. " \
"Warnings can be ignored without causing the installation to fail.")


/* pfgswquery.c */

/* pfgsystem.c */

#define	PFG_SY_HEADING	PFGSTR("Create:")

/* pfgtoplevel.c */
#define	PFG_ER_NODISPLAY	PFGSTR("Couldn't open display!\n" \
				"Is your display variable set properly?\n")

/* pfgtutor.c */

#define	PFG_TU_INSTALL	PFGSTR("Solaris Install")


/* pfgupgrade.c */

#define	PFG_UP_RESTART	PFGSTR("Restart")
#define	PFG_UP_INITIAL	PFGSTR("Initial")

/* pfgusedisks.c */

/* pfio.c */

/* pfparse.c */

/* pfutil.c */

/* pfvalidate.c */

#endif	/* _PFG_LABELS_H */
