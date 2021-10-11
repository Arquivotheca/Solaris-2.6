#ifndef lint
#pragma ident "@(#)soft_locale.c 1.4 96/06/25 SMI"
#endif
/*
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#include "spmisoft_lib.h"
#include <string.h>
#include <locale.h>
#include <stdlib.h>

/* Local Globals */

struct locmap	*global_locmap = NULL;	/* List of localization maps */

/* Local Statics and Constants */

static struct loclang {
	char	   *locale;
	char	   *language;
};

/* Public Function Prototype */

Module * 	swi_get_all_locales(void);
int      	swi_select_locale(Module *, char *);
int      	swi_deselect_locale(Module *, char *);
int		swi_valid_locale(Module *, char *);

/* Library Function Prototype */

void     	sync_l10n(Module *);
void     	localize_packages(Module *);
void     	sort_locales(Module *);
char		*get_lang_from_loc_array(char *);
char		*get_C_lang_from_locale(char *);
int     	add_locale_list(Module *, StringList *);

/* Local Function Prototype */

static int	resolve_package_l10n(Node *, caddr_t);
static char	*get_lang_from_locale(char *);
static int     	select_partial_locale(Module *, char *);
static int     	select_base_locale(Module *, char *);
static int     	add_locale(Module *, char *);
static int	has_a_selected_locale(Modinfo *, Module *);

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
			sync_l10n(mod);
			return (SUCCESS);
		}
	}

	return (ERR_BADLOCALE);
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
	if (get_C_lang_from_locale(locale) == NULL)
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
				(void) strncpy(locstring, locale, cp - locale);
				locstring[cp - locale] = '\0';
			} else
				(void) strcpy(locstring, locale);
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
 * sync_l10n()
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
sync_l10n(Module * prod)
{
	Module	*ml, *mp;
	PkgsLocalized *pkgloc;

#ifdef SW_LIB_LOGGING
	sw_lib_log_hook("sync_l10n");
#endif

	if (prod == NULL)
		return;

	for (ml = prod->info.prod->p_locale; ml; ml = ml->next) {
		for (mp = ml->sub; mp; mp = mp->next) {
			if (!has_a_selected_locale(mp->info.mod, prod)) {
				mp->info.mod->m_status = UNSELECTED;
				continue;
			}
			if (mp->info.mod->m_pkgs_lclzd == NULL) {
				mp->info.mod->m_status = SELECTED;
				continue;
			}
			for (pkgloc = mp->info.mod->m_pkgs_lclzd; pkgloc;
			    pkgloc = pkgloc->next)
				if (pkgloc->pkg_lclzd->m_status == SELECTED ||
				    pkgloc->pkg_lclzd->m_status == REQUIRED)
					break;
			if (pkgloc)
				mp->info.mod->m_status = SELECTED;
			else
				mp->info.mod->m_status = UNSELECTED;
		}
	}
}

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
 *	Walk the product package list to (1) build the list of "packages
 *	that localize this packages" for each package, and (2) for
 *	each locale in the product's p_locale list, build the list
 *	of l10n packages for that locale.  This function is called
 *	when building a product structure that describes either
 *	an installable media, or an installed product.
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
 * add_locale_list()
 *	Add the locales from the list into the locale list for the
 *	specified product.
 * Parameters:
 *	prod	- non-NULL Product structure pointer
 *	loc_str_list	- a StringList of locales.
 * Return:
 *	ERR_INVALIDTYPE	- invalid product type
 *	ERR_INVALID	- invalid locale
 *	SUCCESS		- a valid locale structure was created
 *			  and added to the locale chain
 * Status:
 *	semi-private (internal library use only)
 */
