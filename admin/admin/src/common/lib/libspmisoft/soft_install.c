#ifndef lint
#pragma ident "@(#)soft_install.c 1.8 96/06/25 SMI"
#endif lint
/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "spmisoft_lib.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <locale.h>
#include <stdlib.h>

extern Module	*head_ptr;

char	swlib_err_buf[1024];

/* Public Function Prototypes */

Module *	swi_load_installed(char *, int);
Modinfo *	swi_next_patch(Modinfo *);
Modinfo *	swi_next_inst(Modinfo *);

/* Library Functino Prototypes */

void		set_cluster_status(Module *);
Modinfo *	find_owning_inst(char *, Modinfo *);
int		is_new_var_sadm(char *);
char	*	INST_RELEASE_read_path(char *);
char	*	services_read_path(char *);
char	*	CLUSTER_read_path(char *);
char	*	clustertoc_read_path(char *);
void 		split_svr_svc(Module *, Module *);
Module  *	add_new_service(char *);

/* Local Function Prototypes */

static void 	remove_extra_clusters(Module *);
static void 	create_null_product(Module *);
static void 	load_spooled_pkgs(Module *);
static int 	load_installed_product(Module *, int);
static int 	remove_service_pkgs(Node *, caddr_t);
static int	load_patch(Node *, caddr_t);
static void	_load_patch(Modinfo *, Module *);
static void 	split_service(Product *);
static void 	set_loaded(Module *);
static int 	is_dataless(Module *);
static int 	isloadedpackage(char *, char *);
static StringList *load_hostname(char *);
static void	remove_instances(Modinfo *);
static void	make_pkg_null(Modinfo *);
static void	free_and_clear(char **);
static void	delete_instance(Modinfo *, Modinfo *);
static int	load_softinfo(Module *);
static int	set_order_100(Node *, caddr_t);
static void	free_soft_desc(struct softinfo_desc *);
static void	create_patchpkg_entry(Modinfo *, Module *, char *);

/* Local Variables */
/* The following variables are used in the *_read_path routines */
static char	*INST_RELEASE_tmp_path = (char *)NULL;
static char	*CLUSTER_tmp_path = (char *)NULL;
static char	*services_tmp_path = (char *)NULL;
static char	*clustertoc_tmp_path = (char *)NULL;

/* local globals */

int profile_upgrade = 0;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * load_installed()
 *	Loads list of installed packages (from /var/sadm/{pkg,install}).
 * Parameters:
 *	rootdir	-
 *	service	-
 * Return:
 *
 * Status:
 *	public
 */
Module *
swi_load_installed(char * rootdir, int service)
{
	Module	*new;
	Module	*mod;
	Module	*new_svc;
	Node	*node;
	Modinfo *mi;
	Arch	*ap, **app;
	char	*os = NULL;
	char	*version = NULL;
	char	*rev = NULL;
	char	 file[MAXPATHLEN];
	char	 os_version[MAXPATHLEN];
	FILE	*fp;
	DIR	*dirp;
	struct dirent	*dp;
	struct stat	statres;
	char	buf[BUFSIZ + 1];
	int	ret;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("load_installed");
#endif

	if (GetSimulation(SIM_SYSSOFT)) {
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Can't load installed media if simulating disks"));
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return ((Module *) NULL);
	}

	/*
	 * Set the default machine type to STANDALONE before we begin processing.
	 * If the machine turns out to be different type it will be set below.
	 */

	/* ---- This is a hack until Laura gets the real solution figured
	 * for a machine being upgraded
	 */

	set_machinetype(MT_SERVER);

	/*
	 * Allocate media structure.  If we've seen this
	 * piece of media before, its contents list will
	 * be non-NULL and we don't have to do anything
	 * this time.
	 */
	new = add_media(rootdir);

	/* do nothing if previously loaded */
	if (new->sub != (Module *)NULL)
		return (new);

	/* set up media including installed os and version */
	new->info.media->med_type = INSTALLED;

	/* Load in the hostname(s) associated with this media */
	(void) sprintf(file, "%s/%s/etc", get_rootdir(), rootdir);
	new->info.media->med_hostname = load_hostname(file);

	(void) sprintf(file, "%s", INST_RELEASE_read_path(rootdir));
	if ((fp = fopen(file, "r")) == (FILE *) NULL) {
		unload_media(new);
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), file);
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return ((Module *) NULL);
	}

	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) -1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		else if (strncmp(buf, "OS=", 3) == 0)
			os = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "VERSION=", 8) == 0)
			version = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "REV=", 4) == 0)
			rev = xstrdup(get_value(buf, '='));
	}
	(void) fclose(fp);

	if (!os || !version) {
		unload_media(new);
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), file);
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return ((Module *)NULL);
	}

	(void) sprintf(os_version, "%s_%s", os, version);
	new->info.media->med_volume = xstrdup(os_version);

	create_null_product(new);		/* set up NULLPRODUCT */
	new->sub->info.prod->p_name = xstrdup(os);
	new->sub->info.prod->p_version = xstrdup(version);
	free(os);
	free(version);

	/* read all locale descriptions */
	read_locale_table(new);

	if (load_installed_product(new->sub, service) != SUCCESS) {
		unload_media(new);
		return ((Module *)NULL);
	}

	if (rev != NULL)
		new->sub->info.prod->p_rev = xstrdup(rev);
	else
		new->sub->info.prod->p_rev = xstrdup("0");
	free(rev);

	/* create/load services if this is a server */
	if (service == FALSE) {
		/*
		 * there are two place the services will be stored.
		 * it will either be var/sadm/softinfo or
		 * var/sadm/system/admin/services, services_read_path will
		 * determine the correct location.
		 */
		(void) sprintf(file, "%s", services_read_path(rootdir));
		if ((dirp = opendir(file)) == (DIR *)NULL) {
			unload_media(new);
			(void) sprintf(swlib_err_buf,
			    dgettext("SUNW_INSTALL_SWLIB",
			    "Stat failed: %s\n"), file);
			if (profile_upgrade)
				(void) printf("%s\n", swlib_err_buf);
			return ((Module *) NULL);
		}

		while ((dp = readdir(dirp)) != (struct dirent *)0) {
			if (strcmp(dp->d_name, ".") == 0 ||
					strcmp(dp->d_name, "..") == 0 ||
					strcmp(dp->d_name, "INST_RELEASE") == 0)
				continue;

			(void) sprintf(file, "%s/%s/export/%s",
						get_rootdir(), rootdir,
						dp->d_name);

			if (path_is_readable(file) != SUCCESS)
				continue;

			new->info.media->med_machine = MT_SERVER;

			/*
			 * Ok, we know that the machine is a server so let the library
			 * know so that if we are doing a DSR upgrade the ResourceObject
			 * logic will work correctly.
			 */

			set_machinetype(MT_SERVER);

			ret = lstat(file, &statres);
			(void) sprintf(file, "%s/export/%s", rootdir,
			    dp->d_name);
			if (ret == 0 &&
			    ((strcmp(dp->d_name, new->info.media->med_volume)
			    != 0) || S_ISLNK(statres.st_mode))) {
				new_svc = load_installed(file, TRUE);
				if (new_svc == NULL) {
					unload_media(new);
					return ((Module *) NULL);
				}
			} else {
				new_svc = duplicate_media(new);
				free(new_svc->info.media->med_dir);
				new_svc->info.media->med_dir = xstrdup(file);
				split_svr_svc(new, new_svc);
				new_svc->info.media->med_machine = MT_SERVICE;
				new_svc->info.media->med_type = INSTALLED_SVC;
			}
			(void) strcpy(os_version,
			    new_svc->info.media->med_volume);
			version = strchr(os_version, '_');
			if (version) {
				*version = '\0';
				version++;
				new_svc->sub->info.prod->p_version =
				    xstrdup(version);
				new_svc->sub->info.prod->p_name =
				    xstrdup(os_version);
			}
			/*
			 * For every service add the patches on to the
			 * local environment.
			 */
			(void) walklist(new_svc->sub->info.prod->p_packages,
				    load_patch, (caddr_t)new->sub);
			if (load_softinfo(new_svc) != SUCCESS)
				sort_packages(new_svc->sub, "");
		}
		(void) closedir(dirp);
		/*
		 * Generate the patches for the local environment
		 */
		(void) walklist(new->sub->info.prod->p_packages,
		    load_patch, (caddr_t)new->sub);

		if (new->info.media->med_machine == MT_STANDALONE)
			new->info.media->med_machine = is_dataless(new);
	} else {
		new->info.media->med_machine = MT_SERVICE;
		new->info.media->med_type = INSTALLED_SVC;
	}

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if ((mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC) && mod->sub) {
			node = findnode(mod->sub->info.prod->p_packages,
			    "SUNWcar");
			if (node == NULL)
				continue;
			app = &(mod->sub->info.prod->p_arches);
			while (*app != (Arch *)NULL) {
				ap = *app;
				if (!ap->a_selected)
					continue;
				for (mi = (Modinfo *)(node->data); mi != NULL;
				    mi = next_inst(mi))
					if (mi->m_shared != NULLPKG &&
					    strcmp(ap->a_arch, mi->m_arch) == 0)
						break;
				if (mi == NULL) {
					*app = ap->a_next;
					ap->a_next = NULL;
					free_arch(ap);
				} else
					app = &(ap->a_next);
			}
		}
	}

	return (new);
}

