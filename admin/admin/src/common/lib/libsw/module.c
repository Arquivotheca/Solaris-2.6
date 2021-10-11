#ifndef lint
#ident   "@(#)module.c 1.33 95/05/31 SMI"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "sw_lib.h"

/* Externals */

extern Module *	head_ptr;

/* Local Globals */

Module *	current_media;
Module *	current_service;
Module *	default_media;
Module *	default_service;

/* Local Statics and Constants */

static MachineType	machinetype = MT_STANDALONE;
static char	 	current_locale[BUFSIZ] = "";
static char *		default_locale = "C";
static char		rootdir[BUFSIZ] = "";

/* Public Function Prototypes */

int      swi_set_current(Module *);
int      swi_set_default(Module *);
Module * swi_get_current_media(void);
Module * swi_get_current_service(void);
Module * swi_get_current_product(void);
Module * swi_get_current_category(ModType);
Module * swi_get_current_metacluster(void);
Module * swi_get_local_metacluster(void);
Module * swi_get_current_cluster(void);
Module * swi_get_current_package(void);
Module * swi_get_default_media(void);
Module * swi_get_default_service(void);
Module * swi_get_default_product(void);
Module * swi_get_default_category(ModType);
Module * swi_get_default_metacluster(void);
Module * swi_get_default_cluster(void);
Module * swi_get_default_package(void);
Module * swi_get_next(Module *);
Module * swi_get_sub(Module *);
Module * swi_get_prev(Module *);
Module * swi_get_head(Module *);
int      swi_mark_required(Module *);
int      swi_mark_module(Module *, ModStatus);
int      swi_mod_status(Module *);
int      swi_toggle_module(Module *);
MachineType swi_get_machinetype(void);
void     swi_set_machinetype(MachineType);
char *   swi_get_current_locale(void);
void     swi_set_rootdir(char *);
void     swi_set_current_locale(char *);
char *   swi_get_default_locale(void);
char *   swi_get_rootdir(void);
int      swi_toggle_product(Module *, ModStatus);
int      swi_mark_module_action(Module *, Action);
int      swi_partial_status(Module *);


/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * set current()
 *	Each module type has a "current" module pointer. This routine looks
 *	at the module type of 'mod' and sets the appropriate "current" pointer
 *	to reference 'mod'. Currently accepted module types are: MEDIA, PRODUCT,
 *	CATEGORY, METACLUSTER, CLUSTER, PACKAGE. Current CLUSTERS, METACLUSTERS,
 *	and PACKAGES are defined on a per-product basis. PRODUCTS, and
 *	CATEGORIES are defined on a per-media basis.
 * Parameters:
 *	mod	    - pointer to module
 * Return:
 *	SUCCESS	    - if successful ERR_NOMEDIA - if mod type is not a MEDIA 
 *		      and there is no current media
 *	ERR_INVALID - if the specified module isn't part of the current media
 *	ERR_NOPROD  - if mod type is CLUSTER or METACLUSTER and there is no
 *		      current product
 * Status:
 *	public
 */
