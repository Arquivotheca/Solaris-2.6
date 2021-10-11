/*
 * Copyright (c) 1992 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#ifndef lint
#ident	"@(#)subr.c 1.9 93/06/18"
#endif

#include "defs.h"
#include "ui.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

int
path_is_writable(char *fn)
{
	return ((access(fn, F_OK) == 0) ? SUCCESS : FAILURE);
}

int
path_is_block_device(path)
	char	*path;
{
	struct stat st;

	if (path[0] == '\0') {
		errno = ENOENT;
		return (FAILURE);
	}

	if (stat(path, &st) < 0)
		return (FAILURE);

	if (S_ISBLK(st.st_mode) == 0) {
		errno = ENOTBLK;
		return (FAILURE);
	}

	return (SUCCESS);
}

/*
 * Return SUCCESS if the file system on which "path"
 * resides is local, i.e., exportable.
 */
int
path_is_local(path)
	char	*path;
{
	struct statvfs stat;

	if (statvfs(path, &stat) < 0)
		return (FAILURE);
	/*
	 * We know these two types are ok
	 */
	if (strcmp(stat.f_basetype, MNTTYPE_UFS) == 0 ||
	    strcmp(stat.f_basetype, MNTTYPE_HSFS) == 0)
		return (SUCCESS);
	/*
	 * We know this type is not
	 */
	if (strcmp(stat.f_basetype, MNTTYPE_NFS) == 0)
		return (FAILURE);

	return (FAILURE);	/* seems safest... */
}

/*
 * Create as many components as needed in order
 * to construct path.  If a cookie is specified,
 * create a file by that name in any directory we
 * create so we know what to clean up later.
 *
 * Returns SUCCESS on success, FAILURE on failure.
 */
int
path_create(char *path,		/* path to construct */
	char	*cookie)	/* "we created it" flag */
{
	struct stat st;
	char	partial[MAXPATHLEN];
	char	flagfile[MAXPATHLEN];
	char	*cp, *last;
	int	creating, unwind, status;
	int	fd;

	creating = 0;
	unwind = 0;
	status = 0;

	(void) strcpy(partial, path);
	last = partial;

	while (last && *last && status == 0) {
		cp = strchr(&last[1], '/');
		if (cp != (char *)0)
			*cp = '\0';	/* temporarily terminate */
		last = cp;
		if (!creating) {
			status = stat(partial, &st);
			if (status < 0) {
				/*
				 * stat failed -- if because the
				 * component doesn't exist we'll
				 * start mkdir'ing here.  Other
				 * errors cause us to exit the loop.
				 */
				if (errno == ENOENT)
					creating = 1;
			} else if (!S_ISDIR(st.st_mode))
				/*
				 * Can't have an intermediate
				 * component that's not a directory.
				 */
				status = -1;
			else
				/*
				 * stat ok, directory ok
				 */
				if (cp != (char *)0)
					*cp = '/';	/* un-terminate */
		}
		/*
		 * Need 2nd test because we may
		 * have just turned on creation.
		 */
		if (creating) {
			status = mkdir(partial, (mode_t)0755);
			if (status < 0) {
				unwind = 1;
				status = -1;
			} else {
				/*
				 * If specified, install the cookie in
				 * any directory we create so we know what
				 * to remove when we're done.  Failure at
				 * this point isn't fatal, it just means
				 * we won't remove the directory later.
				 */
				(void) sprintf(flagfile, "%s/%s",
					partial, cookie);
				fd = open(flagfile,
					O_WRONLY|O_CREAT, (mode_t)0600);
				if (fd >= 0)
					(void) close(fd);
				if (cp != (char *)0)
					*cp = '/';	/* un-terminate */
			}
		}
	}

	if (creating && unwind)
		(void) path_remove(partial, cookie);

	return (status < 0 ? FAILURE : SUCCESS);
}

/*
 * Remove part or all of a path, controlled by
 * the presence of the cookie.  If specified,
 * remove all components of the path containing
 * a file matching the cookie.  If not specified,
 * remove only the last component of the path.
 *
 * Returns 0 on success, -1 on failure.
 */
int
path_remove(char *path,		/* path to remove */
	char	*cookie)	/* "we created it" flag */
{
	struct stat st;
	char	partial[MAXPATHLEN];
	char	flagfile[MAXPATHLEN];
	char	*last;
	int	status;

	if (!cookie)
		return (rmdir(path));

	status = 0;

	(void) strcpy(partial, path);
	last = partial;

	while (last && status == 0) {
		last = strrchr(partial, '/');
		/*
		 * Check for the flag file -- if found
		 * assume we created the component and
		 * remove both flag file and directory.
		 */
		(void) sprintf(flagfile, "%s/%s", partial, cookie);
		status = stat(flagfile, &st);
		if (status == 0) {
			(void) unlink(flagfile);
			status = rmdir(partial);
		}
		if (last != (char *)0)
			*last = '\0';	/* terminate */
	}
	return (status < 0 ? FAILURE : SUCCESS);
}

