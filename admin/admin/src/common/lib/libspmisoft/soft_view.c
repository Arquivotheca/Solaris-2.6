#ifndef lint
#pragma ident "@(#)soft_view.c 1.3 96/06/03 SMI"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */
#include "spmisoft_lib.h"
#include <string.h>
#include <stdlib.h>

/* Public Function Prototypes */

int      	load_default_view(Module *);
int      	load_view(Module *, Module *);
int		load_local_view(Module *);
Module * 	get_current_view(Module *);
void     	clear_view(Module *);
void     	clear_all_view(Module *);
int      	has_view(Module *, Module *);

/* Library Function Prototypes */

/* Local Function Prototypes */

static int	 change_view_mod(Node *, caddr_t);
static int	 change_view_locale(Node *, caddr_t);
static int	 change_view_arches(Node *, caddr_t);
static int	 save_view_pkg(Node *, caddr_t);
static int	 save_view_cluster(Node *, caddr_t);
static int	 save_view_4x(Node *, caddr_t data);
static int	 clear_package_action(Node *, caddr_t);
static int	 clear_cluster_action(Node *, caddr_t);
static void	 save_current_view(Module *);
static Product * create_view(Module *, Module *);
static int	 clear_package_view(Node *, caddr_t);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * load_default_view()
 *	Make the default view associated with the product 'prod'
 *	the current view, and save the current view away in the
 *	view lists.
 * Parameters:
 *	prod	- pointer to product module
 * Return:
 *	SUCCESS	- always returns this
 * Status:
 *	public
 */ 
int
load_default_view(Module * prod)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("load_default_view");
#endif

	if (prod->info.prod->p_current_view == prod->info.prod)
		return (SUCCESS);
	save_current_view(prod);
	walklist(prod->info.prod->p_view_4x, change_view_mod, NULL);
	walklist(prod->info.prod->p_view_pkgs, change_view_mod, NULL);
	walklist(prod->info.prod->p_view_cluster, change_view_mod, NULL);
	walklist(prod->info.prod->p_view_locale, change_view_locale, NULL);
	walklist(prod->info.prod->p_view_arches, change_view_arches, NULL);
	prod->info.prod->p_current_view = prod->info.prod;
	sync_l10n(prod);
	return (SUCCESS);
}

/*
 * load_view()
 *	Make the view associated with 'media' current, and store away
 *	the current view in the view lists.
 * Parameters:
 *	prod	- pointer to product module
 *	media	- pointer to media module against which the view is to
 *		  be made current
 * Return:
 *	SUCCESS	- always returns this value
 * Status:
 *	public
 */
int
load_view(Module * prod, Module *media)
{
	Product *p;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("load_view");
#endif

	for (p = prod->info.prod; p != NULL; p = p->p_next_view) {
		if (p->p_view_from == media) 
			break;
	}
	if (p == NULL) {
		save_current_view(prod);
		p = create_view(prod, media);
		load_default_view(prod);
		prod->info.prod->p_current_view = p;

	} else {
		if (prod->info.prod->p_current_view == p)
			return (SUCCESS);

		save_current_view(prod);
		walklist(p->p_view_4x, change_view_mod, NULL);
		walklist(p->p_view_pkgs, change_view_mod, NULL);
		walklist(p->p_view_cluster, change_view_mod, NULL);
		walklist(p->p_view_locale, change_view_locale, NULL);
		walklist(p->p_view_arches, change_view_arches, NULL);
		prod->info.prod->p_current_view = p;
		sync_l10n(prod);
	}
	return (SUCCESS);
}

/*
 * load_local_view()
 *	Load the view of the specified product from the "local"
 *	environment (that is, the environment of the system being
 *	being upgraded).  This function is only meaningful during
 *	an upgrade.  During an initial install, there won't be a
 *	local environment and this function will return "FAILURE".
 * Parameters:
 *	prod	- pointer to product module
 * Return:
 *	SUCCESS	- view was loaded.
 *	FAILURE - no local environment could be found
 * Status:
 *	public
 */
int
load_local_view(Module * prod)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("load_local_view");
#endif

	for (mod = get_media_head(); mod != NULL; mod = mod->next)
		if (mod->info.media->med_type == INSTALLED &&
		    mod->info.media->med_dir &&
		    strcmp(mod->info.media->med_dir, "/") == 0)
			return (load_view(prod, mod));
	return (FAILURE);
}

/*
 * has_view()
 *
 * Parameters:
 *	prod	-
 *	media	-
 * Return:
 *	SUCCESS	-
 *	FAILURE	-
 * Status:
 *	public
 */
