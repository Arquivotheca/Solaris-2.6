#ifndef lint
#pragma ident "@(#)v_pfg_lfs.c 1.8 96/05/15 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_pfg_lfs.c
 * Group:	ttinstall
 * Description:
 */

#include <string.h>

#include "pf.h"

static void setOrigDefaultMountEnt(Disk_t * diskPtr, int slice);
static void pfgCreateLayoutArray();
static void pfgNullDisk(Disk_t *ptr);
static Defmnt_t **getOrigDefaultMountList(MachineType type);

#ifndef True
#define	True 1
#endif

#ifndef False
#define	False 0
#endif

/*
 * Set the original default mount entry for a slice.  The
 * original default entry is determined based on machine type
 * This function is called if to reset the default mount entry
 * for a mount entry that was removed because the existing vtoc
 * information was read in from disk, or a file system that was
 * marked as preserved was unpreserved.
 */

static void
setOrigDefaultMountEnt(Disk_t * diskPtr, int slice)
{
	Defmnt_t **defaultMounts;
	Defmnt_t mountEnt;
	int i;

	if (get_dfltmnt_ent(&mountEnt, slice_mntpnt(diskPtr, slice)) == D_OK) {
		defaultMounts = getOrigDefaultMountList(get_machinetype());
		for (i = 0;
		    defaultMounts[i] &&
		    defaultMounts[i]->name != (char *) NULL;
		    i++) {
			if (strcmp(slice_mntpnt(diskPtr, slice),
				defaultMounts[i]->name) == 0) {
				mountEnt.status = defaultMounts[i]->status;
				break;
			}
		}
	}
}

/*
 * this function set size to zero and name to NULL for slices that
 * unpreserved from a previously preserved state
 */
void
pfgNullUnpres()
{
	Disk_t *ptr;
	int i;

	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		for (i = 0; i < NUMPARTS; i++) {
			if (comm_slice_preserved(ptr, i)) {
				if (slice_not_preserved(ptr, i)) {
					/*
					 * set size to 0, start is
					 * automatically set to 0
					 */
					set_slice_geom(ptr, i,
					    GEOM_IGNORE, 0);
					setOrigDefaultMountEnt(ptr,
					    i);
					set_slice_mnt(ptr, i,
					    "", NULL);
					if (slice_stuck(ptr, i)) {
						slice_stuck_off(ptr, i);
					}
				}
			}
		}
	}
}

/*
 * function that resets the names of non-preserved slices,
 * this is necessary since the user may have changed the slice's
 * mount point without preserving the slice.  In the case where
 * the mount point name was changed without preserving the slice
 * then the mount point should be reset to the commited mount point
 */
void
pfgResetNames()
{
	Disk_t *ptr;
	int i;

	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			for (i = 0; i < LAST_STDSLICE + 1; i++) {
				if (slice_not_preserved(ptr, i)) {
					/*
					 * don't reset name of non-preserved
					 * slice with NULL name and 0 size
					 */
					if ((slice_mntpnt(ptr, i) == NULL ||
						(strcmp(slice_mntpnt(ptr, i),
						"") == 0)) && 
						slice_size(ptr, i) == 0) {
						continue;
					}

					if (strcmp(slice_mntpnt(ptr, i),
					    comm_slice_mntpnt(ptr, i))) {
						set_slice_mnt(ptr, i,
						comm_slice_mntpnt(ptr, i),
						NULL);
					}
				}
			}
		}
	}
}



/*
 * Function to reset the default mount list for the current machine type.
 */
void
pfgResetDefaults()
{
	Defmnt_t **mountList;
	Disk_t *dp;
	int i;
	Defmnt_t mountEnt;

	mountList = getOrigDefaultMountList(get_machinetype());
	(void) set_dfltmnt_list(mountList);

	/* mark preserved filesystems as selected */
	for (dp = first_disk(); dp; dp = next_disk(dp)) {
		if (disk_selected(dp)) {
			for (i = 0; i < NUMPARTS; i++) {
				if (slice_preserved(dp, i)) {
					if (get_dfltmnt_ent(&mountEnt,
						slice_mntpnt(dp, i)) == D_OK) {
						mountEnt.status = DFLT_SELECT;
						set_dfltmnt_ent(&mountEnt,
						    slice_mntpnt(dp, i));
					}
				}
			}
		}
	}
}



/*
 * This function sets the install libraries default mount list for
 * manual layout.  This involves marking all default file systems that
 * don't exist on the disk as ignored, and the ones that do exist as
 * selected.  This is necessary so that the install library's size
 * checking algorithms function correctly
 */
