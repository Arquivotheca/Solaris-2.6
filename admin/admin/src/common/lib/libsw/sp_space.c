#ifndef lint
#pragma ident   "@(#)sp_space.c 1.83 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "sw_lib.h"
#include "sw_space.h"

#include <sys/stat.h>
#include <malloc.h>

/* Local Statics and Constants */

#define	SP_XLAT		1
#define	SP_RXLAT	2
static	Space 	**cur_sp = NULL;
static	FILE	*dfp = (FILE *)NULL;
static	int	sw_debug_flag = 0;

/* Public Function Prototypes */
Space 	**swi_calc_cluster_space(Module *, ModStatus, int);
Space 	**swi_calc_tot_space(Product *);
Space	**swi_space_meter(char **);
Space	**swi_swm_space_meter(char **);
Space	**swi_upg_space_meter(void);
void	swi_free_space_tab(Space **);

/* Library function prototypes */
int	is_space_ok(Space **);
long	tot_pkg_space(Modinfo *);
int	save_files(char *);
void	init_save_files(void);
int	calc_pkg_space(char *, Modinfo *);
int	final_space_chk(void);

/* Local Function Prototypes */

static int	walk_add_mi_space(Node *, caddr_t);
static int	walk_add_unselect_cspace(Modinfo *, caddr_t);
static int	walk_add_select_cspace(Modinfo *, caddr_t);
static int	walk_upg_installed_mi_space(Node *, caddr_t);
static int	walk_upg_preserved_mi_space(Node *, caddr_t);
static int 	walk_upg_final_chk(Node *, caddr_t);
static int 	walk_upg_final_chk_spooled(Node *, caddr_t);
static int	walk_upg_final_chk_ispooled(Node *, caddr_t);
static int 	add_space_upg_final_chk(Modinfo *, caddr_t);
static int	service_going_away(Module *);
static int	is_servermod(Module *);
static void	add_dflt_fs(Modinfo *, char *);
static void	sp_to_dspace(Modinfo *, Space **);
static void	add_contents_space(Product *, float);
static void	add_pkgdir(Modinfo *, char *);
static int	walk_upg_final_chk_pkgdir(Node *, caddr_t);
static void	upg_meter_sym_trans(Space **, int);
static void	free_space_tabent(Space *);
static void	compute_patchdir_space(Product *);
static FILE *	open_debug_print_file();
static void	close_debug_print_file(FILE *);
static void	print_space_usage(FILE *, char *, Space **);
static int	sp_add_patch_space(Product *, int);
static void	count_space(char *, struct patdir_entry *);
static int	pkg_match(struct patdir_entry *, Product *);

/* Globals and Externals */

extern Space 	**cur_stab;
extern Space 	**sort_space_fs();
extern struct patch_space_reqd *patch_space_head;
extern void	spin(int);
extern int	get_client_space();
extern int	has_view();

char	*slasha = NULL;
Space	**upg_stab, **upg_istab;
int	upg_state;

FILE	*ef = stderr;

static	Space	**sfiles_stab;
static	char 	*Pkgs_dir;

#define	ROOT_COMPONENT		0x0001
#define	NATIVE_USR_COMPONENT	0x0002
#define	NONNATIVE_USR_COMPONENT	0x0004
#define	OPT_COMPONENT		0x0008
#define	SPOOLED_COMPONENT	0x0010

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * tot_pkg_space()
 *	Add up all space fields the package uses.
 * Parameters:
 *	mp	-
 * Return:
 * Status:
 *	public
 */
long
tot_pkg_space(Modinfo * mp)
{
	int 	i;
	daddr_t 	tot = 0;

	if (mp == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: tot_pkg_space():\n");
		(void) fprintf(ef, "Passed a NULL pointer.\n");
#endif
		return (SP_ERR_PARAM_INVAL);
	}

	for (i = 0; i < N_LOCAL_FS; i++)
		tot += mp->m_deflt_fs[i];

	return (tot);
}

/*
 * is_space_ok()
 *	Take a table created with space_meter or upg_space_meter and
 *	evaluate whether or not we have enough space.
 * Parameters:
 *	sp	-
 * Return:
 * Status:
 *	public
 */
int
is_space_ok(Space ** sp)
{
	int	i;

	if (sp == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: is_space_ok():\n");
		(void) fprintf(ef, "Passed a NULL pointer.\n");
#endif
		return (SP_ERR_PARAM_INVAL);
	}

	if (sp[0]->fsi == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: is_space_ok():\n");
		(void) fprintf(ef, "Passed a table without fs information.\n");
#endif
		return (SP_ERR_PARAM_INVAL);
	}

	for (i = 0; sp[i]; i++) {
		if (sp[i]->bused > sp[i]->fsi->f_bavail)
			return (SP_ERR_NOT_ENOUGH_SPACE);
	}
	return (SUCCESS);
}

/*
 * calc_cluster_space()
 *	Create a space table using the default mount points.
 *	Module must either correspond to a cluster or package.
 *	Populate the table based on space usage for the cluster
 *	package. This routine assumes only 1 product (read initial
 *	install)!
 * Parameters:
 *	mod	- pointer to cluster module
 *	status	- SELECTED or UNSELECTED (not currently implemented)
 *	flags	- calculation constraint flag:
 *			CSPACE_ARCH, CSPACE_LOCALE,
 *			CSPACE_NONE, or CSPACE_ALL
 * Return:
 *	NULL	- error
 *	!NULL	- space table pointer with space calculations per FS
 * Status:
 *	public
 */
Space **
swi_calc_cluster_space(Module *mod, ModStatus status, int flags)
{
	Module	*hmod, *prodmod;
	Product	*prod;
	static	Space	**sp = NULL;

	chk_sp_init();
	hmod = get_media_head();
	prodmod = hmod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	if (mod == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: calc_cluster_space():\n");
		(void) fprintf(ef, "Passed a NULL pointer.\n");
#endif
		return ((Space **)NULL);
	}

	if (sp == NULL) {
		sp = load_def_spacetab(NULL);
		if (sp == NULL)
			return ((Space **)NULL);
	} else {
		reset_stab(sp);
	}

	begin_qspace_chk(sp);
	if (status == UNSELECTED)
		walktree(mod, walk_add_unselect_cspace, (caddr_t)flags);
	else if (status == SELECTED)
		walktree(mod, walk_add_select_cspace, (caddr_t)flags);

	end_space_chk();
	return (sp);
}

/*
 * calc_tot_space()
 *	Walk packages list adding up space for selected packages.
 * Parameters:
 *	prod	-
 * Return:
 *
 * Status:
 *	public
 */
Space **
swi_calc_tot_space(Product * prod)
{
	static	Space	**sp = NULL;

	if (prod == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: calc_tot_space():\n");
		(void) fprintf(ef, "Passed a NULL pointer.\n");
#endif
		return (NULL);
	}

	chk_sp_init();

	Pkgs_dir = prod->p_pkgdir;

	if (sp == NULL) {
		sp = load_def_spacetab(NULL);
		if (sp == NULL)
			return (NULL);
	} else {
		reset_stab(sp);
	}

	cur_sp = sp;
	
	begin_qspace_chk(sp);

	(void) walklist(prod->p_packages, walk_add_mi_space, NULL);

	end_space_chk();
	add_fs_overhead(sp, 0);
	return (sp);
}

