#ifndef lint
#pragma ident "@(#)v_sdisk.c 1.65 96/10/02 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_sdisk.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <sys/fs/ufs_fs.h>

#include "pf.h"
#include "v_types.h"
#include "spmisvc_api.h"
#include "spmiapp_strings.h"
#include "v_misc.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_disk_private.h"

/*
 * This file contains the View interface layer to the `sdisk' functionality
 * in the underlying install disk library.
 */

/* Global variables: */
int v_errno = 0;

/*
 * disk code defines the number of partitions on a disk, need to propogate
 * this into the UI code, but don't want to expose the underlying disk
 * library, so must resort to this global variable
 *
 * NUMPARTS should be defined in disk.h
 */
#ifdef NUMPARTS
int N_Slices = LAST_STDSLICE + 1;
#else
int N_Slices = 8;
#endif

/*
 * alts and overlap are keywordws to the disk library denoting slices
 * which required special treatment.  Provide access to the keywords to
 * the UI code by providing global variables pointing to the names.
 *
 * ALTS and OVERLAP should be defined in disk.h
 */
#ifdef ALTS
char *Alts = ALTS;
#else
char *Alts = "alts";
#endif

#ifdef OVERLAP
char *Overlap = OVERLAP;
#else
char *Overlap = "overlap";
#endif

/*
 * global data/structures shared between v_sdisk.c and v_fdisk.c
 *
 * accessed through v_disk_private.h
 *
 */
V_Disk_t *_disks;
Disk_t *_current_disk = (Disk_t *) NULL;
int _current_disk_index = -1;
int _num_disks = -1;

/*
 * static v_sdisk.c variables ...
 */
static V_Units_t _default_units = V_MBYTES;
static int _showcyls = FALSE;
static int _overlap = FALSE;

/*
 * v_get_first_disk(void)
 *
 * Provides interface to the routines which probe the system for disks.
 *
 *	If the disk list head ptr is NULL, call the disk library
 *	   routine which probes for disks and initializes the list.
 *  returns pointer to head of disk list
 */
Disk_t *
v_get_first_disk(void)
{
	if (first_disk() == NULL) {
		if (GetSimulation(SIM_SYSDISK))
			(void) DiskobjInitList(DISKFILE(pfProfile));
		else
			(void) DiskobjInitList(NULL);
		(void) ResobjInitList();
	}

	return (first_disk());
}

/*
 * v_init_disks(void)
 *
 * Initializes the View's array of disk structures
 *
 * input:
 * returns:
 *	number of disks probed out.
 *
 * algorithm:
 *	if necessary, initializes the View's list of disks.
 *	Traverse the list of disks: store each disk structure into
 *	our own array, this is so we can associate some state
 *	information with each disk that is Install specific.
 *	While traversing the list, keep count of the number of disks,
 *	Store the number of disks into the global variable '_num_disks'
 *	return the number of disks.
 */
int
v_init_disks(void)
{
	int i, index;
	Disk_t *tmp;

	/*
	 * count number of disks
	 */
	for (i = 0, tmp = v_get_first_disk(); tmp; i++, tmp = next_disk(tmp));

	_num_disks = i;

	/*
	 * create an array to hold the number of disks + a terminating NULL
	 * struct
	 */
	_disks = (V_Disk_t *) xcalloc((_num_disks + 1) * sizeof (V_Disk_t));

	/*
	 * disk library keeps three different states of configuration info:
	 * 'orig', `commit' and `current'...
	 *
	 * initial `current' state of disk comes from `orig', really want to
	 * zero out current state.
	 *
	 */
	for (i = 0, tmp = first_disk();
	    tmp && (i < _num_disks);
	    i++, tmp = next_disk(tmp)) {

		_disks[i].status = V_DISK_LIMBO;
		_disks[i].info = tmp;

		/*
		 * check to see if disk is in a known bad state.
		 *
		 * if it isn't, then it is possible to use it, clear out
		 * CURRENT and COMMITED registers and set initial  status to
		 * `not used'
		 */
		if (disk_not_okay(tmp) || disk_unusable(tmp)) {
			_disks[i].status = V_DISK_NOTOKAY;
		} else {

			(void) select_disk(_disks[i].info, "");

			for (index = 1; index <= FD_NUMPART; index++) {
				(void) set_part_preserve(_disks[i].info, index,
					PRES_YES);
			}
			(void) v_unconfig_disk(i);
			(void) deselect_disk(_disks[i].info, "");
		}

	}

	_current_disk_index = 0;

	return (_num_disks);
}

/*
 * v_get_n_disks(void)
 *
 * Provides access to the number of disks currently known to the disk
 *   library and the `view' layer.
 *
 * input:
 * returns:
 *	number of disks.
 *
 * algorithm:
 */
int
v_get_n_disks(void)
{
	if (_num_disks == -1)
		(void) v_init_disks();

	return (_num_disks);
}

/*
 * v_get_disk_index_from_name(char *name)
 *  retrieves index of disk with `name' or -1 if not found
 *
 * input:
 *	name of disk of interest
 *
 * returns:
 *  index of disk if found
 *	-1 if matching disk not found
 *
 * algorithm:
 */
int
v_get_disk_index_from_name(char *name)
{
	int i;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	for (i = 0; i < _num_disks; i++)
		if (strcmp(disk_name(_disks[i].info), name) == 0)
			return (i);

	return (-1);
}

/*
 * v_get_disk_status(i)
 *  retrieves status of i'th disk.
 *
 * input:
 *	index of disk of interest.
 *
 * returns:
 * 	V_DISK_UNEDITED if index is out of bounds or if the disk info pointer
 * 	is scrambled.  'real' disk status otherwise.
 *
 * algorithm:
 */
