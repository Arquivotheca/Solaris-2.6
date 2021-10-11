#ifndef lint
#ident   "@(#)media.c 1.50 96/02/08 SMI"
#endif
/*
 * Copyright (c) 1995-1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */
#include "sw_lib.h"

#include <sys/mount.h>
#include <sys/cdio.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#ifndef i386
#include <sys/dklabel.h>
#else
#include <sys/vtoc.h>
#endif
#include <sys/fs/ufs_fs.h>

/* External Globals */

/*
 * these had to be referenced because the set_current() function
 * has no mechanism for NULLing out the *_media pointers
 */
extern Module	*current_media;
extern Module	*default_media;

/* Local Statics and Constants */

/* Local Globals */

Media	*avail_media;		/* list of media/directories we have read */
Media	*inst_media;		/* list of installed packages */
Module	*head_ptr;		/* pointer to module at head of chain */
struct	patch_space_reqd *patch_space_head = NULL;

int	eject_on_exit = 1;

/* Public Function Prototypes */

Module * 	swi_add_media(char *);
Module * 	swi_add_specific_media(char *, char *);
int		swi_load_media(Module *, int);
int		swi_mount_media(Module *, char *, MediaType);
int		swi_unload_media(Module *);
void		swi_set_eject_on_exit(int);
Module * 	swi_get_media_head(void);
Module * 	swi_find_media(char *, char *);

/* Library Function Prototypes */

Module * 	duplicate_media(Module *);
void		dup_clstr_tree(Module *, Module *);
Module *	find_service_media(char *);

/* Local Function Prototypes */

static int		doumount(char *, char *, int);
static void		doeject(char *);
static int		read_product(Module *);
static Module * 	dup_tree(Module *, Module *, Module *);
static int		dup_clstr_list(Node *np, caddr_t);
static int		dup_list(Node *, caddr_t);
static Depend *		duplicate_depend(Depend *);
static Modinfo *	dup_modinfo(Modinfo *, caddr_t);
static char *		canonical_path(char *, char *);
static void		canonize_dir(char **);
static struct patch_space_reqd *load_patch_space_requirements(void);
static void		process_patch_space_entry(char *,
			    struct patch_space_reqd *);
