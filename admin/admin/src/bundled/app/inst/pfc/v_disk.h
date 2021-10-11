#ifndef lint
#pragma ident "@(#)v_disk.h 1.80 96/04/25 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_disk.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_DISK_H
#define	_V_DISK_H

#ifndef _DISK_H
#include "spmistore_api.h"
#endif

/* need Units_t typedef */
typedef enum {
	V_MBYTES = 0,
	V_CYLINDERS = 1,
	V_KBYTES = 2,
	V_BLOCKS = 3
} V_Units_t;

typedef enum {
	V_DISK_LIMBO = 0,
	V_DISK_UNEDITED = 1,
	V_DISK_EDITED = 2,
	V_AUTO_CONFIGED = 3,
	V_DISK_NOTOKAY = 4
} V_DiskStatus_t;

typedef enum {
	V_IGNORE_ERRORS = 0,
	V_CHECK_ERRORS = 1
} V_CommitErrors_t;

typedef enum {
	V_NOERR = 0,
	V_NODISK = 1,
	V_BADARG = 2,
	V_NOSPACE = 3,
	V_DUPMNT = 4,
	V_IGNORED = 5,
	V_CHANGED = 6,
	V_CANTPRES = 7,
	V_PRESERVED = 8,
	V_BADDISK = 9,
	V_OFF = 10,
	V_ZERO = 11,
	V_OVER = 12,
	V_ILLEGAL = 13,
	V_ALTSLICE = 14,
	V_BOOTFIXED = 15,
	V_NOTSELECT = 16,
	V_LOCKED = 17,
	V_GEOMCHNG = 18,
	V_NOGEOM = 19,
	V_NOFIT = 20,
	V_NOSOLARIS = 21,
	V_BADORDER = 22,
	V_OUTOFREACH = 23,
	V_BOOTCONFIG = 24,
	V_SMALLSWAP = 25,
	V_ALIGNED = 26,
	V_NO_ROOTFS = 94,
	V_SHOULDNTPRES = 95,
	V_NODISK_SELECTED = 96,
	V_TOOSMALL = 97,
	V_CONFLICT = 98,
	V_PATH_TOO_LONG = 99
} V_DiskError_t;

typedef enum {
	V_SUNIXOS = 0,
	V_DOSPRIMARY = 1,
	V_DOSEXT = 2,
	V_UNUSED  = 3,
	V_OTHER   = 4
} V_DiskPart_t;


/* global variables */
extern int v_errno;
extern int N_Slices;		/* set from the #define NUMPARTS */
extern int N_Partitions;	/* set from the #define FD_NUMPART */

extern char *Alts;		/* `alts' slice keyword */
extern char *Overlap;		/* `overlap' slice keyword */

