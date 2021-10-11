#ifndef lint
#pragma ident "@(#)sw_swi_defines.h 1.7 95/05/31 SMI"
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

/* admin.c */

#define admin_file swi_admin_file
#define admin_write swi_admin_write

/* arch.c */

#define get_all_arches swi_get_all_arches
#define package_selected swi_package_selected
#define get_default_arch swi_get_default_arch
#define get_default_impl swi_get_default_impl
#define get_default_inst swi_get_default_inst
#define get_default_machine swi_get_default_machine
#define get_default_platform swi_get_default_platform
#define get_actual_platform swi_get_actual_platform
#define select_arch swi_select_arch
#define deselect_arch swi_deselect_arch
#define mark_arch swi_mark_arch
#define valid_arch swi_valid_arch

/* client.c */

#define name2ipaddr swi_name2ipaddr
#define test_mount swi_test_mount
#define get_rfs_test_status swi_get_rfs_test_status
#define set_rfs_test_status swi_set_rfs_test_status

/* depend.c */

#define check_sw_depends swi_check_sw_depends
#define get_depend_pkgs swi_get_depend_pkgs

/* install.c */

#define load_installed swi_load_installed
#define next_patch swi_next_patch
#define next_inst swi_next_inst

/* locale.c */

#define get_all_locales swi_get_all_locales
#define update_l10n_package_status swi_update_l10n_package_status
#define select_locale swi_select_locale
#define deselect_locale swi_deselect_locale
#define mark_locales swi_mark_locales
#define valid_locale swi_valid_locale

/* media.c */

#define add_media swi_add_media
#define add_specific_media swi_add_specific_media
#define load_media swi_load_media
#define mount_media swi_mount_media
#define unload_media swi_unload_media
#define set_eject_on_exit swi_set_eject_on_exit
#define get_media_head swi_get_media_head
#define find_media swi_find_media

/* module.c */

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
#define get_machinetype swi_get_machinetype
#define set_machinetype swi_set_machinetype
#define get_current_locale swi_get_current_locale
#define set_rootdir swi_set_rootdir
#define set_current_locale swi_set_current_locale
#define get_default_locale swi_get_default_locale
#define get_rootdir swi_get_rootdir
#define toggle_product swi_toggle_product
#define mark_module_action swi_mark_module_action
#define partial_status swi_partial_status

/* mount.c */

#define mount_fs swi_mount_fs
#define umount_fs swi_umount_fs
#define share_fs swi_share_fs
#define unshare_fs swi_unshare_fs

/* pkgexec.c */

#define add_pkg  swi_add_pkg 
#define remove_pkg  swi_remove_pkg 


/* prod.c */

#define get_clustertoc_path swi_get_clustertoc_path
#define path_is_readable swi_path_is_readable
#define media_category swi_media_category

/* util.c */

#define sw_lib_init swi_sw_lib_init
#define get_err_str swi_get_err_str
#define error_and_exit swi_error_and_exit
#define xcalloc swi_xcalloc
#define xmalloc swi_xmalloc
#define xrealloc swi_xrealloc
#define xstrdup swi_xstrdup
#define deselect_usr_pkgs swi_deselect_usr_pkgs
#define set_instdir_svc_svr swi_set_instdir_svc_svr
#define clear_instdir_svc_svr swi_clear_instdir_svc_svr
#define set_action_for_machine_type swi_set_action_for_machine_type
#define sort_space_fs swi_sort_space_fs
#define percent_free_space swi_percent_free_space
#define set_sw_debug swi_set_sw_debug
#define gen_bootblk_path swi_gen_bootblk_path
#define gen_pboot_path swi_gen_pboot_path
#define gen_openfirmware_path swi_gen_openfirmware_path

/* dump.c */

#define dumptree swi_dumptree

/* update_actions.c */

#define load_clients swi_load_clients
#define update_action swi_update_action
#define upg_select_locale swi_upg_select_locale
#define upg_deselect_locale swi_upg_deselect_locale

/* do_upgrade.c */

#define set_debug swi_set_debug
#define set_skip_mod_search swi_set_skip_mod_search
#define set_pkg_hist_file swi_set_pkg_hist_file
#define set_onlineupgrade_mode swi_set_onlineupgrade_mode
#define upgrade_all_envs swi_upgrade_all_envs
#define local_upgrade swi_local_upgrade
#define do_upgrade swi_do_upgrade
#define do_product_upgrade swi_do_product_upgrade
#define do_find_modified swi_do_find_modified
#define do_final_space_check swi_do_final_space_check
#define do_write_upgrade_script swi_do_write_upgrade_script
#define nonnative_upgrade swi_nonnative_upgrade

/* platform.c */
#define write_platform_file swi_write_platform_file

/* mountall.c */

#define mount_and_add_swap swi_mount_and_add_swap
#define umount_and_delete_swap swi_umount_and_delete_swap
#define umount_all swi_umount_all
#define unswap_all swi_unswap_all

/* upg_recover.c */

#define partial_upgrade swi_partial_upgrade
#define resume_upgrade swi_resume_upgrade

/* v_version.c */

#define prod_vcmp swi_prod_vcmp
#define pkg_vcmp swi_pkg_vcmp
#define is_patch swi_is_patch
#define is_patch_of swi_is_patch_of

/* sp_calc.c */

#define min_req_space swi_min_req_space

/* sp_util.c */
#define valid_mountp swi_valid_mountp

/* sp_space.c */

#define free_space_tab swi_free_space_tab
#define space_meter swi_space_meter
#define swm_space_meter swi_swm_space_meter
#define upg_space_meter swi_upg_space_meter
#define calc_cluster_space swi_calc_cluster_space
#define calc_tot_space swi_calc_tot_space
#define tot_pkg_space swi_tot_pkg_space

/* sp_print_results.c */

#define print_final_results swi_print_final_results
#define gen_final_space_report swi_gen_final_space_report
#define free_final_space_report swi_free_final_space_report

#ifdef __cplusplus
}
#endif

#endif _SW_SWI_DEFINES_H