int
has_view(Module * prod, Module * media)
{
	Product *p;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("has_view");
#endif

	for (p = prod->info.prod; p != NULL; p = p->p_next_view) {
		if (p->p_view_from == media) 
			return (SUCCESS);
	}
	return (FAILURE);
}

/*
 * get_current_view()
 *	Return a pointer to the media module associated with the
 *	current view wrt the product 'prod'.
 * Parameters:
 *	prod	 - pointer to product module containing the view
 * Return:
 *	NULL	 - no current view
 *	Module * - pointer to media module
 * Status:
 *	public
 */
Module *
get_current_view(Module * prod)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_view");
#endif

	if (prod != (Module *) NULL && prod->info.prod->p_current_view)
		return (prod->info.prod->p_current_view->p_view_from);

	return (NULL);
}

/*
 * clear_view()
 *	Set the product status to UNSELETED. For all packages and
 * 	clusters under the product, set the status to UNSELECTED,
 *	set the shared field to NOTDUPLICATE (if not a NULLPKG),
 *	set the action field to NO_ACTION_DEFINED, clear the reference
 *	count, and for packages only, free the current installation base
 *	directory field, if defined.
 * Parameters:
 *	prod	- pointer to product module
 * Return:
 *	none
 * Status:
 *	public
 */
void
clear_view(Module *prod)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("clear_view");
#endif

	prod->info.prod->p_status = UNSELECTED;
	walklist(prod->info.prod->p_clusters, clear_cluster_action, (caddr_t)0);
	walklist(prod->info.prod->p_packages, clear_package_action, (caddr_t)0);
	return;
}

/*
 * clear_all_view()
 * Parameters:
 *	prod	- pointer to product module
 * Return:
 *	none
 * Status:
 *	public
 */
void
clear_all_view(Module *prod)
{
	struct module	*mod;
	Arch		*ap;
	
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("clear_all_view");
#endif

	prod->info.prod->p_status = UNSELECTED;
	walklist(prod->info.prod->p_clusters, clear_cluster_action, (caddr_t)0);
	walklist(prod->info.prod->p_packages, clear_package_view, (caddr_t)0);
	
	for (mod = prod->info.prod->p_locale; mod != NULL; mod = mod->next)
		mod->info.locale->l_selected = 0;

	for (ap = prod->info.prod->p_arches; ap != NULL; ap = ap->a_next)
		ap->a_selected = 0;

	sync_l10n(prod);
	return;
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * clear_package_action()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
clear_package_action(Node * np, caddr_t data)
{
	Modinfo	*mi;

	for (mi = (Modinfo *)(np->data); mi != NULL; mi = next_inst(mi)) {
		mi->m_status = UNSELECTED;
		if (mi->m_shared != NULLPKG) {
			mi->m_shared = NOTDUPLICATE;
		}
		mi->m_action = NO_ACTION_DEFINED;
		mi->m_refcnt = 0;
		if (mi->m_instdir != NULL) {
			free(mi->m_instdir);
			mi->m_instdir = NULL;
		}
	}
	return (SUCCESS);
}

/*
 * clear_package_view()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
clear_package_view(Node * np, caddr_t data)
{
	Modinfo	*mi;

	for (mi = (Modinfo *)(np->data); mi != NULL; mi = next_inst(mi)) {
		mi->m_status = UNSELECTED;
		if (mi->m_shared != NULLPKG) {
			mi->m_shared = NOTDUPLICATE;
		}
		mi->m_action = NO_ACTION_DEFINED;
		mi->m_refcnt = 0;
		/* turn off everything but PART_OF_CLUSTER */
		mi->m_flags &= PART_OF_CLUSTER;
		if (mi->m_instdir != NULL) {
			free(mi->m_instdir);
			mi->m_instdir = NULL;
		}
	}
	return (SUCCESS);
}

