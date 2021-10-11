#ifndef lint
#ident	 "@(#)do_upgrade.c 1.46 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1994-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "sw_lib.h"
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

/* Local Statics and Constants */

static Module		*u_newmedia = NULL;
static Product		*u_newproduct = NULL;
static Module		*u_newproductmod = NULL;

/* Local Globals */

int	g_debug = 0;
int	g_online_upgrade = 0;
int	g_skipmodsearch = 0;
char	*g_dumpfile;
char	*pkg_hist_path = NULL;
char	*dump_default = "/tmp/upgrade_dump";
int	upg_fs_freespace[N_LOCAL_FS] = {
	/* root */		 5,
	/* usr */		 0,
	/* usr_own_fs */	 -1,
	/* opt */		 0,
	/* swap */		 0, /* not applicable */
	/* var */		 5,
	/* export/exec */	 5,
	/* export/swap */	 5,
	/* export/root */	 5,
	/* export/home */	 5,
	/* export */		 5
};

#define	REQUIRED_METACLUSTER "SUNWCreq"

/* Externals */

extern int	profile_upgrade;

/* Public Function Prototypes */

void		swi_set_debug(char *);
void		swi_set_skip_mod_search(void);
void		swi_set_pkg_hist_file(char *);
void		swi_set_onlineupgrade_mode(void);
int		swi_upgrade_all_envs(void);
int		swi_do_upgrade(void);
int		swi_do_product_upgrade(Module *);
int		swi_do_find_modified(void);
int		swi_do_final_space_check(void);
void		swi_do_write_upgrade_script(void);
int		swi_nonnative_upgrade(StringList *);

/* Library Function Prototypes */
void		init_upg_fs_overhead(void);

/* Local Function Prototypes */

static void	upgrade_env(Module *, Action);
static void	upgrade_dataless(Module *);
static void	upgrade_client_env(Module *);
static void	add_savedfile_space(Module *);
static int	count_file_space(Node *, caddr_t);
static void	_count_file_space(Modinfo *, Product *);
static void	set_initial_selected_arches(Module *, Module *);
static int	find_modified_all(Module *);
static void	do_add_savedfile_space(Module *);
static void	log_spacechk_failure(int);
static void	initialize_upgrade(void);
static int	setup_for_local_upgrade(void);
static int	setup_for_nonnative_upgrade(StringList *);
static int	set_upgrade_actions(void);

extern int sp_err_code;
extern int sp_err_subcode;
extern char *sp_err_path;

static char *new_logpath = "/var/sadm/system/logs/upgrade_log";
static char *old_logpath = "/var/sadm/install_data/upgrade_log";

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */


void
swi_set_debug(char *dumpfile)
{
	g_debug = 1;
	if (dumpfile)
		g_dumpfile = dumpfile;
	else
		g_dumpfile = dump_default;
}

void
swi_set_skip_mod_search(void)
{
	g_skipmodsearch = 1;
}

void
swi_set_pkg_hist_file(char *path)
{
	pkg_hist_path = path;
}

void
swi_set_onlineupgrade_mode(void)
{
	g_online_upgrade = 1;
}

void
init_upg_fs_overhead(void)
{
	set_upg_fs_overhead(upg_fs_freespace);
}

int
swi_upgrade_all_envs(void)
{
	return (local_upgrade());
}

int
swi_local_upgrade(void)
{
	int			status;

	initialize_upgrade();
	status = setup_for_local_upgrade();
	if (status != 0)
		return (status);

	return(set_upgrade_actions());
}

int
swi_nonnative_upgrade(StringList *client_list)
{
	int			status;

	initialize_upgrade();
	status = setup_for_nonnative_upgrade(client_list);
	if (status != 0)
		return (status);

	return(set_upgrade_actions());
}

int
swi_do_upgrade()
{
	return (do_product_upgrade(u_newproductmod));
}

int
swi_do_find_modified()
{
	return (find_modified_all(u_newproductmod));
}

int
swi_do_final_space_check()
{
	do_add_savedfile_space(u_newproductmod);
	return (final_space_chk());
}

void
swi_do_write_upgrade_script() {
	set_umount_script_fcn(gen_mount_script, gen_installboot);
	(void) write_script(u_newproductmod);
}

