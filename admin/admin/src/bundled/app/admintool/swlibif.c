/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)swlibif.c	1.38 96/09/23 Sun Microsystems"

/*	swlibif.c 	*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <nl_types.h>
#include <X11/Intrinsic.h>

#include "spmisoft_api.h"
#include "media.h"
#include "software.h"
#include "util.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

static Module   *installed_media = NULL;/* installed service we're modifying */
static FSspace** currentFSspace = NULL;

FSspace**
installed_fs_layout()
{
    if (currentFSspace == NULL)
 	currentFSspace = load_current_fs_layout();

    return(currentFSspace);
}

/*
 * Return size in KB of a pkg
 */
int
pkg_size(Modinfo * pkg)
{
    int i;
    int sz = 0;

    if (pkg->m_shared == NULLPKG) {
	return(pkg_size((Modinfo *)pkg->m_instances->data));
    }
    for (i = 0; i < N_LOCAL_FS; i++) {
        sz += pkg->m_deflt_fs[i];
    }
    return(sz);
}
/*
 * Compute size of L10N packages with locale 'loc'
 * linked off a module m.
 */
int
calc_l10n_size(Module * m, char * loc)
{
    Modinfo * mi;
    L10N * l;
    int sz = 0;

    if (m == NULL || m->type != PACKAGE)
    	return(0);

    mi = m->info.mod;
    if (mi == NULL || mi->m_l10n == NULL)
        return(0);

    l = mi->m_l10n;
    while (l) {
        mi = l->l10n_package;
        if (mi->m_status == SELECTED && strcmp(mi->m_locale, loc) == 0)
	    sz += pkg_size(mi);
	l = l->l10n_next;
    }
    return(sz);
}

Module * 
get_parent_product(Module * m)
{
	Module * p;
	p = m;
	while (p && p->type != PRODUCT && p->type != NULLPRODUCT)
		p = p->parent;

	return(p);
}

/* 
 * Compute size of selected pkgs rooted at module 'm'.
 * If 'locale' is non-null calculate only pkgs that do
 * localization for locale 'locale'.
 */
int
selectedModuleSize(Module * m, char * locale)
{
     Modinfo * mi = NULL;
     int sz = 0;

     if (m->type == PRODUCT || m->type == NULLPRODUCT ||
         m->type == CLUSTER || m->type == METACLUSTER) {
     	Module * s = get_sub(m);
    	while (s) {
	    sz += selectedModuleSize(s, locale);
	    s = get_next(s);
	}
     } else if (m->type == PACKAGE) {
        if (locale) {
		/* 
                 * If base pkg has a l10n list, sum from that list.
                 * If not, test if locale == locale of m, if they are
                 * then base pkg is a relevant to this locale, so
                 * return its size.
                 */
		if (m->info.mod->m_l10n)	
	            sz = calc_l10n_size(m, locale);
	        else if (m->info.mod->m_locale &&
			 (strcmp(locale, m->info.mod->m_locale) == 0) &&
  	                 (m->info.mod->m_status == SELECTED))
		    sz = pkg_size(m->info.mod);
        } else
  	    if (m->info.mod->m_status == SELECTED)
	        sz = pkg_size(m->info.mod);
     }
     return(sz);
}

void
unload_installed_media()
{
    if (installed_media)
	unload_media(installed_media);
}

void
set_installed_media(Module *media)
{
        installed_media = media;
}

Module *
get_installed_media(void)
{
    Module *m, *next;
    for (m = get_media_head(); m != NULL; m = next) {
	next = m->next;
	if (m->info.media->med_type == INSTALLED)
	    return(m);
    }
    return (NULL);
}

