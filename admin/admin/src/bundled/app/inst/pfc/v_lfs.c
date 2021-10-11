#ifndef lint
#pragma ident "@(#)v_lfs.c 1.74 96/09/16 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_lfs.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/fs/ufs_fs.h>
#include <libintl.h>

#include "pf.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_misc.h"
#include "v_lfs.h"
#include "v_upgrade.h"

/*
 * This file contains the View interface layer to the local file system
 * library.  It provides an abstraction layer for accessing the
 * functionality that defines and manages local file systems.
 *
 * There are two different perspectives to local file systems: file systems
 * (mount points) which have been configured on local disks, and file
 * systems which *need* to be configured onto local disks.  Sort of the
 * difference betwen optional and required...
 *
 * The view library exposes the different perspectives by providing access
 * routines for retrieving file system names and sizes by `index' (ie., for
 * all local file systems that are configured, what is the i'th one's
 * size...) These routines have the form `v_get_lfs_*(int i)'.
 *
 * Specific mount point/file system information is retrieved by mount point
 * name (ie., what is the size of "/usr").  These routines have the form
 * `v_get_mntpt_*(char *fs)'.
 *
 */

/* typedefs and defines */

/* Static Globals: */
static int _n_local_fs = 0;

static char **_lfs = (char **) NULL;

/* Forward declarations: */
static char **v_int_get_config_fs(char **fs);
int v_is_default_mount_point(char *);

/*
 * build the array of local file system mount points: construct this array
 * as the union of the default mount points for this system type, and any
 * additional file systems that the user has configured.
 */
void
v_set_n_lfs(void)
{
	int i;
	int j;
	static Defmnt_t **defaults = (Defmnt_t **) NULL;
	static char **configed = (char **) NULL;

	defaults = get_dfltmnt_list(defaults);

	/*
	 * copy default mnt pts into global local file system array
	 */
	for (i = 0; defaults[i]->name && *defaults[i]->name; i++) {

		if (i == _n_local_fs) {
			_lfs = (char **) realloc(_lfs, (i + 1) *
			    sizeof (char *));

			_n_local_fs++;
		}
		_lfs[i] = defaults[i]->name;

	}

	/*
	 * now add any additional user-specified mntpts
	 */
	configed = v_int_get_config_fs(configed);
	for (j = 0; configed[j] && *configed[j]; j++) {

		if (v_is_default_mount_point(configed[j]) == FALSE) {

			if (i == _n_local_fs) {
				_lfs = (char **) realloc(_lfs, (i + 1) *
				    sizeof (char *));

				_n_local_fs++;
			}
			_lfs[i] = configed[j];

			++i;
		}
	}

	_lfs = (char **) realloc(_lfs, (i + 1) * sizeof (char *));
	_lfs[i] = (char *) NULL;

	_n_local_fs = i;

}

/*
 * v_get_n_lfs(void)
 *
 * return number of file systems
 *
 * input:
 * returns:
 * algorithm:
 */
int
v_get_n_lfs(void)
{
	return (_n_local_fs);
}

/*
 * v_any_lfs_configed(void)
 *
 * boolean, indicates if any local file systems have been configured.
 *
 * input:
 * returns:
 *	0 - if no file systems have been configured
 *	1 - if there's a file system configured
 * algorithm:
 */
int
v_any_lfs_configed(void)
{
	int i;

	for (i = 0; i < v_get_n_lfs(); i++) {
		if (find_mnt_pnt(NULL, NULL, _lfs[i], NULL, CFG_CURRENT))
			return (1);

	}

	return (0);
}

/*
 * v_lfs_configed(int i)
 *
 * Determines if the i'th file system is configured...
 * Hueristic used: is it's partition non-zero sized
 *
 * input:
 *	index of i'th lfs[] entry
 *
 * returns:
 *	0 - if i'th mount point size == 0, or if mount point not found
 *	!0 - if i'th mount point size > 0
 *
 * algorithm:
 */
