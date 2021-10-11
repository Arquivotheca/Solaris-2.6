#ifndef lint
#pragma ident "@(#)soft_util.c 1.5 96/05/15 SMI"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved.
 */
#include <sys/stat.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "spmicommon_api.h"
#include "spmisoft_lib.h"

/* Public Function Prototypes */

void	swi_sw_lib_init(int);
int	swi_set_instdir_svc_svr(Module *);
void	swi_clear_instdir_svc_svr(Module *);
char    *swi_gen_bootblk_path(char *);
char	*swi_gen_pboot_path(char *);
char	*swi_gen_openfirmware_path(char *);
int	is_upgrade(void);

/* Library Function Prototypes */

void		set_primary_arch(Module *);
int		sort_packages(Module *, char *);
Module *	sort_modules(Module *);
File *		crackfile(char *, char *, FileType);
void		sw_lib_log_hook(char *);
int		isa_handled(char *);
void		isa_handled_clear(void);
void		sort_ordered_pkglist(Module *);
int		UsrpackagesExist(void);
void		set_is_upgrade(int);

/* Local Function Prototypes */

static int 	pkg_order_cmp(Node *, Node *);
static void 	order_pkgs(Product *);
static void 	list_insert(Node *, Node *);
static void 	list_append(Node *, Node *);
static void 	list_unlink(Node *);
static int 	dependson(Node *, Node *);
static int 	_set_primary_arch(Node *, caddr_t);
static int	change_view_status(Node *, caddr_t);

/* Local Globals */

int 		default_ptype = PTYPE_USR;

char *def_mnt_pnt[] = {		/* order must match FileSys defines */
	"/",
	"/usr",
	"/usr/openwin",
	"/opt",
	"swap",
	"/var",
	"/export/exec",
	"/export/swap",
	"/export/root",
	"/export/home",
	"/export",
	NULL
};

/* Local Statics and Constants */

static char	*bootblk_path = NULL;
static char	*pboot_path = NULL;

int	s_is_upgrade = 0;

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * sw_lib_init()
 *	Provides an application with the ability to specify information
 *	to control some of the libraries default behavior.  Defaults are
 *	provided for all of these values, so calling this function is
 *	optional.  Only the functions which are to be changed from their
 *	current values need to be filled in.
 * Parameters:
 *	ptype		- specifies the default type of a package if no package
 *			  type is defined. Default is PTYPE_USR.
 *	disk_space	- specifies the percent of free space which should be
 *			  used in calculating space requirements. To set this
 *			  to "0%" the argument "NO_EXTRA_SPACE" should be used.
 *			  Default is 5% free space.
 * Return:
 *	none
 * Status:
 *	Public
 */
void
swi_sw_lib_init(int ptype)
{
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("sw_lib_init");
#endif

	/* restrict default file creation mask */
	(void) umask(022);
	if (ptype != 0)
		default_ptype = ptype;

	do_spacecheck_init();
}

/*
 * set_instdir_svc_svr()
 *	Initial install specific function called when the machine type is set
 *	to server. This function creates the necessary views and sets the
 *	necessary information to allow the space code to correctly calculate
 *	the space required for the service.
 * Parameters:
 *	prod	- pointer to the product module
 * Return:
 *	ERR_NULLPKG  -	the shared status of one of the packages
 *			associated with the product is a NULLPKG
 *	SUCCESS	     -	all other cases
 * Status:
 *	public
 */