static struct patch_num *dup_patchnum_list(struct patch_num *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * add_media()
 *	Adds the media specified by 'dir' to the list of known media.
 *	The media can refer to a CD, CD image, or installed system.
 *	If the media already exists in the media chain, a pointer to
 *	the existing module is returned, otherwise a pointer to the
 *	newly created module is returned. The pointer returned is used
 *	in future calls to routines such as load_media() and mount_media().
 * Parameters:
 *	dir	  - media name of directory in filesystem namespace
 * Return:
 *	NULL	  - error adding media
 *	Module *  - pointer to the media module structure
 * Status:
 *	public
 */
Module *
swi_add_media(char * dir)
{
	Module	*mp;
	char	 dirbuf[MAXPATHLEN];
	char	*dirp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("add_media");
#endif

	if ((dirp = canonical_path(dir, dirbuf)) == NULL)
		return (NULL);

	/* find the existing media structure if it exists */
	for (mp = head_ptr; mp != (Module *) NULL; mp = mp->next) {
		if (mp->type != MEDIA)
			continue;
		if (mp->info.media->med_device != NULL) {
			canonize_dir(&(mp->info.media->med_device));
			if (strcmp(mp->info.media->med_device, dirp) == 0) {
				break;
			}
		}
		if (mp->info.media->med_dir != NULL) {
			canonize_dir(&(mp->info.media->med_dir));
			if (strcmp(mp->info.media->med_dir, dirp) == 0)
				break;
		}
	}
	if (mp != NULL)
		return (mp);

	return (add_specific_media(dirp, NULL));
}

/*
 * add_specific_media()
 *	Used by SWMTOOL in place of add_media() to allow the specification
 *	of volumes and devices in place of mounted directories. 'volume'
 *	will eventually go away, it's only here for compatibility with the
 *	previous versions of swmtool which implemented their own volume
 *	management scheme.
 * Parameters:
 *	dir	- character string continaing media directory name
 *	dev	- character string continaing media device name
 * Return:
 *	NULL	  - error adding media
 *	Module *  - pointer to the media module structure
 * Status:
 *	public
 */
Module *
swi_add_specific_media(char * dir, char * dev)
{
	Module	*mp;
	Module	*i;
	char	dirbuf[MAXPATHLEN];
	char	devbuf[MAXPATHLEN];
	char	*dirp = NULL;
	char	*devp = NULL;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("add_specific_media");
#endif

	if (dir)
		if ((dirp = canonical_path(dir, dirbuf)) == NULL)
			return (NULL);
	if (dev)
		if ((devp = canonical_path(dev, devbuf)) == NULL)
			return (NULL);

	/* find the existing media structure if it exists */
	for (mp = head_ptr; mp != (Module *) NULL; mp = mp->next) {
		if (mp->type != MEDIA)
			continue;
		if (devp != NULL) {
			if (mp->info.media->med_device == NULL)
				continue;
			canonize_dir(&(mp->info.media->med_device));
			if (strcmp(mp->info.media->med_device, devp) != 0)
				continue;
		}
		if (dirp != NULL) {
			if (mp->info.media->med_dir == NULL)
				continue;
			canonize_dir(&(mp->info.media->med_dir));
			if (strcmp(mp->info.media->med_dir, dirp) != 0)
				continue;
		}
		break;
	}

	if (mp == (Module *)NULL) {
		/* allocate and initialize module structure */
		mp = (Module *)xcalloc(sizeof (Module));

		/* allocate and initialize Media structure */
		mp->info.media = (Media *)xcalloc(sizeof (Media));

		/* set device or dir to dir/device as appropropriate */
		if (devp == NULL)
			mp->info.media->med_device = (char *)NULL;
		else
			mp->info.media->med_device = (char *)xstrdup(devp);
		if (dirp == NULL)
			mp->info.media->med_dir = (char *)NULL;
		else
			mp->info.media->med_dir = (char *)xstrdup(dirp);

		/* add to the end of the chain */
		if (head_ptr == NULL) {
			head_ptr = mp;
			mp->prev = (Module *)NULL;
		} else {
			for (i = head_ptr; i->next != NULL; i = i->next)
				;
			i->next = mp;
			mp->prev = i;
		}
		mp->head = head_ptr;
		mp->type = MEDIA;
		mp->next = (Module *) NULL;
		mp->sub = (Module *) NULL;
		mp->parent = (Module *) NULL;
	}
	return (mp);
}

/*
 * load_media()
 *	Loads the products and builds the software tree associated with the
 *	specified media 'mod'. If 'mod' is NULL the current media is loaded.
 *	If 'use_packagetoc' is TRUE, the ".packagetoc" file is used to obtain
 *	the package information if it's available. If "use_packagetoc" is FALSE
 *	or a ".packagetoc" isn't available, the package information is loaded
 *	from the pkgmap files. In general, "use_packagetoc" should be TRUE,
 *	because of the significant speed-up in reading in this information.
 * Parameters:
 *	mod		- pointer to media module (NULL if current media is to
 *			  be used)
 *	use_packagetoc	- TRUE of FALSE
 * Return:
 *	ERR_NOMEDIA	- media module NULL and no current media
 *	ERR_INVALIDTYPE - 'mod' is not a media module
 *	ERR_UNMOUNTED	- media does not have a name in the filesystem namespace
 *	ERR_NOPROD	- no products available on the specified
 *	other		- all codes returned by load_product() indicating
 *			  problems in loading specific products
 *	SUCCESS		- media loaded and software tree built successfully
 * Status:
 *	public
 */
int
swi_load_media(Module * mod, int use_packagetoc)
{
	Module	*media;
	int	return_val;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("load_media");
#endif

	/* Get correct module pointer */
	media = mod;
	if (mod == (Module *)NULL)
		media = get_current_media();

	if (media == (Module *)NULL)
		return (ERR_NOMEDIA);

	if (media->type != MEDIA)
		return (ERR_INVALIDTYPE);

	if (media->info.media->med_dir == (char *)NULL)
		return (ERR_UMOUNTED);

	/*
	 *  Load patch-space requirements information.
	 *  Note:  It's sort of a kludge to load this here, since it
	 *  really isn't associated with the media being loaded.  But
	 *  we need to load it somewhere and this is as good a place as
	 *  any.  This whole patch-space requirements file will probably
	 *  get reimplemented somewhat in a subsequent release.
	 */
	if (patch_space_head == NULL)
		patch_space_head = load_patch_space_requirements();

	/* load toc and products */
	if (read_product(media) != SUCCESS)
		return (ERR_NOPROD);

	/* read all locale descriptions */
	read_locale_table(media);

	if ((return_val = load_all_products(media, use_packagetoc)) != SUCCESS)
		return (return_val);

	/* set default product to first product */
	media->info.media->med_deflt_prod = media->sub;

	return (SUCCESS);
}


/*
 * mount_media()
 *	(SWMTOOL specific) Mounts the media device specified by the 'mod'
 *	on the mount point specified by 'mount_pt'. 'type' allows the
 *	caller to specify the type of the media.
 * Parameters:
 *	mod	    - pointer to media module
 *	mount_pt    - mount point for media
 *	type	    - media type specifier (see enum media_type)
 * Returns:
 *	SUCCESS	    - successful
 *	ERR_MOUNTED - already mounted
 *	ERR_MOUNTPT - the specified mount point doesn't exist
 *	ERR_VOLUME  - volume management is running
 *	errno	    - open or mount of specified device fails
 * Status:
 *	public
 */
int
swi_mount_media(Module * mod, char * mount_pt, MediaType type)
{
	int		status, fd;
	Module		*media;
	struct stat	sb;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mount_media");
#endif

	if (!mod)
		media = get_current_media();
	else
		media = mod;

	if (!media || media->type != MEDIA)
		return (ERR_INVALIDTYPE);

	media->info.media->med_type = type;

	if ((media->info.media->med_device == (char *)NULL) ||
				(media->info.media->med_dir != (char *)NULL))
		return (ERR_MOUNTED);

	if (path_is_readable(mount_pt) != SUCCESS)
		return (ERR_MOUNTPT);
	media->info.media->med_status = MOUNT_NO_CREATE;

	if ((fd = open(media->info.media->med_device, O_RDONLY)) < 0) {
		if (path_is_readable(media->info.media->med_device) == SUCCESS
					&& stat("/vol/dev", &sb) == 0)
			return (ERR_VOLUME);
		else
			return (errno);
	}
	close(fd);

	status = mount_fs(media->info.media->med_device, mount_pt, "hsfs");
	if (status < 0 && errno == EINVAL)
		status = mount_fs(media->info.media->med_device, mount_pt,
									"ufs");
	if (status < 0)
		return (errno);

	media->info.media->med_dir = (char *)xstrdup(mount_pt);

	return (SUCCESS);
}


/*
 * unload_media()
 *	Unloads the specified media.  All data structures associated with the
 *	sub-modules of this media are freed. If 'mod' is NULL, the current
 *	media is unloaded.  If the media being unloaded is the current media,
 *	set the 'current_media' pointer to NULL.  If it is the default media,
 *	set the 'default_media' pointer to NULL. Explicit access to these
 *	global pointers is required because there is no library mechanism to
 *	do so (set_current() would require another parameter)
 * Parameters:
 *	mod	- pointer to media module to unload
 * Return:
 *	ERR_INVALIDTYPE	- 'mod' is not a media module
 *	other		- value returned during unmount attempt on media
 *	SUCCESS		- media unload successful
 * Status:
 *	public
 */
int
swi_unload_media(Module * mod)
{
	Module	*media;
	Media	*mp;
	int	err = SUCCESS;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("unload_media");
#endif

	if (!mod)
		media = get_current_media();
	else
		media = mod;

	if (!media || (media->type != MEDIA))
		return (ERR_INVALIDTYPE);

	mp = media->info.media;

	/****** shouldn't we free all views of this media first ??? *******/

	if (mp->med_type == CDROM)
		err = doumount(mp->med_device, mp->med_dir, eject_on_exit);

	/* remove media structure from media chain */
	if (media->prev == NULL)
		head_ptr = media->next;
	else
		media->prev->next = media->next;

	if (media->next != NULL)
		media->next->prev = media->prev;

	/*
	 * Reset the current_media and/or default_media pointers if needed
	 * This ought to be done by some call such as set_current(NULL, MEDIA)
	 * and set_default(NULL, MEDIA), but this is not how the API works at
	 * this time.
	 */
	if (media == get_current_media())
		current_media = (Module *)NULL;

	if (media == get_default_media())
		default_media = (Module *)NULL;

	/* deallocate the media module */
	free_media(media);

	return (err);
}

/*
 * set_eject_on_exit()
 *	Sets a value to indicate whether a previously mounted media
 *	(CDROM only) should be ejected when unload_media is called. 'value'
 *	of #!=0 indicates that the media should be ejected on when unloaded.
 *	A 'value' of 0 indicates that the media should not be ejected.
 * Parameters:
 *	value	- '0' (set to eject on exit) or '# != 0' (set not to eject
 *		  on exit)
 * Return:
 *	none
 * Status:
 *	Public
 */
void
swi_set_eject_on_exit(int value)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_eject_on_exit");
#endif
	if (value == 0)
		eject_on_exit = 0;
	else
		eject_on_exit = 1;
}