/*
 * next_patch()
 *	Return a pointer to the Modinfo structuer associated with the
 *	next_patch field of the Modinfo structure 'mod'.
 * Parameters:
 *	mod	   - pointer to Modinfo structure
 * Return:
 *	NULL	   - no instances in the patch chain
 *	Modinfo *  - pointer to next patch Modinfo structure
 */
Modinfo *
swi_next_patch(Modinfo * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("next_patch");
#endif
	if (mod->m_next_patch == NULL)
		return (NULL);
	return ((Modinfo *) mod->m_next_patch->data);
}

/*
 * next_inst()
 *	Return a pointer to the Modinfo structure associated with the instances
 *	field of the Modinfo structure 'mod'.
 * Parameters:
 *	mod	   - pointer to Modinfo structure
 * Return:
 *	NULL	   - no instances in the chain
 *	Modinfo *  - pointer to next instance Modinfo structure
 */
Modinfo *
swi_next_inst(Modinfo * mod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("next_inst");
#endif
	if (mod->m_instances == NULL)
		return (NULL);

	return ((Modinfo *) mod->m_instances->data);
}

/*
 * add_new_service()
 *	Adds a new service specified by 'rootdir' to the media chain, creating
 *	a null product structure under the media structure, and returning a
 *	pointer to the module associated with the new service.
 * Parameters:
 *	rootdir	  - pathname of directory which serves as the root of the
 *		    designated service
 * Return:
 *	Module *  - pointer to new media module for the service
 * Status:
 *	public
 */
Module *
add_new_service(char * rootdir)
{
	Module	*new;
	char    *cp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("add_new_service");
#endif

	/*
	 * Allocate media structure.  If we've seen this piece of media before,
	 * its contents list will be non-NULL and we don't have to do anything
	 * this time.
	 */
	new = add_media(rootdir);

	/* do nothing if previously loaded */
	if (new->sub != (Module *) NULL)
		return (new);

	/* set up media including installed os and version */
	new->info.media->med_type = INSTALLED;

	cp = strrchr(rootdir, '/');
	cp++;
	new->info.media->med_volume = xstrdup(cp);

	create_null_product(new);		/* set up NULLPRODUCT */

	new->info.media->med_machine = MT_SERVICE;
	new->info.media->med_type = INSTALLED_SVC;

	return (new);
}

/*
 * set_cluster_status()
 *	Sets the module status (SELECTED, REQUIRED, UNSELECTED, PARTIAL)
 *	associated with the all clusters and metaclusters under the 'mod'
 *	sub-tree to match that of 'mod'. This function is recursive, so that
 *	the status of every sub-cluster is also set by this call. Cluster or
 *	metacluster status is set based on the status of the packages which
 *	are contained within it.
 * Parameters:
 *	mod	- pointer to cluster module
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_cluster_status(Module * mod)
{
	Module *i;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_cluster_status");
#endif

	for (i = mod; i != NULL; i = i->next) {
		if (i->sub)
			set_cluster_status(i->sub);
		if (i->type == CLUSTER || i->type == METACLUSTER)
			i->info.mod->m_status = mod_status(i);
	}
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * find_owning_inst()
 *
 * Parameters:
 *	inst	-
 *	mod	-
 * Return:
 * Status:
 *	semi-private (internal library use only)
 */