int
swi_set_current(Module * mod)
{
	Module 	*tmp;
	Module 	*t3;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_current");
#endif

	switch (mod->type) {
	case MEDIA:
		current_media = mod;
		break;
	case PRODUCT:
		if ((tmp = get_current_media()) == NULL)
			if ((tmp = get_default_media()) == NULL)
				return (ERR_NOMEDIA);
		/* verify module is a part of the selected media */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		tmp->info.media->med_cur_prod = mod;
		break;
	case CATEGORY:
		if ((tmp = get_current_media()) == NULL)
			if ((tmp = get_default_media()) == NULL)
				return (ERR_NOMEDIA);
		/* verify category is a part of the selected media */
		if (mod->parent != tmp) {
			if ((tmp = get_current_product()) == NULL)
				if ((tmp = get_default_product()) == NULL)
					return (ERR_NOMEDIA);
			if (mod->parent != tmp)
				return (ERR_INVALID);
			tmp->info.prod->p_cur_cat = mod;
		} else {
			tmp->info.media->med_cur_cat = mod;
		}
			break;
	case METACLUSTER:
		if ((tmp = get_current_product()) == NULL)
			if ((tmp = get_default_product()) == NULL)
				return (ERR_NOPROD);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		tmp->info.prod->p_current_view->p_cur_meta = mod;
		break;
	case CLUSTER:
		if (((tmp = get_current_metacluster()) == NULL) &&
				((tmp = get_default_metacluster()) == NULL))
			if (((tmp = get_current_product()) == NULL) &&
				((tmp = get_default_product()) == NULL))
				return (ERR_NOMEDIA);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		if ((tmp = get_current_product()) == NULL)
			if ((tmp = get_default_product()) == NULL)
			return (ERR_NOPRODUCT);
		tmp->info.prod->p_current_view->p_cur_cluster = mod;
		break;
	case PACKAGE:
		if (((tmp = get_current_cluster()) == NULL) &&
			((tmp = get_default_cluster()) == NULL))
			if (((tmp = get_current_metacluster()) == NULL) &&
				((tmp = get_default_metacluster()) == NULL))
				if (((tmp = get_current_product()) == NULL) &&
					((tmp = get_default_product()) == NULL))
					return (ERR_NOMEDIA);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		if (((tmp = get_current_product()) == NULL) &&
				((tmp = get_default_product()) == NULL))
			return (ERR_NOMEDIA);
		tmp->info.prod->p_current_view->p_cur_pkg = mod;
		break;
	}
	return (SUCCESS);
}

/*
 * set_default()
 *	Each module type has a "default" module pointer. This routine looks
 *	at the module type of 'mod' and sets the appropriate "default" pointer
 *	to reference 'mod'. Currently accepted module types are: MEDIA, PRODUCT,
 *	CATEGORY, METACLUSTER, CLUSTER, PACKAGE. Current CLUSTERS, METACLUSTERS,
 *	and PACKAGES are defined on a per-product basis. PRODUCTS, and 
 *	CATEGORIES are defined on a per-media basis.
 * Parameters:
 *	mod	    - pointer to module
 * Return:
 *	SUCCESS	    - if successful ERR_NOMEDIA - if mod type is not a MEDIA 
 *		      and there is no current media
 *	ERR_INVALID - if the specified module isn't part of the current media
 *	ERR_NOPROD  - if mod type is CLUSTER or METACLUSTER and there is no
 *		      current product
 * Status:
 *	public
 */
int
swi_set_default(Module * mod)
{
	Module 	*tmp;
	Module 	*t3;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_default");
#endif

	switch (mod->type) {
	case MEDIA:
		default_media = mod;
		break;
	case PRODUCT:
		if ((tmp = get_current_media()) == NULL)
			if ((tmp = get_default_media()) == NULL)
				return (ERR_NOMEDIA);
		/* verify module is a part of the selected media */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		tmp->info.media->med_deflt_prod = mod;
		break;
	case CATEGORY:
		if ((tmp = get_current_media()) == NULL)
			if ((tmp = get_default_media()) == NULL)
				return (ERR_NOMEDIA);
		/* verify category is a part of the selected media */
		if (mod->parent != tmp) {
			if ((tmp = get_current_product()) == NULL)
				if ((tmp = get_default_product()) == NULL)
					return (ERR_NOMEDIA);
			if (mod->parent != tmp)
				return (ERR_INVALID);
			tmp->info.prod->p_cur_cat = mod;
		} else {
			tmp->info.media->med_cur_cat = mod;
		}
		break;
	case METACLUSTER:
		if ((tmp = get_current_product()) == NULL)
			if ((tmp = get_default_product()) == NULL)
				return (ERR_NOPROD);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		tmp->info.prod->p_current_view->p_deflt_meta = mod;
		break;
	case CLUSTER:
		if ((tmp = get_current_metacluster()) == NULL)
			if ((tmp = get_default_metacluster()) == NULL)
				if ((tmp = get_current_product()) == NULL)
					if ((tmp=get_default_product()) == NULL)
						return (ERR_NOMEDIA);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		if ((tmp = get_current_product()) == NULL)
			if ((tmp = get_default_product()) == NULL)
				return (ERR_NOMEDIA);
		tmp->info.prod->p_current_view->p_deflt_cluster = mod;
		break;
	case PACKAGE:
		if (((tmp = get_current_cluster()) == NULL) &&
				((tmp = get_default_cluster()) == NULL))
			if (((tmp = get_current_metacluster()) == NULL) &&
				((tmp = get_default_metacluster()) == NULL))
				if (((tmp = get_current_product()) == NULL) &&
					((tmp = get_current_product()) == NULL))
					return (ERR_NOMEDIA);
		/* verify module is a part of the selected product */
		for (t3 = mod->parent; t3 != NULL; t3 = t3->parent) {
			if (t3 == tmp)
				break;
		}
		if (t3 == NULL)
			return (ERR_INVALID);
		if ((tmp = get_current_product()) == NULL)
			return (ERR_NOMEDIA);
		tmp->info.prod->p_current_view->p_deflt_pkg = mod;
		break;
	}
	return (SUCCESS);
}