/*
 * get_media_head()
 *	Return the pointer to the head of the media module list.
 * Parameters:
 *	none
 * Return:
 *	Module *  - pointer to module at the head of the media list
 * Status:
 *	public
 */
Module *
swi_get_media_head(void)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_media_head");
#endif
	return (head_ptr);
}

/*
 * find_media()
 *	Traverse the media Modinfo list looking for a media whose device
 *	is 'dev' and whose directory is 'dir'. If only one of 'dev' or 'dir'
 *	are specified, then only one is used in the match. Return a pointer
 *	to that structure.
 * Parameters:
 *	dev	- string containing device name of desired media
 *	dir	- string containing directory name of desired media
 * Return:
 *	NULL	 - no matching media found in tree
 *	Module * - pointer to matching media module
 * Status:
 *	public
 */
Module *
swi_find_media(char * dir, char * dev)
{
	Module	*mp;
	char	dirbuf[MAXPATHLEN];
	char	devbuf[MAXPATHLEN];
	char	*dirp = NULL;
	char	*devp = NULL;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("find_media");
#endif

	if (dir)
		if ((dirp = canonical_path(dir, dirbuf)) == NULL)
			return (NULL);
	if (dev)
		if ((devp = canonical_path(dev, devbuf)) == NULL)
			return (NULL);

	/* find the existing media structure if it exists */
	for (mp = head_ptr; mp != (Module *) NULL; mp = mp->next) {
		if (mp->type != MEDIA)
			continue;
		if (dev != NULL) {
			if (mp->info.media->med_device == NULL)
				continue;
			canonize_dir(&(mp->info.media->med_device));
			if (strcmp(mp->info.media->med_device, devp) != 0)
				continue;
		}
		if (dir != NULL) {
			if (mp->info.media->med_dir == NULL)
				continue;
			canonize_dir(&(mp->info.media->med_dir));
			if (strcmp(mp->info.media->med_dir, dirp) != 0)
				continue;
		}
		break;
	}
	return (mp);
}

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * duplicate_media()
 * Parameters:
 *	current	-
 * Return:
 * Status:
 * 	semi-private (internal library use only)
 */
