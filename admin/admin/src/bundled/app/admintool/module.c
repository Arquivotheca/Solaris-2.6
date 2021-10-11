/* Copyright 1995 Sun Microsystems, Inc. */

/*

#pragma ident "@(#)module.c	1.1 95/05/05 Sun Microsystems"

*/

/* module.c */

/*
 * There routines support cancel button on custom dialog. Upon
 * entry to custom dialog, a copy of selected Module and its
 * subtree is made and stored away in FocusData struct. 
 * All manipulation in custom dialog is done to original
 * tree. Copy of tree is used to reset values if user exits
 * custom dialog using Cancel button.
 *
 * Only a sparse copy of subtree is made. That is, only those
 * fields which may be modified within custom dialog are copied.
 * Other fields simply use values within original module structure.
 * For example, a copy of m_basedir is made because it may be 
 * changed.  Likewise for the m_l10n list.
 */

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <nl_types.h>

#include "spmisoft_api.h"
#include "util.h"

nl_catd	_catd;	/* for catgets(), defined in main.c */

Modinfo * duplicateModinfo(Modinfo *);
Module * duplicateModule(Module *, Module *, Module *, Module *);

#define NO_MEMORY_MSG catgets(_catd, 8, 504,"module:can't allocate memory")
#define CORRUPT_MODULE_MSG catgets(_catd, 8, 505, "Illegal module type")


L10N *
newL10N() 
{
    L10N * new;

    new = (L10N *) malloc(sizeof(L10N));
    if (new == NULL)
	fatal(NO_MEMORY_MSG);
    memset(new, 0, sizeof(L10N));
    return(new);
}

L10N *
duplicateL10NList(L10N * l10n)
{

    L10N * l;
    L10N * new;
    L10N * head = NULL;

    if (l10n == NULL)
	return(NULL);

    head = new = newL10N();
    new->l10n_package = duplicateModinfo(l10n->l10n_package);

    l = l10n->l10n_next;
    while (l) {
	new->l10n_next = newL10N();
	new = new->l10n_next;
        new->l10n_package = duplicateModinfo(l->l10n_package);
 	l = l->l10n_next;
    }
    return(head);
}

Modinfo * 
duplicateModinfo(Modinfo * mi)
{
    Modinfo * new;
	
    if (mi == NULL)
	return(NULL);

    new = (Modinfo *) malloc(sizeof(Modinfo));
    if (new == NULL)
	fatal(NO_MEMORY_MSG);

    memset(new, 0, sizeof(Modinfo));

    /* These are the only two fields which can be
     * modified within custom dialog.
     */
    new->m_status = mi->m_status;	
    new->m_basedir = mi->m_basedir ? strdup(mi->m_basedir) : NULL;

    /* This can be changed indirectly, so I need to copy it. */
    new->m_action = mi->m_action;	

    /* We can use these pointer value because we will
     * not modify in custom screen.
     */
    new->m_pdepends = mi->m_pdepends;
    new->m_idepends = mi->m_idepends;
    new->m_rdepends = mi->m_rdepends;

    /* This stuff is displayed, so I simply use values
     * from original module.
     */
    new->m_pkgid = mi->m_pkgid;
    new->m_pkginst = mi->m_pkginst;
    new->m_pkg_dir = mi->m_pkg_dir;
    new->m_name = mi->m_name;
    new->m_vendor = mi->m_vendor;
    new->m_version = mi->m_version;
    new->m_prodname = mi->m_prodname;
    new->m_prodvers = mi->m_prodvers;
    new->m_arch = mi->m_arch;
    new->m_expand_arch = mi->m_expand_arch;
    new->m_desc = mi->m_desc;

    new->m_category = mi->m_category;
    new->m_locale = mi->m_locale;

    new->m_l10n = duplicateL10NList(mi->m_l10n);

/*  Need not copy these...not used, not modified.
    Since I have typed them, I am going to leave
    them around.

    new->m_instdate = mi->m_instdate ? strdup(mi->m_instdate) : NULL;
    new->m_patchid = mi->m_patchid ? strdup(mi->m_patchid) : NULL;
    new->m_l10n_pkglist = mi->m_l10n_pkglist ? strdup(mi->m_l10n_pkglist) : NULL;
    new->m_order = mi->m_order;	
    new->m_shared = mi->m_shared;	
    new->m_flags = mi->m_flags;	
    new->m_refcnt = mi->m_refcnt;	
    new->m_sunw_ptype = mi->m_sunw_ptype;	
    new->m_instances = NULL;
    new->m_next_path = NULL;
    new->m_patchof = NULL;
    new->m_text = NULL;
    new->m_demo = NULL;
    new->m_install = NULL;
    new->m_icon = NULL;
    new->m_filediff = NULL;
    new->m_pkg_hist = NULL;
    new->m_instdir = mi->m_instdir ? strdup(mi->m_instdir) : NULL;
*/

    new->m_spooled_size = mi->m_spooled_size;
    memcpy(new->m_deflt_fs, mi->m_deflt_fs, sizeof(daddr_t) * N_LOCAL_FS);

    return(new);
}