Modinfo *
find_owning_inst(char * inst, Modinfo * mod)
{
	Modinfo	*i, *k;

	for (i = mod; i != NULL; i = next_inst(i)) {
		for (k = i; k != NULL; k = next_patch(k)) {
			if (k->m_pkginst && strcmp(k->m_pkginst, inst) == 0)
					return (i);
		}
	}
	return (NULL);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * load_spooled_pkgs()
 *
 * Parameters:
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
load_spooled_pkgs(Module * prod)
{
	struct dirent	*dp, *sub_dp;
	DIR		*dirp, *sub_dirp;
	char		 tmp[MAXPATHLEN];
	char		 tmp2[MAXPATHLEN];
	char		 tmp3[MAXPATHLEN];
	char		*save_pkgdir;

	(void) sprintf(tmp, "%s/export/root/templates/%s", get_rootdir(),
	    prod->parent->info.media->med_volume);
	dirp = opendir(tmp);
	if (dirp == (DIR *)0)
		return;

	save_pkgdir = prod->info.prod->p_pkgdir;
	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
					strcmp(dp->d_name, "..") == 0)
			continue;

		(void) sprintf(tmp2, "%s/%s", tmp, dp->d_name);
		(void) sprintf(tmp3, "/export/root/templates/%s/%s",
				prod->parent->info.media->med_volume,
				dp->d_name);
		sub_dirp = opendir(tmp2);
		if (sub_dirp == (DIR *)0)
			continue;
		while ((sub_dp = readdir(sub_dirp)) != (struct dirent *)0) {
			if (strcmp(sub_dp->d_name, ".") == 0 ||
					strcmp(sub_dp->d_name, "..") == 0)
				continue;
			if (isloadedpackage(tmp3, sub_dp->d_name)) {
				prod->info.prod->p_pkgdir = tmp3;
				load_pkginfo(prod, sub_dp->d_name,
								SPOOLED_NOTDUP);
			}
		}
		(void) closedir(sub_dirp);
	}
	(void) closedir(dirp);
	prod->info.prod->p_pkgdir = save_pkgdir;

	return;
}

/*
 * is_dataless()
 * Parameters:
 *	med	- media module pointer
 * Return:
 *
 * Status:
 *	private
 */
static int
is_dataless(Module * med)
{
	DIR		*dirp;
	struct dirent	*dp;
	char		file[MAXPATHLEN];
	int		client_type = MT_DATALESS;

	if (strcmp(med->info.media->med_dir, "/") != 0)
		client_type = MT_DISKLESS;

	(void) sprintf(file, "%s/%s/usr",
			get_rootdir(), med->info.media->med_dir);

	if ((dirp = opendir(file)) == (DIR *)NULL)
		return (client_type);

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
				strcmp(dp->d_name, "..") == 0)
			continue;
		(void) closedir(dirp);
		return (MT_STANDALONE);
	}
	(void) closedir(dirp);
	return (client_type);
}

/*
 * isloadedpackage()
 *	Determine whether something is a package of not
 * Parameters:
 *	dir	-
 *	name	-
 * Return:
 *	0	-
 *	1	-
 * Status:
 *	private
 */
static int
isloadedpackage(char * dir, char * name)
{
	char	pkginfo[MAXPATHLEN];

	(void) sprintf(pkginfo, "%s/%s/%s/pkginfo", get_rootdir(), dir, name);
	if (path_is_readable(pkginfo) == FAILURE)
		return (0);

	return (1);
}

/*
 * create_null_product()
 *	Reads the products available from the specified media location.
 *	If no location is specified, the current media location (see
 *	get_current_media) is used.
 * Parameters:
 *	mod	-
 * Returns:
 *	none
 * Status:
 *	private
 * Note:
 *	Creates a NULL product structure
 */
static void
create_null_product(Module * mod)
{
	Product	*prod = (Product *)0;
	Module	**mpp;
	Module	*media;

	/* Get correct module pointer */
	if (mod == (Module *)NULL)
		media = get_current_media();
	else
		media = mod;

	if (media == (Module *)NULL)
		return;		/* No media */

	/* set up to create new module structures and add them to the list */
	if (!media->info.media->med_dir)
		return;		/* Not mounted */
	prod = (Product *)xcalloc(sizeof (Product));
	prod->p_pkgdir = xstrdup(media->info.media->med_dir);
	prod->p_current_view = prod;
	prod->p_categories = (Module *) xcalloc(sizeof (Module));
	prod->p_categories->type = CATEGORY;
	prod->p_categories->info.cat = (Category *) xcalloc(sizeof (Category));
	prod->p_categories->info.cat->cat_name =
				xstrdup((char *)dgettext("SUNW_INSTALL_SWLIB",
							"All Software"));
	mpp = &(media->sub);
	*mpp = (Module *)xcalloc(sizeof (Module));
	(*mpp)->info.prod = prod;
	(*mpp)->type = NULLPRODUCT;
	(*mpp)->sub = (Module *)NULL;
	(*mpp)->prev = NULL;
	(*mpp)->head = media->sub;
	(*mpp)->next = (Module *)NULL;
	(*mpp)->parent = media;
	prod->p_categories->parent = *mpp;

	return;
}