Module *
duplicate_media(Module * current)
{
	Module 		*new;
	Module 		*mp, *mp2;
	Arch   		*ap, *ap2;
	Module 		*tp, *tp2;
	Module	*cat, *cat2;
	PlatGroup	*pg, *pg2;
	Platform	*plat, *plat2;

	/* allocate and initialize module structure */
	new = (Module *)xcalloc(sizeof (Module));
	new->type = current->type;
	for (mp = current->sub; mp != NULL; mp = mp->next) {
		if (new->sub == NULL) {
			new->sub = mp2 = (Module *)xcalloc(sizeof (Module));
			mp2->prev = NULL;
		} else {
			mp2->next = (Module *)xcalloc(sizeof (Module));
			mp2->next->prev = mp2;
			mp2 = mp2->next;
		}
		mp2->type = mp->type;
		mp2->head = new->sub;
		mp2->parent = new;

		mp2->info.prod = (Product *)xcalloc(sizeof (Product));
		if (mp->info.prod->p_name != NULL)
			mp2->info.prod->p_name = xstrdup(mp->info.prod->p_name);
		if (mp->info.prod->p_version != NULL)
			mp2->info.prod->p_version =
					xstrdup(mp->info.prod->p_version);
		if (mp->info.prod->p_rev != NULL)
			mp2->info.prod->p_rev =
					xstrdup(mp->info.prod->p_rev);
		if (mp->info.prod->p_id != NULL)
			mp2->info.prod->p_id = xstrdup(mp->info.prod->p_id);
		if (mp->info.prod->p_pkgdir != NULL)
			mp2->info.prod->p_pkgdir =
					xstrdup(mp->info.prod->p_pkgdir);
		if (mp->info.prod->p_rootdir != NULL)
			mp2->info.prod->p_rootdir =
					xstrdup(mp->info.prod->p_rootdir);
		if (mp->info.prod->p_instdir != NULL)
			mp2->info.prod->p_instdir =
					xstrdup(mp->info.prod->p_instdir);
		mp2->info.prod->p_status = mp->info.prod->p_status;
		mp2->info.prod->p_current_view = mp2->info.prod;
		mp2->info.prod->p_next_view = NULL;
		mp2->info.prod->p_view_pkgs = NULL;
		mp2->info.prod->p_view_cluster = NULL;
		mp2->info.prod->p_view_locale = NULL;
		mp2->info.prod->p_view_arches = NULL;
		mp2->info.prod->p_packages = getlist();
		walklist(mp->info.prod->p_packages, dup_list, (caddr_t)mp2);
		mp2->info.prod->p_clusters = getlist();
		walklist(mp->info.prod->p_clusters,dup_clstr_list,(caddr_t)mp2);
		ap2 = NULL;
		for (ap = mp->info.prod->p_arches; ap != NULL; ap = ap->a_next) {
			if (ap2 == NULL) {
				mp2->info.prod->p_arches = ap2 =
						(Arch *)xcalloc(sizeof (Arch));
			} else {
				ap2->a_next = (Arch *)xcalloc(sizeof (Arch));
				ap2 = ap2->a_next;
			}
			ap2->a_arch = xstrdup(ap->a_arch);
			ap2->a_selected = ap->a_selected;
			ap2->a_loaded = ap->a_loaded;
		}

		/* duplicate platform groups */
		pg2 = NULL;
		for (pg = mp->info.prod->p_platgrp; pg != NULL; pg = pg->next) {
			if (pg2 == NULL)
				mp2->info.prod->p_platgrp = pg2 =
				    (PlatGroup *)xcalloc(sizeof (PlatGroup));
			else {
				pg2->next = (PlatGroup *)xcalloc(sizeof
				    (PlatGroup));
				pg2 = pg2->next;
			}
			pg2->pltgrp_name = xstrdup(pg->pltgrp_name);
			pg2->pltgrp_isa = xstrdup(pg->pltgrp_isa);
			pg2->pltgrp_export = pg->pltgrp_export;
			plat2 = NULL;
			for (plat = pg->pltgrp_members; plat != NULL;
					plat = plat->next) {
				if (plat2 == NULL)
					pg2->pltgrp_members = plat2 =
					    (Platform *)xcalloc(sizeof
					    (Platform));
				else {
					plat2->next = (Platform *)xcalloc(sizeof
					    (Platform));
					plat2 = plat2->next;
				}
				plat2->plat_name = xstrdup(plat->plat_name);
				plat2->plat_uname_id =
					xstrdup(plat->plat_uname_id);
				plat2->plat_machine =
					xstrdup(plat->plat_machine);
				plat2->plat_group = xstrdup(plat->plat_group);
				plat2->plat_isa = xstrdup(plat->plat_isa);
			}
		}

		tp2 = NULL;
		for (tp = mp->info.prod->p_locale; tp != NULL; tp = tp->next) {
			if (tp2 == NULL) {
				mp2->info.prod->p_locale = tp2 =
					(Module *)xcalloc(sizeof (Module));
				tp2->prev = NULL;
				tp2->head = tp2;
			} else {
				tp2->next = (Module *)xcalloc(sizeof (Module));
				tp2->next->prev = tp2;
				tp2->next->head = tp2->head;
				tp2 = tp2->next;
			}
			tp2->type = tp->type;
			tp2->parent = mp2;
			tp2->info.locale = (Locale *)xcalloc(sizeof (Locale));
			tp2->info.locale->l_locale =
					xstrdup(tp->info.locale->l_locale);
			tp2->info.locale->l_language =
					xstrdup(tp->info.locale->l_language);
			tp2->info.locale->l_selected =
					tp->info.locale->l_selected;
		}
		localize_packages(mp2);
		cat2 = NULL;
		for (cat = mp->info.prod->p_categories; cat != NULL;
							cat = cat->next) {
			if (cat2 == NULL) {
				mp2->info.prod->p_categories = cat2 =
					(Module *)xcalloc(sizeof (Module));
			} else {
				cat2->next =
					(Module *)xcalloc(sizeof (Module));
				cat2 = cat2->next;
			}
			cat2->type = CATEGORY;
			cat2->info.cat = (Category *)xcalloc(sizeof (Category));
			cat2->info.cat->cat_name =
					xstrdup(cat->info.cat->cat_name);
			cat2->parent = mp2;
			tp2 = NULL;
			for (tp = cat->sub; tp != NULL; tp = tp->next) {
				if (tp2 == NULL) {
					cat2->sub = tp2 =
					(Module *)xcalloc(sizeof (Module));
					tp2->prev = NULL;
					tp2->head = tp2;
				} else {
					tp2->next = (Module *)
							xcalloc(sizeof (Module));
					tp2->next->prev = tp2;
					tp2->next->head = tp2->head;
					tp2 = tp2->next;
				}
				tp2->info.mod = (Modinfo *)
					(findnode(mp2->info.prod->p_packages,
						tp->info.mod->m_pkgid))->data;
			}
		}

		mp2->sub = dup_tree(mp->sub, mp2, mp2);
	}

	/* allocate and initialize Media structure */
	new->info.media = (Media *)xcalloc(sizeof (Media));
	new->info.media->med_type = current->info.media->med_type;
	new->info.media->med_status = current->info.media->med_status;
	if (current->info.media->med_device != NULL)
		new->info.media->med_device =
				xstrdup(current->info.media->med_device);
	if (current->info.media->med_dir != NULL)
		new->info.media->med_dir =
				xstrdup(current->info.media->med_dir);
	if (current->info.media->med_volume != NULL)
		new->info.media->med_volume =
				xstrdup(current->info.media->med_volume);

	/* add to end of media chain */
	for (mp = head_ptr; mp->next != NULL; mp = mp->next)
		;
	mp->next = new;
	new->prev = mp;
	media_category(new);
	return (new);
}