/*
 * clear_cluster_action()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
clear_cluster_action(Node * np, caddr_t data)
{
	Modinfo	*mi;
	mi = ((Module *)(np->data))->info.mod;
	mi->m_status = UNSELECTED;
	mi->m_shared = NOTDUPLICATE;
	mi->m_action = NO_ACTION_DEFINED;
	mi->m_refcnt = 0;
	return (SUCCESS);
}


/*
 * change_view_mod()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
change_view_mod(Node * np, caddr_t data)
{
	View	 *view;
	Modinfo  *info;

	view = (View *)np->data;
	view->v_info.v_mod->m_status = *view->v_status_ptr;
	view->v_info.v_mod->m_shared = view->v_shared;
	view->v_info.v_mod->m_action = view->v_action;
	view->v_info.v_mod->m_refcnt = view->v_refcnt;
	view->v_info.v_mod->m_instdir = view->v_instdir;
	view->v_info.v_mod->m_flags = view->v_flags;
	view->v_info.v_mod->m_fs_usage = view->v_fs_usage;
	for (info = next_inst(view->v_info.v_mod); info; info = next_inst(info)){
		view = view->v_instances;
		info->m_action = view->v_action;
		info->m_refcnt = view->v_refcnt;
		info->m_instdir = view->v_instdir;
		info->m_flags = view->v_flags;
		info->m_fs_usage = view->v_fs_usage;
		info->m_status = *view->v_status_ptr;
	}
	return (SUCCESS);
}

/*
 * change_view_locale()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	 SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
change_view_locale(Node * np, caddr_t data)
{
	View	*view;

	view = (View *)np->data;
	view->v_info.v_locale->l_selected = *view->v_status_ptr;
	return (SUCCESS);
}

/*
 * change_view_arches()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 *	 SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
change_view_arches(Node * np, caddr_t data)
{
	View	*view;

	view = (View *)np->data;
	view->v_info.v_arch->a_selected = *view->v_status_ptr;
	return (SUCCESS);
}

/*
 * save_current_view()
 * Parameters:
 *	prod	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static void
save_current_view(Module * prod)
{
	Product *p;
	View    *v;
	Node    *n;
	Arch    *a;
	Module  *mp;

	p = prod->info.prod->p_current_view;
	if (p->p_sw_4x) 
		walklist(p->p_sw_4x, save_view_4x, (caddr_t)p);
	if (p->p_packages) 
		walklist(p->p_packages, save_view_pkg, (caddr_t)p);
	if (p->p_clusters) 
		walklist(p->p_clusters, save_view_cluster, (caddr_t)p);
	if (p->p_view_arches == NULL) 
		p->p_view_arches = getlist();
	for (a=p->p_arches; a != NULL; a = a->a_next) {
		if ((n = findnode(p->p_view_arches, a->a_arch)) == NULL) {
			v = (View *)xcalloc(sizeof (View));
			v->v_type = ARCH;
			v->v_status_ptr = &v->v_status;
			*v->v_status_ptr = a->a_selected;
			v->v_info.v_arch = a;
			n = getnode();
			n->key = xstrdup(a->a_arch);
			n->data = v;
			n->delproc = &free_np_view;
			(void) addnode(p->p_view_arches, n);
		} else {
			v = (View *)n->data;
			*v->v_status_ptr = a->a_selected;
		}
	}
	if (p->p_view_locale == NULL) 
		p->p_view_locale = getlist();
	for (mp=p->p_locale; mp != NULL; mp = mp->next) {
		if ((n = findnode(p->p_view_locale, mp->info.locale->l_locale)) 
								== NULL) {
			v = (View *)xcalloc(sizeof (View));
			v->v_type = LOCALE;
			v->v_info.v_locale = mp->info.locale;
			v->v_status_ptr = &v->v_status;
			*v->v_status_ptr = mp->info.locale->l_selected;
			n = getnode();
			n->key = xstrdup(mp->info.locale->l_locale);
			n->data = v;
			n->delproc = &free_np_view;    
			(void) addnode(p->p_view_locale, n);
		} else {
			v = (View *)n->data;
			*v->v_status_ptr = mp->info.locale->l_selected;
		}
	}
}

/*
 * save_view_pkg()
 * Parameters:
 *	np	- node pointer
 *	data	- 
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
save_view_pkg(Node * np, caddr_t data)
{
	Modinfo *info;
	Product *p;
	View    *v;
	Node    *n;

	info = (Modinfo *)np->data;
	/*LINTED [alignment ok]*/
	p = (Product *)data;

	if (p->p_view_pkgs == NULL) 
		p->p_view_pkgs = getlist();
	if ((n = findnode(p->p_view_pkgs, info->m_pkgid)) != NULL) {
		v = (View *)n->data;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
		v->v_fs_usage = info->m_fs_usage;
		for (info = next_inst(info); info; info = next_inst(info)) {
			v = v->v_instances;
			v->v_action = info->m_action;
			v->v_refcnt = info->m_refcnt;
			v->v_instdir = info->m_instdir;
			v->v_flags = info->m_flags;
			*v->v_status_ptr = info->m_status;
			v->v_fs_usage = info->m_fs_usage;
		}
	} else {
		v = (View *)xcalloc(sizeof (View));
		v->v_type = PACKAGE;
		v->v_status_ptr = &v->v_status;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
		v->v_fs_usage = info->m_fs_usage;
		v->v_info.v_mod = info;
		n = getnode();
		n->key = xstrdup(info->m_pkgid);
		n->data = v;
		n->delproc = &free_np_view;    /* set delete function */
		(void) addnode(p->p_view_pkgs, n);
		for (info = next_inst(info); info; info = next_inst(info)) {
			v->v_instances = (View *)xcalloc(sizeof (View));
			v = v->v_instances;	
			v->v_status_ptr = &v->v_status;
			*v->v_status_ptr = info->m_status;
			v->v_action = info->m_action;
			v->v_refcnt = info->m_refcnt;
			v->v_instdir = info->m_instdir;
			v->v_flags = info->m_flags;
			v->v_fs_usage = info->m_fs_usage;
		}
	} 
	return (SUCCESS);
}

