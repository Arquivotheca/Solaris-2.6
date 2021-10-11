#ifndef lint
#pragma ident "@(#)v_check.h 1.16 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_check.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_CHECK_H
#define	_V_CHECK_H

#define	PARTITION_WARNING 	0x01
#define	SOFTWARE_WARNING 	0x02
#define	REMOTEFS_WARNING 	0x04

typedef enum config_status_t {
	CONFIG_OK = 0,
	CONFIG_WARNING = 1
}	Config_Status_t;

#ifdef __cplusplus
extern "C" {
#endif

	extern Config_Status_t v_check_disks(void);
	extern Config_Status_t v_check_part(void);
	extern Config_Status_t v_check_sw_depends(void);

	extern int v_valid_host_ip_addr(char *);
	extern int v_valid_hostname(char *);
	extern int v_valid_filesys_name(char *);

	extern int v_get_n_small_filesys(void);
	extern char *v_get_small_filesys(int i);
	extern char *v_get_small_filesys_reqd(int i);
	extern char *v_get_small_filesys_avail(int i);
	extern int v_get_n_depends(void);
	extern char *v_get_depends_pkgid(int i);
	extern char *v_get_depends_pkgname(int i);
	extern char *v_get_dependson_pkgid(int i);
	extern char *v_get_dependson_pkgname(int i);

#ifdef __cplusplus
}

#endif

#endif				/* _V_CHECK_H */