int
swi_set_instdir_svc_svr(Module * prod)
{
	Modinfo	*i, *info;
	Node	*pkg = prod->info.prod->p_packages->list;
	Module	*svc, *svcnext;
	char	buf[MAXPATHLEN];
	int	ret_val = SUCCESS;
	Product	*pi = prod->info.prod;
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_instdir_svc_svr");
#endif

	set_is_upgrade(0);

	load_default_view(prod);
	/*LINTED [var set before used]*/
	for (svc = get_media_head(); svc != NULL; svc = svcnext) {
		svcnext = svc->next;
		if ((svc->info.media->med_type != INSTALLED) &&
		    (svc->info.media->med_type != INSTALLED_SVC))
			continue;
		free_full_view(prod, svc);
		unload_media(svc);
	}
	for (pkg = pkg->next; pkg != pi->p_packages->list; pkg = pkg->next) {
		i = (Modinfo *) pkg->data;
		i->m_status = UNSELECTED;
		if (i->m_shared == NULLPKG) {
			ret_val = ERR_NULLPKG;
			continue;
		}
		if ((supports_arch(get_default_arch(), i->m_arch) == TRUE) ||
					strcmp("all", i->m_arch) == 0 ||
					strcmp("all.all", i->m_arch) == 0)
			i->m_action = TO_BE_PKGADDED;
		else
			i->m_action = NO_ACTION_DEFINED;
	}
	(void) sprintf(buf, "/export");
	svc = add_new_service(buf);
	load_view(prod, svc);
	for (pkg = pkg->next; pkg != pi->p_packages->list; pkg = pkg->next) {
		i = (Modinfo *) pkg->data;
		if (i->m_sunw_ptype == PTYPE_ROOT) {
			for (info = i; info != NULL; info = next_inst(info)) {
				info->m_action = TO_BE_SPOOLED;
				(void) sprintf(buf,
					"/export/root/templates/%s_%s/%s_%s_%s",
					pi->p_name, pi->p_version,
					info->m_pkgid, info->m_version,
					info->m_expand_arch);
				info->m_instdir = xstrdup(buf);
			}
		} else if ((i->m_sunw_ptype == PTYPE_KVM) &&
		    (! is_KBI_service(prod->info.prod))) {
			/*
			 * NOTICE: the use of the is_KBI_service is
			 * used as a temporary measure for dealing
			 * with the new KBI world. In this world there
			 * should no longer be KVM type packages, but
			 * that revolution is slow comming. With no
			 * KVM type package there is no need for the
			 * special /export/exec/kvm directory.
			 */
			for (info = i; info != NULL; info = next_inst(info)) {
				if (supports_arch(get_default_arch(),
				    info->m_arch) != TRUE)
					info->m_action = TO_BE_PKGADDED;
				(void) sprintf(buf,
				    "/export/exec/kvm/%s_%s_%s%s",
				    pi->p_name, pi->p_version,
				    info->m_arch, info->m_basedir);
				info->m_instdir = xstrdup(buf);
			}
		} else {
			for (info = i; info != NULL;
						info = next_inst(info))
				info->m_action = NO_ACTION_DEFINED;
		}
	}
	load_default_view(prod);
	walklist(pi->p_next_view->p_view_pkgs, change_view_status,
			(caddr_t)0);
	walklist(pi->p_next_view->p_view_cluster, change_view_status,
			(caddr_t)0);
	walklist(pi->p_next_view->p_view_locale, change_view_status,
			(caddr_t)1);
	walklist(pi->p_next_view->p_view_arches, change_view_status,
			(caddr_t)2);
	pi->p_next_view->p_view_from->info.media->med_flags = SVC_TO_BE_REMOVED;

	return (ret_val);

}

/*
 * clear_instdir_svc_svr()
 * 	Initial install specific called when machine type is changed from
 *	being a server. This function destroys all service views associated
 *	with the media "/export".
 * Parameters:
 *	prod	- pointer to product module being manipulated
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_clear_instdir_svc_svr(Module * prod)
{
	Module	*svc;
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("clear_instdir_svr");
#endif


	load_default_view(prod);
	clear_all_view(prod);
	if ((svc = find_media("/export", NULL)) != NULL) {
		free_full_view(prod, svc);
		unload_media(svc);
	}
	return;
}

/*
 * set_primary_arch()
 *	If any of the architectures associated with the Product were selected
 *	then ???
 * Parameters:
 *	prod	- pointer to product Module being manipulated
 * Return:
 *	none
 * Status:
 *	public
 */
void
set_primary_arch(Module * prod)
{
	int	do_walk = FALSE;
	Arch  *arch;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("set_primary_arch");
#endif

	for (arch = prod->info.prod->p_arches; arch != NULL;
						arch = arch->a_next) {
		if (arch->a_selected)
			do_walk = TRUE;
	}
	if (do_walk == FALSE)
		return;

	walklist(prod->info.prod->p_packages, _set_primary_arch, (caddr_t)prod);
	return;
}

/*
 * sw_lib_log_hook
 *	Print out sw log info.  Provides hook for setting
 *	breakpoints.
 * Parameters:
 *	funcname - name of function logging that it was called.
 * Return:
 *	none
 * Status:
 *	Public
 */
/*ARGSUSED0*/
void
sw_lib_log_hook(char *funcname)
{
	return;
}

/*
 * gen_bootblk_path
 *	Generate the pathname of the bootblk file.
 * Parameters:
 *	rootdir - root relative to which the bootblk should be found.
 * Return:
 *	pathname of bootblk
 * Status:
 *	Public
 */
