#ifndef lint
#pragma ident   "@(#)update_actions.c 1.68 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "sw_lib.h"

#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <netdb.h>

struct client {
	struct client	*next_client;
	char		client_name[MAXHOSTNAMELEN];
	char		*client_root;
};

/* Public Statics */
int	in_final_upgrade_stage = 0;
char *swdebug_pkg_name = "SUNWnosuchpkg";
SW_diffrev	*g_sw_diffrev = NULL;

/* Local Statics and Constats */

static char	*template_dir = "/export/root/templates";
static int	s_is_dataless = -1;	/* initialized to "unknown" */
static char	stringhold[MAXPATHLEN];
static int	do_dumptree = 0;

/* Local Globals */

#define	CLIENT_TO_BE_UPGRADED	0x0001
#define	streq(s1, s2)		(strcmp((s1), (s2)) == 0)

int		g_is_swm = 0;
char		*g_swmscriptpath;
struct client	*g_client_list;

/* Public Function Prototypes */
int	swi_load_clients(void);
void	swi_update_action(Module *);
void	swi_upg_select_locale(Module *, char *);
void	swi_upg_deselect_locale(Module *, char *);

/* Library function protypes */
void	generate_swm_script(char *);
int	is_server(void);
int	is_dataless_machine(void);
void	mark_preserved(Module *);
void	mark_removed(Module *);
Module *	get_localmedia(void);
void	set_final_upgrade_mode(int);

/* Library Function Prototypes */
int	update_module_actions(Module *, Module *, Action, Environ_Action);
char *	split_name(char **);
void	unreq_nonroot(Module *);
Modinfo *	find_new_package(Product *, char *, char *,
    Arch_match_type *);
int	debug_bkpt(void);

/* Local Function Prototypes */

static Arch_match_type 		compatible_arch(char *, char *);
static struct client *	find_clients(void);
static int		process_package(Module *, Modinfo *, Action,
    Environ_Action);
static void 		mark_cluster_tree(Module *, Module *);
static int		mark_action(Node *, caddr_t);
static int		mark_module_tree(Module *, Module *, Action,
    Environ_Action);
static Node * 		mark_cluster_selected(char *);
static char *		genspooldir(Modinfo *);
static int 		set_dflt_action(Node *, caddr_t);
static void 		_set_dflt_action(Modinfo *, Module *);
static void 		process_cluster(Module *);
static void 		mark_required_metacluster(Module *);
static void 		set_inst_dir(Module *, Modinfo *, Modinfo *);
static int 		cluster_match(char *, Module *);
static void 		walktree_for_l10n(Module *);
static void 		spool_selected_arches(char *);
static int 		is_arch_selected(char *);
static int 		is_arch_supported(char *);
static int 		is_ptype_usr(Node *, caddr_t);
static int 		unreq(Node *, caddr_t);
static void 		reset_action(Module *);
static void 		reset_cluster_action(Module *);
static int		reset_instdir(Node *, caddr_t);
static int 		_reset_cluster_action(Node *, caddr_t);
static void 		reprocess_package(Modinfo *);
static void		reprocess_module_tree(Module *, Module *);
static int 		set_alt_clsstat(int selected, Module *);
static void		update_patch_status(Product *);
static void		diff_rev(Modinfo *, Modinfo *);
static void		set_instances_action(Modinfo *mi, Action data);

void		 unreq_nonroot(Module *mod);

#define	REQUIRED_METACLUSTER "SUNWCreq"
#define	MAXPKGNAME	10

static Product		*g_newproduct = NULL;
static Module		*g_newproductmod = NULL;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * generate_swm_script()
 * Parameters:
 *	scriptpath	-
 * Return:
 *	none
 * Status:
 *	void
 */
void
generate_swm_script(char * scriptpath)
{
	Module *mod;
	Module *prodmod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("generate_swm_script");
#endif

	g_is_swm = 1;
	g_swmscriptpath = scriptpath;

	if (do_dumptree)
		dumptree("/tmp/dump.out");

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
					mod->info.media->med_type != INSTALLED)
			for (prodmod = mod->sub; prodmod != NULL;
						prodmod = prodmod->next)
				(void) write_script(prodmod);
	}
	return;
}

/*
 * is_server()
 * Parameters:
 *	none
 * Return:
 *	0	-
 *	1	-
 * Status:
 *	public
 */
int
is_server(void)
{
	Module *mod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next)
		if (mod->info.media->med_type == INSTALLED_SVC &&
			!(mod->info.media->med_flags & SVC_TO_BE_REMOVED))  {
			return (1);
		}
	return (0);
}

/*
 * is_dataless_machine()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
is_dataless_machine(void)
{
	Module *mod;

	if (s_is_dataless != -1)
		return (s_is_dataless);

	/*
	 *  assume dataless  until a usr-type
	 *  package is found.
	 */
	s_is_dataless = 1;
	mod = get_media_head();
	while (mod != NULL) {
		if (mod->info.media->med_type == INSTALLED)  {
			walklist(mod->sub->info.prod->p_packages,
				is_ptype_usr, (caddr_t) 0);
			break;
		}
		mod = mod->next;
	}
	return (s_is_dataless);
}

/*
 * update_module_actions()
 * Parameters:
 *	media_mod	-
 *	prodmod		-
 *	action		-
 *	env_action	-
 * Return:
 *	none
 * Status:
 *	public
 */
int
update_module_actions(Module * media_mod, Module * prodmod, Action action,
    Environ_Action env_action)
{
	Module	*mod, *mod2;
	Modinfo *mi;
	int	retval;

	if (media_mod == NULL || media_mod->sub == NULL)
		return (ERR_INVALID);
	g_newproductmod = prodmod;
	g_newproduct = prodmod->info.prod;
	reset_action(media_mod);
	reset_cluster_action(media_mod);

	walklist(g_newproductmod->info.prod->p_packages, reset_instdir,
	    (caddr_t)0);

	mark_required_metacluster(prodmod);

	/* process the installed metacluster */
	for (mod = media_mod->sub->sub; mod != NULL; mod = mod->next)
		if (mod->type == METACLUSTER) {
			mark_cluster_tree(media_mod, mod);
			retval = mark_module_tree(media_mod, mod, action,
			    env_action);
			if (retval != SUCCESS)
				return (retval);
			break;
		}

	/*
	 * Now process the other packages, looking for packages
	 * that are installed but that are not in the installed
	 * metacluster.
	 */

	for (mod = media_mod->sub->sub; mod != NULL; mod = mod->next) {
		if (mod->type == METACLUSTER)
			continue;
		if (mod->type == CLUSTER)
			mark_cluster_tree(media_mod, mod);
		retval = mark_module_tree(media_mod, mod, action, env_action);
		if (retval != SUCCESS)
			return (retval);
	}

	/*
	 * Set up the actions for the currently installed localization
	 * packages and the new versions.
	 */
	for (mod = media_mod->sub->info.prod->p_locale; mod != NULL;
		mod = mod->next)
		for (mod2 = mod->sub; mod2 != NULL; mod2 = mod2->next) {
			mi = mod2->info.mod;
			if (mi->m_shared != NULLPKG) {
				retval = process_package(media_mod, mi,
					action, env_action);
				if (retval != SUCCESS)
					return (retval);
			}
			while ((mi = next_inst(mi)) != NULL)
				if (mi->m_shared != NULLPKG) {
					retval = process_package(media_mod, mi,
					    action, env_action);
					if (retval != SUCCESS)
						return (retval);
				}
		}

	/*
	 * now set up the action and basedir fields for all
	 * remaining packages in the media tree
	 */
	walklist(prodmod->info.prod->p_packages, set_dflt_action,
	    (caddr_t)media_mod);

	/* set selected locales */
	mod = media_mod->sub->info.prod->p_locale;
	if (mod) {
		while (mod != NULL) {
			if (mod->info.locale->l_selected)
				select_locale(prodmod,
					mod->info.locale->l_locale);
			mod = mod->next;
		}
		update_l10n_package_status(prodmod);
	}

	/* clean up the cluster actions */
	for (mod = media_mod->sub->sub; mod != NULL; mod = mod->next)
		set_cluster_status(mod);

	/* mark any new l10n packages */
	walktree_for_l10n(prodmod);

	/* update the status of the patches */
	update_patch_status(media_mod->sub->info.prod);

	return (SUCCESS);
}