/*
 * remove_extra_clusters()
 * Parameters:
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
remove_extra_clusters(Module * prod)
{
	Module		*mod, *meta = NULL;
	Module		*next_mod;
	Module		*head;
	char		tmp[MAXPATHLEN];
	char		buf[BUFSIZ+1];
	FILE		*fp;
	Node		*np, *np2;
	Modinfo		*info;
	Module		tmpmod;

	if (prod->sub == NULL || prod->info.prod->p_clusters == NULL)
		return;

	for (np = (Node*)prod->info.prod->p_packages->list->next;
		np != prod->info.prod->p_packages->list; np = np->next) {
		info = (Modinfo*)np->data;
		if (info->m_shared != NULLPKG)
			info->m_status = SELECTED;
	}

	if (prod->info.prod->p_rootdir != NULL)
		(void) sprintf(tmp, "%s",
		    CLUSTER_read_path(prod->info.prod->p_rootdir));
	else
		(void) sprintf(tmp, "%s", CLUSTER_read_path("/"));

	if ((fp = fopen(tmp, "r")) == (FILE *) NULL)
		return;
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) -1] = '\0';
		if (strncmp(buf, "CLUSTER=", 8) == 0) {
			for (next_mod = mod = prod->sub; mod != NULL;
			    mod = next_mod) {
				next_mod = mod->next;
				if (mod->type != METACLUSTER)
					continue;
				if (strcmp(mod->info.mod->m_pkgid,
						get_value(buf, '=')) == 0) {
					mark_module(mod, SELECTED);
					meta = mod;
					meta->head = prod->sub;
				} else {
					if (mod->prev != NULL) {
						mod->prev->next = mod->next;
					} else
						prod->sub = mod->next;

					if (mod->next != NULL)
						mod->next->prev = mod->prev;
					np = findnode(
						prod->info.prod->p_clusters,
						mod->info.mod->m_pkgid);
					delnode(np);
				}
			}
		}
	}
	(void) fclose(fp);

	head = ((Module *)
		((Node *)prod->info.prod->p_clusters->list->next)->data)->head;
	for (np = (Node*)prod->info.prod->p_clusters->list->next;
		/*LINTED [var set before used]*/
		np != prod->info.prod->p_clusters->list; np = np2) {
		np2 = np->next;
		mod = (Module*)np->data;
		if (mod->info.mod->m_status == SELECTED ||
						mod_status(mod) == SELECTED)
			continue;
		if (mod->type == CLUSTER) {
			if (mod->prev != NULL)
				mod->prev->next = mod->next;
			else
				head = mod->next;
			if (mod->next != NULL)
				mod->next->prev = mod->prev;
			delnode(np);
		}
	}

	for (np = (Node*)prod->info.prod->p_packages->list->next;
		np != prod->info.prod->p_packages->list; np = np2) {
		np2 = np->next;
		info = (Modinfo*)np->data;
		if ((info->m_shared == NULLPKG) &&
				(info->m_status != SELECTED)) {
			delnode(np);
		} else
			info->m_status = UNSELECTED;
	}

	if (meta != NULL)
		mark_module(meta, UNSELECTED);

	for (np = (Node*)prod->info.prod->p_clusters->list->next;
		np != prod->info.prod->p_clusters->list; np = np->next) {
		mod = (Module*)np->data;
		if (mod->info.mod->m_status != SELECTED)
			toggle_module(mod);
		if (mod->type != METACLUSTER) {
			mod->head = head;
			np2 = np;
		}
	}

	if (meta == (Module *) NULL) {
		free_list(prod->info.prod->p_clusters);
		prod->info.prod->p_clusters = NULL;
		for (np = (Node*)prod->info.prod->p_packages->list->next;
				np != prod->info.prod->p_packages->list;
				np = np->next)
			((Modinfo*)np->data)->m_status = UNSELECTED;

		promote_packages(prod, &tmpmod, NULL);
		prod->sub = tmpmod.next;
	} else {
		if (np2 == (Node *) NULL) {
			promote_packages(prod, &tmpmod, NULL);
		} else {
			promote_packages(prod, (Module *)np2->data,
						((Module *)np2->data)->head);
		}
	}
}

/*
 * split_svr_svc()
 *	Removes the components of the media tree not associated
 *	with the specifed server or service.
 * Parameters:
 *	media	-
 *	svc	-
 * Return:
 *	none
 * Status:
 *	private
 * Implementation Notes:  Here are the various kinds of packages and
 *   how they are represented in the the server's environment and the
 *   the service's environment, after the split:
 *
 *	Type of Pkg		Representation in	Representation in
 *				Server			Service
 * ------------------------------------------------------------------------
 *	/opt package or		NOTDUPLICATE		make NULLPKG  (*)
 *	other package
 *	(anything not in
 *	/export/exec and
 *	of type PTYPE_UNKNOWN)
 *
 *	native kvm pkgs		NOTDUPLICATE		DUPLICATE
 *	(native ISA and
 *	karch)
 *
 *	non-native kvm, in	NOTDUPLICATE		DUPLICATE
 *	/usr/platform (KBI)
 *
 *	non-native KVM, ins	make NULLPKG (**)	NOTDUPLICATE (*)
 *	/export/exec/kvm
 *	(pre-KBI type)
 *
 *	usr or ow type,		NOTDUPLICATE		DUPLICATE
 *	native ISA (not in
 *	/export/exec)
 *
 *	usr or ow type,		make NULLPKG (**)	NOTDUPLICATE (*)
 *	non-native ISA (in
 *	/export/exec)
 *
 *	root, spooled		make NULLPKG (**)	SPOOLED_NOTDUP (*)
 *
 *	root, not spooled,	NOTDUPLICATE		make NULLPKG (*)
 *	native
 *
 *	root, not spooled,	NOTDUPLICATE		make NULLPKG (*)
 *	non-native, basedir
 *	not in /export/exec
 *	(a disk bootable from
 *	multiple machine
 *
 *	root, not spooled,	make NULLPKG (**)	NOTDUPLICATE (*)
 *	non-native, basedir
 *	in  /export/exec (a
 *	strange case, shouldn't
 *	happen, but we need
 *	to cover all the
 *	possibilities)
 */
void
split_svr_svc(Module * media, Module * svc)
{
	Module *i;
	Arch	*ap, *arch, *next;

	svc->info.media->med_flags =
			svc->info.media->med_flags | SPLIT_FROM_SERVER;
	/* cleanup service */
	for (i = svc->sub; i != NULL; i = i->next)
		split_service(i->info.prod);
	/* cleanup server */
	for (i = media->sub; i != NULL; i = i->next) {
		/* remove multiple architecture packages from server */
		if (media->info.media->med_machine == MT_SERVER) {
			(void) walklist(i->info.prod->p_packages,
			    remove_service_pkgs, (caddr_t)i);
			for (arch = ap = i->info.prod->p_arches; ap != NULL;
			/*LINTED [var set before used]*/
								ap = next) {
				if (strcmp(ap->a_arch,
				    get_default_arch()) != 0) {
					if (arch == ap) {
						next = arch = ap->a_next;
						i->info.prod->p_arches = arch;
					} else {
						next = arch->a_next =
							ap->a_next;
					}
					ap->a_next = NULL;
					free_arch(ap);
				} else {
					if (arch != ap)
						ap = arch->a_next;
					next = arch->a_next;
				}
			}
		}
	}

	return;

}

/*
 * load_installed_product()
 *	Loads the product specified into a linked module list.
 * Parameters:
 *	prod	-
 * Return:
 * Status:
 *	private
 */