int
swi_do_product_upgrade(Module *prodmod)
{
	int	status;
	char	cmd[MAXPATHLEN];
	char	spacereport[MAXPATHLEN];
	char	logfile[MAXPATHLEN];


	set_final_upgrade_mode(1);

	if (is_KBI_service(prodmod->info.prod)) {
		/* Setup path the the space_required file */
		(void) sprintf(spacereport,
		    "%s/var/sadm/system/data/upgrade_space_required",
		    get_rootdir());
		/* make new directories if need be */
		if (! is_new_var_sadm("/")) {
			char	dir[MAXPATHLEN];
			char	tdir[MAXPATHLEN];

			/*
			 * Since this root is pre var/sadm change make the
			 * directories.
			 */
			sprintf(dir, "%s/var/sadm/system", get_rootdir());
			mkdir(dir, (mode_t)00755);
			sprintf(tdir, "%s/logs", dir);
			mkdir(tdir, (mode_t)00755);
			sprintf(tdir, "%s/data", dir);
			mkdir(tdir, (mode_t)00755);
			sprintf(tdir, "%s/admin", dir);
			mkdir(tdir, (mode_t)00755);
			sprintf(tdir, "%s/admin/services", dir);
			mkdir(tdir, (mode_t)00755);
		}
	} else
		(void) sprintf(spacereport,
		    "%s/var/sadm/install_data/upgrade_space_required",
		    get_rootdir());

	/* do the search for modified files */

	if (!g_skipmodsearch) {
		printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Finding modified files.  This may take several minutes.\n"));
		status = find_modified_all(prodmod);
	}

	if (g_debug)
		(void) dumptree(g_dumpfile);

	/* do final space checking */
	printf(dgettext("SUNW_INSTALL_SWLIB",
	    "\rCalculating space requirements.\n"));
	do_add_savedfile_space(prodmod);
	if (!g_skipmodsearch) {
		status = final_space_chk();
		printf("\r");
	} else
		status = 0;
	if (g_debug && !g_skipmodsearch) {
		print_final_results(spacereport);
	}
	if (status) {
		if (status == SP_ERR_NOT_ENOUGH_SPACE) {
			print_final_results(spacereport);
			printf(dgettext("SUNW_INSTALL_SWLIB",
			    "Insufficient space for upgrade.\n"));
			printf(dgettext("SUNW_INSTALL_SWLIB",
			    "Space required in each file system is:\n\n"));
			(void) sprintf(cmd, "cat %s", spacereport);
			(void) system(cmd);
			if (profile_upgrade) {
				(void) printf(dgettext("SUNW_INSTALL_SWLIB",
				    "\nAfter rebooting, the space report displayed above will be in:\n"));
				if (is_KBI_service(prodmod->info.prod))
					(void) printf("/var/sadm/system/data/upgrade_space_required.\n\n");
				else
					(void) printf("/var/sadm/install_data/upgrade_space_required.\n\n");
			} else
				(void) printf("\n");
		} else {
			log_spacechk_failure(status);
		}
		if (!g_debug) {
			set_final_upgrade_mode(0);
			return (status);
		}
	} else
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Space check complete.\n"));

	if (g_debug)
		(void) printf("Writing script\n");

	set_umount_script_fcn(gen_mount_script, gen_installboot);
	(void) write_script(prodmod);

	if (!g_debug) {
		if (is_KBI_service(prodmod->info.prod)) {
			sprintf(logfile, "%s%s", get_rootdir(), new_logpath);
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/system/admin/upgrade_script "
			    "%s 2>&1 | tee %s", get_rootdir(),
			    ((strcmp(get_rootdir(), "") == 0) ? "/" :
				get_rootdir()),
			    logfile);
		} else {
			sprintf(logfile, "%s%s", get_rootdir(), old_logpath);
			(void) sprintf(cmd,
			    "/bin/sh %s/var/sadm/install_data/upgrade_script "
			    "%s 2>&1 | tee %s", get_rootdir(),
			    ((strcmp(get_rootdir(), "") == 0) ? "/" :
				get_rootdir()),
			    logfile);
		}
		status = system(cmd);
		chmod(logfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

		/*
		 *  Call the driver-upgrade script, if present.
		 *  It doesn't take the rootdir as an argument (though
		 *  it should).
		 */
		if (access("/tmp/diskette_rc.d/inst9.sh", X_OK) == 0) {
			(void) system("/sbin/sh /tmp/diskette_rc.d/inst9.sh");
		}
		set_final_upgrade_mode(0);
		return (status);
	}
	set_final_upgrade_mode(0);
	return (0);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static int
find_modified_all(Module *prodmod)
{
	Module	*mod;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE) ||
		    (mod->info.media->med_flags & MODIFIED_FILES_FOUND))
			continue;
		/*
		 * don't scan for modified files if this
		 * service is actually the server's own
		 * service.  It's already been scanned.
		 */
		if (mod->info.media->med_type == INSTALLED_SVC &&
		    mod->info.media->med_flags & SPLIT_FROM_SERVER)
			continue;

		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			(void) load_view(prodmod, mod);
			find_modified(mod);
			mod->info.media->med_flags |= MODIFIED_FILES_FOUND;
		}
	}

	return (0);
}

