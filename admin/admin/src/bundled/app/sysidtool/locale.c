/*
 *  Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */
/*
 * Module:	locale.c
 * Group:	sysid
 * Description:	This file contains the routines needed to prompt
 *		the user for the desired locale.
 */

#pragma	ident	"@(#)locale.c 1.29 96/10/17"

#include <stdio.h>
#include <locale.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "sysidtool.h"

/*
 * Finding one of these locales on an image means that the image has
 * multi-byte locales, which we can't support in the CUI.
 * We also have to play games with the UI to insure it gets the right
 * font set in the GUI.
 */
char *mb_locales[] = {"ja", "ja_JP.PCK", "ko", "zh", "zh_TW", NULL};

void save_locale(char *);

/* value domain buffer gets realloced in chunks of this */
#define	LOCALE_MENU_SIZE	20
#define	LANG_MENU_SIZE		10

#define	STR_LANG	"LANG="
#define	LEN_LANG	(sizeof (STR_LANG) - 1)
#define	STR_LC_COLLATE	"LC_COLLATE="
#define	LEN_LC_COLLATE	(sizeof (STR_LC_COLLATE) - 1)
#define	STR_LC_CTYPE	"LC_CTYPE="
#define	LEN_LC_CTYPE	(sizeof (STR_LC_CTYPE) - 1)
#define	STR_LC_MESSAGES	"LC_MESSAGES="
#define	LEN_LC_MESSAGES	(sizeof (STR_LC_MESSAGES) - 1)
#define	STR_LC_MONETARY	"LC_MONETARY="
#define	LEN_LC_MONETARY	(sizeof (STR_LC_MONETARY) - 1)
#define	STR_LC_NUMERIC	"LC_NUMERIC="
#define	LEN_LC_NUMERIC	(sizeof (STR_LC_NUMERIC) - 1)
#define	STR_LC_TIME	"LC_TIME="
#define	LEN_LC_TIME	(sizeof (STR_LC_TIME) - 1)

/* where the localizations live */
#define	NLSPATH		"/usr/lib/locale"
static char *nlspath = NLSPATH;

/* Internal Data structure to store lang/locale information */
typedef struct locale_ll {
	char *lang;
	int  n_locales;
	char **locales;
	struct locale_ll *next;
} LocaleList;

/* LOCAL Functions */
static void create_lang_entry(char *, char *);
static LocaleList *get_lang_entry(char *);
static void add_locale_entry_to_lang(LocaleList *, char *);
static int get_lang_name_from_map(char *, char *);
static void update_init(FILE *fp, char *locale);
static void set_lc(char *, char *, char *, char *, char *, char *);

/* Static variables used to store language/locale system information */
static int		n_langs = 0;		/* number of languages */
static LocaleList *ll_list = NULL;  /* lang/locale list */



/*
 * Function
 *		create_lang_entry
 *
 * Description
 *		Create a language/locale list node and link
 *		it into the lang/locale linked list.
 *
 * Scope
 *		Private
 *
 * Parameters
 *		lang - language to add
 *		locale - locale which uses lang
 *
 * Return
 *		none
 *
 */
static void
create_lang_entry(char *lang, char *locale)
{
	LocaleList *tmp_ll;

	if (ll_list) {
		tmp_ll = ll_list;
		ll_list = calloc(1, sizeof (LocaleList));
		ll_list->next = tmp_ll;
	} else {
		ll_list = calloc(1, sizeof (LocaleList));
		ll_list->next = NULL;
	}

	ll_list->lang = strdup(lang);
	ll_list->locales = calloc(LOCALE_MENU_SIZE, sizeof (char *));
	ll_list->locales[0] = strdup(locale);
	ll_list->n_locales = 1;

	n_langs++;
}


/*
 * Function
 *		get_lang_entry
 *
 * Description
 *		Get the language/locale list node which uses
 *		the specified language
 *
 * Scope
 *		Private
 *
 * Parameters
 *		lang - language to search for
 *
 * Return
 *		a pointer to the correct lang/locale node or NULL
 *
 */
static LocaleList *
get_lang_entry(char *lang_name)
{
	int i;
	LocaleList *list;

	for (i = 0, list = ll_list; list != NULL; i++) {
		if (strcmp(list->lang, lang_name) == 0)
			break;
		list = list->next;
	}

	return (list);
}