static int
load_installed_product(Module * prod, int service)
{
	struct dirent	*dp;
	DIR		*dirp;
	FILE		*fp;
	char		 tmp[MAXPATHLEN];
	char		 buf[BUFSIZ + 1];
	Module		*mod;
	int		 err;

	if (prod->info.prod->p_packages != NULL ||
					prod->info.prod->p_sw_4x != NULL)
		return (ERR_PREVLOAD);

	/* Use packageinfo files and/or read in 4.x software */
	if (prod->info.prod->p_pkgdir == NULL)
		return (ERR_INVALID);
	prod->info.prod->p_rootdir = prod->info.prod->p_pkgdir;

	/* read .platform file, if one is present */
	load_installed_platforms(prod);

	(void) sprintf(tmp, "%s/var/sadm/pkg", prod->info.prod->p_pkgdir);
	prod->info.prod->p_pkgdir = xstrdup(tmp);
	(void) sprintf(tmp, "%s/%s", get_rootdir(), prod->info.prod->p_pkgdir);
	dirp = opendir(tmp);
	if (dirp == (DIR *)0) {
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), tmp);
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return (ERR_NODIR);
	}

	prod->info.prod->p_packages = getlist();
	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;
		if (isloadedpackage(prod->info.prod->p_pkgdir, dp->d_name))
			load_pkginfo(prod, dp->d_name, INSTALLED);
	}
	(void) closedir(dirp);
	if (strcmp(prod->info.prod->p_rootdir, "/") == 0 || service == TRUE)
		load_spooled_pkgs(prod);
	localize_packages(prod);

	if (prod->info.prod->p_rootdir != NULL)
		(void) sprintf(tmp, "%s",
		    clustertoc_read_path(prod->info.prod->p_rootdir));
	else
		(void) sprintf(tmp, "%s", clustertoc_read_path("/"));

	if ((err = load_clusters(prod, tmp)) != SUCCESS) {
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), tmp);
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return (err);
	}
	remove_extra_clusters(prod);
	set_loaded(prod);

	/* set inital status on all clusters based on their contents */
	set_cluster_status(prod);
	if (prod->info.prod->p_rootdir != NULL)
		(void) sprintf(tmp, "%s",
		    CLUSTER_read_path(prod->info.prod->p_rootdir));
	else
		(void) sprintf(tmp, "%s", CLUSTER_read_path("/"));

	if ((fp = fopen(tmp, "r")) == (FILE *) NULL) {
		(void) sprintf(swlib_err_buf, dgettext("SUNW_INSTALL_SWLIB",
		    "Stat failed: %s\n"), tmp);
		if (profile_upgrade)
			(void) printf("%s\n", swlib_err_buf);
		return (ERR_NOFILE);
	}
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) -1] = '\0';
		if (strncmp(buf, "CLUSTER=", 8) == 0) {
			for (mod = prod->sub; mod != NULL; mod = mod->next) {
				if (mod->type != METACLUSTER)
					continue;
				if (strcmp(mod->info.mod->m_pkgid,
						get_value(buf, '=')) == 0)
					mod->info.mod->m_status = LOADED;
				else
					mod->info.mod->m_status = UNSELECTED;
			}
		}
	}
	(void) fclose(fp);

	/* Sort locale strings */
	sort_locales(prod);

	if (prod->sub == NULL)
		return (ERR_NOPROD);

	return (SUCCESS);
}

/*
 * remove_service_pkgs()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
remove_service_pkgs(Node * np, caddr_t data)
{
	Modinfo	*i, *info;
	Node	*nxt_node;

	info = (Modinfo*)np->data;
	i = info;
	while (i) {
		nxt_node = i->m_instances;
		/*
		 *  There are three conditions that can cause a package
		 *  to be removed from the server's environment structure:
		 *  (1) it's a spooled root package, or (2) it's installed in
		 *  /export/exec, or (3) it's a kvm, usr, or ow package that
		 *  is non-native. (the last test is not redundant because
		 *  some packages have no basedir at all, such as SUNWowrqd.)
		 */
		if ((i->m_sunw_ptype == PTYPE_ROOT &&
		    (i->m_shared == SPOOLED_DUP ||
		     i->m_shared == SPOOLED_NOTDUP)) ||

		    (i->m_basedir &&
		    strneq(i->m_basedir, "/export/exec/", 13)) ||

		    ((i->m_sunw_ptype == PTYPE_KVM ||
		    i->m_sunw_ptype == PTYPE_USR ||
		    i->m_sunw_ptype == PTYPE_OW) &&
		    !isa_of_arch_is(i->m_arch, get_default_inst()) &&
		    !streq(i->m_arch, "all"))) {
			if (i == info)
				make_pkg_null(i);
			else
				delete_instance(info, i);
		}
		if (nxt_node)
			i = nxt_node->data;
		else
			i = NULL;
	}
	return (SUCCESS);
}

static void
delete_instance(Modinfo *head_mi, Modinfo *delete_mi)
{
	Node	*np, **npp;

	npp = &(head_mi->m_instances);
	while (*npp != NULL) {
		if ((*npp)->data == delete_mi) {
			np = *npp;
			*npp = delete_mi->m_instances;
			delete_mi->m_instances = NULL;
			delnode(np);
			break;
		}
		npp = &(((Modinfo *)(*npp)->data)->m_instances);
	}
}

static void
remove_instances(Modinfo *mi)
{
	Modinfo	*j;
	Node	*tmp_np;

	for (j = next_inst(mi); j != NULL; j = next_inst(mi)) {
		tmp_np = mi->m_instances;
		mi->m_instances = j->m_instances;
		j->m_instances = NULL;
		delnode(tmp_np);
	}
}

static void
make_pkg_null(Modinfo *mi)
{
	mi->m_shared = NULLPKG;
	mi->m_status = UNSELECTED;
	free_patch_instances(mi);
	free_and_clear(&(mi->m_pkg_dir));
	free_and_clear(&(mi->m_name));
	free_and_clear(&(mi->m_version));
	free_and_clear(&(mi->m_prodname));
	free_and_clear(&(mi->m_prodvers));
	free_and_clear(&(mi->m_desc));
	free_and_clear(&(mi->m_category));
	free_and_clear(&(mi->m_basedir));
	free_and_clear(&(mi->m_instdir));
	free_and_clear(&(mi->m_locale));
	free_and_clear(&(mi->m_l10n_pkglist));
	StringListFree(mi->m_loc_strlist);
	mi->m_loc_strlist = NULL;
	free_pkgs_lclzd(mi->m_pkgs_lclzd);
	mi->m_pkgs_lclzd = NULL;
	free_file(mi->m_icon);
	mi->m_icon = NULL;
	if (mi->m_text != NULL)
		free_file(*(mi->m_text));
	mi->m_text = NULL;
	if (mi->m_demo != NULL)
		free_file(*(mi->m_demo));
	mi->m_demo = NULL;
	free_file(mi->m_install);
	mi->m_install = NULL;
}

