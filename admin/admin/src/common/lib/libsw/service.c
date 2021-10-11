#ifndef lint
#pragma ident "@(#)service.c 1.24 95/07/27"
#endif
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#include <signal.h>
#include <fcntl.h>

/* Public Function Prototypes */

#ifdef notdef
void     	remove_all_services(void); 
int      	remove_service(Module *, char *); 
#endif
int      	add_service(Module *, char *, Module *);

/* Local Function Prototypes */

static void	mark_loaded_pkgs(Module *, Module *);
static Module * find_svc_media(Module *);
#ifdef notdef
static Module * find_svc_view(Module *);
static int	rm_selected_arch(Node *, caddr_t);
static void    	rm_view(Module *, Module *); 
#endif

#define	streq(s1,s2)		(strcmp((s1),(s2)) == 0)

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * add_service()
 * Parameters:
 *	prod	-
 *	arch	-
 *	clstr	-
 * Return:
 * Status:
 *	public
 */
int
add_service(Module *prod, char *arch, Module *clstr)
{
	Module *media, *mp, *locmed;
	char    buf[MAXPATHLEN];
	int	state = 0;
	Module *mod;
	Node	*np;
	int	retval;
	Product	*pi;
	Arch	*ap;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("add_service");
#endif

	pi = prod->info.prod;
	media = find_svc_media(prod);
	if (media == NULL) {
		(void) sprintf(buf, "/export/%s_%s", pi->p_name,
		    pi->p_version);
		if (find_media(buf, NULL) != NULL)
			return (ERR_DIFFREV);
		locmed = get_localmedia();
		/* if service being added is already the native OS */
		if (locmed &&
		    locmed->sub &&
		    locmed->sub->info.prod->p_name &&
		    streq(locmed->sub->info.prod->p_name, pi->p_name) &&
		    locmed->sub->info.prod->p_version &&
		    streq(locmed->sub->info.prod->p_version, pi->p_version)) {
			media = duplicate_media(locmed);
			free(media->info.media->med_dir);
			media->info.media->med_dir = xstrdup(buf);
			split_svr_svc(locmed, media);
			for (ap = media->sub->info.prod->p_arches;
			    ap != NULL; ap = ap->a_next) {
				ap->a_selected = FALSE;
				ap->a_loaded = FALSE;
			}
			media->info.media->med_machine = MT_SERVICE;
			media->info.media->med_type = INSTALLED_SVC;
		} else {
			media = add_new_service(buf);
			/*
			 * kludge here:  add_new_service sets p_pkgdir to
			 * the rootdir "/export/Solaris_2.x".  Don't know why,
			 * but it make be risky to change that at this point,
			 * since initial install may depend on that.  So just
			 * free that bogus value here and set p_pkgdir to
			 * the actual location of the packages.
			 */
			if (media->sub->info.prod->p_pkgdir)
				free(media->sub->info.prod->p_pkgdir);
			media->sub->info.prod->p_pkgdir = xmalloc(strlen(buf) +
			    strlen("/var/sadm") + 1);
			strcpy(media->sub->info.prod->p_pkgdir, buf);
			strcat(media->sub->info.prod->p_pkgdir, "/var/sadm");
			media->sub->info.prod->p_rootdir = xstrdup(buf);
			media->sub->info.prod->p_rev = xstrdup(pi->p_rev);
		}
		media->info.media->med_flags |= NEW_SERVICE;
		state = 1;
	}
/* FIX LATER WHEN REWRITING THIS */
#ifdef notdef
	else if (media->info.media->med_flags & SVC_TO_BE_REMOVED)
		media->info.media->med_flags &= ~SVC_TO_BE_REMOVED;
#endif

	if (has_view(prod, media) == FAILURE) {
		(void) load_view(prod, media);
		clear_view(prod);
		retval = update_module_actions(media, prod, TO_BE_PRESERVED,
		    ADD_SVC_TO_ENV);
		if (retval != SUCCESS)
			return (retval);
	} else {
		(void) load_view(prod, media);
		state = 2;
	}

	if (clstr == NULL) {
		if (state == 0) {
			mark_loaded_pkgs(prod, media->sub);
			(void) mod_status(prod);
		} else if (state == 1)
			mark_submodules(prod->sub, SELECTED);
	} else {
		(void) mark_module(clstr, SELECTED);
	}

	if (media->info.media->med_flags & SPLIT_FROM_SERVER) {
		for (mp = get_media_head(); mp != NULL; mp = mp->next) {
			if (mp->info.media->med_type == INSTALLED &&
			    strcmp(mp->info.media->med_dir, "/") == 0) {
				for (mod = mp->sub->sub; mod != NULL;
				    mod = mod->next) {
					if (mod->type == METACLUSTER) {
						np = findnode(
						    prod->info.prod->p_clusters,
						    mod->info.mod->m_pkgid);
						if (np)	
							(void) mark_module((Module *)
							    (np->data),
							    SELECTED);
					}
				}
			}
		}
	}

	if (select_arch(prod, arch) != SUCCESS)
		return (ERR_BADARCH);
	
	/* always select the native arch */
	select_arch(prod, get_default_arch());

	mark_arch(prod);

	if (!(media->info.media->med_flags & NEW_SERVICE))
		media->info.media->med_flags |= SVC_TO_BE_MODIFIED;

	return (SUCCESS);
}

