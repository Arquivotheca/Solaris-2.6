#ifndef lint
#pragma ident "@(#)v_pfg_disks.c 1.23 96/06/27 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc. All rights reserved.
 */
/*
 * Module:	v_pfg_disks.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "pf.h"
#include "pferror.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_lfs.h"

#ifndef	True
#define	True	1
#endif

#ifndef	False
#define	False	0
#endif

/* structure to temporarily save disk information */
typedef struct {
	int sliceSize;
	int sliceStart;
	char *sliceMount;
	char stuckState;
} TmpDiskStruct;

static	void pfgLayoutErr(void *dialog, int err);
static	TmpDiskStruct TmpDisk[NUMPARTS];

/*
 * temporary struct used to store
 * disk information during cylinder editing
 * it is used to reset disk if user cancels
 * edit changes
 */
/* static TmpDiskStruct TmpDisk[NUMPARTS]; */

#ifdef BUILD_GUI
pfErCode
pfLoadDisks(int *numDisks)
{

	if (DISKFILE(pfProfile)) {
		if (!(*numDisks = DiskobjInitList(DISKFILE(pfProfile))))
			return (pfErLOADVTOCFILE);
	} else {
		if (!(*numDisks = DiskobjInitList(NULL)))
			return (pfErFINDDISKS);
	}

	if (ResobjInitList() != D_OK)
		return (pfErNOKNOWNRESOURCES);

	pfPreserveFdisk();
	(void) sigignore(SIGSEGV); /* trap set in disk libs */
	(void) sigignore(SIGBUS); /* trap set in disk libs */
	return (pfOK);
}
#endif

void
pfgNullDisks()
{
	Disk_t *ptr;
	int i, err;

	/*
	 * Null out the selected disks
	 */
	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			err = SdiskobjConfig(LAYOUT_RESET, ptr, NULL);
			if (err != D_OK) {
				pfgLayoutErr(NULL, err);
			}
		}
	}
	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			/*
			 * need to restore committed preserved slices
			 */
			for (i = 0; i < NUMPARTS; i++) {
				if (comm_slice_preserved(ptr, i)) {
					slice_stuck_on(ptr, i);
					err = set_slice_geom(ptr, i,
					    GEOM_ORIG, GEOM_ORIG);
					if (err != D_OK) {
						pfgLayoutErr(NULL, err);
					}
					err = set_slice_mnt(ptr, i,
					    comm_slice_mntpnt(ptr, i), NULL);
					if (err != D_OK) {
						pfgLayoutErr(NULL, err);
					}
					err = set_slice_preserve(ptr, i,
					    PRES_YES);
					if (err != D_OK) {
						pfgLayoutErr(NULL, err);
					}
				}
			}
		}
	}
}

/* ARGSUSED */
static void
pfgLayoutErr(void * dialog, int err)
{
	write_debug(CUI_DEBUG_L1,
		"****************LAYOUT ERROR*****************");
}

/*
 * function to determine if disk has been modified. Returns 1 if modified,
 * 0 if not modified
 */

int
IsDiskModified(char *diskName)
{
	Disk_t *ptr;
	int i;

	ptr = find_disk(diskName);
	for (i = 0; i < LAST_STDSLICE; i++) {
		if (slice_size(ptr, i) != orig_slice_size(ptr, i)) {
			return (1);
		} else if (slice_start(ptr, i) != orig_slice_start(ptr, i)) {
			return (1);
		} else if (strcmp(slice_mntpnt(ptr, i),
			orig_slice_mntpnt(ptr, i))) {
			return (1);
		}
	}
	return (0);
}
/*
 * determine if default boot disk is selected
 */
int
pfgIsBootSelected()
{
	Disk_t *bootDisk;

	if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) != D_OK ||
			bootDisk == NULL)
		return (True);

	return (disk_selected(bootDisk));
}


void
pfgResetDisks()
{
	Disk_t *d;

	for (d = first_disk(); d; d = next_disk(d)) {
		if (disk_selected(d)) {
			restore_disk(d, CFG_COMMIT);
		}
	}
}


void
pfgCommitDisks()
{
	Disk_t *ptr;
	int err;

	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			err = commit_disk_config(ptr);
			if (err != D_OK) {
				(void) printf("invalid disk settings\n");
			}
		}
	}
}


int
pfgIsBootDrive(Disk_t *disk)
{
	char	name[32];

	if (BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, name,
			NULL) == D_OK && name[0] != NULL &&
			streq(name, disk_name(disk))) {
		return (True);
	} else {
		return (False);
	}
}