V_DiskStatus_t
v_get_disk_status(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (V_DISK_UNEDITED);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_DISK_UNEDITED);

	return (_disks[i].status);
}

/*
 * v_get_disk_status_str(i)
 *  retrieves status of i'th disk as a string
 *
 * input:
 *	index of disk of interest.
 *
 * returns:
 *  pointer to static buffer with disk's status
 *
 * algorithm:
 */
char *
v_get_disk_status_str(int i)
{
	static char buf[128];

	(void) strcpy(buf, DISK_ERROR_NO_INFO);

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (buf);

	if (_disks[i].info == (Disk_t *) NULL)
		return (buf);

	/*
	 * note that the messages for `DISK_PREP_CANT_FORMAT' &
	 * DISK_PREP_NOPGEOM are the same.  this is because the different
	 * error conditions result in the same net problem, but are arrived
	 * at slightly differently.  More detailed error/recovery text may
	 * be provided in the future for the different conditions, so make
	 * them separate messages
	 */
	if (_disks[i].status == V_DISK_NOTOKAY) {

		/*
		 * NOTOKAY is a generic problem.
		 *
		 * see if we can find more detailed info about the disk and its
		 * problem
		 */
		if (disk_bad_controller(_disks[i].info)) {
			(void) strcpy(buf, DISK_PREP_BAD_CONTROLLER);
		} else if (disk_unk_controller(_disks[i].info)) {
			(void) strcpy(buf, DISK_PREP_UNKNOWN_CONTROLLER);
		} else if (disk_cant_format(_disks[i].info)) {
			(void) strcpy(buf, DISK_PREP_CANT_FORMAT);
		} else if (disk_no_pgeom(_disks[i].info)) {
			(void) strcpy(buf, DISK_PREP_NOPGEOM);
		}
	}
	return (buf);
}

/*
 * v_set_disk_selected(int i, int use)
 * 	set i'th disk selected state
 * 	is i'th disk to be used (: available for auto-configuration)?
 *
 * input:
 *	index of disk of interest.
 *
 * return:
 *	V_OK - disk selected status set OK.
 *	V_FAILURE - disk selected status not set OK
 *
 * algorithm:
 */
int
v_set_disk_selected(int i, int use)
{
	int err;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (V_FAILURE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	if (use)
		err = select_disk(_disks[i].info, "");
	else
		err = deselect_disk(_disks[i].info, "");

	if (err != D_OK)
		return (V_FAILURE);
	else
		return (V_OK);
}

/*
 * v_get_disk_selected(int i)
 * 	get i'th disk selected state
 *
 * input:
 *	index of disk of interest.
 *
 * returns:
 * 	current state of disks's `selected' bit... 1 or 0
 *
 * algorithm:
 */
int
v_get_disk_selected(int i)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (TRUE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (TRUE);

	return (disk_selected(_disks[i].info) ? 1 : 0);

}

/*
 * v_get_disk_usable(int i)
 *  get i'th disk usable state (selected and has Solaris partition)
 *
 * input:
 *  index of disk of interest.
 *
 * returns:
 *  1 - if disk is selected and can be used (has editable sdisk geometry)
 *  0 - if disk can not be used
 *
 * algorithm:
 */
int
v_get_disk_usable(int i)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (TRUE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (TRUE);

	return (sdisk_is_usable(_disks[i].info) ? 1 : 0);

}

/*
 * v_is_bootdrive(int i)
 *
 *  is i'th disk the currently marked bootdrive?
 *
 * input:
 *  index of disk of interest.
 *
 * returns:
 *  1 - if disk is the bootdrive
 *  0 - if disk is not
 *
 * algorithm:
 */
int
v_is_bootdrive(int i)
{
	Disk_t *    dp;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == NULL)
		return (0);

	if (DiskobjFindBoot(CFG_CURRENT, &dp) == D_OK && dp != NULL &&
			streq(disk_name(dp), disk_name(_disks[i].info))) {
		write_debug(CUI_DEBUG_L1, "disk is boot disk");
		return (1);
	}

	write_debug(CUI_DEBUG_L1, "disk is not boot disk");

	return (0);

}

/*
 * v_get_default_bootdrive_name(void)
 *
 *  what is the name of the default bootdrive?
 *
 * input:
 *  index of disk of interest.
 *
 * returns:
 *  1 - if disk is the bootdrive
 *  0 - if disk is not
 *
 * algorithm:
 */
char *
v_get_default_bootdrive_name(void)
{
	Disk_t *	dp;

	if (DiskobjFindBoot(CFG_EXIST, &dp) == D_OK && dp != NULL)
		return (disk_name(dp));
	else
		return (NULL);
}

/*
 * v_alt_slice_reqd(int disk)
 *
 * 	does this disk have/need an alternate sector slice?
 *
 * returns:
 *	0 - if disk does not have/need alt sector slice
 *	!0 - if it does.
 */
int
v_alt_slice_reqd(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	return (get_minimum_fs_size(ALTSECTOR, _disks[i].info, DONTROLLUP));
}

/*
 * v_get_disk_slices_intact(int i)
 *  get i'th disk's Solaris partition's 'intactness'
 *
 * input:
 *  index of disk of interest.
 *
 * returns:
 *	returns 1 if the disk's geometry  has not been changed
 *	(if it has been changed, then the original slice information is
 *	 lost.)
 *
 *	returns 0 if disk is not intact.
 *
 * algorithm:
 */
