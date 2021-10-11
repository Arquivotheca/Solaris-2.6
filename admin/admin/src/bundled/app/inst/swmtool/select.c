/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#ifndef lint
#ident	"@(#)select.c 1.15 93/12/07"
#endif

#include "defs.h"

static int count_pkg(Node *, caddr_t);
static int reset_cluster_action(Node *, caddr_t);
static int reset_package_action(Node *, caddr_t);
static int reset_4x_action(Node *, caddr_t);
static int mod_rm_status(Module *);

void
mark_selection(Module *mod, Select_mode func)
{
	SWM_view view = get_view();
	SWM_mode mode = get_mode();
	ModStatus	status;
	Action		action;
	Module		*subp, *parentp;

	/*
	 * Propagate selection status up the
	 * hierarchy chain -- do this so that
	 * if all components of a cluster are
	 * individually selected, the cluster
	 * appears selected.
	 */
	parentp = mod;
	while (parentp->parent != (Module *)0 &&
	    parentp->type != PRODUCT && parentp->type != NULLPRODUCT)
		parentp = parentp->parent;

	if (mode == MODE_INSTALL) {
		switch (func) {
		case MOD_SELECT:
			status = SELECTED;
			break;
		case MOD_DESELECT:
			status = UNSELECTED;
			break;
		case MOD_TOGGLE:
			if (mod->type == PRODUCT) {
				if (mod->info.prod->p_status == SELECTED)
					status = UNSELECTED;
				else
					status = SELECTED;
			} else if (mod->info.mod->m_status == SELECTED)
				status = UNSELECTED;
			else
				status = SELECTED;
			break;
		}
		if (mod->type == PRODUCT) {
			toggle_product(mod, status);
			if (view == VIEW_NATIVE) {
				for (subp = mod->sub; subp; subp = subp->next) {
					mark_module_action(subp,
					    status == SELECTED ?
						TO_BE_PKGADDED :
						NO_ACTION_DEFINED);
				}
			}
		} else {
			mark_module(mod, status);
			if (view == VIEW_NATIVE) {
				mark_module_action(mod,
				    status == SELECTED ?
					TO_BE_PKGADDED : NO_ACTION_DEFINED);
			}
			/*
			 * Propagate status up the tree
			 */
			if (parentp != mod) {
				status = mod_status(parentp);

				if (parentp->type == PRODUCT ||
				    parentp->type == NULLPRODUCT)
					parentp->info.prod->p_status = status;
				else
					parentp->info.mod->m_status = status;
			}
		}
	} else if (mode == MODE_REMOVE) {
		switch (func) {
		case MOD_SELECT:
			action = TO_BE_REMOVED;
			break;
		case MOD_DESELECT:
			action = NO_ACTION_DEFINED;
			break;
		case MOD_TOGGLE:
			if (mod->type != PRODUCT && mod->type != NULLPRODUCT) {
				if (mod->info.mod->m_action ==
				    NO_ACTION_DEFINED)
					action = TO_BE_REMOVED;
				else
					action = NO_ACTION_DEFINED;
			}
			break;
		}
		if (mod->type != PRODUCT && mod->type != NULLPRODUCT)
			mark_module_action(mod, action);
		else
			for (subp = mod->sub; subp; subp = subp->next)
				mark_selection(subp, MOD_TOGGLE);
		/*
		 * Propagate status up the tree.
		 */
		if (parentp != mod) {
			status = mod_rm_status(parentp);

			if (parentp->type != PRODUCT &&
			    parentp->type != NULLPRODUCT)
				parentp->info.mod->m_action =
				    status == SELECTED ?
					TO_BE_REMOVED : NO_ACTION_DEFINED;
		}
	}
}