/*
 * split_name()
 * Parameters:
 *	cpp	-
 * Return:
 * Status:
 *	public
 */
char *
split_name(char ** cpp)
{
	char *p;
	int n;

	if (*cpp != NULL && **cpp != NULL) {
		p = (char *)strpbrk(*cpp, " ");
		if (p == NULL)
			n = strlen(*cpp);
		else
			n = p - *cpp;
		strncpy(stringhold, *cpp, n);
		stringhold[n] = '\0';
		*cpp += n;
		if (**cpp == ' ')
			(*cpp)++;
		return (stringhold);
	} else
		return (NULL);
}

/*
 * mark_preserved()
 *	Takes a module pointer to a media struct
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
mark_preserved(Module * mod)
{
	Module	*prodmod;

	prodmod = mod->sub;
	while (prodmod) {
		walklist(prodmod->info.prod->p_packages, mark_action,
		    (caddr_t)TO_BE_PRESERVED);
		prodmod = prodmod->next;
	}
	return;
}

/*
 * mark_removed()
 * 	Takes a module pointer to a media struct
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
mark_removed(Module *mod)
{
	Module	*prodmod;

	prodmod = mod->sub;
	while (prodmod) {
		walklist(prodmod->info.prod->p_packages, mark_action,
		    (caddr_t)TO_BE_REMOVED);
		prodmod = prodmod->next;
	}
	return;
}

/*
 * load_clients()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
swi_load_clients(void)
{
	struct client		*clientptr;

	if (is_server()) {
		g_client_list = find_clients();
		for (clientptr = g_client_list; clientptr != NULL;
			clientptr = clientptr->next_client) {
			load_installed(clientptr->client_root, FALSE);
		}
	}
	return (0);
}

/*
 * update_action()
 * The user has toggled a module in the main screen (which is the
 * system's own environment).  Now, make every other environment
 * agree with the user's choice.  Note: this is complex, because
 * some of the environments may already be in the same state as
 * the main environment.  Those environments must be left
 * untouched.  The logic must also take into account partial
 * clusters.  Partial clusters can have a partial status of either
 * SELECTED (there is at least one non-required package selected)
 * or UNSELECTED (all component packages are either REQUIRED or
 * UNSELECTED).  All partial clusters must have their status
 * changed to be consistent with the main cluster, which may
 * itself be PARTIAL.  Here's the logic for clusters (it's obvious
 * for packages) :
 *
 *		    Module Status of Cluster in Alternate Environment
 *
 *		   SELECTED	UNSELECTED	PARTIAL
 * Mod Status
 * of cluster in ---------------------------------------------------
 * main env.	 |	      |		    | toggle module;
 *		 |	      |		    | if (still PARTIAL)
 *   SELECTED	 | no action  |	toggle	    |    toggle module again
 *		 |--------------------------------------------------
 *		 |	      |		    | if (partial_status ==
 *   UNSELECTED  | toggle     |	no action   |    SELECTED) toggle
 *		 |--------------------------------------------------
 *		 |	      |		    | if (partial_status ==
 *   PARTIAL	 | toggle     |	no action   |    SELECTED) toggle
 *		 |--------------------------------------------------
 *
 * Note that an incoming cluster status of PARTIAL is equivalent to
 * to UNSELECTED.  This is because the result of toggling a cluster
 * is never PARTIAL with a partial_status of SELECTED.
 * Parameters:
 *	toggled_mod	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_update_action(Module * toggled_mod)
{
	Module	*mod;
	Node	*node;
	Module	*mediamod;
	char	id[MAXPKGNAME];
	int	selected, change_made;

	mediamod = get_localmedia();
	reprocess_module_tree(mediamod, mediamod->sub);
	mark_arch(g_newproductmod);
	update_l10n_package_status(g_newproductmod);
	update_patch_status(mediamod->sub->info.prod);
	mod = toggled_mod;
	(void) strcpy(id, mod->info.mod->m_pkgid);
	selected = mod->info.mod->m_status;

	if (selected == REQUIRED)
		return;

	/* find the same module in every view and update it also */

	mediamod = get_media_head();
	while (mediamod != NULL) {
		change_made = 0;
		if ((mediamod->info.media->med_type == INSTALLED_SVC ||
		    mediamod->info.media->med_type == INSTALLED) &&
		    mediamod != get_localmedia() &&
		    has_view(g_newproductmod, mediamod) == SUCCESS) {
			(void) load_view(g_newproductmod, mediamod);
			if (mod->type == CLUSTER) {
				node = findnode(g_newproduct->p_clusters, id);
				if (node == NULL)
					continue;
				change_made = set_alt_clsstat(selected,
					(Module *)(node->data));
			} else if (mod->type == PACKAGE) {
				node = findnode(g_newproduct->p_packages, id);
				if (node == NULL)
					continue;
				if (((Modinfo *)(node->data))->m_status !=
					selected) {
					((Modinfo *)(node->data))->m_status =
						selected;
					change_made = 1;
				}
			}
			if (change_made) {
				reprocess_module_tree(mediamod, mediamod->sub);
				/* if a client */
				if (mediamod->info.media->med_type ==
					INSTALLED) {
					unreq_nonroot(g_newproductmod);
					set_primary_arch(g_newproductmod);
				} else
					mark_arch(g_newproductmod);
				update_l10n_package_status(g_newproductmod);
				update_patch_status(mediamod->sub->info.prod);
			}
		}
		mediamod = mediamod->next;
	}
	(void) load_view(g_newproductmod, get_localmedia());
}

/*
 * upg_select_locale()
 * Parameters:
 *	prodmod	-
 *	locale	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_upg_select_locale(Module *prodmod, char *locale)
{
	Module *mediamod;

	mediamod = get_media_head();
	while (mediamod != NULL) {
		if ((mediamod->info.media->med_type == INSTALLED_SVC ||
		    mediamod->info.media->med_type == INSTALLED) &&
		    has_view(prodmod, mediamod) == SUCCESS) {
			(void) load_view(prodmod, mediamod);
			select_locale(prodmod, locale);
			update_l10n_package_status(prodmod);
		}
		mediamod = mediamod->next;
	}
	(void) load_view(prodmod, get_localmedia());
}

/*
 * upg_deselect_locale()
 * Parameters:
 *	prodmod	-
 *	locale	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_upg_deselect_locale(Module * prodmod, char * locale)
{
	Module *mediamod, *m;
	int	locale_loaded;

	mediamod = get_media_head();
	while (mediamod != NULL) {
		if ((mediamod->info.media->med_type == INSTALLED_SVC ||
		    mediamod->info.media->med_type == INSTALLED) &&
		    mediamod->sub != NULL &&
		    has_view(prodmod, mediamod) == SUCCESS) {
			(void) load_view(prodmod, mediamod);
			locale_loaded = FALSE;
			for (m = mediamod->sub->info.prod->p_locale;
				m != NULL; m = m->next) {
				if (streq(locale, m->info.locale->l_locale) &&
					m->info.locale->l_selected)
					locale_loaded = TRUE;
			}
			if (locale_loaded == FALSE) {
				deselect_locale(prodmod, locale);
				update_l10n_package_status(prodmod);
			}
		}
		mediamod = mediamod->next;
	}
	(void) load_view(prodmod, get_localmedia());
}

/*
 * get_localmedia()
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 *	public
 */