int
v_get_disk_slices_intact(int i)
{

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (TRUE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (TRUE);

	return (sdisk_geom_same(_disks[i].info, CFG_COMMIT) == D_OK);

}

/*
 * v_set_current_disk(i)
 *  sets view's concept of the current disk.
 *
 * needed a way to remove any context of a current disk.
 * this allows us to recalculate the default file system sizes
 * independent of disk geometry.  This is necessary when looping
 * or going back in the parade sequence.
 *
 * input:
 *	index of new current disk
 *
 * returns:
 *
 *	V_FAILURE if index is out of bounds (< 0 || > _num_disks)
 *	(V_OK) otherwise.
 *
 * algorithm:
 */
V_Status_t
v_set_current_disk(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i == -1) {
		_current_disk = (Disk_t *) NULL;
		_current_disk_index = i;
		return (V_OK);
	}

	if (i >= _num_disks || i < 0)
		return (V_FAILURE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	_current_disk = _disks[i].info;
	_current_disk_index = i;

	return (V_OK);

}

/*
 * v_get_current_disk()
 *  returns view's concept of the current disk.
 *
 * input:
 *
 * returns:
 *	index of current disk
 *
 * algorithm:
 */
int
v_get_current_disk(void)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	return (_current_disk_index);

}

/*
 * v_int_get_current_disk_ptr(void)
 *  returns view's concept of the current disk as a pointer, this
 *  is an internal function and should only be used within the view
 *  layer.
 *
 * input:
 *
 * returns:
 *	index of current disk
 *
 * algorithm:
 */
Disk_t *
v_int_get_current_disk_ptr()
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	return (_current_disk);

}

/*
 * v_get_disk_name(i)
 *
 * input:    index of disk of interest
 * returns:  pointer to i'th disk's name (eg., "c0t0d0")
 *		NULL pointer on failure.
 *
 * algorithm:
 */
char *
v_get_disk_name(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return ((char *) NULL);

	if (_disks[i].info == (Disk_t *) NULL)
		return ((char *) NULL);

	return (disk_name(_disks[i].info));

}

/*
 * v_get_sdisk_capacity(i)
 *
 * Capactiy of solaris partition
 *
 * input:    index of disk of interest
 * returns:  i'th disk's solaris partition size in current units
 *		0 on error.
 */
int
v_get_sdisk_capacity(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	if (sdisk_geom_null(_disks[i].info))
		return (0);

	if (_disks[i].status == V_DISK_NOTOKAY)
		return (0);

	return ((int) blocks2size(_disks[i].info,
		usable_sdisk_blks(_disks[i].info), ROUNDDOWN));
}

/*
 * v_get_disk_capacity(i)
 *
 * Capactiy of entire disk
 *
 * input:    index of disk of interest
 * returns:  i'th disk's size in current units
 *		V_FAILURE on error.
 */
int
v_get_disk_capacity(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	if (_disks[i].status == V_DISK_NOTOKAY)
		return (0);

	return ((int) blocks2size(_disks[i].info,
		usable_disk_blks(_disks[i].info), ROUNDDOWN));
}

/*
 * v_get_sdisk_size(i)
 *
 * True virtual disk size: size of entire solaris partition, including
 *	`extra' slices for boot slice and alt cyl slice
 *
 * input:    index of disk of interest
 * returns:  i'th disk's size in current units
 *		V_FAILURE on error.
 */
int
v_get_sdisk_size(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	if (_disks[i].status == V_DISK_NOTOKAY)
		return (0);

	return (blocks2size(_disks[i].info,
		total_sdisk_blks(_disks[i].info), ROUNDDOWN));
}

/*
 * v_get_disk_size(i)
 *
 * True disk size: size of entire disk.
 *
 * input:    index of disk of interest
 * returns:  i'th disk's size in current units
 *		V_FAILURE on error.
 */
int
v_get_disk_size(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	if (_disks[i].status == V_DISK_NOTOKAY)
		return (0);

	return (blocks2size(_disks[i].info,
		usable_disk_blks(_disks[i].info), ROUNDDOWN));
}

/*
 * v_get_disk_mountpts(i)
 *
 * input:    index of disk of interest
 * returns:  pointer to static buffer with i'th disk's file systems,
 *		catenated together
 *		pointer to empty buffer on error.
 */
char *
v_get_disk_mountpts(int i)
{
	static char buf[256];
	char *mount;
	int len;
	int j;

	buf[0] = '\0';

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (buf);

	if (_disks[i].info == (Disk_t *) NULL)
		return (buf);

	len = 255;
	/*
	 * catenate file system names
	 *
	 * buf -> "/, /usr, "
	 */
	for (j = 0; j < N_Slices; j++) {

		mount = slice_mntpnt(_disks[i].info, j);

		/*
		 * skip overlapped or alternate sector slices
		 */
		if (strcmp(mount, Alts) == 0 || strcmp(mount, Overlap) == 0)
			continue;

		if (mount && *mount && len > (int) (strlen(mount) + 1)) {

			(void) strcat(buf, mount);
			(void) strcat(buf, ", ");
			len -= (strlen(mount) + 2);

		} else if (mount && *mount) {

			(void) strncat(buf, mount, len - 4);
			(void) strcat(buf, "...>");

		}
	}

	/*
	 * buf -> "/, /usr, " zap
	 */
	len = strlen(buf);
	if (buf[len - 2] == ',')
		buf[len - 2] = '\0';

	return (buf);
}

/*
 * v_get_space_avail(i)
 *
 * input:    index of disk of interest
 * returns:  i'th disk's unallocated space in current units
 *		V_FAILURE on error.
 */
int
v_get_space_avail(int i)
{
	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (0);

	if (_disks[i].info == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_current_disk,
		sdisk_space_avail(_disks[i].info), ROUNDDOWN));
}

/*
 * v_get_disp_units_str()
 *
 * input:
 * returns: pointer to static buffer containining I18N string
 *		representation of current units
 */