int
add_locale_list(Module * prod, StringList * loc_str_list)
{
	int	stat;

	while (loc_str_list) {
		stat = add_locale(prod, loc_str_list->string_ptr);
		if (stat != SUCCESS)
			return (stat);
		loc_str_list = loc_str_list->next;
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
 *	Walklist() function used to (1) build the list of "packages
 *	that localize this packages" for each package, and (2) for
 *	each locale in the product's p_locale list, build the list
 *	of l10n packages for that locale.  This function is called
 *	when building a product structure that describes either
 *	an installable media, or an installed product.
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
	Node	*tn;
	char	*tmp;
	Module	*prod;
	Module	*mp, *mp1, *mp2;
	L10N	*lp;
	Modinfo	*mi, *milp;
	PkgsLocalized	*loclzd_pkg;

	/* use current product if data == NULL */
	if (data == NULL)
		prod = get_current_product();
	else
		/*LINTED [alignment ok]*/
		prod = (Module *)data;
	if (prod == NULL)
		return (ERR_NOPROD);

	mi = (Modinfo *)np->data;

	/*
	 * if this package is a localization package (has locale and list of
	 * affected packages that is non-NULL)
	 */
	if (mi->m_loc_strlist && mi->m_l10n_pkglist &&
	    (*(mi->m_l10n_pkglist))) {
		buf = (char *)xstrdup(mi->m_l10n_pkglist);
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

				milp = (Modinfo *)tn->data;
				/*
		 		 * point package milp at package mi if no
				 * version is specified or if the version
				 * specified exactly matches milp's version
				 */
				if (version == (char *) NULL ||
				    streq(version, milp->m_version)) {
					lp = (L10N *) xcalloc(sizeof (L10N));
					lp->l10n_package = mi;
					lp->l10n_next = milp->m_l10n;
					milp->m_l10n = lp;
	
					loclzd_pkg = (PkgsLocalized *)
					    xcalloc((size_t) sizeof
					    (PkgsLocalized));
					loclzd_pkg->pkg_lclzd = milp;
					link_to((Item **)&mi->m_pkgs_lclzd,
					    (Item *)loclzd_pkg);
				}
			}

			if (delim) {			/* another token? */
				/* skip any leading white space */
				for (++delim; delim && *delim &&
				    ((*delim == ' ') || (*delim == '\t'));
				    ++delim);
					pkgid = delim;
			}
		} while (delim);

		free(buf);
	}
	/* Add to locale tree */
	if (mi->m_loc_strlist) {
		for (mp=prod->info.prod->p_locale; mp; mp = mp->next) {
			if (StringListFind(mi->m_loc_strlist,
					mp->info.locale->l_locale) == NULL)
				continue;
			mp2 = NULL;
			for (mp1=mp->sub; mp1; mp1 = mp1->next) {
				if (strcmp(mp1->info.mod->m_pkgid,
						mi->m_pkgid) == NULL)
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
			mp1->info.mod = mi;
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
get_lang_from_locale(char * locale)
{
	LocMap	*lmap;

	if ((lmap = global_locmap) != NULL) {
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
get_C_lang_from_locale(char * locale)
{
	LocMap	*lmap;

	if ((lmap = global_locmap) != NULL) {
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

	for (lmap = global_locmap; lmap != NULL; lmap = lmap->next) {
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
			sync_l10n(mod);
			return (SUCCESS);
		}
	}

	return (ERR_BADLOCALE);
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
 *	static
 */

static int
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

	if ((cp = get_lang_from_locale(locale)) == (char *)NULL
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
 * has_a_selected_locale()
 *	Determine whether at least one of the locales supplied by
 *	specified localization package is selected.
 * Parameters:
 *	modinfo - pointer to a modinfo struct for a package.
 *	prod - product in which the locales are defined.
 * Return:
 *	0 - none of the locales are selected.
 *	1 - at least one of the selected locales are selected.
 * Status:
 *	static
 */
static int
has_a_selected_locale(Modinfo *modinfo, Module *prod)
{
	StringList	*str;
	Module		*loc;

	for (str = modinfo->m_loc_strlist; str; str = str->next) {
		for (loc = prod->info.prod->p_locale; loc; loc = loc->next)
			if (streq(str->string_ptr, loc->info.locale->l_locale))
				break;
		if (loc && loc->info.locale->l_selected)
			return (1);
	}
	return (0);
}
