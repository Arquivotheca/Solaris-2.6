#ifndef lint
#pragma ident "@(#)soft_prod.c 1.9 96/08/05 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc. All Rights Reserved.
 */
#include "spmisoft_lib.h"

#include <dirent.h>
#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Public Function Prototypes */

char *   	swi_get_clustertoc_path(Module *);
void		swi_media_category(Module *);

/* Library Function Prototypes */

int		load_all_products(Module *, int);
int		load_pkginfo(Module *, char *, int);
int		load_clusters(Module *, char *);
char *		get_value(char *, char);
void		promote_packages(Module *, Module *, Module *);

/* Local Function Prototypes */

static Module	*load_subclusters(FILE *, char *, Module *, Module *);
static void	promote_clusters(Module *, Module *);
static int	is4x(char *, char *);
static void	load_package(Module *, char *);
static int	load_product(Module *, int);
static char	*get_packagetoc_path(Module *);
static char	*get_orderfile_path(Module *);
static int	load_packagetoc(Module *);
static void	add_to_category(Module *, Modinfo *info);
static int	ispackage(char *, char *);
static void	arch_setup(Module *);
static int	arch_setup_package(Node *, caddr_t);
static int	load_4x(Module *, char *);
static struct patch_num *alloc_newarch_patchlist(char *);
static Node *	add_null_pkg(Module *, char *);

/* Globals and Externals */

extern int default_ptype;
static struct ptype known_ptypes[] = {
	{"root", 4, PTYPE_ROOT},
	{"kvm",	 3, PTYPE_KVM},
	{"usr",  3, PTYPE_USR},
	{"ow",	 2, PTYPE_OW},
	{"",	 0, '\0'}
};

static char	install_lib_l10n_name[] = "SUNW_INSTALL_SWLIB";

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * get_clustertoc_path()
 *	Returns a pointer to the clustertoc file associated with the product
 *	'mod'. Path returned may or may not exist.
 * Parameters:
 *	mod	- pointer to product module structure
 * Return:
 *	NULL	- 'mod' is not a valid product module pointer, or, if NULL,
 *		  there is no current product
 *	char *	- pathname to clustertoc file
 * Status:
 *	Public
 */
char *
swi_get_clustertoc_path(Module * mod)
{
	static char	 cluster_toc[MAXPATHLEN];
	Module		*prod;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_clustertoc_path");
#endif

	if (mod == (Module *) NULL)
		prod = get_current_product();
	else
		prod = mod;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return (NULL);

	(void) sprintf(cluster_toc, "%s/locale/%s/%s",
			prod->info.prod->p_pkgdir,
			get_current_locale(), CLUSTER_TOC_NAME);
	/*
	 * make sure this file is readable, if it isn't, return path to
	 * the default .clustertoc (.../locale/C/.clustertoc)
	 */
	if (path_is_readable(cluster_toc) == FAILURE) {
		(void) sprintf(cluster_toc, "%s/locale/%s/%s",
			prod->info.prod->p_pkgdir, get_default_locale(),
			CLUSTER_TOC_NAME);
	}
	/*
	 * if .../locale/C/.clustertoc isn't readable, look for clustertoc
	 * on the top level.  (Provides backwards compatibility w/Jupiter
	 * and x86 developers release CD's
	 */
	if (path_is_readable(cluster_toc) == FAILURE) {
		(void) sprintf(cluster_toc, "%s/%s", prod->info.prod->p_pkgdir,
				CLUSTER_TOC_NAME);
	}

	return (cluster_toc);
}

/*
 * media_category()
 * Parameters:
 *	mod	- valid media module pointer
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_media_category(Module * mod)
{
	Module *list = NULL;
	Module *cp, *catp, *catp2;
	Module   *mp, *mp2, *mp3;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("media_category");
#endif

	if (mod->info.media->med_cat == NULL) {
		mod->info.media->med_cat = (Module *)xcalloc(sizeof (Module));
		mod->info.media->med_cat->prev = NULL;
		mod->info.media->med_cat->type = CATEGORY;
		mod->info.media->med_cat->parent = mod;
		mod->info.media->med_cat->next = (Module *)NULL;
		mod->info.media->med_cat->info.cat =
					(Category *)xcalloc(sizeof (Category));
		mod->info.media->med_cat->info.cat->cat_name  =
			xstrdup((char *)
			dgettext("SUNW_INSTALL_SWLIB", "All Software"));
	}
	for (mp = mod->sub; mp; mp = mp->next) {
		if (mp->type != PRODUCT && mp->type != NULLPRODUCT)
			continue;
		list = mp->info.prod->p_categories;
		for (cp = list->next; cp; cp = cp->next) {
			catp =  mod->info.media->med_cat;
			for (catp2 = catp->next; catp2; catp2 = catp2->next) {
				if (strcmp(catp2->info.cat->cat_name,
						cp->info.cat->cat_name) == 0)
					break;
				catp = catp2;
			}
			if (catp2 == (Module *)0) {
				catp2 = catp->next =
				    (Module *)xcalloc(sizeof (Module));
				catp2->prev = catp;
				catp2->type = CATEGORY;
				catp2->parent = mod;
				catp2->head = mod->info.media->med_cat;
				catp2->next = (Module *)NULL;
				catp2->info.cat =
				    (Category *)xcalloc(sizeof (Category));
				catp2->info.cat->cat_name  =
				    xstrdup(cp->info.cat->cat_name);
			}

			/* add to the end of the chain */
			mp3 = NULL;
			for (mp2 = catp2->sub; mp2; mp2 = mp2->next)
				mp3 = mp2;

			mp2 = (Module *)xcalloc(sizeof (Module));
			if (mp3 == NULL)
				catp2->sub = mp2;
			else
				mp3->next = mp2;
			mp2->next = (Module *)NULL;
			mp2->type = PRODUCT;
			mp2->prev = mp3;
			mp2->parent = catp2;
			mp2->info.mod = mp->info.mod;

		}
	}
	return;
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * load_all_products()
 *	Traverse the media product chain for 'media' (or the current media if
 *	'media' is NULL), and load the assocated products.into a linked module
 *	list.
 * Parameters:
 *	media		-  pointer to media module associated with the product
 *			   (if NULL, the current media is used)
 *	use_dotfiles -  flag to load_product() to indicate if the .packagetoc
 *			and .order files are to be used during the load
 * Return:
 *	SUCCESS		- all products loaded successfully
 *	ERR_INVALIDTYPE	- 'mod' is not a media module, or is NULL and there
 *			  is no current media
 *	ERR_NOMEDIA	- the media has no product sub-tree
 *	other		- return values from load_product() if one of the
 *			  products did not load successfully
 * Status:
 *	semi-private (internal library use only)
 */
