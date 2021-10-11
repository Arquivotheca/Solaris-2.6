#ifndef lint
#pragma ident "@(#)sw_swi.h 1.8 95/05/31 SMI"
#endif
/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SW_SWI_H
#define	_SW_SWI_H

#include "sw_api.h"



/*		FUNCTION PROTOTYPES 		*/

#ifdef __cplusplus
extern "C" {
#endif

extern void	enter_swlib(char *);
extern void	exit_swlib(void);

/*
 * internal version of external functions.  These functions are all
 * called by "wrappers" when used by external functions.
 */

/* admin.c */

extern char	*swi_admin_file(char *);
extern int	swi_admin_write(char *, Admin_file *);

/* arch.c */

extern Arch *	swi_get_all_arches(Module *);
extern int	swi_package_selected(Node *, char *);
extern char *	swi_get_default_arch(void);
extern char *	swi_get_default_impl(void);
extern char *	swi_get_default_inst(void);
extern char *	swi_get_default_machine(void);
extern char *	swi_get_default_platform(void);
extern char *	swi_get_actual_platform(void);
extern int	swi_select_arch(Module *, char *);
extern int	swi_deselect_arch(Module *, char *);
extern void	swi_mark_arch(Module *);
extern int	swi_valid_arch(Module *, char *);

/* client.c */

extern char *	swi_name2ipaddr(char *);
extern int	swi_test_mount(Remote_FS *, int);
extern TestMount	swi_get_rfs_test_status(Remote_FS *);
extern int	swi_set_rfs_test_status(Remote_FS *, TestMount);

/* depend.c */

extern int	swi_check_sw_depends(void);
extern Depend * swi_get_depend_pkgs(void);

/* install.c */

extern Module *	swi_load_installed(char *, int);
extern Modinfo *swi_next_patch(Modinfo *);
extern Modinfo *swi_next_inst(Modinfo *);

/* locale.c */

extern Module *	swi_get_all_locales(void);
extern void	swi_update_l10n_package_status(Module *);
extern int	swi_select_locale(Module *, char *);
extern int	swi_deselect_locale(Module *, char *);
extern void	swi_mark_locales(Module *, ModStatus);
extern int	swi_valid_locale(Module *, char *);

/* media.c */

extern Module *	swi_add_media(char *);
extern Module *	swi_add_specific_media(char *, char *);
extern int	swi_load_media(Module *, int);
extern int	swi_mount_media(Module *, char *, MediaType);
extern int	swi_unload_media(Module *);
extern void	swi_set_eject_on_exit(int);
extern Module *	swi_get_media_head(void);
extern Module *	swi_find_media(char *, char *);

/* module.c */

extern int 	swi_set_current(Module *);
extern int	swi_set_default(Module *);
extern Module *	swi_get_current_media(void);
extern Module *	swi_get_current_service(void);
extern Module *	swi_get_current_product(void);
extern Module *	swi_get_current_category(ModType);
extern Module *	swi_get_current_metacluster(void);
extern Module *	swi_get_local_metacluster(void);
extern Module *	swi_get_current_cluster(void);
extern Module *	swi_get_current_package(void);
extern Module *	swi_get_default_media(void);
extern Module *	swi_get_default_service(void);
extern Module *	swi_get_default_product(void);
extern Module *	swi_get_default_category(ModType);
extern Module *	swi_get_default_metacluster(void);
extern Module *	swi_get_default_cluster(void);
extern Module *	swi_get_default_package(void);
extern Module *	swi_get_next(Module *);
extern Module *	swi_get_sub(Module *);
extern Module *	swi_get_prev(Module *);
extern Module *	swi_get_head(Module *);
extern int	swi_mark_required(Module *);
extern int	swi_mark_module(Module *, ModStatus);
extern int	swi_mod_status(Module *);
extern int	swi_toggle_module(Module *);
extern MachineType swi_get_machinetype(void);
extern void	swi_set_machinetype(MachineType);
extern char *	swi_get_current_locale(void);
extern void	swi_set_rootdir(char *);
extern void	swi_set_current_locale(char *);
extern char *	swi_get_default_locale(void);
extern char *	swi_get_rootdir(void);
extern int	swi_toggle_product(Module *, ModStatus);
extern int	swi_mark_module_action(Module *, Action);
extern int	swi_partial_status(Module *);

/* mount.c */

extern int	swi_mount_fs(char *, char *, char *);
extern int	swi_umount_fs(char *);
extern int	swi_share_fs(char *);
extern int	swi_unshare_fs(char *);

/* pkgexec.c */

extern int	swi_add_pkg(char *, PkgFlags *, char *);
extern int	swi_remove_pkg(char *, PkgFlags *);


/* prod.c */

extern char *	swi_get_clustertoc_path(Module *);
extern int	swi_path_is_readable(char *);
extern void	swi_media_category(Module *);

/* util.c */

extern void	swi_sw_lib_init(void(*)(int), int, int);
extern char *	swi_get_err_str(int);
extern void	swi_error_and_exit(int);
extern void *	swi_xcalloc(size_t size);
extern void *	swi_xmalloc(size_t);
extern void *	swi_xrealloc(void *, size_t);
extern char *	swi_xstrdup(char *);
extern void	swi_deselect_usr_pkgs(Module *);
extern int	swi_set_instdir_svc_svr(Module *);
extern void	swi_clear_instdir_svc_svr(Module *);
extern void	swi_set_action_for_machine_type(Module *);
extern Space **	swi_sort_space_fs(Space **, char **);
extern int	swi_percent_free_space(void);
extern int	swi_set_sw_debug(int);
extern char *	swi_gen_bootblk_path(char *);
extern char *	swi_gen_pboot_path(char *);
extern char *	swi_gen_openfirmware_path(char *);

/* dump.c */

extern int	swi_dumptree(char *);

/* update_actions.c */

extern int	swi_load_clients(void);
extern void	swi_update_action(Module *);
extern void	swi_upg_select_locale(Module *, char *);
extern void	swi_upg_deselect_locale(Module *, char *);

/* do_upgrade.c */

extern void	swi_set_debug(char *);
extern void	swi_set_skip_mod_search(void);
extern void	swi_set_pkg_hist_file(char *);
extern void	swi_set_onlineupgrade_mode(void);
extern int	swi_upgrade_all_envs(void);
extern int	swi_local_upgrade(void);
extern int	swi_do_upgrade(void);
extern int	swi_do_product_upgrade(Module *);
extern int	swi_do_find_modified(void);
extern int	swi_do_final_space_check(void);
extern void	swi_do_write_upgrade_script(void);
extern int	swi_nonnative_upgrade(StringList *);

/* platform.c */
extern int	swi_write_platform_file(char *, Module *);

/* mountall.c */

extern int	swi_mount_and_add_swap(char *);
extern int	swi_umount_and_delete_swap(void);
extern int	swi_umount_all(void);
extern int	swi_unswap_all(void);

/* upg_recover.c */

extern int	swi_partial_upgrade(void);
extern int	swi_resume_upgrade(void);

/* v_version.c */

extern int	swi_prod_vcmp(char *, char *);
extern int	swi_pkg_vcmp(char *, char *);
extern int	swi_is_patch(Modinfo *);
extern int	swi_is_patch_of(Modinfo *, Modinfo *);

/* sp_calc.c */

extern u_int	swi_min_req_space(u_int);

/* sp_util.c */
extern int	swi_valid_mountp(char *);

/* sp_space.c */

extern void	swi_free_space_tab(Space **);
extern Space	**swi_space_meter(char **);
extern Space	**swi_swm_space_meter(char **);
extern Space	**swi_upg_space_meter(void);
extern Space 	**swi_calc_cluster_space(Module *, ModStatus, int);
extern Space 	**swi_calc_tot_space(Product *);
extern long	swi_tot_pkg_space(Modinfo *);

/* sp_print_results.c */

extern void	swi_print_final_results(char *);
extern SW_space_results	*swi_gen_final_space_report();
extern void	swi_free_final_space_report(SW_space_results *);

#ifdef __cplusplus
}
#endif

#endif _SW_SWI_H
