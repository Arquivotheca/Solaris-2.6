#ifndef lint
#pragma ident "@(#)sw_swi_defines.h 1.8 96/06/10 SMI"
#endif
/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SW_SWI_DEFINES_H
#define	_SW_SWI_DEFINES_H

/*		FUNCTION PROTOTYPES 		*/

#ifdef __cplusplus
#define { {
#endif

/* soft_admin.c */

#define getset_admin_file swi_getset_admin_file
#define admin_write swi_admin_write

/* soft_arch.c */

#define get_default_arch swi_get_default_arch
#define get_default_impl swi_get_default_impl
#define get_all_arches swi_get_all_arches
#define package_selected swi_package_selected
#define select_arch swi_select_arch
#define deselect_arch swi_deselect_arch
#define mark_arch swi_mark_arch
#define valid_arch swi_valid_arch

/* soft_depend.c */

#define check_sw_depends swi_check_sw_depends
#define get_depend_pkgs swi_get_depend_pkgs

/* soft_install.c */

#define load_installed swi_load_installed
#define next_patch swi_next_patch
#define next_inst swi_next_inst

/* soft_locale.c */

#define get_all_locales swi_get_all_locales
#define select_locale swi_select_locale
#define deselect_locale swi_deselect_locale
#define valid_locale swi_valid_locale

/* soft_media.c */

#define add_media swi_add_media
#define add_specific_media swi_add_specific_media
#define load_media swi_load_media
#define unload_media swi_unload_media
#define set_eject_on_exit swi_set_eject_on_exit
#define get_media_head swi_get_media_head
#define find_media swi_find_media

/* soft_module.c */

#define set_current swi_set_current
#define set_default swi_set_default
#define get_current_media swi_get_current_media
#define get_current_service swi_get_current_service
#define get_current_product swi_get_current_product
#define get_current_category swi_get_current_category
#define get_current_metacluster swi_get_current_metacluster
#define get_local_metacluster swi_get_local_metacluster
#define get_current_cluster swi_get_current_cluster
#define get_current_package swi_get_current_package
#define get_default_media swi_get_default_media
#define get_default_service swi_get_default_service
#define get_default_product swi_get_default_product
#define get_default_category swi_get_default_category
#define get_default_metacluster swi_get_default_metacluster
#define get_default_cluster swi_get_default_cluster
#define get_default_package swi_get_default_package
#define get_next swi_get_next
#define get_sub swi_get_sub
#define get_prev swi_get_prev
#define get_head swi_get_head
#define mark_required swi_mark_required
#define mark_module swi_mark_module
#define mod_status swi_mod_status
#define toggle_module swi_toggle_module
#define get_current_locale swi_get_current_locale
#define set_current_locale swi_set_current_locale
#define get_default_locale swi_get_default_locale
#define toggle_product swi_toggle_product
#define partial_status swi_partial_status

/* soft_prod.c */

#define get_clustertoc_path swi_get_clustertoc_path
#define media_category swi_media_category

/* soft_util.c */

#define set_instdir_svc_svr swi_set_instdir_svc_svr
#define clear_instdir_svc_svr swi_clear_instdir_svc_svr
#define gen_pboot_path swi_gen_pboot_path
#define gen_bootblk_path swi_gen_bootblk_path
#define gen_openfirmware_path swi_gen_openfirmware_path

/* soft_dump.c */

#define dumptree swi_dumptree

/* soft_update_actions.c */

#define load_clients swi_load_clients
#define update_action swi_update_action
#define upg_select_locale swi_upg_select_locale
#define upg_deselect_locale swi_upg_deselect_locale

/* soft_platform.c */

#define write_platform_file swi_write_platform_file

/* soft_v_version.c */

#define prod_vcmp swi_prod_vcmp
#define pkg_vcmp swi_pkg_vcmp
#define is_patch swi_is_patch
#define is_patch_of swi_is_patch_of

/* soft_sp_util.c */
#define valid_mountp swi_valid_mountp

/* soft_sp_space.c */

#define free_space_tab swi_free_space_tab
#define free_fsspace swi_free_fsspace
#define calc_cluster_space swi_calc_cluster_space
#define calc_tot_space swi_calc_tot_space
#define tot_pkg_space swi_tot_pkg_space
#define calc_sw_fs_usage swi_calc_sw_fs_usage

#ifdef __cplusplus
}
#endif

#endif _SW_SWI_DEFINES_H