/*
 * space_meter()
 *	Allocate a space table based on either the default mount points
 *	or the ones listed in in 'mplist'. Run the software tree and
 *	populate the table.
 * Parameters:
 *	mplist	 - array of mount points for which space is to be metered.
 *		   If this is NULL, the default mount point list will be used
 * Return:
 * 	NULL	 - invalid mount point list
 *	Space ** - pointer to allocated and initialized array of space
 *		   structures
 * Status:
 *	public
 */
Space **
swi_space_meter(char **mplist)
{
	Module	*mod, *prodmod;
	Product	*prod;
	Module  *cur_view;
	static	Space **new_sp = NULL;
	static	prev_null = 0;

	if (mplist != (char **)NULL && mplist[0] == (char *)NULL)
		mplist = NULL;

	if (!valid_mountp_list(mplist)) {
#ifdef DEBUG
		(void) fprintf(ef,
			"DEBUG: space_meter(): Invalid mount point passed\n");
#endif
		return (NULL);
	}

	if ((mod = get_media_head()) == (Module *)NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: space_meter(): media head NULL\n");
#endif
		return (NULL);
	}

	dfp = open_debug_print_file();
	
	prodmod = mod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	/* set up the space table */
	chk_sp_init();
	if (mplist == NULL || mplist == def_mnt_pnt) {
		if (prev_null == 1) {
			/* Reuse table. */
			sort_spacetab(new_sp);
			reset_stab(new_sp);
		} else {
			free_space_tab(new_sp);
			new_sp = load_def_spacetab(NULL);
		}
		prev_null = 1;
	} else {
		free_space_tab(new_sp);
		new_sp = load_defined_spacetab(mplist);
		prev_null = 0;
	}

	if (new_sp == NULL)
		return (NULL);

	if (get_sw_debug())
		print_space_usage(dfp, "Space Meter: Before doing anything",
		    new_sp);
	
	/* calculate space requirements of the tree */
	begin_qspace_chk(new_sp);

	if (get_sw_debug())
		print_space_usage(dfp, "Space Meter: After qspace_chk",
		    new_sp);
	
	(void) walklist(prod->p_packages, walk_add_mi_space, prod->p_rootdir);
	if (get_sw_debug())
		print_space_usage(dfp, "Space Meter: After walking packages",
		    new_sp);
	(void) sp_add_patch_space(prod,
	    NATIVE_USR_COMPONENT | OPT_COMPONENT | ROOT_COMPONENT);
	if (get_sw_debug())
		print_space_usage(dfp,
		    "Space Meter: After adding patch space requirements",
		    new_sp);
	
	cur_view = get_current_view(prodmod);
	(void) load_default_view(prodmod);
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type == INSTALLED_SVC) {
			if (service_going_away(mod))
				continue;
			if (has_view(prodmod, mod) != SUCCESS)
				continue;

			(void) load_view(prodmod, mod);
			(void) walklist(prod->p_packages, walk_add_mi_space,
				mod->sub->info.prod->p_rootdir);
			if (get_sw_debug())
				print_space_usage(dfp,
				    "Space Meter: After walking packages (2nd)",
				    new_sp);
			/*
			 *  Currently, initial-install only allocates space
			 *  for the shared service of the same ISA as the
			 *  server itself.  That's why we only add up
			 *  non-native /usr components here.  Native /usr
			 *  components would have already been accounted for.
			 */
			(void) sp_add_patch_space(mod->sub->info.prod,
			    NONNATIVE_USR_COMPONENT | SPOOLED_COMPONENT);
			if (get_sw_debug())
				print_space_usage(dfp,
				    "Space Meter: After adding patch "
				    "space requirements (2nd)",
				    new_sp);
	
		}
	}

	if (cur_view != get_current_view(prodmod)) {
		if (cur_view == NULL)
			(void) load_default_view(prodmod);
		else
			(void) load_view(prodmod, cur_view);
	}

	end_space_chk();
	if (get_sw_debug())
		print_space_usage(dfp, "Space Meter: After space computing",
		    new_sp);
	add_fs_overhead(new_sp, SP_CNT_DEVS);
	if (get_sw_debug())
		print_space_usage(dfp, "Space Meter: After adding overhead",
		    new_sp);

	close_debug_print_file(dfp);
	
	if (mplist != NULL)
		return (sort_space_fs(new_sp, mplist));

	return (sort_space_fs(new_sp, def_mnt_pnt));
}

/*
 * swm_space_meter()
 * Parameters:
 *	mplist	-
 * Return:
 * Status:
 *	private
 */
Space **
swi_swm_space_meter(char **mplist)
{
	Module	*media, *prodmod, *mod;
	Product	*prod;
	static	Space	**sp_meter = NULL;
	long	avail, used;
	int	i;
	Module  *cur_view;
	Media	*mp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("swm_space_meter");
#endif

	if (sp_meter == NULL) {
		if (mplist == NULL)
			sp_meter = load_spacetab(NULL, def_mnt_pnt);
		else
			sp_meter = load_spacetab(NULL, mplist);

		if (sp_meter == NULL)
			return (NULL);
	} else {
		if (reset_swm_stab(sp_meter) != SUCCESS)
			return (NULL);
		sort_spacetab(sp_meter);
	}

	begin_qspace_chk(sp_meter);
	for (media = get_media_head(); media; media = media->next) {
		mp = media->info.media;
		if (mp->med_type == INSTALLED ||
					mp->med_type == INSTALLED_SVC) {
			if (service_going_away(media))
				continue;
			for (mod = get_media_head(); mod; mod = mod->next) {
				if (mp->med_type == INSTALLED ||
						mp->med_type == INSTALLED_SVC)
					continue;
				for (prodmod = mod->sub;
					prodmod; prodmod = prodmod->next) {
					if (has_view(prodmod, media) != SUCCESS)
						continue;
					cur_view = get_current_view(prodmod);
					(void) load_view(prodmod, media);
					prod = prodmod->info.prod;
					Pkgs_dir = prod->p_pkgdir;
					(void) walklist(prod->p_packages,
						walk_add_mi_space,
						media->sub->info.prod->p_rootdir);
					if (cur_view !=
						get_current_view(prodmod)) {
						if (cur_view == NULL)
							(void) load_default_view(
								prodmod);
						else
							(void) load_view(prodmod,
								cur_view);
					}
				}
			}
		} else {
			for (prodmod = media->sub;
				prodmod; prodmod = prodmod->next) {
				cur_view = get_current_view(prodmod);
				(void) load_default_view(prodmod);
				prod = prodmod->info.prod;
				Pkgs_dir = prod->p_pkgdir;
				(void) walklist(prod->p_packages,
					walk_add_mi_space, prod->p_rootdir);
				if (cur_view != get_current_view(prodmod) &&
					cur_view != NULL)
					(void) load_view(prodmod, cur_view);
			}
		}
	}
	end_space_chk();

	add_fs_overhead(sp_meter, 0);

	for (i = 0; sp_meter[i]; i++) {
		avail = sp_meter[i]->fsi->f_bavail;
		used = sp_meter[i]->fsi->f_blocks - sp_meter[i]->fsi->f_bfree;
		sp_meter[i]->bused += used;
		sp_meter[i]->bavail = avail;
	}
	if (mplist != NULL)
		return (sort_space_fs(sp_meter, mplist));
	return (sort_space_fs(sp_meter, def_mnt_pnt));
}