Module *
get_localmedia(void)
{
	Module *mod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED &&
			strcmp(mod->info.media->med_dir, "/") == 0)
			return (mod);
	}
	return ((Module *)NULL);
}

/*
 * unreq_nonroot()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
unreq_nonroot(Module * mod)
{
	walklist(mod->info.prod->p_packages, unreq, (caddr_t)0);
	while (mod != NULL) {
		if (mod->type == METACLUSTER &&
		    strcmp(mod->info.mod->m_pkgid,
			REQUIRED_METACLUSTER) == 0) {
			set_cluster_status(mod);
			break;
		}
		mod = mod->next;
	}
	return;
}

/*
 * set_final_upgrade_mode()
 * Parameters:
 *	mode
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_final_upgrade_mode(int mode)
{
	in_final_upgrade_stage = mode;
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * unreq()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
unreq(Node * np, caddr_t data)
{
	Modinfo	*mi;

	mi = (Modinfo *)(np->data);
	if (mi->m_shared != NULLPKG && mi->m_sunw_ptype != PTYPE_ROOT) {
		mi->m_status = UNSELECTED;
		mi->m_action = CANNOT_BE_ADDED_TO_ENV;
	}
	while ((mi = next_inst(mi)) != NULL) {
		if (mi->m_sunw_ptype != PTYPE_ROOT) {
			mi->m_status = UNSELECTED;
			mi->m_action = CANNOT_BE_ADDED_TO_ENV;
		}
	}
	return (0);
}

/*
 * is_ptype_usr()
 * Parameters:
 *	np	- node pointer
 *	data	- ignored
 * Return:
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
is_ptype_usr(Node * np, caddr_t data)
{
	Modinfo	*mi;

	mi = (Modinfo *)(np->data);
	if (mi->m_sunw_ptype == PTYPE_USR)
		s_is_dataless = 0;
	return (0);
}

/*
 * set_dflt_action()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 * Status:
 *	private
 */
static int
set_dflt_action(Node * np, caddr_t data)
{
	Modinfo	*mi;
	Module *media_mod;

	mi = (Modinfo *)(np->data);
	/*LINTED [alignment ok]*/
	media_mod = (Module *)data;
	_set_dflt_action(mi, media_mod);
	while ((mi = next_inst(mi)) != NULL)
		_set_dflt_action(mi, media_mod);
	return (0);
}

/*
 * _set_dflt_action()
 * Parameters:
 *	mi	  -
 *	media_mod -
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_set_dflt_action(Modinfo * mi, Module * media_mod)
{
	if (mi->m_shared != NULLPKG && mi->m_action == NO_ACTION_DEFINED) {
		if (mi->m_sunw_ptype == PTYPE_ROOT) {
			if (media_mod->info.media->med_type == INSTALLED)
				mi->m_action = TO_BE_PKGADDED;
			else
				mi->m_action = TO_BE_SPOOLED;
		} else {
			if (media_mod->info.media->med_type == INSTALLED)
				mi->m_action = TO_BE_PKGADDED;
			else { /* it's a service */
				/*
				 *  In 2.1, opt packages have a SUNW_PKGTYPE
				 *  of usr and a basedir of /opt.  In 2.2,
				 *  opt packages have a SUNW_PKGTYPE of
				 *  UNKNOWN.
				 */
				if (mi->m_sunw_ptype == PTYPE_UNKNOWN ||
				    (mi->m_sunw_ptype == PTYPE_USR &&
					streq(mi->m_basedir, "/opt")) ||
					streq(mi->m_arch, "all")) {
					mi->m_action =
						CANNOT_BE_ADDED_TO_ENV;
					return;
				}

				if (media_mod->info.media->med_flags &
					SPLIT_FROM_SERVER) {
					if (supports_arch(
						get_default_arch(), mi->m_arch))
						mi->m_action =
							ADDED_BY_SHARED_ENV;
					else
						mi->m_action = TO_BE_PKGADDED;
				} else
					mi->m_action = TO_BE_PKGADDED;
			}
		}
		if (mi->m_action == TO_BE_PKGADDED ||
			mi->m_action == TO_BE_SPOOLED)
			set_inst_dir(media_mod, mi, NULL);
	}
	return;
}

/*
 * mark_module_tree()
 * Parameters:
 *	media_mod	-
 *	mod		-
 *	action		-
 *	env_action
 * Return:
 *	none
 * Status:
 *	private
 */
static int
mark_module_tree(Module *media_mod, Module *mod, Action action,
    Environ_Action env_action)
{
	Modinfo *mi,
		*mip;	/* For looping throught patches */
	Module	*child;
	int	retval;

	/*
	 * Do a depth-first search of the module tree, marking
	 * modules appropriately.
	 */
	mi = mod->info.mod;
	if (mod->type == PACKAGE) {
		/*
		 * When the service is of a different ISA than the server,
		 * and the package doesn't exist for the native ISA, the
		 * module at the head of the instance chain will be a
		 * spooled package, not a NULLPKG, so don't assume that
		 * when we're looking at a root package for a service, that
		 * the first instance is necessarily a NULLPKG.
		 */
		if ((!(media_mod->info.media->med_type == INSTALLED_SVC &&
		    (media_mod->info.media->med_flags & SPLIT_FROM_SERVER) &&
		    mi->m_sunw_ptype == PTYPE_ROOT)) ||
		    mi->m_shared != NULLPKG) {
			retval = process_package(media_mod, mi, action,
			    env_action);
			if (retval != SUCCESS)
				return (retval);
			/* process any patches also. */
			if (mi->m_next_patch != NULL)
				for (mip = next_patch(mi); mip != NULL;
				    mip = next_patch(mip)) {
					retval = process_package(media_mod,
					    mip, action, env_action);
					if (retval != SUCCESS)
						return (retval);
				}
		}
	}
	/*
	 * Process all of the patches for all of the instances.
	 *   Outer Loop gets all of the instances and the inner loop
	 *   gets all of the patches.
	 */
	for (mi = next_inst(mi); mi != NULL; mi = next_inst(mi)) {
		for (mip = mi; mip != NULL; mip = next_patch(mip)) {
			retval = process_package(media_mod, mip, action,
			    env_action);
			if (retval != SUCCESS)
				return (retval);
		}
	}
	child = mod->sub;
	while (child) {
		retval = mark_module_tree(media_mod, child, action, env_action);
		if (retval != SUCCESS)
			return (retval);
		child = child->next;
	}
	return (SUCCESS);
}

/*
 * reprocess_module_tree()
 * Parameters:
 *	media_mod	-
 *	mod		-
 * Return:
 *
 * Status:
 *	private
 */
static void
reprocess_module_tree(Module * media_mod, Module * mod)
{
	Modinfo *mi;
	Node	*node;
	Module	*child;

	/*
	 * Do a depth-first search of the module tree, marking
	 * modules appropriately.
	 */
	if (mod->type == PACKAGE) {
		mi = mod->info.mod;
		if ((!(media_mod->info.media->med_type == INSTALLED_SVC &&
			mi->m_sunw_ptype == PTYPE_ROOT)) ||
			mi->m_shared != NULLPKG)
			reprocess_package(mi);
		while ((node = mi->m_instances) != NULL) {
			mi = (Modinfo *)(node->data);
			reprocess_package(mi);
		}
	}
	child = mod->sub;
	while (child) {
		reprocess_module_tree(media_mod, child);
		child = child->next;
	}
}