int
load_all_products(Module *media, int use_dotfiles)
{
	Module * mod;
	int	 return_val = SUCCESS;
	int	 tmp;

	if (media != NULL)
		mod = media;
	else
		mod = get_current_media();

	if (!mod || mod->type != MEDIA)
		return (ERR_INVALIDTYPE);

	if (mod->sub == NULL)
		return (ERR_NOMEDIA);

	for (mod = mod->sub; mod != NULL; mod = mod->next) {
		if ((tmp = load_product(mod, use_dotfiles)) != SUCCESS)
			return_val = tmp;
	}

	return (return_val);
}




/*
 * load_clusters()
 *	Load in all clusters and top-level clusters (Meta-clusters). Require
 *	all clusters to be defined before any meta-clusters. If this condition
 *	is not met, the parsing and construction of the software hierarchy
 *	will fail ungracefully.
 * Parameters:
 *	prod	   -
 *	clustertoc -
 * Return:
 *	ERR_NOFILE -
 * Status:
 *	semi-private (internal library use only)
 */
int
load_clusters(Module * prod, char * clustertoc)
{
	FILE		*fp;
	char		buf[BUFSIZ + 1];
	Node		*np;
	Modinfo		*info;
	Module		*hdclst, *curclst,	/* ptrs to regular clusters */
			*hdmeta, *curmeta;	/* ptrs to meta-clusters */
	Module		*current;
	Module		 tmp;

	(void) memset((char *)&tmp, 0, sizeof (Module));

	hdclst = curclst = hdmeta = curmeta = (Module *) NULL;

	if ((fp = fopen(clustertoc, "r")) == (FILE *) NULL) {
		promote_packages(prod, &tmp, NULL);
		prod->sub = tmp.next;
		return (ERR_NOFILE);
	}
	/* make new a new hashlist for the clusters */
	prod->info.prod->p_clusters = getlist();

	while (fgets(buf, BUFSIZ, fp))
	{
		buf[strlen(buf) - 1] = '\0';

		if (buf[0] == '#' || buf[0] == '\n' || strlen(buf) == 0)
			continue;
		else if ((strncmp(buf, "CLUSTER=", 8) == 0) ||
				(strncmp(buf, "METACLUSTER=", 12) == 0)) {
			if (strncmp(buf, "CLUSTER=", 8) == 0) {
				if (curclst) {
					curclst->next =
					(Module *) xcalloc(sizeof (Module));
					curclst->next->prev = curclst;
					curclst = curclst->next;
				} else {
					hdclst = curclst =
					(Module *) xcalloc(sizeof (Module));
				}
				current = curclst;
				current->type = CLUSTER;
				current->head = hdclst;
			} else {
				if (curmeta) {
					curmeta->next =
					(Module *) xcalloc(sizeof (Module));
					curmeta->next->prev = curmeta;
					curmeta = curmeta->next;
				} else {
					hdmeta = curmeta =
					(Module *) xcalloc(sizeof (Module));
				}
				current = curmeta;
				current->type = METACLUSTER;
				current->head = hdmeta;
				current->parent = prod;
			}
			current->next = (Module *) NULL;
			info = (Modinfo *) xcalloc(sizeof (Modinfo));
			current->info.mod = info;
			info->m_pkgid = (char *)xstrdup(get_value(buf, '='));
		}else if (strncmp(buf, "NAME=", 5) == 0)
			info->m_name = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "DESC=", 5) == 0)
			info->m_desc = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "VENDOR=", 7) == 0)
			info->m_vendor = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "VERSION=", 8) == 0)
			info->m_version = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "SUNW_CSRMEMBER", 14) == 0) {
			current->sub =
				load_subclusters(fp, buf, current, prod);
			np = getnode();
			np->key = xstrdup(info->m_pkgid);  /* make modID key */
			np->delproc = &free_np_module;
			current->sub = sort_modules(current->sub);
			if (current->type == CLUSTER) {
				np->data = (char *) curclst;
			} else
				np->data = (char *) curmeta;
			/*
			 * save cluster for later lookup since a cluster may be
			 * a subcluster of several clusters
			 */
			(void) addnode(prod->info.prod->p_clusters, np);

		} else
			return (ERR_BADENTRY);
	}
	(void) fclose(fp);

	/* put module hierarchy into product structure */
	prod->sub = hdmeta;
	if (curclst == (Module *) NULL) {
		promote_packages(prod, &tmp, NULL);
		if (hdmeta == (Module *) NULL)
			prod->sub = tmp.next;
	} else
		promote_packages(prod, curclst, hdclst);

	return (SUCCESS);
}

/*
 * promote_packages()
 *	Find each unselected package and add to list of clusters for
 *	consistency in displaying
 * Parameters:
 *	prod	-
 *	curclst	-
 *	hdclst	-
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
promote_packages(Module * prod, Module * curclst, Module * hdclst)
{
	Node	*np;
	Module	*mod;
	Modinfo	*info;
	List	*packages;

	if (prod->info.prod->p_packages == NULL)
		return;

	packages = prod->info.prod->p_packages;
	np = (Node *) packages->list;
	mod = curclst;
	for (np = np->next; np != packages->list; np = np->next) {
		info = (Modinfo *) np->data;

		if (info->m_status == SELECTED) {
			/* it's in a cluster, unselect... */
			info->m_status = UNSELECTED;
		} else {
			/*
			 * not in a cluster, fit into hierarchy as a cluster
			 * if it isn't a l10n package.
			 */
			if (info->m_loc_strlist == NULL) {
				curclst->next = (Module *)
						xcalloc(sizeof (Module));
				curclst->next->prev = curclst;
				curclst->next->head = curclst->head;
				curclst->next->parent = curclst->parent;
				curclst = curclst->next;
				curclst->type = PACKAGE;
				curclst->info.mod = info;
				curclst->next = (Module *) NULL;
			}
		}
	}
	if (hdclst == NULL)
		promote_clusters(prod, mod->next);
	else
		promote_clusters(prod, hdclst);
	return;
}