/*
 * get_current_media()
 *	Return a pointer to the current media. If none is defined,
 *	return the default media.
 * Parameters:
 *	none
 * Return:
 *	Module *	- pointer to media module structure
 * Status:
 *	public
 */
Module *
swi_get_current_media(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_media");
#endif
	if (current_media != NULL)
		return (current_media);
	return (get_default_media());
}

/*
 * get_current_service() - returns a pointer to the current media
 */

Module *
swi_get_current_service()
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_service");
#endif
	if (current_media != NULL)
		return (current_service);
	return (get_default_service());
}

/*
 * get_current_product() -  returns pointer to the current product
 */
Module *
swi_get_current_product(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_product");
#endif

	if ((mod = get_current_media()) == NULL)
		if ((mod = get_default_media()) == NULL)
			return ((Module *)NULL);
	if (mod->info.media->med_cur_prod == NULL)
		return (get_default_product());
	return (mod->info.media->med_cur_prod);
}

/*
 * get_current_category() -  returns pointer to the current product
 */
Module *
swi_get_current_category(ModType type)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_category");
#endif
	
	if (type == MEDIA) {
		if ((mod = get_current_media()) == NULL)
			if ((mod = get_default_media()) == NULL)
				return ((Module *)NULL);
		if (mod->info.media->med_cur_cat == NULL)
			return (get_default_category(MEDIA));
		return (mod->info.media->med_cur_cat);
	} else {
		if ((mod = get_current_product()) == NULL)
			if ((mod = get_default_product()) == NULL)
				return ((Module *)NULL);
		if (mod->info.prod->p_cur_cat == NULL)
			return (get_default_category(PRODUCT));
		return (mod->info.prod->p_cur_cat);
	}
}

/*
 * get_current_metacluster() -  returns pointer to the current product
 */
Module *
swi_get_current_metacluster(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_metacluster");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	if (mod->info.prod->p_current_view->p_cur_meta == NULL)
		return (get_default_metacluster());
	return (mod->info.prod->p_current_view->p_cur_meta);
}


/*
 * get_local_metacluster() -  returns pointer to the current product
 */
Module *
swi_get_local_metacluster(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_local_metacluster");
#endif

	if ((mod = get_current_product()) == NULL)
		return ((Module *)NULL);

	for (mod = mod->sub; mod; mod = mod->next)
		if (mod->type == METACLUSTER &&
		    (mod->info.mod->m_status == SELECTED ||
		    mod->info.mod->m_status == REQUIRED))
			return (mod);
	return ((Module *)NULL);
}

/*
 * get_current_cluster() -  returns pointer to the current product
 */
Module *
swi_get_current_cluster(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_cluster");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	if (mod->info.prod->p_current_view->p_cur_cluster == NULL)
		return (get_default_cluster());
	return (mod->info.prod->p_current_view->p_cur_cluster);
}


/*
 * get_current_package() -  returns pointer to the current product
 */
Module *
swi_get_current_package(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_package");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	if (mod->info.prod->p_current_view->p_cur_pkg == NULL)
		return (get_default_package());
	return (mod->info.prod->p_current_view->p_cur_pkg);
}

/*
 * get_default_media() - returns a pointer to the current media
 */