void
reset_media()
{
	Module * media, * next;

	for (media = get_media_head(); media != (Module *)0; media = next) {
          next = media->next;
          if (media->type != MEDIA)
                continue;       /* sanity check */

	  unload_media(media);
	}
#if 0
	char installed_media_dir[MAXPATHLEN];

	media = get_installed_media();

	if (media != (Module *)0)
		(void) realpath(media->info.media->med_dir,installed_media_dir);
	else
		installed_media_dir[0] = '\0';

	for (media = get_media_head(); media != (Module *)0; media = next) {
          next = media->next;
          if (media->type != MEDIA)
                continue;       /* sanity check */
          if (media->info.media->med_type == INSTALLED ||
              media->info.media->med_type == INSTALLED_SVC)
                if (unload_media(media) != SUCCESS)
                  fatal(catgets(_catd, 8, 446, "PANIC: cannot free list of installed software!\n"));
       }

	media = load_installed("/", FALSE);
	if (media == (Module *)0)
		fatal(catgets(_catd, 8, 447, "PANIC:  cannot reload list of installed software!\n"));	

	/*
         * Restore installed media pointer
	 */
        if (installed_media_dir[0] != '\0')
                set_installed_media(find_media(installed_media_dir, (char *)0));
#endif

}

int
init_sw_lib()
{
	Module * media;

	sw_lib_init(NULL);

	/*
	 * Get the information about software
	 * installed on the local system.
	 * load_installed returns NULL if admintool is running
         * on < 2.5 of OS.
	 */
	media = load_installed("/", FALSE);
	if (media == NULL)
		return(0);	
	installed_media = find_media("/", (char*)0);
	set_installed_media(installed_media);


	return(1);
}

Module * 
get_installed_sw()
{
	static Module *m;

	installed_media = get_installed_media();

	for (m = installed_media->sub; m; m = m->next)
		if (strcmp(m->info.mod->m_pkgid, REQD_METACLUSTER) == 0) {
			mark_required(m);
			set_current(m);
			break;
		}

	set_current(installed_media);

	return (installed_media->sub);
}

Module *
get_source_sw(char * media_path)
{

	extern int FudInvalid;
        int retries;

#ifdef SW_INSTALLER
	extern Widget addsoftwaredialog;
	Widget dialog = addsoftwaredialog;
#else
	extern Widget sysmgrmain;
	Widget dialog = sysmgrmain;
#endif
	static Module *media = NULL;
	Media* med;
	int status;
	char	msg[1024];
	Module	* retm = NULL;
	Module * head;
	extern Module* head_ptr;

#if 0
	/* load software modules */
	if (media)			/* Unload old before loading new */
	    unload_media(media); 
#endif
	FudInvalid = 1;
	/* media = add_media(strdup(media_path)); */

#ifdef USE_ADD_MEDIA_TO_HEAD
	media = add_media_to_head(media_path);
#endif

	media = (Module*) malloc(sizeof(Module));
	memset(media, 0, sizeof(Module));

	med = (Media*) malloc(sizeof(Media));
	memset(med, 0, sizeof(Media));

	med->med_dir = strdup(media_path);
	med->med_type = ANYTYPE;
   	media->info.media = med;
	media->type = MEDIA;

/* BEGIN very dangerous code.

 Until unload_media() works and I can add a media
   to head of media list, I am going to manipulate media head_ptr
   directly. One would have to stretch their imagination to think
   of a more evil coding practice.
  
   Do you think I am going to put my name on this?
*/

        head = get_media_head();

	if (head) {
	    head->prev = media;
	    media->next = head;
	    head_ptr = media;

/* END very dangerous code. */
	} else 
	    media = add_media(strdup(media_path));

	
	retries = 0;
retry:
	status = load_media(media, 1);	/* use .packagetoc if present */

	switch (status) {
		case ERR_INVALIDTYPE:
			display_error(dialog, catgets(_catd, 8, 448, "Media type invalid"));
			media = (Module *)0;
			break;
		case ERR_NOMEDIA:
			display_error(dialog, catgets(_catd, 8, 449, "Media not found"));
			media = (Module *)0;
			break;
		case ERR_UMOUNTED:
			display_error(dialog, catgets(_catd, 8, 450, "Media not mounted"));
			media = (Module *)0;
			break;
		case ERR_NOPROD:
		case ERR_NOLOAD:
			display_error(dialog,
			    catgets(_catd, 8, 451, "No installable software on the media.\nIf the software on the media is grouped in\nsub-directories, try specifying the name of\n one of the subdirectories.\n"));
			media = (Module *)0;
			break;
		case SUCCESS:
		case ERR_NOFILE:	/* no .clustertoc file */
			retries = 0;
			FudInvalid = 0;
			break;
		default:
		/* If the s/w library were any more understandable, I would
		   be embarassed by this code. For some reason, when changin	
		   media, a strange error is report only every other time. SO,
		   instead of bugging user with a message, I simply give it
	           another shot. If it fails more that twice, I show message.
		*/
			retries++;
		        if (retries > 1) {	
			  sprintf(msg, catgets(_catd, 8, 455, "Unspecified media error (%d)"),status);
			  display_error(dialog, msg); 
			  media = (Module *)0;
		        } else 
				goto retry;
			break;
	}

	if (media != NULL) {
		set_current(media);
		retm = get_sub(media);
		if (retm == NULL) {
			display_error(dialog,
			    catgets(_catd, 8, 456, "No installable software on the media.\nIf the software on the media is grouped in\nsub-directories, try specifying the name of\none of the subdirectories.\n"));
			return(NULL);
		} else
			return (retm);
	}
	else {
		return(NULL);
	}

}