/*
 * load_pkginfo()
 *	If the pkginfo and pkgmap files are consistent with a valid
 *	package, create a new Modinfo structure for the package,
 *	populate it with data from the files, and add it to the
 *	product's package list.
 * Parameters:
 *	mod	- pointer to the product Module structure
 *	name	- name of package (and thus the package directory)
 *	options	- specifies if the product directory should be offset
 *		  from the root directory.  Valid values:
 *			INSTALLED
 *			INSTALLED_SVC
 *			SPOOLED_NOTDUP
 *			SPOOLED_DUP
 * Return:
 *	SUCCESS		- a package module was created successfully
 *	ERR_INVALID	- this is not a valid package directory
 * Status:
 *	semi-private (internal library use only)
 */
int
load_pkginfo(Module * mod, char * name, int options)
{
	FILE	*mp;
	Modinfo *info = (Modinfo *) NULL;
	char	infoname[MAXPATHLEN];
	char	dirname[MAXPATHLEN];
	char	buf[BUFSIZ];
	char	*str;
	File	*file;
	int	i;
	int	required;

	/* note that this will break unless these 4 values are different */

	if ((MediaType) options == INSTALLED ||
			(MediaType) options == INSTALLED_SVC ||
			(ModState) options == SPOOLED_NOTDUP ||
			(ModState) options == SPOOLED_DUP) {
		(void) sprintf(dirname, "%s/%s/%s",
			get_rootdir(), mod->info.prod->p_pkgdir, name);
	} else {
		(void) sprintf(dirname, "%s/%s",
			mod->info.prod->p_pkgdir, name);
	}

	/*
	 * read in the the pkginfo file
	 */

	(void) sprintf(infoname, "%s/pkginfo", dirname);

	if ((mp = fopen(infoname, "r")) == (FILE *)0)
		return (ERR_INVALID);

	/* make new Modinfo structure */

	info = (Modinfo *) xcalloc(sizeof (Modinfo));
	info->m_pkg_dir = xstrdup(name);

	required = 0;

	while (fgets(buf, BUFSIZ, mp) != (char *)NULL) {
		if (strncmp(buf, "PKG=", 4) == 0) {
			info->m_pkgid = xstrdup(get_value(buf, '='));
			++required;
		} else if (strncmp(buf, "NAME=", 5) == 0) {
			info->m_name = xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
			++required;
		} else if (strncmp(buf, "PRODNAME=", 9) == 0)
			info->m_prodname = xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "PRODVERS=", 9) == 0)
			info->m_prodvers = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "VENDOR=", 7) == 0)
			info->m_vendor = xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "VERSION=", 8) == 0) {
			info->m_version = xstrdup(get_value(buf, '='));
			++required;
		} else if (strncmp(buf, "ARCH=", 5) == 0) {
			info->m_arch = xstrdup(get_value(buf, '='));
			(void) expand_arch(info);
			add_arch(mod, info->m_arch);
			++required;
		} else if (strncmp(buf, "DESC=", 5) == 0)
			info->m_desc = xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "BASEDIR=", 8) == 0)
			info->m_basedir = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PKGINST=", 8) == 0)
			info->m_pkginst = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "SUNW_PATCHID=", 13) == 0)
			info->m_patchid = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PATCHLIST=", 10) == 0)
			info->m_newarch_patches =
			    alloc_newarch_patchlist(get_value(buf, '='));
		else if (strncmp(buf, "CATEGORY=", 9) == 0) {
			info->m_category = xstrdup(get_value(buf, '='));
			++required;
		} else if (strncmp(buf, "SUNW_PKGTYPE=", 13) == 0) {
			str = get_value(buf, '=');
			for (i = 0; known_ptypes[i].namelen; i++) {
				if (strncmp(str, known_ptypes[i].name,
						known_ptypes[i].namelen) == 0) {
						info->m_sunw_ptype =
						known_ptypes[i].flag;
					break;
				}
			}
		} else if (strncmp(buf, "SUNW_LOC=", 9) == 0) {
			info->m_locale = xstrdup(get_value(buf, '='));
			info->m_loc_strlist = StringListBuild(info->m_locale,
			    ',');

			/* reject packages which have a bogus locale field */
			if (add_locale_list(mod, info->m_loc_strlist) !=
			    SUCCESS) {
				free_modinfo(info);
				return (ERR_INVALID);
			}
		} else if (strncmp(buf, "SUNW_PKGLIST=", 13) == 0) {
			info->m_l10n_pkglist = xstrdup(get_value(buf, '='));
		} else if (strncmp(buf, "INSTDATE=", 9) == 0)
			info->m_instdate = xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "SUNW_ICON=", 10) == 0)
			info->m_icon =
			    (File *)crackfile(dirname, buf, ICONFILE);
		else if (strncmp(buf, "SUNW_RUN=", 9) == 0) {
			file = (File *)crackfile(dirname, buf, RUNFILE);
			i = 0;
			if (info->m_demo) {
				while (info->m_demo[i] != (File *)0)
					i++;
			}
			info->m_demo = (File **)
				xrealloc(info->m_demo, (i + 2) * sizeof (File));
			info->m_demo[i] = file;
			info->m_demo[i + 1] = (File *)0;
		} else if (strncmp(buf, "SUNW_TEXT=", 10) == 0) {
			file = (File *)crackfile(dirname, buf, TEXTFILE);
			i = 0;
			if (info->m_text) {
				while (info->m_text[i] != (File *)0)
					i++;
			}
			info->m_text = (File **)
				xrealloc(info->m_text, (i + 2) * sizeof (File));
			info->m_text[i] = file;
			info->m_text[i + 1] = (File *)0;
		} else if (strncmp(buf, "CLASS=", 6) == 0) {
			str = get_value(buf, '=');
			if (strcmp(str, "required") == 0)
				info->m_status = REQUIRED;
		}
	}

	(void) fclose(mp);

	/*
	 * there are currently 5 fields required by the SVID to be in
	 * the pkginfo file. If they aren't all present, there's something
	 * wrong with the package, so punt it
	 */
	if (required != 5) {
		free_modinfo(info);
		return (ERR_INVALID);
	}

	/*
	 * Root packages don't have a BASEDIR, but their effective
	 * basedir is /.  So set it that way.
	 */

	if (info->m_basedir == NULL)
		info->m_basedir = xstrdup("/");

	/*
	 * calculate the package space based on the pkgmap file (since
	 * the install directory is optional this may legitimately
	 * fail), so the return code is currently ignored
	 */
	if ((MediaType)options != INSTALLED) {
		(void) sprintf(infoname, "%s/pkgmap", dirname);
		(void) calc_pkg_space(infoname, info);
	}

	if ((ModState)options == SPOOLED_NOTDUP) {
		info->m_shared = SPOOLED_NOTDUP;
		info->m_instdir = xstrdup(mod->info.prod->p_pkgdir);
	} else {
		info->m_shared = NOTDUPLICATE;
	}

	/*
	 * load the depend structures. The depend file is optional and
	 * may not be present
	 */
	read_pkg_depends(mod, info);

	/*
	 * add the new package Modinfo structure to the product package
	 * list
	 */
	add_package(mod, info);

	/*
	 * check to see if the category is a new one, and if so, add it to
	 * the product category list
	 */
	add_to_category(mod, info);

	return (SUCCESS);
}

