#ifndef lint
#ident   "@(#)locale.c 1.37 95/02/24 SMI"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
#include "sw_lib.h"

/* Local Statics and Constants */

static struct loclang {
	char	   *locale;
	char	   *language;
};

/*
 * Order these so the known used ones are first.
 */
static struct loclang loc_array[] = {
	{"C", "Default Locale"},
	{"ca", "Catalan"},
	{"de", "German"},
	{"en", "English"},
	{"es", "Spanish"},
	{"fr", "French"},
	{"it", "Italian"},
	{"ja", "Japanese"},
	{"ko", "Korean"},
	{"sv", "Swedish"},
	{"zh", "Chinese"},
	{"zh_TW", "Chinese/Taiwan"},
	{"ar", "Arabic"},
	{"bg", "Bulgarian"},
	{"co", "Corsican"},
	{"cs", "Czech"},
	{"cy", "Welsh"},
	{"da", "Danish"},
	{"de_CH", "Swiss German"},
	{"el", "Greek"},
	{"en_UK", "English/UK"},
	{"en_US", "English/USA"},
	{"eo", "Esperanto"},
	{"eu", "Basque"},
	{"fa", "Persian"},
	{"fi", "Finnish"},
	{"fr_BE", "French/Belgium"},
	{"fr_CA", "Canadian French"},
	{"fr_CH", "Swiss French"},
	{"fy", "Frisian"},
	{"ga", "Irish"},
	{"gd", "Scots Gaelic"},
	{"hu", "Hungarian"},
	{"is", "Icelandic"},
	{"iw", "Hebrew"},
	{"ji", "Yiddish"},
	{"kl", "Greenlandic"},
	{"lv", "Latvian"},
	{"nl", "Dutch"},
	{"no", "Norwegian"},
	{"pl", "Polish"},
	{"pt", "Portuguese"},
	{"ro", "Romanian"},
	{"ru", "Russian"},
	{"sh", "Serbo-Croatian"},
	{"sk", "Slovak"},
	{"sr", "Serbian"},
	{"tr", "Turkish"}
};

/*
 * this just gets the strings into the .po file for
 * translation.  I do a get text later when actually displaying
 * the string...
 */
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Default Locale")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Arabic")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Bulgarian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Catalan")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Corsican")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Czech")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Welsh")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Danish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "German")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Swiss German")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Greek")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "English")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "English/UK")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "English/USA")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Esperanto")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Spanish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Basque")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Persian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Finnish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "French")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "French/Belgium")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Canadian French")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Swiss French")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Frisian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Irish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Scots Gaelic")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Hungarian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Icelandic")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Italian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Hebrew")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Japanese")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Yiddish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Greenlandic")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Korean")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Latvian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Dutch")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Norwegian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Polish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Portuguese")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Romanian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Russian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Serbo-Croatian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Slovak")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Serbian")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Swedish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Turkish")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Chinese")
#undef DUMMY
#define DUMMY dgettext("SUNW_INSTALL_SWLIB", "Chinese/Taiwan")
#undef DUMMY

/* Public Function Prototype */

Module * 	swi_get_all_locales(void);
void     	swi_update_l10n_package_status(Module *);
int      	swi_select_locale(Module *, char *);
int      	swi_deselect_locale(Module *, char *);
void     	swi_mark_locales(Module *, ModStatus);
int		swi_valid_locale(Module *, char *);

/* Library Function Prototype */

void     	localize_packages(Module *);
int      	add_locale(Module *, char *);
void     	sort_locales(Module *);
char		*get_lang_from_loc_array(char *);
char		*get_C_lang_from_locale(Module *, char *);

/* Local Function Prototype */