static int sw_lib_initialized = 0;

Module * 
sysman_list_sw(char * src)
{

	Module * m = NULL;

	if (sw_lib_initialized == 0) {
	    if (init_sw_lib() == 0)
		return(NULL);
	} 
/*
	else
	    reset_media();
*/


	if (src == NULL) {
		m = get_installed_sw();
#ifdef METER
                currentFSspace = installed_fs_layout();
#endif
	}
	else {
		m = get_source_sw(src);
	}
        sw_lib_initialized = 1;
	return(m);

}

/*
 *  Traverse L10N list and mark pkgs that localize
 *  for locale 'locale'
 */
void
mark_l10n(L10N * l10n, ModStatus status, char * locale)
{
	L10N * l = l10n;
	while (l) {
            Modinfo * mi = l->l10n_package;
	    if (strcmp(mi->m_locale, locale) == 0) {
	        mi->m_status = status;
 		mi->m_action = ((status == SELECTED) ? TO_BE_PKGADDED :
						      NO_ACTION_DEFINED);
	    }
  	    l = l->l10n_next;
	}
}

/*
 * This is a generalized version of mark_module from swlib.
 * Any type of Module can be passed as arg and mark_module
 * is called appropriately. For product/meta/cluster mark_cluster
 * is called recursively. 
 * If locale is non-NULL, mark ONLY those pkgs which localize for
 * that locale.
 */

void
mark_cluster(Module * mod, ModStatus status, char * locale)
{
    Module * s;
    if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
	for (s = mod->sub; s != NULL; s = s->next)
		mark_cluster(s, status, locale);
    } 
    else if (mod->type == CLUSTER || mod->type == METACLUSTER) {
 	mod->info.mod->m_status = status;
 	mod->info.mod->m_action = ((status == SELECTED) ? TO_BE_PKGADDED :
                                                      NO_ACTION_DEFINED);
	s = get_sub(mod);
	while (s) {
 	    mark_cluster(s, status, locale);
	    s = get_next(s);
	}
    }
    else if (mod->type == PACKAGE) {
	L10N * l10n;
        if (locale) {
	    /* 
             * If module is a "base" pkg and localizations are
             * hung off l10n list, then set appropriate l10n
             * packages.
             */
	    if (l10n = mod->info.mod->m_l10n)
		mark_l10n(l10n, status, locale);
	    else {
	    /* 
             * Else, it may be that base pkg is in fact a
             * localization pkg. If its m_locale is non-NULL
             * and matches 'locale', then this is case and
             * set status of base pkg.
             */
	        if (mod->info.mod->m_locale &&
	             	strcmp(locale, mod->info.mod->m_locale) == 0)
	 	    mod->info.mod->m_status = status;
 		    mod->info.mod->m_action = 
		      (status == SELECTED) ? TO_BE_PKGADDED : NO_ACTION_DEFINED;
	     }
	}
	else {
	    mod->info.mod->m_status = status;
 	    mod->info.mod->m_action = ((status == SELECTED) ? TO_BE_PKGADDED :
                                                      NO_ACTION_DEFINED);
	}
    }
}