/*
 * Function
 *		add_locale_entry_to_lang
 *
 * Description
 *		Add an additional locale to the list of
 *		locales that use lang.
 *
 * Scope
 *		Private
 *
 * Parameters
 *		langp - pointer to the lang/locale node to add locale to
 *		locale - locale which uses lang
 *
 * Return
 *		none
 *
 */
static void
add_locale_entry_to_lang(LocaleList *langp, char *locale_name)
{
	int i;
	int n_locales;

	n_locales = langp->n_locales;
	for (i = 0; i < n_locales; i++) {
		if (strcmp(langp->locales[i], locale_name) == 0)
			return;
	}

	if (((n_locales+1) % LOCALE_MENU_SIZE) == 0) {
		n_locales += LOCALE_MENU_SIZE+1;
		langp->locales = (char **)realloc(langp->locales,
					n_locales * sizeof (char *));
	}

	langp->locales[langp->n_locales++] = strdup(locale_name);
}


/*
 * Function
 *		get_lang_name_from_map
 *
 * Description
 *		Read the locale_map file and return the value of
 *		either the LANG or the LC_MESSAGES variables.
 *
 * Scope
 *		Private
 *
 * Paramters
 *		locale_name - locale to use in finding the locale_map
 *		lang_name - character array to copy the value found
 *
 * Return
 *		1 - a language value was found and copied into lang_name
 *		0 - no language value found in locale_map
 *
 */
static int
get_lang_name_from_map(char *locale_name, char *lang_name)
{
	FILE *fp;
	char line[BUFSIZ];
	char path[MAXPATHLEN];
	int ret;

	ret = 0;
	(void) sprintf(path, "%s/%s/locale_map", nlspath, locale_name);
	if ((fp = fopen(path, "r")) != NULL) {
		while (fgets(line, BUFSIZ, fp) != NULL) {
			if (strncmp(STR_LANG, line, LEN_LANG) == 0) {
				ret = 1;
				line[strlen(line) - 1] = '\0';
				strncpy(lang_name, line + LEN_LANG, BUFSIZ-1);
			} else if (strncmp(STR_LC_MESSAGES, line,
				LEN_LC_MESSAGES) == 0) {
				line[strlen(line) - 1] = '\0';
				strncpy(lang_name, line + LEN_LC_MESSAGES,
					BUFSIZ-1);
				ret = 1;
			}
			if (ret > 0)
				break;
		}
		fclose(fp);
	}

	return (ret);
}

/*
 * Function
 *		get_num_locales
 *
 * Description
 *		Get the total number of locales on the booted system.
 *
 * Scope
 *		Public
 *
 * Parameters
 *			none
 *
 * Return
 *		the number of locales on the system
 *
 */
int
get_num_locales()
{
	int i;
	LocaleList *ll;
	int num_locales;

	for (i = 0, ll = ll_list, num_locales = 0; ll != NULL; i++) {
		num_locales += ll->n_locales;
		ll = ll->next;
	}
	return (num_locales);
}

/*
 * Function
 *		get_lang_strings
 *
 * Description
 *		Create an array of strings containing all of the
 *		languages available on the booted system.
 *
 * Scope
 *		Public
 *
 * Parameters
 *		langp - a pointer to an array of strings
 *			the values will be created and copied
 *			into langp unless it is passed in as NULL
 *
 * Return
 *		the number of languages on the system
 *
 */
int
get_lang_strings(char ***langp)
{
	int i;
	LocaleList *ll;

	if (langp != NULL) {
		*langp = calloc(n_langs, sizeof (char *));
		for (i = 0, ll = ll_list; ll != NULL; i++) {
			(*langp)[i] = strdup(ll->lang);
			ll = ll->next;
		}
	}
	return (n_langs);
}

/*
 * Function
 *		free_lang_strings
 *
 * Description
 *		Free whatever dynamic memory was allocated in
 *		the get_lang_strings function.
 *
 * Scope
 *		Public
 *
 * Parameters
 *		langp - pointer to an array of character strings
 *
 * Return
 *		none
 *
 */
void
free_lang_strings(char ***langp)
{
	int i;

	if (langp != NULL) {
		for (i = 0; i < n_langs; i++)
			free((*langp)[i]);

		free(*langp);
		*langp = NULL;
	}
}



/*
 * Function
 *		get_lang_locale_strings
 *
 * Description
 *		Create an array of strings containing all of the
 *		locales which use lang available on the booted system.
 *
 * Scope
 *		Public
 *
 * Parameters
 *		lang - the lang to search for
 *		localep - a pointer to an array of strings
 *			the values will be created and copied
 *			into localep unless it is passed in as NULL
 *
 * Return
 *		the number of locales which use lang on the system
 *
 */
