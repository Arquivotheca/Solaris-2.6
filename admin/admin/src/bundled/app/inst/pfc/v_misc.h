#ifndef lint
#pragma ident "@(#)v_misc.h 1.30 96/06/23 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_misc.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_MISC_H
#define	_V_MISC_H

/*
 * client data structure that is passed to the SystemUpdate backend
 * for the initial path
 * and is then passed back into all the update callbacks.
 */
typedef struct {
	int totalK;
	int remainingK;
	int doneK;
} pfcSUInitialData;

#ifdef __cplusplus
extern "C" {
#endif

	extern int v_cleanup_prev_install(void);
	extern void v_reset_view_libraries(void);
	extern void v_int_error_exit(int);

	extern void v_set_reboot(int val);
	extern int v_get_reboot(void);

	extern void v_set_progname(char *);
	extern char *v_get_progname(void);

	extern void v_set_environ(char **);
	extern char **v_get_environ(void);

	extern char *v_get_default_impl(void);
	extern char *v_get_default_inst(void);

	extern char *v_get_string_from_type(V_SystemType_t);
	extern void v_set_system_type(V_SystemType_t);
	extern V_SystemType_t v_get_system_type(void);

	extern void v_set_install_type(V_InstallType_t);

	extern int v_get_n_diskless_clients(void);
	extern int v_get_diskless_swap(void);
	extern int v_get_n_cache_clients(void);
	extern int v_get_root_client_size(void);
	extern void v_set_root_client_size(int);
	extern V_Status_t v_set_n_diskless_clients(int);
	extern V_Status_t v_set_diskless_swap(int);
	extern V_Status_t v_set_n_cache_clients(int);

	/* initial install backend and progress display */
	extern parAction_t v_do_install(void);
	extern int pfcSystemUpdateInitialCB(void *client_data, void *call_data);

	extern void v_exec_sh(int);
	extern void v_show_pkgs(void);

#ifdef __cplusplus
}
#endif

#endif				/* _V_MISC_H */