/*
 * dup_clstr_tree()
 * Parameters:
 *	cur	-
 *	clstr	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
dup_clstr_tree(Module * cur, Module * clstr)
{
	Module *i;
	Module *k;

	clstr->type = cur->type;
	clstr->info.mod = cur->info.mod;

	for (i = cur->sub; i != NULL; i = i->next) {
		if (clstr->sub == NULL)
			clstr->sub = k = (Module *)xcalloc(sizeof (Module));
		else {
			k->next = (Module *)xcalloc(sizeof (Module));
			k->next->prev = k;
			k = k->next;
		}
		k->parent = clstr;
		k->head = clstr->sub;
		k->type = i->type;
		k->info.mod = i->info.mod;
		if (i->sub != NULL)
			dup_clstr_tree(i, k);
	}
	return;
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * dup_tree()
 * Parameters:
 *	mod	-
 *	newmod	-
 *	parent	-
 * Return:
 * Status:
 *	private
 */
static Module *
dup_tree(Module * mod, Module * newmod, Module *parent)
{
	Module *i, *h, *k;
	Module *j = NULL;

	for (i = mod; i != NULL; i = i->next) {
		if (j == NULL) {
			h = j = (Module *)xcalloc(sizeof (Module));
			j->prev = NULL;
		} else {
			j->next = (Module *)xcalloc(sizeof (Module));
			j->next->prev = j;
			j = j->next;
		}
		j->parent = parent;
		if (i->type == PACKAGE) {
			j->head = h;
			j->type = i->type;
			j->info.mod = (Modinfo *)
				((findnode(newmod->info.prod->p_packages,
						i->info.mod->m_pkgid))->data);
		} else if (i->type == CLUSTER) {
			k = (Module *)((findnode(newmod->info.prod->p_clusters,
						i->info.mod->m_pkgid))->data);
			j->type = k->type;
			j->info.mod = k->info.mod;
			if (i->sub != NULL)
				j->sub = dup_tree(i->sub, newmod, j);
		} else if (i->type == METACLUSTER) {
			j->type = METACLUSTER;
			j->info.mod = dup_modinfo(i->info.mod, (caddr_t)newmod);
			if (i->sub != NULL)
				j->sub = dup_tree(i->sub, newmod, j);
		} else
			return (NULL);
	}
	return (h);
}

/*
 * dup_clstr_list()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
dup_clstr_list(Node * np, caddr_t data)
{
	Module *mp;
	Module *current;
	Node   *np2;

	current = (Module *)np->data;

	mp = (Module *)xcalloc(sizeof (Module));
	mp->type = CLUSTER;
	/*LINTED [alignment ok]*/
	mp->parent = (Module *)data;
	mp->info.mod = dup_modinfo(current->info.mod, data);
	np2 = getnode();
	np2->type = np->type;
	np2->key = xstrdup(mp->info.mod->m_pkgid);
	np2->data = (void *)mp;
	np2->delproc = &free_np_module;    /* set delete function */
	/*LINTED [alignment ok]*/
	(void) addnode(((Module *)data)->info.prod->p_clusters, np2);
	return (SUCCESS);
}

/*
 * dup_list()
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
dup_list(Node * np, caddr_t data)
{
	Modinfo *info;
	info = dup_modinfo((Modinfo*)np->data, data);
	/*LINTED [alignment ok]*/
	add_package((Module *)data, info);
	return (SUCCESS);
}

/*
 * duplicate_depend()
 * Parameters:
 *	depend	-
 * Return:
 * Status:
 *	private
 */
static Depend *
duplicate_depend(Depend * depend)
{
	Depend	*i, *j;
	Depend	*hdr = NULL;

	for (i = depend; i != NULL; i = i->d_next) {
		if (hdr == NULL) {
			hdr = j = (Depend *)xcalloc(sizeof (Depend));
			j->d_prev = NULL;
		} else {
			j->d_next = (Depend *)xcalloc(sizeof (Depend));
			j->d_next->d_prev = j;
			j = j->d_next;
		}
		if (i->d_pkgid != NULL)
			j->d_pkgid = xstrdup(i->d_pkgid);
		if (i->d_version != NULL)
			j->d_version = xstrdup(i->d_version);
		if (i->d_arch != NULL)
			j->d_arch = xstrdup(i->d_arch);
	}

	return (hdr);
}

/*
 * dup_modinfo()
 * Parameters:
 *	mod	-
 *	data	-
 * Return:
 *
 * Status:
 *	private
 */
