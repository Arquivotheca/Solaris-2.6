#ifndef lint
#pragma ident "@(#)app_lfs.c 1.6 96/04/27 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_lfs.c
 * Group:	libspmiapp
 * Description:
 *	Application library level local file system handling routines.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "spmicommon_api.h"
#include "spmistore_api.h"
#include "spmiapp_api.h"
#include "app_utils.h"

#include "app_strings.h"

/*
 * Function:	any_preservable_filesystems
 * Description:
 *	A function used by both installtool and ttinstall to determine
 *	if there are any preservable file systems on the system being
 *	installed.
 * Scope:	PUBLIC
 * Parameters:  None
 * Return:	[int]
 *	0 - No Preservable file systems found
 *	1 - Preservable file systems found
 */

int
any_preservable_filesystems(void)
{
	int	i;
	Disk_t	*dp;

	WALK_DISK_LIST(dp) {
		if (disk_selected(dp)) {
			/*
			 * check the sdisk geometry to make sure it is not NULL
			 */
			if (sdisk_geom_null(dp))
				continue;
			/*
			 * check the sdisk geometry, if existing
			 * differs from current
			 * then the geometry has changed and file systems
			 * cannot be preserved
			 */
			if (sdisk_geom_same(dp, CFG_EXIST) != D_OK)
					continue;
			/*
			 * if you get this far then the geometry
			 * check has passed for each
			 * disk, now check the size of each slice to see if it may
			 * contain data (size > 0)
			 *
			 * if the size of a slice is greater than zero, this
			 * slice can be preserved
			 */
			WALK_SLICES(i) {
				if ((orig_slice_size(dp, i) > 0)
					&& !slice_locked(dp,i))
						return(1);
			}
		}
	}

	/*
	 * else no preservable file systems were found (size <= 0)
	 */
	return (0);

}