/*
 * get_value()
 *	Parse out value from string passed in. str should be of the form:
 *	"TOKENxVALUE\n" where x=delim.  The trailing \n is optional, and
 *	will be removed.
 *	Also, leading and trailing white space will be removed from VALUE.
 * Parameters:
 *	str	- string pointer to text line to be parsed
 *	delim	- a character delimeter
 * Return:
 * Status:
 *	semi-private (internal library use only)
 */
char	*
get_value(char * str, char delim)
{
	char	   *cp, *cp1;

	if ((cp = strchr(str, delim)) == NULL)
		return (NULL);

	cp += 1;		/* value\n	*/
	cp1 = strchr(cp, '\n');
	if (cp1 && *cp1)
		*cp1 = '\0';	/* value	*/

	/* chop leading white space */
	for (; cp && *cp && ((*cp == ' ') || (*cp == '\t')); ++cp)
		;

	if (*cp == '\0')
		return("");

	/* chop trailing white space */
	for (cp1 = cp + strlen(cp) - 1;
	    cp1 >= cp && ((*cp1 == ' ') || (*cp1 == '\t')); --cp1)
		*cp1 = '\0';

	if (cp && *cp)
		return (cp);

	return ("");
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * add_to_category()
 *
 * Parameters:
 *	mod	-
 *	info	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
add_to_category(Module * mod, Modinfo * info)
{
	Module *list = NULL;
	Module *catp, *catp2;
	char	  namebuf[MAXCATNAMELEN];
	char	 *cp, *end;
	Module   *mp, *mp2;

	list = mod->info.prod->p_categories;

	cp = info->m_category;
	while (*cp != '\0') {
		while (*cp && (isspace((u_char)*cp) || *cp == ','))
			cp++;
		if (*cp == '\0')
			break;
		end = cp;
		while (*end && *end != ',')
			end++;
		(void) strncpy(namebuf, cp, end - cp);
		namebuf[end - cp] = '\0';
		cp = end;
		/*
		 * Loop through category list, looking
		 * for matching category entries.  If
		 * found, add module to category.  If
		 * not found, add as a new category.
		 * We skip the first category -- it's the
		 * entire contents list.
		 */
		catp2 = list;
		for (catp = list->next; catp; catp = catp->next) {
			if (strcmp(namebuf, catp->info.cat->cat_name) == 0)
				/* found match */
				break;
			catp2 = catp;
		}
		if (catp == (Module *)0) {
			catp = catp2->next =
			    (Module *)xcalloc(sizeof (Module));
			catp->prev = catp2;
			catp->type = CATEGORY;
			catp->parent = mod;
			catp->head = mod->info.prod->p_categories;
			catp->next = (Module *)NULL;
			catp->info.cat = (Category *)xcalloc(sizeof (Category));
			catp->info.cat->cat_name  =  xstrdup(namebuf);
		}

		/* add to the end of the chain */
		mp = NULL;
		for (mp2 = catp->sub; mp2; mp2 = mp2->next)
			mp = mp2;

		mp2 = (Module *)xcalloc(sizeof (Module));
		if (mp == NULL)
			catp->sub = mp2;
		else
			mp->next = mp2;
		mp2->next = (Module *)NULL;
		mp2->prev = mp;
		mp2->parent = catp;
		mp2->info.mod = info;
	}
	return;
}

/*
 * promote_clusters()
 *	Find each unselected package and cluster and add to list of
 *	metaclusters for consistency in displaying
 * Parameters:
 *	prod	-
 *	hdclst	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
promote_clusters(Module * prod, Module * hdclst)
{
	Module	 *mod;
	Module	 *curclst;

	if (prod->info.prod->p_clusters == NULL)
		return;
	for (mod = (Module*)prod->info.prod->p_clusters->list->next->data;
						mod != NULL; mod = mod->next)
		mark_module(mod, UNSELECTED);
	curclst = prod->sub;
	for (mod = prod->sub; mod != NULL; mod = mod->next) {
		mark_module(mod, SELECTED);
		curclst = mod;
	}

	for (mod = hdclst; mod != NULL; mod = mod->next) {

		if (mod->info.mod->m_status == SELECTED)
			continue;
		/*
		 * not in a metacluster, fit into hierarchy as a
		 * metacluster
		 */
		if (curclst == NULL) {
			prod->sub = curclst =
			    (Module *)xcalloc(sizeof (Module));
			curclst->prev = NULL;
			curclst->head = prod->sub;
			curclst->parent = prod;
		} else {
			curclst->next = (Module *) xcalloc(sizeof (Module));
			curclst->next->prev = curclst;
			curclst->next->head = curclst->head;
			curclst->next->parent = curclst->parent;
			curclst = curclst->next;
		}
		curclst->type = mod->type;
		curclst->sub = mod->sub;
		curclst->info.mod = mod->info.mod;
		curclst->next = (Module *) NULL;
	}
	for (mod = prod->sub; mod != NULL; mod = mod->next)
		mark_module(mod, UNSELECTED);

	return;
}

/*
 * arch_setup()
 * Parameters:
 *	prod	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
arch_setup(Module * prod)
{
	walklist(prod->info.prod->p_packages, arch_setup_package, NULL);
	return;
}

/*
 * get_packagetoc_path()
 *
 * Parameters:
 *	mod	-
 * Return:
 * Status:
 *	private
 */