Module *
swi_get_default_media()
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_media");
#endif
	if (default_media == NULL)
		default_media = head_ptr;
	return (default_media);
}

/*
 * get_default_service() - returns a pointer to the current media
 */

Module *
swi_get_default_service()
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_service");
#endif
	if (default_media == NULL)
		default_service = head_ptr;
	return (default_service);
}

/*
 * get_default_product() -  returns pointer to the current product
 */
Module *
swi_get_default_product(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_product");
#endif

	if ((mod = get_current_media()) == NULL)
		if ((mod = get_default_media()) == NULL)
			return ((Module *)NULL);
	if (mod->info.media->med_deflt_prod == NULL)
		return (mod->sub);
	return (mod->info.media->med_deflt_prod);
}

/*
 * get_default_category()
 *	returns pointer to the current product
 * Parameters:
 *	type	-
 * Return:
 * Status:
 *	public
 */
Module *
swi_get_default_category(ModType type)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_category");
#endif
	
	if (type == MEDIA) {
		if ((mod = get_current_media()) == NULL)
			if ((mod = get_default_media()) == NULL)
				return ((Module *)NULL);
		if (mod->info.media->med_deflt_cat == NULL)
			return (mod->info.media->med_cat);
		return (mod->info.media->med_deflt_cat);
	} else {
		if ((mod = get_current_product()) == NULL)
			if ((mod = get_default_product()) == NULL)
				return ((Module *)NULL);
		if (mod->info.prod->p_deflt_cat == NULL)
			return (mod->info.prod->p_categories);
		return (mod->info.prod->p_deflt_cat);
	}
}

/*
 * get_default_metacluster()
 *	returns pointer to the current product
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
Module *
swi_get_default_metacluster(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_metacluster");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	if (mod->info.prod->p_current_view->p_deflt_meta == NULL)
		return (mod->sub);
	return (mod->info.prod->p_current_view->p_deflt_meta);
}

/*
 * get_default_cluster()
 *	Returns pointer to the current product
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 * 	public
 */
Module *
swi_get_default_cluster(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_cluster");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	return (mod->info.prod->p_current_view->p_deflt_cluster);
}


/*
 * get_default_package()
 *	Returns pointer to the current product
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 *	public
 */
Module *
swi_get_default_package(void)
{
	Module	*mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_package");
#endif

	if ((mod = get_current_product()) == NULL)
		if ((mod = get_default_product()) == NULL)
			return ((Module *)NULL);
	return (mod->info.prod->p_current_view->p_deflt_pkg);
}

/*
 * get_next()
 *	Returns a pointer to the module in the chain which follows 'mod'.
 *	Returns NULL if no module follows "mod".  Provided to allow the
 *	caller to dereference the next pointer of the module without
 *	knowledge of the underlying data structures.
 * Parameters:
 *	mod	- pointer to current module
 * Return:
 *	NULL	  - no next module
 *	Module *  - pointer to next module
 * Status:
 *	public
 */
Module *
swi_get_next(Module * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_next");
#endif
	return (mod->next);
}

/*
 * get_sub()
 *	Returns pointer to the sub-module of 'mod'.  Returns NULL if
 *	'mod' has not sub-modules. Provided to allow the caller to
 *	dereference the sub pointer of the module without knowledge of
 *	the underlying data structures.
 * Parameters:
 *	mod	 - pointer to current module
 * Return:
 *	NULL	 - no next sub module
 *	Module * - pointer to next submodule
 * Status:
 *	public
 */
Module *
swi_get_sub(Module * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_sub");
#endif
	if (mod->sub == NULL)
		return (NULL);
	if (mod->sub->type == NULLPRODUCT)
		return (mod->sub->sub);
	return (mod->sub);
}

/*
 * get_prev()
 *	Returns a pointer to the module in the chain which precedes 'mod'.
 *	Returns NULL if no module precedes 'mod'. Provided to allow the
 *	caller to dereference the previous pointer of the module without
 *	knowledge of the underlying data structures.
 * Parameters:
 *	mod	 - pointer to current module
 * Return:
 * 	NULL	 - no preceding module
 *	Module * - pointer to preceeding module
 * Status:
 *	public
 */