/*
 * free_space_tab()
 *	Free space used by a space table.
 * Parameters:
 *	sp	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_free_space_tab(Space **sp)
{
	int	i;

	if (sp == NULL)
		return;
	for (i = 0; sp[i]; i++) {
		free_space_tabent(sp[i]);
	}
	(void) free(sp);
}

/*
 * calc_pkg_space()
 *	Calculate the default space for each pkg on a cd. Reads the
 *	packages pkgmap and space file and creates a space table which
 *	is attached to the modinfo structure.
 * Parameters:
 *	pkgmap_path - full path to package map file
 *	mp	    -
 * Return:
 *	SP_ERR_PARAM_INVAL	- invalid package map file or
 *				  invalid modinfo pointer
 *	SUCCESS			- space calculated correctly
 *	other			- return values from subroutines
 * Status:
 *	public
 */
int
calc_pkg_space(char *pkgmap_path, Modinfo *mp)
{
	int	ret;
	static	Space	**sp = NULL;
	char	*cp;
	char	space_path[MAXPATHLEN];

	if (mp == NULL || pkgmap_path == NULL)
		return (SP_ERR_PARAM_INVAL);

	chk_sp_init();

	/*
	 * Set path to space file.
	 */
	(void) strcpy(space_path, pkgmap_path);
	cp = strrchr(space_path, '/');
	*cp = '\0';
	(void) strcat(space_path, "/install/space");

	if (sp == NULL)
		sp = load_def_spacetab(NULL);
	else
		reset_stab(sp);

	begin_space_chk(sp);

	if ((ret = sp_read_pkg_map(pkgmap_path, mp->m_pkg_dir, NULL,
			mp->m_basedir, SP_CNT_DEVS)) != SUCCESS)
		return (ret);

	if (path_is_readable(space_path) == SUCCESS) {
		if ((ret = sp_read_space_file(space_path,
				NULL, NULL)) != SUCCESS)
			return (ret);
	}

	end_space_chk();
	sp_to_dspace(mp, sp);

	return (SUCCESS);
}

/*
 * upg_space_meter()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
Space **
swi_upg_space_meter(void)
{
	Module	*media, *prodmod, *mod;
	Module  *cur_view;
	Product	*prod;
	static	Space	**sp_meter = NULL, **installed_sp = NULL;
	static	Space	**preserved_sp = NULL;
	daddr_t	statvfs_usage;
	int	i;

	if (sp_meter == NULL) {
		sp_meter = load_spacetab(NULL, NULL);
		if (sp_meter == NULL)
			return (NULL);
		upg_meter_sym_trans(sp_meter, SP_XLAT);
	} else {
		reset_stab(sp_meter);
		sort_spacetab(sp_meter);
	}

	if (installed_sp == NULL) {
		installed_sp = load_spacetab(NULL, NULL);
		if (installed_sp == NULL)
			return (NULL);
		upg_meter_sym_trans(installed_sp, SP_XLAT);
		begin_qspace_chk(installed_sp);
		for (mod = get_media_head(); mod; mod = mod->next) {
			if ((mod->info.media->med_type != INSTALLED) &&
				(mod->info.media->med_type != INSTALLED_SVC))
				continue;
			if (service_going_away(mod))
				continue;
			if (mod->info.media->med_flags & NEW_SERVICE)
				continue;
			for (prodmod = mod->sub; prodmod; prodmod =
						prodmod->next) {
				prod = prodmod->info.prod;
				Pkgs_dir = prod->p_pkgdir;
				(void) walklist(prod->p_packages,
					walk_upg_installed_mi_space,
					mod->sub->info.prod->p_rootdir);

				if (mod->info.media->med_flags
						& SPLIT_FROM_SERVER)
					continue;
				add_contents_space(prod, 2);
			}
		}
		/*
		 * Files in in this directory get counted as extra space when in
		 * fact most will be replaced. This is a fudge based on the
		 * size of these files in the core cluster.
		 */
		add_file_blks("/var/sadm/pkg", 400, 150, 0);
		end_space_chk();

		preserved_sp = load_spacetab(NULL, NULL);
		if (preserved_sp == NULL)
			return (NULL);
		upg_meter_sym_trans(preserved_sp, SP_XLAT);
		begin_qspace_chk(preserved_sp);
		for (mod = get_media_head(); mod; mod = mod->next) {
			if ((mod->info.media->med_type != INSTALLED) &&
				(mod->info.media->med_type != INSTALLED_SVC))
				continue;
			if (service_going_away(mod))
				continue;
			if (mod->info.media->med_flags & NEW_SERVICE)
				continue;
			for (prodmod = mod->sub; prodmod; prodmod =
						prodmod->next) {
				prod = prodmod->info.prod;
				Pkgs_dir = prod->p_pkgdir;
				(void) walklist(prod->p_packages,
					walk_upg_preserved_mi_space,
					mod->sub->info.prod->p_rootdir);
			}
		}
		end_space_chk();
	}

	begin_qspace_chk(sp_meter);
	for (media = get_media_head(); media; media = media->next) {
		if (media->info.media->med_type == INSTALLED ||
			media->info.media->med_type == INSTALLED_SVC) {
			if (service_going_away(media))
				continue;
			for (mod = get_media_head(); mod; mod = mod->next) {
				if (mod->info.media->med_type == INSTALLED ||
						mod->info.media->med_type ==
						INSTALLED_SVC)
					continue;

				for (prodmod = mod->sub;
					prodmod; prodmod = prodmod->next) {
					if (has_view(prodmod, media) != SUCCESS)
						continue;
					cur_view = get_current_view(prodmod);
					(void) load_view(prodmod, media);
					prod = prodmod->info.prod;
					Pkgs_dir = prod->p_pkgdir;
					(void) walklist(prod->p_packages,
						walk_add_mi_space,
						media->sub->info.prod->p_rootdir);
					if (cur_view !=
						get_current_view(prodmod)) {
						if (cur_view == NULL)
							(void) load_default_view(
								prodmod);
						else
							(void) load_view(prodmod,
								cur_view);
					}
				}
			}
		} else {
			for (prodmod = media->sub;
				prodmod; prodmod = prodmod->next) {
				cur_view = get_current_view(prodmod);
				(void) load_default_view(prodmod);
				prod = prodmod->info.prod;
				Pkgs_dir = prod->p_pkgdir;
				(void) walklist(prod->p_packages,
					walk_add_mi_space, prod->p_rootdir);
				if (cur_view != get_current_view(prodmod) &&
					cur_view != NULL)
					(void) load_view(prodmod, cur_view);
			}
		}
	}
	end_space_chk();

	add_upg_fs_overhead(sp_meter);

	/*
	 * Add int extra space and preverved space.
	 */
	for (i = 0; sp_meter[i]; i++) {
		statvfs_usage = installed_sp[i]->fsi->f_blocks -
			installed_sp[i]->fsi->f_bfree;
		sp_meter[i]->bavail = installed_sp[i]->fsi->f_bavail;
		if (sp_meter[i]->bused != 0) {
			sp_meter[i]->bused = sp_meter[i]->bused +
				(statvfs_usage - installed_sp[i]->bused);
		} else {
			sp_meter[i]->bused = statvfs_usage;
		}

		sp_meter[i]->bused += preserved_sp[i]->bused;
	}

	return (sort_space_fs(sp_meter, def_mnt_pnt));
}