/*
 * mark_cluster_tree()
 * Parameters:
 *	media_mod	-
 *	mod		-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
mark_cluster_tree(Module *media_mod, Module *mod)
{
	Module	*child;

	/*
	 * Do a depth-first search of the module tree, marking
	 * modules appropriately.
	 */
	if (mod->type == CLUSTER || mod->type == METACLUSTER)
		process_cluster(mod);
	child = mod->sub;
	while (child) {
		mark_cluster_tree(media_mod, child);
		child = child->next;
	}
	return;
}

/*
 * process_cluster()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
process_cluster(Module * mod)
{
	Modinfo *mi;
	char	*cp, *p;

	mi = mod->info.mod;
	/*
	 * if action is not NO_ACTION_DEFINED, we've already
	 * looked at it.
	 *
	 * metaclusters are processed even if they are only
	 * partially selected.  Regular clusters are only
	 * processed if they are fully selected.
	 */

	if (mi->m_action != NO_ACTION_DEFINED ||
		(mod->type == CLUSTER && mi->m_status != SELECTED) ||
		mi->m_status == UNSELECTED)
		return;

	if (mi->m_pkg_hist != NULL) {
		cp = mi->m_pkg_hist->replaced_by;
		while ((p = split_name(&cp)) != NULL)
			mark_cluster_selected(p);
	}
	if (mi->m_pkg_hist == NULL || !mi->m_pkg_hist->to_be_removed)
		mark_cluster_selected(mi->m_pkgid);

	mi->m_action = TO_BE_REPLACED;
	return;
}

/*
 * mark_cluster_selected()
 * Parameters:
 *	p	-
 * Return:
 *
 * Status:
 *	private
 */
static Node *
mark_cluster_selected(char *p)
{
	Node	*node;
	Module	*mod;

	node = findnode(g_newproduct->p_clusters, p);
	if (node) {
		mod = (Module *)(node->data);
		mark_module(mod, SELECTED);
	}
	return (node);
}

/*
 * find_new_package()
 * Parameters:
 *	prod	 -
 *	id	 -
 *	arch	 -
 *	archmatch -
 * Return:
 * Status:
 *	private
 */
Modinfo *
find_new_package(Product *prod, char *id, char *arch, Arch_match_type *match)
{
	Node	*node, *savenode;
	Modinfo *mi;

	*match = PKGID_NOT_PRESENT;
	node = findnode(prod->p_packages, id);
	savenode = node;
	if (node) {
		mi = (Modinfo *)(node->data);
		if (arch == NULL)
			return (mi);

		/*
		 *  ARCH_NOT_SUPPORTED means that the architecture isn't
		 *  supported at all by the installation media (example:
		 *  currently installed package has arch=sparc, but the
		 *  installation CD is for intel.  We don't want to
		 *  remove the sparc packages just because they don't
		 *  have replacements on the installation CD.)
		 *  NO_ARCH_MATCH means that the architecture is supported
		 *  on the installation CD, but that there is no replacement
		 *  package (a package with compatible architecture) for
		 *  this particular package.
		 */
		if (!is_arch_supported(arch)) {
			*match = ARCH_NOT_SUPPORTED;
			return (NULL);
		}
		*match = NO_ARCH_MATCH;
		if (mi->m_shared != NULLPKG) {
			*match = compatible_arch(arch, mi->m_arch);
			if (*match == ARCH_MATCH ||
			    *match == ARCH_LESS_SPECIFIC)
				return (mi);
		}
		while (*match == NO_ARCH_MATCH &&
		    (node = mi->m_instances) != NULL) {
			mi = (Modinfo *)(node->data);
			if (mi->m_shared != NULLPKG) {
				*match = compatible_arch(arch, mi->m_arch);
				if (*match == ARCH_MATCH ||
				    *match == ARCH_LESS_SPECIFIC)
					return (mi);
			}
		}
		if (*match == ARCH_MORE_SPECIFIC) {
			node = savenode;
			mi = (Modinfo *)(node->data);
			if (mi->m_shared != NULLPKG) {
				if (is_arch_selected(mi->m_arch))
					return (mi);
			}
			while ((node = mi->m_instances) != NULL) {
				mi = (Modinfo *)(node->data);
				if (mi->m_shared != NULLPKG)
					if (is_arch_selected(mi->m_arch))
						return (mi);
			}
		}
		return (NULL);
	}
	return (NULL);
}

static Arch_match_type
compatible_arch(char *oldarch, char *newarch)
{
	char *o_arch, *n_arch;
	char *o_endfield, *n_endfield;
	int o_len, n_len;

	o_arch = oldarch;
	n_arch = newarch;

	if (strcmp(o_arch, n_arch) == 0)
		return (ARCH_MATCH);
	if (strcmp(o_arch, "all") == 0)
		return (ARCH_MORE_SPECIFIC);
	if (strcmp(n_arch, "all") == 0)
		return (ARCH_LESS_SPECIFIC);
	while (*o_arch && *n_arch) {
		o_endfield = strchr(o_arch, '.');
		n_endfield = strchr(n_arch, '.');
		o_len = (o_endfield ? o_endfield - o_arch : strlen(o_arch));
		n_len = (n_endfield ? n_endfield - n_arch : strlen(n_arch));
		if (o_len != n_len)
			return (NO_ARCH_MATCH);
		if (strncmp(o_arch, n_arch, o_len) != 0)
			return (NO_ARCH_MATCH);
		if (o_endfield != NULL && n_endfield == NULL)
			return (ARCH_LESS_SPECIFIC);
		if (n_endfield != NULL && o_endfield == NULL)
			return (ARCH_MORE_SPECIFIC);
		if (n_endfield == NULL && o_endfield == NULL)
			return (ARCH_MATCH);
		o_arch += o_len+1;
		n_arch += o_len+1;
	}
	/*
	 *  If the architecture adheres to the standard, we should
	 *  never reach this point.
	 */
	if (*o_arch == '\0')
		printf("Illegal architecture format: %s\n", oldarch);
	if (*n_arch == '\0')
		printf("Illegal architecture format: %s\n", newarch);
	return (NO_ARCH_MATCH);
}

int
debug_bkpt(void)
{
	return (0);
}

/*
 * process_package()
 * Parameters:
 *	media_mod	-
 *	mi		-
 *	action		-
 *	env_action	-
 * Return:
 *	none
 * Status:
 *	private
 */