int
get_lang_locale_strings(char *lang, char ***localep)
{
	LocaleList *ll;

	ll = get_lang_entry(lang);
	/*
	 * if get_lang_entry returns NULL
	 * (this should never happen) then
	 * send back 0 locales found
	 */
	if (ll == NULL) {
		if (localep != NULL)
			*localep = NULL;
		return (0);
	}
	if (localep != NULL) {
		*localep = ll->locales;
	}
	return (ll->n_locales);
}

/*
 * Function
 *		free_lang_locale_strings
 *
 * Description
 *		Free whatever dynamic memory was allocated in
 *		the get_lang_locale_strings function.
 *		Currently, there is none so it simply returns.
 *
 * Scope
 *		Public
 *
 * Parameters
 *		localep - pointer to an array of character strings
 *
 * Return
 *		none
 *
 */
void
free_lang_locale_strings(char ***localep)
{
}


/*
 * do_locale:
 *	returns 1 if locale should be determined, 0 if no need
 *
 *	The idea is to scan the directories under /usr/lib/locale
 *	for locales and categories that are in the proper arrangement,
 *	and create a menu of available locales for user selection.
 *	Only create the menu if non-default locale is found.
 *	The menu is used in case the user must be prompted.
 *	This should be called before prompt_locale().
 */
int
do_locale()
{
	DIR		*locale_dir;		/* /usr/lib/locale */
	struct dirent	*locale;		/* entries in locale_dir */
	char		path[MAXPATHLEN];
	char		*lang_name;
	char		*locale_name;
	char 		lang[BUFSIZ];
	struct stat	buf;
	LocaleList *langp;

	/* we always have at least 1 lang/locale, the default (english) */
	create_lang_entry("C", "C");

	/*
	 * Loop thru all entries in the locale directory,
	 * checking for directories which contain locale
	 * subdirectories.  If there is at least one, then
	 * do the locale menu.
	 */
	locale_dir = opendir(nlspath);
	while (locale_dir && (locale = readdir(locale_dir))) {

		/* Obviously, exclude the current/parent directories */
		if (strcmp(locale->d_name, ".") == 0 ||
		    strcmp(locale->d_name, "..") == 0)
			continue;

		/* exclude the default locale */
		if (strcmp(locale->d_name, "C") == 0)
			continue;

		/* Is this entry a directory? */
		(void) sprintf(path, "%s/%s", nlspath, locale->d_name);
		if (lstat(path, &buf) || !S_ISDIR(buf.st_mode))
			continue;

		/* Is this entry a l10n directory? */
		(void) sprintf(path, "%s/%s/LC_MESSAGES", nlspath,
			locale->d_name);
		if (stat(path, &buf) || !S_ISDIR(buf.st_mode))
			continue;

		/* Does this l10n directory include l10n's for sysidtool? */
		(void) sprintf(path, "%s/%s/LC_MESSAGES/%s.mo", nlspath,
			locale->d_name, TEXT_DOMAIN);
		if (!stat(path, &buf) && S_ISREG(buf.st_mode)) {
			lang_name = locale->d_name;
			locale_name = locale->d_name;
		} else {

			/* check for a partial locale */
			(void) sprintf(path, "%s/%s/locale_description",
				nlspath, locale->d_name);
			if (stat(path, &buf) || !S_ISREG(buf.st_mode))
				continue;
			locale_name = locale->d_name;
			lang_name = lang;
			get_lang_name_from_map(locale_name, lang_name);
		}

		fprintf(debugfp, "do_locale: locale dir: %s\n", locale->d_name);

		/* we have an l10n, include in menu */

		/*
		 * Check to see if it already exists in the lang data structure
		 */
		langp = get_lang_entry(lang_name);

		/*
		 * if not found then create a new entry and add it to
		 * the lang data structure
		 */
		if (langp == NULL) {
			create_lang_entry(lang_name, locale_name);
		} else {
			add_locale_entry_to_lang(langp, locale_name);
		}
	}

	/* Close locale directory */
	if (locale_dir)
		(void) closedir(locale_dir);

	fprintf(debugfp, "do_locale: num languages: %d\n", n_langs);

	/* if no other l10n's, just use native messages */
	if (n_langs > 1) {
		return (1);
	} else{
		return (0);
	}
}