/*
 * init_save_files()
 *
 * initialize or reset the saved_files space table.
 */

void
init_save_files()
{
	if (sfiles_stab != NULL)
		zero_spacetab(sfiles_stab);
}

/*
 * save_files()
 *	Create and populate a space table for save files.
 *	This table will later be used in final_space_chk().
 * Parameters:
 *	fname	-
 * Return:
 *	SP_ERR_PARAM_INVAL	-
 * Status:
 *	public
 */
int
save_files(char * fname)
{
	int	ret;

	chk_sp_init();

	if (fname == NULL)
		return (SP_ERR_PARAM_INVAL);

	if (sfiles_stab == NULL) {
		sfiles_stab = load_spacetab(NULL, NULL);
		if (sfiles_stab == NULL)
			return (SP_ERR_STAB_CREATE);
	}

	if (slasha) {
		if (!do_chroot(slasha))
			return (SP_ERR_CHROOT);
	}

	cur_stab = sfiles_stab;
	ret = do_stat_file(fname);
	cur_stab = (Space **)NULL;

	if (slasha) {
		if (!do_chroot("/"))
			return (SP_ERR_CHROOT);
	}
	return (ret);
}

/*
 * final_space_chk()
 *	Upgrade's final space check.
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
final_space_chk(void)
{
	Module  *mod, *newmedia;
	Module	*split_from_servermod = NULL;
	Product *prod1, *prod2;
	char	*p_rootdir;
	int 	i, err = 0;
	int 	ret = SUCCESS;

	chk_sp_init();
	upg_state |= SP_UPG;

	upg_stab = load_spacetab(NULL, NULL);
	if (upg_stab == NULL)
		return (SP_ERR_STAB_CREATE);

	upg_istab = load_spacetab(NULL, NULL);
	if (upg_istab == NULL)
		return (SP_ERR_STAB_CREATE);

	dfp = open_debug_print_file();
	/*
	 * Grab newmedia pointer and service shared with server info.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if ((mod->info.media->med_type != INSTALLED) &&
			(mod->info.media->med_type != INSTALLED_SVC)) {
			newmedia = mod;
		}
		if ((mod->info.media->med_type == INSTALLED_SVC) &&
			(mod->info.media->med_flags & SPLIT_FROM_SERVER)) {
			split_from_servermod = mod;
		}
	}

	/*
	 * Calculate space for pkgs currently on the system.
	 */
	begin_space_chk(upg_istab);
	upg_state |= SP_UPG_INSTALLED_CHK;
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {

		if ((mod->info.media->med_type != INSTALLED) &&
			(mod->info.media->med_type != INSTALLED_SVC))
			continue;
		if (mod->info.media->med_flags & NEW_SERVICE)
			continue;
		if (service_going_away(mod))
			continue;
		/*
		 *  If the media isn't the basis of an upgrade and
		 *  isn't being modified, or is an unchanged service,
		 *  skip it.
		 */
		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE) &&
		    svc_unchanged(mod->info.media))
			continue;

		prod1 = mod->sub->info.prod;

		if (get_sw_debug())
			print_space_usage(dfp, "Before doing anything",
			    upg_istab);

		/*
		 * Add space for /var/sadm/pkg/<pkg>'s we know about.
		 */
		(void) walklist(prod1->p_packages,
			walk_upg_final_chk_pkgdir,
			prod1->p_rootdir);

		if (get_sw_debug())
			print_space_usage(dfp,
			    "After adding in inital packages", upg_istab);

		/*
		 * Add space for /var/sadm/patch/<patchid> directories.
		 */
		compute_patchdir_space(prod1);

		if (get_sw_debug())
			print_space_usage(dfp, "After Adding in patches",
			    upg_istab);

		/*
		 * Pick up space for spooled packages.
		 */
		if (mod == split_from_servermod) {
			(void) walklist(prod1->p_packages,
				walk_upg_final_chk_ispooled,
				prod1->p_rootdir);
			continue;
		}

		if (get_sw_debug())
			print_space_usage(dfp, "After adding spooled packages",
			    upg_istab);

		(void) sp_load_contents(prod1, NULL);

		add_contents_space(prod1, 1);

		if (get_sw_debug())
			print_space_usage(dfp, "After loading/adding contents",
			    upg_istab);

	}
	end_space_chk();
	upg_state &= ~SP_UPG_INSTALLED_CHK;

	/*
	 * Start the final space table by adding extra space
	 * calculated above.
	 */
	begin_space_chk(upg_stab);
	upg_state |= SP_UPG_EXTRA;
	upg_state |= SP_UPG_SPACE_CHK;

	if (slasha) {
		if (!do_chroot(slasha)) {
			close_debug_print_file(dfp);
			return (SP_ERR_CHROOT);
		}
	}
	for (i = 0; upg_istab[i]; i++) {
		daddr_t	bused, bdiff, fused, fdiff;

		bused = upg_istab[i]->fsi->f_blocks -
					upg_istab[i]->fsi->f_bfree;
		fused = upg_istab[i]->fsi->f_files -
					upg_istab[i]->fsi->f_ffree;

		if (bused > upg_istab[i]->bused)
			bdiff = bused - upg_istab[i]->bused;
		else
			bdiff = 0;
		if (fused > upg_istab[i]->fused)
			fdiff = fused - upg_istab[i]->fused;
		else
			fdiff = 0;

		add_file_blks(upg_istab[i]->mountp, bdiff, fdiff, SP_DIRECTORY);
	}

	if (get_sw_debug())
		print_space_usage(dfp,
		    "After adding in extra space (using upg_stab)", upg_stab);

	if (slasha) {
		if (!do_chroot("/")) {
			close_debug_print_file(dfp);
			return (SP_ERR_CHROOT);
		}
	}
	upg_state &= ~SP_UPG_EXTRA;

	free_space_tab(upg_istab); upg_istab = NULL;

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if ((mod->info.media->med_type != INSTALLED) &&
			(mod->info.media->med_type != INSTALLED_SVC))
			continue;
		if (service_going_away(mod))
			continue;

		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE) &&
		    svc_unchanged(mod->info.media))
			continue;

		prod1 = mod->sub->info.prod;
		prod2 = NULL;

		if (mod != split_from_servermod) {
			/*
			 * Count installed pkgs and services
			 */

			/*
			 * If we share the server as a service.
			 */
			if ((is_servermod(mod)) && (split_from_servermod))
				prod2 = split_from_servermod->sub->info.prod;

			if (mod->info.media->med_type == INSTALLED_SVC) {
				if (!(mod->info.media->med_flags &
							NEW_SERVICE)) {
					(void) sp_load_contents(prod1, NULL);
				}
			} else {
				(void) sp_load_contents(prod1, prod2);
			}

			add_contents_space(prod1, 2.5);
		}

		if (get_sw_debug())
			print_space_usage(dfp,
			    "After loading contents file (preserved files)",
			    upg_stab);

		ret = walklist(prod1->p_packages, walk_upg_final_chk_spooled,
			prod1->p_rootdir);

		if (get_sw_debug())
			print_space_usage(dfp,
			    "After loading new spooled packages", upg_stab);

		if (has_view(newmedia->sub, mod) != SUCCESS)
			continue;
		/*
		 * Count new pkgs and services.
		 */
		p_rootdir = prod1->p_rootdir;
		(void) load_view(newmedia->sub, mod);
		prod1 = newmedia->sub->info.prod;
		Pkgs_dir = prod1->p_pkgdir;
		ret = walklist(prod1->p_packages, walk_upg_final_chk,
			p_rootdir);
		if (get_sw_debug())
			print_space_usage(dfp,
			    "After walking new packages & svcs", upg_stab);

		if (ret != SUCCESS) {
			close_debug_print_file(dfp);
			return (ret);
		}

		/*
		 * Count space for patches to be installed after upgrade
		 * is complete (this might be driver-update patches or
		 * general-purpose patches).
		 */

		if (mod->info.media->med_type == INSTALLED_SVC) {
			if (mod->info.media->med_flags & SPLIT_FROM_SERVER)
				ret = sp_add_patch_space(prod1, 
				    SPOOLED_COMPONENT |
				    NONNATIVE_USR_COMPONENT);
			else
				ret = sp_add_patch_space(prod1, 
				    SPOOLED_COMPONENT |
				    NONNATIVE_USR_COMPONENT |
				    NATIVE_USR_COMPONENT);
		} else {
			if (mod == get_localmedia())
				ret = sp_add_patch_space(prod1, 
				    ROOT_COMPONENT |
				    NATIVE_USR_COMPONENT |
				    OPT_COMPONENT);
			else	/* it's a diskless client */
				ret = sp_add_patch_space(prod1, 
				    ROOT_COMPONENT);
		}
			
		if (ret != SUCCESS) {
			close_debug_print_file(dfp);
			return (ret);
		}

		if (get_sw_debug())
			print_space_usage(dfp,
			    "After adding space for patches", upg_stab);

	}

	/*
	 * save_files() should have been called before us.
	 */
	if (sfiles_stab == NULL) {
		int dummy = 0; dummy += 1;	/* Make lint shut up */
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: final_space_chk():\n");
		(void) fprintf(ef, "No space for save files calculated\n");
#endif
	} else {
		for (i = 0; sfiles_stab[i]; i++) {
			if (sfiles_stab[i]->bused == 0)
				continue;
			add_file_blks(sfiles_stab[i]->mountp,
				sfiles_stab[i]->bused,
				sfiles_stab[i]->fused, SP_DIRECTORY);
		}
	}

	if (get_sw_debug())
		print_space_usage(dfp, "After adding in save files",
		    upg_stab);

	end_space_chk();

	add_upg_fs_overhead(upg_stab);

	if (get_sw_debug())
		print_space_usage(dfp, "After adding in overhead",
		    upg_stab);

	if (get_sw_debug()) {
		fprintf(dfp, "\nSpace available:\n");
		fprintf(dfp, "%20s:    Blocks  \t  Inodes\n",
    		    "Mount Point");
	}

	for (i = 0; upg_stab[i]; i++) {
		if (upg_stab[i]->fsi == NULL) {
#ifdef DEBUG
			(void) fprintf(ef, "DEBUG: final_space_chk():\n");
			(void) fprintf(ef, "upg_stab->fsi == NULL\n");
#endif
			continue;
		}
		if (upg_stab[i]->touched == 0)
			continue;
		if (get_sw_debug())
			fprintf(dfp, "%20s:  %10ld\t%10ld\n",
			    upg_stab[i]->mountp,
			    tot_bavail(upg_stab, i),
			    upg_stab[i]->fsi->f_files);
		if (upg_stab[i]->bused > tot_bavail(upg_stab, i))
			err++;
		if (upg_stab[i]->fused > upg_stab[i]->fsi->f_files)
			err++;
	}

	if (get_sw_debug())
		print_space_usage(dfp, "This should be the final count",
		    upg_stab);

	close_debug_print_file(dfp);

	if (err)
		return (SP_ERR_NOT_ENOUGH_SPACE);
	return (SUCCESS);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * walk_add_mi_space()
 *	walklist() processing routine used to add modinfo space.
 * Parameters:
 *	np	   - current node being processed
 * 	rootdir_p  - package root directory
 * Return:
 *	0
 * Status:
 *	private
 */