static int
process_package(Module *media_mod, Modinfo *mi, Action action,
    Environ_Action env_action)
{
	Modinfo  	*mnew, *imi;
	char		*cp, *p;
	Arch_match_type	archmatch;
	Product		*oldProd;
	Node		*node;

	if (mi->m_pkgid && streq(mi->m_pkgid, swdebug_pkg_name))
		(void) debug_bkpt();
	/*
	 * if action is not NO_ACTION_DEFINED, we've already
	 * looked at it.
	 */
	if (mi->m_action != NO_ACTION_DEFINED)
		return (SUCCESS);

	/*
	 *  If the package has a history entry with a REMOVE_FROM_CLUSTER
	 *  field, and the installed metacluster is one of the ones from
	 *  which this package is to be removed, mark it for removal.
	 */
	if (env_action == ENV_TO_BE_UPGRADED && mi->m_pkg_hist != NULL &&
		cluster_match(mi->m_pkg_hist->cluster_rm_list, media_mod)) {
		if (action == TO_BE_PRESERVED)
			mi->m_action = TO_BE_PRESERVED;
		else
			mi->m_action = TO_BE_REMOVED;
		mi->m_flags |= DO_PKGRM;
		mi->m_flags |= CONTENTS_GOING_AWAY;
		/*
		 * Also remove all of the instances of this packages. To do
		 * this we need to find the head of the instance chain, and
		 * mark all of the instances.
		 */
		if (media_mod->info.media->med_upg_from != (Module *)NULL)
			oldProd =
				media_mod->info.media->med_upg_from->sub->info.prod;
		else
			oldProd = media_mod->sub->info.prod;
		if ((node = findnode(oldProd->p_packages, mi->m_pkgid)) ==
		    NULL)
			return (FAILURE);
		    
		set_instances_action((Modinfo *)node->data, TO_BE_REMOVED);

		return (SUCCESS);
	}

	/*
	 * This is some special case code for upgrading from a pre-KBI
	 * service to a post-KBI service. In this case the package type KVM
	 * is special. It is special because it special meaning in the pre-KBI
	 * world, but does not in post-KBI. So to solve some problems any
	 * package of type KVM will be explicitly marked as needing removal.
	 */
	/*
	 * NOTICE THIS CODE is TEMPORARY!!
	 * This code should be checking for upgrades between pre and post
	 * KBI systems. The check for old systems has been temporally
	 * removed due to problems with NULLPRODUCTs. This will be fixed in
	 * the future.
	 */
	if (env_action == ENV_TO_BE_UPGRADED &&
	    mi->m_sunw_ptype == PTYPE_KVM && is_KBI_service(g_newproduct)) {
		if (media_mod->info.media->med_upg_from != (Module *)NULL)
			oldProd =
				media_mod->info.media->med_upg_from->sub->info.prod;
		else
			oldProd = media_mod->sub->info.prod;
		/*
		 * Now make sure we are upgrading from a pre-KBI system
		 */
		if (! is_KBI_service(oldProd))
			for (imi = mi; imi != NULL; imi = next_inst(imi)) {
				imi->m_action = TO_BE_REMOVED;
				imi->m_flags |= DO_PKGRM;
			}
	}

	/*
	 *  Use the package history entry to map existing packages to
	 *  the packages that replace them.  Set the status and
	 *  actions for the replacement packages.  If the currently-
	 *  installed package will still be installed after the
	 *  the upgrade (that is, its "to_be_removed" value is
	 *  FALSE), its status won't be set in this block.  It will
	 *  be set in the next block.
	 */
	if (env_action == ENV_TO_BE_UPGRADED && mi->m_pkg_hist != NULL) {
		if (mi->m_pkg_hist->to_be_removed)
			if (action == TO_BE_PRESERVED)
				mi->m_action = TO_BE_PRESERVED;
			else
				mi->m_action = TO_BE_REMOVED;
		cp = mi->m_pkg_hist->replaced_by;
		while ((p = split_name(&cp)) != NULL) {
			mnew = find_new_package(g_newproduct, p, mi->m_arch,
			    &archmatch);
			if (mnew) {
				/*
				 *  If currently-installed pkg is a
				 *  NULLPKG, it was explicitly-removed
				 *  by the user.  Its replacement pkgs
				 *  should be UNSELECTED.
				 */
				if (mi->m_shared == NULLPKG) {
					if (mnew->m_status != REQUIRED)
						mnew->m_status = UNSELECTED;
					while ((mnew = next_inst(mnew)) != NULL)
						if (mnew->m_status != REQUIRED)
							mnew->m_status =
							    UNSELECTED;
				} else {
					if (mnew->m_status != REQUIRED)
						mnew->m_status = SELECTED;
					if (mi->m_shared == SPOOLED_NOTDUP)
						mnew->m_action = TO_BE_SPOOLED;
					else if (mi->m_shared == NOTDUPLICATE)
						mnew->m_action = TO_BE_PKGADDED;
					else /* it's a duplicate */
						mnew->m_action =
							ADDED_BY_SHARED_ENV;
					set_inst_dir(media_mod, mnew, NULL);
				}
			}
		}
	}
	if (env_action == ADD_SVC_TO_ENV || mi->m_pkg_hist == NULL ||
	    !mi->m_pkg_hist->to_be_removed) {
		mnew = find_new_package(g_newproduct, mi->m_pkgid,
		    mi->m_arch, &archmatch);
		if ((archmatch == PKGID_NOT_PRESENT ||
		    archmatch == ARCH_NOT_SUPPORTED) &&
		    mi->m_shared != NULLPKG) {
			mi->m_action = TO_BE_PRESERVED;
			return (SUCCESS);
		}
		if (archmatch == NO_ARCH_MATCH && mi->m_shared != NULLPKG) {
			if (env_action == ADD_SVC_TO_ENV) {
				mi->m_action = TO_BE_PRESERVED;
				return (SUCCESS);
			}
			if (mi->m_shared == SPOOLED_NOTDUP) {
				mi->m_action = TO_BE_REMOVED;
				spool_selected_arches(mi->m_pkgid);
				return (SUCCESS);
			} else {
				mi->m_action = TO_BE_REMOVED;
				mi->m_flags |= DO_PKGRM;
				return (SUCCESS);
			}
		} else if (archmatch == ARCH_MORE_SPECIFIC &&
			    mi->m_shared != NULLPKG) {
			if (env_action == ADD_SVC_TO_ENV) {
				diff_rev(mi, mnew);
				return (ERR_DIFFREV);
			}
			if (mi->m_shared == SPOOLED_NOTDUP) {
				mi->m_action = TO_BE_REMOVED;
				spool_selected_arches(mi->m_pkgid);
				return (SUCCESS);
			}
			if (mnew == NULL) {
				mi->m_action = TO_BE_REMOVED;
				mi->m_flags |= DO_PKGRM;
				return (SUCCESS);
			}
		} else if (archmatch == ARCH_LESS_SPECIFIC &&
			    mi->m_shared != NULLPKG) {
			if (env_action == ADD_SVC_TO_ENV) {
				diff_rev(mi, mnew);
				return (ERR_DIFFREV);
			}
			if (mi->m_shared == SPOOLED_NOTDUP) {
				mi->m_action = TO_BE_REMOVED;
				spool_selected_arches(mi->m_pkgid);
				return (SUCCESS);
			}
		}
		if (mnew != NULL && mi->m_shared == NULLPKG) {
			if (mnew->m_status != REQUIRED)
				mnew->m_status = UNSELECTED;
			while ((mnew = next_inst(mnew)) != NULL)
				if (mnew->m_status != REQUIRED)
					mnew->m_status = UNSELECTED;
			return (SUCCESS);
		}
		if (mi->m_shared == SPOOLED_NOTDUP ||
			mi->m_shared == SPOOLED_DUP) {
			if (action == TO_BE_PRESERVED) {
				if (mnew != NULL) {
					if (mnew->m_status != REQUIRED)
						mnew->m_status = SELECTED;
					if (pkg_fullver_cmp(mnew, mi) ==
					    V_EQUAL_TO) {
						mi->m_action = TO_BE_PRESERVED;
						mnew->m_action =
							EXISTING_NO_ACTION;
						if (mi->m_instdir)
							mnew->m_instdir =
								xstrdup(
								mi->m_instdir);
						else
							mnew->m_instdir =
								NULL;
					} else {
#ifndef IGNORE_DIFF_REV
						if (env_action ==
						    ADD_SVC_TO_ENV) {
							diff_rev(mi, mnew);
							return (ERR_DIFFREV);
						}
#else
						if (env_action ==
						    ADD_SVC_TO_ENV) {
							mi->m_action =
							    TO_BE_PRESERVED;
							mnew->m_action =
							    EXISTING_NO_ACTION;
							if (mi->m_instdir)
								mnew->m_instdir=
								 xstrdup(
								 mi->m_instdir);
							else
								mnew->m_instdir=
								 NULL;
						}
#endif
						mi->m_action = TO_BE_REMOVED;
						mnew->m_action = TO_BE_SPOOLED;
					}
				} else
					mi->m_action = TO_BE_PRESERVED;
			} else {
				mi->m_action = TO_BE_REMOVED;
				if (mnew != NULL) {
					if (mnew->m_status != REQUIRED)
						mnew->m_status = SELECTED;
					mnew->m_action = TO_BE_SPOOLED;
				}
			}
		} else {
			if (mnew == NULL)
				mi->m_action = TO_BE_PRESERVED;
			else if (pkg_fullver_cmp(mnew, mi) == V_EQUAL_TO) {
				mi->m_action = TO_BE_PRESERVED;
				if (mnew->m_status != REQUIRED)
					mnew->m_status = SELECTED;
				mnew->m_action = EXISTING_NO_ACTION;
				if (mi->m_instdir)
					mnew->m_instdir =
						xstrdup(mi->m_instdir);
				else
					mnew->m_instdir = NULL;
			} else {
#ifndef IGNORE_DIFF_REV
				if (env_action == ADD_SVC_TO_ENV) {
					diff_rev(mi, mnew);
					return (ERR_DIFFREV);
				}
#else
				if (env_action == ADD_SVC_TO_ENV) {
					mi->m_action = TO_BE_PRESERVED;
					if (mnew->m_status != REQUIRED)
						mnew->m_status = SELECTED;
					mnew->m_action = EXISTING_NO_ACTION;
					if (mi->m_instdir)
						mnew->m_instdir =
							xstrdup(mi->m_instdir);
					else
						mnew->m_instdir = NULL;
				}
#endif
				mi->m_action = action;
				if (mnew->m_status != REQUIRED)
					mnew->m_status = SELECTED;
				if (mi->m_shared == NOTDUPLICATE) {
					mnew->m_action = TO_BE_PKGADDED;
					if (!(mi->m_pkg_hist &&
					    mi->m_pkg_hist->needs_pkgrm))
						mnew->m_flags |=
						    INSTANCE_ALREADY_PRESENT;
					/*
					 * Also we need to remove all of
					 * the duplicate instances of this
					 * package before actually adding
					 * the new package.
					 */
					set_instances_action(mi,
					    TO_BE_REMOVED);
				} else { /* it's a duplicate */
				/*
				 * hack here:  if a package changes from
				 * being a "usr" package to a "root" package,
				 * it will appear as a duplicate in the
				 * service's media structure, but needs to
				 * be spooled in the new media structure.
				 * This check will fail if we tried to
				 * upgrade a non-native service (that is,
				 * non-shared), but since we don't do that,
				 * this check is adequate to fix the bug.
				 */
					if (mnew->m_sunw_ptype == PTYPE_ROOT)
						mnew->m_action = TO_BE_SPOOLED;
					else
						mnew->m_action =
						    ADDED_BY_SHARED_ENV;
				}
			}
		}
		if (mnew != NULL)
			set_inst_dir(media_mod, mnew, mi);
	}
	return (SUCCESS);
}

