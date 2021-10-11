#ifndef lint
#ident	 "@(#)soft_do_upgrade.c 1.1 96/05/15 SMI"
#endif
/*
 * Copyright (c) 1994-1996, by Sun Microsystems, Inc.  All rights reserved.
 */

#include <assert.h>
#include <fcntl.h>
#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "spmicommon_api.h"
#include "spmisoft_lib.h"

/* Local Statics and Constants */

static Module		*u_newmedia = NULL;
static Product		*u_newproduct = NULL;
static Module		*u_newproductmod = NULL;

/* Local Globals */

int	g_online_upgrade = 0;
char	*pkg_hist_path = NULL;

/* Public Function Prototypes */

void		set_pkg_hist_file(char *);
void		set_onlineupgrade_mode(void);
int		upgrade_all_envs(void);
int		nonnative_upgrade(StringList *);

/* Local Function Prototypes */

static void	upgrade_env(Module *, Action);
static void	upgrade_client_env(Module *);
static void	set_initial_selected_arches(Module *, Module *);
static void	initialize_upgrade(void);
static int	setup_for_local_upgrade(void);
static int	setup_for_nonnative_upgrade(StringList *);
static int	set_upgrade_actions(void);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

void
set_pkg_hist_file(char *path)
{
	pkg_hist_path = path;
}

void
set_onlineupgrade_mode(void)
{
	g_online_upgrade = 1;
}

int
upgrade_all_envs(void)
{
	int			status;

	initialize_upgrade();
	status = setup_for_local_upgrade();
	if (status != 0)
		return (status);

	return(set_upgrade_actions());
}

int
nonnative_upgrade(StringList *client_list)
{
	int			status;

	initialize_upgrade();
	status = setup_for_nonnative_upgrade(client_list);
	if (status != 0)
		return (status);

	return(set_upgrade_actions());
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static void
set_initial_selected_arches(Module *mod, Module *mod_upg_to)
{
	Arch *ap;

	for (ap = mod->sub->info.prod->p_arches; ap != NULL; ap = ap->a_next) {
		if (ap->a_loaded) {
			(void) select_arch(u_newproductmod, ap->a_arch);
			if (mod_upg_to && mod_upg_to != mod &&
			    mod_upg_to->sub) {
				add_arch(mod_upg_to->sub, ap->a_arch);
				select_arch(mod_upg_to->sub, ap->a_arch);
			}
		}
	}
	return;
}

static void
upgrade_env(Module *mod, Action action)
{
	update_module_actions(mod, u_newproductmod, action, ENV_TO_BE_UPGRADED);
}

static void
upgrade_client_env(Module *mod)
{
	update_module_actions(mod, u_newproductmod, TO_BE_REPLACED,
	    ENV_TO_BE_UPGRADED);
	unreq_nonroot(u_newproductmod);
	set_primary_arch(u_newproductmod);
}

static void
initialize_upgrade(void)
{
	Module		*mod;
	char		path[MAXPATHLEN];

	set_is_upgrade(1);

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
		    mod->info.media->med_type != INSTALLED &&
		    mod->sub->type == PRODUCT &&
		    strcmp(mod->sub->info.prod->p_name, "Solaris") == 0)
			u_newmedia = mod;
	}
	u_newproductmod = u_newmedia->sub;
	u_newproduct = u_newproductmod->info.prod;

	if (!pkg_hist_path)
		(void) sprintf(path, "%s/.pkghistory",
		    u_newmedia->sub->info.prod->p_pkgdir);
	else
		(void) sprintf(path, "%s/.pkghistory", pkg_hist_path);

	read_pkg_history_file(path);

	if (!pkg_hist_path)
		(void) sprintf(path, "%s/.clusterhistory",
		    u_newmedia->sub->info.prod->p_pkgdir);
	else
		(void) sprintf(path, "%s/.clusterhistory", pkg_hist_path);

	read_cls_history_file(path);
}