int
v_lfs_configed(int i)
{
	int		n = v_get_n_lfs();
	Mntpnt_t	info;

	if (i < n && *_lfs[i]) {
		if (find_mnt_pnt(NULL, NULL, _lfs[i], &info, CFG_CURRENT))
			return (slice_size(info.dp, info.slice));
	}

	return (0);
}

/*
 * v_get_lfs_mntpt(int i)
 *
 * get i'th file system's mount point
 *
 * input:
 *	index of i'th lfs[] entry
 *
 * returns:
 * 	pointer to static buffer containing i'th file systems mount point.
 *
 * algorithm:
 */
char *
v_get_lfs_mntpt(int i)
{
	static char buf[MAXMNTLEN];

	buf[0] = '\0';

	if (i >= 0 && i < _n_local_fs) {

		(void) strcpy(buf, _lfs[i]);
		return (buf);

	} else
		return (NULL);

}

void
v_update_lfs_space(void)
{
	ResobjUpdateContent();
}

/*
 * v_get_mntpt_suggested_size(char *mntpt, double *n)
 *
 * get mntpt's suggested size
 *
 * input:
 *	mntpt:	char * pointer to mount point name of interest
 *	*n:	pointer to a double to store size suggestion in
 *
 * returns:
 *
 * algorithm:
 * 	Must set this file systems `status' to DONTCARE to get
 *	accurate numbers regardless of what file systems are
 *	currently configured.
 */
void
v_get_mntpt_suggested_size(char *mntpt, double *n)
{
	struct mnt_pnt d;
	Disk_t *curdisk;
	Defmnt_t mp;
	int status;

	if (get_dfltmnt_ent(&mp, mntpt) == D_OK) {
		status = mp.status;

		mp.status = DFLT_DONTCARE;
		(void) set_dfltmnt_ent(&mp, mntpt);

		/*
		 * try to find the disk that the file system is on, this
		 * allows us to do more accurate diskblocks-to-mb
		 * conversion, taking into accouting cylinder rounding.
		 *
		 * if the file system doesn't exist yet, use the current disk,
		 * if there is one.
		 *
		 * as a last resort just do raw sectors to mb conversion and do
		 * no rounding.
		 *
		 */
		if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL, mntpt, &d,
			CFG_CURRENT))
			*n = blocks2size(d.dp, get_default_fs_size(mntpt, d.dp,
				DONTROLLUP), ROUNDUP);
		else if ((curdisk = v_int_get_current_disk_ptr()) !=
		    (Disk_t *) NULL)
			*n = blocks2size(curdisk,
			    get_default_fs_size(mntpt, curdisk, DONTROLLUP),
			    ROUNDUP);
		else {
			if (v_get_disp_units() == V_MBYTES) {
				*n = sectors_to_kb(get_default_fs_size(mntpt,
					curdisk, DONTROLLUP)) / KBYTE;
			}
			/*
			 * no disk and displaying in cylinders?  This
			 * shouldn't ever happen.
			 */
		}

		mp.status = status;
		(void) set_dfltmnt_ent(&mp, mntpt);
	} else
		*n = 0;
}

/*
 * v_get_lfs_suggested_size(int i)
 *
 * get i'th local file system suggested size
 *
 * input:
 *	i:	index of local file system of interest
 *
 * returns:
 *	suggested size as a double.
 *
 * algorithm:
 */
double
v_get_lfs_suggested_size(int i)
{
	char *mntpt;
	double n;

	if ((mntpt = v_get_lfs_mntpt(i)) != (char *) NULL) {
		v_get_mntpt_suggested_size(mntpt, &n);
		return (n);
	} else
		return (0.0);
}

/*
 * v_get_mntpt_req_size(char *mntpt, double *n)
 *
 * get mntpt's required size
 *
 * input:
 *	mntpt:	char * pointer to mount point name of interest
 *	*n:	pointer to a double to store size requirement in
 *
 * returns:
 *
 * algorithm:
 */