static int
walk_add_mi_space(Node *np, caddr_t rootdir_p)
{
	Modinfo	*i, *j;

	for (i = (Modinfo *)np->data; i != (Modinfo *)NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			if (meets_reqs(j))
				add_dflt_fs(j, rootdir_p);
		}
	}
	i = (Modinfo *)np->data;
	
	if (get_sw_debug()) {
		char buf[BUFSIZ];
		sprintf(buf, "walk_add_mi_space:after adding %s",
		    i->m_pkgid);
		print_space_usage(dfp, buf, cur_sp);
	}
	return (0);
}

/*
 * walk_upg_installed_mi_space()
 *	For upgrade space meter.
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 *	0
 * Status:
 *	private
 */
static int
walk_upg_installed_mi_space(Node *np, caddr_t rootdir_p)
{
	Modinfo	*i;

	for (i = (Modinfo *)np->data; i != (Modinfo *)NULL; i = next_inst(i)) {
		if ((i->m_shared != DUPLICATE) &&
					(i->m_shared != SPOOLED_DUP) &&
					(i->m_shared != NULLPKG))
			add_dflt_fs(i, rootdir_p);
	}
	i = (Modinfo *)np->data;
	if (get_sw_debug()) {
		char buf[BUFSIZ];
		sprintf(buf, "walk_upg_installed_mi_space:after adding %s",
		    i->m_pkgid);
		print_space_usage(dfp, buf, cur_sp);
	}

	return (0);
}

/*
 * walk_upg_preserved_mi_space()
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 *	0
 * Status:
 *	private
 */
static int
walk_upg_preserved_mi_space(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		if ((i->m_shared != DUPLICATE) &&
			(i->m_shared != SPOOLED_DUP) &&
			(i->m_shared != NULLPKG) &&
			(i->m_action == TO_BE_PRESERVED))
			add_dflt_fs(i, rootdir_p);
	}
	return (0);
}

