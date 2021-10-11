#ifndef lint
#pragma ident "@(#)disk_fs_util.h 1.3 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	disk_fs_util.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _DISK_FS_UTIL_H
#define	_DISK_FS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

	extern void	_commit_all_selected_disks(int);
	extern void	_clear_all_selected_disks(int);
	extern void	_reset_all_selected_disks(int);
	extern void	_restore_all_selected_disks_commit(int);

	extern int	_get_reqd_space();
	extern int	_get_avail_space(int);
	extern int	_get_used_disks(int);

#ifdef __cplusplus
}

#endif

#endif	/* _DISK_FS_UTIL_H */