char *
swi_gen_bootblk_path(char *rootdir)
{
	char	pathbuf[MAXPATHLEN];

	if (bootblk_path != NULL)
		free(bootblk_path);

	/*
	 *  First try the new location of the bootblk:
	 *  /usr/platform/<platform_name>/lib/fs/ufs/bootblk
	 */
	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/platform/");
	(void) strcat(pathbuf, get_default_platform());
	(void) strcat(pathbuf, "/lib/fs/ufs/bootblk");

	if (path_is_readable(pathbuf) == SUCCESS) {
		bootblk_path = xstrdup(pathbuf);
		return (bootblk_path);
	}

	/*
	 *  Next, try the new location, but under the machine type:
	 *  /usr/platform/<machine_type>/lib/fs/ufs/bootblk
	 */
	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/platform/");
	(void) strcat(pathbuf, get_default_impl());
	(void) strcat(pathbuf, "/lib/fs/ufs/bootblk");

	if (path_is_readable(pathbuf) == SUCCESS) {
		bootblk_path = xstrdup(pathbuf);
		return (bootblk_path);
	}

	/* The new platform-dependent path failed.  Try the old path */

	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/lib/fs/ufs/bootblk");

	if (path_is_readable(pathbuf) == SUCCESS) {
		bootblk_path = xstrdup(pathbuf);
		return (bootblk_path);
	}

	return (NULL);
}


/*
 * gen_pboot_path
 *	Generate the pathname of the pboot file.
 * Parameters:
 *	rootdir - root relative to which the pboot should be found.
 * Return:
 *	pathname of pboot
 * Status:
 *	Public
 */
char *
swi_gen_pboot_path(char *rootdir)
{
	char	pathbuf[MAXPATHLEN];

	if (pboot_path != NULL)
		free(pboot_path);

	/*
	 *  First try the new location of the pboot:
	 *  /usr/platform/<platform_name>/lib/fs/ufs/pboot
	 */
	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/platform/");
	(void) strcat(pathbuf, get_default_platform());
	(void) strcat(pathbuf, "/lib/fs/ufs/pboot");

	if (path_is_readable(pathbuf) == SUCCESS) {
		pboot_path = xstrdup(pathbuf);
		return (pboot_path);
	}

	/*
	 *  Next, try the new location, but under the machine type:
	 *  /usr/platform/<machine_type>/lib/fs/ufs/pboot
	 */
	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/platform/");
	(void) strcat(pathbuf, get_default_impl());
	(void) strcat(pathbuf, "/lib/fs/ufs/pboot");

	if (path_is_readable(pathbuf) == SUCCESS) {
		pboot_path = xstrdup(pathbuf);
		return (pboot_path);
	}

	/* The new platform-dependent path failed.  Try the old path */

	if (strcmp(rootdir, "/") != 0)
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/usr/lib/fs/ufs/pboot");

	if (path_is_readable(pathbuf) == SUCCESS) {
		pboot_path = xstrdup(pathbuf);
		return (pboot_path);
	}

	return (NULL);
}

/*
 * gen_openfirmware_path
 *	Generate the pathname of the openfirmware file.
 * Parameters:
 *	rootdir - root relative to which the pboot should be found.
 * Return:
 *	pathname of pboot
 * Status:
 *	Public
 */
char *
swi_gen_openfirmware_path(char *rootdir)
{
	static char	*ofw_path = NULL;
	char		pathbuf[MAXPATHLEN];

	/*
	 * if the pointer has already been used, free it and
	 * reset it
	 */
	if (ofw_path != NULL) {
		free(ofw_path);
		ofw_path = NULL;
	}

	/*
	 * look for the openfirmware file in the platform
	 * directory:
	 * <rootdir>/platform/<platform_name>/openfirmware.x41
	 */
	if (!streq(rootdir, "/"))
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/platform/");
	(void) strcat(pathbuf, get_default_platform());
	(void) strcat(pathbuf, "/openfirmware.x41");

	/* see if the file exists */
	if (path_is_readable(pathbuf) == SUCCESS) {
		ofw_path = xstrdup(pathbuf);
		return (ofw_path);
	}

	/*
	 * look for the openfirmware file in the inst directory:
	 * <rootdir>/platform/<machine_type>/openfirmware.x41
	 */
	if (!streq(rootdir, "/"))
		(void) strcpy(pathbuf, rootdir);
	else
		pathbuf[0] = '\0';

	(void) strcat(pathbuf, "/platform/");
	(void) strcat(pathbuf, get_default_impl());
	(void) strcat(pathbuf, "/openfirmware.x41");

	/* see if the file exists */
	if (path_is_readable(pathbuf) == SUCCESS) {
		ofw_path = xstrdup(pathbuf);
		return (ofw_path);
	}

	return (NULL);
}