void
pfgSetManualDefaultMounts()
{
	Defmnt_t **mountList = (Defmnt_t **) NULL;
	int i;

	mountList = get_dfltmnt_list(mountList);

	for (i = 0; mountList[i] != NULL; i++) {
		if (mountList[i]->allowed == 0)
			continue;
		if (find_mnt_pnt(NULL, NULL, mountList[i]->name,
		    (Mntpnt_t *) NULL, CFG_CURRENT) == True) {
			mountList[i]->status = DFLT_SELECT;
		} else {
			mountList[i]->status = DFLT_IGNORE;
		}
		(void) set_dfltmnt_ent(mountList[i], mountList[i]->name);
	}
	(void) free_dfltmnt_list(mountList);
}

/*
 * file global variables used to maintain a list
 * of disks in the current layout
 */

/*
 * array containing pointer to disks
 * that are currently layed out
 */
static Disk_t **LayOutDisks = NULL;

static int NumLayOutDisks = 0;

/*
 * This function creates an array for storing the list
 * of disks that are in the current layout.
 * The function counts the number of disks than mallocs
 * an array of disk pointers.
 */
static void
pfgCreateLayoutArray()
{
	int numDisks = 0;
	Disk_t *disk;

	if (LayOutDisks != NULL) {
		return; /* array has already been created */
	}

	for (disk = first_disk(); disk != NULL; disk = next_disk(disk)) {
		numDisks++;
	}

	LayOutDisks = (Disk_t **) xmalloc(numDisks * sizeof (Disk_t *));

}

/*
 * This function build the lists of disks in the current layout.
 * The list is need so that if the user attempt to continue after
 * previously doing a layout, any disks that have been added can
 * be determined by comparing the list of selected disk with the
 * list of disks that have been layed out.  Any disk that is selected
 * but has not been layed out will need to be nulled out.  This is
 * necessary because if the disk is nulled out than the existing file
 * systems on the disk will show up incorrectly in the layout
 */
void
pfgBuildLayoutArray()
{
	Disk_t *disk;

	if (LayOutDisks == NULL) { /* create array if it doesn't exist */
		pfgCreateLayoutArray();
	}

	NumLayOutDisks = 0;
	for (disk = first_disk(); disk; disk = next_disk(disk)) {
		if (disk_selected(disk)) {
			LayOutDisks[NumLayOutDisks] = disk;
			NumLayOutDisks++;
		}
	}
}

/*
 * This function compares the selected disks against the
 * disks that have been layed out to determine which disks
 * are selected but haven't been layed out.  If a disk is
 * found that is selected but isn't in the current layout
 * than that disk is nulled out.
 */

void
pfgCompareLayout()
{
	Disk_t *disk;
	int layedOut = False;
	int i;

	for (disk = first_disk(); disk; disk = next_disk(disk)) {
		if (disk_selected(disk)) {
			/*
			 * check if disk is in current layout
			 */
			if (LayOutDisks != NULL) {
				for (i = 0; i <= NumLayOutDisks; i++) {
					if (disk == LayOutDisks[i]) {
						layedOut = True;
						break;
					}
				}
			}
			if (layedOut == False) { /* disk is not in layout */
				pfgNullDisk(disk);
			}
			layedOut = False;
		}
	}
}		

/*
 * function to null out a single disk.
 * the function saves any preserve information
 */
static void
pfgNullDisk(Disk_t *ptr)
{
	int i;

	/*
	 * Null out the selected disk
	 */
	(void) SdiskobjConfig(LAYOUT_RESET, ptr, NULL);

	/*
	 * need to restore committed preserved slices
	 */
	for (i = 0; i < NUMPARTS; i++) {
		if (comm_slice_preserved(ptr, i)) {
			slice_stuck_on(ptr, i);
			set_slice_geom(ptr, i, GEOM_ORIG, GEOM_ORIG);
			set_slice_mnt(ptr, i, comm_slice_mntpnt(ptr, i), NULL);
			set_slice_preserve(ptr, i, PRES_YES);
		}
	}
}


static Defmnt_t **orig_server_fs = (Defmnt_t **) NULL;
static Defmnt_t **orig_stand_fs = (Defmnt_t **) NULL;

void
saveDefaultMountList(MachineType type)
{
	switch (type) {
		case MT_SERVER:
		if (orig_server_fs == (Defmnt_t **) NULL)
			orig_server_fs = get_dfltmnt_list(orig_server_fs);

		break;

	case MT_STANDALONE:
	default:
		if (orig_stand_fs == (Defmnt_t **) NULL)
			orig_stand_fs = get_dfltmnt_list(orig_stand_fs);

		break;
	}

}

Defmnt_t **
getOrigDefaultMountList(MachineType type)
{

	switch (type) {
		case MT_SERVER:
		return (orig_server_fs);
	case MT_STANDALONE:
		return (orig_stand_fs);
	default:
		return (NULL);
	}
}
