#ifndef lint
#pragma ident "@(#)soft_sp_space.c 1.15 96/08/30 SMI"
#endif
/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include "spmisoft_lib.h"
#include "sw_space.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Local Globals */

/* Local Statics and Constants */

static	FSspace 	**cur_sp = NULL;
static	FILE	*dfp = (FILE *)NULL;

/* Public Function Prototypes */
FSspace **swi_calc_cluster_space(Module *, ModStatus);
ulong	swi_calc_tot_space(Product *);
void	swi_free_space_tab(FSspace **);
void	swi_free_fsspace(FSspace *);
ulong	swi_tot_pkg_space(Modinfo *);
int	swi_calc_sw_fs_usage(FSspace **, int (*)(void *, void *), void *);
FSspace **gen_dflt_fs_spaceinfo(void);

/* Library function prototypes */
int	calc_pkg_space(char *, Modinfo *);

/* Local Function Prototypes */

static int	walk_add_mi_space(Node *, caddr_t);
static int	walk_add_unselect_cspace(Modinfo *, caddr_t);
static int	walk_add_select_cspace(Modinfo *, caddr_t);
static int 	walk_upg_final_chk(Node *, caddr_t);
static int	walk_upg_preserved_pkgs(Node *np, caddr_t rootdir_p);
static int	walk_upg_final_chk_isspooled(Node *, caddr_t);
static int 	add_space_upg_final_chk(Modinfo *, caddr_t);
static int	service_going_away(Module *);
static int	is_servermod(Module *);
static void	add_dflt_fs(Modinfo *, char *);
static void	sp_to_dspace(Modinfo *, FSspace **);
static void	add_contents_space(Product *, float);
static void	compute_pkg_ovhd(Modinfo *, char *);
static void	add_pkg_ovhd(Modinfo *, char *);
static int	walk_upg_final_chk_pkgdir(Node *, caddr_t);
static void	compute_patchdir_space(Product *);
static FILE *	open_debug_print_file();
static void	close_debug_print_file(FILE *);
static void	print_space_usage(FILE *, char *, FSspace **);
static int	sp_add_patch_space(Product *, int);
static int	pkg_match(struct patdir_entry *, Product *);
static int	count_file_space(Node *, caddr_t);
static void	_count_file_space(Modinfo *, Product *);
static void	do_add_savedfile_space(Module *);
static FSspace	**calc_extra_contents(void);
static int	upg_calc_sw_fs_usage(FSspace **, int (*)(void *, void*),
	void *);
static int	inin_calc_sw_fs_usage(FSspace **, int (*)(void *, void*),
	void *);
static ulong	total_contents_lines(void);
static long	contents_lines(Module *);
static ulong	get_spooled_size(char *);

/* Globals and Externals */

extern struct patch_space_reqd *patch_space_head;
extern	int	doing_add_service;

char	*slasha = NULL;
int	upg_state;
FILE	*ef = stderr;