Boolean
is_sub(Module * m, char * pkgid)
{
	Module * sub;

	if (m->type == PACKAGE)
		return(False);

	sub = m->sub;
	while (sub) {
		if (strcmp(sub->info.mod->m_pkgid, pkgid) == 0) {
			return(True);
		}
		sub = sub->next;
	}
	return(False);
}


/*
 * Determine if Module pointed to by *m is in 
 * cluster pointed to by *cluster. If so, return 1.
 */
static int
isInCluster(Module * cluster, Module * m)
{
	Module * c = cluster->sub;

	/* 
         * The number of tests here is a bit of overkill
         * but won't hurt for now and will support future
         * expansion of Module tree wherein you can have
         * e.g. metaclusters containing metaclusters.
         */
	if (cluster->type != CLUSTER &&
            cluster->type != METACLUSTER &&
	    cluster->type != NULLPRODUCT &&
	    cluster->type != PRODUCT)
		return(0);

	if (c == NULL || m == NULL)
		return(0);

	while (c) {
		if (c->type == CLUSTER ||
		    c->type == METACLUSTER ||
		    c->type == NULLPRODUCT ||
	            c->type == PRODUCT) {
			return(isInCluster(c, m));
		} else if (strcmp(get_mod_name(c), get_mod_name(m)) == 0) {
			return(1);
		}
		c = c->next;
  	}
	return(0);
}

/*
 * Determine if the addition of Module m is "risky."
 *
 * Current definition of "risky" is whether or not
 * the specified module is contained within the
 * Core Solaris metacluster.
 *
 * It only makes sense to call this routine if Solaris CD
 * has been recognized.
 */

#define CORE_NAME "Core System Support"

int
isRiskyToAdd(addCtxt * ctxt, Module *m)
{
	Module * core_m;
	char * nm;

	/* 
         * The images for which we have a definition of "risky"
         * is a Solaris image. If not, bail.
         */
	if (!ctxt->isSolarisImage || (m == NULL))
		return(0);

	/* Get top of tree */
	core_m = ctxt->top_level_module->sub;
	
	/* 
         * Look at top level metaclusters until Core is
	 * found.
         */
	while (core_m &&
	  	(core_m->type == METACLUSTER) &&
		(nm = get_mod_name(core_m)) &&
		(strncmp(nm, CORE_NAME, strlen(CORE_NAME)) != 0)) {
	    core_m = core_m->next;
	}	
	if (!core_m || !nm)
		return(0);

	return(isInCluster(core_m, m));
}

/*
 * See whether a pkginfo command issued against the spooled root
 * directory succeeds.  It if does, the directory is in package
 * format.
 *
 * return TRUE if it's in package format
 * else, return FALSE
 *
 * This is an adaption from src/common/lib/libsw/api_services.c
 *
 * This is required to accommodate bad .packagetoc files and 
 * because it is the safe thing to do.
 */
 