void
saveDiskConfig(Disk_t *diskPtr)
{
	int i;

	for (i = 0; i < LAST_STDSLICE + 1; i++) {
		TmpDisk[i].sliceSize = slice_size(diskPtr, i);
		TmpDisk[i].sliceStart = slice_start(diskPtr, i);
		TmpDisk[i].sliceMount = xstrdup(slice_mntpnt(diskPtr, i));
		TmpDisk[i].stuckState = (char) slice_stuck(diskPtr, i);
	}
}


void
restoreDiskConfig(Disk_t *diskPtr)
{
	int i;

	for (i = 0; i < LAST_STDSLICE + 1; i++) {
		set_slice_geom(diskPtr, i, TmpDisk[i].sliceStart,
		    TmpDisk[i].sliceSize);
		set_slice_mnt(diskPtr, i, TmpDisk[i].sliceMount, NULL);
		if (TmpDisk[i].stuckState) {
			slice_stuck_on(diskPtr, i);
		} else {
			slice_stuck_off(diskPtr, i);
		}
	}
}


void
pfgLoadExistingDisk(Disk_t *diskPtr)
{
	int i;
	int err;
	Defmnt_t mountEnt;

	/*
	 * reset default mountpoint masks
	 */
	for (i = 0; i <= LAST_STDSLICE; i++) {
		if (get_dfltmnt_ent(&mountEnt, slice_mntpnt(diskPtr, i))
		    == D_OK) {
			mountEnt.status = DFLT_IGNORE;
			write_debug(CUI_DEBUG_L1, "status is ignored");
			set_dfltmnt_ent(&mountEnt, slice_mntpnt(diskPtr,
			    i));
		}
	}
	/* load existing vtoc info */
	(void) SdiskobjConfig(LAYOUT_EXIST, diskPtr, NULL);

	for (i = 0; i <= LAST_STDSLICE; i++) {
		if (comm_slice_preserved(diskPtr, i)) {
			slice_stuck_on(diskPtr, i);
			err = set_slice_geom(diskPtr, i, GEOM_ORIG, GEOM_ORIG);
			if (err != D_OK) {
				pfgLayoutErr(NULL, err);
			}
			err = set_slice_mnt(diskPtr, i,
			    comm_slice_mntpnt(diskPtr, i), NULL);
			if (err != D_OK) {
				pfgLayoutErr(NULL, err);
			}
			err = set_slice_preserve(diskPtr, i, PRES_YES);
			if (err != D_OK) {
				pfgLayoutErr(NULL, err);
			}
		}
	}
	for (i = 0; i <= LAST_STDSLICE; i++) {
		if (get_dfltmnt_ent(&mountEnt, slice_mntpnt(diskPtr, i))
		    == D_OK) {
			mountEnt.status = DFLT_SELECT;
			write_debug(CUI_DEBUG_L1, "status is ignored");
			set_dfltmnt_ent(&mountEnt, slice_mntpnt(diskPtr,
			    i));
		}
	}
}


/*
 * Description:
 *  This function configures the disks with the default configuration
 *  generated by the disk library
 */
int
pfgInitializeDisks()
{
	Disk_t *ptr, *bootDisk;
	int err;

	/*
	 * determine if boot drive is set and selected, if not make first
	 * selected drive the boot drive
	 */
	if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) != D_OK ||
			bootDisk == NULL || disk_not_selected(bootDisk)) {
		WALK_DISK_LIST(ptr) {
			if (disk_selected(ptr)) {
				if (BootobjSetAttribute(CFG_CURRENT,
					    BOOTOBJ_DISK, disk_name(ptr),
					    BOOTOBJ_DEVICE, -1,
					    NULL) == D_OK) {
					break;
				}
			}
		}
	}
	pfgCommitDisks();
	err = SdiskobjAutolayout();

	/*
	 * loop through disk list and allocate unused disk space
	 * for selected disks.
	 */
	if (err == D_OK) {
		for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
			if (disk_selected(ptr)) {
				SdiskobjAllocateUnused(ptr);
			}
		}
	}

	return (err);
}

/*
 * Check the disk status and exit with an error message if
 * there's a problem, otherwise start the parade.
 */
void
pfCheckDisks(void) {
	Disk_t *dp;
	int ndisks;

	ndisks = v_get_n_disks();

	/* Are there any known disks? */
	if (ndisks == 0) {
		wnoutrefresh(stdscr);
		(void) simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_WARNING,
			APP_ER_NOKNOWNDISKS);
		tty_cleanup();
		exit(EXIT_INSTALL_FAILURE);
	}

	/* Even if disks are known, are they usable? */
	WALK_DISK_LIST(dp) {
		if (disk_okay(dp))
			break;
	}
	if (dp == NULL) {
		wnoutrefresh(stdscr);
		(void) simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_WARNING,
			APP_ER_NOUSABLEDISKS);
		tty_cleanup();
		exit(EXIT_INSTALL_FAILURE);
	}
}