char *
v_get_disp_units_str(void)
{
	static char buf[64];

	buf[0] = '\0';

	switch (get_units()) {
	case D_MBYTE:
		(void) strcpy(buf, gettext("MB"));
		break;

	case D_KBYTE:
		(void) strcpy(buf, gettext("KB"));
		break;

	case D_CYLS:
		(void) strncpy(buf, gettext("Cyls"), 63);
		break;

	case D_BLOCK:
		(void) strncpy(buf, gettext("Blks"), 63);
		break;

	default:
		(void) strcpy(buf, gettext("MB"));
		break;

	}

	return (buf);
}

/*
 * v_get_cur_start_cyl(i)
 *
 * returns:  starting cylinder for current state of i'th slice on current disk
 */
int
v_get_cur_start_cyl(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (slice_start(_current_disk, i));
}

/*
 * v_get_orig_start_cyl(i)
 *
 * returns:  starting cylinder for original state of i'th slice on current disk
 */
int
v_get_orig_start_cyl(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (orig_slice_start(_current_disk, i));
}

/*
 * v_get_comm_start_cyl(i)
 *
 * returns:  starting cylinder for committed state of i'th slice on current disk
 */
int
v_get_comm_start_cyl(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (comm_slice_start(_current_disk, i));
}

/*
 * v_get_cur_end_cyl(i)
 *
 * returns:  ending cylinder for current state of i'th slice on current disk
 */
int
v_get_cur_end_cyl(int i)
{
	Units_t old_units;
	int end_cyl;

	/*
	 * must save current display units and reset later. Need to get
	 * slice size in cylinders and add to start cylinder to compute end
	 * cylinder.
	 */

	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);
	else {

		old_units = set_units(D_CYLS);

		end_cyl = slice_start(_current_disk, i) +
		    blocks2size(_current_disk, slice_size(_current_disk, i),
			ROUNDUP);
		end_cyl = (end_cyl == 0) ? 0 : end_cyl - 1;

		(void) set_units(old_units);

		return (end_cyl);

	}
}

/*
 * v_get_cur_size(i)
 *
 * returns:  size for current state of i'th slice on current disk.
 *		size in current display units.
 */
int
v_get_cur_size(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_current_disk,
	    slice_size(_current_disk, i), ROUNDDOWN));
}

/*
 * v_get_orig_size(i)
 *
 * returns:  size for original state of i'th slice on current disk.
 *		size in current display units.
 */
int
v_get_orig_size(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_current_disk,
		orig_slice_size(_current_disk, i), ROUNDDOWN));
}

/*
 * v_get_comm_size(i)
 *
 * returns:  size for commited state of i'th slice on current disk.
 *		size in current display units.
 */
int
v_get_comm_size(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (blocks2size(_current_disk,
		comm_slice_size(_current_disk, i), ROUNDDOWN));
}
/*
 * v_get_cur_mount_pt(i)
 *
 * returns:  pointer to mount point for current state of i'th slice on
 *		current disk
 */
char *
v_get_cur_mount_pt(int i)
{
	if (invalid_sdisk_slice(i))
		return ((char *) NULL);

	if (_current_disk == (Disk_t *) NULL)
		return ((char *) NULL);

	return (slice_mntpnt(_current_disk, i));
}

/*
 * v_get_lock_state(i)
 *
 * returns:  the slice_locked state of the i'th slice on
 * 		current disk
 */
int
v_get_lock_state(int i)
{
	if (invalid_sdisk_slice(i))
		return (NULL);

	if (_current_disk == (Disk_t *) NULL)
		return (NULL);

	return (slice_locked(_current_disk, i));
}

/*
 * v_get_orig_mount_pt(i)
 *
 * returns:  pointer to mount point for original state of i'th slice on
 * 		current disk
 */
char *
v_get_orig_mount_pt(int i)
{
	if (invalid_sdisk_slice(i))
		return ((char *) NULL);

	if (_current_disk == (Disk_t *) NULL)
		return ((char *) NULL);

	return (orig_slice_mntpnt(_current_disk, i));
}

/*
 * v_get_comm_mount_pt(i)
 *
 * returns:  pointer to mount point for commited state of i'th slice on
 * 		current disk
 */
char *
v_get_comm_mount_pt(int i)
{
	if (invalid_sdisk_slice(i))
		return ((char *) NULL);

	if (_current_disk == (Disk_t *) NULL)
		return ((char *) NULL);

	return (comm_slice_mntpnt(_current_disk, i));
}

/*
 * v_get_cur_preserved(i)
 *
 * returns:  returns preserved boolean for current state of i'th slice
 * on current disk
 */
int
v_get_cur_preserved(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (slice_preserved(_current_disk, i));
}

/*
 * v_get_comm_preserved(i)
 *
 * returns:  returns preserved boolean for last committed state of i'th slice
 * on current disk
 */
int
v_get_comm_preserved(int i)
{
	if (invalid_sdisk_slice(i))
		return (0);

	if (_current_disk == (Disk_t *) NULL)
		return (0);

	return (comm_slice_preserved(_current_disk, i));
}

/*
 * v_set_start_cyl(i, val, overlap)
 *
 * function: set starting cylinder in i'th slice to val
 * 	do *not* allow disk library to adjust starting cylinders of other
 *	slices (GEOM_IGNORE).
 *
 *	overlap indicates if we're going to allow disk library to adjust
 *	starting cylinders of all slices to make them contiguous.
 *
 * returns:  (V_OK) on success
 *		V_FAILURE and sets v_errno
 */
