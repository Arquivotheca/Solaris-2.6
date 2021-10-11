#ifndef lint
#pragma ident "@(#)pfg_strings.h 1.49 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfg_strings.h
 * Group:	installtool
 * Description:
 */

#ifndef	_PFG_STRINGS_H
#define	_PFG_STRINGS_H

#ifndef SUNW_INSTALL_INSTALL
#define	SUNW_INSTALL_INSTALL	"SUNW_INSTALL_INSTALL"
#endif

#define	PFGSTR(x)		dgettext(SUNW_INSTALL_INSTALL, x)

/* default width of strings' text-areas */
#define	XM_DEFAULT_COLUMNS	45

/*
 *
 */
#define	TITLE_BOOTDISKQUERY	PFGSTR("Select Different Boot Device?")
#define	MSG_BOOTDISK	PFGSTR(\
"On this screen you can select the %s where the root (/) file system " \
"will be configured for installing Solaris software. %s")

#define MSG_BOOTDISK_ANY	PFGSTR(\
"If you want the Solaris installation program to select a %s for you, " \
"select Any from the Device option menu.\n\n%s")

#define MSG_BOOTDISK_ANY_NO_PREF	PFGSTR(\
"If you want the Solaris installation program to select a %s for you " \
"from any of the disks listed, select No Preference.\n\n%s")

#define	MSG_BOOTDISK_REBOOT	PFGSTR(\
"NOTE: By default, the system's hardware (EEPROM) will be configured " \
"to always boot from the boot device you've selected.  If you do not " \
"want to automatically reboot from this device, deselect this option.")

#define	PFG_RESET	PFGSTR("Reset")
#define	PFG_DEVICELABEL	PFGSTR("Device")
#define	PFG_EXISTING_BOOT	PFGSTR("Original Boot Device: ")
#define	PFG_CHANGEBOOT		PFGSTR("Select...")
#define	PFG_SELECTROOT		PFGSTR("Select Root Location")
#define	PFG_BOOTDISKLABEL		PFGSTR("Boot Disk: %s")
#define	PFG_BOOTDEVICELABEL		PFGSTR("Boot Device: %s%c%d")
#define	MSG_DESELECT_BOOT	PFGSTR(\
"Do you really want to deselect the default boot disk <%s>?")

#define	MSG_DESELECT_BOOT1	PFGSTR(\
"Do you really want to deselect the default boot device <%s%c%d>?")

/*
 * installtool app level usage strings
 */
#define	PFG_PARAMS_PUBLIC_USAGE NULL
#define	PFG_PARAMS_PRIVATE_USAGE	PFGSTR(\
	"\t[-u] (enable upgrade and server selections)\n" \
	"\t[-v] (enable verbose ui output)\n" \
	"\t[-M] (map toplevel shell - for QA PARTNER)\n")
#define	PFG_PARAMS_TRAILING_USAGE NULL

/*
 *
 */

#define	MSG_DEPENDS		PFGSTR(\
"When customizing a software group, you added or removed packages " \
"that other software depends on to function, or you added packages that "\
"now require other software. Click OK to ignore this problem if you plan to "\
"mount the required software later, or if you're sure you do not want "\
"the functionality of the dependent software.")

/*
 *
 */

#define	MSG_SPACE		PFGSTR(\
"You have not specified the minimum disk space for installing " \
"the software you've selected. You can resolve this by:\n\n" \
"- Adding more or different disks \n" \
"- Removing software \n" \
"- Changing sizes of file systems \n\n" \
"You can ignore the space " \
"problem, but installing Solaris software may not be successful.")

/*
 *
 */

#define	MSG_SYS_TYPE		PFGSTR(\
"On this screen you must specify one of the following system types.  " \
"A system type determines where a system will get its directories " \
"and file systems, and whether it provides portions of Solaris software " \
"to other systems.")

/*
 *
 */
#define	MSG_AUTOFAIL	PFGSTR(\
"Auto-layout was not successful because of disk space problems. " \
"To resolve this problem, you can reduce the number of file systems " \
"to auto-layout, manually lay out file systems, or unpreserve some " \
"file systems.")

#define	MSG_NOBOOT PFGSTR(\
"You have not selected the boot device (%s) for installing and " \
"automatically rebooting Solaris software.\n\n" \
"> To let the Solaris installation program configure a boot device " \
"for you, choose OK.\n\n" \
"> To select a different boot device, choose Select...")

#define	MSG_ALTBOOT PFGSTR(\
"Because the default boot disk (%s) was not selected, or was not " \
"available, the following disk has been assigned as the boot disk:\n\n" \
"\t\t%s\n\n" \
"For x86 systems, this means that when you're finished installing " \
"Solaris software, you must use the Solaris boot diskette whenever you boot " \
"the system.\n\n" \
"For SPARC and PowerPC systems, this means that after installing "\
"Solaris software, and before rebooting, you must manually change " \
"the boot disk using the eeprom command.")

#define	MSG_NEWALTBOOT PFGSTR(\
"Moving / (root) to disk (%s) will automatically make it the " \
"boot disk. For x86 systems, this means whenever you boot the " \
"Solaris software, you must use the Solaris boot diskette.\n\n" \
"For SPARC and PowerPC systems, this means that you must manually " \
"change the boot device using the eeprom command before rebooting.")
#define	MSG_NEWALTBOOT_BASE PFGSTR(\
"Moving / (root) to disk (%s) will automatically make it the " \
"boot disk. %s")
#define	MSG_NEWALTBOOT_X86 PFGSTR(\
"For x86 systems, this means whenever you boot the " \
"Solaris software, you must use the Solaris boot diskette.\n\n")
#define	MSG_NEWALTBOOT_SPARC PFGSTR(\
"For SPARC systems, this means that you must manually change the " \
"boot device using the eeprom command before rebooting.")
#define	MSG_NEWALTBOOT_PPC PFGSTR(\
"For PowerPC systems, this means that you must manually " \
"change the boot device using the eeprom command before rebooting.")

#define	MSG_BASE_CHOICE_OK_CHANGE	PFGSTR(\
"Do you really want to change software groups? Changing " \
"software groups deletes any customizations you've made " \
"to the previously selected software group.")

#endif	/* _PFG_STRINGS_H */