/*
 * walk_add_unselect_cspace()
 *	Add space for all patches associated with a given module. The flags
 *	field is used to specify the conditions under which an instance or
 *	localization module should be included
 * Parameters:
 *	mod	- pointer to current module being processed
 *	flags	- CSPACE_ALL, CSPACE_ARCH, CSPACE_NONE, or CSPACE_LOCALE
 * Return:
 *	0	- always returns this
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
walk_add_unselect_cspace(Modinfo *mod, caddr_t flags)
{
	Modinfo	*i;
	int	addit;

	for (addit = 1, i = mod; i != (Modinfo *) NULL
					; i = next_patch(i), addit = 1) {
		if (addit == 1)
			add_dflt_fs(i, "/");
	}

	return (0);
}

/*
 * walk_add_select_cspace()
 *	Add space for a clusters components which are SELECTED, REQUIRED,
 *	or PARTIAL.
 * Parameters:
 *	mod	- current module pointer
 *	flags	- ignored
 * Return:
 *	0	- always returns this
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
walk_add_select_cspace(Modinfo *mod, caddr_t flags)
{
	Modinfo	*i;

	for (i = mod; i != (Modinfo *) NULL; i = next_patch(i)) {

		if ((i->m_status == SELECTED) ||
			(i->m_status == REQUIRED) ||
			(i->m_status == PARTIAL)) {
			add_dflt_fs(i, "/");
		}
	}
	return (0);
}

/*
 * walk_upg_final_chk_spooled()
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 * Status:
 *	private
 */
static int
walk_upg_final_chk_spooled(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i, *j;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			spin(1);
			if ((meets_reqs(j)) &&
				(j->m_shared == SPOOLED_NOTDUP))
				add_dflt_fs(j, rootdir_p);
		}
	}
	return (0);
}

/*
 * walk_upg_final_chk_ispooled()
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 * 	0
 * Status:
 *	private
 */
static int
walk_upg_final_chk_ispooled(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i, *j;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			spin(1);
			if (j->m_shared == SPOOLED_NOTDUP)
				add_dflt_fs(j, rootdir_p);
		}
	}
	return (0);
}

/*
 * walk_upg_final_chk()
 *	New pkgs and svcs.
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 *	return value from add_space_upg_final_chk()
 *	SUCCESS
 * Status:
 *	private
 */
static int
walk_upg_final_chk(Node *np, caddr_t rootdir_p)
{
	int ret;
	Modinfo	*orig_mod, *i, *j;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			spin(1);
			if (meets_reqs(j)) {
				ret = add_space_upg_final_chk(j, rootdir_p);
				if (ret != SUCCESS)
					return (ret);
			}
		}
	}
	return (SUCCESS);
}

/*
 * add_space_upg_final_chk()
 *	New pkgs and svcs.
 * Parameters:
 *	mp	  -
 *	rootdir_p -
 * Return:
 * Status:
 *	private
 */
static int
add_space_upg_final_chk(Modinfo *mp, caddr_t rootdir_p)
{
	char	*rootdir, *bdir;
	char	*slash = "/";
	char	path[MAXPATHLEN];
	char	pkgmap_path[MAXPATHLEN];
	char	space_path[MAXPATHLEN];
	long	sp_sz;
	int	ret;

	if (mp->m_instdir)	bdir = mp->m_instdir;
	else			bdir = mp->m_basedir;

	if (rootdir_p == NULL)	rootdir = slash;
	else			rootdir = rootdir_p;

	if (mp->m_action == TO_BE_SPOOLED) {
		set_path(path, Pkgs_dir, NULL, mp->m_pkg_dir);
		if (mp->m_spooled_size == 0) {
			if ((sp_sz = get_spooled_size(path)) > 0)
			mp->m_spooled_size = (daddr_t) sp_sz;
		}
		if (slasha) {
			if (!do_chroot(slasha))
				return (SP_ERR_CHROOT);
		}
		add_file_blks(bdir, mp->m_spooled_size, 0, SP_DIRECTORY);
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SUCCESS);
	}

	(void) sprintf(pkgmap_path, "%s/%s/pkgmap", Pkgs_dir, mp->m_pkg_dir);
	(void) sprintf(space_path, "%s/%s/install/space",
				Pkgs_dir, mp->m_pkg_dir);

	ret = sp_read_pkg_map(pkgmap_path, mp->m_pkg_dir, rootdir, bdir, 0);
	if (ret != SUCCESS)
		return (ret);

	if (path_is_readable(space_path) == SUCCESS) {
		ret = sp_read_space_file(space_path,  rootdir, bdir);
		if (ret != SUCCESS)
			return (ret);
	}
	return (SUCCESS);
}

/*
 * add_dflt_fs()
 * Parameters:
 *	mp	  -
 *	rootdir_p -
 * Return:
 *	none
 * Status:
 *	private
 */