static void
free_and_clear(char **cpp)
{
	if (*cpp != NULL) {
		free(*cpp);
		*cpp = NULL;
	}
}

/*
 * load_patch()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
load_patch(Node * np, caddr_t data)
{
	Modinfo	*mi, *mip;
	Module	*prodmod;

	/*LINTED [alignment ok]*/
	prodmod = (Module *)data;
	mi = (Modinfo*)np->data;
	for (mip = mi; mip != NULL; mip = next_patch(mip))
		_load_patch(mip, prodmod);
	while ((mi = next_inst(mi)) != NULL)
		for (mip = mi; mip != NULL; mip = next_patch(mip))
			_load_patch(mip, prodmod);
	return (SUCCESS);
}

/*
 * _load_patch()
 * Parameters:
 *	mi
 *	prodmod
 * Return:
 *	void
 * Status:
 *	private
 */
static void
_load_patch(Modinfo *mi, Module *prodmod)
{
	struct patch_num *pnum;
	char	patch_num_string[256];

	if ((mi->m_patchid == NULL && mi->m_newarch_patches == NULL) ||
	    mi->m_shared == NULLPKG || mi->m_shared == DUPLICATE ||
	    mi->m_shared == SPOOLED_DUP)
		return;

	/*
	 *  We only process the m_patchid if the m_newarch_patches is
	 *  NULL because the m_patchid field is redundant if there are
	 *  any new-architecture patches applied to the packages.
	 */
	if (mi->m_newarch_patches == NULL && mi->m_patchid)
		create_patchpkg_entry(mi, prodmod, mi->m_patchid);
	for (pnum = mi->m_newarch_patches; pnum != NULL; pnum = pnum->next) {
		(void) sprintf(patch_num_string, "%s-%s", pnum->patch_num_id,
		    pnum->patch_num_rev_string);
		create_patchpkg_entry(mi, prodmod, patch_num_string);
	}
}

static void
create_patchpkg_entry(Modinfo *mi, Module *prodmod, char *patchid)
{
	struct patch *pat;
	struct patchpkg *ppkg;
	/*
	 *  If there is already a patch on the patch list with this
	 *  patchid, find it.
	 */
	for (pat = prodmod->info.prod->p_patches; pat != NULL;
	    pat = pat->next)
		if ((strcmp(pat->patchid, patchid)) == 0)
			break;

	if (pat == NULL) {
		/* allocate a struct for this patch */
		pat = (struct patch *) xcalloc((size_t)
		    sizeof (struct patch));
		pat->patchid = xstrdup(patchid);

		/* queue it to front of p_patches */
		pat->next = prodmod->info.prod->p_patches;
		prodmod->info.prod->p_patches = pat;
	}

	ppkg = (struct patchpkg *) xmalloc((size_t) sizeof (struct patchpkg));
	ppkg->pkgmod = mi;

	/* attach ppkg structure to head of chain of patch packages */
	ppkg->next = pat->patchpkgs;
	pat->patchpkgs = ppkg;
}

/*
 * split_service()
 *
 * Parameters:
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
split_service(Product * prod)
{
	Modinfo	*i, *k, *info;
	Node	*pkg = prod->p_packages->list;
	Node	*pkg_tmp;
	Node	*nxt_node;

	for (pkg = pkg->next; pkg != prod->p_packages->list; pkg = pkg_tmp) {
		pkg_tmp = pkg->next;
		info = (Modinfo *) pkg->data;
		if (info->m_sunw_ptype == PTYPE_ROOT) {
			i = info;
			while (i) {
				nxt_node = i->m_instances;
				if (i->m_shared != SPOOLED_DUP &&
				    i->m_shared != SPOOLED_NOTDUP &&
				    (i->m_basedir == NULL ||
				    !strneq(i->m_basedir,
				    "/export/exec/", 13))) {
					if (i == info)
						make_pkg_null(i);
					else
						delete_instance(info, i);
				}
				for (k = i; k != NULL; k = next_patch(k)) {
					if (k->m_shared == SPOOLED_DUP)
						k->m_shared = SPOOLED_NOTDUP;
				}
				if (nxt_node)
					i = nxt_node->data;
				else
					i = NULL;
			}
		}

		/*
		 *  For packages of type PTYPE_UNKNOWN (or 0), and which
		 *  do not install into /export/exec, remove them entirely by
		 *  marking the head instance as a NULLPKG and freeing all
		 *  instances and patches.  These packages are not part of
		 *  the service.
		 */
		if ((info->m_basedir &&
		    !strneq(info->m_basedir, "/export/exec/", 13)) &&
		    (info->m_sunw_ptype == PTYPE_UNKNOWN ||
		    info->m_sunw_ptype == 0 ||
		    streq(info->m_basedir, "/opt") ||
		    strneq(info->m_basedir, "/opt/", 5))) {
			remove_instances(info);
			make_pkg_null(info);
			continue;
		}

		for (i = info; i != NULL; i = next_inst(i)) {
			/*
			 *  Mark as NOTDUPLICATE any packages that are
			 *  installed in /export/exec/<anything>.
			 *
			 *  Also mark as NOTDUPLICATE any packages that
			 *  are of type usr, ow, or kvm and which are of
			 *  a non-native ISA.  This get the SUNWowrqd
			 *  package upgraded.  SUNWowrqd has no contents,
			 *  and therefore has no basedir.
			 */
			if (i->m_basedir &&
			    strneq(i->m_basedir, "/export/exec/", 13)) {
				for (k = i; k != NULL; k = next_patch(k))
					k->m_shared = NOTDUPLICATE;
			}
			if ((i->m_sunw_ptype == PTYPE_KVM ||
			    i->m_sunw_ptype == PTYPE_USR ||
			    i->m_sunw_ptype == PTYPE_OW) &&
			    !isa_of_arch_is(i->m_arch, get_default_inst()) &&
			    !streq(i->m_arch, "all")) {
				for (k = i; k != NULL; k = next_patch(k))
					k->m_shared = NOTDUPLICATE;
			}
		}
	}
	return;
}