Product * 
duplicateProduct(Product * p)
{
    Product * new;

    if (p == NULL)
	return(NULL);

    new = (Product *) malloc(sizeof(Product));
    if (new == NULL)
	fatal(NO_MEMORY_MSG);

    memset(new, 0, sizeof(Product));

    new->p_name = p->p_name ? strdup(p->p_name) : NULL;
    new->p_version = p->p_version ? strdup(p->p_version) : NULL;
    new->p_rev = p->p_rev ? strdup(p->p_rev) : NULL;
    new->p_status = p->p_status;
    new->p_id = p->p_id ? strdup(p->p_id) : NULL;
    new->p_pkgdir = p->p_pkgdir ? strdup(p->p_pkgdir) : NULL;
    new->p_instdir = p->p_instdir ? strdup(p->p_instdir) : NULL;
    /*
    new->p_arches;  
    new->p_swcfg;
    new->p_platgrp;
    new->p_hwcfg; 
    new->p_sw_4x;
    new->p_packages;
    new->p_clusters;  
    new->p_locale;
    new->p_orphan_patch;
    new->p_rootdir;
    new->p_cur_meta;
    new->p_cur_cluster;
    new->p_cur_pkg;
    new->p_cur_cat;
    new->p_deflt_meta;
    new->p_deflt_cluster;
    new->p_deflt_pkg;
    new->p_deflt_cat;
    new->p_view_from;
    new->p_view_4x;
    new->p_view_pkgs;
    new->p_view_cluster;
    new->p_view_locale;
    new->p_view_arches;
    new->p_current_view;
    new->p_next_view;
    new->p_categories; 
    new->p_patches;
    */

    return(new);
}
/* 
 * Routines to manipulate Module tree in order
 * to squirrel away a piece for customization
 * operation.
 */

Module * 
duplicateList(Module * m, Module * parent)
{
    Module * head, *new_p, *next = NULL, * p; 
    if (m == NULL)
	return(NULL);
    head = new_p = duplicateModule(m, NULL, NULL, parent);
    new_p->head = new_p;	/* I am head of my own list */
    p = get_next(m);
    while (p) {
	new_p->next = duplicateModule(p, new_p, head, parent);
	new_p = new_p->next;
 	p = get_next(p);
    }
    return(head);
}

/*
 * Called on way in to customize dialog to
 * save away a piece of Module tree if need
 * 
 */

Module *
duplicateModule(Module * m, Module * prev, Module * head, Module * parent)
{
	Module * new;

	if (m == NULL)
	    return(NULL);

	new = malloc(sizeof(Module));
	if (new == NULL)
		fatal(NO_MEMORY_MSG);
	memset(new, 0, sizeof(Module));

	new->prev = prev;
	new->head = head;
	new->parent = parent;
	new->type = m->type;

	switch (m->type) {
	    case MODULE:
	    case PACKAGE:
	    case CLUSTER:
	    case METACLUSTER:
		new->info.mod = duplicateModinfo(m->info.mod);
		break;
	    case PRODUCT:
	    case NULLPRODUCT:
		new->info.prod = duplicateProduct(m->info.prod);
		break;
	    case LOCALE:
		new->info.locale = NULL;
		break;
	    case CATEGORY:
		break;
	    default:
		fatal(CORRUPT_MODULE_MSG);
	}	
	
	if (m->sub) {
		new->sub = duplicateList(m->sub, new);
	}	
	if (m->parent == NULL) {
		new->next = duplicateList(m->next, NULL);
	}
	return(new);
}