/*
 * reprocess_package()
 * Parameters:
 *	mi
 * Return:
 *	none
 * Status:
 *	private
 */
static void
reprocess_package(Modinfo *mi)
{
	Modinfo  *mnew;
	Arch_match_type	archmatch;

	if (mi->m_pkgid && streq(mi->m_pkgid, swdebug_pkg_name))
		debug_bkpt();
	/*
	 * We only care about modules of type NOTDUPLICATE.  Spooled
	 * packages are always marked for removal.  Duplicate packages
	 * are not interesting because there is never an action
	 * associated with them.  We also don't care about packages
	 * with a to_be_removed flag set.  Since they are always
	 * removed, their status never changes.
	 */
	if (mi->m_shared != NOTDUPLICATE ||
	    (mi->m_pkg_hist && mi->m_pkg_hist->to_be_removed))
		return;

	/*
	 * See if package has a corresponding package in the
	 * new media structure.  If not, just return because
	 * there isn't any reason to reprocess it.
	 */
	mnew = find_new_package(g_newproduct, mi->m_pkgid, mi->m_arch,
			    &archmatch);

	if (mnew == NULL || mnew->m_shared == NULLPKG)
		return;

	if (mnew->m_status == UNSELECTED) {
		mi->m_action = TO_BE_REMOVED;
		mi->m_flags |= DO_PKGRM;
		mi->m_flags |= CONTENTS_GOING_AWAY;
		return;
	} else {
		if (pkg_fullver_cmp(mnew, mi) == V_EQUAL_TO) {
			mi->m_action = TO_BE_PRESERVED;
			mnew->m_action = EXISTING_NO_ACTION;
		} else {
			mi->m_action = TO_BE_REPLACED;
			if (!(mi->m_pkg_hist && mi->m_pkg_hist->needs_pkgrm))
				mnew->m_flags |= INSTANCE_ALREADY_PRESENT;
		}
		mi->m_flags &= ~DO_PKGRM;
		mi->m_flags &= ~CONTENTS_GOING_AWAY;
	}
}

/*
 * reset_action()
 *	Takes a module pointer to a media struct
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
reset_action(Module *mod)
{
	Module	*prodmod;

	prodmod = mod->sub;
	while (prodmod &&
	    (prodmod->type == PRODUCT || prodmod->type == NULLPRODUCT)) {
		walklist(prodmod->info.prod->p_packages, mark_action,
		    (caddr_t)NO_ACTION_DEFINED);
		prodmod = prodmod->next;
	}
	return;
}

/*
 * reset_cluster_action()
 *	Takes a module pointer to a media struct
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
reset_cluster_action(Module *mod)
{
	Module	*prodmod;

	prodmod = mod->sub;
	while (prodmod &&
		(prodmod->type == PRODUCT || prodmod->type == NULLPRODUCT)) {
		walklist(prodmod->info.prod->p_clusters,
		    _reset_cluster_action, (caddr_t)NO_ACTION_DEFINED);
		prodmod = prodmod->next;
	}
	return;
}

/*
 * mark_action()
 * Parameters:
 *	np	- node pointer
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
static int
mark_action(Node * np, caddr_t data)
{
	Modinfo *mi;

	mi = (Modinfo *)(np->data);
	mi->m_action = (Action)(data);
	while ((mi = next_inst(mi)) != NULL)
		mi->m_action = (Action)(data);

	return (0);
}

/*
 * _reset_cluster_action()
 * Parameters:
 *	np	- node pointer
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
static int
_reset_cluster_action(Node * np, caddr_t data)
{
	Module *mod;

	mod = (Module *)(np->data);
	mod->info.mod->m_action = (Action)(data);
	return (0);
}

/*
 * reset_instdir()
 * Parameters:
 *	np	- node poiner
 *	data	-
 * Return:
 *	0
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
reset_instdir(Node * np, caddr_t data)
{
	Modinfo *mi;

	mi = (Modinfo *)(np->data);
	if (mi->m_instdir) {
		free(mi->m_instdir);
		mi->m_instdir = NULL;
	}
	while ((mi = next_inst(mi)) != NULL) {
		if (mi->m_instdir) {
			free(mi->m_instdir);
			mi->m_instdir = NULL;
		}
	}
	return (0);
}

/*
 * generate string of the form:
 * /export/root/templates/<product>_<ver>/<pkg>_<pkgver>_<arch>
 */