/*
 * set_loaded()
 *
 * Parameters:
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
set_loaded(Module * prod)
{
	Node		*np;
	Modinfo		*mi, *mip;
	Arch		*ap;
	Module		*mp;

	np = (Node *) prod->info.prod->p_packages->list;
	for (np = np->next; np != prod->info.prod->p_packages->list;
							np = np->next) {
		for (mi = (Modinfo *)np->data; mi != NULL; mi = next_inst(mi))
			for (mip = mi; mip != NULL; mip = next_patch(mip))
				if (mip->m_shared != NULLPKG)
					mip->m_status = LOADED;
				else
					mip->m_status = UNSELECTED;
	}

	for (ap = prod->info.prod->p_arches; ap != NULL; ap = ap->a_next) {
		ap->a_selected = TRUE;
		ap->a_loaded = TRUE;
	}
	for (mp = prod->info.prod->p_locale; mp != NULL; mp = mp->next)
		mp->info.locale->l_selected = mod_status(mp);
	prod->info.prod->p_status = LOADED;
	return;
}

/*
 * is_new_var_sadm()
 *	this function will return tru if the new var/sadm directory
 * 	structure is present or false of it is not present. For simplicity
 * 	and to have a strict rule, the new structure is defined by the
 *	location of the INST_RELEASE file.
 *
 * Called By: load_installed()
 *
 * Parameters:
 *	rootdir - This is the root directory passed in by caller.
 * Return:
 *	1	true the new structure is present
 *	0	false the new structure is not present (old structure assumed)
 * Status:
 *	software library internel
 */
int
is_new_var_sadm(char	*rootdir)
{
	char		tmpFile[MAXPATHLEN];	/* A scratch variable */
	FILE		*fp;
	char		buf[BUFSIZ + 1];
	char		*os = NULL;
	char		*version = NULL;
	char		os_version[MAXPATHLEN];

	/* Now setup a few global variables for some important files. Each */
	/* file name needs to be tested individually for the case of an */
	/* interrupted upgrade or partical upgrade. What is done is the */
	/* file in the new directory tree is checked first, if it is not */
	/* present the old directroy tree is used. */


	(void) sprintf(tmpFile, "%s/%s/var/sadm/system/admin/INST_RELEASE",
	    get_rootdir(), rootdir);

	if ((fp = fopen(tmpFile, "r")) == (FILE *) NULL) {
		/*
		 * Since the open failed we must now check the old path.
		 */
		(void) sprintf(tmpFile, "%s/%s/var/sadm/softinfo/INST_RELEASE",
		    get_rootdir(), rootdir);
		if ((fp = fopen(tmpFile, "r")) == (FILE *) NULL) {
			/*
			 * this is an error. The INST_RELEASE file does not
			 * appear to exist.  Since this code can't quit,
			 * just return (0).
			 */
			return (0);
		}
	}

	/*
	 * Now that the file is opened we can read out the VERSION and OS
	 * to determin where the the var/sadm information is.
	 */
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) -1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		else if (strncmp(buf, "OS=", 3) == 0)
			os = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "VERSION=", 8) == 0)
			version = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "REV=", 4) == 0)
			continue;
	}
	(void) fclose(fp);
	(void) sprintf(os_version, "%s_%s", os, version);
	free(os);
	free(version);

	/*
	 * We now have a version string to compare with, if the version is
	 * < Solaris_2.5 then this is pre-KBI and pre new var/sadm
	 */
	if (prod_vcmp(os_version, "Solaris_2.5") >= 0)
		return (1);	/* A version, 2.5 or higher */
	else
		return (0);	/* Less than 2.5 */

}

/*
 * INST_RELEASE_read_path()
 *	this function will return the correct path for the INST_RELEASE
 *	file.
 *
 * Called By: load_installed()
 *
 * Parameters:
 *	rootdir - This is the root directory passed in by caller.
 * Return:
 *	the path to the file or NULL if not found.
 * Status:
 *	software library internal
 */
char *
INST_RELEASE_read_path(char	*rootdir)
{
	/*
	 * Check to see if the temp variable used to hold the path is
	 * free. If no free it, then allocate a new one.
	 */
	if (INST_RELEASE_tmp_path != (char *)NULL)
		free(INST_RELEASE_tmp_path);
	INST_RELEASE_tmp_path = (char *) xmalloc(MAXPATHLEN);

	if (is_new_var_sadm(rootdir))
		(void) sprintf(INST_RELEASE_tmp_path,
		    "%s/%s/var/sadm/system/admin/INST_RELEASE",
		    get_rootdir(), rootdir);
	else
		(void) sprintf(INST_RELEASE_tmp_path,
		    "%s/%s/var/sadm/softinfo/INST_RELEASE",
		    get_rootdir(), rootdir);

	return (INST_RELEASE_tmp_path);
}

/*
 * services_read_path()
 *	this function will return the correct path for the softinfo
 *	services directory.
 *
 * Called By: load_installed()
 *
 * Parameters:
 *	rootdir - This is the root directory passed in by caller.
 * Return:
 *	the path to the file or NULL if not found.
 * Status:
 *	software library internal
 */
char *
services_read_path(char	*rootdir)
{
	/*
	 * Check to see if the temp variable used to hold the path is
	 * free. If no free it, then allocate a new one.
	 */
	if (services_tmp_path != (char *)NULL)
		free(services_tmp_path);
	services_tmp_path = (char *) xmalloc(MAXPATHLEN);

	if (is_new_var_sadm(rootdir))
		(void) sprintf(services_tmp_path,
		    "%s/%s/var/sadm/system/admin/services",
		    get_rootdir(), rootdir);
	else
		(void) sprintf(services_tmp_path, "%s/%s/var/sadm/softinfo",
		    get_rootdir(), rootdir);

	return (services_tmp_path);
}

/*
 * CULSTER_read_path()
 *	this function will return the correct path for the CLUSTER
 *	file.
 *
 * Called By: remove_extra_clusters, load_installed_product
 *
 * Parameters:
 *	rootdir - This is the root directory passed in by caller.
 * Return:
 *	the path to the file or NULL if not found.
 * Status:
 *	software library internal
 */
char *
CLUSTER_read_path(char	*rootdir)
{
	/*
	 * Check to see if the temp variable used to hold the path is
	 * free. If no free it, then allocate a new one.
	 */
	if (CLUSTER_tmp_path != (char *)NULL)
		free(CLUSTER_tmp_path);
	CLUSTER_tmp_path = (char *) xmalloc(MAXPATHLEN);

	if (is_new_var_sadm(rootdir))
		(void) sprintf(CLUSTER_tmp_path,
		    "%s/%s/var/sadm/system/admin/CLUSTER",
		    get_rootdir(), rootdir);
	else
		(void) sprintf(CLUSTER_tmp_path,
		    "%s/%s/var/sadm/install_data/CLUSTER",
		    get_rootdir(), rootdir);

	return (CLUSTER_tmp_path);
}