static char *
get_packagetoc_path(Module * mod)
{
	static char	 package_toc[MAXPATHLEN];
	Module		*prod;

	if (mod == (Module *) NULL)
		prod = get_current_product();
	else
		prod = mod;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return (NULL);

	(void) sprintf(package_toc, "%s/locale/%s/%s",
				prod->info.prod->p_pkgdir,
				get_current_locale(), PACKAGE_TOC_NAME);
	/*
	 * make sure this file is readable, if it isn't, return path to
	 * the default .packagetoc (.../locale/C/.packagetoc)
	 */
	if (path_is_readable(package_toc) == FAILURE) {
		(void) sprintf(package_toc, "%s/locale/%s/%s",
			prod->info.prod->p_pkgdir, get_default_locale(),
			PACKAGE_TOC_NAME);
	}
	/*
	 * if .../locale/C/.packagetoc isn't readable, look for packagetoc
	 * on the top level.  (Provides backwards compatibility w/Jupiter
	 * and x86 developers release CD's
	 */
	if (path_is_readable(package_toc) == FAILURE) {
		(void) sprintf(package_toc, "%s/%s",
			prod->info.prod->p_pkgdir,
			PACKAGE_TOC_NAME);
	}
	return (package_toc);
}

/*
 * get_orderfile_path()
 *
 * Parameters:
 *	mod	-
 * Return:
 *
 * Status:
 *	private
 */
static char *
get_orderfile_path(Module * mod)
{
	static char	 order_file[MAXPATHLEN];
	Module		*prod;

	if (mod == (Module *) NULL)
		prod = get_current_product();
	else
		prod = mod;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return (NULL);

	(void) sprintf(order_file, "%s/%s", prod->info.prod->p_pkgdir,
				ORDER_FILE_NAME);
	return (order_file);
}

/*
 * load_packagetoc()
 *	Parse .packagetoc file for a product. Populate the hashed list of
 *	packages
 * Parameters:
 *	mod	-
 * Return:
 *
 * Status:
 *	private
 */
static int
load_packagetoc(Module * mod)
{

	Product		*prod;
	FILE	   *fp;
	char		buf[BUFSIZ + 1];
	Modinfo	*info = (Modinfo *) NULL;
	Depend	 *tdpnd = (Depend *) NULL;
	char	   *str, *cp;
	int		 i;
	int		 l10n = FALSE;

	cp = get_packagetoc_path(mod);
	if (!cp || (fp = fopen(cp, "r")) == (FILE *) NULL)
		return (ERR_NOFILE);

	/* make new a new hashlist for the packages */
	prod = mod->info.prod;
	prod->p_packages = getlist();

	while (fgets(buf, BUFSIZ, fp)) {
		buf[strlen(buf) - 1] = '\0';

		if (buf[0] == '#' || buf[0] == '\n')
			continue;
		else if (strncmp(buf, "PKG=", 4) == 0)
		{
			/* put current Modinfo structure into list */
			if (info != (Modinfo *) NULL) {
				add_package(mod, info);
				if (info->m_category != NULL)
					add_to_category(mod, info);
			}

			/* make new Modinfo structure */
			info = (Modinfo *) xcalloc(sizeof (Modinfo));
			info->m_pkgid = (char *)xstrdup(get_value(buf, '='));
			info->m_shared = NOTDUPLICATE;

		} else if (strncmp(buf, "DESC=", 5) == 0)
			info->m_desc = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "BASEDIR=", 8) == 0)
			info->m_basedir = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "CATEGORY=", 9) == 0)
			info->m_category = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PKGDIR=", 7) == 0)
			info->m_pkg_dir = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "NAME=", 5) == 0)
			info->m_name = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "VENDOR=", 7) == 0)
			info->m_vendor = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "VERSION=", 8) == 0)
			info->m_version = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "PRODNAME=", 9) == 0)
			info->m_prodname = (char *)xstrdup(dgettext(
			    install_lib_l10n_name, get_value(buf, '=')));
		else if (strncmp(buf, "PRODVERS=", 9) == 0)
			info->m_prodvers = (char *)xstrdup(get_value(buf, '='));
		else if (strncmp(buf, "SUNW_PKGTYPE=", 12) == 0) {
			str = get_value(buf, '=');
			for (i = 0; known_ptypes[i].namelen; i++)
			{
				info->m_sunw_ptype = default_ptype;
				if (strncmp(str, known_ptypes[i].name,
						known_ptypes[i].namelen) == 0) {
					info->m_sunw_ptype =
							known_ptypes[i].flag;
					break;
				}
				}
		} else if (strncmp(buf, "ARCH=", 5) == 0) {
			/* save a copy */
			info->m_arch = (char*)xstrdup(get_value(buf, '='));
			(void) expand_arch(info);
			add_arch(mod, info->m_arch);
		} else if (strncmp(buf, "SUNW_LOC=", 9) == 0) {
			info->m_locale = (char *)xstrdup(get_value(buf, '='));
			info->m_loc_strlist = StringListBuild(info->m_locale,
			    ',');

			(void) add_locale_list(mod, info->m_loc_strlist);
		} else if (strncmp(buf, "SUNW_PKGLIST=", 13) == 0) {
			l10n = TRUE;
			info->m_l10n_pkglist =
					(char *)xstrdup(get_value(buf, '='));
		} else if (strncmp(buf, "SUNW_PDEPEND=", 13) == 0) {
			tdpnd = (Depend *)xcalloc(sizeof (Depend));

			str = get_value(buf, '=');
			/* need to parse instances out... */
			if (cp = strchr(str, ':')) {
				parse_instance_spec(tdpnd, cp+1);
				*cp = '\0';
			}

			tdpnd->d_pkgid = (char *)xstrdup(str);

			if (info->m_pdepends != (Depend *) NULL) {
				tdpnd->d_next = info->m_pdepends;
				info->m_pdepends->d_prev = tdpnd;
			}
			info->m_pdepends = tdpnd;
		} else if (strncmp(buf, "SUNW_IDEPEND=", 13) == 0) {
			tdpnd = (Depend *)xcalloc(sizeof (Depend));

			str = get_value(buf, '=');
			/* need to parse instances out... */
			if (cp = strchr(str, ':')) {
				parse_instance_spec(tdpnd, cp+1);
				*cp = '\0';	 /* pitch the instance spec */
			}

			tdpnd->d_pkgid = (char *)xstrdup(str);

			if (info->m_idepends != (Depend *) NULL) {
				tdpnd->d_next = info->m_idepends;
				info->m_idepends->d_prev = tdpnd;
			}
			info->m_idepends = tdpnd;
		} else if (strncmp(buf, "SUNW_RDEPEND=", 13) == 0) {
				tdpnd = (Depend *)xcalloc(sizeof (Depend));

			str = get_value(buf, '=');
			/* need to parse instances out... */
			if (cp = strchr(str, ':')) {
				parse_instance_spec(tdpnd, cp+1);
				*cp = '\0';	 /* pitch the instance spec */
			}

			tdpnd->d_pkgid = (char *)xstrdup(str);

			if (info->m_rdepends != (Depend *) NULL) {
				tdpnd->d_next = info->m_rdepends;
				info->m_rdepends->d_prev = tdpnd;
			}
			info->m_rdepends = tdpnd;
		}

		else if (strncmp(buf, "ROOTSIZE=", 9) == 0)
			info->m_deflt_fs[ROOT_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "VARSIZE=", 8) == 0)
			info->m_deflt_fs[VAR_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "OPTSIZE=", 8) == 0)
			info->m_deflt_fs[OPT_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "EXPORTSIZE=", 11) == 0)
			info->m_deflt_fs[EXPORT_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "USRSIZE=", 8) == 0)
			info->m_deflt_fs[USR_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "USROWNSIZE=", 11) == 0)
			info->m_deflt_fs[USR_OWN_FS] =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "SPOOLEDSIZE=", 12) == 0)
			info->m_spooled_size =
				(daddr_t) atoi(get_value(buf, '=')) / KBYTE;
		else if (strncmp(buf, "PATCHLIST=", 10) == 0)
			info->m_newarch_patches =
			    alloc_newarch_patchlist(get_value(buf, '='));
	}

	/*
	 * Root packages don't have a BASEDIR, but their effective
	 * basedir is /.  So set it that way.
	 */

	if (info->m_basedir == NULL)
		info->m_basedir = xstrdup("/");

	/* put last Modinfo structure into list */
	if (info != (Modinfo *) NULL) {
		add_package(mod, info);
		if (info->m_category != NULL)
			add_to_category(mod, info);
	}

	if (l10n)
		localize_packages(mod);

	(void) fclose(fp);
	return (SUCCESS);
}