#ifdef __cplusplus
extern "C" {
#endif

	/*
	 * Exported routines from View layer of disk library: (from
	 * v_sdisk.c);
	 */
	extern void v_set_default_showcyls(int val);
	extern int v_get_default_showcyls(void);
	extern void v_set_disp_units(V_Units_t);
	extern V_Units_t v_get_disp_units(void);
	extern void v_set_default_overlap(int);
	extern int v_get_default_overlap(void);

	extern Disk_t *v_get_first_disk(void);
	extern int v_init_disks(void);
	extern int v_get_n_disks(void);
	extern int v_get_n_avail_disks(void);
	extern int v_set_disk_selected(int, int);
	extern int v_get_disk_selected(int);
	extern int v_get_disk_usable(int);
	extern int v_is_bootdrive(int);
	extern int v_alt_slice_reqd(int);
	extern char *v_get_default_bootdrive_name(void);
	extern int v_get_disk_slices_intact(int);

	extern int v_get_disk_size(int);
	extern int v_get_sdisk_size(int);
	extern int v_get_sdisk_capacity(int);
	extern int v_get_disk_capacity(int);
	extern int v_get_space_avail(int);

	extern char *v_get_disk_name(int);
	extern int v_get_disk_index_from_name(char *);
	extern char *v_get_disk_mountpts(int);

	extern char *v_get_disp_units_str(void);
	extern char *v_get_disk_status_str(int i);
	extern V_Status_t v_set_current_disk(int);
	extern V_DiskStatus_t v_get_disk_status(int i);

	extern Disk_t *v_int_get_current_disk_ptr(void);
	extern int v_get_current_disk(void);

	/* accessing/testing slice preservability and errors */
	extern V_Status_t v_restore_orig_size(int);
	extern V_DiskError_t v_get_preserve_ok(int, int, char *);
	extern int v_has_preserved_slice(int);
	extern int v_get_n_conflicts(void);
	extern int v_get_conflicting_slice(int);

	/* accessing current state of current disk/slice */
	extern int v_get_cur_start_cyl(int);
	extern int v_get_cur_end_cyl(int);
	extern int v_get_cur_size(int);
	extern int v_get_cur_preserved(int);
	extern char *v_get_cur_mount_pt(int);

	/* accessing original state of current disk/slice */
	extern int v_get_orig_start_cyl(int);
	extern int v_get_orig_size(int);
	extern int v_get_lock_state(int);
	extern char *v_get_orig_mount_pt(int);
	extern void v_restore_disk_orig(int);
	extern int v_restore_orig_slices(int);

	/* accessing committed state of current disk/slice */
	extern int v_get_comm_start_cyl(int);
	extern int v_get_comm_size(int);
	extern char *v_get_comm_mount_pt(int);
	extern int v_get_comm_preserved(int);
	extern void v_restore_disk_commit(int);

	/* checkpoint and restore of disk state (only for slice editor) */
	extern void *v_free_checkpoint(void *);
	extern void *v_checkpoint_disk(int);
	extern void *v_restore_checkpoint(int, void *);

	extern void v_undo_disk(int);
	extern int v_sdisk_validate(int);
	extern char *v_sdisk_get_err_buf(void);
	extern int v_get_v_errno(void);

	extern V_Status_t v_set_start_cyl(int, int, int);
	extern V_Status_t v_set_size(int, int, int);
	extern V_Status_t v_set_mount_pt(int, char *);
	extern V_Status_t v_set_preserved(int, int);
	extern V_Status_t v_commit_disk(int, V_CommitErrors_t);

	extern int v_get_has_auto_partitioning(void);
	extern void v_set_has_auto_partitioning(int);
	extern int v_auto_config_disks(void);
	extern void v_unauto_config_disks(void);
	extern int v_auto_config_disk(int);
	extern int v_unconfig_disk(int);
	extern int v_clear_disk(int);

	/*
	 * External interface to FDISK functionality, implementation is in
	 * v_fdisk.c
	 */

	/*
	 * Partitions are numbered starting from 1, provide macros to
	 * convert to/from 0 based indexing.
	 */
#define	FD_PART_NUM(i)	(i + 1)
#define	FD_PART_IDX(i)	(i - 1)

	extern int v_boot_disk_selected(void);

	extern int v_fdisk_get_space_avail(int);
	extern int v_fdisk_flabel_req(int);
	extern int v_fdisk_flabel_exist(int);
	extern int v_fdisk_flabel_has_spart(int);
	extern int v_fdisk_get_part_maxsize(int);
	extern int v_fdisk_get_max_partsize(int);
	extern int v_fdisk_set_solaris_max_partsize(int);
	extern int v_fdisk_get_max_partsize_free(int);
	extern int v_fdisk_set_solaris_free_partsize(int);
	extern int v_fdisk_part_is_active(int);
	extern int v_fdisk_set_active_part(int);
	extern int v_fdisk_set_default_flabel(int);
	extern V_DiskPart_t v_fdisk_get_part_type(int);
	extern char *v_fdisk_get_part_type_str(int);

	extern char *v_fdisk_get_type_str_by_index(int i);
	extern V_DiskPart_t v_fdisk_get_type_by_index(int i);
	extern int v_fdisk_get_n_part_types();

	extern int v_fdisk_set_part_type(int, V_DiskPart_t);
	extern int v_fdisk_get_part_size(int);
	extern int v_fdisk_set_part_size(int, int);
	extern int v_fdisk_get_part_startsect(int);
	extern int v_fdisk_set_part_startcyl(int, int);
	extern int v_fdisk_get_part_endcyl(int);

	extern int v_cyls_to_mb(int, int);
	extern int v_mb_to_cyls(int, int);
	extern char *v_fdisk_get_err_buf(void);
	extern int v_fdisk_validate(int);

#ifdef __cplusplus
}

#endif

#endif				/* _V_DISK_H */
