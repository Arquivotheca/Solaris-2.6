#ifndef lint
#pragma ident "@(#)v_sw.h 1.39 96/04/30 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_sw.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_SW_H
#define	_V_SW_H

typedef struct sizes_t {
	char sz[11][12];
}	Sizes_t;

#ifdef __cplusplus
extern "C" {
#endif

	extern void v_set_swhier_root(Module * mod);


	/* ---- product functions ---- */
	extern int v_init_sw(char *dir);
	extern int v_set_init_sw_config(void);

	extern int v_count_products(void);
	extern char *v_get_product_name(void);
	extern char *v_get_product_version(void);
	extern void v_restore_default_view(void);

	/* ---- metacluster functions ---- */
	extern int v_is_current_metaclst(int);
	extern void v_set_current_metaclst(int);
	extern int v_get_current_metaclst(void);
	extern char *v_get_metaclst_name(int i);
	extern char *v_get_metaclst_size(int i);
	extern int v_get_n_metaclsts(void);
	extern void v_set_metaclst_dflt_sizes();

	extern int v_metaclst_edited(void);
	extern int v_get_n_metaclst_deltas(int);
	extern char *v_get_delta_package_name(int);
	/* ---- locale functions ----- */

	extern int v_get_n_locales(void);
	extern int v_get_locale_status(int);
	extern int v_set_locale_status(int, int);
	extern int v_get_current_locale(void);
	extern int v_set_current_locale(int);

	extern int v_set_default_locale(char *);
	extern char *v_get_default_locale(void);

	extern char *v_get_locale_name(int);
	extern char *v_get_locale_language(int);

	/* ---- architecture functions ---- */

	extern int v_get_n_arches(void);
	extern int v_get_current_arch(void);
	extern int v_set_current_arch(int);
	extern int v_get_arch_status(int);
	extern int v_set_arch_status(int, int);
	extern void v_init_native_arch(void);
	extern void v_clear_nonnative_arches(void);
	extern int v_is_native_arch(int);

	extern char *v_get_arch_name(int i);
	extern char *v_get_selected_arches(void);

	extern char *v_get_pkgname_from_pkgid(char *);

	/* ---- generic module functions ---- */

	extern int v_next_module(void);
	extern int v_prev_module(void);
	extern int v_expand_module(int);
	extern int v_contract_module(int);
	extern int v_get_module_level(int);

	extern int v_get_n_modules(void);

	extern int v_get_submods_are_shown(int);
	extern int v_get_module_has_submods(int);

	extern int v_get_current_module(void);
	extern int v_set_current_module(int);
	extern int v_current_module_is_metaclst(void);

	extern V_ModType_t v_get_module_type(int);
	extern int v_get_module_status(int);
	extern int v_set_module_status(int);
	extern int v_get_module_class(int);

	extern int v_set_module_basedir(int, char *);
	extern char *v_get_module_basedir(int);
	extern char *v_get_module_id(int);
	extern char *v_get_module_name(int);
	extern char *v_get_module_description(int);
	extern char *v_get_module_version(int);
	extern char *v_get_module_vendor(int);
	extern char *v_get_module_arches(int);
	extern int v_get_size_in_kbytes(char *);
	extern char *v_get_module_size(int);
	extern Sizes_t *v_get_module_fsspace_used(int);
	extern unsigned int v_get_total_kb_to_install(void);

	extern int v_get_n_selected_packages(void);

#ifdef __cplusplus
}

#endif

#endif				/* _V_SW_H */