/*
 * arch_setup_package()
 *
 * Parameters:
 *	np	-
 *	data	- ignored
 * Return:
 *	SUCCESS
 * Status:
 *	privte
 */
/*ARGSUSED1*/
static int
arch_setup_package(Node * np, caddr_t data)
{
	Modinfo *info, *info2;
	Node	*tp;

	info = (Modinfo*)np->data;
	if ((supports_arch(get_default_arch(), info->m_arch) == TRUE) ||
	    strcmp("all", info->m_arch) == 0 ||
	    strcmp("all.all", info->m_arch) == 0) {
		return (SUCCESS);
	}

	info2 = (Modinfo *)xcalloc(sizeof (Modinfo));
	info2->m_pkgid = xstrdup(info->m_pkgid);
	info2->m_name = xstrdup(info->m_name);
	info2->m_shared = NULLPKG;
	info2->m_arch = xstrdup(get_default_arch());
	info2->m_expand_arch = xstrdup(get_default_arch());
	/*
	 *  The NULLPKG at the head of the instance chain must have
	 *  a dependency list, since the dependency list of the
	 *  module at the head of the instance chain is used for
	 *  ordering the packages.
	 *
	 *  NOTE:  the fact that the dependency list of the first
	 *  instance is the ONLY one used for ordering the packages,
	 *  assumes that all instances of a particular package have
	 *  the same dependencies (perhaps a bad assumption).  But
	 *  changing that would required giving each package instance
	 *  its own entry in the p_packages hash list.  This might
	 *  be a good idea for all kinds of reasons, but it's a big
	 *  change.
	 */
	info2->m_pdepends = duplicate_depend(info->m_pdepends);
	info2->m_idepends = duplicate_depend(info->m_idepends);
	info2->m_rdepends = duplicate_depend(info->m_rdepends);
	info2->m_desc = xstrdup(dgettext("SUNW_INSTALL_SWLIB",
	    "This package is not supported on this architecture"));
	tp = getnode();			/* new list node */
	tp->key = xstrdup(info->m_pkgid);	/* make pkgid key */
	tp->delproc = &free_np_modinfo;	/* set delete function */
	tp->data = (char *)np->data;
	np->data = (char *)info2;
	info2->m_instances = tp;

	return (SUCCESS);
}

/*
 * load_product()
 *	Loads the product specified into a linked module list. Use .packagetoc
 *	and .order files if use_dotfiles is TRUE and available, otherwise parse
 *	packages for configuration information.
 * Parameters:
 *	prod		-
 *	use_dotfiles	-  [RO] (TRUE|FALSE)
 *			   Flag indicating if existing .packagetoc
 *			   and .order files are to be used during the load
 * Return:
 * Status:
 *	private
 */
