#ifndef lint
#pragma ident "@(#)v_rfs.h 1.18 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_rfs.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_RFS_H
#define	_V_RFS_H

#ifdef __cplusplus
extern "C" {
#endif

	/* need to pick up typedef for Remote_FS */
#ifndef SW_STRUCTS
#include "spmisoft_api.h"
#endif

	extern Remote_FS *v_get_first_rfs(void);
	extern char *v_get_default_rfs(char *);
	extern void v_clear_export_fs(void);
	extern void v_delete_all_rfs(void);
	extern void v_delete_rfs(int);
	extern void v_init_rfs_info(void);
	extern int v_new_rfs(char *, char *, char *, char *);
	extern int v_any_rfs_configed(void);
	extern int v_rfs_configed(int);
	extern int v_get_n_rfs(void);
	extern int v_set_current_rfs(int);
	extern int v_get_current_rfs(void);
	extern int v_set_rfs_ip_addr(int, char *);
	extern char *v_get_rfs_ip_addr(int);
	extern int v_set_rfs_mnt_pt(int, char *);
	extern char *v_get_rfs_mnt_pt(int);
	extern int v_set_rfs_server(int, char *);
	extern char *v_get_rfs_server(int);
	extern int v_set_rfs_server_path(int, char *);
	extern char *v_get_rfs_server_path(int);
	extern int v_test_rfs_mount(int);
	V_TestMount_t v_get_rfs_test_status(int);
	extern int v_set_rfs_test_status(int, V_TestMount_t);

	extern int v_get_n_exports(void);
	extern int v_get_current_export_fs(void);
	extern char *v_get_export_fs_name(int);
	extern int v_test_export_mount(char *, char *);
	extern int v_init_server_exports(char *);
	extern char *v_ipaddr_from_hostname(char *);

#ifdef __cplusplus
}

#endif

#endif				/* _V_RFS_H */