V_Status_t
v_set_start_cyl(int i, int val, int overlap)
{
	int adjust;

	if (invalid_sdisk_slice(i))
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	/*
	 * if overlap is true, we don't want to force the slices to be
	 * contiguous, so we explicitly turn off auto_adjust() in the disk
	 * lib
	 *
	 * if overlap is false, we want to force the slices to be contiguous,
	 * so we let the disk library auto_adjust.
	 *
	 */
	if (overlap != 0) {

		slice_stuck_on(_current_disk, i);
		adjust = set_slice_autoadjust(0);

		v_errno = set_slice_geom(_current_disk, i, val, GEOM_IGNORE);

		slice_stuck_off(_current_disk, i);
		(void) set_slice_autoadjust(adjust);

		if (v_errno != D_OK) {
			return (V_FAILURE);
		}
	} else {

		if (v_errno = set_slice_geom(_current_disk, i, val,
		    GEOM_IGNORE))
			return (V_FAILURE);

	}

	return (V_OK);
}

/*
 * v_restore_orig_size(i)
 *
 * function: restores size on i'th slice to it's `true' original size.
 *
 *	v_set_size() introduces roudning errors which affect the preserve
 *	testing logic in the disk library.  Need to avoid them using this
 *	function.
 *
 */
V_Status_t
v_restore_orig_size(int i)
{
	int adjust;

	if (invalid_sdisk_slice(i))
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	slice_stuck_on(_current_disk, i);
	adjust = set_slice_autoadjust(0);

	v_errno = set_slice_geom(_current_disk, i, GEOM_IGNORE,
	    orig_slice_size(_current_disk, i));

	(void) set_slice_autoadjust(adjust);
	slice_stuck_off(_current_disk, i);

	if (v_errno != D_OK)
		return (V_FAILURE);
	else
		return (V_OK);
}

/*
 * v_set_size(i, val, overlap)
 *
 * function: sets size on i'th slice to val
 *
 *	val is either in MB or Cyls, need to convert to blocks before passing
 *  into disk library.
 *
 *	overlap indicates if we're going to allow disk library to adjust
 *	starting cylinders of all slices to make them contiguous.
 *
 * returns:  (V_OK)
 * 		V_FAILURE and sets v_errno
 */
V_Status_t
v_set_size(int i, int val, int overlap)
{
	int adjust;
	int blocks;

	if (invalid_sdisk_slice(i))
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	blocks = size2blocks(_current_disk, val);

	/*
	 * two different ways of setting the size. if overlap is true, we
	 * don't want to force the slices to be contiguous, so we explicitly
	 * turn off auto_adjust() in the disk lib
	 *
	 * if overlap is false, we want to force the slices to be contiguous,
	 * so we let the disk library auto_adjust.
	 *
	 */
	if (overlap != 0) {

		slice_stuck_on(_current_disk, i);
		adjust = set_slice_autoadjust(0);

		v_errno = set_slice_geom(_current_disk, i, GEOM_IGNORE, blocks);

		(void) set_slice_autoadjust(adjust);
		slice_stuck_off(_current_disk, i);

		if (v_errno != D_OK)
			return (V_FAILURE);
	} else {

		if ((v_errno = set_slice_geom(_current_disk, i, GEOM_IGNORE,
			    blocks)) != D_OK)
			return (V_FAILURE);

	}

	return (V_OK);
}

/*
 * v_set_mount_pt(slice, val)
 *
 * function: sets mount point on i'th slice to val
 * returns:  (V_OK)
 *		V_FAILURE and sets v_errno
 */
V_Status_t
v_set_mount_pt(int slice, char *val)
{
	if (slice >= N_Slices || slice < 0)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	if (val == (char *) NULL)
		val = "";

	if ((int) strlen(val) > MAXMNTLEN - 1) {
		v_errno = V_PATH_TOO_LONG;
		return (V_FAILURE);
	}


	if (v_errno = set_slice_mnt(_current_disk, slice, val, (char *) NULL))
		return (V_FAILURE);

	return (V_OK);
}

/*
 * v_set_preserved(slice, val)
 *
 * function: sets preserved boolean on i'th slice to val
 * 	(TRUE if val != 0)
 * returns:  (V_OK)
 * 	V_FAILURE and sets v_errno
 */
V_Status_t
v_set_preserved(int slice, int val)
{
	if (slice >= N_Slices || slice < 0)
		return (V_FAILURE);

	if (_current_disk == (Disk_t *) NULL)
		return (V_FAILURE);

	if (v_errno = set_slice_preserve(_current_disk, slice,
		(val ? PRES_YES : PRES_NO)))
		return (V_FAILURE);

	return (V_OK);
}

/*
 * v_commit_disk(i)
 *
 * function: saves any changes made to i'th disk
 * returns: (V_OK)
 * 	V_FAILURE
 * algorithm:
 * 	check index against disk list boundaries.
 *  call disk library function which commits disk state
 * 	if no errors committing disk, or ignoring errors
 * 		set status on this disk to `edited'
 * 	else
 * 		inpterpret error code and return message
 *
 */
V_Status_t
v_commit_disk(int i, V_CommitErrors_t check)
{

	if (i >= _num_disks || i < 0)
		return (V_FAILURE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	v_errno = commit_disk_config(_disks[i].info);

	if (v_errno == D_OK || check == V_IGNORE_ERRORS) {

		_disks[i].status = V_DISK_EDITED;
		return (V_OK);

	} else {
		return (V_FAILURE);
	}

}
/*
 * v_sdisk_get_err_buf()
 *
 * function: this is only called in response to a v_sdisk_validate() call
 *	which produced a non-negative error code.  We want to create a
 *	reasonably detailed error message containing the disk library's
 *	abbreviated message.  Also want to include some ideas for how
 *	to resolve the problem.
 * returns:  current disk error text formatted by the disk lib
 */
char *
v_sdisk_get_err_buf(void)
{
	static char errbuf[BUFSIZ];

#define	SDISK_ERROR_PRESCRIPT	gettext(\
	"This disk configuration is invalid and may not be used in its "\
	"current form.  The specific problem is:")

#define	SDISK_ERROR_POSTSCRIPT	gettext(\
	"You must resolve this problem before the disk configuration can "\
	"be saved.")