void
resetModinfo(Module* to, Module* from)
{
    L10N * from_l, * to_l;

	/* reset selection status */
    	to->info.mod->m_status = from->info.mod->m_status;
    	to->info.mod->m_action = from->info.mod->m_action;

    	free_mem(to->info.mod->m_basedir);

	/* reset basedir value */
	/* make a copy of basedir b/c destroyModule will free it */
    	to->info.mod->m_basedir = from->info.mod->m_basedir ? 
					strdup(from->info.mod->m_basedir) :
					NULL;

    	free_mem(to->info.mod->m_instdir);
 	to->info.mod->m_instdir = from->info.mod->m_instdir ?
					strdup(from->info.mod->m_instdir) :
					NULL;

	/* reset status of localized pkgs */
	to_l = to->info.mod->m_l10n;
	from_l = from->info.mod->m_l10n;
	while (to_l && from_l) {
		to_l->l10n_package->m_status = from_l->l10n_package->m_status;
		to_l = to_l->l10n_next;
		from_l = from_l->l10n_next;
	}
}

void
resetProduct(Module* to, Module* from)
{
    Product* to_p, *from_p;

    to_p = to->info.prod;
    from_p = from->info.prod;

    to_p->p_status = from_p->p_status;
}
/*
 * After Cancel from customize operation, 
 * reset values in a_with with those from Module b.
 *
 * Since the only fields manipulated in custom
 * dialog are m_status and m_basedir, these are
 * the only values that need be reset.
 */

void
resetModule(Module * a_with, Module * b)
{
    Module * from, * to;
    extern char * get_mod_name(Module *);

    if (!a_with || !b)
	return;

    /* 
     * A few sanity checks to get a good feeling that 
     * Module args are the "same"
     */

    if (a_with->type != b->type)
	return;
   
    if (strcmp(get_mod_name(a_with), get_mod_name(b)))
	return;

    from = b;
    to = a_with;

    do {
	if (to->type == NULLPRODUCT || to->type == PRODUCT)
	    resetProduct(to, from);
        else 
	    resetModinfo(to, from);


	if (from->sub)
		resetModule(to->sub, from->sub);

    } while ((to = get_next(to)) &&  (from = get_next(from)));
}

/*
 * Invoked after Cancel/OK of customize operation.
 * Frees all memory associated with a Module
 * tree rooted at Module * m
 * 
 * Don't free char * ptrs that were borrowed from original
 * Module tree.
 */

void
destroyPackage(Module* m)
{
    L10N * l, * l_next;
    /* 
     * Only free these fields. All others point 
     * into "real" tree.
     */

    /* free localized pkgs */
    l = m->info.mod->m_l10n;
    while (l) {
	l_next = l->l10n_next;
	/* free the pkg Modinfo struct */
	free_mem(l->l10n_package);

	/* free the L10N struct */
	free_mem(l);

	l = l_next;
    }
    free_mem(m->info.mod->m_basedir);
    free_mem(m);
}

void
destroyProduct(Module* m)
{
    Product* p = m->info.prod;

    free_mem(p->p_name);
    free_mem( p->p_version );
    free_mem( p->p_rev );
    free_mem( p->p_id );
    free_mem( p->p_pkgdir );
    free_mem( p->p_instdir );
    free_mem(m);
}

void
destroyModule(Module * m)
{

    if (m == NULL)
	return;

    /* A good old-fashion post-order traversal... */

    if (m->sub)
	destroyModule(m->sub);
    if (m->next)
	destroyModule(m->next);

    if (m->type == PRODUCT || m->type == NULLPRODUCT)
	destroyProduct(m);
    else
	destroyPackage(m);
}