static void
add_dflt_fs(Modinfo *mp, char *rootdir_p)
{
	daddr_t 	num;
	long	sp_sz;
	char	*bdir;
	char 	path[MAXPATHLEN];
	
	if (cur_stab == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_dflt_fs():\n");
		(void) fprintf(ef, "Called while cur_stab == NULL\n");
#endif
		return;
	}

	if (mp->m_instdir)	bdir = mp->m_instdir;
	else			bdir = mp->m_basedir;

	/*
	 * We need to fix up the base dir if it is /usr. This is done to
	 * correctly get the /usr/openwin space added to the correct
	 * bucket. The problem is that some of the openwin package have a
	 * base dir of / and others have /usr, therefore to get the
	 * set_path to work correctly bdir must be set to /
	 */
	if (bdir != NULL && strcmp(bdir, "/usr") == 0)
	    bdir = "/";
	
	if (mp->m_action == TO_BE_SPOOLED) {
		set_path(path, Pkgs_dir, NULL, mp->m_pkg_dir);
		if (mp->m_spooled_size == 0) {
			if ((sp_sz = get_spooled_size(path)) > 0)
				mp->m_spooled_size = (daddr_t) sp_sz;
		}
		set_path(path, mp->m_instdir, NULL, "/");
		add_file_blks(path, mp->m_spooled_size, 0, SP_DIRECTORY);
		return;

	} else if (mp->m_shared == SPOOLED_NOTDUP) {
		if (mp->m_spooled_size == 0) {
			set_path(path, slasha, mp->m_instdir, "/");
			if ((sp_sz = get_spooled_size(path)) > 0)
				mp->m_spooled_size = (daddr_t) sp_sz;
		}
		set_path(path, mp->m_instdir, NULL, "/");
		add_file_blks(path, mp->m_spooled_size, 0, SP_DIRECTORY);
		return;
	}

	num = mp->m_deflt_fs[ROOT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[USR_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/usr");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[USR_OWN_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/usr/openwin");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[OPT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/opt");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[VAR_FS];
	if (num != 0) {
		set_path(path, rootdir_p, NULL, "/var");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[EXP_EXEC_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/exec");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[EXP_ROOT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/root");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[EXP_HOME_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/home");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
	num = mp->m_deflt_fs[EXPORT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export");
		add_file_blks(path, num, 0, SP_MOUNTP);
	}
}

/*
 * sp_to_dspace()
 * Parameters:
 *	mp	-
 *	sp	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
sp_to_dspace(Modinfo *mp, Space **sp)
{
	int i;

	for (i = 0; i < N_LOCAL_FS; i++)
		mp->m_deflt_fs[i] = 0;

	for (i = 0; sp[i]; i++) {
		if (strcmp(sp[i]->mountp, "/") == 0)
			mp->m_deflt_fs[ROOT_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/usr") == 0)
			mp->m_deflt_fs[USR_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/usr/openwin") == 0)
			mp->m_deflt_fs[USR_OWN_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/opt") == 0)
			mp->m_deflt_fs[OPT_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/var") == 0)
			mp->m_deflt_fs[VAR_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/export/exec") == 0)
			mp->m_deflt_fs[EXP_EXEC_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/export/root") == 0)
			mp->m_deflt_fs[EXP_ROOT_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/export/home") == 0)
			mp->m_deflt_fs[EXP_HOME_FS] = sp[i]->bused;
		else if (strcmp(sp[i]->mountp, "/export") == 0)
			mp->m_deflt_fs[EXPORT_FS] = sp[i]->bused;
	}
}

/*
 * service_going_away()
 *	A sevice which is going away.
 * Parameters:
 *	mod	-
 * Return:
 * Status:
 *	private
 */
static int
service_going_away(Module *mod)
{
	int flags;

	if (mod->info.media->med_type == INSTALLED_SVC) {
		flags = mod->info.media->med_flags;
		if ((flags & SVC_TO_BE_REMOVED) &&
			(!(flags & BASIS_OF_UPGRADE)))
			return (1);
	}
	return (0);
}

/*
 * is_servermod()
 * Parameters:
 *	mod	-
 * Return:
 *	0	-
 *	1	-
 * Status:
 *	private
 */
static int
is_servermod(Module *mod)
{
	if ((mod->info.media->med_type == INSTALLED) &&
		(mod->info.media->med_dir != NULL) &&
		(strcoll(mod->info.media->med_dir, "/") == 0))
		return (1);
	return (0);
}

/*
 * add_contents_space()
 *	For upgrade when we are deriving extra files on the system we must
 *	adjust for the space used by the contents file. By multiplying by
 *	two we grossly approximate for other install related files
 *	in /var/sadm/ not listed in the contents file.
 * Parameters:
 *	prod	-
 *	mult	- this is the multiplier
 * Return:
 *	none
 * Status:
 *	private
 */
static void
add_contents_space(Product * prod, float mult)
{
	char		contname[MAXPATHLEN];
	struct	stat	st;

	if (slasha) {
		if (!do_chroot(slasha))
			return;
	}

	set_path(contname, prod->p_rootdir, NULL, "var/sadm/install/contents");

	if (stat(contname, &st) < 0) {
		int dummy = 0; dummy += 1; /* Make lint shut up */
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: add_contents_space():\n");
		(void) fprintf(ef, "stat failed for file %s\n", contname);
		perror("stat");
#endif
	} else {
		add_file(contname, (int)((float)st.st_size * mult), 1, 0);
	}
	if (slasha)
		(void) do_chroot("/");
}

/*
 * walk_upg_final_chk_pkgdir()
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 *	0
 * Status:
 *	private
 */
static int
walk_upg_final_chk_pkgdir(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i, *j;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		if (i->m_shared == NOTDUPLICATE && !meets_reqs(i))
			for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
				spin(1);
				add_pkgdir(j, rootdir_p);
			}
	}
	return (0);
}

/*
 * add_pkgdir()
 * Parameters:
 *	mi	-
 *	rootdir -
 * Return:
 *	none
 * Status:
 *	private
 */
static void
add_pkgdir(Modinfo *mi, char *rootdir)
{
	int	blks = 0;
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	char	path[MAXPATHLEN];
	FILE	*pp;

	if (slasha) {
		if (!do_chroot(slasha))
			return;
	}

	set_path(path, rootdir, "/var/sadm/pkg", mi->m_pkginst);

	(void) sprintf(command, "/usr/bin/du -sk %s", path);
	if ((pp = popen(command, "r")) == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: track_pkgdir():\n");
		(void) fprintf(ef, "popen failed for du.\n");
#endif
		if (slasha)
			(void) do_chroot("/");
		return;
	}
	if (fgets(buf, BUFSIZ, pp) != NULL) {
		buf[strlen(buf)-1] = '\0';
		(void) sscanf(buf, "%d %*s", &blks);
	}
	(void) pclose(pp);

	/*
	 * Estimate 7 inodes.
	 */
	if (blks != 0)
		add_file_blks(path, (daddr_t) blks, (daddr_t) 7, SP_DIRECTORY);

	if (slasha)
		(void) do_chroot("/");
	return;
}

/*
 * upg_meter_sym_trans()
 *	See if any of the standard mount points are symbolic links
 *	to local ufs partition. If so replace the actual mount
 *	with the default mount point it corresponds to.
 *	We don't currently do the reverse translation.
 * Parameters:
 *	sp	-
 *	flag	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
upg_meter_sym_trans(Space **sp, int flag)
{
	char		*path, buf[MAXPATHLEN], pathbuf[MAXPATHLEN];
	int		i, j, n;
	static int	firstime = 1;
	static char 	*tr[N_LOCAL_FS];

	static char *usr_fs = "/usr";
	static char *usr_own_fs = "/usr/openwin";
	static char *opt_fs = "/opt";
	static char *var_fs = "/var";
	static char *exp_exec_fs = "/export/exec";
	static char *exp_swap_fs = "/export/swap";
	static char *exp_root_fs = "/export/root";
	static char *export_fs = "/export";

	if (firstime) {
		if (slasha) {
			if (!do_chroot(slasha))
				return;
		}
		for (i = 0; i < N_LOCAL_FS; i++) {
			tr[i] = NULL;

			switch (i)
			{
			case USR_FS:		path = usr_fs; break;
			case USR_OWN_FS:	path = usr_own_fs; break;
			case OPT_FS:		path = opt_fs; break;
			case VAR_FS:		path = var_fs; break;
			case EXP_EXEC_FS:	path = exp_exec_fs; break;
			case EXP_SWAP_FS:	path = exp_swap_fs; break;
			case EXP_ROOT_FS:	path = exp_root_fs; break;
			case EXPORT_FS:		path = export_fs; break;
			default:		continue;
			}

			n = readlink(path, buf, (MAXPATHLEN - 1));
			if (n < 1)
				continue;
			buf[n] = '\0';

			if (*buf == '/') {
				set_path(pathbuf, "/", NULL, buf);
			} else {
				char	*cp, lead[MAXPATHLEN];

				strcpy(lead, path);
				cp = strrchr(lead, '/');
				if (cp != lead)
					*cp = '\0';
				else
					*(++cp) = '\0';
				set_path(pathbuf, lead, NULL, buf);
			}
			tr[i] = (char *) xstrdup(pathbuf);
		}
		if (slasha)
			(void) do_chroot("/");
		firstime = 0;
	}

	for (i = 0; i < N_LOCAL_FS; i++) {
		if (!tr[i])
			continue;

		switch (i)
		{
		case USR_FS:		path = usr_fs; break;
		case USR_OWN_FS:	path = usr_own_fs; break;
		case OPT_FS:		path = opt_fs; break;
		case VAR_FS:		path = var_fs; break;
		case EXP_EXEC_FS:	path = exp_exec_fs; break;
		case EXP_SWAP_FS:	path = exp_swap_fs; break;
		case EXP_ROOT_FS:	path = exp_root_fs; break;
		case EXPORT_FS:		path = export_fs; break;
		default:		continue;
		}

		for (j = 0; sp[j]; j++) {
			if (flag == SP_XLAT) {
				if (strcmp(sp[j]->mountp, tr[i]) == 0) {
					free(sp[j]->mountp);
					sp[j]->mountp = path;
				}
			} else {	/* SP_RXLAT */
				if (strcmp(sp[j]->mountp, path) == 0) {
					sp[j]->mountp = tr[i];
				}
			}
		}
	}

	/*
	 * Remove entries in the table not on the def_mnt_list array.
	 */
	if (flag == SP_XLAT) {
		int lastent, found;

		for (i = 0; sp[i]; i++);
		lastent = --i;

		for (i = 0; sp[i]; i++) {
			found = 0;
			for (j = 0; def_mnt_pnt[j]; j++) {
				if (strcmp(sp[i]->mountp,
						def_mnt_pnt[j]) == 0) {
					found = 1;
					break;
				}
			}
			if (!found) {
				free_space_tabent(sp[i]);
				sp[i] = sp[lastent--];
			}
		}
		sort_spacetab(sp);
	}
}