static Modinfo *
dup_modinfo(Modinfo * mod, caddr_t data)
{
	Modinfo *mp;

	mp = (Modinfo *)xcalloc(sizeof (Modinfo));
	mp->m_order = mod->m_order;
	mp->m_status = mod->m_status;
	mp->m_flags = mod->m_flags;
	if (mod->m_shared == SPOOLED_NOTDUP)
		mp->m_shared = SPOOLED_DUP;
	else if (mod->m_shared == NOTDUPLICATE)
		mp->m_shared = DUPLICATE;
	else
		mp->m_shared = mod->m_shared;
	mp->m_refcnt = mod->m_refcnt;
	mp->m_sunw_ptype = mod->m_sunw_ptype;
	if (mod->m_pkgid != NULL)
		mp->m_pkgid = xstrdup(mod->m_pkgid);
	if (mod->m_pkginst != NULL)
		mp->m_pkginst = xstrdup(mod->m_pkginst);
	if (mod->m_pkg_dir != NULL)
		mp->m_pkg_dir = xstrdup(mod->m_pkg_dir);
	if (mod->m_name != NULL)
		mp->m_name = xstrdup(mod->m_name);
	if (mod->m_vendor != NULL)
		mp->m_vendor = xstrdup(mod->m_vendor);
	if (mod->m_version != NULL)
		mp->m_version = xstrdup(mod->m_version);
	if (mod->m_prodname != NULL)
		mp->m_prodname = xstrdup(mod->m_prodname);
	if (mod->m_prodvers != NULL)
		mp->m_prodvers = xstrdup(mod->m_prodvers);
	if (mod->m_arch != NULL)
		mp->m_arch = xstrdup(mod->m_arch);
	if (mod->m_desc != NULL)
		mp->m_desc = xstrdup(mod->m_desc);
	if (mod->m_category != NULL)
		mp->m_category = xstrdup(mod->m_category);
	if (mod->m_instdate != NULL)
		mp->m_instdate = xstrdup(mod->m_instdate);
	if (mod->m_locale != NULL)
		mp->m_locale = xstrdup(mod->m_locale);
	if (mod->m_l10n_pkglist != NULL)
		mp->m_l10n_pkglist =
				xstrdup(mod->m_l10n_pkglist);
	mp->m_pdepends = duplicate_depend(mod->m_pdepends);
	mp->m_idepends = duplicate_depend(mod->m_idepends);
	mp->m_rdepends = duplicate_depend(mod->m_rdepends);
	if (mod->m_instances != NULL) {
		dup_list(mod->m_instances, data);
	}
	/*
	 * Duplicate all of the patch information
	 */
	if (mod->m_patchid != NULL)
		mp->m_patchid = xstrdup(mod->m_patchid);
	mp->m_patchof = mod->m_patchof;
	if (mod->m_next_patch != NULL) {
		dup_list(mod->m_next_patch, data);
	}
	if (mod->m_newarch_patches != NULL)
		mp->m_newarch_patches =
		    dup_patchnum_list(mod->m_newarch_patches);

	if (mod->m_basedir != NULL)
		mp->m_basedir = xstrdup(mod->m_basedir);
	if (mod->m_instdir != NULL)
		mp->m_instdir = xstrdup(mod->m_instdir);
	mp->m_pkg_hist = mod->m_pkg_hist;
	if (mp->m_pkg_hist)
		mp->m_pkg_hist->ref_count++;
	return (mp);
}

/*
 * doumount()
 *	Do the actual unmounting (and ejecting)
 * Parameters:
 *	device	-
 *	dir	-
 *	eject	-
 * Return:
 *	errno	-
 *	SUCCESS
 * Status:
 *	private
 */
/*ARGSUSED*/
static int
doumount(char * device, char * dir, int eject)
{

	if (umount_fs(dir) < 0)
		return (errno);
	if (eject)
		doeject(device);
	return (SUCCESS);
}

/*
 * doeject()
 *	Eject the raw media device. Currently supports CDROM devices ONLY.
 *	Device name formats accepted are:
 * 		/dev/dsk/<anything>, /dev/rdsk/<anything>
 *		/dev/<anything>,
 *		/dev/r<anything>	<-- compatibility device names
 * Parameters:
 *	device	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
doeject(char * device)
{
	int	fd;
	static char rawdevice[MAXPATHLEN];
	char	*cp;

	/* Eject ioctl only supported on raw devices.  */
	if (strncmp(device, "/dev/dsk/", 9) == 0)
		sprintf(rawdevice, "/dev/r%s", strstr(device, "dsk/"));
	else if (strncmp(device, "/dev/rdsk/", 10) == 0)
		sprintf(rawdevice, "%s", device);
	else if ((cp = strrchr(device,  '/')) == (char *)NULL)
		return;
	else {
		cp++;
		if (*cp == 'r')
			sprintf(rawdevice, "/dev/%s", cp);
		else
			sprintf(rawdevice, "/dev/r%s", cp);
	}

	fd = open(rawdevice, O_RDONLY);
	if (fd < 0)
		return;

	(void) ioctl(fd, CDROMEJECT);
	(void) close(fd);
	return;
}

/*
 * read_product()
 *	Reads the products available from the specified media location.
 *	If no location is specified, the current media location (see
 *	get_current_media) is used.  Returns a linked list pointing to
 *	the products available.  If the media doesn't contain a .cdtoc file,
 *	a NULL product structure is created.
 * Parameters:
 *	mod	-
 * Return:
 * Status:
 *	private
 */