/*
 * remove_service()
 * Parameters:
 *	prod	-
 *	arch	-
 * Return:
 * Status:
 *	public
 */
#ifdef notdef
int
remove_service(Module *prod, char *arch)
{
	
	Arch   *ap; 
	Arch   *archp = NULL;
	int     selected = 0;
	Module *mod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("remove_service");
#endif

	/* not allowed to remove the architecture of the machine */
	if (arch && (strcmp(arch, get_default_arch()) == 0
		||  strcmp(arch, get_default_impl()) == 0
		||  strcmp(arch, get_default_inst()) == 0))
			return (ERR_INVARCH);

	/* a service which doesn't yet exist */
	if (prod->info.prod->p_packages == NULL) {
		if ((mod = find_svc_view(prod)) == NULL)
			return (ERR_DIFFREV);
		if (arch == NULL) {
			rm_view(mod->sub, prod->parent);
			if (prod->parent->prev != NULL)
				prod->parent->prev->next = prod->parent->next;
			if (prod->parent->next != NULL)
				prod->parent->next->prev = prod->parent->prev;
			free_media(prod->parent);
		} else {
			deselect_arch(mod->sub, arch);
			mark_arch(mod->sub);
		}
		return (SUCCESS);
	}

	if (arch == NULL) {
		prod->parent->info.media->med_flags |= SVC_TO_BE_REMOVED;
		return (SUCCESS);
	}
	
	for (ap = get_all_arches(prod); ap != NULL; ap = ap->a_next) {
		if (supports_arch(ap->a_arch, arch) == TRUE) {
			archp = ap;
			continue;
		} 
		if (ap->a_selected == TRUE)
			selected ++;
	}
	if (archp == NULL) 
		return (ERR_BADARCH);

	if (selected == 0) {
		prod->parent->info.media->med_flags |= SVC_TO_BE_REMOVED;
		return (SUCCESS);
	}
	
	if (archp->a_selected != TRUE)
		return (SUCCESS);
	
	archp->a_selected = FALSE;
	walklist(prod->info.prod->p_packages, rm_selected_arch, (caddr_t)arch);
	
	return (SUCCESS);
	
}

/*
 * remove_all_services()
 *	Scan the list of media structures, and if the media type is INSTALLED_SVC
 *	and the media is flagged as a NEW_SERVICE, then the media flags are marked
 *	with SVC_TO_BE_REMOVED.
 *	NOTE:	Only new services can be marked for removal. Removal of existing 
 *		services not yet supported.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	public
 */
void
remove_all_services(void)
{
	Module	*media;
	Media	*mi;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("remove_all_services");
#endif

	for (media = get_media_head(); media != NULL; media = media->next) {
		mi = media->info.media;
		if ((mi->med_type != INSTALLED_SVC) || !(mi->med_flags & NEW_SERVICE))
			continue;
		mi->med_flags |= SVC_TO_BE_REMOVED;
	}
	return;
}
#endif

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