#define	SDISK_ERROR_DEFAULT	gettext(\
	"This disk configuration is invalid and may not "\
	"be used in its current form.\n\nNo further information is available.")

	if ((err_text != (char *) NULL) && err_text[0] != '\0')
		(void) sprintf(errbuf, "%s\n\n%s\n\n%s", SDISK_ERROR_PRESCRIPT,
		    err_text, SDISK_ERROR_POSTSCRIPT);
	else
		(void) sprintf(errbuf, "%s %d\n\n%s", TITLE_ERROR,
			v_errno, SDISK_ERROR_DEFAULT);
	return (errbuf);
}

/*
 * v_sdisk_validate()
 *
 * Validates that the S-Disk configuration in the CURRENT state is sane.
 */
int
v_sdisk_validate(int disk)
{
	Errmsg_t *	elp;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (disk >= _num_disks || disk < 0)
		return (V_OK);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (V_OK);

	/*
	 * is SDISK OK?  Errors which can't be tolerated:
	 *	D_ZERO   - failure; slice(s) too small for a filesystem
	 *	D_DUPMNT - failure; duplicate mounts between slices
	 *	D_OVER   - failure; two or more slices overlap
	 *	D_OFF    - failure; slice runs past disk
	 *	D_OUTOFREACH - failure; boot blocks out of reach
	 */
	err_text[0] = '\0';
	if (check_sdisk(_disks[disk].info) > 0) {
		WALK_LIST(elp, get_error_list()) {
			v_errno = elp->code;
			if (v_errno == D_ZERO || v_errno == D_DUPMNT ||
					v_errno == D_OVER ||
					v_errno == D_OFF ||
					v_errno == D_ILLEGAL ||
					v_errno == D_OUTOFREACH) {
				free_error_list();
				return (V_FAILURE);
			}
		}

		free_error_list();
	}
	return (V_OK);
}

/*
 * v_get_v_errno()
 *
 * function:
 * returns:  current disk errno value in terms of the view libraries
 * error codes.  V_* instead of D_*
 *
 */
int
v_get_v_errno(void)
{
	V_DiskError_t err = V_NOERR;

	switch (v_errno) {
	case D_NODISK:
		err = V_NODISK;
		break;

	case D_BADARG:
		err = V_BADARG;
		break;

	case D_NOSPACE:
		err = V_NOSPACE;
		break;

	case D_IGNORED:
		err = V_IGNORED;
		break;

	case D_DUPMNT:
		err = V_DUPMNT;
		break;

	case D_CHANGED:
		err = V_CHANGED;
		break;

	case D_CANTPRES:
		err = V_CANTPRES;
		break;

	case D_PRESERVED:
		err = V_PRESERVED;
		break;

	case D_BADDISK:
		err = V_BADDISK;
		break;

	case D_OFF:
		err = V_OFF;
		break;

	case D_ZERO:
		err = V_ZERO;
		break;

	case D_OVER:
		err = V_OVER;
		break;

	case D_ILLEGAL:
		err = V_ILLEGAL;
		break;

	case D_ALTSLICE:
		err = V_ALTSLICE;
		break;

	case D_NOTSELECT:
		err = V_NOTSELECT;
		break;

	case D_GEOMCHNG:
		err = V_GEOMCHNG;
		break;

	case D_NOGEOM:
		err = V_NOGEOM;
		break;

	case D_NOFIT:
		err = V_NOFIT;
		break;

	case D_NOSOLARIS:
		err = V_NOSOLARIS;
		break;

	case D_BADORDER:
		err = V_BADORDER;
		break;

	case D_LOCKED:
		err = V_LOCKED;
		break;

	case D_BOOTFIXED:
		err = V_BOOTFIXED;
		break;

	case D_BOOTCONFIG:
		err = V_BOOTCONFIG;
		break;

	case D_SMALLSWAP:
		err = V_SMALLSWAP;
		break;

	case D_ALIGNED:
		err = V_ALIGNED;
		break;

	case D_OUTOFREACH:
		err = V_OUTOFREACH;
		break;

		/*
		 * these are specific to the UI implentation...
		 */
	case V_NO_ROOTFS:
		err = V_NO_ROOTFS;
		break;

	case V_NODISK_SELECTED:
		err = V_NODISK_SELECTED;
		break;

	case V_TOOSMALL:
		err = V_TOOSMALL;
		break;

	case V_CONFLICT:
		err = V_CONFLICT;
		break;

	case V_PATH_TOO_LONG:
		err = V_PATH_TOO_LONG;
		break;

	default:
		err = -1;
		break;
	}

	return ((int) err);
}

/*
 * v_checkpoint_disk(i)
 *
 * function: checkpoint current state of i'th disk, this is *only* for use by
 * the disk editor... since it requires that the current disk state be
 * consistent & valid.
 *
 * returns an opaque pointer to the checkpointed state, this pointer should be
 * handed back to v_restore_checkpoint() to restore the checkpoint or
 * v_free_checkpoint() to recover the memory, failure to call either will
 * result in a memory leak;
 *
 */
