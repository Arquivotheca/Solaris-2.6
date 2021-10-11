#ifndef lint
#pragma ident "@(#)sw_swi.h 1.8 96/06/10 SMI"
#endif
/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SW_SWI_H
#define	_SW_SWI_H

#include "spmisoft_api.h"



/*		FUNCTION PROTOTYPES 		*/

#ifdef __cplusplus
extern "C" {
#endif

void	enter_swlib(char *);
void	exit_swlib(void);

/*
 * internal version of external functions.  These functions are all
 * called by "wrappers" when used by external functions.
 */

/* soft_admin.c */

char	*swi_getset_admin_file(char *);
int	swi_admin_write(char *, Admin_file *);

/* soft_arch.c */

char	*swi_get_default_arch(void);
char	*swi_get_default_impl(void);
Arch	*swi_get_all_arches(Module *);
int	swi_package_selected(Node *, char *);
int	swi_select_arch(Module *, char *);
int	swi_deselect_arch(Module *, char *);
void	swi_mark_arch(Module *);
int	swi_valid_arch(Module *, char *);

/* soft_depend.c */

int	swi_check_sw_depends(void);
Depend	*swi_get_depend_pkgs(void);

/* soft_install.c */

Module	*swi_load_installed(char *, int);
Modinfo	*swi_next_patch(Modinfo *);
Modinfo	*swi_next_inst(Modinfo *);

/* soft_locale.c */

Module	*swi_get_all_locales(void);
int	swi_select_locale(Module *, char *);
int	swi_deselect_locale(Module *, char *);
int	swi_valid_locale(Module *, char *);

/* soft_media.c */

Module	*swi_add_media(char *);
Module	*swi_add_specific_media(char *, char *);
int	swi_load_media(Module *, int);
int	swi_unload_media(Module *);
void	swi_set_eject_on_exit(int);
Module	*swi_get_media_head(void);
Module	*swi_find_media(char *, char *);

/* soft_module.c */

int 	swi_set_current(Module *);
int	swi_set_default(Module *);
Module	*swi_get_current_media(void);
Module	*swi_get_current_service(void);
Module	*swi_get_current_product(void);
Module	*swi_get_current_category(ModType);
Module	*swi_get_current_metacluster(void);
Module	*swi_get_local_metacluster(void);
Module	*swi_get_current_cluster(void);
Module	*swi_get_current_package(void);
Module	*swi_get_default_media(void);
Module	*swi_get_default_service(void);
Module	*swi_get_default_product(void);
Module	*swi_get_default_category(ModType);
Module	*swi_get_default_metacluster(void);
Module	*swi_get_default_cluster(void);
Module	*swi_get_default_package(void);
Module	*swi_get_next(Module *);
Module	*swi_get_sub(Module *);
Module	*swi_get_prev(Module *);
Module	*swi_get_head(Module *);
int	swi_mark_required(Module *);
int	swi_mark_module(Module *, ModStatus);
int	swi_mod_status(Module *);
int	swi_toggle_module(Module *);
char	*swi_get_current_locale(void);
void	swi_set_current_locale(char *);
char	*swi_get_default_locale(void);
int	swi_toggle_product(Module *, ModStatus);
int	swi_mark_module_action(Module *, Action);
int	swi_partial_status(Module *);

/* soft_prod.c */

char	*swi_get_clustertoc_path(Module *);
void	swi_media_category(Module *);

/* soft_util.c */

void	swi_sw_lib_init(int);
int	swi_set_instdir_svc_svr(Module *);
void	swi_clear_instdir_svc_svr(Module *);
char	*swi_gen_bootblk_path(char *);
char	*swi_gen_pboot_path(char *);
char	*swi_gen_openfirmware_path(char *);

/* soft_dump.c */

int	swi_dumptree(char *);

/* soft_update_actions.c */

int	swi_load_clients(void);
void	swi_update_action(Module *);
void	swi_upg_select_locale(Module *, char *);
void	swi_upg_deselect_locale(Module *, char *);

/* soft_platform.c */
int	swi_write_platform_file(char *, Module *);

/* soft_v_version.c */

int	swi_prod_vcmp(char *, char *);
int	swi_pkg_vcmp(char *, char *);
int	swi_is_patch(Modinfo *);
int	swi_is_patch_of(Modinfo *, Modinfo *);

/* soft_sp_util.c */
int	swi_valid_mountp(char *);

/* soft_sp_space.c */

FSspace **swi_calc_cluster_space(Module *, ModStatus);
ulong	swi_calc_tot_space(Product *);
ulong	swi_tot_pkg_space(Modinfo *);
int	swi_calc_sw_fs_usage(FSspace **, int (*)(void *, void *), void *);
void    swi_free_fsspace(FSspace *);

#ifdef __cplusplus
}
#endif

#endif _SW_SWI_H
