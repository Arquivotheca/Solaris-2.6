#ifndef lint
#pragma ident "@(#)v_lfs.h 1.26 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_lfs.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_LFS_H
#define	_V_LFS_H

#ifdef __cplusplus
extern "C" {
#endif

	extern double v_get_lfs_suggested_size(int);
	extern double v_get_lfs_configed_size(int);
	extern double v_get_lfs_req_size(int);
	extern int v_get_mntpt_size_hint(char *, int);
	extern int v_is_default_mount_point(char *);
	extern void v_get_mntpt_suggested_size(char *, double *);
	extern void v_get_mntpt_configed_size(char *, double *);
	extern void v_get_mntpt_req_size(char *, double *);
	extern void v_set_n_lfs(void);
	extern void v_update_lfs_space(void);
	extern char *v_get_lfs_mntpt(int);
	extern char *v_get_disk_and_slice_from_lfs_name(char *);
	extern char *v_get_disk_from_lfs_name(char *);
	extern int v_get_n_lfs(void);
	extern int v_any_lfs_configed(void);
	extern int v_lfs_configed(int);

	extern double v_get_dflt_lfs_space_total(void);

	/*
	 * stuff for dealing with the default file system list and mask
	 */
	extern int v_set_fs_defaults();
	extern int v_set_default_fs_status(int, int);
	extern int v_get_default_fs_status(int);
	extern char *v_get_default_fs_name(int);
	extern int v_get_n_default_fs(void);
	extern void v_restore_default_fs_table(void);

#ifdef __cplusplus
}

#endif

#endif				/* _V_LFS_H */