/*
 * do_locale_simulate:
 *	returns 1 if locale should be determined, 0 if no need
 *
 *  This function is used to allow simulation of the locale
 *  selection interfaces.  An array of character strings is
 *  passed in.  This is used to build the internal data structures.
 *	Only create the menu if non-default locale is found.
 *	The menu is used in case the user must be prompted.
 *	This should be called before prompt_locale().
 */
int
do_locale_simulate(int n_locales, char **localelist)
{
	int i;

	/* we always have at least 1 lang/locale, the default (english) */
	create_lang_entry("C", "C");

	for (i = 0; i < n_locales; i++)
		create_lang_entry(localelist[i], localelist[i]);

	if (n_langs > 1)
		return (1);

	return (0);
}

/*
 * Function:	get_l10n_string
 * Description:	Given the internal name for a locale, retrieve the English
 *		locale description (found in locale_description file), and
 *		the translation for that description using either the
 *		current locale (full locale), or the locale specified in the
 *		locale_map file for lc_messages. The return code reflects
 *		the degree of success in obtaining the necessary data.
 *		By design, if a value is set for 'desc', a value is
 *		guaranteed to be set for 'trans'.
 * Scope:	private
 * Parameters:	is_lang	[RO]
 *			Flag used to determine if this function has been called
 *			for the creation of the language list so that special
 *			processing can be done for the 'English' language.  The value
 *			of the flag is either 1 (this is the language list) or
 *			0 (this is the locale list)
 *		locale	[RO, *RO]
 *			Pointer to a string containing the internal name of
 *			a full or partial locale.
 *		trans	[RO, *RW]
 *			Preallocated buffer used to retrieve the translated
 *			locale description when one is available. Buffer must
 *			be of size 'len'.
 *		desc	[RO, *RW]
 *			Preallocated buffer used to retrieve the English
 *			locale description when one is available. Buffer must
 *			be of size 'len'.
 *		len	[RO]
 *			Specifier indicating buffer size for 'trans' and 'desc'
 * Return:	 0	there was a description, and there was a translation
 *			for that description; desc and trans are both set
 *			appropriately
 *		 1	there was a description, but there was no translation
 *			available/possible; desc and trans are both set to the
 *			English
 *			description
 *		-1	there was no description available; desc and trans are
 *			not set
 */
int
get_l10n_string(int is_lang, char *locale, char *trans, char *desc, int len)
{
	char		path[MAXPATHLEN];
	FILE		*fp;
	int		l;
	char		lang[MAX_LOCALE], lc_collate[MAX_LOCALE],
			lc_ctype[MAX_LOCALE], lc_messages[MAX_LOCALE],
			lc_monetary[MAX_LOCALE], lc_numeric[MAX_LOCALE],
			lc_time[MAX_LOCALE];
	char		current_locale[MAX_LOCALE];

	/* validate parameters */
	if (locale == NULL || trans == NULL || desc == NULL || len <= 0)
		return (-1);

	/* 'C' always is English with no translations */
	if (strcmp(locale, "C") == 0 && is_lang != 0) {
		(void) strcpy(desc, "English");
		(void) strcpy(trans, "English");
		return (1);
	}

	/* the locale_description file should exist for ALL locales */
	(void) sprintf(path, "%s/%s/locale_description", nlspath, locale);
	if ((fp = fopen(path, "r")) == NULL)
		return (-1);

	/* the locale_description file should contain an English description */
	if (fgets(desc, len, fp) == NULL) {
		(void) fclose(fp);
		return (-1);
	}

	(void) fclose(fp);

	/*
	 * strip off any extraneous newlines and initialize the translated
	 * value to be the description
	 */
	l = strlen(desc) - 1;
	if (desc[l] == '\n')
		desc[l] = 0;
	(void) strcpy(trans, desc);

	/*
	 * for partial locales, look up the translation in the mother
	 * language (if available); for full locales, use the current
	 * language
	 */
	(void) sprintf(path, "%s/%s/locale_map", nlspath, locale);
	if ((fp = fopen(path, "r")) == NULL) {
		(void) strcpy(trans, dgettext("SUNW_LOCALE_DESCR", desc));
	} else {
		(void) read_locale_file(fp, lang, lc_collate,
			lc_ctype, lc_messages, lc_monetary, lc_numeric,
			lc_time);
		(void) fclose(fp);
		if (strcmp(lc_messages, "C") != 0) {
			(void) strcpy(current_locale,
				setlocale(LC_MESSAGES, NULL));
			(void) setlocale(LC_MESSAGES, lc_messages);
			(void) strcpy(trans, dgettext("SUNW_LOCALE_DESCR",
				desc));
			(void) setlocale(LC_MESSAGES, current_locale);
		}
	}

	if (strcmp(desc, trans) == 0)
		return (1);

	return (0);
}