char 	*Pkgs_dir;
static	FSspace **Tmp_fstab;

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
ulong
swi_tot_pkg_space(Modinfo * mp)
{
	int 	i;
	ulong 	tot = 0;

	if (mp == NULL) {
		return (0);
	}

	for (i = 0; i < N_LOCAL_FS; i++)
		tot += mp->m_deflt_fs[i];

	return (tot);
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
FSspace **
swi_calc_cluster_space(Module *mod, ModStatus status)
{
	Module	*hmod, *prodmod;
	Product	*prod;
	static	FSspace	**sp = NULL;

	hmod = get_media_head();
	prodmod = hmod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	if (mod == NULL)
		return ((FSspace **)NULL);

	if (sp == NULL) {
		sp = load_def_spacetab(NULL);
		if (sp == NULL)
			return ((FSspace **)NULL);
	} else {
		reset_stab(sp);
	}

	begin_global_qspace_sum(sp);
	if (status == UNSELECTED)
		walktree(mod, walk_add_unselect_cspace, (caddr_t)NULL);
	else if (status == SELECTED)
		walktree(mod, walk_add_select_cspace, (caddr_t)NULL);

	(void) end_global_space_sum();
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
ulong
swi_calc_tot_space(Product * prod)
{
	static	FSspace	**sp = NULL;
	ulong	sum;
	int	i;

	if (prod == NULL) {
#ifdef DEBUG
		(void) fprintf(ef, "DEBUG: calc_tot_space():\n");
		(void) fprintf(ef, "Passed a NULL pointer.\n");
#endif
		return (NULL);
	}


	Pkgs_dir = prod->p_pkgdir;

	if (sp == NULL) {
		sp = load_def_spacetab(NULL);
		if (sp == NULL)
			return (NULL);
	} else {
		reset_stab(sp);
	}

	cur_sp = sp;

	begin_global_qspace_sum(sp);

	(void) walklist(prod->p_packages, walk_add_mi_space, NULL);

	(void) end_global_space_sum();

	for (sum = 0, i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		sum += sp[i]->fsp_reqd_contents_space;
	}
	return (sum);
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
swi_free_space_tab(FSspace **sp)
{
	int	i;

	if (sp == NULL)
		return;
	for (i = 0; sp[i]; i++) {
		free_fsspace(sp[i]);
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
	static	FSspace	**sp = NULL;
	char	*cp;
	char	space_path[MAXPATHLEN];

	if (mp == NULL || pkgmap_path == NULL)
		return (SP_ERR_PARAM_INVAL);

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

	begin_specific_space_sum(sp);

	if ((ret = sp_read_pkg_map(pkgmap_path, mp->m_pkg_dir, NULL,
			mp->m_basedir, SP_CNT_DEVS, sp)) != SUCCESS)
		return (ret);

	if (path_is_readable(space_path) == SUCCESS) {
		if ((ret = sp_read_space_file(space_path,
				NULL, NULL, sp)) != SUCCESS)
			return (ret);
	}

	end_specific_space_sum(sp);
	sp_to_dspace(mp, sp);

	return (SUCCESS);
}

int
swi_calc_sw_fs_usage(FSspace **fs_list, int (*callback_proc)(void *, void*),
	void *callback_arg)
{
	if (is_upgrade() || doing_add_service)
		return (upg_calc_sw_fs_usage(fs_list, callback_proc,
		    callback_arg));
	else
		return (inin_calc_sw_fs_usage(fs_list, callback_proc,
		    callback_arg));
}

/*
 * gen_dflt_fs_spaceinfo()
 *	Allocate a space table based on the default mount points.
 *	Run the software tree and populate the table.
 * Parameters:
 *
 * Return:
 * 	NULL	 - invalid mount point list
 *	Space ** - pointer to allocated and initialized array of space
 *		   structures
 * Status:
 *	public
 */
FSspace **
gen_dflt_fs_spaceinfo(void)
{
	Module	*mod, *prodmod;
	Product	*prod;
	static	FSspace **new_sp = NULL;
	static	prev_null = 0;

	if ((mod = get_media_head()) == (Module *)NULL) {
		return (NULL);
	}

	prodmod = mod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	/* set up the space table */
	if (prev_null == 1) {
		/* Reuse table. */
		sort_spacetab(new_sp);
		reset_stab(new_sp);
	} else {
		free_space_tab(new_sp);
		new_sp = load_def_spacetab(NULL);
	}
	prev_null = 1;

	if (new_sp == NULL)
		return (NULL);

	if (calc_sw_fs_usage(new_sp, NULL, NULL) != SUCCESS)
		return (NULL);

	return (new_sp);
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

	if (get_trace_level() > 0) {
		char buf[BUFSIZ];
		(void) sprintf(buf, "walk_add_mi_space:after adding %s",
		    i->m_pkgid);
		print_space_usage(dfp, buf, cur_sp);
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

	for (i = mod; i != (Modinfo *) NULL; i = next_patch(i)) {
			add_dflt_fs(i, "/");
	}

	return (0);
}

/*
 * walk_add_select_cspace()
 *	Add space for a clusters components which are SELECTED, REQUIRED,
 *	or PARTIALLY_SELECTED.
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
			(i->m_status == PARTIALLY_SELECTED)) {
			add_dflt_fs(i, "/");
		}
	}
	return (0);
}

/*
 * walk_upg_preserved_pkgs()
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 * Status:
 *	private
 */
static int
walk_upg_preserved_pkgs(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i, *j;
	char	path[MAXPATHLEN];

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			if (meets_reqs(j)) {
				if (j->m_shared == SPOOLED_NOTDUP) {
					set_path(path, j->m_instdir, NULL, "/");
					add_file_blks(path, j->m_spooled_size,
					    0, SP_DIRECTORY, (FSspace **)NULL);
				} else {
					add_pkg_ovhd(j, rootdir_p);
					add_contents_record(j->m_fs_usage,
					    (FSspace **)NULL);
				}
			}
		}
	}
	return (0);
}

/*
 * walk_upg_final_chk_isspooled() - count the currently-spooled
 *	packages.
 *
 *	note:  If ProgressInCountMode() returns true, just count
 *		the spooled packages.
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 * 	0
 * Status:
 *	private
 */
/*ARGSUSED1*/
static int
walk_upg_final_chk_isspooled(Node *np, caddr_t data)
{
	Modinfo	*orig_mod, *i, *j;
	ulong	sp_sz;
	char	path[MAXPATHLEN];

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		for (j = i; j != (Modinfo *) NULL; j = next_patch(j)) {
			if (j->m_shared == SPOOLED_NOTDUP) {
				if (ProgressInCountMode())
					ProgressCountActions(PROG_DIR_DU, 1);
				else {
					if (j->m_spooled_size == 0) {
						set_path(path, slasha,
						    j->m_instdir, "/");
						if ((sp_sz =
						    get_spooled_size(path)) > 0)
							j->m_spooled_size =
							    sp_sz;
						ProgressAdvance(PROG_DIR_DU, 1,
						    VAL_SPOOLPKG_SPACE,
						    j->m_pkgid);
					}
					set_path(path, j->m_instdir, NULL, "/");
					add_file_blks(path, j->m_spooled_size,
					    0, SP_DIRECTORY, (FSspace **)NULL);
				}
			}
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
	struct	stat	sbuf;

	if (mp->m_instdir)	bdir = mp->m_instdir;
	else			bdir = mp->m_basedir;

	if (rootdir_p == NULL)	rootdir = slash;
	else			rootdir = rootdir_p;

	if (mp->m_action == TO_BE_SPOOLED) {
		if (ProgressInCountMode())
			return (SUCCESS);
		set_path(path, Pkgs_dir, NULL, mp->m_pkg_dir);
		if (mp->m_spooled_size == 0) {
			if ((sp_sz = get_spooled_size(path)) > 0)
			mp->m_spooled_size = (ulong) sp_sz;
		}
		if (slasha) {
			if (!do_chroot(slasha))
				return (SP_ERR_CHROOT);
		}
		add_file_blks(bdir, mp->m_spooled_size, 0, SP_DIRECTORY,
		    (FSspace **)NULL);
		if (slasha) {
			if (!do_chroot("/"))
				return (SP_ERR_CHROOT);
		}
		return (SUCCESS);
	}

	if (mp->m_fs_usage == NULL) {
		(void) sprintf(pkgmap_path, "%s/%s/pkgmap", Pkgs_dir,
		    mp->m_pkg_dir);
		(void) sprintf(space_path, "%s/%s/install/space",
				Pkgs_dir, mp->m_pkg_dir);

		if (ProgressInCountMode()) {
			if (stat(pkgmap_path, &sbuf) == 0)
				ProgressCountActions(PROG_PKGMAP_SIZE,
				    sbuf.st_size);
			return (SUCCESS);
		}

		reset_stab(Tmp_fstab);
		begin_specific_space_sum(Tmp_fstab);
		ret = sp_read_pkg_map(pkgmap_path, mp->m_pkg_dir, rootdir,
		    bdir, 0, Tmp_fstab);
		if (ret != SUCCESS) {
			end_specific_space_sum(Tmp_fstab);
			return (ret);
		}

		if (path_is_readable(space_path) == SUCCESS) {
			ret = sp_read_space_file(space_path,  rootdir, bdir,
			    Tmp_fstab);
			if (ret != SUCCESS) {
				end_specific_space_sum(Tmp_fstab);
				return (ret);
			}
		}

		end_specific_space_sum(Tmp_fstab);
		mp->m_fs_usage = contents_record_from_stab(Tmp_fstab,
		    (ContentsRecord *)NULL);
		mp->m_pkgovhd_size = 10;  /* estimate 10 blks per package */
	}
	add_pkg_ovhd(mp, rootdir);
	add_contents_record(mp->m_fs_usage, (FSspace **)NULL);
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
	ulong 	num;
	long	sp_sz;
	char	*bdir;
	char 	path[MAXPATHLEN];

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
				mp->m_spooled_size = (ulong) sp_sz;
		}
		set_path(path, mp->m_instdir, NULL, "/");
		add_file_blks(path, mp->m_spooled_size, 0, SP_DIRECTORY,
		    (FSspace **)NULL);
		return;

	}

	num = mp->m_deflt_fs[ROOT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[USR_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/usr");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[USR_OWN_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/usr/openwin");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[OPT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/opt");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[VAR_FS];
	if (num != 0) {
		set_path(path, rootdir_p, NULL, "/var");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[EXP_EXEC_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/exec");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[EXP_ROOT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/root");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[EXP_HOME_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export/home");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
	}
	num = mp->m_deflt_fs[EXPORT_FS];
	if (num != 0) {
		set_path(path, rootdir_p, bdir, "/export");
		add_file_blks(path, num, 0, SP_MOUNTP, (FSspace **)NULL);
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
sp_to_dspace(Modinfo *mp, FSspace **sp)
{
	int i;

	for (i = 0; i < N_LOCAL_FS; i++)
		mp->m_deflt_fs[i] = 0;

	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		if (strcmp(sp[i]->fsp_mntpnt, "/") == 0)
			mp->m_deflt_fs[ROOT_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/usr") == 0)
			mp->m_deflt_fs[USR_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/usr/openwin") == 0)
			mp->m_deflt_fs[USR_OWN_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/opt") == 0)
			mp->m_deflt_fs[OPT_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/var") == 0)
			mp->m_deflt_fs[VAR_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/export/exec") == 0)
			mp->m_deflt_fs[EXP_EXEC_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/export/root") == 0)
			mp->m_deflt_fs[EXP_ROOT_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/export/home") == 0)
			mp->m_deflt_fs[EXP_HOME_FS] =
			    sp[i]->fsp_reqd_contents_space;
		else if (strcmp(sp[i]->fsp_mntpnt, "/export") == 0)
			mp->m_deflt_fs[EXPORT_FS] =
			    sp[i]->fsp_reqd_contents_space;
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
		add_file(contname, (int)((float)st.st_size * mult), 1, 0,
		    (FSspace **)NULL);
	}
	if (slasha)
		(void) do_chroot("/");
}

/*
 * walk_upg_final_chk_pkgdir() - compute the space for the
 *	/var/sadm/pkg/<pkginst> directories (existing).
 *
 *	Note: this function has two modes.  If ProgressInCountMode()
 *		returns TRUE, just count the package directories.
 * Parameters:
 *	np	  -
 *	rootdir_p -
 * Return:
 *	0
 * Status:
 *	private
 *
 */
static int
walk_upg_final_chk_pkgdir(Node *np, caddr_t rootdir_p)
{
	Modinfo	*orig_mod, *i, *j;

	orig_mod = (void *) np->data;
	for (i = orig_mod; i != (Modinfo *) NULL; i = next_inst(i)) {
		if (i->m_shared == NOTDUPLICATE) {
			if (!(i->m_flags & IS_UNBUNDLED_PKG)) {
				for (j = i; j != (Modinfo *) NULL;
				    j = next_patch(j)) {
					if (ProgressInCountMode())
						ProgressCountActions(
						    PROG_DIR_DU, 1);
					else {
						compute_pkg_ovhd(j, rootdir_p);
						add_pkg_ovhd(j, rootdir_p);
						ProgressAdvance(PROG_DIR_DU, 1,
						    VAL_CURPKG_SPACE,
						    j->m_pkginst ?
						    j->m_pkginst : j->m_pkgid);
					}
				}
			}
		}
	}
	return (0);
}

/*
 * compute_pkg_ovhd()
 * Parameters:
 *	mi	-
 *	rootdir -
 * Return:
 *	none
 * Status:
 *	private
 */
static void
compute_pkg_ovhd(Modinfo *mi, char *rootdir)
{
	int	blks = 0;
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	char	path[MAXPATHLEN];
	FILE	*pp;

	if (slasha) {
		if (!do_chroot(slasha))
			return;
	}

	set_path(path, rootdir, "/var/sadm/pkg", mi->m_pkginst ?
	    mi->m_pkginst : mi->m_pkgid);

	(void) sprintf(command, "/usr/bin/du -sk %s", path);
	if ((pp = popen(command, "r")) == NULL) {
		if (slasha)
			(void) do_chroot("/");
		return;
	}
	while (!feof(pp)) {
		if (fgets(buf, BUFSIZ, pp) != NULL) {
			buf[strlen(buf) - 1] = '\0';
			(void) sscanf(buf, "%d %*s", &blks);
		}
	}
	(void) pclose(pp);

	mi->m_pkgovhd_size = blks;

	if (slasha)
		(void) do_chroot("/");
	return;
}

/*
 * add_pkg_ovhd()
 * Parameters:
 *	mi	-
 *	rootdir -
 * Return:
 *	none
 * Status:
 *	private
 */
static void
add_pkg_ovhd(Modinfo *mi, char *rootdir)
{
	char	path[MAXPATHLEN];

	if (slasha) {
		if (!do_chroot(slasha))
			return;
	}

	set_path(path, rootdir, "/var/sadm/pkg", mi->m_pkginst ?
	    mi->m_pkginst : mi->m_pkgid);

	/*
	 * Estimate 7 inodes.
	 */
	if (mi->m_pkgovhd_size != 0)
		add_file_blks(path, mi->m_pkgovhd_size, (ulong) 7,
		    SP_DIRECTORY, (FSspace **)NULL);

	if (slasha)
		(void) do_chroot("/");
	return;
}

/*
 * compute_patchdir_space() - compute the space used by
 *	/var/sadm/patch/<pkginst> directories.  If ProgressInCountMode()
 *	returns TRUE, just count the directories to be du'd.
 * Parameters:
 *	prod
 * Return:
 *	none
 * Status:
 *	private
 */
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
		if (ProgressInCountMode()) {
			ProgressCountActions(PROG_DIR_DU, 1);
			continue;
		}
		set_path(path, prod->p_rootdir, "/var/sadm/patch",
		    p->patchid);

		(void) sprintf(command, "/usr/bin/du -sk %s", path);
		if ((pp = popen(command, "r")) == NULL) {
			if (slasha)
				(void) do_chroot("/");
			return;
		}
		while (!feof(pp)) {
			if (fgets(buf, BUFSIZ, pp) != NULL) {
				buf[strlen(buf) - 1] = '\0';
				(void) sscanf(buf, "%d %*s", &blks);
			}
		}
		(void) pclose(pp);

		/*
		 * Estimate 7 inodes.
		 */
		if (blks != 0)
			add_file_blks(path, (ulong) blks, (ulong) 7,
			    SP_DIRECTORY, (FSspace **)NULL);

		ProgressAdvance(PROG_DIR_DU, 1, VAL_CURPATCH_SPACE,
			p->patchid);
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

	if (get_trace_level() > 0 && log_file != NULL) {
		fp = fopen(log_file, "w");

		if (fp != NULL)
			(void) chmod(log_file, S_IRUSR | S_IWUSR |
			    S_IRGRP | S_IROTH);
	}
	return (fp);
}

static void
close_debug_print_file(FILE *fp)
{
	if (fp != NULL) {
		(void) fclose(fp);
		fp = NULL;
	}
}

static void
print_space_usage(FILE *fp, char *message, FSspace **sp)
{
	int	i;

	if (sp == (FSspace **)NULL || fp == NULL)
		return;

	(void) fprintf(fp, "\nSpace consumed at: %s\n", message);

	(void) fprintf(fp, "%20s:  Blks Used \tInodes Used\n",
	    "Mount Point");

	/*
	 * For every file system print out the necessary information
	 */
	for (i = 0; sp[i]; i++) {
		if (sp[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		(void) fprintf(fp, "%20s:  %10ld\t%10ld\n", sp[i]->fsp_mntpnt,
		    sp[i]->fsp_reqd_contents_space,
		    sp[i]->fsp_cts.contents_inodes_used);
	}
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
					add_file(fullpath,
					    pde->patdir_kbytes * 1024,
					    pde->patdir_inodes, SP_DIRECTORY,
					    (FSspace **)NULL);
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
						add_file(fullpath,
						    pde->patdir_kbytes * 1024,
						    pde->patdir_inodes,
						    SP_DIRECTORY,
						    (FSspace **)NULL);
				else
					if ((component_types &
					    NONNATIVE_USR_COMPONENT) &&
					    pkg_match(pde, prod))
						add_file(fullpath,
						    pde->patdir_kbytes * 1024,
						    pde->patdir_inodes,
						    SP_DIRECTORY,
						    (FSspace **)NULL);
				continue;
			}

			if (strncmp(pde->patdir_dir, "/opt/", 5) == 0 ||
			    strcmp(pde->patdir_dir, "/opt") == 0) {
				if ((component_types & OPT_COMPONENT) &&
				    pkg_match(pde, prod))
					add_file(fullpath,
					    pde->patdir_kbytes * 1024,
					    pde->patdir_inodes, SP_DIRECTORY,
					    (FSspace **)NULL);
				continue;
			}

			if ((component_types & ROOT_COMPONENT) &&
			    pkg_match(pde, prod))
				add_file(fullpath, pde->patdir_kbytes * 1024,
				    pde->patdir_inodes, SP_DIRECTORY,
				    (FSspace **)NULL);
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

void
swi_free_fsspace(FSspace *fsp)
{
	(void) free(fsp->fsp_mntpnt);
	StringListFree(fsp->fsp_pkg_databases);
	(void) free(fsp);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

static int
upg_calc_sw_fs_usage(FSspace **fs_list, int (*callback_proc)(void *, void*),
	void *callback_arg)
{
	Module  *mod, *newmedia;
	Module	*split_from_servermod = NULL;
	Product *prod1;
	int 	ret = SUCCESS;
	FSspace	**xstab;
	static	int first_pass = 1;

	ProgressAdvance(PROG_BEGIN, 0, VAL_ANALYZE_BEGIN, NULL);

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

	if (newmedia == NULL)
		return (ERR_NOMEDIA);
	upg_state |= SP_UPG;

	/*
	 *  If this is the first pass through the code, use the
	 *  callback functions to record progress.  Subsequent
	 *  calls should make use of the data calculated in the
	 *  first pass and so should be fast enough to not require
	 *  progress metering.
	 *
	 *  Count total space checking actions to be performed.
	 */
	if (first_pass && callback_proc != NULL) {
		ProgressBeginActionCount();

		/* count the number of lines to be processed by find_modified */
		if (!doing_add_service)
			ProgressCountActions(PROG_FIND_MODIFIED,
			    total_contents_lines());

		/* call calc_extra_contents in action-counting mode */
		(void) calc_extra_contents();

		for (mod = get_media_head(); mod != NULL; mod = mod->next) {
			if ((mod->info.media->med_type != INSTALLED) &&
				(mod->info.media->med_type != INSTALLED_SVC))
				continue;
			if (service_going_away(mod))
				continue;

			if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE) &&
			    svc_unchanged(mod->info.media))
				continue;

			if (has_view(newmedia->sub, mod) != SUCCESS)
				continue;

			(void) load_view(newmedia->sub, mod);
			Pkgs_dir = newmedia->sub->info.prod->p_pkgdir;
			(void) walklist(newmedia->sub->info.prod->p_packages,
			    walk_upg_final_chk, (caddr_t)0);

		}
		ProgressBeginMetering(callback_proc, callback_arg);
	}

	if (!doing_add_service)
		find_modified_all();
	dfp = open_debug_print_file();

	xstab = calc_extra_contents();
	if (xstab == NULL)
		return (FAILURE);

	reset_stab(fs_list);

	/*
	 * Start the final space table by adding extra space
	 * calculated above.
	 */
	begin_global_space_sum(fs_list);

	add_spacetab(xstab, (FSspace **)NULL);
	if (get_trace_level() > 0)
		print_space_usage(dfp,
		    "After adding in extra space (using fs_list)", fs_list);


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

		if (mod != split_from_servermod)
			add_contents_space(prod1, 2.5);

		(void) walklist(prod1->p_packages, walk_upg_preserved_pkgs,
		    prod1->p_rootdir);

		if (get_trace_level() > 0)
			print_space_usage(dfp,
			    "After loading preserved packages",
			    fs_list);

		if (has_view(newmedia->sub, mod) != SUCCESS)
			continue;
		/*
		 * Count new pkgs (both pkgadded and spooled) and services.
		 */
		Tmp_fstab = get_current_fs_layout();
		(void) load_view(newmedia->sub, mod);
		Pkgs_dir = newmedia->sub->info.prod->p_pkgdir;
		/*
		 *  The following values are passed globally to
		 *  walk_upg_final_chk:
		 *
		 *  Pkgs_dir : the directory containing the new packages.
		 *  Tmp_fstab : the fstab used for temporary per-package
		 * 	space calculations (as necessary).
		 */
		ret = walklist(newmedia->sub->info.prod->p_packages,
		    walk_upg_final_chk, prod1->p_rootdir);
		if (get_trace_level() > 0)
			print_space_usage(dfp,
			    "After walking new packages (both spooled and "
			    "pkgadded)", fs_list);

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
				ret = sp_add_patch_space(
				    newmedia->sub->info.prod,
				    SPOOLED_COMPONENT |
				    NONNATIVE_USR_COMPONENT);
			else
				ret = sp_add_patch_space(
				    newmedia->sub->info.prod,
				    SPOOLED_COMPONENT |
				    NONNATIVE_USR_COMPONENT |
				    NATIVE_USR_COMPONENT);
		} else {
			if (mod == get_localmedia())
				ret = sp_add_patch_space(
				    newmedia->sub->info.prod,
				    ROOT_COMPONENT |
				    NATIVE_USR_COMPONENT |
				    OPT_COMPONENT);
			else	/* it's a diskless client */
				ret = sp_add_patch_space(
				    newmedia->sub->info.prod,
				    ROOT_COMPONENT);
		}

		if (ret != SUCCESS) {
			close_debug_print_file(dfp);
			return (ret);
		}

		if (get_trace_level() > 0)
			print_space_usage(dfp,
			    "After adding space for patches", fs_list);

	}

	do_add_savedfile_space(newmedia->sub);
	if (get_trace_level() > 0)
		print_space_usage(dfp, "After adding in save files",
		    fs_list);

	(void) end_global_space_sum();

	if (get_trace_level() > 0) {
		(void) fprintf(dfp, "\nSpace available:\n");
		(void) fprintf(dfp, "%20s:    Blocks  \t  Inodes\n",
		    "Mount Point");
	}

	if (first_pass)
		ProgressEndMetering();
	ProgressAdvance(PROG_END, 0, VAL_ANALYZE_END, NULL);

	first_pass = 0;
	upg_state &= ~SP_UPG;

	close_debug_print_file(dfp);
	return (SUCCESS);
}

/*ARGSUSED1*/
static int
inin_calc_sw_fs_usage(FSspace **fs_list, int (*callback_proc)(void *, void*),
	void *callback_arg)
{
	Module	*mod, *prodmod;
	Product	*prod;
	Module  *cur_view;

	if ((mod = get_media_head()) == (Module *)NULL)
		return (NULL);

	dfp = open_debug_print_file();

	prodmod = mod->sub;
	prod = prodmod->info.prod;
	Pkgs_dir = prod->p_pkgdir;

	/* set up the space table */
	sort_spacetab(fs_list);
	reset_stab(fs_list);

	if (get_trace_level() > 0)
		print_space_usage(dfp,
		    "inin_calc_sw_fs_usage: Before doing anything", fs_list);

	/* calculate space requirements of the tree */
	begin_global_qspace_sum(fs_list);

	if (get_trace_level() > 0)
		print_space_usage(dfp,
		    "inin_calc_sw_fs_usage: After qspace_chk", fs_list);

	(void) walklist(prod->p_packages, walk_add_mi_space, prod->p_rootdir);
	if (get_trace_level() > 0)
		print_space_usage(dfp,
		    "inin_calc_sw_fs_usage: After walking packages", fs_list);
	(void) sp_add_patch_space(prod,
	    NATIVE_USR_COMPONENT | OPT_COMPONENT | ROOT_COMPONENT);

	if (get_trace_level() > 0)
		print_space_usage(dfp,
		"inin_calc_sw_fs_usage: After adding patch space requirements",
		    fs_list);

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
			if (get_trace_level() > 0)
				print_space_usage(dfp,
		    "inin_calc_sw_fs_usage: After walking packages (2nd)",
				    fs_list);
			/*
			 *  Currently, initial-install only allocates space
			 *  for the shared service of the same ISA as the
			 *  server itself.  That's why we only add up
			 *  non-native /usr components here.  Native /usr
			 *  components would have already been accounted for.
			 */
			(void) sp_add_patch_space(mod->sub->info.prod,
			    NONNATIVE_USR_COMPONENT | SPOOLED_COMPONENT);
			if (get_trace_level() > 0)
				print_space_usage(dfp,
				    "inin_calc_sw_fs_usage: After adding patch "
				    "space requirements (2nd)",
				    fs_list);

		}
	}

	if (cur_view != get_current_view(prodmod)) {
		if (cur_view == NULL)
			(void) load_default_view(prodmod);
		else
			(void) load_view(prodmod, cur_view);
	}

	(void) end_global_space_sum();
	if (get_trace_level() > 0)
		print_space_usage(dfp,
		    "inin_calc_sw_fs_usage: After space computing", fs_list);

	close_debug_print_file(dfp);

	return (SUCCESS);
}

static void
do_add_savedfile_space(Module *prodmod)
{
	Module	*mod;

	if (slasha)
		do_chroot(slasha);

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
			(void) walklist(mod->sub->info.prod->p_packages,
			    count_file_space, (caddr_t)(mod->sub->info.prod));
		}
	}

	if (slasha)
		do_chroot("/");
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
		 *  1)	the replacing package is selected or required and
		 *	TO_BE_PKGADDED
		 *  2)	if the action is not TO_BE_PRESERVED and
		 *	if the contents of the package are not going away
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

			(void) strcpy(file, prod->p_rootdir);
			(void) strcat(file, fdp->component_path);
			(void) record_save_file(file, (FSspace **)NULL);
		}
		fdp = fdp->diff_next;
	}
}

/*
 * calc_extra_contents()
 *	Calculate the amount of space in each file system that
 *	is not accounted for any by any package or patch.
 *
 *	This function has two modes, controlled by the value
 *	returned by ProgressInCountMode().  When in progress-counting
 *	mode, this function does nothing but count the number of actions
 *	to be performed by the validation phase.  This is to provide a
 *	total number of actions so that a percent-complete number can
 *	be computed for the validation (for the progress-display callbacks).
 *
 * Parameters:
 * Return:
 * Status:
 *	public
 */
static FSspace **
calc_extra_contents(void)
{
	Module  *mod;
	Module	*split_from_servermod = NULL;
	Product *prod1, *prod2;
	static	FSspace  **upg_xstab = NULL;
	FSspace  **upg_istab;
	int	i;

	if (upg_xstab != NULL)
		return (upg_xstab);

	if (!ProgressInCountMode()) {
		upg_istab = get_current_fs_layout();
		if (upg_istab == NULL)
			return (NULL);
	}

	/*
	 * Grab newmedia pointer and service shared with server info.
	 */
	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if ((mod->info.media->med_type == INSTALLED_SVC) &&
			(mod->info.media->med_flags & SPLIT_FROM_SERVER)) {
			split_from_servermod = mod;
		}
	}

	/*
	 * Calculate space for pkgs currently on the system.
	 */
	if (!ProgressInCountMode()) {
		begin_global_space_sum(upg_istab);
	}
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

		if (!ProgressInCountMode() && get_trace_level() > 0)
			print_space_usage(dfp, "Before doing anything",
			    upg_istab);

		/*
		 * Add space for /var/sadm/pkg/<pkg>'s we know about.
		 * walk_upg_final_chk_pkgdir will only count the packages
		 * if ProgressInCountMode().
		 */
		(void) walklist(prod1->p_packages,
			walk_upg_final_chk_pkgdir,
			prod1->p_rootdir);

		if (!ProgressInCountMode() && get_trace_level() > 0)
			print_space_usage(dfp,
			    "After adding in initial packages", upg_istab);

		/*
		 * Add space for /var/sadm/patch/<patchid> directories.
		 * compute_patchdir_space() will only count the patches
		 * if ProgressInCountMode();
		 */
		compute_patchdir_space(prod1);

		if (!ProgressInCountMode() && get_trace_level() > 0)
			print_space_usage(dfp, "After Adding in patches",
			    upg_istab);

		/*
		 * Pick up space for spooled packages.
		 * walk_upg_final_chk_isspooled() will only count the patches
		 * if ProgressInCountMode();
		 */
		if (mod == split_from_servermod) {
			(void) walklist(prod1->p_packages,
				walk_upg_final_chk_isspooled, (caddr_t)0);
			continue;
		}

		if (!ProgressInCountMode() && get_trace_level() > 0)
			print_space_usage(dfp, "After adding spooled packages",
			    upg_istab);

		if (!ProgressInCountMode()) {

			/*
			 * If we share the server as a service.
			 */

			if ((is_servermod(mod)) && (split_from_servermod))
				prod2 = split_from_servermod->sub->info.prod;

			/*
			 * Otherwise, set it to NULL so that we don't get
			 * into trouble in the sp_load_contents()
			 */

			else
				prod2 = NULL;

			(void) sp_load_contents(prod1, prod2);

			add_contents_space(prod1, 1);
		} else
			ProgressCountActions(PROG_CONTENTS_LINES,
			    contents_lines(mod));

		if (!ProgressInCountMode() && get_trace_level() > 0)
			print_space_usage(dfp, "After loading/adding contents",
			    upg_istab);



	}
	if (ProgressInCountMode())
		return (NULL);
	(void) end_global_space_sum();

	upg_xstab = get_current_fs_layout();
	if (upg_xstab == NULL)
		return (NULL);
	for (i = 0; upg_istab[i]; i++) {
		ulong	bused, bdiff, fused, fdiff;

		if (upg_istab[i]->fsp_flags & FS_IGNORE_ENTRY)
			continue;
		bused = upg_istab[i]->fsp_fsi->f_blocks -
					upg_istab[i]->fsp_fsi->f_bfree;
		fused = upg_istab[i]->fsp_fsi->f_files -
					upg_istab[i]->fsp_fsi->f_ffree;

		if (bused > upg_istab[i]->fsp_reqd_contents_space)
			bdiff = bused - upg_istab[i]->fsp_reqd_contents_space;
		else
			bdiff = 0;
		if (fused > upg_istab[i]->fsp_cts.contents_inodes_used)
			fdiff = fused -
			    upg_istab[i]->fsp_cts.contents_inodes_used;
		else
			fdiff = 0;

		fsp_set_field(upg_xstab[i], FSP_CONTENTS_NONPKG, bdiff);
		upg_xstab[i]->fsp_cts.contents_inodes_used = fdiff;
	}
	free_space_tab(upg_istab);
	return (upg_xstab);
}

static ulong
total_contents_lines(void)
{
	Module	*mod;
	ulong total_lines = 0;

	for (mod = get_media_head(); mod != NULL;
	    mod = mod->next) {
		if (!(mod->info.media->med_flags & BASIS_OF_UPGRADE))
			continue;
		/*
		 * don't scan for modified files if this
		 * service is actually the server's own
		 * service.
		 */
		if (mod->info.media->med_type == INSTALLED_SVC &&
		    mod->info.media->med_flags & SPLIT_FROM_SERVER)
			continue;

		if (mod->info.media->med_type == INSTALLED ||
		    mod->info.media->med_type == INSTALLED_SVC)
			total_lines += contents_lines(mod);
	}
	return (total_lines);
}

/* return the number of lines in a particular contents file */
static long
contents_lines(Module *mod)
{
	char	cmd[MAXPATHLEN + 20];
	long	lines = 0;
	FILE	*pp;
	char	buf[BUFSIZ];

#ifdef POPEN_BUG
	/*
	 * On i86, this code will sometimes return 0.  Not sure what the
	 * cause is, but need to find it eventually.  Keep this code segment
	 * around for debugging.
	 * The replacement code is more efficient
	 */
	(void) strcpy(cmd, "/usr/bin/wc -l ");
	if (*get_rootdir() != '\0')
		(void) strcat(cmd, get_rootdir());

	(void) strcat(cmd, mod->sub->info.prod->p_rootdir);
	(void) strcat(cmd, "/var/sadm/install/contents 2>/dev/null");

	if ((pp = popen(cmd, "r")) == NULL)
		return (0);
	while (!feof(pp)) {
		if (fgets(buf, BUFSIZ, pp) != NULL) {
			buf[strlen(buf)-1] = '\0';
			(void) sscanf(buf, "%ld %*s", &lines);
		}
	}
	(void) pclose(pp);
#else
	(void) strcpy(cmd, get_rootdir());
	(void) strcat(cmd, mod->sub->info.prod->p_rootdir);
	(void) strcat(cmd, "/var/sadm/install/contents");

	if ((pp = fopen(cmd, "r")) == NULL)
		return (0);
	while (!feof(pp)) {
		if (fgets(buf, BUFSIZ, pp) != NULL)
			lines++;
	}
	(void) fclose(pp);
#endif
	return (lines);
}
/*
 * get_spooled_size()
 * Get the number of blocks used by the filesystem tree specified by 'pkgdir'.
 * Parameter:	pkgdir	- directory to summarize
 * Return:	# >= 0	- block count
 */
static ulong
get_spooled_size(char * pkgdir)
{
	daddr_t	blks = 0;
	char	buf[BUFSIZ], command[MAXPATHLEN + 20];
	FILE	*pp;

	if (pkgdir == NULL) {
#ifdef DEBUG
		(void) fprintf(ef,
		    "DEBUG: get_spooled_size(): pkgdir = NULL.\n");
#endif
		return (0);

	}

	if (path_is_readable(pkgdir) != SUCCESS) {
		set_sp_err(SP_ERR_STAT, errno, pkgdir);
#ifdef DEBUG
		(void) fprintf(ef,
		    "DEBUG: get_spooled_size(): path unreadable: %s.\n",
		    pkgdir);
#endif
		return (0);
	}

	(void) sprintf(command, "/usr/bin/du -sk %s", pkgdir);
	if ((pp = popen(command, "r")) == NULL) {
		set_sp_err(SP_ERR_POPEN, -1, command);
#ifdef DEBUG
		(void) fprintf(ef,
		    "DEBUG: get_spooled_size(): popen failed for du.\n");
#endif
		return (0);
	}
	while (!feof(pp)) {
		if (fgets(buf, BUFSIZ, pp) != NULL) {
			buf[strlen(buf)-1] = '\0';
			(void) sscanf(buf, "%ld %*s", &blks);
		}
	}
	(void) pclose(pp);

	return (blks);
}
