#ifndef lint
#pragma ident "@(#)sw_lib.h 1.27 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _SW_LIB_INT_H
#define	_SW_LIB_INT_H

#include "sw_api.h"
#include "sw_swi.h"
#include "sw_swi_defines.h"

/* media flags */
#define	NEW_SERVICE		0x1	/* service is new	*/
#define	SVC_TO_BE_REMOVED	0x2	/* service is going to be removed */
#define	SVC_TO_BE_MODIFIED	0x4	/* service is going to be modified */
#define	SVC_UNCHANGED		0x0
#define SVC_MOD_MASK		0x7

#define	SPLIT_FROM_SERVER	0x8	/* service shares /var/sadm with
					 * local environment.
					 */

#define	BUILT_FROM_UPGRADE	0x10	/* env is built from an upgrade */
#define	BASIS_OF_UPGRADE	0x20	/* env is basis of an upgrade */

#define	MODIFIED_FILES_FOUND	0x40	/* the find_modified has been done
					 * for this environment.
					 */

#define svc_unchanged(mi)	\
	(((mi)->med_flags & SVC_MOD_MASK) == SVC_UNCHANGED)

#define	set_svc_modstate(mi, state)        \
	((mi)->med_flags = ((mi)->med_flags & ~SVC_MOD_MASK) | (state))

struct softinfo_desc {
	StringList *soft_arches;
	StringList *soft_packages;
};

struct patch_space_reqd {
	struct	patch_space_reqd *next;
	char	*patsp_arch;
	struct	patdir_entry {
		struct	patdir_entry *next;
		char	*patdir_dir;
		int	patdir_spooled;
		ulong	patdir_kbytes;
		ulong	patdir_inodes;
		char	*patdir_pkgid;
	}	*patsp_direntry;
};

/*	FUNCTION PROTOTYPES 	*/