void *
v_checkpoint_disk(int disk)
{
	Slice_t *	chkpt = NULL;
	Disk_t *	dp;
	int i;

	chkpt = (Slice_t *) xmalloc(N_Slices * sizeof (Slice_t));

	(void) memset(chkpt, 0, N_Slices * sizeof (Slice_t));

	for (i = 0; i < N_Slices; i++) {
		dp = _disks[disk].info;
		chkpt[i].state = slice_state(dp, i);
		chkpt[i].start = slice_start(dp, i);
		chkpt[i].size = slice_size(dp, i);
		(void) strcpy(chkpt[i].use, slice_use(dp, i));
	}

	return ((void *) chkpt);
}

void *
v_free_checkpoint(void *chkpt)
{

	if (chkpt != (void *) NULL) {
		free((void *) chkpt);
		chkpt = (void *) NULL;
	}
	return (chkpt);
}

/*
 * v_restore_checkpoint(int, (Slice_t *) chkpt)
 *
 * function: restores disk to state values described by `chkpt' this is *only*
 * for use by the disk editor... since it requires that the checkpointed
 * state be consistent & valid.
 *
 */
void *
v_restore_checkpoint(int disk, void *chkpt)
{
	int i;
	Disk_t *    dp;

	for (i = 0; i < N_Slices; i++) {
		dp = _disks[disk].info;
		slice_state(dp, i) = ((Slice_t *) chkpt)[i].state;
		slice_start(dp, i) = ((Slice_t *) chkpt)[i].start;
		slice_size(dp, i) = ((Slice_t *) chkpt)[i].size;
		(void) strcpy(slice_use(dp, i), ((Slice_t *) chkpt)[i].use);
	}

	free((void *) chkpt);
	chkpt = (void *) NULL;

	return (chkpt);
}

/*
 * v_restore_disk_commit(i)
 *
 * function: reset i'th disk's parameters to last committed state. returns:
 */
void
v_restore_disk_commit(int i)
{
	if (i >= _num_disks || i < 0)
		return;

	if (_disks[i].info == NULL)
		return;

	restore_disk(_disks[i].info, CFG_COMMIT);

}

/*
 * v_restore_orig_slices(i)
 *
 * function: reset i'th disk's slice information to existing this resets disk
 * back to its original-just-into-install state. returns:
 */