Module *
swi_get_prev(Module * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_prev");
#endif
	if (mod->prev == NULL)
		return (NULL);
	if (mod->prev->type == NULLPRODUCT)
		return (mod->prev->prev);
	return (mod->prev);
}

/*
 * get_head()
 *	Returns a pointer to the head of the chain of module "mod".
 *	Provided to allow the caller to dereference the head pointer of
 *	the module without knowledge of the underlying data structures.
 * Parameters:		
 *	mod	 - pointer to current module
 * Return:
 *	Module * - pointer to the head module
 * Status:
 *	public
 */
Module *
swi_get_head(Module * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_head");
#endif
	return (mod->head);
}


/*
 * mark_required()
 *	Call 'mark_module()' for 'modp', setting the status of the module
 *	and its sub-tree to REQUIRED.
 * Parameters:
 *	modp	- pointer to module which heads the tree segment
 *		  being marked (cannot be MEDIA or PRODUCT type)
 * Return:
 *	all values returned by mark_module
 * Status:
 *	public
 */
int
swi_mark_required(Module * modp)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mark_required");
#endif
	return (mark_module(modp, REQUIRED));
}

/*
 * mark_module()
 *	Used to mark modules (other than PRODUCT and MEDIA) with the
 *	specified 'status'. Marking will affect 'modp' as well as all
 *	the modules in the sub-tree. Modules marked as REQUIRED are not
 *	affected. If 'status' is SELECTED, the reference count is
 *	incremented. If 'status' is UNSELECTED, the reference count is
 *	set to '0'.
 * Parameters:
 *	modp	- pointer to module which heads the tree segment
 *		  being marked (cannot be MEDIA or PRODUCT type)
 *	status	- any valid ModStatus type
 * Return:	
 *	ERR_INVALIDTYPE	- module type passed in was MEDIA or PRODUCT
 *	SUCCESS		- all other conditions
 * Status:
 *	public
 */
int
swi_mark_module(Module * modp, ModStatus status)
{
	Module	*mp;
	Node	*np;
	Module  *prod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mark_module");
#endif

	if (modp->type == PRODUCT || modp->type == MEDIA)
		return (ERR_INVALIDTYPE);

	if (modp->info.mod->m_status != REQUIRED) {
		modp->info.mod->m_status = status;
		if (status == SELECTED) {
			modp->info.mod->m_refcnt += 1;
		} else if (status == UNSELECTED)
			modp->info.mod->m_refcnt = 0;

		for (prod = modp->parent; prod != NULL; prod = prod->parent) {
			if (prod->type == PRODUCT || prod->type == NULLPRODUCT)
				break;
		}
		if (prod && prod->info.prod->p_packages != NULL) {
			np = findnode(prod->info.prod->p_packages,
						modp->info.mod->m_pkgid);
			if (np != NULL)
				update_selected_arch(np, (caddr_t)prod);
			if ((modp->type == PACKAGE) && modp->info.mod->m_l10n)
				mark_locales(modp, status);
		}
			
	}
	for (mp = modp->sub; mp; mp = mp->next)
		mark_module(mp, status);

	return (SUCCESS);
}

/*
 * mod_status()
 *	Returns the status field of module 'mod'. MEDIA modules are
 *	not accepted. Return value based on the following (hopefully
 *	correct, but no guarantees) representation of the algorithm:
 *
 *	(1)	'mod' type is PRODUCT with a status of REQUIRED, return
 *		REQUIRED
 *	(2)	'mod' type is PRODUCT and status is not REQUIRED and
 *		there is a submodule, return the status of the submodule
 *	(3)	'mod' type is not PRODUCT and is not LOCALE, if status
 *		is REQUIRED, then return REQUIRED
 *	(4)	'mod' type is not PRODUCT and is not REQUIRED and has
 *		no sub-modules, return the 'mod' status field.
 *	otherwise:	
 *
 *	(5) 	if there is a sub-module, recursively call this routine
 *		for that module;
 *	(6)	if the module had sub-modules which all met criteria
 *		(1)-(4), return the status of 'mod'
 *	(7)	otherwise if none of the siblings of 'mod' were set
 *		to SELECTED, REQUIRED, or LOADED, then return UNSELECTED
 *	(8)	otherwise, if they were all LOADED, all SELECTED, or
 *		all REQUIRED, then return LOADED, SELECTED, or REQUIRED
 *		respectively
 *	(9)	otherwise, if some were LOADED, *and* some were SELECTED
 *		or some were REQUIRED, then return PARTIAL.
 * Parameters:
 *	 mod	 - pointer to module for which the status is to be checked  
 *		   (cannot be of type MEDIA)
 * Return:
 *	ERR_INVALIDTYPE	 - module type was MEDIA
 *	ERR_INVALID	 - the tree is in an invalid state
 *	REQUIRED
 *	PARTIAL
 *	SELECTED
 *	UNSELECTED
 *	LOADED		 - see above
 * Status:
 *	public
 */