static int
load_product(Module *prod, int use_dotfiles)
{
	struct dirent	*dp;
	DIR		*dirp;
	int		pkgtoc = FAILURE;
	int		loaded = FAILURE;
	int		return_val = SUCCESS;
	char		*orderfile;
	int		status;

	if (prod == NULL ||
			(prod->type != PRODUCT && prod->type != NULLPRODUCT))
		return (ERR_INVALIDTYPE);

	if (prod->info.prod->p_packages != NULL ||
				prod->info.prod->p_sw_4x != NULL)
		return (ERR_PREVLOAD);

	/* read in .platform/<OEM> files, if present */
	if ((status = load_platforms(prod)) != 0 && status != ERR_NODIR)
		return (ERR_INVALID);

	/* read in .packagetoc file */
	if (use_dotfiles == TRUE)
		pkgtoc = load_packagetoc(prod);

	if (pkgtoc != SUCCESS) {

		/* Use packageinfo files and/or read in 4.x software */

		if (prod->info.prod->p_pkgdir == NULL)
			return (ERR_INVALID);
		dirp = opendir(prod->info.prod->p_pkgdir);
		if (dirp == (DIR *)0)
			return (ERR_NODIR);

		prod->info.prod->p_packages = getlist();
		while ((dp = readdir(dirp)) != (struct dirent *)0) {
			if (strcmp(dp->d_name, ".") == 0 ||
				strcmp(dp->d_name, "..") == 0)
				continue;
			loaded = SUCCESS;
			if (ispackage(prod->info.prod->p_pkgdir, dp->d_name))
				load_package(prod, dp->d_name);
			else if (is4x(prod->info.prod->p_pkgdir, dp->d_name))
				(void) load_4x(prod, dp->d_name);
		}
		(void) closedir(dirp);
		if (loaded != SUCCESS)
			return (ERR_NOPROD);
		localize_packages(prod);
	}

	/* do package specific setup */
	if (prod->info.prod->p_packages != NULL) {
		arch_setup(prod);
		if ((loaded = load_clusters(prod, get_clustertoc_path(prod)))
								!= SUCCESS)
			return_val  = loaded;

		/* set inital status on all clusters based on their contents */
		set_cluster_status(prod->sub);

		/* sort packages into proper order for the interface file.  */
		if (use_dotfiles)
			orderfile = get_orderfile_path(prod);
		else
			orderfile = "";
		sort_packages(prod, orderfile);
	}

	/* Sort locale strings */
	sort_locales(prod);

	return (return_val);
}

/*
 * load_subclusters()
 *	Read subclusters into a list of modules. first subcluster is passed in
 *	in `buf'. if parent module is a CLUSTER, mark subs as `selected' so we
 *	know which ones are included in CLUSTERS
 * Parameters:
 *	fp	-
 *	buf	-
 *	parent	-
 *	prod	-
 * Return:
 *
 * Status:
 *	private
 */
static Module  *
load_subclusters(FILE * fp, char * buf, Module * parent, Module * prod)
{
	Modinfo	*info;
	Module	 *head, *current, *mod;
	Node	   *np;

	current = head = (Module *) NULL;

	do {
		if (strncmp(buf, "END", 3) == 0)
			break;

		if (strncmp(buf, "SUNW_CSRMEMBER", 14) == 0) {
			if (current) {
				current->next =
				    (Module *) xcalloc(sizeof (Module));
				current->next->prev = current;
				current = current->next;
			} else {
				head = current =
				    (Module *) xcalloc(sizeof (Module));
				current->prev = (Module *)NULL;
			}
			current->head = head;
			current->parent = parent;
			/* get pkg/clstr from the appropriate hashed list */
			if (np = findnode(prod->info.prod->p_packages,
							get_value(buf, '='))) {
				info = (Modinfo *) np->data;
				current->info.mod = info;
				current->type = PACKAGE;
				current->info.mod->m_flags |= PART_OF_CLUSTER;

		/* HACK reading a cluster definition, mark any packages...  */
				if (parent->type == CLUSTER)
					info->m_status = SELECTED;

			} else if (np = findnode(prod->info.prod->p_clusters,
						get_value(buf, '='))) {
				mod = (Module *) np->data;
				dup_clstr_tree(mod, current);
				current->info.mod->m_flags |= PART_OF_CLUSTER;
			} else {
				if ((np = add_null_pkg(
				    prod, get_value(buf, '='))) == NULL)
					return (NULL);
				info = (Modinfo *) np->data;
				current->info.mod = info;
				current->type = PACKAGE;

				/*
				 * HACK reading a cluster definition,
				 * mark any packages...
				 */
				if (parent->type == CLUSTER)
					info->m_status = SELECTED;
			}
		} else
			return (NULL);

		if (fgets(buf, BUFSIZ, fp))
			buf[strlen(buf) - 1] = '\0';
	/*CONSTCOND*/
	} while (1);

	if (current)
		current->next = (Module *) NULL;

	return (head);
}

/*
 * ispackage()
 *	Determine if the directory 'name' in the media directory
 *	'dir' appears to be a package. This is done by checking for
 *	a non-executable but readable pkginfo and pkgmap files.
 * Parameters:
 *	dir	- media directory containing proposed packages
 *	name	- directory name for possible package
 * Return:
 *	1	- valid package directory
 *	0	- this is not a valid package directory
 * Status:
 *	private
 */
static int
ispackage(char * dir, char * name)
{
	char	file[MAXPATHLEN];

	(void) sprintf(file, "%s/%s/pkginfo", dir, name);

	if (access(file, R_OK) == 0) {
		(void) sprintf(file, "%s/%s/pkgmap", dir, name);
		if (access(file, R_OK) == 0)
			return (1);
	}
	return (0);
}

/*
 * is4x()
 *	Determine whether something is 4.x based software
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
is4x(char * dir, char * name)
{
	char	info[MAXPATHLEN];

	(void) sprintf(info, "%s/%s/_info", dir, name);

	if (access(info, R_OK) != 0)
		return (0);

	return (1);
}

/*
 * load_package()
 *	Load the pkginfo file
 * Parameters:
 *	prod	-
 *	pkg_dir	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
load_package(Module * prod, char * pkg_dir)
{
	(void) load_pkginfo(prod, pkg_dir, NULL);
}

/*
 * load_4x()
 *	Load a 4.x-style unbundled product
 * Parameters:
 *	prod	-
 * 	name	-
 * Return:
 *
 * Status:
 *	private
 */