int
is_upgrade(void)
{
	return(s_is_upgrade);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * sort_packages()
 * Open the 'ofn' order file, which should contain a per-line list of
 * pkg IDs. Set the order field of the module associated with the
 * package to it's ordinal position in this file.  This file tells us
 * what order packages must be installed in. Used to sort sort the
 * hashed-list so that packages are accessed in the right order.
 * Parameters:	prod	- product module pointer containing hash-list
 *		ofn	- order file name
 * Return:	SUCCESS	   - pkg IDs were loaded and the hash-list sorted,
 *			     or the order file was not accessable
 *		ERR_BADPKG - the pkgid in the order file was not in
 *			     the hash-list; abort the routine at this point.
 * Note:  this routine does nothing to check that all order fields have
 *	  been assigned values, nor that values weren't carried over from
 *	  a previous call
 */
int
sort_packages(Module * prod, char *ofn)
{
	Node	*np;
	Modinfo	*info;
	int	i;
	FILE	*fp;
	char	buf[BUFSIZ];

	if ((path_is_readable(ofn) == FAILURE) ||
			((fp = fopen(ofn, "r")) == (FILE *) NULL)) {
		order_pkgs(prod->info.prod);
		return (SUCCESS);
	}

	for (i = 1; fgets(buf, BUFSIZ, fp) != (char *) NULL; i++) {
		buf[strlen(buf) - 1] = '\0';	/* chop newline */

		if (buf[0] == '#')		/* skip comment lines */
			continue;

		if (np = findnode(prod->info.prod->p_packages, buf)) {
			info = (Modinfo *) np->data;
			info->m_order = i;
		} else {
			fclose(fp);
			return (ERR_BADPKG);
		}
	}

	fclose(fp);

	sort_ordered_pkglist(prod);
	return (SUCCESS);
}


/*
 * sort_ordered_pkglist()
 * Sort the package list, where the packages have already had
 * the m_order fields set.
 *
 * Return:	nothing
 */
void
sort_ordered_pkglist(Module * prod)
{
	/* all packages should have order numbers, sort em.  */
	sortlist(prod->info.prod->p_packages, pkg_order_cmp);
}


/*
 * sort_modules()
 * Sort a Module chain in ascending order based on the module name.
 * Parameters:	list	- pointer to module
 * Return:	NULL	 - NULL parameter
 *		Module * - pointer to the head of the list
 */
Module	 *
sort_modules(Module * list)
{

	Module  *p, *q, *r;

	if (list == NULL)
		return (NULL);
	for (p = list->next; p != NULL; p = p->next) {
		r = p->next;
	 	for (q = list; q != r; q = q->next) {
			if (strcoll(p->info.mod->m_name,
			    q->info.mod->m_name) < 0) {
				if (p->next != NULL)
					p->next->prev = p->prev;
				p->prev->next = p->next;
				p->prev = q->prev;
				p->next = q;
				if (q->prev != NULL)
					q->prev->next = p;
				else
					list = p;
				q->prev = p;
				break;
			}
		}
	}

	/* reset head pointer just in case it moved */
	for (q = list; q != NULL; q = q->next)
		q->head = list;
	return (list);
}

/*
 * crackfile()
 *	Crack a file specification into component parts and initialize a
 *	file structure.  The types and their syntax are as follows:
 *
 * Executable types:
 *   Installation file/script:
 *	I: path_name:external_name:file_type:args
 *
 *   Demo file/script:
 *	E: path_name:external_name:file_type:args
 *	SUNW_RUN=path_name:external_name:file_type:args
 *
 * Text type:
 *	T: path_name:external_name:file_type
 *	SUNW_TEXT=path_name:external_name:file_type
 *
 * Bitmap type:
 *	B: path_name:bitmap_type
 *	SUNW_ICON=path_name:bitmap_type
 *
 * Parameters:	dir	 -
 *		buf	 -
 *		basetype -
 * Return:
 * Status:
 *	semi-private (internal library use only)
 */
File *
crackfile(char *dir, char *buf, FileType basetype)
{
	File	*file;
	char	*path = "";
	char	*name = "";
	char	*type = "";
	char	*args = "";
	char	*cp;
	int	plen, nlen, dlen;

	dlen = strlen(dir) + 1;	/* <dir>/ */
	/*
	 * For the first token, we try an '=' first
	 * assuming it's a packge -- if that doesn't
	 * work we'll fall back on _info syntax.
	 */
	path = get_value(buf, '=');
	if (path == NULL || path[0] == '\0') {
		path = get_value(buf, ':');
		if (path == NULL || path[0] == '\0')
			return ((File *)0);
	}

	file = (File *) xcalloc(sizeof (File));

	if (basetype == ICONFILE) {
		type = get_value(path, ':');
		if (type != NULL && type[0] != '\0') {
			cp = type;
			while (*cp != ':' && cp > path)
				--cp;
			plen = cp - path;
			if (type[0] == 'X')
				file->f_type = X11BITMAP;
			else
				file->f_type = PIXRECT;
		} else {
			plen = strlen(path);
			file->f_type = PIXRECT;
		}
		if (path[0] == '/') {
			/* absolute path used */
			file->f_path = (char *)xmalloc(plen + 1);
			(void) strncpy(file->f_path, path, plen);
			file->f_path[plen] = '\0';
		} else {
			file->f_path = (char *) xcalloc(dlen + plen + 1);
			(void) sprintf(file->f_path, "%s/", dir);
			(void) strncat(file->f_path, path, plen);
			file->f_path[dlen + plen] = '\0';
		}

		return (file);
	}

	file->f_data = (caddr_t)0;

	name = get_value(path, ':');
	if (name != NULL && name[0] != '\0') {
		for (cp = name; *cp != ':' && cp > path ; --cp)
			;
		/*
		 * X: path_name:external_name...
		 */
		plen = cp - path;
		type = get_value(name, ':');
		if (type != NULL && type[0] != '\0') {
			/*
			 * X: path_name:external_name:file_type...
			 */
			cp = type;
			while (*cp != ':' && cp > path)
				--cp;
			nlen = cp - name;
			args = get_value(type, ':');
			if (args == NULL)
				file->f_args = xstrdup("");
			else
				file->f_args = xstrdup(args);
		} else
			/* no file type spec */
			nlen = strlen(name);
		file->f_name = (char *) xmalloc(nlen + 1);
		(void) strncpy(file->f_name, name, nlen);
		file->f_name[nlen] = '\0';
	} else {
		plen = strlen(path);
		nlen = 0;
	}

	if (path[0] == '/') {
		/*
		 * Someone was stupid enough to use
		 * an absolute path...
		 */
		file->f_path = (char *)xmalloc(plen + 1);
		(void) strncpy(file->f_path, path, plen);
		file->f_path[plen] = '\0';
	} else {
		file->f_path = (char *)xmalloc(dlen + plen + 1);
		(void) sprintf(file->f_path, "%s/", dir);
		(void) strncat(file->f_path, path, plen);
		file->f_path[dlen + plen] = '\0';
	}

	switch (basetype) {
	case TEXTFILE:
		if (type == NULL || type[0] == 'P')
			file->f_type = POSTSCRIPT;
		else
			file->f_type = ASCII;
		break;
	case RUNFILE:
		if (type == NULL || type[0] == 'R')
			file->f_type = ROLLING;
		else
			file->f_type = EXECUTABLE;
		break;
	default:
		return (NULL);
	}
	return (file);
}

/*
 * keyvalue_parse()
 *	Convert a key-value pair line into canonical form.  The
 *	operation is performed in-place.
 *	The following conversions are performed:
 *	- remove leading white space.
 *	- remove any white space before or after the "=".
 *	- remove any comments (anything after a '#')
 *	- null-terminate the keyword.
 *	- remove trailing blanks.
 *	- if the line is empty after these conversions, convert the
 *	  string to the null string and return a value pointer of NULL.
 *
 * Parameters:	buf - a pointer to the string to be converted to canonical
 *		      form.
 * Return:
 *	a pointer to the value field.  Return NULL if none.
 *	at return, the original buffer now points to the null-terminated
 *	 	keyword only.  
 * Status:
 *	semi-private (internal library use only)
 */
char *
keyvalue_parse(char *buf)
{
	char	*rp, *wp;
	char	*cp;
	int	len;

	if (buf == NULL)
		return (NULL);
	rp = buf;
	wp = buf;

	/* eat leading blanks */
	while (isspace(*rp))
		rp++;

	/* trim comments */

	if ((cp = strchr(rp, '#')) != NULL)
		*cp = '\0';

	/* trim trailing white space */

	len = strlen(rp);
	if (len > 0) {
		cp = rp + len - 1;  /* *cp points to last char */
		while (isspace(*cp) && cp >= rp - 1)
			cp--;
		/* cp points to last non-white char, or to rp - 1 */
		++cp;
		*cp = '\0';
	}

	if (strlen(rp) == 0) {
		*buf = '\0';
		return (NULL);
	}

	/*
	 *  We now know that there is at least one non-null char in the
	 *  line pointed to by rp (though not necessarily in the line
	 *  pointed to by buf, since we haven't collapsed buf yet.)
	 *  Leading and trailing blanks are gone, and comments are gone.
	 */

	/*  Move the keyword to the beginning of buf */
	while (!isspace(*rp) && *rp != '=' && *rp != '\0')
		*wp++ = *rp++;

	*wp++ = '\0';	/* keyword is now null-terminated */

	/* find the '=' (if there is one) */

	while (*rp != '\0' && isspace(*rp))
		rp++;

	if (*rp != '=')		/* there is no keyword-value */
		return (NULL);

	/* now skip over white space between the '=' and the value */
	while (*rp != '\0' && isspace(*rp))
		rp++;

	/*
	 *  rp now either points to the end of the string, or to the
	 *  beginning of the keyword's value.  If end-of-string, there is no
	 *  keyword value.
	 */

	if (*rp == '\0')
		return (NULL);
	else
		return (rp);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * pkg_order_cmp()
 *	Return the difference of the order of Node 'b' from the order of
 *	Node 'b'. Used in 'sortlist' to sort the hash-list.
 * Parameters:
 *	a	- pointer to Node 'a'
 *	b	- pointer to Node 'b'
 * Return:
 *	#	- order delta
 * Status:
 *	private
 */
static int
pkg_order_cmp(Node * a, Node * b)
{
	register Modinfo *infoa, *infob;

	infoa = (Modinfo *) a->data;
	infob = (Modinfo *) b->data;
	return ((infoa->m_order) - (infob->m_order));
}

/*
 * order_rest_pkgs()
 *	note that this will cause an infinate loop should circular
 * 	dependancies exist.  These are explicitly prohibited, but
 *	do occasionally occur.
 * Parameters:
 *	prod	- Product structure pointer
 *	p	- Node pointer to check dependancies on
 *	q	- Node pointer indicating where to start checking
 *	srtd	- Node pointer indicating start of chain
 * Return:
 *	none
 * Status:
 *	private
 */
static void
order_rest_pkgs(Product * prod, Node *p, Node *q, Node **srtd)
{
	/*
	 * search remainder for any packages p depends on
	 * -- p was inserted into middle of list, check remaining packages for
	 * any the p depends on, put them before p.  Do recursively to
	 * ensure that dependancies of packages which depend on p are also
	 * met.
	 */
	if (((Modinfo *)(p->data))->m_flags & IN_ORDER_FCN)
		return;
	((Modinfo *)(p->data))->m_flags |= IN_ORDER_FCN;
	while (q != prod->p_packages->list) {
		if (dependson(p, q) &&
		    (!((((Modinfo *)(q->data))->m_flags) & IN_ORDER_FCN))) {
			list_unlink(q);
			list_insert(q, p);	/* q before p */

			/* if p was the head, make q the new head */
			if (p == *srtd)
				*srtd = q;

			/*
			 * call recursively to ensure that q's dependancies
			 * are met
			 */
			order_rest_pkgs(prod, q, p->next, srtd);

			q = p->next;
		} else
			q = q->next;
	}
	((Modinfo *)(p->data))->m_flags &= ~IN_ORDER_FCN;
}

/*
 * order_pkgs()
 *
 * Parameters:
 *	prod	- Product structure pointer
 * Return:
 *	none
 * Status:
 *	private
 */
static void
order_pkgs(Product * prod)
{
	Node	   *p, *q;
	Node	   *srtd, *rest;

	p = q = srtd = rest = (Node *) NULL;
	srtd = prod->p_packages->list->next;
	rest = srtd->next;
	srtd->next = srtd->prev = prod->p_packages->list;

	/*
	 * modified list_insertion sort...
	 * if p has no dependencies, place at beginning of list.
	 * else place p in list before first package dependent on it or at end.
	 * If p is placed into the middle of the list, check remaining
	 * packages for any the p depends on, put them before p.
	 */
	while (rest != prod->p_packages->list) {
		p = rest;
		rest = rest->next;

		if ((((Modinfo *) p->data)->m_pdepends == (Depend *) NULL) &&
			(((Modinfo *)p->data)->m_l10n_pkglist == NULL)) {
			/* no dependency */
			list_insert(p, srtd);		/* p is new head */
			srtd = p;
		} else {
			for (q = srtd; q->next != prod->p_packages->list;
					q = q->next) {
				if (dependson(q, p)) {	/* q has 1st dep. on p */
					list_insert(p, q); /* put p before q */

					if (q == srtd) /* HACK! q is 1st srtd pkg */
						srtd = p; /* caused by hashed list end nodes */
					break;
				}
			}

			if (q->next == prod->p_packages->list) {/* q is last pkg  */
				if (dependson(q, p)) {		/* is q dep. on p? */
					list_insert(p, q);	/* put p before q */

					if (q == srtd)	/* HACK! q was only srtd pkg */
						srtd=p;	/* caused by hashed list end nodes */
				} else
					list_append(p, q);	/* put p after q */
			} else {
				/* search remainder for any packages p depends on */
				q = q->next;
				order_rest_pkgs(prod, p, q, &srtd);

			}
		}
	}

	prod->p_packages->list->next = srtd;	/* put sorted list back */
}

/*
 * list_insert()
 *	Add the Node 'p' in front of Node 'q' in a list.
 * Parameters:
 *	p	- pointer to Node being added
 * 	q	- pointer to Node in list
 * Return:
 *	none
 * Status:
 *	private
 */
static void
list_insert(Node * p, Node * q)
{
	if (q->prev)
		q->prev->next = p;

	p->prev = q->prev;
	q->prev = p;
	p->next = q;
}

/*
 * list_append()
 *	Add the Node 'p' after Node 'q' in a list.
 * Parameters:
 *	p	- pointer to Node to be added
 *	q	- pointer to Node in list
 * Return:
 *	none
 * Status:
 *	private
 */
static void
list_append(Node * p, Node * q)
{
	if (q->next)
		q->next->prev = p;

	p->next = q->next;
	q->next = p;
	p->prev = q;
	return;
}

/*
 * list_unlink()
 *	Unlink a Node from a list.
 * Parameters:
 *	q	- pointer to Node to be unlinked
 * Return:
 *	none
 * Status:
 *	private
 */
static void
list_unlink(Node * q)
{
	if (q->prev)
		q->prev->next = q->next;

	if (q->next)
		q->next->prev = q->prev;

	q->prev = q->next = (Node *) NULL;
}

/*
 * dependson()
 *	See if the pkgid of the 'n2' module is in the dependency list of the
 *	'n1' module. If it's not in the pdepends list, check the l10n_pkglist.
 * Parameters:
 *	n1	- pointer to first Node
 *	n2 	- pointer to second Node
 * Return:
 *	0	- dependency is false
 *	1	- dependency is true
 * Status:
 *	private
 */
static int
dependson(Node * n1, Node * n2)
{
	Depend	 *tmp;
	char	 *str;
	char	 *version;
	char	 *delim;
	Modinfo	 *info1 = (Modinfo *) n1->data, *info2 = (Modinfo *) n2->data;

	/* does n1 depend on n2? is n2's name on n1's dependency list? */

	for (tmp = info1->m_pdepends; tmp; tmp = tmp->d_next) {
		if (strcmp(tmp->d_pkgid, info2->m_pkgid) == 0)
			return (1);
	}

	if (info1->m_l10n_pkglist == "")
		return (0);

	for (delim = str = info1->m_l10n_pkglist; str; str = delim) {
		if (delim = strchr(str, ','))
			*delim = '\0';
		if (version = strchr(str, ':')){
			*version = '\0';
			/* pkgid match. restore the list before returning */
			if (strcmp(str, info2->m_pkgid) == 0) {
				*version = ':';
				if (delim)
					*delim = ',';
				return (1);
			}
		} else {
			/* pkgid match. restore the list before returning */
			if (strcmp(str, info2->m_pkgid) == 0) {
				if (delim)
					*delim = ',';
				return (1);
			}
		}
		/* ??? */
		if (delim) {
			*delim = ',';
			delim++;
			if (strncmp(delim, "REV=", 4) == 0) {
				str = delim;
				if (delim = strchr(str, ','))
					delim++;
			}
		}
	}
	return (0);
}

/*
 * change_view_status()
 *	Change the status on the view associated with 'np'
 * Parameters:
 *	np	- pointer to Node holding view
 *	data	- flag field:	0 - view and all instances
 *				1 - locale
 *				2 - arch
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
change_view_status(Node * np, caddr_t data)
{
	View 	*vp;
	Modinfo *ip;

	vp = (View *)np->data;

	switch ((int)data) {

	case 0 : 	vp->v_status_ptr = &vp->v_info.v_mod->m_status;
			for (ip = next_inst(vp->v_info.v_mod); ip;
			    ip = next_inst(ip)) {
				vp = vp->v_instances;
				vp->v_status_ptr = &ip->m_status;
			}
			break;

	case 1: 	vp->v_status_ptr = (ModStatus*)&vp->v_info.v_locale->l_selected;
			break;

	case 2: 	vp->v_status_ptr = (ModStatus*)&vp->v_info.v_arch->a_selected;
			break;
	}

	return (SUCCESS);
}
/*
 * _set_primary_arch()
 *	Used by set_primary_arch() call to walklist(). If the Module is not
 *	UNSELECTED, walk the arch list, and for each arch structure, walk the
 *	Modinfo list associated with 'np' looking for the architecture instance
 *	which matches, and set the status field of those modinfos to the status
 *	of the Module, otherwise set it to be UNSELECTED.
 * Parameters:
 *	np	- pointer to Node currently being processed by walklist()
 *	data	- pointer to Product module
 * Return:
 *	SUCCESS
 * Status:
 *	private
 */
static int
_set_primary_arch(Node * np, caddr_t data)
{
	Modinfo	*info;
	Modinfo	*i;
	Arch	*arch;
	ModStatus stat;

	info = (Modinfo *)np->data;
	if ((stat = info->m_status) == UNSELECTED)
		return (SUCCESS);

	/*LINTED [alignment ok]*/
	for (arch = ((Module *)data)->info.prod->p_arches; arch != NULL;
							arch = arch->a_next) {
		for (i = info; i != NULL; i = next_inst(i))
			if (strcmp(arch->a_arch, i->m_arch) == 0) {
				if (arch->a_selected)
					i->m_status = stat;
				else
					i->m_status = UNSELECTED;
			}
	}
	return (SUCCESS);
}

/*
 * isa_handled()
 * Parametrs:
 *	isa -
 * Return:
 * Status:
 *	private
 */
struct isa_entry {
	struct isa_entry	*next;
	char			isaname[16];
};

static struct isa_entry *isachain = NULL;

int
isa_handled(char *isa)
{
	struct isa_entry *ip;

	for (ip = isachain; ip != NULL; ip = ip->next)
		if (strcmp(isa, ip->isaname) == 0)
			return (1);
	ip = (struct isa_entry *) xmalloc((size_t) sizeof (struct isa_entry));
	(void) strcpy(ip->isaname, isa);
	ip->next = isachain;
	isachain = ip;
	return (0);
}

void
isa_handled_clear()
{
	struct isa_entry *ip, *ipnext;

	for (ip = isachain; ip != NULL; ) {
		ipnext = ip->next;
		free(ip);
		ip = ipnext;
	}
	isachain = NULL;
}

/*ARGSUSED0*/
void
enter_swlib(char *funcname)
{
	return;
}

void
exit_swlib(void)
{
	return;
}

/*
 * Function:	UsrpackagesExist
 * Description:	Check the system to see if critical "/usr" packages have
 *		been installed in the system mounted relative to get_rootdir().
 *		"/usr" files must exist on all systems for upgradeability (i.e.
 *		dataless systems will not have these files). The existence
 *		of "/usr" files is achieved by looking for the existence of the
 *		SUNWcsu package.
 * Scope:	private
 * Parameters:	none
 * Return:	1	- /usr packages are present
 *		0	- /usr packages do not exist
 */
int
UsrpackagesExist(void)
{
	char	path[MAXPATHLEN];

	(void) sprintf(path, "%s/var/sadm/pkg/SUNWcsu", get_rootdir());
	if (access(path, F_OK) == 0)
		return (1);

	return (0);
}

void
set_is_upgrade(int i)
{
	s_is_upgrade = i;
}