#ifdef __cplusplus
extern "C" {
#endif

/* arch.c */

extern void	expand_arch(Modinfo *);
extern void	add_arch(Module *, char *);
extern void	add_package(Module *, Modinfo *);
extern void	add_4x(Module *, Modinfo *);
extern Node 	*add_null_pkg(Module *, char *);
extern int	update_selected_arch(Node *, caddr_t);
extern int	supports_arch(char *, char *);
extern int	media_supports_arch(Product *, char *);
extern void	extract_isa(char *, char *);
extern int	media_supports_isa(Product *, char *);
extern int	fullarch_is_selected(Product *, char *);
extern int	fullarch_is_loaded(Product *, char *);
extern int	isa_is_selected(Product *, char *);
extern int	isa_is_loaded(Product *, char *);
extern int	isa_of_arch_is(char *, char *);
extern int	arch_is_selected(Product *, char *);

/* depend.c */

extern void	parse_instance_spec(Depend *, char *);
extern void	read_pkg_depends(Module *, Modinfo *);

/* do_upgrade.c */
extern void	init_upg_fs_overhead(void);

/* find_modified.c */

extern void	find_modified(Module *);
extern void	canoninplace(char *);

/* free.c */

extern void	free_np_modinfo(Node *);
extern void	free_media(Module *);
extern void	free_list(List *);
extern void	free_np_module(Node *);
extern void	free_np_view(Node *);
extern void	free_arch(Arch *);
extern void	free_full_view(Module *, Module *);
extern void	free_modinfo(Modinfo *);
extern void	free_sw_config_list(SW_config *swcfg);
extern void	free_platform(Platform *plat);
extern void	free_platgroup(PlatGroup *platgrp);
extern void	free_hw_config(HW_config *hwcfg);
extern void	free_file(struct file *);
extern void	free_patch_instances(Modinfo *);

/* install.c */

extern Modinfo	*	find_owning_inst(char *, Modinfo *);
extern int	is_new_var_sadm(char *);
extern char	*INST_RELEASE_read_path(char *);
extern char	*services_read_path(char *);
extern char	*CLUSTER_read_path(char *);
extern char	*clustertoc_read_path(char *);
extern void	split_svr_svc(Module *, Module *);
extern void	set_cluster_status(Module *);
extern Module	*add_new_service(char *);

/* locale.c */

extern void	localize_packages(Module *);
extern int	add_locale(Module *, char *);
extern void	sort_locales(Module *);
extern char	*get_lang_from_loc_array(char *);
extern char	*get_C_lang_from_locale(Module *, char *);

/* locale_lookup.c */
extern void	read_locale_table(Module *media);
extern char	*get_locale_description(char *, char *);
extern char	*get_locale_desc_from_media(char *, char *);

/* media.c */

extern Module *	duplicate_media(Module *);
extern void	dup_clstr_tree(Module *, Module *);
extern Module * find_service_media(char *);

/* module.c */

extern void	mark_submodules(Module *, ModStatus);

/* mount_all.c */
extern int	gen_mount_script(FILE *, int);
extern void	gen_umount_script(FILE *);
extern int	umount_root(void);
extern void	gen_installboot(FILE *);
extern void	set_profile_upgrade(void);
extern char *	get_failed_mntdev(void);

/* pkghist.c */

extern void	read_pkg_history_file(char *);
extern void	read_cls_history_file(char *);
extern void	free_history(struct pkg_hist *);

/* platform.c */
extern int	load_platforms(Module *);
extern void	load_installed_platforms(Module *);
extern void	upg_write_platform_file(FILE *, char *, Product *, Product *);
extern void	upg_write_plat_softinfo(FILE *, Product *, Product *);

/* proc.c */

extern int	proc_walk(int (*)(int, char *), char *);
extern int	proc_running(int, char *);
extern int	proc_kill(int, char *);

/* prod.c */

extern int	load_all_products(Module *, int);
extern int	load_clusters(Module *, char *);
extern void	promote_packages(Module *, Module *, Module *);
extern int	load_pkginfo(Module *, char *, int);
extern char *	get_value(char *, char);

/* service.c */

extern void	remove_all_services(void);
extern int	remove_service(Module *, char *);
extern int	add_service(Module *, char *, Module *);

/* sp_calc.c */
extern daddr_t	upg_percent_free_space(char *);
extern void	set_upg_fs_overhead(int *);
 
/* sp_load.c */
extern int	sp_load_contents(Product *, Product *);

/* sp_space.c */
extern void	init_save_files(void);
extern int	save_files(char *);
extern int	calc_pkg_space(char *, Modinfo *);
extern int	final_space_chk(void);

/* write_script.c */
extern int	write_script(Module *);
extern void	scriptwrite(FILE *, char **, ...);
extern void	set_umount_script_fcn(int (*)(FILE *, int), void (*)(FILE *));
extern int	is_KBI_service(Product *);
extern char *	upgrade_script_path(Product *);
extern char *	upgrade_log_path(Product *);

/* update_actions.c */
extern int	update_module_actions(Module *, Module *, Action,
    Environ_Action);
extern char *	split_name(char **);
extern void	unreq_nonroot(Module *);
extern Modinfo *find_new_package(Product *, char *, char*,
    Arch_match_type *);
extern void	generate_swm_script(char *);
extern int	is_server(void);
extern int	is_dataless_machine(void);
extern void	mark_preserved(Module *);
extern void	mark_removed(Module *);
extern Module *	get_localmedia(void);
extern void	set_final_upgrade_mode(int);

/* util.c */

extern File *	crackfile(char *, char *, FileType);
extern int	sort_packages(Module *, char *);
extern Module *	sort_modules(Module *);
extern int	isa_handled(char *);
extern void	isa_handled_clear(void);
extern void	set_primary_arch(Module *);
extern void	link_to(Item **, Item *);
extern int	get_sw_debug(void);
extern int	string_in_list(StringList *, char *);
extern void	sort_ordered_pkglist(Module *);

/* v_version.c */

extern int	pkg_fullver_cmp(Modinfo *, Modinfo *);

/* view.c */
extern void	clear_all_view(Module *);
extern int	load_default_view(Module *);
extern int	load_view(Module *, Module *);
extern int	load_local_view(Module *);
extern Module *	get_current_view(Module *);
extern void	clear_view(Module *);
extern int	has_view(Module *, Module *);

/* walktree.c */

extern void	walktree(Module *, int (*)(Modinfo *, caddr_t), caddr_t);


#ifdef __cplusplus
}
#endif

#endif _SW_LIB_INT_H
