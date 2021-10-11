#ifndef lint
#pragma ident "@(#)v_pfg_fdisk.c 1.6 96/05/09 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_pfg_fdisk.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgdisks.h"

void
pfPreserveFdisk(void)
{
	Disk_t *ptr;
	int i;

	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_fdisk_req(ptr)) {
			if (!fdisk_no_flabel(ptr)) {
				if (select_disk(ptr, NULL) == D_OK) {
					for (i = 1; i <= FD_NUMPART; i++) {
						set_part_preserve(ptr, i,
						    PRES_YES);
					}
					commit_disk_config(ptr);
					deselect_disk(ptr, NULL);
				}
			}
		}
	}
}

int
useEntireDisk(Disk_t *diskPtr)
{
	int err;
	int i;

	/* check for partitions that are inuse */
	for (i = 1; i <= FD_NUMPART; i++) {
		if (part_id(diskPtr, i) != UNUSED) {
			if (pfgQuery(solarpart_dialog, pfQFDISKCHANGE)
				== False) {
				return (FAILURE);
			} else {
				break;
			}
		}
	}
	(void) FdiskobjConfig(LAYOUT_RESET, diskPtr, NULL);
	err = FdiskobjConfig(LAYOUT_DEFAULT, diskPtr, NULL);
	return (err);
}

int
useLargestPart(Disk_t *diskPtr)
{
	int part, maxSize = 0, err;

	/* find largest free hole */
	getLargestPart(diskPtr, &maxSize, &part);

	if (maxSize > 0) {
		err = set_part_attr(diskPtr, part, SUNIXOS, GEOM_IGNORE);
		if (err != D_OK) {
			pfgDiskError(solarpart_dialog, NULL, err);
			return (err);
		}
		err = set_part_geom(diskPtr, part, GEOM_IGNORE, maxSize);
		if (err != D_OK) {
			pfgDiskError(solarpart_dialog, NULL, err);
		}
	} else {
		pfgWarning(solarpart_dialog, pfErNOSPACE);
		err = -1;
	}
	return (err);
}

void
getLargestPart(Disk_t *diskPtr, int *maxSize, int *part)
{
	int i, size;

	*maxSize = 0;
	for (i = 1; i <= FD_NUMPART; i++) {
		if ((size = max_size_part_hole(diskPtr, i)) > *maxSize &&
		    part_id(diskPtr, i) == UNUSED) {
			*maxSize = size;
			*part = i;
		}
	}
}