static int
load_4x(Module * prod, char * name)
{
	FILE	*fp;
	Module	*mp, *mod;
	Modinfo	*info;
	char	dirname[MAXPATHLEN];
	char	buf[BUFSIZ];
	char	*os = (char *)0;
	char	*arch = (char *)0;
	File	*file;
	int	nfiles;

	mp = (Module *)xcalloc(sizeof (Module));
	mp->type = UNBUNDLED_4X;
	mp->info.mod = info = (Modinfo *) xcalloc(sizeof (Modinfo));
	info->m_pkgid = xstrdup(name);

	(void) sprintf(dirname, "%s/%s/_info", prod->info.prod->p_pkgdir, name);
	fp = fopen(dirname, "r");
	if (fp == (FILE *)NULL)
		return (ERR_NOPKG);

	(void) sprintf(dirname, "%s/%s", prod->info.prod->p_pkgdir, name);

	while (fgets(buf, BUFSIZ, fp)) {
		switch (buf[0]) {
		case 'N':	/* product name */
			info->m_prodname = xstrdup(get_value(buf, ':'));
			break;
		case 'V':	/* version */
			info->m_version = xstrdup(get_value(buf, ':'));
			break;
		case 'C':	/* vendor */
			info->m_vendor = xstrdup(get_value(buf, ':'));
			break;
		case 'O':	/* os release */
			os = xstrdup(get_value(buf, ':'));
			break;
		case 'A':	/* architectures */
			arch = xstrdup(get_value(buf, ':'));
			break;
		case 'Y':	/* keywords (really category) */
			info->m_desc = xstrdup(get_value(buf, ':'));
			info->m_category = xstrdup(get_value(buf, ':'));
			break;
		case 'T':	/* text file */
			file = (File *)crackfile(dirname, buf, TEXTFILE);
			nfiles = 0;
			if (info->m_text) {
				while (info->m_text[nfiles] != (File *)0)
					nfiles++;
			}
			info->m_text = (File **)xrealloc(
				info->m_text, (nfiles + 2) * sizeof (File));
			info->m_text[nfiles] = file;
			info->m_text[nfiles + 1] = (File *)0;
			break;
		case 'I':	/* install script */
			info->m_install =
			    (File*)crackfile(dirname, buf, RUNFILE);
			break;
		case 'E':	/* demo file/script script */
			file = (File *)crackfile(dirname, buf, RUNFILE);
			nfiles = 0;
			if (info->m_demo) {
				while (info->m_demo[nfiles] != (File *)0)
					nfiles++;
			}
			info->m_demo = (File **)xrealloc(
				info->m_demo, (nfiles + 2) * sizeof (File));
			info->m_demo[nfiles] = file;
			info->m_demo[nfiles + 1] = (File *)0;
			break;
		case 'B':	/* icon */
			info->m_icon =
			    (File *)crackfile(dirname, buf, ICONFILE);
			break;
		default:
			break;
		}
	}
	(void) fclose(fp);

	if (info->m_prodname == (char *)0 || info->m_prodname[0] == '\0')
		info->m_prodname = xstrdup(info->m_pkgid);
	info->m_name = xstrdup(info->m_prodname);

	info->m_basedir = xstrdup("/");
	info->m_instdir = NULL;

	if (arch) {
		info->m_arch = arch;
		if (os) {
			info->m_arch = (char *)xrealloc((void *)info->m_arch,
				strlen(info->m_arch) + strlen(os) + 3);
			(void) strcat(info->m_arch, ", ");
			(void) strcat(info->m_arch, os);
		}
	} else if (os)
		info->m_arch = os;

	if (info->m_category != (char *)0)
		add_to_category(prod, info);

	if (prod->info.prod->p_sw_4x == NULL) {
		prod->info.prod->p_sw_4x = getlist();
		dellist(&prod->info.prod->p_packages);
		prod->info.prod->p_packages = NULL;
		prod->sub = mp;
	} else {
		for (mod = prod->sub; mod->next != NULL; mod = mod->next)
			;
		mod->next = mp;
		mp->prev = mod;
	}
	mp->head = prod->sub;
	mp->parent = prod;

	add_4x(prod, info);
	return (SUCCESS);
}
/*
 * alloc_newarch_patchlist()
 *	allocate a linked list of patch_num structure to represent
 *	the list of new-style patches listed in a packages'
 *	PATCHLIST variable.
 * Parameters:
 *	prod	-
 * 	name	-
 * Return:
 *
 * Status:
 *	private
 */
static struct patch_num *
alloc_newarch_patchlist(char *cp)
{
	char			holdstring[256], *p, *tp;
	struct patch_num	*pnum, *pnum_head;

	pnum_head = NULL;
	while (cp != NULL && *cp != '\0') {
		/* skip over any leading white space */
		while (*cp != '\0' && isspace(*cp))
			cp++;

		if (*cp == '\0')
			break;

		/* find the end of this entry */
		for (p = cp; *p != '\0' && !isspace(*p); p++)
			;

		/* copy this entry to the temporary work area */
		(void) strncpy(holdstring, cp, p - cp);
		holdstring[p - cp] = '\0';

		/*
		 *  move cp to the end of the string to prepare
		 *  for the next iteration.
		 */
		cp = p;

		/*
		 *  Now process the copied entry.  Find the '-' in the
		 *  patch number.  If there is none, or if it's the first
		 *  or last character in the string, reject the patch number.
		 */
		p = (char *)strchr(holdstring, '-');
		if (p == NULL || p == holdstring || *(p + 1) == '\0')
			continue;

		/*
		 *  Replace the hyphen by a zero and move p to the first
		 *  character of the revision string.
		 */
		*p++ = '\0';

		/*
		 *  Make sure the revision string contains nothing but
		 *  decimal digits.
		 */
		for (tp = p; *tp != '\0' && isdigit(*tp); tp++)
			;
		if (*tp != '\0')
			continue;

		pnum = (struct patch_num *)xcalloc((size_t)
		    sizeof (struct patch_num));
		pnum->patch_num_id = xstrdup(holdstring);
		pnum->patch_num_rev_string = xstrdup(p);
		pnum->patch_num_rev = atoi(p);

		/* link the patch number entry to the list */
		link_to((Item**)&pnum_head, (Item *)pnum);
	}

	return (pnum_head);
}
/*
 * add_null_pkg()
 *
 * Parameters:
 *	prod	-
 *	name	-
 * Return:
 *
 * Status:
 * 	semi-private (internal library use only)
 */
static Node *
add_null_pkg(Module * prod, char * name)
{
	Node		*np;
	Modinfo		*info;

	info = (Modinfo *)xcalloc(sizeof (Modinfo));
	info->m_pkgid = xstrdup(name);
	info->m_name = xstrdup(name);
	info->m_shared = NULLPKG;
	np = getnode();		/* new list node */
	np->key = xstrdup(info->m_pkgid);	/* make pkgid key */
	np->data = (char *) info;	/* put Modinfo into node */
	np->delproc = &free_np_modinfo;    /* set delete function */

	if (addnode(prod->info.prod->p_packages, np) == -1)
		return (findnode(prod->info.prod->p_packages, info->m_pkgid));
	return (np);
}