void
reset_selections(int do_selection, int do_arch, int do_locale)
{
	Module	*media, *prod;
	Arch	*arch;
	Module	*loc;
	SWM_mode mode = get_mode();
	SWM_view view = get_view();

	/*
	 * Reset default views and installed media first,
	 * then muck with the views corresponding to services.
	 */
	for (media = get_media_head(); media; media = media->next) {
		for (prod = media->sub; prod; prod = get_next(prod)) {
			load_default_view(prod);
			if (do_selection) {
			    if (media->info.media->med_type == INSTALLED ||
				media->info.media->med_type == INSTALLED_SVC) {
					walklist(prod->info.prod->p_clusters,
					    reset_cluster_action, (caddr_t)0);
					walklist(prod->info.prod->p_packages,
					    reset_package_action, (caddr_t)0);
			    } else {
					clear_view(prod);
					walklist(prod->info.prod->p_sw_4x,
					    reset_4x_action, (caddr_t)0);
			    }
			}
			if (media->info.media->med_type != INSTALLED &&
			    media->info.media->med_type != INSTALLED_SVC) {
				if (do_arch)
					for (arch = get_all_arches(prod);
					    arch; arch = arch->a_next) {
						if (supports_arch(
						    get_default_arch(),
						    arch->a_arch))
							arch->a_selected = 1;
						else
							arch->a_selected = 0;
					}
				if (do_locale)
					for (loc = get_all_locales();
					    loc; loc = loc->next)
						loc->info.locale->l_selected= 0;
			}
		}
	}
	if (view == VIEW_NATIVE || mode == MODE_REMOVE) {
		remove_all_services();
		return;
	}
}

static int
count_pkg(Node *np, caddr_t data)
{
	Modinfo	*info = (Modinfo *)np->data;
	/*LINTED [alignment ok]*/
	int	*p_cnt = (int *)data;

	if (info->m_status == SELECTED ||
	    info->m_status == REQUIRED ||
	    info->m_action == TO_BE_REMOVED)
		(*p_cnt)++;
	return (0);
}

int
count_selections(Module *media)
{
	Module	*mod;
	Node	node;
	int	count;

	if (media == (Module *)0)
		return (0);

	for (mod = media->sub, count = 0;
	    mod != (Module *)0 && count == 0; mod = get_next(mod)) {
		if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
			if (mod->info.prod->p_sw_4x)
				(void) walklist(mod->info.prod->p_sw_4x,
					count_pkg, (caddr_t)&count);
			else if (mod->info.prod->p_packages)
				(void) walklist(mod->info.prod->p_packages,
					count_pkg, (caddr_t)&count);
		} else {
			node.data = (void *)mod->info.mod;
			(void) count_pkg(&node, (caddr_t)&count);
		}
	}
	return (count);
}

/*ARGSUSED1*/
static int
reset_cluster_action(Node *node, caddr_t arg)
{
	Modinfo	*mi;

	mi = ((Module *)(node->data))->info.mod;
	mi->m_action = NO_ACTION_DEFINED;
	mi->m_refcnt = 0;
	return (SUCCESS);
}

/*ARGSUSED1*/
static int
reset_package_action(Node *node, caddr_t arg)
{
	Modinfo	*inst, *patch;

	for (inst = (Modinfo *)(node->data);
	    inst != (Modinfo *)0;
	    inst = next_inst(inst)) {
		for (patch = inst;
		    patch != (Modinfo *)0;
		    patch = next_patch(patch)) {
			patch->m_action = NO_ACTION_DEFINED;
			patch->m_refcnt = 0;
		}
	}
	return (SUCCESS);
}

/*ARGSUSED1*/
static int
reset_4x_action(Node *node, caddr_t arg)
{
	Modinfo	*mi;

	for (mi = (Modinfo *)(node->data);
	    mi != (Modinfo *)0; mi = next_inst(mi)) {
		mi->m_status = UNSELECTED;
		mi->m_action = NO_ACTION_DEFINED;
		mi->m_refcnt = 0;
	}
	return (SUCCESS);
}

/*
 * mod_rm_status() - Like mod_status, except for removals
 */
static int
mod_rm_status(Module *mod)
{
	register int    n, m;
	register Module *mp;

	if (mod->type == MEDIA)
		return (ERR_INVALIDTYPE);

	if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
		if (mod->sub == (Module *)0)
			return (UNSELECTED);
	} else if (mod->sub == (Module *)0)
		return (mod->info.mod->m_action == TO_BE_REMOVED ?
							SELECTED : UNSELECTED);

	for (n = 0, m = 0, mp = mod->sub; mp; mp = mp->next) {
		if (mp->sub != (Module *)0) {
			if (mod_rm_status(mp) == SELECTED)
				mp->info.mod->m_action = TO_BE_REMOVED;
			else
				mp->info.mod->m_action = NO_ACTION_DEFINED;
		}
		n++;
		if (mp->info.mod->m_action == TO_BE_REMOVED)
			m++;
	}

	if (n == 0) {
		return (mod->info.mod->m_action == TO_BE_REMOVED ?
							SELECTED : UNSELECTED);
	} else if (m == 0)
		return (UNSELECTED);
	else if (m == n)
		return (SELECTED);
	else if (m < n)
		return (PARTIAL);
	else
		return (ERR_INVALID);

}