static SWM_mode mode = MODE_INSTALL;	/* current operating mode */
static SWM_view view = VIEW_NATIVE;	/* current view of distribution */

SWM_mode
#ifdef __STDC__
get_mode(void)
#else
get_mode()
#endif
{
	return (mode);
}

void
set_mode(newmode)
	SWM_mode newmode;
{
	if (mode != newmode)
		mode = newmode;
}

SWM_view
#ifdef __STDC__
get_view(void)
#else
get_view()
#endif
{
	return (view);
}

void
set_view(newview)
	SWM_view newview;
{
	if (view != newview)
		view = newview;
}

static Module	*installed_media;	/* installed service we're modifying */

void
set_installed_media(Module *media)
{
	installed_media = media;
}

Module *
get_installed_media(void)
{
	return (installed_media);
}

char *
get_full_name(Module *mod)
{
	static char name[1024];

	if (mod == (Module *)0)
		return ((char *)0);

	switch (mod->type) {
	case MEDIA:
		if (mod->info.media->med_type == INSTALLED)
			(void) strcpy(name, gettext("Native Installation"));
		else
			(void) strcpy(name, mod->info.media->med_dir);
		break;
	case PRODUCT:
		(void) sprintf(name, "%s %s",
			mod->info.prod->p_name, mod->info.prod->p_version);
		break;
	case LOCALE:
		(void) strcpy(name, mod->info.locale->l_locale);
		break;
	case CATEGORY:
		(void) strcpy(name, mod->info.cat->cat_name);
		break;
	default:
		(void) strcpy(name, mod->info.mod->m_name);
		break;
	}
	return (name);
}

char *
get_short_name(Module *mod)
{
	static char name[1024];
	char	*cp;

	if (mod == (Module *)0)
		return ((char *)0);

	switch (mod->type) {
	case MEDIA:
		if (mod->info.media->med_type == INSTALLED)
			(void) strcpy(name, gettext("Native Installation"));
		else {
			cp = strrchr(mod->info.media->med_dir, '/');
			if (cp == (char *)0)
				cp = mod->info.media->med_dir;
			else if (cp != mod->info.media->med_dir)
				cp++;
			(void) strcpy(name, cp);
		}
		break;
	case PRODUCT:
		(void) strcpy(name, mod->info.prod->p_name);
		break;
	case LOCALE:
		(void) strcpy(name, mod->info.locale->l_locale);
		break;
	case CATEGORY:
		(void) strcpy(name, mod->info.cat->cat_name);
		break;
	default:
		(void) strcpy(name, mod->info.mod->m_pkgid);
		break;
	}
	return (name);
}

/*
 * Only do category matching at the product level,
 * i.e., show all products on the media matching
 * the specified category.  Once below the product
 * level, we show everything.
 */
int
in_category(Module *category, Module *mod)
{
	Module  *cat, *prod;
	int	incat = FALSE;

	if (category == (Module *)0)
		incat = TRUE;
	else if (mod->type == PRODUCT) {
		if (mod->info.prod->p_categories == (Module *)0)
			incat = TRUE;
	} else if (mod->type == UNBUNDLED_4X) {
		if (mod->parent->info.prod->p_categories == (Module *)0)
			incat = TRUE;
	} else
		incat = TRUE;

	if (incat == FALSE) {
		prod = get_parent_product(mod);

		if (prod != (Module *)0) {
			for (cat = prod->info.prod->p_categories;
			    cat != (Module *)0 && incat == FALSE;
			    cat = cat->next) {
				if (strcmp(cat->info.cat->cat_name,
				    category->info.cat->cat_name) == 0)
					incat = TRUE;
			}
		} else
			incat = TRUE;
	}
	return (incat);
}

static Module *source_media = (Module *)0;

void
set_source_media(Module *media)
{
	source_media = media;
}

Module *
get_source_media(void)
{
	return (source_media);
}

Module *
get_parent_product(Module *mod)
{
	Module	*parent = mod;

	while (parent != (Module *)0 &&
	    parent->type != PRODUCT && parent->type != NULLPRODUCT)
		parent = parent->parent;

	return (parent);
}

/*
 * Search for the first icon
 * associated with any of our
 * direct children
 */
File *
get_module_icon(Module *mod)
{
	Module	*sub;
	File	*icon = (File *)0;

	if (mod->type != PRODUCT && mod->info.mod->m_icon != (File *)0)
		return (mod->info.mod->m_icon);

	for (sub = mod->sub; sub; sub = sub->next) {
		if (sub->type != PRODUCT && sub->type != NULLPRODUCT) {
			icon = sub->info.mod->m_icon;
			if (icon != (File *)0)
				break;
		}
	}
	return (icon);
}