static int
setup_for_local_upgrade(void)
{
	Module		*mod, *new_svc_mod, *svc_to_be_upg;
	char		media_version[MAXPATHLEN];
	char		media_service[MAXPATHLEN];
	Arch		*ap;

	/*
	 *  Determine whether medium supports the native architecture.
	 */

	if (!media_supports_arch(u_newproduct, get_default_arch()))
		return (ERR_NONNATIVE_MEDIA);

	/* mark the local media as BASIS_OF_UPGRADE */
	mod = get_localmedia();
	assert(mod != NULL);
	mod->info.media->med_flags |= BASIS_OF_UPGRADE;

	/*
	 * Determine whether there is a (shared) server of the native
	 * architecture and Solaris version.  If not, we're done.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next)
		if (mod->info.media->med_type == INSTALLED_SVC &&
		    mod->info.media->med_flags & SPLIT_FROM_SERVER)
			break;

	if (mod == NULL)
		return (0);
	svc_to_be_upg = mod;

	/*
	 * Now we need to determine whether the service is of the
	 * same Solaris version as the version of Solaris on the medium.
	 */

	(void) sprintf(media_version, "%s_%s",
	    u_newproduct->p_name, u_newproduct->p_version);
	(void) sprintf(media_service, "/export/%s", media_version);
			
	/*
	 * if the old and new versions differ only in the rev. field,
	 * don't need to create new service.
	 */
	if (strcmp(media_version, svc_to_be_upg->info.media->med_volume) == 0) {
		svc_to_be_upg->info.media->med_flags |=
			BASIS_OF_UPGRADE | BUILT_FROM_UPGRADE;
		svc_to_be_upg->info.media->med_upg_to = svc_to_be_upg;
		svc_to_be_upg->info.media->med_upg_from = svc_to_be_upg;
		set_svc_modstate(svc_to_be_upg->info.media, SVC_UNCHANGED);

	} else {  /* we're upgrading to a different full release */

		/*
		 *  Don't allow the upgrade if the new release already exists
		 *  as a service on this machine.  Yes, this prevents some
		 *  reasonable upgrade scenarios, but the implementation
		 *  complexity is prohibitive for this release.
		 */
		if ((new_svc_mod = find_service_media(media_version)) != NULL)
			return (ERR_SVC_ALREADY_EXISTS);

		new_svc_mod = add_new_service(media_service);
		/*
		 * kludge here:  add_new_service sets p_pkgdir to
		 * the rootdir "/export/Solaris_2.x".  Don't know why,
		 * but it make be risky to change that at this point,
		 * since initial install may depend on that.  So just
		 * free that bogus value here and set p_pkgdir to
		 * the actual location of the packages.
		 */
		if (new_svc_mod->sub->info.prod->p_pkgdir)
			free(new_svc_mod->sub->info.prod->p_pkgdir);
		new_svc_mod->sub->info.prod->p_pkgdir = xmalloc(strlen(
		    media_service) + strlen("/var/sadm") + 1);
		(void) strcpy(new_svc_mod->sub->info.prod->p_pkgdir, media_service);
		(void) strcat(new_svc_mod->sub->info.prod->p_pkgdir, "/var/sadm");
		new_svc_mod->sub->info.prod->p_rootdir = xstrdup(media_service);
		new_svc_mod->info.media->med_flags |= BUILT_FROM_UPGRADE;
		new_svc_mod->info.media->med_upg_from = svc_to_be_upg;
		svc_to_be_upg->info.media->med_flags |= BASIS_OF_UPGRADE;
		svc_to_be_upg->info.media->med_upg_to = new_svc_mod;
		set_svc_modstate(svc_to_be_upg->info.media, SVC_TO_BE_REMOVED);
		set_svc_modstate(new_svc_mod->info.media, NEW_SERVICE);
	}

	/* find all client environments that are to be upgraded */
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED)
			continue;
		if (strcmp(mod->info.media->med_dir, "/") == 0)
			continue;  /* it's the localmedia */

		/*
		 *  It's a diskless client.
		 *
		 *  Diskless clients get upgraded if they are running
		 *  the same version of Solaris as the service being
		 *  upgraded and if the installation medium supports
		 *  their particular architecture and implementation.
		 */

#ifdef SELECTIVE_CLIENT_UPGRADE
		/*
		 *  This code is commented out for now.  Upgrade will
		 *  always upgrade all clients that CAN be upgraded
		 *  now.   This isn't ideal, but otherwise we'd have
		 *  to add too much new interface stuff to the
		 *  non-native upgrade command.  And then, the non-native
		 *  upgrade would have more flexibility than the base
		 *  upgrade.
		 */
		if (strcmp(mod->info.media->med_volume,
		    svc_to_be_upg->info.media->med_volume) != 0)
			continue;	/* versions don't match */
#endif

		if (!mod->sub || !mod->sub->info.prod ||
		    !mod->sub->info.prod->p_arches)
			continue;

		for (ap = mod->sub->info.prod->p_arches; ap; ap = ap->a_next) {
			if (!ap->a_loaded)
				continue;
			if (media_supports_arch(u_newproduct, ap->a_arch)) {
				mod->info.media->med_flags |=
				    BASIS_OF_UPGRADE;
				break;
			}
		}
	}

	return (0);
}