int
swi_mod_status(Module * mod)
{
	register int	n, m, l, r, p;
	register Module *mp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mod_status");
#endif

	if (mod->type == MEDIA)
		return (ERR_INVALIDTYPE);

	if (mod->type == PRODUCT) {
		if (mod->info.prod->p_status == REQUIRED)
			return (REQUIRED);

		if (mod->sub == NULL)
			return (mod->info.prod->p_status);
	} else {
		if (mod->type != LOCALE)
			if (mod->info.mod->m_status == REQUIRED)
				return (REQUIRED);

		if (mod->sub == NULL)
			return (mod->info.mod->m_status);
	}

	n = 0;
	m = 0;
	l = 0;
	p = 0;
	r = 0;
	for (mp = mod->sub; mp; mp = mp->next) {
		if (mp->sub != NULL)
			mp->info.mod->m_status = mod_status(mp);
		n++;
		if (mp->info.mod->m_status == REQUIRED)
			r++;
		if (mp->info.mod->m_status == SELECTED ||
					mp->info.mod->m_status == REQUIRED)
			m++;
		if (mp->info.mod->m_status == PARTIAL)
			p++;
		if (mp->info.mod->m_status == LOADED)
			l++;
	}

	if (n == 0) {
		if (mod->type == PRODUCT)
			return (mod->info.prod->p_status);
		else
			return (mod->info.mod->m_status);
	} else if (m == 0 && l == 0 && p == 0)
		return (UNSELECTED);
	else if (l == n)
		return (LOADED);
	else if (r == n)
		return (REQUIRED);
	else if (m == n)
		return (SELECTED);
	else if (m < n && l < n)
		return (PARTIAL);
	else
		return (ERR_INVALID);
}

/*
 * partial_status()
 *
 * Parameters:
 *	mod	-
 * Return:
 *	SELECTED    -
 *	UNSELECTED  -
 * Status:
 *	public
 */
int
swi_partial_status(Module * mod)
{
	Module *mp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("partial_status");
#endif

	for (mp = mod->sub; mp; mp = mp->next) {
		if (mp->sub != NULL)
			if (partial_status(mp) == SELECTED)
				return (SELECTED);

		if (mp->info.mod->m_status == SELECTED)
			return (SELECTED);
	}
	return (UNSELECTED);
}

/*
 * toggle_module()
 *	Toggles the status if the module 'mod' and the sub-tree under 'mod'.
 *	If 'mod' is SELECTED, it becomes UNSELECTED (if refcnt is 1, otherwise
 *	refcnt is decremented). If 'mod' is PARTIAL, and something under 'mod'
 *	is SELECTED (rather than REQUIRED) then the sub-tree is UNSELECTED.
 *	Otherwise, everything is SELECTED. Used only by ttinstall, mainly for 
 *	historical reasons.
 * Parameters:
 *	mod	- pointer to module at head of sub-tree
 * Return:
 *	#	- the modinfo status of 'mod' once the traversal is completed
 *		  (see 'ModStatus')
 */
