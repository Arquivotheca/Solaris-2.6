#ifndef lint
#pragma ident "@(#)app_disks.c 1.6 96/09/26 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_disks.c
 * Group:	libspmiapp
 * Description:
 *	Generic disk handling convenience functions needed by the apps.
 */
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

#include "spmiapp_lib.h"
#include "spmisvc_lib.h"
#include "app_utils.h"

/*
 * Function: DiskRestoreAll
 * Description:
 *	Restore all the disks in disk list to the current state
 *	from the requested state.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	state:	CFG_COMMIT or CFG_EXIST
 * Return: none
 * Globals: operates on the global disk list.
 * Notes:
 */
void
DiskRestoreAll(Label_t state)
{
	Disk_t *dp;

	WALK_DISK_LIST(dp) {
		restore_disk(dp, state);
	}
}

/*
 * Function: DiskCommitAll
 * Description:
 *	Commit all the disks in disk list (i.e. copy the current
 *	configuration to the committed state of the disks.
 *
 * Scope:	PUBLIC
 * Parameters:  none
 * Return: none
 * Globals: operates on the global disk list.
 * Notes:
 */
void
DiskCommitAll(void)
{
	Disk_t *dp;

	WALK_DISK_LIST(dp) {
		commit_disk_config(dp);
	}
}

/*
 * Function: DiskSelectAll
 * Description:
 *	Select or deselect all the disks in the disk list.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	select:
 *		0: deselect all disks
 *		!0: select all disks
 * Return: none
 * Globals: operates on the global disk list.
 * Notes:
 */
void
DiskSelectAll(int select)
{
	Disk_t *dp;

	WALK_DISK_LIST(dp) {
		if (select)
			select_disk(dp, NULL);
		else
			deselect_disk(dp, NULL);
	}
}

/*
 * Function: DiskNullAll
 * Description:
 *	Configure all selected disks to be empty.
 *
 * Scope:	PUBLIC
 * Parameters: none
 * Return: none
 * Globals: operates on the global disk list.
 * Notes:
 */
void
DiskNullAll(void)
{
	Disk_t *dp;

	WALK_DISK_LIST(dp) {
		SdiskobjConfig(LAYOUT_RESET, dp, NULL);
	}
}

void
DiskPrintAll(void)
{
	Disk_t *dp;

	WALK_DISK_LIST(dp) {
		print_disk(dp, NULL);
	}
}

/*
 * Function:	DiskGetContentDefault
 * Description:	Sum up the total space that would be required to
 *		hold the default layout configuration
 * Scope:	public
 * Parameters:	none
 * Return:	# >= 0	total number of sectors
 */
int
DiskGetContentDefault(void)
{
	ResobjHandle	res;
	int		total = 0;
	int		subtotal;

	/* sum up all independent file system resources */
	WALK_DIRECTORY_LIST(res) {
		if (ResobjIsIndependent(res))
			total += ResobjGetContent(res, ADOPT_ALL,
					RESSIZE_DEFAULT);
	}

	subtotal = total;
	/* find the minimum swap total for this system */
	total += ResobjGetSwap(RESSIZE_DEFAULT);

	if (get_trace_level())
		write_message(LOG, STATMSG, LEVEL1,
		    "===(Default) Grand Total: %d, +swap: %d",
		    sectors_to_mb(subtotal), sectors_to_mb(total));
	return (sectors_to_mb(total));
}

/*
 * Function:	DiskGetContentMinimum
 * Description:
 *	Sum up the total space that would be required to hold the minimum
 *	layout configuration.
 * Scope:	public
 * Parameters:	none
 * Return:	# >= 0	total number of sectors
 */
int
DiskGetContentMinimum(void)
{
	ResobjHandle	res;
	int		total = 0;
	int		subtotal;

	/* sum up all independent file system resources */
	WALK_DIRECTORY_LIST(res) {
		if (ResobjIsIndependent(res))
			total += ResobjGetContent(res, ADOPT_ALL,
					RESSIZE_MINIMUM);
	}

	subtotal = total;
	/* find the minimum swap total for this system */
	total += ResobjGetSwap(RESSIZE_MINIMUM);
	if (get_trace_level())
		write_message(LOG, STATMSG, LEVEL1,
		    "===(Minimum) Grand Total: %d, +swap: %d",
		    sectors_to_mb(subtotal), sectors_to_mb(total));
	return (sectors_to_mb(total));
}