/*
 * culstertoc_read_path()
 *	this function will return the correct path for the .clustertoc
 *	file.
 *
 * Called By: load_installed_product
 *
 * Parameters:
 *	rootdir - This is the root directory passed in by caller.
 * Return:
 *	the path to the file or NULL if not found.
 * Status:
 *	software library internal
 */
char *
clustertoc_read_path(char	*rootdir)
{
	/*
	 * Check to see if the temp variable used to hold the path is
	 * free. If not free it, then allocate a new one.
	 */
	if (clustertoc_tmp_path != (char *)NULL)
		free(clustertoc_tmp_path);
	clustertoc_tmp_path = (char *) xmalloc(MAXPATHLEN);

	if (is_new_var_sadm(rootdir))
		(void) sprintf(clustertoc_tmp_path,
		    "%s/%s/var/sadm/system/admin/.clustertoc",
		    get_rootdir(), rootdir);
	else
		(void) sprintf(clustertoc_tmp_path,
		    "%s/%s/var/sadm/install_data/.clustertoc",
		    get_rootdir(), rootdir);

	return (clustertoc_tmp_path);
}

static StringList *
load_hostname(char *	dir)
{
	DIR		*dirp;
	struct dirent	*dp;
	FILE		*fp;
	StringList	*host_list = NULL,
			*host;
	char		line[BUFSIZ];
	char		file[MAXPATHLEN];

	/*
	 * it is OK if we can't read the directory passed in. We may be on
	 * the cdrom.
	 */

	if ((dirp = opendir(dir)) == (DIR *)0)
		return (NULL);

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strncmp(dp->d_name, "hostname.", 9) != 0 ||
		    strlen(dp->d_name) < 10)
			continue;

		(void) sprintf(file, "%s/%s", dir, dp->d_name);

		if ((fp = fopen(file, "r")) != (FILE *) NULL) {
			if (fgets(line, BUFSIZ, fp) != NULL) {
				line[strlen(line) - 1] = '\0';
				if (line[0] != '\0') {
					host = xcalloc(
					    (size_t) sizeof (StringList));
					host->string_ptr = xstrdup(line);
					link_to((Item **)&host_list,
					    (Item *)host);
				}
			}
			fclose(fp);
		}
	}
	closedir(dirp);

	return (host_list);
}

/*
 * load_softinfo()
 *	Load the information from a softinfo file.
 *
 * Parameters:
 *	svc - pointer to service Media module.
 * Return:
 *	none
 * Status:
 *	software library internal
 */
static int
load_softinfo(Module *svc)
{
	char	 softinfo_file[MAXPATHLEN];
	FILE	*fp;
	char	 buf[BUFSIZ + 1];
	char	 key[BUFSIZ];
	int	 len, i;
	char	*cp, *cp2, *cp3, *servdir;
	struct softinfo_desc *soft_desc;
	StringList *sl;
	Node	*np;

	if (svc->sub == NULL || svc->sub->info.prod == NULL)
		return (FAILURE);

	servdir = services_read_path("/");

	(void) sprintf(softinfo_file, "%s/%s",
	    servdir, svc->info.media->med_volume);

	if ((fp = fopen(softinfo_file, "r")) == (FILE *)NULL)
		return (FAILURE);

	soft_desc = (struct softinfo_desc *)xcalloc(
	    (size_t) sizeof (struct softinfo_desc));
	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		if ((cp = strchr(buf, '=')) == NULL)
			continue;
		len = cp - buf;
		(void) strncpy(key, buf, len);
		key[len] = '\0';
		cp++;	/* cp now points to string after '=' */

		if (streq(key, "KVM_PATH")) {
			if ((cp2 = strchr(cp, ':')) == NULL) {
				free_soft_desc(soft_desc);
				return (FAILURE);
			}
			*cp2 = '\0';
			if (!StringListFind(soft_desc->soft_arches, cp)) {
				sl = (StringList *)xcalloc(
				    (size_t) sizeof (StringList));
				sl->string_ptr = xstrdup(cp);
				link_to((Item **)&soft_desc->soft_arches,
				    (Item *)sl);
			}
		} else if (streq(key, "ROOT")) {
			if ((cp2 = strchr(buf, ':')) == NULL ||
			    (cp3 = strchr(cp2, ',')) == NULL) {
				free_soft_desc(soft_desc);
				return (FAILURE);
			}

			*cp2++ = '\0';
			*cp3 = '\0';

			cp3 = strchr(cp, '.');

			/*
			 *  cp now points to the package architecture.
			 *  cp2 now points to the package abbreviation.
			 *  cp3 now points to the ".<karch>" part of the
			 *  architecture, if there is such a part.
			 */

			if (cp3 != NULL && !streq(cp3, ".all") &&
			    !StringListFind(soft_desc->soft_arches, cp)) {
				sl = (StringList *)xcalloc(
				    (size_t) sizeof (StringList));
				sl->string_ptr = xstrdup(cp);
				link_to((Item **)&soft_desc->soft_arches,
				    (Item *)sl);
			}

			if (!StringListFind(soft_desc->soft_packages, cp2)) {
				sl = (StringList *)xcalloc(
				    (size_t) sizeof (StringList));
				sl->string_ptr = xstrdup(cp2);
				link_to((Item **)&soft_desc->soft_packages,
				    (Item *)sl);
			}

		}
	}
	fclose(fp);

	/*
	 *  Now use the data structure just built, then free it.
	 *  First, use it to order the packages in the service.
	 */

	(void) walklist(svc->sub->info.prod->p_packages, set_order_100,
	    (caddr_t)0);
	for (i = 1, sl = soft_desc->soft_packages; sl != NULL;
							i++, sl = sl->next)
		if (np = findnode(svc->sub->info.prod->p_packages,
							sl->string_ptr))
			((Modinfo *)np->data)->m_order = i;
	sort_ordered_pkglist(svc->sub);

	/*
	 * free the soft_desc structure.
	 */
	StringListFree(soft_desc->soft_arches);
	StringListFree(soft_desc->soft_packages);
	free(soft_desc);
	return (SUCCESS);
}

static int
/*ARGSUSED1*/
set_order_100(Node * np, caddr_t data)
{
	((Modinfo*)np->data)->m_order = 100;
	return (SUCCESS);
}

static void
free_soft_desc(struct softinfo_desc *soft_desc)
{
	StringListFree(soft_desc->soft_arches);
	StringListFree(soft_desc->soft_packages);
	free(soft_desc);
}