/*
 * free_space_tabent()
 * Parameters:
 *	sp	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
free_space_tabent(Space *sp)
{
	if (sp == NULL)
		return;
	if (sp->mountp)
		(void) free(sp->mountp);
	if (sp->fsi) {
		if (sp->fsi->device)
			(void) free(sp->fsi->device);
		(void) free(sp->fsi);
	}
	(void) free(sp);
}

static void
compute_patchdir_space(Product *prod)
{
	struct patch *p;
	int	blks = 0;
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	char	path[MAXPATHLEN];
	FILE	*pp;

	if (slasha) {
		if (!do_chroot(slasha))
			return;
	}

	for (p = prod->p_patches; p != NULL; p = p->next) {
		/* only count space for patches being removed */
		if (!p->removed)
			continue;
		set_path(path, prod->p_rootdir, "/var/sadm/patch",
		    p->patchid);

		(void) sprintf(command, "/usr/bin/du -sk %s", path);
		if ((pp = popen(command, "r")) == NULL) {
			if (slasha)
				(void) do_chroot("/");
			return;
		}
		if (fgets(buf, BUFSIZ, pp) != NULL) {
			buf[strlen(buf)-1] = '\0';
			(void) sscanf(buf, "%d %*s", &blks);
		}
		(void) pclose(pp);

		/*
		 * Estimate 7 inodes.
		 */
		if (blks != 0)
			add_file_blks(path, (daddr_t) blks, (daddr_t) 7,
			    SP_DIRECTORY);
	}

	if (slasha)
		(void) do_chroot("/");
	return;
}

static FILE *
open_debug_print_file()
{
	FILE		*fp = NULL;
	char		*log_file = "/tmp/space.log";

	sw_debug_flag = set_sw_debug(1);
	
	if (get_sw_debug() && log_file != NULL) {
/*		(void) sprintf(tmpFile, "%s/%s", get_rootdir(), log_file);
		fp = fopen(tmpFile, "w"); */
		fp = fopen(log_file, "w");
		
		if (fp != NULL)
			chmod(log_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}
	return (fp);
}

static void
close_debug_print_file(FILE *fp)
{
	(void) set_sw_debug(sw_debug_flag);
	
	if (fp != NULL) {
		fclose(fp);
	}
}

static void
print_space_usage(FILE *fp, char *message, Space **sp)
{
	int	i;

	if (sp == (Space **)NULL || fp == NULL)
		return;

	fprintf(fp, "\nSpace consumed at: %s\n", message);

	fprintf(fp, "%20s:  Blks Used \tInods Used\tBlks Avail\n",
	    "Mount Point");

	/*
	 * For every file system print out the necessary information
	 */
	for (i = 0; sp[i]; i++)
		fprintf(fp, "%20s:  %10ld\t%10ld\t%10ld\n", sp[i]->mountp,
		    sp[i]->bused, sp[i]->fused, sp[i]->bavail);
}

/*
 * sp_add_patch_space()
 *
 * Parameters:
 *	prod - product where patches will be applied.
 * Return:
 *	SUCCESS
 */
static int
sp_add_patch_space(Product *prod, int component_types)
{
	char		fullpath[MAXPATHLEN + 1];
	struct patch_space_reqd	*psr;
	struct patdir_entry	*pde;

	if (upg_state & SP_UPG) {
		if (slasha) {
			if (!do_chroot(slasha))
				return (SP_ERR_CHROOT);
		}
	}
	for (psr = patch_space_head; psr != NULL; psr = psr->next) {

		if (!arch_is_selected(prod, psr->patsp_arch))
			continue;

		for (pde = psr->patsp_direntry; pde != NULL; pde = pde->next) {

			if (pde->patdir_spooled) {
				if ((component_types & SPOOLED_COMPONENT) &&
				    pkg_match(pde, prod) &&
				    prod->p_name != NULL &&
				    prod->p_version != NULL) {
					(void) sprintf(fullpath,
					    "/export/root/templates/%s_%s",
					    prod->p_name, prod->p_version);
					count_space(fullpath, pde);
				}
				continue;
			}

			set_path(fullpath, prod->p_rootdir, NULL,
			    pde->patdir_dir);

			if (strncmp(pde->patdir_dir, "/usr/", 5) == 0 ||
			     strcmp(pde->patdir_dir, "/usr") == 0) {
				if (supports_arch(get_default_arch(),
				    psr->patsp_arch))
					if ((component_types &
					    NATIVE_USR_COMPONENT) &&
					    pkg_match(pde, prod))
						count_space(fullpath, pde);
				else
					if ((component_types &
					    NONNATIVE_USR_COMPONENT) &&
					    pkg_match(pde, prod))
						count_space(fullpath, pde);
				continue;
			}

			if (strncmp(pde->patdir_dir, "/opt/", 5) == 0 ||
			     strcmp(pde->patdir_dir, "/opt") == 0) {
				if ((component_types & OPT_COMPONENT) &&
				    pkg_match(pde, prod))
					count_space(fullpath, pde);
				continue;
			}

			if ((component_types & ROOT_COMPONENT) &&
			    pkg_match(pde, prod))
				count_space(fullpath, pde);
		}
	}

	if (upg_state & SP_UPG) {
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
	}

	return (SUCCESS);
}

static void
count_space(char *path, struct patdir_entry *pde)
{
	add_file(path, pde->patdir_kbytes * 1024, pde->patdir_inodes,
	    SP_DIRECTORY);
}

static int
pkg_match(struct patdir_entry *pde, Product *prod)
{
	Node	*node;

	if (pde->patdir_pkgid) {
		node = findnode(prod->p_packages, pde->patdir_pkgid);
		if (node && node->data &&
		    (((Modinfo *)(node->data))->m_status == SELECTED ||
		     ((Modinfo *)(node->data))->m_status == REQUIRED))
			return (1);
		else
			return (0);
	}
	return (1);
}
		    