static int
read_product(Module * mod)
{
	char	name[MAXPATHLEN];
	FILE	*fp;
	Product	*prod = (Product *)0;
	Module	**mpp;
	Module	*prev = NULL;
	Module	*media;
	char	buf[BUFSIZ];
	char	buf1[BUFSIZ];
	char	*cp;

	/* Get correct module pointer */
	if (mod == (Module *)NULL)
		media = get_current_media();
	else
		media = mod;

	if (media == (Module *)NULL)
		return (ERR_NOMEDIA);

	/* set up to create new module structures and add them to the list */
	if (!media->info.media->med_dir)
		return (ERR_UNMOUNT);
	if (media->info.media->med_type == INSTALLED ||
	    media->info.media->med_type == INSTALLED_SVC) {
		sprintf(buf, "%s/%s", get_rootdir(), media->info.media->med_dir);
		if (path_is_readable(buf) == FAILURE)
			return (ERR_UNMOUNT);
		(void) sprintf(name, "%s/.cdtoc", buf);
	} else {
		if (path_is_readable(media->info.media->med_dir) == FAILURE)
			return (ERR_UNMOUNT);
		(void) sprintf(name, "%s/.cdtoc", media->info.media->med_dir);
	}

	mpp = &(media->sub);

	/* set up null structure if there is no .cdtoc */
	if ((fp = fopen(name, "r")) == (FILE *)0) {
		prod = (Product *)xcalloc(sizeof (Product));
		prod->p_pkgdir = xstrdup(media->info.media->med_dir);
		prod->p_current_view = prod;
		prod->p_categories = (Module *) xcalloc(sizeof (Module));
		prod->p_categories->type = CATEGORY;
		prod->p_categories->info.cat =
					(Category *) xcalloc(sizeof (Category));
		prod->p_categories->info.cat->cat_name =
			xstrdup((char *)dgettext(
							"SUNW_INSTALL_SWLIB",
							"All Software"));

		*mpp = (Module *)xcalloc(sizeof (Module));
		(*mpp)->info.prod = prod;
		prod->p_categories->parent = *mpp;
		(*mpp)->type = NULLPRODUCT;
		(*mpp)->sub = (Module *)NULL;
		(*mpp)->prev = NULL;
		(*mpp)->head = media->sub;
		(*mpp)->next = (Module *)NULL;
		(*mpp)->parent = media;
		return (SUCCESS);
	}
	/*
	 * parse out Product Name, Version, and package directory from cdtoc
	 *
	 * PRODNAME, PRODVERS, PRODDIR
	 */

	while (fgets(buf, BUFSIZ, fp)) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		else if (strncmp(buf, "PRODNAME=", 9) == 0) {
			/* allocate data structure if needed, then clear */
			if (!prod)
				prod = (Product *)xcalloc(sizeof (Product));
			if (prod->p_name && prod->p_pkgdir && prod->p_version) {
				*mpp = (Module *)xcalloc(sizeof (Module));
				(*mpp)->info.prod = prod;
				(*mpp)->type = PRODUCT;
				(*mpp)->sub = (Module *)NULL;
				(*mpp)->head = mod->sub;
				(*mpp)->next = (Module *)NULL;
				(*mpp)->parent = media;
				(*mpp)->prev = prev;
				prod->p_current_view = prod;
				prev = *mpp;
				mpp = &((*mpp)->next);
				prod = (Product *)xcalloc(sizeof (Product));
			}

			prod->p_name = (char *)xstrdup(get_value(buf, '='));
			prod->p_current_view = prod;
			prod->p_categories = (Module *) xcalloc(sizeof (Module));
			prod->p_categories->type = CATEGORY;
			prod->p_categories->info.cat =
					(Category *) xcalloc(sizeof (Category));
			prod->p_categories->info.cat->cat_name =
					xstrdup((char *)dgettext(
							"SUNW_INSTALL_SWLIB",
							"All Software"));
		} else if (!prod)  /* ignore unless PRODNAME was given */
			continue;
		else if (strncmp(buf, "PRODID=", 7) == 0)
			prod->p_id = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PRODVERS=", 9) == 0)
			prod->p_version = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PRODDIR=", 8) == 0) {
			/* relative or absolute path ? */
			if ((cp = (char *)get_value(buf, '='))[0] == '/')
				prod->p_pkgdir = (char *)xstrdup(cp);
			else {
				sprintf(buf1, "%s/%s",
						media->info.media->med_dir, cp);
				prod->p_pkgdir = (char *)xstrdup(buf1);
			}
			if (cp = get_value(cp, '_'))
				cp = get_value(cp, '_');
			if (cp && *cp)
				prod->p_rev = xstrdup(cp);
			else
				prod->p_rev = xstrdup("0");
		}

	}
	if (prod->p_name && prod->p_pkgdir && prod->p_version) {
		*mpp = (Module *)xcalloc(sizeof (Module));
		(*mpp)->info.prod = prod;
		(*mpp)->type = PRODUCT;
		(*mpp)->sub = (Module *)NULL;
		(*mpp)->prev = media;
		(*mpp)->head = mod->sub;
		(*mpp)->next = (Module *)NULL;
		(*mpp)->parent = media;
		(*mpp)->prev = prev;
		prod->p_current_view = prod;
		prod->p_categories->parent = *mpp;
		prod = (Product *)NULL;
	}
	if (prod)
		free(prod);

	(void) fclose(fp);
	return (SUCCESS);
}