int
swi_toggle_module(Module * mod)
{
	register Module *mod1;
	Module	*prod;
	Node	*np;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("toggle_module");
#endif

#define mod_info ((Modinfo *) mod->info.mod)

	/*
	 * mark module as SELECTED or UNSELECTED update reference counter if
	 * already selected...
	 *
	 */
	if (mod_info->m_status != REQUIRED && mod_info->m_status != LOADED) {
		if (mod_status(mod) == PARTIAL)
			mod_info->m_status = partial_status(mod);

		if (mod_info->m_status == SELECTED) {
			/*
			 * if explicitly deselecting a package, set refcnt to 0.
			 */
			if (mod->type == PACKAGE)
				mod_info->m_refcnt = 0;
			else
				mod_info->m_refcnt = (mod_info->m_refcnt ?
						mod_info->m_refcnt - 1 : 0);

		} else {
	  		mod_info->m_status = SELECTED;
				mod_info->m_refcnt += 1;
		}

		if (mod_info->m_refcnt == 0)
			mod_info->m_status = UNSELECTED;
	}

	/*
	 * if this is a package, and it has localizations mark any localization
	 * packages whose locales have been selected
	 */
	if ((mod->type == PACKAGE) && mod_info->m_l10n) {
		mark_locales(mod, mod_info->m_status);
		for (prod = mod->parent; prod != NULL; prod = prod->parent) {
			if (prod->type == PRODUCT || prod->type == NULLPRODUCT)
				break;
		}
		if (prod && prod->info.prod->p_packages != NULL) {
			np = findnode(prod->info.prod->p_packages,
						mod->info.mod->m_pkgid);
			if (np != NULL)
				update_selected_arch(np, (caddr_t)prod);
		}

	/*
	 * mark components of CLUSTERS and METACLUSTERs with the same status as
	 * their parent,
	 *
	 * Yuck! if deselecting a METACLUSTER, need to deselect all possible
	 * components (bugid 1082126), some other stuff may have been selected
	 * that we need to deselect.  Pass `all' subtree into
	 * unselect_submodules() to explicitly mark them. maybe should do this
	 * explicit marking at a higher level in the software?
	 *
	 */
	} else if (mod->type == CLUSTER) {
		mark_submodules(mod->sub, mod_info->m_status);
		mod_info->m_status = mod_status(mod);
	} else if (mod->type == METACLUSTER) {
	 	if (mod_info->m_status == UNSELECTED)
			mark_submodules(mod->head->sub, mod_info->m_status);
		else
			mark_submodules(mod->sub, mod_info->m_status);

		for (mod1 = mod->parent->sub; mod1 != NULL && mod1->sub != NULL;
							mod1 = mod1->next) {
			if (mod1->sub == mod)
				continue;
			mod_status(mod1);

		}
	}
	return (mod_info->m_status);

#undef mod_info

}

/*
 * get_machintype()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
MachineType
swi_get_machinetype(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_machinetype");
#endif
	return (machinetype);
}

/*
 * set_machinetype()
 *	Set the global machine "type" specifier
 * Parameters:
 *	type	- machine type specifier (valid types: MT_SERVER, 
 *		  MT_DATALESS, MT_DISKLESS, MT_CCLIENT, MT_SERVICE)
 * Return:
 *	none
 * Status:
 *	Public
 */
void
swi_set_machinetype(MachineType type)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_machinetype");
#endif
	machinetype = type;
}

/*
 * get_current_locale()
 *	Returns the current locale.  If no locale has previously been set,
	the default locale ("C") is returned.
 * Parameters:
 *	none
 * Return:
 *	"C"	- no current locale defined
 *	char *	- name of current locale
 * Status:
 *	Public
 */
char *
swi_get_current_locale(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_current_locale");
#endif
   if (current_locale[0] == '\0')
	return ("C");
   else
	return (current_locale);
}

/*
 * set_current_locale()
 *	Set the "current" locale to be 'loc'.
 * Parameters:
 *	loc	- string containing location to be set
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_set_current_locale(char * loc)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_current_locale");
#endif
   strcpy(current_locale, loc);
}

/*
 * get_current_locale()
 *	Return the "current" locale.
 * Parameters:
 *	none
 * Return:
 *	char *	- pointer to the current locale string
 * Status:
 *	public
 */
