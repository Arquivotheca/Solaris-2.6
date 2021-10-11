#ifndef lint
#pragma ident "@(#)app_usedisks.c 1.7 96/06/17 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_usedisks.c
 * Group:	libspmiapp
 * Description:
 *	Application library level disk selection handling routines.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmiapp_api.h"
#include "spmisvc_api.h"
#include "app_utils.h"

#include "app_strings.h"

static void _DiskAutoSelect(Disk_t *disk_ptr);
static void _FdiskAutoSelect(Disk_t *disk_ptr);

void
DiskAutoSelect(Disk_t *disk_ptr)
{
	/*
	 * they passed in a disk ptr - just check this one
	 * (e.g. useful for checking just the boot disk)
	 */

	/*
	 * In general, I don't know if any different checking or
	 * validation should be done when auto-selecting the boot disk
	 * (i.e. size constraints checking where '/' is going, etc.)
	 */
	if (disk_ptr) {
		_DiskAutoSelect(disk_ptr);
	} else {
		/* try auto selecting all the disks */
		WALK_DISK_LIST(disk_ptr) {
			_DiskAutoSelect(disk_ptr);
		}
	}
}

static void
_DiskAutoSelect(Disk_t *disk_ptr)
{
	/* disk must be okay */
	if (disk_not_okay(disk_ptr))
		return;

	if (disk_fdisk_req(disk_ptr)) {
		_FdiskAutoSelect(disk_ptr);
	} else {
		(void) select_disk(disk_ptr, NULL);
	}
}

int
DiskIsCurrentBootDisk(Disk_t *disk_ptr)
{
	Disk_t *boot_disk;

	if (DiskobjFindBoot(CFG_CURRENT, &boot_disk) == D_OK) {
		if (disk_ptr == boot_disk)
			return (1);
		else
			return (0);
	} else {
		return (0);
	}
}

/*
 * Function: DiskGetSize
 * Description:
 *	Gets the size of the disk that should be displayed in the
 *	disk selection screen.  Which size to display is different
 *	depending on whether it's being put in the selected or the
 *	unselected list.
 * Scope:	PUBLIC
 * Parameters:
 *	disk_ptr - [RO]
 *	selected - [RO]
 *		- indicates if the disk size we want returned
 *		  is the size for when this disk is in the selected
 *		  list (1), or the unselected list (0).
 * Return:	[int]
 *		- the size of the disk in MB
 * Globals:	none
 * Notes:
 */
int
DiskGetSize(Disk_t *disk_ptr, int selected)
{
	int size;

	if (!disk_ptr)
		return (0);

#if 0
	if (disk_not_okay(disk_ptr))
		size = 0;
	else if (disk_selected(disk_ptr))
		size = blocks_to_mb_trunc(disk_ptr,
			usable_sdisk_blks(disk_ptr));
	else
		size = blocks_to_mb_trunc(disk_ptr,
			usable_disk_blks(disk_ptr));
#else
	if (selected)
		size = blocks_to_mb_trunc(disk_ptr,
			usable_sdisk_blks(disk_ptr));
	else
		size = blocks_to_mb_trunc(disk_ptr,
			usable_disk_blks(disk_ptr));
#endif
	return (size);
}

/*
 * Function: DiskMakeListName
 * Description:
 *	make the 'name' of the disk that will be put in the
 *	lists in the disk selection screen.
 *	(e.g. "c0t0d0 bootdrive 404MB", or "c0t0d0          404MB")
 * Scope:	PUBLIC
 * Parameters:
 *	disk_ptr - [RO]
 *	selected - [RO]
 *		- indicates if the disk size we want returned
 *		  is the size for when this disk is in the selected
 *		  list (1), or the unselected list (0).
 * Return:	[char *]
 *		- name to use in disk list. Space for this is malloced
 *		  here and should be free'd by the application when
 *		  done using it.
 * Globals:	none
 * Notes:
 */
char *
DiskMakeListName(Disk_t *disk_ptr, int selected)
{
	char *disk_label;
	char tmp[80];
	int size;
	int i;

	disk_label = (char *) xmalloc(strlen(APP_BOOTDRIVE) + 1);
	if (DiskIsCurrentBootDisk(disk_ptr)) {
		(void) strcpy(disk_label, APP_BOOTDRIVE);
	} else {
		for (i = 0; i < strlen(APP_BOOTDRIVE); i++)
			disk_label[i] = ' ';
		disk_label[i] = '\0';
	}

	size = DiskGetSize(disk_ptr, selected);

	(void) sprintf(tmp, " %-6s %s %4d MB",
		disk_name(disk_ptr),
		disk_label,
		size);

	write_debug(APP_DEBUG_L1,
		"Disk name in list is: %s", tmp);

	return (xstrdup(tmp));
}

int
DiskGetListTotal(int selected)
{
	Disk_t *disk_ptr;
	int total = 0;

	WALK_DISK_LIST(disk_ptr) {
		if ((disk_selected(disk_ptr) && selected) ||
		    (!disk_selected(disk_ptr) && !selected)) {
			total += DiskGetSize(disk_ptr, selected);
		}
	}

	return (total);
}

static void
_FdiskAutoSelect(Disk_t *disk_ptr)
{
	int err;

	select_disk(disk_ptr, NULL);

	/* select it if it already has a solaris partition */
	if (get_solaris_part(disk_ptr, CFG_CURRENT)) {
		return;
	}

	err = FdiskobjConfig(LAYOUT_DEFAULT, disk_ptr, NULL);
	if (err == D_OK) {
		write_debug(APP_DEBUG_L1,
			"auto layout maxfree performed");
	} else {
		deselect_disk(disk_ptr, NULL);
	}
}