static int	resolve_package_l10n(Node *, caddr_t);
static int	l10n_status_from_locale_status(Node *, caddr_t);
static int	_l10n_status_from_locale_status(Modinfo *, Module *);
static char	*get_lang_from_locale(Module *, char *);
static int     	select_partial_locale(Module *, char *);
static int     	select_base_locale(Module *, char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * get_all_locales()
 *	Return the list of locale modules assocated with the current
 *	product.
 * Parameters:
 *	none
 * Return:
 *	NULL	 - no locales associated with the current product
 *	Module * - pointer to locale list
 * Status:
 *	public
 */
Module *
swi_get_all_locales(void)
{
	Module	*prod = get_current_product();
#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("get_all_locales");
#endif

	return (prod->info.prod->p_locale);
}

/*
 * update_l10n_package_status()
 *	Mark the localization packages as selected or unselected base of the 
 *	locale status of the associated product 'prod', and the status of each 
 *	package in the product (i.e.  SELECTED or REQUIRED). If 'prod' is NULL,
 *	the current product is used.  This function must be called each time 
 *	the status of one (or more) locales changes.
 * Parameters:
 *	prod	- pointer to product module
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_update_l10n_package_status(Module * prod)
{
	Module	*p;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("update_l10n_package_status");
#endif

	if (prod == NULL)
		p = get_current_product();
	else
		p = prod;

	walklist(p->info.prod->p_packages,
			l10n_status_from_locale_status, (caddr_t)p);
}

/*
 * deselect_locale()
 *	If the product has a locale structure of the specified type,
 *	set the status of the locale structure to UNSELECTED.
 * Parameters:
 *	mod	- pointer to product module
 *	locale	- specifies name of locale to be deselected
 * Return:
 *	ERR_INVALIDTYPE	- 'mod' is neither PRODUCT or NULLPRODUCT
 *	ERR_BADLOCALE	- invalid locale parameter specified for this
 *			  product
 *	SUCCESS		- locale structure of type 'locale' cleared
 *			  successfully
 * Status:
 *	public
 */
int
swi_deselect_locale(Module *mod, char *locale)
{
	Module *m;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("deselect_locale");
#endif

	if (mod->type != PRODUCT && mod->type != NULLPRODUCT)
		return (ERR_INVALIDTYPE);

	for (m = mod->info.prod->p_locale; m != NULL; m = m->next) {
		if (strcmp(locale, m->info.locale->l_locale) == 0) {
			m->info.locale->l_selected = UNSELECTED;
			return (SUCCESS);
		}
	}

	return (ERR_BADLOCALE);
}

/*
 * mark_locales()
 *	For each of the localization packages associated with a given
 *	module, run through the product locale chain, and for each
 *	locale is selected, mark the appropriate localization package
 *	as selected (or increment its reference count). For those marked
 *	as unselected, decrement the associated localization package
 *	reference count (until it gets to 0, at which point, deselect it).
 * Parameters:
 *	mod	- pointer to package module
 *	status	- module status (SELECTED/UNSELECTED/REQUIRED)
 * Return:
 *	none
 * Status:
 *	public
 */
void
swi_mark_locales(Module * mod, ModStatus status)
{
	L10N	*l10np;		/* localizations for this package */
	Module	*prod = get_current_product();
	Module	*lp;		/* locales to install */

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("mark_locales");
#endif
#define l10np_info ((Modinfo *)l10np->l10n_package)

	/*
	 * If there is no current product, then find the product which
	 * is the parent of the module 'mod' by traversing back up the
	 * tree. If you find a product (which you should if everything
	 * is working correctly) set that product as the current product.
	 * Note that get_current_product() could originally be NULL
	 * because the default and current media are NULL.
	 */
	if (prod == (Module *)NULL) {
		for (prod = mod->parent; prod; prod = prod->parent) {
			if (prod->type == PRODUCT ||
						prod->type == NULLPRODUCT)
				break;
		}
		if (prod)
			set_current(prod);
	}

	/*
	 * for each l10n of this package
	 *     for each locale of the product
	 *	 if this localization package matches the locale
	 *	     if the package is selected & the locale is selected
	 *	         mark the l10n package as selected
	 *	     else
	 *	         mark the l10n package as unselected
	 */
	for (l10np = ((Modinfo *) mod->info.mod)->m_l10n; l10np;
						l10np = l10np->l10n_next)
		for (lp = prod->info.prod->p_locale; lp; lp = lp->next)
			if (strcmp(lp->info.locale->l_locale,
						l10np_info->m_locale) == 0) {
				/*
				 * need to test both lengths since
				 * "de" == "de_CH" when only 2 chars are checked
	 			 */
				if ((status == SELECTED || status == REQUIRED)
						&& lp->info.locale->l_selected)
					l10np_info->m_refcnt += 1;
				else if (status == UNSELECTED &&
						lp->info.locale->l_selected)
					l10np_info->m_refcnt =
						l10np_info->m_refcnt ?
						l10np_info->m_refcnt - 1 : 0;

				if (l10np_info->m_status != REQUIRED) {
					if (l10np_info->m_refcnt > 0)
						l10np_info->m_status = SELECTED;
					else
						l10np_info->m_status =
						    UNSELECTED;
				}
			}
#undef l10np_info

}

/*
 * valid_locale()
 *	Boolean function which checks the string 'locale'
 *	against all known locales and returns TRUE (1) if
 *	it is a valid (known) locale, and FALSE (0) if it
 *	is not.
 * Parameters:
 *	locale	- non-NULL pointer to case specific locale
 *		  string
 * Return:
 *	1	- locale matched
 *	0	- locale match failed
 */
int
swi_valid_locale(Module *prodmod, char *locale)
{
	if (get_C_lang_from_locale(prodmod, locale) == NULL)
		return (0);
	else
		return (1);
}

/*
 * select_locale()
 *	Break up a (possibly composite) locale string into individual
 *	locales and select each of them.
 *
 *	A composite locale looks like:
 *		/fr/fr/fr/fr/fr/C
 * Parameters:
 *	mod	- product module pointer (must be type PRODUCT or NULLPRODUCT)
 *	locale	- name of locale to be SELECTED
 * Return:
 *	ERR_INVALIDTYPE	- 'mod' is neither a PRODUCT or NULLPRODUCT
 *	ERR_BADLOCALE	- 'locale' is not part of the locale chain for 'mod'
 *	SUCCESS		- locale structure of type 'locale' set successfully
 * Status:
 *	public
 */
int
swi_select_locale(Module * mod, char * locale)
{
	int	ret, final_code;
	char	*cp;
	char	locstring[80];

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("select_locale");
#endif

	if (mod->type != PRODUCT && mod->type != NULLPRODUCT)
		return (ERR_INVALIDTYPE);

	final_code = SUCCESS;

	if (locale[0] == '/')		/* it's a composite locale */
		while (locale && *locale) {
			locale++;	/* skip over '/' */
			if ((cp = strchr(locale, '/')) != NULL) {
				strncpy(locstring, locale, cp - locale);
				locstring[cp - locale] = '\0';
			} else
				strcpy(locstring, locale);
			if (strlen(locstring) != 0) {
				ret = select_partial_locale(mod, locstring);
				if (ret != SUCCESS)
					final_code = ret;
			}
			locale = cp;
		}
	else
		final_code = select_partial_locale(mod, locale);
	
	return (final_code);
}
/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * sort_locales()
 *	Walk the locale chain for a given product and sort the
 *	language order alphabetically based on the language name.
 * Parameters:
 *	prod	- product to be sorted
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
sort_locales(Module * prod)
{

	Module  *p, *q;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("sort_locales");
#endif

	if (prod->info.prod->p_locale == NULL)
		return;

      	for (p = prod->info.prod->p_locale->next; p != NULL; p = p->next) {
	 	for (q = prod->info.prod->p_locale; q != p; q = q->next) {
			if (strcoll(p->info.locale->l_language,
					q->info.locale->l_language) < 0) {
				if (p->next != NULL)
					p->next->prev = p->prev;
				p->prev->next = p->next;
				p->prev = q->prev;
				p->next = q;

				if (q->prev != NULL)
					q->prev->next = p;
				else
					prod->info.prod->p_locale = p;

				q->prev = p;
				break;
			}
		}
	}
}

/*
 * localize_packages()
 *	Walk the product package list and select those packages which
 *	are affected by an active localization.
 * Parameters:
 *	prod	- pointer to product structure for product being walked
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
localize_packages(Module * prod)
{

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("localize_packages");
#endif
	if ((prod->type == PRODUCT) || (prod->type == NULLPRODUCT))
		walklist(prod->info.prod->p_packages,
				resolve_package_l10n, (caddr_t) prod);
}

/*
 * add_locale()
 *	Search for a specified locale in the current list of known
 *	product locales, and if it doesn't already exist, add it
 *	to the list. The locale must be legal (see loc_array[]) and
 *	the product structure must by type PRODUCT or NULLPRODUCT
 * Parameters:
 *	prod	- non-NULL Product structure pointer
 *	locale	- non-NULL locale
 * Return:
 *	ERR_INVALIDTYPE	- invalid product type
 *	ERR_INVALID	- invalid locale
 *	SUCCESS		- a valid locale structure was created
 *			  and added to the locale chain
 * Status:
 *	semi-private (internal library use only)
 */
int
add_locale(Module * prod, char * locale)
{
	Module	*lp, *lastlocale;
	char	*cp;


#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("add_locale");
#endif
	lp = lastlocale = (Module *) NULL;

	if ((prod->type != PRODUCT) && (prod->type != NULLPRODUCT))
		return (ERR_INVALIDTYPE);

	if ((cp = get_lang_from_locale(prod, locale)) == (char *)NULL
				|| strcmp(locale, "C") == 0)
		return (ERR_INVALID);

	for (lp = prod->info.prod->p_locale; lp; lp = lp->next) {
		if (strcmp(locale, lp->info.locale->l_locale) == 0)
			break;
		else
			lastlocale = lp;
	}

	if (lp == (Module *) NULL) {
		lp = (Module *) xcalloc(sizeof (Module));
		lp->info.locale = (Locale *) xcalloc(sizeof (Locale));
		lp->info.locale->l_locale = (char *)xstrdup(locale);
		lp->info.locale->l_language = cp;
		lp->type = LOCALE;
		lp->next = NULL;
		lp->sub = NULL;
		lp->head = prod->info.prod->p_locale;
		lp->parent = prod;

		if (lastlocale) {
			lastlocale->next = lp;
			lp->prev = lastlocale;
		} else {
			prod->info.prod->p_locale = lp;
			lp->prev = NULL;
		}
	}
	return (SUCCESS);
}

/*
 * get_lang_from_loc_array
 *
 * Attempt to map a locale id ("fr", "de") to its English
 * description, using the array loc_array.  THis is only used
 * for media that don't have locale_description files.
 */

char *
get_lang_from_loc_array(char *locale)
{
	int	i;
	int	high = (int) (sizeof (loc_array) / sizeof (struct loclang));

	for (i = 0; i < high; i++) {
		if (strcmp(locale, loc_array[i].locale) == 0)
			return (loc_array[i].locale);
	}
	return (NULL);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * resolve_package_l10n()
 *	Walklist() function used to
 * Parameters:
 *	np	- node pointer for package being processed
 *	data	- product structure pointer parenting the package list
 * Return:
 *	ERR_NOPROD - product structure invalid
 * Status:
 *	private
 */
static int
resolve_package_l10n(Node * np, caddr_t data)
{
	char	*buf;
	char	*pkgid;
	char	*version;
	char	*delim;
	char	*tmp;
	Node	*tn;
	Module	*prod;
	Module	*mp, *mp1, *mp2;
	L10N	*lp;

	/* use current product if data == NULL */
	if (data == NULL)
		prod = get_current_product();
	else
		/*LINTED [alignment ok]*/
		prod = (Module *)data;
	if (prod == NULL)
		return (ERR_NOPROD);

	/*
	 * if this package is a localization package (has locale and list of
	 * affected packages that is non-NULL)
	 */
	if (((Modinfo *) np->data)->m_locale &&
		(((Modinfo *) np->data)->m_l10n_pkglist) &&
		(*((Modinfo *) np->data)->m_l10n_pkglist)) {
		buf = (char *)xstrdup(((Modinfo *) np->data)->m_l10n_pkglist);
		pkgid = buf;

		do
		{
			/*
			 * break pkglist into components:
		  	 *
			 * pkg1:version, pkg2:version..., pkgn:version
			 *	^    ^      ^
			 * pkgid__|    |      |
			 * version_____|      |
			 * delim______________|
			 */
			if (delim = strchr(pkgid, ','))
				*delim = '\0';

			if (version = strchr(pkgid, ':')) {
				*version = '\0';
				++version;
				if (delim) {
					tmp = delim;
					++delim;
					if (strncmp(delim, "REV=", 4) == 0) {
						*tmp = ',';
						if (delim = strchr(++tmp, ','))
							*delim = '\0';
					} else
						delim = tmp;
				}

			}

			/*
		 	 * figure out if package specification matchings a `
			 * package we know about
		 	 */
			if ((tn = findnode(prod->info.prod->p_packages, pkgid))
							!= (Node *) NULL) {
				/*
		 		 * point package tn at package np if no
				 * version is specified or if the version
				 * specified exactly matches tn's version
				 */
				if (version == (char *) NULL || (version &&
						(strcmp(version, ((Modinfo *)
						tn->data)->m_version) == 0))) {
					lp = (L10N *) xcalloc(sizeof (L10N));
					lp->l10n_package = (Modinfo *) np->data;
					lp->l10n_next =
						((Modinfo *) tn->data)->m_l10n;
					((Modinfo *) tn->data)->m_l10n = lp;
				}
			}

			if (delim) {			/* another token? */
				/* skip any leading white space */
				for (++delim; delim && *delim &&
					   ((*delim == ' ') || (*delim == '\t')); ++delim);
				pkgid = delim;
			}
		} while (delim);

		free(buf);
	}
	/* Add to locale tree */
	if (((Modinfo *)np->data)->m_locale) {
		for (mp=prod->info.prod->p_locale; mp; mp = mp->next) {
			if (strcmp(((Modinfo *)np->data)->m_locale,
						mp->info.locale->l_locale) != 0)
				continue;
			mp2 = NULL;
			for (mp1=mp->sub; mp1; mp1 = mp1->next) {
				if (strcmp(mp1->info.mod->m_pkgid,
					((Modinfo *)np->data)->m_pkgid) == NULL)
					break;
				mp2 = mp1;
			}
			if (mp1)
				continue;
			if (!mp2) {
				mp1 = mp->sub =
					(Module *)xcalloc(sizeof (Module));
				mp1->prev = NULL;
			} else {
				mp1 = mp2->next =
					(Module *)xcalloc(sizeof (Module));
				mp1->prev = mp2;
			}
			mp1->info.mod = (Modinfo *)np->data;
			mp1->parent = mp;
			mp1->head = mp->sub;
		}
	}
	return (SUCCESS);
}
/*
 * get_lang_from_locale()
 *	Returns a descriptive string describing the language represented
 *	by `locale'.  The string will have been translated into the
 *	appropriate local language.
 * Parameters:
 *	locale	-
 * Return:
 *	
 * Status:
 *	private
 */
static char *
get_lang_from_locale(Module *prodmod, char * locale)
{
	LocMap	*lmap;

	if (prodmod->parent && prodmod->parent->info.media &&
	    (lmap = prodmod->parent->info.media->med_locmap) != NULL) {
		while (lmap) {
			if (strcmp(lmap->locmap_partial, locale) == 0) {
				if (lmap->locmap_description)
					return (dgettext("SUNW_LOCALE_DESCR",
					    lmap->locmap_description));
				else
					break;
			}
			lmap = lmap->next;
		}
	}

	return (dgettext("SUNW_INSTALL_SWLIB",
	    get_lang_from_loc_array(locale)));
}

/*
 * get_lang_from_locale()
 *	Returns a descriptive string describing the language represented
 *	by `locale'.  The string is not translated.
 * Parameters:
 *	locale	-
 * Return:
 *	
 * Status:
 *	private
 */
char *
get_C_lang_from_locale(Module *prodmod, char * locale)
{
	LocMap	*lmap;

	if (prodmod->parent && prodmod->parent->info.media &&
	    (lmap = prodmod->parent->info.media->med_locmap) != NULL) {
		while (lmap) {
			if (strcmp(lmap->locmap_partial, locale) == 0) {
				if (lmap->locmap_description)
					return (lmap->locmap_description);
				else
					break;
			}
			lmap = lmap->next;
		}
	}

	return (get_lang_from_loc_array(locale));
}

/*
 * l10n_status_from_locale_status()
 *	Run through all the packages associated with the product. If
 *	any of them have a locale field (its a localization package)
 *	and its locale matches a selected product locale, then mark
 *	it as selected and set its action.
 * Parameters:
 *	np	-
 *	data	-
 * Return:
 *	ERR_NOPROD
 *	SUCCESS
 * Status:
 *	private
 */
static int
l10n_status_from_locale_status(Node * np, caddr_t data)
{
	Modinfo *mi;
	Module  *prod;
	int	status;

	/*LINTED [alignment ok]*/
	prod = (Module *) data;
	mi = (Modinfo *) np->data;

	status = _l10n_status_from_locale_status(mi, prod);
	if (status != SUCCESS)
		return (status);
	while ((mi = next_inst(mi)) != NULL) {
		status = _l10n_status_from_locale_status(mi, prod);
		if (status != SUCCESS)
			return (status);
	}
	return (status);
}

/*
 * l10n_status_from_locale_status()
 *	Run through all the packages associated with the product. If
 *	any of them have a locale field (its a localization package)
 *	and its locale matches a selected product locale, then mark
 *	it as selected and set its action.
 * Parameters:
 *	l10n	-
 *	prod	-
 * Return:
 *	ERR_NOPROD
 *	SUCCESS
 * Status:
 *	private
 */
static int
_l10n_status_from_locale_status(Modinfo *l10n, Module *prod)
{
	Module  *lp;		/* locales to install */
	Node	*tn;
	char	*buf;
	char	*pkgid;
	char	*version;
	char	*delim;
	char	*tmp;
	int	env_is_service = 0;

	if (prod == NULL)
		return (ERR_NOPROD);

	/*
	 * if not a localization package, or if REQUIRED (it can't be
	 * deselected), return.
	 */
	if (l10n->m_locale == NULL || l10n->m_status == REQUIRED ||
	    l10n->m_shared == NULLPKG)
		return (SUCCESS);

	/*
	 *  Determine whether the environment being installed or
	 *  upgraded here is a service.
	 */
	if (prod->info.prod->p_current_view->p_view_from &&
	    prod->info.prod->p_current_view->p_view_from->info.media->med_type
	    == INSTALLED_SVC)
		env_is_service = 1;

	/*
	 *  for each locale of the product
	 *     if this localization package matches the locale
	 *	break, found right locale
	 *  if locale is unselected
	 *      deselect localization package
	 *  else if localization package doesn't localize any package
	 *      select localization package
	 *  else
	 *      for each package on pkglist
	 *	 if package is selected
	 *	      mark localization package selected
	 */
	for (lp = prod->info.prod->p_locale; lp; lp = lp->next) {
		if (strcmp(lp->info.locale->l_locale, l10n->m_locale) == 0)
			break;
	}

	/* package matches unselected locale - mark it deselected */
	if (lp == NULL || !lp->info.locale->l_selected) {
		l10n->m_status = UNSELECTED;
		l10n->m_refcnt = 0;
	/*
	 * generic package associated with a selected locale - mark it
	 * SELECTED and increment its reference count
	 */
	} else if (!l10n->m_l10n_pkglist
			|| (strcmp(l10n->m_l10n_pkglist, "") == 0)) {
		l10n->m_status = SELECTED;

		if (l10n->m_action == NO_ACTION_DEFINED) {
			if (env_is_service && l10n->m_sunw_ptype == PTYPE_ROOT)
				l10n->m_action = TO_BE_SPOOLED;
			else
				l10n->m_action = TO_BE_PKGADDED;
		}

		l10n->m_refcnt += 1;
	/*
	 * non-generic selected package...also select the packages impacted
	 * by it
	 */
	} else {
		l10n->m_status = UNSELECTED;
		l10n->m_refcnt = 0;
		buf = (char *)xstrdup(l10n->m_l10n_pkglist);
		pkgid = buf;
		do
		{
			/*
			 * break pkglist into components:
			 *	pkg1:version, pkg2:version..., pkgn:version
			 *	^    ^      ^
			 * pkgid__|    |      |
			 * version_____|      |
			 * delim______________|
			 */
			if (delim = strchr(pkgid, ','))
				*delim = '\0';

			if (version = strchr(pkgid, ':')) {
				*version = '\0';
				++version;
				if (delim) {
					tmp = delim;
					++delim;
					if (strncmp(delim, "REV=", 4) == 0) {
						*tmp = ',';
						if (delim = strchr(++tmp, ','))
							*delim = '\0';
					} else
						delim = tmp;
				}

			}

			/* find package specified */
			if ((tn = findnode(prod->info.prod->p_packages, pkgid))
							!= (Node *) NULL) {
				if (version == (char *) NULL || (version &&
						(strcmp(version, ((Modinfo *)
						tn->data)->m_version) == 0))) {
					if (((Modinfo*)tn->data)->m_status ==
								SELECTED ||
					   ((Modinfo*)tn->data)->m_status ==
								REQUIRED) {
						l10n->m_status = SELECTED;

						if (l10n->m_action ==
						    NO_ACTION_DEFINED) {
							if (env_is_service &&
							  l10n->m_sunw_ptype ==
							  PTYPE_ROOT)
								l10n->m_action =
								  TO_BE_SPOOLED;
							else
								l10n->m_action =
								 TO_BE_PKGADDED;
						}

						l10n->m_refcnt += 1;
					}
				}
			}

			if (delim) {			/* another token? */
				/* skip any leading white space */
				for (++delim; delim && *delim &&
					   ((*delim == ' ') | (*delim == '\t'));
					      ++delim);
				pkgid = delim;
			}
		} while (delim);

		free(buf);
	}
	return (SUCCESS);
}

/*
 * select_partial_locale()
 *	Select the partial locale, and all base locales required by the
 *	partial locale.  If the selection of any of the base locales
 *	succeed, or if one of the base locales is "C", return SUCCESS (even
 *	if the selection of the partial locale failed, or if the selection
 *	of any of the base locales failed.)  If there are no base locales
 *	associated with the partial locale, return the result of selecting
 *	the partial locale.
 * Parameters:
 *	mod	- product module pointer (must be type PRODUCT or NULLPRODUCT)
 *	locale	- name of locale to be SELECTED
 * Return:
 *	ERR_BADLOCALE	- 'locale' can't be selected.
 *	SUCCESS		- locale structure of type 'locale' set successfully
 * Status:
 *	public
 */
static int
select_partial_locale(Module * mod, char * locale)
{
	int	part_status, base_status, base_locale_selected;
	StringList *str;
	LocMap	*lmap;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("select_partial_locale");
#endif

	part_status = select_base_locale(mod, locale);

	/* defensive programming */
	if (!mod->parent || !mod->parent->info.media)
		return (part_status);

	for (lmap = mod->parent->info.media->med_locmap; lmap != NULL;
	    lmap = lmap->next) {
		if (strcmp(lmap->locmap_partial, locale) != 0)
			continue;
		if (lmap->locmap_base == NULL)
			break;
		base_locale_selected = 0;
		for (str = lmap->locmap_base; str; str = str->next) {
			if (strcmp(str->string_ptr, "C") == 0) {
				base_locale_selected = 1;
				continue;
			}
			base_status = select_base_locale(mod, str->string_ptr);
			if (base_status == SUCCESS)
				base_locale_selected = 1;
		}
		if (base_locale_selected)
			return (SUCCESS);
		else
			return (ERR_BADLOCALE);
	}

	return (part_status);
}

/*
 * select_base_locale()
 *	Scan the 'mod' product locale list for a member which matches 'locale'.
 *	If found, set the status of that locale structure to "SELECTED" and
 *	return.
 * Parameters:
 *	mod	- product module pointer (must be type PRODUCT or NULLPRODUCT)
 *	locale	- name of locale to be SELECTED
 * Return:
 *	ERR_INVALIDTYPE	- 'mod' is neither a PRODUCT or NULLPRODUCT
 *	ERR_BADLOCALE	- 'locale' is not part of the locale chain for 'mod'
 *	SUCCESS		- locale structure of type 'locale' set successfully
 * Status:
 *	public
 */
static int
select_base_locale(Module * mod, char * locale)
{
	Module	*m;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("select_base_locale");
#endif

	for (m = mod->info.prod->p_locale; m != NULL; m = m->next) {
		if (strcmp(locale, m->info.locale->l_locale) == 0) {
			m->info.locale->l_selected = SELECTED;
			return (SUCCESS);
		}
	}

	return (ERR_BADLOCALE);
}