#ifdef notdef
/*
 * rm_selected_arch()
 * Parameters:
 *	np	- node pointer
 *	data	-
 * Return:
 * Status:
 *	private
 */
static int
rm_selected_arch(Node *np, caddr_t data)
{
	Modinfo *i, *k;

	for (i = (Modinfo*)np->data; i != NULL; i = next_inst(i)) {
		if (strcmp(i->m_arch, (char *)data) == 0) {
			for (k = i; k != NULL; k = next_patch(k)) 
				k->m_action = TO_BE_REMOVED;
		}
	}
	return (0);
}

/*
 * find_svc_view()
 * Parameters:
 *	prod	-
 * Return:
 * Status:
 *	private
 */
static Module *
find_svc_view(Module *prod)
{
	Module *mp;
	char    buf[MAXPATHLEN];

	for (mp = get_media_head(); mp != NULL; mp = mp->next) {
		if (mp->info.media->med_type == INSTALLED_SVC 
				|| mp->info.media->med_type == INSTALLED)
			continue;
		(void) sprintf(buf, "%s_%s", mp->sub->info.prod->p_name, 
				mp->sub->info.prod->p_version); 
		if (strcmp(buf, prod->parent->info.media->med_volume) != 0)
			continue;
		if (strcmp(prod->info.prod->p_rev, 
					mp->sub->info.prod->p_rev) !=0)
			continue;
		break;
	} 
	return (mp);	
}
#endif

/*
 * find_svc_media()
 * Parameters:
 *	prod	-
 * Return:
 * Status:
 *	private
 */
static Module *
find_svc_media(Module *prod)
{
	Module *mp;
	char    buf[MAXPATHLEN];

	for (mp = get_media_head(); mp != NULL; mp = mp->next) {
		if (mp->info.media->med_type != INSTALLED_SVC)
			continue;
		if (mp == prod->parent)
			continue;
		(void) sprintf(buf, "%s_%s", prod->info.prod->p_name, 
						prod->info.prod->p_version);
		if (strcmp(buf, mp->info.media->med_volume) != 0)
			continue;
		if (strcmp(prod->info.prod->p_rev, 
					mp->sub->info.prod->p_rev) !=0)
			continue;
		break;
	} 
	return (mp);	
} 

/*
 * mark_loaded_pkgs()
 * Parameters:
 *	prod	-
 * 	exist	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
mark_loaded_pkgs(Module * prod, Module * exist)
{
	Node *pkg, *np;

	for (pkg = exist->info.prod->p_packages->list->next; 
		pkg != exist->info.prod->p_packages->list; pkg = pkg->next) {
		np = findnode(prod->info.prod->p_packages, 
					((Modinfo *)pkg->data)->m_pkgid);
		if (np != NULL)
			((Modinfo*)np->data)->m_status = SELECTED;
	}
	return;
}

#ifdef notdef
/*
 * rm_view()
 *	Scan the list of views associated with 'prod'. If one matches
 *	'media', then remove it from the list of views and destroy it.
 * Parameters:
 *	prod	- pointer to product module
 *	media	- pointer to media structure identifying view to delete
 * Return:
 *	none
 * Status:
 *	public
 */
static void
rm_view(Module * prod, Module * media)
{
	Product	*p, *prev;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("rm_view");
#endif
		
	prev = (Product *) NULL;
	for (p = prod->info.prod; p != NULL; p = p->p_next_view) {
		if (p->p_view_from == media) {
			if (prev == (Product *) NULL)
				prod->info.prod = p->p_next_view;
			else
				prev->p_next_view = p->p_next_view;
			free_list(p->p_view_4x);
			free_list(p->p_view_pkgs);
			free_list(p->p_view_cluster);
			free_list(p->p_view_locale);
			free_list(p->p_view_arches);
			free(p);
			break;
		}
		prev = p;
	}
	return;
}
#endif