static char *
genspooldir(Modinfo *mi)
{
	int	len;
	char	*cp;

	len = strlen(template_dir) +
			strlen(g_newproduct->p_name) +
			strlen(g_newproduct->p_version) +
			strlen(mi->m_pkgid) +
			strlen(mi->m_version) +
			strlen(mi->m_arch);
	len += 6;		/* miscelleneous delimeters in string */

	if (strchr(mi->m_arch, '.')) {
		cp = (char *) xmalloc((size_t)len);
		(void) sprintf(cp, "%s/%s_%s/%s_%s_%s", template_dir,
		    g_newproduct->p_name,
		    g_newproduct->p_version,
		    mi->m_pkgid, mi->m_version, mi->m_arch);
	} else {
		len += 5;   /* ".all" */
		cp = (char *) xmalloc((size_t)len);
		(void) sprintf(cp, "%s/%s_%s/%s_%s_%s.all", template_dir,
		    g_newproduct->p_name,
		    g_newproduct->p_version,
		    mi->m_pkgid, mi->m_version, mi->m_arch);
	}
	return (cp);
}

/*
 * set_alt_clsstat()
 * Parameters:
 *	selected	-
 *	mod		-
 * Return:
 *
 * Status:
 *	private
 */
static int
set_alt_clsstat(int selected, Module *mod)
{
int	toggles_needed;

	if (selected != PARTIAL && mod->info.mod->m_status != PARTIAL)
		if (selected != mod->info.mod->m_status)
			toggles_needed = 1;
		else
			toggles_needed = 0;
	else if (selected == PARTIAL || selected == UNSELECTED)
		if (mod->info.mod->m_status == SELECTED ||
			(mod->info.mod->m_status == PARTIAL &&
			partial_status(mod) == SELECTED))
			toggles_needed = 1;
		else
			toggles_needed = 0;
	else   /* selected == SELECTED and mod_info->status == PARTIAL */
		toggles_needed = 2;

	if (toggles_needed == 2) {
		toggle_module(mod);
		if (mod->info.mod->m_status == PARTIAL)
			toggle_module(mod);
	} else if (toggles_needed == 1)
		toggle_module(mod);

	return (toggles_needed);
}

/*
 * find_clients()
 * Parameters:
 *	none
 * Return:
 *
 * Status:
 *	private
 */
static struct client *
find_clients(void)
{
	char	file[MAXPATHLEN];
	char	line[MAXPATHLEN];
	char	rootdir[MAXPATHLEN];
	DIR	*dirp;
	struct dirent	*dp;
	struct client	*client_ptr, *client_head;
	char	cname[MAXHOSTNAMELEN];
	FILE	*fp;

	client_head = NULL;

	(void) sprintf(file, "%s/export/root", get_rootdir());
	if ((dirp = opendir(file)) == (DIR *)NULL)
		return ((struct client *)NULL);

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
					strcmp(dp->d_name, "..") == 0 ||
					strcmp(dp->d_name, "templates") == 0)
			continue;

		(void) sprintf(file,
		    "%s/export/root/%s/var/sadm/install/contents",
		    get_rootdir(), dp->d_name);
		if (path_is_readable(file) != SUCCESS)
			continue;
		client_ptr = (struct client *)
			xmalloc((size_t)sizeof (struct client));
		(void) strcpy(client_ptr->client_name, dp->d_name);
		client_ptr->client_root = xmalloc((size_t)
		    strlen("/export/root/") + strlen(dp->d_name) + 1);
		(void) strcpy(client_ptr->client_root, "/export/root/");
		(void) strcat(client_ptr->client_root, dp->d_name);
		client_ptr->next_client = client_head;
		client_head = client_ptr;
	}
	(void) closedir(dirp);

	(void) sprintf(file, "%s/etc/dfs/dfstab", get_rootdir());
	if ((fp = fopen(file, "r")) == NULL)
		return (client_head);

	/* check /etc/dfs/dfstab for any other clients */
	while (fgets(line, BUFSIZ, fp)) {
		if (sscanf(line, "share -F nfs -o rw=%*[^,],root=%s %s",
						cname, rootdir) != 2)
			continue;

		if (cname == NULL || rootdir == NULL)
			continue;

		for (client_ptr = client_head; client_ptr; client_ptr =
						client_ptr->next_client) {
			if (streq(cname, client_ptr->client_name))
				break;
		}
		if (client_ptr)
			continue;

		(void) sprintf(file, "%s/%s/var/sadm/install/contents",
					get_rootdir(), rootdir);
		if (path_is_readable(file) != SUCCESS)
			continue;

		client_ptr = (struct client *)
			xmalloc((size_t)sizeof (struct client));
		(void) strcpy(client_ptr->client_name, cname);
		client_ptr->client_root = xstrdup(rootdir);
		client_ptr->next_client = client_head;
		client_head = client_ptr;
	}

	(void) fclose(fp);
	return (client_head);
}

/*
 * mark_required_metacluster()
 * Parameters:
 *	prodmod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
mark_required_metacluster(Module * prodmod)
{
	Module *mod;

	mod = prodmod->sub;
	/*
	 * mod now points to the first cluster.  Find the
	 * required metacluster and mark in required in the
	 * new media structure.
	 */

	while (mod != NULL) {
		if (mod->type == METACLUSTER &&
			strcmp(mod->info.mod->m_pkgid,
			REQUIRED_METACLUSTER) == 0) {
			mark_required(mod);
			break;
		}
		mod = mod->next;
	}
	return;
}

/*
 * set_inst_dir()
 *	set the installation directory for the new package.
 *	media_mod:	the media module which heads the existing service or
 *			environment
 *	    mnew :	modinfo struct of new package,
 *	      mi :	modinfo struct of existing package that this package is
 *			replacing (may be NULL)
 * Parameters:
 *	media_mod	-
 *	mnew		-
 *	mi		-
 * Return:
 *	none
 */