void
v_get_mntpt_req_size(char *mntpt, double *n)
{
	struct mnt_pnt d;
	Defmnt_t mp;
	int status;
	Disk_t *curdisk;

	if (get_dfltmnt_ent(&mp, mntpt) == D_OK) {
		status = mp.status;

		mp.status = DFLT_DONTCARE;
		(void) set_dfltmnt_ent(&mp, mntpt);

		/*
		 * try to find the disk that the file system is on, this
		 * allows us to do more accurate diskblocks-to-mb
		 * conversion, taking into accouting cylinder rounding.
		 *
		 * if the file system doesn't exist yet, use the current disk,
		 * if there is one.
		 *
		 * as a last resort just do raw sectors to mb conversion and do
		 * no rounding.
		 *
		 */
		if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL, mntpt, &d,
			CFG_CURRENT))
			*n = blocks2size(d.dp, get_minimum_fs_size(mntpt, d.dp,
				DONTROLLUP), ROUNDUP);
		else if ((curdisk = v_int_get_current_disk_ptr()) !=
		    (Disk_t *) NULL)
			*n = blocks2size(curdisk,
			    get_minimum_fs_size(mntpt, curdisk, DONTROLLUP),
			    ROUNDUP);
		else {
			if (v_get_disp_units() == V_MBYTES) {
				*n = sectors_to_kb(get_minimum_fs_size(mntpt,
					curdisk, DONTROLLUP)) / KBYTE;
			}
			/*
			 * no disk and displaying in cylinders?  This
			 * shouldn't ever happen.
			 */
		}

		mp.status = status;
		(void) set_dfltmnt_ent(&mp, mntpt);
	} else
		*n = 0;
}

/*
 * v_get_lfs_req_size(int i)
 *
 * get i'th local file system required size
 *
 * input:
 *	i:	index of local file system of interest
 *
 * returns:
 *	required size as a double.
 *
 * algorithm:
 */
double
v_get_lfs_req_size(int i)
{
	char *mntpt;
	double n;

	if ((mntpt = v_get_lfs_mntpt(i)) != (char *) NULL) {
		(void) v_get_mntpt_req_size(mntpt, &n);
		return (n);
	} else
		return (0.0);
}

/*
 * existing_mnt_pnt_size(char *mntpt)
 *
 * get mntpt's configured size (from the sw data structure with true file
 *	system size).
 *
 * input:
 *	mntpt:	char * pointer to mount point name of interest
 *
 * returns:
 *
 * algorithm:
 */
static double
existing_mnt_pnt_size(char *mntpt)
{

	int i;
	FSspace **fs_space = (FSspace **) NULL;

	fs_space = get_current_fs_layout(0);

	/*
	 * need to compute size of existing file system... space code gives
	 * us the file system info from vfsstat()
	 *
	 * fs size =  (# blocks * bytes-per-block ) / bytes-per-MB)
	 */
	i = 0;
	while (fs_space[i] && (fs_space[i] != (FSspace *) NULL))
		if (strcmp(mntpt, fs_space[i]->fsp_mntpnt) == 0)
			return ((double) ((fs_space[i]->fsp_fsi->f_frsize *
				    fs_space[i]->fsp_fsi->f_blocks) /
				MBYTE));
		else
			++i;

	return (0.0);
}

/*
 * v_get_mntpt_configed_size(char *mntpt, double *n)
 *
 * get mntpt's configured size
 *
 * input:
 *	mntpt:	char * pointer to mount point name of interest
 *	*n:	pointer to a double to store size configured in
 *
 * returns:
 *
 * algorithm:
 */
void
v_get_mntpt_configed_size(char *mntpt, double *n)
{
	struct mnt_pnt d;

	*n = 0.0;

	if (v_is_upgrade()) {

		*n = (double) existing_mnt_pnt_size(mntpt);

	} else {

		if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL, mntpt, &d,
			CFG_CURRENT))
			*n = (double) blocks_to_mb_trunc(d.dp,
				slice_size(d.dp, d.slice));

	}
}