static int
setup_for_nonnative_upgrade(StringList *client_list)
{
	Module		*mod, *shared_service, *svc_to_be_upg, *medmod;
	Module		*localmedia;
	char		media_version[MAXPATHLEN];
	char		media_service[MAXPATHLEN];
	Arch		*ap;

	/*
	 *  On a non-native upgrade, we upgrade as much as possible of
	 *  the shared service.  If client_list is NULL, we upgrade any
	 *  clients that are (1) using the service that is being upgraded
	 *  or which are not supported by any currently-installed service
	 *  and (2) can be upgraded using the medium.  If client_list is
	 *  non-null, we upgrade any client on the list that can be
	 *  upgraded by the installation medium (regardless of whether
	 *  its service was upgraded or not).  Clients on the list that
	 *  can't be upgraded are left untouched (a warning message will
	 *  be printed).
	 */

	/*
	 * The above comment is how this will eventually work.  For now,
	 * support for an explicit client_list is being removed.  It
	 * can be found in the SCCS history of the file.
	 */

	/* find the shared service, if there is one */
	shared_service = NULL;
	svc_to_be_upg= NULL;
	for (mod = get_media_head(); mod != NULL; mod = mod->next)
		if (mod->info.media->med_type == INSTALLED_SVC &&
		    mod->info.media->med_flags & SPLIT_FROM_SERVER) {
			shared_service = mod;
			break;
		}

	if (shared_service) {
		/*
		 * Now we need to determine whether the service is of
		 * the same Solaris version as the version of Solaris
		 * on the medium.  If it isn't, the shared service
		 * can't be upgraded using this medium.
		 */

		(void) sprintf(media_version, "%s_%s",
		    u_newproduct->p_name, u_newproduct->p_version);
		(void) sprintf(media_service, "/export/%s", media_version);
			
		if (strcmp(media_version,
		    shared_service->info.media->med_volume) == 0) {
			svc_to_be_upg = shared_service;
			svc_to_be_upg->info.media->med_flags |=
				BASIS_OF_UPGRADE | BUILT_FROM_UPGRADE;
			svc_to_be_upg->info.media->med_upg_to = svc_to_be_upg;
			svc_to_be_upg->info.media->med_upg_from = svc_to_be_upg;
			set_svc_modstate(svc_to_be_upg->info.media,
			    SVC_UNCHANGED);
			localmedia = get_localmedia();
			localmedia->info.media->med_flags |= BASIS_OF_UPGRADE;
		}
	}

	/* find all client environments that are to be upgraded */
	if (client_list == NULL) {
		if (svc_to_be_upg == NULL) {  /* nothing to do */
			return (ERR_NOTHING_TO_UPGRADE);
		}
		/*
		 *  Find all clients that need to be upgraded and
		 *  which can be upgraded.
		 */
		for (mod = get_media_head(); mod != NULL; mod = mod->next) {
			if (mod->info.media->med_type != INSTALLED)
				continue;
			if (strcmp(mod->info.media->med_dir, "/") == 0)
				continue;  /* it's the localmedia */
	
			/*
			 *  It's a diskless client.
			 *
			 *  Diskless clients get upgraded if they are running
			 *  the same version of Solaris as the service being
			 *  upgraded, or if their service isn't supported at
			 *  all and if the installation medium supports
			 *  their particular architecture and implementation.
			 */

			if ((medmod = find_service_media(
			    mod->info.media->med_volume)) != NULL &&
			    medmod->sub) {
				for (ap = mod->sub->info.prod->p_arches; ap;
				    ap = ap->a_next) {
					if (!ap->a_loaded)
						continue;
					if (fullarch_is_loaded(
					    medmod->sub->info.prod, ap->a_arch))
						break;
				}
				if (ap != NULL)	{ /* client is supported */
					if (strcmp(mod->info.media->med_volume,
					    svc_to_be_upg->info.media->
					    med_volume) != 0)
						/* versions don't match */
						continue;
				}
			}

			if (!mod->sub || !mod->sub->info.prod ||
			    !mod->sub->info.prod->p_arches)
				continue;

			for (ap = mod->sub->info.prod->p_arches; ap;
			    ap = ap->a_next) {
				if (!ap->a_loaded)
					continue;
				if (media_supports_arch(u_newproduct,
				    ap->a_arch)) {
					mod->info.media->med_flags |=
					    BASIS_OF_UPGRADE;
					break;
				}
			}
		}
	}
	return (0);
}

static int
set_upgrade_actions(void)
{
	Module			*mod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_flags & SVC_TO_BE_REMOVED &&
		    !(mod->info.media->med_flags & BASIS_OF_UPGRADE)) {
			mark_removed(mod); 
			continue;
		}
		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE))
			continue;
		if (mod->info.media->med_type == INSTALLED) {
			if (strcmp(mod->info.media->med_dir, "/") == 0) {
				(void) load_view(u_newproductmod, mod);
				clear_view(u_newproductmod);
				set_initial_selected_arches(mod, NULL);
				upgrade_env(mod, TO_BE_REPLACED);
			} else {	/* it's a client */
				(void) load_view(u_newproductmod, mod);
				clear_view(u_newproductmod);
				set_initial_selected_arches(mod, NULL);
				upgrade_client_env(mod);
			}
		} else if (mod->info.media->med_type == INSTALLED_SVC) {
			(void) load_view(u_newproductmod,
			    mod->info.media->med_upg_to);
			clear_view(u_newproductmod);
			set_initial_selected_arches(mod,
			    mod->info.media->med_upg_to);
			upgrade_env(mod, TO_BE_REPLACED);
		}
	}
	if (has_view(u_newproductmod, get_localmedia()) == SUCCESS)
		(void) load_view(u_newproductmod, get_localmedia());
	return (0);
}