static void
set_inst_dir(Module * media_mod, Modinfo * mnew, Modinfo * mi)
{
	char	buf[MAXPATHLEN];
	char	isabuf[ARCH_LENGTH];
	char	*cp;

	if (mnew->m_action == EXISTING_NO_ACTION)
		return;
	else if (mnew->m_action == TO_BE_SPOOLED)
		mnew->m_instdir = genspooldir(mnew);
	else if (mnew->m_action == TO_BE_PKGADDED ||
	    mnew->m_action == ADDED_BY_SHARED_ENV) {
		if (media_mod->info.media->med_type == INSTALLED) {
			if (mi && mi->m_basedir != NULL &&
			    !streq(mi->m_basedir, mnew->m_basedir)) {
				/* use basedir of existing package */
				mnew->m_instdir = xstrdup(mi->m_basedir);
			} else {
				/* use basedir instead */
				mnew->m_instdir = NULL;
			}
		} else {		/* it's a service */
			strcpy(isabuf, mnew->m_arch);
			cp = strchr(isabuf, '.');
			if (cp != NULL)
				*cp = '\0';
			if (media_mod->info.media->med_flags &
			    SPLIT_FROM_SERVER) {
				/*
				 * NOTICE: There is a bit of magic
				 * that is going on here. for post-KBI
				 * services there are no KVM type
				 * packages, but there is a small
				 * transition period were they may
				 * exist. So to fix this problem the
				 * use of the is_KBI_service routine
				 * is used to show if this is a KBI
				 * service or not. For post-KBI
				 * services there is no need for the
				 * special /export/exec/kvm directory,
				 * so the instdir should just be the
				 * base dir.
				 */
				if ((mnew->m_sunw_ptype == PTYPE_KVM &&
				    (!is_KBI_service(g_newproduct))) &&
				    strcmp(get_default_arch(),
					mnew->m_arch) != 0) {
					sprintf(buf,
					    "/export/exec/kvm/%s_%s_%s",
					    g_newproduct->p_name,
					    g_newproduct->p_version,
					    mnew->m_arch);
					if (strcmp(mnew->m_basedir, "/") != 0)
						(void) strcat(buf,
						    mnew->m_basedir);
					mnew->m_instdir = xstrdup(buf);
				} else if (mnew->m_sunw_ptype == PTYPE_KVM &&
				    is_KBI_service(g_newproduct) &&
				    !supports_arch(get_default_arch(),
				    isabuf)) {
					sprintf(buf,
					    "/export/exec/%s_%s_%s.all",
					    g_newproduct->p_name,
					    g_newproduct->p_version,
					    isabuf);
					if (strcmp(mnew->m_basedir, "/") != 0)
						(void) strcat(buf,
						    mnew->m_basedir);
					mnew->m_instdir = xstrdup(buf);
				} else if ((mnew->m_sunw_ptype == PTYPE_USR ||
				    mnew->m_sunw_ptype == PTYPE_OW) &&
				    !supports_arch(get_default_arch(),
					mnew->m_arch)) {
					sprintf(buf,
					    "/export/exec/%s_%s_%s",
					    g_newproduct->p_name,
					    g_newproduct->p_version,
					    mnew->m_expand_arch);
					if (strcmp(mnew->m_basedir, "/") != 0)
						(void) strcat(buf,
						    mnew->m_basedir);
					mnew->m_instdir = xstrdup(buf);
				} else  /* use basedir */
					mnew->m_instdir = NULL;
			} else {
				if (mnew->m_sunw_ptype == PTYPE_KVM &&
				    !is_KBI_service(g_newproduct))
					sprintf(buf, "/usr.kvm_%s",
					    mnew->m_arch);
				else if (mnew->m_sunw_ptype == PTYPE_KVM &&
				    is_KBI_service(g_newproduct))
					sprintf(buf, "/usr_%s.all",
					    isabuf);
				else if (mnew->m_sunw_ptype == PTYPE_USR ||
				    mnew->m_sunw_ptype == PTYPE_OW)
					sprintf(buf, "/usr_%s.all",
					    mnew->m_arch);
				else   /* opt or shared */
					sprintf(buf, "/export/%s_%s",
					    g_newproduct->p_name,
					    g_newproduct->p_version);
				if (strcmp(mnew->m_basedir, "/") != 0)
					(void) strcat(buf, mnew->m_basedir);
				mnew->m_instdir = xstrdup(buf);

			}
		}
	}
}

/*
 * cluster_match()
 * Parameters:
 *	cls_list	-
 *	media_mod	-
 * Return:
 *
 * Status:
 *	private
 */
static int
cluster_match(char *cls_list, Module *media_mod)
{
	char	*cp, *p;
	Module	*mod;

	if (cls_list == NULL)
		return (0);

	for (mod = media_mod->sub->sub; mod != NULL; mod = mod->next)
		if (mod->type == METACLUSTER)
			break;
	if (mod == NULL)
		return (0);

	cp = cls_list;
	while ((p = split_name(&cp)) != NULL)
		if (strcmp(p, mod->info.mod->m_pkgid) == 0)
			return (1);
	return (0);
}

/*
 * walktree_for_l10n()
 * Parameters:
 *	mod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
walktree_for_l10n(Module *mod)
{
	Module *child;

	if (mod->type == PACKAGE && mod->info.mod->m_l10n)
		mark_locales(mod, mod->info.mod->m_status);
	child = mod->sub;
	while (child) {
		walktree_for_l10n(child);
		child = child->next;
	}
}

/*
 * spool_selected_arches()
 * Parameters:
 *	id	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
spool_selected_arches(char * id)
{
	Node	*node;
	Modinfo	*mi;

	node = findnode(g_newproduct->p_packages, id);
	if (node) {
		mi = (Modinfo *)(node->data);
		if (mi->m_shared != NULLPKG && is_arch_selected(mi->m_arch)) {
			mi->m_status = REQUIRED;
			mi->m_action = TO_BE_SPOOLED;
			mi->m_instdir = genspooldir(mi);
		}
		while ((node = mi->m_instances) != NULL) {
			mi = (Modinfo *)(node->data);
			if (mi->m_shared != NULLPKG &&
				is_arch_selected(mi->m_arch)) {
				mi->m_status = REQUIRED;
				mi->m_action = TO_BE_SPOOLED;
				mi->m_instdir = genspooldir(mi);
			}
		}
	}
	return;
}

/*
 * is_arch_selected()
 * Parameters:
 *	arch	-
 * Return:
 *
 * Status:
 *	private
 */
static int
is_arch_selected(char * arch)
{
	Arch *ap;
	int ret;

	for (ap = g_newproduct->p_arches; ap != NULL; ap = ap->a_next)
		if (ap->a_selected) {
			ret = compatible_arch(arch, ap->a_arch);
			if (ret == ARCH_MATCH || ret == ARCH_MORE_SPECIFIC)
				return (1);
		}
	return (0);
}

/*
 * is_arch_supported()
 * Parameters:
 *	arch	-
 * Return:
 *
 * Status:
 *	private
 */
static int
is_arch_supported(char * arch)
{
	Arch *ap;
	int ret;

	for (ap = g_newproduct->p_arches; ap != NULL; ap = ap->a_next) {
		ret = compatible_arch(arch, ap->a_arch);
		if (ret == ARCH_MATCH || ret == ARCH_MORE_SPECIFIC)
				return (1);
	}
	return (0);
}

static void
update_patch_status(Product *prod)
{
	struct patch *p;
	struct patchpkg *ppkg;

	for (p = prod->p_patches; p != NULL; p = p->next) {
		for (ppkg = p->patchpkgs; ppkg != NULL; ppkg = ppkg->next) {
			if (ppkg->pkgmod->m_patchof) {
				if (ppkg->pkgmod->m_patchof->m_action ==
				    TO_BE_PRESERVED)
					break;
			} else
				if (ppkg->pkgmod->m_action == TO_BE_PRESERVED)
					break;
		}
		/*
		 *  If any of the patch packages are for packages that
		 *  are being preserved, the patch as a whole will not
		 *  be removed.
		 */
		if (ppkg != NULL)
			p->removed = 0;
		else
			p->removed = 1;
	}
}

static void
diff_rev(Modinfo *mi, Modinfo *mnew)
{
	if (g_sw_diffrev)
		free_diff_rev(g_sw_diffrev);
	g_sw_diffrev = (SW_diffrev *) xcalloc((size_t) sizeof (SW_diffrev));
	g_sw_diffrev->sw_diffrev_pkg = xstrdup(mi->m_pkgid);
	g_sw_diffrev->sw_diffrev_arch = xstrdup(mi->m_arch);
	g_sw_diffrev->sw_diffrev_curver = xstrdup(mi->m_version);
	if (mnew && mnew->m_version)
		g_sw_diffrev->sw_diffrev_newver = xstrdup(mnew->m_version);
	else
		g_sw_diffrev->sw_diffrev_newver = xstrdup("");
}

/*
 * set_instances_action()
 *	This private function runs all of the instances of a package and
 *	sets the instance's action code. Primarly this function is used to
 *	remove extra instances of a package.
 *
 * Parameters:
 *	np	- node poiner
 *	data	- action code
 * Return:
 *	0
 * Status:
 *	private
 */
static void
set_instances_action(Modinfo *mi, Action data)
{
	Modinfo *imi;

	imi = mi;
	while ((imi = next_inst(imi)) != NULL)
		if (mi->m_arch != NULL && imi->m_arch != NULL &&
		    imi->m_shared != SPOOLED_NOTDUP &&
		    streq(mi->m_arch, imi->m_arch)) {
			imi->m_action = data;
			imi->m_flags |= DO_PKGRM;
		}
}