char *
swi_get_default_locale(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_default_locale");
#endif
   return (default_locale);
}

/*
 * get_rootdir()
 *	Returns the rootdir previously set by a call to set_rootdir(). If 
 *	set_rootdir() hasn't been called this returns a pointer to a NULL 
 *	string.
 * Parameters:                 
 *	none 
 * Return:
 *	char *	- pointer to current rootdir string
 * Status:
 *	public
 */
char *
swi_get_rootdir(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_rootdir");
#endif
	return (rootdir);
}

/*
 * set_rootdir()
 *	Sets the global 'rootdir' variable. Used to install packages
 *	to 'newrootdir'.
 * Parameters:
 *	newrootdir	- pathname used to set rootdir
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_set_rootdir(char * newrootdir)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_rootdir");
#endif
	(void) strcpy(rootdir, newrootdir);
	canoninplace(rootdir);
}


/*
 * toggle_product()
 * Parameters:
 *	mod	-
 *	type	-
 * Return:
 *	
 * Status:
 *	public
 */
int
swi_toggle_product(Module * mod, ModStatus type)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("toggle_product");
#endif
	mark_submodules(mod->sub, type);
	mod->info.prod->p_status = mod_status(mod->sub);
	return (mod->info.prod->p_status);
}

/*
 * mark_module_action()
 * Parameters:
 *	modp	-
 *	action	-
 * Return:
 *	SUCCESS		-
 *	ERR_INVALIDTYPE -
 * Status:
 *	public
 */
int
swi_mark_module_action(Module * modp, Action action)
{
	Module	*mp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mark_module_action");
#endif

	if (modp->type == PRODUCT || modp->type == MEDIA)
		return (ERR_INVALIDTYPE);

	modp->info.mod->m_action = action;

	for (mp = modp->sub; mp; mp = mp->next) {
		mark_module_action(mp, action);
	}
	return (SUCCESS);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * mark_submodules()
 *
 * Parameters:
 *	first	-
 *	status	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
mark_submodules(Module * first, ModStatus status)
{
	register Module *mod;
	Module	*prod;
	Node	*np;
	
	/* find product so instances can be correctly marked */
	for (prod = first->parent; prod != NULL; prod = prod->parent) {
		if (prod->type == PRODUCT || prod->type == NULLPRODUCT)
			break;
	}

#define mod_info  ((Modinfo *) mod->info.mod)
	for (mod = first; mod != NULL; mod = mod->next) {
		/*
		 * maintain refcnts for packages and clusters:
		 * select an unselected package/cluster-> 
		 *				set selected, incr refcnt
		 * select a selected package/cluster-> incr refcnt
		 * unselect an unselected p/c->	do nothing
		 * unselect a selected p/c->	   decr refcnt,
		 * if refcnt = 0 set unselected
		 */
		if (mod_info->m_status != REQUIRED) {
			if (status == SELECTED) {
				mod_info->m_refcnt += 1;
				mod_info->m_status = SELECTED;
			} else if (status == UNSELECTED) {
				if (mod_info->m_status == SELECTED &&
							mod_info->m_refcnt)
					mod_info->m_refcnt =
						(mod_info->m_refcnt ?
						mod_info->m_refcnt - 1 : 0);

				if (mod_info->m_refcnt == 0)
					mod_info->m_status = UNSELECTED;
			}

			if (mod->type == CLUSTER) {
				mark_submodules(mod->sub, status);
				mod_info->m_status = mod_status(mod);
			} else if (mod->type == METACLUSTER) {
				mark_submodules(mod->sub, status);
				mod_info->m_status = mod_status(mod);
			}
			
			/* update architecture specific packages */
			if (prod && prod->info.prod->p_packages != NULL) {
				np = findnode(prod->info.prod->p_packages,
						mod->info.mod->m_pkgid);
				if (np != NULL)
					update_selected_arch(np, (caddr_t)prod);
			}
		}
		/*
		 * if this is a package, and it has localizations mark any
		 * localization packages whose locales have been selected
		 */
		if ((mod->type == PACKAGE) && mod_info->m_l10n)
			mark_locales(mod, mod_info->m_status);
	}
#undef mod_info
}