static char *
canonical_path(char *dir, char *dir2)
{
	char	dirbuf[MAXPATHLEN];
	char	*cp;

	/* first convert relative paths to absolute paths */
	if (dir[0] != '/') {
		(void) getcwd(dirbuf, sizeof (dirbuf));
		if (dirbuf == (char *)0)
			return (NULL);
		(void) strcat(dirbuf, "/");
		(void) strcat(dirbuf, dir);
		cp = dirbuf;
	} else
		cp = dir;

	/* Now resolve all symlinks, etc. */

	/*
	 * if realpath() fails because the path doesn't exist, just
	 * return the path, since it's ok for the file not to exist yet.
	 */
	if (realpath(cp, dir2) == NULL)
		strcpy(dir2, cp);
	return (dir2);
}

static void
canonize_dir(char **dirpp)
{
	char	dirbuf[MAXPATHLEN];

	if (canonical_path(*dirpp, dirbuf) == NULL)
		return;
	if (strcmp(*dirpp, dirbuf) != 0) {
		free(*dirpp);
		*dirpp = xstrdup(dirbuf);
	}
}

/*
 * find_service_media()
 *	Traverse the media Modinfo list looking for the specified service,
 *	(such as "Solaris_2.4").
 * Parameters:
 *	svc	- string containing name of the service
 * Return:
 *	NULL	 - no matching media found in tree
 *	Module * - pointer to matching media module
 * Status:
 *	library-shared
 */
Module *
find_service_media(char * svc)
{
	Module	*mp;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("find_service_media");
#endif

	/* find the existing media structure if it exists */
	for (mp = head_ptr; mp != (Module *) NULL; mp = mp->next) {
		if (mp->info.media->med_type == INSTALLED_SVC &&
		    strcmp(mp->info.media->med_volume, svc) == 0)
			return (mp);
	}
	return (NULL);
}

/*
 * load_patch_space_requirements()
 *	Look for files of the form /tmp/patch_space_reqd.* .  If any
 *	such files are found, read them and build the patch_space_reqd
 *	structures for them and queue them to patch_space_head.
 * 	This information will be fed into the space roll-ups so that
 *	space required for patches will be allocated (in the case of
 *	initial install) or verified to be available (in the case of
 *	upgrade).
 * Parameters:
 *	None
 * Return:
 *	a linked list of patch_space_reqd structures.
 * Status:
 *	Private
 */
static struct  patch_space_reqd *
load_patch_space_requirements(void)
{
	static char patch_space_file[] = "patch_space_reqd.";
	char	path[MAXPATHLEN];
	DIR	*dirp;
	struct dirent	*dp;
	struct patch_space_reqd *patch_head = NULL;
	struct patch_space_reqd *psr = NULL;
	FILE	*fp;
	char	buf[BUFSIZ + 1];
	char	 key[BUFSIZ];
	int	 len;
	char	*cp;

	if ((dirp = opendir("/tmp")) == NULL) {
		return (NULL);
	}

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		if (strncmp(patch_space_file, dp->d_name,
		    strlen(patch_space_file)) != 0)
			continue;

		(void) sprintf(path, "/tmp/%s", dp->d_name);

		if ((fp = fopen(path, "r")) == (FILE *)NULL)
			continue;

		while (fgets(buf, BUFSIZ, fp)) {
			buf[strlen(buf) - 1] = '\0';
			if (buf[0] == '#' || buf[0] == '\n')
				continue;

			if ((cp = strchr(buf, '=')) == NULL) {
				process_patch_space_entry(buf, psr);
				continue;
			}
			len = cp - buf;
			strncpy(key, buf, len);
			key[len] = '\0';
			cp++;	/* cp now points to string after '=' */

			if (strcmp(key, "ARCH") == 0) {
				if (psr)
					link_to((Item **)&patch_head,
					    (Item *)psr);
				psr = (struct patch_space_reqd *)xcalloc
				    ((size_t) sizeof (struct patch_space_reqd));
				psr->patsp_arch = xstrdup(cp);
			}
		}
		fclose(fp);
	}
	(void) closedir(dirp);

	if (psr)
		link_to((Item **)&patch_head, (Item *)psr);

	return (patch_head);
}

static void
process_patch_space_entry(char *buf, struct patch_space_reqd *psr)
{
	char	s1[BUFSIZ + 1], pkgid[256];
	ulong	d1, d2;
	struct	patdir_entry *pde;
	int	num_match;

	if (psr == NULL)
		return;

	num_match = sscanf(buf, "%s %lu %lu %s", s1, &d1, &d2, pkgid);

	if (num_match < 3)
		return;

	if (s1[0] != '\0') {
		if (strcmp(s1, "SPOOLED") != 0 && s1[0] != '/')
			return;
		pde = (struct patdir_entry *)xcalloc((size_t) sizeof
		    (struct patdir_entry));
		if (strcmp(s1, "SPOOLED") == 0)
			pde->patdir_spooled = 1;
		else
			pde->patdir_dir = xstrdup(s1);
		pde->patdir_kbytes = d1;
		pde->patdir_inodes = d2;
		if (num_match > 3)
			pde->patdir_pkgid = xstrdup(pkgid);
		link_to((Item **)&psr->patsp_direntry, (Item *)pde);
	}
}

static struct patch_num *
dup_patchnum_list(struct patch_num *pnumlist)
{
	struct patch_num *pnum;
	struct patch_num *pnum_head = NULL;


	for ( ; pnumlist != NULL; pnumlist = pnumlist->next) {
		pnum = (struct patch_num *)xcalloc((size_t) sizeof (struct
		    patch_num));
		pnum->patch_num_id = xstrdup(pnumlist->patch_num_id);
		pnum->patch_num_rev_string =
		    xstrdup(pnumlist->patch_num_rev_string);
		pnum->patch_num_rev = pnumlist->patch_num_rev;
		link_to((Item **)&pnum_head, (Item *)pnum);
	}
	return (pnum_head);
}
