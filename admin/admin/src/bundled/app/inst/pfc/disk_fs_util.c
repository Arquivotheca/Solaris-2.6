#ifndef lint
#pragma ident "@(#)disk_fs_util.c 1.9 96/06/27 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	disk_fs_util.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include <sys/param.h>
#include <sys/types.h>

#include "pf.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_misc.h"

#include "disk_fs_util.h"

void
_commit_all_selected_disks(int n)
{
	register int i;

	for (i = 0; i < n; i++) {
		if (v_get_disk_selected(i) != 0)
			(void) v_commit_disk(i, V_IGNORE_ERRORS);
	}
}

void
_clear_all_selected_disks(int n)
{
	register int i;

	for (i = 0; i < n; i++) {
		if (v_get_disk_selected(i) != 0)
			(void) v_clear_disk(i);
	}
}

void
_reset_all_selected_disks(int n)
{
	register int i;

	for (i = 0; i < n; i++) {
		if (v_get_disk_selected(i) != 0)
			(void) v_unconfig_disk(i);
	}
}

void
_restore_all_selected_disks_commit(int n)
{
	register int i;

	for (i = 0; i < n; i++) {
		if (v_get_disk_selected(i) != 0)
			v_restore_disk_commit(i);
	}
}

#ifdef debug
static void
_print_required_space()
{
	int	n;
	int	i;
	int	reqd = 0;

	n = v_get_n_lfs();

	for (i = 0; i < n; i++)
		(void) fprintf(stdout, "%s: %d\r\n", v_get_lfs_mntpt(i),
		    (int) v_get_lfs_suggested_size(i));

}

#endif

int
_get_avail_space(ndisks)
{
	int avail;
	int i;

	/*
	 * for each disk available for use, add it's overall size to the
	 * usable total.
	 */
	for (i = 0, avail = 0; i < ndisks; i++) {

		if (v_get_disk_usable(i) == 1) {

			avail += v_get_sdisk_capacity(i);

		}
	}

	return (avail);
}

/*
 * calculates space `required' for a successful install. basically just
 * totals the `required' space for all the default file systems.
 */
int
_get_reqd_space()
{
	return (DiskGetContentMinimum());
}

int
_get_used_disks(int ndisks)
{
	int i;
	int used_disks;

	for (i = 0, used_disks = 0; i < ndisks; i++) {

		if (v_get_disk_usable(i) == 1) {

			++used_disks;

		}
	}

	return (used_disks);
}