static void
do_add_savedfile_space(Module *prodmod)
{
	Module	*mod;

	/* clear any previous saved file stats */
	init_save_files();

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE))
			continue;
		/*
		 * don't scan for modified files if this
		 * service is actually the server's own
		 * service.  It's already been scanned.
		 */
		if (mod->info.media->med_type == INSTALLED_SVC &&
		    mod->info.media->med_flags & SPLIT_FROM_SERVER)
			continue;

		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) {
			(void) load_view(prodmod, mod);
			add_savedfile_space(mod);
		}
	}
}

static void
add_savedfile_space(Module * mod)
{
	(void) walklist(mod->sub->info.prod->p_packages, count_file_space,
			(caddr_t)(mod->sub->info.prod));
	return;
}

static int
count_file_space(Node *node, caddr_t arg)
{
	Modinfo	*mi;
	Product	*prod;

	mi = (Modinfo *)(node->data);
	/*LINTED [alignment ok]*/
	prod = (Product *)arg;
	_count_file_space(mi, prod);
	while ((mi = next_inst(mi)) != NULL)
		_count_file_space(mi, prod);
	return (0);
}

static void
_count_file_space(Modinfo *mi, Product *prod)
{
	struct filediff *fdp;
	char	file[MAXPATHLEN];

	fdp = mi->m_filediff;
	while (fdp != NULL) {
		/*
		 * A file needs to be saved if its contents has changed
		 * and at least one of these two conditions is satisfied:
		 *  1)  the replacing package is selected or required and
		 *	TO_BE_PKGADDED
		 *  2) if the action is not TO_BE_PRESERVED and
		 *     if the contents of the package are not going away
		 */
		if ((fdp->diff_flags & DIFF_CONTENTS) &&

		/* 1st condition */
		    ((fdp->replacing_pkg != NULL &&
		    (fdp->replacing_pkg->m_status == SELECTED ||
		    fdp->replacing_pkg->m_status == REQUIRED) &&
		    fdp->replacing_pkg->m_action == TO_BE_PKGADDED) ||

		/* 2nd condition */
		    (mi->m_action != TO_BE_PRESERVED &&
			!(mi->m_flags & CONTENTS_GOING_AWAY)))) {

			strcpy(file, prod->p_rootdir);
			strcat(file, fdp->component_path);
			save_files(file);
		}
		fdp = fdp->diff_next;
	}
}

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
upgrade_dataless(Module *mod)
{
	update_module_actions(mod, u_newproductmod, TO_BE_REPLACED,
	    ENV_TO_BE_UPGRADED);
	unreq_nonroot(u_newproductmod);
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
log_spacechk_failure(int code)
{
	char *nullstring = "NULL";

	/*
	 *  If sp_err_path is null, make sure it points to a valid
	 *  string so that none of these printfs coredump.
	 */
	if (sp_err_path == NULL)
		sp_err_path = nullstring;

	switch (code) {
	case SP_ERR_STAT:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_STATVFS:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Statvfs failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_GETMNTENT:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Getmntent failed: errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_MALLOC:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Malloc failed.\n"));
		break;

	case SP_ERR_PATH_INVAL:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Internal error: invalid path: %s\n"), sp_err_path);
		break;

	case SP_ERR_CHROOT:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Failure doing chroot.\n"));
		break;

	case SP_ERR_NOSLICES:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "No upgradable slices found.\n"));
		break;

	case SP_ERR_POPEN:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Popen failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "error = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_OPEN:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Open failed: %s\n"), sp_err_path);
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "errno = %d\n"), sp_err_subcode);
		break;

	case SP_ERR_PARAM_INVAL:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Internal error: invalid parameter.\n"));
		break;

	case SP_ERR_STAB_CREATE:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Space check failed: couldn't create file-system table.\n"));
		if (sp_err_code != SP_ERR_STAB_CREATE) {
			(void) printf(dgettext("SUNW_INSTALL_SWLIB",
			    "Reason for failure:\n"));
			log_spacechk_failure(sp_err_code);
		}
		break;

	case SP_ERR_CORRUPT_CONTENTS:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Space check failed: package database is corrupted.\n"));
		break;

	case SP_ERR_CORRUPT_PKGMAP:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Space check failed: package's pkgmap is not in the correct format.\n"));
		break;

	case SP_ERR_CORRUPT_SPACEFILE:
		(void) printf(dgettext("SUNW_INSTALL_SWLIB",
		    "Space check failed: package's spacefile is not in the correct format.\n"));
		break;

	}
}

static void
initialize_upgrade(void)
{
	Module		*mod;
	char		path[MAXPATHLEN];

	init_upg_fs_overhead();

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
		strcpy(new_svc_mod->sub->info.prod->p_pkgdir, media_service);
		strcat(new_svc_mod->sub->info.prod->p_pkgdir, "/var/sadm");
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
				if (is_dataless_machine())						
					upgrade_dataless(mod);
				else
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