Boolean
pkg_exists(char *spooled_root, char * pkg_dir)
{
        int             i;
        int             status;
        char            pkginfo_cmd[MAXPATHLEN];
	struct stat	statb;
 
	if (pkg_dir == NULL)
		return(FALSE);

#ifdef USE_PKGINFO
	/*
         * Run a pkginfo to determine is named path is a pkg.
         * This is time consuming...
         */
        sprintf(pkginfo_cmd, "/usr/bin/pkginfo  -d %s %s >> /dev/null 2>&1",
                spooled_root, pkg_dir);
        if ((status = system(pkginfo_cmd)) == 0) {
#else
	/* 
         * This is a quicker, yet less reliable method of
         * determining whether a pkg exists.
         */
        sprintf(pkginfo_cmd, "%s/%s", spooled_root, pkg_dir);
	if (stat(pkginfo_cmd, &statb) == 0) {
#endif
	    return(TRUE);
	} else {
	    return(FALSE);
       }
}

/* 
 * Traverse Module tree rooted and Module 'm' and
 * determine if it and constituent packages are a
 * valid collections of packages. Validity check is
 * ultimately performed by pkg_exists().
 */
int
isValidModule(addCtxt * ctxt, Module * m)
{
    char * pkg_path;

    if (ctxt == NULL || m == NULL)
	return (0);

    if (m->type == PRODUCT || m->type == CLUSTER || m->type == METACLUSTER) {
	int ret = 1;
    	Module * s = get_sub(m);
        while (s) {
	    ret = isValidModule(ctxt, s);
            if (ret == 0)
 		return(0);
	    s = get_next(s);
        }	
	return( ret );
    }
    else if (m->type == PACKAGE) {
        /* 
         * Since some Module trees don't come with a PRODUCT
         * module, I have to rely on what user typed in as path
         * to packages.
         */
        if (ctxt->current_product)
	    pkg_path = ctxt->current_product->info.prod->p_pkgdir;
	else
            pkg_path = ctxt->pkg_path ? ctxt->pkg_path : ctxt->install_path;

	if (m->info.mod->m_shared == NULLPKG) {
	    int ret = 1;
	    Node *i = m->info.mod->m_instances;
	    while (i) {
		Modinfo *mi = (Modinfo *)i->data;
		ret = pkg_exists(pkg_path, mi->m_pkg_dir ?
		    mi->m_pkg_dir :
		    mi->m_pkgid);
		if (ret == 0)
		    return(0);
		i = i->next;
	    }	
	    return( ret );
	} else {
	    return(pkg_exists(pkg_path, m->info.mod->m_pkg_dir ?
		    m->info.mod->m_pkg_dir :
		    m->info.mod->m_pkgid));
	}
    }
    return(0);
}

/*
 * Set selection status of pkg to UNSELECTED.
 */

static void
reset_pkg(Modinfo * mi)
{
    mi->m_status = UNSELECTED;
    mi->m_action = NO_ACTION_DEFINED;
    mi->m_refcnt = 0;
}

/* 
 * Reset status of module to UNSELECTED and set its refcnt
 * field to zero.
 *
 * This function is called by walklist.
 *
 * I am on thin ice here, circumventing the swlib interface.
 * Caveat, developer.
 */
int
reset_status(Node *node, caddr_t arg)
{
	Modinfo * mods = (Modinfo *) node->data;
	addCtxt * ctxt = (addCtxt *) arg;
if (mods->m_status == SELECTED)
#ifdef TEST_ADD
	fprintf(stderr, "resetting %s(%s)\n", mods->m_name, mods->m_pkgid ?
							    mods->m_pkgid :"x");
#endif
	reset_pkg(mods);	
}

/* 
 * Set selected status of all modules to UNSELECTED by calling 
 * reset_pkg() and recurse.
 *
 * This differs from reset_status in that it is called directly.
 */
void
reset_module(Module * m)
{
    Module * s;
    L10N * l;
    if (m->type == CLUSTER || m->type == METACLUSTER) {
	reset_pkg(m->info.mod);
	s = get_sub(m);
	while (s) {
	    reset_module(s);
	    s = get_next(s);
	}
    }
    else {
	reset_pkg(m->info.mod);
	if (l = m->info.mod->m_l10n) {
	    while (l) {
		reset_pkg(l->l10n_package);
		l = l->l10n_next;
	    }
	}
    }
}

/*
 * Given an L10N list, return selected status.
 * Only consider those pkgs that match the locale
 * passed as 'loc' arg. If 'loc' is NULL, then
 * consider status of all L10N pkgs.
 */
ModStatus
getL10NselectedStatus(L10N * l, char * loc)
{
    Modinfo * mi;
    int lsel = 0, lpart = 0, lun = 0;
    int ltot = 0;
    while (l) {
        mi = l->l10n_package;
	/* skip those pkgs that don't match locale */
        if (loc && strcmp(mi->m_locale, loc)) {
	    l = l->l10n_next;
	    continue;
	}
	switch (mi->m_status) {
	case SELECTED:
		lsel++; break;
	case UNSELECTED:
		lun++; break;
	case PARTIALLY_SELECTED:
		lpart++; break;
	}
	ltot++;
	l = l->l10n_next;
    }
    if (lun == ltot)
        return(UNSELECTED);
    else if (lsel == ltot)
        return(SELECTED);
    else if ((lsel && (lsel < ltot)) || lpart)
        return(PARTIALLY_SELECTED);
}

/*
 * Return selection status of module 'm'. If 'loc' is
 * non-NULL, then consider only status of pkg that
 * localizes 'm' as relevant.
 */
ModStatus
getModuleStatus(Module * m, char * loc)
{
    extern L10N * getL10Ns(Module *);
    int tot = 0, un = 0, part = 0, sel = 0;

    if (m->type == CLUSTER || m->type == METACLUSTER ||
       m->type == PRODUCT || m->type == NULLPRODUCT) {
	 Module * s = get_sub(m);
	 while (s) {
            switch(getModuleStatus(s, loc)) {
	        case SELECTED:
			sel++;
			break;
		 case PARTIALLY_SELECTED:
			part++;
			break;
		 case UNSELECTED:
			un++;
			break;
	    }
	    tot++;
	    s = get_next(s);
	}
	if (un == tot)
	    return(UNSELECTED);
	else if (sel == tot)
	    return(SELECTED);
	else if ((sel && (sel < tot)) || part)
	    return(PARTIALLY_SELECTED);
    }
    else  {
	L10N * l;
	if (loc) {
	    l = getL10Ns(m);
	    if (l)
	        return (getL10NselectedStatus(l, loc));
	    else if (m->info.mod->m_locale && 
		     strcmp(loc, m->info.mod->m_locale) == 0)
		return(m->info.mod->m_status);
	    else
		return(UNSELECTED);
	} else
	    return(m->info.mod->m_status);
    }
}

/*
 * The following functions are dummy placeholders required
 * by the libraries for callback progress displays
 */

void
cleanup_and_exit()
{
	exit(2);
}

void
progress_init()
{
}

void
progress_done()
{
}

void
progress_cleanup()
{
}

void
interactive_pkgadd(int *result)
{
}

void
interactive_pkgrm(int *result)
{
}

int
start_pkgadd(char *pkgdir)
{
	return (1);
}

int
end_pkgadd(char *pkgdir)
{
	return (1);
}

/*
 * Traverse list of products calling walklist() for each one.
 * Operation performed by walklist() is passed as 2nd arguement.
 * 3rd arg is 'call data' to walklist. If cd == NULL, then pass
 * Module m itself.
 */

void
apply_to_all_products(addCtxt * ctxt, int (func)(Node *, caddr_t), caddr_t cd)
{
    Module * m = get_parent_product(ctxt->top_level_module);
    while (m) {
        ctxt->current_product = m; 
 	walklist(m->info.prod->p_packages,  func, cd ? cd : (caddr_t) m);
	m = get_next(m);
    }
}

#include "spmicommon_api.h"
#include "spmistore_api.h"

/*
 * admintool_space_meter()
 *
 * This is slightly changed version of libspmisvc:space_meter.
 *  1) mplist arg can not be NULL.
 *  2) _SECOND_ entry from media list is used.
 * 
 *
 *      Allocate a space table based on either the default mount points
 *      or the ones listed in in 'mplist'. Run the software tree and
 *      populate the table.
 * Parameters:
 *      mplist   - array of mount points for which space is to be metered.
 * Return:
 *      NULL     - invalid mount point list
 *      Space ** - pointer to allocated and initialized array of space
 *                 structures
 * Status:
 *      public
 */
FSspace **
admintool_space_meter(char **mplist)
{
        Module  *mod, *prodmod;
        Product *prod;
        static FSspace **new_sp = NULL;

        if (mplist != (char **)NULL && mplist[0] == (char *)NULL)
                mplist = NULL;

        if (!valid_mountp_list(mplist)) {
#ifdef DEBUG
                (void) printf(
                        "DEBUG: space_meter(): Invalid mount point passed\n");
#endif
                return (NULL);
        }

        if ((mod = get_media_head()) == (Module *)NULL) {
#ifdef DEBUG
                (void) printf("DEBUG: space_meter(): media head NULL\n");
#endif
                return (NULL);
        }


        prodmod = mod->sub;
        prod = prodmod->info.prod;

        /* set up the space table */
        new_sp = (FSspace**) load_defined_spacetab(mplist);

        if (new_sp == NULL)
                return (NULL);

        if (calc_sw_fs_usage(new_sp, NULL, NULL) != SUCCESS)
                return (NULL);

        return (new_sp);
}