void
save_locale(char *locale)
{
	FILE *fp, *tfp;
	char line[BUFSIZ], tfile[80];

	fprintf(debugfp, "save_locale: %s\n", locale);

	(void) sprintf(tfile, "/tmp/sysidnet%ld", getpid());

	if ((tfp = fopen(tfile, "w")) == NULL)
		return;

	if ((fp = fopen("/etc/default/init", "r")) != NULL) {
		while (fgets(line, BUFSIZ, fp) != NULL) {
			if (strncmp("LANG=", line, 5) == 0)
				continue;
			if (strncmp("LC_", line, 3) == 0)
				continue;

			if (fputs(line, tfp) == EOF) {
				(void) fclose(fp);
				(void) fclose(tfp);
				return;
			}
		}
	}

	(void) fclose(fp);

	update_init(tfp, locale);

	(void) fclose(tfp);

	if ((fp = fopen("/etc/default/init", "w")) == NULL)
		return;

	if ((tfp = fopen(tfile, "r")) == NULL) {
		fclose(fp);
		return;
	}

	unlink(tfile);

	while (fgets(line, BUFSIZ, tfp) != NULL)
		if (fputs(line, fp) == EOF)
			break;

	(void) fclose(fp);
	(void) fclose(tfp);
}

static void
update_init(FILE *fp, char *locale)
{
	FILE *mfp;
	char path[MAXPATHLEN], lc_collate[MAX_LOCALE], lc_ctype[MAX_LOCALE],
		lc_messages[MAX_LOCALE], lc_monetary[MAX_LOCALE],
		lc_numeric[MAX_LOCALE], lc_time[MAX_LOCALE], lang[MAX_LOCALE];

	fprintf(debugfp, "update_init: %s\n", locale);

	(void) sprintf(path, "%s/%s/locale_map", nlspath, locale);
	if ((mfp = fopen(path, "r")) == NULL) {
		if (strcmp(locale, "C") != 0)
			fprintf(fp, "LANG=%s\n", locale);

		set_lang(locale);
	} else {
		(void) read_locale_file(mfp, lang, lc_collate, lc_ctype,
			lc_messages, lc_monetary, lc_numeric, lc_time);
		fclose(mfp);

		fprintf(fp, "LC_COLLATE=%s\n", lc_collate);
		fprintf(fp, "LC_CTYPE=%s\n", lc_ctype);
		fprintf(fp, "LC_MESSAGES=%s\n", lc_messages);
		fprintf(fp, "LC_MONETARY=%s\n", lc_monetary);
		fprintf(fp, "LC_NUMERIC=%s\n", lc_numeric);
		fprintf(fp, "LC_TIME=%s\n", lc_time);

		set_lc(lc_collate, lc_ctype, lc_messages, lc_monetary,
			lc_numeric, lc_time);
	}
}

int
lookup_locale()
{
	int status = 0;
	FILE *fp;
	char locale[50];
	char lc_collate[25], lc_ctype[25], lc_messages[25], lc_monetary[25],
		lc_numeric[25], lc_time[25];

	locale[0] = '\0';

	if ((fp = fopen("/etc/default/init", "r")) != NULL) {
		status = read_locale_file(fp, locale, lc_collate, lc_ctype,
			lc_messages, lc_monetary, lc_numeric, lc_time);
		(void) fclose(fp);
	}

	fprintf(debugfp, "lookup_locale: %d %s\n", status, locale);

	if (status == 1)
		set_lang(locale);
	else if (status == 2)
		set_lc(lc_collate, lc_ctype, lc_messages, lc_monetary,
			lc_numeric, lc_time);

	return (status);
}