/*
 * v_get_lfs_configed_size(int i)
 *
 * get i'th local file system configured size
 *
 * input:
 *	i:	index of local file system of interest
 *
 * returns:
 *	configured size as a double.
 *
 * algorithm:
 */
double
v_get_lfs_configed_size(int i)
{
	char *mntpt;
	double n;

	if ((mntpt = v_get_lfs_mntpt(i)) != (char *) NULL) {
		v_get_mntpt_configed_size(mntpt, &n);
		return (n);
	} else
		return (0.0);
}

/*
 * v_get_disk_and_slice_from_lfs_name(char *mntpt)
 *
 * Get disk/slice info for a file system from its name
 *
 * input:
 *	mntpt:	char pointer with the mount point of interest
 *
 * returns:
 * 	pointer to static buffer containing disk name in the form: c0t0d0s0
 *
 * algorithm:
 */
char *
v_get_disk_and_slice_from_lfs_name(char *mntpt)
{
	struct mnt_pnt mp_info;

	static char buf[32];

	buf[0] = '\0';

	if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL, mntpt, &mp_info,
		CFG_CURRENT))
		(void) sprintf(buf, "%ss%1d", mp_info.dp->name,
		    mp_info.slice);

	return (buf);
}

/*
 * v_get_disk_from_lfs_name(char *mntpt)
 *
 * Get disk info for a file system from its name
 *
 * input:
 *	mntpt:	char pointer with the mount point of interest
 *
 * returns:
 * 	pointer to static buffer containing disk name in the form: c0t0d0
 *
 * algorithm:
 */
char *
v_get_disk_from_lfs_name(char *mntpt)
{
	struct mnt_pnt mp_info;

	static char buf[32];

	buf[0] = '\0';

	if (find_mnt_pnt((Disk_t *) NULL, (char *) NULL, mntpt, &mp_info,
		CFG_CURRENT))
		(void) sprintf(buf, "%s", mp_info.dp->name);

	return (buf);
}

/*
 * v_int_get_config_fs(char **fs)
 *
 * figures out what file system have been configured on the local disks.
 *
 * input:
 *	pointer to an array of char * holding the file system names.
 *
 * returns:
 *	pointer to an array of char * holding the file system names.
 *
 * algorithm:
 *	for each disk and each possible slice, if there is a file system
 *	on the slice, add the file system's name to the array.
 *
 */
static char **
v_int_get_config_fs(char **fs)
{
	int i;
	int j;
	int k = 0;		/* number of configured file systems */
	int disk;
	char *mount;

	/*
	 * remember number of buckets in the array passed in & out, also
	 * reset the `current' disk when done
	 */
	static int max = 0;

	if (max == 0) {
		fs = (char **) malloc(16 * (sizeof (char *)));
		max = 16;
	}
	disk = v_get_current_disk();

	for (i = 0; i < v_get_n_disks(); i++) {

		if (v_get_disk_usable(i) == 1) {

			(void) v_set_current_disk(i);
			for (j = 0; j < N_Slices; j++) {

				mount = v_get_cur_mount_pt(j);
				if (mount && *mount) {

					fs[k++] = mount;
					if (k == max) {
						fs = (char **) realloc(fs,
						    (max + 16) *
						    sizeof (char *));
						max += 16;
					}
				}
			}
		}
	}

	/* make sure array is `terminated' */
	fs[k] = "";

	(void) v_set_current_disk(disk);

	return (fs);
}

/*
 * predicate: is the file system a `required' file system which has
 *	its space information tracked?
 *
 * sensitive to system type.
 */
int
v_is_default_mount_point(char *fs)
{
	return (get_dfltmnt_ent((Defmnt_t *) NULL, fs) == D_OK);
}