int
v_restore_orig_slices(int i)
{
	if (i >= _num_disks || i < 0)
		return (V_FAILURE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	if ((v_errno = SdiskobjConfig(LAYOUT_EXIST, _disks[i].info,
			NULL)) != D_OK)
		return (V_FAILURE);

	return (V_OK);
}

/*
 * v_restore_disk_orig(i)
 *
 * function: reset i'th disk's parameters to original state, commit the orig
 * state... this resets disk back to its original-just-into-install state.
 * returns:
 */
void
v_restore_disk_orig(int i)
{
	if (i >= _num_disks || i < 0)
		return;

	if (_disks[i].info == (Disk_t *) NULL)
		return;

	(void) restore_disk(_disks[i].info, CFG_EXIST);
	(void) commit_disk_config(_disks[i].info);

}

/*
 * v_clear_disk(i)
 *
 * function:  clears i'th disk's current state
 * returns:   V_OK or V_FAILURE
 * algorithm:
 *	check index against disk array boundaries.
 *	clears disk's `current' state
 *	set disk's status to unedited...
 */
int
v_clear_disk(int i)
{
	if (i >= _num_disks || i < 0)
		return (V_FAILURE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	(void) SdiskobjConfig(LAYOUT_RESET, _disks[i].info, NULL);

	_disks[i].status = V_DISK_UNEDITED;

	return (V_OK);
}

/*
 * v_unconfig_disk(i)
 *
 * function:  returns i'th disk to initial, untouched state.
 * returns:
 * algorithm:
 *	check index against disk array boundaries.
 *	clear disk's current state and commit it.
 *	set disk's status to unedited...
 *
 * NB: this is really gross, and should change or be integrated with
 *	v_unconfig_disk().  The major difference is that this commits
 *	the cleared state.
 */
int
v_unconfig_disk(i)
{

	if (_disks[i].info == (Disk_t *) NULL)
		return (V_FAILURE);

	(void) SdiskobjConfig(LAYOUT_RESET, _disks[i].info, NULL);
	(void) commit_disk_config(_disks[i].info);

	_disks[i].status = V_DISK_UNEDITED;

	return (V_OK);
}

/* --------- stuff for auto-partitioning -------------- */

static int _auto_partitioning = FALSE;

/*
 * v_get_has_auto_partitioning()
 *
 * function:  get/set auto_paritioing global variable.
 * returns:
 */
int
v_get_has_auto_partitioning(void)
{
	return (_auto_partitioning);
}

void
v_set_has_auto_partitioning(int val)
{
	_auto_partitioning = val;
}

/*
 * v_unauto_config_disks()
 *
 * function:  attempts to unwind auto-partitioned disks
 *
 * returns:
 * algorithm:
 *	undoes all auto-partitioned disks
 */
void
v_unauto_config_disks(void)
{
	int i;

	for (i = 0; i < _num_disks; i++)
		if (disk_selected(_disks[i].info) &&
		    _disks[i].status == V_AUTO_CONFIGED)
			(void) v_clear_disk(i);
}

/*
 * v_auto_config_disks()
 *
 * function:
 *	attempts to auto-partition disks and size/assign file systems
 *	to slices.
 * returns:
 *	 0:  auto_partitioning was successful
 *	-1:  ran out of disk space, not everything will fit
 *	-2:  didn't find any disks at all.
 *
 * algorithm:
 *	assumes that the disks have already been probed. apply default
 *	disk partitioning hueristic to each disk in turn until all file systems
 *	to be assigned have been assigned successfully, or until there are no
 *	more disks available.
 *
 */
int
v_auto_config_disks()
{
	int i;

	if (_num_disks) {

		if (SdiskobjAutolayout() == D_OK) {
			for (i = 0; i < _num_disks; i++) {
				if (disk_selected(_disks[i].info)) {
					SdiskobjAllocateUnused(_disks[i].info);
				}
			}
			return (0);
		} else {
			v_restore_default_fs_table();
			return (-1);
		}
	}

	return (-2);		/* no disks at all */

}

/*
 * return number of `unconfigured' disks
 */
int
v_get_n_avail_disks(void)
{
	int i, j;

	if (_num_disks) {

		for (i = j = 0; i < _num_disks; i++)
			if (disk_selected(_disks[i].info) &&
			    _disks[i].status != V_DISK_EDITED)
				++j;

		return (j);

	} else
		return (0);
}

/*
 * get set utilities for disk editing `parameters', wanted to use get/set
 * functions instead of global variables...
 */
/*
 * functions to get/set default disk size display units
 */
void
v_set_disp_units(V_Units_t units)
{
	Units_t val;

	switch (units) {
	case V_MBYTES:
		val = D_MBYTE;
		break;

	case V_KBYTES:
		val = D_KBYTE;
		break;

	case V_CYLINDERS:
		val = D_CYLS;
		break;

	case V_BLOCKS:
		val = D_BLOCK;
		break;

	default:
		val = D_MBYTE;
		break;

	}

	_default_units = units;	/* remember current units */
	(void) set_units(val);

}

V_Units_t
v_get_disp_units(void)
{
	return (_default_units);
}

/*
 * functions to get/set disk cylinder display in the disk editor
 */
void
v_set_default_showcyls(int val)
{
	_showcyls = val;
}

int
v_get_default_showcyls(void)
{
	return (_showcyls);
}

/*
 * functions to get/set disk slices cylinder overlap flag -- controls stuff
 * in the disk editor as well as the disk library
 */
void
v_set_default_overlap(int val)
{
	_overlap = val;
}

int
v_get_default_overlap(void)
{
	return (_overlap);
}

/*
 * new stuff for handling `preserve' semantics on file systems...
 *
 */
static int *_conflicts = (int *) NULL;
static int _nconflicts = 0;

/*
 * v_get_n_conflicts(): return # of slices found in conflict of last slice
 * whose preservability was checked... (see v_get_preserve_ok())
 */
int
v_get_n_conflicts()
{
	return (_nconflicts);
}

/*
 * v_get_conflicting_slice(i): return slice index of i'th conflicting slice
 * that was found in conflict with the last slice whose preservability was
 * checked... (see v_get_preserve_ok())
 *
 * return -1 if i is out of range, or the i'th slice index is out of range
 */
int
v_get_conflicting_slice(int i)
{
	if (i > _nconflicts || i < 0)
		return (-1);

	if (_conflicts[i] >= N_Slices || _conflicts[i] < 0)
		return (-1);

	return (_conflicts[i]);
}

/*
 * v_get_preserve_ok()
 *
 * function:  checks to see if slice and mount point are preservable.
 *
 * returns:
 *	V_CANTPRES:	mount point cannot be preserved
 *	V_CHANGED:	slice cannot be preserved... conflicts with edited
 *			configuration
 *	V_LOCKED:	slice cannot be preserved... locked
 *	V_FAILURE:	bogus disk or slice
 *	V_OK:		OK to preserve
 *
 * algorithm:
 *	uses underlying disk library utility functions.
 */
V_DiskError_t
v_get_preserve_ok(int disk, int slice, char *mntpt)
{
	int starts;
	daddr_t size;

	if (slice >= N_Slices || slice < 0)
		return (V_CANTPRES);

	if (_disks[disk].info == (Disk_t *) NULL)
		return (V_CANTPRES);

	/*
	 * was this slice aligned at load time?  Either start cylinder
	 * or size rounded up to a cylinder boundary... if so, then
	 * the slice is not preserveable
	 */
	if (orig_slice_aligned(_disks[disk].info, slice))
		return (V_ALIGNED);

	/*
	 * is this mount point preservable (not /, /usr or /var)
	 */
	if (filesys_preserve_ok(mntpt) != D_OK)
		return (V_CANTPRES);

	/*
	 * is this mount point one of the `really-shouldn't-preserve' ones?
	 */
	if (strcmp(mntpt, OPT) == 0 || strcmp(mntpt, EXPORT) == 0) {
		return (V_SHOULDNTPRES);
	}
	/*
	 * if this is not a known overlap slice... see if there's a problem
	 * with overlapping some other slice...
	 *
	 * _nconflicts = number of conflicting slices if 'slice' is marked
	 * preserved
	 *
	 * _conflicts is a pointer to an array of _nconflicts slice indexes
	 */
	_conflicts = (int *) NULL;
	_nconflicts = 0;

	starts = orig_slice_start(_disks[disk].info, slice);
	size = orig_slice_size(_disks[disk].info, slice);

	if (strcmp(mntpt, Overlap) != 0 &&
	    (_nconflicts = slice_overlaps(_disks[disk].info, slice, starts,
		size, &_conflicts)))
		return (V_CONFLICT);

	return (V_NOERR);
}

/*
 * v_has_preserved_slice(disk)
 *
 * boolean function:
 *	returns true if i'th disk has at least one slice marked as preserved
 *	returns false if i'th disk has no slices marked as preserved
 */
int
v_has_preserved_slice(int i)
{
	int j;

	if (_num_disks == -1 && _current_disk_index == -1)
		(void) v_init_disks();

	if (i >= _num_disks || i < 0)
		return (FALSE);

	if (_disks[i].info == (Disk_t *) NULL)
		return (FALSE);

	for (j = 0; j < N_Slices; j++) {
		if (slice_preserved(_disks[i].info, j))
			return (TRUE);
	}

	return (FALSE);

}