int
read_locale_file(FILE *fp, char *lang, char *lc_collate, char *lc_ctype,
	char *lc_messages, char *lc_monetary, char *lc_numeric, char *lc_time)
{
	int status = 0;
	char line[BUFSIZ];

	strcpy(lc_collate, "C");
	strcpy(lc_ctype, "C");
	strcpy(lc_messages, "C");
	strcpy(lc_monetary, "C");
	strcpy(lc_numeric, "C");
	strcpy(lc_time, "C");

	while (fgets(line, BUFSIZ, fp) != NULL) {
		if (strncmp(STR_LANG, line, LEN_LANG) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lang, line + LEN_LANG);
			status = 1;
		} else if (strncmp(STR_LC_COLLATE, line, LEN_LC_COLLATE) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_collate, line + LEN_LC_COLLATE);
			status = 2;
		} else if (strncmp(STR_LC_CTYPE, line, LEN_LC_CTYPE) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_ctype, line + LEN_LC_CTYPE);
			status = 2;
		} else if (strncmp(STR_LC_MESSAGES, line, LEN_LC_MESSAGES)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_messages, line + LEN_LC_MESSAGES);
			status = 2;
		} else if (strncmp(STR_LC_MONETARY, line, LEN_LC_MONETARY)
		    == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_monetary, line + LEN_LC_MONETARY);
			status = 2;
		} else if (strncmp(STR_LC_NUMERIC, line, LEN_LC_NUMERIC) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_numeric, line + LEN_LC_NUMERIC);
			status = 2;
		} else if (strncmp(STR_LC_TIME, line, LEN_LC_TIME) == 0) {
			line[strlen(line) - 1] = '\0';
			strcpy(lc_time, line + LEN_LC_TIME);
			status = 2;
		}
	}

	return (status);
}

void
set_lang(char *locale)
{
	static char	tmpstr[MAX_LOCALE + 6];

	fprintf(debugfp, "set_lang %s\n", locale);

	(void) setlocale(LC_ALL, locale);
	(void) sprintf(tmpstr, "LANG=%s", locale);
	(void) putenv(tmpstr);
}

static void
set_lc(char *lc_collate, char *lc_ctype, char *lc_messages, char *lc_monetary,
	char *lc_numeric, char *lc_time)
{
	static char	tmpstr1[MAX_LOCALE + 15];
	static char	tmpstr2[MAX_LOCALE + 15];
	static char	tmpstr3[MAX_LOCALE + 15];
	static char	tmpstr4[MAX_LOCALE + 15];
	static char	tmpstr5[MAX_LOCALE + 15];
	static char	tmpstr6[MAX_LOCALE + 15];

	fprintf(debugfp, "set_lc %s %s %s %s %s %s\n", lc_collate, lc_ctype,
		lc_messages, lc_monetary, lc_numeric, lc_time);

	(void) setlocale(LC_COLLATE, lc_collate);
	(void) sprintf(tmpstr1, "LC_COLLATE=%s", lc_collate);
	(void) putenv(tmpstr1);

	(void) setlocale(LC_CTYPE, lc_ctype);
	(void) sprintf(tmpstr2, "LC_CTYPE=%s", lc_ctype);
	(void) putenv(tmpstr2);

	(void) setlocale(LC_MESSAGES, lc_messages);
	(void) sprintf(tmpstr3, "LC_MESSAGES=%s", lc_messages);
	(void) putenv(tmpstr3);

	(void) setlocale(LC_MONETARY, lc_monetary);
	(void) sprintf(tmpstr4, "LC_MONETARY=%s", lc_monetary);
	(void) putenv(tmpstr4);

	(void) setlocale(LC_NUMERIC, lc_numeric);
	(void) sprintf(tmpstr5, "LC_NUMERIC=%s", lc_numeric);
	(void) putenv(tmpstr5);

	(void) setlocale(LC_TIME, lc_time);
	(void) sprintf(tmpstr6, "LC_TIME=%s", lc_time);
	(void) putenv(tmpstr6);
}

#ifdef TEST


/*
 * The following is a driver that tests that the
 * lang/locale data structure built by do_locale
 * is built correctly.
 *
 * If no command line parameters are called then
 * the current system is examined.  If there is
 * a command line parameter it is used instead of
 * /usr/lib/locale.
 *
 */
FILE *debugfp;

main(int argc, char **argv)
{
	char **langp;
	char **localep;
	int	 i;
	int	 j;
	int  num_langs;
	int  num_locales;

	if (argc > 1)
		nlspath = argv[1];

	debugfp = stderr;
	do_locale();
	num_langs = get_lang_strings(&langp);
	for (i = 0; i < num_langs; i++) {
		fprintf(stderr, "LANG - %s\n", langp[i]);
		num_locales = get_lang_locale_strings(langp[i], &localep);
		for (j = 0; j < num_locales; j++) {
			fprintf(stderr, "     LOCALE(%d)  %s\n", j, localep[j]);
		}
		free_lang_locale_strings(&localep);
	}
	free_lang_strings(&langp);
}

#endif /* TEST */