/*
 * returns a size hint for the file system mount point passed
 * in.
 *
 * this size hint includes space for all of the `child' file systems
 * which are `rolled' up into `mntpt'.
 *
 * returns value in current `size units'.
 *
 * `minimum' indicates if the default file system free space should be
 * included in the hint.
 */
int
v_get_mntpt_size_hint(char *mntpt, int min)
{
	register int i;
	static Defmnt_t **fs = (Defmnt_t **) NULL;
	Disk_t *curdisk = v_int_get_current_disk_ptr();

	fs = get_dfltmnt_list(fs);

	/*
	 * if this isn't a `required' file system... there is no size
	 * hint...
	 */
	if (v_is_default_mount_point(mntpt) == 0)
		return ((int) 0);

	/*
	 * figure out which file systems have already been configured, mark
	 * the one's which have as `DFLT_SELECT'
	 */
	for (i = 0; fs[i] && fs[i]->name != (char *) NULL; i++) {

		if (find_mnt_pnt((Disk_t *) 0, (char *) 0, fs[i]->name,
			(Mntpnt_t *) NULL, CFG_CURRENT))
			fs[i]->status = DFLT_SELECT;
		else if (fs[i]->status != DFLT_DONTCARE)
			fs[i]->status = DFLT_IGNORE;

	}

	(void) set_dfltmnt_list(fs);

	/*
	 * return a hint for this file system.  if min != 0, then provide a
	 * minimum size excluding free space
	 */
	if (min)
		return (blocks2size(curdisk,
			get_minimum_fs_size(mntpt, curdisk, ROLLUP), ROUNDUP));
	else
		return (blocks2size(curdisk,
			get_default_fs_size(mntpt, curdisk, ROLLUP), ROUNDUP));

}

static Defmnt_t **orig_server_fs = (Defmnt_t **) NULL;
static Defmnt_t **orig_stand_fs = (Defmnt_t **) NULL;

void
v_save_default_fs_table(V_SystemType_t type)
{
	switch (type) {
		case V_SERVER:
		if (orig_server_fs == (Defmnt_t **) NULL)
			orig_server_fs = get_dfltmnt_list(orig_server_fs);

		break;

	case V_STANDALONE:
	default:
		if (orig_stand_fs == (Defmnt_t **) NULL)
			orig_stand_fs = get_dfltmnt_list(orig_stand_fs);

		break;
	}

}

void
v_restore_default_fs_table(void)
{
	Defmnt_t **mpp = (Defmnt_t **) NULL;
	Defmnt_t **orig_mpp = (Defmnt_t **) NULL;
	int i;

	/*
	 * make a scratch mpp array of the correct size
	 */
	mpp = get_dfltmnt_list(mpp);

	switch (v_get_system_type()) {
	case V_SERVER:
		orig_mpp = orig_server_fs;
		break;

	case V_STANDALONE:
	default:
		orig_mpp = orig_stand_fs;
		break;

	}

	/*
	 * copy original values into scratch mpp array, then override
	 * defaults with any file systems have already been explicitly
	 * configured by marking them as `DFLT_SELECT'
	 */
	for (i = 0; mpp[i] && mpp[i]->name != (char *) NULL; i++) {

		if (find_mnt_pnt((Disk_t *) 0, (char *) 0, mpp[i]->name,
			(Mntpnt_t *) NULL, CFG_COMMIT)) {
			mpp[i]->status = DFLT_SELECT;
		} else {
			mpp[i]->status = orig_mpp[i]->status;
		}

	}

	(void) set_dfltmnt_list(mpp);

	/*
	 * free scratch mpp array
	 */
	mpp = free_dfltmnt_list(mpp);

}

/*
 * return number of default file systems associated with current system type
 */
static int _n_default_fs = -1;
static int _fs_indexes[32];	/* need NUMDEFMNT from disk lib */
static Defmnt_t **def_fs = (Defmnt_t **) NULL;
static Defmnt_t **cur_def_fs = (Defmnt_t **) NULL;