/*
 * save_view_cluster()
 * Parameters:
 *	np	- node pointer
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
save_view_cluster(Node *np, caddr_t data)
{
	Modinfo *info;
	Product *p;
	View    *v;
	Node    *n;

	info = ((Module *)np->data)->info.mod;
	/*LINTED [alignment ok]*/
	p = (Product *)data;

	if (p->p_view_cluster == NULL) 
		p->p_view_cluster = getlist();
	if ((n = findnode(p->p_view_cluster, info->m_pkgid)) != NULL) {
		v = (View *)n->data;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
	} else {
		v = (View *)xcalloc(sizeof (View));
		v->v_type = ((Module *)np->data)->type;
		v->v_status_ptr = &v->v_status;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
		v->v_info.v_mod = info;
		n = getnode();
		n->key = xstrdup(info->m_pkgid);
		n->data = v;
		n->delproc = &free_np_view; 
		(void) addnode(p->p_view_cluster, n);
	}
	return (SUCCESS);
}

/*
 * save_view_4x()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
save_view_4x(Node * np, caddr_t data)
{
	Modinfo *info;
	Product *p;
	View    *v;
	Node    *n;

	info = (Modinfo *)np->data;
	/*LINTED [alignment ok]*/
	p = (Product *)data;

	if (p->p_view_4x == NULL) 
		p->p_view_4x = getlist();
	if ((n = findnode(p->p_view_4x, info->m_pkgid)) != NULL) {
		v = (View *)n->data;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
	} else {
		v = (View *)xcalloc(sizeof (View));
		v->v_type = UNBUNDLED_4X;
		v->v_status_ptr = &v->v_status;
		*v->v_status_ptr = info->m_status;
		v->v_shared = info->m_shared;
		v->v_action = info->m_action;
		v->v_refcnt = info->m_refcnt;
		v->v_instdir = info->m_instdir;
		v->v_flags = info->m_flags;
		v->v_info.v_mod = info;
		n = getnode();
		n->key = xstrdup(info->m_pkgid);
		n->data = v;
		n->delproc = &free_np_view;
		(void) addnode(p->p_view_4x, n);
	}
	return (SUCCESS);
}

/*
 * create_view()
 * Parameters:
 *	prod	-
 *	media	-
 * Return:
 * Status:
 *	private
 */
static Product *
create_view(Module * prod, Module * media)
{
	Product *p, *q;

	for (p = prod->info.prod; p->p_next_view != NULL; p = p->p_next_view) 
		;
	q = p->p_next_view = (Product *)xcalloc(sizeof (Product));
	q->p_name = p->p_name;
	q->p_version = p->p_version;
	q->p_id = p->p_id;
	q->p_pkgdir = p->p_pkgdir;
	q->p_arches = p->p_arches;
	q->p_sw_4x = p->p_sw_4x;
	q->p_packages = p->p_packages;
	q->p_clusters = p->p_clusters;
	q->p_locale = p->p_locale;
	q->p_rootdir = p->p_rootdir;
	q->p_categories = p->p_categories;
	q->p_cur_meta = p->p_cur_meta;
	q->p_cur_cluster = p->p_cur_cluster;
	q->p_cur_pkg = p->p_cur_pkg;
	q->p_deflt_meta = p->p_deflt_meta;
	q->p_deflt_cluster = p->p_deflt_cluster;
	q->p_deflt_pkg = p->p_deflt_pkg;
	q->p_view_from = media;
	return (q);
}