/*
 * prototyping `space meter' screen.
 */
void
v_restore_current_default_fs()
{
	(void) set_dfltmnt_list(cur_def_fs);

}
void
v_save_current_default_fs()
{
	cur_def_fs = get_dfltmnt_list(cur_def_fs);

}

/*
 * returns minimum size for i'th default file system using current state of
 * the default file system mask (def_fs).
 *
 * v_get_n_default_fs() must be called before this
 *
 */
v_get_default_fs_req_size(int i)
{
	Disk_t *curdisk = v_int_get_current_disk_ptr();

	/* set to current */
	(void) set_dfltmnt_list(def_fs);

	return (blocks2size(curdisk,
		get_minimum_fs_size(def_fs[_fs_indexes[i]]->name, curdisk,
		ROLLUP), ROUNDUP));
}

/*
 * returns recommended size for i'th default file system
 * using current state of the default file system mask (def_fs).
 *
 * v_get_n_default_fs() must be called before this
 *
 */
v_get_default_fs_sug_size(int i)
{
	Disk_t *curdisk = v_int_get_current_disk_ptr();

	/* set to current */
	(void) set_dfltmnt_list(def_fs);

	return (blocks2size(curdisk,
		get_default_fs_size(def_fs[_fs_indexes[i]]->name, curdisk,
		ROLLUP), ROUNDUP));
}

int
v_get_n_default_fs(void)
{
	int i;

	def_fs = get_dfltmnt_list(def_fs);

	/*
	 * figure out which file systems are the defaults for this system
	 * type.  Should be the ones marked DFLT_SELECT or DFLT_DONTCARE
	 * remember the real index to each default file system
	 */
	_n_default_fs = 0;
	for (i = 0; def_fs[i] && def_fs[i]->name != (char *) NULL; i++) {
		if (def_fs[i]->status == DFLT_SELECT ||
		    def_fs[i]->status == DFLT_DONTCARE)
			_fs_indexes[_n_default_fs++] = i;
	}

	return (_n_default_fs);
}

/* get i'th default mountpoint's name */
char *
v_get_default_fs_name(int i)
{
	if (i < 0 || i >= _n_default_fs)
		return ((char *) NULL);

	return (def_fs[_fs_indexes[i]]->name);

}

/* get i'th default mountpoint's status */
int
v_get_default_fs_status(int i)
{
	if (i < 0 || i >= _n_default_fs)
		return (0);

	return (def_fs[_fs_indexes[i]]->status == DFLT_SELECT);

}

/* set i'th default mountpoint's status */
int
v_set_default_fs_status(int i, int status)
{

	if (status != 0)
		def_fs[_fs_indexes[i]]->status = DFLT_SELECT;
	else if (status == 0 && def_fs[_fs_indexes[i]]->status == DFLT_SELECT)
		def_fs[_fs_indexes[i]]->status = DFLT_DONTCARE;

	return (1);
}

/* pushd mountpoints down into disk lib */
int
v_set_fs_defaults()
{

	(void) set_dfltmnt_list(def_fs);

	return (1);
}

/*
 * calculates space `required' for a successful install.  this is used a
 * one-number metric in a variety of dispplays.  It probably won't last...
 * basically just totals the `required' space for all the default file
 * systems.
 */
double
v_get_dflt_lfs_space_total()
{
#ifdef OLD_VIEW
	int n;
	int i;
	double reqd = 0.0;

	v_save_current_default_fs();

	/*
	 * clear out any 'current disk' context since this affects
	 * the default size calculations... (ick!)
	 */
	(void) v_set_current_disk(-1);
	v_restore_default_fs_table();
	n = v_get_n_lfs();

	for (i = 0; i < n; i++)
		reqd += (double) v_get_lfs_suggested_size(i);

	v_restore_current_default_fs();

	return (reqd);
#endif

	return ((double) DiskGetContentDefault());
}
